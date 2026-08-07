#ifndef __MP4_ENCODE_H
#define __MP4_ENCODE_H

#include "basic_include.h"
#include "fatfs/osal_file.h"
#define mp4_moov mp4_common_box_s
#define mp4_trak mp4_common_box_s
#define mp4_mdia mp4_common_box_s
#define mp4_minf mp4_common_box_s
#define mp4_dinf mp4_common_box_s
#define mp4_stbl mp4_common_box_s
#define mp4_mdat mp4_common_box_s
#define mp4_free mp4_common_box_s
#define mp4_esds mp4_common_box_full_s

/************************************************************************/
/*          Some values of MP4(E/D)_track_t->object_type_indication     */
/************************************************************************/
// MPEG-4 AAC (all profiles)
#define MP4_OBJECT_TYPE_AUDIO_ISO_IEC_14496_3              0x40
// MPEG-2 AAC, Main profile
#define MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_MAIN_PROFILE 0x66
// MPEG-2 AAC, LC profile
#define MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_LC_PROFILE   0x67
// MPEG-2 AAC, SSR profile
#define MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_SSR_PROFILE  0x68
// H.264 (AVC) video
#define MP4_OBJECT_TYPE_AVC                                0x21
// H.265 (HEVC) video
#define MP4_OBJECT_TYPE_HEVC                               0x23
// http://www.mp4ra.org/object.html 0xC0-E0  && 0xE2 - 0xFE are specified as "user private"
#define MP4_OBJECT_TYPE_USER_PRIVATE                       0xC0

#define MAX_PPS_SPS_LEN (32)

#ifndef SEEK_SET
#define SEEK_SET 0 /* set file offset to offset */
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1 /* set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define SEEK_END 2 /* set file offset to EOF plus offset */
#endif

enum
{
    MP4_OK,
    MP4_INIT_ERR,
    MP4_FULL_ERR,
    MP4_V_TYPE_ERR,
    MP4_I_ERR,
    MP4_B_ERR,
    MP4_P_ERR,
    MP4_WRITE_ERR,
    MP4_AUDIO_ERR,
};



typedef struct
{
    uint32_t sample_count;
    uint32_t delta;
} stts_entries;


// 这里记录的都是对应表格的起始地址,分配的缓冲区长度都是512?
// 如果512写入完毕,则offset+512,
typedef struct
{
    uint32_t type; // 1:视频  2:音频,0是无效
    uint32_t tkhd_duration_offset;
    uint32_t mdhd_duration_offset; // time scale为时间单位
    uint32_t duration;
    uint32_t count; // 记录保存了多少帧数据
    uint32_t stts_offset;
    uint32_t stsc_offset;
    uint32_t stsz_offset;
    uint32_t stco_offset;
    uint32_t stss_offset;

    uint32_t     stts_write_offset; // 当前需要写入的位置
    uint32_t     stts_need_write_len;
    uint32_t     stts_count;
    stts_entries last_entries; // 如果相同,则需要一直增加,减少stts的空间

    uint32_t stsc_write_offset; // 当前需要写入的位置
    uint32_t stsc_need_write_len;
    uint32_t stsc_count;

    uint32_t stsz_write_offset; // 当前需要写入的位置
    uint32_t stsz_need_write_len;
    uint32_t stsz_count;

    uint32_t stco_write_offset; // 当前需要写入的位置
    uint32_t stco_need_write_len;
    uint32_t stco_count;

    uint32_t stss_write_offset; // 当前需要写入的位置
    uint32_t stss_need_write_len;
    uint32_t stss_count;

    uint8_t stts_tmp_buf[512]; // 除了这个是比较特殊外,其他都是写完清0
    uint8_t stsc_tmp_buf[512];
    uint8_t stsz_tmp_buf[512];
    uint8_t stco_tmp_buf[512];
    uint8_t stss_tmp_buf[512];

} trak_key_msg;


// pps和sps预分配空间,就不动态申请了,考虑用psram,多几十字节
typedef struct
{
    uint8_t     cache[512];
    uint8_t  init;
    uint8_t  audio_enable;
    uint16_t asps_len;
    uint8_t *asps_data;
    uint16_t video_w;
    uint16_t video_h;
    uint32_t msg_end;

    uint16_t pps_len;
    uint16_t sps_len;
    uint8_t *sps_data;
    uint8_t *pps_data;

    uint32_t     trak_count;
    uint32_t     file_max_size;
    uint32_t     mdat_size;        // 记录mdat的实际size,如果后续需要用到,则调用
    uint32_t     mdat_size_offset; // 记录mdat的size变量偏移
    uint32_t     mdat_offset;
    uint32_t     mdat_nowoffset;
    uint32_t     mvhd_duration_offset;
    F_FILE      *fp;
    // 设置缓冲区
    trak_key_msg trak[2];
} mp4_key_msg;


typedef struct
{
    uint32_t size;
    char     boxname[4];
} mp4_common_box_s; // 通用结构体

typedef struct
{
    uint32_t size;
    char     boxname[4];
    uint32_t version : 8, flags : 24;
} mp4_common_box_full_s; // 通用结构体

typedef struct
{
    char     major[4];
    uint32_t version;
    char    *compatible_brands; // 字符串,每一个是4byte(不能带'\0'),默认"mp42isom"
} mp4_ftyp;

typedef struct
{
    uint32_t size;
    char     boxname[4];
    uint32_t version : 8, flags : 24; // version+flags
    uint32_t creation_time;
    uint32_t modification_time;
    uint32_t timescale;
    uint32_t duration;
    uint32_t rate;
    uint16_t volume;
    uint8_t  reserved1[2];
    uint8_t  reserved2[8];
    uint32_t matrix[9];
    uint32_t pre_defined[6];
    uint32_t next_track_id;
} mp4_mvhd;

typedef struct
{
    uint32_t size; // 采用预留方式,可以动态添加数据
    char     boxname[4];
    uint32_t version : 8, flags : 24; // version+flags
    uint32_t creation_time;
    uint32_t modification_time;
    uint32_t track_ID;
    uint8_t  reserved1[4];
    uint32_t duration;
    uint8_t  reserved2[8];
    uint16_t layer;
    uint16_t alternate_group;
    uint16_t volume;
    uint8_t  reserved3[2];
    uint32_t matrix[9];
    uint32_t width;  // 16整数+16小数
    uint32_t height; // 16整数+16小数
} mp4_tkhd;

typedef struct
{
    uint32_t size;
    char     boxname[4];
    uint32_t version : 8, flags : 24; // version+flags
    uint32_t creation_time;
    uint32_t modification_time;
    uint32_t timescale;
    uint32_t duration;
    uint16_t language;
    uint16_t pre_defined;
} mp4_mdhd;

typedef struct
{
    uint32_t size;
    char     boxname[4];
    uint32_t version : 8, flags : 24; // version+flags
    uint32_t pre_defined;
    char     handler_type[4];
    uint8_t  reserved[12];
    // char name[1];           //可变参数
} mp4_hdlr;

typedef struct
{
    uint32_t size;
    char     boxname[4];
    uint32_t version : 8, flags : 24; // version+flags
    uint16_t graphicsmode;
    uint16_t opcolorR;
    uint16_t opcolorG;
    uint16_t opcolorB;
} mp4_vmhd;

typedef struct
{
    uint32_t size;
    char     boxname[4];
    uint32_t version : 8, flags : 24; // version+flags
    uint32_t entry_count;
} mp4_dref;

typedef struct
{
    uint32_t size;
    char     boxname[4];
    uint32_t version : 8, flags : 24; // version+flags
} mp4_url;

typedef struct
{
    uint32_t size;
    char     boxname[4];
    uint32_t version : 8, flags : 24; // version+flags
    uint32_t entry_count;
} mp4_stsd;

typedef struct __attribute__((packed))
{
    uint32_t size;
    char     boxname[4];
    uint8_t  reserved1[6];
    uint16_t data_reference_index;
    uint8_t  pre_defined1[16];
    uint16_t width;
    uint16_t height;
    uint32_t horiz_resolution;
    uint32_t vert_resolution;
    uint8_t  reserved2[4];
    uint16_t frame_count;
    uint8_t  compressorname[32];
    uint16_t depth;
    uint8_t  pre_defined2[2];
} mp4_avc1;

// 因为涉及pps和sps,所以需要找到对应pps和sps才能初始化
// 测试可以先采用固定模式
typedef struct __attribute__((packed))
{
    uint32_t size;
    char     boxname[4];
    uint8_t  configurationVersion;  // 1
    uint8_t  AVCProfileIndication;  // sps[3]
    uint8_t  profile_compatibility; // sps[4]
    uint8_t  AVCLevelIndication;    // sps[5]
    uint8_t  lengthSizeMinusOne;    // 0xff
#if 0
    uint8_t numOfSPS;
    uint8_t *sps_data;
    uint16_t sps_data_size;
    uint8_t numOfPPS;
    uint8_t *pps_data;
    uint16_t pps_data_size;
#endif
} mp4_avcC;

typedef struct
{
    uint8_t  numOfSPS;
    uint8_t *sps_data;
    uint16_t sps_data_size;
    uint8_t  numOfPPS;
    uint8_t *pps_data;
    uint16_t pps_data_size;
} mp4_sps_pps;

typedef struct __attribute__((packed))
{
    uint32_t size;
    char     boxname[4];
    uint32_t version : 8, flags : 24; // version+flags
    uint32_t entry_count;             // 应该采用预留空间的方法
    union
    {
        stts_entries entries;
        stts_entries end;
    };
} mp4_stts;

typedef struct
{
    uint32_t first_chunk;
    uint32_t per_chunk;
    uint32_t index;
} stsc_entries;
typedef struct __attribute__((packed))
{
    uint32_t size;
    char     boxname[4];
    uint32_t version : 8, flags : 24; // version+flags
    uint32_t entry_count;             // 应该采用预留空间的方法
    union
    {
        stsc_entries entries;
        stsc_entries end;
    };

} mp4_stsc;

typedef struct __attribute__((packed))
{
    uint32_t size;
    char     boxname[4];
    uint32_t version : 8, flags : 24; // version+flags
    uint32_t sample_size;             // 应该采用预留空间的方法
    uint32_t sample_count;
    union
    {
        uint32_t entries;
        uint32_t end;
    };
} mp4_stsz;

typedef struct __attribute__((packed))
{
    uint32_t size;
    char     boxname[4];
    uint32_t version : 8, flags : 24; // version+flags
    uint32_t entry_count;             // 应该采用预留空间的方法
    union
    {
        uint32_t chunk_offsets;
        uint32_t end;
    };
} mp4_stco;

typedef struct __attribute__((packed))
{
    uint32_t size;
    char     boxname[4];
    uint32_t version : 8, flags : 24; // version+flags
    uint32_t entry_count;             // 应该采用预留空间的方法
    union
    {
        uint32_t sample_numbers;
        uint32_t end;
    };
} mp4_stss;

typedef struct
{
    uint32_t size;
    char     boxname[4];
    uint16_t balance;
    uint16_t reserved;

} mp4_smhd;

typedef struct __attribute__((packed))
{
    uint32_t size;
    char     boxname[4];
    uint8_t  reserved4[4];
    uint8_t  reserved2[2];
    uint16_t dataReferenceIndex;
    uint16_t version, revisionLevel;
    uint32_t vendor;
    uint16_t channelCount;
    uint16_t sampleSize;
    uint8_t  reserved[4];
    uint32_t time_scale;

} mp4_mp4a;

typedef uint32_t (*mp4_func)(F_FILE *fp, uint32_t offset, mp4_key_msg *msg);

typedef struct
{
    uint32_t offset;
    mp4_func func;
} mp4_func_stack;




uint32_t write_aac_data(mp4_key_msg *msg, uint8_t *aac_buf, uint32_t size, uint32_t duration);
uint32_t mp4_syn(mp4_key_msg *msg);
void *MP4_open_init(F_FILE *fp, uint8_t audio_en);
uint32_t mp4_audio_cfg_init(mp4_key_msg *msg, uint8_t *asps_data, uint8_t len);
uint32_t mp4_set_max_size(mp4_key_msg *msg, uint32_t max_size);
uint32_t mp4_video_cfg_init(mp4_key_msg *msg, uint16_t w, uint16_t h);
uint32_t write_h264_pps_sps(mp4_key_msg *msg, uint8_t *nal_buf, uint32_t size);
uint32_t write_h264_data(mp4_key_msg *msg, uint8_t *nal_buf, uint32_t size, uint32_t duration);
uint32_t mp4_deinit(mp4_key_msg *msg);
#endif