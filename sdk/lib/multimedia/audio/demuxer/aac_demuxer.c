#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/multimedia/AVContainer.h"
#include "../audio_coder.h"

#define AAC_BUF_SIZE            2048
#define ADTS_HEADER_SIZE        7
#define AAC_MAX_FRAME           8192
#define AAC_DEMUXER_SFB_NUM     4

// AAC 帧常量
#define AAC_SAMPLES_PER_FRAME   1024        // AAC 每帧采样数
#define AAC_MS_TO_NS            1000000     // 毫秒转纳秒
#define AAC_MS_TO_US            1000        // 毫秒转微秒
#define AAC_BUFFER_COMPACT_THRESH  512      // 缓冲区整理阈值

// ADTS 同步字
#define ADTS_SYNC_WORD_0        0xFF
#define ADTS_SYNC_WORD_1_MASK   0xF0
#define ADTS_SYNC_WORD_1_VALUE  0xF0

typedef struct {
    uint8_t data[AAC_BUF_SIZE];
    uint16_t len;
    uint16_t pos;
} aac_buf_t;

typedef enum {
    AAC_PHASE_HEADER = 0,
    AAC_PHASE_BODY,
} aac_phase_t;

/**
 * @brief AAC Demuxer 上下文结构体
 * @note 管理 AAC/ADTS 格式音频的解封装状态
 */
typedef struct {
    /*=== 核心接口与上下文 ===*/
    struct msi                 *owner;              // 上层播放器实例
    const struct AVDemuxerOps  *ops;                // 文件操作接口 
    void                      *file_handle;         // 文件/URL句柄
    aac_buf_t                  buffer;              // 输入数据缓冲区 
    txAudioInfo_t         codec_info;          // 编码信息
    uint32_t                   samplerate;          // 采样率
    uint32_t                   channels;            // 声道数
    uint16_t                   frame_size;          // 当前帧大小
    aac_phase_t                parse_phase;         // 解析阶段
    uint32_t                   stream_pos;          // 当前文件位置 (用于seek)
    uint8_t                    get_first_frame;     // 是否已获取第一帧
    uint8_t                    info_initialized : 1;// 音频参数是否已初始化
    uint32_t                   total_duration;      // 总时长 (毫秒)
    int64_t                    file_size;           // 文件总大小
    uint32_t                   first_frame_pos;     // 第一帧在文件中的偏移
    uint8_t                    duration_precise : 1;// 时长是否精确扫描获得
    int32_t                    stream_type;         // 流类型
    uint32_t                   sample_count;        // 已采样帧数
    uint32_t                   sample_total_size;   // 采样帧总大小
    uint64_t                   current_time_us;     // 当前播放时间 (微秒)
} aac_demuxer_t;

static const uint32_t aac_sr_table[] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000, 7350
};

static uint32_t get_adts_frame_length(uint8_t *data)
{
    if (!data || data[0] != ADTS_SYNC_WORD_0 || (data[1] & ADTS_SYNC_WORD_1_MASK) != ADTS_SYNC_WORD_1_VALUE) {
        return 0;
    }
    return (((uint32_t)(data[3] & 0x03)) << 11) | 
           (((uint32_t)data[4]) << 3) | 
           (((uint32_t)(data[5] & 0xE0)) >> 5);
}

static bool aac_demuxer_ensure(aac_demuxer_t *d, uint16_t need)
{
    uint16_t have = d->buffer.len - d->buffer.pos;
    
    if (have >= need) {
        return true;
    }

    uint16_t want = need - have;
    
    if (d->stream_type != AVDEMUXER_STREAM_FILE) {
        uint16_t space = AAC_BUF_SIZE - d->buffer.len;
        uint16_t prefer = want * 2;
        if (prefer < 512) prefer = 512;
        if (prefer > space) prefer = space;
        if (prefer > want) want = prefer;
    }
    
    if (d->buffer.len + want > AAC_BUF_SIZE) {
        uint16_t remain = have;
        if (remain > 0 && d->buffer.pos > 0) {
            memmove(d->buffer.data, d->buffer.data + d->buffer.pos, remain);
            d->buffer.len = remain;
            d->buffer.pos = 0;
        } else {
            return false;
        }
    }

    int32_t got = d->ops->read(d->buffer.data + d->buffer.len, 1, want, d->file_handle);
    if (got > 0) {
        d->buffer.len += got;
        d->stream_pos += got;
    }
    
    return (d->buffer.len - d->buffer.pos) >= need;
}

static uint32_t aac_scan_duration_precise(aac_demuxer_t *d)
{
    uint8_t hdr[ADTS_HEADER_SIZE];
    uint32_t frame_count = 0;
    uint32_t current_pos = d->first_frame_pos;
    uint32_t sr = d->samplerate;
    uint64_t start_ms = os_mseconds();
    
    uint32_t saved_pos = d->stream_pos;
    
    if (d->ops->seek(d->file_handle, current_pos, SEEK_SET) != 0) {
        return 0;
    }
    
    while (1) {
        int ret = d->ops->read(hdr, 1, ADTS_HEADER_SIZE, d->file_handle);
        if (ret < ADTS_HEADER_SIZE) {
            break;
        }
        
        if (hdr[0] != ADTS_SYNC_WORD_0 || (hdr[1] & ADTS_SYNC_WORD_1_MASK) != ADTS_SYNC_WORD_1_VALUE) {
            current_pos++;
            if (d->ops->seek(d->file_handle, current_pos, SEEK_SET) != 0) break;
            continue;
        }
        
        uint32_t frame_len = get_adts_frame_length(hdr);
        if (frame_len == 0 || frame_len > AAC_MAX_FRAME) {
            current_pos++;
            if (d->ops->seek(d->file_handle, current_pos, SEEK_SET) != 0) break;
            continue;
        }
        
        uint8_t sr_idx = (hdr[2] >> 2) & 0x0F;
        if (sr_idx < 13) {
            sr = aac_sr_table[sr_idx];
        }
        
        frame_count++;
        
        current_pos += frame_len;
        if (current_pos >= d->file_size) break;
        
        if (d->ops->seek(d->file_handle, current_pos, SEEK_SET) != 0) break;
        
        if (os_mseconds() - start_ms > 2000) {
            frame_count = 0;
            break;
        }
    }
    
    d->ops->seek(d->file_handle, saved_pos, SEEK_SET);
    
    if (frame_count > 0 && sr > 0) {
        uint32_t duration = (uint64_t)frame_count * AAC_SAMPLES_PER_FRAME * AAC_MS_TO_US / sr;
        return duration;
    }
    
    return 0;
}

static uint32_t aac_estimate_duration(aac_demuxer_t *d, uint8_t *first_frame_data)
{
    if (d->file_size <= 0 || d->samplerate == 0) {
        return 0;
    }
    
    uint32_t data_start_pos = d->first_frame_pos;
    if (data_start_pos == 0) {
        data_start_pos = d->stream_pos - (d->buffer.len - d->buffer.pos);
        if (data_start_pos == 0) {
            data_start_pos = 7;
        }
    }
    
    if (data_start_pos >= d->file_size) {
        return 0;
    }
    
    uint32_t frame_len = get_adts_frame_length(first_frame_data);
    if (frame_len == 0) {
        return 0;
    }
    
    uint32_t audio_data_size = (uint32_t)(d->file_size - data_start_pos);
    uint32_t estimated_frames = audio_data_size / frame_len;
    uint32_t duration = (uint64_t)estimated_frames * AAC_SAMPLES_PER_FRAME * AAC_MS_TO_US / d->samplerate;
    
    return duration;
}

static int32_t aac_demuxer_output_frame(aac_demuxer_t *d)
{
    uint8_t *frame_data = d->buffer.data + d->buffer.pos;
    
    if (!d->info_initialized) {
        memset(&d->codec_info, 0, sizeof(d->codec_info));
        d->codec_info.codec_id = AUDIO_CODEC_AAC;
        
        uint8_t sr_idx = (frame_data[2] >> 2) & 0x0F;
        d->samplerate = (sr_idx < 13) ? aac_sr_table[sr_idx] : 44100;
        d->channels = ((frame_data[2] >> 6) & 0x03) == 3 ? 1 : 2;
        
        d->codec_info.sample_rate = d->samplerate;
        d->codec_info.channels = d->channels;
        d->codec_info.ch_layout = (d->channels == 1) ? AUDIO_CH_LAYOUT_MONO : AUDIO_CH_LAYOUT_STEREO;
        d->codec_info.frame_size = 1024;
        
        d->info_initialized = 1;
        
        d->first_frame_pos = d->stream_pos - (d->buffer.len - d->buffer.pos);
        
        if (d->stream_type == AVDEMUXER_STREAM_FILE && 
            d->file_size > 0 && 
            d->file_size < (10 * 1024 * 1024)) {
            d->total_duration = aac_scan_duration_precise(d);
            if (d->total_duration > 0) {
                d->duration_precise = 1;
            }
        }
        
        if (d->total_duration == 0) {
            d->total_duration = aac_estimate_duration(d, frame_data);
            d->duration_precise = 0;
        }
    }
    
    if (d->total_duration == 0 && d->stream_type != AVDEMUXER_STREAM_FILE) {
        if (d->ops->ioctl) {
            int64_t new_file_size = 0;
            d->ops->ioctl(d->owner, AVDEMUXER_GET_FILE_SIZE, (uint32)&new_file_size, 0);
            if (new_file_size > 0 && new_file_size != d->file_size) {
                d->file_size = new_file_size;
                d->total_duration = aac_estimate_duration(d, frame_data);
            }
        }
    }
    
    if (!d->duration_precise && d->file_size > 0 && d->sample_count < 20) {
        d->sample_total_size += d->frame_size;
        d->sample_count++;
        
        if (d->sample_count >= 5) {
            uint32_t avg_frame_size = d->sample_total_size / d->sample_count;
            uint32_t audio_data_size = (uint32_t)(d->file_size - d->first_frame_pos);
            if (audio_data_size > 0 && avg_frame_size > 0 && d->samplerate > 0) {
                uint32_t estimated_frames = audio_data_size / avg_frame_size;
                uint32_t new_duration = (uint64_t)estimated_frames * AAC_SAMPLES_PER_FRAME * AAC_MS_TO_US / d->samplerate;
                if (new_duration > 0 && new_duration != d->total_duration) {
                    d->total_duration = new_duration;
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
    fb->codec_info = &d->codec_info;
    fb->time = (uint32_t)(d->current_time_us / 1000);
    fb->mtype = MEDIA_DATA_AUDIO;
    fb->stype = AUDIO_CODEC_AAC;



    uint64_t frame_duration_us = (uint64_t)AAC_SAMPLES_PER_FRAME * 1000000 / d->samplerate;
    d->current_time_us += frame_duration_us;

    d->ops->outFB(d->owner, fb);

    d->buffer.pos += d->frame_size;
    uint16_t remain = d->buffer.len - d->buffer.pos;
    if (remain > 0 && remain < AAC_BUFFER_COMPACT_THRESH) {
        memmove(d->buffer.data, d->buffer.data + d->buffer.pos, remain);
        d->buffer.len = remain;
        d->buffer.pos = 0;
    } else {
        d->buffer.len = 0;
        d->buffer.pos = 0;
    }

    return (int32_t)d->frame_size;
}

static void *aac_demuxer_init(void *hdl, const struct AVDemuxerOps *ops, void *hdr, uint32_t len, struct msi *owner)
{
    aac_demuxer_t *d = decoder_mem_zalloc(sizeof(aac_demuxer_t));
    if (!d) {
        return NULL;
    }

    d->owner = owner;
    d->file_handle = hdl;
    d->ops = ops;
    d->current_time_us = 0;
    d->get_first_frame = 0;
    d->info_initialized = 0;
    d->parse_phase = AAC_PHASE_HEADER;
    d->total_duration = 0;
    d->file_size = 0;
    d->first_frame_pos = 0;
    d->duration_precise = 0;
    d->stream_type = AVDEMUXER_STREAM_FILE;

    if (ops->ioctl) {
        ops->ioctl(owner, AVDEMUXER_GET_FILE_SIZE, (uint32)&d->file_size, 0);
        d->stream_type = ops->ioctl(owner, AVDEMUXER_GET_STREAM_TYPE, 0, 0);
    }

    if (hdr && len > 0) {
        uint32_t copy = (len > AAC_BUF_SIZE) ? AAC_BUF_SIZE : len;
        memcpy(d->buffer.data, hdr, copy);
        d->buffer.len = copy;
        d->stream_pos = copy;
    }

    return d;
}

static int32_t aac_demuxer_release(void *c)
{
    aac_demuxer_t *d = (aac_demuxer_t *)c;
    decoder_mem_free(d);
    return 0;
}

static uint32_t aac_demuxer_calc_seek_pos(aac_demuxer_t *d, uint32_t time_ms)
{
	if (d->total_duration > 0 && time_ms > d->total_duration) {
        time_ms = d->total_duration;
    }
	
    if (d->sample_count > 0 && d->sample_total_size > 0) {
        uint32_t avg_frame_size = d->sample_total_size / d->sample_count;
        uint32_t sr = d->samplerate ? d->samplerate : 8000;
        uint32_t frame_duration_ms = (AAC_SAMPLES_PER_FRAME * 1000) / sr;
        uint32_t target_frame = time_ms / frame_duration_ms;
        uint32_t est_pos = d->first_frame_pos + target_frame * avg_frame_size;
        
        if (est_pos < d->first_frame_pos) est_pos = d->first_frame_pos;
        if (d->file_size > 0 && est_pos > d->file_size - ADTS_HEADER_SIZE) {
            est_pos = d->file_size - ADTS_HEADER_SIZE;
        }
        return est_pos;
    }

    uint32_t est_pos = d->first_frame_pos + (time_ms * 2);
    if (d->file_size > 0 && est_pos > d->file_size - ADTS_HEADER_SIZE) {
        est_pos = d->file_size - ADTS_HEADER_SIZE;
    }
    return est_pos;
}

static int32_t aac_demuxer_do_seek(void *c, uint32_t time)
{
    aac_demuxer_t *d = (aac_demuxer_t *)c;
    uint32_t pos;

    if (!d || !d->ops || !d->ops->seek) {
        return -1;
    }
    
	if (d->total_duration > 0 && time > d->total_duration) {
        time = d->total_duration;
    }
	
    pos = aac_demuxer_calc_seek_pos(d, time);
    
    if (d->ops->seek(d->file_handle, pos, SEEK_SET) != 0) {
        return -1;
    }
    
    d->buffer.len = 0;
    d->buffer.pos = 0;
    d->parse_phase = AAC_PHASE_HEADER;
    d->get_first_frame = 0;
    
    d->current_time_us = (uint64_t)time * 1000;
    
    return 0;
}

static int32_t aac_demuxer_do_demux(void *c)
{
    aac_demuxer_t *d = (aac_demuxer_t *)c;
    
    if (!d || !d->ops) {
        return -EINVAL;
    }

    if(d->owner->fb_limits.counter == 0){
        return -ENOMEM;
    }
    

    if (d->parse_phase == AAC_PHASE_HEADER) {
        if (!aac_demuxer_ensure(d, ADTS_HEADER_SIZE)) {
            uint16_t remain = d->buffer.len - d->buffer.pos;
            bool is_eof = d->ops->eof && d->ops->eof(d->file_handle);
            
            if (remain > 0 && remain < ADTS_HEADER_SIZE && is_eof) {
                d->buffer.len = 0;
                d->buffer.pos = 0;
                return 0;
            }
            
            if (remain > 0 && remain < AAC_BUFFER_COMPACT_THRESH) {
                memmove(d->buffer.data, d->buffer.data + d->buffer.pos, remain);
                d->buffer.len = remain;
                d->buffer.pos = 0;
            }
            return 0;
        }

        uint8_t *hdr = d->buffer.data + d->buffer.pos;
        
        if (hdr[0] != ADTS_SYNC_WORD_0 || (hdr[1] & ADTS_SYNC_WORD_1_MASK) != ADTS_SYNC_WORD_1_VALUE) {
            uint8_t *start = d->buffer.data + d->buffer.pos + 1;
            uint8_t *end = d->buffer.data + d->buffer.len;
            uint8_t *next_sync = memchr(start, 0xFF, end - start);
            
            if (!next_sync && d->ops->eof && d->ops->eof(d->file_handle)) {
                d->buffer.len = 0;
                d->buffer.pos = 0;
                return 0;
            }
            
            if (next_sync) {
                d->buffer.pos = next_sync - d->buffer.data;
            } else {
                d->buffer.pos = d->buffer.len;
            }
            
            uint16_t remain = d->buffer.len - d->buffer.pos;
            if (remain > 0 && remain < AAC_BUFFER_COMPACT_THRESH) {
                memmove(d->buffer.data, d->buffer.data + d->buffer.pos, remain);
                d->buffer.len = remain;
                d->buffer.pos = 0;
            } else {
                d->buffer.len = 0;
                d->buffer.pos = 0;
            }
            return -EAGAIN;
        }

        d->frame_size = get_adts_frame_length(hdr);
        if (d->frame_size == 0 || d->frame_size > AAC_MAX_FRAME) {
            d->buffer.pos++;
            uint16_t remain = d->buffer.len - d->buffer.pos;
            if (remain > 0 && remain < AAC_BUFFER_COMPACT_THRESH) {
                memmove(d->buffer.data, d->buffer.data + d->buffer.pos, remain);
                d->buffer.len = remain;
                d->buffer.pos = 0;
            }
            return -EAGAIN;
        }

        d->parse_phase = AAC_PHASE_BODY;
    }

    if (d->parse_phase == AAC_PHASE_BODY) {
        uint16_t have = d->buffer.len - d->buffer.pos;
        if (have < d->frame_size) {
            if (!aac_demuxer_ensure(d, d->frame_size)) {
                if (d->ops->eof && d->ops->eof(d->file_handle)) {
                    d->buffer.len = 0;
                    d->buffer.pos = 0;
                    d->parse_phase = AAC_PHASE_HEADER;
                }
                return 0;
            }
        }

        int32_t ret = aac_demuxer_output_frame(d);
        d->parse_phase = AAC_PHASE_HEADER;
        return ret;
    }
    
    return 0;
}


static int32_t aac_demuxer_ioctl(void *c, uint32_t cmd, uint32_t param1, uint32_t param2)
{
    aac_demuxer_t *d = (aac_demuxer_t *)c;
    switch (cmd) {
        case AVDEMUXER_GET_TOTAL_DURATION:
            if (param1) {
                *(uint32_t*)param1 = d->total_duration;
            }
            break;
        default:
            break;
    }
    return 0;
}

const __avdemuxer struct AVDemuxer aac_demuxer = {
    .type         = MEDIA_CONTAINER_RAW_AAC,
    .name         = "aac_demuxer",
    .init         = aac_demuxer_init,
    .release      = aac_demuxer_release,
    .do_seek      = aac_demuxer_do_seek,
    .do_demux     = aac_demuxer_do_demux,
    .ioctl        = aac_demuxer_ioctl,
};
