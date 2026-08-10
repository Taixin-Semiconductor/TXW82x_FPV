#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "../audio_coder.h"
#include "aenc_msi.h"
#include "hal/acodec.h"

/* msi组件实例化数量控制 */
static uint8 aenc_msi_req(struct audio_enc_mgr *mgr)
{
    uint8 i = 0;
    uint32 flags = disable_irq();
    for (i = 0; i < mgr->chan_cnt; i++) {
        if (mgr->chans[i] == NULL) {
            mgr->chans[i] = (struct audio_enc_msi *)(-1);
            break;
        }
    }
    enable_irq(flags);
    return i;
}

static void aenc_msi_idle(struct audio_enc_mgr *mgr, uint8 id)
{
    uint32 flags = disable_irq();
    mgr->chans[id] = NULL;
    enable_irq(flags);
}

static void aenc_msi_free(struct audio_enc_msi *enc)
{
    if (enc) {
        msi_del_output(enc->msi, NULL, NULL, NULL);

        if (enc->enc) {
            acodec_close(enc->mgr->dev, enc->enc);;
            enc->enc = NULL;
        }

        if (enc->id < enc->mgr->chan_cnt && enc->mgr->chans[enc->id] == enc) {
            aenc_msi_idle(enc->mgr, enc->id);
        }

        if (enc->mgr->chan_names[enc->id]) {
            encoder_mem_free((void*)(enc->mgr->chan_names[enc->id]));
            enc->mgr->chan_names[enc->id] = NULL;
        }

        os_mutex_del(&enc->lock);
        encoder_mem_free(enc);
    }
}

static void aenc_msi_encode_fb(struct audio_enc_msi *enc, struct framebuff *fb)
{   
    int32 ret = RET_ERR;
    struct framebuff *outfb;
    int32 enc_nbytes = 0;

    if (enc == NULL || (enc->need_more && fb == NULL)) {
        goto __end;
    }

	if(fb) {
		if(enc->update_timestamp) {
			enc->timestamp = fb->time;
			enc->update_timestamp = 0;			
		}
		enc->get_total_size += fb->len / sizeof(int16);
	}
    
    txAudioInfo_t *actual_info = (txAudioInfo_t*)fb->codec_info;
    enc->req.info.actual_sample_rate = actual_info->sample_rate;
    enc->req.info.actual_channels = actual_info->channels;
    enc->req.info.frame_size = enc->audio_info.frame_size;
    enc->req.chan = enc->enc;
    enc->req.fb = fb;
    enc->req.outbuf = NULL;
    ret = acodec_encode(enc->mgr->dev, &enc->req);
    enc->need_more = enc->req.info.need_more;
    if (ret != RET_OK) {
        aenc_err("ENCODE_FAIL: ret=%d\n", ret);
        goto __end;
    }

    enc_nbytes = enc->req.outbuf_len;
    if (enc_nbytes <= 0) {
        goto __end;
    }

    enc->out_total_size += enc->audio_info.frame_size;

    if(enc->get_total_size == enc->out_total_size) {
        enc->update_timestamp = 1;
        enc->get_total_size = 0;
        enc->out_total_size = 0;
    }

    outfb = msi_alloc_fb(enc->msi, NULL, NULL, enc_nbytes, 0, 0);
    if (!outfb) {
        aenc_err("%s: alloc fb failed! fb_limits:%d\n", enc->msi->name, enc->msi->fb_limits.counter);
        goto __end;
    }

    os_memcpy(outfb->data, enc->req.outbuf, enc_nbytes);

    outfb->mtype = (enc->mgr->fb_type & 0xFF00) >> 8;
    outfb->stype = enc->mgr->fb_type & 0xFF;
    outfb->time = enc->timestamp;
    enc->timestamp += enc->interval_time;
    outfb->codec_info = &(enc->audio_info);
    fbq_enqueue(&enc->outQ, outfb, 0);
__end:
    if(fb) {
        fb_put(fb);
    }
}

static int32 aenc_msi_output_fb(struct audio_enc_msi *enc)
{
    struct framebuff *outfb = enc->out_fb ? enc->out_fb : fbq_dequeue(&enc->outQ, 0);
    while (outfb) {
        if (msi_output_fb(enc->msi, outfb, 0) == 0) {
            os_mutex_lock(&enc->lock, osWaitForever);
            enc->output_delay = 1;
            os_mutex_unlock(&enc->lock);            
            break;
        } else {
            enc->out_fb = NULL;
        }
        outfb = fbq_dequeue(&enc->outQ, 0);
    }
    return RET_OK;
}

static int32 aenc_msi_encode_next(struct audio_enc_msi *enc)
{
    uint8  outQ_cnt;

    if (enc->need_more && enc->msi->fb_limits.counter) { //输出队列未满，继续编码
        outQ_cnt = fbq_count(&enc->msi->fbQ);
        if(outQ_cnt) {
            aenc_msi_encode_fb(enc, msi_get_fb(enc->msi, 0));
        }
        else {
            os_mutex_lock(&enc->lock, osWaitForever);
            enc->encode_delay = 1;
            os_mutex_unlock(&enc->lock);
        }
    } else if(!enc->need_more && enc->msi->fb_limits.counter) {
        aenc_msi_encode_fb(enc, NULL);
    }
    else {
        os_mutex_lock(&enc->lock, osWaitForever);
        enc->encode_delay = 1;
        os_mutex_unlock(&enc->lock);        
    }

    return RET_OK;
}

static int32 aenc_msi_run(struct audio_enc_msi *enc)
{
    aenc_msi_encode_next(enc);
    aenc_msi_output_fb(enc);

    return RET_OK;
}

static int32 aenc_msi_work(struct os_work *work)
{
    uint8 delay = 0;
    struct audio_enc_msi *enc = container_of(work, struct audio_enc_msi, work);
    if(enc->msi->delete) {
        msi_free(enc->msi);
        return -1;
    }
    if (enc->start && !enc->pause) {   
        aenc_msi_run(enc);
        delay = enc->encode_delay || enc->output_delay;
        if (delay) {
            audio_coder_msi_delay_run(work, delay);
        } else {
            audio_coder_msi_run(work);
        }
    }
    return 0;
}

static int32 aenc_msi_start(struct audio_enc_msi *enc)
{
    enc->pause = 0;
    if (enc->start) {
        aenc_warn("%s run ...\r\n", enc->msi->name);
        return audio_coder_msi_run(&enc->work);
    }

    aenc_warn("(%s) start!\r\n", enc->msi->name);
    enc->start = 1;
    return audio_coder_msi_run(&enc->work);
}

static int32 aenc_msi_stop(struct audio_enc_msi *enc)
{
    aenc_warn("(%s) stop ...\r\n", enc->msi->name);
    enc->start = 0;
    enc->pause = 0;
    msi_clear(enc->msi);
    fbq_clear(&enc->outQ);
    fb_put(enc->out_fb);
    enc->out_fb = NULL;
    msi_discard_fb(enc->msi, NULL, enc->msi);
    aenc_warn("(%s) stop DONE! %d\r\n", enc->msi->name, enc->msi->users.counter);
    return RET_OK;
}

static int32 aenc_msi_pause(struct audio_enc_msi *enc)
{
    enc->pause = 1;
    return RET_OK;
}

int32 aenc_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int32 ret = RET_OK;
    struct audio_enc_msi *enc = (struct audio_enc_msi *)(msi->priv);

    switch (cmd_id) {
		case MSI_CMD_TRANS_FB_END:
            os_mutex_lock(&enc->lock, osWaitForever);
            enc->encode_delay = 0;
            os_mutex_unlock(&enc->lock);
            audio_coder_msi_run(&(enc->work));
			break;
        case MSI_CMD_START:
            ret = aenc_msi_start(enc);
            break;
        case MSI_CMD_STOP:
        case MSI_CMD_CLEAR:
            ret = aenc_msi_stop(enc);
            break;
        case MSI_CMD_PAUSE:
            ret = aenc_msi_pause(enc);
            break;
        case MSI_CMD_PRE_DESTROY:
            msi->delete = 1;
            ret = RET_ERR;
            break;
        case MSI_CMD_POST_DESTROY:
            aenc_warn("%s destroy!\r\n", enc->msi->name);
            aenc_msi_stop(enc);
            aenc_msi_free(enc);
            break;
        case MSI_CMD_FREE_FB_END:
            os_mutex_lock(&enc->lock, osWaitForever);
            enc->output_delay = 0;
            os_mutex_unlock(&enc->lock); 
            audio_coder_msi_run(&(enc->work));       
            break;
        case MSI_CMD_GET_RUNNING:
            *((uint32*)param1) = enc->start;
            ret = fbq_count(&enc->outQ) || enc->out_fb || fbq_count(&enc->msi->fbQ);
            break;
        case MSI_CMD_SET_BITRATE:
            acodec_ioctl(enc->mgr->dev, enc->enc, AENC_IOCTL_SET_BITRATE, param1);
            break;
        default:
            aenc_dbg("ACTION: unknown cmd %d\n", cmd_id);
            break;
    }
    return ret;
}

struct audio_enc_msi *aenc_msi_new(struct audio_enc_mgr *mgr, uint16 type, txAudioInfo_t *audio_info)
{
    uint8 id = 0;
    const char *chan_name = NULL;

    ASSERT(mgr && mgr->chans);
    id = aenc_msi_req(mgr);
    if (id == mgr->chan_cnt) {
        aenc_err("BUSY! no more free encoder! [%s]\r\n", mgr->chan_names[0]);
        return NULL;
    }

    struct audio_enc_msi *enc = (struct audio_enc_msi *)encoder_mem_zalloc(mgr->msi_size);
    if (enc == NULL) {
        goto __init_err;
    }

    enc->id          = id;
    enc->mgr         = mgr;
    os_memcpy(&enc->audio_info, audio_info, sizeof(txAudioInfo_t));

    enc->enc = acodec_open(mgr->dev, audio_info); 
    if (!enc->enc) {
        aenc_err("%s open fail! sample_rate=%d, channels=%d\r\n", mgr->chan_names[id], audio_info->sample_rate, audio_info->channels);
        goto __init_err;
    }

    if (audio_info->reuse_msi_name) {
        chan_name = audio_info->reuse_msi_name;
    } else {
        chan_name = mgr->chan_default_names[id];   
    }
    mgr->chan_names[id] = (char*)encoder_mem_zalloc(os_strlen(chan_name)+1);
    if (mgr->chan_names[id] == NULL) {
        goto __init_err;
    }
    os_memcpy(mgr->chan_names[id], chan_name, os_strlen(chan_name)+1);

    enc->msi = msi_new(mgr->chan_names[id], mgr->outQ_size, NULL);
    if (enc->msi == NULL) {
        goto __init_err;
    }

    fbq_init(&enc->outQ, NULL, mgr->outQ_size, enc->msi);
    OS_WORK_INIT(&enc->work, aenc_msi_work, 0);
    os_mutex_init(&enc->lock);
    enc->msi->type   = type;
    enc->msi->priv   = enc;
    enc->msi->action = (msi_action)aenc_msi_action;
    enc->msi->enable = 1;
    enc->msi->fb_limits.counter = mgr->outQ_size;
    enc->msi->fb_alloc = (malloc_cb_t)encoder_mem_alloc;
    enc->msi->fb_free  = (mfree_cb_t)encoder_mem_free;
    enc->interval_time = audio_info->frame_size * 1000 / audio_info->sample_rate;
	enc->update_timestamp = 1;
    mgr->chans[id] = (struct audio_enc_msi *)enc;
    aenc_warn("%s init success! sample rate:%d\n", mgr->chan_names[id], audio_info->sample_rate);	
    return enc;

__init_err:
    if (enc) {
        acodec_close(mgr->dev, enc->enc);
        fbq_destroy(&enc->outQ);
        msi_destroy(enc->msi);
        encoder_mem_free(enc);
        aenc_msi_idle(mgr, id);
        if (mgr->chan_names[id]) {
            encoder_mem_free((void*)(mgr->chan_names[id]));
            mgr->chan_names[id] = NULL;
        }
    }
    return NULL;
}

int32 aenc_mgr_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    struct audio_enc_msi *enc = NULL;

    if (msi->chan_mgr && cmd_id == MSI_CMD_NEW_CHANNEL) {
        txAudioInfo_t *codec = (txAudioInfo_t *)param1;
        enc = aenc_msi_new((struct audio_enc_mgr *)msi->priv, msi->type, codec);
        *(struct msi **)param2 = enc ? enc->msi : NULL;
        return enc ? RET_OK : -ENOMEM;
    }
    return -ENOSTR;
}

struct msi *aenc_get_msi(audio_codec_t type, const char *self_str, txAudioInfo_t *info)
{
    struct msi *enc_msi = NULL;
    char msi_name[26] = {0};
    switch(type) {
        case AUDIO_CODEC_AAC:
            os_snprintf(msi_name, 25, "aacenc_""%.4s_%02d%01d_%03d", self_str, info->sample_rate/1000, info->channels, info->frame_size/10);
			enc_msi = msi_find((const char*)msi_name, 0);
            if(enc_msi == NULL) {
                info->reuse_msi_name = msi_name;
                enc_msi = msi_find2("aacenc", 0, 1, info);
            }
            break;
        case AUDIO_CODEC_OPUS:
            os_snprintf(msi_name, 25, "opusenc_""%.4s_%02d%01d_%03d", self_str, info->sample_rate/1000, info->channels, info->frame_size/10);
            enc_msi = msi_find((const char*)msi_name, 0);
            if(enc_msi == NULL) {
                info->reuse_msi_name = msi_name;
                enc_msi = msi_find2("opusenc", 0, 1, info);
            }
            break;  
        case AUDIO_CODEC_ALAW:
            os_snprintf(msi_name, 25, "alawenc_""%.4s_%02d%01d_%03d", self_str, info->sample_rate/1000, info->channels, info->frame_size/10);
            enc_msi = msi_find((const char*)msi_name, 0);
            if(enc_msi == NULL) {
                info->reuse_msi_name = msi_name;
                enc_msi = msi_find2("alawenc", 0, 1, info);
            }
            break;
        case AUDIO_CODEC_ULAW:
            os_snprintf(msi_name, 25, "ulawenc_""%.4s_%02d%01d_%03d", self_str, info->sample_rate/1000, info->channels, info->frame_size/10);
            enc_msi = msi_find((const char*)msi_name, 0);
            if(enc_msi == NULL) {
                info->reuse_msi_name = msi_name;
                enc_msi = msi_find2("ulawenc", 0, 1, info);
            }
            break;          
		default:
			break;
    }
    return enc_msi;
}