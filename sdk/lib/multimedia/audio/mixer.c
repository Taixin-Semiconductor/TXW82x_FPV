#include "basic_include.h"
#include "dev/audio/ausys.h"
#include "dev/audio/ausys_da.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "hal/aures.h"
#include "audio_coder.h"

#define mixer_dbg(fmt, ...)    //os_printf(KERN_DEBUG"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define mixer_err(fmt, ...)    os_printf(KERN_ERR"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define mixer_warn(fmt, ...)   os_printf(KERN_WARNING"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define MIXER_MSI_FBQ        (8)
#define MIXER_CHAN_MAX       (8)
#define MIXER_DELQ_SIZE      (MIXER_CHAN_MAX*MIXER_MSI_FBQ)
#define MIXER_SAMPLERATE_MAX (16000) //resample模块支持的最大采样率
#define MIXER_SR_FOLLOW      (0)

struct audio_mixer_mgr {
    struct msi *msi;
    void       *task;
    int16      *mix_buff;
    uint32      sample_rate;
    uint16      frm_size;
    uint8       latency_ms;
    uint8       bit_depth;
    uint8       channles;
    uint8       dac_volume;
    int8        delQ_cnt;
    int8        active_chan;
    int8        recover;        // 标识：向DAC推送数据后，恢复混音模式
    struct audio_mixer_msi *mix_mixer; //记录当前调整混音模式的mixer
    struct audio_mixer_msi *active_mixer; //记录最后一个active状态的mixer
    struct os_mutex   lock;
    struct framebuff *delQ[MIXER_DELQ_SIZE]; //待释放的fb
    struct audio_mixer_msi *mixers[MIXER_CHAN_MAX];
    struct aures_device *aures_dev;
} g_mixer;

struct audio_mixer_msi {
    struct msi       *msi;
    struct framebuff *fb;      //当前正在处理的fb
    uint16 fb_off;             //当前处理的fb的数据偏移
    uint8  id;
    uint8  start: 1;
    uint8  pause: 1;
    uint8  error: 1;
    uint8  eof:   1;
    uint8  ducking_pause: 1;   // 被抢占暂停
    uint8  active: 1;
    uint8  rev: 2;
    uint32 input_sample_rate;  // 当前音频流的sample rate
    void  *resample;           // 重采样模块句柄
    uint8  mode;               // 混音模式，用于检测自动恢复normal模式
    uint8  volume;             // 当前音量值，取值范围 0~100
    uint8  volume_bak;         // 其他通路切换混音模式时，备份当前通路的volume值
    uint32 plytime;
};

static const char *amixer_names[MIXER_CHAN_MAX] = {
    "mixer#1", "mixer#2", "mixer#3", "mixer#4", "mixer#5", "mixer#6", "mixer#7", "mixer#8",
};

static int32 audio_mixer_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2);
static void audio_mixer_set_dac_volume(struct audio_mixer_msi *mixer, uint32 volume);

// 实例化数量控制
static uint8 audio_mixer_msi_req()
{
    uint8 i = 0;
    uint32 flags = disable_irq();
    for (i = 0; i < MIXER_CHAN_MAX; i++) {
        if (g_mixer.mixers[i] == NULL) {
            g_mixer.mixers[i] = (struct audio_mixer_msi *)(-1);//占位
            break;
        }
    }
    enable_irq(flags);
    return i;
}

static void audio_mixer_msi_idle(uint8 id)
{
    uint32 flags = disable_irq();
    g_mixer.mixers[id] = NULL;
    enable_irq(flags);
}

void audio_mixer_active(struct audio_mixer_msi *mixer, uint32 active, uint8 force)
{
    if (active) {
        if (fbq_count(&mixer->msi->fbQ) || mixer->fb || force) {
            if (!mixer->active) {
                mixer->active = 1;
                g_mixer.active_chan++;
                mixer_warn("mixer active chans:%d\r\n", g_mixer.active_chan);
            }
            g_mixer.active_mixer = mixer;
        }
    } else {
        if (mixer->active) {
            mixer->active = 0;
            g_mixer.active_chan--;
            mixer_warn("mixer active chans:%d\r\n", g_mixer.active_chan);
        }
    }
}

void audio_mixer_set_volume(struct audio_mixer_msi *mixer, uint32 volume)
{
    uint32 vol = volume;
    if (vol > 100) { vol = 100; }
    if (vol < 0)   { vol = 0; }
    mixer->volume = vol;

    if(g_mixer.active_chan <= 1){
        //audio_mixer_set_dac_volume(mixer, volume);
    }
    mixer_warn("set [%s] volume %d\r\n", mixer->msi->name, vol);
}

void audio_mixer_set_dac_volume(struct audio_mixer_msi *mixer, uint32 volume)
{
    if (g_mixer.dac_volume != volume) {
        g_mixer.dac_volume = volume;
        ausys_da_change_volume(volume);
        mixer_warn("set dac volume %d%\r\n", volume);
    }
}

static void audio_mixer_adjust_volume(struct audio_mixer_msi *mixer, uint32 volume, uint8 pause)
{
    struct audio_mixer_msi *_mixer;

    for (uint8 ch = 0; ch < MIXER_CHAN_MAX; ch++) {
        _mixer = g_mixer.mixers[ch];

        if (_mixer == mixer || _mixer == NULL || _mixer == (struct audio_mixer_msi *)(-1)) {
            continue;
        }

        if (volume == 100) { //恢复
            audio_mixer_active(_mixer, (_mixer->ducking_pause == 1), 0);
            if (_mixer->volume_bak) {
                _mixer->volume = _mixer->volume_bak;
                _mixer->volume_bak = 0;
                _mixer->ducking_pause = 0;
            }
        } else { //调整
            if (_mixer->volume_bak == 0) {
                _mixer->volume_bak = _mixer->volume;
            }
            _mixer->volume = volume;
            if (pause) {
                _mixer->ducking_pause = 1;
                audio_mixer_active(_mixer, 0, 0);
            } else {
                _mixer->ducking_pause = 0;
                audio_mixer_active(_mixer, 1, 0);
            }
        }
    }
}

// 设置混音模式，可以压低/暂停 其他数据流
static int32 audio_mixer_change_mode(struct audio_mixer_msi *mixer, audio_mix_mode_t mode, uint8 volume)
{
    if (!mixer->start) {
        return -EINVAL;
    }

    if (g_mixer.mix_mixer && mixer != g_mixer.mix_mixer && mode <= g_mixer.mix_mixer->mode) {
        return -EBUSY; //低优先级的模式，不能切换
    }

    switch (mode) {
        case AUDIO_MIX_MODE_NORMAL:
            audio_mixer_adjust_volume(mixer, 100, 0);    // 恢复所有其他通道
            break;
        case AUDIO_MIX_MODE_DUCKING:
            audio_mixer_adjust_volume(mixer, volume, 0); // 压低其他通道
            break;
        case AUDIO_MIX_MODE_EXCLUSIVE:
            audio_mixer_adjust_volume(mixer, 0, 1);      // 暂停其他通道
            break;
        default:
            ASSERT(0);
            break;
    }

    mixer_warn("[%s] change MIXING mode: %d\r\n", mixer->msi->name, mode);
    mixer->mode = mode;
    g_mixer.mix_mixer = mode ? mixer : NULL;
    return RET_OK;
}

static uint32 audio_mixer_calc_frmsize(uint32 sample_rate, uint32 duration_ms, uint8 bit_depth, uint8 channels)
{
    uint64 total_bits  = (uint64)sample_rate * duration_ms * bit_depth * channels;
    uint32 byte_length = (uint32)(total_bits / 8000);
    byte_length = ALIGN(byte_length, 4);
    if (byte_length < 256) {
        byte_length = 256;
    }
    return byte_length;
}

static void audio_mixer_open_resample(struct audio_mixer_msi *mixer, uint32 sample_rate)
{
    if (mixer->resample) { aures_close(g_mixer.aures_dev, mixer->resample); }
    mixer->resample = aures_open(g_mixer.aures_dev, sample_rate, g_mixer.sample_rate, 1);
    mixer->input_sample_rate = sample_rate;
    if (mixer->resample == NULL) {
        mixer_err("[%s] open resample fail!\r\n", mixer->msi->name);
    }else{
        mixer_warn("open resamle %d -> %d\r\n", sample_rate, g_mixer.sample_rate);
    }
}

static void audio_mixer_resample(struct audio_mixer_msi *mixer)
{
#if 0 //这部分代码后面需要修改：使用 txAudioInfo_t
    AUDIO_TRACK *track = (AUDIO_TRACK *)mixer->fb->priv;
    uint32 sample_rate = track->samplerate;
#else //使用txAudioinfo_t之后启用这部分代码
    txAudioInfo_t *info = (txAudioInfo_t *)mixer->fb->codec_info;
    uint32 sample_rate = info->sample_rate;
#endif
    uint8 change_sr = (g_mixer.active_chan == 1 && sample_rate != g_mixer.sample_rate) && MIXER_SR_FOLLOW;
    if (g_mixer.active_chan > 1 && g_mixer.sample_rate > MIXER_SAMPLERATE_MAX && MIXER_SR_FOLLOW) {
        sample_rate = MIXER_SAMPLERATE_MAX;
        change_sr   = 1;
    }

    if (change_sr) {
        g_mixer.sample_rate = sample_rate;
        ausys_da_change_sample_rate(sample_rate);
        mixer_warn("mixer sample rate change to %d\r\n", sample_rate);
    }

    if (g_mixer.sample_rate == sample_rate) {
        return;
    }

    if (mixer->resample == NULL) {
        audio_mixer_open_resample(mixer, sample_rate);
    } else {
        if (mixer->input_sample_rate != info->sample_rate) { /*sample rate 发生动态变化*/
            audio_mixer_open_resample(mixer, sample_rate);
        }
    }

    if (mixer->resample) {
        uint32 resample_nsamples = (mixer->fb->len / 2) * g_mixer.sample_rate / sample_rate + 10;
        struct framebuff *nfb = msi_alloc_fb(mixer->msi, NULL, NULL, resample_nsamples * 2, 0, 0);
        if (nfb) {
            struct aures_req req;
            req.chan = mixer->resample;
            req.in_data = (int16 *)mixer->fb->data;
            req.in_nsamples = mixer->fb->len / 2;
            req.outbuf = (int16 *)nfb->data;
            req.out_nsamples = resample_nsamples;
            aures_resample(g_mixer.aures_dev, &req);
            nfb->len   = req.out_nsamples * 2;
            nfb->last  = mixer->fb->last;
            nfb->time  = mixer->fb->time;
            nfb->mtype = mixer->fb->mtype;
            nfb->stype = mixer->fb->stype;
            ASSERT(g_mixer.delQ_cnt < (MIXER_DELQ_SIZE - 1));
            g_mixer.delQ[++g_mixer.delQ_cnt] = mixer->fb;
            mixer->fb = nfb;
        }
    } else {
        ASSERT(g_mixer.delQ_cnt < (MIXER_DELQ_SIZE - 1));
        if (mixer->fb->last && mixer == g_mixer.mix_mixer) {
            g_mixer.recover = 1; /* 恢复混音模式: 最后一帧数据*/
        }
        g_mixer.delQ[++g_mixer.delQ_cnt] = mixer->fb;
        mixer->fb = NULL;
    }
}

static void audio_mixer_fb_get(struct audio_mixer_msi *mixer)
{
    if (mixer->fb == NULL) {
        mixer->fb = msi_get_fb(mixer->msi, 0);
        mixer->fb_off = 0;
        if (mixer->fb) {
            mixer->plytime = mixer->fb->time;
            audio_mixer_resample(mixer); //新数据，重采样
        }
    }
    if (mixer->fb == NULL && mixer->eof) {
        audio_mixer_active(mixer, 0, 0);
    }
}

static void audio_mixer_fb_put(struct audio_mixer_msi *mixer)
{
    if (mixer->fb_off >= mixer->fb->len) {
        ASSERT(g_mixer.delQ_cnt < (MIXER_DELQ_SIZE - 1));
        if (mixer->fb->last && mixer == g_mixer.mix_mixer) {
            audio_mixer_active(mixer, 0, 0);
            g_mixer.recover = 1; /* 恢复混音模式: 最后一帧数据*/
        }
        g_mixer.delQ[++g_mixer.delQ_cnt] = mixer->fb;
        mixer->fb = NULL;
        mixer->fb_off = 0;
    }
}

static int16 audio_mixer_chan_data(struct audio_mixer_msi *mixer)
{
    audio_mixer_fb_get(mixer);
    if (mixer->fb == NULL) {
        return 0;
    }

    uint16_t sample = (uint16_t)mixer->fb->data[mixer->fb_off + 0] |
                      (uint16_t)mixer->fb->data[mixer->fb_off + 1] << 8;
    mixer->fb_off += 2;
    audio_mixer_fb_put(mixer);
    return (int16_t)sample;
}

// 饱和加法 (防止定点数溢出)
static inline int32 audio_mixer_sat_add(int32 a, int32 b)
{
    int64 sum = (int64)a + (int64)b;
    if (sum > 32767)  { return 32767; }
    if (sum < -32768) { return -32768; }
    return (int32)sum;
}

static inline int32 audio_mixer_vol_scale(int16 sample, uint32 vol)
{
    int32 scaled = (int32)sample * (int32)vol / 100;
    if (scaled > 32767) return 32767;
    if (scaled < -32768) return -32768;
    return scaled;
}

static int32 audio_mixer_process_fixed(int32 *active)
{
    int64 sum = 0;
    uint8 active_cnt = 0;
    struct audio_mixer_msi *mixer;

    for (uint8 ch = 0; ch < MIXER_CHAN_MAX; ch++) {
        mixer = g_mixer.mixers[ch];
        if (!mixer || mixer == (struct audio_mixer_msi *)(-1)) {
            continue;
        }
        if (!mixer->start || mixer->pause || mixer->ducking_pause || mixer->error) {
            continue;
        }
        if (!mixer->fb && !fbq_count(&mixer->msi->fbQ)) {
            if (mixer->eof && mixer == g_mixer.mix_mixer) {
                audio_mixer_active(mixer, 0, 0);
                g_mixer.recover = 1; //未收到last=1的fb??
            }
            continue;
        }

        active_cnt++;
        int16 sample = audio_mixer_chan_data(mixer);
        if (sample == 0) {
            continue;
        }

        int32 scaled_sample = audio_mixer_vol_scale(sample, mixer->volume);
        sum += scaled_sample;
    }

    if (sum > 32767)  { sum = 32767; }
    if (sum < -32768) { sum = -32768; }

    if (*active == -1) {
        *active = active_cnt;
    }
    return sum;
}

static uint32 audio_mixer_process_1(void)
{
    int16 avail = 0;
    int16 copy = 0;
    uint32 tot_len = 0;
    struct audio_mixer_msi *mixer = g_mixer.active_mixer;
    uint8 *buf = (uint8 *)g_mixer.mix_buff;

    if (mixer == NULL) { return 0; }

    while (tot_len < g_mixer.frm_size) {
        audio_mixer_fb_get(mixer);
        if (mixer->fb == NULL) {
            break;
        }

        copy  = g_mixer.frm_size - tot_len;
        avail = mixer->fb->len - mixer->fb_off;
        copy  = min(copy, avail);
        os_memcpy(buf + tot_len, mixer->fb->data + mixer->fb_off, copy);
        tot_len += copy;
        mixer->fb_off += copy;
        audio_mixer_fb_put(mixer);
    }

    return tot_len;
}

static uint32 audio_mixer_process_n(void)
{
    int32 active = -1;
    uint32 samples_cnt = g_mixer.frm_size / 2;

    for (uint32 i = 0; i < samples_cnt && active; i++) {
        int32 sample = audio_mixer_process_fixed(&active);
        g_mixer.mix_buff[i] = (int16)sample;
    }

    return active > 0 ? g_mixer.frm_size : 0;
}

static void audio_mixer_task(void *arg)
{
    uint32 frmlen = 0;
    while (1) {
        frmlen = 0;
        g_mixer.delQ_cnt = -1;

        os_mutex_lock(&g_mixer.lock, osWaitForever);
        if (g_mixer.active_chan) {
            if (g_mixer.active_chan == 1 && MIXER_SR_FOLLOW) {
                frmlen = audio_mixer_process_1();
            } else {
                frmlen = audio_mixer_process_n();
            }
        }
        os_mutex_unlock(&g_mixer.lock);

        if (frmlen > 0) {
            ausys_da_put(g_mixer.mix_buff, frmlen);
        }

        if (g_mixer.recover) {
            audio_mixer_change_mode(g_mixer.mix_mixer, AUDIO_MIX_MODE_NORMAL, 0);
            g_mixer.recover  = 0;
        }

        while (g_mixer.delQ_cnt >= 0) {
            fb_put(g_mixer.delQ[g_mixer.delQ_cnt]);
            g_mixer.delQ_cnt--;
        }

        if (frmlen == 0) {
            os_sleep_ms(10);
        }
    }
}

static void audio_mixer_free(struct audio_mixer_msi *mixer)
{
    if (mixer->mode != AUDIO_MIX_MODE_NORMAL) {
        audio_mixer_adjust_volume(mixer, 100, 0);
    }

    fb_put(mixer->fb);
    mixer->fb = NULL;
    msi_clear(mixer->msi);
    audio_mixer_active(mixer, 0, 0);
    audio_mixer_msi_idle(mixer->id);
    decoder_mem_free(mixer);
}

static void audio_mixer_start(struct audio_mixer_msi *mixer)
{
    mixer->pause = 0;
    mixer->start = 1;
    audio_mixer_active(mixer, 1, 0);
    mixer_warn("%s: start!\r\n", mixer->msi->name);
}

static void audio_mixer_stop(struct audio_mixer_msi *mixer)
{
    if (g_mixer.mix_mixer == mixer) {
        audio_mixer_change_mode(mixer, AUDIO_MIX_MODE_NORMAL, 0);
    }

    mixer->eof   = 0;
    mixer->start = 0;
    fb_put(mixer->fb);
    mixer->fb = NULL;
    msi_clear(mixer->msi);
    audio_mixer_active(mixer, 0, 0);
    if (mixer->resample) {
        aures_close(g_mixer.aures_dev, mixer->resample);
        mixer->resample = NULL;
    }
    mixer_warn("%s: stop. %d\r\n", mixer->msi->name, g_mixer.active_chan);
}

static struct audio_mixer_msi *audio_mixer_new(uint32 arg)
{
    uint8 id = audio_mixer_msi_req();
    if (id == MIXER_CHAN_MAX) {
        mixer_err("audio mixer: no free chan!\r\n");
        return NULL;
    }

    struct audio_mixer_msi *mixer = decoder_mem_alloc(sizeof(struct audio_mixer_msi));
    if (mixer == NULL) {
        mixer_err("audio mixer: decoder_mem_alloc fail!\r\n");
        audio_mixer_msi_idle(id);
        return NULL;
    }

    os_memset(mixer, 0, sizeof(struct audio_mixer_msi));
    mixer->msi = msi_new(amixer_names[id], MIXER_MSI_FBQ, NULL);
    if (mixer->msi == NULL) {
        decoder_mem_free(mixer);
        audio_mixer_msi_idle(id);
        mixer_err("audio mixer: msi_new fail!\r\n");
        return NULL;
    }

    mixer->id = id;
    mixer->start = 1;
    mixer->volume = 50;
    mixer->mode = AUDIO_MIX_MODE_NORMAL;
    mixer->msi->fb_limits.counter = MIXER_MSI_FBQ;
    mixer->msi->fb_alloc = (malloc_cb_t)decoder_mem_alloc;
    mixer->msi->fb_free  = (mfree_cb_t)decoder_mem_free;
    mixer->msi->priv     = mixer;
    mixer->msi->action   = (msi_action)audio_mixer_action;
    mixer->msi->enable   = 1;
    g_mixer.mixers[id] = mixer;
    mixer_warn("new mixer %s\r\n", amixer_names[id]);
    return mixer;
}

static void audio_mixer_dump(struct audio_mixer_msi *mixer)
{
    os_printf(KERN_NOTICE"[%s] mode:%d, volume:%d/%d%s%s%s%s%s, active:%d\r\n",
              mixer->msi->name, mixer->mode,
              mixer->volume, mixer->volume_bak,
              mixer->start ? ", Start" : ", Stop",
              mixer->pause ? ", Pause" : "",
              mixer->ducking_pause ? ", DuckingPause" : "",
              mixer->error ? ", Error" : "",
              mixer->eof ? ", EOF" : "",
              g_mixer.active_chan);

    if (mixer->fb) {
        struct framebuff *p = mixer->fb;
        os_printf(KERN_NOTICE"    fb:%p, users:%d, type:%d/%d, data:%p, len:%d, srcID:%d, Tag:%d%s%s%s%s%s, msi:%s\r\n",
                  p, p->users.counter, p->mtype, p->stype, p->data, p->len, p->srcID, p->datatag,
                  p->pool ? ", Pool" : ", Alloc",
                  p->used ? ", Used" : "",
                  p->clone ? ", Clone" : "",
                  p->keyfrm ? ", KeyFrame" : "",
                  p->last ? ", Last" : "",
                  p->msi ? p->msi->name : "");
    }
}

static int32 audio_mixer_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int32 ret = RET_OK;
    struct audio_mixer_msi *mixer = (struct audio_mixer_msi *)(msi->priv);

    if (msi == g_mixer.msi) {
        if (cmd_id == MSI_CMD_NEW_CHANNEL) {
            os_mutex_lock(&g_mixer.lock, osWaitForever);
            mixer = audio_mixer_new(param1);
            os_mutex_unlock(&g_mixer.lock);
            *(struct msi **)param2 = (mixer ? mixer->msi : NULL);
        }
        return 0;
    }

    os_mutex_lock(&g_mixer.lock, osWaitForever);
    switch (cmd_id) {
        case MSI_CMD_START:
            audio_mixer_start(mixer);
            break;
        case MSI_CMD_STOP:
            audio_mixer_stop(mixer);
            break;
        case MSI_CMD_PAUSE:
            mixer_warn("%s: PAUSE!\r\n", msi->name);
            mixer->pause = 1;
            audio_mixer_active(mixer, 0, 0);
            break;
        case MSI_CMD_SET_SPEED:
            break;
        case MSI_CMD_SET_VOLUME:
            audio_mixer_set_volume(mixer, param1);
            break;
        case MSI_CMD_GET_VOLUME:
            if(param1) *(uint8 *)param1 = mixer->volume;
            break;
        case MSI_CMD_SET_DAC_VOLUME:
            audio_mixer_set_dac_volume(mixer, param1);
            break;
        case MSI_CMD_POST_DESTROY:
            mixer->start = 0;
            mixer_warn("%s destroy!\r\n", mixer->msi->name);
            audio_mixer_free(mixer);
            break;
        case MSI_CMD_SET_MIXER_MODE:
            ret = audio_mixer_change_mode(mixer, param1, param2);
            break;
        case MSI_CMD_DUMP:
            audio_mixer_dump(mixer);
            break;
        case MSI_CMD_EOF:
            mixer->eof = param1 ? 1 : 0;
            break;
        case MSI_CMD_GET_DAC_BUFTIME:
            *(uint32 *)param1 = g_mixer.latency_ms * 3;
            break;
        case MSI_CMD_GET_AUDIO_PLAYTIME:
        case MSI_CMD_GET_PLAYTIME:
            if (mixer->plytime > 3 * g_mixer.latency_ms) {
                *(uint32 *)param1 = mixer->plytime - 3 * g_mixer.latency_ms;
            } else {
                *(uint32 *)param1 = mixer->plytime;
            }
            break;
        case MSI_CMD_TRANS_FB:
            audio_mixer_active(mixer, 1, 1);
            break;
        default:
            break;
    }
    os_mutex_unlock(&g_mixer.lock);

    msi_output_cmd(g_mixer.msi, cmd_id, param1, param2);
    return ret;
}

int32 audio_mixer_init(uint32 sample_rate, uint32 latency_ms, uint32 bit_depth, uint32 channels)
{
    uint8 inited = 0;
    uint32 buff_size;

    if (g_mixer.msi) { return RET_OK; }

    g_mixer.msi = msi_new(MIXER_MSI, 0, &inited);
    if (g_mixer.msi == NULL) {
        return -ENOMEM;
    }

    if (sample_rate == 0) { sample_rate = 16000; }
    if (latency_ms == 0)  { latency_ms = 20; }
    if (bit_depth == 0)   { bit_depth = 16; }
    if (channels == 0)    { channels = 1; }
    if (sample_rate > MIXER_SAMPLERATE_MAX) {
        sample_rate = MIXER_SAMPLERATE_MAX;
    }

    if (inited) {
        g_mixer.sample_rate = sample_rate;
        g_mixer.latency_ms  = latency_ms;
        g_mixer.bit_depth   = bit_depth;
        g_mixer.channles    = channels;

        buff_size = audio_mixer_calc_frmsize(MIXER_SAMPLERATE_MAX, latency_ms, bit_depth, channels);
        g_mixer.mix_buff = decoder_mem_alloc(buff_size);
        if (g_mixer.mix_buff == NULL) {
            mixer_err("alloc fail, size:%d\r\n", buff_size);
            return -ENOMEM;
        }

        os_mutex_init(&g_mixer.lock);
        os_memset(g_mixer.mixers, 0, sizeof(g_mixer.mixers));
        g_mixer.frm_size = audio_mixer_calc_frmsize(sample_rate, latency_ms, bit_depth, channels);

        g_mixer.msi->fb_alloc = (malloc_cb_t)decoder_mem_alloc;
        g_mixer.msi->fb_free  = (mfree_cb_t)decoder_mem_free;
        g_mixer.msi->priv     = &g_mixer;
        g_mixer.msi->action   = (msi_action)audio_mixer_action;
        g_mixer.msi->type     = 0;
        g_mixer.msi->chan_mgr = 1;
        g_mixer.msi->enable   = 1;

        g_mixer.aures_dev = (struct aures_device *)dev_get(HG_AUDIO_RESAMPLER_DEVID);

        ausys_da_init(sample_rate, 16, 1, AUSYS_AUDA, buff_size * 3, buff_size, g_mixer.frm_size);
        ausys_da_change_volume(100);
        ausys_da_play();

        g_mixer.task = os_task_create("audio_mixer", audio_mixer_task, NULL,
                                      OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 1024);
        os_printf("Audio Mixer Init: SR=%d, Latency=%dms, Bit:%d, CH:%d, BufSize=%d/%d\r\n",
                  sample_rate, latency_ms, bit_depth, channels, buff_size, g_mixer.frm_size);
    }
    return RET_OK;
}

