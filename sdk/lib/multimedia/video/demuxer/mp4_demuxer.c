#include "basic_include.h"
#include "hal/vcodec.h"
#include "lib/multimedia/framebuff.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/AVContainer.h"

#define mp4_dbg(fmt, ...)      //os_printf("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define mp4_err(fmt, ...)      os_printf(KERN_ERR"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define mp4_warn(fmt, ...)     os_printf(KERN_WARNING fmt, ##__VA_ARGS__)

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
//#define MP4_SAVE_TRACK   //网络播放时保存track数据到SD卡
//#define MP4_SAVE_MDAT    //网络播放时保存mdat数据到SD卡
int fflush(void *stream);
int fclose(void *stream);
int fseek(void *stream, off_t offset, int whence);
size_t fread(void *ptr, size_t size, size_t nmemb, void *stream);
void *fopen(const char *filename, const char *mode);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, void *stream);
/////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////
#define IO_BUFFER_SIZE      (8*1024)
#define MP4_MAX_TRACKS      (8)
#define MP4_BOX_HEADER_SIZE (8)
#define MP4_BOX_DEPTH       (16)

#define MP4_TAG_FMT      "%c%c%c%c"
#define MP4_TAG_STR(tag) (char)((tag) >> 24), (char)((tag) >> 16), (char)((tag) >> 8), (char)(tag)

/////////////////////////////////////////////////////////////////////////////////////////////////////
// 常见MP4 Box类型标识 (四字符转32位无符号整数)
#define MP4_TAG(a,b,c,d) (((a)<<24) | ((b)<<16) | ((c)<<8) | (d))
#define BOX_FTYPE          MP4_TAG('f','t','y','p')   // 文件类型
#define BOX_MOOV           MP4_TAG('m','o','o','v')   // 媒体元数据根容器
#define BOX_MDAT           MP4_TAG('m','d','a','t')   // 音视频数据
#define BOX_FREE           MP4_TAG('f','r','e','e')   // 空闲空间
#define BOX_SKIP           MP4_TAG('s','k','i','p')   // 跳过数据
#define BOX_MOOF           MP4_TAG('m','o','o','f')   // 分片片段头 (fMP4)
#define BOX_MFRA           MP4_TAG('m','f','r','a')   // 随机访问索引
#define BOX_MVHD           MP4_TAG('m','v','h','d')   // 电影头
#define BOX_TRAK           MP4_TAG('t','r','a','k')   // 轨道
#define BOX_UDTA           MP4_TAG('u','d','t','a')   // 用户数据
#define BOX_META           MP4_TAG('m','e','t','a')   // 元数据
#define BOX_IODS           MP4_TAG('i','o','d','s')   // 初始对象描述
#define BOX_ODS            MP4_TAG('o','d','s',' ')   // 媒体对象描述
#define BOX_TKHD           MP4_TAG('t','k','h','d')   // 轨道头
#define BOX_MDIA           MP4_TAG('m','d','i','a')   // 媒体信息
#define BOX_EDTS           MP4_TAG('e','d','t','s')   // 编辑列表
#define BOX_TRGR           MP4_TAG('t','r','g','r')   // 轨道分组
#define BOX_ELST           MP4_TAG('e','l','s','t')   // 编辑列表项
#define BOX_MDHD           MP4_TAG('m','d','h','d')   // 媒体头
#define BOX_HDLR           MP4_TAG('h','d','l','r')   // 处理器类型
#define BOX_MINF           MP4_TAG('m','i','n','f')   // 媒体信息容器
#define BOX_VMHD           MP4_TAG('v','m','h','d')   // 视频媒体头
#define BOX_SMHD           MP4_TAG('s','m','h','d')   // 音频媒体头
#define BOX_HMHD           MP4_TAG('h','m','h','d')   // 提示轨道头
#define BOX_NMHD           MP4_TAG('n','m','h','d')   // 空媒体头
#define BOX_DINF           MP4_TAG('d','i','n','f')   // 数据信息
#define BOX_STBL           MP4_TAG('s','t','b','l')   // 样本表（核心）
#define BOX_DREF           MP4_TAG('d','r','e','f')   // 数据引用
#define BOX_URL            MP4_TAG('u','r','l',' ')   // 本地资源
#define BOX_URN            MP4_TAG('u','r','n',' ')   // 网络资源
#define BOX_STSD           MP4_TAG('s','t','s','d')   // 样本描述
#define BOX_STTS           MP4_TAG('s','t','t','s')   // 解码时间戳（DTS）
#define BOX_CTTS           MP4_TAG('c','t','t','s')   // 显示时间偏移（PTS）
#define BOX_STSC           MP4_TAG('s','t','s','c')   // sample -> chunk
#define BOX_STSZ           MP4_TAG('s','t','s','z')   // 样本大小
#define BOX_STZ2           MP4_TAG('s','t','z','2')   // 紧凑样本大小
#define BOX_STCO           MP4_TAG('s','t','c','o')   // chunk 偏移(32位)
#define BOX_CO64           MP4_TAG('c','o','6','4')   // chunk 偏移(64位)
#define BOX_STSS           MP4_TAG('s','t','s','s')   // 关键帧列表
#define BOX_STPS           MP4_TAG('s','t','p','s')   // 逐步播放样本
#define BOX_STSH           MP4_TAG('s','t','s','h')   // 样本组
#define BOX_SDTP           MP4_TAG('s','d','t','p')   // 样本依赖类型
#define BOX_SGPD           MP4_TAG('s','g','p','d')   // 样本组定义
#define BOX_SBGP           MP4_TAG('s','b','g','p')   // 样本到组映射
#define BOX_AVC1           MP4_TAG('a','v','c','1')   // H.264 视频
#define BOX_AVC2           MP4_TAG('a','v','c','2')   // H.264
#define BOX_HVC1           MP4_TAG('h','v','c','1')   // H.265
#define BOX_HEV1           MP4_TAG('h','e','v','1')   // H.265
#define BOX_MP4A           MP4_TAG('m','p','4','a')   // AAC 音频
#define BOX_MP3            MP4_TAG('m','p','3',' ')   // MP3 音频
#define BOX_G711           MP4_TAG('g','7','1','1')   // G711
#define BOX_G726           MP4_TAG('g','7','2','6')   // G726
#define BOX_PCM            MP4_TAG('p','c','m',' ')   // PCM音频
#define BOX_TEXT           MP4_TAG('t','e','x','t')   // 字幕
#define BOX_SBUT           MP4_TAG('s','b','u','t')   // 字幕
#define BOX_AVCC           MP4_TAG('a','v','c','C')   // H.264 配置（SPS/PPS）
#define BOX_HVCC           MP4_TAG('h','v','c','C')   // H.265 配置
#define BOX_ESDS           MP4_TAG('e','s','d','s')   // AAC 音频配置
#define BOX_D263           MP4_TAG('d','2','6','3')   // H.263
#define BOX_G726           MP4_TAG('g','7','2','6')   // G726
#define BOX_TRAF           MP4_TAG('t','r','a','f')   // 轨道片段
#define BOX_MFHD           MP4_TAG('m','f','h','d')   // 片段头
#define BOX_TFHD           MP4_TAG('t','f','h','d')   // 轨道片段头
#define BOX_TRUN           MP4_TAG('t','r','u','n')   // 轨道运行样本
#define BOX_SAIO           MP4_TAG('s','a','i','o')   // 辅助数据偏移
#define BOX_SAIZ           MP4_TAG('s','a','i','z')   // 辅助数据大小
#define BOX_BTRT           MP4_TAG('b','t','r','t')   // 码率信息
#define BOX_PSSH           MP4_TAG('p','s','s','h')   // 保护系统数据
#define BOX_SENC           MP4_TAG('s','e','n','c')   // 样本加密
#define BOX_TENC           MP4_TAG('t','e','n','c')   // 轨道加密
#define BOX_SINF           MP4_TAG('s','i','n','f')   // 样本信息
#define BOX_FRMA           MP4_TAG('f','r','m','a')   // 原始格式
#define BOX_SCHM           MP4_TAG('s','c','h','m')   // 加密方案
#define BOX_SCHI           MP4_TAG('s','c','h','i')   // 加密头信息
#define BOX_NAME           MP4_TAG('n','a','m','e')   // 标题
#define BOX_COPY           MP4_TAG('c','p','r','t')   // 版权
#define BOX_SOAM           MP4_TAG('s','o','a','m')   // 作者
#define BOX_DESC           MP4_TAG('d','e','s','c')   // 描述
#define BOX_ALBUM          MP4_TAG('a','l','b','m')   // 专辑
#define BOX_ARTIST         MP4_TAG('a','r','t','s')   // 艺术家
#define BOX_DATE           MP4_TAG('d','a','t','a')   // 日期
#define BOX_GENRE          MP4_TAG('g','n','r','e')   // 风格

/////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct {
    uint8_t   object_type_indication;   /* 编码类型，如 0x40 = AAC */
    uint8_t   stream_type;
    uint8_t   up_stream;
    uint8_t   dsi_len;
    uint8_t   dsi_data[8];              /* 8byte 是否足够?? */
    uint32_t  buffer_size_db;           /* 24 位有效 */
    uint32_t  max_bitrate;              /* bps */
    uint32_t  avg_bitrate;              /* bps */

    union {
        struct {
            uint8_t  audio_object_type;   /* 5 bits */
            uint8_t  sampling_freq_index; /* 4 bits */
            uint8_t  channel_config;      /* 4 bits */
            uint32_t sample_rate;         /* Hz */
            uint8_t  channels;
        } aac;
        /* 可添加 mp3, ac3 等结构体 */
    } specific;
} esds_info_t;

typedef struct {
    uint8_t   profile_space;          // 2 bits
    uint8_t   tier_flag;              // 1 bit
    uint8_t   profile_idc;            // 5 bits
    uint8_t   profile_compatibility;  // 32 bits，实际只存低8位？标准为4字节，这里简化
    uint8_t   level_idc;              // 8 bits
    uint8_t   nal_len_bytes;          // NAL 长度字段字节数 (1,2,4)
    uint8_t  *vps_data;
    uint16_t  vps_size;
    uint8_t  *sps_data;
    uint16_t  sps_size;
    uint8_t  *pps_data;
    uint16_t  pps_size;
} hevc_codec_info_t;

struct mp4_mdat_buf {
    uint32 rpos;
    uint32 wpos;
    uint32 qsize;
    char  *buf;
};

/* MP4 box信息cache机制 */
struct mp4_box_cache {
    void    *entries;           //cache的缓存buffer
    uint32_t cache_count;       //cache中当前缓存的条目数量
    uint32_t start_index;       //cache中第一个条目的索引
};

/* MP4 box信息缓存 */
struct mp4_box_info {
    struct mp4_box_cache cache1; //使用全量加载时仅填充cache1
    struct mp4_box_cache cache2; //双cache乒乓加载
    uint32_t cache_size;         //cache可缓存的条目数量
    uint32_t cur_cache;          //当前使用的cache
    uint32_t total_count;        //box信息条目总数量
    uint64_t offset;             //box的信息数据在文件中的偏移
};

/* STSC: Sample-to-Chunk Box, 描述chunk与sample的映射关系：每个 Chunk 包含多少个 Sample
   Chunk 是 MP4 中媒体数据的最小存储单元（一个 Chunk 包含 N 个 Sample），STSC 是「Sample → Chunk」的核心映射表。
*/
struct stsc_entry {
    uint32_t first_chunk;      // 起始块索引（1-based）
    uint32_t samples_per_chunk;// 每个块中的样本数
};

/* STTS: Time-to-Sample Box，解码时间戳表
   每个条目表示连续相同增量的一批样本(Time-to-Sample Box)
   记录每个样本的解码时间增量（delta），用于计算每个样本的解码时间戳（DTS）。
*/
struct stts_entry {
    uint32_t sample_count;     // 连续拥有相同 sample_delta 的样本数量；
    uint32_t sample_delta;     // 每个样本的时间增量（单位：轨道的 time_scale，如 time_scale=1000 则 delta=40 表示 40ms）。
};

/* STSS: Sync Sample Table，同步样本表，也常称关键帧索引表
   快速定位关键帧：视频编码中，关键帧（I 帧）可独立解码，非关键帧（P/B 帧）依赖前序帧；
*/
struct stss_entry { uint32_t sample_index; };

/* STCO/CO64 : Chunk Offset Box，块偏移量表.
   记录每个 Chunk 在文件中的起始字节偏移：
     STCO：32 位偏移（适用于文件大小 < 4GB）；
     CO64：64 位偏移（适用于文件大小 ≥ 4GB）。
*/
struct stco_entry { uint32_t chunk_offset; };
struct stco64_entry { uint64_t chunk_offset; };

/* STSZ: Sample Size Box，样本大小表：记录每个样本的字节大小。
   分为「固定大小」和「可变大小」两种模式
*/
struct stsz_entry {    uint32_t sample_size; };
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

struct mp4_track {
    uint8_t  mtype;              // 媒体类型（视频/音频/字幕等，见media_types.h）
    uint8_t  stype;              // 具体编码类型（如H264、AAC等）
    uint8_t  use_co64;
    uint8_t  track_idx;

    uint32_t track_id;           // 轨道ID（1-based）
    uint32_t time_scale;         // 该轨道的时间刻度（每秒的ticks数）
    uint32_t sample_size;        // 如果所有样本大小相同，则此值为固定大小，否则为0
    uint64_t duration;           // 轨道总时长（以time_scale为单位）

    void     *codec_data;        // 解码器配置 (avcC/hvCc/esds)
    uint32_t  codec_len;         // 配置长度

    struct mp4_box_info stss;    //stss-关键帧索引表          - 全量加载
    struct mp4_box_info stsc;    //stsc-样本到块映射表         - 全量加载
    struct mp4_box_info stts;    //stts-解码时间戳表          - 全量加载
    struct mp4_box_info stco;    //stco-块偏移量表           - 惰性加载
    struct mp4_box_info stsz;    //stsz-样本大小表           - 惰性加载

    uint32_t next_sample_index;  // 下一个sample的索引号
    uint32_t next_sample_size;   // 下一个sample的size
    uint32_t next_sample_time;   // 下一个sample的time(毫秒)
    uint64_t next_sample_offset; // 下一个sample的偏移
    uint32_t seek_sample_index;  // seek选中的sample索引

    // stsc 上一次查找结果
    uint32_t stsc_last_entry;          // 上次命中的条目索引
    uint32_t stsc_last_samples_before; // 命中的 chunk 之前的累计 sample 数
    uint32_t stsc_last_chunk_start;    // 命中的 chunk 编号

    //编码器参数信息（根据媒体类型使用对应的结构体）
    union {
        txVideoInfo_t    video;
        txAudioInfo_t    audio;
        txSubtitleInfo_t subtitle;
    } codec_info;

#ifdef MP4_SAVE_TRACK
    void *fp_track;
#endif
};

struct mp4_context;
typedef int32_t (*box_hdl)(struct mp4_context *c);

// MP4 box解析栈
struct mp4_box {
    struct mp4_box *parent;
    uint64_t box_size;
    uint64_t read_size;
    uint32_t box_tag;
    box_hdl  hdl;
    uint8_t  skip: 1;       //跳过后续的数据
    uint8_t  child_box: 1;  //进入子box解析阶段
};

// MP4解复用器上下文
struct mp4_context {
    const struct AVDemuxerOps *ops; // 文件操作接口（read/seek/eof等）
    void       *file;               // 文件句柄（传递给ops使用）
    struct msi *owner;              // 上层模块句柄（用于分配帧缓冲等）
    uint64_t    file_size;          // 文件/数据流总大小。-1 表示为直播流
    uint64_t    offset;             // 记录当前的文件读取偏移。
    uint64_t    offset_bak;         // 记录seek之前的偏移。
    uint64_t    mdat_offset;        // 记录mdat box在文件中的偏移
    uint8_t     stream_type;        // 0:本地文件，1:网络流，2:直播流
    uint8_t     track_cnt;          // 实际轨道数量
    int8_t      cur_box;            // 当前正在解析的Box在栈中的索引（0xFF表示无）
    int8_t      cur_track;          // 当前在解析哪个track
    int8_t      video_track;        // 当前的 Video Track
    int8_t      audio_track;        // 当前的 Audio Track
    int8_t      subtitle_track;     // 当前的 Subtitle Track
    uint8_t     moov_parsed : 1;    // moov信息是否已被解析
    uint8_t     moov_tail   : 1;    // moov信息在文件末尾
    uint8_t     buffering   : 1;    // 缓存数据，暂停demux
    uint8_t     iobuf_dis   : 1;    // io_buf是否被禁用。支持后续优化播放网络流：打开第2个链接重新加载moov信息，不影响当前的数据缓冲

    struct mp4_track *tracks[MP4_MAX_TRACKS]; // 轨道指针数组
    struct mp4_box    boxs[MP4_BOX_DEPTH];    // Box栈，支持最多16层嵌套

    struct framebuff *cur_frame;    // 当前正在组装中的帧缓冲
    uint32_t cur_frame_len;         // 当前帧已填充的字节数
    uint32_t box_cache_size;
    uint32_t delay_seek_time;       // 保存的seek时间
    uint64_t total_duration_ms;     // 文件总时长（毫秒）

    struct rbuffer io_buf;

#ifdef MP4_SAVE_MDAT
    void *fp_mdat;
#endif
} ;

#ifdef MP4_SAVE_TRACK
void mp4_save_track_to_file(struct mp4_context *c, struct mp4_track *track, void *data, uint32 size)
{
    if (c->stream_type == AVDEMUXER_STREAM_URLFILE) {
        if (track->fp_track == NULL) {
            char filename[32];
            sprintf(filename, "%s_track%d", c->owner->name, track->track_idx);
            track->fp_track = fopen(filename, "w+");
        }
        if (track->fp_track) {
            fwrite(data, 1, size, track->fp_track);
        }
    }
}
#else
#define mp4_save_track_to_file(c, track, data, size)
#endif
#ifdef MP4_SAVE_MDAT
void mp4_save_mdat_to_file(struct mp4_context *c, void *data, uint32 size)
{
    if (c->stream_type == AVDEMUXER_STREAM_URLFILE) {
        if (c->fp_mdat == NULL) {
            char filename[32];
            sprintf(filename, "%s_mdat", c->owner->name);
            c->fp_mdat = fopen(filename, "w+");
        }
        if (size && c->fp_mdat) {
            fwrite(data, 1, size, c->fp_mdat);
        }
    }
}
#else
#define mp4_save_mdat_to_file(c, data, size)
#endif

/////////////////////////////////////////////////////////////////////////////
// ESDS 数据解析
/* 采样率表 (AAC) */
static const uint32_t aac_sampling_freq_table[] = {
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
    16000, 12000, 11025, 8000, 7350
};

typedef int (*mp4_dsi_parser_func)(struct mp4_context *c, struct mp4_track *track, esds_info_t *info);

/* 读取 BER 编码的长度 (最多4字节) */
static uint32_t mp4_esds_read_ber_length(const uint8_t **ptr, const uint8_t *end)
{
    uint32_t len = 0;
    int bytes = 0;
    while (*ptr < end && bytes < 4) {
        uint8_t b = *(*ptr)++;
        len = (len << 7) | (b & 0x7F);
        bytes++;
        if (!(b & 0x80)) {
            break;
        }
    }
    return len;
}

/* 解析 AAC 的 DecoderSpecificInfo */
static int mp4_esds_parse_aac_dsi(struct mp4_context *c, struct mp4_track *track, esds_info_t *info)
{
    if (info->dsi_len < 2) {
        return -1;
    }

    uint16_t asc = (info->dsi_data[0] << 8) | info->dsi_data[1];
    info->specific.aac.audio_object_type = (asc >> 11) & 0x1F;
    info->specific.aac.sampling_freq_index = (asc >> 7) & 0x0F;
    info->specific.aac.channel_config = (asc >> 3) & 0x0F;

    /* 获取实际采样率 */
    if (info->specific.aac.sampling_freq_index < 13) {
        info->specific.aac.sample_rate = aac_sampling_freq_table[info->specific.aac.sampling_freq_index];
        track->codec_info.audio.sample_rate = info->specific.aac.sample_rate;
    } else if (info->specific.aac.sampling_freq_index == 15 && info->dsi_len >= 5) {
        uint32_t sr = (info->dsi_data[2] << 16) | (info->dsi_data[3] << 8) | info->dsi_data[4];
        info->specific.aac.sample_rate = sr;
        track->codec_info.audio.sample_rate = sr;
    } else {
        info->specific.aac.sample_rate = 0;
    }

    /* 声道数 */
    if (info->specific.aac.channel_config >= 1 && info->specific.aac.channel_config <= 8) {
        info->specific.aac.channels = info->specific.aac.channel_config;
        track->codec_info.audio.channels = info->specific.aac.channel_config;
    } else {
        info->specific.aac.channels = 0;
    }

    return 0;
}

static mp4_dsi_parser_func mp4_esds_get_dsi_parser(uint8_t oti)
{
    switch (oti) {
        case 0x40:  /* AAC */
            return mp4_esds_parse_aac_dsi;
        /* 扩展:
        case 0x6B:  return mp4_esds_parse_mp3_dsi;
        case 0xA5:  return mp4_esds_parse_ac3_dsi;
        */
        default:
            return NULL;
    }
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static int mp4_detect_box(struct mp4_context *c);
static int32_t mp4_box_push(struct mp4_context *c, box_hdl hdl, uint32_t box_tag, struct mp4_box *parent);
static int32_t mp4_track_load_stco(struct mp4_context *c, struct mp4_track *track, uint32_t next_chunk_idx);
static int32_t mp4_track_load_stsz(struct mp4_context *c, struct mp4_track *track, uint32_t next_chunk_idx);
static box_hdl mp4_get_box_hdl(struct mp4_context *c, uint32_t box_tag);
static int32_t mp4_delay_seek(struct mp4_context *c, uint32_t time_ms);

// 更新box的read size 【moov解析阶段】
static void mp4_box_read_size(struct mp4_context *c, struct mp4_box *box, uint32_t size)
{
    while (box) {
        box->read_size += size;
        box = box->parent;
    }
}
//MP4 box 选择当前需要填充哪个cache【moov解析阶段】
static struct mp4_box_cache *mp4_box_select_cache(struct mp4_context *c, struct mp4_box_info *info, uint32_t entry_size)
{
    struct mp4_box_cache *cache;

    if (info->cache1.cache_count < info->cache_size &&
        info->cache1.start_index + info->cache1.cache_count <= info->total_count) {
        cache = &info->cache1;
    } else if (info->cache2.cache_count < info->cache_size &&
               info->cache2.start_index + info->cache2.cache_count <= info->total_count) {
        cache = &info->cache2;
    } else {
        return NULL;
    }
    if (!cache->entries && info->cache_size) {
        cache->entries = decoder_mem_alloc(info->cache_size * entry_size);
    }
    return cache;
}
static int32_t mp4_box_parse_done(struct mp4_track *track, struct mp4_box *box, struct mp4_box_info *info, struct mp4_box_cache *cache)
{
    uint8_t done = (box->read_size >= box->box_size) ||
                   (cache->cache_count >= info->cache_size) ||
                   (cache->start_index + cache->cache_count > info->total_count);
    if (done) {
        mp4_dbg("track %d ["MP4_TAG_FMT"] %s parse done! start:%d, count:%d\r\n",
                 track->track_idx, MP4_TAG_STR(box->box_tag),
                 cache == &info->cache1 ? "cache1" : "cache2",
                 cache->start_index, cache->cache_count);
    }
    return done;
}

static int mp4_box_skip_remain(struct mp4_context *c, struct mp4_box *box)
{
    uint32_t skip  = box->box_size - box->read_size;
    uint32_t avail = RB_COUNT(&c->io_buf);

    box->skip = 1;

    if (skip == 0) {
        return -EAGAIN;
    }

    mp4_dbg("box ["MP4_TAG_FMT"] skip %d bytes. box size: %llu, read size:%llu\r\n",
             MP4_TAG_STR(box->box_tag), skip, box->box_size, box->read_size);

    if (avail >= skip) {
        rbuffer_get(&c->io_buf, NULL, skip);
    } else {
        rbuffer_reset(&c->io_buf);
        c->ops->seek(c->file, c->offset + skip, SEEK_SET);
    }

    c->offset += skip;
    mp4_box_read_size(c, box, skip);
    return skip;
}

static int32_t mp4_box_avail(struct mp4_context *c)
{
    return RB_COUNT(&c->io_buf);
}
static int32_t mp4_box_read(struct mp4_context *c, struct mp4_box *box, uint8 *buf, uint32 len)
{
    if (mp4_box_avail(c) < len) {
        return -EAGAIN;
    }

    uint32 avail = RB_COUNT(&c->io_buf);
    uint32 count = min(len, avail);
    count = rbuffer_get(&c->io_buf, buf, count);
    c->offset += count;
    mp4_box_read_size(c, box, count);
    return count;
}
static int32_t mp4_box_fill(struct mp4_context *c, uint32_t need_size)
{
    uint32_t tot_len = 0;
    uint32_t count = RB_IDLE(&c->io_buf);

    count = min(need_size, count);
    if (count == 0) {
        return 0;
    }

    if (c->io_buf.wpos < c->io_buf.rpos) {
        tot_len = c->ops->read(c->io_buf.rbq + c->io_buf.wpos, 1, count, c->file);
    } else {
        uint32_t len1 = c->io_buf.qsize - c->io_buf.wpos;
        len1 = min(count, len1);
        uint32_t len2 = c->ops->read(c->io_buf.rbq + c->io_buf.wpos, 1, len1, c->file);
        if (len2 > 0) {
            tot_len += len2;
            if (len1 == len2 && count > len1) {
                len2 = count - len1;
                len2 = c->ops->read(c->io_buf.rbq, 1, len2, c->file);
                if (len2 > 0) {
                    tot_len += len2;
                }
            }
        }
    }

    c->io_buf.wpos += tot_len;
    if (c->io_buf.wpos >= c->io_buf.qsize) {
        c->io_buf.wpos -= c->io_buf.qsize;
    }

    return tot_len;
}

// MP4 box 跳过剩余的数据
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//MDAT数据经过cache时，以下API有效
// reload moov box信息完成后，回到mdat的读取位置【mdat数据读取阶段】
static void mp4_mdat_goback(struct mp4_context *c)
{
    if (c->offset_bak) {
        c->ops->seek(c->file, c->offset_bak, SEEK_SET);
        c->offset = c->offset_bak;
        c->offset_bak = 0;
        if(!c->iobuf_dis){
            rbuffer_reset(&c->io_buf);
        }
        mp4_warn("mp4 mdat goback! (offset: %llu)\r\n", c->offset);
    }
}

static int32_t mp4_mdat_read(struct mp4_context *c, struct mp4_track *track, uint8 *dst, uint32 len)
{
    uint32_t rlen  = 0;
    uint32_t count = RB_COUNT(&c->io_buf);
    count = min(count, len);
    if (!c->iobuf_dis && count > 0) {
        rbuffer_get(&c->io_buf, dst, count);
        dst  += count;
        len  -= count;
        rlen += count;
        c->offset += count;
    }
    if (len > 0) {
        count = c->ops->avail(c->file);
        count = min(count, len);
        count = c->ops->read(dst, 1, count, c->file);
        rlen += count;
        c->offset += count;
    }
    return rlen;
}

static int32 mp4_mdat_seek(struct mp4_context *c, uint64_t offset)
{
    uint32_t count = RB_COUNT(&c->io_buf);
    if (!c->iobuf_dis && count > 0 && offset <= c->offset + count) {
        rbuffer_get(&c->io_buf, NULL, offset - c->offset);
    } else {
        int32_t ret = c->ops->seek(c->file, offset, SEEK_SET);
        ASSERT(ret == 0);
        if(!c->iobuf_dis){
            rbuffer_reset(&c->io_buf);
        }
    }
    c->offset = offset;
    return 0;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static int mp4_track_is_active(struct mp4_context *c, uint32_t track_idx)
{
    if (track_idx == c->video_track) { return 1; }
    if (track_idx == c->audio_track) { return 1; }
    if (track_idx == c->subtitle_track) { return 1; }
    return 0;
}
static struct mp4_track *mp4_track_find(struct mp4_context *c, uint8_t track_id)
{
    int8_t i;
    for (i = 0; i < c->track_cnt; i++) {
        if (c->tracks[i] && c->tracks[i]->track_id == track_id) {
            return c->tracks[i];
        }
    }
    return NULL;
}
static inline uint32_t mp4_track_calcu_ms(struct mp4_context *c, struct mp4_track *track, uint64_t dts)
{
    return (uint32_t)(dts * 1000 / track->time_scale);
}
static uint64_t mp4_track_chunk_offset(struct mp4_context *c, struct mp4_box_cache *cache, uint32_t chunk_idx, uint8 use_co64)
{
    uint32_t cache_idx = chunk_idx - cache->start_index;
    if (chunk_idx == 0 || cache_idx >= cache->cache_count) {
        return 0;
    }
    if (use_co64) {
        struct stco64_entry *stco64 = (struct stco64_entry *)cache->entries;
        return stco64[cache_idx].chunk_offset;
    } else {
        struct stco_entry *stco = (struct stco_entry *)cache->entries;
        return stco[cache_idx].chunk_offset;
    }
}
// 二分查找：找到 ≤ sample_idx 的最大关键帧 sample 编号
static uint32_t mp4_track_get_key_sample(struct mp4_context *c, struct mp4_track *track, uint32_t sample_idx)
{
    if (track->stss.cache1.cache_count == 0) {
        return 0;   // 没有关键帧表，返回0
    }

    // stss全量加载，使用cache1
    struct stss_entry *stss = (struct stss_entry *)track->stss.cache1.entries;
    uint32_t count = track->stss.cache1.cache_count;

    // sample_idx 小于第一个关键帧？ 直接返回第一个关键帧
    if (sample_idx < stss[0].sample_index) {
        return stss[0].sample_index;
    }

    uint32_t left = 0, right = count - 1;
    uint32_t result = 0;

    while (left <= right) {
        uint32_t mid = left + (right - left) / 2;
        uint32_t s = stss[mid].sample_index;

        if (s == sample_idx) {
            mp4_dbg("track %d find key sample %d for sample %d\r\n", track->track_idx, s, sample_idx);
            return s;
        } else if (s < sample_idx) {
            result = s;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    mp4_dbg("track %d find key sample %d for sample %d\r\n", track->track_idx, result, sample_idx);
    return result;
}

//根据sample index 查找其对应的chunk index
static uint32_t mp4_track_get_sample_chunk(struct mp4_context *c, struct mp4_track *track, 
                                            uint32_t sample_idx, uint32_t seek, 
                                            uint32_t *first_sample)
{
    if (!track || sample_idx == 0) return 0;

    struct stsc_entry *stsc = (struct stsc_entry *)track->stsc.cache1.entries;
    uint32_t num_entries = track->stsc.cache_size;
    uint32_t total_chunks = track->stco.total_count;

    uint32_t entry_idx = 0;          // 当前处理的 stsc 条目索引
    uint32_t samples_before = 0;     // 当前 chunk 之前的总 sample 数
    uint32_t cur_chunk = 1;          // 当前要检查的 chunk 编号 (1-based)

    // 若不强制从头查找，且上次缓存有效，则尝试从缓存位置继续
    if (!seek && track->stsc_last_entry < num_entries && sample_idx >= track->stsc_last_samples_before + 1) {
        entry_idx = track->stsc_last_entry;
        samples_before = track->stsc_last_samples_before;
        cur_chunk = track->stsc_last_chunk_start;
    }

    // 遍历 stsc 条目
    for (; entry_idx < num_entries; entry_idx++) {
        uint32_t first_chunk = stsc[entry_idx].first_chunk;
        uint32_t spc = stsc[entry_idx].samples_per_chunk;
        uint32_t next_first_chunk = (entry_idx + 1 < num_entries) ? stsc[entry_idx+1].first_chunk : (total_chunks + 1);

        // 当前条目覆盖的 chunk 范围 [first_chunk, next_first_chunk-1]
        if (cur_chunk < first_chunk) cur_chunk = first_chunk;
        if (cur_chunk > total_chunks) break;   // 超出实际 chunk 数

        uint32_t chunk_end = next_first_chunk - 1;
        if (chunk_end > total_chunks) chunk_end = total_chunks;

        // 如果当前 chunk 已经超出本条目范围，跳到下一个条目
        if (cur_chunk > chunk_end) continue;

        // 计算本条目内从 cur_chunk 开始的 chunk 数量
        uint32_t chunks_in_entry = chunk_end - cur_chunk + 1;
        uint32_t samples_in_these_chunks = chunks_in_entry * spc;

        // 判断目标 sample 是否在本条目覆盖的范围内
        if (sample_idx <= samples_before + samples_in_these_chunks) {
            // 目标位于本条目内，计算具体 chunk
            uint32_t offset = sample_idx - samples_before;   // 相对于 cur_chunk 起始的 sample 偏移 (1-based)
            uint32_t chunk_offset = (offset + spc - 1) / spc; // 需要跳过的 chunk 个数 (1-based)
            uint32_t target_chunk = cur_chunk + chunk_offset - 1;
            uint32_t samples_before_target = samples_before + (chunk_offset - 1) * spc;

            if (first_sample) *first_sample = samples_before_target + 1;

            // 更新缓存
            track->stsc_last_entry = entry_idx;
            track->stsc_last_samples_before = samples_before_target;
            track->stsc_last_chunk_start = target_chunk;

            return target_chunk;
        }

        // 目标不在此条目内，累加样本数，移动到下一个条目起始 chunk
        samples_before += samples_in_these_chunks;
        cur_chunk = next_first_chunk;
    }

    // 未找到：sample_idx 超出总样本数
    return 0;
}
// 在stsz cache中查找sample_idx的sample size
static uint32_t mp4_track_sample_size(struct mp4_context *c, struct mp4_box_cache *cache, uint32_t sample_idx)
{
    uint32_t cache_idx = sample_idx - cache->start_index;
    if (sample_idx && cache_idx < cache->cache_count) {
        struct stsz_entry *stsz = (struct stsz_entry *)cache->entries;
        return stsz[cache_idx].sample_size;
    } else {
        return 0;
    }
}
static uint32_t mp4_track_get_sample_size(struct mp4_context *c, struct mp4_track *track, uint32_t sample_idx, uint8_t sw_cache)
{
    uint32_t sample_size;

    if (track->sample_size > 0) { //固定帧长
        return track->sample_size;
    }

    sample_size = mp4_track_sample_size(c, &track->stsz.cache1, sample_idx);
    if (sample_size > 0) {
        if (sw_cache && track->stsz.cur_cache == 1) {
            mp4_dbg("track %d stsz use cache1. (%d - %d)\r\n", track->track_idx, sample_idx, track->stsz.cache1.start_index);
            track->stsz.cur_cache = 0;
        }
        return sample_size;
    }

    sample_size = mp4_track_sample_size(c, &track->stsz.cache2, sample_idx);
    if (sample_size > 0) {
        if (sw_cache && track->stsz.cur_cache == 0) {
            mp4_dbg("track %d stsz use cache2. (%d - %d)\r\n", track->track_idx, sample_idx, track->stsz.cache2.start_index);
            track->stsz.cur_cache = 1;
        }
        return sample_size;
    }
    return 0;
}
static uint64_t mp4_track_get_chunk_offset(struct mp4_context *c, struct mp4_track *track, uint32_t chunk_idx, uint8_t sw_cache)
{
    uint64_t offset;

    offset = mp4_track_chunk_offset(c, &track->stco.cache1, chunk_idx, track->use_co64);
    if (offset > 0) {
        if (sw_cache && track->stco.cur_cache == 1) {
            mp4_dbg("track %d stco use cache1. (%d - %d)\r\n", track->track_idx, chunk_idx, track->stco.cache1.start_index);
            track->stco.cur_cache = 0;
        }
        return offset;
    }
    offset = mp4_track_chunk_offset(c, &track->stco.cache2, chunk_idx, track->use_co64);
    if (offset > 0) {
        if (sw_cache && track->stco.cur_cache == 0) {
            mp4_dbg("track %d stco use cache2. (%d - %d)\r\n", track->track_idx, chunk_idx, track->stco.cache2.start_index);
            track->stco.cur_cache = 1;
        }
        return offset;
    }
    return 0;
}
static uint64_t mp4_track_get_sample_offset(struct mp4_context *c, struct mp4_track *track, uint32_t sample_idx, uint8_t sw_cache)
{
    if (sample_idx == 0 || sample_idx > track->stsz.total_count) {
        return 0;
    }

    uint32_t first_sample = 0;
    uint32_t chunk_idx = mp4_track_get_sample_chunk(c, track, sample_idx, 0, &first_sample);
    if (chunk_idx == 0) {
        mp4_warn("can not find chunk index for sample %d !\r\n", sample_idx);
        return 0;
    }

    uint64_t offset = mp4_track_get_chunk_offset(c, track, chunk_idx, sw_cache);
    if (offset == 0) {
        mp4_dbg("can not find chunk offset for chunk %d (sample:%d)!\r\n", chunk_idx, sample_idx);
        return 0;
    }

    if(track->sample_size){ //固定sample size
        offset += (sample_idx-first_sample)*track->sample_size;
    }else{
        for(uint32_t s=first_sample; s<sample_idx; s++){
            offset += mp4_track_get_sample_size(c, track, s, 0); //reload时需要先reload stsz
        }
    }
    mp4_dbg("track %d: sample %d, first sample:%d, offset:%lld\r\n", track->track_idx, sample_idx, first_sample, offset);
    return offset;
}

static uint32_t  mp4_track_get_sample_time(struct mp4_context *c, struct mp4_track *track, uint32_t sample_idx)
{
    if (!track || track->stts.total_count == 0 || sample_idx == 0) {
        return 0;
    }

    if (sample_idx > track->stsz.total_count) {
        return 0;
    }

    uint64_t dts = 0;
    uint32_t target = sample_idx - 1;
    uint32_t current = 0;
    struct stts_entry *stts = (struct stts_entry *)track->stts.cache1.entries; //stts 目前使用全量加载

    for (uint32_t i = 0; i < track->stts.cache1.cache_count; i++) {
        uint32_t count = stts[i].sample_count;
        uint32_t delta = stts[i].sample_delta;

        if (target < current + count) {
            dts += (uint64_t)(target - current) * delta;
            return (uint32_t)(dts * 1000 / track->time_scale);
        }

        dts += (uint64_t)count * delta;
        current += count;
    }

    return (uint32_t)(dts * 1000 / track->time_scale);
}

//根据时间（毫秒）查找样本索引
static uint32_t mp4_track_seek_sample(struct mp4_context *c, struct mp4_track *track, uint32_t time_ms)
{
    if (!track || track->stts.cache1.cache_count == 0 || track->time_scale == 0) {
        return 0;
    }

    // 将毫秒时间转换为轨道的 timescale 单位
    uint64_t accum_ts   = 0;
    uint32_t sample_idx = 1;
    uint64_t target_ts  = (uint64_t)time_ms * track->time_scale / 1000;
    struct stts_entry *stts = (struct stts_entry *)track->stts.cache1.entries; //stts 目前使用全量加载

    //stts目前为全量加载
    for (uint16_t i = 0; i < track->stts.cache1.cache_count; i++) {
        uint32_t count = stts[i].sample_count;
        uint32_t delta = stts[i].sample_delta;
        uint64_t segment_duration = (uint64_t)count * delta;

        if (target_ts < accum_ts + segment_duration) {
            uint64_t offset_ts = target_ts - accum_ts;
            sample_idx += (uint32_t)(offset_ts / delta);
            break;
        }

        accum_ts += segment_duration;
        sample_idx += count;
    }

    mp4_warn("track %d find sample %d at %d ms.\r\n", track->track_idx, sample_idx, time_ms);
    return sample_idx;
}
static void mp4_track_update_next_sample(struct mp4_context *c, struct mp4_track *track, uint32_t next_index)
{
    if (!track || track->stsz.total_count == 0) {
        return;
    }

    if (next_index > 0) {
        track->next_sample_index = next_index;
    } else {
        track->next_sample_index++;
        if (track->next_sample_index > track->stsz.total_count) {
            mp4_warn("track %d EOF\r\n", track->track_idx);
            return;
        }
    }

    track->next_sample_time   = mp4_track_get_sample_time(c, track, track->next_sample_index);
    track->next_sample_offset = mp4_track_get_sample_offset(c, track, track->next_sample_index, 1);
    track->next_sample_size   = mp4_track_get_sample_size(c, track, track->next_sample_index, 1);

    mp4_dbg("track %d next sample offset:%llu, index:%d, size:%d, time:%d.\n",
            track->track_idx, track->next_sample_offset, track->next_sample_index,
            track->next_sample_size, track->next_sample_time);
}


static void mp4_track_fill_aac_ADTS(uint8_t *dsi, uint8_t *adts, uint32_t aac_data_length)
{
    uint8_t audio_object_type = (dsi[0] >> 3) & 0x1F;
    uint8_t profile = audio_object_type - 1;               // AAC LC: 2->1
    uint8_t samplerate_index = ((dsi[0] & 0x07) << 1) | (dsi[1] >> 7);
    uint8_t channels = (dsi[1] >> 3) & 0x0F;               // 1-8
    uint32_t frame_length = aac_data_length + 7;           // ADTS头7字节 + 数据

    adts[0] = 0xFF;                         // syncword high 8 bits
    adts[1] = 0xF0;                         // syncword low 4 bits (0xF)
    adts[1] |= (0x00 << 3);                 // ID: 0 = MPEG-4 (推荐)
    adts[1] |= (0x00 << 1);                 // layer: 0
    adts[1] |= 0x01;                        // protection_absent: 1 (无CRC)

    adts[2] = (profile << 6);               // profile 2 bits
    adts[2] |= (samplerate_index << 2);     // sampling_frequency_index 4 bits
    adts[2] |= (0x00 << 1);                 // private_bit: 0
    adts[2] |= (channels >> 2) & 0x01;      // channel_configuration 高位 (1 bit)

    adts[3] = (channels & 0x03) << 6;       // channel_configuration 低位 (2 bits)
    adts[3] |= (frame_length >> 11) & 0x03; // frame_length bits 11-12

    adts[4] = (frame_length >> 3) & 0xFF;   // frame_length bits 3-10
    adts[5] = (frame_length & 0x07) << 5;   // frame_length bits 0-2 (高5位)

    // 设置 buffer_fullness (11 bits)，常用 0x7FF 表示 VBR
    uint16_t buffer_fullness = 0x7FF;
    adts[5] |= (buffer_fullness >> 6) & 0x07;   // buffer_fullness 高3位 (放入adts[5]低3位)
    adts[6] = (buffer_fullness & 0x3F) << 2;    // buffer_fullness 低6位 (放入adts[6]高6位)
    adts[6] |= 0x00;                            // number_of_raw_data_blocks_in_frame = 0
}
static struct framebuff *mp4_track_alloc_fb(struct mp4_context *c, struct mp4_track *track)
{
    struct framebuff *fb = NULL;
    if (track->mtype == MEDIA_DATA_AUDIO && track->stype == AUDIO_CODEC_AAC) {
        fb = msi_alloc_fb(c->owner, NULL, NULL, track->next_sample_size + 7, 0, 0);
        if (fb && track->codec_data) {
            esds_info_t *info = (esds_info_t *)track->codec_data;
            mp4_track_fill_aac_ADTS(info->dsi_data, fb->data, track->next_sample_size);
            c->cur_frame_len = 7; //填充 7 byte
        }
    } else {
        fb = msi_alloc_fb(c->owner, NULL, NULL, track->next_sample_size, 0, 0);
    }
    return fb;
}
static int32 mp4_track_is_keyfrm(struct mp4_context *c, struct mp4_track *track)
{
    uint32 keyfrm_idx = mp4_track_get_key_sample(c, track, track->next_sample_index);
    return keyfrm_idx == track->next_sample_index;
}
static void mp4_track_find_video_NALU(struct framebuff *fb)
{
    uint8_t *data   = fb->data;
    uint32_t offset = 0;

    while (offset + 4 <= fb->len) {
        // NAL单元长度-大端
        uint32_t nalu_len = get_unaligned_be32(data + offset);
        if (nalu_len == 0 || offset + 4 + nalu_len > fb->len) {
            break;  // 数据损坏
        }

        // 获取NAL类型
        uint8_t *nalu = data + offset + 4;
        if (fb->stype == VIDEO_CODEC_H265) {  // H.265
            uint8_t nal_type = (nalu[0] >> 1) & 0x3F;
            if (nal_type <= 31) {
                fb->data = nalu;
                fb->len  = nalu_len;
                break;
            }
        } else {  // H.264
            uint8_t nal_type = nalu[0] & 0x1F;
            if (nal_type == 1 || nal_type == 5) { //I/P/B
                fb->data = nalu;
                fb->len  = nalu_len;
                break;
            }
        }
        // 跳过当前NAL单元
        offset += (4 + nalu_len);
    }
}
static int mp4_track_output_fb(struct mp4_context *c, struct mp4_track *track)
{
    struct framebuff *fb = c->cur_frame;

    // track未被选中，丢弃数据
    if (!mp4_track_is_active(c, track->track_idx)) {
        mp4_save_track_to_file(c, track, fb->data, fb->len);
        fb_put(fb);
        c->cur_frame = NULL;
        c->cur_frame_len = 0;
        return 0;
    }

    fb->time  = track->next_sample_time;
    fb->mtype = track->mtype;
    fb->stype = track->stype;

    mp4_dbg("track %d output sample %d, size:%d(%d), time:%d. cur offset:%llu\r\n",
            track->track_idx, track->next_sample_index, track->next_sample_size, fb->len, fb->time, c->offset);

    if (track->mtype == MEDIA_DATA_VIDEO) {
        fb->codec_info   = &track->codec_info.video;
        fb->keyfrm = mp4_track_is_keyfrm(c, track);
        mp4_track_find_video_NALU(fb);
    } else if (track->mtype == MEDIA_DATA_AUDIO) {
        fb->codec_info = &track->codec_info.audio;
    } else if (track->mtype == MEDIA_DATA_SUBTITLE) {
        fb->codec_info = &track->codec_info.subtitle;
    } else {
        fb->codec_info = NULL;
    }

    mp4_save_track_to_file(c, track, fb->data, fb->len);
    c->ops->outFB(c->owner, fb);
    c->cur_frame = NULL;
    c->cur_frame_len = 0;
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

static struct mp4_track *mp4_select_next_track(struct mp4_context *c)
{
    struct mp4_track *selected = NULL;
    uint64_t min_offset = UINT64_MAX;

    for (int i = 0; i < c->track_cnt; i++) {
        struct mp4_track *t = c->tracks[i];
        if (t && t->next_sample_offset < min_offset && t->next_sample_index <= t->stsz.total_count) {
            min_offset = t->next_sample_offset;
            selected   = t;
        }
    }
    return selected;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// mvhd box：解析总时长
//【不能多次进入，需要等待足够的数据才能开始解析】
static int32_t mp4_box_mvhd_hdl(struct mp4_context *c)
{
    struct mp4_box *box = &c->boxs[c->cur_box];
    uint8_t buf[32];
    uint8_t version;
    uint64_t duration = 0;
    uint32_t time_scale = 0;

    //最小数据： 36 byte
    if (mp4_box_avail(c) < 36) {
        return 0; //数据不足，需要网络缓冲
    }

    mp4_box_read(c, box, buf, 4);
    version = buf[0];

    if (version == 0) {
        mp4_box_read(c, box, buf, 20);
        time_scale = get_unaligned_be32(buf + 8);
        duration = get_unaligned_be32(buf + 12);
    } else if (version == 1) {
        mp4_box_read(c, box, buf, 32);
        time_scale = get_unaligned_be32(buf + 8);
        duration = get_unaligned_be64(buf + 12);
    }

    if (time_scale > 0) {
        c->total_duration_ms = duration * 1000 / time_scale;
    }

    return mp4_box_skip_remain(c, box);
}

//【不能多次进入，需要等待足够的数据才能开始解析】
static int32_t mp4_box_tkhd_hdl(struct mp4_context *c)
{
    struct mp4_box *box = &c->boxs[c->cur_box];
    uint8_t  buf[48];
    uint8_t  version;
    uint32_t track_id;

    //最小数据： 48 byte
    if (mp4_box_avail(c) < 48) {
        return 0; //数据不足，需要网络缓冲
    }

    mp4_box_read(c, box, buf, 4);
    version = buf[0];

    if (version == 0) {
        mp4_box_read(c, box, buf, 28);
    } else {
        mp4_box_read(c, box, buf, 44);
    }

    track_id = get_unaligned_be32(buf + 8);
    struct mp4_track *track = mp4_track_find(c, track_id);
    if (track) {
        c->cur_track = track->track_idx;
        return mp4_box_skip_remain(c, box);
    }

    if (c->track_cnt >= MP4_MAX_TRACKS) {
        mp4_err("Only support %d tracks!\r\n", MP4_MAX_TRACKS);
        return mp4_box_skip_remain(c, box);
    }

    track = decoder_mem_calloc(1, sizeof(struct mp4_track));
    if (!track) {
        return mp4_box_skip_remain(c, box);
    }

    track->mtype = MEDIA_DATA_UNKNOWN;
    track->stype = AUDIO_CODEC_INVALID;
    track->track_id  = track_id;
    track->track_idx = c->track_cnt;
    c->tracks[c->track_cnt] = track;
    c->cur_track = c->track_cnt;
    c->track_cnt++;
    return mp4_box_skip_remain(c, box);
}

// hdlr box：解析handler_type，设置轨道的媒体类型
//【不能多次进入，需要等待足够的数据才能开始解析】
static int32_t mp4_box_hdlr_hdl(struct mp4_context *c)
{
    struct mp4_box *box = &c->boxs[c->cur_box];
    struct mp4_track *track = c->tracks[c->cur_track];
    uint8_t buf[24];

    if (!track) {
        return mp4_box_skip_remain(c, box);
    }

    //最小数据： 24 byte
    if (mp4_box_read(c, box, buf, 24) < 0) {
        return 0; //数据不足，需要网络缓冲
    }

    uint32_t handler_type = get_unaligned_be32(buf + 8);
    switch (handler_type) {
        case MP4_TAG('v', 'i', 'd', 'e'):
            track->mtype = MEDIA_DATA_VIDEO;
            if (c->video_track == -1) { c->video_track = track->track_idx; }
            mp4_warn("Track %d is Video\r\n", track->track_idx);
            break;
        case MP4_TAG('s', 'o', 'u', 'n'):
            track->mtype = MEDIA_DATA_AUDIO;
            if (c->audio_track == -1) { c->audio_track = track->track_idx; }
            mp4_warn("Track %d is Audio\r\n", track->track_idx);
            break;
        case MP4_TAG('s', 'b', 'u', 't'):
            track->mtype = MEDIA_DATA_SUBTITLE;
            if (c->subtitle_track == -1) { c->subtitle_track = track->track_idx; }
            mp4_warn("Track %d is Subtitle\r\n", track->track_idx);
            break;
        default:
            track->mtype = MEDIA_DATA_UNKNOWN;
    }

    return mp4_box_skip_remain(c, box);
}

// mdhd box：解析time_scale和duration
//【不能多次进入，需要等待足够的数据才能开始解析】
static int32_t mp4_box_mdhd_hdl(struct mp4_context *c)
{
    struct mp4_box *box = &c->boxs[c->cur_box];
    struct mp4_track *track = c->tracks[c->cur_track];
    uint8_t version;
    uint8_t buf[36] = {0};  // 最大支持 version1(36字节)

    if (!track) {
        return mp4_box_skip_remain(c, box);
    }

    //最小数据： 36 byte
    if (mp4_box_avail(c) < 36) {
        return 0; //数据不足，需要网络缓冲
    }

    // 先读 version + flags（固定 4 字节）
    mp4_box_read(c, box, buf, 4);
    version = buf[0];
    // 根据 version 读取剩余数据
    if (version == 0) {
        // version0: 剩余 20 字节，总长 24
        mp4_box_read(c, box, buf + 4, 20);
        track->time_scale = get_unaligned_be32(buf + 12);
        track->duration   = get_unaligned_be32(buf + 16);
    } else if (version == 1) {
        // version1: 剩余 32 字节，总长 36
        mp4_box_read(c, box, buf + 4, 32);
        track->time_scale = get_unaligned_be32(buf + 12);
        track->duration   = get_unaligned_be64(buf + 16);
    } else {
        // 不支持的 version，跳过整个 box
        return mp4_box_skip_remain(c, box);
    }

    // 跳过剩余数据
    return mp4_box_skip_remain(c, box);
}

// avc1 是容器box，需要解析子box
//【不能多次进入，需要等待足够的数据才能开始解析】
static int32_t mp4_box_avc1_hdl(struct mp4_context *c)
{
    struct mp4_box *box = &c->boxs[c->cur_box];
    struct mp4_track *track = c->tracks[c->cur_track];
    uint8_t buf[78]; // VisualSampleEntry 固定部分

    if (mp4_box_read(c, box, buf, sizeof(buf)) < 0) {
        return 0; //数据不足，需要网络缓冲
    }

    if (track) {
        // 标准偏移：width 在 24，height 在 26
        uint16_t width  = get_unaligned_be16(buf + 24);;
        uint16_t height = get_unaligned_be16(buf + 26);
        track->stype = VIDEO_CODEC_H264;
        track->codec_info.video.codec_id = VIDEO_CODEC_H264;
        track->codec_info.video.width  = width;
        track->codec_info.video.height = height;
        mp4_warn("track %d H264 Video: %dx%d\n", track->track_idx, width, height);
    }

    box->child_box = 1;
    return mp4_detect_box(c);
}

static int32_t mp4_box_hev1_hdl(struct mp4_context *c)
{
    struct mp4_box *box = &c->boxs[c->cur_box];
    struct mp4_track *track = c->tracks[c->cur_track];
    uint8_t buf[78]; // VisualSampleEntry 固定部分

    if (box->box_size < sizeof(buf)) {
        mp4_err("hev1 box too small: size=%u < %zu\n", box->box_size, sizeof(buf));
        return mp4_box_skip_remain(c, box);
    }

    if (mp4_box_read(c, box, buf, sizeof(buf)) < 0) {
        return 0; //数据不足，需要网络缓冲
    }

    if (track) {
        // 标准偏移：width 在 24，height 在 26
        uint16_t width  = (buf[24] << 8) | buf[25];
        uint16_t height = (buf[26] << 8) | buf[27];
        track->stype = VIDEO_CODEC_H265;
        track->codec_info.video.codec_id = VIDEO_CODEC_H265;
        track->codec_info.video.width  = width;
        track->codec_info.video.height = height;
        mp4_warn("track %d HEVC Video: %dx%d\n", track->track_idx, width, height);
    }

    box->child_box = 1;
    return mp4_detect_box(c);
}

// hvc1 和 hev1 完全一样，只是四字符码不同
static int32_t mp4_box_hvc1_hdl(struct mp4_context *c)
{
    return mp4_box_hev1_hdl(c);
}

static int32_t mp4_box_mp4a_hdl(struct mp4_context *c)
{
    struct mp4_box *box = &c->boxs[c->cur_box];
    struct mp4_track *track = c->tracks[c->cur_track];
    uint8_t buf[28];

    if (box->box_size < sizeof(buf)) {
        mp4_err("mp4a box too small: size=%u < %zu\n", box->box_size, sizeof(buf));
        return mp4_box_skip_remain(c, box);
    }

    if (mp4_box_read(c, box, buf, sizeof(buf)) < 0) {
        return 0; //数据不足，需要网络缓冲
    }

    if (track) {
        uint16_t channel_count = get_unaligned_be16(buf + 16);
        uint16_t sample_size   = get_unaligned_be16(buf + 18);
        uint32_t sample_rate_raw = get_unaligned_be32(buf + 24);
        uint32_t sample_rate = sample_rate_raw >> 16;
        track->codec_info.audio.codec_id = AUDIO_CODEC_INVALID;
        track->codec_info.audio.channels = channel_count;
        track->codec_info.audio.sample_rate = sample_rate;
        track->codec_info.audio.bit_rate = 0;
        track->codec_info.audio.frame_size = 0;
        mp4_warn("track %d audio : channels=%u, sample_size=%u, sample_rate=%u\n",
                 track->track_idx, channel_count, sample_size, sample_rate);
    }

    box->child_box = 1;
    return mp4_detect_box(c);
}

//【不能多次进入，需要等待足够的数据才能开始解析】
static int32_t mp4_box_esds_hdl(struct mp4_context *c)
{
    struct mp4_box *box = &c->boxs[c->cur_box];
    struct mp4_track *track = c->tracks[c->cur_track];
    esds_info_t *info = NULL;
    uint8_t *esds_data = NULL;

    if (!track) {
        return mp4_box_skip_remain(c, box);
    }

    // 计算 esds 数据长度
    uint32_t esds_len = box->box_size - box->read_size;
    if (esds_len < 4) {
        mp4_warn("esds box has no data\n");
        return mp4_box_skip_remain(c, box);
    }

    if (mp4_box_avail(c) < esds_len) {
        return 0; //数据不足，需要网络缓冲
    }

    esds_data = decoder_mem_alloc(esds_len);
    if (!esds_data) {
        mp4_err("Failed to allocate esds data\n");
        return mp4_box_skip_remain(c, box);
    }
    mp4_box_read(c, box, esds_data, esds_len);

    info = decoder_mem_alloc(sizeof(esds_info_t));
    if (!info) {
        decoder_mem_free(esds_data);
        mp4_err("Failed to allocate esds_info_t\n");
        return mp4_box_skip_remain(c, box);
    }
    memset(info, 0, sizeof(esds_info_t));

    const uint8_t *p   = esds_data + 4; // 跳过 version(1) 和 flags(3)
    const uint8_t *end = esds_data + esds_len;

    /* 查找 ES_Descriptor (tag = 0x03) */
    while (p < end && *p != 0x03) { p++; }
    if (p >= end) {
        mp4_err("ES_Descriptor (0x03) not found\n");
        goto __error;
    }

    p++; /* 跳过 tag 0x03 */
    uint32_t es_len = mp4_esds_read_ber_length(&p, end);
    if (p + es_len > end) {
        mp4_err("ES_Descriptor length exceeds box boundary\n");
        goto __error;
    }

    /* ES_Descriptor 内部: ES_ID (2字节) + flags (1字节) */
    if (p + 3 > end) {
        mp4_err("ES_Descriptor too short\n");
        goto __error;
    }
    p += 3;  /* 跳过 ES_ID 和 flags */

    /* 查找 DecoderConfigDescriptor (tag = 0x04) */
    while (p < end && *p != 0x04) { p++; }
    if (p >= end) {
        mp4_err("DecoderConfigDescriptor (0x04) not found\n");
        goto __error;
    }

    p++; /* 跳过 tag 0x04 */
    uint32_t dcd_len = mp4_esds_read_ber_length(&p, end);
    if (p + dcd_len > end) {
        mp4_err("DecoderConfigDescriptor length exceeds box boundary\n");
        goto __error;
    }

    /* 解析 DecoderConfigDescriptor 固定字段 (13字节) */
    if (p + 13 > end) {
        mp4_err("DecoderConfigDescriptor too short for fixed fields\n");
        goto __error;
    }

    info->object_type_indication = *p++;
    uint8_t stream_byte = *p++;
    info->stream_type = (stream_byte >> 2) & 0x3F;
    info->up_stream = stream_byte & 0x03;
    info->buffer_size_db = (p[0] << 16) | (p[1] << 8) | p[2];
    p += 3;
    info->max_bitrate = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
    p += 4;
    info->avg_bitrate = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
    p += 4;

    // 设置比特率（优先使用平均比特率）
    track->codec_info.audio.bit_rate = info->avg_bitrate ? info->avg_bitrate : info->max_bitrate;

    /* 根据 object_type_indication 设置编码类型 */
    switch (info->object_type_indication) {
        case 0x40:
        case 0x66:
        case 0x67:
        case 0x68:
            track->stype = AUDIO_CODEC_AAC;
            track->codec_info.audio.codec_id = AUDIO_CODEC_AAC;
            mp4_warn("track %d audio codec: AAC\r\n", track->track_idx);
            break;
        case 0x6B:
            track->stype = AUDIO_CODEC_MP3;
            track->codec_info.audio.codec_id = AUDIO_CODEC_MP3;
            mp4_warn("track %d audio codec: MP3\r\n", track->track_idx);
            break;
        case 0xA5:
            track->codec_info.audio.codec_id = AUDIO_CODEC_AC3;
            mp4_warn("track %d audio codec: AC-3\r\n", track->track_idx);
            break;
        default:
            mp4_warn("track %d Unknown audio codec: 0x%02X\n", track->track_idx, info->object_type_indication);
            track->codec_info.audio.codec_id = AUDIO_CODEC_INVALID;
    }

    /* 在 DecoderConfigDescriptor 剩余数据中查找 DecoderSpecificInfo (tag 0x05) */
    uint32_t remaining = dcd_len - 13;
    const uint8_t *sub_start = p;
    const uint8_t *sub_end = p + remaining;
    while (sub_start < sub_end) {
        uint8_t sub_tag = *sub_start++;
        uint32_t sub_len = mp4_esds_read_ber_length(&sub_start, sub_end);
        if (sub_start + sub_len > sub_end) {
            mp4_err("DecoderSpecificInfo length exceeds descriptor boundary\n");
            break;
        }

        if (sub_tag == 0x05) {
            if (sub_len > 7) {
                mp4_warn("DSI too large (%u bytes), truncating to 7\n", sub_len);
            }
            info->dsi_len = (sub_len > 7) ? 7 : (uint8_t)sub_len;;
            os_memcpy(info->dsi_data, sub_start, info->dsi_len);
            mp4_dsi_parser_func parser = mp4_esds_get_dsi_parser(info->object_type_indication);
            if (parser) { parser(c, track, info); }
            break;
        }
        sub_start += sub_len;
    }

    decoder_mem_free(track->codec_data);
    track->codec_data = info;
    track->codec_len  = sizeof(esds_info_t);
    decoder_mem_free(esds_data);
    return mp4_box_skip_remain(c, box);

__error:
    if (info) {
        decoder_mem_free(info);
    }
    if (esds_data) {
        decoder_mem_free(esds_data);
    }
    return mp4_box_skip_remain(c, box);
}

//【不能多次进入，需要等待足够的数据才能开始解析】
static int32_t mp4_box_avcc_hdl(struct mp4_context *c)
{
    struct mp4_box *box = &c->boxs[c->cur_box];
    struct mp4_track *track = c->tracks[c->cur_track];
    uint8_t *codec_data = NULL;
    h264_avcc_info_t *info = NULL;
    const uint8_t *p;
    uint32_t avcc_len;

    if (!track) {
        return mp4_box_skip_remain(c, box);
    }

    avcc_len = box->box_size - box->read_size;
    if (avcc_len == 0) {
        mp4_warn("avcC box has no data\n");
        return mp4_box_skip_remain(c, box);
    }

    if (mp4_box_avail(c) < avcc_len) {
        return 0; //数据不足，需要网络缓冲
    }

    codec_data = decoder_mem_calloc(1, avcc_len);
    if (!codec_data) {
        mp4_err("Failed to allocate avcC data\n");
        return mp4_box_skip_remain(c, box);
    }
    mp4_box_read(c, box, codec_data, avcc_len);

    /* 1. 验证头部 */
    if (avcc_len < 6) {
        mp4_err("avcC too short: %u bytes (min 6)\n", avcc_len);
        goto error;
    }
    if (codec_data[0] != 1) {
        mp4_err("avcC configurationVersion != 1\n");
        goto error;
    }

    /* 2. 分配并初始化 info 结构 */
    info = decoder_mem_alloc(sizeof(h264_avcc_info_t));
    if (!info) {
        mp4_err("Failed to allocate h264_codec_info_t\n");
        goto error;
    }
    memset(info, 0, sizeof(h264_avcc_info_t));

    info->profile = codec_data[1];
    //info->compat  = codec_data[2];
    info->level   = codec_data[3];
    //info->nal_len_bytes = (codec_data[4] & 0x03) + 1;

    /* 3. 解析 SPS */
    p = codec_data + 5;
    uint8_t sps_count = *p++ & 0x1F;
    if (sps_count == 0) {
        mp4_err("No SPS in avcC\n");
        goto error;
    }
    if (p + 2 > codec_data + avcc_len) { goto size_error; }
    uint16_t sps_len = (p[0] << 8) | p[1];
    p += 2;
    if (p + sps_len > codec_data + avcc_len) { goto size_error; }

    info->sps_size = sps_len;
    info->sps_data = decoder_mem_alloc(sps_len);
    if (!info->sps_data) {
        mp4_err("Failed to allocate SPS\n");
        goto error;
    }
    os_memcpy(info->sps_data, p, sps_len);
    p += sps_len;

    /* 4. 解析 PPS */
    if (p + 1 > codec_data + avcc_len) { goto size_error; }
    uint8_t pps_count = *p++ & 0x1F;
    if (pps_count == 0) {
        mp4_err("No PPS in avcC\n");
        goto error;
    }
    if (p + 2 > codec_data + avcc_len) { goto size_error; }
    uint16_t pps_len = (p[0] << 8) | p[1];
    p += 2;
    if (p + pps_len > codec_data + avcc_len) { goto size_error; }

    info->pps_size = pps_len;
    info->pps_data = decoder_mem_alloc(pps_len);
    if (!info->pps_data) {
        mp4_err("Failed to allocate PPS\n");
        goto error;
    }
    os_memcpy(info->pps_data, p, pps_len);

    /* 5. 替换 track 中的 codec_data */
    if (track->codec_data) {
        h264_avcc_info_t *old = (h264_avcc_info_t *)track->codec_data;
        if (old->sps_data) { decoder_mem_free((void *)old->sps_data); }
        if (old->pps_data) { decoder_mem_free((void *)old->pps_data); }
        decoder_mem_free(old);
    }

    track->codec_data = info;
    track->codec_len = sizeof(h264_avcc_info_t);

    track->codec_info.video.extradata = (uint8_t *)info;
    track->codec_info.video.extradata_size = sizeof(h264_avcc_info_t);
    decoder_mem_free(codec_data);
    return mp4_box_skip_remain(c, box);

size_error:
    mp4_err("avcC size error while parsing\n");
error:
    if (info) {
        if (info->sps_data) { decoder_mem_free((void *)info->sps_data); }
        if (info->pps_data) { decoder_mem_free((void *)info->pps_data); }
        decoder_mem_free(info);
    }
    decoder_mem_free(codec_data);
    return mp4_box_skip_remain(c, box);
}

// HVCC box：H.265 解码器配置 (VPS/SPS/PPS)
//【不能多次进入，需要等待足够的数据才能开始解析】
static int32_t mp4_box_hvcc_hdl(struct mp4_context *c)
{
    struct mp4_box *box = &c->boxs[c->cur_box];
    struct mp4_track *track = c->tracks[c->cur_track];
    uint8_t *codec_data = NULL;
    hevc_codec_info_t *info = NULL;
    const uint8_t *p;
    uint32_t hvcc_len;

    if (!track) {
        return mp4_box_skip_remain(c, box);
    }

    hvcc_len = box->box_size - box->read_size;
    if (hvcc_len == 0) {
        mp4_warn("hvcC box has no data\n");
        return mp4_box_skip_remain(c, box);
    }

    if (mp4_box_avail(c) < hvcc_len) {
        return 0; //数据不足，需要网络缓冲
    }

    codec_data = decoder_mem_calloc(1, hvcc_len);
    if (!codec_data) {
        mp4_err("Failed to allocate hvcC data\n");
        return mp4_box_skip_remain(c, box);
    }
    mp4_box_read(c, box, codec_data, hvcc_len);

    /* 验证头部 */
    if (hvcc_len < 23) {
        mp4_err("hvcC too short: %u bytes (min 23)\n", hvcc_len);
        goto error;
    }
    if (codec_data[0] != 1) {
        mp4_err("hvcC configurationVersion != 1\n");
        goto error;
    }

    /* 分配并初始化 info 结构 */
    info = decoder_mem_alloc(sizeof(hevc_codec_info_t));
    if (!info) {
        mp4_err("Failed to allocate hevc_codec_info_t\n");
        goto error;
    }
    memset(info, 0, sizeof(hevc_codec_info_t));

    /* 解析 profile 相关字段 */
    uint8_t profile_byte = codec_data[1];
    info->profile_space  = (profile_byte >> 6) & 0x03;
    info->tier_flag      = (profile_byte >> 5) & 0x01;
    info->profile_idc    = profile_byte & 0x1F;

    info->profile_compatibility = (codec_data[2] << 24) | (codec_data[3] << 16) |
                                  (codec_data[4] << 8)  | codec_data[5];

    info->level_idc = codec_data[12];

    /* 解析 NAL 长度字段 */
    if (hvcc_len < 22) { goto size_error; }
    info->nal_len_bytes = (codec_data[21] & 0x03) + 1;

    /* 解析 NAL 单元数组 */
    p = codec_data + 22;
    uint8_t num_arrays = *p++;
    for (int i = 0; i < num_arrays && p < codec_data + hvcc_len; i++) {
        if (p + 1 > codec_data + hvcc_len) { goto size_error; }
        uint8_t array_type = *p++ & 0x3F;
        if (p + 2 > codec_data + hvcc_len) { goto size_error; }
        uint16_t num_nalus = (p[0] << 8) | p[1];
        p += 2;
        for (int j = 0; j < num_nalus; j++) {
            if (p + 2 > codec_data + hvcc_len) { goto size_error; }
            uint16_t nal_len = (p[0] << 8) | p[1];
            p += 2;
            if (p + nal_len > codec_data + hvcc_len) { goto size_error; }

            // 根据 array_type 存储对应的 NAL 单元
            if (array_type == 32) { // VPS
                info->vps_data = decoder_mem_alloc(nal_len);
                if (info->vps_data) {
                    os_memcpy(info->vps_data, p, nal_len);
                    info->vps_size = nal_len;
                }
            } else if (array_type == 33) { // SPS
                info->sps_data = decoder_mem_alloc(nal_len);
                if (info->sps_data) {
                    os_memcpy(info->sps_data, p, nal_len);
                    info->sps_size = nal_len;
                }
            } else if (array_type == 34) { // PPS
                info->pps_data = decoder_mem_alloc(nal_len);
                if (info->pps_data) {
                    os_memcpy(info->pps_data, p, nal_len);
                    info->pps_size = nal_len;
                }
            }
            p += nal_len;
        }
    }

    /* 替换 track 中的 codec_data */
    if (track->codec_data) {
        hevc_codec_info_t *old = (hevc_codec_info_t *)track->codec_data;
        if (old->vps_data) { decoder_mem_free((void *)old->vps_data); }
        if (old->sps_data) { decoder_mem_free((void *)old->sps_data); }
        if (old->pps_data) { decoder_mem_free((void *)old->pps_data); }
        decoder_mem_free(old);
    }

    track->codec_data = info;
    track->codec_len = sizeof(hevc_codec_info_t);
    decoder_mem_free(codec_data);
    return mp4_box_skip_remain(c, box);

size_error:
    mp4_err("hvcC size error while parsing\n");
error:
    if (info) {
        if (info->vps_data) { decoder_mem_free((void *)info->vps_data); }
        if (info->sps_data) { decoder_mem_free((void *)info->sps_data); }
        if (info->pps_data) { decoder_mem_free((void *)info->pps_data); }
        decoder_mem_free(info);
    }
    decoder_mem_free(codec_data);
    return mp4_box_skip_remain(c, box);
}

// stsd box：解析编码器配置
//【不能多次进入，需要等待足够的数据才能开始解析】
static int32_t mp4_box_stsd_hdl(struct mp4_context *c)
{
    struct mp4_box *box = &c->boxs[c->cur_box];
    uint8_t hdr[8];

    // 最小数据：8 byte
    if (mp4_box_read(c, box, hdr, 8) < 0) {
        return 0; //数据不足，需要网络缓冲
    }

    // 如果没有条目，跳过
    uint32_t entry_count = get_unaligned_be32(hdr + 4);
    if (entry_count == 0) {
        return mp4_box_skip_remain(c, box);
    }

    /* 解析子box*/
    box->child_box = 1;
    return mp4_detect_box(c);
}

// stts box：解码时间戳表, 支持惰性加载
//【box数据很大，需要多次进入才能完成解析】
static int32_t mp4_box_stts_hdl(struct mp4_context *c)
{
    struct mp4_box *box = &c->boxs[c->cur_box];
    struct mp4_track *track = c->tracks[c->cur_track];
    struct mp4_box_cache *cache = NULL;

    mp4_dbg("track %d parse stts box, remamin %d\r\n", track->track_idx, box->box_size - box->read_size);
    //解析header
    if (track->stts.total_count == 0) {
        uint8_t buf[8];
        if (mp4_box_read(c, box, buf, 8) < 0) {
            return 0; //数据不足，需要网络缓冲
        }
        track->stts.cur_cache = 0;
        track->stts.offset = c->offset;
        track->stts.total_count = get_unaligned_be32(buf + 4);
        track->stts.cache_size  = track->stts.total_count;
#if 0 // 目前使用全量加载
        if (c->box_cache_size && track->stts.cache_size > c->box_cache_size) {
            track->stts.cache_size = c->box_cache_size;
        }
#endif
        track->stts.cache1.start_index = 1;
        track->stts.cache2.start_index = 1 + track->stts.cache_size;
        mp4_warn("track %d [stts] cache size:%d (need memory %d bytes)\r\n",
                 track->track_idx, track->stts.cache_size,
                 track->stts.cache_size * sizeof(struct stts_entry));
    }

    //选择cache
    cache = mp4_box_select_cache(c, &track->stts, sizeof(struct stts_entry));
    if (cache == NULL) {
        return mp4_box_skip_remain(c, box);
    }
    if (!cache->entries) {
        mp4_err("alloc stts cache fail! (size:%d)\r\n", track->stts.cache_size);
        return -ENOMEM;
    }

    //填充cache
    struct stts_entry *stts = (struct stts_entry *)cache->entries;
    uint8_t skip = mp4_box_parse_done(track, box, &track->stts, cache);
    while (!skip) {
        uint8_t buf[8];
        if (mp4_box_read(c, box, buf, 8) < 0) {
            return 0; //数据不足，需要网络缓冲
        }

        stts[cache->cache_count].sample_count = get_unaligned_be32(buf);
        stts[cache->cache_count].sample_delta = get_unaligned_be32(buf + 4);
        mp4_dbg("track %d [stts] [%d: sample_count %d, sample_delta %d]\r\n",
                track->track_idx, cache->cache_count,
                stts[cache->cache_count].sample_count,
                stts[cache->cache_count].sample_delta);

        cache->cache_count++;
        skip = mp4_box_parse_done(track, box, &track->stts, cache);
    }

    //解析到末尾
    if ((cache->start_index + cache->cache_count > track->stts.total_count)) {
        mp4_box_skip_remain(c, box);
    }

    return -EAGAIN; //解析成功，继续加载
}

// stsc box：样本到块映射表, 支持惰性加载
//【box数据很大，需要多次进入才能完成解析】
static int32_t mp4_box_stsc_hdl(struct mp4_context *c)
{
    struct mp4_box *box = &c->boxs[c->cur_box];
    struct mp4_track *track = c->tracks[c->cur_track];
    struct mp4_box_cache *cache = NULL;

    mp4_dbg("track %d parse stsc box, remamin %d\r\n", track->track_idx, box->box_size - box->read_size);
    //解析header
    if (track->stsc.total_count == 0) {
        uint8_t buf[8];
        if (mp4_box_read(c, box, buf, 8) < 0) {
            return 0; //数据不足，需要网络缓冲
        }
        track->stsc.cur_cache = 0;
        track->stsc.offset = c->offset;
        track->stsc.total_count = get_unaligned_be32(buf + 4);
        track->stsc.cache_size  = track->stsc.total_count;
#if 0 //全量加载
        if (c->box_cache_size && track->stsc.cache_size > c->box_cache_size) {
            track->stsc.cache_size = c->box_cache_size;
        }
#endif
        track->stsc.cache1.start_index = 1;
        track->stsc.cache2.start_index = 1 + track->stsc.cache_size;
        mp4_warn("track %d [stsc] cache size:%d (need memory %d bytes)\r\n",
                 track->track_idx, track->stsc.cache_size,
                 track->stsc.cache_size * sizeof(struct stsc_entry));
    }

    //选择cache
    cache = mp4_box_select_cache(c, &track->stsc, sizeof(struct stsc_entry));
    if (cache == NULL) {
        return mp4_box_skip_remain(c, box);
    }
    if (!cache->entries) {
        mp4_err("alloc stsc fail, cache count:%d\r\n", track->stsc.cache_size);
        return -ENOMEM;
    }

    //填充cache
    struct stsc_entry *stsc = (struct stsc_entry *)cache->entries;
    uint8_t skip = mp4_box_parse_done(track, box, &track->stsc, cache);
    while (!skip) {
        uint8_t buf[12];
        if (mp4_box_read(c, box, buf, 12) < 0) {
            return 0; //数据不足，需要网络缓冲
        }

        stsc[cache->cache_count].first_chunk = get_unaligned_be32(buf);
        stsc[cache->cache_count].samples_per_chunk = get_unaligned_be32(buf + 4);
        mp4_dbg("track %d [stsc] [%d: first_chunk %d, samples_per_chunk %d]\r\n",
                track->track_idx, cache->cache_count,
                stsc[cache->cache_count].first_chunk,
                stsc[cache->cache_count].samples_per_chunk);

        cache->cache_count++;
        skip = mp4_box_parse_done(track, box, &track->stsc, cache);
    }

    //解析到末尾
    if ((cache->start_index + cache->cache_count > track->stsc.total_count)) {
        mp4_box_skip_remain(c, box);
    }

    return -EAGAIN; //解析成功，继续加载
}

// stco box：块偏移量表
//【box数据很大，需要多次进入才能完成解析】
static int32_t mp4_box_stco_hdl(struct mp4_context *c)
{
    struct mp4_box *box = &c->boxs[c->cur_box];
    struct mp4_track *track = c->tracks[c->cur_track];
    struct mp4_box_cache *cache = NULL;

    mp4_dbg("track %d parse stco box, remamin %d\r\n", track->track_idx, box->box_size - box->read_size);
    //解析header
    if (track->stco.total_count == 0) {
        uint8_t buf[8];
        if (mp4_box_read(c, box, buf, 8) < 0) {
            return 0; //数据不足，需要网络缓冲
        }
        track->use_co64 = 0;
        track->stco.cur_cache = 0;
        track->stco.offset = c->offset;
        track->stco.total_count = get_unaligned_be32(buf + 4);
        track->stco.cache_size  = track->stco.total_count;
        if (c->box_cache_size && track->stco.cache_size > c->box_cache_size) {
            track->stco.cache_size = c->box_cache_size;
        }
        track->stco.cache1.start_index = 1;
        track->stco.cache2.start_index = 1 + track->stco.cache_size;
        mp4_warn("track %d [stco] cache size:%d (need memory %d bytes)\r\n",
                 track->track_idx, track->stco.cache_size,
                 track->stco.cache_size * sizeof(struct stco_entry));
    }

    //选择cache
    cache = mp4_box_select_cache(c, &track->stco, sizeof(struct stco_entry));
    if (cache == NULL) {
        return mp4_box_skip_remain(c, box);
    }
    if (!cache->entries) {
        mp4_err("alloc stts fail, cache count:%d\r\n", track->stco.cache_size);
        return -ENOMEM;
    }

    //填充cache
    struct stco_entry *stco = (struct stco_entry *)cache->entries;
    uint8_t skip = mp4_box_parse_done(track, box, &track->stco, cache);
    while (!skip) {
        uint8_t buf[4];
        if (mp4_box_read(c, box, buf, 4) < 0) {
            return 0; //数据不足，需要网络缓冲
        }

        stco[cache->cache_count].chunk_offset = get_unaligned_be32(buf);
        mp4_dbg("track %d [stco] [%d: chunk %d, offset %d]\r\n",
                track->track_idx, cache->cache_count,
                cache->start_index + cache->cache_count,
                stco[cache->cache_count].chunk_offset);

        cache->cache_count++;
        skip = mp4_box_parse_done(track, box, &track->stco, cache);
    }

    //解析到末尾
    if ((cache->start_index + cache->cache_count > track->stco.total_count)) {
        mp4_box_skip_remain(c, box);
    }

    return -EAGAIN; //解析成功，继续加载
}

// co64 box：64位块偏移量表，类似stco
//【box数据很大，需要多次进入才能完成解析】
static int32_t mp4_box_co64_hdl(struct mp4_context *c)
{
    struct mp4_box   *box   = &c->boxs[c->cur_box];
    struct mp4_track *track = c->tracks[c->cur_track];
    struct mp4_box_cache *cache = NULL;

    mp4_dbg("track %d parse stco64 box, remamin %d\r\n", track->track_idx, box->box_size - box->read_size);
    //解析header
    if (track->stco.total_count == 0) {
        uint8_t buf[8];
        if (mp4_box_read(c, box, buf, 8) < 0) {
            return 0; //数据不足，需要网络缓冲
        }
        track->use_co64 = 1;
        track->stco.cur_cache = 0;
        track->stco.offset = c->offset;
        track->stco.total_count = get_unaligned_be32(buf + 4);
        track->stco.cache_size  = track->stco.total_count;
        if (c->box_cache_size && track->stco.cache_size > c->box_cache_size) {
            track->stco.cache_size = c->box_cache_size;
        }
        track->stco.cache1.start_index = 1;
        track->stco.cache2.start_index = 1 + track->stco.cache_size;
        mp4_warn("track %d [stco64] cache size:%d (need memory %d bytes)\r\n",
                 track->track_idx, track->stco.cache_size,
                 track->stco.cache_size * sizeof(struct stco64_entry));
    }

    //选择cache
    cache = mp4_box_select_cache(c, &track->stco, sizeof(struct stco64_entry));
    if (cache == NULL) {
        return mp4_box_skip_remain(c, box);
    }
    if (!cache->entries) {
        mp4_err("alloc stco fail, cache count:%d\r\n", track->stco.cache_size);
        return -ENOMEM;
    }

    //填充cache
    struct stco64_entry *stco64 = (struct stco64_entry *)cache->entries;
    uint8_t skip = mp4_box_parse_done(track, box, &track->stco, cache);
    while (!skip) {
        uint8_t buf[8];
        if (mp4_box_read(c, box, buf, 8) < 0) {
            return 0; //数据不足，需要网络缓冲
        }

        stco64[cache->cache_count].chunk_offset = get_unaligned_be64(buf);
        mp4_dbg("track %d [stco64] [%d: chunk %d, offset:%llu]\r\n",
                track->track_idx, cache->cache_count,
                cache->start_index + cache->cache_count,
                stco64[cache->cache_count].chunk_offset);

        cache->cache_count++;
        skip = mp4_box_parse_done(track, box, &track->stco, cache);
    }

    //解析到末尾
    if ((cache->start_index + cache->cache_count > track->stco.total_count)) {
        mp4_box_skip_remain(c, box);
    }

    return -EAGAIN; //解析成功，继续加载
}

// stsz box：样本大小表，记录每个样本的大小，本地文件使用惰性加载
//【box数据很大，需要多次进入才能完成解析】
static int32_t mp4_box_stsz_hdl(struct mp4_context *c)
{
    struct mp4_box *box = &c->boxs[c->cur_box];
    struct mp4_track *track = c->tracks[c->cur_track];
    struct mp4_box_cache *cache = NULL;

    mp4_dbg("track %d parse stsz box, remamin %d\r\n", track->track_idx, box->box_size - box->read_size);
    //解析header
    if (track->stsz.total_count == 0) {
        uint8_t buf[12];
        if (mp4_box_read(c, box, buf, 12) < 0) {
            return 0; //数据不足，需要网络缓冲
        }
        track->stsz.cur_cache = 0;
        track->stsz.offset = c->offset;
        track->sample_size = get_unaligned_be32(buf + 4);
        track->stsz.total_count = get_unaligned_be32(buf + 8);
        track->stsz.cache_size  = track->sample_size ? 0 : track->stsz.total_count;
        if (c->box_cache_size && track->stsz.cache_size > c->box_cache_size) {
            track->stsz.cache_size = c->box_cache_size;
        }
        track->stsz.cache1.start_index = 1;
        track->stsz.cache2.start_index = 1 + track->stsz.cache_size;
        mp4_warn("track %d [stsz] cache size:%d (need memory %d bytes)\r\n",
                 track->track_idx, track->stsz.cache_size,
                 track->stsz.cache_size * sizeof(struct stsz_entry));
    }

    //固定帧长
    if (track->sample_size) {
        return -EAGAIN;
    }

    //选择cache
    cache = mp4_box_select_cache(c, &track->stsz, sizeof(struct stsz_entry));
    if (cache == NULL) {
        return mp4_box_skip_remain(c, box);
    }
    if (!cache->entries) {
        mp4_err("alloc stsz fail, cache count:%d\r\n", track->stsz.cache_size);
        return -ENOMEM;
    }

    //填充cache
    struct stsz_entry *stsz = (struct stsz_entry *)cache->entries;
    uint8_t skip = mp4_box_parse_done(track, box, &track->stsz, cache);
    while (!skip) {
        uint8_t buf[4];
        if (mp4_box_read(c, box, buf, 4) < 0) {
            return 0; //数据不足，需要网络缓冲
        }

        stsz[cache->cache_count].sample_size = get_unaligned_be32(buf);
        mp4_dbg("track %d [stsz] [%d: sample %d, size %d]\r\n",
                track->track_idx, cache->cache_count,
                cache->start_index + cache->cache_count,
                stsz[cache->cache_count].sample_size);

        cache->cache_count++;
        skip = mp4_box_parse_done(track, box, &track->stsz, cache);
    }

    //解析到末尾
    if ((cache->start_index + cache->cache_count > track->stsz.total_count)) {
        mp4_box_skip_remain(c, box);
    }

    return -EAGAIN; //解析成功，继续加载
}

// stss box：关键帧样本索引表，使用全量加载
//【box数据很大，需要多次进入才能完成解析】
static int32_t mp4_box_stss_hdl(struct mp4_context *c)
{
    struct mp4_box *box = &c->boxs[c->cur_box];
    struct mp4_track *track = c->tracks[c->cur_track];
    struct mp4_box_cache *cache = NULL;

    mp4_dbg("track %d parse stss box, remamin %d\r\n", track->track_idx, box->box_size - box->read_size);
    if (track->mtype != MEDIA_DATA_VIDEO) { //非视频数据，不解析stss
        return mp4_box_skip_remain(c, box);
    }

    //解析header
    if (track->stss.total_count == 0) {
        uint8_t buf[8];
        if (mp4_box_read(c, box, buf, 8) < 0) {
            return 0; //数据不足，需要网络缓冲
        }
        track->stss.cur_cache = 0;
        track->stss.offset = c->offset;
        track->stss.total_count = get_unaligned_be32(buf + 4);
        track->stss.cache_size  = track->stss.total_count;
#if 0 //目前使用全局加载
        if (c->box_cache_size && track->stss.cache_size > c->box_cache_size) {
            track->stss.cache_size = c->box_cache_size;
        }
#endif
        track->stss.cache1.start_index = 1;
        track->stss.cache2.start_index = 1 + track->stss.cache_size;
        mp4_warn("track %d [stss] cache size:%d (need memory %d bytes)\r\n",
                 track->track_idx, track->stss.cache_size,
                 track->stss.cache_size * sizeof(struct stss_entry));
    }

    //选择cache
    cache = mp4_box_select_cache(c, &track->stss, sizeof(struct stss_entry));
    if (cache == NULL) {
        return mp4_box_skip_remain(c, box);
    }
    if (!cache->entries) {
        mp4_err("alloc stts fail, cache count:%d\r\n", track->stss.cache_size);
        return -ENOMEM;
    }

    //填充cache
    struct stss_entry *stss = (struct stss_entry *)cache->entries;
    uint8_t skip = mp4_box_parse_done(track, box, &track->stss, cache);
    while (!skip) {
        uint8_t buf[4];
        if (mp4_box_read(c, box, buf, 4) < 0) {
            return 0; //数据不足，需要网络缓冲
        }

        uint32_t sample_index = get_unaligned_be32(buf);
        stss[cache->cache_count].sample_index = sample_index;
        mp4_dbg("track %d [stss] [%d: sample %d]\r\n", track->track_idx, cache->cache_count, sample_index);

        cache->cache_count++;
        skip = mp4_box_parse_done(track, box, &track->stss, cache);
    }

    //解析到末尾
    if ((cache->start_index + cache->cache_count > track->stss.total_count)) {
        mp4_box_skip_remain(c, box);
    }

    return -EAGAIN; //解析成功，继续加载
}

//【不能多次进入，需要等待足够的数据才能开始解析】
static int32_t mp4_box_dref_hdl(struct mp4_context *c)
{
    struct mp4_box *box = &c->boxs[c->cur_box];
    uint8_t  buf[8] = {0};

    //至少有8byte数据
    if (mp4_box_read(c, box, buf, 8) < 0) {
        return 0; //数据不足，需要网络缓冲
    }

    /* 解析子box*/
    box->child_box = 1;
    return mp4_detect_box(c);
}
// 容器box，检测子box
static int32_t mp4_box_parent_hdl(struct mp4_context *c)
{
    return mp4_detect_box(c);
}
// mdat box：媒体数据容器，负责实际的样本数据读取和输出
//【box数据很大，需要多次进入才能完成解析】
static int32_t mp4_box_mdat_hdl(struct mp4_context *c)
{
    if (c->cur_track >= 0) {
        if (mp4_track_load_stsz(c, c->tracks[c->cur_track], 0)) {
            return -EAGAIN;
        }
        if (mp4_track_load_stco(c, c->tracks[c->cur_track], 0)) {
            return -EAGAIN;
        }
    }

    if (atomic_read(&c->owner->fb_limits) == 0 || c->buffering) {
        return 0;
    }

    if (c->cur_frame) {
        uint32_t len;
        struct mp4_track *track = c->tracks[c->cur_track];
        if (c->offset < track->next_sample_offset) {
            len = track->next_sample_offset - c->offset;
            return mp4_mdat_read(c, track, NULL, len);
        }

        len = c->cur_frame->len - c->cur_frame_len;
        len = mp4_mdat_read(c, track, c->cur_frame->data + c->cur_frame_len, len);
        c->cur_frame_len += len;
        if (c->cur_frame_len >= c->cur_frame->len) {
            mp4_track_output_fb(c, track);
            mp4_track_update_next_sample(c, track, 0);
        }
        return len;
    } else {
        struct mp4_track *track = mp4_select_next_track(c);
        if (!track) {
            c->ops->seek(c->file, 0, SEEK_END);
            mp4_warn("cur offset:%llu, no next sample!\r\n", c->offset);
            return 0;
        }

        c->cur_track = track->track_idx;
        c->cur_frame = mp4_track_alloc_fb(c, track);
        if (!c->cur_frame) {
            return -ENOMEM;
        }
        return -EAGAIN; //继续加载下一个sample数据
    }
}

static const struct {
    uint32_t tag;
    int32_t (*hdl)(struct mp4_context *c);
} mp4_box_hdls[] = {
    { BOX_MOOV,  mp4_box_parent_hdl },
    { BOX_MOOF,  mp4_box_parent_hdl },
    { BOX_MVHD,  mp4_box_mvhd_hdl },
    { BOX_TRAK,  mp4_box_parent_hdl },
    { BOX_META,  mp4_box_parent_hdl },
    { BOX_TKHD,  mp4_box_tkhd_hdl },
    { BOX_MDIA,  mp4_box_parent_hdl },
    { BOX_EDTS,  mp4_box_parent_hdl },
    { BOX_MDHD,  mp4_box_mdhd_hdl },
    { BOX_HDLR,  mp4_box_hdlr_hdl },
    { BOX_MINF,  mp4_box_parent_hdl },
    { BOX_DINF,  mp4_box_parent_hdl },
    { BOX_STBL,  mp4_box_parent_hdl },
    { BOX_DREF,  mp4_box_dref_hdl },
    { BOX_STSD,  mp4_box_stsd_hdl },
    { BOX_STTS,  mp4_box_stts_hdl },
    { BOX_STSC,  mp4_box_stsc_hdl },
    { BOX_STSZ,  mp4_box_stsz_hdl },
    { BOX_STCO,  mp4_box_stco_hdl },
    { BOX_CO64,  mp4_box_co64_hdl },
    { BOX_STSS,  mp4_box_stss_hdl },
    { BOX_AVC1,  mp4_box_avc1_hdl },
    { BOX_AVC2,  mp4_box_avc1_hdl },
    { BOX_HVC1,  mp4_box_hvc1_hdl },
    { BOX_HEV1,  mp4_box_hev1_hdl },
    { BOX_MP4A,  mp4_box_mp4a_hdl },
    { BOX_AVCC,  mp4_box_avcc_hdl },
    { BOX_HVCC,  mp4_box_hvcc_hdl },
    { BOX_ESDS,  mp4_box_esds_hdl },
    { BOX_TRAF,  mp4_box_parent_hdl },
    { BOX_MDAT,  mp4_box_mdat_hdl },
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//  加载另一批stco信息
static int32_t  mp4_track_load_stco(struct mp4_context *c, struct mp4_track *track, uint32_t next_chunk_idx)
{
    uint32_t box_tag    = (track->use_co64 ? BOX_CO64 : BOX_STCO);
    box_hdl  hdl        = (track->use_co64 ? mp4_box_co64_hdl : mp4_box_stco_hdl);
    uint8_t  entry_size = (track->use_co64 ? sizeof(struct stco64_entry) : sizeof(struct stco_entry));
    struct mp4_box_cache *cur_cache  = (track->stco.cur_cache ? &track->stco.cache2 : &track->stco.cache1);
    struct mp4_box_cache *next_cache = (track->stco.cur_cache ? &track->stco.cache1 : &track->stco.cache2);

    if (next_chunk_idx == 0) {
        next_chunk_idx = cur_cache->start_index + cur_cache->cache_count; //下一批chunk index
    }

    if ((next_chunk_idx > track->stco.total_count) || (next_chunk_idx == next_cache->start_index)) {
        return 0;
    }

    uint64_t offset   = track->stco.offset + (next_chunk_idx - 1) * entry_size;
    uint32_t box_size = track->stco.cache_size * entry_size;

    mp4_warn("track %d [stco] %s reload! start:%d. (%llu)\r\n", track->track_idx,
             track->stco.cur_cache ? "cache1" : "cache2", next_chunk_idx, c->offset);
    if (c->ops->seek(c->file, (off_t)offset, SEEK_SET) < 0) {
        return 0;
    }

    c->offset_bak = c->offset;
    c->offset = offset;
    rbuffer_reset(&c->io_buf);
    c->cur_track = track->track_idx; //切换当前track
    next_cache->cache_count = 0;
    next_cache->start_index = next_chunk_idx;
    mp4_box_push(c, hdl, box_tag, NULL); //stco box入栈
    c->boxs[c->cur_box].box_size = box_size; //需要读取的size
    return 1;
}

//  加载另一批stsz信息
static int32_t mp4_track_load_stsz(struct mp4_context *c, struct mp4_track *track, uint32_t next_sample_idx)
{
    struct mp4_box_cache *cur_cache  = (track->stsz.cur_cache ? &track->stsz.cache2 : &track->stsz.cache1);
    struct mp4_box_cache *next_cache = (track->stsz.cur_cache ? &track->stsz.cache1 : &track->stsz.cache2);

    if (next_sample_idx == 0) {
        if (track->next_sample_index < cur_cache->start_index + (cur_cache->cache_count / 2)) {
            return 0;
        }
        next_sample_idx = cur_cache->start_index + cur_cache->cache_count; //下一个sample index
    }

    if ((next_sample_idx > track->stsz.total_count) || (next_sample_idx == next_cache->start_index)) {
        return 0;
    }

    uint64_t offset   = track->stsz.offset + (next_sample_idx - 1) * sizeof(struct stsz_entry);
    uint32_t box_size = track->stsz.cache_size * sizeof(struct stsz_entry);

    mp4_warn("track %d [stsz] %s reload! start:%d. (%llu)\r\n", track->track_idx,
             track->stsz.cur_cache ? "cache1" : "cache2", next_sample_idx, c->offset);
    if (c->ops->seek(c->file, (off_t)offset, SEEK_SET) < 0) {
        return 0;
    }

    c->offset_bak = c->offset;
    c->offset = offset;
    rbuffer_reset(&c->io_buf);
    c->cur_track = track->track_idx; //切换当前track
    next_cache->cache_count = 0;
    next_cache->start_index = next_sample_idx;
    mp4_box_push(c, mp4_box_stsz_hdl, BOX_STSZ, NULL); //stsz box入栈
    c->boxs[c->cur_box].box_size = box_size; //需要读取的size
    return 1;
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

static box_hdl mp4_get_box_hdl(struct mp4_context *c, uint32_t box_tag)
{
    for (uint32_t i = 0; i < ARRAY_SIZE(mp4_box_hdls); i++) {
        if (mp4_box_hdls[i].tag == box_tag) {
            return mp4_box_hdls[i].hdl;
            break;
        }
    }
    return NULL;
}

static void mp4_box_pop(struct mp4_context *c)
{
    //先弹出当前的box
    struct mp4_box *box = &c->boxs[c->cur_box];
    do {
        switch (box->box_tag) {
            case BOX_MOOV:
                if (c->moov_tail) {
                    c->ops->seek(c->file, c->mdat_offset, SEEK_SET);
                    c->offset = c->mdat_offset;
                    rbuffer_reset(&c->io_buf);
                    mp4_warn("moov parse done, go back to mdat box (offset %llu)\r\n", c->offset);
                }
                break;
            case BOX_TRAK:
                mp4_track_update_next_sample(c, c->tracks[c->cur_track], 0);
                mp4_warn("track %d parse done, first sample:\r\n", c->cur_track);
                mp4_warn("    sample index :%d\r\n", c->tracks[c->cur_track]->next_sample_index);
                mp4_warn("    sample size  :%d\r\n", c->tracks[c->cur_track]->next_sample_size);
                mp4_warn("    sample time  :%d\r\n", c->tracks[c->cur_track]->next_sample_time);
                mp4_warn("    sample offset:%lld\r\n", c->tracks[c->cur_track]->next_sample_offset);
                c->cur_track = -1;
                break;
            default:
                break;
        }

        mp4_dbg("["MP4_TAG_FMT"] parse done! cur_box:%d, cur offset:%llu.\r\n",
                MP4_TAG_STR(box->box_tag), c->cur_box, c->offset);

        c->cur_box--;
        if (c->cur_box < 0) { break; }
        box = &c->boxs[c->cur_box];
        if (box->read_size < box->box_size) { break; }
    } while (1);
}

static int32_t mp4_box_push(struct mp4_context *c, box_hdl hdl, uint32_t box_tag, struct mp4_box *parent)
{
    if (c->cur_box >= (MP4_BOX_DEPTH - 1)) {
        mp4_err("box depth overflow! %d\r\n", MP4_BOX_DEPTH);
        return 0;
    }

    c->cur_box++;
    struct mp4_box *box = &c->boxs[c->cur_box];
    os_memset(box, 0, sizeof(struct mp4_box));
    box->box_tag = box_tag;
    box->parent  = parent;
    box->hdl     = hdl;
    return 1;
}

static int mp4_detect_box(struct mp4_context *c)
{
    uint8_t header[MP4_BOX_HEADER_SIZE];
    struct mp4_box *parent;

    if (mp4_box_read(c, NULL, header, MP4_BOX_HEADER_SIZE) < 0) {
        return 0; //数据不足，需要网络缓冲
    }

    uint64_t offset   = c->offset - MP4_BOX_HEADER_SIZE;
    uint64_t box_size = get_unaligned_be32(header);
    uint32_t box_tag  = get_unaligned_be32(header + 4);
    box_hdl  hdl      = mp4_get_box_hdl(c, box_tag);
    if (box_size == 0) box_size = c->file_size - offset;

    //for(int i=0;i<c->cur_box+1;i++) _os_printf("  ");
    //_os_printf("["MP4_TAG_FMT"], size:%llu, offset:%llu. %d\r\n", MP4_TAG_STR(box_tag), box_size, offset, c->cur_box);

    if (box_tag == BOX_MOOV) {
        c->moov_parsed = 1;
    } else if (box_tag == BOX_MDAT) {
        c->mdat_offset = offset;
    }

    parent = (c->cur_box < 0 ? NULL : &c->boxs[c->cur_box]);
    if (mp4_box_push(c, hdl, box_tag, parent)) {
        c->boxs[c->cur_box].box_size  = box_size;
        mp4_box_read_size(c, &c->boxs[c->cur_box], MP4_BOX_HEADER_SIZE);
    }
    return -EAGAIN;
}

static int32_t mp4_parse_box(struct mp4_context *c)
{
    int ret = 0;
    struct mp4_box *box = &c->boxs[c->cur_box];

    if (box->box_size == 1) {
        uint8_t buf[8];
        if (mp4_box_read(c, box, buf, 8) < 0) {
            return 0; //数据不足，需要网络缓冲
        }
        box->box_size = get_unaligned_be64(buf);
        mp4_warn("["MP4_TAG_FMT"], large size:%llu\r\n", MP4_TAG_STR(box->box_tag), box->box_size);
    }

    if (box->box_tag == BOX_MDAT) {
        if (!c->moov_parsed) {
            mp4_box_pop(c); //弹出当前的box
            uint64_t offset = c->mdat_offset + box->box_size;
            c->ops->seek(c->file, offset, SEEK_SET);
            c->offset = offset;
            c->moov_tail = 1;
            rbuffer_reset(&c->io_buf);
            mp4_warn("moov is at tail ?? try to get it! seek to %llu.\r\n", offset);
            return -EAGAIN; //继续加载
        }
    }

    if (box->hdl == NULL || box->skip) {
        ret = mp4_box_skip_remain(c, box);
    } else {
        if (box->child_box) {
            ret = mp4_detect_box(c);
        } else {
            ret = box->hdl(c);
        }
    }

    if (box->read_size >= box->box_size) {
        mp4_box_pop(c);
        if (c->delay_seek_time != -1) {
            return mp4_delay_seek(c, c->delay_seek_time);
        }
        mp4_mdat_goback(c);
    }
    return ret;
}

static int32_t mp4_do_demux(void *ctx)
{
    int32_t ret1 = 0;
    int32_t ret2 = 0;
    struct mp4_context *c = (struct mp4_context *)ctx;
    if (!c) {
        return 0;
    }

    struct mp4_box *box = (c->cur_box < 0 ? NULL : &c->boxs[c->cur_box]);
    if (!box) {
        uint32_t avail = mp4_box_avail(c);
        if (avail < MP4_BOX_HEADER_SIZE) {
            ret1 = mp4_box_fill(c, MP4_BOX_HEADER_SIZE - avail);
        }
        if (mp4_box_avail(c) < MP4_BOX_HEADER_SIZE) {
            return 0; //数据不足，需要网络缓冲
        }
        ret1 = mp4_detect_box(c);
        box = (c->cur_box < 0 ? NULL : &c->boxs[c->cur_box]);
    }

    if (box && box->box_tag != BOX_MDAT) {
        uint32_t remain = box->box_size - box->read_size;
        if (mp4_box_avail(c) < remain) {
            remain -= mp4_box_avail(c);
            ret1 = mp4_box_fill(c, remain);
        }
    }

    if (c->cur_box >= 0) {
        ret2 = mp4_parse_box(c);
    }
    return (ret2 ? ret2 : ret1);
}

static uint32_t mp4_seek_keyfrm(struct mp4_context *c, uint32_t time_ms)
{
    if (c->video_track >= 0) {
        struct mp4_track *track = c->tracks[c->video_track];
        uint32_t sample_idx = mp4_track_seek_sample(c, track, time_ms);
        if (sample_idx > track->stsz.total_count) {
            return -1;
        }
        track->seek_sample_index = mp4_track_get_key_sample(c, track, sample_idx);
        time_ms = mp4_track_get_sample_time(c, track, track->seek_sample_index);
        mp4_warn("mp4 seek to keyfrm %d, time:%d ms\r\n", track->seek_sample_index, time_ms);
    }
    return (uint32_t)time_ms;
}

//将track seek 到指定的位置，更新next sample信息
static int32_t mp4_track_seek(struct mp4_context *c, struct mp4_track *track, uint32_t time_ms)
{
    uint32_t sample_idx = track->seek_sample_index;

    if (sample_idx == 0) {
        sample_idx = mp4_track_seek_sample(c, track, time_ms);
        track->seek_sample_index = sample_idx;
    }

    if (sample_idx > track->stsz.total_count || sample_idx == 0) {
        mp4_dbg("track %d seek error! sample %d, total %d\r\n", track->track_idx, sample_idx, track->stsz.total_count);
        return 1;
    }

    if (track->next_sample_index == sample_idx) {
        mp4_dbg("track %d seek done! sample:%d, offset:%d\r\n", track->track_idx, track->next_sample_index, track->next_sample_offset);
        return 1;
    }

    uint64_t next_sample_offset = mp4_track_get_sample_offset(c, track, sample_idx, 1);
    uint32_t next_sample_size   = mp4_track_get_sample_size(c, track, sample_idx, 1);
    if (next_sample_offset && next_sample_size) {
        track->next_sample_index  = sample_idx;
        track->next_sample_offset = next_sample_offset;
        track->next_sample_size   = next_sample_size;
        track->next_sample_time   = mp4_track_get_sample_time(c, track, sample_idx);
        mp4_warn("track %d seek done! sample:%d, offset:%d\r\n", track->track_idx, track->next_sample_index, track->next_sample_offset);
        return 1;
    } else if (next_sample_size == 0) {
        uint32_t s = sample_idx > 32 ? sample_idx - 32 : 1;
        if (mp4_track_load_stsz(c, track, s)) {
            return -1;
        }
    } else if (next_sample_offset == 0) {
        uint32_t chunk_idx = mp4_track_get_sample_chunk(c, track, sample_idx, 1, NULL);
        chunk_idx = (chunk_idx > 32) ? chunk_idx - 32 : 1;
        if (mp4_track_load_stco(c, track, chunk_idx)) {
            return -1;
        }
    }

    return 0;
}

static int32_t mp4_delay_seek(struct mp4_context *c, uint32_t time_ms)
{
    int8_t done = 0;

    mp4_warn("mp4_delay_seek to %d ms\r\n", time_ms);
    c->delay_seek_time = -1;

    //将各个track seek到正确的位置，可能需要reload stco/stsz等信息
    for (int i = 0; i < c->track_cnt; i++) {
        struct mp4_track *track = c->tracks[i];
        if (track == NULL) continue;

        int32_t ret = mp4_track_seek(c, track, time_ms);
        if (ret == -1) {
            c->delay_seek_time = time_ms;
            return 0;
        } else if (ret == 1) {
            done++;
        }
    }

    if (done == c->track_cnt) {
        struct mp4_track *track = mp4_select_next_track(c);
        mp4_warn("seek done! track %d sample %d, offset:%llu, size:%d, time:%d\r\n", 
                    track->track_idx, 
                    track->next_sample_index,
                    track->next_sample_offset,
                    track->next_sample_size,
                    track->next_sample_time);
        mp4_mdat_seek(c, track->next_sample_offset);
        c->cur_track = track->track_idx;
        c->cur_frame = mp4_track_alloc_fb(c, track);
        if (!c->cur_frame) {
            return -ENOMEM;
        }
        return 0;
    }

    return -EAGAIN;
}

// 定位到指定时间点（毫秒）
static int32_t mp4_do_seek(void *ctx, uint32_t time_ms)
{
    struct mp4_context *c = (struct mp4_context *)ctx;

    if (!c || c->file_size == (uint64_t) - 1 || c->video_track == -1) {
        return -ENOTSUP;
    }

    mp4_warn("mp4 seek to %d ms. total duration:%d ms\r\n", time_ms, c->total_duration_ms);
    if (time_ms > c->total_duration_ms) {
        return -ENOTSUP;
    }

    c->buffering = 0;
    for (int i = 0; i < c->track_cnt; i++) {
        if (c->tracks[i]) c->tracks[i]->seek_sample_index = 0;
    }

    time_ms = mp4_seek_keyfrm(c, time_ms);
    if (time_ms > c->total_duration_ms) {
        return -ENOTSUP;
    }

    if (c->cur_frame) {
        fb_put(c->cur_frame);
        c->cur_frame = NULL;
        c->cur_frame_len = 0;
    }

    mp4_delay_seek(c, time_ms);
    return RET_OK;
}

static void *mp4_init(void *hdl, const struct AVDemuxerOps *ops, void *hdr, uint32_t len, struct msi *owner)
{
    struct mp4_context *c = decoder_mem_calloc(1, sizeof(struct mp4_context));
    if (!c) {
        return NULL;
    }

    c->cur_track   = -1;
    c->cur_box     = -1;
    c->video_track = -1;
    c->audio_track = -1;
    c->subtitle_track = -1;
    c->delay_seek_time  = -1;
    c->ops      = ops;
    c->file     = hdl;
    c->owner    = owner;
    c->stream_type = ops->ioctl(owner, AVDEMUXER_GET_STREAM_TYPE, 0, 0);
    ops->ioctl(owner, AVDEMUXER_GET_FILE_SIZE, (uint32)&c->file_size, 0);
    c->io_buf.qsize = max(IO_BUFFER_SIZE, len + 1);
    c->io_buf.rbq   = decoder_mem_alloc(c->io_buf.qsize);
    if (c->io_buf.rbq == NULL) {
        decoder_mem_free(c);
        return NULL;
    }
    os_memcpy(c->io_buf.rbq, hdr, len);
    c->io_buf.wpos = len;

    switch (c->stream_type) {
        case AVDEMUXER_STREAM_FILE:
            c->box_cache_size = 1024;
            break;
        case AVDEMUXER_STREAM_URLFILE:
            //c->box_cache_size = c->io_buf.qsize > (512 * 1024) ? 2048 : 0;
            break;
        default:
            break;
    }

    return c;
}

static void mp4_free_codec_data(struct mp4_track *track)
{
    if (track->codec_data == NULL) { return; }
    if (track->mtype == MEDIA_DATA_VIDEO && track->stype == VIDEO_CODEC_H264) {
        h264_avcc_info_t *info = (h264_avcc_info_t *)track->codec_data;
        if (info->sps_data) { decoder_mem_free((void *)info->sps_data); }
        if (info->pps_data) { decoder_mem_free((void *)info->pps_data); }
        decoder_mem_free(info);
    } else if (track->mtype == MEDIA_DATA_VIDEO && track->stype == VIDEO_CODEC_H265) {
        hevc_codec_info_t *info = (hevc_codec_info_t *)track->codec_data;
        if (info->vps_data) { decoder_mem_free((void *)info->vps_data); }
        if (info->sps_data) { decoder_mem_free((void *)info->sps_data); }
        if (info->pps_data) { decoder_mem_free((void *)info->pps_data); }
        decoder_mem_free(info);
    } else {
        decoder_mem_free(track->codec_data);
    }
    track->codec_data = NULL;
}
static int32_t mp4_release(void *ctx)
{
    struct mp4_context *c = (struct mp4_context *)ctx;
    if (!c) {
        return 0;
    }

    for (int i = 0; i < c->track_cnt; i++) {
        struct mp4_track *t = c->tracks[i];
        if (t) {
#ifdef MP4_SAVE_TRACK
            if (t->fp_track) { fclose(t->fp_track); }
#endif
            mp4_free_codec_data(t);
            decoder_mem_free(t->stts.cache1.entries);
            decoder_mem_free(t->stts.cache2.entries);
            decoder_mem_free(t->stsc.cache1.entries);
            decoder_mem_free(t->stsc.cache2.entries);
            decoder_mem_free(t->stss.cache1.entries);
            decoder_mem_free(t->stss.cache2.entries);
            decoder_mem_free(t->stco.cache1.entries);
            decoder_mem_free(t->stco.cache2.entries);
            decoder_mem_free(t->stsz.cache1.entries);
            decoder_mem_free(t->stsz.cache2.entries);
            decoder_mem_free(t);
            c->tracks[i] = NULL;
        }
    }

#ifdef MP4_SAVE_MDAT
    if (c->fp_mdat) { fclose(c->fp_mdat); }
#endif

    if (c->cur_frame) {
        fb_put(c->cur_frame);
    }

    decoder_mem_free(c->io_buf.rbq);
    decoder_mem_free(c);
    return 0;
}

static int mp4_set_track(struct mp4_context *c, uint32_t track_id)
{
    if (track_id >= c->track_cnt) {
        return -EINVAL;
    }

    struct mp4_track *t = c->tracks[track_id];
    if (t && !mp4_track_is_active(c, track_id)) {
        if (t->mtype == MEDIA_DATA_AUDIO) {
            c->audio_track = track_id;
        } else if (t->mtype == MEDIA_DATA_SUBTITLE) {
            c->subtitle_track = track_id;
        }
        return 0;
    }
    return -1;
}

static int mp4_ioctl(void *ctx, uint32_t cmd, uint32_t param1, uint32_t param2)
{
    int32 ret = 0;
    struct mp4_context *c = (struct mp4_context *)ctx;
    if (!c) {
        return -EINVAL;
    }

    switch (cmd) {
        case AVDEMUXER_GET_TOTAL_DURATION:
            if (param1) {
                *(uint32_t *)param1 = (uint32_t)c->total_duration_ms;
            }
        case AVDEMUXER_SET_TRACK:
            ret = mp4_set_track(c, param1);
            break;
        case AVDEMUXER_SET_BUFFERING:
            c->buffering = param1;
            mp4_dbg("mp4 demuxer buffering: %d\r\n", c->buffering);
            break;
        default:
            ret = -ENOTSUP;
            break;
    }
    return ret;
}
__avdemuxer const struct AVDemuxer mp4_demuxer = {
    .type     = MEDIA_CONTAINER_MP4,
    .name     = "mp4-demuxer",
    .init     = mp4_init,
    .release  = mp4_release,
    .do_seek  = mp4_do_seek,
    .do_demux = mp4_do_demux,
    .ioctl    = mp4_ioctl,
};
