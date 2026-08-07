#include "basic_include.h"
#include "rtthread.h"
#include "sysevt_usb/sysevt_usb.h"


#define SYSEVT_USB_MALLOC   os_malloc
#define SYSEVT_USB_ZALLOC   os_zalloc
#define SYSEVT_USB_FREE     os_free

/* ------------ SYSTEM EVENT USB VIDEO CLASS ------------ */

#include "usbh_video.h"
static struct sysevt_usbh_video_priv_data *g_sysevt_usbh_video_data = NULL;

int system_event_usbh_video_init()
{
    if (!g_sysevt_usbh_video_data) {
        g_sysevt_usbh_video_data = SYSEVT_USB_ZALLOC(sizeof(struct sysevt_usbh_video_priv_data) * CONFIG_SYSEVT_USB_VIDEO_MAX_NUM);
        if (g_sysevt_usbh_video_data) {
            // 初始化默认值
            os_strcpy(g_sysevt_usbh_video_data[0].video_dev_name, "/dev/video0");
            g_sysevt_usbh_video_data[0].status = SYSEVT_USB_STATUS_DISCONNECT;

            os_strcpy(g_sysevt_usbh_video_data[1].video_dev_name, "/dev/video1");
            g_sysevt_usbh_video_data[1].status = SYSEVT_USB_STATUS_DISCONNECT;

        } else {
            os_printf("sysevt_usbh_video_data malloc failed\n");
            return RET_ERR;
        }
    }

    return RET_OK;
}

int system_event_usbh_video_deinit()
{
    if (g_sysevt_usbh_video_data) {
        SYSEVT_USB_FREE(g_sysevt_usbh_video_data);
        g_sysevt_usbh_video_data = NULL;
    }
    return RET_OK;
}

int system_event_usbh_video_update_info(int dev_num, const char* dev_name, uint32_t uvc_format, uint32_t width, uint32_t height)
{
    if (dev_num < 0 || dev_num >= CONFIG_SYSEVT_USB_VIDEO_MAX_NUM) {
        goto __exit_err;
    }

    if (!g_sysevt_usbh_video_data) {
        goto __exit_err;
    }

    if (dev_name) {
        os_strncpy(g_sysevt_usbh_video_data[dev_num].video_dev_name, dev_name, sizeof(g_sysevt_usbh_video_data[dev_num].video_dev_name) - 1);
    }
    g_sysevt_usbh_video_data[dev_num].uvc_format = uvc_format;
    g_sysevt_usbh_video_data[dev_num].width      = width;
    g_sysevt_usbh_video_data[dev_num].height     = height;

    return RET_OK;

__exit_err:
    return RET_ERR;
}

int system_event_usbh_video_get_info(int dev_num, struct sysevt_usbh_video_priv_data *info)
{
    if (dev_num < 0 || dev_num >= CONFIG_SYSEVT_USB_VIDEO_MAX_NUM) {
        goto __exit_err;
    }

    if (!g_sysevt_usbh_video_data) {
        goto __exit_err;
    }

    os_memcpy(info, &g_sysevt_usbh_video_data[dev_num], sizeof(struct sysevt_usbh_video_priv_data));
    return RET_OK;

__exit_err:
    return RET_ERR;
}

int system_event_usbh_video_new_event(int dev_num, enum SYSEVT_USB_SUBEVT subevt)
{
    if (dev_num < 0 || dev_num >= CONFIG_SYSEVT_USB_VIDEO_MAX_NUM) {
        goto __exit_err;
    }

    if (!g_sysevt_usbh_video_data) {
        goto __exit_err;
    }

    if (subevt == SYSEVT_USB_DEVICE_CONNECT) {
        g_sysevt_usbh_video_data[dev_num].status = SYSEVT_USB_STATUS_CONNECT;
    } 
    else if (subevt == SYSEVT_USB_DEVICE_DISCONNECT) {
        g_sysevt_usbh_video_data[dev_num].status = SYSEVT_USB_STATUS_DISCONNECT;
    } 
    else {
        goto __exit_err;
    }

    sys_event_new(SYS_EVENT(SYS_EVENT_USB, subevt), (uint32)&g_sysevt_usbh_video_data[dev_num]);

    return RET_OK;

__exit_err:
    return RET_ERR;
}

void system_event_usbh_video_hdl(uint32 event_id, uint32 data, uint32 priv)
{
    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_USB, SYSEVT_USB_DEVICE_CONNECT):
        {
            struct sysevt_usbh_video_priv_data *video_data = (struct sysevt_usbh_video_priv_data *)data;
            os_printf("----- SYSEVT_USB_DEVICE_CONNECT: dev_name=%s, format=0x%X, %u x %u ----- \n",
                      video_data->video_dev_name,
                      video_data->uvc_format,
                      video_data->width,
                      video_data->height);
            extern void usb_event_connect_hdl(void);
            usb_event_connect_hdl();
        }
            break;

        case SYS_EVENT(SYS_EVENT_USB, SYSEVT_USB_DEVICE_DISCONNECT):
        {
            struct sysevt_usbh_video_priv_data *video_data = (struct sysevt_usbh_video_priv_data *)data;
            os_printf("----- SYSEVT_USB_DEVICE_DISCONNECT: dev_name=%s ----- \n",
                      video_data->video_dev_name);
            extern void usb_event_disconnect_hdl(void);
            usb_event_disconnect_hdl();
        }
            break;

        default:
            break;

    }
}

