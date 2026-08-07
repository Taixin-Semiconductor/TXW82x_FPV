#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "osal/string.h"
#include "fatfs/osal_file.h"

#include "stream_define.h"
#include "adts.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

/**************************************************************************************************************
 * stts:记录每一帧的解码时间,如果时间一致,count增加,压缩大小,所以录像的时候,可以适当修改解码时间,
 *      让解码时间尽量一样,可以减少存储空间也有助于解码需要的空间
 * stsc:chunk_box,代表每一个chunk有多少个sample,可能现阶段只考虑普通的,后续遇到不同的mp4视频,再去实现
 * stsz:每一帧数据的size
 * stco:每一个chunk的偏移
 * stss:关键帧的位置(第几帧)
 *
 *
 * 这里是简单的mp4解码,暂时不考虑多流情况,解析的是minimp4保存单视频的文件
 **************************************************************************************************************/

#define MP4_ABORT(a)                                                                                                                                                                                   \
    {                                                                                                                                                                                                  \
        if (a)                                                                                                                                                                                         \
        {                                                                                                                                                                                              \
            ret = __LINE__;                                                                                                                                                                            \
            goto abort_end;                                                                                                                                                                            \
        }                                                                                                                                                                                              \
        else                                                                                                                                                                                           \
        {                                                                                                                                                                                              \
            ret = 0;                                                                                                                                                                                   \
        }                                                                                                                                                                                              \
    }

// 结构体申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC os_malloc
#define STREAM_LIBC_FREE   os_free
#define STREAM_LIBC_ZALLOC os_zalloc

#define MAX_MP4_DEMUX_TX 8
#define MAX_TRY_COUNT    (10)

extern uint8_t *mp4_demux_get_sps(struct msi *msi, uint16_t *sps_len);
extern uint8_t *mp4_demux_get_pps(struct msi *msi, uint16_t *pps_len);

enum MP4_DEMUX_EVT
{
    MP4_DEMUX_JMP   = BIT(0),
    MP4_DEMUX_START = BIT(1),
    MP4_DEMUX_STOP  = BIT(2),
    MP4_DEMUX_EXIT  = BIT(3),
};

typedef struct
{
    uint32_t sample_count;
    uint32_t sample_delta;
} mp4_stts;

typedef struct
{
    uint32_t first_chunk;
    uint32_t sample_per_chunk;
    uint32_t sample_description_index;
} mp4_stsc;

typedef struct
{
    uint32_t sample_size;
} mp4_stsz;

typedef struct
{
    uint32_t sample_time;
} mp4_stsz_time;

typedef struct
{
    uint32_t chunk_offset; // Sample size  MP4 Explorer
} mp4_stco;

typedef struct
{
    uint32_t sample_number;
} mp4_stss;

typedef struct
{
    uint8_t  version;
    uint8_t  flags[3];
    uint32_t creation_time;
    uint32_t modification_time;
    uint32_t track_id;
    uint32_t reserved1;
    uint32_t duration;
    uint32_t reserved2[2];
    uint16_t layer;
    uint16_t alternate_group;
    uint16_t volume;
    uint16_t reserved3;
    uint8_t  matrix[36];
    uint32_t width;
    uint32_t height;
} mp4_trak;

typedef struct
{
    uint32_t  stts_count;
    mp4_stts *stts;

    uint32_t  stsc_count;
    mp4_stsc *stsc;

    uint32_t       stsz_count;
    mp4_stsz      *stsz;
    mp4_stsz_time *stsz_time;

    uint32_t  stco_count;
    mp4_stco *stco;

    uint32_t  stss_count;
    mp4_stss *stss;
    uint32_t  key_frame_offset; // 关键帧的偏移,0-stss_count

    uint8_t *pps;
    uint16_t pps_len;
    uint8_t *sps;
    uint16_t sps_len;

} main_box;

#pragma pack(2)
typedef struct
{
    uint8_t  reserved[6];          // 保留字段，通常为 0
    uint16_t data_reference_index; // 数据引用索引

    uint16_t pre_defined;     // 保留字段，通常为 0
    uint16_t reserved1;       // 保留字段，通常为 0
    uint32_t pre_defined2[3]; // 保留字段，通常为 0

    uint16_t width;  // 视频宽度
    uint16_t height; // 视频高度

    uint32_t horizresolution; // 水平分辨率，通常为 0x00480000（72 dpi）
    uint32_t vertresolution;  // 垂直分辨率，通常为 0x00480000（72 dpi）

    uint32_t reserved2; // 保留字段，通常为 0

    uint16_t frame_count; // 每个样本的帧数，通常为 1

    uint8_t compressorname[32]; // 压缩器名称，第一个字节表示名称长度，后续为名称字符串，剩余部分填充为 0

    uint16_t depth; // 图像的颜色深度，通常为 0x0018（24 位）

    int16_t pre_defined3; // 保留字段，通常为 -1（0xFFFF）

    // 后续可能包含 avcC box（AVCDecoderConfigurationRecord）
} AVC1Box;
#pragma pack()

typedef struct
{
    uint8_t configurationVersion;  // 配置版本，通常为 1
    uint8_t AVCProfileIndication;  // 表示使用的 H.264 配置文件
    uint8_t profile_compatibility; // 配置文件兼容性
    uint8_t AVCLevelIndication;    // 表示使用的 H.264 等级
    uint8_t lengthSizeMinusOne;    // NAL 单元长度字段的字节数减一，通常为 3（表示长度字段为 4 字节）

} AVCC_BOX;

// 解析pps和sps的结构体
typedef struct
{
    uint8_t numOfSequenceParameterSets;
    uint8_t sequenceParameterSetLength_H;
    uint8_t sequenceParameterSetLength_L;
} PPS_SPS;

typedef struct
{
    uint32_t type; // video或者sound
    uint32_t init;
    mp4_trak trak;
    main_box box;
} trak;

typedef struct
{
    uint8_t version;
    uint8_t flags[3];
} MP4_gen_head;

struct mp4_demux_msi_s
{
    struct msi     *msi;
    char        *filename; // 只是保存指针,不会被用到
    F_FILE         *fp;
    trak            trak_t[2]; // trak0:视频，trak1:音频
    uint8_t         trak_index;
    uint8_t         aac_dsi[2];
    uint8_t        *pps;
    uint8_t        *sps;
    uint16_t        pps_len;
    uint16_t        sps_len;
    uint32_t        play_vframe_num; // 视频播放第几帧
    uint32_t        jmp_vframe_num;  // 视频播放第几帧
    uint32_t        play_aframe_num; // 音频播放第几帧
    uint32_t        audio_samplerate;
    struct os_event evt;
    struct fbpool   tx_pool;
};

extern uint32_t box_read(struct mp4_demux_msi_s *mp4_demux, const char *name, int32_t max_size);
typedef uint32_t (*MP4_parse_func)(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size);

typedef struct
{
    const char    *name;
    MP4_parse_func func;
} MP4_parse_register;

#define BIG4_ENDIAN(X) ((X & 0xff000000) >> 24 | (X & 0x00FF0000) >> 8 | (X & 0x0000FF00) << 8 | (X & 0x000000FF) << 24)
#define BIG2_ENDIAN(X) ((X & 0xFF00) >> 8 | (X & 0x00FF) << 8)

uint32_t general_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    int32_t  r_len = max_size - 8;
    uint32_t ret;
    // os_printf("[%s] offset:%X\tlen:%d\n", box_name, osal_ftell(fp), r_len);
    ret = box_read(mp4_demux, box_name, r_len);
    return ret;
}

uint32_t not_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    os_printf("%s:%d\tname:%s\n", __FUNCTION__, __LINE__, box_name);
    return 0;
}

// 实际内容的位置
uint32_t mdat_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    F_FILE *fp    = mp4_demux->fp;
    int32_t r_len = max_size - 8;
    os_printf("offset:%X\tlen:%d\n", osal_ftell(fp), r_len);
    return 0;
}

static uint32_t stts_sample_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    F_FILE      *fp     = mp4_demux->fp;
    trak        *trak_t = &mp4_demux->trak_t[mp4_demux->trak_index];
    main_box    *box    = &trak_t->box;
    MP4_gen_head head;
    uint32_t     entry_count = 0;
    uint32_t     ret         = 0;
    ret                      = osal_fread(&head, 1, sizeof(MP4_gen_head), fp);
    MP4_ABORT(ret == 0);
    ret = osal_fread(&entry_count, 1, sizeof(entry_count), fp);
    MP4_ABORT(ret == 0);

    entry_count = BIG4_ENDIAN(entry_count);
    os_printf("[%s] entry_count:%d\n", box_name, entry_count);
    // 需要解释各个sample的时间
    box->stts_count = entry_count;
    if (box->stts)
    {
        os_printf("%s:%d err,not support more stts\n", __FUNCTION__, __LINE__);
    }
    else
    {
        box->stts = (mp4_stts *) STREAM_MALLOC(entry_count * sizeof(mp4_stts));
        ret       = osal_fread(box->stts, entry_count, sizeof(mp4_stts), fp);
        MP4_ABORT(ret == 0);

        // 读取完毕,然后为stsz_time创建空间保存时间戳的空间,可以加快索引,但是需要空间变多
        // 先计算一下sample总数量,理论应该和stsz_count一致才对
        uint32_t sample_count = 0;
        for (uint32_t i = 0; i < entry_count; i++)
        {
            sample_count += BIG4_ENDIAN(box->stts[i].sample_count);
        }
        // 申请空间,时间用uint32来保存
        box->stsz_time = (mp4_stsz_time *) STREAM_MALLOC(sample_count * sizeof(mp4_stsz_time));
        if (box->stsz_time)
        {
            uint32_t count  = 0;
            uint32_t offset = 0;
            uint32_t time   = 0;
            for (uint32_t i = 0; i < entry_count; i++)
            {
                count = BIG4_ENDIAN(box->stts[i].sample_count);
                for (uint32_t j = 0; j < count; j++)
                {
                    time += (BIG4_ENDIAN(box->stts[i].sample_delta) / 90);
                    box->stsz_time[offset].sample_time = time;
                    offset++;
                }
            }
        }
    }
abort_end:
    return ret;
}

uint32_t stts_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    return stts_sample_parse(mp4_demux, box_name, max_size - 8);
}

static uint32_t stsc_sample_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    F_FILE      *fp     = mp4_demux->fp;
    trak        *trak_t = &mp4_demux->trak_t[mp4_demux->trak_index];
    MP4_gen_head head;
    uint32_t     ret         = 0;
    uint32_t     entry_count = 0;
    ret                      = osal_fread(&head, 1, sizeof(MP4_gen_head), fp);
    MP4_ABORT(ret == 0);
    ret = osal_fread(&entry_count, 1, sizeof(entry_count), fp);
    MP4_ABORT(ret == 0);
    entry_count = BIG4_ENDIAN(entry_count);
    os_printf("[%s] entry_count:%d\n", box_name, entry_count);
    main_box *box   = &trak_t->box;
    // 需要解释chunk的位置
    box->stsc_count = entry_count;
    if (box->stsc)
    {
        os_printf("%s:%d err,not support more stts\n", __FUNCTION__, __LINE__);
    }
    else
    {
        box->stsc = (mp4_stsc *) STREAM_MALLOC(entry_count * sizeof(mp4_stsc));
        ret       = osal_fread(box->stsc, entry_count, sizeof(mp4_stsc), fp);
        MP4_ABORT(ret == 0);
    }
abort_end:
    return ret;
}

uint32_t stsc_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    return stsc_sample_parse(mp4_demux, box_name, max_size - 8);
}

static uint32_t stsd_sample_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    F_FILE      *fp = mp4_demux->fp;
    MP4_gen_head head;
    uint32_t     entry_count = 0;
    uint32_t     ret         = 0;

    ret = osal_fread(&head, 1, sizeof(MP4_gen_head), fp);
    MP4_ABORT(ret == 0);
    ret = osal_fread(&entry_count, 1, sizeof(entry_count), fp);
    MP4_ABORT(ret == 0);
    entry_count = BIG4_ENDIAN(entry_count);
    os_printf("[%s] entry_count:%d\n", box_name, entry_count);
    ret = general_parse(mp4_demux, box_name, max_size - 8);
    MP4_ABORT(ret > 0);
abort_end:
    return ret;
}

uint32_t stsd_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    return stsd_sample_parse(mp4_demux, box_name, max_size - 8);
}

static uint32_t avc1_sample_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    F_FILE  *fp = mp4_demux->fp;
    AVC1Box  avc1_box;
    uint32_t ret = 0;
    os_printf("AVC1 Box size:%d\tmax_size:%d\n", sizeof(AVC1Box), max_size);
    ret = osal_fread(&avc1_box, 1, sizeof(AVC1Box), fp);
    MP4_ABORT(ret == 0);
    ret = box_read(mp4_demux, box_name, max_size - sizeof(AVC1Box));
    MP4_ABORT(ret > 0);
abort_end:
    os_printf("%s:%d\tret:%d\n", __FUNCTION__, __LINE__, ret);
    return ret;
}
uint32_t avc1_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    return avc1_sample_parse(mp4_demux, box_name, max_size - 8);
}

// 解析AVCC,这里当作是h264去解析,解析pps和sps的参数
static uint32_t avcc_sample_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    F_FILE   *fp     = mp4_demux->fp;
    trak     *trak_t = &mp4_demux->trak_t[mp4_demux->trak_index];
    AVCC_BOX  avcc_box;
    PPS_SPS   pps_sps;
    main_box *box = &trak_t->box;
    uint32_t  ret = 0;
    ret           = osal_fread(&avcc_box, 1, sizeof(AVCC_BOX), fp);
    MP4_ABORT(ret == 0);
    os_printf("[%s] offset:%X\tlen:%d\n", box_name, osal_ftell(fp), max_size);

    uint8_t  pps = 0, sps = 0;
    uint16_t len;
    while (!pps || !sps)
    {
        // 开始解析pps或者sps
        ret = osal_fread(&pps_sps, 1, sizeof(PPS_SPS), fp);
        MP4_ABORT(ret == 0);
        os_printf("pps_sps.numOfSequenceParameterSets:%X\tpps_sps.sequenceParameterSetLength_H:%d\tpps_sps.sequenceParameterSetLength_L:%d\n", pps_sps.numOfSequenceParameterSets,
                  pps_sps.sequenceParameterSetLength_H, pps_sps.sequenceParameterSetLength_L);
        len = (pps_sps.sequenceParameterSetLength_H << 8 | pps_sps.sequenceParameterSetLength_L);
        // SPS
        if (pps_sps.numOfSequenceParameterSets == 0xE1)
        {
            sps = 1;
            os_printf("SPS len:%d\n", len);
            // 读取sps数据保存
            box->sps = (uint8_t *) STREAM_MALLOC(len);
            if (box->sps)
            {
                ret = osal_fread(box->sps, 1, len, fp);
                MP4_ABORT(ret == 0);
                box->sps_len = len;
            }
            else
            {
                os_printf("malloc sps err\n");
            }
        }
        // PPS
        else if (pps_sps.numOfSequenceParameterSets == 0x01)
        {
            pps = 1;
            os_printf("PPS len:%d\n", len);
            // 读取pps数据保存
            box->pps = (uint8_t *) STREAM_MALLOC(len);
            if (box->pps)
            {
                ret = osal_fread(box->pps, 1, len, fp);
                MP4_ABORT(ret == 0);
                box->pps_len = len;
            }
            else
            {
                os_printf("malloc pps err\n");
            }
        }
    }

abort_end:
    return ret;
}
uint32_t avcc_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    return avcc_sample_parse(mp4_demux, box_name, max_size - 8);
}

static uint32_t stsz_sample_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    F_FILE      *fp     = mp4_demux->fp;
    trak        *trak_t = &mp4_demux->trak_t[mp4_demux->trak_index];
    MP4_gen_head head;
    main_box    *box = &trak_t->box;
    uint32_t     sample_size;
    uint32_t     sample_count = 0;
    uint32_t     ret          = 0;
    ret                       = osal_fread(&head, 1, sizeof(MP4_gen_head), fp);
    MP4_ABORT(ret == 0);
    ret = osal_fread(&sample_size, 1, sizeof(sample_size), fp);
    MP4_ABORT(ret == 0);
    sample_size = BIG4_ENDIAN(sample_size);
    os_printf("[%s] sample_size:%d\n", box_name, sample_size);
    if (sample_size != 0)
    {
        os_printf("[%s] not support sample_size == 0\n", box_name);
        return ret;
    }
    // 读取sample_count
    ret = osal_fread(&sample_count, 1, sizeof(sample_count), fp);
    MP4_ABORT(ret == 0);
    sample_count = BIG4_ENDIAN(sample_count);

    os_printf("[%s] sample_count:%d\n", box_name, sample_count);
    // 需要解释chunk的位置

    box->stsz_count = sample_count;
    if (box->stsz)
    {
        os_printf("%s:%d err,not support more stts\n", __FUNCTION__, __LINE__);
    }
    else
    {
        box->stsz = (mp4_stsz *) STREAM_MALLOC(sample_count * sizeof(mp4_stsz));
        ret       = osal_fread(box->stsz, sample_count, sizeof(mp4_stsz), fp);
        MP4_ABORT(ret == 0);
    }
abort_end:
    return ret;
}

uint32_t stsz_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    return stsz_sample_parse(mp4_demux, box_name, max_size - 8);
}

static uint32_t stco_sample_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    F_FILE      *fp     = mp4_demux->fp;
    trak        *trak_t = &mp4_demux->trak_t[mp4_demux->trak_index];
    main_box    *box    = &trak_t->box;
    MP4_gen_head head;
    uint32_t     chunk_offset_box_entry_count;
    uint32_t     ret = 0;
    ret              = osal_fread(&head, 1, sizeof(MP4_gen_head), fp);
    MP4_ABORT(ret == 0);
    ret = osal_fread(&chunk_offset_box_entry_count, 1, sizeof(chunk_offset_box_entry_count), fp);
    MP4_ABORT(ret == 0);
    chunk_offset_box_entry_count = BIG4_ENDIAN(chunk_offset_box_entry_count);
    os_printf("[%s] chunk_offset_box_entry_count:%d\n", box_name, chunk_offset_box_entry_count);

    // 需要解析chunk_box的位置的位置
    box->stco_count = chunk_offset_box_entry_count;
    if (box->stco)
    {
        os_printf("%s:%d err,not support more stco:%X\n", __FUNCTION__, __LINE__, box->stco);
    }
    else
    {
        box->stco = (mp4_stco *) STREAM_MALLOC(chunk_offset_box_entry_count * sizeof(mp4_stco));
        ret       = osal_fread(box->stco, chunk_offset_box_entry_count, sizeof(mp4_stco), fp);
        MP4_ABORT(ret == 0);
    }
abort_end:
    return ret;
}

uint32_t stco_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    return stco_sample_parse(mp4_demux, box_name, max_size - 8);
}

static uint32_t stss_sample_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    F_FILE      *fp     = mp4_demux->fp;
    trak        *trak_t = &mp4_demux->trak_t[mp4_demux->trak_index];
    main_box    *box    = &trak_t->box;
    MP4_gen_head head;
    uint32_t     Sync_sample_box_entry_count;
    uint32_t     ret = 0;
    ret              = osal_fread(&head, 1, sizeof(MP4_gen_head), fp);
    MP4_ABORT(ret == 0);
    ret = osal_fread(&Sync_sample_box_entry_count, 1, sizeof(Sync_sample_box_entry_count), fp);
    MP4_ABORT(ret == 0);
    Sync_sample_box_entry_count = BIG4_ENDIAN(Sync_sample_box_entry_count);
    os_printf("[%s] Sync_sample_box_entry_count:%d\n", box_name, Sync_sample_box_entry_count);

    // 需要解析Sync_sample_box_entry_count的列表
    box->stss_count = Sync_sample_box_entry_count;
    if (box->stss)
    {
        os_printf("%s:%d err,not support more stts\n", __FUNCTION__, __LINE__);
    }
    else
    {
        box->stss = (mp4_stss *) STREAM_MALLOC(Sync_sample_box_entry_count * sizeof(mp4_stss));
        ret       = osal_fread(box->stss, Sync_sample_box_entry_count, sizeof(mp4_stss), fp);
        MP4_ABORT(ret == 0);
    }

abort_end:
    return ret;
}

uint32_t stss_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    return stss_sample_parse(mp4_demux, box_name, max_size - 8);
}

static uint32_t mp4a_sample_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    uint8_t  samplerate_index;
    F_FILE  *fp        = mp4_demux->fp;
    uint8_t *mp4a_data = (uint8_t *) STREAM_MALLOC(max_size);
    uint32_t ret       = 0;
    ret                = osal_fread(mp4a_data, 1, max_size, fp);
    MP4_ABORT(ret == 0);
    mp4_demux->aac_dsi[0] = mp4a_data[max_size - 2];
    mp4_demux->aac_dsi[1] = mp4a_data[max_size - 1];

    samplerate_index = ((mp4_demux->aac_dsi[0] & 0x07) << 1) | (mp4_demux->aac_dsi[1] >> 7);
    switch (samplerate_index)
    {
        case 0x8:
            mp4_demux->audio_samplerate = 16000;
            break;
        case 0xB:
            mp4_demux->audio_samplerate = 8000;
            break;
        default:
            mp4_demux->audio_samplerate = 8000;
            break;
    }
    os_printf("mp4_demux->audio_samplerate:%d\n", mp4_demux->audio_samplerate);
    STREAM_FREE(mp4a_data);
abort_end:
    return ret;
}

uint32_t mp4a_parse(struct mp4_demux_msi_s *mp4_demux, const char *box_name, int32_t max_size)
{
    return mp4a_sample_parse(mp4_demux, box_name, max_size - 8);
}

const MP4_parse_register MP4_func[] = {
        {"moov", general_parse}, {"trak", general_parse}, {"mdia", general_parse}, {"minf", general_parse},
        {"stbl", general_parse}, {"stts", stts_parse},    {"stsc", stsc_parse},    {"stsz", stsz_parse},
        {"stco", stco_parse},    {"stss", stss_parse},    {"mdat", mdat_parse},    {"stsd", stsd_parse},
        {"avc1", avc1_parse},    {"avcC", avcc_parse},    {"mp4a", mp4a_parse},    {(const char *) NULL, not_parse},
};

MP4_parse_func get_func(const char *name)
{
    int i = 0;
    for (i = 0; MP4_func[i].name != NULL; i++)
    {

        if (strcmp(MP4_func[i].name, name) == 0)
        {
            return MP4_func[i].func;
        }
    }
    return MP4_func[i].func;
}

uint32_t box_read(struct mp4_demux_msi_s *mp4_demux, const char *name, int32_t max_size)
{
    F_FILE        *fp        = mp4_demux->fp;
    uint8_t        get_trak  = 0;
    int32_t        read_size = max_size;
    int32_t        box_len;
    uint8_t        box_name[5] = {0};
    uint32_t       cur_offset;
    MP4_parse_func func;
    uint32_t       ret = 0;
    while (read_size > 8)
    {
        ret = osal_fread(&box_len, 1, 4, fp);
        MP4_ABORT(ret == 0);
        ret = osal_fread(box_name, 1, 4, fp);
        MP4_ABORT(ret == 0);
        box_len    = BIG4_ENDIAN(box_len);
        cur_offset = osal_ftell(fp);
        if (get_trak && (os_strncmp(box_name, "trak", os_strlen("trak")) == 0))
        {
            os_printf("more trak\n");
            mp4_demux->trak_index++;
        }
        if (os_strncmp(box_name, "trak", os_strlen("trak")) == 0)
        {
            get_trak                                      = 1;
            mp4_demux->trak_t[mp4_demux->trak_index].init = 1;
        }

        os_printf("box_name:%s\tbox_len:%d\n", box_name, box_len);
        func = get_func((const char *) box_name);

        if (func)
        {
            // 检查是否有注册对应的函数,如果有就执行,没有就使用通用的方法
            ret = func(mp4_demux, (const char *) box_name, box_len);
            MP4_ABORT(ret > 0);
        }

        osal_fseek(fp, cur_offset + box_len - 8);
        read_size = read_size - box_len;
    }
abort_end:
    return ret;
}

// 获取一个帧相对文件的偏移
uint32_t get_frame_offset(trak *trak_t, uint32_t sample_offset, uint32_t *read_size)
{
    main_box *box = &trak_t->box;
    // 这里暂时仅仅考虑sssc都是1  1  1的情况,暂时看到minimp4输出是这样的格式
    if (box->stco_count > sample_offset)
    {
        // 现在理论chunk只有一种,所以这里粗暴判断,不匹配就不能解析
        if (box->stsz_count == box->stco_count && box->stsz)
        {
            *read_size = BIG4_ENDIAN(box->stsz[sample_offset].sample_size);
        }
        return BIG4_ENDIAN(box->stco[sample_offset].chunk_offset);
    }
    else
    {
        return 0;
    }
}

// 获取pts的一个帧的NUM,应该返回一个关键帧,同时返回pts的真实值,因为传入一个pts,是一个约等于值,实际pts应该与关键帧一致
// 都是使用90000为视频帧的默认值计算
uint32_t get_frame_num_from_pts(trak *trak_t, uint32_t ms)
{
    main_box *box          = &trak_t->box;
    uint32_t  duration     = ms;
    uint32_t  last_duation = 0;
    uint32_t  now_duation  = 0;

    uint32_t last_frame_num       = 0;
    uint32_t now_frame_num        = 0;
    uint32_t find_main_frame      = 0;
    uint32_t last_find_main_frame = 0;
    uint32_t match_frame_num      = 0;
    // 搜索duration范围,记录最近的关键帧,规则是往回退
    // 这里可以加速,使用二分法或者其他算法加速,现在仅仅实现逻辑
    if (box->stts && box->stts_count > 0)
    {
        for (int i = 0; i < box->stsz_count; i++)
        {
            now_frame_num = i;
            now_duation   = box->stsz_time[i].sample_time;
            // 找到duation对应的位置
            if (duration >= last_duation && duration <= now_duation)
            {
                break;
            }
            last_duation   = now_duation;
            last_frame_num = now_frame_num;
        }
    }

    // 找到,开始寻找关键帧位置,通过last_frame_num和now_frame_num来寻找最近的关键帧
    if (duration >= last_duation && duration <= now_duation)
    {
        if (box->stss && box->stss_count > 0)
        {
            last_find_main_frame = BIG4_ENDIAN(box->stss[0].sample_number);
            for (int i = 0; i < box->stss_count; i++)
            {
                find_main_frame = BIG4_ENDIAN(box->stss[i].sample_number);
                // 找到了,退出,应该返回last_find_main_frame
                if (find_main_frame > now_frame_num)
                {
                    match_frame_num = last_find_main_frame;
                    break;
                }
                last_find_main_frame = find_main_frame;
            }
        }
    }
    return match_frame_num;
}

uint32_t get_frame_num_pts(trak *trak_t, uint32_t num)
{
    main_box *box = &trak_t->box;
    if (num < box->stsz_count)
    {
        // os_printf("box->stsz_time[%d].sample_time:%d\n",num,box->stsz_time[num].sample_time);
        return box->stsz_time[num].sample_time;
    }
    else
    {
        return 0;
    }
}

void mp4_demux_thread(void *d)
{
    struct msi             *msi       = (struct msi *) d;
    struct mp4_demux_msi_s *mp4_demux = (struct mp4_demux_msi_s *) msi->priv;
    int                     ret       = 0;
    if (!mp4_demux->fp)
    {
        os_printf("file open fail\n");
        return;
    }

    uint32_t vframe_offset     = 0;
    uint32_t vframe_size       = 0;
    uint32_t aframe_offset     = 0;
    uint32_t aframe_size       = 0;
    uint32_t video_timestamp   = 0;
    // 默认播放是第一帧
    mp4_demux->play_vframe_num = 0;
    mp4_demux->play_aframe_num = 0;

    struct framebuff *fb        = NULL;
    uint32_t          rflags    = 0;
    uint32_t          err_times = 0;
    uint32_t          err;
    uint8_t           count = 0;
    uint8_t           flag  = 0;

    // 这里开始进行视频的播放,需要填充时间戳,然后解码给到播放器或者其他地方
    // 这里会不停发送数据,这里只是管自己是否有多余的节点,播放速度以及快进快退由其他地方发命令
    uint32_t last_play_time = os_jiffies();
    // 主要是为了不要随意删除当前msi,只有触发命令才可以退出
    msi_get(msi);
    os_event_wait(&mp4_demux->evt, MP4_DEMUX_START | MP4_DEMUX_STOP, &rflags, OS_EVENT_WMODE_CLEAR | OS_EVENT_WMODE_OR, -1);

    if (rflags & MP4_DEMUX_STOP)
    {
        ret = __LINE__;
        goto mp4_demux_thread_exit;
    }
    os_printf("mp4_demux->trak_t[1].init:%d\t%d\n", mp4_demux->trak_t[0].init, mp4_demux->trak_t[1].init);
    while (1)
    {
    mp4_demux_thread_open_again:
        if (!mp4_demux->fp)
        {
            err_times++;
            mp4_demux->fp = osal_fopen(mp4_demux->filename, "rb");
        }

        if (!mp4_demux->fp)
        {
            if (err_times > MAX_TRY_COUNT)
            {
                ret = __LINE__;
                goto mp4_demux_thread_exit;
            }
            os_sleep_ms(5);
            goto mp4_demux_thread_open_again;
        }
        err_times = 0;
    mp4_demux_thread_JMP:
        video_timestamp = get_frame_num_pts(&mp4_demux->trak_t[0], mp4_demux->play_vframe_num);
        if (video_timestamp > os_jiffies() - last_play_time)
        {
            os_sleep_ms(1);
        }
        else
        {
            vframe_offset = get_frame_offset(&mp4_demux->trak_t[0], mp4_demux->play_vframe_num, &vframe_size);
            os_printf("vframe_offset:%d\tvframe_size:%d\n", vframe_offset, vframe_size);
            os_printf("mp4_demux->play_vframe_num:%d\n", mp4_demux->play_vframe_num);
            if (!vframe_offset || vframe_size == 0)
            {
                flag |= BIT(0);
                if (flag == (BIT(0) | BIT(1)))
                {
                    break;
                }
                ret = __LINE__;
                goto mp4_demux_audio;
            }
        mp4_demux_thread_again:
            rflags = 0;
            os_event_wait(&mp4_demux->evt, MP4_DEMUX_JMP | MP4_DEMUX_STOP, &rflags, OS_EVENT_WMODE_CLEAR | OS_EVENT_WMODE_OR, 0);
            // 退出线程
            if (rflags & MP4_DEMUX_STOP)
            {
                break;
            }

            // 快进快退
            if (rflags & MP4_DEMUX_JMP)
            {
                mp4_demux->play_vframe_num = mp4_demux->jmp_vframe_num;
                video_timestamp            = get_frame_num_pts(&mp4_demux->trak_t[0], mp4_demux->play_vframe_num);
                // 配置播放的时间
                last_play_time             = os_jiffies() - video_timestamp;
                mp4_demux->play_aframe_num = video_timestamp / (1024 * 1000 / mp4_demux->audio_samplerate);
                goto mp4_demux_thread_JMP;
            }

            fb = fbpool_get(&mp4_demux->tx_pool, 0, mp4_demux->msi);
            if (!fb)
            {
                os_sleep_ms(1);
                goto mp4_demux_thread_again;
            }
            else
            {
                extern uint8_t is_key_frame(trak * trak_t, uint32_t frame_num);
                // os_printf("is key:%d\n", is_key_frame(NULL, mp4_demux->play_vframe_num));
                fb->time = get_frame_num_pts(&mp4_demux->trak_t[0], mp4_demux->play_vframe_num);
                fb->data = (uint8_t *) STREAM_MALLOC(vframe_size);
                if (!fb->data)
                {
                    msi_delete_fb(NULL, fb);
                    os_sleep_ms(1);
                    continue;
                }
                fb->len   = vframe_size;
                fb->mtype = F_H264;
                fb->stype = FSTYPE_H264_FILE;
                osal_fseek(mp4_demux->fp, vframe_offset + 4);
                err = osal_fread(fb->data, 1, vframe_size - 4, mp4_demux->fp);
                // os_printf("frame_size:%d\t%02X\t%02X\n", frame_size,fb->data[0], fb->data[1]);
                // 暂时只有I帧和P帧
                // 可能文件系统有错,重新尝试打开文件系统,超过一定次数就退出

                if (err)
                {
                    if (is_key_frame(&mp4_demux->trak_t[0], mp4_demux->play_vframe_num))
                    {
                        struct fb_h264_s *priv = (struct fb_h264_s *) STREAM_LIBC_ZALLOC(sizeof(struct fb_h264_s));
                        priv->pps              = mp4_demux->pps;
                        priv->pps_len          = mp4_demux->pps_len;
                        priv->sps              = mp4_demux->sps;
                        priv->sps_len          = mp4_demux->sps_len;
                        fb->priv               = (void *) priv;
                        priv->type             = 1;
                        priv->count            = count;
                        priv->start_len        = 0;
                    }
                    else
                    {
                        struct fb_h264_s *priv = (struct fb_h264_s *) STREAM_LIBC_ZALLOC(sizeof(struct fb_h264_s));
                        priv->type             = 2;
                        fb->priv               = (void *) priv;
                        priv->count            = count;
                        priv->start_len        = 0;
                    }
                    // os_printf("play_vframe_num:%d\tkey:%d\tfb:%X\tpriv:%X\tcount:%d\n",mp4_demux->play_vframe_num,is_key_frame(NULL, mp4_demux->play_vframe_num),fb,fb->priv,count);
                    count++;
                    msi_output_fb(mp4_demux->msi, fb);
                    _os_printf("M");
                    fb = NULL;
                }

                if (!err)
                {
                    msi_delete_fb(NULL, fb);
                    fb = NULL;
                    osal_fclose(mp4_demux->fp);
                    mp4_demux->fp = NULL;
                    goto mp4_demux_thread_open_again;
                }
            }

            mp4_demux->play_vframe_num++;
        }
    mp4_demux_audio:
        if (!mp4_demux->trak_t[1].init)
        {
            flag |= BIT(1);
            continue;
        }
        if (get_frame_num_pts(&mp4_demux->trak_t[1], mp4_demux->play_aframe_num) > os_jiffies() - last_play_time)
        {
            os_sleep_ms(1);
        }
        else
        {
            aframe_offset = get_frame_offset(&mp4_demux->trak_t[1], mp4_demux->play_aframe_num, &aframe_size);
            if ((!aframe_offset || aframe_size == 0))
            {
                flag |= BIT(1);
                if (flag == (BIT(0) | BIT(1)))
                {
                    break;
                }
            }
            if (aframe_size)
            {
                fb = fbpool_get(&mp4_demux->tx_pool, 0, mp4_demux->msi);
                while (!fb)
                {
                    os_sleep_ms(1);
                    fb = fbpool_get(&mp4_demux->tx_pool, 0, mp4_demux->msi);
                }
                fb->data = (uint8_t *) STREAM_MALLOC(aframe_size + 7);
                if (!fb->data)
                {
                    msi_delete_fb(NULL, fb);
                    os_sleep_ms(1);
                    continue;
                }
                fb->time  = get_frame_num_pts(&mp4_demux->trak_t[1], mp4_demux->play_aframe_num);
                fb->len   = aframe_size + 7;
                fb->mtype = F_AUDIO;
                osal_fseek(mp4_demux->fp, aframe_offset);
                err = osal_fread(fb->data + 7, 1, aframe_size, mp4_demux->fp);
                if (err)
                {
                    aac_dsi_to_adts(mp4_demux->aac_dsi, fb->data, aframe_size);
                    msi_output_fb(mp4_demux->msi, fb);
                    fb = NULL;
                }

                if (!err)
                {
                    msi_delete_fb(NULL, fb);
                    fb = NULL;
                    osal_fclose(mp4_demux->fp);
                    mp4_demux->fp = NULL;
                    goto mp4_demux_thread_open_again;
                }
                mp4_demux->play_aframe_num++;
            }
        }
    }
mp4_demux_thread_exit:
    if (mp4_demux->fp)
    {
        osal_fclose(mp4_demux->fp);
        mp4_demux->fp = NULL;
    }

    for (uint32_t i = 0; i <= mp4_demux->trak_index; i++)
    {
        main_box *box = &mp4_demux->trak_t[i].box;
        // 释放空间
        if (box->stts)
        {
            STREAM_FREE(box->stts);
        }

        if (box->stsc)
        {
            STREAM_FREE(box->stsc);
        }

        if (box->stsz)
        {
            STREAM_FREE(box->stsz);
        }

        if (box->stsz_time)
        {
            STREAM_FREE(box->stsz_time);
        }
        if (box->stco)
        {
            STREAM_FREE(box->stco);
        }

        if (box->stss)
        {
            STREAM_FREE(box->stss);
        }

        if (box->sps)
        {
            STREAM_FREE(box->sps);
        }

        if (box->pps)
        {
            STREAM_FREE(box->pps);
        }
    }
    os_event_set(&mp4_demux->evt, MP4_DEMUX_EXIT, NULL);
    os_printf("mp4_demux_thread exit\tret:%d\n", ret);
    // 这个时候才可以安全释放msi
    msi_put(msi);
}

static int32_t mp4_demux_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t                 ret       = RET_OK;
    struct mp4_demux_msi_s *mp4_demux = (struct mp4_demux_msi_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            os_event_wait(&mp4_demux->evt, MP4_DEMUX_EXIT, NULL, OS_EVENT_WMODE_OR, -1);
            os_printf("############################################%s:%d\n", __FUNCTION__, __LINE__);
            os_event_del(&mp4_demux->evt);
            fbpool_destroy(&mp4_demux->tx_pool);
            if (mp4_demux->fp)
            {
                osal_fclose(mp4_demux->fp);
            }

            if (mp4_demux->sps)
            {
                STREAM_LIBC_FREE(mp4_demux->sps);
                mp4_demux->sps = NULL;
            }

            if (mp4_demux->pps)
            {
                STREAM_LIBC_FREE(mp4_demux->pps);
                mp4_demux->pps = NULL;
            }

            if (mp4_demux->filename)
            {
                STREAM_FREE(mp4_demux->filename);
                mp4_demux->filename = NULL;
            }
            STREAM_LIBC_FREE(mp4_demux);
        }
        break;

        case MSI_CMD_PRE_DESTROY:
        {
            os_event_set(&mp4_demux->evt, MP4_DEMUX_STOP, NULL);
        }
        break;

        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            // os_printf("mp4_demux:%X\tfb:%X\tdata:%X\n", mp4_demux, fb, fb->data);
            if (fb->data)
            {
                STREAM_FREE(fb->data);
                fb->data = NULL;
            }

            if (fb->priv)
            {
                STREAM_LIBC_FREE(fb->priv);
                fb->priv = NULL;
            }
            fbpool_put(&mp4_demux->tx_pool, fb);
            // 不需要内核去释放fb
            ret = RET_OK + 1;
        }
        break;
#if 1
        // 这个属于媒体控制
        case MSI_CMD_VIDEO_DEMUX_CTRL:
        {
            uint32_t cmd_self = (uint32_t) param1;
            uint32_t arg      = param2;
            switch (cmd_self)
            {
                case MSI_VIDEO_DEMUX_JMP_TIME:
                {
                    uint32_t jmp_vframe_num = get_frame_num_from_pts(&mp4_demux->trak_t[0], arg);
                    os_printf("jmp_vframe_num:%d\n", jmp_vframe_num);
                    // 找不到,保持原样
                    if (jmp_vframe_num)
                    {
                        mp4_demux->jmp_vframe_num = jmp_vframe_num - 1;
                        // 唤醒线程,修改
                        os_event_set(&mp4_demux->evt, MP4_DEMUX_JMP, NULL);
                        // 返回当前播放的时间(h264就是关键帧的时间)
                        return get_frame_num_pts(&mp4_demux->trak_t[0], jmp_vframe_num);
                    }
                }

                break;

                case MSI_VIDEO_DEMUX_REWIND_TIME:
                    break;

                case MSI_VIDEO_DEMUX_SET_STATUS:
                    break;

                case MSI_VIDEO_DEMUX_GET_STATUS:
                    break;
                case MSI_VIDEO_DEMUX_START:
                {
                    os_event_set(&mp4_demux->evt, MP4_DEMUX_START, NULL);
                    break;
                }
                default:
                    break;
            }
        }
        break;
#endif

        default:
            break;
    }
    return ret;
}

struct msi *mp4_demux_msi_init(const char *msi_name, const char *filename)
{
    uint8_t                 isnew     = 0;
    struct msi             *msi       = msi_new(msi_name, 0, &isnew);
    struct mp4_demux_msi_s *mp4_demux = NULL;
    if (isnew)
    {
        mp4_demux = (struct mp4_demux_msi_s *) STREAM_LIBC_ZALLOC(sizeof(struct mp4_demux_msi_s));
        if (!mp4_demux)
        {
            goto mp4_demux_msi_init_err;
        }
        char *name     = STREAM_MALLOC(strlen(filename) + 1);
        mp4_demux->msi = msi;
        memcpy(name, filename, strlen(filename) + 1);
        mp4_demux->filename = name;
        msi->priv           = (void *) mp4_demux;
        // 先open文件
        mp4_demux->fp       = osal_fopen(name, "rb");
        if (!mp4_demux->fp)
        {
            os_printf("file open fail:%s\n", name);
            goto mp4_demux_msi_init_err;
        }
        msi->action = mp4_demux_msi_action;
        os_event_init(&mp4_demux->evt);
        fbpool_init(&mp4_demux->tx_pool, MAX_MP4_DEMUX_TX);

        uint32_t filesize = osal_fsize(mp4_demux->fp);
        uint32_t mp4_ret  = box_read(mp4_demux, "start", filesize);
        os_printf("mp4_ret:%d\n", mp4_ret);
        uint16_t sps_len, pps_len;
        uint8_t *sps = mp4_demux_get_sps(msi, &sps_len);
        uint8_t *pps = mp4_demux_get_pps(msi, &pps_len);
        os_printf("sps:%X\tlen:%d\n", sps, sps_len);
        os_printf("pps:%X\tlen:%d\n", pps, pps_len);

        mp4_demux->sps_len = sps_len;
        mp4_demux->pps_len = pps_len;

        if (sps && pps)
        {
            mp4_demux->sps = (uint8_t *) STREAM_LIBC_MALLOC(sps_len);
            mp4_demux->pps = (uint8_t *) STREAM_LIBC_MALLOC(pps_len);
            memcpy(mp4_demux->sps, sps, sps_len);
            memcpy(mp4_demux->pps, pps, pps_len);
        }
        else
        {
            os_printf("malloc sps or pps err\n");
            goto mp4_demux_msi_init_err;
        }

        msi->enable = 1;
    }
    else
    {
        goto mp4_demux_msi_init_err;
    }
    void *mp4_hdl = os_task_create("mp4_demux", mp4_demux_thread, msi, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2048);
    // void *mp4_hdl = os_task_create("mp4", mp4_encode_thread, msi, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2048);
    os_printf("mp4_hdl:%X\n", mp4_hdl);
    //  正常退出
    goto mp4_demux_msi_init_end;

mp4_demux_msi_init_err:
    if (mp4_demux)
    {
        STREAM_LIBC_FREE(mp4_demux);
    }
    // 不要重复打开,同一个名称需要等待上一次写入完成后并且关闭后才可以重新打开
    // 这里是因为可能有错误,但是又new了一次,所以删除
    // mp4_demux由destroy内部去释放
    if (msi)
    {
        msi_destroy(msi);
        msi = NULL;
    }
mp4_demux_msi_init_end:
    return msi;
}

uint8_t *mp4_demux_get_sps(struct msi *msi, uint16_t *sps_len)
{
    struct mp4_demux_msi_s *mp4_demux = (struct mp4_demux_msi_s *) msi->priv;
    main_box               *box       = &mp4_demux->trak_t[0].box;
    if (box->sps)
    {
        if (sps_len)
        {
            *sps_len = box->sps_len;
        }
        return box->sps;
    }
    return NULL;
}

uint8_t *mp4_demux_get_pps(struct msi *msi, uint16_t *pps_len)
{
    struct mp4_demux_msi_s *mp4_demux = (struct mp4_demux_msi_s *) msi->priv;
    main_box               *box       = &mp4_demux->trak_t[0].box;
    if (box->pps)
    {
        if (pps_len)
        {
            *pps_len = box->pps_len;
        }
        return box->pps;
    }
    return NULL;
}

uint8_t is_key_frame(trak *trak_t, uint32_t frame_num)
{
    main_box *box = &trak_t->box;
    uint32_t  last_key_frame, next_key_frame;
    // 没有stss,默认所有都是关键帧
    if (!box->stss)
    {
        return 1;
    }
    frame_num = frame_num + 1; // MP4内部是从1开始,数组搜索是从0开始,所以需要+1
    // 尝试快速索引一下关键帧的位置
    if (box->key_frame_offset < box->stco_count)
    {
        if (box->key_frame_offset < box->stco_count - 1)
        {
            last_key_frame = BIG4_ENDIAN(box->stss[box->key_frame_offset].sample_number);
            next_key_frame = BIG4_ENDIAN(box->stss[box->key_frame_offset + 1].sample_number);

            // 判断是否在范围
            if (last_key_frame <= frame_num && next_key_frame > frame_num)
            {
                return frame_num == last_key_frame;
            }
            // 不在范围,往后面去搜索
            else if (frame_num > next_key_frame)
            {
                last_key_frame = BIG4_ENDIAN(box->stss[box->key_frame_offset].sample_number);
                for (int i = box->key_frame_offset; i < box->stss_count; i++, box->key_frame_offset++)
                {
                    next_key_frame = BIG4_ENDIAN(box->stss[i].sample_number);
                    // 找到了,退出,应该返回last_find_main_frame
                    if (next_key_frame > frame_num)
                    {

                        return frame_num == last_key_frame;
                    }
                    last_key_frame = next_key_frame;
                }
                return 0;
            }
            // 重头搜索
            else
            {
                last_key_frame        = BIG4_ENDIAN(box->stss[0].sample_number);
                box->key_frame_offset = 0;
                for (int i = 0; i < box->stss_count; i++, box->key_frame_offset++)
                {
                    next_key_frame = BIG4_ENDIAN(box->stss[i].sample_number);
                    // 找到了,退出,应该返回last_find_main_frame
                    if (next_key_frame > frame_num)
                    {
                        return frame_num == last_key_frame;
                    }
                    last_key_frame = next_key_frame;
                }
                return 0;
            }
        }
        // box->key_frame_offset 特殊,搜索已经到最后了
        else
        {
            last_key_frame = BIG4_ENDIAN(box->stss[box->key_frame_offset].sample_number);
            if (last_key_frame <= frame_num)
            {
                return frame_num == last_key_frame;
            }

            // 小,要重头开始搜索
            else
            {
                last_key_frame        = BIG4_ENDIAN(box->stss[0].sample_number);
                box->key_frame_offset = 0;
                for (int i = 0; i < box->stss_count; i++, box->key_frame_offset++)
                {
                    next_key_frame = BIG4_ENDIAN(box->stss[i].sample_number);
                    // 找到了,退出,应该返回last_find_main_frame
                    if (next_key_frame > frame_num)
                    {
                        return frame_num == last_key_frame;
                    }
                    last_key_frame = next_key_frame;
                }
            }
        }
    }
    // 异常?暂时不支持大于,如果有必要,可以采用重头开始搜索,但这里不处理
    else
    {
    }

    return 0;
}