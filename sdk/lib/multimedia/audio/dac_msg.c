#include "basic_include.h"
#include "dev/audio/ausys.h"
#include "dev/audio/ausys_da.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"

#define dacmsg_dbg(fmt, ...)    //os_printf(KERN_DEBUG"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define dacmsg_err(fmt, ...)    os_printf(KERN_ERR"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define dacmsg_warn(fmt, ...)   os_printf(KERN_WARNING"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

struct audio_dac_msg {
    void *task;
    void *dac_hdl;
    struct msi *msi;
    struct framebuff *fb_null;
    txAudioInfo_t audio_info;
} gDACMSG;

static void dac_msg_task()
{
    int32 ret;
    struct ausys_da_msg ausys_msg;

    while (1) {
        ret = ausys_da_get_msg(&ausys_msg, osWaitForever);
        if (ret) {
            os_sleep_ms(5);
            continue;
        }

        if (ausys_msg.type & AUSYS_DA_MSG_PLAY_HALF) {
            if (msi_output_fb(gDACMSG.msi, NULL, 0)) {
                if (ausys_msg.da_content.fifo_next_addr == 0) {
                    if (gDACMSG.fb_null == NULL || gDACMSG.fb_null->len != ausys_msg.da_content.fifo_next_len) {
                        fb_put(gDACMSG.fb_null);
                        gDACMSG.fb_null = msi_alloc_fb(gDACMSG.msi, NULL, NULL, ausys_msg.da_content.fifo_next_len, 0, 0);
                        if (gDACMSG.fb_null) {
                            gDACMSG.fb_null->mtype = MEDIA_DATA_AUDIO;
                            gDACMSG.fb_null->stype = AUDIO_CODEC_DAC_LOOPBACK;
                            gDACMSG.fb_null->codec_info = &(gDACMSG.audio_info);
                            os_memset(gDACMSG.fb_null->data, 0, ausys_msg.da_content.fifo_next_len);
                        } else {
                            dacmsg_err("no mem\r\n");
                        }
                    }

                    if (gDACMSG.fb_null) {
                        gDACMSG.fb_null->time  = os_jiffies();
                        fb_get(gDACMSG.fb_null);
                        msi_output_fb(gDACMSG.msi, gDACMSG.fb_null, 0);
                    }
                } else {
                    struct framebuff *fb = msi_alloc_fb(gDACMSG.msi, NULL, NULL, ausys_msg.da_content.fifo_next_len, 0, 0);
                    if (fb) {
                        fb->mtype = MEDIA_DATA_AUDIO;
                        fb->stype = AUDIO_CODEC_DAC_LOOPBACK;
                        fb->time  = os_jiffies();
                        fb->codec_info = &(gDACMSG.audio_info);
                        hw_memcpy((void *)fb->data, (void *)ausys_msg.da_content.fifo_next_addr, ausys_msg.da_content.fifo_next_len);
                        msi_output_fb(gDACMSG.msi, fb, 0);
                    } else {
                        dacmsg_err("no mem\r\n");
                    }
                }
            }
        }
    }
}

static int32 dac_msg_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int32 ret = RET_OK;

    switch (cmd_id) {
        default:
            break;
    }
    return ret;
}

int32 dac_msg_init(uint32 msg, uint32 sample_rate, uint32 channels)
{
    uint8 init = 0;

    gDACMSG.msi = msi_new(DACMSG_MSI, 0, &init);
    if (gDACMSG.msi == NULL) {
        return -ENOMEM;
    }

    if (msg == 0) {
        msg = AUSYS_DA_MSG_PLAY_DONE | AUSYS_DA_MSG_PLAY_HALF | AUSYS_DA_MSG_FIFO_EMPTY;
    }

    if (init) {
        gDACMSG.msi->type   = 0;
        gDACMSG.msi->priv   = &gDACMSG;
        gDACMSG.msi->fb_limits.counter = 16;
        gDACMSG.msi->fb_alloc = (malloc_cb_t)decoder_mem_alloc;
        gDACMSG.msi->fb_free  = (mfree_cb_t)decoder_mem_free;
        gDACMSG.msi->action   = (msi_action)dac_msg_msi_action;
        gDACMSG.msi->enable   = 0; //不接收其他组件推数据

        gDACMSG.audio_info.sample_rate = sample_rate;
        gDACMSG.audio_info.channels = channels;

        ausys_da_register_msg(msg);
        gDACMSG.task = os_task_create("dac_msg", dac_msg_task, 0, OS_TASK_PRIORITY_HIGH, 5, NULL, 768);
        os_printf("dac msg init! msg=%x\r\n", msg);
    }

    return RET_OK;
}

void dac_msg_task_suspend()
{
    os_task_suspend2(gDACMSG.task);
    ausys_da_suspend();
}

void dac_msg_task_resume()
{
    ausys_da_resume(); 
    os_task_resume2(gDACMSG.task);
}