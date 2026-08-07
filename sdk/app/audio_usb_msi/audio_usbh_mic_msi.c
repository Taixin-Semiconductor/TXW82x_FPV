#include "lib/audio/uac/uac_host.h"
#include "lib/heap/av_heap.h"
#include "audio_usbh_msi.h"

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC      av_malloc
#define STREAM_LIBC_FREE        av_free
#define STREAM_LIBC_ZALLOC      av_zalloc

#define AUDIO_LEN               (1024)
#define MAX_UAC_APP             4

static int32_t uac_host_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    struct uac_msi_s *uac_mic = (struct uac_msi_s *)msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            fbpool_destroy(&uac_mic->tx_pool);
            if(uac_mic) {
                STREAM_LIBC_FREE(uac_mic);
                msi->priv = NULL;
            }
        }
        break;

        case MSI_CMD_PRE_DESTROY:
        {
        }
        break;

        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;
            // os_printf("fb:%X\n",fb);
            if (fb->data)
            {
                STREAM_LIBC_FREE(fb->data);
                fb->data = NULL;
            }
            fbpool_put(&uac_mic->tx_pool, fb);
            // 不需要内核去释放fb
            ret = RET_OK + 1;
        }
        break;
    }
    return ret;
}

static void get_usbmic_audio(void *d)
{
	uint32_t usbmic_timeout = 0;
	uint32_t usb_dma_irq_count = 0;
    struct framebuff *fb = NULL;
    int16 *realdata = NULL;
    int16 *audio_addr = NULL;
	uint32_t audio_len = 0;
    uint32_t stop_flag = 0;
	UAC_MANAGE *usbmic_manage = NULL;
    struct msi *msi = (struct msi *)d;
	if(!msi)
	{
		os_printf("open usbmic stream err\n");
		return;
	}

    struct uac_msi_s *uac_mic = (struct uac_msi_s *)msi->priv;

	while(1)
	{
        os_event_wait(&uac_mic->evt, UAC_TASK_STOP, (uint32 *)&stop_flag, OS_EVENT_WMODE_OR, 0);
        if (stop_flag)
        {
            goto get_usbmic_audio_end;
        }

		if (usbmic_timeout > 500) {
			if(usb_dma_irq_count == *(uac_mic->p_usb_dma_irq_time)) {
				goto get_usbmic_audio_end;
			}
			usb_dma_irq_count = *(uac_mic->p_usb_dma_irq_time);
			usbmic_timeout = 0;
		}
		usbmic_timeout++;
        
		usbmic_manage = get_usbmic_frame();
		if (usbmic_manage)
		{
            fb = fbpool_get(&uac_mic->tx_pool, 0, uac_mic->msi);
			if(fb) 
            {
				_os_printf("L");
				audio_len = get_uac_frame_datalen(usbmic_manage);
				audio_addr = (int16 *)get_uac_frame_data(usbmic_manage);
                realdata = (int16 *)STREAM_LIBC_ZALLOC(AUDIO_LEN);
                if (realdata && audio_addr) {
                    fb->data = (uint8_t*)realdata;
                    os_memcpy(realdata, audio_addr, audio_len);
                    realdata = NULL;
                    audio_addr = NULL;
                } else {
                    os_printf("%s %d malloc failed\n", __FUNCTION__, __LINE__);
                    goto get_usbmic_audio_end;
                }

                fb->mtype = F_AUDIO;
                fb->stype = FSTYPE_AUDIO_USB_MIC;
                fb->len = audio_len;

                msi_output_fb(msi, fb);
                fb = NULL;

                del_usbmic_frame(usbmic_manage);
                usbmic_manage = NULL;
                realdata = NULL;
                audio_addr = NULL;
                audio_len = 0;

				usbmic_timeout = 0;
			}
			else 
            {
				del_usbmic_frame(usbmic_manage);
				usbmic_manage = NULL;
			}
		}
        else
        {
            // _os_printf("M");
            os_sleep_ms(1);
        }
	}

get_usbmic_audio_end:
    if (usbmic_manage)
    {
        del_usbmic_frame(usbmic_manage);
        usbmic_manage = NULL;
    }
    if (fb)
    {
        msi_delete_fb(NULL, fb);
        fb = NULL;
    }
    if(realdata)
    {
        STREAM_LIBC_FREE(realdata);
        realdata = NULL;
    }
	usbmic_room_del();
    msi_destroy(msi);
	return;
}

static void usbmic_audio_stream_init(int* p_usb_dma_irq_time)
{
    struct msi *msi = NULL;
usb_host_enum_finish_init_start:
    os_printf("%s %d\n",__FUNCTION__,__LINE__);
    msi = msi_new(S_USB_MIC, 0, NULL);

	if(!msi) {
		_os_printf("%s open stream err!\n",__FUNCTION__);
		return;
	}
    struct uac_msi_s *uac_mic = (struct uac_msi_s *)msi->priv;
    if(!uac_mic){
        uac_mic = (struct uac_msi_s *)STREAM_LIBC_ZALLOC(sizeof(struct uac_msi_s));
        msi->priv = (void *)uac_mic;
        uac_mic->msi = msi;
        uac_mic->p_usb_dma_irq_time = p_usb_dma_irq_time;
        msi->action = (msi_action)uac_host_action;
        os_event_init(&uac_mic->evt);
        fbpool_init(&uac_mic->tx_pool, MAX_UAC_APP);

        msi->enable = 1;
        msi_add_output(msi, NULL, R_USB_SPK);

        os_task_create("usbmic_audio", get_usbmic_audio, (void *)msi, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 1024);
    }
    else
    {
        // 发送一个evt,通知线程需要退出了
        os_event_set(&uac_mic->evt, UAC_TASK_STOP, NULL);

        // 等到线程已经退出
        os_event_wait(&uac_mic->evt, UAC_TASK_EXIT, NULL, OS_EVENT_WMODE_OR, -1);

        msi->name = NULL;
        msi_destroy(msi);        

        goto usb_host_enum_finish_init_start;
    }
}

void usbmic_audio_stream_deinit(void)
{

}

void usbmic_enum_finish(int* p_usb_dma_irq_time)
{
	usbmic_audio_stream_init(p_usb_dma_irq_time);
}