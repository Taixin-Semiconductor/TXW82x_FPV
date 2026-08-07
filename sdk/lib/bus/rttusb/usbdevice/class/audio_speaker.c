/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-09-19     flybreak     the first version
 */

//#include <rthw.h>
#include <rtthread.h>
#include "include/rttusb_device.h"

#ifdef RT_USB_DEVICE_AUDIO_SPEAKER
#include "stream_frame.h"
#include "uaudioreg.h"
#include "lib/multimedia/msi.h"

#define AUDIO_SAMPLERATE    8000
#define AUDIO_CHANNEL       1
#define RESOLUTION_BITS     16
#define AUDIO_INTERVAL_TIME 64  //ms

#define RESOLUTION_BYTE     (RESOLUTION_BITS / 8)
#define AUDIO_PER_MS_SZ    ((AUDIO_SAMPLERATE * AUDIO_CHANNEL * RESOLUTION_BYTE) / 1000)
#define AUDIO_BUFFER_SZ    (AUDIO_PER_MS_SZ * AUDIO_INTERVAL_TIME)  

#if defined(RT_USBD_SPEAKER_DEVICE_NAME)
    #define SPEAKER_DEVICE_NAME    RT_USBD_SPEAKER_DEVICE_NAME
#else
    #define SPEAKER_DEVICE_NAME    "sound0"
#endif

#define EVENT_AUDIO_START   (1 << 0)
#define EVENT_AUDIO_STOP    (1 << 1)

#define SPK_INTF_STR_INDEX 9
/*
 * uac speaker descriptor define
 */

#define UAC_CS_INTERFACE            0x24
#define UAC_CS_ENDPOINT             0x25

#define UAC_MAX_PACKET_SIZE         AUDIO_BUFFER_SZ
#define UAC_EP_MAX_PACKET_SIZE      AUDIO_PER_MS_SZ
#define UAC_CHANNEL_NUM             AUDIO_CHANNEL


struct uac_ac_descriptor
{
#ifdef RT_USB_DEVICE_COMPOSITE
    struct uiad_descriptor iad_desc;
#endif
    struct uinterface_descriptor intf_desc;
    struct usb_audio_control_descriptor hdr_desc;
    struct usb_audio_input_terminal it_desc;
    struct usb_audio_output_terminal ot_desc;
#if UAC_USE_FEATURE_UNIT
    struct usb_audio_feature_unit feature_unit_desc;
#endif
};

struct uac_as_descriptor
{
    struct uinterface_descriptor intf_desc;
    struct usb_audio_streaming_interface_descriptor hdr_desc;
    struct usb_audio_streaming_type1_descriptor format_type_desc;
    struct uendpoint_descriptor ep_desc;
    struct usb_audio_streaming_endpoint_descriptor as_ep_desc;
};

/*
 * uac speaker device type
 */

struct uac_audio_speaker
{
    rt_device_t  dev;
    rt_event_t   event;
    rt_uint8_t   open_count;

    rt_uint8_t  *buffer;
    rt_uint32_t  buffer_index;
    uep_t        ep;

    struct msi *msi;
    struct fbpool tx_pool;
};
static struct uac_audio_speaker speaker;

rt_align(4)
static struct udevice_descriptor dev_desc =
{
    USB_DESC_LENGTH_DEVICE,     //bLength;
    USB_DESC_TYPE_DEVICE,       //type;
    USB_BCD_VERSION,            //bcdUSB;
    USB_CLASS_DEVICE,           //bDeviceClass;
    0x00,                       //bDeviceSubClass;
    0x00,                       //bDeviceProtocol;
    UAC_EP_MAX_PACKET_SIZE,     //bMaxPacketSize0;
    _VENDOR_ID,                 //idVendor;
    _PRODUCT_ID,                //idProduct;
    USB_BCD_DEVICE,             //bcdDevice;
    USB_STRING_MANU_INDEX,      //iManufacturer;
    USB_STRING_PRODUCT_INDEX,   //iProduct;
    USB_STRING_SERIAL_INDEX,    //iSerialNumber;Unused.
    USB_DYNAMIC,                //bNumConfigurations;
};

//FS and HS needed
rt_align(4)
static struct usb_qualifier_descriptor dev_qualifier =
{
    sizeof(dev_qualifier),          //bLength
    USB_DESC_TYPE_DEVICEQUALIFIER,  //bDescriptorType
    0x0200,                         //bcdUSB
    USB_CLASS_AUDIO,                //bDeviceClass
    0x00,                           //bDeviceSubClass
    0x00,                           //bDeviceProtocol
    64,                             //bMaxPacketSize0
    0x01,                           //bNumConfigurations
    0,
};

rt_align(4)
const static char *_ustring[] =
{
    "Language",
    "RT-Thread Team.",
    "RT-Thread Audio Speaker",
    "32021919830108",
    "Configuration",
    "Interface",
};

rt_align(4)
static struct uac_ac_descriptor ac_desc =
{
#ifdef RT_USB_DEVICE_COMPOSITE
    /* Interface Association Descriptor */
    {
        USB_DESC_LENGTH_IAD,
        USB_DESC_TYPE_IAD,
        USB_DYNAMIC,
        0x02,
        USB_CLASS_AUDIO,
        USB_SUBCLASS_AUDIOSTREAMING,
        0x00,
        0x00,
    },
#endif
    /* Interface Descriptor */
    {
        USB_DESC_LENGTH_INTERFACE,
        USB_DESC_TYPE_INTERFACE,
        USB_DYNAMIC,
        0x00,
        0x00,
        USB_CLASS_AUDIO,
        USB_SUBCLASS_AUDIOCONTROL,
        0x00,
#ifdef RT_USB_DEVICE_COMPOSITE
        SPK_INTF_STR_INDEX,
#else
        0x00,
#endif
    },
    /* Header Descriptor */
    {
        sizeof(struct usb_audio_control_descriptor),
        UAC_CS_INTERFACE,
        UDESCSUB_AC_HEADER,
        0x0100,    /* Version: 1.00 */
        0x0027,    /* Total length: 39 */
        0x01,      /* Total number of interfaces: 1 */
        {0x01},    /* Interface number: 1 */
    },
    /*  Input Terminal Descriptor */
    {
        sizeof(struct usb_audio_input_terminal),
        UAC_CS_INTERFACE,
        UDESCSUB_AC_INPUT,
        0x01,      /* Terminal ID: 1 */
        0x0101,    /* Terminal Type: USB Streaming (0x0101) */
        0x00,      /* Assoc Terminal: 0 */
        0x01,      /* Number Channels: 1 */
        0x0000,    /* Channel Config: 0x0000 */
        0x00,      /* Channel Names: 0 */
        0x00,      /* Terminal: 0 */
    },
    /*  Output Terminal Descriptor */
    {
        sizeof(struct usb_audio_output_terminal),
        UAC_CS_INTERFACE,
        UDESCSUB_AC_OUTPUT,
        0x02,      /* Terminal ID: 2 */
        0x0301,    /* Terminal Type */
        0x00,      /* Assoc Terminal: 0 */
        0x01,      /* Source ID: 1 */
        0x00,      /* Terminal: 0 */
    },
#if UAC_USE_FEATURE_UNIT
    /*  Feature unit Descriptor */
    {
        sizeof(struct usb_audio_feature_unit),
        UAC_CS_INTERFACE,
        UDESCSUB_AC_FEATURE,
        0x02,
        0x01，
        0x01,
        0x00,
        0x01,
    },
#endif
};

rt_align(4)
static struct uinterface_descriptor as_desc0 =
{
    USB_DESC_LENGTH_INTERFACE,
    USB_DESC_TYPE_INTERFACE,
    USB_DYNAMIC,
    0x00,
    0x00,
    USB_CLASS_AUDIO,
    USB_SUBCLASS_AUDIOSTREAMING,
    0x00,
    0x00,
};

rt_align(4)
static struct uac_as_descriptor as_desc =
{
    /* Interface Descriptor */
    {
        USB_DESC_LENGTH_INTERFACE,
        USB_DESC_TYPE_INTERFACE,
        USB_DYNAMIC,
        0x01,
        0x01,
        USB_CLASS_AUDIO,
        USB_SUBCLASS_AUDIOSTREAMING,
        0x00,
        0x00,
    },
    /* General AS Descriptor */
    {
        sizeof(struct usb_audio_streaming_interface_descriptor),
        UAC_CS_INTERFACE,
        AS_GENERAL,
        0x01,      /* Terminal ID: 1 */
        0x01,      /* Interface delay in frames: 1 */
        UA_FMT_PCM,
    },
    /* Format type i Descriptor */
    {
        sizeof(struct usb_audio_streaming_type1_descriptor),
        UAC_CS_INTERFACE,
        FORMAT_TYPE,
        FORMAT_TYPE_I,
        UAC_CHANNEL_NUM,
        2,         /* Subframe Size: 2 */
        RESOLUTION_BITS,
        0x01,      /* Samples Frequence Type: 1 */
        {0},       /* Samples Frequence */
    },
    /* Endpoint Descriptor */
    {
        USB_DESC_LENGTH_ENDPOINT,
        USB_DESC_TYPE_ENDPOINT,
        USB_DYNAMIC | USB_DIR_OUT,
        USB_EP_ATTR_ISOC,
        UAC_EP_MAX_PACKET_SIZE,
        0x04    ,           //HS:0x04 FS:0x01
    },
    /* AS Endpoint Descriptor */
    {
        sizeof(struct usb_audio_streaming_endpoint_descriptor),
        UAC_CS_ENDPOINT,
        AS_GENERAL,
    },
};

static int32_t uac_speaker_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    struct uac_audio_speaker *speaker = (struct uac_msi_s *)msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            fbpool_destroy(&speaker->tx_pool);
            if(speaker) {
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
                os_free(fb->data);
                fb->data = NULL;
            }
            fbpool_put(&speaker->tx_pool, fb);
            // 不需要内核去释放fb
            ret = RET_OK + 1;
        }
        break;
    }
    return ret;
}

static int usbd_audio_spk_msi_init(struct uac_audio_speaker *speaker, rt_uint8_t fb_tx_num)
{
    speaker->msi = msi_new(S_USB_MIC, 0, NULL);
    if (!speaker->msi)
    {
        os_printf("creat speaker->msi stream fail\n");
        return -RT_ERROR;
    }
    fbpool_init(&speaker->tx_pool, fb_tx_num);
    speaker->msi->priv = speaker;
    speaker->msi->action = uac_speaker_action;
    speaker->msi->enable = 1;

    /* MSI add output */
    /* ... */

    return RT_EOK;
}

static void usbd_audio_spk_msi_deinit(struct uac_audio_speaker *speaker)
{
    if (speaker->msi)
    {
        msi_destroy(speaker->msi);
        speaker->msi = NULL;
    }
}

static int usbd_audio_spk_msi_output_fb(struct uac_audio_speaker *speaker, rt_uint8_t *buffer, rt_uint32_t size)
{
    struct framebuff *fb = NULL;
    fb = fbpool_get(&speaker->tx_pool, 0, speaker->msi);
    if (fb)
    {
        fb->data = os_zalloc(size);
        if (!fb->data)
        {
            os_printf("alloc fb data failed\n");
            msi_delete_fb(NULL, fb);
            fb = NULL;
            return -RT_ERROR;
        }

        rt_memcpy(fb->data, buffer, size);
        fb->mtype = SOUND;
        fb->stype = SOUND_USB_SPK;
        fb->len = size;
        msi_output_fb(speaker->msi, fb);
    }
    else
    {
        os_printf("get src data_f failed\n");
        return -RT_ERROR;
    }
    return RT_EOK;
}


static rt_err_t _audio_start(ufunction_t func)
{
    speaker.ep->request.buffer = speaker.buffer + speaker.buffer_index;
    speaker.ep->request.size = UAC_MAX_PACKET_SIZE / 2;
    speaker.ep->request.req_type = UIO_REQUEST_READ_FULL;
    rt_usbd_io_request(func->device, speaker.ep, &speaker.ep->request);
    speaker.buffer_index += UAC_MAX_PACKET_SIZE / 2;
    if (speaker.buffer_index >= UAC_MAX_PACKET_SIZE)
    {
        speaker.buffer_index = 0;
    }
    speaker.open_count ++;
    rt_event_send(speaker.event, EVENT_AUDIO_START);

    return 0;
}

static rt_err_t _audio_stop(ufunction_t func)
{
    speaker.open_count --;
    rt_event_send(speaker.event, EVENT_AUDIO_STOP);
    return 0;
}

static rt_err_t _ep_data_handler(ufunction_t func, rt_size_t size)
{
    RT_ASSERT(func != RT_NULL);

    speaker.ep->request.buffer = speaker.buffer + speaker.buffer_index;
    speaker.ep->request.size = UAC_MAX_PACKET_SIZE / 2;
    speaker.ep->request.req_type = UIO_REQUEST_READ_FULL;
    rt_usbd_io_request(func->device, speaker.ep, &speaker.ep->request);

    speaker.buffer_index += UAC_MAX_PACKET_SIZE / 2;

    if (speaker.buffer_index >= UAC_MAX_PACKET_SIZE)
    {
        speaker.buffer_index = 0;
    }
    
    usbd_audio_spk_msi_output_fb(&speaker, speaker.buffer + speaker.buffer_index, UAC_MAX_PACKET_SIZE / 2);
    
    return RT_EOK;
}

static rt_err_t _interface_as_handler(ufunction_t func, ureq_t setup)
{
    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    LOG_D("_interface_as_handler");

    if ((setup->request_type & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_STANDARD)
    {
        switch (setup->bRequest)
        {
        case USB_REQ_GET_INTERFACE:
            break;
        case USB_REQ_SET_INTERFACE:
            LOG_D("set interface handler");
            if (setup->wValue == 1)
            {
                _audio_start(func);
            }
            else if (setup->wValue == 0)
            {
                _audio_stop(func);
            }
            break;
        default:
            LOG_D("unknown uac request 0x%x", setup->bRequest);
            return -RT_ERROR;
        }
    }

    return RT_EOK;
}

static rt_err_t _function_enable(ufunction_t func)
{
    RT_ASSERT(func != RT_NULL);

    LOG_D("uac function enable");

    return RT_EOK;
}

static rt_err_t _function_disable(ufunction_t func)
{
    RT_ASSERT(func != RT_NULL);

    LOG_D("uac function disable");
    _audio_stop(func);
    return RT_EOK;
}

static struct ufunction_ops ops =
{
    _function_enable,
    _function_disable,
    RT_NULL,
};
/**
 * This function will configure uac descriptor.
 *
 * @param comm the communication interface number.
 * @param data the data interface number.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _uac_descriptor_config(struct uac_ac_descriptor *ac,
                                       rt_uint8_t cintf_nr, struct uac_as_descriptor *as, rt_uint8_t sintf_nr)
{
    ac->hdr_desc.baInterfaceNr[0] = sintf_nr;
#ifdef RT_USB_DEVICE_COMPOSITE
    ac->iad_desc.bFirstInterface = cintf_nr;
#endif

    return RT_EOK;
}

static rt_err_t _uac_samplerate_config(struct uac_as_descriptor *as, rt_uint32_t samplerate)
{
    as->format_type_desc.tSamFreq[0 * 3 + 2] = samplerate >> 16 & 0xff;
    as->format_type_desc.tSamFreq[0 * 3 + 1] = samplerate >> 8 & 0xff;
    as->format_type_desc.tSamFreq[0 * 3 + 0] = samplerate & 0xff;
    return RT_EOK;
}

/**
 * This function will create a uac function instance.
 *
 * @param device the usb device object.
 *
 * @return RT_EOK on successful.
 */
ufunction_t rt_usbd_function_uac_speaker_create(udevice_t device)
{
    ufunction_t func;
    uintf_t intf_ac, intf_as;
    ualtsetting_t setting_as0;
    ualtsetting_t setting_ac, setting_as;
    struct uac_as_descriptor *as_desc_t;

    /* parameter check */
    RT_ASSERT(device != RT_NULL);

#ifdef RT_USB_DEVICE_COMPOSITE
    rt_usbd_device_set_interface_string(device, SPK_INTF_STR_INDEX, _ustring[2]);
#else
    /* set usb device string description */
    rt_usbd_device_set_string(device, _ustring);
#endif
    /* create a uac function */
    func = rt_usbd_function_new(device, &dev_desc, &ops);
    //not support HS
    //rt_usbd_device_set_qualifier(device, &dev_qualifier);

    /* create interface */
    intf_ac = rt_usbd_interface_new(device, RT_NULL);
    intf_as = rt_usbd_interface_new(device, _interface_as_handler);

    /* create alternate setting */
    setting_ac = rt_usbd_altsetting_new(sizeof(struct uac_ac_descriptor));
    setting_as0 = rt_usbd_altsetting_new(sizeof(struct uinterface_descriptor));
    setting_as = rt_usbd_altsetting_new(sizeof(struct uac_as_descriptor));
    /* config desc in alternate setting */
    rt_usbd_altsetting_config_descriptor(setting_ac, &ac_desc,
                                         (rt_off_t) & ((struct uac_ac_descriptor *)0)->intf_desc);
    rt_usbd_altsetting_config_descriptor(setting_as0, &as_desc0, 0);
    rt_usbd_altsetting_config_descriptor(setting_as, &as_desc,
                                         (rt_off_t) & ((struct uac_as_descriptor *)0)->intf_desc);
    /* configure the uac interface descriptor */
    _uac_descriptor_config(setting_ac->desc, intf_ac->intf_num, setting_as->desc, intf_as->intf_num);
    _uac_samplerate_config(setting_as->desc, AUDIO_SAMPLERATE);

    /* create endpoint */
    as_desc_t = (struct uac_as_descriptor *)setting_as->desc;
    speaker.ep = rt_usbd_endpoint_new(&as_desc_t->ep_desc, _ep_data_handler);

    /* add the endpoint to the alternate setting */
    rt_usbd_altsetting_add_endpoint(setting_as, speaker.ep);

    /* add the alternate setting to the interface, then set default setting of the interface */
    rt_usbd_interface_add_altsetting(intf_ac, setting_ac);
    rt_usbd_set_altsetting(intf_ac, 0);
    rt_usbd_interface_add_altsetting(intf_as, setting_as0);
    rt_usbd_interface_add_altsetting(intf_as, setting_as);
    rt_usbd_set_altsetting(intf_as, 0);

    /* add the interface to the uac function */
    rt_usbd_function_add_interface(func, intf_ac);
    rt_usbd_function_add_interface(func, intf_as);

    return func;
}

int audio_speaker_init(void)
{
    rt_thread_t speaker_tid;
    if (speaker.buffer == RT_NULL)
        speaker.buffer = rt_malloc(UAC_MAX_PACKET_SIZE);
    if (speaker.buffer == RT_NULL)
    {
        os_printf("speaker buffer malloc failed\n");
    }
    speaker.buffer_index = 0;
    speaker.event = rt_event_create("speaker_event", RT_IPC_FLAG_FIFO);

    usbd_audio_spk_msi_init(&speaker, 8);

    return RT_EOK;
}


/*
 *  register uac class
 */
static struct udclass uac_speaker_class =
{
    .rt_usbd_function_create = rt_usbd_function_uac_speaker_create
};


//需要初始化DAC的采样率，与UAC一致
int rt_usbd_uac_speaker_class_register(rt_uint32_t dev_id)
{
    rt_usbd_class_register(&uac_speaker_class, dev_id);
    return 0;
}


#endif
