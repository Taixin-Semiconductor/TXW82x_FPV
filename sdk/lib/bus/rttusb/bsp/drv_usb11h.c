#include <rtthread.h>
#include <include/rttusb_host.h>
#include "hal/usb_device.h"
#include "dev/usb/usb11_v0/hgusb11_v0_host_api.h"
#ifdef RT_USBH_UVC
#include "usbh_video.h"
#endif
#ifdef RT_USBH_UAC
#include "usbh_audio.h"
#endif

#define USB11_HOST_MAX_PIPE 3

typedef struct {
    struct usb_device *usb;
    uhcd_t hcd;
    rt_uint32_t pipe_in_index; // 按bit记录主控可用端点
    rt_uint32_t pipe_out_index;
    struct os_event trx_lock;
    struct os_event trx_done;
} usb_core_instance;

static usb_core_instance _hg_usbh;
static volatile rt_bool_t connect_status = RT_FALSE;

#define USBH_REGISTER_FLAG  BIT(1)
static volatile rt_uint32_t usbh_flag = 0;

static rt_uint32_t hg_usbh_irq_hdl(rt_uint32_t irq, rt_uint32_t param1, rt_uint32_t param2, rt_uint32_t param3)
{
    struct hgusb11_host *p_dev = (struct hgusb11_host *)dev_get(HG_USB11HOST_DEVID);
    usb_core_instance *core = (usb_core_instance *)param1;
    rt_uint32_t usb_ep = param2 & 0xF;

    switch (irq) {
        case USB_DEV_RESET_IRQ:
            break;
        case USB_DEV_SUSPEND_IRQ:
            break;
        case USB_DEV_RESUME_IRQ:
            break;
        case USB_DEV_SOF_IRQ:
            break;
        case USB_DEV_CTL_IRQ:
            break;
        case USB_EP_RX_IRQ:
            os_event_set(&core->trx_done, BIT(usb_ep+16), NULL);
            if (connect_status) {
#ifdef RT_USBH_UVC
            rtt_usbh_video_irq(p_dev, usb_ep, _hg_usbh.hcd);
#endif
#ifdef RT_USBH_UAC
            //rtt_usbh_audio_irq(p_dev, irq, usb_ep);
#endif
            } else {
                hgusb11_v0_host_ep_abort(_hg_usbh.usb, USB_CORE_HOST_EP_RX, usb_ep);
                hgusb11_v0_host_reset_ep_rxcsr(usb_ep);
                rt_kprintf("usb11 connect status is NULL , stop in rx irq\n");
            }
            break;
        case USB_EP_TX_IRQ:
            os_event_set(&core->trx_done, BIT(usb_ep), NULL);
            if (connect_status) {
#ifdef RT_USBH_UAC
            //rtt_usbh_audio_irq(p_dev, irq, usb_ep);
#endif
            } else {
                hgusb11_v0_host_ep_abort(_hg_usbh.usb, USB_CORE_HOST_EP_TX, usb_ep);
                hgusb11_v0_host_reset_ep_txcsr(usb_ep);
                rt_kprintf("usb11 connect status is NULL , stop in tx irq\n");
            }
            break;
        case USB_CONNECT:
            if (hgusb11_v0_host_dev_speed_detect() && (usbh_flag & USBH_REGISTER_FLAG)) {
                if (!connect_status) {
                    connect_status = RT_TRUE;
                    rt_kprintf("usb11 connected\r\n");
                    rt_usbh_root_hub_connect_handler(core->hcd, 1, RT_TRUE);
                    // hg_usb_connect_detect_using();
                }
            }
            break;
        case USB_DISCONNECT:
            if (!hgusb11_v0_host_dev_speed_detect() && (usbh_flag & USBH_REGISTER_FLAG)) {
                if (connect_status) {
                    connect_status = RT_FALSE;
                    rt_kprintf("usb11 disconnect\r\n");
                    rt_usbh_root_hub_disconnect_handler(core->hcd, 1);
                    // hg_usb_connect_detect_recfg();
                }
            }
            break;
        case USB_BABBLE:
            break;
        case USB_XACT_ERR:
            break;
        default:
            break;
    }
    return 0;
}

static rt_err_t drv_reset_port(rt_uint8_t port)
{
    rt_kprintf("usb11 reset port\r\n");
    hgusb11_v0_host_reset();
    hgusb11_v0_host_set_address( 0);
    return RT_EOK;
}

static rt_uint8_t drv_get_free_pipe_index(rt_uint32_t *pipe_index)
{
    rt_uint8_t idx;
    for (idx = 1; idx <= USB11_HOST_MAX_PIPE; ++idx) {
        if (!(*pipe_index & BIT(idx))) {
            *pipe_index |= BIT(idx);
            return idx;
        }
    }
    return 0xff;
}

static inline void drv_free_pipe_index(rt_uint32_t *pipe_index, rt_uint8_t index)
{
    *pipe_index &= ~BIT(index);
}

static inline rt_bool_t drv_pipe_index_check(rt_uint32_t *pipe_index, rt_uint8_t index)
{
    return (*pipe_index & BIT(index)) ? RT_TRUE : RT_FALSE;
}

static int drv_pipe_xfer(upipe_t pipe, rt_uint8_t token, void *buffer, int nbytes, int timeouts)
{
    struct usb_device *hgusb = (struct usb_device *)_hg_usbh.usb;
    rt_int32_t ret = RET_OK;
    int total_len = 0;
    rt_size_t remain_size;
    remain_size = nbytes;
    rt_uint8_t * pbuffer = (rt_uint8_t *)buffer;

    if (!connect_status) {
        os_printf("connect_status is null\n");
        return -RT_EIO;
    }
    if (pipe->pipe_index > USB11_HOST_MAX_PIPE) {
        return -RT_EIO;
    }
    if ((pipe->ep.bEndpointAddress & USB_DIR_MASK) == USB_DIR_IN) {
        if (!drv_pipe_index_check(&_hg_usbh.pipe_in_index, pipe->pipe_index)) {
            return -RT_EIO;
        }
    } else {
        if (!drv_pipe_index_check(&_hg_usbh.pipe_out_index, pipe->pipe_index)) {
            return -RT_EIO;
        }
    }
    
    hgusb11_v0_host_set_address(pipe->inst->address);

    if ((pipe->ep.bEndpointAddress & USB_DIR_MASK) == USB_DIR_IN) {
        // RX不用互斥
        ret = os_event_wait(&_hg_usbh.trx_lock, BIT(pipe->pipe_index+16), NULL,
                            OS_EVENT_WMODE_AND|OS_EVENT_WMODE_CLEAR, osWaitForever);
        if (ret) {
            LOG_D("drv_pipe_xfer rx req timeout!\r\n");
            pipe->status = UPIPE_STATUS_ERROR;
            return RET_ERR;
        }
        // 端点0软件分包
        if (pipe->pipe_index == 0) {
            ret = hgusb11_v0_host_ep0_rx_kick(hgusb, pbuffer, remain_size);
            if(ret == RET_OK) {
                total_len = remain_size;
            } else {
                pipe->status = UPIPE_STATUS_ERROR;
                total_len = ret;
            }
        } else {
            os_event_clear(&_hg_usbh.trx_done, BIT(pipe->pipe_index+16), NULL);
            hgusb11_v0_host_ep_rx_kick(hgusb, pipe->pipe_index, (rt_uint8_t*)buffer, remain_size);
            ret = os_event_wait(&_hg_usbh.trx_done, BIT(pipe->pipe_index+16), NULL,
                        OS_EVENT_WMODE_AND|OS_EVENT_WMODE_CLEAR, (timeouts ? timeouts : osWaitForever));
            if (ret) {
                LOG_D("drv_pipe_xfer rx timeout!\r\n");
                pipe->status = UPIPE_STATUS_ERROR;
                hgusb11_v0_host_ep_abort(_hg_usbh.usb, USB_CORE_HOST_EP_RX, pipe->pipe_index);
            }
            else
            {
                total_len = hgusb11_v0_host_ep_get_rx_dma_len(pipe->pipe_index);
                if (hgusb11_v0_host_is_xact_err(pipe->pipe_index, USB_DIR_IN) 
                    || hgusb11_v0_host_is_rx_stall(pipe->pipe_index, USB_DIR_IN))
                {
                    os_printf("%s usb1.1 rx dma err or stall, ep num is %d!!!!!\n",__FUNCTION__,pipe->pipe_index);
                }
            }

        }
        os_event_set(&_hg_usbh.trx_lock, BIT(pipe->pipe_index+16), NULL);
    } else {
        // TX需要互斥
        ret = os_event_wait(&_hg_usbh.trx_lock, BIT(pipe->pipe_index), NULL,
                    OS_EVENT_WMODE_AND|OS_EVENT_WMODE_CLEAR, osWaitForever);
        if (ret) {
            LOG_D("drv_pipe_xfer tx req timeout!\r\n");
            pipe->status = UPIPE_STATUS_ERROR;
            return RET_ERR;
        }
        if (token == USBH_PID_SETUP) {
            // setup包只会是ep0
            ret = hgusb11_v0_host_ep0_tx_kick(hgusb, 1, buffer, remain_size);
            if (ret != RET_OK) {
                //TX ERROR
                total_len = 0;
                pipe->status = UPIPE_STATUS_ERROR;
            } else {
                total_len = 8;
            }
        } else {
            // 端点0软件分包
            if (pipe->pipe_index == 0) {
                ret = hgusb11_v0_host_ep0_tx_kick(hgusb, 0, buffer, remain_size);
                if(ret == RET_OK) {
                    total_len = remain_size;
                } else {
                    total_len = ret;
                    pipe->status = UPIPE_STATUS_ERROR;
                }                
            } else {
                os_event_clear(&_hg_usbh.trx_done, BIT(pipe->pipe_index), NULL);
                hgusb11_v0_host_ep_tx_kick(hgusb, pipe->pipe_index, (rt_uint8_t*)buffer, remain_size, 0);
                ret = os_event_wait(&_hg_usbh.trx_done, BIT(pipe->pipe_index), NULL,
                            OS_EVENT_WMODE_AND|OS_EVENT_WMODE_CLEAR, (timeouts ? timeouts : osWaitForever));
                if (ret) {
                    LOG_D("drv_pipe_xfer tx timeout!\r\n");
                    pipe->status = UPIPE_STATUS_ERROR;
                    hgusb11_v0_host_ep_abort(_hg_usbh.usb, USB_CORE_HOST_EP_TX, pipe->pipe_index);
                    total_len = RET_ERR;
                } else {
                    total_len = remain_size;
                    if (hgusb11_v0_host_is_xact_err(pipe->pipe_index, USB_DIR_OUT) 
                        || hgusb11_v0_host_is_rx_stall(pipe->pipe_index, USB_DIR_OUT))
                    {
                        os_printf("%s usb1.1 tx dma err or stall, ep num is %d!!!!!\n",__FUNCTION__,pipe->pipe_index);
                        total_len = 0;
                    }
                }
                
            }
        }
        os_event_set(&_hg_usbh.trx_lock, BIT(pipe->pipe_index) | BIT(0), NULL);
    }

    if (pipe->callback != RT_NULL) pipe->callback(pipe);
    // 其余情况都是和请求长度一致
    return total_len;
}

static rt_err_t drv_open_pipe(upipe_t pipe)
{

    if ((pipe->ep.bEndpointAddress & USB_DIR_MASK) == USB_DIR_IN) {
        if ((pipe->ep.bEndpointAddress & USB_EPNO_MASK) != 0) {
            os_printf("drv open rx pipe\n");
            pipe->pipe_index = drv_get_free_pipe_index(&_hg_usbh.pipe_in_index);

            hgusb11_v0_host_ep_init(_hg_usbh.usb,
                USB_CORE_HOST_EP_RX,
                pipe->pipe_index,
                (pipe->ep.bEndpointAddress & USB_EPNO_MASK),
                (pipe->ep.bmAttributes & USB_EP_ATTR_TYPE_MASK),
                pipe->ep.wMaxPacketSize);

            if((pipe->ep.bmAttributes & USB_EP_ATTR_TYPE_MASK) != USB_EP_ATTR_BULK)
            {
                hgusb11_v0_host_set_ep_interval(USB_CORE_HOST_EP_RX, pipe->pipe_index, pipe->ep.bInterval);
            }
            else
            {
                //NAK TIMEOUT
                hgusb11_v0_host_set_ep_interval(USB_CORE_HOST_EP_RX, pipe->pipe_index, 0);
            }

        } else {
            _hg_usbh.pipe_in_index |= BIT(0);
        }
    } else {
        if ((pipe->ep.bEndpointAddress & USB_EPNO_MASK) != 0) {
            os_printf("drv open tx pipe\n");
            pipe->pipe_index = drv_get_free_pipe_index(&_hg_usbh.pipe_out_index);
            hgusb11_v0_host_ep_init(_hg_usbh.usb,
                USB_CORE_HOST_EP_TX,
                pipe->pipe_index,
                (pipe->ep.bEndpointAddress & USB_EPNO_MASK),
                (pipe->ep.bmAttributes & USB_EP_ATTR_TYPE_MASK),
                pipe->ep.wMaxPacketSize);
            if((pipe->ep.bmAttributes & USB_EP_ATTR_TYPE_MASK) != USB_EP_ATTR_BULK)
            {
                hgusb11_v0_host_set_ep_interval(USB_CORE_HOST_EP_TX, pipe->pipe_index, pipe->ep.bInterval);
            }
            else
            {
                //NAK TIMEOUT
                hgusb11_v0_host_set_ep_interval(USB_CORE_HOST_EP_TX, pipe->pipe_index, 0);
            }

        } else {
            _hg_usbh.pipe_out_index |= BIT(0);
        }
    }
    rt_kprintf("open pipe idx:%d ep%d dir:%x\r\n",
        pipe->pipe_index, (pipe->ep.bEndpointAddress & USB_EPNO_MASK), (pipe->ep.bEndpointAddress & USB_DIR_MASK));
    return RT_EOK;
}

static rt_err_t drv_close_pipe(upipe_t pipe)
{
    rt_kprintf("close pipe idx:%d\r\n", pipe->pipe_index);
    if ((pipe->ep.bEndpointAddress & USB_DIR_MASK) == USB_DIR_IN) {
        drv_free_pipe_index(&_hg_usbh.pipe_in_index, pipe->pipe_index);
        if (pipe->pipe_index != 0) {
            hgusb11_v0_host_ep_abort(_hg_usbh.usb, USB_CORE_HOST_EP_RX, pipe->pipe_index);
            os_event_set(&_hg_usbh.trx_lock, BIT(pipe->pipe_index+16), NULL);
        }
    } else {
        drv_free_pipe_index(&_hg_usbh.pipe_out_index, pipe->pipe_index);
        if (pipe->pipe_index != 0) {
            hgusb11_v0_host_ep_abort(_hg_usbh.usb, USB_CORE_HOST_EP_TX, pipe->pipe_index);
            os_event_set(&_hg_usbh.trx_lock, BIT(pipe->pipe_index), NULL);
        }
    }
    return RT_EOK;
}

static const struct uhcd_ops _uhcd_ops = {
    drv_reset_port,
    drv_pipe_xfer,
    drv_open_pipe,
    drv_close_pipe,
};

rt_err_t hg_usb11h_register(rt_uint32_t devid)
{
    uhcd_t uhcd = RT_NULL;
    
    uhcd = (uhcd_t)dev_get(devid);
    if (uhcd == RT_NULL) {
        uhcd = (uhcd_t)os_zalloc(sizeof(struct uhcd));
        if (uhcd == RT_NULL) {
            rt_kprintf("uhcd malloc failed\r\n");
            return -RT_ENOMEM;
        }

        os_printf("%s %d\n",__FUNCTION__,__LINE__);

        uhcd->ops = (uhcd_ops_t)&_uhcd_ops;
        uhcd->num_ports = 1;
        dev_register(devid, (struct dev_obj *)uhcd);

        os_memset(&_hg_usbh, 0, sizeof(usb_core_instance));
        _hg_usbh.hcd = uhcd;
        os_event_init(&_hg_usbh.trx_done);
        os_event_init(&_hg_usbh.trx_lock);
        os_event_set(&_hg_usbh.trx_lock, 0xffffffff, NULL);
        _hg_usbh.usb = (struct usb_device *)dev_get(HG_USB11HOST_DEVID);
        RT_ASSERT(_hg_usbh.usb);
        usbh_flag |= USBH_REGISTER_FLAG;
        usb_device_open(_hg_usbh.usb, NULL);
        usb_device_request_irq(_hg_usbh.usb, hg_usbh_irq_hdl, (rt_uint32_t)&_hg_usbh);

        rt_usb_host_init(devid, "usb11_host");
    }

    return RT_EOK;
}

rt_err_t hg_usb11h_unregister(rt_uint32_t devid)
{
    uhcd_t uhc;
    uhc = (uhcd_t)dev_get(devid);
    if (uhc) {
        printf("%s %d\n",__FUNCTION__,__LINE__);
        
        usbh_flag &= ~USBH_REGISTER_FLAG;
        
        if(connect_status) {
            connect_status = RT_FALSE;
            rt_usbh_root_hub_disconnect_handler(_hg_usbh.hcd, 1);
        }

        rt_usb_host_deinit(devid);

        if(_hg_usbh.usb)
        {
            usb_device_close(_hg_usbh.usb);
            os_event_del(&_hg_usbh.trx_done);
            os_event_del(&_hg_usbh.trx_lock);
            _hg_usbh.usb = RT_NULL;
        }

        if(_hg_usbh.hcd)
        {
            dev_unregister((struct dev_obj *)_hg_usbh.hcd);
            os_free(_hg_usbh.hcd);
            _hg_usbh.hcd = RT_NULL;
        }

    }

    return RT_EOK;
}