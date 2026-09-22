#include "mp4.h"

int32_t mp4_net_mdat_hdl(struct mp4_context *c)
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
        if (c->offset < track->next_sample_offset) {
            len = track->next_sample_offset - c->offset;
            return mp4_mdat_read(c, track, NULL, len);
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
        /* 与本地文件路径同语义: 配额耗尽的轨跳过优先读另一路, 全部暂时不可读返回-EAGAIN立即重试(0会被上层当作EOF判定) */
        struct mp4_track *selected = NULL;
        uint64_t min_offset = UINT64_MAX;

        c->output_blocked = 0;
        for (int i = 0; i < c->track_cnt; i++) {
            struct mp4_track *t = c->tracks[i];
            if (t && t->next_sample_index <= t->stsz.total_count) {
                if (mp4_track_output_blocked(c, t->mtype)) {
                    c->output_blocked = 1; //该路配额耗尽, 跳过
                    continue;
                }
                if (t->next_sample_offset < min_offset) {
                    min_offset = t->next_sample_offset;
                    selected   = t;
                }
            }
        }
        if (!selected) {
            if (c->output_blocked) {
                return -EAGAIN;
            }
            c->ops->seek(c->file, 0, SEEK_END);
            mp4_warn("cur offset:%llu, no next sample!\r\n", c->offset);
            return 0;
        }

        c->cur_track = selected->track_idx;
        c->cur_frame = mp4_track_alloc_fb(c, selected);
        if (!c->cur_frame) {
            return -ENOMEM;
        }
        return -EAGAIN; //继续加载下一个sample数据
    }
}