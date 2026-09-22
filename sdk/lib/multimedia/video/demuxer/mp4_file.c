#include "mp4.h"

void mp4_mdat_goback(struct mp4_context *c)
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

int32_t mp4_mdat_seek(struct mp4_context *c, uint64_t offset)
{
    uint32_t count = RB_COUNT(&c->io_buf);
    if (!c->iobuf_dis && count > 0 && offset >= c->offset && offset <= c->offset + count) {
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

uint32_t mp4_seek_keyfrm(struct mp4_context *c, uint32_t time_ms)
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

// 将track seek 到指定的位置，更新next sample信息
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

int32_t mp4_delay_seek(struct mp4_context *c, uint32_t time_ms)
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

struct mp4_track *mp4_file_select_next_track(struct mp4_context *c)
{
    struct mp4_track *selected = NULL;
    uint32_t min_time = UINT32_MAX;

    c->output_blocked = 0;

    for (int i = 0; i < c->track_cnt; i++) {
        struct mp4_track *t = c->tracks[i];
        if (t && t->next_sample_index <= t->stsz.total_count) {
            if (mp4_track_output_blocked(c, t->mtype)) {
                c->output_blocked = 1; //该路配额耗尽, 跳过
                continue;
            }
            if (t->next_sample_time >= min_time) {
                continue;
            }
            min_time = t->next_sample_time;
            selected = t;
        }
    }
    return selected;
}

int32_t mp4_file_mdat_hdl(struct mp4_context *c)
{
    if (c->cur_track >= 0) {
        if (mp4_track_load_stsz(c, c->tracks[c->cur_track], 0)) {
            return -EAGAIN;
        }
        if (mp4_track_load_stco(c, c->tracks[c->cur_track], 0)) {
            return -EAGAIN;
        }
    }

    if (c->buffering) {
        return 0;
    }
    if (atomic_read(&c->owner->fb_limits) == 0) {
        return -EAGAIN;
    }

    if (c->cur_frame) {
        uint32_t len;
        struct mp4_track *track = c->tracks[c->cur_track];
        if (c->offset != track->next_sample_offset) {
            mp4_mdat_seek(c, track->next_sample_offset);
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
        struct mp4_track *track = mp4_file_select_next_track(c);
        if (!track) {
            if (c->output_blocked) {
                return -EAGAIN;
            }
            c->ops->seek(c->file, 0, SEEK_END);
            mp4_warn("cur offset:%llu, no next sample!\r\n", c->offset);
            return 0;
        }

        c->cur_track = track->track_idx;
        mp4_mdat_seek(c, track->next_sample_offset);
        c->cur_frame = mp4_track_alloc_fb(c, track);
        if (!c->cur_frame) {
            return -ENOMEM;
        }
        return -EAGAIN; //继续加载下一个sample数据
    }
}