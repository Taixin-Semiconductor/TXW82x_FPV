#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/multimedia/AVContainer.h"  
#include "../audio_coder.h"

#define AMR_BUF_SIZE            1024
#define AMR_DEMUXER_SFB_NUM     6

#define AMRNB_HEADER            "#!AMR\n"
#define AMRWB_HEADER            "#!AMR-WB\n"

typedef struct {
    uint8_t data[AMR_BUF_SIZE];
    uint16_t len;
    uint16_t pos;
} amr_buf_t;

typedef enum {
    AMR_NB = 1,
    AMR_WB,
} amr_type_t;

/**
 * @brief AMR Demuxer 上下文结构体
 * @note 管理 AMR-NB/WB 格式音频的解封装状态
 */
typedef struct {
    struct msi                 *owner;              // 上层播放器实例
    const struct AVDemuxerOps  *ops;                // 文件操作接口
    void                      *file_handle;         // 文件/URL句柄
    amr_buf_t                  buffer;              // 输入数据缓冲区
    txAudioInfo_t         codec_info;          // 编码信息
    const uint8_t              *frame_size_table;   // 帧大小表 (NB或WB)
    uint8_t                    coder_type;          // 编码类型 (AMR_NB/AMR_WB)
    uint32_t                   header_size;         // 文件头大小
    uint32_t                   stream_pos;          // 当前文件位置
    uint8_t                    info_initialized : 1;// 是否已初始化
    uint8_t                    duration_precise : 1;// 时长是否精确
    uint32_t                   total_duration;      // 总时长 (毫秒)
    int64_t                    file_size;           // 文件总大小
    uint32_t                   avg_frame_size;      // 平均帧大小
    int32_t                    stream_type;         // 流类型
    uint32_t                   sample_count;        // 已采样帧数
    uint32_t                   sample_total_size;   // 采样帧总大小
    uint64_t                   current_time_us;     // 当前播放时间 (微秒)
} amr_demuxer_t;


static const uint8_t amrnb_frame_size_table[16] = {
    13, 14, 16, 18, 20, 21, 27, 32, 6, 0, 0, 0, 0, 0, 0, 1
};

static const uint8_t amrwb_frame_size_table[16] = {
    18, 24, 33, 37, 41, 47, 51, 59, 61, 6, 0, 0, 0, 0, 1, 1
};

static bool amr_demuxer_ensure(amr_demuxer_t *d, uint16_t need)
{
    uint16_t have = d->buffer.len - d->buffer.pos;
    
    if (have >= need) {
        return true;
    }

    uint16_t want = need - have;
    
    if (d->buffer.len + want > AMR_BUF_SIZE) {
        uint16_t remain = have;
        if (remain > 0 && d->buffer.pos > 0) {
            memmove(d->buffer.data, d->buffer.data + d->buffer.pos, remain);
            d->buffer.len = remain;
            d->buffer.pos = 0;
        } else if (remain == 0) {
            d->buffer.len = 0;
            d->buffer.pos = 0;
        } else {
            return false;
        }
    }

    uint16_t to_read = want;
    if (d->stream_type != AVDEMUXER_STREAM_FILE) {
        uint16_t space = AMR_BUF_SIZE - d->buffer.len;
        uint16_t prefer = want * 2;
        if (prefer < 512) prefer = 512;
        if (prefer > space) prefer = space;
        if (prefer > want) to_read = prefer;
    }
    
    int32_t got = d->ops->read(d->buffer.data + d->buffer.len, 1, to_read, d->file_handle);
    if (got > 0) {
        d->buffer.len += got;
        d->stream_pos += got;
    } else if (got < 0) {
        return false;
    }
    
    return (d->buffer.len - d->buffer.pos) >= need;
}

static inline uint8_t amr_get_frame_size(amr_demuxer_t *d, uint8_t frame_hdr)
{
    if (frame_hdr & 0x80) {
        return 0;
    }
    uint8_t frame_type = (frame_hdr >> 3) & 0x0F;
    if (frame_type > 15) {
        return 0;
    }
    return d->frame_size_table[frame_type];
}

static uint32_t amr_scan_duration_precise(amr_demuxer_t *d)
{
    uint8_t frame_hdr;
    uint32_t frame_count = 0;
    uint32_t current_pos = d->header_size;
    uint64_t start_ms = os_mseconds();
    
    uint32_t saved_pos = d->stream_pos;
    
    if (d->ops->seek(d->file_handle, current_pos, SEEK_SET) != 0) {
        return 0;
    }
    
    while (1) {
        int ret = d->ops->read(&frame_hdr, 1, 1, d->file_handle);
        if (ret < 1) {
            break;
        }
        
        uint8_t frame_size = amr_get_frame_size(d, frame_hdr);
        if (frame_size == 0) {
            current_pos++;
            if (d->ops->seek(d->file_handle, current_pos, SEEK_SET) != 0) break;
            continue;
        }
        
        frame_count++;
        
        current_pos += frame_size;
        if (current_pos >= d->file_size) break;
        
        if (d->ops->seek(d->file_handle, current_pos, SEEK_SET) != 0) break;
        
        if (os_mseconds() - start_ms > 2000) {
            frame_count = 0;
            break;
        }
    }
    
    d->ops->seek(d->file_handle, saved_pos, SEEK_SET);
    
    if (frame_count > 0) {
        uint32_t duration = frame_count * 20;
        return duration;
    }
    
    return 0;
}

static uint32_t amr_estimate_duration(amr_demuxer_t *d)
{
    if (d->file_size <= 0 || d->header_size <= 0) {
        return 0;
    }
    
    uint32_t audio_data_size = d->file_size - d->header_size;
    uint32_t avg_size = d->avg_frame_size;
    
    if (d->ops->seek && d->stream_type == AVDEMUXER_STREAM_FILE) {
        uint8_t frame_hdr;
        uint32_t sample_frames = 0;
        uint32_t sample_bytes = 0;
        uint32_t current_pos = d->header_size;
        uint32_t saved_pos = d->stream_pos;
        
        if (d->ops->seek(d->file_handle, current_pos, SEEK_SET) == 0) {
            for (int i = 0; i < 100; i++) {
                if (d->ops->read(&frame_hdr, 1, 1, d->file_handle) < 1) break;
                
                uint8_t frame_size = amr_get_frame_size(d, frame_hdr);
                if (frame_size == 0) {
                    current_pos++;
                    if (d->ops->seek(d->file_handle, current_pos, SEEK_SET) != 0) break;
                    continue;
                }
                
                sample_frames++;
                sample_bytes += frame_size;
                current_pos += frame_size;
                
                if (d->ops->seek(d->file_handle, current_pos, SEEK_SET) != 0) break;
            }
            
            if (sample_frames > 0) {
                avg_size = sample_bytes / sample_frames;
                d->avg_frame_size = avg_size;
            }
        }
        
        d->ops->seek(d->file_handle, saved_pos, SEEK_SET);
    }
    
    if (avg_size == 0) {
        avg_size = (d->coder_type == AMR_WB) ? 40 : 24;
    }
    
    uint32_t estimated_frames = audio_data_size / avg_size;
    uint32_t duration = estimated_frames * 20;
    
    return duration;
}

static int32_t amr_demuxer_output_frame(amr_demuxer_t *d, uint8_t frame_type)
{
    uint16_t frame_len = d->frame_size_table[frame_type];
    
    if ((d->buffer.len - d->buffer.pos) < frame_len) {
        if (!amr_demuxer_ensure(d, frame_len)) {
            return 0;
        }
    }

    if (!d->info_initialized) {
        memset(&d->codec_info, 0, sizeof(d->codec_info));
        
        if (d->coder_type == AMR_WB) {
            d->codec_info.codec_id = AUDIO_CODEC_AMR_WB;
            d->codec_info.sample_rate = 16000;
            d->codec_info.frame_size = 320;
        } else {
            d->codec_info.codec_id = AUDIO_CODEC_AMR_NB;
            d->codec_info.sample_rate = 8000;
            d->codec_info.frame_size = 160;
        }
        
        d->codec_info.channels = 1;
        d->codec_info.ch_layout = AUDIO_CH_LAYOUT_MONO;
        d->info_initialized = 1;
        
        if (d->stream_type == AVDEMUXER_STREAM_FILE && 
            d->file_size > 0 && 
            d->file_size < (10 * 1024 * 1024)) {
            d->total_duration = amr_scan_duration_precise(d);
            if (d->total_duration > 0) {
                d->duration_precise = 1;
            } else {
                d->total_duration = amr_estimate_duration(d);
                d->duration_precise = 0;
            }
        } else {
            d->total_duration = amr_estimate_duration(d);
            d->duration_precise = 0;
        }
    } else if (d->total_duration == 0 && d->stream_type != AVDEMUXER_STREAM_FILE) {
        if (d->ops->ioctl) {
            d->ops->ioctl(d->owner, AVDEMUXER_GET_FILE_SIZE, (uint32)&d->file_size, 0);
        }
        if (d->file_size > 0 && d->header_size > 0) {
            d->total_duration = amr_estimate_duration(d);
        }
    }
    
    if (!d->duration_precise && d->file_size > 0 && d->sample_count < 20) {
        d->sample_total_size += frame_len;
        d->sample_count++;
        
        if (d->sample_count >= 5) {
            uint32_t avg_frame_size = d->sample_total_size / d->sample_count;
            if (avg_frame_size > 0) {
                d->avg_frame_size = avg_frame_size;
            }
            uint32_t audio_data_size = (uint32_t)(d->file_size - d->header_size);
            if (audio_data_size > 0 && avg_frame_size > 0) {
                uint32_t estimated_frames = audio_data_size / avg_frame_size;
                uint32_t new_duration = estimated_frames * 20;
                if (new_duration > 0 && new_duration != d->total_duration) {
                    d->total_duration = new_duration;
                }
            }
        }
    }
    
    struct framebuff *fb = msi_alloc_fb(d->owner, NULL, NULL, frame_len, 0, 0);
    if (!fb) {
        return -ENOMEM;
    }
    
    memcpy(fb->data, d->buffer.data + d->buffer.pos, frame_len);
    fb->mtype = MEDIA_DATA_AUDIO;
    fb->codec_info = &d->codec_info;
    fb->time = (uint32_t)(d->current_time_us / 1000);

    if (d->coder_type == AMR_WB) {
        fb->stype = AUDIO_CODEC_AMR_WB;  
    } else {
        fb->stype = AUDIO_CODEC_AMR_NB; 
    }

    d->current_time_us += 20000;
    d->ops->outFB(d->owner, fb);

    d->buffer.pos += frame_len;
    
    uint16_t remain = d->buffer.len - d->buffer.pos;
    if (remain > 0 && remain < 64) {
        memmove(d->buffer.data, d->buffer.data + d->buffer.pos, remain);
        d->buffer.len = remain;
        d->buffer.pos = 0;
    }

    return (int32_t)frame_len;
}

static void *amr_demuxer_init(void *hdl, const struct AVDemuxerOps *ops, void *hdr, uint32_t len, struct msi *owner)
{
    amr_demuxer_t *d = decoder_mem_zalloc(sizeof(amr_demuxer_t));
    if (!d) {
        return NULL;
    }

    d->owner = owner;
    d->file_handle = hdl;
    d->ops = ops;
    d->current_time_us = 0;
    d->info_initialized = 0;
    d->total_duration = 0;
    d->file_size = 0;
    d->avg_frame_size = 20;
    d->stream_type = AVDEMUXER_STREAM_FILE;

    uint32_t header_size = 0;
    if (len >= strlen(AMRWB_HEADER) && 
        memcmp(hdr, AMRWB_HEADER, strlen(AMRWB_HEADER)) == 0) {
        d->coder_type = AMR_WB;
        d->frame_size_table = amrwb_frame_size_table;
        header_size = strlen(AMRWB_HEADER);
        d->avg_frame_size = 40;
    }
    else if (len >= strlen(AMRNB_HEADER) && 
             memcmp(hdr, AMRNB_HEADER, strlen(AMRNB_HEADER)) == 0) {
        d->coder_type = AMR_NB;
        d->frame_size_table = amrnb_frame_size_table;
        header_size = strlen(AMRNB_HEADER);
        d->avg_frame_size = 32;
    }
    else {
        decoder_mem_free(d);
        return NULL;
    }

    d->header_size = header_size;

    if (len > header_size) {
        uint32_t data_len = len - header_size;
        if (data_len > AMR_BUF_SIZE) data_len = AMR_BUF_SIZE;
        memcpy(d->buffer.data, (uint8_t*)hdr + header_size, data_len);
        d->buffer.len = data_len;
        d->stream_pos = len;
    }

    if (ops->ioctl) {
        ops->ioctl(owner, AVDEMUXER_GET_FILE_SIZE, (uint32)&d->file_size, 0);
        d->stream_type = ops->ioctl(owner, AVDEMUXER_GET_STREAM_TYPE, 0, 0);
        
        if (d->file_size > d->header_size) {
            uint32_t data_size = d->file_size - d->header_size;
            uint32_t est_frames = data_size / d->avg_frame_size;
            d->total_duration = est_frames * 20;
        }
    }

    return d;
}

static int32_t amr_demuxer_release(void *c)
{
    amr_demuxer_t *d = (amr_demuxer_t *)c;
    decoder_mem_free(d);
    return 0;
}

static uint32_t amr_demuxer_calc_seek_pos(amr_demuxer_t *d, uint32_t time_ms)
{
    if (d->total_duration > 0 && time_ms > d->total_duration) {
        time_ms = d->total_duration;
    }
    
    uint32_t target_frame = time_ms / 20;  
    
    uint32_t avg_frame = (d->avg_frame_size > 0) ? d->avg_frame_size : 32; 
    uint32_t est_pos = d->header_size + target_frame * avg_frame;
    
   if (d->file_size > 0 && est_pos >= d->file_size) {
        est_pos = (d->file_size > d->header_size + 10) ? 
                  (d->file_size - 10) : d->header_size;
    }
    
    return est_pos;
}

static int32_t amr_demuxer_do_seek(void *c, uint32_t time_ms)
{
    amr_demuxer_t *d = (amr_demuxer_t *)c;
    uint32_t pos;

    if (!d || !d->ops || !d->ops->seek) {
        return -1;
    }

    if (d->total_duration > 0 && time_ms > d->total_duration) {
        time_ms = d->total_duration;
    }
    
    pos = amr_demuxer_calc_seek_pos(d, time_ms);

    if (d->ops->seek(d->file_handle, pos, SEEK_SET) != 0) {
        return -1;
    }

    d->buffer.len = 0;
    d->buffer.pos = 0;
    d->stream_pos = pos;
    d->current_time_us = (uint64_t)time_ms * 1000;
    
    return 0;
}

static int32_t amr_demuxer_do_demux(void *c)
{
    amr_demuxer_t *d = (amr_demuxer_t *)c;
    
    if (!d || !d->ops) {
        return -EINVAL;
    }

    if(d->owner->fb_limits.counter == 0){
        return -ENOMEM;
    }

    if (d->file_size > 0 && d->stream_pos >= d->file_size) {
        return 0;
    }

    uint32_t remain = d->buffer.len - d->buffer.pos;
    if (remain < 1) {
        if (!amr_demuxer_ensure(d, 512)) {
            return 0;
        }
        remain = d->buffer.len - d->buffer.pos;
        if (remain < 1) {
            return 0;
        }
    }

    uint8_t *frame_hdr = d->buffer.data + d->buffer.pos;
    uint8_t frame_type = (frame_hdr[0] >> 3) & 0x0F;
    
    uint8_t max_valid_type = (d->coder_type == AMR_WB) ? 9 : 8;
    if (frame_type > max_valid_type && frame_type != 15) {
        d->buffer.pos++;
        uint16_t new_remain = d->buffer.len - d->buffer.pos;
        if (new_remain > 0 && new_remain < 64) {
            memmove(d->buffer.data, d->buffer.data + d->buffer.pos, new_remain);
            d->buffer.len = new_remain;
            d->buffer.pos = 0;
        }
        return -EAGAIN;
    }

    uint8_t frame_size = amr_get_frame_size(d, frame_type);
    if (frame_size == 0) {
        d->buffer.pos++;
        return -EAGAIN;
    }

    if (remain < frame_size) {
        if (!amr_demuxer_ensure(d, frame_size)) {
            return 0;
        }
    }

    int32_t ret = amr_demuxer_output_frame(d, frame_type);
    return ret;
}

static int32_t amr_demuxer_ioctl(void *c, uint32_t cmd, uint32_t param1, uint32_t param2)
{
    amr_demuxer_t *d = (amr_demuxer_t *)c;
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

const __avdemuxer struct AVDemuxer amr_demuxer = {
    .type         = MEDIA_CONTAINER_AMR,
    .name         = "amr_demuxer",
    .init         = amr_demuxer_init,
    .release      = amr_demuxer_release,
    .do_seek      = amr_demuxer_do_seek,
    .do_demux     = amr_demuxer_do_demux,
    .ioctl        = amr_demuxer_ioctl,
};