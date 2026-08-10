#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/audio/wsola/wsola_process.h"
#include "../audio_coder.h"
#include "adec_msi.h"
#include "hal/acodec.h"
#include "hal/auchange.h"

static void *adec_msi_fb_alloc(uint32 size, void *priv)
{
    (void)priv;
    return decoder_mem_alloc(size);
}

static void adec_msi_fb_free(void *ptr, void *priv)
{
    (void)priv;
    decoder_mem_free(ptr);
}

/* audio decoder msi 模块 共用代码 */

/* msi组件实例化数量控制 */
static uint8 adec_msi_req(struct audio_dec_mgr *mgr)
{
    uint8 i = 0;
    uint32 flags = disable_irq();
    for (i = 0; i < mgr->chan_cnt; i++) {
        if (mgr->chans[i] == NULL) {
            mgr->chans[i] = (struct audio_dec_msi *)(-1);
            break;
        }
    }
    enable_irq(flags);
    return i;
}

static void adec_msi_idle(struct audio_dec_mgr *mgr, uint8 id)
{
    uint32 flags = disable_irq();
    mgr->chans[id] = NULL;
    enable_irq(flags);
}

static void adec_msi_free(struct audio_dec_msi *dec)
{
    if (dec) {
        msi_del_output(dec->msi, NULL, NULL, NULL);
        fbq_destroy(&dec->plyQ);
        msi_put(dec->avsync);
        msi_put(dec->follow);

        if (dec->dec) {
            acodec_close(dec->mgr->dev, dec->dec);;
            dec->dec = NULL;
        }

        if (dec->speed.stream) {
            auchange_close(dec->mgr->ac_dev, dec->speed.stream);
            dec->speed.stream = NULL;
        }

        if (dec->id < dec->mgr->chan_cnt && dec->mgr->chans[dec->id] == dec) {
            adec_msi_idle(dec->mgr, dec->id);
        }

        os_mutex_del(&dec->lock);
        decoder_mem_free(dec);
    }
}

//音视频同步检测: 返回需要等待的时间
static int32 adec_msi_check_avsync(struct audio_dec_msi *dec, struct framebuff *fb)
{
    uint32 plytime = 0;
    msi_cmd2(dec->avsync, MSI_CMD_GET_PLAYTIME, (uint32)&plytime, 0);

    if (plytime) {
        return fb->time > plytime ? fb->time - plytime : 0;
    } else {
        if (dec->start_plytime == 0) {
            dec->start_plytime = os_mseconds() - fb->time;
        }
        return 0;
    }
}

static int32 adec_msi_check_follow(struct audio_dec_msi *dec)
{
    if (dec->follow) {
        if (msi_do_cmd(dec->follow, MSI_CMD_GET_RUNNING, 0, 0) == 1) {

            return dec->interval_time;
        }
    }
    return 0;
}

static void adec_msi_decode_fb(struct audio_dec_msi *dec, struct framebuff *fb)
{   
    uint8 channels = 0;
    uint16 sample_rate = 0;
    int32 ret = RET_ERR;
    int32 dec_nsamples = 0;
    int32 pts_nsamples = 0;
    struct framebuff *plyfb;
    struct auchange_req req;

    if (dec == NULL || fb == NULL) {
        fb_put(fb);
        return;
    }

    dec->req.info.target_sample_rate = 0;
    dec->req.info.target_channels = 1;
    dec->req.chan = dec->dec;
    dec->req.fb = fb;
    dec->req.outbuf = NULL;
    if(fb->len == 0) {
        dec->req.info.flags = ADEC_FLAG_PLC;
    }
    ret = acodec_decode(dec->mgr->dev, &dec->req);
    dec->req.info.flags = ADEC_FLAG_NONE;
    if (ret != RET_OK) {
        adec_err("DECODE_FAIL: frame_len=%d\n", fb->len);
        fb_put(fb);
        return;
    }
    dec_nsamples = dec->req.outbuf_len / sizeof(int16);
    os_memcpy(g_audio_decbuff, dec->req.outbuf, dec->req.outbuf_len);

    channels = (dec->req.info.target_channels>0)?dec->req.info.target_channels:dec->req.info.actual_channels;
    sample_rate = (dec->req.info.target_sample_rate>0)?dec->req.info.target_sample_rate:dec->req.info.actual_sample_rate;

    if (sample_rate > 0 && dec_nsamples > 0) {
        dec->interval_time = dec_nsamples * 1000 / sample_rate;
        if (dec->dac_buftime && dec->interval_time > dec->dac_buftime) {
            dec->interval_time = dec->dac_buftime;
        }
    }

    // PTS变速处理
    if (dec->speed.enabled && dec->speed.stream == NULL && sample_rate > 0) {
        dec->speed.stream = auchange_open(dec->mgr->ac_dev, sample_rate, 1, dec_nsamples);
        if(dec->speed.stream) {
            auchange_ioctl(dec->mgr->ac_dev, dec->speed.stream, AUCHANGE_IOCTL_SET_SPEED, (uint32)(dec->speed.speed));
            auchange_ioctl(dec->mgr->ac_dev, dec->speed.stream, AUCHANGE_IOCTL_SET_PITCH, (uint32)(dec->speed.pitch));
        }
    }

    if (dec->speed.enabled && dec->speed.stream) {
        req.chan = dec->speed.stream;
        req.data = g_audio_decbuff;
        req.nsamples = dec_nsamples;
        auchange_write(dec->mgr->ac_dev, &req);
        pts_nsamples = req.nsamples;
        auchange_ioctl(dec->mgr->ac_dev, dec->speed.stream, AUCHANGE_IOCTL_READ_AVAILABLE, (uint32)(&dec_nsamples));
    }

    if (dec_nsamples <= 0) {
        // adec_warn("dec_nsamples <= 0 after pts, drop fb %d %d %d\n",fb->len,dec->req.outbuf_len,dec_nsamples);
        fb_put(fb);
        return;
    }

    plyfb = msi_alloc_fb(dec->msi, NULL, NULL, dec_nsamples * sizeof(int16), 0, 0);
    if (!plyfb) {
        adec_err("%s: alloc fb failed! fb_limits:%d\n", dec->msi->name, dec->msi->fb_limits.counter);
        fb_put(fb);
        return;
    }

    if (dec->speed.enabled && dec->speed.stream) {
        req.chan = dec->speed.stream;
        req.data = (int16 *)plyfb->data;
        req.nsamples = dec_nsamples;
        auchange_read(dec->mgr->ac_dev, &req);
        dec_nsamples = req.nsamples;
    } else {
        os_memcpy(plyfb->data, g_audio_decbuff, dec_nsamples * sizeof(int16));
    }

    plyfb->mtype = MEDIA_DATA_AUDIO;
    plyfb->stype = AUDIO_CODEC_PCM_S16LE;
    plyfb->last = fb->last;
    plyfb->time = fb->time;
    plyfb->priv = fb->priv;
    plyfb->codec_info = &(dec->audio_info);
    fbq_enqueue(&dec->plyQ, plyfb, 0);
    fb_put(fb);
}

static int32 adec_msi_output_plyfb(struct audio_dec_msi *dec)
{
    uint16 ply_delay = dec->interval_time;
    struct framebuff *plyfb = dec->ply_fb ? dec->ply_fb : fbq_dequeue(&dec->plyQ, 0);
    while (plyfb) {
        ply_delay = adec_msi_check_avsync(dec, plyfb);
        if (ply_delay == 0) {
            if (msi_output_fb(dec->msi, plyfb, 1) == 0) {
                dec->ply_fb = plyfb;
                ply_delay = dec->interval_time;
                break;
            } else {
                dec->ply_fb = NULL;
            }
        } else { //播放时间未到
            dec->ply_fb = plyfb;
            break;
        }
        plyfb = fbq_dequeue(&dec->plyQ, 0);
    }
    ply_delay = ply_delay / 2; //留点余量：取二分之一的时间
    return ply_delay;
}

static int32 adec_msi_decode_next(struct audio_dec_msi *dec)
{
    uint16 dec_delay = 0;
    uint8  decQ_cnt;

    decQ_cnt = fbq_count(&dec->msi->fbQ);
    if (decQ_cnt && dec->msi->fb_limits.counter) { //播放队列未满，继续解码
        adec_msi_decode_fb(dec, msi_get_fb(dec->msi, 0));
    } else {
        dec_delay = dec->interval_time; //播放队列已满
    }

    return dec_delay;
}

static int32 adec_msi_run(struct audio_dec_msi *dec)
{
    uint16 ply_delay = 0;
    uint16 dec_delay = 0;

    ply_delay = adec_msi_check_follow(dec);
    if (ply_delay) { return ply_delay; }

    //处理待播放数据
    ply_delay = adec_msi_output_plyfb(dec);

    //继续解码
    dec_delay = adec_msi_decode_next(dec);
    return min(dec_delay, ply_delay);
}

static int32 adec_msi_work(struct os_work *work)
{
    struct audio_dec_msi *dec = container_of(work, struct audio_dec_msi, work);

    if (dec->start && !dec->pause && !dec->ply_end) {
        os_mutex_lock(&dec->lock, osWaitForever);
        uint16 delay = adec_msi_run(dec);
        os_mutex_unlock(&dec->lock);
        if (delay) {
            audio_coder_msi_delay_run(work, delay);
        } else {
            audio_coder_msi_run(work);
        }
    }
    return 0;
}

static int32 adec_msi_start(struct audio_dec_msi *dec)
{
    dec->pause = 0;
    dec->ply_end = 0;
    dec->start_plytime = 0;
    if (dec->start) {
        os_printf("%s run ...\r\n", dec->msi->name);
        return audio_coder_msi_run(&dec->work);
    }

    adec_warn("(%s) start!\r\n", dec->msi->name);
    dec->start = 1;
    msi_output_cmd(dec->msi, MSI_CMD_GET_DAC_BUFTIME, (uint32)&dec->dac_buftime, 0);
    return audio_coder_msi_run(&dec->work);
}

static int32 adec_msi_stop(struct audio_dec_msi *dec)
{
    adec_warn("(%s) stop ...\r\n", dec->msi->name);
    dec->start = 0;
    dec->pause = 0;
    dec->eof = 0;
    dec->ply_end = 0;
    dec->start_plytime = 0;
    os_work_cancle(&dec->work, 1);
    msi_clear(dec->msi);
    fbq_clear(&dec->plyQ);
    fb_put(dec->ply_fb);
    dec->ply_fb = NULL;
    msi_discard_fb(dec->msi, NULL, dec->msi);
    adec_warn("(%s) stop DONE! %d\r\n", dec->msi->name, dec->msi->users.counter);
    return RET_OK;
}

static int32 adec_msi_pause(struct audio_dec_msi *dec)
{
    dec->pause = 1;
    dec->start_plytime = 0;
    return RET_OK;
}

static int32 adec_msi_set_speed(struct audio_dec_msi *dec, uint32 param1, uint32 param2)
{
    dec->speed.enabled = 1;
    if (dec->speed.enabled && (param1 != dec->speed.speed)) {
        dec->speed.speed = param1;
        if (dec->speed.stream) {
            auchange_ioctl(dec->mgr->ac_dev, dec->speed.stream, AUCHANGE_IOCTL_SET_SPEED, (uint32)(dec->speed.speed));
        }
        adec_warn("dec decoder set speed %d !\r\n", dec->speed.speed);
    }
    return RET_OK;
}

static int32 adec_msi_set_avsync(struct audio_dec_msi *dec, struct msi *master)
{
    if (master != dec->avsync) {
        msi_put(dec->avsync);
        dec->avsync = master;
        msi_get(dec->avsync);
        adec_warn("%s: AVSync Master %s!\r\n", dec->msi->name, master ? master->name : "<NULL>");
    }
    return RET_OK;
}

int32 adec_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int32 ret = RET_OK;
    struct audio_dec_msi *dec = (struct audio_dec_msi *)(msi->priv);

    switch (cmd_id) {
        case MSI_CMD_START:
            ret = adec_msi_start(dec);
            break;
        case MSI_CMD_STOP:
        case MSI_CMD_CLEAR:
            ret = adec_msi_stop(dec);
            break;
        case MSI_CMD_PAUSE:
            ret = adec_msi_pause(dec);
            break;
        case MSI_CMD_SET_SPEED:
            ret = adec_msi_set_speed(dec, param1, param2);
            break;
        case MSI_CMD_EOF:
            dec->eof = (param1 ? 1 : 0);
            break;
        case MSI_CMD_AVSYNC_MASTER:
            ret = adec_msi_set_avsync(dec, (struct msi *)param1);
            break;
        case MSI_CMD_POST_DESTROY:
            adec_warn("%s destroy!\r\n", dec->msi->name);
            adec_msi_free(dec);
            break;
        case MSI_CMD_FREE_FB:
            if (dec->eof && dec->msi->fb_limits.counter == dec->mgr->plyQ_size && fbq_count(&dec->msi->fbQ) == 0) {
                dec->ply_end = 1;
                msi_notify(dec->msi, MSI_CMD_PLAY_END, 0, 0);//所有的数据已播放完成
            }
            if (!dec->pause && dec->start && dec->msi->fb_limits.counter && fbq_count(&dec->msi->fbQ)) {
                audio_coder_msi_run(&dec->work);
            }
            break;
        case MSI_CMD_DUMP:
            os_printf(KERN_NOTICE"[%s]: plyQ:%d/%d, AVSync:%s%s%s%s%s, ply_fb:%p\r\n", msi->name,
                      fbq_count(&dec->plyQ), dec->mgr->plyQ_size,
                      dec->avsync ? dec->avsync->name : "<NULL>",
                      dec->start ? ", Start" : ", Stop",
                      dec->pause ? ", Pause" : "",
                      dec->eof ? ", EOF" : "",
                      dec->ply_end ? ", END" : "", dec->ply_fb);
            break;
        case MSI_CMD_FOLLOW_OUTPUT:
            msi_put(dec->follow);
            dec->follow = (struct msi *)param1;
            if (dec->follow) {
                msi_get(dec->follow);
                msi_reuse_output(dec->msi, dec->follow);
                adec_warn("%s follow %s\r\n", dec->msi->name, dec->follow->name);
            }
            break;
        case MSI_CMD_GET_RUNNING:
            ret = fbq_count(&dec->plyQ) || dec->ply_fb || fbq_count(&dec->msi->fbQ);
            break;
        default:
            adec_dbg("ACTION: unknown cmd %d\n", cmd_id);
            break;
    }
    return ret;
}

struct audio_dec_msi *adec_msi_new(struct audio_dec_mgr *mgr, uint16 type, txAudioInfo_t *audio_info)
{
    uint8 id = 0;

    ASSERT(mgr && mgr->chans);
    id = adec_msi_req(mgr);
    if (id == mgr->chan_cnt) {
        adec_err("BUSY! no more free decoder! [%s]\r\n", mgr->chan_names[0]);
        return NULL;
    }

    struct audio_dec_msi *dec = (struct audio_dec_msi *)decoder_mem_zalloc(mgr->msi_size);
    if (dec == NULL) {
        goto __init_err;
    }

    dec->id          = id;
    dec->mgr         = mgr;
    dec->speed.speed = 100;
    dec->speed.pitch = 100;
    dec->interval_time = 100;
    os_memcpy(&dec->audio_info, audio_info, sizeof(txAudioInfo_t));

    dec->dec = acodec_open(mgr->dev, audio_info); //这部分代码后面需要调整
    if (!dec->dec) {
        adec_err("%s open fail! sample_rate=%d, channels=%d\r\n", mgr->chan_names[id], audio_info->sample_rate, audio_info->channels);
        goto __init_err;
    }

    dec->msi = msi_new(mgr->chan_names[id], mgr->plyQ_size, NULL);
    if (dec->msi == NULL) {
        goto __init_err;
    }

    fbq_init(&dec->plyQ, NULL, mgr->plyQ_size, dec->msi);
    OS_WORK_INIT(&dec->work, adec_msi_work, 0);
    os_mutex_init(&dec->lock);
    dec->msi->type   = type;
    dec->msi->priv   = dec;
    dec->msi->action = (msi_action)adec_msi_action;
    dec->msi->enable = 1;
    dec->msi->mgr    = mgr->msi;
    dec->msi->fb_limits.counter = mgr->plyQ_size;
    dec->msi->fb_alloc = adec_msi_fb_alloc;
    dec->msi->fb_free  = adec_msi_fb_free;
    mgr->chans[id] = (struct audio_dec_msi *)dec;
    adec_warn("%s init success! sample rate:%d\n", mgr->chan_names[id], audio_info->sample_rate);
    return dec;

__init_err:
    if (dec) {
        acodec_close(mgr->dev, dec->dec);
        fbq_destroy(&dec->plyQ);
        msi_destroy(dec->msi);
        decoder_mem_free(dec);
        adec_msi_idle(mgr, id);
    }
    return NULL;
}

int32 adec_mgr_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    struct audio_dec_msi *dec = NULL;

    if (msi->chan_mgr && cmd_id == MSI_CMD_NEW_CHANNEL) {
        txAudioInfo_t *codec = (txAudioInfo_t *)param1;
        dec = adec_msi_new((struct audio_dec_mgr *)msi->priv, msi->type, codec);
        *(struct msi **)param2 = dec ? dec->msi : NULL;
        return dec ? RET_OK : -ENOMEM;
    }
    return -ENOSTR;
}

