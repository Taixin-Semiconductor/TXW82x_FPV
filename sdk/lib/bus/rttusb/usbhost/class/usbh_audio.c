/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "rtthread.h"

#ifdef RT_USBH_UAC

#include "basic_include.h"
#include "rttusb_host.h"
#include "usbh_audio.h"

#define DEV_FORMAT "/dev/audio%d"

/* general descriptor field offsets */
#define DESC_bLength            0 /** Length offset */
#define DESC_bDescriptorType    1 /** Descriptor type offset */
#define DESC_bDescriptorSubType 2 /** Descriptor subtype offset */

/* interface descriptor field offsets */
#define INTF_DESC_bInterfaceNumber  2 /** Interface number offset */
#define INTF_DESC_bAlternateSetting 3 /** Alternate setting offset */

#define AUDIO_FREE_PIPE             0
#define AUDIO_ALLOC_PIPE            1

#define AUDIO_MIC_EN                1   //应用层MIC使能
#define AUDIO_SPK_EN                1   //应用层SPK使能
#define AUDIO_VOLUME_CONTROL_EN     0   //UAC音量控制使能 (根据设备是否支持该功能，自行选择打开)

#define AUDIO_SET_INTF_ALTSETTING   1

/* ---------------------------- MIC ---------------------------- */
#define AUDIO_MIC_SAMPLING_FREQ         8000     //设置MIC接收的采样率
#define AUDIO_MIC_RESOLUTION_BITS       16       //设置MIC接收的采样精度
#define AUDIO_MIC_MODULE_CHANNEL        1        //设置MIC接收的通道数

#define AUDIO_MIC_RESOLUTION_BYTES      (AUDIO_MIC_RESOLUTION_BITS / 8)
#define AUDIO_RX_PACKET_SIZE            ((AUDIO_MIC_SAMPLING_FREQ  * AUDIO_MIC_MODULE_CHANNEL * AUDIO_MIC_RESOLUTION_BYTES) / 1000)     //计算MIC每毫秒接收的数据包大小

/* ---------------------------- SPK ---------------------------- */
#define AUDIO_SPK_SAMPLING_FREQ         8000     //设置SPK发送的采样率
#define AUDIO_SPK_RESOLUTION_BITS       16       //设置SPK发送的采样精度
#define AUDIO_SPK_MODULE_CHANNEL        1        //设置SPK发送的通道数

#define AUDIO_SPK_RESOLUTION_BYTES      (AUDIO_SPK_RESOLUTION_BITS / 8)
#define AUDIO_TX_PACKET_SIZE            ((AUDIO_SPK_SAMPLING_FREQ  * AUDIO_SPK_MODULE_CHANNEL * AUDIO_SPK_RESOLUTION_BYTES) / 1000)     //计算SPK每毫秒发送的数据包大小


extern uint32 usbmic_packet_len;
extern uint8_t uac_run;
extern uint8_t uac_open;

static int usb_dma_uac_irq_times = 0;

static struct uclass_driver usbh_audio_class;

rt_uint8_t g_audio_buf[128];

struct usbh_audio g_audio_class[CONFIG_USBHOST_MAX_AUDIO_CLASS];
static rt_uint32_t g_devinuse = 0;

static struct usbh_audio *usbh_audio_class_alloc(void)
{
    int devno;

    for (devno = 0; devno < CONFIG_USBHOST_MAX_AUDIO_CLASS; devno++) {
        if ((g_devinuse & (1 << devno)) == 0) {
            g_devinuse |= (1 << devno);
            memset(&g_audio_class[devno], 0, sizeof(struct usbh_audio));
            g_audio_class[devno].minor = devno;
            return &g_audio_class[devno];
        }
    }
    return RT_NULL;
}

static void usbh_audio_class_free(struct usbh_audio *audio_class)
{
    int devno = audio_class->minor;

    if (devno >= 0 && devno < 32) {
        g_devinuse &= ~(1 << devno);
    }
    memset(audio_class, 0, sizeof(struct usbh_audio));
}

int usbh_audio_open(struct usbh_audio *audio_class, const char *name, rt_uint32_t samp_freq)
{
    struct urequest setup;
    struct uendpoint_descriptor *ep_desc = RT_NULL;
    rt_uint8_t intf = 0xff;
    rt_uint8_t altsetting = 1;
    int timeout = USB_TIMEOUT_BASIC;

    // if (audio_class->is_opened) {
    //     return 0;
    // }

    for (rt_uint8_t i = 0; i < audio_class->module_num; i++) {
        if (strcmp(name, audio_class->module[i].name) == 0) {
            printf("audio_class->module[%d]\n",i);
            for (rt_uint8_t j = 0; j < audio_class->num_of_intf_altsettings; j++) {
                for (rt_uint8_t k = 0; k < audio_class->module[i].altsetting[j].sampfreq_num; k++) {
                    if (audio_class->module[i].altsetting[j].sampfreq[k] == samp_freq) {
                        intf = audio_class->module[i].data_intf;
                        altsetting = j;
                        printf("usb host audio open:%d %d\n",intf,altsetting);
                        goto freq_found;
                    }
                }
            }
        }
    }

    return -EIO;

freq_found:

    setup.request_type = USB_REQ_TYPE_DIR_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE;
    setup.bRequest = USB_REQ_SET_INTERFACE;
    setup.wValue = altsetting;
    setup.wIndex = intf;
    setup.wLength = 0;

    if(rt_usb_hcd_setup_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_out, &setup, timeout) != 8)
    {
        return RET_ERR;
    }
    if(rt_usb_hcd_pipe_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_in, RT_NULL, 0, timeout) != 0)
    {
        return RET_ERR;
    }

    if (strcmp(name, "mic") == 0)
    {
        ep_desc = &audio_class->isoin;
    }
    else if (strcmp(name, "speaker") == 0)
    {
        ep_desc = &audio_class->isoout;
    }

    setup.request_type = USB_REQ_TYPE_DIR_OUT | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_ENDPOINT;
    setup.bRequest = SET_CUR;
    setup.wValue = (SAMPLING_FREQ_CONTROL << 8) | 0x00;
    setup.wIndex = ep_desc->bEndpointAddress;
    setup.wLength = 3;

    memcpy(g_audio_buf, &samp_freq, 3);

    if(rt_usb_hcd_setup_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_out, &setup, timeout) != 8)
    {
        return RET_ERR;
    }
    if(rt_usb_hcd_pipe_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_out, g_audio_buf, 3, timeout) != 3)
    {
        return RET_ERR;
    }
    if(rt_usb_hcd_pipe_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_in, RT_NULL, 0, timeout) != 0)
    {
        return RET_ERR;
    }

    os_printf("Open audio module :%s, altsetting: %u\r\n", name, altsetting);
    audio_class->is_opened = TRUE;
    uac_open = 1;
	uac_run = 1;
    return RT_EOK;
}

int usbh_audio_close(struct usbh_audio *audio_class, const char *name)
{
    struct urequest setup;
    rt_uint8_t intf = 0xff;
    int timeout = USB_TIMEOUT_BASIC;

    for (size_t i = 0; i < audio_class->module_num; i++) {
        if (strcmp(name, audio_class->module[i].name) == 0) {
            intf = audio_class->module[i].data_intf;
        }
    }

    if (intf == 0xff) {
        return -EIO;
    }

    LOG_D("Close audio module :%s\r\n", name);
    audio_class->is_opened = FALSE;

    setup.request_type = USB_REQ_TYPE_DIR_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE;
    setup.bRequest = USB_REQ_SET_INTERFACE;
    setup.wValue = 0;
    setup.wIndex = intf;
    setup.wLength = 0;

    if(rt_usb_hcd_setup_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_out, &setup, timeout) != 8)
    {
        return RT_ERROR;
    }
    if(rt_usb_hcd_pipe_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_in, RT_NULL, 0, timeout) != 0)
    {
        return RT_ERROR;
    }

    uac_open = 0;
	uac_run = 0;
    return RT_EOK;
}

int usbh_audio_get_min_volume(struct usbh_audio *audio_class, const char *name, rt_uint16_t *min_volume)
{
    struct urequest setup;
    rt_uint8_t intf = 0xff;
    rt_uint8_t feature_id = 0xff;
    rt_uint8_t ch;
    int timeout = USB_TIMEOUT_BASIC;

    for (size_t i = 0; i < audio_class->module_num; i++) {
        if (strcmp(name, audio_class->module[i].name) == 0) {
            intf = audio_class->ctrl_intf;
            feature_id = audio_class->module[i].feature_unit_id;
            ch = audio_class->module[i].altsetting[AUDIO_SET_INTF_ALTSETTING].channels;
        }
    }

    if (intf == 0xff) {
        return -EIO;
    }   

    setup.request_type = USB_REQ_TYPE_DIR_IN | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE;
    setup.bRequest = GET_MIN;
    setup.wValue = (VOLUME_CONTROL << 8) | ch;
    setup.wIndex = (feature_id << 8) | intf;
    setup.wLength = 2;    

    if(rt_usb_hcd_setup_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_out, &setup, timeout) != 8)
    {
        return RT_ERROR;
    }
    if(rt_usb_hcd_pipe_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_in, g_audio_buf, 2, timeout) != 2)
    {
        return RT_ERROR;
    }  
    if(rt_usb_hcd_pipe_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_out, RT_NULL, 0, timeout) != 0)
    {
        return RT_ERROR;
    }    

    memcpy(min_volume, g_audio_buf, 2);    
    return RT_EOK;
}

int usbh_audio_get_max_volume(struct usbh_audio *audio_class, const char *name, rt_uint16_t *max_volume)
{
    struct urequest setup;
    rt_uint8_t intf = 0xff;
    rt_uint8_t feature_id = 0xff;
    rt_uint8_t ch;
    int timeout = USB_TIMEOUT_BASIC;

    for (size_t i = 0; i < audio_class->module_num; i++) {
        if (strcmp(name, audio_class->module[i].name) == 0) {
            intf = audio_class->ctrl_intf;
            feature_id = audio_class->module[i].feature_unit_id;
            ch = audio_class->module[i].altsetting[AUDIO_SET_INTF_ALTSETTING].channels;
        }
    }

    if (intf == 0xff) {
        return -EIO;
    }   

    setup.request_type = USB_REQ_TYPE_DIR_IN | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE;
    setup.bRequest = GET_MAX;
    setup.wValue = (VOLUME_CONTROL << 8) | ch;
    setup.wIndex = (feature_id << 8) | intf;
    setup.wLength = 2;    

    if(rt_usb_hcd_setup_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_out, &setup, timeout) != 8)
    {
        return RT_ERROR;
    }
    if(rt_usb_hcd_pipe_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_in, g_audio_buf, 2, timeout) != 2)
    {
        return RT_ERROR;
    }  
    if(rt_usb_hcd_pipe_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_out, RT_NULL, 0, timeout) != 0)
    {
        return RT_ERROR;
    }        

    memcpy(max_volume, g_audio_buf, 2);
    return RT_EOK;
}

int usbh_audio_get_cur_volume(struct usbh_audio *audio_class, const char *name, rt_uint16_t *cur_volume)
{
    struct urequest setup;
    rt_uint8_t intf = 0xff;
    rt_uint8_t feature_id = 0xff;
    rt_uint8_t ch;
    int timeout = USB_TIMEOUT_BASIC;

    for (size_t i = 0; i < audio_class->module_num; i++) {
        if (strcmp(name, audio_class->module[i].name) == 0) {
            intf = audio_class->ctrl_intf;
            feature_id = audio_class->module[i].feature_unit_id;
            ch = audio_class->module[i].altsetting[AUDIO_SET_INTF_ALTSETTING].channels;
        }
    }

    if (intf == 0xff) {
        return -EIO;
    }   

    setup.request_type = USB_REQ_TYPE_DIR_IN | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE;
    setup.bRequest = GET_CUR;
    setup.wValue = (VOLUME_CONTROL << 8) | ch;
    setup.wIndex = (feature_id << 8) | intf;
    setup.wLength = 2;    

    if(rt_usb_hcd_setup_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_out, &setup, timeout) != 8)
    {
        return RT_ERROR;
    }
    if(rt_usb_hcd_pipe_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_in, g_audio_buf, 2, timeout) != 2)
    {
        return RT_ERROR;
    }  
    if(rt_usb_hcd_pipe_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_out, RT_NULL, 0, timeout) != 0)
    {
        return RT_ERROR;
    }        

    memcpy(cur_volume, g_audio_buf, 2);
    return RT_EOK;
}

int usbh_audio_get_res_volume(struct usbh_audio *audio_class, const char *name, rt_uint16_t *res_volume)
{
    struct urequest setup;
    rt_uint8_t intf = 0xff;
    rt_uint8_t feature_id = 0xff;
    rt_uint8_t ch;
    int timeout = USB_TIMEOUT_BASIC;

    for (size_t i = 0; i < audio_class->module_num; i++) {
        if (strcmp(name, audio_class->module[i].name) == 0) {
            intf = audio_class->ctrl_intf;
            feature_id = audio_class->module[i].feature_unit_id;
            ch = audio_class->module[i].altsetting[AUDIO_SET_INTF_ALTSETTING].channels;
        }
    }

    if (intf == 0xff) {
        return -EIO;
    }   

    setup.request_type = USB_REQ_TYPE_DIR_IN | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE;
    setup.bRequest = GET_RES;
    setup.wValue = (VOLUME_CONTROL << 8) | ch;
    setup.wIndex = (feature_id << 8) | intf;
    setup.wLength = 2;    

    if(rt_usb_hcd_setup_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_out, &setup, timeout) != 8)
    {
        return RT_ERROR;
    }
    if(rt_usb_hcd_pipe_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_in, g_audio_buf, 2, timeout) != 2)
    {
        return RT_ERROR;
    }  
    if(rt_usb_hcd_pipe_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_out, RT_NULL, 0, timeout) != 0)
    {
        return RT_ERROR;
    }        

    memcpy(res_volume, g_audio_buf, 2);
    return RT_EOK;
}

int usbh_audio_set_volume(struct usbh_audio *audio_class, const char *name, rt_uint16_t volume_hex)
{
    struct urequest setup;
    rt_uint8_t intf = 0xff;
    rt_uint8_t feature_id = 0xff;
    rt_uint8_t ch;
    int timeout = USB_TIMEOUT_BASIC;

    if(audio_class == RT_NULL)
    {
        return -EIO;
    }

    for (size_t i = 0; i < audio_class->module_num; i++) {
        if (strcmp(name, audio_class->module[i].name) == 0) {
            intf = audio_class->ctrl_intf;
            feature_id = audio_class->module[i].feature_unit_id;
            ch = audio_class->module[i].altsetting[AUDIO_SET_INTF_ALTSETTING].channels;
        }
    }

    if (intf == 0xff || feature_id == 0xff) {
        return -EIO;
    }

    setup.request_type = USB_REQ_TYPE_DIR_OUT | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE;
    setup.bRequest = SET_CUR;
    setup.wValue = (VOLUME_CONTROL << 8) | ch;
    setup.wIndex = (feature_id << 8) | intf;
    setup.wLength = 2;

    memcpy(g_audio_buf, &volume_hex, 2);

    if(rt_usb_hcd_setup_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_out, &setup, timeout) != 8)
    {
        return RT_ERROR;
    }
    if(rt_usb_hcd_pipe_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_out, g_audio_buf, 2, timeout) != 2)
    {
        return RT_ERROR;
    }  
    if(rt_usb_hcd_pipe_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_in, RT_NULL, 0, timeout) != 0)
    {
        return RT_ERROR;
    }      

    return RT_EOK;
}

int usbh_audio_set_volume_db(struct usbh_audio *audio_class, const char *name, int volume_db, int min_volume_db, int max_volume_db)
{
    struct urequest setup;
    rt_uint8_t intf = 0xff;
    rt_uint8_t feature_id = 0xff;
    rt_uint16_t volume_hex;
    float volume_min_db,volume_max_db;
    rt_uint8_t ch;
    int timeout = USB_TIMEOUT_BASIC;

    if(audio_class == RT_NULL)
    {
        return -EIO;
    }

    if ((volume_db > 127) || (volume_db < -127)) {
        return -EIO;
    }

    for (size_t i = 0; i < audio_class->module_num; i++) {
        if (strcmp(name, audio_class->module[i].name) == 0) {
            intf = audio_class->ctrl_intf;
            feature_id = audio_class->module[i].feature_unit_id;
            ch = audio_class->module[i].altsetting[AUDIO_SET_INTF_ALTSETTING].channels;
        }
    }

    if (intf == 0xff || feature_id == 0xff) {
        return -EIO;
    }

    setup.request_type = USB_REQ_TYPE_DIR_OUT | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE;
    setup.bRequest = SET_CUR;
    setup.wValue = (VOLUME_CONTROL << 8) | ch;
    setup.wIndex = (feature_id << 8) | intf;
    setup.wLength = 2;

    if (min_volume_db < 0x8000) {
        volume_min_db = (float)min_volume_db / 256;
    } else {
        volume_min_db = (float)(min_volume_db - 0x10000) / 256;
    }

    if (max_volume_db < 0x8000) {
        volume_max_db = (float)max_volume_db / 256;
    } else {
        volume_max_db = (float)(max_volume_db - 0x10000) / 256;
    }

    os_printf("Get ch: [%s] , dB range: %f dB ~ %f dB , set %d dB\r\n", name, volume_min_db, volume_max_db, volume_db);

    if (volume_db > volume_max_db || volume_db < volume_min_db)
    {
        os_printf("set (%d dB) not in range!!!\n",volume_db);
        return RT_ERROR;
    }

    if (volume_db >= 0) {
        volume_hex = volume_db * 256;
        if (volume_hex > max_volume_db) {
            os_printf("%s [ERR]volume_hex(%x) not in range\n",__FUNCTION__,volume_hex);
            return RT_ERROR;
        }
    } else {
        volume_hex = volume_db * 256 + 0x10000;
        if (volume_hex < min_volume_db) {
            os_printf("%s [ERR]volume_hex(%x) not in range\n",__FUNCTION__,volume_hex);
            return RT_ERROR;
        }
    }

    memcpy(g_audio_buf, &volume_hex, 2);

    if(rt_usb_hcd_setup_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_out, &setup, timeout) != 8)
    {
        return RT_ERROR;
    }
    if(rt_usb_hcd_pipe_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_out, g_audio_buf, 2, timeout) != 2)
    {
        return RT_ERROR;
    }  
    if(rt_usb_hcd_pipe_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_in, RT_NULL, 0, timeout) != 0)
    {
        return RT_ERROR;
    }      

    os_printf("%s success, volume_hex:%x\n",__FUNCTION__,volume_hex);    

    return RT_EOK;
}

int usbh_audio_set_mute(struct usbh_audio *audio_class, const char *name, bool mute)
{
    struct urequest setup;
    rt_uint8_t intf = 0xff;
    rt_uint8_t feature_id = 0xff;
	int timeout = USB_TIMEOUT_BASIC;
    rt_uint8_t ch;
    for (size_t i = 0; i < audio_class->module_num; i++) {
        if (strcmp(name, audio_class->module[i].name) == 0) {
            intf = audio_class->ctrl_intf;
            feature_id = audio_class->module[i].feature_unit_id;
            ch = audio_class->module[i].altsetting[AUDIO_SET_INTF_ALTSETTING].channels;
        }
    }

    if (intf == 0xff) {
        return -EIO;
    }

    setup.request_type = USB_REQ_TYPE_DIR_OUT | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE;
    setup.bRequest = SET_CUR;
    setup.wValue = (MUTE_CONTROL << 8) | ch;
    setup.wIndex = (feature_id << 8) | intf;
    setup.wLength = 1;

    memcpy(g_audio_buf, &mute, 1);

    if(rt_usb_hcd_setup_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_out, &setup, timeout) != 8)
    {
        return RT_ERROR;
    }
    if(rt_usb_hcd_pipe_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_out, g_audio_buf, 1, timeout) != 1)
    {
        return RT_ERROR;
    }  
    if(rt_usb_hcd_pipe_xfer(audio_class->device->hcd, audio_class->device->pipe_ep0_in, RT_NULL, 0, timeout) != 0)
    {
        return RT_ERROR;
    }   

    return RT_EOK;
}

void usbh_audio_list_module(struct usbh_audio *audio_class)
{
    uep_desc_t ep_desc = RT_NULL;
    rt_uint8_t mult;
    rt_uint16_t mps;

    os_printf("============= Audio module information ===================\r\n");
    os_printf("bcdADC :%04x\r\n", audio_class->bcdADC);
    os_printf("Num of modules :%u\r\n", audio_class->module_num);
    os_printf("Num of altsettings:%u\r\n", audio_class->num_of_intf_altsettings);

    for (rt_uint8_t i = 0; i < audio_class->module_num; i++) {
        os_printf("  module name :%s\r\n", audio_class->module[i].name);
        os_printf("  module feature unit id :%d\r\n", audio_class->module[i].feature_unit_id);
        if(strcmp("mic", audio_class->module[i].name) == 0) {
            ep_desc = &audio_class->isoin;
        }else if (strcmp("speaker", audio_class->module[i].name) == 0) {
            ep_desc = &audio_class->isoout;
        }
        mult = (ep_desc->wMaxPacketSize & USB_MAXPACKETSIZE_ADDITIONAL_TRANSCATION_MASK) >> USB_MAXPACKETSIZE_ADDITIONAL_TRANSCATION_SHIFT;
        mps = ep_desc->wMaxPacketSize & USB_MAXPACKETSIZE_MASK;
        if (ep_desc->bEndpointAddress & 0x80) {
            audio_class->isoin_mps = mps * (mult + 1);
        } else {
            audio_class->isoout_mps = mps * (mult + 1);
        }

        os_printf("  Ep=%02X Attr=%02x Mps=%d Interval=%02u Mult=%02u\r\n", 
                    ep_desc->bEndpointAddress,  
                    ep_desc->bmAttributes, 
                    mps,
                    ep_desc->bInterval,
                    mult);

        for (rt_uint8_t j = 0; j < audio_class->num_of_intf_altsettings; j++) {
            if (j == 0) {
                os_printf("      Ingore altsetting 0\r\n");
                continue;
            }
            os_printf("      Altsetting %u\r\n", j);
            os_printf("          module channels :%u\r\n", audio_class->module[i].altsetting[j].channels);
            //LOG_D("        module format_type :%u\r\n",audio_class->module[i].altsetting[j].format_type);
            os_printf("          module bitresolution :%u\r\n", audio_class->module[i].altsetting[j].bitresolution);
            os_printf("          module sampfreq num :%u\r\n", audio_class->module[i].altsetting[j].sampfreq_num);

            for (rt_uint8_t k = 0; k < audio_class->module[i].altsetting[j].sampfreq_num; k++) {
                os_printf("              module sampfreq :%d hz\r\n", audio_class->module[i].altsetting[j].sampfreq[k]);
            }
        }
    }

    os_printf("============= Audio module information ===================\r\n");
}

static rt_err_t rt_usbh_class_driver_audio_enable(void *arg)
{
    int ret;
    struct uhintf **p_intf = arg;
    rt_uint8_t intf;
    rt_uint8_t cur_iface = 0xff;
    rt_uint8_t cur_iface_count = 0xff;
    rt_uint8_t cur_alt_setting = 0xff;
    rt_uint8_t input_offset = 0;
    rt_uint8_t output_offset = 0;
    rt_uint8_t feature_unit_offset = 0;
    rt_uint8_t format_offset = 0;
    rt_uint8_t *p = RT_NULL;
    uhcd_t hcd = NULL;
    uep_desc_t ep_desc;

    if (p_intf[0] == NULL) {
        return -EIO;
    }   

    hcd = p_intf[0]->device->hcd;
    intf = p_intf[0]->intf_desc->bInterfaceNumber;

    struct usbh_audio *audio_class = usbh_audio_class_alloc();
    if (audio_class == RT_NULL) {
        LOG_D("Fail to alloc audio_class\r\n");
        return RET_ERR;
    }

    audio_class->device = p_intf[0]->device;
    audio_class->ctrl_intf = intf;
    audio_class->num_of_intf_altsettings = 2;

    p_intf[0]->user_data = audio_class;

    p = (rt_uint8_t *)(p_intf[0]->device->cfg_desc);
    while (p[DESC_bLength]) {
        switch (p[DESC_bDescriptorType]) {
            case USB_DESC_TYPE_IAD:
                cur_iface_count = p[3];
                break;
            case USB_DESC_TYPE_INTERFACE:
                cur_iface = p[INTF_DESC_bInterfaceNumber];
                cur_alt_setting = p[INTF_DESC_bAlternateSetting];
                break;
            case USB_DESC_TYPE_ENDPOINT:
                //os_printf("USB_DESC_TYPE_ENDPOINT\n");
                if(((cur_iface == audio_class->module[0].data_intf) && (audio_class->module[0].data_intf != 0)) || 
                   ((cur_iface == audio_class->module[1].data_intf) && (audio_class->module[1].data_intf != 0)))
                {
                    ep_desc = (struct uendpoint_descriptor *)p;

                    os_printf("Altsetting:%u, Ep=%02x Attr=%02u Mps=%d Interval=%02u Cur_iface:%d\r\n",
                                cur_alt_setting,
                                ep_desc->bEndpointAddress,
                                ep_desc->bmAttributes,
                                ep_desc->wMaxPacketSize,
                                ep_desc->bInterval,
                                cur_iface);

                    if (cur_alt_setting == AUDIO_SET_INTF_ALTSETTING) {
                        if(((ep_desc->bEndpointAddress & USB_DIR_MASK) == USB_DIR_IN ) && 
                            ((ep_desc->bmAttributes & USB_EP_ATTR_TYPE_MASK) == USB_EP_ATTR_ISOC))
                        {   
                            audio_class->isoin = *ep_desc;
                        }

                        if(((ep_desc->bEndpointAddress & USB_DIR_MASK) == USB_DIR_OUT ) && 
                            ((ep_desc->bmAttributes & USB_EP_ATTR_TYPE_MASK) == USB_EP_ATTR_ISOC))
                        {
                            audio_class->isoout = *ep_desc;
                        }
                    }

                }
                break;
            case UDESC_CS_INTERFACE:
                if (cur_iface == audio_class->ctrl_intf) {
                    switch (p[DESC_bDescriptorSubType]) {
                        case UDESCSUB_AC_HEADER: {
                            struct usb_audio_control_descriptor *desc = (struct usb_audio_control_descriptor *)p;
                            audio_class->bcdADC = desc->bcdADC;
                            audio_class->bInCollection = desc->bInCollection;
                        } break;
                        case UDESCSUB_AC_INPUT: {
                            struct usb_audio_input_terminal *desc = (struct usb_audio_input_terminal *)p;

                            audio_class->module[input_offset].input_terminal_id = desc->bTerminalId;
                            audio_class->module[input_offset].input_terminal_type = desc->wTerminalType;
                            audio_class->module[input_offset].input_channel_config = desc->wChannelConfig;

                            if (desc->wTerminalType == UAT_STREAM) {
                                audio_class->module[input_offset].terminal_link_id = desc->bTerminalId;
                            }
                            if (desc->wTerminalType == UATI_MICROPHONE) {
                                audio_class->module[input_offset].name = "mic";
                            }
                            input_offset++;
                        } break;
                            break;
                        case UDESCSUB_AC_OUTPUT: {
                            struct usb_audio_output_terminal *desc = (struct usb_audio_output_terminal *)p;
                            audio_class->module[output_offset].output_terminal_id = desc->bTerminalId;
                            audio_class->module[output_offset].output_terminal_type = desc->wTerminalType;
                            if (desc->wTerminalType == UAT_STREAM) {
                                audio_class->module[output_offset].terminal_link_id = desc->bTerminalId;
                            }
                            if (desc->wTerminalType == UATO_SPEAKER) {
                                audio_class->module[output_offset].name = "speaker";
                            }
                            output_offset++;
                        } break;
                        case UDESCSUB_AC_FEATURE: {
                            struct usb_audio_feature_unit *desc = (struct usb_audio_feature_unit *)p;
                            audio_class->module[feature_unit_offset].feature_unit_id = desc->bUnitId;
                            audio_class->module[feature_unit_offset].feature_unit_controlsize = desc->bControlSize;

                            for (rt_uint8_t j = 0; j < desc->bControlSize; j++) {
                                audio_class->module[feature_unit_offset].feature_unit_controls[j] = p[6 + j];
                            }
                            feature_unit_offset++;
                        } break;
                        case UDESCSUB_AC_PROCESSING:

                            break;
                        default:
                            break;
                    }
                } else if ((cur_iface < (audio_class->ctrl_intf + cur_iface_count)) && (cur_iface > audio_class->ctrl_intf)) {
                    switch (p[DESC_bDescriptorSubType]) {
                        case AS_GENERAL:

                            break;
                        case FORMAT_TYPE: {
                            struct usb_audio_streaming_type1_descriptor *desc = (struct usb_audio_streaming_type1_descriptor *)p;

                            audio_class->module[format_offset].data_intf = cur_iface;
                            audio_class->module[format_offset].altsetting[cur_alt_setting].channels = desc->bNrChannels;
                            audio_class->module[format_offset].altsetting[cur_alt_setting].format_type = desc->bFormatType;
                            audio_class->module[format_offset].altsetting[cur_alt_setting].bitresolution = desc->bBitResolution;
                            audio_class->module[format_offset].altsetting[cur_alt_setting].sampfreq_num = desc->bSamFreqType;

                            for (rt_uint8_t j = 0; j < desc->bSamFreqType; j++) {
                                audio_class->module[format_offset].altsetting[cur_alt_setting].sampfreq[j] = (rt_uint32_t)(p[10 + j * 3] << 16) |
                                                                                                             (rt_uint32_t)(p[9 + j * 3] << 8) |
                                                                                                             (rt_uint32_t)(p[8 + j * 3] << 0);
                            }
                            if (cur_alt_setting == (audio_class->num_of_intf_altsettings - 1)) {
                                format_offset++;
                            }
                        } break;
                        default:
                            break;
                    }
                }
                break;
            default:
                break;
        }
        /* skip to next descriptor */
        p += p[DESC_bLength];
    }

    if ((input_offset != output_offset) && (input_offset != feature_unit_offset) && (input_offset != format_offset)) {
        return -EIO;
    }

    audio_class->module_num = input_offset;

    usbh_audio_list_module(audio_class);

    audio_class->rx_buff = (rt_uint8_t *)rt_malloc(AUDIO_RX_PACKET_SIZE);
    if(audio_class->rx_buff == RT_NULL) {
        os_printf("malloc rx_buff fail\n");
        return RT_ENOMEM;
    }


    for (size_t i = 0; i < audio_class->module_num; i++) {
        ret = usbh_audio_close(audio_class, audio_class->module[i].name);
        if (ret < 0) {
            os_printf("Fail to close audio module :%s\r\n", audio_class->module[i].name);
            return ret;
        }
    }

    snprintf(audio_class->devname, CONFIG_USBHOST_DEV_NAMELEN, DEV_FORMAT, audio_class->minor);
    os_printf("Register Audio Class:%s\r\n", audio_class->devname);

    usbh_audio_run(audio_class);
    return 0;
}

static rt_err_t rt_usbh_class_driver_audio_disable(void *arg)
{
    int ret = RT_EOK;
    struct uhintf *p_intf = arg;

    if (p_intf == NULL) {
        return -EIO;
    }  

    struct usbh_audio *audio_class = (struct usbh_audio *)p_intf->user_data;

    if (audio_class) {
        if (audio_class->devname[0] != '\0') {
            os_printf("Unregister Audio Class:%s\r\n", audio_class->devname);
            usbh_audio_stop(audio_class);
        }

        if(audio_class->rx_buff) {
            rt_free(audio_class->rx_buff);
        }

        usbh_audio_class_free(audio_class);
    }

    return ret;
}

/* ----------------------------- USB 2.0 / USB 1.1 UAC IRQ ----------------------------- */
static void _usbh_audio_spk_tx(void * p_dev, uint8_t ep, uint8_t *data, uint32_t len)
{
    struct usb_device_dma_cfg dma_cfg;

    if (p_dev == RT_NULL || data == RT_NULL || len == 0) {
        return;
    }

    dma_cfg.ep_index = ep;
    dma_cfg.ep_dir   = USB_CORE_HOST_EP_TX;
    dma_cfg.dma_addr = (rt_uint32_t *)data;
    dma_cfg.dma_len  = len;

    usb_device_ioctl((struct usb_device *)p_dev, USB_HOST_KICK_EP_DMA, (rt_uint32_t)&dma_cfg, (rt_uint32_t)RT_NULL);
}


bool rtt_uac_data_deal(void *p_dev, rt_uint8_t ep)
{
    rt_int32_t rx_len = 0;
    usb_device_ioctl((struct usb_device *)p_dev, USB_HOST_GET_RX_DMA_LEN, ep, (rt_uint32_t)&rx_len);

    if((rx_len == 0) || (rx_len != AUDIO_RX_PACKET_SIZE)){
        return 1;
    }
	
	if(rx_len < 0){
        os_printf("The length of the configured dma needs to be increased\n");
        return 1;
    }
	
    usbmic_packet_len = rx_len;
    // os_printf("rx_len:%d\n", rx_len);
    return 0;    
}

void rtt_usbh_audio_irq(void * dev, rt_uint32_t irq, rt_uint8_t ep)
{
    rt_uint32_t ret;
    struct usb_device *p_dev = (struct usb_device *)dev;
    struct usb_device_dma_cfg dma_cfg;

    if (!p_dev)
        return;    

    switch (irq)
    {
    case USB_EP_RX_IRQ:
    if ((ep == (&g_audio_class[0])->pipe_in->pipe_index) && ((&g_audio_class[0])->pipe_in != RT_NULL)) {
        #if AUDIO_MIC_EN
        usb_dma_uac_irq_times++;
        if(uac_open && uac_run) {
            ret = rtt_uac_data_deal(p_dev, ep);
            if(ret == 0){
                ret = usbmic_deal((struct usb_device *)p_dev, (uint8_t *)(&g_audio_class[0])->rx_buff);
            }

            dma_cfg.ep_index = ep;
            dma_cfg.ep_dir   = USB_CORE_HOST_EP_RX;
            dma_cfg.dma_addr = (rt_uint32_t *)g_audio_class[0].rx_buff;
            dma_cfg.dma_len = AUDIO_RX_PACKET_SIZE;
            usb_device_ioctl((struct usb_device *)p_dev, USB_HOST_KICK_EP_DMA, (rt_uint32_t)&dma_cfg, (rt_uint32_t)RT_NULL);
        }
        #endif
    }
    break;

    case USB_EP_TX_IRQ:
    if ((ep == (&g_audio_class[0])->pipe_out->pipe_index) && ((&g_audio_class[0])->pipe_out != RT_NULL)) {
        #if AUDIO_SPK_EN
        usb_dma_uac_irq_times++;
        if(uac_open && uac_run)
		{
            usbspk_tx((struct usb_device *)p_dev, ep, AUDIO_TX_PACKET_SIZE, _usbh_audio_spk_tx);
        }
        #endif        
    }
    break;

    default:
        break;
    }
}

rt_uint32_t rtt_usbh_audio_dev_pipe_mange(rt_uint8_t dev_num, const char *name, rt_uint8_t alloc_or_free)
{
    uep_desc_t ep_desc = RT_NULL;
    upipe_t pipe;
    struct usbh_audio *audio_class = RT_NULL;

    if(dev_num >= CONFIG_USBHOST_MAX_AUDIO_CLASS)
    {
        os_printf("Invalid dev_num %d!!!!!!!!\n",dev_num);
        return -RT_ERROR;
    }

    audio_class = &g_audio_class[dev_num];

    if (strcmp(name, "mic") == 0) {
        ep_desc = &audio_class->isoin;
    } else if (strcmp(name, "speaker") == 0) {
        ep_desc = &audio_class->isoout;
    }

    os_printf("Audio Class dev%d: Altsetting:%u, Ep=%02x Attr=%02u Mps=%d Interval=%02u\r\n",
                dev_num,
                AUDIO_SET_INTF_ALTSETTING,
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
        pipe = rt_usb_instance_find_pipe(audio_class->device, ep_desc->bEndpointAddress);

        if(pipe != RT_NULL) {
            os_printf("g_video_class[%d] pipe has been alloced!!!\n",dev_num);
            return -RT_ERROR;
        }

        if (rt_usb_hcd_alloc_pipe(audio_class->device->hcd, &pipe, audio_class->device, ep_desc) != RT_EOK) {
            rt_kprintf("alloc iso pipe failed\n");
            return -RT_ERROR;
        }
        os_printf("alloc iso pipe ok %x\n",pipe);

        rt_usb_instance_add_pipe(audio_class->device, pipe);

        if ((ep_desc->bEndpointAddress & USB_DIR_MASK) == USB_DIR_IN)
        {
            audio_class->pipe_in = pipe;
        }
        else
        {
            audio_class->pipe_out = pipe;
        }

        struct usb_device *p_dev = NULL;
        switch(g_audio_class[0].device->hcd->devid)
        {
            case HG_USB11_HOST_CONTROLLER_DEVID:
            {
                os_printf("HG_USB11_HOST_CONTROLLER_DEVID\n");
                p_dev = (struct usb_device *)dev_get(HG_USB11HOST_DEVID);
                if (p_dev) {
                    os_printf("kick usb11\n");
                };
                /* USB Full Speed : 1 frame = 1 ms */
                if ((ep_desc->bEndpointAddress & USB_DIR_MASK) == USB_DIR_IN) 
                {
                    usb_device_ioctl(p_dev, USB_HOST_SET_RX_INTERVAL, pipe->pipe_index, 1);
                } 
                else
                {
                    usb_device_ioctl(p_dev, USB_HOST_SET_TX_INTERVAL, pipe->pipe_index, 1);
                } 
            }
                break;
            
            case HG_USB_HOST_CONTROLLER_DEVID:
            {
                os_printf("HG_USB_HOST_CONTROLLER_DEVID\n");
                p_dev = (struct usb_device *)dev_get(HG_USBDEV_DEVID);
                if (p_dev) {
                    os_printf("kick usb20\n");
                };
                if (usb_device_ioctl(p_dev, USB_HOST_GET_BUS_SPEED, 0, 0)) {
                    /* USB High Speed : 8 microframe = 1 ms */
                    if ((ep_desc->bEndpointAddress & USB_DIR_MASK) == USB_DIR_IN) 
                    {
                        usb_device_ioctl(p_dev, USB_HOST_SET_RX_INTERVAL, pipe->pipe_index, 4);
                    } 
                    else
                    {
                        usb_device_ioctl(p_dev, USB_HOST_SET_TX_INTERVAL, pipe->pipe_index, 4);
                    }
                } else {
                    /* USB Full Speed : 1 frame = 1 ms */
                    if ((ep_desc->bEndpointAddress & USB_DIR_MASK) == USB_DIR_IN) 
                    {
                        usb_device_ioctl(p_dev, USB_HOST_SET_RX_INTERVAL, pipe->pipe_index, 1);
                    } 
                    else
                    {
                        usb_device_ioctl(p_dev, USB_HOST_SET_TX_INTERVAL, pipe->pipe_index, 1);
                    }            
                }
            }
                break;
            default:
                break;
        }
    }
    else
    {
        if ((ep_desc->bEndpointAddress & USB_DIR_MASK) == USB_DIR_IN)
        {
            if(audio_class->pipe_in != RT_NULL) {
                rt_list_remove(&audio_class->pipe_in->list);
                rt_usb_hcd_free_pipe(audio_class->device->hcd, audio_class->pipe_in);
                audio_class->pipe_in = RT_NULL;
            } else {
                os_printf("g_audio_class[%d] pipe_in is NULL, not free\n",dev_num);
                return -RT_ERROR;
            }
        }
        else
        {
            if(audio_class->pipe_out != RT_NULL) {
                rt_list_remove(&audio_class->pipe_out->list);
                rt_usb_hcd_free_pipe(audio_class->device->hcd, audio_class->pipe_out);
                audio_class->pipe_out = RT_NULL;
            } else {
                os_printf("g_audio_class[%d] pipe_out is NULL, not free\n",dev_num);
                return -RT_ERROR;
            }
        }
    }

    return RT_EOK;
}


/* 用户调用接口 */
rt_uint32_t rtt_usbh_audio_user_open()
{
	rt_uint32_t ret;
    struct usbh_audio *audio_class = &g_audio_class[0];
    struct usb_device *p_dev = NULL;

    switch(g_audio_class[0].device->hcd->devid)
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

#if AUDIO_VOLUME_CONTROL_EN
	rt_uint16_t min_volume,max_volume,res_volume,cur_volume;
#if AUDIO_MIC_EN
    //设置设备非静音状态和音量大小
    usbh_audio_set_mute(audio_class, "mic", 0);

    //获取设备最大最小音量，设置音量大小
    usbh_audio_get_cur_volume(audio_class, "mic", &cur_volume);
    usbh_audio_get_min_volume(audio_class, "mic", &min_volume);
    usbh_audio_get_max_volume(audio_class, "mic", &max_volume);
    usbh_audio_get_res_volume(audio_class, "mic", &res_volume);
    os_printf("[mic] min volume:%x,max volume:%x cur_volume:%x res_volume:%x\n",min_volume,max_volume,cur_volume,res_volume);
    usbh_audio_set_volume(audio_class, "mic", max_volume);  //默认设置最大音量
    //usbh_audio_set_volume_db(audio_class, "mic", -127, min_volume, max_volume);     //根据打印的可调dB范围设置
#endif

#if AUDIO_SPK_EN
    //设置设备非静音状态
    usbh_audio_set_mute(audio_class, "speaker", 0);

    //获取设备最大最小音量，设置音量大小
    usbh_audio_get_cur_volume(audio_class, "speaker", &cur_volume);
    usbh_audio_get_min_volume(audio_class, "speaker", &min_volume);
    usbh_audio_get_max_volume(audio_class, "speaker", &max_volume);
    usbh_audio_get_res_volume(audio_class, "spekaer", &res_volume);
    os_printf("[speaker] min volume:%x,max volume:%x cur_volume:%x res_volume:%x\n",min_volume,max_volume,cur_volume,res_volume);
    usbh_audio_set_volume(audio_class, "speaker", max_volume);  //默认设置最大音量
    //usbh_audio_set_volume_db(audio_class, "speaker", -37, min_volume, max_volume);  //根据打印的可调dB范围设置
#endif
#endif

    //给设备分配pipe
#if AUDIO_MIC_EN
    rtt_usbh_audio_dev_pipe_mange(0, "mic", AUDIO_ALLOC_PIPE);
#endif
#if AUDIO_SPK_EN
    rtt_usbh_audio_dev_pipe_mange(0, "speaker", AUDIO_ALLOC_PIPE);
#endif

    //USB发送打开UAC数据流命令
#if AUDIO_MIC_EN    
    ret = usbh_audio_open(audio_class, "mic", AUDIO_MIC_SAMPLING_FREQ);
    if(ret != 0){
        printf("mic open fail\n");
        return -RT_ERROR;
    }
#endif
#if AUDIO_SPK_EN
    ret = usbh_audio_open(audio_class, "speaker", AUDIO_SPK_SAMPLING_FREQ);
    if(ret != 0){
        printf("speaker open fail\n");
        return -RT_ERROR;
    }
#endif

    //应用层数据流初始化
#if AUDIO_MIC_EN
    usbmic_room_init();
    usbmic_enum_finish(&usb_dma_uac_irq_times);
#endif
#if AUDIO_SPK_EN
    usbspk_room_init(AUDIO_TX_PACKET_SIZE);
    usbspk_enum_finish(&usb_dma_uac_irq_times);
#endif

    //使能端点收发，触发中断循环
#if AUDIO_MIC_EN    
    struct usb_device_dma_cfg dma_cfg;
    dma_cfg.ep_index = (&g_audio_class[0])->pipe_in->pipe_index;
    dma_cfg.ep_dir   = USB_CORE_HOST_EP_RX;
    dma_cfg.dma_addr = (rt_uint32_t *)g_audio_class[0].rx_buff;
    dma_cfg.dma_len = AUDIO_RX_PACKET_SIZE;
    usb_device_ioctl((struct usb_device *)p_dev, USB_HOST_KICK_EP_DMA, (rt_uint32_t)&dma_cfg, (rt_uint32_t)RT_NULL);
#endif
#if AUDIO_SPK_EN
    usbspk_tx(p_dev, (&g_audio_class[0])->pipe_out->pipe_index, AUDIO_TX_PACKET_SIZE, _usbh_audio_spk_tx);
#endif

    return RET_OK;
}

rt_uint32_t rtt_usbh_audio_user_close()
{
    struct usbh_audio *audio_class = &g_audio_class[0];
    rtt_usbh_audio_user_stop();
    #if AUDIO_MIC_EN 
    rtt_usbh_audio_dev_pipe_mange(0, "mic", AUDIO_FREE_PIPE);
    #endif
    #if AUDIO_SPK_EN
    rtt_usbh_audio_dev_pipe_mange(0, "speaker", AUDIO_FREE_PIPE);
    #endif

    #if AUDIO_MIC_EN 
    usbh_audio_close(audio_class, "mic");
    #endif
    #if AUDIO_SPK_EN
    usbh_audio_close(audio_class, "speaker");
    #endif

    return RT_EOK;
}

rt_uint32_t rtt_usbh_audio_user_stop()
{
    struct usb_device *p_dev = NULL;

    uac_stop();
    switch(g_audio_class[0].device->hcd->devid)
    {
        case HG_USB11_HOST_CONTROLLER_DEVID:
        {
            os_printf("HG_USB11_HOST_CONTROLLER_DEVID\n");
			p_dev = (struct usb_device *)dev_get(HG_USB11HOST_DEVID);
            if (p_dev) {
                #if AUDIO_MIC_EN
                hgusb11_v0_host_ep_abort(p_dev, USB_CORE_HOST_EP_RX, (&g_audio_class[0])->pipe_in->pipe_index);
                #endif
                #if AUDIO_SPK_EN
                hgusb11_v0_host_ep_abort(p_dev, USB_CORE_HOST_EP_TX, (&g_audio_class[0])->pipe_out->pipe_index);
                #endif
            };
        }
			break;
        
        case HG_USB_HOST_CONTROLLER_DEVID:
		{
            os_printf("HG_USB_HOST_CONTROLLER_DEVID\n");
            p_dev = (struct usb_device *)dev_get(HG_USBDEV_DEVID);
            if (p_dev) {
                #if AUDIO_MIC_EN
                hgusb20_ep_rx_abort((struct hgusb20_dev *)p_dev, (&g_audio_class[0])->pipe_in->pipe_index);
                #endif
                #if AUDIO_SPK_EN
                hgusb20_ep_tx_abort((struct hgusb20_dev *)p_dev, (&g_audio_class[0])->pipe_out->pipe_index);
                #endif
            };
		}
            break;
        default:
            break;
    }

    return RT_EOK;
}

rt_uint32_t rtt_usbh_audio_user_start()
{
    struct usb_device *p_dev = NULL;

    switch(g_audio_class[0].device->hcd->devid)
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

    if(uac_open) {
        uac_run = 1;
        #if AUDIO_MIC_EN
        struct usb_device_dma_cfg dma_cfg;
        dma_cfg.ep_index = (&g_audio_class[0])->pipe_in->pipe_index;
        dma_cfg.ep_dir   = USB_CORE_HOST_EP_RX;
        dma_cfg.dma_addr = (rt_uint32_t *)g_audio_class[0].rx_buff;
        dma_cfg.dma_len = AUDIO_RX_PACKET_SIZE;
        usb_device_ioctl((struct usb_device *)p_dev, USB_HOST_KICK_EP_DMA, (rt_uint32_t)&dma_cfg, (rt_uint32_t)RT_NULL);
        #endif
        #if AUDIO_SPK_EN
        usbspk_tx((struct usb_device *)p_dev, (&g_audio_class[0])->pipe_out->pipe_index, AUDIO_TX_PACKET_SIZE, _usbh_audio_spk_tx);
        #endif
    }

    return RT_EOK;
}


int32 demo_atcmd_usbh_audio(const char *cmd, char *argv[], uint32 argc)
{
    if (*argv[0] == '1') {
        if (*argv[1] == '1') {
            rtt_usbh_audio_user_open();
        }

        if (*argv[1] == '0') {
            rtt_usbh_audio_user_close();
        }
    } else if (*argv[0] == '2') {
        if (*argv[1] == '1') {

        }

        if (*argv[1] == '0') {

        }
    }

	printf("OK/n");
    return 0;
}

void usbh_audio_thread_entry(void *arg)
{
    //struct usbh_audio *audio_class = arg;
}


__attribute__((weak)) void usbh_audio_run(struct usbh_audio *audio_class)
{
    rtt_usbh_audio_user_open();

#if 0
    rt_thread_t usbh_audio_thread;
    usbh_audio_thread = rt_thread_create("usbh_video_thread", 
                                          usbh_audio_thread_entry, 
                                          audio_class, 
                                          1024, 
                                          OS_TASK_PRIORITY_NORMAL, 
                                          20);

    if(usbh_audio_thread != RT_NULL)
        rt_thread_startup(usbh_audio_thread);
#endif

}

__attribute__((weak)) void usbh_audio_stop(struct usbh_audio *audio_class)
{
}



ucd_t rt_usbh_class_driver_audio(void)
{
    usbh_audio_class.class_code = USB_CLASS_AUDIO;

    usbh_audio_class.enable = rt_usbh_class_driver_audio_enable;
    usbh_audio_class.disable = rt_usbh_class_driver_audio_disable;

    return &usbh_audio_class;
}

#endif