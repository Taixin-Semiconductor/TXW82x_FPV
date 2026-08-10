#include <rtthread.h>
#include <include/rttusb_host.h>
#include "zxinfo.h"
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
#include "syscfg.h"

#ifdef RT_USBH_VENDOR_ZXINFO

// #define USB_VENDOR_ID_ZXINFO   0x3361
// #define USB_PRODUCT_ID_ZXINFO  0x7B6E // ZX800

#define ZXINFO_ATCMD_BUFF_SIZE 128

static char recv_str[ZXINFO_ATCMD_BUFF_SIZE];

static struct uclass_driver zxinfo_driver;

__weak void mifi_led_control(rt_int16_t rsrq, rt_int16_t rssi){};

static rt_bool_t send_recv_atcmd_check(void *context, const char *send_str,
            const char *check_str, char *ret_str)
{
    struct usb_zxinfo_at *zxinfo_at = context;
    uinst_t device = zxinfo_at->device;
    int recv_size;
    char *recv_str = NULL;
    rt_bool_t pass = RT_FALSE;

    os_sprintf((char *)zxinfo_at->at_cmd_buff, send_str);
    rt_usb_hcd_pipe_xfer(device->hcd, zxinfo_at->pipe_out,
        zxinfo_at->at_cmd_buff, os_strlen(send_str), 0);
    os_sleep_ms(10); // 需要点延迟给LTE模组反应
    do {
        // 每次读取会清除缓存
        os_memset(zxinfo_at->at_cmd_buff, 0, ZXINFO_ATCMD_BUFF_SIZE);
        recv_size = rt_usb_hcd_pipe_xfer(device->hcd, zxinfo_at->pipe_in,
            zxinfo_at->at_cmd_buff, ZXINFO_ATCMD_BUFF_SIZE, 10);
        if (recv_size > 0) {
            recv_str = os_strstr(zxinfo_at->at_cmd_buff, check_str);
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

static void zxinfo_network_info(void *context, char *recv_str)
{
    struct usb_zxinfo_at *zxinfo_at = context;
    //char *argv[4];
    char **argv = NULL;
    int argc = 0;
    rt_int16_t rssi, ber;
    rt_uint16_t num_dl;
    rt_uint16_t freq_dl, freq_ul;
    rt_uint16_t freq_wifi = 0;
    rt_uint8_t band = 0, rxqual = 0;

    argv = os_malloc(4 * sizeof(char *));
    if (argv == NULL) {
        return;
    }

    // 获取运营商
    if (send_recv_atcmd_check(zxinfo_at, "AT+COPS?\r\n", "+COPS:", recv_str) == RT_TRUE) {
        argc = os_strtok(recv_str, ",", argv, 4);
        if (argc >= 3) {
            // 0,0,"CHINA MOBILE",7
            _os_printf("%s\r\n", argv[2]);
        }
    }
    // 获取网络信息
    if (send_recv_atcmd_check(zxinfo_at, "AT+QNWINFO\r\n", "+QNWINFO:", recv_str) == RT_TRUE) {
        argc = os_strtok(recv_str, ",", argv, 4);
        if (argc >= 4) {
            // "FDD LTE",46001,"LTE BAND 3",1650
            band = os_atoi(argv[2] + 10);
            num_dl = os_atoi(argv[3]);
            freq_dl = eutra_channel_freq_mapping(band, num_dl, 0);
            freq_ul = eutra_channel_freq_mapping(band, num_dl, 1);
            _os_printf("BAND: %d\r\n", band);
            _os_printf("F_dl: %d MHz, F_ul: %d MHz\r\n", freq_dl, freq_ul);
            // 检查有无和wifi频点冲突
            freq_wifi = sys_status.channel * 5 + 2407;
            // TDD同频ul和dl一致
            if (band == 40) {
                if ((freq_wifi - freq_dl < 90) && sys_status.channel != 13) {
                    os_printf("!!!!!!! OVERLAP at %d and %d !!!!!!!\r\n", freq_wifi, freq_dl);
                    SYSEVT_NEW_LTE_EVT(SYSEVT_LTE_OVERLAP_WIFI, 13); // 切去高信道
                }
            } else if (band == 41) {
                if ((freq_dl - freq_wifi < 90) && sys_status.channel != 1) {
                    os_printf("!!!!!!! OVERLAP at %d and %d !!!!!!!\r\n", freq_wifi, freq_dl);
                    SYSEVT_NEW_LTE_EVT(SYSEVT_LTE_OVERLAP_WIFI, 1); // 切去低信道
                }
            }
        }
    }
    if (send_recv_atcmd_check(zxinfo_at, "AT+CSQ\r\n", "+CSQ:", recv_str) == RT_TRUE) {
        argc = os_strtok(recv_str, ",", argv, 4);
        if (argc >= 2) {
            // 28,99
            rssi = os_atoi(argv[0]); // dBm
            rxqual = os_atoi(argv[1]);
            mifi_led_control(0, -113 + rssi * 2);
            if (rssi == 99) {
                _os_printf("RSSI: unknow\r\n");
            } else if (rssi <= 0) {
                _os_printf("RSSI: <= -113 dBm\r\n");
            } else if (rssi >= 31) {
                _os_printf("RSSI: >= -51 dBm\r\n");
            } else {
                _os_printf("RSSI: %d dBm\r\n", -113 + rssi * 2);
            }
            // RXQUAL_0               BER < 0,2 %   Assumed value = 0,14 %
            // RXQUAL_1     0,2 %   < BER < 0,4 %   Assumed value = 0,28 %
            // RXQUAL_2     0,4 %   < BER < 0,8 %   Assumed value = 0,57 %
            // RXQUAL_3     0,8 %   < BER < 1,6 %   Assumed value = 1,13 %
            // RXQUAL_4     1,6 %   < BER < 3,2 %   Assumed value = 2,26 %
            // RXQUAL_5     3,2 %   < BER < 6,4 %   Assumed value = 4,53 %
            // RXQUAL_6     6,4 %   < BER < 12,8 %  Assumed value = 9,05 %
            // RXQUAL_7     12,8 %  < BER           Assumed value = 18,10 %
            ber = 1 << (rxqual + 1);
            if (rxqual == 99) {
                _os_printf("BER: unknow\r\n");
            } else if (rxqual <= 0) {
                _os_printf("BER: < 0.2 %%\r\n");
            } else if (rxqual >= 7) {
                _os_printf("BER: > 12.8 %%\r\n");
            } else {
                _os_printf("BER: %d.%d %% ~ %d.%d %%\r\n", (ber/2)/10, (ber/2)%10, ber/10, ber%10);
            }
        }
    }
    os_free(argv);
}

static void zxinfo_at_recv(void *context)
{
    struct usb_zxinfo_at *zxinfo_at = context;

    while (1) {
        if (zxinfo_at->retry > 10) {
            zxinfo_at->retry = 0;
            zxinfo_at->state = ZXINFO_STATE_UNKNOW;
        }
        // os_printf("recv state:%d\r\n", zxinfo_at->state);
        switch (zxinfo_at->state) {
            case ZXINFO_STATE_UNKNOW:
                zxinfo_at->retry = 0;
                zxinfo_at->state = ZXINFO_STATE_CHECK_AT_STATUS;
                break;
            case ZXINFO_STATE_CHECK_AT_STATUS:
                // AT查询模块是否工作启动初始化流程
                if (send_recv_atcmd_check(zxinfo_at, "AT\r\n", "OK", NULL) == RT_TRUE) {
                    zxinfo_at->retry = 0;
                    zxinfo_at->state = ZXINFO_STATE_CHECK_SIM_STATUS;
                    os_printf("AT ready\r\n");
                } else {
                    zxinfo_at->retry++;
                    os_sleep(1);
                }
                break;
            case ZXINFO_STATE_CHECK_SIM_STATUS:
                if (send_recv_atcmd_check(zxinfo_at, "AT+CPIN?\r\n", "+CPIN: READY", NULL) == RT_TRUE) {
                    zxinfo_at->retry = 0;
                    zxinfo_at->state = ZXINFO_STATE_CHECK_CS_STATUS;
                    os_printf("SIM ready\r\n");
                } else {
                    zxinfo_at->retry++;
                    os_sleep(1);
                }
                break;
            case ZXINFO_STATE_CHECK_CS_STATUS:
                // AT+CREG查询CS域网络注册状态
                if (send_recv_atcmd_check(zxinfo_at, "AT+CREG?\r\n", "+CREG: 0,1", NULL) == RT_TRUE) {
                    zxinfo_at->retry = 0;
                    zxinfo_at->state = ZXINFO_STATE_CHECK_PS_STATUS;
                    os_printf("PS ready\r\n");
                } else {
                    zxinfo_at->retry++;
                    os_sleep(1);
                }
                break;
            case ZXINFO_STATE_CHECK_PS_STATUS:
                // AT+CEREG查询EPS域网络注册状态
                if (send_recv_atcmd_check(zxinfo_at, "AT+CEREG?\r\n", "+CEREG: 0,1", NULL) == RT_TRUE) {
                    zxinfo_at->retry = 0;
                    zxinfo_at->state = ZXINFO_STATE_INITIALIZED;
                    SYSEVT_NEW_LTE_EVT(SYSEVT_LTE_CONNECTED, 0);
                    os_printf("CS ready\r\n");
                } else {
                    zxinfo_at->retry++;
                    os_sleep(1);
                }
                break;
            case ZXINFO_STATE_CONFIG_PDP_CONTEXT:
                // 设置band优先级，排除band 40/41
                send_recv_atcmd_check(zxinfo_at, "AT+QCFG=\"band\",0x0,0x6200000095\r\n", "+QCFG: \"band\",", NULL);
                // AT+QICSGP设置场景参数
                if (send_recv_atcmd_check(zxinfo_at, "AT+QICSGP=1,1,\"UNINET\",\"\",\"\",1\r\n", "OK", NULL) == RT_TRUE) {
                    zxinfo_at->retry = 0;
                    zxinfo_at->state = ZXINFO_STATE_ACTIVE_PDP_CONTEXT;
                    os_printf("PDP config\r\n");
                } else {
                    zxinfo_at->retry++;
                    os_sleep(1);
                }
                break;
            case ZXINFO_STATE_ACTIVE_PDP_CONTEXT:
                // AT+QIACT激活PDP场景
                if (send_recv_atcmd_check(zxinfo_at, "AT+QIACT=1\r\n", "OK", NULL) == RT_TRUE) {
                    zxinfo_at->retry = 0;
                    zxinfo_at->state = ZXINFO_STATE_CHECK_IP_STATUS;
                    os_printf("PDP ready\r\n");
                } else {
                    zxinfo_at->retry++;
                    if (zxinfo_at->retry > 3) {
                        zxinfo_at->retry = 0;
                        zxinfo_at->state = ZXINFO_STATE_DEACTIVE_PDP_CONTEXT;
                    }
                    os_sleep(1);
                }
                break;
            case ZXINFO_STATE_CHECK_IP_STATUS:
                // AT+QIACT查询激活PDP场景和IP
                if (send_recv_atcmd_check(zxinfo_at, "AT+QIACT?\r\n", "+QIACT: 1,1,1,", recv_str) == RT_TRUE) {
                    zxinfo_at->retry = 0;
                    zxinfo_at->state = ZXINFO_STATE_CONNECT_USB_ADAPTER;
                    os_printf("Get IP: %s", recv_str);
                    os_printf("PDP actived\r\n");
                } else {
                    zxinfo_at->retry++;
                    os_sleep(1);
                }
                break;
            case ZXINFO_STATE_CONNECT_USB_ADAPTER:
                // AT+QNETDEVCTL连接USB网卡
                if (send_recv_atcmd_check(zxinfo_at, "AT+QNETDEVCTL=1,1,1\r\n", "OK", recv_str) == RT_TRUE) {
                    zxinfo_at->retry = 0;
                    zxinfo_at->state = ZXINFO_STATE_INITIALIZED;
                    SYSEVT_NEW_LTE_EVT(SYSEVT_LTE_CONNECTED, 0);
                    os_printf("network conneted\r\n");
                } else {
                    zxinfo_at->retry++;
                    os_sleep(1);
                }
                break;
            case ZXINFO_STATE_INITIALIZED:
                // 定时10s不知道干什么好呢
                zxinfo_network_info(zxinfo_at, recv_str);
                os_sleep(10);
                break;
            case ZXINFO_STATE_DEACTIVE_PDP_CONTEXT:
                // 失败后反激活模组
                if (send_recv_atcmd_check(zxinfo_at, "AT+QIDEACT=1\r\n", "OK", NULL) == RT_TRUE) {
                    zxinfo_at->retry = 0;
                    zxinfo_at->state = ZXINFO_STATE_CHECK_SIM_STATUS;
                    os_printf("PDP deactived\r\n");
                } else {
                    zxinfo_at->retry++;
                    os_sleep(1);
                }
                break;
            case ZXINFO_STATE_POWERDOWN:
                if (send_recv_atcmd_check(zxinfo_at, "AT+QPOWD\r\n", "POWERED DOWN", NULL) == RT_TRUE) {
                    zxinfo_at->retry = 0;
                    os_printf("LTE power down\r\n");
                    os_sleep(2);
                    mcu_reset();
                } else {
                    zxinfo_at->retry++;
                    os_sleep(1);
                }
                break;
            default:
                break;
        }
    }
    os_printf("at task end\r\n");
}

static rt_err_t rt_usbh_zxinfo_enable(void *arg)
{
    rt_err_t ret = RET_OK;
    struct uhintf **intf = arg;
    uhcd_t hcd = NULL;
    uep_desc_t ep_desc = NULL;
    struct ustring_descriptor str_desc __attribute__((aligned(4)));
    struct usb_zxinfo_at *zxinfo_at = NULL;
    struct usb_cdc_line_coding line_coding;
    upipe_t pipe;
    rt_uint8_t ep_index, i;

    if (intf[0] == NULL) {
        return -EIO;
    }

    hcd = intf[0]->device->hcd;
    os_printf("subclass %d, protocal %d\r\n",
        intf[0]->intf_desc->bInterfaceSubClass,
        intf[0]->intf_desc->bInterfaceProtocol);

    if (intf[0]->intf_desc->bInterfaceSubClass == 2 && intf[0]->intf_desc->bInterfaceProtocol == 1) {
        // 中云信安的AT口也是走复合设备描述符的
        if (intf[0]->device->dev_desc.idProduct != USB_PRODUCT_ID_ZXINFO) {
            return RET_ERR;
        }
        // 中云信安有多个复合设备，第二个复合设备是AT串口，算起来接口号是5，但索引是用复合设备前面的那个所以是4
        if (intf[0]->intf_desc->bInterfaceNumber != 4) {
            return RET_ERR;
        }
        // 通过字符描述符判断哪个是主调试串口
        ret = rt_usbh_get_string_descriptor(intf[0]->device, intf[0]->intf_desc->iInterface,
                &str_desc, sizeof(struct ustring_descriptor));
        if (ret != RET_OK) {
            os_printf("no string\r\n");
            return ret;
        }
        for (i = 0; i < str_desc.bLength; i += 2) { // 暂时没支持UNICODE的打印，默认ASCII可以隔一个打印
            _os_printf("%c", str_desc.String[i / 2]);
        }
        _os_printf("\r\n");

        os_printf("zxinfo_at\r\n");
        zxinfo_at = (struct usb_zxinfo_at *)os_zalloc(sizeof(struct usb_zxinfo_at));
        if (zxinfo_at == NULL) {
            os_printf("zxinfo at alloc fail\r\n");
            return -ENOMEM;
        }
        // 按照命令预期回复长度对齐预留长度
        zxinfo_at->at_cmd_buff = os_malloc(ZXINFO_ATCMD_BUFF_SIZE);
        if (zxinfo_at->at_cmd_buff == NULL) {
            os_printf("at_cmd_buff alloc fail\r\n");
            os_free(zxinfo_at);
            return -ENOMEM;
        }
        zxinfo_at->device = intf[0]->device;
        intf[0]->user_data = zxinfo_at;
        // 顺便注册devid
        dev_register(HG_USB_AT_DEVID, (struct dev_obj *)zxinfo_at);

        for (ep_index = 0; ep_index < intf[1]->intf_desc->bNumEndpoints; ++ep_index) {
            rt_usbh_get_endpoint_descriptor(intf[1]->intf_desc, ep_index, &ep_desc);
            if (ep_desc == NULL) {
                os_printf("no ep_desc\r\n");
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
                zxinfo_at->pipe_in = pipe;
            } else {
                zxinfo_at->pipe_out = pipe;
            }
        }

        // 获取串口参数
        os_memset(&line_coding, 0, sizeof(line_coding));
        rt_usbh_cdc_get_line_coding(intf[0]->device, intf[0]->intf_desc->bInterfaceNumber, &line_coding);
        os_printf("Serial: %d %d %d %d\r\n",
            line_coding.dwDTERate, line_coding.bCharFormat, line_coding.bParityType, line_coding.bDataBits);

        rt_thread_init(&zxinfo_at->recv_task, "at_recv", zxinfo_at_recv, zxinfo_at,
                        NULL, 512, OS_TASK_PRIORITY_BELOW_NORMAL, 20);
        rt_thread_startup(&zxinfo_at->recv_task);
    }

    return ret;
}

static rt_err_t rt_usbh_zxinfo_disable(void *arg)
{
    struct uhintf *intf = arg;
    uhcd_t hcd = NULL;
    struct usb_zxinfo_at *zxinfo_at = NULL;

    if (intf == NULL) {
        return -EIO;
    }

    zxinfo_at = intf->user_data;
    if (zxinfo_at) {
        hcd = intf->device->hcd;
        dev_unregister((struct dev_obj *)zxinfo_at);
        rt_thread_detach(&zxinfo_at->recv_task);
        os_free(zxinfo_at->at_cmd_buff);
        os_free(zxinfo_at);
    }

    return RET_OK;
}

void rt_usbh_zxinfo_at_run(void *arg)
{
    rt_usbh_zxinfo_enable(arg);
}

void rt_usbh_zxinfo_at_stop(void *arg)
{
    rt_usbh_zxinfo_disable(arg);
}

// 中云信安使用CDC类型接口，不用注册VENDOR了，从cdc.c中调用enable处理
ucd_t rt_usbh_class_driver_zxinfo(void)
{
    zxinfo_driver.class_code = USB_CLASS_CDC;

    zxinfo_driver.enable = rt_usbh_zxinfo_enable;
    zxinfo_driver.disable = rt_usbh_zxinfo_disable;

    return &zxinfo_driver;
}

#endif