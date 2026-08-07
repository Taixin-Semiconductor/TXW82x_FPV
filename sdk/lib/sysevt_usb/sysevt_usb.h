#ifndef __SYSEVT_USB_H_
#define __SYSEVT_USB_H_

#include "lib/common/sysevt.h"

enum sysevt_usb_status
{
    SYSEVT_USB_STATUS_CONNECT       = BIT(1),
    SYSEVT_USB_STATUS_DISCONNECT    = BIT(2),
};


/* ------------ SYSTEM EVENT USB VIDEO CLASS ------------ */
struct sysevt_usbh_video_priv_data
{
    char video_dev_name[16];
    uint32_t uvc_format;
    uint32_t width;
    uint32_t height;
    uint32_t status;
};

#define CONFIG_SYSEVT_USB_VIDEO_MAX_NUM 2

int system_event_usbh_video_init();
int system_event_usbh_video_deinit();
int system_event_usbh_video_update_info(int dev_num, const char* dev_name, uint32_t uvc_format, uint32_t width, uint32_t height);
int system_event_usbh_video_get_info(int dev_num, struct sysevt_usbh_video_priv_data *info);
int system_event_usbh_video_new_event(int dev_num, enum SYSEVT_USB_SUBEVT subevt);
void system_event_usbh_video_hdl(uint32 event_id, uint32 data, uint32 priv);

#endif