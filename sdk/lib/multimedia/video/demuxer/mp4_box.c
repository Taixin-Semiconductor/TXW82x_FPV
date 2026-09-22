#include "mp4.h"

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

int32_t mp4_box_avail(struct mp4_context *c)
{
    return RB_COUNT(&c->io_buf);
}
static int32_t mp4_box_read(struct mp4_context *c, struct mp4_box *box, uint8_t *buf, uint32_t len)
{
    if (mp4_box_avail(c) < len) {
        return -EAGAIN;
    }

    uint32_t avail = RB_COUNT(&c->io_buf);
    uint32_t count = min(len, avail);
    count = rbuffer_get(&c->io_buf, buf, count);
    c->offset += count;
    mp4_box_read_size(c, box, count);
    return count;
}
int32_t mp4_box_fill(struct mp4_context *c, uint32_t need_size)
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

// mp4_mdat_goback 在 mp4_file.c 中定义

// mp4_mdat_read 在 mp4_demux.c 中定义

// mp4_mdat_seek 在 mp4_file.c 中定义

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 这里不需要 mp4_track_is_active 等，因为它们在 mp4_demux.c 中

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
int32_t mp4_box_stco_hdl(struct mp4_context *c)
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
int32_t mp4_box_co64_hdl(struct mp4_context *c)
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
int32_t mp4_box_stsz_hdl(struct mp4_context *c)
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
    if (c->stream_type == AVDEMUXER_STREAM_FILE)
        return mp4_file_mdat_hdl(c);
    else
        return mp4_net_mdat_hdl(c);
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

int32_t mp4_box_push(struct mp4_context *c, box_hdl hdl, uint32_t box_tag, struct mp4_box *parent)
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

int mp4_detect_box(struct mp4_context *c)
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

int32_t mp4_parse_box(struct mp4_context *c)
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