#ifndef __VIDEO_APP_USB_MSI_H
#define __VIDEO_APP_USB_MSI_H
#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "dev/usb/uvc_host.h"

enum uvc_app_usb
{
    UVC_TASK_EXIT = BIT(0),
    UVC_TASK_STOP = BIT(1),
};

typedef struct list_head* (*usbh_video_get_frame)(void *dev);
typedef void (*usbh_video_free_node)(void *dev, struct list_head* del);
typedef void (*usbh_video_del_frame)(void *dev, void *get_f);
typedef void (*usbh_video_set_frame_using)(void *frame);
typedef void (*usbh_video_set_frame_idle)(void*frame);
typedef int  (*usbh_video_get_frame_blank_node_num)(void *dev, struct list_head *head);
typedef bool (*usbh_video_deinit_cb)(void* dev);

struct usbh_video_priv_func
{
    usbh_video_get_frame                    get_frame;
    usbh_video_free_node                    free_node;
    usbh_video_del_frame                    del_frame;
    usbh_video_set_frame_using              set_frame_using;
    usbh_video_set_frame_idle               set_frame_idle;
    usbh_video_get_frame_blank_node_num     get_frame_node_num;
    usbh_video_deinit_cb                    deinit_cb;
    int dev_num;
    void *p_dev;
};



struct uvc_msi_s
{
    struct os_task task;
    struct msi *msi;
    struct os_event evt;
    struct fbpool tx_pool;

    uint32_t malloc_max_size; // 记录当前应该申请的最大size
    uint32_t current_photo_max_size;
    // 记录的时间与次数主要为了动态修改malloc_max_size,尽可能节约空间
    uint32_t malloc_time;       // 记录malloc_max_size修改的size
    uint32_t malloc_count_near; // 记录从记录malloc_max_size开始计算,申请的次数
    uint32_t uvc_format;
    struct usbh_video_priv_func *func;
    int* p_usb_dma_irq_time;
    void *priv;
};

void usbh_video_enum_finish_init(const char *video_dev_name, uint32_t uvc_format ,struct usbh_video_priv_func* func, int* p_usb_dma_irq_time);

#endif
