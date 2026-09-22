#ifndef __MP4_H__
#define __MP4_H__

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

// 音视频 framebuff 独立配额
struct mp4_fb_quota {
    atomic_t users;        // 引用计数: 持有者1 + 在途帧数, 归零释放本对象
    atomic_t video_left;   // 视频当前剩余配额
    atomic_t audio_left;   // 音频当前剩余配额
    uint8_t  video_max;    // 视频配额上限
    uint8_t  audio_max;    // 音频配额上限
    uint8_t  inited;       // 配额上限是否已初始化
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
    uint8_t     output_blocked : 1; // 还有剩余样本但对应轨道配额耗尽被跳过, 暂时无轨可输出

    struct mp4_fb_quota *fb_quota;  // 音视频 framebuff 独立配额对象(独立引用计数, 见结构体注释)

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
void mp4_save_track_to_file(struct mp4_context *c, struct mp4_track *track, void *data, uint32_t size);
#else
#define mp4_save_track_to_file(c, track, data, size)
#endif
#ifdef MP4_SAVE_MDAT
void mp4_save_mdat_to_file(struct mp4_context *c, void *data, uint32_t size);
#else
#define mp4_save_mdat_to_file(c, data, size)
#endif

/* 跨文件调用的函数声明 */

// mp4.c 入口函数
void *mp4_init(void *hdl, const struct AVDemuxerOps *ops, void *hdr, uint32_t len, struct msi *owner);
int32_t mp4_release(void *ctx);
int32_t mp4_do_demux(void *ctx);
int32_t mp4_do_seek(void *ctx, uint32_t time_ms);
int mp4_ioctl(void *ctx, uint32_t cmd, uint32_t param1, uint32_t param2);

// mp4_box.c
int32_t mp4_box_avail(struct mp4_context *c);
int32_t mp4_box_fill(struct mp4_context *c, uint32_t need_size);
int mp4_detect_box(struct mp4_context *c);
int32_t mp4_parse_box(struct mp4_context *c);
int32_t mp4_box_push(struct mp4_context *c, box_hdl hdl, uint32_t box_tag, struct mp4_box *parent);
int32_t mp4_box_stco_hdl(struct mp4_context *c);
int32_t mp4_box_co64_hdl(struct mp4_context *c);
int32_t mp4_box_stsz_hdl(struct mp4_context *c);

// mp4_demux.c
int mp4_track_is_active(struct mp4_context *c, uint32_t track_idx);
struct mp4_track *mp4_track_find(struct mp4_context *c, uint8_t track_id);
uint32_t mp4_track_get_key_sample(struct mp4_context *c, struct mp4_track *track, uint32_t sample_idx);
uint32_t mp4_track_get_sample_chunk(struct mp4_context *c, struct mp4_track *track, uint32_t sample_idx, uint32_t seek, uint32_t *first_sample);
uint32_t mp4_track_get_sample_size(struct mp4_context *c, struct mp4_track *track, uint32_t sample_idx, uint8_t sw_cache);
uint64_t mp4_track_get_sample_offset(struct mp4_context *c, struct mp4_track *track, uint32_t sample_idx, uint8_t sw_cache);
uint32_t mp4_track_get_sample_time(struct mp4_context *c, struct mp4_track *track, uint32_t sample_idx);
uint32_t mp4_track_seek_sample(struct mp4_context *c, struct mp4_track *track, uint32_t time_ms);
int mp4_track_output_fb(struct mp4_context *c, struct mp4_track *track);
void mp4_track_update_next_sample(struct mp4_context *c, struct mp4_track *track, uint32_t next_index);
struct mp4_track *mp4_select_next_track(struct mp4_context *c);
int mp4_track_output_blocked(struct mp4_context *c, uint8 mtype);
void mp4_fb_quota_put(struct mp4_fb_quota *q);   // 归还配额对象引用, 最后一个归还者释放对象
struct framebuff *mp4_track_alloc_fb(struct mp4_context *c, struct mp4_track *track);
int32_t mp4_mdat_read(struct mp4_context *c, struct mp4_track *track, uint8_t *dst, uint32_t len);
void mp4_free_codec_data(struct mp4_track *track);
int32_t mp4_track_load_stco(struct mp4_context *c, struct mp4_track *track, uint32_t next_chunk_idx);
int32_t mp4_track_load_stsz(struct mp4_context *c, struct mp4_track *track, uint32_t next_sample_idx);

// mp4_file.c
int32_t mp4_file_mdat_hdl(struct mp4_context *c);
struct mp4_track *mp4_file_select_next_track(struct mp4_context *c);
int32_t mp4_mdat_seek(struct mp4_context *c, uint64_t offset);
void mp4_mdat_goback(struct mp4_context *c);
uint32_t mp4_seek_keyfrm(struct mp4_context *c, uint32_t time_ms);
int32_t mp4_delay_seek(struct mp4_context *c, uint32_t time_ms);

// mp4_net.c
int32_t mp4_net_mdat_hdl(struct mp4_context *c);

#endif /* __MP4_H__ */