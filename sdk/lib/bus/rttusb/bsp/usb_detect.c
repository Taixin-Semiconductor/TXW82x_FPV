/*
    USB 主从切换检测
*/
#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "dev/usb/hgusb20_v1_dev_api.h"
#include "osal/work.h"
#include "include/rttusb_host.h"
#include "include/rttusb_device.h"
#include "drv_usbd.h"
#include "lib/heap/sysheap.h"


enum {
    USB_STATUS_NONE,     //未连接
    USB_STATUS_DEVICE,   //当Device
    USB_STATUS_HOST,     //当Host
    USB_STATUS_USING,    //使用中
};

struct usb_connect_structure
{
    struct os_work usb_detect_wk;
    uint32_t usb_connect_status;            //0：无状态  1：USB_STATUS_DEVICE   2：USB_STATUS_HOST  3：USB使用中
    uint32_t usb_connect_last_status;
}hg_usb_detect;

rt_err_t hg_usbh_register(rt_uint32_t devid);
rt_err_t hg_usbh_unregister(rt_uint32_t devid);

extern struct hgusb20_dev usb20_dev;
static void hg_usb_detect_device_open()
{
    uint32 mask = 0;
    struct usb_device *p_dev = (struct usb_device *)dev_get(HG_USBDEV_DEVID);

    if(p_dev == NULL)
    {
        os_printf("%s %d\n",__FUNCTION__,__LINE__);
        mask = disable_irq();
        hgusb20_dev_attach(HG_USBDEV_DEVID, &usb20_dev);
        enable_irq(mask);

        hg_usbd_recover(HG_USB_DEV_CONTROLLER_DEVID);
    }else{
        os_printf("%s p_dev is exist\n",__FUNCTION__);
    }

    
}

static void hg_usb_detect_device_close()
{
    struct usb_device *p_dev = (struct usb_device *)dev_get(HG_USBDEV_DEVID);

    if(p_dev)
    {
        os_printf("%s %d p_dev:%x\n",__FUNCTION__,__LINE__,p_dev);
        dev_unregister((struct dev_obj *)p_dev);
    }else{
        os_printf("%s p_dev is NULL\n",__FUNCTION__);
    }
}

static void hg_usb_detect_host_open()
{
    uint32 mask = 0;
    uint32_t ret;
    struct usb_device *p_dev = (struct usb_device *)dev_get(HG_USBDEV_DEVID);

    if(p_dev == NULL)
    {
        mask = disable_irq();
		ret = hgusb20_host_attach(HG_USBDEV_DEVID, &usb20_dev);
		enable_irq(mask);
        p_dev = (struct usb_device *)dev_get(HG_USBDEV_DEVID);

        hg_usbh_register(HG_USB_HOST_CONTROLLER_DEVID);
    }else{
        os_printf("%s p_dev is exist\n",__FUNCTION__);
    }

}

static void hg_usb_detect_host_close()
{
    struct usb_device *p_dev = (struct usb_device *)dev_get(HG_USBDEV_DEVID);

    if(p_dev)
    {
        os_printf("%s %d\n",__FUNCTION__,__LINE__);
        dev_unregister((struct dev_obj *)p_dev);
    }else{
        os_printf("%s p_dev is NULL\n",__FUNCTION__);
    }
}

static void hg_usb_detect_init()
{
    hg_usbh_unregister(HG_USB_HOST_CONTROLLER_DEVID);
    hg_usbd_unregister(HG_USB_DEV_CONTROLLER_DEVID);
}


static int32 hg_usb_connect_detect_work(struct os_work *work)
{
    uint8_t ret;
    uint8_t detect_tem = USB_CONNECTED_NONE;
    static uint32_t timeout = 0;
    struct hgusb20_dev *p_dev = (struct hgusb20_dev *)dev_get(HG_USBDEV_DEVID);
    if(p_dev == NULL)
    {
        os_printf("hg_usb_connect_detect_work: dev_get fail\n");
        goto __exit;
    }
    
    if(hg_usb_detect.usb_connect_status == USB_STATUS_NONE)
    {
        timeout++;
    }
    else
    {
        timeout = 0;
    }

    switch(hg_usb_detect.usb_connect_status) {
        case USB_STATUS_NONE:
            if(timeout > 2)                              
            {
                hg_usb_detect_init();                    //检测前关闭应用线程
                while(1)                                 //误检测判断
                {
                    ret = hgusb20_connected(p_dev);
                    if(detect_tem != ret) {
                        detect_tem = ret;
                    } else {
                        break;
                    }
                }
                os_printf("USB_STATUS_NONE freemem:%d \r\n", sysheap_freesize(&sram_heap));
                hg_usb_detect.usb_connect_status = detect_tem;
                os_printf("===hgusb20_connected ret:%d===\n",detect_tem);
            }                                         
        break;

        case USB_STATUS_DEVICE:                          //检测到插入电脑，关闭主机模式，开启从机模式
            hg_usb_detect_host_close();
            hg_usb_detect_device_open();
            os_printf("USB_STATUS_DEVICE freemem:%d \r\n", sysheap_freesize(&sram_heap));
            hg_usb_detect.usb_connect_last_status = USB_STATUS_DEVICE;
            hg_usb_detect.usb_connect_status = USB_STATUS_USING;
        break;

        case USB_STATUS_HOST:                            //检测到插入设备，关闭从机模式，开启主机模式
            hg_usb_detect_device_close();
            hg_usb_detect_host_open();
            os_printf("USB_STATUS_HOST freemem:%d \r\n", sysheap_freesize(&sram_heap));
            hg_usb_detect.usb_connect_last_status = USB_STATUS_HOST;
            hg_usb_detect.usb_connect_status = USB_STATUS_USING;
        break;

        case USB_STATUS_USING:
            if(hg_usb_detect.usb_connect_last_status == USB_STATUS_DEVICE) {  //如果是从机模式，在这里检测是否脱离主机
                ret = hgusb20_is_host_online(p_dev);
                os_printf("hgusb20_is_host_online:%d\n",ret);
                if(!ret) {
                    hg_usb_detect.usb_connect_status = USB_STATUS_NONE;
                } else {
                    hg_usb_detect.usb_connect_status = USB_STATUS_USING;
                }
            }
            os_sleep_ms(10);
        break;
    }

__exit:
    os_run_work_delay(&hg_usb_detect.usb_detect_wk, 500);         //重新检测的时间配置
    return 0;
}

void hg_usb_connect_detect_using(void)
{
    os_printf("%s %d\n",__FUNCTION__,__LINE__);
    hg_usb_detect.usb_connect_status = USB_STATUS_USING;
}

void hg_usb_connect_detect_recfg(void)
{
    os_printf("%s %d\n",__FUNCTION__,__LINE__);
    hg_usb_detect.usb_connect_status = USB_STATUS_NONE;
}

void hg_usb_connect_detect_init(void)
{
    /* RTT USB Device 未留有释放内存接口，避免重复申请空间，因而先初始化 USB Device */
    /* 前面一定要在 devic.c 先调用 hgusb20_dev_attach                            */
    /* 若需同时注册多个device设备，需定义宏RT_USB_DEVICE_COMPOSITE成复合设备       */
    /* 若要注册host设备请参考 usbhost.c 文件                                     */

    hg_usbd_class_driver_register();
    
    hg_usbd_register(HG_USB_DEV_CONTROLLER_DEVID);

    OS_WORK_INIT(&hg_usb_detect.usb_detect_wk, hg_usb_connect_detect_work, 0);
    os_run_work_delay(&hg_usb_detect.usb_detect_wk, 500);
}

void hg_usb_connect_detect_deinit(void)
{
    os_work_cancle(&hg_usb_detect.usb_detect_wk, 1);
}