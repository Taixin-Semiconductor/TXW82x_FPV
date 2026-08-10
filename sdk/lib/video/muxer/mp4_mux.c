#include "lib/fs/fatfs/osal_file.h"
#include "lib/heap/av_psram_heap.h"
#include "osal/string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "audio_msi/audio_adc.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/multimedia/msi.h"
#include "osal/string.h"
#include "stream_define.h"
#include "mp4_mux.h"


// data 申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC os_malloc
#define STREAM_LIBC_FREE   os_free
#define STREAM_LIBC_ZALLOC os_zalloc

#define MP4_FUNC_MACRO(x) mp4_##x##_write
#define MP4_S_MACRO(x)    mp4_##x

// mp4 裁剪为实际文件大小
#define MP4_TRUNCATE_REAL_EN    0

#if FF_USE_FASTSEEK
#define MP4_FAST_SEEK_CLMT_ITEMS_MIN 64U

static uint32_t *mp4_fast_seek_create(F_FILE *fp)
{
    uint32_t *cltbl;
    uint32_t  table_items = MP4_FAST_SEEK_CLMT_ITEMS_MIN;
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
            os_printf(KERN_WARNING "mp4 fast seek alloc failed, items:%d\n", table_items);
            return NULL;
        }

        cltbl[0]  = table_items;
        fp->cltbl = (DWORD *) cltbl;
        res       = f_lseek(fp, CREATE_LINKMAP);
        if (res == FR_OK)
        {
            os_printf(KERN_INFO "mp4 fast seek enabled, items:%d\n", cltbl[0]);
            return cltbl;
        }

        if (res == FR_NOT_ENOUGH_CORE && cltbl[0] > table_items)
        {
            table_items = cltbl[0];
            fp->cltbl   = NULL;
            STREAM_FREE(cltbl);
            continue;
        }

        os_printf(KERN_WARNING "mp4 fast seek disabled, res:%d, items:%d\n", res, cltbl[0]);
        fp->cltbl = NULL;
        STREAM_FREE(cltbl);
        return NULL;
    }
}

static void mp4_fast_seek_destroy(F_FILE *fp, uint32_t **cltbl)
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
static uint32_t *mp4_fast_seek_create(F_FILE *fp)
{
    (void) fp;
    return NULL;
}

static void mp4_fast_seek_destroy(F_FILE *fp, uint32_t **cltbl)
{
    (void) fp;
    (void) cltbl;
}
#endif

static int mp4_default_ops_write(void *file, const void *buf, uint32_t len);
static int mp4_default_ops_skip(void *file, uint32_t len);
static uint32_t mp4_default_ops_tell(void *file);
static int mp4_default_ops_write_at(void *file, uint32_t offset, const void *buf, uint32_t len);
static int mp4_default_ops_flush(void *file);
static int mp4_default_ops_finish(void *file);
static int mp4_default_ops_tail(void *file, const void *tail, uint32_t tail_len,
                                uint32_t logical_end, uint32_t reserved_end);
static void mp4_default_ops_init(F_FILE *fp, file_ops_t *ops);

static uint32_t mp4_set_io_offset(mp4_key_msg *msg, uint32_t offset)
{
    if (!msg || !msg->ops_init)
    {
        return offset;
    }

    if (msg->ops.tell && msg->ops.skip)
    {
        uint32_t file_offset = msg->ops.tell(msg->ops.file);
        if (offset > file_offset)
        {
            msg->ops.skip(msg->ops.file, offset - file_offset);
        }
    }
    msg->io_offset = offset;
    if (msg->msg_end < offset)
    {
        msg->msg_end = offset;
    }
    return offset;
}

static uint32_t mp4_tell(mp4_key_msg *msg)
{
    if (!msg)
    {
        return 0;
    }

    if (msg->ops_init)
    {
        return msg->io_offset;
    }

    return msg->fp ? osal_ftell(msg->fp) : 0;
}

static uint32_t mp4_track_timescale(const mp4_key_msg *msg, uint32_t track_index)
{
    if (track_index == 0)
    {
        return MP4_VIDEO_TIMESCALE;
    }

    return msg->audio_samplerate ? msg->audio_samplerate : 8000U;
}

static uint32_t mp4_track_duration_ms(const mp4_key_msg *msg, uint32_t track_index)
{
    uint32_t timescale;

    if (!msg || track_index >= msg->trak_count)
    {
        return 0;
    }

    timescale = mp4_track_timescale(msg, track_index);
    return (uint32_t) ((((unsigned long long) msg->trak[track_index].duration_ticks * 1000U) + timescale / 2U) / timescale);
}

static uint32_t mp4_written_duration_ms(mp4_key_msg *msg)
{
    uint32_t max_duration = 0;

    if (!msg)
    {
        return 0;
    }

    for (int i = 0; i < msg->trak_count; i++)
    {
        uint32_t duration_ms = mp4_track_duration_ms(msg, i);
        max_duration = max_duration > duration_ms ? max_duration : duration_ms;
    }

    return max_duration;
}

static void pre_mp4_seek(F_FILE *fp, uint32_t offset)
{
    uint32_t filesize  = osal_fsize(fp);
    if(filesize != offset)
    {
        osal_fseek(fp, offset);
        osal_ftruncate(fp);
        _os_printf("mp4 size: %d\n", osal_fsize(fp));
    }
}

static uint32_t mp4_seek(mp4_key_msg *msg, int32_t offset, int seek_mode)
{
    uint32_t fp_offset = 0;
    uint32_t filesize;
    F_FILE  *fp;

    if (!msg || !msg->fp)
    {
        return 0;
    }

    fp       = msg->fp;
    filesize = osal_fsize(fp);
    if (msg->ops_init)
    {
        if (seek_mode == SEEK_SET)
        {
            mp4_set_io_offset(msg, offset);
        }
        else if (seek_mode == SEEK_CUR)
        {
            mp4_set_io_offset(msg, msg->io_offset + offset);
        }
        else if (seek_mode == SEEK_END)
        {
            mp4_set_io_offset(msg, osal_fsize(fp) + offset);
        }
        return offset;
    }

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

static uint32_t mp4_write(mp4_key_msg *msg, const void *buf, uint32_t size, uint32_t n)
{
    // 返回值0是代表异常
    uint32_t ret              = 1;
    uint32_t write_size_total = size * n;
    F_FILE   *fp;

    if (!msg || !msg->fp)
    {
        return 1;
    }

    fp = msg->fp;
    if (msg->ops_init)
    {
        if (!buf && write_size_total)
        {
            return 1;
        }

        if (msg->ops.tell && msg->ops.write && msg->ops.tell(msg->ops.file) == msg->io_offset)
        {
            ret = msg->ops.write(msg->ops.file, buf, write_size_total) ? 1 : 0;
        }
        else if (msg->ops.write_at)
        {
            if (msg->ops.tell && msg->ops.skip)
            {
                uint32_t file_offset = msg->ops.tell(msg->ops.file);
                uint32_t write_end = msg->io_offset + write_size_total;
                if (write_end > file_offset)
                {
                    if (msg->ops.skip(msg->ops.file, write_end - file_offset))
                    {
                        return 1;
                    }
                }
            }
            ret = msg->ops.write_at(msg->ops.file, msg->io_offset, buf, write_size_total) ? 1 : 0;
        }
        else
        {
            return 1;
        }

        if (!ret)
        {
            msg->io_offset += write_size_total;
            if (msg->msg_end < msg->io_offset)
            {
                msg->msg_end = msg->io_offset;
            }
        }
        return ret;
    }

    ret = osal_fwrite((void *) buf, 1, write_size_total, fp);
    return ret != write_size_total;
}

static uint32_t mp4_mdat_write_data(mp4_key_msg *msg, const void *buf, uint32_t len)
{
    if (!msg || !msg->mdat_writer_init || !msg->ops_init || !msg->ops.write)
    {
        os_printf("%s %d, msg->mdat_writer_init: %d, msg->ops_init: %d\n", __FUNCTION__, __LINE__, msg ? msg->mdat_writer_init : -1, msg ? msg->ops_init : -1);
        return 1;
    }

    if (msg->ops.write(msg->ops.file, buf, len))
    {
        os_printf("%s %d\r\n", __FUNCTION__, __LINE__);
        return 1;
    }

    msg->io_offset += len;
    if (msg->msg_end < msg->io_offset)
    {
        msg->msg_end = msg->io_offset;
    }

    return 0;
}

static void mp4_reset_init_state(mp4_key_msg *msg)
{
    if (!msg)
    {
        return;
    }

    mp4_fast_seek_destroy(msg->fp, &msg->fast_seek_tbl);

    msg->init                 = 0;
    msg->msg_end              = 0;
    msg->sync_time             = 0;
    msg->trak_count           = 0;
    msg->mdat_size            = 0;
    msg->mdat_size_offset     = 0;
    msg->mdat_offset          = 0;
    msg->mdat_nowoffset       = 0;
    msg->mvhd_duration_offset = 0;
    msg->mdat_writer_init     = 0;
    msg->io_offset            = (msg->ops_init && msg->ops.tell) ? msg->ops.tell(msg->ops.file) : 0;
    memset(msg->trak, 0, sizeof(msg->trak));
}

F_FILE *mp4_open(const char *filename, char *mode)
{
    return osal_fopen(filename, mode);
}

void mp4_close(F_FILE *fp)
{
    osal_fclose(fp);
}

static void mp4_truncate(mp4_key_msg *msg, uint32_t offset)
{
    if (!msg || !msg->fp)
    {
        return;
    }

    if (msg->ops_init)
    {
        mp4_set_io_offset(msg, msg->io_offset + offset);
        return;
    }

    mp4_seek(msg, offset, SEEK_CUR); // 预留的的空间
    // osal_ftruncate(fp);
}

static void mp4_file_syn(mp4_key_msg *msg)
{
    if (!msg || !msg->fp)
    {
        return;
    }

    if (msg->ops_init && msg->ops.flush)
    {
        msg->ops.flush(msg->ops.file);
        return;
    }

    osal_fsync(msg->fp);
}

static int mp4_default_ops_write(void *file, const void *buf, uint32_t len)
{
    F_FILE *fp = (F_FILE *) file;
    uint32_t written;

    if (!fp || (!buf && len))
    {
        return -1;
    }

    written = osal_fwrite((void *) buf, 1, len, fp);
    return written == len ? 0 : -1;
}

static int mp4_default_ops_skip(void *file, uint32_t len)
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

static uint32_t mp4_default_ops_tell(void *file)
{
    F_FILE *fp = (F_FILE *) file;
    return fp ? osal_ftell(fp) : 0;
}

static int mp4_default_ops_write_at(void *file, uint32_t offset, const void *buf, uint32_t len)
{
    F_FILE *fp = (F_FILE *) file;
    uint32_t restore_offset;

    if (!fp || (!buf && len))
    {
        return -1;
    }

    restore_offset = osal_ftell(fp);
    osal_fseek(fp, offset);
    if (mp4_default_ops_write(file, buf, len))
    {
        return -1;
    }
    osal_fseek(fp, restore_offset);
    return 0;
}

static int mp4_default_ops_flush(void *file)
{
    F_FILE *fp = (F_FILE *) file;

    if (!fp)
    {
        return -1;
    }

    return osal_fsync(fp) == 0 ? 0 : -1;
}

static int mp4_default_ops_finish(void *file)
{
    return mp4_default_ops_flush(file);
}

static int mp4_default_ops_tail(void *file, const void *tail, uint32_t tail_len,
                                uint32_t logical_end, uint32_t reserved_end)
{
    (void) file;
    (void) tail;
    (void) tail_len;
    (void) logical_end;
    (void) reserved_end;
    return 0;
}

static void mp4_default_ops_init(F_FILE *fp, file_ops_t *ops)
{
    memset(ops, 0, sizeof(*ops));
    ops->file     = fp;
    ops->write    = mp4_default_ops_write;
    ops->skip     = mp4_default_ops_skip;
    ops->tell     = mp4_default_ops_tell;
    ops->write_at = mp4_default_ops_write_at;
    ops->flush    = mp4_default_ops_flush;
    ops->finish   = mp4_default_ops_finish;
    ops->tail     = mp4_default_ops_tail;
}



#define ATOM(x)                                                                                                                                                                                        \
    {                                                                                                                                                                                                  \
        stack->offset = mp4_tell(msg);                                                                                                                                                                  \
        stack->func   = MP4_FUNC_MACRO(x);                                                                                                                                                             \
        stack++;                                                                                                                                                                                       \
        mp4_seek(msg, sizeof(MP4_S_MACRO(x)), SEEK_CUR);                                                                                                                                                \
    }
#define ATOM_NOT_OFFSET(x)                                                                                                                                                                             \
    {                                                                                                                                                                                                  \
        stack->offset = mp4_tell(msg);                                                                                                                                                                  \
        stack->func   = MP4_FUNC_MACRO(x);                                                                                                                                                             \
        stack++;                                                                                                                                                                                       \
    }
#define END_ATOM                                                                                                                                                                                       \
    {                                                                                                                                                                                                  \
        --stack;                                                                                                                                                                                       \
        stack->func(fp, stack->offset, msg);                                                                                                                                                           \
        mp4_seek(msg, msg->msg_end, SEEK_SET);                                                                                                                                                          \
    }

#define VIDEO_NAME     "VideoHandler"
#define SOUND_NAME     "SoundHandler"
#define BIG4_ENDIAN(X) (((X) & 0xff000000) >> 24 | ((X) & 0x00FF0000) >> 8 | ((X) & 0x0000FF00) << 8 | ((X) & 0x000000FF) << 24)
#define BIG3_ENDIAN(X) (((X) & 0xff0000) >> 16 | ((X) & 0x00FF00) >> 8 | ((X) & 0x0000FF) << 16)
#define BIG2_ENDIAN(X) (((X) & 0xFF00) >> 8 | ((X) & 0x00FF) << 8)
#define BIG1_ENDIAN(X) ((X))

#define STTS_COUNT (0x2000)
#define STSC_COUNT (1)
#define STSZ_COUNT (0x20000)
#define STCO_COUNT (0x20000)
#define STSS_COUNT (0x10000)
static const char          language[4] = "und";
static const unsigned char box_ftyp[]  = {
#if 1
        0, 0, 0, 0x18, 'f', 't', 'y', 'p', 'm', 'p', '4', '2', 0, 0, 0, 0, 'm', 'p', '4', '2', 'i', 's', 'o', 'm',
#else
        // as in ffmpeg
        0, 0, 0, 0x20, 'f', 't', 'y', 'p', 'i', 's', 'o', 'm', 0, 0, 2, 0, 'm', 'p', '4', '1', 'i', 's', 'o', 'm', 'i', 's', 'o', '2', 'a', 'v', 'c', '1',
#endif
};

#define mp4_moov mp4_common_box_s
#define mp4_trak mp4_common_box_s
#define mp4_mdia mp4_common_box_s
#define mp4_minf mp4_common_box_s
#define mp4_dinf mp4_common_box_s
#define mp4_stbl mp4_common_box_s
#define mp4_mdat mp4_common_box_s
#define mp4_free mp4_common_box_s
#define mp4_esds mp4_common_box_full_s

uint32_t        mp4_audio_cfg_init(mp4_key_msg *msg, uint8_t *asps_data, uint8_t len);
static uint32_t _MP4_init(F_FILE *fp, mp4_key_msg *msg);
static uint32_t write_h264_nal(mp4_key_msg *msg, uint8_t *nal_buf, uint32_t size, uint32_t duration_ticks, int keyflag);

uint32_t write_h264_pps_sps(mp4_key_msg *msg, uint8_t *nal_buf, uint32_t size);

// 获取nal的size,从0开始搜索,返回的是的nal头的size,offset相对于头的偏移(通过多次调用,可以用于计算nal_size)
// 返回0代表搜索不到nal的头
static uint8_t get_nal_size(uint8_t *buf, uint32_t size, uint32_t *offset)
{
    uint32_t pos = 0;
    while ((size - pos) > 3)
    {
        if (buf[pos] == 0 && buf[pos + 1] == 0 && buf[pos + 2] == 1)
        {
            *offset = pos;
            return 3;
        }

        if (buf[pos] == 0 && buf[pos + 1] == 0 && buf[pos + 2] == 0 && buf[pos + 3] == 1)
        {
            *offset = pos;
            return 4;
        }

        pos++;
    }
    return 0;
}

// 只是获取pps和sps的nalsize
static uint8_t *get_sps_pps_nal_size(uint8_t *buf, uint32_t size, uint32_t *nal_size, uint8_t *head_size)
{
    uint32_t offset;
    uint8_t  nal_head_size = get_nal_size(buf, size, &offset);
    uint8_t  nal_type;
    uint8_t *ret_buf = NULL;
    // 找到头部,检查类型
    if (nal_head_size && offset + nal_head_size < size)
    {
        nal_type = buf[nal_head_size + offset] & 0x1f;

        // 找到sps和pps就返回长度和偏移(相对buf的偏移)
        if (nal_type == 7 || nal_type == 8)
        {
            // 查找下一个nal
            nal_head_size = get_nal_size(buf + offset + nal_head_size, size - (offset + nal_head_size), nal_size);
            // os_printf("nal_head_size:%d\tnal_size:%d\n",nal_head_size,*nal_size);
            if (nal_head_size)
            {
                // 偏移到nal的头部

                ret_buf    = buf + offset;
                // 返回nal的头size
                *head_size = nal_head_size;
            }
        }
    }

    return ret_buf;
}

uint32_t mp4_ftyp_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_seek(msg, offset, SEEK_SET);
    mp4_write(msg, (void *) box_ftyp, 1, sizeof(box_ftyp));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

// 写入一个moov的结构,并且预留空间
uint32_t mp4_moov_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    uint32_t         now_offset = mp4_tell(msg);
    mp4_common_box_s moov;
    memcpy(moov.boxname, "moov", 4);
    moov.size = BIG4_ENDIAN(now_offset - offset);
    mp4_seek(msg, offset, SEEK_SET);
    mp4_write(msg, &moov, 1, sizeof(moov));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_mvhd_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_mvhd mvhd;
    memset(&mvhd, 0, sizeof(mvhd));
    mvhd.size = BIG4_ENDIAN(sizeof(mp4_mvhd));
    memcpy(mvhd.boxname, "mvhd", 4);
    mvhd.creation_time     = BIG4_ENDIAN(0);
    mvhd.modification_time = BIG4_ENDIAN(0);
    mvhd.timescale         = BIG4_ENDIAN(1000);
    mvhd.duration          = BIG4_ENDIAN(0);
    mvhd.rate              = BIG4_ENDIAN(0x00010000);
    mvhd.volume            = BIG2_ENDIAN(0x0100);

    mvhd.matrix[0] = BIG4_ENDIAN(0x00010000);
    mvhd.matrix[1] = BIG4_ENDIAN(0);
    mvhd.matrix[2] = BIG4_ENDIAN(0);
    mvhd.matrix[3] = BIG4_ENDIAN(0);
    mvhd.matrix[4] = BIG4_ENDIAN(0x00010000);
    mvhd.matrix[5] = BIG4_ENDIAN(0);
    mvhd.matrix[6] = BIG4_ENDIAN(0);
    mvhd.matrix[7] = BIG4_ENDIAN(0);
    mvhd.matrix[8] = BIG4_ENDIAN(0x40000000);

    mvhd.pre_defined[0] = BIG4_ENDIAN(0);
    mvhd.pre_defined[1] = BIG4_ENDIAN(0);
    mvhd.pre_defined[2] = BIG4_ENDIAN(0);
    mvhd.pre_defined[3] = BIG4_ENDIAN(0);
    mvhd.pre_defined[4] = BIG4_ENDIAN(0);
    mvhd.pre_defined[5] = BIG4_ENDIAN(0);
    mvhd.next_track_id  = BIG4_ENDIAN(3);

    mp4_seek(msg, offset, SEEK_SET);
    msg->mvhd_duration_offset = mp4_tell(msg) + ((uint32_t) &(mvhd.duration) - (uint32_t) &mvhd);
    mp4_write(msg, &mvhd, 1, sizeof(mvhd));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_trak_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_trak trak;
    uint32_t now_offset = mp4_tell(msg);
    trak.size           = BIG4_ENDIAN(now_offset - offset);
    memcpy(trak.boxname, "trak", 4);
    mp4_seek(msg, offset, SEEK_SET);
    mp4_write(msg, &trak, 1, sizeof(trak));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_tkhd_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_tkhd tkhd;
    memset(&tkhd, 0, sizeof(tkhd));
    tkhd.size = BIG4_ENDIAN(sizeof(tkhd));
    memcpy(tkhd.boxname, "tkhd", 4);

    tkhd.flags             = BIG3_ENDIAN(7);
    tkhd.creation_time     = BIG4_ENDIAN(0);
    tkhd.modification_time = BIG4_ENDIAN(0);
    tkhd.track_ID          = BIG4_ENDIAN(msg->trak_count);
    tkhd.duration          = BIG4_ENDIAN(19985);
    tkhd.volume            = BIG2_ENDIAN(0x0100);

    tkhd.matrix[0] = BIG4_ENDIAN(0x00010000);
    tkhd.matrix[1] = BIG4_ENDIAN(0);
    tkhd.matrix[2] = BIG4_ENDIAN(0);
    tkhd.matrix[3] = BIG4_ENDIAN(0);
    tkhd.matrix[4] = BIG4_ENDIAN(0x00010000);
    tkhd.matrix[5] = BIG4_ENDIAN(0);
    tkhd.matrix[6] = BIG4_ENDIAN(0);
    tkhd.matrix[7] = BIG4_ENDIAN(0);
    tkhd.matrix[8] = BIG4_ENDIAN(0x40000000);

    if (msg->trak_count == 1)
    {
        // 视频的长宽
        uint32_t width  = msg->video_w << 16;
        uint32_t height = msg->video_h << 16;
        tkhd.width      = BIG4_ENDIAN(width);
        tkhd.height     = BIG4_ENDIAN(height);
    }
    else
    {
        // 音频
        tkhd.width  = BIG4_ENDIAN(0);
        tkhd.height = BIG4_ENDIAN(0);
    }
    mp4_seek(msg, offset, SEEK_SET);

    msg->trak[msg->trak_count - 1].tkhd_duration_offset = mp4_tell(msg) + ((uint32_t) &(tkhd.duration) - (uint32_t) &tkhd);
    mp4_write(msg, &tkhd, 1, sizeof(tkhd));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_mdia_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_mdia mdia;
    uint32_t now_offset = mp4_tell(msg);
    mdia.size           = BIG4_ENDIAN(now_offset - offset);
    memcpy(mdia.boxname, "mdia", 4);
    mp4_seek(msg, offset, SEEK_SET);
    mp4_write(msg, &mdia, 1, sizeof(mdia));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_mdhd_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_mdhd mdhd;
    memset(&mdhd, 0, sizeof(mdhd));
    mdhd.size = BIG4_ENDIAN(sizeof(mdhd));
    memcpy(mdhd.boxname, "mdhd", 4);

    mdhd.creation_time     = BIG4_ENDIAN(0);
    mdhd.modification_time = BIG4_ENDIAN(0);
    uint32_t track_index   = msg->trak_count > 0 ? msg->trak_count - 1 : 0;
    uint32_t timescale     = mp4_track_timescale(msg, track_index);
    mdhd.timescale = BIG4_ENDIAN(timescale);
    // 这个需要重新修改,要记录好位置
    mdhd.duration          = BIG4_ENDIAN(1798650);

    int lang_code = ((language[0] & 31) << 10) | ((language[1] & 31) << 5) | (language[2] & 31);
    mdhd.language = BIG2_ENDIAN(lang_code);

    mp4_seek(msg, offset, SEEK_SET);
    msg->trak[msg->trak_count - 1].mdhd_duration_offset = mp4_tell(msg) + ((uint32_t) &(mdhd.duration) - (uint32_t) &mdhd);
    mp4_write(msg, &mdhd, 1, sizeof(mdhd));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    // 需要记录当前的offset,后续结束的时候,需要修改这里的时间
    return 0;
}

uint32_t mp4_hdlr_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_hdlr hdlr;
    memset(&hdlr, 0, sizeof(hdlr));
    hdlr.size = BIG4_ENDIAN(sizeof(hdlr) + strlen(VIDEO_NAME) + 1);
    memcpy(hdlr.boxname, "hdlr", 4);
    if (msg->trak_count == 1)
    {
        memcpy(hdlr.handler_type, "vide", 4);
    }
    else if (msg->trak_count == 2)
    {
        memcpy(hdlr.handler_type, "soun", 4);
    }
    mp4_seek(msg, offset, SEEK_SET);
    mp4_write(msg, &hdlr, 1, sizeof(hdlr));
    if (msg->trak_count == 1)
    {
        mp4_write(msg, VIDEO_NAME, 1, strlen(VIDEO_NAME) + 1);
    }
    else if (msg->trak_count == 2)
    {
        mp4_write(msg, SOUND_NAME, 1, strlen(SOUND_NAME) + 1);
    }
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_minf_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_minf minf;
    uint32_t now_offset = mp4_tell(msg);
    memset(&minf, 0, sizeof(minf));
    minf.size = BIG4_ENDIAN(now_offset - offset);
    memcpy(minf.boxname, "minf", 4);
    mp4_seek(msg, offset, SEEK_SET);
    mp4_write(msg, &minf, 1, sizeof(minf));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_vmhd_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_vmhd vmhd;
    memset(&vmhd, 0, sizeof(vmhd));
    vmhd.size = BIG4_ENDIAN(sizeof(vmhd)); // 假参数,先填写,后续需要修正
    memcpy(vmhd.boxname, "vmhd", 4);
    vmhd.flags = BIG3_ENDIAN(1);
    mp4_seek(msg, offset, SEEK_SET);
    mp4_write(msg, &vmhd, 1, sizeof(vmhd));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_dinf_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_dinf dinf;
    uint32_t now_offset = mp4_tell(msg);
    memset(&dinf, 0, sizeof(dinf));
    dinf.size = BIG4_ENDIAN(now_offset - offset);
    memcpy(dinf.boxname, "dinf", 4);
    mp4_seek(msg, offset, SEEK_SET);
    mp4_write(msg, &dinf, 1, sizeof(dinf));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_dref_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_dref dref;
    uint32_t now_offset = mp4_tell(msg);
    memset(&dref, 0, sizeof(dref));
    dref.size = BIG4_ENDIAN(now_offset - offset);
    memcpy(dref.boxname, "dref", 4);
    dref.entry_count = BIG4_ENDIAN(1);
    mp4_seek(msg, offset, SEEK_SET);
    mp4_write(msg, &dref, 1, sizeof(dref));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_url_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_url url;
    memset(&url, 0, sizeof(url));
    url.size = BIG4_ENDIAN(sizeof(url));
    memcpy(url.boxname, "url ", 4);
    url.flags = BIG3_ENDIAN(1);
    mp4_seek(msg, offset, SEEK_SET);
    mp4_write(msg, &url, 1, sizeof(url));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_stbl_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_stbl stbl;
    uint32_t now_offset = mp4_tell(msg);
    memset(&stbl, 0, sizeof(stbl));
    stbl.size = BIG4_ENDIAN(now_offset - offset);
    memcpy(stbl.boxname, "stbl", 4);
    mp4_seek(msg, offset, SEEK_SET);
    mp4_write(msg, &stbl, 1, sizeof(stbl));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_stsd_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_stsd stsd;
    uint32_t now_offset = mp4_tell(msg);
    memset(&stsd, 0, sizeof(stsd));
    stsd.size = BIG4_ENDIAN(now_offset - offset);
    memcpy(stsd.boxname, "stsd", 4);
    stsd.entry_count = BIG4_ENDIAN(1);
    mp4_seek(msg, offset, SEEK_SET);
    mp4_write(msg, &stsd, 1, sizeof(stsd));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_avc1_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_avc1 avc1;
    uint32_t now_offset = mp4_tell(msg);
    memset(&avc1, 0, sizeof(avc1));
    avc1.size = BIG4_ENDIAN(now_offset - offset);
    memcpy(avc1.boxname, "avc1", 4);

    // 需要修改
    avc1.data_reference_index = BIG2_ENDIAN(1);
    avc1.width                = BIG2_ENDIAN(msg->video_w);
    avc1.height               = BIG2_ENDIAN(msg->video_h);
    avc1.horiz_resolution     = BIG4_ENDIAN(0x00480000);
    avc1.vert_resolution      = BIG4_ENDIAN(0x00480000);
    avc1.frame_count          = BIG2_ENDIAN(1);
    avc1.depth                = BIG2_ENDIAN(0x0018);
    avc1.pre_defined2[0]      = BIG1_ENDIAN(0xff);
    avc1.pre_defined2[1]      = BIG1_ENDIAN(0xff);

    mp4_seek(msg, offset, SEEK_SET);
    mp4_write(msg, &avc1, 1, sizeof(avc1));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_avcC_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_avcC avcc;
    uint8_t  offset1 = 0;
    uint8_t  len     = 1 + 2 + msg->sps_len + 1 + 2 + msg->pps_len;
    memset(&avcc, 0, sizeof(avcc));
    avcc.size = BIG4_ENDIAN(sizeof(avcc) + len); // 假参数,先填写,后续需要修正
    memcpy(avcc.boxname, "avcC", 4);

    avcc.configurationVersion  = BIG1_ENDIAN(1);
    avcc.AVCProfileIndication  = BIG1_ENDIAN(msg->sps_data[1]);
    avcc.profile_compatibility = BIG1_ENDIAN(msg->sps_data[2]);
    avcc.AVCLevelIndication    = BIG1_ENDIAN(msg->sps_data[3]);
    avcc.lengthSizeMinusOne    = BIG1_ENDIAN(0xff);

    uint8_t *sps_pps   = (uint8_t *) STREAM_LIBC_MALLOC(len);
    sps_pps[offset1++] = BIG1_ENDIAN(1 | 0xe0);
    uint16_t numOf     = BIG2_ENDIAN(msg->sps_len);
    memcpy(&sps_pps[offset1], &numOf, 2);
    offset1 += 2;
    memcpy(&sps_pps[offset1], msg->sps_data, msg->sps_len);
    offset1 += msg->sps_len;

    sps_pps[offset1++] = BIG1_ENDIAN(1);
    numOf              = BIG2_ENDIAN(msg->pps_len);
    memcpy(&sps_pps[offset1], &numOf, 2);
    offset1 += 2;

    memcpy(&sps_pps[offset1], msg->pps_data, msg->pps_len);
    offset1 += msg->pps_len;
    mp4_seek(msg, offset, SEEK_SET);
    mp4_write(msg, &avcc, 1, sizeof(avcc));
    mp4_write(msg, sps_pps, 1, len);
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    if (sps_pps)
    {
        STREAM_LIBC_FREE(sps_pps);
    }
    return 0;
}

uint32_t mp4_colr_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_colr colr;

    memset(&colr, 0, sizeof(colr));
    memcpy(colr.boxname, "colr", 4);
    memcpy(colr.colour_type, "nclc", 4);

    colr.size                     = BIG4_ENDIAN(sizeof(colr));
    colr.full_range_flag          = BIG1_ENDIAN(80);
    colr.colour_primaries         = BIG2_ENDIAN(1);
    colr.transfer_characteristics = BIG2_ENDIAN(1);
    colr.matrix_coefficients      = BIG2_ENDIAN(1);
    mp4_seek(msg, offset, SEEK_SET);
    mp4_write(msg, &colr, 1, sizeof(colr));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_stts_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_stts stts;
    memset(&stts, 0, sizeof(stts));
    stts.size = BIG4_ENDIAN((uint32_t) &stts.end - (uint32_t) &stts + STTS_COUNT * sizeof(stts.entries)); // 申请的最大值
    memcpy(stts.boxname, "stts", 4);
    stts.entry_count = BIG4_ENDIAN(0);
    mp4_seek(msg, offset, SEEK_SET);
    msg->trak[msg->trak_count - 1].stts_write_offset = msg->trak[msg->trak_count - 1].stts_offset = mp4_tell(msg) + ((uint32_t) &(stts.end) - (uint32_t) &stts);
    mp4_write(msg, &stts, 1, (uint32_t) &stts.end - (uint32_t) &stts);
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    // fseek预留足够空间,这里尽量往512对齐或者4K对齐
    mp4_truncate(msg, STTS_COUNT * sizeof(stts.end));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_stsc_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_stsc stsc;
    memset(&stsc, 0, sizeof(stsc));
    stsc.size = BIG4_ENDIAN((uint32_t) &stsc.end - (uint32_t) &stsc + STSC_COUNT * sizeof(stsc.end)); // 假参数,先填写,后续需要修正
    memcpy(stsc.boxname, "stsc", 4);
    stsc.entry_count = BIG4_ENDIAN(STSC_COUNT);
    mp4_seek(msg, offset, SEEK_SET);
    msg->trak[msg->trak_count - 1].stsc_write_offset = msg->trak[msg->trak_count - 1].stsc_offset = mp4_tell(msg) + ((uint32_t) &(stsc.end) - (uint32_t) &stsc);
    mp4_write(msg, &stsc, 1, (uint32_t) &stsc.end - (uint32_t) &stsc);

    stsc_entries entries;
    // stsc用默认值,不需要动态
    entries.first_chunk = BIG4_ENDIAN(1);
    entries.per_chunk   = BIG4_ENDIAN(1);
    entries.index       = BIG4_ENDIAN(1);
    mp4_write(msg, &entries, 1, sizeof(entries));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_stsz_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_stsz stsz;
    memset(&stsz, 0, sizeof(stsz));
    stsz.size = BIG4_ENDIAN((uint32_t) &stsz.end - (uint32_t) &stsz + STSZ_COUNT * sizeof(stsz.end)); // 假参数,先填写,后续需要修正
    memcpy(stsz.boxname, "stsz", 4);
    stsz.sample_size  = BIG4_ENDIAN(0);
    stsz.sample_count = BIG4_ENDIAN(0);
    mp4_seek(msg, offset, SEEK_SET);
    msg->trak[msg->trak_count - 1].stsz_write_offset = msg->trak[msg->trak_count - 1].stsz_offset = mp4_tell(msg) + ((uint32_t) &(stsz.end) - (uint32_t) &stsz);
    mp4_write(msg, &stsz, 1, (uint32_t) &stsz.end - (uint32_t) &stsz);
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);

    mp4_truncate(msg, STSZ_COUNT * sizeof(stsz.end));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_stco_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_stco stco;
    memset(&stco, 0, sizeof(stco));
    stco.size = BIG4_ENDIAN((uint32_t) &stco.end - (uint32_t) &stco + STCO_COUNT * sizeof(stco.end)); // 假参数,先填写,后续需要修正
    memcpy(stco.boxname, "stco", 4);
    stco.entry_count = BIG4_ENDIAN(0);
    mp4_seek(msg, offset, SEEK_SET);
    msg->trak[msg->trak_count - 1].stco_write_offset = msg->trak[msg->trak_count - 1].stco_offset = mp4_tell(msg) + ((uint32_t) &(stco.end) - (uint32_t) &stco);
    mp4_write(msg, &stco, 1, (uint32_t) &stco.end - (uint32_t) &stco);
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);

    // fseek预留足够空间
    mp4_truncate(msg, STCO_COUNT * sizeof(stco.end));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_stss_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_stss stss;
    memset(&stss, 0, sizeof(stss));
    stss.size = BIG4_ENDIAN((uint32_t) &stss.end - (uint32_t) &stss + STSS_COUNT * sizeof(stss.end)); // 假参数,先填写,后续需要修正
    memcpy(stss.boxname, "stss", 4);
    stss.entry_count = BIG4_ENDIAN(0);
    mp4_seek(msg, offset, SEEK_SET);
    msg->trak[msg->trak_count - 1].stss_write_offset = msg->trak[msg->trak_count - 1].stss_offset = mp4_tell(msg) + ((uint32_t) &(stss.end) - (uint32_t) &stss);
    mp4_write(msg, &stss, 1, (uint32_t) &stss.end - (uint32_t) &stss);
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    // fseek预留足够空间
    mp4_truncate(msg, STSS_COUNT * sizeof(stss.end));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_smhd_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_smhd smhd;
    memset(&smhd, 0, sizeof(smhd));
    smhd.size = BIG4_ENDIAN(sizeof(mp4_smhd));
    memcpy(smhd.boxname, "smhd", 4);
    smhd.balance  = BIG2_ENDIAN(0);
    smhd.reserved = BIG2_ENDIAN(0);
    mp4_seek(msg, offset, SEEK_SET);
    mp4_write(msg, &smhd, 1, sizeof(smhd));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

uint32_t mp4_mp4a_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_mp4a mp4a;
    memset(&mp4a, 0, sizeof(mp4_mp4a));
    uint32_t now_offset = mp4_tell(msg);
    mp4a.size           = BIG4_ENDIAN(now_offset - offset);
    memcpy(mp4a.boxname, "mp4a", 4);
    mp4a.dataReferenceIndex = BIG2_ENDIAN(1);
    mp4a.channelCount       = BIG2_ENDIAN(1);
    mp4a.sampleSize         = BIG2_ENDIAN(16);
    // ISO 14496-12: AudioSampleEntry的sampleRate是16.16定点数(FixedPoint32)
    // 写入 samplerate << 16, BIG4_ENDIAN 确保大端字节序写入
    uint32_t sr = msg->audio_samplerate ? msg->audio_samplerate : 8000;
    mp4a.time_scale         = BIG4_ENDIAN(sr << 16);
    mp4_seek(msg, offset, SEEK_SET);
    mp4_write(msg, &mp4a, 1, sizeof(mp4a));
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

/**
*   calculate size of length field of OD box
    这里默认传入size是小于0x7F,没有做兼容,打印报错
*/
static int od_size_of_size(int size)
{
    if (size > 0x7f)
    {
        os_printf(KERN_ERR "%s:%d err,size:%X\n", __FUNCTION__, __LINE__, size);
    }
    int i, size_of_size = 1;
    for (i = size; i > 0x7F; i -= 0x7F)
    {
        size_of_size++;
    }
    return size_of_size;
}

uint32_t mp4_esds_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_esds esds;
    memset(&esds, 0, sizeof(mp4_esds));

    memcpy(esds.boxname, "esds", 4);

    // 写入esds的额外参数
    if (msg->asps_len)
    {
        int dsi_bytes     = msg->asps_len; //  - two bytes size field
        int dsi_size_size = od_size_of_size(dsi_bytes);
        int dcd_bytes     = dsi_bytes + dsi_size_size + 1 + (1 + 1 + 3 + 4 + 4);
        int dcd_size_size = od_size_of_size(dcd_bytes);
        int esd_bytes     = dcd_bytes + dcd_size_size + 1 + 3;

        uint8_t  OD_ESD          = BIG1_ENDIAN(3);
        uint8_t  esd_bytes_w     = BIG1_ENDIAN(esd_bytes);
        uint16_t ES_ID           = BIG2_ENDIAN(0);
        uint8_t  flags           = BIG1_ENDIAN(0);
        uint8_t  OD_DCD          = BIG1_ENDIAN(4);
        uint8_t  dcd_bytes_w     = BIG1_ENDIAN(dcd_bytes);
        uint8_t  OD_DCD_data     = BIG1_ENDIAN(MP4_OBJECT_TYPE_AUDIO_ISO_IEC_14496_3);
        uint8_t  stream_type     = BIG1_ENDIAN(5 << 2);
        uint8_t  bufferSizeDB[3] = {0x00, 0x30, 0x00}; // channelcount * 6144/8,这里写死先,就是单声道
        uint32_t zero_data       = BIG4_ENDIAN(0);
        uint8_t  OD_DSI          = BIG1_ENDIAN(5);
        uint8_t  dsi_bytes_w     = BIG1_ENDIAN(dsi_bytes);
        uint8_t *aps_data        = msg->asps_data;

        esds.size = BIG4_ENDIAN(sizeof(mp4_esds) + esd_bytes + 2);
        mp4_seek(msg, offset, SEEK_SET);
        mp4_write(msg, &esds, 1, sizeof(esds));

        mp4_write(msg, &OD_ESD, 1, sizeof(OD_ESD));
        mp4_write(msg, &esd_bytes_w, 1, sizeof(esd_bytes_w));
        mp4_write(msg, &ES_ID, 1, sizeof(ES_ID));
        mp4_write(msg, &flags, 1, sizeof(flags));
        mp4_write(msg, &OD_DCD, 1, sizeof(OD_DCD));
        mp4_write(msg, &dcd_bytes_w, 1, sizeof(dcd_bytes_w));
        mp4_write(msg, &OD_DCD_data, 1, sizeof(OD_DCD_data));
        mp4_write(msg, &stream_type, 1, sizeof(stream_type));
        mp4_write(msg, bufferSizeDB, 1, sizeof(bufferSizeDB));
        mp4_write(msg, &zero_data, 1, sizeof(zero_data));
        mp4_write(msg, &zero_data, 1, sizeof(zero_data));
        mp4_write(msg, &OD_DSI, 1, sizeof(OD_DSI));
        mp4_write(msg, &dsi_bytes_w, 1, sizeof(dsi_bytes_w));
        mp4_write(msg, aps_data, 1, msg->asps_len);
    }
    else
    {
        esds.size = BIG4_ENDIAN(sizeof(mp4_esds));
        mp4_seek(msg, offset, SEEK_SET);
        mp4_write(msg, &esds, 1, sizeof(esds));
    }
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);

    return 0;
}

uint32_t mp4_mdat_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_mdat mdat;
    uint32_t max_mdata_size = msg->file_max_size;
    uint32_t mdat_data_offset;
    memcpy(mdat.boxname, "mdat", 4);
    mp4_seek(msg, offset, SEEK_SET);
    msg->msg_end     = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    msg->mdat_offset = mp4_tell(msg);
    mdat_data_offset = muxer_file_align_up(msg->mdat_offset + sizeof(mdat));
    if (max_mdata_size && max_mdata_size <= mdat_data_offset)
    {
        return 1;
    }

    mdat.size = BIG4_ENDIAN(max_mdata_size ? (max_mdata_size - msg->mdat_offset) : sizeof(mdat));
    // 记录mdat_size
    if (mp4_write(msg, &mdat, 1, sizeof(mdat)))
    {
        return 1;
    }
    msg->mdat_nowoffset = mdat_data_offset;
    msg->mdat_size      = max_mdata_size ? (max_mdata_size - msg->mdat_offset) : sizeof(mdat);
    if (!msg->ops_init || !msg->ops.skip || msg->ops.skip(msg->ops.file, mdat_data_offset - msg->io_offset))
    {
        return 1;
    }
    msg->io_offset = mdat_data_offset;
    if (msg->msg_end < msg->io_offset)
    {
        msg->msg_end = msg->io_offset;
    }
    msg->mdat_writer_init = 1;
    if (max_mdata_size)
    {
        msg->io_offset = mdat_data_offset;
    }
    else
    {
        mp4_seek(msg, mdat_data_offset, SEEK_SET);
    }
    msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    return 0;
}

// 这个主要是为了对齐作用
uint32_t mp4_free_write(F_FILE *fp, uint32_t offset, mp4_key_msg *msg)
{
    mp4_free _free;
    uint32_t align_size;
    memcpy(_free.boxname, "free", 4);
    mp4_seek(msg, offset, SEEK_SET);
    align_size = 0x200 - mp4_tell(msg) % 512;
    if (align_size)
    {
        _free.size = BIG4_ENDIAN(align_size);
        mp4_write(msg, &_free, 1, sizeof(_free));
        msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
        mp4_truncate(msg, align_size - sizeof(_free));
        msg->msg_end = msg->msg_end > mp4_tell(msg) ? msg->msg_end : mp4_tell(msg);
    }
    return 0;
}

uint32_t mp4_sync(mp4_key_msg *msg)
{
    uint32_t      ret          = 0;
    uint32_t      max_duration = 0;
    uint32_t      sync_duration;
    uint32_t      nowoffset;
    // 先尝试同步视频
    trak_key_msg *vtrak;

    if (!msg || !msg->fp || !msg->init)
    {
        return 0;
    }

    nowoffset = mp4_tell(msg);
    for (int i = 0; i < msg->trak_count; i++)
    {
        vtrak = &msg->trak[i];
        if (vtrak->stts_need_write_len || vtrak->last_entries.sample_count > 0)
        {
            uint32_t stts_count = vtrak->stts_count;
            if (stts_count >= STTS_COUNT)
            {
                ret++;
            }
            mp4_seek(msg, vtrak->stts_write_offset, SEEK_SET);
            if (vtrak->stts_need_write_len)
            {
                mp4_write(msg, vtrak->stts_tmp_buf, 1, vtrak->stts_need_write_len);
                vtrak->stts_write_offset += vtrak->stts_need_write_len;
                vtrak->stts_need_write_len = 0;
            }
            if (vtrak->last_entries.sample_count > 0)
            {
                stts_entries entries;
                entries.delta        = BIG4_ENDIAN(vtrak->last_entries.delta);
                entries.sample_count = BIG4_ENDIAN(vtrak->last_entries.sample_count);
                // 将最后一次的stts写入到文件(大部分情况是覆盖上一个目录条)
                ret |= mp4_write(msg, &entries, 1, sizeof(entries));
                stts_count++;
            }

            // 回写对应的count值
            stts_count = BIG4_ENDIAN(stts_count);
            mp4_seek(msg, vtrak->stts_offset - 4, SEEK_SET);
            ret |= mp4_write(msg, &stts_count, 1, sizeof(stts_count));
        }
        if (vtrak->stsc_need_write_len)
        {
            if (vtrak->stsc_count >= STSC_COUNT)
            {
                ret++;
            }

            mp4_seek(msg, vtrak->stsc_write_offset, SEEK_SET);
            mp4_write(msg, vtrak->stsc_tmp_buf, 1, vtrak->stsc_need_write_len);
            vtrak->stsc_write_offset += vtrak->stsc_need_write_len;
            vtrak->stsc_need_write_len = 0;
            // 回写对应的count值
            uint32_t stsc_count        = BIG4_ENDIAN(vtrak->stsc_count);
            mp4_seek(msg, vtrak->stsc_offset - 4, SEEK_SET);
            ret |= mp4_write(msg, &stsc_count, 1, sizeof(stsc_count));
        }

        if (vtrak->stsz_need_write_len)
        {
            if (vtrak->stsz_count >= STSZ_COUNT)
            {
                ret++;
            }
            mp4_seek(msg, vtrak->stsz_write_offset, SEEK_SET);
            mp4_write(msg, vtrak->stsz_tmp_buf, 1, vtrak->stsz_need_write_len);
            vtrak->stsz_write_offset += vtrak->stsz_need_write_len;
            vtrak->stsz_need_write_len = 0;

            // 回写对应的count值
            uint32_t stsz_count = BIG4_ENDIAN(vtrak->stsz_count);
            mp4_seek(msg, vtrak->stsz_offset - 4, SEEK_SET);
            ret |= mp4_write(msg, &stsz_count, 1, sizeof(stsz_count));
        }

        if (vtrak->stco_need_write_len)
        {
            if (vtrak->stco_count >= STCO_COUNT)
            {
                ret++;
            }
            mp4_seek(msg, vtrak->stco_write_offset, SEEK_SET);
            ret |= mp4_write(msg, vtrak->stco_tmp_buf, 1, vtrak->stco_need_write_len);
            vtrak->stco_write_offset += vtrak->stco_need_write_len;
            vtrak->stco_need_write_len = 0;

            // 回写对应的count值
            uint32_t stco_count = BIG4_ENDIAN(vtrak->stco_count);
            mp4_seek(msg, vtrak->stco_offset - 4, SEEK_SET);
            ret |= mp4_write(msg, &stco_count, 1, sizeof(stco_count));
        }

        if (vtrak->stss_need_write_len)
        {
            if (vtrak->stss_count >= STSS_COUNT)
            {
                ret++;
            }
            mp4_seek(msg, vtrak->stss_write_offset, SEEK_SET);
            ret |= mp4_write(msg, vtrak->stss_tmp_buf, 1, vtrak->stss_need_write_len);
            vtrak->stss_write_offset += vtrak->stss_need_write_len;
            vtrak->stss_need_write_len = 0;

            // 回写对应的count值
            uint32_t stss_count = BIG4_ENDIAN(vtrak->stss_count);
            mp4_seek(msg, vtrak->stss_offset - 4, SEEK_SET);
            ret |= mp4_write(msg, &stss_count, 1, sizeof(stss_count));
        }
        // 将时间updata到文件
        uint32_t duration_ms   = mp4_track_duration_ms(msg, i);
        uint32_t mdhd_duration = BIG4_ENDIAN(vtrak->duration_ticks);
        uint32_t duration      = BIG4_ENDIAN(duration_ms);
        mp4_seek(msg, vtrak->mdhd_duration_offset, SEEK_SET);
        ret |= mp4_write(msg, &mdhd_duration, 1, sizeof(mdhd_duration));

        mp4_seek(msg, vtrak->tkhd_duration_offset, SEEK_SET);
        ret |= mp4_write(msg, &duration, 1, sizeof(duration));

        max_duration = max_duration > duration_ms ? max_duration : duration_ms;
    }

    os_printf(KERN_INFO "max_duration:%d\n", max_duration);
    sync_duration = max_duration;
    mp4_seek(msg, msg->mvhd_duration_offset, SEEK_SET);
    max_duration = BIG4_ENDIAN(max_duration);
    ret |= mp4_write(msg, &max_duration, 1, sizeof(max_duration));

    // 检查一下文件长度是否要更新
    if (msg->mdat_nowoffset - msg->mdat_offset > msg->mdat_size)
    {
        msg->mdat_size     = msg->mdat_nowoffset - msg->mdat_offset;
        uint32_t mdat_size = BIG4_ENDIAN(msg->mdat_size);
        mp4_seek(msg, msg->mdat_offset, SEEK_SET);
        ret |= mp4_write(msg, &mdat_size, 1, sizeof(mdat_size));
    }

    mp4_file_syn(msg);
    if (!ret)
    {
        msg->sync_time = sync_duration;
    }
    mp4_seek(msg, nowoffset, SEEK_SET);
    return ret;
}

uint32_t mp4_sync_time(mp4_key_msg *msg, uint32_t time_ms)
{
    uint32_t written_duration;

    if (!msg || !msg->fp)
    {
        return 0;
    }

    if (time_ms == 0)
    {
        return mp4_sync(msg);
    }

    written_duration = mp4_written_duration_ms(msg);
    if (written_duration < msg->sync_time || written_duration - msg->sync_time >= time_ms)
    {
        return mp4_sync(msg);
    }

    return 0;
}

static uint8_t mp4_update_sample_tables(trak_key_msg *vtrak, uint32_t sample_size, uint32_t sample_offset, uint32_t delta)
{
    uint8_t  flag   = 0;

    vtrak->count++;
    if (vtrak->last_entries.delta != 0 && vtrak->last_entries.delta != delta)
    {
        stts_entries *entries = (stts_entries *) (vtrak->stts_tmp_buf + vtrak->stts_need_write_len);
        entries->sample_count = BIG4_ENDIAN(vtrak->last_entries.sample_count);
        entries->delta        = BIG4_ENDIAN(vtrak->last_entries.delta);

        vtrak->stts_need_write_len += sizeof(vtrak->last_entries);
        vtrak->last_entries.sample_count = 0;
        vtrak->stts_count++;
    }
    vtrak->last_entries.delta = delta;
    vtrak->last_entries.sample_count++;
    if (vtrak->stts_need_write_len >= 512 || vtrak->stts_count >= STTS_COUNT)
    {
        flag = 1;
    }

    uint32_t *stsz_sample_size = (uint32_t *) (vtrak->stsz_tmp_buf + vtrak->stsz_need_write_len);
    *stsz_sample_size          = BIG4_ENDIAN(sample_size);
    vtrak->stsz_need_write_len += sizeof(*stsz_sample_size);
    vtrak->stsz_count++;
    if (vtrak->stsz_need_write_len >= 512 || vtrak->stsz_count >= STSZ_COUNT)
    {
        flag = 1;
    }

    uint32_t *stco_offset = (uint32_t *) (vtrak->stco_tmp_buf + vtrak->stco_need_write_len);
    *stco_offset          = BIG4_ENDIAN(sample_offset);
    vtrak->stco_need_write_len += sizeof(*stco_offset);
    vtrak->stco_count++;
    if (vtrak->stco_need_write_len >= 512 || vtrak->stco_count >= STCO_COUNT)
    {
        flag = 1;
    }

    return flag;
}

static uint32_t mp4_update_video_sample(mp4_key_msg *msg, uint32_t size, uint32_t duration_ticks, int keyflag)
{
    // 更新stts
    uint32_t      ret   = 0;
    uint8_t       flag  = 0;
    trak_key_msg *vtrak = &msg->trak[0];

    vtrak->duration_ticks += duration_ticks;
    flag = mp4_update_sample_tables(vtrak, size, msg->mdat_nowoffset, duration_ticks);
    msg->mdat_nowoffset += size;

    // 如果是关键帧则更新stss
    if (keyflag)
    {
        // 更新stss
        uint32_t *stss_count = (uint32_t *) (vtrak->stss_tmp_buf + vtrak->stss_need_write_len);
        *stss_count          = BIG4_ENDIAN(vtrak->count);
        vtrak->stss_need_write_len += sizeof(*stss_count);
        vtrak->stss_count++;
        // 如果大于512,则写入到sd卡
        if (vtrak->stss_need_write_len >= 512 || vtrak->stss_count >= STSS_COUNT)
        {
            flag = 1;
        }
    }

    if (flag)
    {
        ret |= mp4_sync(msg);
    }
    return ret;
}

static uint32_t mp4_update_audio_sample(mp4_key_msg *msg, uint32_t size, uint32_t duration_ticks)
{
    uint32_t      ret   = 0;
    uint8_t       flag  = 0;
    trak_key_msg *vtrak = &msg->trak[1];
    vtrak->duration_ticks += duration_ticks;
    flag = mp4_update_sample_tables(vtrak, size, msg->mdat_nowoffset, duration_ticks);
    msg->mdat_nowoffset += (size);

    if (flag)
    {
        ret |= mp4_sync(msg);
    }
    return ret;
}

static uint32_t write_h264_nal(mp4_key_msg *msg, uint8_t *nal_buf, uint32_t size, uint32_t duration_ticks, int keyflag)
{
    uint32_t ret = 0;
    uint32_t sample_size = size + 4;

    if (msg->file_max_size > 0 && (msg->mdat_nowoffset - msg->mdat_offset + sample_size > msg->mdat_size))
    {
        ret |= (MP4_FULL_ERR << 16);
        goto write_h264_nal_end;
    }
    uint32_t big_size = BIG4_ENDIAN(size);
    ret |= mp4_mdat_write_data(msg, &big_size, sizeof(big_size));
    ret |= mp4_mdat_write_data(msg, nal_buf, size);
    if (ret)
    {
        ret |= (MP4_WRITE_ERR << 16);
        goto write_h264_nal_end;
    }
    ret |= mp4_update_video_sample(msg, sample_size, duration_ticks, keyflag);
    if (ret)
    {
        ret |= (MP4_WRITE_ERR << 16);
    }
write_h264_nal_end:
    return ret;
}

uint32_t write_aac_data(mp4_key_msg *msg, uint8_t *aac_buf, uint32_t size, uint32_t duration_ticks)
{
    uint32_t ret = 0;
    if (msg->trak_count < 2 || !msg->audio_enable)
    {
        ret |= (MP4_AUDIO_ERR << 16);
        goto write_aac_data_end;
    }
    else
    {
        if (msg->file_max_size > 0 && (msg->mdat_nowoffset - msg->mdat_offset + size > msg->mdat_size))
        {
            ret |= (MP4_FULL_ERR << 16);
            goto write_aac_data_end;
        }
        ret |= mp4_mdat_write_data(msg, aac_buf, size);
        if (ret)
        {
            ret |= (MP4_WRITE_ERR << 16);
            goto write_aac_data_end;
        }
        // 更新box
        ret |= mp4_update_audio_sample(msg, size, duration_ticks);
        if (ret)
        {
            ret |= (MP4_WRITE_ERR << 16);
        }
    }

write_aac_data_end:
    return ret;
}

// 写入sps和pps,写完后,就可以将index预分配好
// 这个接口期待给的就是对应nal的数据,如果为了效率,这个接口就不再去检查数据是否正确
/*******************************************************************
 * nal_buf:nal的头数据(包含00 00 00 01)
 * nal_head_size: 就是nal头部的长度 00 00 00 01就是4,00 00 01就是3
 * nal_size: nal的长度,包含nal_head_size
 *******************************************************************/
uint32_t write_aac_data_batch(mp4_key_msg *msg, uint8_t *aac_buf, uint32_t total_size,
                              uint32_t *sizes, uint32_t *duration_ticks, uint32_t frame_count)
{
    uint32_t ret               = 0;
    uint32_t base_offset       = 0;
    uint32_t cumulative_offset = 0;
    uint8_t  flag              = 0;

    if (frame_count == 0)
    {
        goto write_aac_data_batch_end;
    }

    if (msg->trak_count < 2 || !msg->audio_enable)
    {
        ret |= (MP4_AUDIO_ERR << 16);
        goto write_aac_data_batch_end;
    }

    if (msg->file_max_size > 0 && (msg->mdat_nowoffset - msg->mdat_offset + total_size > msg->mdat_size))
    {
        ret |= (MP4_FULL_ERR << 16);
        goto write_aac_data_batch_end;
    }

    trak_key_msg *vtrak = &msg->trak[1];

    if (vtrak->stts_need_write_len + frame_count * sizeof(stts_entries) > sizeof(vtrak->stts_tmp_buf) ||
        vtrak->stsz_need_write_len + frame_count * sizeof(uint32_t) > sizeof(vtrak->stsz_tmp_buf) ||
        vtrak->stco_need_write_len + frame_count * sizeof(uint32_t) > sizeof(vtrak->stco_tmp_buf))
    {
        ret |= mp4_sync(msg);
        if (ret)
        {
            goto write_aac_data_batch_end;
        }
    }

    ret |= mp4_mdat_write_data(msg, aac_buf, total_size);
    if (ret)
    {
        ret |= (MP4_WRITE_ERR << 16);
        goto write_aac_data_batch_end;
    }

    base_offset = msg->mdat_nowoffset;
    for (uint32_t i = 0; i < frame_count; i++)
    {
        vtrak->duration_ticks += duration_ticks[i];
        flag |= mp4_update_sample_tables(vtrak, sizes[i], base_offset + cumulative_offset, duration_ticks[i]);
        cumulative_offset += sizes[i];
    }

    msg->mdat_nowoffset += total_size;
    if (flag)
    {
        ret |= mp4_sync(msg);
    }

write_aac_data_batch_end:
    return ret;
}

uint32_t write_h264_pps_sps(mp4_key_msg *msg, uint8_t *nal_buf, uint32_t size)
{
    F_FILE  *fp            = msg->fp;
    uint32_t ret           = 1;
    uint8_t  pps_sps_times = 0;
    uint8_t *next_nal_buf  = nal_buf;
    uint8_t *sps_pps_buf   = nal_buf;
    uint8_t  pps_len = 0, sps_len = 0;
    uint8_t  nal_head_size;
    uint8_t *pps_buf = NULL;
    uint8_t *sps_buf = NULL;
    uint32_t nal_size;
    if (!msg->init)
    {
        uint32_t head_start_time = os_jiffies();
        while (sps_pps_buf && pps_sps_times < 2)
        {
            sps_pps_buf = get_sps_pps_nal_size(next_nal_buf, 64, &nal_size, &nal_head_size);
            if (sps_pps_buf && (sps_pps_buf[nal_head_size] & 0x1f) == 7)
            {
                sps_buf      = sps_pps_buf + nal_head_size;
                sps_len      = nal_size;
                next_nal_buf = sps_pps_buf + nal_size + nal_head_size;
            }
            else if (sps_pps_buf && (sps_pps_buf[nal_head_size] & 0x1f) == 8)
            {
                pps_buf      = sps_pps_buf + nal_head_size;
                pps_len      = nal_size;
                next_nal_buf = sps_pps_buf + nal_size + nal_head_size;
            }
            // 不匹配,就不再去获取pps或者sps了
            else
            {
                break;
            }
            // os_printf("sps_pps_buf[nal_head_size]& 0x1f):%d\n",sps_pps_buf[nal_head_size]& 0x1f);
            pps_sps_times++;
        }
        // 符合要求
        if (pps_buf && sps_buf)
        {
            // 初始化index
            msg->pps_data = pps_buf;
            msg->sps_data = sps_buf;
            msg->pps_len  = pps_len;
            msg->sps_len  = sps_len;
            ret           = _MP4_init(fp, msg);
            if (!ret)
            {
                msg->init     = 1;
                msg->sync_time = mp4_written_duration_ms(msg);
            }
            else
            {
                mp4_reset_init_state(msg);
                if (msg->ops_init && msg->ops.tell)
                {
                    msg->io_offset = msg->ops.tell(msg->ops.file);
                }
            }
        }
        uint32_t head_end_time = os_jiffies();
        os_printf(KERN_INFO "pps sps spend time:%d\ttell:%X\n", head_end_time - head_start_time, osal_ftell(fp));
    }
    else
    {
        ret = 0;
    }

    return ret;
}

// 默认给的数据就是h264的数据,不再进行搜索
uint32_t write_h264_data(mp4_key_msg *msg, uint8_t *nal_buf, uint32_t size, uint32_t duration_ticks)
{
    uint32_t ret        = MP4_OK;
    uint32_t start_time = os_jiffies();
    if (nal_buf[4] == 0x61)
    {
        ret |= write_h264_nal(msg, (uint8_t *) &nal_buf[4], size - 4, duration_ticks, 0);
        if (ret)
        {
            ret |= (MP4_P_ERR << 16);
        }
    }
    else if (nal_buf[4] == 0x65)
    {
        ret |= write_h264_nal(msg, (uint8_t *) &nal_buf[4], size - 4, duration_ticks, 1);
        if (ret)
        {
            ret |= (MP4_I_ERR << 16);
        }
    }
    else
    {
        ret |= (MP4_V_TYPE_ERR << 16);
    }
    uint32_t end_time = os_jiffies();
    if (end_time - start_time > 500)
    {
        os_printf(KERN_INFO "%s:%d\tspend time:%d\n", __FUNCTION__, __LINE__, end_time - start_time);
    }
    return ret;
}

uint32_t mp4_deinit(mp4_key_msg *msg)
{
    uint32_t ret = 0;

    if (!msg)
    {
        return 0;
    }

    if (msg->init)
    {
        if (msg->mdat_writer_init)
        {
            msg->mdat_nowoffset = msg->io_offset;
            ret |= msg->ops.finish ? msg->ops.finish(msg->ops.file) : 1;
        }
        ret |= mp4_sync(msg);
    }

    if (msg->init && MP4_TRUNCATE_REAL_EN)
    {
        msg->mdat_size = msg->mdat_nowoffset - msg->mdat_offset;
        uint32_t mdat_size = BIG4_ENDIAN(msg->mdat_size);
        mp4_seek(msg, msg->mdat_offset, SEEK_SET);
        mp4_write(msg, &mdat_size, 1, sizeof(mdat_size));
        mp4_file_syn(msg);
#if MP4_TRUNCATE_REAL_EN
        mp4_seek(msg, msg->mdat_nowoffset, SEEK_SET);
        osal_ftruncate(msg->fp);                    // 裁掉后面没用空间
#endif
    }

    mp4_fast_seek_destroy(msg->fp, &msg->fast_seek_tbl);
    STREAM_FREE(msg);
    return ret;
}

uint32_t mp4_audio_cfg_init(mp4_key_msg *msg, uint8_t *asps_data, uint8_t len)
{
    msg->asps_data = asps_data;
    msg->asps_len  = len;
    return 0;
}

uint32_t mp4_set_max_size(mp4_key_msg *msg, uint32_t max_size)
{
    if (!msg)
    {
        return 1;
    }

    msg->file_max_size = max_size ? muxer_file_align_up(max_size) : 0;
    return 0;
}

uint32_t mp4_set_audio_samplerate(mp4_key_msg *msg, uint32_t samplerate)
{
    msg->audio_samplerate = samplerate;
    return 0;
}

uint32_t mp4_video_cfg_init(mp4_key_msg *msg, uint16_t w, uint16_t h)
{
    if (!msg->init)
    {
        msg->video_w = w;
        msg->video_h = h;
    }
    return 0;
}

// mp4初始化,没什么用,应该是预分配一下内存空间
// 因为其他数据实际的index需要获取到第一帧I帧才能知道分辨率,pps和sps
uint32_t mp4_set_file(mp4_key_msg *msg, const file_ops_t *ops)
{
    if (!msg || !ops)
    {
        return 1;
    }

    os_memcpy(&msg->ops, ops, sizeof(msg->ops));
    msg->ops_init = (msg->ops.file && msg->ops.write && msg->ops.skip &&
                     msg->ops.tell && msg->ops.write_at && msg->ops.flush &&
                     msg->ops.finish) ? 1 : 0;
    msg->io_offset = msg->ops_init && msg->ops.tell ? msg->ops.tell(msg->ops.file) : 0;
    return msg->ops_init ? 0 : 1;
}

void *mp4_open_init_with_file(F_FILE *fp, const file_ops_t *ops, uint8_t audio_en)
{
    mp4_key_msg *msg = STREAM_MALLOC(sizeof(mp4_key_msg));
    if (msg)
    {
        memset(msg, 0, sizeof(mp4_key_msg));
        msg->fp           = fp;
        msg->audio_enable = audio_en;
        if (ops)
        {
            if (mp4_set_file(msg, ops))
            {
                STREAM_FREE(msg);
                return NULL;
            }
        }
        else
        {
            file_ops_t default_ops;
            mp4_default_ops_init(fp, &default_ops);
            if (mp4_set_file(msg, &default_ops))
            {
                STREAM_FREE(msg);
                return NULL;
            }
        }
    }
    return (void *) msg;
}

void *MP4_open_init(F_FILE *fp, uint8_t audio_en)
{
    return mp4_open_init_with_file(fp, NULL, audio_en);
}

// clang-format off
static uint32_t _MP4_init(F_FILE *fp,mp4_key_msg *msg)
{
    mp4_reset_init_state(msg);
    if (msg->ops_init && msg->ops.tell)
    {
        msg->io_offset = msg->ops.tell(msg->ops.file);
    }
    if(msg->file_max_size)
    {
        pre_mp4_seek(fp, msg->file_max_size);
        msg->fast_seek_tbl = mp4_fast_seek_create(fp);
    }
    mp4_seek(msg,0,SEEK_SET);
    mp4_func_stack offset_stack[20];
    mp4_func_stack *stack = offset_stack;
    ATOM(ftyp)
    END_ATOM
    ATOM(moov)
        ATOM(mvhd)
        END_ATOM
        ATOM(trak)
            msg->trak_count++;
            ATOM(tkhd)
            END_ATOM
            ATOM(mdia)
                ATOM(mdhd)
                END_ATOM
                ATOM(hdlr)
                END_ATOM
                ATOM(minf)
                    ATOM(vmhd)
                    END_ATOM
                    ATOM(dinf)
                        ATOM(dref)
                            ATOM(url)
                            END_ATOM
                        END_ATOM
                    END_ATOM

                    ATOM(stbl)
                        ATOM(stsd)
                            ATOM(avc1)
                                ATOM(avcC)
                                END_ATOM
                                ATOM(colr)
                                END_ATOM
                            END_ATOM
                        END_ATOM
                        ATOM(free)
                        END_ATOM
                        ATOM(stts)
                        END_ATOM
                        ATOM(stsc)
                        END_ATOM
                        ATOM(stsz)
                        END_ATOM
                        ATOM(stco)
                        END_ATOM
                        ATOM(stss)
                        END_ATOM
                    END_ATOM

                END_ATOM
            END_ATOM
        END_ATOM

        if(msg->audio_enable)
        {
            ATOM(trak)
                msg->trak_count++;
                ATOM(tkhd)
                END_ATOM
                ATOM(mdia)
                    ATOM(mdhd)
                    END_ATOM
                    ATOM(hdlr)
                    END_ATOM
                    ATOM(minf)
                        ATOM(smhd)
                        END_ATOM
                        ATOM(dinf)
                            ATOM(dref)
                                ATOM(url)
                                END_ATOM
                            END_ATOM
                        END_ATOM

                        ATOM(stbl)
                            ATOM(stsd)
                                ATOM(mp4a)
                                   // msg->asps_len = 2;
                                    //msg->asps_data = g_asps_data;
                                    ATOM(esds)
                                    END_ATOM
                                END_ATOM
                            END_ATOM

                            ATOM(free)
                            END_ATOM
                            ATOM(stts)
                            END_ATOM
                            ATOM(stsc)
                            END_ATOM
                            ATOM(stsz)
                            END_ATOM
                            ATOM(stco)
                            END_ATOM
                        END_ATOM

                    END_ATOM
                END_ATOM
            END_ATOM
        }

    END_ATOM
    ATOM(mdat)
    END_ATOM
    return msg->mdat_writer_init ? 0 : 1;
}
// clang-format on
