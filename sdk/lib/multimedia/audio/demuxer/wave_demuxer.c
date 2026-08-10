#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/multimedia/AVContainer.h"
#include "../audio_coder.h"

#define WAVE_BUF_SIZE           4096
#define WAVE_DEMUXER_SFB_NUM    4
#define WAVE_HEADER_SIZE        44

typedef struct {
    uint8_t data[WAVE_BUF_SIZE];
    uint16_t len;
    uint16_t pos;
} wave_buf_t;

/**
 * @brief WAV Demuxer 上下文结构体
 * @note 管理 WAV/PCM 格式音频的解封装状态
 */
typedef struct {
    struct msi                 *owner;              // 上层播放器实例
    const struct AVDemuxerOps  *ops;                // 文件操作接口
    void                      *file_handle;         // 文件/URL句柄
    wave_buf_t                 buffer;              // 输入数据缓冲区
    txAudioInfo_t         codec_info;          // 编码信息
    uint8_t                    codec_id;            // 编码格式 (PCM/ALAW/ULAW)
    uint32_t                   samplerate;          // 采样率 
    uint16_t                   channels;            // 声道数
    uint16_t                   bits_per_sample;     // 采样位数
    uint16_t                   block_align;         // 块对齐
    uint32_t                   byte_rate;           // 字节率
    uint32_t                   data_offset;         // 音频数据起始偏移
    uint32_t                   data_size;           // 音频数据大小
    uint32_t                   stream_pos;          // 当前文件位置
    uint8_t                    info_initialized : 1;// 是否已初始化
    uint32_t                   total_duration;      // 总时长 (毫秒)
    int64_t                    file_size;           // 文件总大小
    int32_t                    stream_type;         // 流类型
    uint64_t                   current_time_us;     // 当前播放时间 (微秒)
} wave_demuxer_t;

static bool wave_demuxer_ensure(wave_demuxer_t *d, uint16_t need)
{
    uint16_t have = d->buffer.len - d->buffer.pos;

    if (have >= need) {
        return true;
    }

    uint16_t want = need - have;
    if (d->buffer.len + want > WAVE_BUF_SIZE) {
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

static int32_t wave_demuxer_output_chunk(wave_demuxer_t *d, void *priv, uint16_t chunk_size)
{
    if (!d->info_initialized) {
        memset(&d->codec_info, 0, sizeof(d->codec_info));
        d->codec_info.codec_id = d->codec_id;
        d->codec_info.sample_rate = d->samplerate;
        d->codec_info.channels = d->channels;
        d->codec_info.ch_layout = (d->channels == 1) ? AUDIO_CH_LAYOUT_MONO : AUDIO_CH_LAYOUT_STEREO;
        d->codec_info.bits_per_coded_sample = d->bits_per_sample;
        d->codec_info.block_align = d->block_align;

        d->codec_info.bit_rate = d->byte_rate * 8;
        d->info_initialized = 1;

        if (d->data_size > 0 && d->byte_rate > 0) {
            d->total_duration = (uint32_t)(((uint64_t)d->data_size * 1000) / d->byte_rate);
        } else if (d->file_size > d->data_offset && d->byte_rate > 0) {
            uint32_t audio_data_size = d->file_size - d->data_offset;
            d->total_duration = (uint32_t)(((uint64_t)audio_data_size * 1000) / d->byte_rate);
        }
    }
    
    if (d->total_duration == 0 && d->stream_type != AVDEMUXER_STREAM_FILE) {
        if (d->ops->ioctl) {
            int64_t new_file_size = 0;
            d->ops->ioctl(d->owner, AVDEMUXER_GET_FILE_SIZE, (uint32)&new_file_size, 0);
            if (new_file_size > 0 && new_file_size != d->file_size) {
                d->file_size = new_file_size;
                if (d->file_size > d->data_offset && d->byte_rate > 0) {
                    uint32_t audio_data_size = d->file_size - d->data_offset;
                    d->total_duration = (uint32_t)(((uint64_t)audio_data_size * 1000) / d->byte_rate);
                }
            }
        }
    }

    uint16_t have = d->buffer.len - d->buffer.pos;
    uint16_t to_output = (chunk_size < have) ? chunk_size : have;

    if (to_output == 0) {
        return -EAGAIN;
    }
    
    struct framebuff *fb = msi_alloc_fb(d->owner, NULL, NULL, to_output, 0, 0);
    if (!fb) {
        //_os_printf("F");
        return -ENOMEM;
    }
    
    memcpy(fb->data, d->buffer.data + d->buffer.pos, to_output);
    fb->codec_info  = &d->codec_info;
    fb->mtype = MEDIA_DATA_AUDIO;
    fb->stype = d->codec_id;

    if (d->byte_rate > 0) {
        fb->time = (uint32_t)(d->current_time_us / 1000);
        d->current_time_us += ((uint64_t)to_output * 1000000) / d->byte_rate;
    } else {
        fb->time = 0;
    }



    d->ops->outFB(d->owner, fb);

    d->buffer.pos += to_output;

    uint16_t remain = d->buffer.len - d->buffer.pos;
    if (remain > 0 && remain < 512) {
        memmove(d->buffer.data, d->buffer.data + d->buffer.pos, remain);
        d->buffer.len = remain;
        d->buffer.pos = 0;
    }

    return (int32_t)to_output;
}

static void *wave_demuxer_init(void *hdl, const struct AVDemuxerOps *ops, void *hdr, uint32_t len, struct msi *owner)
{

    if (!hdr || len < WAVE_HEADER_SIZE) {
        return NULL;
    }

    wave_demuxer_t *d = decoder_mem_zalloc(sizeof(wave_demuxer_t));
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

    uint8_t *header = (uint8_t*)hdr;

    if (memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) {
        decoder_mem_free(d);
        return NULL;
    }

    if (memcmp(header + 12, "fmt ", 4) != 0) {
        decoder_mem_free(d);
        return NULL;
    }

    //uint32_t riff_size = header[4] | (header[5] << 8) | (header[6] << 16) | (header[7] << 24);

    uint16_t audio_format = header[20] | (header[21] << 8);
    d->channels = header[22] | (header[23] << 8);
    d->samplerate = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);
    d->bits_per_sample = header[34] | (header[35] << 8);
    
    uint32_t fmt_chunk_size = header[16] | (header[17] << 8) | (header[18] << 16) | (header[19] << 24);

    switch (audio_format) {
        case 0x0001:
            d->codec_id = AUDIO_CODEC_PCM_S16LE;
            break;
        case 0x0006:
            d->codec_id = AUDIO_CODEC_ALAW;
            break;
        case 0x0007:
            d->codec_id = AUDIO_CODEC_ULAW;
            break;
        default:
            decoder_mem_free(d);
            return NULL;
    }

    d->block_align = d->channels * (d->bits_per_sample / 8);
    if (d->block_align == 0) {
        d->block_align = 1;
    }

    if (len < 44) {
        decoder_mem_free(d);
        return NULL;
    }

    d->stream_type = AVDEMUXER_STREAM_FILE;
    if (ops && ops->ioctl) {
        ops->ioctl(owner, AVDEMUXER_GET_FILE_SIZE, (uint32)&d->file_size, 0);
        d->stream_type = ops->ioctl(owner, AVDEMUXER_GET_STREAM_TYPE, 0, 0);
    }

    d->data_offset = 0;
    d->data_size = 0;

    uint32_t offset = 20 + fmt_chunk_size;
    if (offset % 2) offset++;
    
    while (offset + 8 <= len) {
        char chunk_name[5] = {0};
        memcpy(chunk_name, header + offset, 4);
        uint32_t chunk_size = header[offset + 4] | (header[offset + 5] << 8) |
                             (header[offset + 6] << 16) | (header[offset + 7] << 24);

        if (memcmp(header + offset, "data", 4) == 0) {
            d->data_size = chunk_size;
            d->data_offset = offset + 8;
            break;
        }
        
        offset += 8 + chunk_size;
        if (offset % 2) offset++;

        if (chunk_size > 0x1000000) {
            break;
        }
    }

    if (d->data_offset == 0) {
        d->data_offset = 44;
        d->data_size = header[40] | (header[41] << 8) | (header[42] << 16) | (header[43] << 24);
    }

    if (d->file_size > 0 && d->data_offset > 0) {
        uint32_t expected_size = d->file_size - d->data_offset;
        
        if (d->data_size == 0 || d->data_size > expected_size || d->data_size < expected_size / 2) {
            d->data_size = expected_size;
        }
    }
    
    d->byte_rate = d->samplerate * d->block_align;
    if (d->byte_rate > 0 && d->data_size > 0) {
        d->total_duration = (uint32_t)(((uint64_t)d->data_size * 1000) / d->byte_rate);
    }

    if (len > d->data_offset) {
        uint32_t data_len = len - d->data_offset;
        if (data_len > WAVE_BUF_SIZE) data_len = WAVE_BUF_SIZE;
        memcpy(d->buffer.data, header + d->data_offset, data_len);
        d->buffer.len = data_len;
        d->stream_pos = len;
    }

    return d;
}

static int32_t wave_demuxer_release(void *c)
{
    wave_demuxer_t *d = (wave_demuxer_t *)c;
    decoder_mem_free(d);
    return 0;
}

static int32_t wave_demuxer_do_seek(void *c, uint32_t time_ms)
{
    wave_demuxer_t *d = (wave_demuxer_t *)c;

    if (!d || !d->ops || !d->ops->seek) {
        return -1;
    }

    if (d->byte_rate == 0) {
        return -1;
    }

    if (d->total_duration > 0 && time_ms > d->total_duration) {
        time_ms = d->total_duration;
    }
	
    uint32_t byte_offset = ((uint64_t)time_ms * d->byte_rate) / 1000;

    uint32_t actual_data_size = d->data_size;
    if (actual_data_size == 0 && d->file_size > d->data_offset) {
        actual_data_size = d->file_size - d->data_offset;
    }

    if (byte_offset > actual_data_size) {
        byte_offset = actual_data_size;
    }

    if (d->block_align > 1) {
        uint32_t aligned = (byte_offset / d->block_align) * d->block_align;
        byte_offset = aligned;
    }

    uint32_t file_pos = d->data_offset + byte_offset;

    int32_t seek_ret = d->ops->seek(d->file_handle, file_pos, SEEK_SET);
    if (seek_ret != 0) {
        return -1;
    }

    // 重置 demuxer buffer 状态，立即返回
    // 数据由 do_demux 通过 ensure 机制自然读取
    d->buffer.len = 0;
    d->buffer.pos = 0;
    d->current_time_us = (uint64_t)time_ms * 1000;

    return 0;
}

static int32_t wave_demuxer_do_demux(void *c)
{
    wave_demuxer_t *d = (wave_demuxer_t *)c;
    
    if (!d || !d->ops) return -EINVAL;

    if (d->owner->fb_limits.counter == 0) {
        return -ENOMEM;
    }
    
    uint16_t have = d->buffer.len - d->buffer.pos;
    if (d->buffer.pos > 0 && have < WAVE_BUF_SIZE / 4) {
        memmove(d->buffer.data, d->buffer.data + d->buffer.pos, have);
        d->buffer.len = have;
        d->buffer.pos = 0;
    }

    if (d->buffer.len < WAVE_BUF_SIZE) {
        uint16_t want = WAVE_BUF_SIZE - d->buffer.len;
        int32_t got = d->ops->read(d->buffer.data + d->buffer.len, 1, want, d->file_handle);
        if (got > 0) {
            d->buffer.len += got;
            d->stream_pos += got;
        }
    }

    uint16_t chunk_size = 1024;
    have = d->buffer.len - d->buffer.pos;

    if (have == 0) {
        if (d->ops->eof && d->ops->eof(d->file_handle)) {
            return 0;
        }
        return 0;
    }

    if (d->stream_type != AVDEMUXER_STREAM_FILE) {
        if (have < chunk_size) {
            return 0;
        }
    } else {
        if (have < chunk_size) {
			if (d->ops->eof && d->ops->eof(d->file_handle)) {
                return 0;  
            }
            chunk_size = have;
        }
    }

    return wave_demuxer_output_chunk(d, d->owner, chunk_size);
}

static int32_t wave_demuxer_ioctl(void *c, uint32_t cmd, uint32_t param1, uint32_t param2)
{
    wave_demuxer_t *d = (wave_demuxer_t *)c;
    
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

const __avdemuxer struct AVDemuxer wave_demuxer = {
    .type         = MEDIA_CONTAINER_WAV,
    .name         = "wave_demuxer",
    .init         = wave_demuxer_init,
    .release      = wave_demuxer_release,
    .do_seek      = wave_demuxer_do_seek,
    .do_demux     = wave_demuxer_do_demux,
    .ioctl        = wave_demuxer_ioctl,
};
