#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "vdec_msi.h"
#include "vdec_wkq.h"
#include "stream_define.h"

/* video decoder msi 模块 共用代码 */

/* msi组件实例化数量控制 */
static uint8 vdec_msi_req(struct video_dec_mgr *mgr)
{
    uint8 i = 0;
    uint32 flags = disable_irq();
    for (i = 0; i < mgr->chan_cnt; i++) {
        if (mgr->chans[i] == NULL) {
            mgr->chans[i] = (struct video_dec_msi *)(-1);
            break;
        }
    }
    enable_irq(flags);
    return i;
}

static void vdec_msi_idle(struct video_dec_mgr *mgr, uint8 id)
{
    uint32 flags = disable_irq();
    mgr->chans[id] = NULL;
    enable_irq(flags);
}

static void vdec_msi_free(struct video_dec_msi *dec)
{
    if (dec) {
        msi_del_output(dec->msi, NULL, NULL, NULL);
        fb_put(dec->ply_fb);
        fb_put(dec->req.fb);
        fb_put(dec->req.fb_out);
        fbq_destroy(&dec->plyQ);
        msi_put(dec->avsync);

        if (dec->chan) {
            vcodec_close(dec->mgr->dev, dec->chan);
            dec->chan = NULL;
        }

        if (dec->id < dec->mgr->chan_cnt && dec->mgr->chans[dec->id] == dec) {
            vdec_msi_idle(dec->mgr, dec->id);
        }

        os_mutex_del(&dec->lock);
        decoder_mem_free(dec);
    }
}

static void vdec_msi_play_end(struct video_dec_msi *dec)
{
    if (dec->eof && !fbq_count(&dec->msi->fbQ) && dec->msi->fb_limits.counter == dec->mgr->plyQ_size) {
        dec->ply_end = 1;
        msi_notify(dec->msi, MSI_CMD_PLAY_END, 0, 0);//所有的数据已播放完成
    }
}

static void vdec_msi_need_more(struct video_dec_msi *dec)
{
    if (!dec->eof && !fbq_count(&dec->msi->fbQ) && dec->msi->fb_limits.counter == dec->mgr->plyQ_size) {
        msi_notify(dec->msi, MSI_CMD_NEED_MORE_DATA, 0, 0);
        vdec_dbg("%s: Need More Data!\r\n", dec->msi->name);
    }
}

//音视频同步检测: 返回需要等待的时间
int32 vdec_msi_check_avsync(struct video_dec_msi *dec, struct framebuff *fb)
{
    uint32 plytime = 0;
    msi_cmd2(dec->avsync, MSI_CMD_GET_PLAYTIME, (uint32)&plytime, 0);

    if (plytime) {
        return fb->time > (plytime+5) ? fb->time - (plytime+5) : 0;
    } else {
        if (dec->start_plytime == 0) {
            dec->start_plytime = os_mseconds() - fb->time;
        }
        uint32 diff = os_mseconds() - dec->start_plytime;
        return fb->time > diff ? fb->time - diff : 0;
    }
    return 0;
}

static void vdec_msi_decode_cb(struct vcodec_decode_req *req, int32 status)
{
    struct video_dec_msi *dec = (struct video_dec_msi *)req->priv;
    dec->decode_status = status;
    vdec_work_run(&dec->work);
}

static void vdec_msi_decode_fb(struct video_dec_msi *dec, struct framebuff *fb)
{
    struct framebuff *plyfb;

    if (dec == NULL || fb == NULL) {
        fb_put(fb);
        return;
    }

    plyfb = msi_alloc_fb(dec->msi, NULL, NULL, 0, sizeof(txYuvInfo_t), 0);
    if (!plyfb) {
        vdec_err("%s: alloc fb failed! fb_limits:%d\n", dec->msi->name, dec->msi->fb_limits.counter);
        fb_put(fb);
        return;
    }

    os_memset(&dec->vdd, 0, sizeof(dec->vdd));
    os_memset(&dec->req.info, 0, sizeof(dec->req.info));
    msi_cmd2(dec->msi, MSI_CMD_GET_VDD_RECT, (uint32)(&dec->vdd), 0);
    dec->req.priv = dec;
    dec->req.chan = dec->chan;
    dec->req.fb   = fb;
    dec->req.fb_out = plyfb;
    dec->req.done = vdec_msi_decode_cb;
    dec->req.info.target_height = dec->vdd.height ? dec->vdd.height : 480;
    dec->req.info.target_width  = dec->vdd.width ? dec->vdd.width : 800;
    dec->decoding = 1;
    dec->decode_status = 1;
    if (vcodec_decode(dec->mgr->dev, &dec->req)) {
        dec->decoding = 0;
        dec->decode_status = 0;
        dec->req.fb = NULL;
        dec->req.fb_out = NULL;
        fb_put(fb);
        fb_put(plyfb);
        vdec_err("video decode request fail!\r\n");
    }
}

static void vdec_msi_decode_result(struct video_dec_msi *dec)
{
    if (dec->decoding && dec->decode_status != 1) {
        if (dec->decode_status == RET_OK && dec->req.fb_out->len) {
            txYuvInfo_t *YUVinfo = (txYuvInfo_t *)dec->req.fb_out->codec_info;
            os_memset(YUVinfo, 0, sizeof(txYuvInfo_t));
            YUVinfo->width = dec->req.info.actual_width;
            YUVinfo->height = dec->req.info.actual_height;
            YUVinfo->x = dec->vdd.x;
            YUVinfo->y = dec->vdd.y;
            YUVinfo->y_off = dec->req.info.y_off;
            YUVinfo->u_off = dec->req.info.u_off;
            YUVinfo->v_off = dec->req.info.v_off;
            dec->req.fb_out->time  = dec->req.fb->time;
            dec->req.fb_out->keyfrm = 0;
            dec->req.fb_out->last  = dec->req.fb->last;
            dec->req.fb_out->mtype = F_YUV;
            dec->req.fb_out->stype = FSTYPE_YUV_P0;
            fbq_enqueue(&dec->plyQ, dec->req.fb_out, 0);
        } else {
            fb_put(dec->req.fb_out);
        }
        fb_put(dec->req.fb);
        dec->req.fb = NULL;
        dec->req.fb_out = NULL;
        dec->decoding = 0;
        dec->decode_status = 0;
    }
}

static int32 vdec_msi_output_plyfb(struct video_dec_msi *dec)
{
    uint16 ply_delay = dec->interval_time;
    struct framebuff *plyfb = dec->ply_fb ? dec->ply_fb : fbq_dequeue(&dec->plyQ, 0);
    while (plyfb) {
        ply_delay = vdec_msi_check_avsync(dec, plyfb);
        if (ply_delay == 0) {
            uint32 playtime = plyfb->time;
            if (msi_output_fb(dec->msi, plyfb, 1) == 0) {
                dec->ply_fb = plyfb;
                ply_delay = dec->interval_time;
                break;
            } else {
                dec->play_time = playtime;
                dec->ply_fb = NULL;
            }
        } else { //播放时间未到
            dec->ply_fb = plyfb;
            break;
        }
        plyfb = fbq_dequeue(&dec->plyQ, 0);
    }
    return ply_delay;
}

static int32 vdec_msi_decode_next(struct video_dec_msi *dec)
{
    uint16 dec_delay = 0;
    uint8  decQ_cnt;

    if (!dec->decoding && dec->msi->fb_limits.counter) {
        decQ_cnt = fbq_count(&dec->msi->fbQ);
        if (decQ_cnt > 0) {
            vdec_msi_decode_fb(dec, msi_get_fb(dec->msi, 0));
        } else {
            dec_delay = dec->interval_time;
        }
    } else {
        dec_delay = dec->interval_time; //播放队列已满
    }
    return dec_delay;
}

static int32 vdec_msi_run(struct video_dec_msi *dec)
{
    //处理解码结果
    vdec_msi_decode_result(dec);

    //处理待播放数据
    uint16 ply_delay = vdec_msi_output_plyfb(dec);

    //继续解码
    uint16 dec_delay = vdec_msi_decode_next(dec);
    return min(dec_delay, ply_delay);
}

static int32 vdec_msi_work(struct os_work *work)
{
    struct video_dec_msi *dec = container_of(work, struct video_dec_msi, work);

    if (dec->start && !dec->pause && !dec->ply_end) {
        os_mutex_lock(&dec->lock, osWaitForever);
        uint16 delay = vdec_msi_run(dec);
        os_mutex_unlock(&dec->lock);
        if (delay) {
            vdec_work_delay_run(work, delay);
        } else {
            vdec_work_run(work);
        }
    }
    return 0;
}

static int32 vdec_msi_start(struct video_dec_msi *dec)
{
    dec->pause = 0;
    dec->ply_end = 0;
    dec->start_plytime = 0;
    if (dec->start) {
        os_printf("%s run ...\r\n", dec->msi->name);
        return vdec_work_run(&dec->work);
    }

    vdec_warn("(%s) start!\r\n", dec->msi->name);
    dec->start = 1;
    return vdec_work_run(&dec->work);
}
static int32 vdec_msi_stop(struct video_dec_msi *dec)
{
    vdec_warn("(%s) stop ...\r\n", dec->msi->name);
    os_mutex_lock(&dec->lock, osWaitForever);
    dec->start = 0;
    dec->pause = 0;
    dec->eof   = 0;
    dec->ply_end = 0;
    dec->start_plytime = 0;
    vcodec_ioctl(dec->mgr->dev, dec->chan, VDEC_IOCTL_CANCLE, (uint32)&dec->req);
    while (dec->decode_status == 1) { os_sleep_ms(5); }
    os_work_cancle(&dec->work, 1);
    msi_clear(dec->msi);
    fbq_clear(&dec->plyQ);
    fb_put(dec->ply_fb);
    fb_put(dec->req.fb);
    fb_put(dec->req.fb_out);
    dec->ply_fb = NULL;
    dec->req.fb = NULL;
    dec->req.fb_out = NULL;
    dec->decoding = 0;
    dec->decode_status = 0;
    msi_discard_fb(dec->msi, NULL, dec->msi);
    os_mutex_unlock(&dec->lock);
    vdec_warn("(%s) stop DONE! %d\r\n", dec->msi->name, dec->msi->users.counter);
    return RET_OK;
}

static int32 vdec_msi_pause(struct video_dec_msi *dec)
{
    dec->pause = 1;
    dec->start_plytime = 0;
    return RET_OK;
}

static int32 vdec_msi_set_speed(struct video_dec_msi *dec, uint32 param1, uint32 param2)
{
    return RET_OK;
}

static int32 vdec_msi_set_avsync(struct video_dec_msi *dec, struct msi *master)
{
    if (master != dec->avsync) {
        msi_put(dec->avsync);
        dec->avsync = master;
        msi_get(dec->avsync);
        vdec_warn("%s: AVSync Master %s!\r\n", dec->msi->name, master ? master->name : "<NULL>");
    }
    return RET_OK;
}

int32 vdec_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int32 ret = RET_OK;
    struct framebuff *fb = NULL;
    struct video_dec_msi *dec = (struct video_dec_msi *)(msi->priv);

    switch (cmd_id) {
        case MSI_CMD_START:
            ret = vdec_msi_start(dec);
            break;
        case MSI_CMD_STOP:
        case MSI_CMD_CLEAR:
            ret = vdec_msi_stop(dec);
            break;
        case MSI_CMD_PAUSE:
            ret = vdec_msi_pause(dec);
            break;
        case MSI_CMD_SET_SPEED:
            ret = vdec_msi_set_speed(dec, param1, param2);
            break;
        case MSI_CMD_EOF:
            dec->eof = (param1 ? 1 : 0);
            break;
        case MSI_CMD_AVSYNC_MASTER:
            ret = vdec_msi_set_avsync(dec, (struct msi *)param1);
            break;
        case MSI_CMD_POST_DESTROY:
            vdec_warn("%s destroy!\r\n", dec->msi->name);
            vdec_msi_free(dec);
            break;
        case MSI_CMD_FREE_FB:
            fb = (struct framebuff *)param1;
            vcodec_ioctl(dec->mgr->dev, dec->chan, VDEC_IOCTL_RELEASE_FBDATA, (uint32)fb->data);
            vdec_msi_play_end(dec);
            vdec_msi_need_more(dec);
            break;
        case MSI_CMD_DUMP:
            os_printf(KERN_NOTICE"[%s]: plyQ:%d/%d, AVSync:%s%s%s%s%s, ply_fb:%p, dec_fb:%p/%p\r\n", msi->name,
                      fbq_count(&dec->plyQ), dec->mgr->plyQ_size,
                      dec->avsync ? dec->avsync->name : "<NULL>",
                      dec->start ? ", Start" : ", Stop",
                      dec->pause ? ", Pause" : "",
                      dec->eof ? ", EOF" : "",
                      dec->ply_end ? ", END" : "", dec->ply_fb, dec->req.fb, dec->req.fb_out);
            break;
        case MSI_CMD_FOLLOW_OUTPUT:
            break;
        case MSI_CMD_GET_RUNNING:
            ret = fbq_count(&dec->plyQ) || dec->ply_fb || fbq_count(&dec->msi->fbQ);
            break;
        case MSI_CMD_GET_PLAYTIME:
            if (dec->avsync == NULL) {
                *(uint32 *)param1 = dec->play_time;
            }
            break;
        case MSI_CMD_GET_VIDEO_PLAYTIME:
            *(uint32 *)param1 = dec->play_time;
            break;
        default:
            vdec_dbg("ACTION: unknown cmd %d\n", cmd_id);
            break;
    }
    return ret;
}

struct video_dec_msi *vdec_msi_new(struct video_dec_mgr *mgr, uint16 type)
{
    uint8 id = 0;

    ASSERT(mgr && mgr->chans);
    id = vdec_msi_req(mgr);
    if (id == mgr->chan_cnt) {
        vdec_err("BUSY! no more free decoder! [%s]\r\n", mgr->chan_names[0]);
        return NULL;
    }

    struct video_dec_msi *dec = (struct video_dec_msi *)decoder_mem_zalloc(mgr->msi_size);
    if (dec == NULL) {
        goto __init_err;
    }

    dec->id   = id;
    dec->mgr  = mgr;
    dec->interval_time = 5;
    dec->chan = vcodec_open(mgr->dev);
    if (!dec->chan) {
        vdec_err("%s open fail!\r\n", mgr->chan_names[id]);
        goto __init_err;
    }

    dec->msi = msi_new(mgr->chan_names[id], 4 * mgr->plyQ_size, NULL);
    if (dec->msi == NULL) {
        goto __init_err;
    }

    fbq_init(&dec->plyQ, NULL, mgr->plyQ_size, dec->msi);
    OS_WORK_INIT(&dec->work, vdec_msi_work, 0);
    os_mutex_init(&dec->lock);
    dec->msi->type   = type;
    dec->msi->priv   = dec;
    dec->msi->action = (msi_action)vdec_msi_action;
    dec->msi->mgr    = mgr->msi;
    dec->msi->fb_limits.counter = mgr->plyQ_size;
    dec->msi->fb_alloc = (malloc_cb_t)decoder_mem_alloc;
    dec->msi->fb_free  = (mfree_cb_t)decoder_mem_free;
    dec->msi->enable = 1;
    mgr->chans[id] = (struct video_dec_msi *)dec;
    vdec_warn("%s init success!\n", mgr->chan_names[id]);
    return dec;

__init_err:
    if (dec) {
        vcodec_close(mgr->dev, dec->chan);
        fbq_destroy(&dec->plyQ);
        msi_destroy(dec->msi);
        decoder_mem_free(dec);
        vdec_msi_idle(mgr, id);
    }
    return NULL;
}

int32 vdec_mgr_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    struct video_dec_msi *dec = NULL;

    if (msi->chan_mgr && cmd_id == MSI_CMD_NEW_CHANNEL) {
        dec = vdec_msi_new((struct video_dec_mgr *)msi->priv, msi->type);
        *(struct msi **)param2 = dec ? dec->msi : NULL;
        return dec ? RET_OK : -ENOMEM;
    }
    return -ENOTSUPP;
}

