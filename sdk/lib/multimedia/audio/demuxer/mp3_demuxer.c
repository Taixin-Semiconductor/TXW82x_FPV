#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/multimedia/AVContainer.h"
#include "../audio_coder.h"

#define MP3_BUF_SIZE            4096  
#define MP3_MAX_FRAME           2880
#define MP3_XING_TOC_SIZE       100

#define MPEG_VER_1              3
#define MPEG_VER_2_5            0

typedef struct {
    uint32_t frames;
    uint32_t bytes;
    uint8_t  toc[MP3_XING_TOC_SIZE];
    bool     has_toc;
} mp3_xing_t;

typedef struct {
    uint8_t  data[MP3_BUF_SIZE];
    uint16_t len;
    uint16_t pos;
} mp3_buf_t;

typedef enum {
    MP3_PHASE_HEADER = 0,
    MP3_PHASE_BODY,
} mp3_phase_t;

/**
 * @brief MP3 Demuxer 上下文结构体
 * @note 管理 MP1/2/3 格式音频的解封装状态，支持 VBR/CBR
 */
typedef struct {
    struct msi            *owner;                   // 上层播放器实例
    const struct AVDemuxerOps *ops;                 // 文件操作接口
    void                  *file_handle;             // 文件/URL句柄
    mp3_buf_t              buffer;                  // 输入数据缓冲区
    txAudioInfo_t     codec_info;              // 编码信息
    uint8_t                mpeg_version;            // MPEG版本 (1/2/2.5)
    uint8_t                mpeg_layer;              // MPEG层 (1/2/3)
    uint16_t               frame_size;              // 当前帧大小
    uint32_t               samplerate;              // 采样率
    uint32_t               first_frame_offset;      // 第一帧偏移 (跳过ID3)
    mp3_xing_t             xing_info;               // Xing/VBRI标签信息
    uint8_t                is_vbr : 1;              // 是否VBR编码
    mp3_phase_t            parse_phase;             // 解析阶段
    uint32_t               stream_pos;              // 当前文件位置
    uint8_t                info_initialized : 1;    // 是否已初始化
    uint8_t                duration_precise : 1;    // 时长是否精确
    uint32_t               id3_skip_remaining;      // ID3标签剩余跳过字节
    uint8_t                retry_cnt;               // 重试计数
    uint32_t               total_duration;          // 总时长 (毫秒)
    int64_t                file_size;               // 文件总大小
    int32_t                stream_type;             // 流类型
    uint32_t               sample_count;            // 已采样帧数
    uint32_t               sample_total_bitrate;    // 采样bitrate总和
    uint64_t               current_time_us;         // 当前播放时间 (微秒)
} mp3_demuxer_t;

static const uint16_t bitrate_tab[2][4][16] = {
    {{0}, {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0}, {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0}, {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0}},
    {{0}, {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0}, {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0}, {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0}}
};
static const uint16_t samplerate_tab[4][3] = {{11025, 12000, 8000}, {0, 0, 0}, {22050, 24000, 16000}, {44100, 48000, 32000}};
static const uint16_t samples_tab[4][4] = {{0, 576, 1152, 384}, {0, 0, 0, 0}, {0, 576, 1152, 384}, {0, 1152, 1152, 384}};

static bool mp3_demuxer_ensure(mp3_demuxer_t *d, uint16_t need)
{
    uint16_t have;
    int32_t got;

    if (d->id3_skip_remaining > 0) {
        uint8_t *tmp = decoder_mem_alloc(256);
        if (!tmp) return false;

        while (d->id3_skip_remaining > 0) {
            uint32_t to_read = (d->id3_skip_remaining > 256) ? 256 : d->id3_skip_remaining;

            got = d->ops->read(tmp, 1, to_read, d->file_handle);
            if (got <= 0) {
                decoder_mem_free(tmp);
                return false;
            }
            d->id3_skip_remaining -= got;
            d->stream_pos += got;
        }
        decoder_mem_free(tmp);
    }

    have = d->buffer.len - d->buffer.pos;
    if (have >= need) {
        return true;
    }

    uint16_t space = MP3_BUF_SIZE - d->buffer.len;
    uint16_t want = need - have;

    if (d->stream_type != AVDEMUXER_STREAM_FILE) {
        uint16_t prefer = want * 2;
        if (prefer < 512) prefer = 512;
        if (prefer > space) prefer = space;
        if (prefer > want) want = prefer;
    } else {
        if (space >= 512 && want < 512) {
            want = 512;
        } else if (want > space) {
            want = space;
        }
    }

    if (d->buffer.len + want > MP3_BUF_SIZE) {
        uint16_t remain = have;
        if (remain > 0 && d->buffer.pos > 0) {
            memmove(d->buffer.data, d->buffer.data + d->buffer.pos, remain);
            d->buffer.len = remain;
            d->buffer.pos = 0;
        } else {
            return false;
        }
    }

    got = d->ops->read(d->buffer.data + d->buffer.len, 1, want, d->file_handle);
    if (got > 0) {
        d->buffer.len += got;
        d->stream_pos += got;
    }

    return (d->buffer.len - d->buffer.pos) >= need;
}

static bool mp3_demuxer_verify_header(uint8_t *hdr, uint16_t *frame_size,
                                      uint8_t *out_ver, uint8_t *out_layer, uint32_t *out_sr)
{
    if (hdr[0] != 0xFF || (hdr[1] & 0xE0) != 0xE0) {
        return false;
    }

    uint8_t ver    = (hdr[1] >> 3) & 0x3;
    uint8_t layer  = (hdr[1] >> 1) & 0x3;
    uint8_t br_idx = (hdr[2] >> 4) & 0xF;
    uint8_t sr_idx = (hdr[2] >> 2) & 0x3;

    if (ver == 1 || layer == 0 || br_idx == 0 || br_idx == 0xF || sr_idx == 3) {
        return false;
    }

    uint8_t ver_tab = (ver == MPEG_VER_1) ? 0 : 1;
    uint32_t bitrate = bitrate_tab[ver_tab][layer][br_idx] * 1000;
    uint32_t samplerate = samplerate_tab[ver][sr_idx];
    uint16_t samples = samples_tab[ver][layer];
    if (bitrate == 0 || samplerate == 0) {
        return false;
    }

    uint8_t padding = (hdr[2] >> 1) & 0x1;
    uint8_t pad_mult = (layer == 3) ? 4 : 1;
    uint16_t size = (samples * bitrate / samplerate) / 8 + padding * pad_mult;
    if(layer == 3) {
        size &= ~3;
    }
    if (size == 0 || size > MP3_MAX_FRAME) {
        return false;
    }

    *frame_size = size;
    if (out_ver)   { *out_ver = ver; }
    if (out_layer) { *out_layer = layer; }
    if (out_sr)    { *out_sr = samplerate; }
    return true;
}

static bool mp3_demuxer_check_xing_frame(mp3_demuxer_t *d, uint8_t *frame_data, uint16_t frame_len)
{
    uint16_t xing_offset = (d->mpeg_version == MPEG_VER_1) ?
                           (((frame_data[3] >> 6) & 0x3) == 3 ? 21 : 36) :
                           (((frame_data[3] >> 6) & 0x3) == 3 ? 13 : 21);

    if (frame_len > xing_offset + 16) {
        if (memcmp(frame_data + xing_offset, "Xing", 4) == 0 ||
            memcmp(frame_data + xing_offset, "Info", 4) == 0) {
            return true;
        }
    }

    if (frame_len > 32 + 26) {
        if (memcmp(frame_data + 32, "VBRI", 4) == 0) {
            return true;
        }
    }

    return false;
}

static void mp3_demuxer_parse_vbr_tag(mp3_demuxer_t *d, uint8_t *frame_data, uint16_t frame_len)
{
    uint8_t *tag_data = NULL;
    uint32_t frames = 0;
    uint32_t bytes = 0;
    bool found = false;

    uint16_t xing_offset = (d->mpeg_version == MPEG_VER_1) ?
                           ((d->codec_info.channels == 1) ? 21 : 36) :
                           ((d->codec_info.channels == 1) ? 13 : 21);

    if (frame_len > xing_offset + 16) {
        tag_data = frame_data + xing_offset;
        if (memcmp(tag_data, "Xing", 4) == 0 || memcmp(tag_data, "Info", 4) == 0) {
            found = true;
            d->is_vbr = true;

            uint32_t flags = (tag_data[4] << 24) | (tag_data[5] << 16) |
                           (tag_data[6] << 8) | tag_data[7];
            uint32_t pos = 8;

            if (flags & 0x01) {
                frames = get_unaligned_be32(tag_data + pos);
                pos += 4;
            }
            if (flags & 0x02) {
                bytes = get_unaligned_be32(tag_data + pos);
                pos += 4;
            }
            if (flags & 0x04) {
                memcpy(d->xing_info.toc, tag_data + pos, 100);
                d->xing_info.has_toc = true;
            }
        }
    }

    if (!found && frame_len > 32 + 26) {
        tag_data = frame_data + 32;
        if (memcmp(tag_data, "VBRI", 4) == 0) {
            found = true;
            d->is_vbr = true;

            bytes = get_unaligned_be32(tag_data + 0x0E);
            frames = get_unaligned_be32(tag_data + 0x16);
        }
    }

    if (!found && frame_len > 36 + 20) {
        uint16_t scan_len = (frame_len > 100) ? 100 : frame_len - 4;
        for (uint16_t i = 0; i < scan_len; i++) {
            if (memcmp(frame_data + i, "LAME", 4) == 0) {
                d->is_vbr = true;
                break;
            }
        }
    }

    if (found && frames > 0) {
        d->xing_info.frames = frames;
        d->xing_info.bytes = bytes;

        if (d->samplerate > 0) {
            uint16_t samples = samples_tab[d->mpeg_version][d->mpeg_layer];
            d->total_duration = (uint32_t)((uint64_t)frames * samples * 1000 / d->samplerate);
        }
    }
}

static uint32_t mp3_estimate_avg_bitrate(mp3_demuxer_t *d)
{
    if (!d->ops->seek || !d->ops->read || d->stream_type != AVDEMUXER_STREAM_FILE) {
        return 0;
    }
    
    uint32_t saved_pos = d->stream_pos;
    uint32_t current_pos = d->first_frame_offset;
    uint32_t total_bitrate = 0;
    uint32_t sample_count = 0;
    uint8_t buf[4];
    
    if (d->ops->seek(d->file_handle, current_pos, SEEK_SET) != 0) {
        return 0;
    }
    
    for (int i = 0; i < 20 && sample_count < 10; i++) {
        if (d->ops->read(buf, 1, 4, d->file_handle) < 4) {
            break;
        }
        
        if (buf[0] != 0xFF || (buf[1] & 0xE0) != 0xE0) {
            current_pos++;
            if (d->ops->seek(d->file_handle, current_pos, SEEK_SET) != 0) {
                break;
            }
            continue;
        }
        
        uint8_t ver_idx = (buf[1] >> 3) & 0x3;
        uint8_t layer = (buf[1] >> 1) & 0x3;
        uint8_t br_idx = (buf[2] >> 4) & 0xF;
        
        if (layer == 0 || br_idx == 0 || br_idx == 15) {
            current_pos++;
            if (d->ops->seek(d->file_handle, current_pos, SEEK_SET) != 0) {
                break;
            }
            continue;
        }
        
        uint8_t ver_tab = (ver_idx == MPEG_VER_1) ? 0 : 1;
        uint32_t frame_bitrate = bitrate_tab[ver_tab][layer][br_idx];
        if (frame_bitrate > 0) {
            total_bitrate += frame_bitrate;
            sample_count++;
        }
        
        uint8_t sr_idx = (buf[2] >> 2) & 0x3;
        uint16_t samplerate = samplerate_tab[ver_idx][sr_idx];
        uint16_t samples = samples_tab[ver_idx][layer];
        uint8_t padding = (buf[2] >> 1) & 0x1;
        
        if (samplerate == 0 || samples == 0) {
            current_pos++;
        } else {
            uint16_t frame_len = (ver_idx == MPEG_VER_1) ?
                ((samples * frame_bitrate * 1000 / samplerate / 8) + padding) :
                ((samples * frame_bitrate * 1000 / samplerate / 8) + padding);
            current_pos += frame_len;
        }
        
        if (d->ops->seek(d->file_handle, current_pos, SEEK_SET) != 0) {
            break;
        }
    }
    
    d->ops->seek(d->file_handle, saved_pos, SEEK_SET);
    
    if (sample_count > 0) {
        uint32_t avg_kbps = total_bitrate / sample_count;
        return avg_kbps * 1000;
    }
    
    return 0;
}

static bool mp3_demuxer_resync(mp3_demuxer_t *d)
{
    d->buffer.len = 0;
    d->buffer.pos = 0;

    int32_t got = d->ops->read(d->buffer.data, 1, MP3_BUF_SIZE, d->file_handle);
    if (got < 4) {
        return false;
    }

    d->buffer.len = got;
    d->stream_pos += got;

    uint8_t *p = d->buffer.data;
    uint8_t *end = d->buffer.data + got - 1;

    while (p < end) {
        p = memchr(p, 0xFF, end - p);
        if (p == NULL) {
            break;
        }
        if ((p[1] & 0xE0) == 0xE0) {
            uint16_t frame_size;
            uint8_t ver, layer;
            uint32_t samplerate;
            if (mp3_demuxer_verify_header(p, &frame_size, &ver, &layer, &samplerate)) {
                if (p + frame_size > d->buffer.data + d->buffer.len) {
                    p++;
                    continue;
                }

                if (p + frame_size + 4 <= d->buffer.data + d->buffer.len) {
                    uint8_t *next_p = p + frame_size;
                    uint16_t next_size;
                    if (next_p[0] == 0xFF && (next_p[1] & 0xE0) == 0xE0 &&
                        mp3_demuxer_verify_header(next_p, &next_size, NULL, NULL, NULL)) {
                        d->buffer.pos = p - d->buffer.data;
                        return true;
                    }
                }
            }
        }
        p++;
    }

    return false;
}

static uint32_t mp3_demuxer_calc_seek_pos(mp3_demuxer_t *d, uint32_t time_ms)
{
    if (!d->info_initialized) {
        return d->first_frame_offset;
    }
    
    if (d->total_duration > 0 && time_ms > d->total_duration) {
        time_ms = d->total_duration;
    }

    if (d->is_vbr && d->xing_info.has_toc && d->xing_info.bytes > 0) {
        uint32_t duration = 0;
        if (d->xing_info.frames > 0 && d->samplerate > 0) {
            uint16_t samples = samples_tab[d->mpeg_version][d->mpeg_layer];
            duration = (uint64_t)d->xing_info.frames * samples * 1000 / d->samplerate;
        }

        if (duration == 0) { duration = 1; }
        if (time_ms > duration) { time_ms = duration; }
        uint8_t percent = (time_ms * 100) / duration;
        if (percent > 99) { percent = 99; }

        uint32_t toc_entry = d->xing_info.toc[percent];
        uint64_t offset = (uint64_t)d->xing_info.bytes * toc_entry / 256;
        uint32_t pos = d->first_frame_offset + (uint32_t)offset;
        return pos;
    }

    if (d->is_vbr && d->xing_info.bytes > 0) {
        uint32_t duration = 0;
        if (d->xing_info.frames > 0 && d->samplerate > 0) {
            uint16_t samples = samples_tab[d->mpeg_version][d->mpeg_layer];
            duration = (uint64_t)d->xing_info.frames * samples * 1000 / d->samplerate;
        }

        if (duration > 0) {
            uint64_t offset = (uint64_t)time_ms * d->xing_info.bytes / duration;
            uint32_t pos = d->first_frame_offset + (uint32_t)offset;
            return pos;
        }
    }

    if (d->codec_info.bit_rate > 0) {
        uint64_t offset = (uint64_t)time_ms * d->codec_info.bit_rate / 8000;
        uint32_t pos = d->first_frame_offset + (uint32_t)offset;
        return pos;
    }

    return d->first_frame_offset;
}

static int32_t mp3_demuxer_output_frame(mp3_demuxer_t *d)
{
    uint8_t *frame_data = d->buffer.data + d->buffer.pos;

    if (!d->info_initialized) {
        if (mp3_demuxer_check_xing_frame(d, frame_data, d->frame_size)) {
            d->buffer.pos += d->frame_size;
            uint16_t remain = d->buffer.len - d->buffer.pos;
            if (remain > 0) {
                memmove(d->buffer.data, d->buffer.data + d->buffer.pos, remain);
                d->buffer.len = remain;
                d->buffer.pos = 0;
            } else {
                d->buffer.len = 0;
                d->buffer.pos = 0;
            }
            d->is_vbr = 1;
            return -EAGAIN;
        }

        memset(&d->codec_info, 0, sizeof(d->codec_info));
        d->codec_info.codec_id = AUDIO_CODEC_MP3;
        d->codec_info.sample_rate = d->samplerate;
        d->codec_info.channels = ((frame_data[3] >> 6) & 0x3) == 3 ? 1 : 2;
        d->codec_info.ch_layout = (d->codec_info.channels == 1) ? AUDIO_CH_LAYOUT_MONO : AUDIO_CH_LAYOUT_STEREO;
        uint8_t br_idx = (frame_data[2] >> 4) & 0xF;
        uint8_t ver_tab = (d->mpeg_version == MPEG_VER_1) ? 0 : 1;
        d->codec_info.bit_rate = bitrate_tab[ver_tab][d->mpeg_layer][br_idx] * 1000;
        d->codec_info.frame_size = samples_tab[d->mpeg_version][d->mpeg_layer];
        os_printf("[demux] ver=%d, layer=%d, frame_size=%d, samples=%d\n",
                  d->mpeg_version, d->mpeg_layer, d->frame_size, d->codec_info.frame_size);
        d->info_initialized = true;
        d->first_frame_offset = d->stream_pos - (d->buffer.len - d->buffer.pos);

        mp3_demuxer_parse_vbr_tag(d, frame_data, d->frame_size);

        if (d->xing_info.frames > 0) {
            d->duration_precise = 1;
        }

        if (d->total_duration == 0 && d->file_size > 0) {
            uint64_t audio_data_size = d->file_size - d->first_frame_offset;
            if (audio_data_size > 0) {
                uint32_t calc_bitrate = d->codec_info.bit_rate;
                
                if (d->is_vbr && d->xing_info.frames == 0 && d->stream_type == AVDEMUXER_STREAM_FILE) {
                    uint32_t avg_bitrate = mp3_estimate_avg_bitrate(d);
                    if (avg_bitrate > 0) {
                        calc_bitrate = avg_bitrate;
                    }
                }
                
                if (calc_bitrate > 0) {
                    uint64_t calc = audio_data_size * 8 * 1000;
                    d->total_duration = (uint32_t)(calc / calc_bitrate);
                }
            }
        }
        
        if (d->total_duration == 0 && d->stream_type != AVDEMUXER_STREAM_FILE) {
            if (d->ops->ioctl) {
                int64_t new_file_size = 0;
                d->ops->ioctl(d->owner, AVDEMUXER_GET_FILE_SIZE, (uint32)&new_file_size, 0);
                if (new_file_size > 0 && new_file_size != d->file_size) {
                    d->file_size = new_file_size;
                    if (d->file_size > 0) {
                        uint64_t audio_data_size = d->file_size - d->first_frame_offset;
                        if (audio_data_size > 0) {
                            uint32_t calc_bitrate = d->codec_info.bit_rate;
                            if (calc_bitrate > 0) {
                                uint64_t calc = audio_data_size * 8 * 1000;
                                d->total_duration = (uint32_t)(calc / calc_bitrate);
                            }
                        }
                    }
                }
            }
        }
    }
    
    if (!d->duration_precise && d->file_size > 0 && d->sample_count < 20) {
        uint8_t br_idx = (frame_data[2] >> 4) & 0xF;
        if (br_idx > 0 && br_idx < 15) {
            uint8_t ver_tab = (d->mpeg_version == MPEG_VER_1) ? 0 : 1;
            uint32_t frame_bitrate = bitrate_tab[ver_tab][d->mpeg_layer][br_idx];
            if (frame_bitrate > 0) {
                d->sample_total_bitrate += frame_bitrate;
                d->sample_count++;
                
                if (d->sample_count >= 10) {
                    uint32_t avg_bitrate_kbps = d->sample_total_bitrate / d->sample_count;
                    uint64_t audio_data_size = d->file_size - d->first_frame_offset;
                    if (audio_data_size > 0 && avg_bitrate_kbps > 0) {
                        uint64_t calc = audio_data_size * 8 * 1000;
                        uint32_t new_duration = (uint32_t)(calc / (avg_bitrate_kbps * 1000));
                        if (new_duration > 0 && new_duration != d->total_duration) {
                            d->total_duration = new_duration;
                        }
                    }
                }
            }
        }
    }

    struct framebuff *fb = msi_alloc_fb(d->owner, NULL, NULL, d->frame_size, 0, 0);
    if (!fb) {
        //_os_printf("F");
        return -ENOMEM;
    }

    memcpy(fb->data, frame_data, d->frame_size);
    fb->codec_info  = &d->codec_info;
    fb->time  = (uint32_t)(d->current_time_us / 1000);
    fb->mtype = MEDIA_DATA_AUDIO;
    fb->stype = AUDIO_CODEC_MP3;


    uint16_t samples = samples_tab[d->mpeg_version][d->mpeg_layer];
    d->current_time_us += (uint64_t)samples * 1000000 / d->samplerate;
    d->ops->outFB(d->owner, fb);

    d->buffer.pos += d->frame_size;
    uint16_t remain = d->buffer.len - d->buffer.pos;
    if (remain > 0) {
        memmove(d->buffer.data, d->buffer.data + d->buffer.pos, remain);
        d->buffer.len = remain;
        d->buffer.pos = 0;
    } else {
        d->buffer.len = 0;
        d->buffer.pos = 0;
    }

    return (int32_t)d->frame_size;
}

static void mp3_demuxer_skip_id3(mp3_demuxer_t *d)
{
    if (d->buffer.len < 10) {
        return;
    }
    if (memcmp(d->buffer.data, "ID3", 3) != 0) {
        return;
    }
    uint32_t size = ((d->buffer.data[6] & 0x7F) << 21) |
                    ((d->buffer.data[7] & 0x7F) << 14) |
                    ((d->buffer.data[8] & 0x7F) << 7) |
                    (d->buffer.data[9] & 0x7F);
    size += 10;
    if (d->buffer.len >= size) {
        d->buffer.pos += size;
        uint16_t remain = d->buffer.len - d->buffer.pos;
        if (remain > 0) {
            memmove(d->buffer.data, d->buffer.data + d->buffer.pos, remain);
            d->buffer.len = remain;
            d->buffer.pos = 0;
        } else {
            d->buffer.len = 0;
            d->buffer.pos = 0;
        }
        return;
    }

    d->id3_skip_remaining = size - d->buffer.len;
    d->buffer.len = 0;
    d->buffer.pos = 0;
}

static void *mp3_demuxer_init(void *hdl, const struct AVDemuxerOps *ops, void *hdr, uint32_t len, struct msi *owner)
{
    mp3_demuxer_t *d = decoder_mem_zalloc(sizeof(mp3_demuxer_t));
    if (!d) {
        return NULL;
    }

    d->owner = owner;
    d->file_handle = hdl;
    d->ops = ops;
    d->current_time_us = 0;
    d->id3_skip_remaining = 0;
    d->retry_cnt = 0;
    ops->ioctl(owner, AVDEMUXER_GET_FILE_SIZE, (uint32)&d->file_size, 0);
    d->stream_type = ops->ioctl(owner, AVDEMUXER_GET_STREAM_TYPE, 0, 0);

    if (hdr && len > 0) {
        uint32_t copy = (len > MP3_BUF_SIZE) ? MP3_BUF_SIZE : len;
        memcpy(d->buffer.data, hdr, copy);
        d->buffer.len = copy;
        d->stream_pos = copy;
    }

    mp3_demuxer_skip_id3(d);
    return d;
}

static int32_t mp3_demuxer_release(void *c)
{
    mp3_demuxer_t *d = (mp3_demuxer_t *)c;
    decoder_mem_free(d);
    return 0;
}

static int32_t mp3_demuxer_do_seek(void *c, uint32_t time)
{
    mp3_demuxer_t *d = (mp3_demuxer_t *)c;

    if (!d || !d->ops || !d->file_handle) {
        return -1;
    }

    if (d->total_duration > 0 && time > d->total_duration) {
        time = d->total_duration;
    }

    uint32_t target = mp3_demuxer_calc_seek_pos(d, time);
    
    if (d->file_size > 0 && target >= (uint32_t)d->file_size) {
        target = (d->file_size > d->first_frame_offset) ? 
                 (uint32_t)(d->file_size - 1) : d->first_frame_offset;
    }

    if (d->ops->seek(d->file_handle, target, SEEK_SET) != 0) {
        return -1;
    }

    d->stream_pos = target;
    d->id3_skip_remaining = 0;
    d->retry_cnt = 0;
    d->buffer.len = 0;
    d->buffer.pos = 0;
    d->parse_phase = MP3_PHASE_HEADER;

    d->current_time_us = (uint64_t)time * 1000;

    return 0;
}

static int32_t mp3_demuxer_do_demux(void *c)
{
    mp3_demuxer_t *d = (mp3_demuxer_t *)c;

    if (!d || !d->ops) {
        return -EINVAL;
    }

    if(d->owner->fb_limits.counter == 0){
        return -ENOMEM;
    }
    
    if (d->parse_phase == MP3_PHASE_HEADER) {

        if (!mp3_demuxer_ensure(d, 4)) {
            uint16_t remain = d->buffer.len - d->buffer.pos;

            if (remain > 0 && remain < 4) {
                d->buffer.len = 0;
                d->buffer.pos = 0;
                d->retry_cnt = 0;
                return 0;
            }

            if (remain > 0 && d->retry_cnt < 5) {
                d->retry_cnt++;
                return 0;
            }

            d->retry_cnt = 0;
            if (remain > 0) {
                memmove(d->buffer.data, d->buffer.data + d->buffer.pos, remain);
                d->buffer.len = remain;
                d->buffer.pos = 0;
            } else {
                d->buffer.len = 0;
                d->buffer.pos = 0;
            }
            return 0;
        }

        d->retry_cnt = 0;

        uint8_t tmp_ver, tmp_layer;
        uint32_t tmp_sr;
        uint16_t tmp_size;
        
        if (!mp3_demuxer_verify_header(d->buffer.data + d->buffer.pos, &tmp_size,
                                       &tmp_ver, &tmp_layer, &tmp_sr)) {
            uint8_t *start = d->buffer.data + d->buffer.pos + 1;
            uint8_t *end = d->buffer.data + d->buffer.len;
            uint8_t *next_sync = memchr(start, 0xFF, end - start);

            if (!next_sync && d->ops->eof && d->ops->eof(d->file_handle)) {
                d->buffer.len = 0;
                d->buffer.pos = 0;
                d->retry_cnt = 0;
                return 0;
            }

            if (next_sync) {
                d->buffer.pos = next_sync - d->buffer.data;
            } else {
                d->buffer.pos = d->buffer.len;
            }

            uint16_t remain = d->buffer.len - d->buffer.pos;
            if (remain > 0) {
                memmove(d->buffer.data, d->buffer.data + d->buffer.pos, remain);
                d->buffer.len = remain;
                d->buffer.pos = 0;
            } else {
                d->buffer.len = 0;
                d->buffer.pos = 0;
            }
            return -EAGAIN;
        }

        if (d->info_initialized) {
            if (tmp_sr != d->samplerate || tmp_layer != d->mpeg_layer || tmp_ver != d->mpeg_version) {
                d->buffer.pos++;
                uint16_t remain = d->buffer.len - d->buffer.pos;
                if (remain > 0) {
                    memmove(d->buffer.data, d->buffer.data + d->buffer.pos, remain);
                    d->buffer.len = remain;
                    d->buffer.pos = 0;
                } else {
                    d->buffer.len = 0;
                    d->buffer.pos = 0;
                }
                return -EAGAIN;
            }
        }
        
        d->frame_size = tmp_size;
        d->mpeg_version = tmp_ver;
        d->mpeg_layer = tmp_layer;
        d->samplerate = tmp_sr;

        d->parse_phase = MP3_PHASE_BODY;
    }

    if (d->parse_phase == MP3_PHASE_BODY) {
        uint16_t have = d->buffer.len - d->buffer.pos;

        if (have < d->frame_size) {
            if (!mp3_demuxer_ensure(d, d->frame_size)) {
                uint16_t remain = d->buffer.len - d->buffer.pos;

                if (d->ops->eof && d->ops->eof(d->file_handle)) {
                    d->buffer.len = 0;
                    d->buffer.pos = 0;
                    d->parse_phase = MP3_PHASE_HEADER;
                    d->retry_cnt = 0;
                    return 0;
                }
                else if (remain > 0 && d->retry_cnt < 5) {
                    d->retry_cnt++;
                    return 0;
                }
                else {
                    d->retry_cnt = 0;
                }
                return 0;
            }
        }

        d->retry_cnt = 0;
        int32_t ret = mp3_demuxer_output_frame(d);
        d->parse_phase = MP3_PHASE_HEADER;
        return ret;
    }
    return -EINVAL;
}

static int32_t mp3_demuxer_ioctl(void *c, uint32_t cmd, uint32_t param1, uint32_t param2)
{
    mp3_demuxer_t *d = (mp3_demuxer_t *)c;
    switch (cmd) {
        case AVDEMUXER_SET_PLAY_SPEED:
            break;
        case AVDEMUXER_SET_TRACK:
            break;
        case AVDEMUXER_GET_TOTAL_DURATION:
            if (param1) *(uint32 *)param1 = d->total_duration;
            break;
    }
    return 0;
}

const __avdemuxer struct AVDemuxer mp3_demuxer = {
    .type         = MEDIA_CONTAINER_MP3,
    .name         = "mp3_demuxer",
    .init         = mp3_demuxer_init,
    .release      = mp3_demuxer_release,
    .do_seek      = mp3_demuxer_do_seek,
    .do_demux     = mp3_demuxer_do_demux,
    .ioctl        = mp3_demuxer_ioctl,
};
