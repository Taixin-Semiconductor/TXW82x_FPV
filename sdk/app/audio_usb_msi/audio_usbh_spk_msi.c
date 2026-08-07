#include "lib/audio/uac/uac_host.h"
#include "lib/heap/av_heap.h"
#include "audio_usbh_msi.h"

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC      av_malloc
#define STREAM_LIBC_FREE        av_free
#define STREAM_LIBC_ZALLOC      av_zalloc

#define AUDIO_LEN               (1024)
#define MAX_UAC_APP             8

static int32_t uac_host_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    struct uac_msi_s *uac_spk = (struct uac_msi_s *)msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            if (uac_spk)
            {
                STREAM_LIBC_FREE(uac_spk);
                msi->priv = NULL;
            }
        }
            break;

        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;
            if(fb->mtype != F_AUDIO)
            {
                ret = RET_ERR;
            }
            else
            {
                // USB MIC 回环数据 
                if(fb->stype != FSTYPE_AUDIO_USB_MIC)
                {
                    ret = RET_ERR;
                }
            }
        }
            break;

        default:
            break;
    }
    return ret;
}

static void send_usbspk_audio(void *d)
{
    int16 *buf = NULL;
    uint32 len = 0;
    uint8_t *audio_addr = NULL;
	UAC_MANAGE *usbspk_manage = NULL;
	uint32_t usbspk_timeout = 0;
	uint32_t usb_dma_irq_count = 0;
    uint32_t stop_flag = 0;
    struct framebuff *fb = NULL;

    struct msi* msi = (struct msi*)d;
    if(!msi)
    {
        os_printf("open usbspk stream err\n");
        return;
    }

    struct uac_msi_s *uac_spk = (struct uac_msi_s *)msi->priv;

    while(1)
    {
        os_event_wait(&uac_spk->evt, UAC_TASK_STOP, (uint32 *)&stop_flag, OS_EVENT_WMODE_OR, 0);
        if (stop_flag)
        {
            goto send_usbspk_audio_end;
        }

		if(usbspk_timeout > 500) {
			if(usb_dma_irq_count == *(uac_spk->p_usb_dma_irq_time)) {
				goto send_usbspk_audio_end;
			}
			usb_dma_irq_count = *(uac_spk->p_usb_dma_irq_time);
			usbspk_timeout = 0;
		}
		usbspk_timeout++;

        fb = msi_get_fb(msi, 0);
        if(fb) {
            usbspk_manage = get_usbspk_new_frame(0);
            if(usbspk_manage) 
            {
                _os_printf("S");
                buf = (int16 *)fb->data;
                len = fb->len; 
                audio_addr = get_uac_frame_data(usbspk_manage);

                if (buf && audio_addr) {
                    os_memcpy((uint8*)audio_addr, buf, len);
                    buf = NULL;
                    audio_addr = NULL;
                } else {
                    os_printf("%s %d audio data get failed\n", __FUNCTION__, __LINE__);
                    goto send_usbspk_audio_end;
                }

                set_uac_frame_datalen(usbspk_manage, len);
                set_uac_frame_sta(usbspk_manage, 1);
                put_usbspk_frame_to_use(usbspk_manage);

                msi_delete_fb(NULL, fb);
                fb = NULL;

                len = 0;
                buf = NULL;
                audio_addr = NULL;
                usbspk_manage = NULL;

                usbspk_timeout = 0;
            }
            else
            {
                msi_delete_fb(NULL, fb);
                fb = NULL;
            }
        }
        else
        {
            // _os_printf("E");
            os_sleep_ms(1); 
        }  
    }

send_usbspk_audio_end:
    if (usbspk_manage)
    {
        del_usbspk_frame(usbspk_manage);
        usbspk_manage = NULL;
    }
    if (fb)
    {
        msi_delete_fb(NULL, fb);
        fb = NULL;
    }
	usbspk_room_del();
    msi_destroy(msi);
    return;
}

static void usbspk_audio_stream_init(int* p_usb_dma_irq_time)
{
    struct msi *msi = NULL;
usb_host_enum_finish_init_start:
    os_printf("%s %d\n",__FUNCTION__,__LINE__);
    msi = msi_new(R_USB_SPK, MAX_UAC_APP, NULL);

    if (!msi)
    {
        _os_printf("%s open stream err!\n", __FUNCTION__);
        return;
    }

    struct uac_msi_s *uac_spk = (struct uac_msi_s *)msi->priv;
    if (!uac_spk)
    {
        uac_spk = (struct uac_msi_s *)STREAM_LIBC_ZALLOC(sizeof(struct uac_msi_s));
        msi->priv = (void *)uac_spk;
        uac_spk->msi = msi;
        uac_spk->p_usb_dma_irq_time = p_usb_dma_irq_time;
        msi->action = (msi_action)uac_host_action;
        os_event_init(&uac_spk->evt);

        msi->enable = 1;

        os_task_create("usbspk_audio", send_usbspk_audio, (void *)msi, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 1024);
    }
    else
    {
        // 发送一个evt,通知线程需要退出了
        os_event_set(&uac_spk->evt, UAC_TASK_STOP, NULL);

        // 等到线程已经退出
        os_event_wait(&uac_spk->evt, UAC_TASK_EXIT, NULL, OS_EVENT_WMODE_OR, -1);

        msi->name = NULL;
        msi_destroy(msi);        

        goto usb_host_enum_finish_init_start;
    }

}

void usbspk_audio_stream_deinit(void)
{

}

void usbspk_enum_finish(int* p_usb_dma_irq_time)
{
	usbspk_audio_stream_init(p_usb_dma_irq_time);
} 
