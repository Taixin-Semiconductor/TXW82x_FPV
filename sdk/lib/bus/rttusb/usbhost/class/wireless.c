#include <rtthread.h>
#include <include/rttusb_host.h>
#include "rndis.h"

#ifdef RT_USBH_WIRELESS

static struct uclass_driver wireless_driver;

static rt_err_t rt_usbh_wireless_enable(void *arg)
{
    struct uhintf **intf = arg;
    uhcd_t hcd = NULL;
    uep_desc_t ep_desc = NULL;
    struct usb_rndis *rndis = NULL;
    upipe_t pipe;
    rt_uint8_t ep_index;

    if (intf[0] == NULL) {
        return -EIO;
    }

    hcd = intf[0]->device->hcd;
    os_printf("subclass %d, protocal %d\r\n",
        intf[0]->intf_desc->bInterfaceSubClass,
        intf[0]->intf_desc->bInterfaceProtocol);

    if (intf[0]->intf_desc->bInterfaceSubClass == 1 && intf[0]->intf_desc->bInterfaceProtocol == 3) {
#ifdef STATIC_RNDIS_NETDEV
        rndis = (struct usb_rndis *)dev_get(HG_LTE_RNDIS_DEVID);
#else
        rndis = (struct usb_rndis *)os_zalloc(sizeof(struct usb_rndis));
#endif
        if (rndis == NULL) {
            os_printf("rndis alloc fail\r\n");
            return -ENOMEM;
        }
        rndis->msg_buffer = (rt_uint8_t *)os_malloc(128);
        if (rndis->msg_buffer == NULL) {
            os_printf("rndis msg buffer alloc fail\r\n");
            os_free(rndis);
            return -ENOMEM;
        }
        rndis->data_buffer = (rt_uint8_t *)os_malloc(2048);
        if (rndis->data_buffer == NULL) {
            os_printf("rndis data buffer alloc fail\r\n");
            os_free(rndis->msg_buffer);
            os_free(rndis);
            return -ENOMEM;
        }
        rndis->ts_buffer = (rt_uint8_t *)os_zalloc(2048);
        if(rndis->ts_buffer == NULL) {
            os_printf("rndis ts_buf alloc fail\r\n");
            os_free(rndis->msg_buffer);
            os_free(rndis->data_buffer);
            os_free(rndis);
            return -ENOMEM;
        }
        rndis->ts_saveLength = 0;
        rndis->device = intf[0]->device;
        rndis->req_id = 1;
        intf[0]->user_data = rndis;

        for (ep_index = 0; ep_index < intf[0]->intf_desc->bNumEndpoints; ++ep_index) {
            rt_usbh_get_endpoint_descriptor(intf[0]->intf_desc, ep_index, &ep_desc);
            if (ep_desc == NULL) {
                return RET_ERR;
            }

            if ((ep_desc->bmAttributes & USB_EP_ATTR_TYPE_MASK) != USB_EP_ATTR_INT)
                continue;
            
            if ((ep_desc->bEndpointAddress & USB_DIR_MASK) == USB_DIR_IN) {
                if (rt_usb_hcd_alloc_pipe(intf[0]->device->hcd, &pipe, intf[0]->device, ep_desc) != RT_EOK) {
                    rt_kprintf("alloc pipe failed\n");
                    return -RT_ERROR;
                }
                rt_usb_instance_add_pipe(intf[0]->device, pipe);
                rndis->pipe_int = pipe;
            }
        }

        for (ep_index = 0; ep_index < intf[1]->intf_desc->bNumEndpoints; ++ep_index) {
            rt_usbh_get_endpoint_descriptor(intf[1]->intf_desc, ep_index, &ep_desc);
            if (ep_desc == NULL) {
                return RET_ERR;
            }
            
            if ((ep_desc->bmAttributes & USB_EP_ATTR_TYPE_MASK) != USB_EP_ATTR_BULK)
                continue;
            
            if (rt_usb_hcd_alloc_pipe(intf[0]->device->hcd, &pipe, intf[0]->device, ep_desc) != RT_EOK) {
                rt_kprintf("alloc pipe failed\n");
                return -RT_ERROR;
            }
            rt_usb_instance_add_pipe(intf[0]->device, pipe);
            if ((ep_desc->bEndpointAddress & USB_DIR_MASK) == USB_DIR_IN) {
                rndis->pipe_in = pipe;
            } else {
                rndis->pipe_out = pipe;
            }
        }

        rt_usbh_rndis_run(rndis);
    }

    return RET_OK;
}

static rt_err_t rt_usbh_wireless_disable(void *arg)
{
    struct uhintf *intf = arg;
    struct usb_rndis *rndis = NULL;

    if (intf == NULL) {
        return -EIO;
    }

    rndis = intf->user_data;
    if (rndis) {
        rt_usbh_rndis_stop(rndis);
    }

    return RET_OK;
}

ucd_t rt_usbh_class_driver_wireless(void)
{
    wireless_driver.class_code = USB_CLASS_WIRELESS;

    wireless_driver.enable = rt_usbh_wireless_enable;
    wireless_driver.disable = rt_usbh_wireless_disable;

    return &wireless_driver;
}

#endif