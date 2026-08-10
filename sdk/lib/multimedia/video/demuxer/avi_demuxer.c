#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/multimedia/AVContainer.h"
#include "lib/multimedia/media_types.h"
#include "lib/multimedia/video.h"
#include "lib/multimedia/audio.h"

#define avi_dbg(fmt, ...)      os_printf("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define avi_err(fmt, ...)      os_printf(KERN_ERR"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define avi_warn(fmt, ...)     os_printf(KERN_WARNING fmt, ##__VA_ARGS__)

// FourCC 宏，大端字符序
#define FOURCC(a,b,c,d) (((uint32_t)(a)<<24)|((uint32_t)(b)<<16)|((uint32_t)(c)<<8)|(d))

// 从内存按大端读取 FourCC
static inline uint32_t avi_fourcc(const void *p)
{
    const uint8_t *b = (const uint8_t *)p;
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
}

#define AUDIO_TMP_BUF_SIZE  (8192)
#define AVI_MAX_CHUNK_SIZE  (2 * 1024 * 1024)   /* 单 chunk 合理上限 2MB */
#define FAST_INDEX_STEP     (300)
#define IDX1_READ_SIZE      (512)

#define AVI_MODE_INDEXED     0
#define AVI_MODE_STREAMING   1

#define AVI_HDR_READ_CHUNK   (4096)
#define AVI_HDR_MAX_SIZE     (262144)

#define AVI_INDEX_WINDOW     (3000)

// avih 主头部，标准 AVI 结构
typedef struct {
    uint32_t dwFourCC;              // chunk 标识 "avih"
    uint32_t dwSize;                // chunk 大小
    uint32_t dwMicroSecPerFrame;    // 每帧微秒数
    uint32_t dwMaxBytesPerSec;      // 最大码率
    uint32_t dwPaddingGranularity;  // 对齐粒度
    uint32_t dwFlages;              // 标志位
    uint32_t dwTotalFrame;          // 总帧数
    uint32_t dwInitialFrames;       // 预读帧数
    uint32_t dwStreams;             // 流数量
    uint32_t dwSuggestedBufferSize; // 建议缓冲大小
    uint32_t dwWidth;               // 视频宽度
    uint32_t dwHeight;              // 视频高度
    uint32_t dwReserved[4];         // 保留
} avi_avih_t;

// strh 流头部，标准 AVI 结构
typedef struct {
    uint32_t fccType;               // 流类型 "vids" / "auds"
    uint32_t fccHandler;            // 编解码器 FourCC
    uint32_t dwFlags;               // 标志位
    uint16_t wPriority;             // 优先级
    uint16_t wLanguage;             // 语言
    uint32_t dwInitalFrames;        // 预读帧数
    uint32_t dwScale;               // 时间刻度
    uint32_t dwRate;                // 时间速率
    uint32_t dwStart;               // 起始时间
    uint32_t dwLength;              // 流长度
    uint32_t dwSuggestedBufferSize; // 建议缓冲大小
    uint32_t dwQuality;             // 质量
    uint32_t dwSampleSize;          // 采样大小
    uint16_t rcFrame[4];            // 裁剪矩形
} avi_strh_t;

// WAVEFORMATEX，strf 中的音频格式描述
typedef struct {
    uint16_t wFormatTag;            // 编码格式标签
    uint16_t nChannels;             // 声道数
    uint32_t nSamplesPerSec;        // 采样率
    uint32_t nAvgBytesPerSec;       // 平均码率
    uint16_t nBlockAlign;           // 块对齐
    uint16_t wBitsPerSample;        // 采样位深
    uint16_t cbSize;                // 附加数据大小
} avi_wavefmt_t;

// idx1 索引项，16 字节
typedef struct {
    uint32_t dwChunkId;             // chunk FourCC
    uint32_t dwFlags;               // 标志位
    uint32_t dwOffset;              // 相对 movi 的偏移
    uint32_t dwSize;                // chunk 大小
} avi_idx1_t;

// 快速索引链表节点，用于加速 seek
typedef struct avi_fast_idx_s {
    struct avi_fast_idx_s *next;    // 下一节点
    uint32_t frame_cnt;             // 对应视频帧计数
    uint32_t audio_bytes;           // 对应音频累计字节
    uint32_t offset;                // 在 idx1 中的文件偏移
} avi_fast_idx_t;

// RIFF chunk 头部，8 字节
typedef struct {
   uint32_t fcc;                    // FourCC 标识
   uint32_t size;                   // chunk 大小
} avi_chunk_t;

// AVI 文件元数据及索引读取状态
typedef struct {
    avi_avih_t avih;                // avih 主头部
    uint32_t fps;                   // 视频帧率
    uint32_t sample_rate;           // 音频采样率
    uint16_t channels;              // 音频声道数
    uint16_t bits;                  // 音频采样位深
    uint32_t movi_off;              // movi LIST 偏移
    uint32_t idx1_off;              // idx1 索引偏移
    uint32_t idx1_len;              // idx1 索引长度
    avi_fast_idx_t *fast_idx;       // 快速索引链表头

    uint32_t v_idx_buf[IDX1_READ_SIZE / 4]; // 视频索引读取缓冲
    uint32_t v_idx_pos;             // 视频索引当前文件位置
    int32_t  v_frame_cnt;           // 视频已输出帧计数
    uint32_t v_idx_left;            // 视频索引剩余字节
    int32_t  v_idx_cached;          // 视频索引缓冲有效字节
    avi_idx1_t *v_idx_ptr;          // 视频索引当前指针

    uint32_t a_idx_buf[IDX1_READ_SIZE / 4]; // 音频索引读取缓冲
    uint32_t a_idx_pos;             // 音频索引当前文件位置
    int32_t  a_byte_cnt;            // 音频累计字节
    uint32_t a_idx_left;            // 音频索引剩余字节
    int32_t  a_idx_cached;          // 音频索引缓冲有效字节
    avi_idx1_t *a_idx_ptr;          // 音频索引当前指针

    uint8_t  track_cnt;             // 流数量
    uint32_t v_fcc;                 // 视频 chunk FourCC
    uint32_t a_fcc;                 // 音频 chunk FourCC

    uint32_t *seek_tab;             // 精简 seek 表，循环覆盖
    uint32_t seek_cap;              // seek 表容量
    uint32_t scan_pos;              // movi 当前扫描位置
    uint32_t out_vframe;            // 已输出视频帧计数
    avi_chunk_t pend_chunk;         // 挂起的 chunk 头
    uint8_t  pend_flag;             // 挂起标记
    uint32_t pend_off;              // 挂起数据偏移

    uint8_t  audio_codec;           // 音频编码类型
    uint8_t  video_codec;           // 视频编码类型 (VIDEO_CODEC_MJPEG / VIDEO_CODEC_H264)
    uint32_t video_fcc;             // strh.fccHandler 原始 FourCC

    uint32_t avg_bytes_per_sec;     // 音频平均字节率（nAvgBytesPerSec，用于时间戳计算）

} avi_file_t;

// AVI demuxer 上下文
typedef struct {
    struct msi *owner;              // 所属 MSI 组件
    const struct AVDemuxerOps *ops; // 文件操作接口
    void *file;                     // 文件句柄
    uint32_t file_pos;              // 当前文件偏移

    avi_file_t *avi;                // AVI 文件结构

    uint32_t v_time;                // 视频当前时间 ms
    uint32_t a_time;                // 音频当前时间 ms
    uint32_t a_out_bytes;           // 音频已输出字节

    txVideoInfo_t v_info;           // 视频参数
    txAudioInfo_t a_info;           // 音频参数

    uint32_t duration;              // 总时长 ms

    uint8_t *hdr;                   // 预读头部数据
    uint32_t hdr_len;               // 预读数据长度
    uint32_t hdr_pos;               // 预读数据指针

    uint32_t pend_v_off;            // 挂起视频偏移
    uint32_t pend_v_size;           // 挂起视频大小
    int32_t  pend_v_frame;          // 挂起视频帧号
    uint32_t pend_a_off;            // 挂起音频偏移
    uint32_t pend_a_size;           // 挂起音频大小

    uint32_t a_chunk_left;          // 音频分片剩余字节
    uint32_t a_chunk_pos;           // 音频分片当前位置

    int32_t  seek_frame;            // seek 目标帧号

    uint8_t  mode;                  // 工作模式
    uint32_t a_chunk_next;          // 音频分片结束位置
    uint32_t hdr_cap;               // 预读缓冲容量

    uint32_t tick_base;             // 播放基准时刻
    uint8_t  seek_sync;             // seek 后音频强制同步计数器
    uint8_t  wait_audio;            // seek 后强制等待音频到来再输出视频
    uint8_t  seek_skip_video;       // seek 后强制跳过视频，优先寻找音频
    uint8_t  seek_skip_cnt;         // 已跳过视频计数，防止无限跳过
} avi_ctx_t;

// 读取数据并累加 file_pos，优先从 hdr 缓冲读取
static inline size_t avi_read(avi_ctx_t *ctx, void *ptr, size_t size)
{
    size_t n = 0;

    if (ctx->hdr && ctx->hdr_pos < ctx->hdr_len) {
        size_t avail = ctx->hdr_len - ctx->hdr_pos;
        n = (size < avail) ? size : avail;
        memcpy(ptr, ctx->hdr + ctx->hdr_pos, n);
        ctx->hdr_pos += n;
        ctx->file_pos += (uint32_t)n;

        if (n == size) {
            return n;
        }

        if (ctx->mode == AVI_MODE_STREAMING && ctx->avi == NULL) {
            return n;
        }

        size_t file_n = ctx->ops->read((char *)ptr + n, 1, size - n, ctx->file);
        ctx->file_pos += (uint32_t)file_n;
        return n + file_n;
    }

    if (ctx->mode == AVI_MODE_STREAMING && ctx->avi == NULL) {
        return 0;
    }

    n = ctx->ops->read(ptr, 1, size, ctx->file);
    ctx->file_pos += (uint32_t)n;
    return n;
}

// 定位文件并同步 file_pos
static inline int avi_seek(avi_ctx_t *ctx, uint32_t offset)
{
    if (ctx->hdr && offset <= ctx->hdr_len) {
        ctx->hdr_pos = offset;
        ctx->file_pos = offset;
        return 0;
    }

    if (ctx->hdr && ctx->mode == AVI_MODE_STREAMING && ctx->avi == NULL) {
        avi_err("hdrl parse: seek %u beyond preload %u, abort!\r\n", offset, ctx->hdr_len);
        return -1;
    }

    int r = ctx->ops->seek(ctx->file, (off_t)offset, SEEK_SET);

    if (ctx->hdr == NULL) {
        ctx->file_pos = offset;
        return 0;
    }

    if (r == 0) {
        ctx->file_pos = offset;
        ctx->hdr_pos = ctx->hdr_len;
    }
    return r;
}

// 循环读取直到凑够 size 字节
static int avi_read_exact(avi_ctx_t *ctx, void *ptr, size_t size)
{
    uint8_t *p = ptr;
    size_t total = 0;
    while (total < size) {
        size_t n = avi_read(ctx, p + total, size - total);
        if (n == 0) {
            if (ctx->ops->eof && ctx->ops->eof(ctx->file)) return -1;
            return 0;
        }
        total += n;
    }
    return total;
}

// 跳过指定字节
static int avi_skip_bytes(avi_ctx_t *ctx, uint32_t nbytes)
{
    uint8_t tmp[256];
    while (nbytes > 0) {
        size_t n = avi_read(ctx, tmp, nbytes > sizeof(tmp) ? sizeof(tmp) : nbytes);
        if (n == 0) return -1;
        nbytes -= (uint32_t)n;
    }
    return 0;
}

// 计算音频每毫秒字节数
static inline uint32_t avi_audio_bpm(const avi_file_t *avi)
{
    if (!avi->sample_rate || !avi->channels || !avi->bits)
        return 0;
    return avi->sample_rate * avi->channels * (avi->bits / 8) / 1000;
}

// 查询当前可读数据长度
static inline uint32_t avi_avail(avi_ctx_t *ctx)
{
    uint32_t hdr_avail = (ctx->hdr && ctx->hdr_pos < ctx->hdr_len)
                         ? (ctx->hdr_len - ctx->hdr_pos) : 0;
    uint32_t file_avail = (ctx->ops->avail) ? (uint32_t)ctx->ops->avail(ctx->file) : 0;
    return hdr_avail + file_avail;
}

// 释放快速索引链表
static void avi_fast_idx_free(avi_fast_idx_t *head)
{
    while (head) {
        avi_fast_idx_t *tmp = head;
        head = head->next;
        decoder_mem_free(tmp);
    }
}

// 追加快速索引节点到链表尾部
static avi_fast_idx_t *avi_fast_idx_append(avi_fast_idx_t *head,
                                           uint32_t frame_cnt,
                                           uint32_t audio_bytes,
                                           uint32_t offset)
{
    avi_fast_idx_t *node = decoder_mem_alloc(sizeof(*node));
    if (!node) return head;

    node->next = NULL;
    node->frame_cnt = frame_cnt;
    node->audio_bytes = audio_bytes;
    node->offset = offset;

    if (!head) {
        head = node;
    } else {
        avi_fast_idx_t *tail = head;
        while (tail->next) tail = tail->next;
        tail->next = node;
    }
    return head;
}

// 解析 strl LIST 中的 strh 和 strf chunk
static int avi_parse_strl(avi_ctx_t *ctx, avi_file_t *avi)
{
    avi_chunk_t str;
    avi_strh_t stream;
    avi_wavefmt_t wav;
    uint8_t track_type = 0; // 0=none, 1=video, 2=audio
    memset(&wav, 0, sizeof(wav));

    while (1) {
        if (avi_read(ctx, &str, sizeof(str)) < sizeof(str)) break;

        if (avi_fourcc(&str.fcc) == FOURCC('s','t','r','h')) {
            if (avi_read(ctx, &stream, sizeof(stream)) < sizeof(stream)) break;
            if (avi_fourcc(&stream.fccType) == FOURCC('v','i','d','s')) {
                avi->v_fcc = (avi->track_cnt == 0) ? FOURCC('0','0','d','c') : FOURCC('0','1','d','c');
                track_type = 1;
                avi->fps = stream.dwRate / stream.dwScale;
                avi->track_cnt++;

                // 识别视频编解码器
                avi->video_fcc = avi_fourcc(&stream.fccHandler);
                switch (avi->video_fcc) {
                    case FOURCC('H','2','6','4'):
                    case FOURCC('h','2','6','4'):
                    case FOURCC('a','v','c','1'):
                    case FOURCC('A','V','C','1'):
                    case FOURCC('X','2','6','4'):
                    case FOURCC('x','2','6','4'):
                        avi->video_codec = VIDEO_CODEC_H264;
                        break;
                    case FOURCC('M','J','P','G'):
                    case FOURCC('m','j','p','g'):
                    case FOURCC('J','P','E','G'):
                    case FOURCC('j','p','e','g'):
                    default:
                        avi->video_codec = VIDEO_CODEC_MJPEG;
                        break;
                }
            } else if (avi_fourcc(&stream.fccType) == FOURCC('a','u','d','s')) {
                avi->a_fcc = (avi->track_cnt == 0) ? FOURCC('0','0','w','b') : FOURCC('0','1','w','b');
                track_type = 2;
                avi->track_cnt++;
            }
            uint32_t chunk_end = ctx->file_pos - sizeof(stream) + str.size;
            if (avi_seek(ctx, chunk_end) != 0) return -1;
        } else if (avi_fourcc(&str.fcc) == FOURCC('s','t','r','f')) {
            uint32_t chunk_body_start = ctx->file_pos;
            if (track_type == 2) {
                uint32_t wav_read_size = (str.size < sizeof(wav)) ? str.size : sizeof(wav);
                size_t n = avi_read(ctx, &wav, wav_read_size);
                if (n >= wav_read_size) {
                    avi->sample_rate = wav.nSamplesPerSec;
                    avi->channels = wav.nChannels;
                    avi->bits = wav.wBitsPerSample;
                    switch (wav.wFormatTag) {
                        case 0x0001: avi->audio_codec = AUDIO_CODEC_PCM_S16LE; break;
                        case 0x0055: avi->audio_codec = AUDIO_CODEC_MP3;       break;
                        case 0x0006: avi->audio_codec = AUDIO_CODEC_ALAW;      break;
                        case 0x0007: avi->audio_codec = AUDIO_CODEC_ULAW;      break;
                        default:
                            avi->audio_codec = AUDIO_CODEC_INVALID;
                            avi_warn("avi: unsupported audio format 0x%04X\n", wav.wFormatTag);
                            break;
                    }
                    avi->avg_bytes_per_sec = wav.nAvgBytesPerSec;
                }
            }
            uint32_t chunk_end = chunk_body_start + str.size;
            if (avi_seek(ctx, chunk_end) != 0) return -1;
            break;
        } else {
            uint32_t skip = str.size;
            if (skip & 1) skip++;
            if (avi_seek(ctx, ctx->file_pos + skip) != 0) return -1;
            continue;
        }
    }
    return 0;
}

// 纯内存解析 hdrl，网络流 init 阶段使用
static avi_file_t *avi_parse_hdrl(avi_ctx_t *ctx)
{
    avi_file_t *avi = NULL;
    char data[16];
    size_t readlen;
    uint32_t chunk_size;
    uint32_t hdrl_start = 0;

    if (avi_seek(ctx, 0) != 0) {
        avi_dbg("avi: seek to head failed\n");
        return NULL;
    }

    readlen = avi_read(ctx, data, 12);
    if (readlen < 12) goto fail;
    if (avi_fourcc(data) != FOURCC('R','I','F','F') || avi_fourcc(data + 8) != FOURCC('A','V','I',' ')) {
        avi_dbg("avi: not RIFF/AVI\n");
        goto fail;
    }

    avi = decoder_mem_zalloc(sizeof(*avi));
    if (!avi) goto fail;

    while (1) {
        readlen = avi_read(ctx, data, 8);
        if (readlen < 8) goto fail;

        os_memcpy(&chunk_size, data + 4, 4);
        chunk_size = (chunk_size + 1) & (~1);

        if (avi_fourcc(data) == FOURCC('L','I','S','T')) {
            readlen = avi_read(ctx, data, 4);
            if (readlen < 4) goto fail;
            chunk_size -= 4;

            if (avi_fourcc(data) == FOURCC('h','d','r','l')) {
                hdrl_start = ctx->file_pos;
                readlen = avi_read(ctx, &avi->avih, sizeof(avi->avih));
                if (readlen < sizeof(avi->avih)) goto fail;

                while (ctx->file_pos < hdrl_start + chunk_size) {
                    avi_chunk_t ch;
                    if (avi_read(ctx, &ch, 8) < 8) break;

                    if (avi_fourcc(&ch.fcc) == FOURCC('L','I','S','T')) {
                        char sub_list[4];
                        if (avi_read(ctx, sub_list, 4) < 4) break;
                        if (avi_fourcc(sub_list) == FOURCC('s','t','r','l')) {
                            uint32_t strl_body_start = ctx->file_pos;
                            uint32_t strl_end = strl_body_start + ch.size - 4;
                            if (avi_parse_strl(ctx, avi) != 0) goto fail;
                            if (ctx->file_pos < strl_end) {
                                if (avi_skip_bytes(ctx, strl_end - ctx->file_pos) != 0) goto fail;
                            }
                        } else {
                            uint32_t skip = (ch.size - 4 + 1) & ~1;
                            if (avi_skip_bytes(ctx, skip) != 0) goto fail;
                        }
                    } else {
                        uint32_t skip = (ch.size + 1) & ~1;
                        if (avi_skip_bytes(ctx, skip) != 0) goto fail;
                    }
                }
            }
            else if (avi_fourcc(data) == FOURCC('m','o','v','i')) {
                avi->movi_off = ctx->file_pos - 4;
                avi_dbg("avi: hdrl parsed, movi_off=%u, file_pos=%u\n",
                        avi->movi_off, ctx->file_pos);
                return avi;
            }
            else {
                if (avi_skip_bytes(ctx, chunk_size) != 0) goto fail;
            }
        }
        else if (avi_fourcc(data) == FOURCC('J','U','N','K')) {
            if (avi_skip_bytes(ctx, chunk_size) != 0) goto fail;
        }
        else {
            if (avi_skip_bytes(ctx, chunk_size) != 0) goto fail;
        }
    }

fail:
    if (avi) decoder_mem_free(avi);
    return NULL;
}

// 本地文件完整解析，含 idx1
static avi_file_t *avi_parse_file(avi_ctx_t *ctx)
{
    avi_file_t *avi = NULL;
    char data[16];
    size_t readlen;
    uint32_t chunk_size;
    uint32_t hdrl_start = 0;
    int parse_ok = 0;

    if (ctx->hdr) {
        decoder_mem_free(ctx->hdr);
        ctx->hdr = NULL;
        ctx->hdr_len = 0;
        ctx->hdr_pos = 0;
    }

    ctx->ops->seek(ctx->file, 0, SEEK_SET);
    ctx->file_pos = 0;

    readlen = avi_read(ctx, data, 12);
    if (readlen < 12) goto fail;
    if (avi_fourcc(data) != FOURCC('R','I','F','F') || avi_fourcc(data + 8) != FOURCC('A','V','I',' ')) {
        goto fail;
    }

    avi = decoder_mem_zalloc(sizeof(*avi));
    if (!avi) goto fail;

    while (1) {
        readlen = avi_read(ctx, data, 8);
        if (readlen < 8) {
            if (readlen > 0) goto fail;
            parse_ok = 1;
            break;
        }

        os_memcpy(&chunk_size, data + 4, 4);
        chunk_size = (chunk_size + 1) & (~1);

        if (avi_fourcc(data) == FOURCC('L','I','S','T')) {
            readlen = avi_read(ctx, data, 4);
            if (readlen < 4) goto fail;
            chunk_size -= 4;

            if (avi_fourcc(data) == FOURCC('h','d','r','l')) {
                hdrl_start = ctx->file_pos;
                avi_seek(ctx, ctx->file_pos + chunk_size);
            } else if (avi_fourcc(data) == FOURCC('I','N','F','O')) {
                avi_seek(ctx, ctx->file_pos + chunk_size);
            } else if (avi_fourcc(data) == FOURCC('m','o','v','i')) {
                avi->movi_off = ctx->file_pos - 4;
                avi_seek(ctx, ctx->file_pos + chunk_size);
            } else {
                goto fail;
            }
        } else if (avi_fourcc(data) == FOURCC('J','U','N','K')) {
            avi_seek(ctx, ctx->file_pos + chunk_size);
        } else if (avi_fourcc(data) == FOURCC('i','d','x','1')) {
            avi->idx1_off = ctx->file_pos;
            avi->idx1_len = chunk_size;
            avi_seek(ctx, ctx->file_pos + chunk_size);
        } else {
            goto fail;
        }
    }

    if (!parse_ok || hdrl_start == 0 || avi->idx1_off == 0) {
        goto fail;
    }

    avi_seek(ctx, hdrl_start);
    readlen = avi_read(ctx, &avi->avih, sizeof(avi->avih));
    if (readlen < sizeof(avi->avih)) goto fail;

    while (1) {
        avi_chunk_t list_hdr;
        readlen = avi_read(ctx, &list_hdr, sizeof(list_hdr));
        if (readlen < sizeof(list_hdr)) break;

        if (avi_fourcc(&list_hdr.fcc) == FOURCC('L','I','S','T')) {
            char list_type[4];
            readlen = avi_read(ctx, list_type, 4);
            if (readlen < 4) break;

            if (avi_fourcc(list_type) == FOURCC('s','t','r','l')) {
                uint32_t strl_list_start = ctx->file_pos - 12;
                if (avi_parse_strl(ctx, avi) != 0) goto fail;
                uint32_t list_end = strl_list_start + 8 + list_hdr.size;
                avi_seek(ctx, list_end);
            } else {
                break;
            }
        } else {
            break;
        }
    }

    avi->v_frame_cnt = 0;
    avi->v_idx_pos = avi->idx1_off;
    avi->v_idx_left = avi->idx1_len;
    avi->v_idx_cached = 0;

    avi->a_byte_cnt = 0;
    avi->a_idx_pos = avi->idx1_off;
    avi->a_idx_left = avi->idx1_len;
    avi->a_idx_cached = 0;

    return avi;

fail:
    if (avi) {
        if (avi->fast_idx) avi_fast_idx_free(avi->fast_idx);
        decoder_mem_free(avi);
    }
    return NULL;
}

// 从 idx1 读取下一个视频帧索引
static void avi_idx1_next_video(avi_ctx_t *ctx, uint32_t *offset,
                                uint32_t *size, int32_t *frame_num)
{
    avi_file_t *avi = ctx->avi;

    while (1) {
        if (avi->v_idx_cached > 0) {
            if (avi_fourcc(&avi->v_idx_ptr->dwChunkId) == avi->v_fcc && avi->v_idx_ptr->dwFlags) {
                *offset = avi->v_idx_ptr->dwOffset + avi->movi_off + 8;
                *size   = avi->v_idx_ptr->dwSize;
                avi->v_frame_cnt++;
                avi->v_idx_ptr++;
                avi->v_idx_cached -= sizeof(avi_idx1_t);
                if (*frame_num <= avi->v_frame_cnt) {
                    *frame_num = avi->v_frame_cnt;
                    return;
                }
                continue;
            }
            avi->v_idx_ptr++;
            avi->v_idx_cached -= sizeof(avi_idx1_t);
            continue;
        }

        if (avi->v_idx_left == 0) {
            *offset = 0;
            *size = 0;
            return;
        }
        uint32_t read_size = (avi->v_idx_left > IDX1_READ_SIZE) ? IDX1_READ_SIZE : avi->v_idx_left;

        ctx->ops->seek(ctx->file, (off_t)avi->v_idx_pos, SEEK_SET);
        ctx->file_pos = avi->v_idx_pos;
        size_t n = ctx->ops->read(avi->v_idx_buf, 1, read_size, ctx->file);
        ctx->file_pos += (uint32_t)n;

        avi->v_idx_left -= (uint32_t)n;
        avi->v_idx_cached = (int32_t)n;
        avi->v_idx_ptr = (avi_idx1_t *)avi->v_idx_buf;
        avi->v_idx_pos += (uint32_t)n;
    }
}

// 从 idx1 读取下一个音频 chunk 索引
static void avi_idx1_next_audio(avi_ctx_t *ctx, uint32_t *offset,
                                uint32_t *size, int32_t *byte_total)
{
    avi_file_t *avi = ctx->avi;

    while (1) {
        if (avi->a_idx_cached > 0) {
            if (avi_fourcc(&avi->a_idx_ptr->dwChunkId) != avi->a_fcc) {
                avi->a_idx_ptr++;
                avi->a_idx_cached -= sizeof(avi_idx1_t);
                continue;
            }
            *offset = avi->a_idx_ptr->dwOffset + avi->movi_off + 8;
            *size   = avi->a_idx_ptr->dwSize;

            avi->a_byte_cnt += (int32_t)(*size);
            avi->a_idx_ptr++;
            avi->a_idx_cached -= sizeof(avi_idx1_t);
            if (*byte_total > avi->a_byte_cnt) {
                continue;
            }
            *byte_total = avi->a_byte_cnt;
            return;
        }

        if (avi->a_idx_left == 0) {
            *offset = 0;
            *size = 0;
            return;
        }
        uint32_t read_size = (avi->a_idx_left > IDX1_READ_SIZE) ? IDX1_READ_SIZE : avi->a_idx_left;

        ctx->ops->seek(ctx->file, (off_t)avi->a_idx_pos, SEEK_SET);
        ctx->file_pos = avi->a_idx_pos;
        size_t n = ctx->ops->read(avi->a_idx_buf, 1, read_size, ctx->file);
        ctx->file_pos += (uint32_t)n;

        avi->a_idx_left -= (uint32_t)n;
        avi->a_idx_cached = (int32_t)n;
        avi->a_idx_ptr = (avi_idx1_t *)avi->a_idx_buf;
        avi->a_idx_pos += (uint32_t)n;
    }
}

// 构建快速索引链表，首次 seek 时调用
static int32_t avi_fast_idx_build(avi_ctx_t *ctx)
{
    avi_file_t *avi = ctx->avi;
    uint8_t buf[IDX1_READ_SIZE + sizeof(avi_idx1_t)];
    uint32_t buf_have = 0;
    uint32_t idx1_offset = avi->idx1_off;
    uint32_t remain = avi->idx1_len;
    uint32_t video_frame_count = 0;
    uint32_t audio_byte_count = 0;

    avi_seek(ctx, avi->idx1_off);

    while (remain > 0 || buf_have >= sizeof(avi_idx1_t)) {
        if (buf_have < sizeof(avi_idx1_t) && remain > 0) {
            uint32_t to_read = (remain > IDX1_READ_SIZE) ? IDX1_READ_SIZE : remain;
            size_t n = avi_read(ctx, buf + buf_have, to_read);
            buf_have += (uint32_t)n;
            remain -= (uint32_t)n;
            if (n == 0) break;
        }

        uint32_t n = buf_have / sizeof(avi_idx1_t);
        avi_idx1_t *entry = (avi_idx1_t *)buf;
        for (uint32_t i = 0; i < n; i++) {
            if (avi_fourcc(&entry[i].dwChunkId) == avi->v_fcc && entry[i].dwFlags) {
                video_frame_count++;
                if (video_frame_count % FAST_INDEX_STEP == 0) {
                    avi->fast_idx = avi_fast_idx_append(avi->fast_idx,
                            video_frame_count, audio_byte_count,
                            idx1_offset + i * sizeof(avi_idx1_t));
                }
            } else if (avi_fourcc(&entry[i].dwChunkId) == avi->a_fcc) {
                audio_byte_count += entry[i].dwSize;
            }
        }

        uint32_t consumed = n * sizeof(avi_idx1_t);
        buf_have -= consumed;
        idx1_offset += consumed;

        if (buf_have > 0)
            memmove(buf, buf + consumed, buf_have);
    }

    return 0;
}

// 通过快速索引定位到目标帧
static void avi_fast_idx_seek(avi_file_t *avi, uint32_t index)
{
    if (index == 0) {
        avi->v_frame_cnt = 0;
        avi->v_idx_cached = 0;
        avi->v_idx_left = avi->idx1_len;
        avi->v_idx_pos = avi->idx1_off;

        avi->a_byte_cnt = 0;
        avi->a_idx_cached = 0;
        avi->a_idx_pos = avi->idx1_off;
        avi->a_idx_left = avi->idx1_len;
        return;
    }

    avi_fast_idx_t *last = NULL;
    for (avi_fast_idx_t *p = avi->fast_idx; p; p = p->next) {
        if (p->frame_cnt > index) break;
        if (p->frame_cnt == index) {
            last = p;
            break;
        }
        last = p;
    }

    if (last) {
        avi->v_frame_cnt = (int32_t)last->frame_cnt;
        avi->v_idx_cached = 0;
        avi->v_idx_left = avi->idx1_len - (last->offset - avi->idx1_off);
        avi->v_idx_pos = last->offset;
    } else {
        avi->v_frame_cnt = 0;
        avi->v_idx_cached = 0;
        avi->v_idx_left = avi->idx1_len;
        avi->v_idx_pos = avi->idx1_off;

        avi->a_byte_cnt = 0;
        avi->a_idx_cached = 0;
        avi->a_idx_pos = avi->idx1_off;
        avi->a_idx_left = avi->idx1_len;
    }

    if (!avi->sample_rate) return;

    uint32_t bps = avi->channels * (avi->bits / 8);
    uint32_t video_time = index * 1000 / avi->fps;
    uint32_t audio_offset = video_time * avi->sample_rate * bps / 1000;

    last = NULL;
    for (avi_fast_idx_t *p = avi->fast_idx; p; p = p->next) {
        if (p->audio_bytes > audio_offset) break;
        if (p->audio_bytes == audio_offset) {
            last = p;
            break;
        }
        last = p;
    }

    if (last) {
        avi->a_byte_cnt = (int32_t)last->audio_bytes;
        avi->a_idx_cached = 0;
        avi->a_idx_left = avi->idx1_len - (last->offset - avi->idx1_off);
        avi->a_idx_pos = last->offset;
    } else {
        avi->a_byte_cnt = 0;
        avi->a_idx_cached = 0;
        avi->a_idx_pos = avi->idx1_off;
        avi->a_idx_left = avi->idx1_len;
    }
}

// 解复用一帧视频
static int32_t avi_demux_video(avi_ctx_t *ctx)
{
    uint32_t offset, size;
    int32_t  frame_num;

    if (ctx->pend_v_size > 0) {
        offset    = ctx->pend_v_off;
        size      = ctx->pend_v_size;
        frame_num = ctx->pend_v_frame;
    } else {
        if (ctx->seek_frame >= 0) {
            frame_num = ctx->seek_frame;
            ctx->seek_frame = -1;
        } else {
            frame_num = ctx->avi->v_frame_cnt;
        }
        avi_idx1_next_video(ctx, &offset, &size, &frame_num);
        if (!size) {
            ctx->v_time = ~0U;
            return 0;
        }
    }

    struct framebuff *fb = msi_alloc_fb(ctx->owner, NULL, NULL, size, 0, 0);
    if (!fb) {
        if (ctx->pend_v_size == 0) {
            ctx->pend_v_off   = offset;
            ctx->pend_v_size  = size;
            ctx->pend_v_frame = frame_num;
        }
        return -EAGAIN;
    }

    ctx->pend_v_size = 0;

    ctx->ops->seek(ctx->file, (off_t)offset, SEEK_SET);
    ctx->file_pos = offset;
    size_t n = ctx->ops->read(fb->data, 1, size, ctx->file);
    ctx->file_pos += (uint32_t)n;
    sys_dcache_clean_range((uint32_t *)fb->data, (int32_t)n);

    fb->mtype = MEDIA_DATA_VIDEO;
    fb->stype = ctx->avi->video_codec ? ctx->avi->video_codec : VIDEO_CODEC_MJPEG;
    fb->time  = (uint32_t)frame_num * 1000 / ctx->avi->fps;
    fb->len   = (uint32_t)n;
    fb->priv  = &ctx->v_info;
    fb->codec_info = &ctx->v_info;

    ctx->v_time = fb->time;

    avi_dbg("[V] send frame=%d time=%u size=%u\r\n", frame_num, fb->time, fb->len);

    ctx->ops->outFB(ctx->owner, fb);
    return (int32_t)n;
}

// 解复用一帧音频
static int32_t avi_demux_audio(avi_ctx_t *ctx)
{
    uint32_t offset, size;

    if (!ctx->avi->sample_rate) {
        ctx->a_time = ~0U;
        return 0;
    }

    if (ctx->pend_a_size > 0) {
        offset = ctx->pend_a_off;
        size   = ctx->pend_a_size;
    } else if (ctx->a_chunk_left > 0) {
        offset = ctx->a_chunk_pos;
        size   = ctx->a_chunk_left;
    } else {
        int32_t byte_total = (int32_t)ctx->a_out_bytes;
        avi_idx1_next_audio(ctx, &offset, &size, &byte_total);
        if (!size) {
            ctx->a_time = ~0U;
            return 0;
        }
        ctx->a_chunk_pos = offset;
    }

    uint32_t chunk = (size > AUDIO_TMP_BUF_SIZE) ? AUDIO_TMP_BUF_SIZE : size;

    struct framebuff *fb = msi_alloc_fb(ctx->owner, NULL, NULL, chunk, 0, 0);
    if (!fb) {
        if (ctx->a_chunk_left == 0 && ctx->pend_a_size == 0) {
            ctx->pend_a_off  = offset;
            ctx->pend_a_size = size;
        }
        return -EAGAIN;
    }

    ctx->pend_a_size = 0;

    ctx->ops->seek(ctx->file, (off_t)offset, SEEK_SET);
    ctx->file_pos = offset;

    size_t read_len = ctx->ops->read(fb->data, 1, chunk, ctx->file);
    ctx->file_pos += (uint32_t)read_len;

    if (read_len == 0) {
        fb_put(fb);
        ctx->a_time = ~0U;
        return 0;
    }

    if (read_len < chunk && ctx->ops->eof && ctx->ops->eof(ctx->file)) {
        fb_put(fb);
        ctx->a_time = ~0U;
        return 0;
    }

    fb->len = (uint32_t)read_len;

    if (ctx->avi->avg_bytes_per_sec) {
        fb->time = ctx->a_out_bytes * 1000 / ctx->avi->avg_bytes_per_sec;
    } else {
        fb->time = ctx->a_out_bytes * 500 / ctx->avi->sample_rate;
    }

    if (ctx->seek_sync > 0) {
        ctx->seek_sync--;
        if (ctx->v_time != ~0U && fb->time + 200 < ctx->v_time) {
            avi_warn("avi: sync audio %u -> %u\n", fb->time, ctx->v_time);
            fb->time = ctx->v_time;
        }
    }

    //fb->priv = &ctx->a_info;
    fb->codec_info = &ctx->a_info;

    fb->mtype = MEDIA_DATA_AUDIO;
    fb->stype = ctx->avi->audio_codec;
    avi_dbg("avi_demux_audio: send chunk=%d time=%d\r\n", fb->len, fb->time);

    // 只累加原始音频数据长度
    ctx->a_out_bytes += (uint32_t)read_len;
    if (ctx->avi->avg_bytes_per_sec) {
        ctx->a_time = ctx->a_out_bytes * 1000 / ctx->avi->avg_bytes_per_sec;
    } else {
        ctx->a_time = ctx->a_out_bytes * 500 / ctx->avi->sample_rate;
    }

    // 用原始读取长度计算剩余分片
    if (read_len < chunk) {
        ctx->a_chunk_left = chunk - (uint32_t)read_len;
        ctx->a_chunk_pos  = offset + (uint32_t)read_len;
    } else {
        ctx->a_chunk_left = 0;
        ctx->a_chunk_pos  = 0;
    }

    ctx->ops->outFB(ctx->owner, fb);
    return (int32_t)fb->len;
}

// 网络流音频分片续传
static int32_t avi_streaming_audio(avi_ctx_t *ctx)
{
    avi_file_t *avi = ctx->avi;
    uint32_t chunk = ctx->a_chunk_left > AUDIO_TMP_BUF_SIZE ?
                     AUDIO_TMP_BUF_SIZE : ctx->a_chunk_left;

    if (avi_avail(ctx) < chunk)
        return 0;

    struct framebuff *fb = msi_alloc_fb(ctx->owner, NULL, NULL, chunk, 0, 0);
    if (!fb) return 0;

    int read_n = avi_read_exact(ctx, fb->data, chunk);
    if (read_n <= 0) {
        fb_put(fb);
        return (read_n == 0) ? 0 : -EAGAIN;
    }
    if ((uint32_t)read_n < chunk && ctx->ops->eof && ctx->ops->eof(ctx->file)) {
        fb_put(fb);
        return 0;
    }

    uint32_t raw_len = (uint32_t)read_n;
    fb->len = raw_len;

    if (avi->avg_bytes_per_sec) {
        fb->time = ctx->a_out_bytes * 1000 / avi->avg_bytes_per_sec;
    } else {
        fb->time = ctx->a_out_bytes * 500 / avi->sample_rate;
    }

    /* seek 后音频分片续传同样强制对齐 */
    if (ctx->seek_sync > 0) {
        ctx->seek_sync--;
        if (ctx->v_time != ~0U && fb->time + 200 < ctx->v_time) {
            avi_warn("avi: sync audio(cont) %u -> %u\n", fb->time, ctx->v_time);
            fb->time = ctx->v_time;
        }
    }

    //fb->priv = &ctx->a_info;
    fb->codec_info = &ctx->a_info;
    fb->mtype = MEDIA_DATA_AUDIO;
    fb->stype = avi->audio_codec;

    ctx->a_out_bytes += raw_len;
    ctx->a_time = fb->time;   /* 对齐后使用实际时间戳 */

    if (raw_len < ctx->a_chunk_left) {
        ctx->a_chunk_left -= raw_len;
        ctx->a_chunk_pos  += raw_len;
    } else {
        ctx->a_chunk_left = 0;
    }

    if (ctx->a_chunk_left == 0) {
        avi->scan_pos = ctx->a_chunk_next;
        ctx->a_chunk_next = 0;
        ctx->wait_audio = 0;   /* 音频分片完成，允许视频输出 */
    } else {
        avi->scan_pos = ctx->a_chunk_pos;
    }

    ctx->ops->outFB(ctx->owner, fb);
    return (int32_t)fb->len;
}

// 网络流读取 chunk 头
static int32_t avi_streaming_header(avi_ctx_t *ctx, avi_chunk_t *ch, uint32_t *data_off)
{
    avi_file_t *avi = ctx->avi;

    if (avi->pend_flag) {
        *ch = avi->pend_chunk;
        *data_off = avi->pend_off;
        if (ch->size == 0 || ch->size > AVI_MAX_CHUNK_SIZE) {
            avi->pend_flag = 0;
        } else {
            return 1;
        }
    }

    if (avi_avail(ctx) < sizeof(*ch))
        return 0;

    /* 滑字节扫描：在可读数据中寻找有效的 chunk FourCC */
    uint8_t scan_buf[256];
    uint32_t avail = avi_avail(ctx);
    uint32_t to_read = (avail > sizeof(scan_buf)) ? sizeof(scan_buf) : avail;
    uint32_t start_pos = ctx->file_pos;

    size_t n = avi_read(ctx, scan_buf, to_read);
    if (n < sizeof(*ch))
        return 0;

    uint32_t found_idx = 0;
    uint8_t found = 0;

    for (uint32_t i = 0; i + sizeof(*ch) <= n; i++) {
        uint32_t fcc = avi_fourcc(scan_buf + i);
        uint32_t sz  = scan_buf[i+4] | (scan_buf[i+5] << 8) |
                       (scan_buf[i+6] << 16) | (scan_buf[i+7] << 24);

        if (sz > AVI_MAX_CHUNK_SIZE)
            continue;

        if (fcc == avi->v_fcc || fcc == avi->a_fcc ||
            fcc == FOURCC('L','I','S','T') || fcc == FOURCC('J','U','N','K') ||
            fcc == FOURCC('i','d','x','1') || fcc == FOURCC('R','I','F','F')) {
            found_idx = i;
            found = 1;
            break;
        }
    }

    if (!found) {
        /* 无同步点，丢弃这 256 字节，等待新数据 */
        avi->scan_pos = ctx->file_pos;
        return 0;
    }

    os_memcpy(ch, scan_buf + found_idx, sizeof(*ch));
    *data_off = start_pos + found_idx + sizeof(*ch);

    /* 将 chunk 头之后的数据塞回 hdr 缓冲，供后续 avi_read 无缝衔接 */
    uint32_t body_start = found_idx + sizeof(*ch);
    uint32_t remain = (uint32_t)n - body_start;

    if (remain > 0) {
        if (ctx->hdr) decoder_mem_free(ctx->hdr);
        ctx->hdr = (uint8_t *)decoder_mem_alloc(remain);
        if (ctx->hdr) {
            os_memcpy(ctx->hdr, scan_buf + body_start, remain);
            ctx->hdr_len = remain;
            ctx->hdr_pos = 0;
        }
    } else {
        if (ctx->hdr) {
            decoder_mem_free(ctx->hdr);
            ctx->hdr = NULL;
            ctx->hdr_len = 0;
            ctx->hdr_pos = 0;
        }
    }

    ctx->file_pos = start_pos + found_idx + sizeof(*ch);

    if (avi_fourcc(&ch->fcc) == FOURCC('L','I','S','T')) {
        if (avi_avail(ctx) < 4) return 0;
        uint8_t tmp[4];
        int ret = avi_read_exact(ctx, tmp, 4);
        if (ret <= 0)
            return (ret == 0) ? 0 : -EAGAIN;
        avi->scan_pos = ctx->file_pos;
        return 0;
    }

    avi->pend_chunk = *ch;
    avi->pend_flag = 1;
    avi->pend_off = *data_off;
    avi_warn("[HDR] fcc=%.4s sz=%u avail=%u pend=%d\n",
             (char*)&ch->fcc, ch->size, avi_avail(ctx), avi->pend_flag);
    return 1;
}

// 网络流逐 chunk 解复用
static int32_t avi_demux_streaming(avi_ctx_t *ctx)
{
    avi_file_t *avi = ctx->avi;
    avi_chunk_t ch;
    uint32_t data_off;
    int is_video, is_audio;

    if (ctx->a_chunk_left > 0)
        return avi_streaming_audio(ctx);

    int ret = avi_streaming_header(ctx, &ch, &data_off);
    if (ret > 0) {
        avi_warn("[DEMUX] chunk=%.4s v=%u a=%u outv=%u avail=%u\n",
                 (char*)&ch.fcc, ctx->v_time, ctx->a_time, avi->out_vframe, avi_avail(ctx));
    }
    if (ret <= 0) return ret;

    if (ch.size == 0) {
        avi_err("avi: zero-size chunk %.4s at %u, skip 8 bytes\n",
                (char*)&ch.fcc, data_off - 8);
        avi->scan_pos = data_off;
        avi->pend_flag = 0;
        return 0;
    }

    uint32_t padded = (ch.size + 1) & ~1;
    uint32_t next_off = data_off + padded;

    is_video = (avi_fourcc(&ch.fcc) == avi->v_fcc);
    is_audio = (avi_fourcc(&ch.fcc) == avi->a_fcc);

    /* seek 后强制先找到音频 chunk，避免视频挂起导致死锁 */
    if (is_video && ctx->wait_audio && avi->sample_rate) {
        uint32_t skip = padded;
        while (skip > 0) {
            uint8_t tmp[256];
            size_t n = avi_read(ctx, tmp, skip > sizeof(tmp) ? sizeof(tmp) : skip);
            if (n == 0) return 0;
            skip -= (uint32_t)n;
        }
        avi->scan_pos = next_off;
        avi->pend_flag = 0;
        avi_dbg("avi: wait_audio, skip video chunk %u bytes\n", padded);
        return 0;
    }

    uint8_t video_skip = 0;
    if (ctx->tick_base > 0 && is_video && avi->fps > 0) {
        uint32_t elapsed = os_jiffies() - ctx->tick_base;
        uint32_t expect = avi->out_vframe * 1000 / avi->fps;
        if (expect > elapsed + 30) {
            if (expect > elapsed + 2000) {
                avi_warn("avi: time base reset, expect=%u elapsed=%u\n", expect, elapsed);
                avi->out_vframe = ctx->v_time * avi->fps / 1000;
                ctx->tick_base = os_jiffies() - ctx->v_time;
            } else {
                avi_warn("[VLIM] skip video f=%u expect=%u elapsed=%u\n",
                         avi->out_vframe, expect, elapsed);
                video_skip = 1;
            }
        }
    }
    if (!video_skip && is_video && avi->sample_rate && ctx->a_time != ~0U) {
        if (ctx->v_time > ctx->a_time + 500) {
            video_skip = 1;
        }
    }

    if (video_skip && ctx->mode == AVI_MODE_STREAMING) {
        uint32_t skip = padded;
        while (skip > 0) {
            uint8_t tmp[256];
            size_t n = avi_read(ctx, tmp, skip > sizeof(tmp) ? sizeof(tmp) : skip);
            if (n == 0) return 0;
            skip -= (uint32_t)n;
        }
        avi->scan_pos = next_off;
        avi->pend_flag = 0;
        avi_dbg("avi: skip video chunk, v=%u a=%u\n", ctx->v_time, ctx->a_time);
        return 0;
    }

    if (!is_video && !is_audio) {
        if (avi_avail(ctx) < padded) return 0;
        uint32_t skip = padded;
        while (skip > 0) {
            uint8_t tmp[256];
            size_t n = avi_read(ctx, tmp, skip > sizeof(tmp) ? sizeof(tmp) : skip);
            if (n == 0) return 0;
            skip -= (uint32_t)n;
        }
        avi->scan_pos = next_off;
        avi->pend_flag = 0;
        return 0;
    }

    if (is_audio) {
        ctx->wait_audio = 0; 
    }

    uint32_t read_size = ch.size;
    if (is_audio && read_size > AUDIO_TMP_BUF_SIZE)
        read_size = AUDIO_TMP_BUF_SIZE;

    if (avi_avail(ctx) < read_size)
        return 0;

    struct framebuff *fb = msi_alloc_fb(ctx->owner, NULL, NULL, read_size, 0, 0);
    if (!fb) return 0;

    int n = avi_read_exact(ctx, fb->data, read_size);
    if (n <= 0) {
        fb_put(fb);
        return (n == 0) ? 0 : -EAGAIN;
    }
    if ((uint32_t)n < read_size && ctx->ops->eof && ctx->ops->eof(ctx->file)) {
        fb_put(fb);
        return 0;
    }

    uint32_t raw_len = (uint32_t)n;
    fb->len = raw_len;

    uint32_t remain_data = ch.size - raw_len;
    uint32_t remain_skip = padded - raw_len;

    if (is_audio && remain_data > 0) {
        ctx->a_chunk_left = remain_data;
        ctx->a_chunk_pos = data_off + raw_len;
        ctx->a_chunk_next = next_off;
    } else if (remain_skip > 0) {
        if (avi_avail(ctx) < remain_skip) {
            fb_put(fb);
            return 0;
        }
        uint8_t tmp[4];
        size_t ns = avi_read_exact(ctx, tmp, remain_skip);
        if (ns <= 0) {
            fb_put(fb);
            return (ns == 0) ? 0 : -EAGAIN;
        }
    }

    if (is_video) {
        if (ctx->seek_skip_video && ctx->wait_audio && ctx->seek_skip_cnt < 10) {
            ctx->seek_skip_cnt++;
            uint32_t skip = padded;
            while (skip > 0) {
                uint8_t tmp[256];
                size_t n = avi_read(ctx, tmp, skip > sizeof(tmp) ? sizeof(tmp) : skip);
                if (n == 0) return 0;
                skip -= (uint32_t)n;
            }
            avi->scan_pos = next_off;
            avi->pend_flag = 0;
            avi_warn("[VIDEO] seek skip #%u, scan=%u, avail=%u\n",
                     ctx->seek_skip_cnt, avi->scan_pos, avi_avail(ctx));
            fb_put(fb);
            return 0;
        }
        ctx->seek_skip_video = 0;
        ctx->seek_skip_cnt = 0;

        fb->mtype = MEDIA_DATA_VIDEO;
        fb->stype = avi->video_codec ? avi->video_codec : VIDEO_CODEC_MJPEG;
        fb->time  = avi->out_vframe * 1000 / avi->fps;
        //fb->priv  = &ctx->v_info;
        fb->codec_info = &ctx->v_info;

        if (fb->stype == VIDEO_CODEC_MJPEG) {
            fb->keyfrm = 1;
        }

        if (avi->seek_tab) {
            uint32_t idx = avi->out_vframe % avi->seek_cap;
            avi->seek_tab[idx] = data_off - 8;
        }

        avi->out_vframe++;
        ctx->v_time = fb->time;
        avi_warn("[VIDEO] send frame=%u time=%u\n", avi->out_vframe - 1, fb->time);
        avi_dbg("[AVI] VIDEO out frame=%u size=%u off=%u key=%d\n",
                avi->out_vframe - 1, fb->len, data_off - 8, fb->keyfrm);
    } else {
        fb->mtype = MEDIA_DATA_AUDIO;
        fb->stype = avi->audio_codec;
        if (avi->avg_bytes_per_sec) {
            fb->time = ctx->a_out_bytes * 1000 / avi->avg_bytes_per_sec;
        } else {
            fb->time = ctx->a_out_bytes * 500 / avi->sample_rate;
        }

        if (ctx->seek_sync > 0) {
            ctx->seek_sync--;
            if (ctx->v_time != ~0U && fb->time + 200 < ctx->v_time) {
                avi_warn("avi: sync audio %u -> %u\n", fb->time, ctx->v_time);
                fb->time = ctx->v_time;
            }
        }

        //fb->priv = &ctx->a_info;
        fb->codec_info = &ctx->a_info;

        ctx->a_out_bytes += raw_len;
        ctx->a_time = fb->time;  
        avi_warn("[AUDIO] send time=%u aout=%u\n", fb->time, ctx->a_out_bytes);
    }

    if (!(is_audio && remain_data > 0))
        avi->scan_pos = next_off;
    avi->pend_flag = 0;

    ctx->ops->outFB(ctx->owner, fb);
    return (int32_t)fb->len;
}

// 网络流首次 hdrl 解析与初始化
static int32_t avi_demux_streaming_init(avi_ctx_t *ctx)
{
    if (ctx->hdr_len < ctx->hdr_cap) {
        uint32_t chunk = ctx->hdr_cap - ctx->hdr_len;
        if (chunk > AVI_HDR_READ_CHUNK) chunk = AVI_HDR_READ_CHUNK;

        size_t n = ctx->ops->read(ctx->hdr + ctx->hdr_len, 1, chunk, ctx->file);
        if (n > 0) {
            ctx->hdr_len += (uint32_t)n;
            avi_dbg("avi: preload %u/%u bytes\n", ctx->hdr_len, ctx->hdr_cap);
        }
    }

    ctx->hdr_pos = 0;
    ctx->file_pos = 0;

    avi_file_t *avi = avi_parse_hdrl(ctx);
    if (!avi) {
        if (ctx->hdr_len >= ctx->hdr_cap) {
            avi_dbg("avi: hdrl fail, max size reached\n");
            ctx->v_time = ~0U;
            ctx->a_time = ~0U;
            return 0;
        }
        return -EAGAIN;
    }

    ctx->avi = avi;

    avi->idx1_off = 0;
    avi->idx1_len = 0;

    if (avi->fps > 0 && avi->avih.dwTotalFrame > 0) {
        ctx->duration = (uint32_t)((uint64_t)avi->avih.dwTotalFrame * 1000 / avi->fps);
    }

    ctx->v_time = 0;
    ctx->a_time = avi->sample_rate ? 0 : ~0U;
    ctx->a_out_bytes = 0;
    ctx->pend_v_size = 0;
    ctx->pend_v_off = 0;
    ctx->pend_v_frame = 0;
    ctx->pend_a_size = 0;
    ctx->pend_a_off = 0;
    ctx->a_chunk_left = 0;
    ctx->a_chunk_pos = 0;
    ctx->a_chunk_next = 0;
    ctx->seek_frame = -1;

    if (avi->fps) {
        ctx->v_info.codec_id = avi->video_codec ? avi->video_codec : VIDEO_CODEC_MJPEG;
        ctx->v_info.fps_num  = (uint16_t)avi->fps;
        ctx->v_info.fps_den  = 1;
        ctx->v_info.width    = (uint16_t)avi->avih.dwWidth;
        ctx->v_info.height   = (uint16_t)avi->avih.dwHeight;
    }
    if (avi->sample_rate) {
        ctx->a_info.codec_id    = avi->audio_codec;
        ctx->a_info.sample_rate = avi->sample_rate;
        ctx->a_info.channels    = avi->channels ? avi->channels : 2;
        ctx->a_info.bits_per_coded_sample = avi->bits ? avi->bits : 16;
        ctx->a_info.block_align = ctx->a_info.channels *
                                  (ctx->a_info.bits_per_coded_sample / 8);
    }

    avi->scan_pos = ctx->file_pos;
    avi->out_vframe = 0;
    avi->pend_flag = 0;

    ctx->tick_base = os_jiffies();

    if (avi->avih.dwTotalFrame > 0) {
        size_t sz = AVI_INDEX_WINDOW * sizeof(uint32_t);
        avi->seek_tab = (uint32_t *)decoder_mem_zalloc(sz);
        if (avi->seek_tab) {
            avi->seek_cap = AVI_INDEX_WINDOW;
        }
    }

    ctx->hdr_pos = ctx->file_pos;

    return 1;
}

// AVDemuxer do_demux 接口
static int32_t avi_demux(void *c)
{
    avi_ctx_t *ctx = c;
    if (!ctx || !ctx->ops) return -EINVAL;

    if (ctx->mode == AVI_MODE_STREAMING && ctx->avi == NULL) {
        int32_t ret = avi_demux_streaming_init(ctx);
        if (ret <= 0) return ret;
    }

    if (ctx->mode == AVI_MODE_STREAMING) {
        if (ctx->v_time == ~0U && ctx->a_time == ~0U)
            return 0;
        int32_t ret = avi_demux_streaming(ctx);
        if (ret == -EAGAIN && ctx->ops->eof(ctx->file)) {
            ctx->v_time = ~0U;
            ctx->a_time = ~0U;
            return 0;
        }
        return ret;
    }

    // INDEXED 模式
    if (!ctx->avi) return -EINVAL;

    if (ctx->v_time == ~0U && ctx->a_time == ~0U) {
        uint8_t buf[512];
        size_t n;
        do {
            n = ctx->ops->read(buf, 1, sizeof(buf), ctx->file);
        } while (n > 0);
        return 0;
    }

    if (ctx->v_time == ~0U) {
        int32_t ret = avi_demux_audio(ctx);
        if (ret == -EAGAIN) return -EAGAIN;
        return ret;
    }
    if (ctx->a_time == ~0U) {
        int32_t ret = avi_demux_video(ctx);
        if (ret == -EAGAIN) return -EAGAIN;
        return ret;
    }

    if (ctx->v_time <= ctx->a_time) {
        if (ctx->tick_base > 0 && ctx->avi->fps > 0) {
            uint32_t elapsed = os_jiffies() - ctx->tick_base;
            uint32_t expect = (uint32_t)ctx->avi->v_frame_cnt * 1000 / ctx->avi->fps;
            if (expect > elapsed + 30)
                return 0;
        }
        int32_t ret = avi_demux_video(ctx);
        if (ret == -EAGAIN) return 0;
        if (ret == 0 && ctx->avi->sample_rate) return -EAGAIN;
        return ret;
    } else {
        int32_t ret = avi_demux_audio(ctx);
        if (ret == -EAGAIN) return 0;
        if (ret == 0 && ctx->avi->fps) return -EAGAIN;
        return ret;
    }
}

// AVDemuxer init 接口
static void *avi_init(void *hdl, const struct AVDemuxerOps *ops,
                      void *hdr, uint32_t len, struct msi *owner)
{
    avi_ctx_t *ctx = decoder_mem_zalloc(sizeof(*ctx));
    if (!ctx) return NULL;

    ctx->owner = owner;
    ctx->ops   = ops;
    ctx->file  = hdl;

    int32_t stream_type = AVDEMUXER_STREAM_FILE;
    if (ops->ioctl) {
        stream_type = ops->ioctl(owner, AVDEMUXER_GET_STREAM_TYPE, 0, 0);
    }

    if (hdr && len > 0 && stream_type != AVDEMUXER_STREAM_FILE) {
        ctx->hdr = decoder_mem_alloc(len);
        if (ctx->hdr) {
            memcpy(ctx->hdr, hdr, len);
            ctx->hdr_len = len;
            ctx->hdr_pos = 0;
        }
    }

    if (stream_type == AVDEMUXER_STREAM_FILE) {
        ctx->mode = AVI_MODE_INDEXED;

        ctx->ops->seek(ctx->file, 0, SEEK_SET);
        ctx->file_pos = 0;

        ctx->avi = avi_parse_file(ctx);
        if (!ctx->avi) {
            avi_dbg("avi_init: avi_parse_file failed\n");
            decoder_mem_free(ctx);
            return NULL;
        }

        if (ctx->avi->fps > 0 && ctx->avi->avih.dwTotalFrame > 0) {
            ctx->duration = (uint32_t)((uint64_t)ctx->avi->avih.dwTotalFrame * 1000 / ctx->avi->fps);
        }
        ctx->v_time = 0;
        ctx->a_time = ctx->avi->sample_rate ? 0 : ~0U;
        ctx->a_out_bytes = 0;
        ctx->pend_v_size = 0;
        ctx->pend_v_off = 0;
        ctx->pend_v_frame = 0;
        ctx->pend_a_size = 0;
        ctx->pend_a_off = 0;
        ctx->a_chunk_left = 0;
        ctx->a_chunk_pos = 0;
        ctx->a_chunk_next = 0;
        ctx->seek_frame = -1;

        if (ctx->avi->idx1_off && ctx->avi->idx1_len) {
            avi_fast_idx_build(ctx);
        }

        if (ctx->avi->fps) {
            ctx->v_info.codec_id = ctx->avi->video_codec ? ctx->avi->video_codec : VIDEO_CODEC_MJPEG;
            ctx->v_info.fps_num  = (uint16_t)ctx->avi->fps;
            ctx->v_info.fps_den  = 1;
            ctx->v_info.width    = (uint16_t)ctx->avi->avih.dwWidth;
            ctx->v_info.height   = (uint16_t)ctx->avi->avih.dwHeight;
        }
        if (ctx->avi->sample_rate) {
            ctx->a_info.codec_id    = ctx->avi->audio_codec;
            ctx->a_info.sample_rate = ctx->avi->sample_rate;
            ctx->a_info.channels    = ctx->avi->channels ? ctx->avi->channels : 2;
            ctx->a_info.bits_per_coded_sample = ctx->avi->bits ? ctx->avi->bits : 16;
            ctx->a_info.block_align = ctx->a_info.channels *
                                      (ctx->a_info.bits_per_coded_sample / 8);
        }
        ctx->tick_base = os_jiffies();
        return ctx;
    }

    ctx->mode = AVI_MODE_STREAMING;

    if (ctx->hdr && ctx->hdr_len > 0 && ctx->hdr_len < AVI_HDR_MAX_SIZE) {
        uint8_t *new_hdr = decoder_mem_alloc(AVI_HDR_MAX_SIZE);
        if (new_hdr) {
            memcpy(new_hdr, ctx->hdr, ctx->hdr_len);
            decoder_mem_free(ctx->hdr);
            ctx->hdr = new_hdr;
            ctx->hdr_cap = AVI_HDR_MAX_SIZE;
        } else {
            ctx->hdr_cap = ctx->hdr_len;
        }
    } else if (ctx->hdr) {
        ctx->hdr_cap = ctx->hdr_len;
    } else {
        ctx->hdr = decoder_mem_alloc(AVI_HDR_MAX_SIZE);
        if (ctx->hdr) ctx->hdr_cap = AVI_HDR_MAX_SIZE;
    }

    avi_dbg("avi_init: STREAMING mode, deferred parse, hdr=%u/%u\n",
            ctx->hdr_len, ctx->hdr_cap);
    return ctx;
}

// AVDemuxer release 接口
static int32_t avi_release(void *c)
{
    avi_ctx_t *ctx = c;
    if (!ctx) return 0;

    if (ctx->avi) {
        if (ctx->avi->fast_idx) avi_fast_idx_free(ctx->avi->fast_idx);
        if (ctx->avi->seek_tab) decoder_mem_free(ctx->avi->seek_tab);
        decoder_mem_free(ctx->avi);
        ctx->avi = NULL;
    }
    if (ctx->hdr) {
        decoder_mem_free(ctx->hdr);
        ctx->hdr = NULL;
    }
    decoder_mem_free(ctx);
    return 0;
}

// 网络流 seek
static int32_t avi_seek_streaming(avi_ctx_t *ctx, uint32_t time_ms)
{
    avi_file_t *avi = ctx->avi;
    uint32_t target = 0;
    if (avi->fps > 0) {
        target = time_ms * avi->fps / 1000;
    }
    if (avi->avih.dwTotalFrame > 0 && target >= avi->avih.dwTotalFrame) {
        target = avi->avih.dwTotalFrame - 1;
    }

    uint32_t new_offset = avi->movi_off;
    uint8_t hit = 0;

    uint32_t total_output = avi->out_vframe;
    if (total_output > 0 && target < total_output && avi->seek_tab) {
        uint32_t window_start = (total_output > avi->seek_cap)
                              ? (total_output - avi->seek_cap)
                              : 0;
        if (target >= window_start) {
            uint32_t idx = target % avi->seek_cap;
            new_offset = avi->seek_tab[idx];
            hit = 1;
        }
    }

    if (!hit) {
        if (avi->avih.dwTotalFrame > 0) {
            int64_t total_size = 0;
            if (ctx->ops->ioctl) {
                ctx->ops->ioctl(ctx->owner, AVDEMUXER_GET_FILE_SIZE, (uint32_t)&total_size, 0);
            }
            if (total_size > (int64_t)avi->movi_off) {
                uint64_t movi_size = (uint64_t)(total_size - avi->movi_off);
                new_offset = avi->movi_off + (uint32_t)((uint64_t)target * movi_size / avi->avih.dwTotalFrame);
            } else {
                new_offset = avi->movi_off;
            }
        } else {
            new_offset = avi->movi_off;
        }
        avi_dbg("[SEEK] frame=%u miss window, estimate offset=%u\n", target, new_offset);
    }

    avi->scan_pos = new_offset;
    avi->out_vframe = target;  

    ctx->v_time = target * 1000 / avi->fps;
    ctx->seek_frame = -1;

    ctx->tick_base = os_jiffies() - ctx->v_time;
    ctx->seek_sync = 3;         

    ctx->pend_v_size = 0;
    ctx->pend_v_off = 0;
    ctx->pend_v_frame = 0;
    ctx->pend_a_size = 0;
    ctx->pend_a_off = 0;
    ctx->a_chunk_left = 0;
    ctx->a_chunk_pos = 0;
    ctx->a_chunk_next = 0;
    avi->pend_flag = 0;

    if (avi->sample_rate) {
        if (avi->avg_bytes_per_sec) {
            ctx->a_out_bytes = ctx->v_time * avi->avg_bytes_per_sec / 1000;
            ctx->a_time = ctx->a_out_bytes * 1000 / avi->avg_bytes_per_sec;
        } else {
            uint32_t bpm = avi_audio_bpm(avi);
            if (bpm) {
                ctx->a_out_bytes = ctx->v_time * bpm;
                ctx->a_time = ctx->a_out_bytes / bpm;
            } else {
                ctx->a_out_bytes = ctx->v_time * avi->sample_rate * 2 / 1000;
                ctx->a_time = ctx->a_out_bytes * 500 / avi->sample_rate;
            }
        }
    }

    if (avi_seek(ctx, new_offset) != 0)
        return -EAGAIN;

    ctx->seek_skip_video = 1;
    ctx->seek_skip_cnt = 0;
    ctx->wait_audio = 1;
    return 0;
}

// AVDemuxer do_seek 接口
static int32_t avi_do_seek(void *c, uint32_t time_ms)
{
    avi_ctx_t *ctx = c;
    if (!ctx || !ctx->avi) return -1;
    if (ctx->mode == AVI_MODE_STREAMING) {
        return avi_seek_streaming(ctx, time_ms);
    }

    uint32_t target_frame = 0;
    if (ctx->avi->fps > 0) {
        target_frame = time_ms * ctx->avi->fps / 1000;
    }
    if (ctx->avi->avih.dwTotalFrame > 0 && target_frame >= ctx->avi->avih.dwTotalFrame) {
        target_frame = ctx->avi->avih.dwTotalFrame - 1;
    }

    avi_fast_idx_seek(ctx->avi, target_frame);
	
    ctx->v_time = target_frame * 1000 / ctx->avi->fps;
    ctx->seek_frame = (int32_t)target_frame;

    if (ctx->avi->sample_rate) {
        if (ctx->avi->avg_bytes_per_sec) {
            ctx->a_out_bytes = ctx->v_time * ctx->avi->avg_bytes_per_sec / 1000;
            ctx->a_time = ctx->a_out_bytes * 1000 / ctx->avi->avg_bytes_per_sec;
        } else {
            uint32_t bpm = avi_audio_bpm(ctx->avi);
            if (bpm) {
                ctx->a_out_bytes = ctx->v_time * bpm;
                ctx->a_time = ctx->a_out_bytes / bpm;
            } else {
                ctx->a_out_bytes = ctx->v_time * ctx->avi->sample_rate * 2 / 1000;
                ctx->a_time = ctx->a_out_bytes * 500 / ctx->avi->sample_rate;
            }
        }
    }

    ctx->pend_v_size = 0;
    ctx->pend_v_off = 0;
    ctx->pend_v_frame = 0;
    ctx->pend_a_size = 0;
    ctx->pend_a_off = 0;
    ctx->a_chunk_left = 0;
    ctx->a_chunk_pos = 0;

    ctx->tick_base = os_jiffies() - ctx->v_time;
    ctx->seek_sync = 3;          
    return 0;
}

// AVDemuxer ioctl 接口
static int32_t avi_ioctl(void *c, uint32_t cmd, uint32_t p1, uint32_t p2)
{
    avi_ctx_t *ctx = c;
    (void)p2;

    switch (cmd) {
        case AVDEMUXER_GET_TOTAL_DURATION:
            if (p1) *(uint32_t *)p1 = ctx->duration;
            break;
        case AVDEMUXER_SET_PLAY_SPEED:
            break;
    }
    return 0;
}

__avdemuxer const struct AVDemuxer avi_demuxer = {
    .type     = MEDIA_CONTAINER_AVI,
    .name     = "avi_demuxer",
    .init     = avi_init,
    .release  = avi_release,
    .do_seek  = avi_do_seek,
    .do_demux = avi_demux,
    .ioctl    = avi_ioctl,
};