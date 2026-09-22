#include "mp4.h"

void mp4_fb_quota_put(struct mp4_fb_quota *q)
{
    if (q && atomic_dec_and_test(&q->users)) {
        decoder_mem_free(q);
    }
}

static void mp4_fb_free(struct framebuff *fb, void *priv)
{
    struct mp4_fb_quota *q = (struct mp4_fb_quota *)priv;
    if (fb->mtype == MEDIA_DATA_VIDEO) {
        atomic_inc(&q->video_left);
        if (atomic_read(&q->video_left) > q->video_max) {
            atomic_set(&q->video_left, q->video_max); 
        }
    } else if (fb->mtype == MEDIA_DATA_AUDIO) {
        atomic_inc(&q->audio_left);
        if (atomic_read(&q->audio_left) > q->audio_max) {
            atomic_set(&q->audio_left, q->audio_max);
        }
    }
    mp4_fb_quota_put(q);         
    decoder_mem_free(fb);        
}

static void mp4_fb_quota_init(struct mp4_context *c)
{
    struct mp4_fb_quota *q = c->fb_quota;
    uint32 total, reserve;
    if (q == NULL || q->inited) {
        return;
    }
    total = atomic_read(&c->owner->fb_limits);
    if (total < 4) {
        total = 4;
    }
    reserve = (total >= 12) ? 1 : 0;
    q->audio_max = (total - reserve) / 2;
    if (q->audio_max < 2) {
        q->audio_max = 2; //至少 2 个可用
    }
    q->video_max = total - reserve - q->audio_max;
    if (q->video_max < 2) {
        q->video_max = 2;
    }
    atomic_set(&q->video_left, q->video_max);
    atomic_set(&q->audio_left, q->audio_max);
    q->inited = 1;
    mp4_dbg("fb quota init: video %d, audio %d (total %d)\r\n",
             q->video_max, q->audio_max, total);
}
int mp4_track_output_blocked(struct mp4_context *c, uint8 mtype)
{
    struct mp4_fb_quota *q = c->fb_quota;

    if (q == NULL) {
        return 0;
    }
    mp4_fb_quota_init(c);

    if (mtype == MEDIA_DATA_VIDEO) {
        return atomic_read(&q->video_left) <= 0;
    }
    if (mtype == MEDIA_DATA_AUDIO) {
        return atomic_read(&q->audio_left) <= 0;
    }
    return 0;
}

int mp4_track_is_active(struct mp4_context *c, uint32_t track_idx)
{
    if (track_idx == c->video_track) { return 1; }
    if (track_idx == c->audio_track) { return 1; }
    if (track_idx == c->subtitle_track) { return 1; }
    return 0;
}

struct mp4_track *mp4_track_find(struct mp4_context *c, uint8_t track_id)
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

uint32_t mp4_track_get_key_sample(struct mp4_context *c, struct mp4_track *track, uint32_t sample_idx)
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

uint32_t mp4_track_get_sample_chunk(struct mp4_context *c, struct mp4_track *track, 
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

uint32_t mp4_track_get_sample_size(struct mp4_context *c, struct mp4_track *track, uint32_t sample_idx, uint8_t sw_cache)
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

uint64_t mp4_track_get_sample_offset(struct mp4_context *c, struct mp4_track *track, uint32_t sample_idx, uint8_t sw_cache)
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

uint32_t mp4_track_get_sample_time(struct mp4_context *c, struct mp4_track *track, uint32_t sample_idx)
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

uint32_t mp4_track_seek_sample(struct mp4_context *c, struct mp4_track *track, uint32_t time_ms)
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

void mp4_track_update_next_sample(struct mp4_context *c, struct mp4_track *track, uint32_t next_index)
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

struct framebuff *mp4_track_alloc_fb(struct mp4_context *c, struct mp4_track *track)
{
    struct framebuff *fb = NULL;
    if (mp4_track_output_blocked(c, track->mtype)) {
        return NULL;
    }

    if (track->mtype == MEDIA_DATA_AUDIO && track->stype == AUDIO_CODEC_AAC) {
        fb = msi_alloc_fb(c->owner, NULL, NULL, track->next_sample_size + 7, 0, 0);
        if (fb && track->codec_data) {
            esds_info_t *info = (esds_info_t *)track->codec_data;
            mp4_track_fill_aac_ADTS(info->dsi_data, fb->data, track->next_sample_size);
            c->cur_frame_len = 7; 
        }
    } else {
        fb = msi_alloc_fb(c->owner, NULL, NULL, track->next_sample_size, 0, 0);
    }

    if (fb) {
        fb->mtype = track->mtype;
        fb->stype = track->stype;
        fb->free      = (mfree_cb_t)mp4_fb_free;
        fb->free_priv = c->fb_quota;
        atomic_inc(&c->fb_quota->users); 

        if (track->mtype == MEDIA_DATA_VIDEO) {
            atomic_dec(&c->fb_quota->video_left);
        } else if (track->mtype == MEDIA_DATA_AUDIO) {
            atomic_dec(&c->fb_quota->audio_left);
        }
    }
    return fb;
}

static int32_t mp4_track_is_keyfrm(struct mp4_context *c, struct mp4_track *track)
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

int mp4_track_output_fb(struct mp4_context *c, struct mp4_track *track)
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

struct mp4_track *mp4_select_next_track(struct mp4_context *c)
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

int32_t mp4_mdat_read(struct mp4_context *c, struct mp4_track *track, uint8_t *dst, uint32_t len)
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

void mp4_free_codec_data(struct mp4_track *track)
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


//  加载另一批stco信息
int32_t mp4_track_load_stco(struct mp4_context *c, struct mp4_track *track, uint32_t next_chunk_idx)
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
int32_t mp4_track_load_stsz(struct mp4_context *c, struct mp4_track *track, uint32_t next_sample_idx)
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