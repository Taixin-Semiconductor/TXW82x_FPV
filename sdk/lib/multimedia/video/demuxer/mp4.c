#include "mp4.h"

int32_t mp4_do_demux(void *ctx)
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

int32_t mp4_do_seek(void *ctx, uint32_t time_ms)
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

void *mp4_init(void *hdl, const struct AVDemuxerOps *ops, void *hdr, uint32_t len, struct msi *owner)
{
    struct mp4_context *c = decoder_mem_calloc(1, sizeof(struct mp4_context));
    if (!c) {
        return NULL;
    }
    c->fb_quota = decoder_mem_zalloc(sizeof(struct mp4_fb_quota));
    if (c->fb_quota == NULL) {
        decoder_mem_free(c);
        return NULL;
    }
    atomic_set(&c->fb_quota->users, 1); 

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

int32_t mp4_release(void *ctx)
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
        c->cur_frame = NULL;
    }
    if (c->fb_quota) {
        mp4_fb_quota_put(c->fb_quota);
        c->fb_quota = NULL;
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

int mp4_ioctl(void *ctx, uint32_t cmd, uint32_t param1, uint32_t param2)
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