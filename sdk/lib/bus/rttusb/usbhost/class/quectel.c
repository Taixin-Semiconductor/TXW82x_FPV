#include <rtthread.h>
#include <include/rttusb_host.h>
#include "quectel.h"
#include "cdc.h"
#include "rndis.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#include "lwip/tcpip.h"
#include "netif/ethernetif.h"
#include "lib/common/sysevt.h"

#ifdef RT_USBH_VENDOR_QUECTEL

#define USB_VENDOR_ID_QUECTEL   0x2C7C
#define USB_PRODUCT_ID_QUECTEL  0x0903 // EC801E-CN

#define QUECTEL_ATCMD_BUFF_SIZE 64

static char recv_str[QUECTEL_ATCMD_BUFF_SIZE];

static struct uclass_driver quectel_driver;

static rt_bool_t send_recv_atcmd_check(void *context, const char *send_str,
            const char *check_str, char *ret_str)
{
    struct usb_quectel_at *quectel_at = context;
    uinst_t device = quectel_at->device;
    int recv_size;
    char *recv_str = NULL;
    rt_bool_t pass = RT_FALSE;

    os_sprintf((char *)quectel_at->at_cmd_buff, send_str);
    rt_usb_hcd_pipe_xfer(device->hcd, quectel_at->pipe_out,
        quectel_at->at_cmd_buff, os_strlen(send_str), 0);
    os_sleep_ms(10); // 需要点延迟给LTE模组反应
    do {
        // 每次读取会清除缓存
        os_memset(quectel_at->at_cmd_buff, 0, QUECTEL_ATCMD_BUFF_SIZE);
        recv_size = rt_usb_hcd_pipe_xfer(device->hcd, quectel_at->pipe_in,
            quectel_at->at_cmd_buff, QUECTEL_ATCMD_BUFF_SIZE, 10);
        if (recv_size > 0) {
            recv_str = os_strstr(quectel_at->at_cmd_buff, check_str);
            if (recv_str != NULL) {
                pass = RT_TRUE;
                if (ret_str != NULL && recv_size > os_strlen(check_str)) {
                    // ret_str存在说明需要获取返回结果做额外判断，复制到函数外面
                    os_memcpy(ret_str, recv_str + os_strlen(check_str),
                        recv_size - os_strlen(check_str));
                    ret_str[recv_size - os_strlen(check_str)] = '\0'; // 字符串结束符
                }
            }
        }
    } while (recv_size > 0);

    return pass;
}

static void quectel_at_recv(void *context)
{
    struct usb_quectel_at *quectel_at = context;

    while (1) {
        if (quectel_at->retry > 3) {
            quectel_at->retry = 0;
            quectel_at->state = QUECTEL_STATE_UNKNOW;
        }
        // os_printf("recv state:%d\r\n", quectel_at->state);
        switch (quectel_at->state) {
            case QUECTEL_STATE_UNKNOW:
                quectel_at->retry = 0;
                quectel_at->state = QUECTEL_STATE_CHECK_AT_STATUS;
                break;
            case QUECTEL_STATE_CHECK_AT_STATUS:
                // AT查询模块是否工作启动初始化流程
                if (send_recv_atcmd_check(quectel_at, "AT\r\n", "OK", NULL) == RT_TRUE) {
                    quectel_at->retry = 0;
                    quectel_at->state = QUECTEL_STATE_CHECK_SIM_STATUS;
                    os_printf("AT ready\r\n");
                } else {
                    quectel_at->retry++;
                    os_sleep(1);
                }
                break;
            case QUECTEL_STATE_CHECK_SIM_STATUS:
                if (send_recv_atcmd_check(quectel_at, "AT+CPIN?\r\n", "+CPIN: READY", NULL) == RT_TRUE) {
                    quectel_at->retry = 0;
                    quectel_at->state = QUECTEL_STATE_CHECK_CS_STATUS;
                    os_printf("SIM ready\r\n");
                } else {
                    quectel_at->retry++;
                    os_sleep(1);
                }
                break;
            case QUECTEL_STATE_CHECK_CS_STATUS:
                // AT+CREG查询CS域网络注册状态
                if (send_recv_atcmd_check(quectel_at, "AT+CREG?\r\n", "+CREG: 0,1", NULL) == RT_TRUE) {
                    quectel_at->retry = 0;
                    quectel_at->state = QUECTEL_STATE_CHECK_PS_STATUS;
                    os_printf("PS ready\r\n");
                } else {
                    quectel_at->retry++;
                    os_sleep(1);
                }
                break;
            case QUECTEL_STATE_CHECK_PS_STATUS:
                // AT+CEREG查询EPS域网络注册状态
                if (send_recv_atcmd_check(quectel_at, "AT+CEREG?\r\n", "+CEREG: 0,1", NULL) == RT_TRUE) {
                    quectel_at->retry = 0;
                    quectel_at->state = QUECTEL_STATE_CHECK_USBNET_STATUS;
                    os_printf("CS ready\r\n");
                } else {
                    quectel_at->retry++;
                    os_sleep(1);
                }
                break;
            case QUECTEL_STATE_CHECK_USBNET_STATUS:
                // 查询usb网卡接口
                if (send_recv_atcmd_check(quectel_at, "AT+QCFG=\"usbnet\"\r\n", "+QCFG: \"usbnet\",", recv_str) == RT_TRUE) {
                    if (os_atoi(recv_str) == 1) {
                        quectel_at->retry = 0;
                        quectel_at->state = QUECTEL_STATE_CONFIG_USBNET_STATUS;
                        os_printf("USBNET is ECM\r\n");
                    } else {
                        quectel_at->retry = 0;
                        quectel_at->state = QUECTEL_STATE_CONFIG_PDP_CONTEXT;
                        os_printf("USBNET is RNDIS\r\n");
                    }
                } else {
                    quectel_at->retry++;
                    os_sleep(1);
                }
                break;
            case QUECTEL_STATE_CONFIG_USBNET_STATUS:
                // ecm网卡，发送命令切换到rndis网卡
                if (send_recv_atcmd_check(quectel_at, "AT+QCFG=\"usbnet\",3\r\n", "OK", NULL) == RT_TRUE) {
                    quectel_at->retry = 0;
                    quectel_at->state = QUECTEL_STATE_POWERDOWN;
                    os_printf("switch to RNDIS\r\n");
                } else {
                    quectel_at->retry++;
                    os_sleep(1);
                }
                break;
            case QUECTEL_STATE_CONFIG_PDP_CONTEXT:
                // AT+QICSGP设置场景参数
                if (send_recv_atcmd_check(quectel_at, "AT+QICSGP=1,1,\"UNINET\",\"\",\"\",1\r\n", "OK", NULL) == RT_TRUE) {
                    quectel_at->retry = 0;
                    quectel_at->state = QUECTEL_STATE_ACTIVE_PDP_CONTEXT;
                    os_printf("PDP config\r\n");
                } else {
                    quectel_at->retry++;
                    os_sleep(1);
                }
                break;
            case QUECTEL_STATE_ACTIVE_PDP_CONTEXT:
                // AT+QIACT激活PDP场景
                if (send_recv_atcmd_check(quectel_at, "AT+QIACT=1\r\n", "OK", NULL) == RT_TRUE) {
                    quectel_at->retry = 0;
                    quectel_at->state = QUECTEL_STATE_CHECK_IP_STATUS;
                    os_printf("PDP ready\r\n");
                } else {
                    quectel_at->retry++;
                    if (quectel_at->retry > 3) {
                        quectel_at->retry = 0;
                        quectel_at->state = QUECTEL_STATE_DEACTIVE_PDP_CONTEXT;
                    }
                    os_sleep(1);
                }
                break;
            case QUECTEL_STATE_CHECK_IP_STATUS:
                // AT+QIACT查询激活PDP场景和IP
                if (send_recv_atcmd_check(quectel_at, "AT+QIACT?\r\n", "+QIACT: 1,1,1,", recv_str) == RT_TRUE) {
                    quectel_at->retry = 0;
                    quectel_at->state = QUECTEL_STATE_CONNECT_USB_ADAPTER;
                    os_printf("Get IP: %s", recv_str);
                    os_printf("PDP actived\r\n");
                } else {
                    quectel_at->retry++;
                    os_sleep(1);
                }
                break;
            case QUECTEL_STATE_CONNECT_USB_ADAPTER:
                // AT+QNETDEVCTL连接USB网卡
                if (send_recv_atcmd_check(quectel_at, "AT+QNETDEVCTL=1,1,1\r\n", "OK", recv_str) == RT_TRUE) {
                    quectel_at->retry = 0;
                    quectel_at->state = QUECTEL_STATE_INITIALIZED;
                    SYSEVT_NEW_LTE_EVT(SYSEVT_LTE_CONNECTED, 0);
                    os_printf("network conneted\r\n");
                } else {
                    quectel_at->retry++;
                    os_sleep(1);
                }
                break;
            case QUECTEL_STATE_INITIALIZED:
                // 定时10s不知道干什么好呢
                os_sleep(10);
                break;
            case QUECTEL_STATE_DEACTIVE_PDP_CONTEXT:
                // 失败后反激活模组
                if (send_recv_atcmd_check(quectel_at, "AT+QIDEACT=1\r\n", "OK", NULL) == RT_TRUE) {
                    quectel_at->retry = 0;
                    quectel_at->state = QUECTEL_STATE_CHECK_SIM_STATUS;
                    os_printf("PDP deactived\r\n");
                } else {
                    quectel_at->retry++;
                    os_sleep(1);
                }
                break;
            case QUECTEL_STATE_POWERDOWN:
                if (send_recv_atcmd_check(quectel_at, "AT+QPOWD\r\n", "POWERED DOWN", NULL) == RT_TRUE) {
                    quectel_at->retry = 0;
                    os_printf("LTE power down\r\n");
                    os_sleep(2);
                    mcu_reset();
                } else {
                    quectel_at->retry++;
                    os_sleep(1);
                }
                break;
            default:
                break;
        }
    }
    os_printf("at task end\r\n");
}

static rt_err_t rt_usbh_quectel_enable(void *arg)
{
    rt_err_t ret = RET_OK;
    struct uhintf *intf = arg;
    uhcd_t hcd = NULL;
    uep_desc_t ep_desc = NULL;
    struct ustring_descriptor str_desc __attribute__((aligned(4)));
    struct usb_quectel_at *quectel_at = NULL;
    struct usb_cdc_line_coding line_coding;
    upipe_t pipe;
    rt_uint8_t ep_index, i;

    if (intf == NULL) {
        return -EIO;
    }

    hcd = intf->device->hcd;
    if (intf->device->dev_desc.idProduct == USB_PRODUCT_ID_QUECTEL) {
        // 怎么有两个AT串口，先直接用第3个接口判断好了
        if (intf->intf_desc->bInterfaceNumber != 3) {
            return RET_ERR;
        }
        // 通过字符描述符判断哪个是主调试串口
        ret = rt_usbh_get_string_descriptor(intf->device, intf->intf_desc->iInterface,
                &str_desc, sizeof(struct ustring_descriptor));
        if (ret != RET_OK) {
            return ret;
        }
        for (i = 0; i < str_desc.bLength; i += 2) { // 暂时没支持UNICODE的打印，默认ASCII可以隔一个打印
            _os_printf("%c", str_desc.String[i / 2]);
        }
        _os_printf("\r\n");

        os_printf("quectel_at\r\n");
        quectel_at = (struct usb_quectel_at *)os_zalloc(sizeof(struct usb_quectel_at));
        if (quectel_at == NULL) {
            os_printf("quectel at alloc fail\r\n");
            return -ENOMEM;
        }
        // 按照命令预期回复长度对齐预留长度
        quectel_at->at_cmd_buff = os_malloc(QUECTEL_ATCMD_BUFF_SIZE);
        if (quectel_at->at_cmd_buff == NULL) {
            os_printf("at_cmd_buff alloc fail\r\n");
            os_free(quectel_at);
            return -ENOMEM;
        }
        quectel_at->device = intf->device;
        intf->user_data = quectel_at;
        // 顺便注册devid
        dev_register(HG_USB_AT_DEVID, (struct dev_obj *)quectel_at);

        for (ep_index = 0; ep_index < intf->intf_desc->bNumEndpoints; ++ep_index) {
            rt_usbh_get_endpoint_descriptor(intf->intf_desc, ep_index, &ep_desc);
            if (ep_desc == NULL) {
                return RET_ERR;
            }

            if ((ep_desc->bmAttributes & USB_EP_ATTR_TYPE_MASK) != USB_EP_ATTR_BULK)
                continue;
            
            if (rt_usb_hcd_alloc_pipe(intf->device->hcd, &pipe, intf->device, ep_desc) != RT_EOK) {
                rt_kprintf("alloc pipe failed\n");
                return -RT_ERROR;
            }
            rt_usb_instance_add_pipe(intf->device, pipe);
            if ((ep_desc->bEndpointAddress & USB_DIR_MASK) == USB_DIR_IN) {
                quectel_at->pipe_in = pipe;
            } else {
                quectel_at->pipe_out = pipe;
            }
        }

        // 获取串口参数
        os_memset(&line_coding, 0, sizeof(line_coding));
        rt_usbh_cdc_get_line_coding(intf->device, intf->intf_desc->bInterfaceNumber, &line_coding);
        os_printf("Serial: %d %d %d %d\r\n",
            line_coding.dwDTERate, line_coding.bCharFormat, line_coding.bParityType, line_coding.bDataBits);

        rt_thread_init(&quectel_at->recv_task, "at_recv", quectel_at_recv, quectel_at,
                        NULL, 512, OS_TASK_PRIORITY_BELOW_NORMAL, 20);
        rt_thread_startup(&quectel_at->recv_task);
    }

    return ret;
}

static rt_err_t rt_usbh_quectel_disable(void *arg)
{
    struct uhintf *intf = arg;
    uhcd_t hcd = NULL;
    struct usb_quectel_at *quectel_at = NULL;

    if (intf == NULL) {
        return -EIO;
    }

    quectel_at = intf->user_data;
    if (quectel_at) {
        hcd = intf->device->hcd;
        dev_unregister((struct dev_obj *)quectel_at);
        rt_thread_detach(&quectel_at->recv_task);
        os_free(quectel_at->at_cmd_buff);
        os_free(quectel_at);
    }

    return RET_OK;
}

ucd_t rt_usbh_class_driver_quectel(void)
{
    quectel_driver.class_code = USB_CLASS_VEND_SPECIFIC;
    quectel_driver.vendor_id = USB_VENDOR_ID_QUECTEL;

    quectel_driver.enable = rt_usbh_quectel_enable;
    quectel_driver.disable = rt_usbh_quectel_disable;

    return &quectel_driver;
}

#endif