#include "include/rttusb_host.h"
#include "cdc.h"

#ifdef RT_USBH_CDC

#ifdef RT_USBH_CDC_THREAD
#define EVENT_CDC_DATA_START (1 << 0)
#define EVENT_CDC_DATA_END   (1 << 1)

static ucdc_data_t cdc_d;
static rt_event_t cdc_data_event;
uint8_t buff_out[64] __attribute__((aligned(4)));
uint8_t buff_in[64] __attribute__((aligned(4)));
#endif

static struct uclass_driver cdc_driver;

rt_err_t rt_usbh_cdc_send_command(uinst_t device, void* buffer, int nbytes)
{
    struct urequest setup;
    int timeout = USB_TIMEOUT_BASIC;

    RT_ASSERT(device != RT_NULL);

    setup.request_type = USB_REQ_TYPE_DIR_OUT | USB_REQ_TYPE_CLASS |
        USB_REQ_TYPE_INTERFACE;
    setup.bRequest = SEND_ENCAPSULATED_COMMAND;
    setup.wIndex = 0;
    setup.wLength = nbytes;
    setup.wValue = 0;

    if(rt_usb_hcd_setup_xfer(device->hcd, device->pipe_ep0_out, &setup, timeout) == 8)
    {
        if(rt_usb_hcd_pipe_xfer(device->hcd, device->pipe_ep0_out, buffer, nbytes, timeout) == nbytes)
        {
            if(rt_usb_hcd_pipe_xfer(device->hcd, device->pipe_ep0_in, RT_NULL, 0, timeout) == 0)
            {
                return nbytes;
            }
        }
    }
    return RT_ERROR;
}

rt_err_t rt_usbh_cdc_get_response(uinst_t device, void* buffer, int nbytes)
{
    struct urequest setup;
    int timeout = USB_TIMEOUT_BASIC;
    int ret_size;

    RT_ASSERT(device != RT_NULL);

    setup.request_type = USB_REQ_TYPE_DIR_IN | USB_REQ_TYPE_CLASS |
        USB_REQ_TYPE_INTERFACE;
    setup.bRequest = GET_ENCAPSULATED_RESPONSE;
    setup.wIndex = 0;
    setup.wLength = nbytes;
    setup.wValue = 0;

    if(rt_usb_hcd_setup_xfer(device->hcd, device->pipe_ep0_out, &setup, timeout) == 8)
    {
        ret_size = rt_usb_hcd_pipe_xfer(device->hcd, device->pipe_ep0_in, buffer, nbytes, timeout);
        if(ret_size > 0)
        {
            if(rt_usb_hcd_pipe_xfer(device->hcd, device->pipe_ep0_out, RT_NULL, 0, timeout) == 0)
            {
                return ret_size;
            }
        }
    }
    return RT_ERROR;
}

rt_err_t rt_usbh_cdc_get_line_coding(uinst_t device, int intf, void* buffer)
{
    struct urequest setup;
    int timeout = USB_TIMEOUT_BASIC;
    int ret_size;

    RT_ASSERT(device != RT_NULL);

    setup.request_type = USB_REQ_TYPE_DIR_IN | USB_REQ_TYPE_CLASS |
        USB_REQ_TYPE_INTERFACE;
    setup.bRequest = GET_LINE_CODING;
    setup.wIndex = intf;
    setup.wLength = 7;
    setup.wValue = 0;

    if(rt_usb_hcd_setup_xfer(device->hcd, device->pipe_ep0_out, &setup, timeout) == 8)
    {
        ret_size = rt_usb_hcd_pipe_xfer(device->hcd, device->pipe_ep0_in, buffer, 7, timeout);
        if(ret_size == 7)
        {
            if(rt_usb_hcd_pipe_xfer(device->hcd, device->pipe_ep0_out, RT_NULL, 0, timeout) == 0)
            {
                return ret_size;
            }
        }
    }
    return RT_ERROR;
}

rt_err_t rt_usbh_cdc_set_line_coding(uinst_t device, int intf, void* buffer)
{
    struct urequest setup;
    int timeout = USB_TIMEOUT_BASIC;

    RT_ASSERT(device != RT_NULL);
    setup.request_type = USB_REQ_TYPE_DIR_OUT | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE;
    setup.bRequest = SET_LINE_CODING;
    setup.wIndex = intf;   // interface
    setup.wLength = 7;
    setup.wValue = 0;

    if(rt_usb_hcd_setup_xfer(device->hcd, device->pipe_ep0_out, &setup, timeout) == 8)
    {
        if(rt_usb_hcd_pipe_xfer(device->hcd, device->pipe_ep0_out, buffer, 7, timeout) == 7)
        {
            if(rt_usb_hcd_pipe_xfer(device->hcd, device->pipe_ep0_in, RT_NULL, 0, timeout) == 0)
            {
                return RT_EOK;
            }
        }
    }
    return -RT_ERROR;
}

rt_err_t rt_usbh_cdc_set_control_line_state(uinst_t device, int intf, void * buffer, int len)
{
    struct urequest setup;
    int timeout = USB_TIMEOUT_BASIC;

    RT_ASSERT(device != RT_NULL);
    setup.request_type = USB_REQ_TYPE_DIR_OUT | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE;
    setup.bRequest = SET_CONTROL_LINE_STATE;
    setup.wIndex = intf;   // interface
    setup.wLength = len;
    setup.wValue = 0;

    if(rt_usb_hcd_setup_xfer(device->hcd, device->pipe_ep0_out, &setup, timeout) == 8)
    {    
        if(rt_usb_hcd_pipe_xfer(device->hcd, device->pipe_ep0_in, RT_NULL, 0, timeout) == 0)
        {
            return RT_EOK;
        }
    }
    return -RT_ERROR;
}

void analysis_cdc_line_coding(struct usb_cdc_line_coding * line_coding)
{
    os_printf("=========line coding=========\n");
    os_printf("dwDTERate:%d\n", line_coding->dwDTERate);
    os_printf("bCharFormat:%d\n", line_coding->bCharFormat);
    os_printf("bParityType:%d\n", line_coding->bParityType);
    os_printf("bDataBits:%d\n", line_coding->bDataBits);
    os_printf("=============================\n");
}

static rt_err_t rt_usbh_get_CDC_interface_descriptor(ucfg_desc_t cfg_desc, int num,
    uintf_desc_t* intf_desc)
{
    rt_uint32_t ptr, depth = 0;
    udesc_t desc;

    /* check parameter */
    RT_ASSERT(cfg_desc != RT_NULL);

    ptr = (rt_uint32_t)cfg_desc + cfg_desc->bLength;
    while(ptr < (rt_uint32_t)cfg_desc + cfg_desc->wTotalLength)
    {
        if(depth++ > 0x40)
        {
            *intf_desc = RT_NULL;
            return -RT_EIO;
        }
        desc = (udesc_t)ptr;
        if(desc->type == USB_DESC_TYPE_INTERFACE)
        {
            if(((uintf_desc_t)desc)->bNumEndpoints == 0)
            {
                ptr = (rt_uint32_t)desc + desc->bLength;
                continue;
            }
            if(((uintf_desc_t)desc)->bInterfaceNumber == num)
            {
                *intf_desc = (uintf_desc_t)desc;

                LOG_D("rt_usb_get_interface_descriptor: %d", num);
                return RT_EOK;
            }
        }
        ptr = (rt_uint32_t)desc + desc->bLength;
    }

    rt_kprintf("rt_usb_get_interface_descriptor %d failed\n", num);
    return -RT_EIO;
}

#ifdef RT_USBH_CDC_THREAD

int32 demo_atcmd_cdc_trans_ctrl(const char *cmd, char *argv[], uint32 argc)
{
    int indx = 0;
    if(*argv[0] == '1') {
        rt_usbh_cdc_trans_init();
    } else if(*argv[0] == '2') {
        rt_usbh_cdc_trans_deinit();
    }
	printf("OK/n");
    return 0;
}

void rt_usbh_cdc_trans_init()
{
    if (cdc_data_event != RT_NULL) {
        rt_event_send(cdc_data_event, EVENT_CDC_DATA_START);
    }
}

void rt_usbh_cdc_trans_deinit()
{
    if (cdc_data_event != RT_NULL) {
        rt_event_send(cdc_data_event, EVENT_CDC_DATA_END);
    }
}


static rt_err_t rt_usbh_cdc_communication_thread(void* arg)
{
    int i;
    struct uhintf **intf = arg;
    uhcd_t hcd = NULL;
    uintf_desc_t intf_desc;
    int timeout = USB_TIMEOUT_BASIC;
    rt_uint32_t e;
    struct usb_cdc_line_coding line_coding;

    os_printf("rt_usbh_cdc_communication_thread arg:%x\n",arg);

    cdc_d->thread_state = 1;
    cdc_d->line_coding = &line_coding;

    while(1)
    {
        if (rt_event_recv(cdc_data_event, EVENT_CDC_DATA_START | EVENT_CDC_DATA_END, 
                          RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, 
                          1000, &e) != RT_EOK)
        {
            continue;
        }

        if (e & EVENT_CDC_DATA_START)
        {

        }else if(e & EVENT_CDC_DATA_END)
        {
            goto __exit;
        }else
        {
            continue;
        }

        os_printf("cdc translate strat\n");

        memset(&line_coding, 0, sizeof(struct usb_cdc_line_coding));
        rt_usbh_cdc_get_line_coding(intf[0]->device, intf[0]->intf_desc->bInterfaceNumber, &line_coding);
        analysis_cdc_line_coding(&line_coding);

        line_coding.dwDTERate = BAUD_RATE_2000000;
        line_coding.bCharFormat = STOP_BITS_1;
        line_coding.bParityType = PARITY_NONE;
        line_coding.bDataBits = DATA_BITS_8;

        rt_usbh_cdc_set_line_coding(intf[0]->device, intf[0]->intf_desc->bInterfaceNumber, &line_coding);
        analysis_cdc_line_coding(&line_coding);

        memset(&line_coding, 0, sizeof(struct usb_cdc_line_coding));
        rt_usbh_cdc_get_line_coding(intf[0]->device, intf[0]->intf_desc->bInterfaceNumber, &line_coding);

        rt_usbh_cdc_set_control_line_state(intf[0]->device, intf[0]->intf_desc->bInterfaceNumber, RT_NULL, 0);
        analysis_cdc_line_coding(&line_coding);

        if(cdc_d->pipe_in == RT_NULL && cdc_d->pipe_out == RT_NULL)
        {
            for(i = 0; i < intf[1]->intf_desc->bNumEndpoints; i++)
            {
                uep_desc_t ep_desc;
                upipe_t pipe;
                rt_usbh_get_endpoint_descriptor(intf[1]->intf_desc, i, &ep_desc);
                if(ep_desc == RT_NULL)
                {
                    rt_kprintf("rt_usb_get_endpoint_descriptor error\n");
                    return -RT_ERROR;
                }
                analysis_usb_ep_desc(ep_desc); //获取端点描述符 打印端点描述符信息
                /* the endpoint type of mass storage class should be BULK */
                if((ep_desc->bmAttributes & USB_EP_ATTR_TYPE_MASK) != USB_EP_ATTR_BULK)
                    continue;
                
                if (rt_usb_hcd_alloc_pipe(intf[0]->device->hcd, &pipe, intf[0]->device, ep_desc) != RT_EOK) {
                    rt_kprintf("alloc pipe failed\n");
                    return -RT_ERROR;
                }
    
                rt_usb_instance_add_pipe(intf[0]->device, pipe);
    
                if ((ep_desc->bEndpointAddress & USB_DIR_MASK) == USB_DIR_IN) {
                    cdc_d->pipe_in = pipe;
                    os_printf("cdc_d->pipe_in:%x pipe:%x\n",cdc_d->pipe_in,pipe);
                } else {
                    cdc_d->pipe_out = pipe;
                    os_printf("cdc_d->pipe_out:%x pipe:%x\n",cdc_d->pipe_out,pipe);
                }     
    
            }
        }

        if(cdc_d->pipe_in != RT_NULL && cdc_d->pipe_out != RT_NULL)
        {
            buff_out[0] = 0xAA;
            buff_out[1] = 0x55;
            buff_out[2] = 0xD1;
            buff_out[3] = 0x00;
            buff_out[4] = 0x00;
            buff_out[5] = 0x00;
            buff_out[6] = 0x00;
            buff_out[7] = 0x00;
            buff_out[8] = 0x00;
            buff_out[9] = 0xD0;
    
            memset(buff_in,0,10);
    
            rt_usb_hcd_pipe_xfer(intf[0]->device->hcd, cdc_d->pipe_out, buff_out, 64, timeout);
    
            rt_usb_hcd_pipe_xfer(intf[0]->device->hcd, cdc_d->pipe_in, buff_in, 64, timeout); 
    
            for(int i = 0; i < 10; i++)
            {
                printf("buff_in[%d]:%x\n",i,buff_in[i]);
            }
        }
    }

__exit:

    rt_thread_suspend(cdc_d->thread);
    cdc_d->thread_state = 0;

    return RT_EOK;

}

#endif

static rt_err_t rt_usbh_cdc_enable(void *arg)
{
    struct uhintf **intf = arg;
    uhcd_t hcd = NULL;

    if (intf[0] == NULL) {
        return -EIO;
    }

    hcd = intf[0]->device->hcd;
    os_printf("subclass %d, protocal %d\r\n",
        intf[0]->intf_desc->bInterfaceSubClass,
        intf[0]->intf_desc->bInterfaceProtocol);

#ifdef RT_USBH_CDC_THREAD
    cdc_d = rt_malloc(sizeof(struct ucdc_data));
    if(cdc_d == RT_NULL)
    {
        rt_kprintf("allocate cdc_d memory failed\n");
        return -RT_ENOMEM;
    }

    rt_memset(cdc_d, 0, sizeof(struct ucdc_data));

    cdc_d->device = intf[0]->device;

    os_printf("cdc_d:%x  cdc_d->device:%x hcd:%x\n",cdc_d,cdc_d->device,cdc_d->device->hcd);

    intf[0]->user_data = (void *)cdc_d;

    os_printf("rt_usbh_cdc_enable arg:%x\n",arg);

    cdc_data_event = rt_event_create("cdc_data_event", RT_IPC_FLAG_FIFO);

    cdc_d->thread = rt_thread_create("cdc_comm_thread",rt_usbh_cdc_communication_thread,arg,1024,OS_TASK_PRIORITY_NORMAL,0);
    if(cdc_d->thread != RT_NULL)
    {
        rt_thread_startup(cdc_d->thread);
    } 

#endif

    return RET_OK;
}

static rt_err_t rt_usbh_cdc_disable(void *arg)
{
    // struct uhintf *intf = arg;

#ifdef RT_USBH_CDC_THREAD

    rt_usbh_cdc_trans_deinit();

    while(!cdc_d->thread_state)
    {
        rt_thread_delay(1);
    };

    if(cdc_d->thread != RT_NULL)
    {
        rt_thread_delete(cdc_d->thread);
        cdc_d->thread = RT_NULL;
    }

    if(cdc_data_event != RT_NULL)
    {
        rt_event_delete(cdc_data_event);
        cdc_data_event = RT_NULL;
    }

    if(cdc_d != RT_NULL)
    {
        rt_free(cdc_d);
        cdc_d = RT_NULL;
    }

#endif

    return RET_OK;
}

ucd_t rt_usbh_class_driver_cdc(void)
{
    cdc_driver.class_code = USB_CLASS_COMM;

    cdc_driver.enable = rt_usbh_cdc_enable;
    cdc_driver.disable = rt_usbh_cdc_disable;

    return &cdc_driver;
}

#endif