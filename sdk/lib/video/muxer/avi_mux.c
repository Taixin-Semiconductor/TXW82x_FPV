#include "basic_include.h"
#include "avi_mux.h"
#include "fatfs/osal_file.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "stream_define.h"

const uint8_t avi_zero = 0;

#ifndef SEEK_SET
#define SEEK_SET 0 /* set file offset to offset */
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1 /* set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define SEEK_END 2 /* set file offset to EOF plus offset */
#endif

static void pre_avi_seek(F_FILE *fp, uint32_t offset)
{
    uint32_t filesize  = osal_fsize(fp);
    if(filesize != offset)
    {
        osal_fseek(fp, offset);
        osal_ftruncate(fp);
        _os_printf("avi size: %d\n", osal_fsize(fp));
    }
}

static uint32_t avi_seek(F_FILE *fp, int32_t offset, int seek_mode)
{
    uint32_t fp_offset = 0;
    uint32_t filesize  = osal_fsize(fp);
    if (seek_mode == SEEK_SET)
    {
        fp_offset = offset;
    }
    else if (seek_mode == SEEK_CUR)
    {
        fp_offset = osal_ftell(fp) + offset;
    }
    else if (seek_mode == SEEK_END)
    {
        fp_offset = osal_fsize(fp) + offset;
    }
    osal_fseek(fp, fp_offset);
    if (filesize < fp_offset)
    {
        osal_ftruncate(fp);
    }
    return offset;
}

static uint32_t avi_write(void *buf, uint32_t size, uint32_t n, F_FILE *fp)
{
    // 返回值0是代表异常
    uint32_t ret              = 1;
    uint32_t write_size_total = size * n;
    ret                       = osal_fwrite(buf, 1, write_size_total, fp);
    return ret != write_size_total;
}

static void avi_close(F_FILE *fp)
{
    osal_fclose(fp);
}

static int av_write(void *file, const void *buf, uint32_t len)
{
    F_FILE *fp = (F_FILE *) file;

    if (!fp || (!buf && len))
    {
        return -1;
    }

    return avi_write((void *) buf, 1, len, fp) ? -1 : 0;
}

static int avi_skip(void *file, uint32_t len)
{
    F_FILE *fp = (F_FILE *) file;
    uint32_t offset;

    if (!fp)
    {
        return -1;
    }

    offset = osal_ftell(fp) + len;
    osal_fseek(fp, offset);
    if (osal_fsize(fp) < offset)
    {
        osal_ftruncate(fp);
    }
    return 0;
}

static uint32_t avi_tell(void *file)
{
    F_FILE *fp = (F_FILE *) file;
    return fp ? osal_ftell(fp) : 0;
}

static int avi_write_at(void *file, uint32_t offset, const void *buf, uint32_t len)
{
    F_FILE *fp = (F_FILE *) file;
    uint32_t restore_offset;

    if (!fp || (!buf && len))
    {
        return -1;
    }

    restore_offset = osal_ftell(fp);
    osal_fseek(fp, offset);
    if (avi_write((void *) buf, 1, len, fp))
    {
        return -1;
    }
    osal_fseek(fp, restore_offset);
    return 0;
}

static int avi_flush(void *file)
{
    F_FILE *fp = (F_FILE *) file;

    if (!fp)
    {
        return -1;
    }

    return osal_fsync(fp) == 0 ? 0 : -1;
}

static int avi_finish(void *file)
{
    return avi_flush(file);
}

static int avi_tail(void *file, const void *tail, uint32_t tail_len,
                                uint32_t logical_end, uint32_t reserved_end)
{
    F_FILE *fp = (F_FILE *) file;
    uint8_t zero[64] = {0};
    uint32_t remain;
    uint32_t restore_offset;

    if (!fp || (!tail && tail_len) || reserved_end < logical_end)
    {
        return -1;
    }

    remain = reserved_end - logical_end;
    if (tail_len > remain)
    {
        return -1;
    }

    restore_offset = osal_ftell(fp);
    osal_fseek(fp, logical_end);
    if (avi_write((void *) tail, 1, tail_len, fp))
    {
        return -1;
    }
    remain -= tail_len;
    while (remain)
    {
        uint32_t write_len = remain > sizeof(zero) ? sizeof(zero) : remain;
        if (avi_write(zero, 1, write_len, fp))
        {
            return -1;
        }
        remain -= write_len;
    }
    osal_fseek(fp, restore_offset);
    return 0;
}

static void avi_default_ops_init(F_FILE *fp, file_ops_t *ops)
{
    os_memset(ops, 0, sizeof(*ops));
    ops->file     = fp;
    ops->write    = av_write;
    ops->skip     = avi_skip;
    ops->tell     = avi_tell;
    ops->write_at = avi_write_at;
    ops->flush    = avi_flush;
    ops->finish   = avi_finish;
    ops->tail     = avi_tail;
}


// data s申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC os_malloc
#define STREAM_LIBC_FREE   os_free
#define STREAM_LIBC_ZALLOC os_zalloc


#define AVIF_HASINDEX      (1 << 4)
#define AVIF_ISINTERLEAVED (1 << 8)
#define AVIIF_KEYFRAME     (1 << 4)

#ifndef offsetof
#define offsetof(type, member) ((size_t) &((type *) 0)->member)
#endif

#define AVI_LIST_EMP (512)
#define IDX_MAX_SIZE (1024*1024)
#define AVI_IDX_ENTRY_SIZE (16U)
#define AVI_IDX_TMP_ENTRIES (AVI_LIST_EMP / sizeof(uint32_t))
#define AVI_FOURCC(a, b, c, d) (((uint32_t)(a) << 0) | ((uint32_t)(b) << 8) | \
                                ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

#pragma pack(1)
typedef struct
{
    uint32_t microsec_per_frame;
    uint32_t maxbytes_per_Sec;
    uint32_t padding_granularity;
    uint32_t flags;
    uint32_t total_frames;
    uint32_t initial_frames;
    uint32_t number_streams;
    uint32_t suggested_bufsize;
    uint32_t width;
    uint32_t height;
    uint32_t reserved[4];
} AVI_HEADER;

typedef struct
{
    char     fcc_type[4];
    char     fcc_codec[4];
    uint32_t flags;
    uint16_t priority;
    uint16_t language;
    uint32_t initial_frames;
    uint32_t scale;
    uint32_t rate;
    uint32_t start;
    uint32_t length;
    uint32_t suggested_bufsize;
    uint32_t quality;
    uint32_t sample_size;
    uint16_t vrect_left;
    uint16_t vrect_top;
    uint16_t vrect_right;
    uint16_t vrect_bottom;

} STREAM_HEADER;

typedef struct
{
    uint16_t format_tag;
    uint16_t channels;
    uint32_t sample_per_sec;
    uint32_t avgbyte_per_sec;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint16_t size;
} WAVE_FORMAT;

typedef struct
{
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint16_t planes;
    uint16_t bitcount;
    uint32_t compression;
    uint32_t image_size;
    uint32_t xpels_per_meter;
    uint32_t ypels_per_meter;
    uint32_t color_used;
    uint32_t color_important;
} BITMAP_FORMAT;

typedef struct
{
    uint32_t    *framesize_lst;
    uint32_t    framesize_fix;
    uint32_t    framesize_idx;
    uint32_t    framesize_max;
    uint32_t    idx1_addr;
    uint32_t    idx1_size_addr;
    uint32_t    idx1_data_addr;
    uint32_t    idx1_end_addr;
    uint32_t    idx1_data_size;
    uint32_t    idx1_write_addr;
    uint32_t    total_idx;
    uint32_t    tmp_idx_offset;
    uint8_t     tmp_idx_data[AVI_IDX_TMP_ENTRIES * AVI_IDX_ENTRY_SIZE];
    
    file_ops_t  ops;
    uint8_t     ops_init;
    uint32_t    movi_addr;
    uint32_t    movi_data_addr;
    uint32_t    movi_addr_end;
    uint32_t    movi_data_size;
    uint32_t    cur_addr;
    uint32_t    movi_guard_block_end;
    uint32_t    last_video_offset;
    uint32_t    last_video_size;
    uint32_t    last_video_flags;
    
    F_FILE      *fp;
    uint32_t    extern_fp;
    uint32_t    syn_time;
    uint32_t    *fast_seek_tbl;
    uint32_t    audio_enable;
    char        video_tag[4];
    uint32_t    riff_size_offset;
    uint32_t    total_frames_offset;
    uint32_t    audio_length_offset;
    uint32_t    video_length_offset;
    uint32_t    movi_size_offset;

    char        riff[4];
    uint32_t    riff_size;
    char        type_avi[4];

    char        hlist[4];
    uint32_t    hlist_size;
    char        type_hdrl[4];

    char        avih[4];
    uint32_t    avih_size;
    AVI_HEADER  avi_header;
#if 1
    char        slist1[4];
    uint32_t    slist1_size;
    char        type_str1[4];

    char        strhdr1[4];
    uint32_t    strhdr1_size;
    STREAM_HEADER strhdr_audio;

    char        strfmt1[4];
    uint32_t    strfmt1_size;
    WAVE_FORMAT strfmt_audio;
#endif

    char        slist2[4];
    uint32_t    slist2_size;
    char        type_str2[4];

    char        strhdr2[4];
    uint32_t    strhdr2_size;
    STREAM_HEADER strhdr_video;

    char        strfmt2[4];
    uint32_t    strfmt2_size;
    BITMAP_FORMAT strfmt_video;

    char        mlist[4];
    uint32_t    mlist_size;
    char        type_movi[4];

} AVI_FILE;
#pragma pack()

#if FF_USE_FASTSEEK
#define AVI_FAST_SEEK_CLMT_ITEMS_MIN 64U

static uint32_t *avi_fast_seek_create(F_FILE *fp)
{
    uint32_t *cltbl;
    uint32_t  table_items = AVI_FAST_SEEK_CLMT_ITEMS_MIN;
    FRESULT   res;

    if (!fp)
    {
        return NULL;
    }

    while (1)
    {
        cltbl = (uint32_t *) STREAM_MALLOC(table_items * sizeof(uint32_t));
        if (!cltbl)
        {
            fp->cltbl = NULL;
            os_printf(KERN_WARNING "avi fast seek alloc failed, items:%d\n", table_items);
            return NULL;
        }

        cltbl[0]  = table_items;
        fp->cltbl = (DWORD *) cltbl;
        res       = f_lseek(fp, CREATE_LINKMAP);
        if (res == FR_OK)
        {
            os_printf(KERN_INFO "avi fast seek enabled, items:%d\n", cltbl[0]);
            return cltbl;
        }

        if (res == FR_NOT_ENOUGH_CORE && cltbl[0] > table_items)
        {
            table_items = cltbl[0];
            fp->cltbl   = NULL;
            STREAM_FREE(cltbl);
            continue;
        }

        os_printf(KERN_WARNING "avi fast seek disabled, res:%d, items:%d\n", res, cltbl[0]);
        fp->cltbl = NULL;
        STREAM_FREE(cltbl);
        return NULL;
    }
}

static void avi_fast_seek_destroy(F_FILE *fp, uint32_t **cltbl)
{
    if (!cltbl || !*cltbl)
    {
        return;
    }

    if (fp)
    {
        fp->cltbl = NULL;
    }
    STREAM_FREE(*cltbl);
    *cltbl = NULL;
}
#else
static uint32_t *avi_fast_seek_create(F_FILE *fp)
{
    (void) fp;
    return NULL;
}

static void avi_fast_seek_destroy(F_FILE *fp, uint32_t **cltbl)
{
    (void) fp;
    (void) cltbl;
}
#endif

static uint32_t avi_write_idx_tmp(void *buf, uint32_t size, uint32_t n, AVI_FILE *avi)
{
    // 返回值0是代表异常
    uint32_t ret              = 0;
    uint32_t write_size_total = size * n;
    if (!avi || !buf || avi->tmp_idx_offset + write_size_total > sizeof(avi->tmp_idx_data))
    {
        return 1;
    }
    os_memcpy(avi->tmp_idx_data + avi->tmp_idx_offset, buf, write_size_total);
    avi->tmp_idx_offset += write_size_total;
    return ret;
}

static uint32_t avi_movi_write_data(AVI_FILE *avi, const void *buf, uint32_t len)
{
    if (!avi || !avi->ops_init || !avi->ops.write)
    {
        return 1;
    }

    return avi->ops.write(avi->ops.file, buf, len) ? 1 : 0;
}

static uint32_t avi_write_idx_syn(AVI_FILE *avi)
{
    uint32_t ret;

    if (!avi || !avi->fp || avi->tmp_idx_offset == 0)
    {
        return 0;
    }

    if (avi->idx1_write_addr + avi->tmp_idx_offset > avi->idx1_end_addr)
    {
        return 1;
    }

    if (!avi->ops_init || !avi->ops.write_at)
    {
        return 1;
    }

    ret = avi->ops.write_at(avi->ops.file, avi->idx1_write_addr, avi->tmp_idx_data, avi->tmp_idx_offset) ? 1 : 0;
    if (!ret)
    {
        avi->idx1_write_addr += avi->tmp_idx_offset;
        avi->tmp_idx_offset = 0;
    }
    return ret;
}

static uint8_t avi_idx_entry_fits(AVI_FILE *avi)
{
    uint32_t remain;
    uint32_t next_offset;

    if (!avi || avi->idx1_write_addr >= avi->idx1_end_addr)
    {
        return 0;
    }

    next_offset = avi->idx1_write_addr + avi->tmp_idx_offset;
    if (next_offset >= avi->idx1_end_addr)
    {
        return 0;
    }

    remain = avi->idx1_end_addr - next_offset;
    if (remain < AVI_IDX_ENTRY_SIZE)
    {
        return 0;
    }

    remain -= AVI_IDX_ENTRY_SIZE;
    return (remain == 0 || remain >= 8);
}

static uint32_t avi_idx_write_entry(AVI_FILE *avi, const char *tag, uint32_t flags, uint32_t offset, uint32_t size)
{
    uint32_t ret = 0;

    if (!avi)
    {
        return 1;
    }

    if (avi->tmp_idx_offset + AVI_IDX_ENTRY_SIZE > sizeof(avi->tmp_idx_data))
    {
        if (avi_write_idx_syn(avi))
        {
            return 1;
        }
    }

    if (!avi_idx_entry_fits(avi))
    {
        return 1;
    }

    ret |= avi_write_idx_tmp((void *) tag, 4, 1, avi);
    ret |= avi_write_idx_tmp(&flags, sizeof(flags), 1, avi);
    ret |= avi_write_idx_tmp(&offset, sizeof(offset), 1, avi);
    ret |= avi_write_idx_tmp(&size, sizeof(size), 1, avi);
    if (!ret)
    {
        avi->idx1_data_size += AVI_IDX_ENTRY_SIZE;
    }

    return ret;
}

static uint32_t avimuxer_written_duration_ms(AVI_FILE *avi)
{
    uint32_t video_ms = 0;
    uint32_t audio_ms = 0;

    if (!avi)
    {
        return 0;
    }

    if (avi->avi_header.microsec_per_frame)
    {
        video_ms = (uint32_t) (((unsigned long long) avi->strhdr_video.length *
                                avi->avi_header.microsec_per_frame) /
                               1000);
    }

    if (avi->strhdr_audio.rate)
    {
        audio_ms = (uint32_t) (((unsigned long long) avi->strhdr_audio.length * 1000) /
                               avi->strhdr_audio.rate);
    }

    return video_ms > audio_ms ? video_ms : audio_ms;
}

static uint8_t avi_chunk_fits(AVI_FILE *avi, uint32_t chunk_size)
{
    uint32_t remain;

    if (!avi || avi->cur_addr >= avi->movi_addr_end)
    {
        return 0;
    }

    remain = avi->movi_addr_end - avi->cur_addr;
    if (chunk_size > remain)
    {
        return 0;
    }

    remain -= chunk_size;
    return (remain == 0 || remain >= 8);
}

static uint32_t avi_write_movi_snapshot_tail(AVI_FILE *avi);
static uint32_t avi_write_idx_snapshot_tail(AVI_FILE *avi);

static uint32_t avi_write_movi_chunk(AVI_FILE *avi, const char *tag, uint32_t flags,
                                     unsigned char *buf, uint32_t len,
                                     uint32_t *chunk_offset)
{
    uint32_t alignlen;
    uint32_t offset;

    if (!avi || !tag || !buf)
    {
        return AVIMUXER_ERR;
    }

    alignlen = (len & 1) ? len + 1 : len;
    offset   = avi->cur_addr - avi->movi_addr + 4;

    if (!avi_chunk_fits(avi, 8 + alignlen) || !avi_idx_entry_fits(avi))
    {
        return AVIMUXER_FULL;
    }

    if (avi_movi_write_data(avi, tag, 4) ||
        avi_movi_write_data(avi, &alignlen, 4) ||
        avi_movi_write_data(avi, buf, len) ||
        ((len & 1) && avi_movi_write_data(avi, (void *) &avi_zero, 1)))
    {
        return AVIMUXER_ERR;
    }

    if (avi_idx_write_entry(avi, tag, flags, offset, alignlen))
    {
        return AVIMUXER_ERR;
    }

    avi->cur_addr += 8 + alignlen;
    avi->total_idx++;
    if (chunk_offset)
    {
        *chunk_offset = offset;
    }

    return AVIMUXER_OK;
}

static uint32_t avi_movi_write_zero(AVI_FILE *avi, uint32_t len)
{
    static const uint8_t zero[64] = {0};

    if (!avi || !avi->ops_init || !avi->ops.write)
    {
        return 1;
    }

    while (len)
    {
        uint32_t write_len = len > sizeof(zero) ? sizeof(zero) : len;
        if (avi->ops.write(avi->ops.file, zero, write_len))
        {
            return 1;
        }
        len -= write_len;
    }

    return 0;
}

static uint32_t avi_movi_skip(AVI_FILE *avi, uint32_t len)
{
    if (!avi || !avi->ops_init || !avi->ops.skip)
    {
        return 1;
    }

    return avi->ops.skip(avi->ops.file, len) ? 1 : 0;
}

static uint32_t avi_write_movi_junk_chunk(AVI_FILE *avi, uint32_t chunk_size)
{
    uint32_t junk_size;

    if (!avi || chunk_size < 8)
    {
        return 1;
    }

    junk_size = chunk_size - 8;
    if (avi_movi_write_data(avi, "JUNK", 4) ||
        avi_movi_write_data(avi, &junk_size, 4) ||
        avi_movi_skip(avi, junk_size))
    {
        return 1;
    }

    avi->cur_addr += chunk_size;
    return 0;
}

static uint32_t avi_write_movi_snapshot_tail(AVI_FILE *avi)
{
    uint32_t remain;
    uint32_t junk_size;
    uint8_t  junk_hdr[8];

    if (!avi || !avi->ops_init || !avi->ops.tail || avi->cur_addr >= avi->movi_addr_end)
    {
        return 0;
    }

    if (avi->ops.tell && avi->ops.tell(avi->ops.file) != avi->cur_addr)
    {
        return 1;
    }

    remain = avi->movi_addr_end - avi->cur_addr;
    if (remain < 8)
    {
        return 1;
    }

    junk_size = remain - 8;
    os_memcpy(junk_hdr, "JUNK", 4);
    os_memcpy(junk_hdr + 4, &junk_size, 4);

    return avi->ops.tail(avi->ops.file, junk_hdr, sizeof(junk_hdr), avi->cur_addr, avi->movi_addr_end) ? 1 : 0;
}

static uint32_t avi_movi_guard_block_end(AVI_FILE *avi)
{
    uint32_t block_end;

    if (!avi)
    {
        return 0;
    }

    block_end = muxer_file_align_up(avi->cur_addr);
    if (block_end == avi->cur_addr)
    {
        block_end += MUXER_FILE_ALIGN_SIZE;
    }

    return block_end;
}

static uint32_t avi_write_movi_guard(AVI_FILE *avi)
{
    uint32_t cur_block_end;

    if (!avi || !avi->ops_init || !avi->ops.write_at || avi->cur_addr <= avi->movi_data_addr)
    {
        return 0;
    }

    cur_block_end = avi_movi_guard_block_end(avi);
    if (cur_block_end <= avi->movi_guard_block_end)
    {
        return 0;
    }

    if (avi_write_movi_snapshot_tail(avi))
    {
        return 1;
    }

    avi->movi_guard_block_end = cur_block_end;
    return 0;
}

static uint32_t avi_write_movi_tail_junk(AVI_FILE *avi)
{
    uint32_t remain;

    if (!avi || avi->cur_addr >= avi->movi_addr_end)
    {
        return 0;
    }

    remain = avi->movi_addr_end - avi->cur_addr;
    if (remain == 0)
    {
        return 0;
    }

    if (remain < 8)
    {
        if (avi_movi_write_zero(avi, remain))
        {
            return 1;
        }
        avi->cur_addr += remain;
        return 0;
    }

    return avi_write_movi_junk_chunk(avi, remain);
}

static uint32_t avi_write_idx_tail_junk(AVI_FILE *avi)
{
    uint32_t remain;

    if (!avi || !avi->ops_init || !avi->ops.write_at || avi->idx1_write_addr >= avi->idx1_end_addr)
    {
        return 0;
    }

    if (avi_write_idx_syn(avi))
    {
        return 1;
    }

    if (avi->idx1_write_addr >= avi->idx1_end_addr)
    {
        return 0;
    }

    remain = avi->idx1_end_addr - avi->idx1_write_addr;
    if (remain == 0)
    {
        return 0;
    }

    if (remain < 8)
    {
        uint8_t zero[8] = {0};
        if (avi->ops.write_at(avi->ops.file, avi->idx1_write_addr, zero, remain))
        {
            return 1;
        }
        avi->idx1_write_addr = avi->idx1_end_addr;
        return 0;
    }

    remain -= 8;
    if (avi->ops.write_at(avi->ops.file, avi->idx1_write_addr, "JUNK", 4) ||
        avi->ops.write_at(avi->ops.file, avi->idx1_write_addr + 4, &remain, 4))
    {
        return 1;
    }

    avi->idx1_write_addr = avi->idx1_end_addr;

    return 0;
}

static uint32_t avi_write_idx_snapshot_tail(AVI_FILE *avi)
{
    uint32_t remain;
    uint32_t junk_size;

    if (!avi || !avi->ops_init || !avi->ops.write_at || avi->idx1_write_addr >= avi->idx1_end_addr)
    {
        return 0;
    }

    remain = avi->idx1_end_addr - avi->idx1_write_addr;
    if (remain < 8)
    {
        return 1;
    }

    junk_size = remain - 8;
    if (avi->ops.write_at(avi->ops.file, avi->idx1_write_addr, "JUNK", 4) ||
        avi->ops.write_at(avi->ops.file, avi->idx1_write_addr + 4, &junk_size, 4))
    {
        return 1;
    }

    return 0;
}

static uint32_t avi_write_align_junk(AVI_FILE *avi, uint32_t start, uint32_t end)
{
    uint32_t remain;

    if (!avi || !avi->ops_init || !avi->ops.write || !avi->ops.skip ||
        !avi->ops.tell || end <= start)
    {
        return 0;
    }

    remain = end - start;
    if (avi->ops.tell(avi->ops.file) != start)
    {
        return 1;
    }

    if (remain >= 8)
    {
        uint32_t size = remain - 8;
        if (avi->ops.write(avi->ops.file, "JUNK", 4) ||
            avi->ops.write(avi->ops.file, &size, 4) ||
            avi->ops.skip(avi->ops.file, size))
        {
            return 1;
        }
    }
    else
    {
        return 1;
    }

    return 0;
}

static uint32_t avi_write_header(AVI_FILE *avi)
{
    if (!avi || !avi->ops_init || !avi->ops.write || !avi->ops.tell)
    {
        return 1;
    }

    avi->riff_size_offset = avi->ops.tell(avi->ops.file) + 4;
    if (avi->ops.write(avi->ops.file, avi->riff, 4) ||
        avi->ops.write(avi->ops.file, &avi->riff_size, 4) ||
        avi->ops.write(avi->ops.file, avi->type_avi, 4))
    {
        return 1;
    }

    if (avi->ops.write(avi->ops.file, avi->hlist, 4) ||
        avi->ops.write(avi->ops.file, &avi->hlist_size, 4) ||
        avi->ops.write(avi->ops.file, avi->type_hdrl, 4))
    {
        return 1;
    }

    avi->total_frames_offset = avi->ops.tell(avi->ops.file) + 8 + offsetof(AVI_HEADER, total_frames);
    if (avi->ops.write(avi->ops.file, avi->avih, 4) ||
        avi->ops.write(avi->ops.file, &avi->avih_size, 4) ||
        avi->ops.write(avi->ops.file, &avi->avi_header, avi->avih_size))
    {
        return 1;
    }

    if (avi->audio_enable)
    {
        if (avi->ops.write(avi->ops.file, avi->slist1, 4) ||
            avi->ops.write(avi->ops.file, &avi->slist1_size, 4) ||
            avi->ops.write(avi->ops.file, avi->type_str1, 4))
        {
            return 1;
        }

        avi->audio_length_offset = avi->ops.tell(avi->ops.file) + 8 + offsetof(STREAM_HEADER, length);
        if (avi->ops.write(avi->ops.file, avi->strhdr1, 4) ||
            avi->ops.write(avi->ops.file, &avi->strhdr1_size, 4) ||
            avi->ops.write(avi->ops.file, &avi->strhdr_audio, avi->strhdr1_size) ||
            avi->ops.write(avi->ops.file, avi->strfmt1, 4) ||
            avi->ops.write(avi->ops.file, &avi->strfmt1_size, 4) ||
            avi->ops.write(avi->ops.file, &avi->strfmt_audio, avi->strfmt1_size))
        {
            return 1;
        }
    }

    if (avi->ops.write(avi->ops.file, avi->slist2, 4) ||
        avi->ops.write(avi->ops.file, &avi->slist2_size, 4) ||
        avi->ops.write(avi->ops.file, avi->type_str2, 4))
    {
        return 1;
    }

    avi->video_length_offset = avi->ops.tell(avi->ops.file) + 8 + offsetof(STREAM_HEADER, length);
    if (avi->ops.write(avi->ops.file, avi->strhdr2, 4) ||
        avi->ops.write(avi->ops.file, &avi->strhdr2_size, 4) ||
        avi->ops.write(avi->ops.file, &avi->strhdr_video, avi->strhdr2_size) ||
        avi->ops.write(avi->ops.file, avi->strfmt2, 4) ||
        avi->ops.write(avi->ops.file, &avi->strfmt2_size, 4) ||
        avi->ops.write(avi->ops.file, &avi->strfmt_video, avi->strfmt2_size))
    {
        return 1;
    }

    avi->movi_size_offset = avi->ops.tell(avi->ops.file) + 4;
    if (avi->ops.write(avi->ops.file, avi->mlist, 4) ||
        avi->ops.write(avi->ops.file, &avi->mlist_size, 4) ||
        avi->ops.write(avi->ops.file, avi->type_movi, 4))
    {
        return 1;
    }

    return 0;
}

void avimuxer_set_file(void *ctx, const file_ops_t *ops)
{
    AVI_FILE *avi = (AVI_FILE *) ctx;

    if (!avi || !ops)
    {
        return;
    }

    os_memcpy(&avi->ops, ops, sizeof(avi->ops));
    avi->ops_init = (avi->ops.file && avi->ops.write && avi->ops.skip &&
                     avi->ops.tell && avi->ops.write_at && avi->ops.flush &&
                     avi->ops.finish && avi->ops.tail) ? 1 : 0;
}

void *avimuxer_init_with_file(void *fp, const file_ops_t *ops, uint32_t max_size, int w, int h, int frate, int h265, int audio_enable)
{
    int       samprate = 8000, channels = 1, sampbits = 16;
    AVI_FILE *avi = STREAM_ZALLOC(1 * sizeof(AVI_FILE));
    if (!avi)
    {
        goto failed;
    }
    avi->extern_fp = 1;
    avi->fp        = (F_FILE *) fp;
    avi->audio_enable = audio_enable ? 1 : 0;
    if (!avi->fp)
    {
        goto failed;
    }

    if (ops)
    {
        avimuxer_set_file(avi, ops);
    }
    else
    {
        file_ops_t default_ops;
        avi_default_ops_init(avi->fp, &default_ops);
        avimuxer_set_file(avi, &default_ops);
    }
    if (!avi->ops_init)
    {
        goto failed;
    }

    //预先分配空间
    max_size = muxer_file_align_up(max_size);
    if (max_size <= IDX_MAX_SIZE + MUXER_FILE_ALIGN_SIZE)
    {
        goto failed;
    }
    pre_avi_seek(avi->fp, max_size);
    avi->fast_seek_tbl = avi_fast_seek_create(avi->fp);

    //开始写入数据头
    avi_seek(avi->fp, 0, SEEK_SET);

    memcpy(avi->avih, "avih", 4);
    avi->avih_size                     = sizeof(AVI_HEADER);
    avi->avi_header.microsec_per_frame = 1000000 / frate;
    avi->avi_header.maxbytes_per_Sec   = w * h * 3;
    avi->avi_header.flags              = AVIF_ISINTERLEAVED | AVIF_HASINDEX;
    avi->avi_header.number_streams     = avi->audio_enable ? 2 : 1;
    avi->avi_header.width              = w;
    avi->avi_header.height             = h;
    avi->avi_header.suggested_bufsize  = w * h * 3;
    memcpy(avi->video_tag, avi->audio_enable ? "01dc" : "00dc", 4);

    if (avi->audio_enable)
    {
        memcpy(avi->strhdr1, "strh", 4);
        memcpy(avi->strhdr_audio.fcc_type, "auds", 4);
        memcpy(avi->strhdr_audio.fcc_codec, "PCM ", 4);
        avi->strhdr1_size                   = sizeof(STREAM_HEADER);
        avi->strhdr_audio.scale             = 1;
        avi->strhdr_audio.rate              = samprate;
        avi->strhdr_audio.suggested_bufsize = samprate * channels * sampbits / 8;
        avi->strhdr_audio.sample_size       = channels * sampbits / 8;

        memcpy(avi->strfmt1, "strf", 4);
        avi->strfmt1_size                 = sizeof(WAVE_FORMAT);
        avi->strfmt_audio.format_tag      = 1;
        avi->strfmt_audio.channels        = channels;
        avi->strfmt_audio.sample_per_sec  = samprate;
        avi->strfmt_audio.avgbyte_per_sec = samprate * channels * sampbits / 8;
        avi->strfmt_audio.block_align     = channels * sampbits / 8;
        avi->strfmt_audio.bits_per_sample = sampbits;

        memcpy(avi->slist1, "LIST", 4);
        memcpy(avi->type_str1, "strl", 4);
        avi->slist1_size = 4 + 8 + avi->strhdr1_size + 8 + avi->strfmt1_size;
    }

    memcpy(avi->strhdr2, "strh", 4);
    memcpy(avi->strhdr_video.fcc_type, "vids", 4);
    memcpy(avi->strhdr_video.fcc_codec, h265 ? "HEV1" : "MJPG", 4);
    avi->strhdr2_size                   = sizeof(STREAM_HEADER);
    avi->strhdr_video.scale             = 1;
    avi->strhdr_video.rate              = frate;
    avi->strhdr_video.suggested_bufsize = 0;
    avi->strhdr_video.quality           = -1;
    avi->strhdr_video.vrect_right       = w;
    avi->strhdr_video.vrect_bottom      = h;

    memcpy(avi->strfmt2, "strf", 4);
    avi->strfmt2_size             = sizeof(BITMAP_FORMAT);
    avi->strfmt_video.size        = 40;
    avi->strfmt_video.width       = w;
    avi->strfmt_video.height      = h;
    avi->strfmt_video.planes      = 1;
    avi->strfmt_video.bitcount    = 24;
    avi->strfmt_video.compression = h265 ? AVI_FOURCC('H', 'E', 'V', '1') : AVI_FOURCC('M', 'J', 'P', 'G');
    avi->strfmt_video.image_size  = w * h * 3;

    memcpy(avi->slist2, "LIST", 4);
    memcpy(avi->type_str2, "strl", 4);
    avi->slist2_size = 4 + 4 + 4 + avi->strhdr2_size + 4 + 4 + avi->strfmt2_size;

    memcpy(avi->riff, "RIFF", 4);
    memcpy(avi->type_avi, "AVI ", 4);
    memcpy(avi->hlist, "LIST", 4);
    memcpy(avi->type_hdrl, "hdrl", 4);
    memcpy(avi->mlist, "LIST", 4);
    memcpy(avi->type_movi, "movi", 4);
    avi->hlist_size = 4 + 8 + avi->avih_size + 8 + avi->slist2_size;
    if (avi->audio_enable)
    {
        avi->hlist_size += 8 + avi->slist1_size;
    }

    if (avi_write_header(avi))
    {
        goto failed;
    }

    uint32_t movi_addr = avi->ops.tell(avi->ops.file);
    avi->movi_addr     = movi_addr;
    avi->movi_data_addr = muxer_file_align_up(movi_addr);
    if (avi->movi_data_addr != movi_addr && avi->movi_data_addr - movi_addr < 8)
    {
        avi->movi_data_addr += MUXER_FILE_ALIGN_SIZE;
    }

    avi->idx1_addr      = muxer_file_align_up(max_size - IDX_MAX_SIZE);
    if (avi->idx1_addr >= max_size)
    {
        goto failed;
    }
    avi->idx1_end_addr  = max_size;
    avi->idx1_size_addr = avi->idx1_addr + 4;
    avi->idx1_data_addr = avi->idx1_addr + 8;
    avi->movi_data_size = avi->idx1_addr - avi->movi_addr;
    if (avi_write_align_junk(avi, avi->movi_addr, avi->movi_data_addr))
    {
        goto failed;
    }
    if (avi->ops.skip(avi->ops.file, avi->movi_data_addr - avi->ops.tell(avi->ops.file)))
    {
        goto failed;
    }
    {
        uint32_t idx1size = 0;
        if (avi->ops.write_at(avi->ops.file, avi->idx1_addr, "idx1", 4) ||
            avi->ops.write_at(avi->ops.file, avi->idx1_addr + 4, &idx1size, 4))
        {
            goto failed;
        }
    }

    avi->movi_addr_end   = avi->idx1_addr;
    avi->idx1_write_addr = avi->idx1_data_addr;
    avi->cur_addr = avi->movi_data_addr;
    avi->movi_guard_block_end = muxer_file_align_up(avi->cur_addr);
    avi->syn_time = 0;
    return avi;

failed:
    if (avi)
    {
        avi_fast_seek_destroy(avi->fp, &avi->fast_seek_tbl);
        STREAM_FREE(avi);
    }
    return NULL;
}

void *avimuxer_init(void *fp, uint32_t max_size, int w, int h, int frate, int h265, int audio_enable)
{
    return avimuxer_init_with_file(fp, NULL, max_size, w, h, frate, h265, audio_enable);
}

static void avimuxer_fix_data_aligned(AVI_FILE *avi, int final_flush)
{
    uint32_t data;
    uint32_t movisize;
    uint32_t riffsize;

    if (!avi || !avi->ops_init || !avi->ops.write_at || !avi->ops.flush)
    {
        return;
    }

    if (final_flush)
    {
        if (avi_write_movi_tail_junk(avi))
        {
            return;
        }
        if (avi->ops.finish(avi->ops.file))
        {
            return;
        }
        if (avi_write_idx_tail_junk(avi))
        {
            return;
        }
    }
    else
    {
        // if (avi_write_movi_snapshot_tail(avi))
        // {
        //     return;
        // }
        avi->movi_guard_block_end = avi_movi_guard_block_end(avi);
        if (avi_write_idx_syn(avi))
        {
            return;
        }
        if (avi_write_idx_snapshot_tail(avi))
        {
            return;
        }
    }

    avi->avi_header.total_frames = avi->strhdr_video.length;

    data = avi->avi_header.total_frames;
    avi->ops.write_at(avi->ops.file, avi->total_frames_offset, &data, 4);

    if (avi->audio_enable)
    {
        data = avi->strhdr_audio.length;
        avi->ops.write_at(avi->ops.file, avi->audio_length_offset, &data, 4);
    }

    data = avi->strhdr_video.length;
    avi->ops.write_at(avi->ops.file, avi->video_length_offset, &data, 4);

    movisize = avi->movi_data_size + 4;
    avi->ops.write_at(avi->ops.file, avi->movi_size_offset, &movisize, 4);

    riffsize = final_flush ? (avi->idx1_end_addr - 8) :
                             (avi->idx1_data_addr + avi->idx1_data_size - 8);
    avi->ops.write_at(avi->ops.file, avi->riff_size_offset, &riffsize, 4);

    avi->ops.write_at(avi->ops.file, avi->idx1_size_addr, &avi->idx1_data_size, 4);

    avi->ops.flush(avi->ops.file);
    avi->syn_time = avimuxer_written_duration_ms(avi);
}

void avimuxer_sync(void *ctx)
{
    AVI_FILE *avi = (AVI_FILE *) ctx;
    if (avi && avi->fp)
    {
        avimuxer_fix_data_aligned(avi, 0);
        avi->syn_time = avimuxer_written_duration_ms(avi);
    }
}

void avimuxer_sync_time(void *ctx, uint32_t time_ms)
{
    AVI_FILE *avi = (AVI_FILE *) ctx;
    uint32_t  written_duration;

    if (!avi || !avi->fp)
    {
        return;
    }

    if (time_ms == 0)
    {
        avimuxer_sync(ctx);
        return;
    }

    written_duration = avimuxer_written_duration_ms(avi);
    if ((avi->syn_time == 0 && written_duration > 0) ||
        written_duration < avi->syn_time ||
        written_duration - avi->syn_time >= time_ms)
    {
        avimuxer_sync(ctx);
    }
}

void avimuxer_exit(void *ctx)
{
    AVI_FILE *avi = (AVI_FILE *) ctx;
    if (avi)
    {
        os_printf("avi->extern_fp:%d\n", avi->extern_fp);
        if (avi->fp)
        {
            avimuxer_fix_data_aligned(avi, 1);
            avi_fast_seek_destroy(avi->fp, &avi->fast_seek_tbl);
            if (!avi->extern_fp)
            {
                avi_close(avi->fp);
            }
        }
        if (avi->framesize_lst)
        {
            STREAM_FREE(avi->framesize_lst);
        }
        STREAM_FREE(avi);
    }
}

uint32_t avimuxer_video(void *ctx, unsigned char *buf, int len, int key, uint8_t insert)
{
    uint32_t ret = AVIMUXER_OK;
    AVI_FILE *avi = (AVI_FILE *) ctx;
    if (avi == NULL)
    {
        ret =  AVIMUXER_ERR;
        goto avimuxer_video_end;
    }
    if (avi->fp)
    {
        uint32_t flags          = key ? AVIIF_KEYFRAME : 0;
        uint32_t chunk_offset   = 0;
        uint32_t chunk_size     = (len & 1) ? len + 1 : len;
        uint8_t video_recorded = 0;

        if (!insert)
        {
            ret = avi_write_movi_chunk(avi, avi->video_tag, flags, buf, len, &chunk_offset);
            if (ret != AVIMUXER_OK)
            {
                goto avimuxer_video_end;
            }

            // if (avi_write_movi_guard(avi))
            // {
            //     ret = AVIMUXER_ERR;
            //     goto avimuxer_video_end;
            // }

            avi->last_video_offset = chunk_offset;
            avi->last_video_size   = chunk_size;
            avi->last_video_flags  = flags;
            video_recorded         = 1;
        }
        else if (avi->last_video_size)
        {
            if (!avi_idx_entry_fits(avi))
            {
                ret = AVIMUXER_FULL;
                goto avimuxer_video_end;
            }

            if (avi_idx_write_entry(avi, avi->video_tag, avi->last_video_flags, avi->last_video_offset, avi->last_video_size))
            {
                ret = AVIMUXER_ERR;
                goto avimuxer_video_end;
            }
            avi->total_idx++;
            video_recorded = 1;
        }

        if (video_recorded)
        {
            avi->strhdr_video.length++;
        }
    }
avimuxer_video_end:
    return ret;
}

uint32_t avimuxer_audio(void *ctx, unsigned char *buf, int len)
{
    uint32_t ret = AVIMUXER_OK;
    AVI_FILE *avi = (AVI_FILE *) ctx;

    if (!avi || !avi->fp)
    {
        return AVIMUXER_ERR;
    }

    if (!avi->audio_enable)
    {
        return AVIMUXER_OK;
    }

    if (!buf || len <= 0)
    {
        return AVIMUXER_ERR;
    }

    ret = avi_write_movi_chunk(avi, "00wb", 0, buf, len, NULL);
    if (ret != AVIMUXER_OK)
    {
        goto avimuxer_audio_end;
    }

    // if (avi_write_movi_guard(avi))
    // {
    //     ret = AVIMUXER_ERR;
    //     goto avimuxer_audio_end;
    // }

    avi->strhdr_audio.length += (len >> 1);
avimuxer_audio_end:
    return ret;
}
