#include "avimuxer.h"
#include "basic_include.h"
#include "fatfs/osal_file.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "stream_define.h"

const uint8_t avi_zero = 0;
#define CONTENT_MAX_SIZE (512 * 1024 * 1024)

#ifndef SEEK_SET
#define SEEK_SET 0 /* set file offset to offset */
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1 /* set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define SEEK_END 2 /* set file offset to EOF plus offset */
#endif


static uint32_t avi_tell(F_FILE *fp)
{
    return osal_ftell(fp);
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
    return !ret;
}

static F_FILE *avi_open(const char *filename, char *mode)
{
    return osal_fopen(filename, mode);
}

static void avi_close(F_FILE *fp)
{
    osal_fclose(fp);
}

static void avi_truncate(F_FILE *fp, uint32_t offset)
{
    avi_seek(fp, offset, SEEK_CUR); // 预留的的空间
    osal_ftruncate(fp);
}


static void avi_file_syn(F_FILE *fp)
{
    osal_fsync(fp);
}


// 结构体申请空间函数
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

#define AVI_AUDIO_FRAME (0 << 31)
#define AVI_VIDEO_FRAME (1 << 31)
#define AVI_KEY_FRAME   (1 << 30)
#define AVI_INSERT_FRAME   (1 << 29)

#ifndef offsetof
#define offsetof(type, member) ((size_t) &((type *) 0)->member)
#endif

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
    uint16_t  vrect_left;
    uint16_t  vrect_top;
    uint16_t  vrect_right;
    uint16_t  vrect_bottom;

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
    uint32_t *framesize_lst;
    uint32_t  framesize_fix;
    uint32_t  framesize_idx;
    uint32_t  framesize_max;
    uint32_t  movi_addr;
    uint32_t  movi_addr_end;
    uint32_t  cur_addr;
    uint32_t  frame_base_addr;
    uint32_t  max_size;
    uint32_t  curpos;

    uint32_t now_idx;
    uint32_t cache_idx;
    uint32_t total_idx;
    uint32_t frame_list[512 / 4];
    F_FILE  *fp;
    uint32_t extern_fp;

    char     riff[4];
    uint32_t riff_size;
    char     type_avi[4];

    char     hlist[4];
    uint32_t hlist_size;
    char     type_hdrl[4];

    char       avih[4];
    uint32_t   avih_size;
    AVI_HEADER avi_header;
#if 1 
    char          slist1[4];
    uint32_t      slist1_size;
    char          type_str1[4];

    char          strhdr1[4];
    uint32_t      strhdr1_size;
    STREAM_HEADER strhdr_audio;

    char          strfmt1[4];
    uint32_t      strfmt1_size;
    WAVE_FORMAT   strfmt_audio;
#endif

    char     slist2[4];
    uint32_t slist2_size;
    char     type_str2[4];

    char          strhdr2[4];
    uint32_t      strhdr2_size;
    STREAM_HEADER strhdr_video;

    char          strfmt2[4];
    uint32_t      strfmt2_size;
    BITMAP_FORMAT strfmt_video;

    char     mlist[4];
    uint32_t mlist_size;
    char     type_movi[4];

} AVI_FILE;
#pragma pack()


void *avimuxer_init2(void *fp,  uint32_t max_size, int w, int h, int frate, int gop, int h265, int sampnum)
{
    int       samprate = 8000, channels = 1, sampbits = 16;
    AVI_FILE *avi = STREAM_ZALLOC(1 * sizeof(AVI_FILE));
    if (!avi)
    {
        goto failed;
    }
    avi->extern_fp = 1;
    avi->fp        = (F_FILE *) fp;
    if (!avi->fp)
    {
        goto failed;
    }


    avi->max_size = max_size;
    memcpy(avi->avih, "avih", 4);
    avi->avih_size                     = sizeof(AVI_HEADER);
    avi->avi_header.microsec_per_frame = 1000000 / frate;
    avi->avi_header.maxbytes_per_Sec   = w * h * 3;
    avi->avi_header.flags              = AVIF_ISINTERLEAVED | AVIF_HASINDEX;
    avi->avi_header.number_streams     = 1;
    avi->avi_header.width              = w;
    avi->avi_header.height             = h;
    avi->avi_header.suggested_bufsize  = w * h * 3;
#if 1
    memcpy(avi->strhdr1, "strh", 4);
    memcpy(avi->strhdr_audio.fcc_type , "auds", 4);
    memcpy(avi->strhdr_audio.fcc_codec, "PCM ", 4);
    avi->strhdr1_size                   = sizeof(STREAM_HEADER);
    avi->strhdr_audio.scale             = 1;
    avi->strhdr_audio.rate              = samprate;
    avi->strhdr_audio.suggested_bufsize = samprate * channels * sampbits / 8;
    avi->strhdr_audio.sample_size       = channels * sampbits / 8;

    memcpy(avi->strfmt1, "strf", 4);
    avi->strfmt1_size                   = sizeof(WAVE_FORMAT);
    avi->strfmt_audio.format_tag        = 1;
    avi->strfmt_audio.channels          = channels;
    avi->strfmt_audio.sample_per_sec    = samprate;
    avi->strfmt_audio.avgbyte_per_sec   = samprate * channels * sampbits / 8;
    avi->strfmt_audio.block_align       = channels * sampbits / 8;
    avi->strfmt_audio.bits_per_sample   = sampbits;
    

    memcpy(avi->slist1   , "LIST", 4);
    memcpy(avi->type_str1, "strl", 4);
    avi->slist1_size =  offsetof(AVI_FILE, slist2) - offsetof(AVI_FILE, type_str1);
#endif

    memcpy(avi->strhdr2, "strh", 4);
    memcpy(avi->strhdr_video.fcc_type, "vids", 4);
    memcpy(avi->strhdr_video.fcc_codec, h265 ? "MJPG" : "MJPG", 4);
    avi->strhdr2_size                   = sizeof(STREAM_HEADER);
    avi->strhdr_video.scale             = 1;
    avi->strhdr_video.rate              = 25;
    avi->strhdr_video.suggested_bufsize = 0;
    avi->strhdr_video.quality           = -1;
    avi->strhdr_video.vrect_right = w;
    avi->strhdr_video.vrect_bottom = h;

    memcpy(avi->strfmt2, "strf", 4);
    avi->strfmt2_size             = sizeof(BITMAP_FORMAT);
    avi->strfmt_video.size        = 40;
    avi->strfmt_video.width       = w;
    avi->strfmt_video.height      = h;
    avi->strfmt_video.planes      = 1;
    avi->strfmt_video.bitcount    = 24;
    avi->strfmt_video.compression = h265 ? (('H' << 0) | ('E' << 8) | ('V' << 16) | ('1' << 24)) : (('M' << 0) | ('J' << 8) | ('P' << 16) | ('G' << 24));
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
    avi->hlist_size = sizeof(AVI_FILE) - 12 -  offsetof(AVI_FILE, type_hdrl) ;

    avi_write(&avi->riff, sizeof(AVI_FILE) - offsetof(AVI_FILE, riff), 1, avi->fp);

    // 预分配文件空间
    uint32_t movi_addr = avi_tell(avi->fp);
    avi->movi_addr     = movi_addr;
    avi_seek(avi->fp, avi->max_size, SEEK_CUR);
    avi->movi_addr_end   = avi_tell(avi->fp);
    avi->frame_base_addr = avi->movi_addr_end;
    avi_seek(avi->fp, movi_addr, SEEK_SET);
    avi->cur_addr = avi_tell(avi->fp);
    return avi;

failed:
    if (avi)
    {
        STREAM_FREE(avi);
    }
    return NULL;
}



static void avimuxer_fix_data(AVI_FILE *avi, int writeidx)
{
    uint32_t data, movisize;


    avi->avi_header.total_frames = avi->strhdr_video.length;
    data                         = avi->avi_header.total_frames;
    avi_seek(avi->fp, offsetof(AVI_FILE, avi_header.total_frames) - offsetof(AVI_FILE, riff), SEEK_SET);
    avi_write(&data, 4, 1, avi->fp);
#if 0
    data = avi->strhdr_audio.length;
    avi_seek(avi->fp, offsetof(AVI_FILE, strhdr_audio.length) - offsetof(AVI_FILE, riff), SEEK_SET);
    avi_write(&data, 4, 1, avi->fp);
#endif

    data = avi->strhdr_video.length;
    avi_seek(avi->fp, offsetof(AVI_FILE, strhdr_video.length) - offsetof(AVI_FILE, riff), SEEK_SET);
    avi_write(&data, 4, 1, avi->fp);

    movisize = avi->max_size + 4;
    avi_seek(avi->fp, offsetof(AVI_FILE, mlist_size) - offsetof(AVI_FILE, riff), SEEK_SET);
    avi_write(&movisize, 4, 1, avi->fp);
    avi_seek(avi->fp, 0, SEEK_END);

    if (writeidx)
    {
        uint32_t idx1size, curpos = 4;
        avi_write("idx1", 4, 1, avi->fp);
        idx1size = avi->framesize_idx * sizeof(uint32_t) * 4;
        avi_write(&idx1size, 4, 1, avi->fp);
        while (avi->framesize_fix < avi->framesize_idx)
        {
            avi_write((avi->framesize_lst[avi->framesize_fix] & AVI_VIDEO_FRAME) ? "01dc" : "00wb", 4, 1, avi->fp);
            data = (avi->framesize_lst[avi->framesize_fix] & AVI_KEY_FRAME) ? AVIIF_KEYFRAME : 0;
            avi_write(&data, 4, 1, avi->fp);
            data = curpos;
            avi_write(&data, 4, 1, avi->fp);
            data = avi->framesize_lst ? avi->framesize_lst[avi->framesize_fix] & 0x3fffffff : 0;
            avi_write(&data, 4, 1, avi->fp);
            curpos += data + 8;
            avi->framesize_fix++;
        }
    }
    os_printf("framesize_idx:%X\n", avi->framesize_idx);
    data = avi->max_size + sizeof(AVI_FILE) - offsetof(AVI_FILE, riff) + avi->framesize_idx * 16;
    avi_seek(avi->fp, offsetof(AVI_FILE, riff_size) - offsetof(AVI_FILE, riff), SEEK_SET);
    avi_write(&data, 4, 1, avi->fp);
    avi_seek(avi->fp, 0, SEEK_END);
}


static void avimuxer_fix_data2(AVI_FILE *avi, int writeidx)
{
    //只有缓冲区的时候才需要回写
    if(avi->cache_idx)
    {
        uint32_t data, movisize;
        avi->avi_header.total_frames = avi->strhdr_video.length;

        data = avi->avi_header.total_frames;
        avi_seek(avi->fp, offsetof(AVI_FILE, avi_header.total_frames) - offsetof(AVI_FILE, riff), SEEK_SET);
        avi_write(&data, 4, 1, avi->fp);

        #if 1
            data = avi->strhdr_audio.length;
            avi_seek(avi->fp, offsetof(AVI_FILE, strhdr_audio.length) - offsetof(AVI_FILE, riff), SEEK_SET);
            avi_write(&data, 4, 1, avi->fp);
        #endif

        data = avi->strhdr_video.length;
        avi_seek(avi->fp, offsetof(AVI_FILE, strhdr_video.length) - offsetof(AVI_FILE, riff), SEEK_SET);
        avi_write(&data, 4, 1, avi->fp);

        movisize = avi->max_size + 4;
        avi_seek(avi->fp, offsetof(AVI_FILE, mlist_size) - offsetof(AVI_FILE, riff), SEEK_SET);
        avi_write(&movisize, 4, 1, avi->fp);
        avi_seek(avi->fp, 0, SEEK_END);

        if (writeidx)
        {

            // 如果相等,需要写入idx1的头
            avi_seek(avi->fp, avi->frame_base_addr, SEEK_SET);
            if (avi->movi_addr_end == avi_tell(avi->fp))
            {
                uint32_t idx1size;
                avi_write("idx1", 4, 1, avi->fp);
                idx1size = avi->total_idx * sizeof(uint32_t) * 4;
                avi_write(&idx1size, 4, 1, avi->fp);
                avi->curpos = 4;
            }
            else
            {
                uint32_t idx1size;
                uint32_t seek_tmp = avi_tell(avi->fp);
                idx1size = avi->total_idx * sizeof(uint32_t) * 4;
                avi_seek(avi->fp, avi->movi_addr_end+4, SEEK_SET);
                avi_write(&idx1size, 4, 1, avi->fp);
                avi_seek(avi->fp, seek_tmp, SEEK_SET);
            }
            while (avi->now_idx < avi->cache_idx)
            {
                avi_write((avi->frame_list[avi->now_idx] & AVI_VIDEO_FRAME) ? "01dc" : "00wb", 4, 1, avi->fp);
                data = (avi->frame_list[avi->now_idx] & AVI_KEY_FRAME) ? AVIIF_KEYFRAME : 0;
                avi_write(&data, 4, 1, avi->fp);
                data = avi->curpos;
                avi_write(&data, 4, 1, avi->fp);
                data = avi->frame_list ? avi->frame_list[avi->now_idx] & 0x0fffffff : 0;
                avi_write(&data, 4, 1, avi->fp);
                //如果是补帧,则不需要增加偏移?
                if(avi->frame_list[avi->now_idx] & AVI_INSERT_FRAME)
                {

                }
                else
                {
                    avi->curpos += data + 8;
                }
                
                avi->now_idx++;
            }

            // 记录当前的位置
            avi->frame_base_addr = avi_tell(avi->fp);

            avi->now_idx   = 0;
            avi->cache_idx = 0;
        }

        data = avi->max_size + sizeof(AVI_FILE) - offsetof(AVI_FILE, riff) + avi->total_idx * 16;
        avi_seek(avi->fp, offsetof(AVI_FILE, riff_size) - offsetof(AVI_FILE, riff), SEEK_SET);
        avi_write(&data, 4, 1, avi->fp);
        avi_seek(avi->fp, 0, SEEK_END);
        avi_file_syn(avi->fp);
        os_printf("avi->total_idx:%d\n",avi->total_idx);
    }

}


void avimuxer_sync(void *ctx)
{
    AVI_FILE *avi = (AVI_FILE *) ctx;
    if (avi && avi->fp)
    {
        avi_file_syn(avi->fp);
        avimuxer_fix_data2(avi,1);
    }
}

void avimuxer_exit(void *ctx)
{
    AVI_FILE *avi = (AVI_FILE *) ctx;
    if (avi)
    {
        if (avi->fp)
        {
            avimuxer_fix_data(avi, 1);
            if (!avi->extern_fp)
            {
                avi_close(avi->fp);
            }
        }
        if(avi->framesize_lst)
        {
            STREAM_FREE(avi->framesize_lst);
        }
        
        STREAM_FREE(avi);
    }
}


void avimuxer_exit2(void *ctx)
{
    AVI_FILE *avi = (AVI_FILE *) ctx;
    if (avi)
    {
        os_printf("avi->extern_fp:%d\n",avi->extern_fp);
        if (avi->fp)
        {
            avimuxer_fix_data2(avi, 1);
            if (!avi->extern_fp)
            {
                avi_close(avi->fp);
            }
        }
        if(avi->framesize_lst)
        {
            STREAM_FREE(avi->framesize_lst);
        }
        
        STREAM_FREE(avi);
    }
}

uint32_t avimuxer_video2(void *ctx, unsigned char *buf, int len, int key, unsigned pts,uint8_t insert)
{
    AVI_FILE *avi = (AVI_FILE *) ctx;
    if (avi == NULL)
    {
        return 1;
    }
    if (avi->fp)
    {
        int alignlen = (len & 1) ? len + 1 : len;
        //补帧,不需要写入数据
        if(!insert)
        {
            avi_seek(avi->fp, avi->cur_addr, SEEK_SET);
            if (avi_tell(avi->fp) - avi->movi_addr + alignlen > avi->movi_addr_end)
            {
                os_printf("%s:%d\n", __FUNCTION__, __LINE__);
                return 2;
            }
            
            avi_write("01dc", 4, 1, avi->fp);
            avi_write(&alignlen, 4, 1, avi->fp);
            avi_write(buf, len, 1, avi->fp);
            if (len & 1)
            {
                avi_write((void*)&avi_zero, 1, 1, avi->fp);
            }
            avi->cur_addr = avi_tell(avi->fp);
            if (avi->movi_addr_end - avi_tell(avi->fp) + 4 > 8)
            {
                // 补junk的数据
                avi_write("JUNK", 4, 1, avi->fp);
                // 补junk长度
                uint32_t junklen = avi->movi_addr_end - avi_tell(avi->fp) - 8 + 4;
                //os_printf("junklen:%X\ttell:%X\n", junklen,avi_tell(avi->fp));
                avi_write(&junklen, 4, 1, avi->fp);
            }
        }
        if (avi->cache_idx < sizeof(avi->frame_list) / 16)
        {
            if(insert)
            {
                avi->frame_list[avi->cache_idx++] = alignlen | AVI_VIDEO_FRAME | AVI_INSERT_FRAME|(key << 30);
            }
            else
            {
                avi->frame_list[avi->cache_idx++] = alignlen | AVI_VIDEO_FRAME | (key << 30);
            }
            
            avi->total_idx++;
        }
        avi->strhdr_video.length++;
    }

    // 同步一次
    if (avi->cache_idx == sizeof(avi->frame_list) / 16)
    {
        avimuxer_fix_data2(avi, 1);
        avi->cache_idx   = 0;
        avi->cache_idx = 0;
    }
    return 0;
}



uint32_t avimuxer_audio2(void *ctx, unsigned char *buf, int len, int key, unsigned pts)
{
    AVI_FILE *avi = (AVI_FILE *) ctx;
    if (avi && avi->fp)
    {
        int alignlen = (len & 1) ? len + 1 : len;
        avi_seek(avi->fp, avi->cur_addr, SEEK_SET);
        if (avi_tell(avi->fp) - avi->movi_addr + alignlen > avi->movi_addr_end)
        {
            os_printf("%s:%d\n", __FUNCTION__, __LINE__);
            return 1;
        }
        avi_write("00wb", 4, 1, avi->fp);
        avi_write(&alignlen, 4, 1, avi->fp);
        avi_write(buf, len, 1, avi->fp);
        if (len & 1)
        {
            avi_write((void*)&avi_zero, 1, 1, avi->fp);
        }

        avi->cur_addr = avi_tell(avi->fp);
        if (avi->movi_addr_end - avi_tell(avi->fp) + 4 > 8)
        {
            // 补junk的数据
            avi_write("JUNK", 4, 1, avi->fp);
            // 补junk长度
            uint32_t junklen = avi->movi_addr_end - avi_tell(avi->fp) - 8 + 4;
            //os_printf("junklen:%X\ttell:%X\n", junklen,avi_tell(avi->fp));
            avi_write(&junklen, 4, 1, avi->fp);
        }


        if (avi->cache_idx < sizeof(avi->frame_list) / 16)
        {
            avi->frame_list[avi->cache_idx++] = alignlen | AVI_AUDIO_FRAME;
            avi->total_idx++;
        }

        avi->strhdr_audio.length += (len>>2);
    }
    // 同步一次
    if (avi->cache_idx == sizeof(avi->frame_list) / 16)
    {
        avimuxer_fix_data2(avi, 1);
        avi->cache_idx = 0;
    }
    return 0;
}