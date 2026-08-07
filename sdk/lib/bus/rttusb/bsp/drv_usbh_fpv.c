#include <rtthread.h>
#include <include/rttusb_host.h>
#include "hal/usb_device.h"
#include "dev/usb/hgusb20_v1_dev_api.h"
#include "math.h"
#ifdef RT_USBH_UVC
#include "usbh_video.h"
#endif
#ifdef RT_USBH_UAC
#include "usbh_audio.h"
#endif

#define USB_HOST_MAX_PIPE 6

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
    struct hgusb20_dev *p_dev = (struct hgusb20_dev *)dev_get(HG_USBDEV_DEVID);
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
            rtt_usbh_audio_irq(p_dev, irq, usb_ep);
#endif
            } else {
                hgusb20_ep_rx_abort(p_dev, usb_ep);
                hgusb20_host_reset_ep_rxcsr(p_dev, usb_ep);
                rt_kprintf("connect status is NULL , stop in rx irq\n");
            }
            break;
        case USB_EP_TX_IRQ:
            os_event_set(&core->trx_done, BIT(usb_ep), NULL);
            if (connect_status) {
#ifdef RT_USBH_UAC
            rtt_usbh_audio_irq(p_dev, irq, usb_ep);
#endif
            } else {
                hgusb20_ep_tx_abort(p_dev, usb_ep);
                hgusb20_host_reset_ep_txcsr(p_dev, usb_ep);
                rt_kprintf("connect status is NULL , stop in tx irq\n");
            }
            break;
        case USB_CONNECT:
            if (hgusb20_is_device_online(p_dev) && (usbh_flag & USBH_REGISTER_FLAG)) {
                if (!connect_status) {
                    connect_status = RT_TRUE;
                    rt_kprintf("usb connected\r\n");
                    rt_usbh_root_hub_connect_handler(core->hcd, 1, RT_TRUE);
                    hg_usb_connect_detect_using();
                }
            }
            break;
        case USB_DISCONNECT:
            if (!hgusb20_is_device_online(p_dev) && (usbh_flag & USBH_REGISTER_FLAG)) {
                if (connect_status) {
                    connect_status = RT_FALSE;
                    rt_kprintf("usb disconnect\r\n");
                    rt_usbh_root_hub_disconnect_handler(core->hcd, 1);
                    hg_usb_connect_detect_recfg();
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
    rt_kprintf("reset port\r\n");
    hgusb20_host_reset((struct hgusb20_dev *)_hg_usbh.usb);
    hgusb20_set_address((struct hgusb20_dev *)_hg_usbh.usb, 0);
    return RT_EOK;
}

static rt_uint8_t drv_get_free_pipe_index(rt_uint32_t *pipe_index)
{
    rt_uint8_t idx;
    for (idx = 1; idx <= USB_HOST_MAX_PIPE; ++idx) {
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
    if (index == 0) {
        return RT_TRUE;
    } 

    return (*pipe_index & BIT(index)) ? RT_TRUE : RT_FALSE;
}

rt_uint8_t drv_get_specific_pipe(rt_uint8_t index, rt_uint8_t in_or_out)
{
    if(index <= USB_HOST_MAX_PIPE && index >= 0){
        if(in_or_out == USB_DIR_IN){
            os_printf("drv_get_specific_pipe in:%d\r\n",index);
            _hg_usbh.pipe_in_index |= BIT(index);
            return index;
        } else {
            os_printf("drv_get_specific_pipe out:%d\r\n",index);
            _hg_usbh.pipe_out_index |= BIT(index);
            return index;            
        }
    } else
        return 0xff;
}

void drv_free_specific_pipe(rt_uint8_t index, rt_uint8_t in_or_out)
{
    os_printf("drv_free_specific_pipe:%d\r\n",index);
    if(index <= USB_HOST_MAX_PIPE && index >= 0){
        if(in_or_out == USB_DIR_IN)
            _hg_usbh.pipe_in_index &= ~BIT(index);
        else
            _hg_usbh.pipe_out_index &= ~BIT(index);
    }
}


static int drv_pipe_xfer(upipe_t pipe, rt_uint8_t token, void *buffer, int nbytes, int timeouts)
{
    struct hgusb20_dev *hgusb = (struct hgusb20_dev *)_hg_usbh.usb;
    rt_int32_t ret = RET_OK;
    rt_bool_t first_pkg = RT_TRUE;
    int total_len = 0;
    rt_size_t remain_size;
    rt_size_t send_size;
    rt_size_t ret_size;
    remain_size = nbytes;
    rt_uint8_t * pbuffer = (rt_uint8_t *)buffer;

    if (!connect_status) {
        os_printf("connect_status is null\n");
        return -RT_EIO;
    }
    if (pipe->pipe_index > USB_HOST_MAX_PIPE) {
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
    
    pipe->status = UPIPE_STATUS_OK;

    hgusb20_set_address(hgusb, pipe->inst->address);

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
            do {
                send_size = (remain_size > pipe->ep.wMaxPacketSize) ? pipe->ep.wMaxPacketSize : remain_size;
                ret = hgusb20_host_ep0_rx(hgusb, pbuffer, first_pkg);
                if (ret) {
                    pipe->status = UPIPE_STATUS_ERROR;
                    break;
                }
                first_pkg = RT_FALSE;
                ret_size = hgusb->ep0_ptr.rx_len;
                total_len += hgusb->ep0_ptr.rx_len;
                if (ret_size == send_size) {
                    remain_size -= send_size;
                    pbuffer += send_size;
                } else {
                    // 收到包小于packet size即短包（应该不会大于吧）提前结束
                    break;
                }
            } while (remain_size > 0);
        } else {
            os_event_clear(&_hg_usbh.trx_done, BIT(pipe->pipe_index+16), NULL);
            hgusb20_ep_rx_kick(hgusb, pipe->pipe_index, (rt_uint32_t)buffer, nbytes);
            ret = os_event_wait(&_hg_usbh.trx_done, BIT(pipe->pipe_index+16), NULL,
                        OS_EVENT_WMODE_AND|OS_EVENT_WMODE_CLEAR, (timeouts ? timeouts : osWaitForever));
            if (ret) {
                LOG_D("drv_pipe_xfer rx timeout!\r\n");
                pipe->status = UPIPE_STATUS_ERROR;
                hgusb20_ep_rx_abort(hgusb, pipe->pipe_index);
            }
            else
            {
                total_len = hgusb20_ep_get_dma_rx_len(hgusb, pipe->pipe_index);
                if (hgusb20_host_is_xact_err(hgusb, pipe->pipe_index, USB_DIR_IN)
                    || hgusb20_host_is_rx_stall(hgusb, pipe->pipe_index, USB_DIR_IN)) 
                {
                    os_printf("%s usb2.0 rx dma err or stall, ep num is %d!!!!!\n",__FUNCTION__,pipe->pipe_index);
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
            os_memcpy(&hgusb->usb_ctrl.cmd, buffer, nbytes);
            ret = hgusb20_host_ep0_setup(hgusb);
            if (ret) {
                //TX ERROR
                total_len = 0;
                pipe->status = UPIPE_STATUS_ERROR;
            } else {
                total_len = 8;
            }
        } else {
            // 端点0软件分包
            if (pipe->pipe_index == 0) {
                do {
                    send_size = (remain_size > pipe->ep.wMaxPacketSize) ? pipe->ep.wMaxPacketSize : remain_size;
                    ret = hgusb20_host_ep0_tx(hgusb, buffer, send_size);
                    if (ret) {
                        pipe->status = UPIPE_STATUS_ERROR;
                        break;
                    }
                    total_len += send_size;
                    remain_size -= send_size;
                    pbuffer += send_size;
                } while (remain_size > 0);
            } else {
                os_event_clear(&_hg_usbh.trx_done, BIT(pipe->pipe_index), NULL);
                hgusb20_ep_tx_kick(hgusb, pipe->pipe_index, (rt_uint32_t)buffer, nbytes);
                ret = os_event_wait(&_hg_usbh.trx_done, BIT(pipe->pipe_index), NULL,
                            OS_EVENT_WMODE_AND|OS_EVENT_WMODE_CLEAR, (timeouts ? timeouts : osWaitForever));
                if (ret) {
                    os_printf("drv_pipe_xfer tx timeout!\r\n");
					total_len = 0;
                    pipe->status = UPIPE_STATUS_ERROR;
                    hgusb20_ep_tx_abort(hgusb, pipe->pipe_index);
                } else {
                    total_len = hgusb20_ep_get_tx_len(hgusb, pipe->pipe_index);
                    if (hgusb20_host_is_xact_err(hgusb, pipe->pipe_index, USB_DIR_OUT)
                        || hgusb20_host_is_rx_stall(hgusb, pipe->pipe_index, USB_DIR_OUT)) 
                    {
                        os_printf("%s usb2.0 tx dma err or stall, ep num is %d!!!!!\n",__FUNCTION__,pipe->pipe_index);
                        total_len = 0;
                    }
                }
            }
        }
        os_event_set(&_hg_usbh.trx_lock, BIT(pipe->pipe_index), NULL);
    }

    if (pipe->callback != RT_NULL) pipe->callback(pipe);
    // 其余情况都是和请求长度一致
    return total_len;
}

static rt_err_t drv_open_pipe(upipe_t pipe)
{
    rt_uint32_t m;
    if ((pipe->ep.bEndpointAddress & USB_DIR_MASK) == USB_DIR_IN) {
        if ((pipe->ep.bEndpointAddress & USB_EPNO_MASK) != 0) {
            os_printf("drv open rx pipe\n");
            pipe->pipe_index = drv_get_free_pipe_index(&_hg_usbh.pipe_in_index);

            hgusb20_pipe_attr usb_pipe;
            usb_pipe.dev_addr = pipe->inst->address;
            usb_pipe.dev_hub_port = pipe->inst->port;
            usb_pipe.u_usb_hub_addr.dev_hub_addr = 0;//hub_set_addr;
            usb_pipe.u_usb_hub_addr.hub_mtt_en = 0;//usb_sw->hub_mtt_en;
            usb_pipe.ep_host = pipe->pipe_index;
            usb_pipe.ep_dev = (pipe->ep.bEndpointAddress & USB_EPNO_MASK);
            usb_pipe.ep_type = (pipe->ep.bmAttributes & USB_EP_ATTR_TYPE_MASK);
            usb_pipe.ep_speed = 0;//usb_sw->child[usb_sw->current_connect_port].speed;
            usb_pipe.max_pkt_size =  pipe->ep.wMaxPacketSize;
            hgusb20_host_rx_pipe_combine((struct hgusb20_dev *)_hg_usbh.usb, &usb_pipe);

            if((pipe->ep.bmAttributes & USB_EP_ATTR_TYPE_MASK) != USB_EP_ATTR_BULK)
            {
                if((pipe->ep.bmAttributes & USB_EP_ATTR_TYPE_MASK) == USB_EP_ATTR_INT)
                {
                    if (((struct hgusb20_dev *)_hg_usbh.usb)->usb_ctrl.bus_high_speed) {
                        m = log2(pipe->ep.bInterval) + 1;
                        os_printf("m = %d\r\n",m);
                        hgusb20_host_set_interval((struct hgusb20_dev *)_hg_usbh.usb, pipe->pipe_index, USB_CORE_HOST_EP_RX, m);
                    } else {
                        hgusb20_host_set_interval((struct hgusb20_dev *)_hg_usbh.usb, pipe->pipe_index, USB_CORE_HOST_EP_RX, pipe->ep.bInterval);
                    }
                }
                else
                {
                    m = log2(pipe->ep.bInterval) + 1;
                    os_printf("m = %d\r\n",m);
                    hgusb20_host_set_interval((struct hgusb20_dev *)_hg_usbh.usb, pipe->pipe_index, USB_CORE_HOST_EP_RX, m);
                }
            }
            else
            {
                //NAK TIMEOUT
                hgusb20_host_set_interval((struct hgusb20_dev *)_hg_usbh.usb, pipe->pipe_index, USB_CORE_HOST_EP_RX, 0);
            }

        } else {
            _hg_usbh.pipe_in_index |= BIT(0);
        }
    } else {
        if ((pipe->ep.bEndpointAddress & USB_EPNO_MASK) != 0) {
            os_printf("drv open tx pipe\n");
            pipe->pipe_index = drv_get_free_pipe_index(&_hg_usbh.pipe_out_index);

            hgusb20_pipe_attr usb_pipe;
            usb_pipe.dev_addr = pipe->inst->address;
            usb_pipe.dev_hub_port = pipe->inst->port;
            usb_pipe.u_usb_hub_addr.dev_hub_addr = 0;//hub_set_addr;
            usb_pipe.u_usb_hub_addr.hub_mtt_en = 0;//usb_sw->hub_mtt_en;
            usb_pipe.ep_host = pipe->pipe_index;
            usb_pipe.ep_dev = (pipe->ep.bEndpointAddress & USB_EPNO_MASK);
            usb_pipe.ep_type = (pipe->ep.bmAttributes & USB_EP_ATTR_TYPE_MASK);
            usb_pipe.ep_speed = 0;//usb_sw->child[usb_sw->current_connect_port].speed;
            usb_pipe.max_pkt_size =  pipe->ep.wMaxPacketSize;
            hgusb20_host_tx_pipe_combine((struct hgusb20_dev *)_hg_usbh.usb, &usb_pipe);

            if((pipe->ep.bmAttributes & USB_EP_ATTR_TYPE_MASK) != USB_EP_ATTR_BULK)
            {
                if((pipe->ep.bmAttributes & USB_EP_ATTR_TYPE_MASK) == USB_EP_ATTR_INT)
                {
                    if (((struct hgusb20_dev *)_hg_usbh.usb)->usb_ctrl.bus_high_speed) {
                        m = log2(pipe->ep.bInterval) + 1;
                        os_printf("m = %d\r\n",m);
                        hgusb20_host_set_interval((struct hgusb20_dev *)_hg_usbh.usb, pipe->pipe_index, USB_CORE_HOST_EP_TX, m);
                    } else {
                        hgusb20_host_set_interval((struct hgusb20_dev *)_hg_usbh.usb, pipe->pipe_index, USB_CORE_HOST_EP_TX, pipe->ep.bInterval);
                    }
                }
                else
                {
                    m = log2(pipe->ep.bInterval) + 1;
                    os_printf("m = %d\r\n",m);
                    hgusb20_host_set_interval((struct hgusb20_dev *)_hg_usbh.usb, pipe->pipe_index, USB_CORE_HOST_EP_TX, m);
                }
            }
            else
            {
                //NAK TIMEOUT
                hgusb20_host_set_interval((struct hgusb20_dev *)_hg_usbh.usb, pipe->pipe_index, USB_CORE_HOST_EP_TX, 0);
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
            hgusb20_ep_rx_abort((struct hgusb20_dev *)_hg_usbh.usb, pipe->pipe_index);
            os_event_set(&_hg_usbh.trx_lock, BIT(pipe->pipe_index+16), NULL);
        }
    } else {
        drv_free_pipe_index(&_hg_usbh.pipe_out_index, pipe->pipe_index);
        if (pipe->pipe_index != 0) {
            hgusb20_ep_tx_abort((struct hgusb20_dev *)_hg_usbh.usb, pipe->pipe_index);
            os_event_set(&_hg_usbh.trx_lock, BIT(pipe->pipe_index), NULL);
        }
    }
    return RT_EOK;
}

static rt_err_t drv_hub_port_set(uhub_param_t param)
{
    hgusb20_pipe_attr usb_ep0_pipe;
    usb_ep0_pipe.u_usb_hub_addr.dev_hub_addr = param->hub_addr;
    usb_ep0_pipe.u_usb_hub_addr.hub_mtt_en = param->hub_mtt_en;
    usb_ep0_pipe.dev_addr = param->dev_addr;
    usb_ep0_pipe.dev_hub_port = param->dev_port;

    switch (param->dev_speed)
    {
        case RTTUSB_DEV_LowSpeed:
            usb_ep0_pipe.ep_speed = 0x3;
            break;

        case RTTUSB_DEV_FullSpeed:
            usb_ep0_pipe.ep_speed = 0x2;
            break;

        case RTTUSB_DEV_HighSpeed:
            usb_ep0_pipe.ep_speed = 0x1;
            break;

        default:
            break;
    }
    hgusb20_host_hub_ep0_mange((struct hgusb20_dev *)_hg_usbh.usb, &usb_ep0_pipe);    
    return RT_EOK;
}

static const struct uhcd_ops _uhcd_ops = {
    drv_reset_port,
    drv_pipe_xfer,
    drv_open_pipe,
    drv_close_pipe,
    drv_hub_port_set,
};

rt_err_t hg_usbh_register(rt_uint32_t devid)
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
        _hg_usbh.usb = (struct usb_device *)dev_get(HG_USBDEV_DEVID);
        RT_ASSERT(_hg_usbh.usb);
        usbh_flag |= USBH_REGISTER_FLAG;
        usb_device_open(_hg_usbh.usb, NULL);
        usb_device_request_irq(_hg_usbh.usb, hg_usbh_irq_hdl, (rt_uint32_t)&_hg_usbh);

        rt_usb_host_init(devid, "usb20_host");
    }

    return RT_EOK;
}

rt_err_t hg_usbh_unregister(rt_uint32_t devid)
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