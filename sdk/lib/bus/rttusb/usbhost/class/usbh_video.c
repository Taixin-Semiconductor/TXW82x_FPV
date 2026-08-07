/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "rtthread.h"

#ifdef RT_USBH_UVC

#include "basic_include.h"
#include "rttusb_host.h"
#include "usbh_video.h"
#include "dev/usb/hgusb20_v1_dev_api.h"
#include "dev/usb/usb11_v0/hgusb11_v0_host_api.h"

#include "lib/multimedia/msi.h"
#include "video_app_usb_msi.h"
#include "video/uvc/rtt_uvc_host.h"
#ifdef RT_USBH_UAC
#include "usbh_audio.h"
#endif

#define DEV_FORMAT "/dev/video%d"

/* general descriptor field offsets */
#define DESC_bLength              0 /** Length offset */
#define DESC_bDescriptorType      1 /** Descriptor type offset */
#define DESC_bDescriptorSubType   2 /** Descriptor subtype offset */
#define DESC_bNumFormats          3 /** Descriptor numformat offset */
#define DESC_bNumFrameDescriptors 4 /** Descriptor numframe offset */
#define DESC_bFormatIndex         3 /** Descriptor format index offset */
#define DESC_bFrameIndex          3 /** Descriptor frame index offset */

/* interface descriptor field offsets */
#define INTF_DESC_bInterfaceNumber  2 /** Interface number offset */
#define INTF_DESC_bAlternateSetting 3 /** Alternate setting offset */
#define INTF_DESC_bInterfaceClass   5

#define VIDEO_FREE_PIPE             0
#define VIDEO_ALLOC_PIPE            1

#define CONFIG_USBHOST_MAX_VIDEO_CLASS 2
 
#define VIDEO_SET_INTF_ALTSETTING  2      //设置需要配置的AltSetting，默认为1 (根据打印，选择所需的Altersetting)

static struct uclass_driver usbh_video_class;

static rt_uint8_t g_video_buf[128];

static const char *format_type[] = { "uncompressed", "mjpeg", "based" };

struct usbh_video g_video_class[CONFIG_USBHOST_MAX_VIDEO_CLASS];
static rt_uint32_t g_devinuse = 0;

extern volatile uint32 rx_packet_len;
extern volatile uint32 rx_packet_len_2;
int usb_dma_h264_irq_times = 0;
int usb_dma_mjpeg_irq_times = 0;

static struct usbh_video *usbh_video_class_alloc(void)
{
    int devno;

    for (devno = 0; devno < CONFIG_USBHOST_MAX_VIDEO_CLASS; devno++) {
        if ((g_devinuse & (1 << devno)) == 0) {
            g_devinuse |= (1 << devno);
            memset(&g_video_class[devno], 0, sizeof(struct usbh_video));
            g_video_class[devno].minor = devno;
            return &g_video_class[devno];
        }
    }
    return NULL;
}

static void usbh_video_class_free(struct usbh_video *video_class)
{
    int devno = video_class->minor;

    if (devno >= 0 && devno < 32) {
        g_devinuse &= ~(1 << devno);
    }
    memset(video_class, 0, sizeof(struct usbh_video));
}

int usbh_video_get(struct usbh_video *video_class, rt_uint8_t request, rt_uint8_t intf, rt_uint8_t entity_id, rt_uint8_t cs, rt_uint8_t *buf, rt_uint16_t len)
{
    struct urequest setup;
    rt_uint8_t retry;
    int timeout = USB_TIMEOUT_BASIC;

    setup.request_type = USB_REQ_TYPE_DIR_IN | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE;
    setup.bRequest = request;
    setup.wValue = cs << 8;
    setup.wIndex = (entity_id << 8) | intf;
    setup.wLength = len;

    retry = 0;
    while (1) {

        if(rt_usb_hcd_setup_xfer(video_class->device->hcd, video_class->device->pipe_ep0_out, &setup, timeout) == 8)
        {
            if(rt_usb_hcd_pipe_xfer(video_class->device->hcd, video_class->device->pipe_ep0_in, g_video_buf, len, timeout) > 0)
            {
                if(rt_usb_hcd_pipe_xfer(video_class->device->hcd, video_class->device->pipe_ep0_out, RT_NULL, 0, timeout) == 0)
                {
                    break;
                }
            }
        }

        retry++;

        if (retry == 3) {
            return RT_ERROR;
        }
    }

    os_sleep_ms(5);

    if (buf) {
        memcpy(buf, g_video_buf, len);
    }

    return RT_EOK;
}

int usbh_video_set(struct usbh_video *video_class, rt_uint8_t request, rt_uint8_t intf, rt_uint8_t entity_id, rt_uint8_t cs, rt_uint8_t *buf, rt_uint16_t len)
{
    struct urequest setup;
    int timeout = USB_TIMEOUT_BASIC;

    setup.request_type = USB_REQ_TYPE_DIR_OUT | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE;
    setup.bRequest = request;
    setup.wValue = cs << 8;
    setup.wIndex = (entity_id << 8) | intf;
    setup.wLength = len;

    memcpy(g_video_buf, buf, len);

    if(rt_usb_hcd_setup_xfer(video_class->device->hcd, video_class->device->pipe_ep0_out, &setup, timeout) != 8)
    {
        return RT_ERROR;
    }
    if(rt_usb_hcd_pipe_xfer(video_class->device->hcd, video_class->device->pipe_ep0_out, g_video_buf, len, timeout) != len)
    {
        return RT_ERROR;
    }
    if(rt_usb_hcd_pipe_xfer(video_class->device->hcd, video_class->device->pipe_ep0_in, RT_NULL, 0, timeout) != 0)
    {
        return RT_ERROR;
    }

    os_sleep_ms(50);
    return RT_EOK;
}

int usbh_videostreaming_get_cur_probe(struct usbh_video *video_class)
{
    return usbh_video_get(video_class, VIDEO_REQUEST_GET_CUR, video_class->data_intf, 0x00, VIDEO_VS_PROBE_CONTROL, (rt_uint8_t *)&video_class->probe, 26);
}

int usbh_videostreaming_set_cur_probe(struct usbh_video *video_class, rt_uint8_t formatindex, rt_uint8_t frameindex, rt_uint32_t dwDefaultFrameInterval)
{
    video_class->probe.bFormatIndex = formatindex;
    video_class->probe.bFrameIndex = frameindex;
    video_class->probe.dwMaxPayloadTransferSize = 0;
    video_class->probe.dwFrameInterval = dwDefaultFrameInterval;
    os_printf("%s set probe interval:%x\n",__FUNCTION__,dwDefaultFrameInterval);
    return usbh_video_set(video_class, VIDEO_REQUEST_SET_CUR, video_class->data_intf, 0x00, VIDEO_VS_PROBE_CONTROL, (rt_uint8_t *)&video_class->probe, 26);
}

int usbh_videostreaming_set_cur_commit(struct usbh_video *video_class, rt_uint8_t formatindex, rt_uint8_t frameindex, rt_uint32_t dwDefaultFrameInterval)
{
    memcpy(&video_class->commit, &video_class->probe, sizeof(struct video_probe_and_commit_controls));
    video_class->commit.bFormatIndex = formatindex;
    video_class->commit.bFrameIndex = frameindex;
    video_class->commit.dwFrameInterval = dwDefaultFrameInterval;
    os_printf("%s set commit interval:%x\n",__FUNCTION__,dwDefaultFrameInterval);
    return usbh_video_set(video_class, VIDEO_REQUEST_SET_CUR, video_class->data_intf, 0x00, VIDEO_VS_COMMIT_CONTROL, (rt_uint8_t *)&video_class->commit, 26);
}

int usbh_video_open(struct usbh_video *video_class,
                    rt_uint8_t format_type,
                    rt_uint16_t wWidth,
                    rt_uint16_t wHeight,
                    rt_uint8_t altsetting)
{
    struct urequest setup;
    struct uendpoint_descriptor *ep_desc;
    rt_uint8_t mult;
    rt_uint16_t mps;
    int ret;
    bool found = false;
    rt_uint8_t formatidx = 0;
    rt_uint8_t frameidx = 0;
    rt_uint8_t step;
    int timeout = USB_TIMEOUT_BASIC;
    rt_uint8_t set_altsetting = altsetting;

    // if (video_class->is_opened) {
    //     return 0;
    // }

    for (rt_uint8_t i = 0; i < video_class->num_of_formats; i++) {
        if (format_type == video_class->format[i].format_type) {
            formatidx = i + 1;
            for (rt_uint8_t j = 0; j < video_class->format[i].num_of_frames; j++) {
                if ((wWidth == video_class->format[i].frame[j].wWidth) &&
                    (wHeight == video_class->format[i].frame[j].wHeight)) {
                    frameidx = j + 1;
                    found = true;
                    break;
                }
            }
        }
    }

    if (found == false) {
        os_printf("No found %d x %d resolution!!!!!!!!!!!!!\n",wWidth,wHeight);
        return RET_ERR;
    }

    if (altsetting >= (video_class->num_of_intf_altsettings)) {
        set_altsetting = video_class->cur_set_altersetting_num;
        os_printf("[altsetting error]VIDEO_SET_INTF_ALTSETTING:%d cur_set:%d\n",VIDEO_SET_INTF_ALTSETTING,set_altsetting);
    }

    /* Open video step:
     * Get CUR request (probe)
     * Set CUR request (probe)
     * Get CUR request (probe)
     * Get MAX request (probe)
     * Get MIN request (probe)
     * Get CUR request (probe)
     * Set CUR request (commit)
     *
    */
    os_printf("%s :set %d x %d resolution\n",__FUNCTION__,wWidth,wHeight);
    os_printf("Formatidx:%d Frameidx:%d\n",formatidx,frameidx);

    step = 0;
    ret = usbh_videostreaming_get_cur_probe(video_class);
    if (ret < 0) {
        goto errout;
    }

    step = 1;
    ret = usbh_videostreaming_set_cur_probe(video_class, formatidx, frameidx, video_class->format[formatidx-1].frame[frameidx-1].dwDefaultFrameInterval);
    if (ret < 0) {
        goto errout;
    }

    step = 2;
    ret = usbh_videostreaming_get_cur_probe(video_class);
    if (ret < 0) {
        goto errout;
    }

    step = 3;
    ret = usbh_video_get(video_class, VIDEO_REQUEST_GET_MAX, video_class->data_intf, 0x00, VIDEO_VS_PROBE_CONTROL, NULL, 26);
    if (ret < 0) {
        goto errout;
    }

    step = 4;
    ret = usbh_video_get(video_class, VIDEO_REQUEST_GET_MIN, video_class->data_intf, 0x00, VIDEO_VS_PROBE_CONTROL, NULL, 26);
    if (ret < 0) {
        goto errout;
    }

    step = 5;
    ret = usbh_videostreaming_set_cur_probe(video_class, formatidx, frameidx, video_class->format[formatidx-1].frame[frameidx-1].dwDefaultFrameInterval);
    if (ret < 0) {
        goto errout;
    }

    step = 6;
    ret = usbh_videostreaming_get_cur_probe(video_class);
    if (ret < 0) {
        goto errout;
    }

    step = 7;
    ret = usbh_videostreaming_set_cur_commit(video_class, formatidx, frameidx, video_class->format[formatidx-1].frame[frameidx-1].dwDefaultFrameInterval);
    if (ret < 0) {
        goto errout;
    }

    if((video_class->pipe_in->ep.bmAttributes & USB_EP_ATTR_TYPE_MASK) == USB_EP_ATTR_ISOC)
    {
        os_printf("ISO set interface altsetting: %u\r\n",set_altsetting);
        step = 8;
        setup.request_type = USB_REQ_TYPE_DIR_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE;
        setup.bRequest = USB_REQ_SET_INTERFACE;
        setup.wValue = set_altsetting;
        setup.wIndex = video_class->data_intf;
        setup.wLength = 0;

        if(rt_usb_hcd_setup_xfer(video_class->device->hcd, video_class->device->pipe_ep0_out, &setup, timeout) != 8)
        {
            goto errout;
        }
        if(rt_usb_hcd_pipe_xfer(video_class->device->hcd, video_class->device->pipe_ep0_in, RT_NULL, 0, timeout) != 0)
        {
            goto errout;
        }        
    }

    ep_desc = &video_class->isoin;
    mult = (ep_desc->wMaxPacketSize & USB_MAXPACKETSIZE_ADDITIONAL_TRANSCATION_MASK) >> USB_MAXPACKETSIZE_ADDITIONAL_TRANSCATION_SHIFT;
    mps = ep_desc->wMaxPacketSize & USB_MAXPACKETSIZE_MASK;
    if (ep_desc->bEndpointAddress & 0x80) {
        video_class->isoin_mps = mps * (mult + 1);
    } else {
        video_class->isoout_mps = mps * (mult + 1);
    }

    os_printf("Open video and select formatidx:%u, frameidx:%u, altsetting:%u\r\n", formatidx, frameidx, set_altsetting);
    video_class->is_opened = TRUE;
    video_class->current_format = format_type;
    return ret;

errout:
    os_printf("Fail to open video in step %u\r\n", step);
    return ret;
}

int usbh_video_close(uinst_t device, struct usbh_video *video_class)
{
    struct urequest setup;
    struct uendpoint_descriptor *ep_desc = RT_NULL;
    int timeout = USB_TIMEOUT_BASIC;

    os_printf("Close video device\r\n");

    video_class->is_opened = FALSE;

    ep_desc = &video_class->isoin;

    if((ep_desc->bmAttributes & USB_EP_ATTR_TYPE_MASK) == USB_EP_ATTR_ISOC)
    {
        os_printf("ISO set interface 0\r\n");
        setup.request_type = USB_REQ_TYPE_DIR_OUT | USB_REQ_TYPE_STANDARD |
            USB_REQ_TYPE_INTERFACE;
        setup.bRequest = USB_REQ_SET_INTERFACE;
        setup.wValue = 0;
        setup.wIndex = video_class->data_intf;
        setup.wLength = 0;

        if(rt_usb_hcd_setup_xfer(device->hcd, device->pipe_ep0_out, &setup, timeout) != 8)
        {
            return RT_ERROR;
        }
        if(rt_usb_hcd_pipe_xfer(video_class->device->hcd, video_class->device->pipe_ep0_in, RT_NULL, 0, timeout) != 0)
        {
            return RT_ERROR;
        }
    }

    if((ep_desc->bmAttributes & USB_EP_ATTR_TYPE_MASK) == USB_EP_ATTR_BULK)
    {
        os_printf("BULK clear ep feature: 0x%x\n",(ep_desc->bEndpointAddress));
        rt_usbh_clear_feature(device, ep_desc->bEndpointAddress, 0);
    }



    return RT_EOK;
}

void usbh_video_list_info(struct usbh_video *video_class)
{


    os_printf("============= Video device information ===================\r\n");
    os_printf("bcdVDC:%04x\r\n", video_class->bcdVDC);
    os_printf("Num of altsettings:%u\r\n", video_class->num_of_intf_altsettings);
    for (rt_uint8_t i = 0; i < video_class->num_of_intf_altsettings; i++) {
        if (video_class->intf_altersetting[i].check_use == 0)
            continue;
        uep_desc_t ep_desc = video_class->intf_altersetting[i].ep_desc_t;
        os_printf("  Altsetting:%u, Ep=%02x Attr=%02u Mps=%d Interval=%02u\r\n",
            i,
            ep_desc->bEndpointAddress,
            ep_desc->bmAttributes,
            ep_desc->wMaxPacketSize,
            ep_desc->bInterval);
    }
    os_printf("\r\n");
    os_printf("bNumFormats:%u\r\n", video_class->num_of_formats);
    for (rt_uint8_t i = 0; i < video_class->num_of_formats; i++) {
        os_printf("\r\n");
        os_printf("  FormatIndex:%u\r\n", i + 1);
        os_printf("  FormatType:%s\r\n", format_type[video_class->format[i].format_type]);
        os_printf("  bNumFrames:%u\r\n", video_class->format[i].num_of_frames);
        os_printf("  Resolution:\r\n");
        for (rt_uint8_t j = 0; j < video_class->format[i].num_of_frames; j++) {
            os_printf("      FrameIndex:%u\r\n", j + 1);
            os_printf("      wWidth: %d, wHeight: %d (%d Fps)\r\n",
                         video_class->format[i].frame[j].wWidth,
                         video_class->format[i].frame[j].wHeight,
                         (video_class->format[i].frame[j].dwDefaultFrameInterval ? (10000000/video_class->format[i].frame[j].dwDefaultFrameInterval) : 0));
        }
    }

    os_printf("============= Video device information ===================\r\n");
}

static void usbh_video_intf_altersetting_ep_config(struct usbh_video *video_class)
{
    uep_desc_t ep_desc = NULL;

    if (video_class->intf_altersetting[VIDEO_SET_INTF_ALTSETTING].check_use)
    {
        ep_desc = video_class->intf_altersetting[VIDEO_SET_INTF_ALTSETTING].ep_desc_t;
        video_class->cur_set_altersetting_num = VIDEO_SET_INTF_ALTSETTING;  
        goto __exit_end;      
    }

    if(VIDEO_SET_INTF_ALTSETTING > (video_class->num_of_intf_altsettings - 1))
    {
        for(rt_uint8_t i = 0; i < video_class->num_of_intf_altsettings; i++)
        {
            if (video_class->intf_altersetting[i].check_use == 0)
            {
                if (i == video_class->num_of_intf_altsettings-1)
                {
                    //正常情况不会进入该条件
                    os_printf("No found altersetting ep!!!!!!\n");
                }
                continue;
            }
            else
            {
                ep_desc = video_class->intf_altersetting[i].ep_desc_t;
                video_class->cur_set_altersetting_num = i;
                break;
            }
        }    
    }

__exit_end:
    if(((ep_desc->bEndpointAddress & USB_DIR_MASK) == USB_DIR_IN ) && 
        ((ep_desc->bmAttributes & USB_EP_ATTR_TYPE_MASK) == USB_EP_ATTR_ISOC))
    {
        video_class->isoin = *ep_desc;  
        video_class->video_rx_size = ep_desc->wMaxPacketSize;

        video_class->uvc_head = 0;
    }

    if(((ep_desc->bEndpointAddress & USB_DIR_MASK) == USB_DIR_IN ) && 
        ((ep_desc->bmAttributes & USB_EP_ATTR_TYPE_MASK) == USB_EP_ATTR_BULK))
    {
        video_class->isoin = *ep_desc;  
        video_class->video_rx_size = ep_desc->wMaxPacketSize;

        video_class->uvc_head = 12+4;  //BULK HEAD
    }  

}

static rt_err_t rt_usbh_class_driver_video_enable(void *arg)
{
    int ret;
    struct uhintf **p_intf = arg;
    rt_uint8_t cur_iface = 0xff;
    rt_uint8_t cur_alt_setting = 0xff;
    rt_uint8_t cur_class = 0xff;
    rt_uint8_t frame_index = 0xff;
    rt_uint8_t format_index = 0xff;
    rt_uint8_t num_of_frames = 0xff;
    rt_uint8_t *p;
    rt_uint8_t intf;
    uhcd_t hcd = NULL;
    rt_int32_t cfg_len = 0;

    if (p_intf[0] == NULL) {
        return -EIO;
    }    

    hcd = p_intf[0]->device->hcd;
    os_printf("subclass %d, protocal %d intfnum:%d\r\n",p_intf[0]->intf_desc->bInterfaceSubClass,p_intf[0]->intf_desc->bInterfaceProtocol, p_intf[0]->intf_desc->bInterfaceNumber);
    os_printf("device:%x cfg_desc:%x hcd:%x\n",(uinst_t)p_intf[0]->device,p_intf[0]->device->cfg_desc,p_intf[0]->device->hcd);

    struct usbh_video *video_class = usbh_video_class_alloc();
    if (video_class == NULL) {
        os_printf("Fail to alloc video_class\r\n");
        return RT_ENOMEM;
    }

    intf = p_intf[0]->intf_desc->bInterfaceNumber;

    video_class->device = p_intf[0]->device;
    video_class->ctrl_intf = intf;
    video_class->data_intf = intf + 1;
    video_class->num_of_intf_altsettings = 2;
    video_class->video_rx_size = 1024;  

    p_intf[0]->user_data = video_class;
    os_printf("p_intf[0]:%x\n",p_intf[0]);

    p = (rt_uint8_t *)p_intf[0]->device->cfg_desc;
    cfg_len = p_intf[0]->device->cfg_desc->wTotalLength;
    os_printf("cfg_len:%x\n",cfg_len);

    while (p[DESC_bLength]) {
        switch (p[DESC_bDescriptorType]) {
            case USB_DESC_TYPE_INTERFACE:
                //os_printf("USB_DESC_TYPE_INTERFACE :%d\n",p[DESC_bDescriptorType]);
                cur_iface = p[INTF_DESC_bInterfaceNumber];
                cur_alt_setting = p[INTF_DESC_bAlternateSetting];
                cur_class = p[INTF_DESC_bInterfaceClass];
                if (cur_iface == video_class->data_intf){
                    video_class->num_of_intf_altsettings = cur_alt_setting + 1;
                }
                break;
            case USB_DESC_TYPE_ENDPOINT:
                //os_printf("USB_DESC_TYPE_ENDPOINT\n");
                if ((cur_iface == video_class->data_intf) && (cur_class == USB_CLASS_VIDEO))
                {
                    video_class->intf_altersetting[cur_alt_setting].ep_desc_t = (uep_desc_t)p;
                    video_class->intf_altersetting[cur_alt_setting].altersetting_num = cur_alt_setting;
                    video_class->intf_altersetting[cur_alt_setting].check_use = 1;
                }
                break;
            case VIDEO_CS_INTERFACE_DESCRIPTOR_TYPE:
                if (cur_iface == video_class->ctrl_intf) {
                    //os_printf("VIDEO_CS_INTERFACE_DESCRIPTOR_TYPE 1 cur_iface:%d p[DESC_bDescriptorSubType]:0x%x\n",cur_iface,p[DESC_bDescriptorSubType]);
                    switch (p[DESC_bDescriptorSubType]) {
                        case VIDEO_VC_HEADER_DESCRIPTOR_SUBTYPE:
                            video_class->bcdVDC = ((rt_uint16_t)p[4] << 8) | (rt_uint16_t)p[3];
                            break;
                        case VIDEO_VC_INPUT_TERMINAL_DESCRIPTOR_SUBTYPE:
                            break;
                        case VIDEO_VC_OUTPUT_TERMINAL_DESCRIPTOR_SUBTYPE:
                            break;
                        case VIDEO_VC_PROCESSING_UNIT_DESCRIPTOR_SUBTYPE:
                            break;

                        default:
                            break;
                    }
                } else if (cur_iface == video_class->data_intf) {
                    //os_printf("VIDEO_CS_INTERFACE_DESCRIPTOR_TYPE 2, cur_inface:%d p[DESC_bDescriptorSubType]:0x%x\n",cur_iface,p[DESC_bDescriptorSubType]);
                    switch (p[DESC_bDescriptorSubType]) {
                        case VIDEO_VS_INPUT_HEADER_DESCRIPTOR_SUBTYPE:
                            video_class->num_of_formats = p[DESC_bNumFormats];
                            if(video_class->num_of_formats > USBH_VIDEO_MAX_FORMAT_NUM) {
                                os_printf("The value of USBH_VIDEO_MAX_FORMAT_NUM needs to be increased,USBH_VIDEO_MAX_FORMAT_NUM:%d video_class->num_of_formats:%d\n",
                                            USBH_VIDEO_MAX_FORMAT_NUM, video_class->num_of_formats);
                            }
                            break;
                        case VIDEO_VS_FORMAT_UNCOMPRESSED_DESCRIPTOR_SUBTYPE:
                            format_index = p[DESC_bFormatIndex];
                            num_of_frames = p[DESC_bNumFrameDescriptors];
                            if (num_of_frames > USBH_VIDEO_MAX_FRAME_NUM) {
                                os_printf("The value of USBH_VIDEO_MAX_FRAME_NUM needs to be increased,USBH_VIDEO_MAX_FRAME_NUM:%d num_of_frames:%d\n",
                                    USBH_VIDEO_MAX_FRAME_NUM, num_of_frames);
                            }

                            video_class->format[format_index - 1].num_of_frames = num_of_frames;
                            video_class->format[format_index - 1].format_type = USBH_VIDEO_FORMAT_UNCOMPRESSED;
                            break;
                        case VIDEO_VS_FORMAT_MJPEG_DESCRIPTOR_SUBTYPE:
                            format_index = p[DESC_bFormatIndex];
                            num_of_frames = p[DESC_bNumFrameDescriptors];
                            if (num_of_frames > USBH_VIDEO_MAX_FRAME_NUM) {
                                os_printf("The value of USBH_VIDEO_MAX_FRAME_NUM needs to be increased,USBH_VIDEO_MAX_FRAME_NUM:%d num_of_frames:%d\n",
                                    USBH_VIDEO_MAX_FRAME_NUM, num_of_frames);
                            }

                            video_class->format[format_index - 1].num_of_frames = num_of_frames;
                            video_class->format[format_index - 1].format_type = USBH_VIDEO_FORMAT_MJPEG;
                            break;
                        case VIDEO_VS_FORMAT_FRAME_BASED_DESCRIPTOR_SUBTYPE:
                            format_index = p[DESC_bFormatIndex];
                            num_of_frames = p[DESC_bNumFrameDescriptors];
                            if (num_of_frames > USBH_VIDEO_MAX_FRAME_NUM) {
                                os_printf("The value of USBH_VIDEO_MAX_FRAME_NUM needs to be increased,USBH_VIDEO_MAX_FRAME_NUM:%d num_of_frames:%d\n",
                                    USBH_VIDEO_MAX_FRAME_NUM, num_of_frames);
                            }

                            video_class->format[format_index - 1].num_of_frames = num_of_frames;
                            video_class->format[format_index - 1].format_type = USBH_VIDEO_FORMAT_BASED;
                            break;
                        case VIDEO_VS_FRAME_UNCOMPRESSED_DESCRIPTOR_SUBTYPE:
                            frame_index = p[DESC_bFrameIndex];

                            video_class->format[format_index - 1].frame[frame_index - 1].wWidth = ((struct video_cs_if_vs_frame_uncompressed_descriptor *)p)->wWidth;
                            video_class->format[format_index - 1].frame[frame_index - 1].wHeight = ((struct video_cs_if_vs_frame_uncompressed_descriptor *)p)->wHeight;
                            video_class->format[format_index - 1].frame[frame_index - 1].dwDefaultFrameInterval = ((struct video_cs_if_vs_frame_uncompressed_descriptor *)p)->dwDefaultFrameInterval;
                            break;
                        case VIDEO_VS_FRAME_MJPEG_DESCRIPTOR_SUBTYPE:
                            frame_index = p[DESC_bFrameIndex];

                            video_class->format[format_index - 1].frame[frame_index - 1].wWidth = ((struct video_cs_if_vs_frame_mjpeg_descriptor *)p)->wWidth;
                            video_class->format[format_index - 1].frame[frame_index - 1].wHeight = ((struct video_cs_if_vs_frame_mjpeg_descriptor *)p)->wHeight;
                            video_class->format[format_index - 1].frame[frame_index - 1].dwDefaultFrameInterval = ((struct video_cs_if_vs_frame_mjpeg_descriptor *)p)->dwDefaultFrameInterval;
                            break;
                        case VIDEO_VS_FRAME_FRAME_BASED_DESCRIPTOR_SUBTYPE:
                            frame_index = p[DESC_bFrameIndex];

                            video_class->format[format_index - 1].frame[frame_index - 1].wWidth = ((struct video_cs_if_vs_frame_h26x_descriptor *)p)->wWidth;
                            video_class->format[format_index - 1].frame[frame_index - 1].wHeight = ((struct video_cs_if_vs_frame_h26x_descriptor *)p)->wHeight;
                            video_class->format[format_index - 1].frame[frame_index - 1].dwDefaultFrameInterval = ((struct video_cs_if_vs_frame_h26x_descriptor *)p)->dwDefaultFrameInterval;
                            break;

                        default:
                            os_printf("VIDEO_CS_INTERFACE_DESCRIPTOR_TYPE default %X\n",p[DESC_bDescriptorSubType]);
                            break;
                    }
                }else{
                    //os_printf("cur_iface:%d default:0x%x\n",cur_iface,p[DESC_bDescriptorSubType]);
                }

                break;

            default:
                break;
        }
        /* skip to next descriptor */
        if (cfg_len > 0) {
            cfg_len -= p[DESC_bLength];
        }
        //os_printf("cfg_len:%d\n",cfg_len);
        if (cfg_len <= 0) {
            break;
        }
        p += p[DESC_bLength];
    }

    usbh_video_list_info(video_class);

    if (cfg_len != 0) {
        os_printf("UVC cfg_desc analysis failed\n");
        return RT_ERROR;
    }

    usbh_video_intf_altersetting_ep_config(video_class);

    video_class->rx_buff = (rt_uint8_t *)rt_malloc(video_class->video_rx_size+video_class->uvc_head);
    if(video_class->rx_buff == RT_NULL) {
        os_printf("malloc rx_buff fail!!!!!!!!!!!\n");
        return RT_ENOMEM;
    }
    os_printf("video_class->rx_buff:%x video_class->uvc_head:%d\n",video_class->rx_buff,video_class->uvc_head);

    #if USBH_VIDEO_PPB
    video_class->usbh_pingpang_flag = 0;
    video_class->rx_double_buff = (rt_uint8_t *)rt_malloc(video_class->video_rx_size+video_class->uvc_head);
    if(video_class->rx_double_buff == RT_NULL) {
        rt_free(video_class->rx_buff);
        video_class->rx_buff = RT_NULL;
        os_printf("malloc rx_double_buff fail!!!!!!!!!\n");
        return RT_ENOMEM;
    }    

    os_printf("video_class->rx_double_buff:%x video_class->uvc_head:%d\n",video_class->rx_double_buff,video_class->uvc_head);
    #endif

    ret = usbh_video_close(p_intf[0]->device,video_class);
    if (ret == RT_ERROR) {
        os_printf("Fail to close video device\r\n");
        return ret;
    }

    snprintf(video_class->devname, CONFIG_USBHOST_DEV_NAMELEN, DEV_FORMAT, video_class->minor);

    os_printf("Register Video Class:%s\r\n", video_class->devname);
    os_printf("video_class device:%x hcd:%x\n",video_class->device,video_class->device->hcd);
    usbh_video_run(video_class);

    return RT_EOK;
}

static rt_err_t rt_usbh_class_driver_video_disable(void *arg)
{
    struct uhintf *p_intf = arg;
    struct usbh_video *video_class;

    if (p_intf == NULL) {
        return -EIO;
    }  
    os_printf("%s %d p_intf:%x\n",__FUNCTION__,__LINE__,p_intf);
    video_class = (struct usbh_video *)p_intf->user_data;

    if (video_class) {
        if (video_class->pipe_in != RT_NULL) {
        }

        if (video_class->pipe_out != RT_NULL) {
        }

        if (video_class->devname[0] != '\0') {
            os_printf("Unregister Video Class:%s\r\n", video_class->devname);
            rtt_usbh_video_user_close(video_class->minor);
            usbh_video_stop(video_class);
        }

        if (video_class->rx_buff != RT_NULL) {
            rt_free(video_class->rx_buff);
            video_class->rx_buff = RT_NULL;
        }

        #if USBH_VIDEO_PPB
        if (video_class->rx_double_buff != RT_NULL) {
            rt_free(video_class->rx_double_buff);
            video_class->rx_double_buff = RT_NULL;
        }
        #endif

        usbh_video_class_free(video_class);
    }

    return RT_EOK;

}

static rt_uint32_t rt_usbh_video_class_ep_search_devnum(rt_uint8_t ep, uhcd_t hcd)
{
    rt_uint32_t dev_num = 0xFF;

    for (int i = 0; i < CONFIG_USBHOST_MAX_VIDEO_CLASS; i++) {
        if ( (ep == g_video_class[i].pipe_in->pipe_index) && 
             (g_video_class[i].pipe_in != RT_NULL)        && 
             (g_video_class[i].device->hcd == hcd)          )           
        {
            if ((g_devinuse & (1 << i)) == 0) {
                break;
            }

            dev_num = i;
            break;
        }
    }

    return dev_num;
}

/* ----------------------------- USB 2.0 / USB 1.1 UVC IRQ ----------------------------- */

bool rtt_uvc_data_deal(void *p_dev, rt_uint8_t ep, rt_uint32_t *rx_dma_len)
{
    rt_int32_t rx_len = 0;
    usb_device_ioctl((struct usb_device *)p_dev, USB_HOST_GET_RX_DMA_LEN, ep, (rt_uint32_t)&rx_len);

    if(rx_len == 0){
        return 1;
    }
	
	if(rx_len < 0){
        os_printf("The length of the configured dma needs to be increased\n");
        return 1;
    }
	
    *rx_dma_len = rx_len;
    // printf("rx len:%d\n",rx_packet_len);

    return 0;
}



void rtt_usbh_video_irq(void * p_dev, rt_uint8_t ep, uhcd_t hcd)
{   
    rt_uint32_t ret;
    rt_uint32_t dev_num;
    rt_uint32_t rx_packet_len = 0;
    struct usb_device_dma_cfg dma_cfg;
    
    if (!p_dev)
        return;

    dev_num = rt_usbh_video_class_ep_search_devnum(ep, hcd);

#if USBH_VIDEO_PPB
    switch(dev_num)
    {
        case 0:
        {
            usb_dma_mjpeg_irq_times++;
            ret = rtt_uvc_data_deal(p_dev, ep, &rx_packet_len); 
            
            if(g_video_class[dev_num].usbh_pingpang_flag)
            {
                dma_cfg.ep_index = ep;
                dma_cfg.ep_dir   = USB_CORE_HOST_EP_RX;
                dma_cfg.dma_addr = (rt_uint32_t *)(g_video_class[dev_num].rx_buff+g_video_class[dev_num].uvc_head);
                dma_cfg.dma_len = g_video_class[dev_num].video_rx_size;
                usb_device_ioctl((struct usb_device *)p_dev, USB_HOST_KICK_EP_DMA, (rt_uint32_t)&dma_cfg, (rt_uint32_t)RT_NULL);
                if(ret == 0) { 
                    uint8_t drop = 0;
                    if(usb_device_ioctl((struct usb_device *)p_dev, USB_HOST_ISO_IS_CRC_ERR, ep, (uint32_t)NULL)) {
                        drop = 1; 
                    }                    
                    process_uvc_payload(dev_num, (g_video_class[dev_num].isoin.bmAttributes & USB_EP_ATTR_TYPE_MASK), g_video_class[dev_num].current_format, (g_video_class[dev_num].rx_double_buff+g_video_class[dev_num].uvc_head), rx_packet_len, drop);
                }
                g_video_class[dev_num].usbh_pingpang_flag = 0;   
            }
            else
            {
                dma_cfg.ep_index = ep;
                dma_cfg.ep_dir   = USB_CORE_HOST_EP_RX;
                dma_cfg.dma_addr = (rt_uint32_t *)(g_video_class[dev_num].rx_double_buff+g_video_class[dev_num].uvc_head);
                dma_cfg.dma_len = g_video_class[dev_num].video_rx_size;
                usb_device_ioctl((struct usb_device *)p_dev, USB_HOST_KICK_EP_DMA, (rt_uint32_t)&dma_cfg, (rt_uint32_t)RT_NULL);            
                if(ret == 0) {
                    uint8_t drop = 0;
                    if(usb_device_ioctl((struct usb_device *)p_dev, USB_HOST_ISO_IS_CRC_ERR, ep, (uint32_t)NULL)) {
                        drop = 1; 
                    }                    
                    process_uvc_payload(dev_num, (g_video_class[dev_num].isoin.bmAttributes & USB_EP_ATTR_TYPE_MASK), g_video_class[dev_num].current_format, (g_video_class[dev_num].rx_buff+g_video_class[dev_num].uvc_head), rx_packet_len, drop);
                }
                g_video_class[dev_num].usbh_pingpang_flag = 1;
            }
        }
            break;

        case 1:
        {
            usb_dma_h264_irq_times++;
            ret = rtt_uvc_data_deal(p_dev, ep, &rx_packet_len); 
            
            if(g_video_class[dev_num].usbh_pingpang_flag)
            {
                dma_cfg.ep_index = ep;
                dma_cfg.ep_dir   = USB_CORE_HOST_EP_RX;
                dma_cfg.dma_addr = (rt_uint32_t *)(g_video_class[dev_num].rx_buff+g_video_class[dev_num].uvc_head);
                dma_cfg.dma_len = g_video_class[dev_num].video_rx_size;
                usb_device_ioctl((struct usb_device *)p_dev, USB_HOST_KICK_EP_DMA, (rt_uint32_t)&dma_cfg, (rt_uint32_t)RT_NULL); 
                if(ret == 0) {
                    uint8_t drop = 0;
                    if(usb_device_ioctl((struct usb_device *)p_dev, USB_HOST_ISO_IS_CRC_ERR, ep, (uint32_t)NULL)) {
                        drop = 1; 
                    }                    
                    process_uvc_payload(dev_num, (g_video_class[dev_num].isoin.bmAttributes & USB_EP_ATTR_TYPE_MASK), g_video_class[dev_num].current_format, (g_video_class[dev_num].rx_double_buff+g_video_class[dev_num].uvc_head), rx_packet_len, drop);
                }
                g_video_class[dev_num].usbh_pingpang_flag = 0;                          
            }
            else
            {
                dma_cfg.ep_index = ep;
                dma_cfg.ep_dir   = USB_CORE_HOST_EP_RX;
                dma_cfg.dma_addr = (rt_uint32_t *)(g_video_class[dev_num].rx_double_buff+g_video_class[dev_num].uvc_head);
                dma_cfg.dma_len = g_video_class[dev_num].video_rx_size;
                usb_device_ioctl((struct usb_device *)p_dev, USB_HOST_KICK_EP_DMA, (rt_uint32_t)&dma_cfg, (rt_uint32_t)RT_NULL);             
                if(ret == 0) {
                    uint8_t drop = 0;
                    if(usb_device_ioctl((struct usb_device *)p_dev, USB_HOST_ISO_IS_CRC_ERR, ep, (uint32_t)NULL)) {
                        drop = 1; 
                    }                    
                    process_uvc_payload(dev_num, (g_video_class[dev_num].isoin.bmAttributes & USB_EP_ATTR_TYPE_MASK), g_video_class[dev_num].current_format, (g_video_class[dev_num].rx_buff+g_video_class[dev_num].uvc_head), rx_packet_len, drop);
                }
                g_video_class[dev_num].usbh_pingpang_flag = 1;              
            }
        }
            break;
        
        default:
            break;
    }
#else
    switch(dev_num)
    {
        case 0:
        {
            usb_dma_mjpeg_irq_times++;
            ret = rtt_uvc_data_deal(p_dev, ep, &rx_packet_len); 
            if(ret == 0) {
                uint8_t drop = 0;
                if(usb_device_ioctl((struct usb_device *)p_dev, USB_HOST_ISO_IS_CRC_ERR, ep, (uint32_t)NULL)) {
                    drop = 1; 
                }                    
                process_uvc_payload(dev_num, (g_video_class[dev_num].isoin.bmAttributes & USB_EP_ATTR_TYPE_MASK), g_video_class[dev_num].current_format, (g_video_class[dev_num].rx_buff+g_video_class[dev_num].uvc_head), rx_packet_len, drop);
            }
            dma_cfg.ep_index = ep;
            dma_cfg.ep_dir   = USB_CORE_HOST_EP_RX;
            dma_cfg.dma_addr = (rt_uint32_t *)(g_video_class[dev_num].rx_buff+g_video_class[dev_num].uvc_head);
            dma_cfg.dma_len = g_video_class[dev_num].video_rx_size;
            usb_device_ioctl((struct usb_device *)p_dev, USB_HOST_KICK_EP_DMA, &dma_cfg, (rt_uint32_t)RT_NULL); 
        }
            break;

        case 1:
        {
            usb_dma_h264_irq_times++;
            ret = rtt_uvc_data_deal(p_dev, ep, &rx_packet_len); 
            if(ret == 0) {
                uint8_t drop = 0;
                if(usb_device_ioctl((struct usb_device *)p_dev, USB_HOST_ISO_IS_CRC_ERR, ep, (uint32_t)NULL)) {
                    drop = 1; 
                }                    
                process_uvc_payload(dev_num, (g_video_class[dev_num].isoin.bmAttributes & USB_EP_ATTR_TYPE_MASK), g_video_class[dev_num].current_format, (g_video_class[dev_num].rx_buff+g_video_class[dev_num].uvc_head), rx_packet_len, drop);
            }
            dma_cfg.ep_index = ep;
            dma_cfg.ep_dir   = USB_CORE_HOST_EP_RX;
            dma_cfg.dma_addr = (rt_uint32_t *)(g_video_class[dev_num].rx_buff+g_video_class[dev_num].uvc_head);
            dma_cfg.dma_len = g_video_class[dev_num].video_rx_size;
            usb_device_ioctl((struct usb_device *)p_dev, USB_HOST_KICK_EP_DMA, &dma_cfg, (rt_uint32_t)RT_NULL);
        }
            break;
        
        default:
            break;
    }
#endif
}


rt_uint32_t rtt_usbh_video_dev_pipe_manage(rt_uint8_t dev_num, rt_uint8_t alloc_or_free)
{
    uep_desc_t ep_desc;
    upipe_t pipe;
    struct usbh_video *video_class = RT_NULL;

    if(dev_num >= CONFIG_USBHOST_MAX_VIDEO_CLASS)
    {
        os_printf("Invalid dev_num %d!!!!!!!\n",dev_num);
        return -RT_ERROR;
    }

    video_class = &g_video_class[dev_num];

    ep_desc = &video_class->isoin;

    os_printf("Video Class dev%d: Altsetting:%u, Ep=%02x Attr=%02u Mps=%d Interval=%02u\r\n",
                dev_num,
                video_class->cur_set_altersetting_num,
                ep_desc->bEndpointAddress,
                ep_desc->bmAttributes,
                ep_desc->wMaxPacketSize,
                ep_desc->bInterval);
    if(ep_desc->bEndpointAddress == 0 || ep_desc->bmAttributes == 0 || ep_desc->wMaxPacketSize == 0)
    {
        return -RT_ERROR;
    }

    if(alloc_or_free)
    {
        pipe = rt_usb_instance_find_pipe(video_class->device, ep_desc->bEndpointAddress);

        if(pipe != RT_NULL) {
            os_printf("g_video_class[%d] pipe has been alloced!!!\n",dev_num);
            return -RT_ERROR;
        }

        if (rt_usb_hcd_alloc_pipe(video_class->device->hcd, &pipe, video_class->device, ep_desc) != RT_EOK) {
            rt_kprintf("alloc iso pipe failed\n");
            return -RT_ERROR;
        }
        os_printf("alloc iso pipe ok %x\n",pipe);

        rt_usb_instance_add_pipe(video_class->device, pipe);

        video_class->pipe_in = pipe;
    }
    else
    {
        if(video_class->pipe_in != RT_NULL) {
            rt_list_remove(&video_class->pipe_in->list);
            rt_usb_hcd_free_pipe(video_class->device->hcd, video_class->pipe_in);
            video_class->pipe_in = RT_NULL;
        } else {
            os_printf("g_video_class[%d] pipe_in is NULL, not free\n",dev_num);
            return -RT_ERROR;
        }

    }

    return RT_EOK;
}



rt_uint32_t rtt_usbh_video_user_open(rt_uint8_t dev_num)
{
    rt_uint32_t ret;
    struct usb_device *p_dev = NULL;

    if ((g_devinuse & (1 << dev_num)) == 0)
    {
        os_printf("video/dev%d is not use!!!\n",dev_num);
        return -RT_ERROR;
    }

    ret = rtt_usbh_video_dev_pipe_manage(dev_num, VIDEO_ALLOC_PIPE);
    if(ret)
    {
        os_printf("alloc pipe failed!!!!!!!!!!!\n");
        return -RT_ERROR;
    }
    
    switch(g_video_class[dev_num].device->hcd->devid)
    {
        case HG_USB11_HOST_CONTROLLER_DEVID:
        {
            os_printf("HG_USB11_HOST_CONTROLLER_DEVID\n");
			p_dev = (struct usb_device *)dev_get(HG_USB11HOST_DEVID);
            if (p_dev) {
                os_printf("kick usb11\n");
            };
        }
			break;
        
        case HG_USB_HOST_CONTROLLER_DEVID:
		{
            os_printf("HG_USB_HOST_CONTROLLER_DEVID\n");
            p_dev = (struct usb_device *)dev_get(HG_USBDEV_DEVID);
            if (p_dev) {
                os_printf("kick usb20\n");
            };
		}
            break;
        default:
            break;
    }

    switch(dev_num)
    {
        case 0:
            usbh_video_open((&g_video_class[0]), USBH_VIDEO_FORMAT_MJPEG, 640, 480, VIDEO_SET_INTF_ALTSETTING);
            #ifdef PSRAM_HEAP
            extern struct usbh_video_priv_func uvc_host_stream_1;
            uvc_host_stream_1.dev_num = dev_num;
            uvc_host_stream_1.p_dev = p_dev;
            usbh_video_enum_finish_init(g_video_class[0].devname,(F_JPG << 8) | (FSTYPE_USB_CAM0),&uvc_host_stream_1,&usb_dma_mjpeg_irq_times);
            #else

            #endif
        break;

        case 1:
            usbh_video_open((&g_video_class[1]), USBH_VIDEO_FORMAT_MJPEG, 1280, 720, VIDEO_SET_INTF_ALTSETTING);
            #ifdef PSRAM_HEAP
            extern struct usbh_video_priv_func uvc_host_stream_2;
            uvc_host_stream_2.dev_num = dev_num;
            uvc_host_stream_2.p_dev = p_dev;
            usbh_video_enum_finish_init(g_video_class[1].devname,(F_JPG << 8) | (FSTYPE_USB_CAM1),&uvc_host_stream_2,&usb_dma_h264_irq_times);
            #else

            #endif
        break;
        
        default:
        break;
    }

    struct usb_device_dma_cfg dma_cfg;
    dma_cfg.ep_index = g_video_class[dev_num].pipe_in->pipe_index;
    dma_cfg.ep_dir   = USB_CORE_HOST_EP_RX;
    dma_cfg.dma_addr = (rt_uint32_t *)(g_video_class[dev_num].rx_buff+g_video_class[dev_num].uvc_head);
    dma_cfg.dma_len = g_video_class[dev_num].video_rx_size;
    usb_device_ioctl((struct usb_device *)p_dev, USB_HOST_KICK_EP_DMA, (rt_uint32_t)&dma_cfg, (rt_uint32_t)RT_NULL); 

    return RT_EOK;
}

rt_uint32_t rtt_usbh_video_user_close(rt_uint8_t dev_num)
{
	rt_uint32_t ret;

    if ((g_devinuse & (1 << dev_num)) == 0)
    {
        os_printf("video/dev%d is not use!!!\n",dev_num);
        return -RT_ERROR;
    }


    ret = rtt_usbh_video_dev_pipe_manage(dev_num, VIDEO_FREE_PIPE);
    if(ret)
    {
        os_printf("free pipe failed!!!!!!!!!!!\n");
        return -RT_ERROR;
    } 
   
    switch(dev_num)
    {
        case 0:
            #ifdef PSRAM_HEAP

            #else

            #endif
        break;

        case 1:
            #ifdef PSRAM_HEAP

            #else

            #endif
        break;
        
        default:
        break;
    }
    usbh_video_close((&g_video_class[dev_num])->device,&g_video_class[dev_num]);

	return RT_EOK;
}

int32 demo_atcmd_select_resolution(const char *cmd, char *argv[], uint32 argc)
{
    //摄像头0，不带UAC
    if (*argv[0] == '1') {
        //打开摄像头0
        if (*argv[1] == '1') {      
            rtt_usbh_video_user_open(0);
        }
        //关闭摄像头0
        if (*argv[1] == '0') {  
            rtt_usbh_video_user_close(0);
        }
    }
    //摄像头1，带UAC
    else if (*argv[0] == '2') {
        //打开摄像头1
        if (*argv[1] == '1') {
            rtt_usbh_video_user_open(1);
            #ifdef RT_USBH_UAC
            rtt_usbh_audio_user_open();    //先打开视频流，再打开音频流
            #endif
        }
        //关闭摄像头1
        if (*argv[1] == '0') {
            #ifdef RT_USBH_UAC
            rtt_usbh_audio_user_close();    //先关闭音频流，再关闭视频流
            #endif 
            rtt_usbh_video_user_close(1);
        }
    }

	printf("OK/n");
    return 0;
}


void usbh_video_thread_entry(void *arg)
{
    //struct usbh_video *video_class = arg;

}

__attribute__((weak)) void usbh_video_run(struct usbh_video *video_class)
{

    switch (video_class->minor)
    {
        case 0:
            os_printf("usbh_video_run video dev 0\n");
            rtt_usbh_video_user_open(0);    // 默认摄像头1为 MJPEG格式 640 x 480
        break;


        case 1:
            os_printf("usbh_video_run video dev 1\n");
            rtt_usbh_video_user_open(1);    // 默认摄像头2为 H264格式 1280 x 720
        break;

        default:
        break;
    }


#if 0
	rt_thread_t usbh_video_thread;
    usbh_video_thread = rt_thread_create("usbh_video_thread", 
                                          usbh_video_thread_entry, 
                                          video_class, 
                                          1024, 
                                          OS_TASK_PRIORITY_NORMAL, 
                                          20);

    if(usbh_video_thread != RT_NULL)
        rt_thread_startup(usbh_video_thread);
#endif

}

__attribute__((weak)) void usbh_video_stop(struct usbh_video *video_class)
{
    switch (video_class->minor)
    {
        case 0:
            os_printf("usbh_video_stop video dev 0\n");
        break;

        case 1:
            os_printf("usbh_video_stop video dev 1\n");
        break;

        default:
        break;
    }

}

ucd_t rt_usbh_class_driver_video(void)
{
    uvc_device_pool_init();
    usbh_video_class.class_code = USB_CLASS_VIDEO;

    usbh_video_class.enable = rt_usbh_class_driver_video_enable;
    usbh_video_class.disable = rt_usbh_class_driver_video_disable;

    return &usbh_video_class;
};

#endif
