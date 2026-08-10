#include <rtthread.h>
#include <include/rttusb_host.h>
#include "chinamobile.h"
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

#ifdef RT_USBH_VENDOR_CHINAMOBILE

#define USB_VENDOR_ID_CHINAMOBILE   0x2ECC
#define USB_PRODUCT_ID_CHINAMOBILE  0x3012 // ML307R

#define CHINAMOBILE_ATCMD_BUFF_SIZE 128

static char recv_str[CHINAMOBILE_ATCMD_BUFF_SIZE];

static struct uclass_driver chinamobile_driver;

static rt_bool_t send_recv_atcmd_check(void *context, const char *send_str,
            const char *check_str, char *ret_str)
{
    struct usb_chinamobile_at *chinamobile_at = context;
    uinst_t device = chinamobile_at->device;
    int recv_size;
    char *recv_str = NULL;
    rt_bool_t pass = RT_FALSE;

    os_sprintf((char *)chinamobile_at->at_cmd_buff, send_str);
    rt_usb_hcd_pipe_xfer(device->hcd, chinamobile_at->pipe_out,
        chinamobile_at->at_cmd_buff, os_strlen(send_str), 0);
    os_sleep_ms(10); // 需要点延迟给LTE模组反应
    do {
        // 每次读取会清除缓存
        os_memset(chinamobile_at->at_cmd_buff, 0, CHINAMOBILE_ATCMD_BUFF_SIZE);
        recv_size = rt_usb_hcd_pipe_xfer(device->hcd, chinamobile_at->pipe_in,
            chinamobile_at->at_cmd_buff, CHINAMOBILE_ATCMD_BUFF_SIZE, 10);
        if (recv_size > 0) {
            recv_str = os_strstr(chinamobile_at->at_cmd_buff, check_str);
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

// 计算下行频点，FDD不知道上行频点号，猜测直接偏固定频率？
static rt_uint16_t eutra_channel_freq_mapping(rt_uint8_t band, rt_uint16_t earfcn, rt_bool_t uplink)
{
    rt_uint16_t freq = 0;

    switch (band) {
        // FDD上下行频点有偏差
        case 3:
            freq = 1805 + (earfcn - 1200) / 10;
            if (uplink)
                freq -= 95;
            break;
        case 5:
            freq = 869 + (earfcn - 2400) / 10;
            if (uplink)
                freq -= 45;
            break;
        case 8:
            freq = 925 + (earfcn - 3450) / 10;
            if (uplink)
                freq -= 45;
            break;
        // TDD上下行使用相同频点
        case 34: freq = 2010 + (earfcn - 36200) / 10; break;
        case 38: freq = 2570 + (earfcn - 37750) / 10; break;
        case 39: freq = 1880 + (earfcn - 38250) / 10; break;
        case 40: freq = 2300 + (earfcn - 38650) / 10; break;
        case 41: freq = 2496 + (earfcn - 39650) / 10; break;
        default: break;
    }

    return freq;
}

static void chinamobile_network_info(void *context, char *recv_str)
{
    struct usb_chinamobile_at *chinamobile_at = context;
    //char *argv[16];
    char **argv = NULL;
    int argc = 0;
    rt_int16_t rsrp, rssi, sinr, rsrq;
    rt_uint16_t num_dl;
    rt_uint16_t freq_dl, freq_ul;
    rt_uint8_t band = 0;

    argv = os_malloc(16 * sizeof(char *));
    if (argv == NULL) {
        return;
    }

    // 获取运营商
    if (send_recv_atcmd_check(chinamobile_at, "AT+COPS?\r\n", "+COPS:", recv_str) == RT_TRUE) {
        argc = os_strtok(recv_str, ",", argv, 16);
        if (argc >= 3) {
            // 0,0,"CHINA MOBILE",7
            _os_printf("%s\r\n", argv[2]);
        }
    }
    // 获取网络信息
    if (send_recv_atcmd_check(chinamobile_at, "AT+MUESTATS=\"sband\"\r\n", "+MUESTATS:", recv_str) == RT_TRUE) {
        argc = os_strtok(recv_str, ",", argv, 16);
        if (argc >= 2) {
            // "sband",39
            band = os_atoi(argv[1]);
            _os_printf("BAND: %d\r\n", band);
        }
    }
    if (send_recv_atcmd_check(chinamobile_at, "AT+MUESTATS=\"radio\"\r\n", "+MUESTATS:", recv_str) == RT_TRUE) {
        argc = os_strtok(recv_str, ",", argv, 16);
        if (argc >= 13) {
            // "radio",4,-790,-560,-32768,0,0,0A513F03,255,200,38400,197,-30
            rsrp = os_atoi(argv[2]); // 0.1dBm
            rssi = os_atoi(argv[3]); // 0.1dBm
            sinr = os_atoi(argv[9]); // 0.1dB
            rsrq = os_atoi(argv[12]); // 0.1dB
            num_dl = os_atoi(argv[10]);
            freq_dl = eutra_channel_freq_mapping(band, num_dl, 0);
            freq_ul = eutra_channel_freq_mapping(band, num_dl, 1);
            _os_printf("RSRP: %d.%d dBm\r\n", rsrp/10, os_abs(rsrp%10));
            _os_printf("RSSI: %d.%d dBm\r\n", rssi/10, os_abs(rssi%10));
            _os_printf("SINR: %d.%d dB\r\n", sinr/10, os_abs(sinr%10));
            _os_printf("RSRQ: %d.%d dB\r\n", rsrq/10, os_abs(rsrq%10));
            _os_printf("F_dl: %d MHz, F_ul: %d MHz\r\n", freq_dl, freq_ul);
        }
    }
    os_free(argv);
}

// 发送一条AT命令会有多条数据返回，发送后多次读循环
static void chinamobile_at_recv(void *context)
{
    struct usb_chinamobile_at *chinamobile_at = context;

    while (1) {
        if (chinamobile_at->retry > 3) {
            chinamobile_at->retry = 0;
            chinamobile_at->state = CHINAMOBILE_STATE_UNKNOW;
        }
        // os_printf("recv state:%d\r\n", chinamobile_at->state);
        switch (chinamobile_at->state) {
            case CHINAMOBILE_STATE_UNKNOW:
                chinamobile_at->retry = 0;
                chinamobile_at->state = CHINAMOBILE_STATE_CHECK_AT_STATUS;
                break;
            case CHINAMOBILE_STATE_CHECK_AT_STATUS:
                // AT查询模块是否工作启动初始化流程
                if (send_recv_atcmd_check(chinamobile_at, "AT\r\n", "OK", NULL) == RT_TRUE) {
                    chinamobile_at->retry = 0;
                    chinamobile_at->state = CHINAMOBILE_STATE_CHECK_SIM_STATUS;
                    os_printf("AT ready\r\n");
                } else {
                    chinamobile_at->retry++;
                    os_sleep(1);
                }
                break;
            case CHINAMOBILE_STATE_CHECK_SIM_STATUS:
                // AT+CPIN查询SIM卡状态
                if (send_recv_atcmd_check(chinamobile_at, "AT+CPIN?\r\n", "+CPIN: READY", NULL) == RT_TRUE) {
                    chinamobile_at->retry = 0;
                    chinamobile_at->state = CHINAMOBILE_STATE_CHECK_PS_STATUS;
                    os_printf("SIM ready\r\n");
                } else {
                    chinamobile_at->retry++;
                    os_sleep(1);
                }
                break;
            case CHINAMOBILE_STATE_CHECK_PS_STATUS:
                // AT+CEREG查询EPS域网络注册状态
                if (send_recv_atcmd_check(chinamobile_at, "AT+CEREG?\r\n", "+CEREG: 0,1", NULL) == RT_TRUE) {
                    chinamobile_at->retry = 0;
                    chinamobile_at->state = CHINAMOBILE_STATE_CHECK_PDP_CONTEXT;
                    os_printf("CS ready\r\n");
                } else {
                    chinamobile_at->retry++;
                    os_sleep(1);
                }
                break;
            case CHINAMOBILE_STATE_CHECK_PDP_CONTEXT:
                // AT+CGDCONT查询PDP场景
                if (send_recv_atcmd_check(chinamobile_at, "AT+CGDCONT?\r\n", "+CGDCONT: 1", NULL) == RT_TRUE) {
                    chinamobile_at->retry = 0;
                    chinamobile_at->state = CHINAMOBILE_STATE_CHECK_IP_STATUS;
                    os_printf("PDP ready\r\n");
                } else {
                    chinamobile_at->retry++;
                    os_sleep(1);
                }
                break;
            case CHINAMOBILE_STATE_CHECK_IP_STATUS:
                // AT+MDIALUP拨号上网
                if (send_recv_atcmd_check(chinamobile_at, "AT+MDIALUP=1,1\r\n", "+MDIALUP: 1,1,", recv_str) == RT_TRUE) {
                    chinamobile_at->retry = 0;
                    chinamobile_at->state = CHINAMOBILE_STATE_INITIALIZED;
                    os_printf("Get IP: %s", recv_str);
                    SYSEVT_NEW_LTE_EVT(SYSEVT_LTE_CONNECTED, 0);
                    os_printf("dial up\r\n");
                } else {
                    chinamobile_at->retry++;
                    os_sleep(1);
                }
                break;
            case CHINAMOBILE_STATE_INITIALIZED:
                // 查询网络信息
                chinamobile_network_info(chinamobile_at, recv_str);
                // 定时10s检查拨号上网状态
                if (send_recv_atcmd_check(chinamobile_at, "AT+MDIALUP?\r\n", "+MDIALUP: 1,1,", NULL) == RT_TRUE) {
                    chinamobile_at->retry = 0;
                    os_printf("LTE status ok\r\n");
                    os_sleep(10);
                } else {
                    chinamobile_at->retry++;
                    os_sleep(1);
                }
                break;
            default:
                break;
        }
    }
    os_printf("at task end\r\n");
}

static rt_err_t rt_usbh_chinamobile_enable(void *arg)
{
    rt_err_t ret = RET_OK;
    struct uhintf *intf = arg;
    uhcd_t hcd = NULL;
    uep_desc_t ep_desc = NULL;
    struct ustring_descriptor str_desc __attribute__((aligned(4)));
    struct usb_chinamobile_at *chinamobile_at = NULL;
    struct usb_cdc_line_coding line_coding;
    upipe_t pipe;
    rt_uint8_t ep_index, i;

    if (intf == NULL) {
        return -EIO;
    }

    hcd = intf->device->hcd;
    if (intf->device->dev_desc.idProduct == USB_PRODUCT_ID_CHINAMOBILE) {
        // 怎么有两个AT串口，先直接用第2个接口判断好了
        if (intf->intf_desc->bInterfaceNumber != 2) {
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

        os_printf("chinamobile_at\r\n");
        chinamobile_at = (struct usb_chinamobile_at *)os_zalloc(sizeof(struct usb_chinamobile_at));
        if (chinamobile_at == NULL) {
            os_printf("chinamobile at alloc fail\r\n");
            return -ENOMEM;
        }
        // 按照命令预期回复长度对齐预留长度
        chinamobile_at->at_cmd_buff = os_malloc(CHINAMOBILE_ATCMD_BUFF_SIZE);
        if (chinamobile_at->at_cmd_buff == NULL) {
            os_printf("at_cmd_buff alloc fail\r\n");
            os_free(chinamobile_at);
            return -ENOMEM;
        }
        chinamobile_at->device = intf->device;
        intf->user_data = chinamobile_at;
        // 顺便注册devid
        dev_register(HG_USB_AT_DEVID, (struct dev_obj *)chinamobile_at);

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
                chinamobile_at->pipe_in = pipe;
            } else {
                chinamobile_at->pipe_out = pipe;
            }
        }

        // 获取串口参数
        os_memset(&line_coding, 0, sizeof(line_coding));
        rt_usbh_cdc_get_line_coding(intf->device, intf->intf_desc->bInterfaceNumber, &line_coding);
        os_printf("Serial: %d %d %d %d\r\n",
            line_coding.dwDTERate, line_coding.bCharFormat, line_coding.bParityType, line_coding.bDataBits);

        rt_thread_init(&chinamobile_at->recv_task, "at_recv", chinamobile_at_recv, chinamobile_at,
                        NULL, 512, OS_TASK_PRIORITY_BELOW_NORMAL, 20);
        rt_thread_startup(&chinamobile_at->recv_task);
    }

    return ret;
}

static rt_err_t rt_usbh_chinamobile_disable(void *arg)
{
    struct uhintf *intf = arg;
    uhcd_t hcd = NULL;
    struct usb_chinamobile_at *chinamobile_at = NULL;

    if (intf == NULL) {
        return -EIO;
    }

    chinamobile_at = intf->user_data;
    if (chinamobile_at) {
        hcd = intf->device->hcd;
        dev_unregister((struct dev_obj *)chinamobile_at);
        rt_thread_detach(&chinamobile_at->recv_task);
        os_free(chinamobile_at->at_cmd_buff);
        os_free(chinamobile_at);
    }

    return RET_OK;
}

ucd_t rt_usbh_class_driver_chinamobile(void)
{
    chinamobile_driver.class_code = USB_CLASS_VEND_SPECIFIC;
    chinamobile_driver.vendor_id = USB_VENDOR_ID_CHINAMOBILE;

    chinamobile_driver.enable = rt_usbh_chinamobile_enable;
    chinamobile_driver.disable = rt_usbh_chinamobile_disable;

    return &chinamobile_driver;
}

#endif