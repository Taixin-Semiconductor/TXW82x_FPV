#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/string.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "osal/work.h"
#include "osal/sleep.h"
#include "osal/timer.h"
#include "hal/gpio.h"
#include "hal/uart.h"
#include "lib/common/common.h"
#include "lib/common/sysevt.h"
#include "lib/heap/sysheap.h"
#include "lib/syscfg/syscfg.h"
#include "lib/lmac/lmac.h"
#include "lib/skb/skbpool.h"
#include "lib/atcmd/libatcmd.h"
#include "lib/bus/xmodem/xmodem.h"
#include "lib/net/skmonitor/skmonitor.h"
#include "lib/net/dhcpd/dhcpd.h"
#include "lib/umac/ieee80211.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#include "lwip/tcpip.h"
#include "netif/ethernetif.h"
#include "syscfg.h"

/* WiFi配对LED控制 */
#if SYS_WIFI_PAIR_LED
static struct os_work ledctrl_wk;
static struct os_work pairkey_wk;

struct {
    uint8 init: 1, last_val: 1, def_val: 1;
} sys_pairctrl;

struct {
    uint8 conn: 1, rssi1: 1, rssi2: 1, rssi3: 1, val: 1, init: 1, res: 1, pair: 1;
    uint8 wk_delay;
} sys_ledctrl;

static int32 sys_pairkey_val(void)
{
    int32 i, v;
    int32 v0 = 0, v1 = 0;

    for (i = 0; i < 50; i++) {
        v = gpio_get_val(WIFI_PAIRKEY_IO);
        if (v) {
            v1++;
        } else  {
            v0++;
        }
    }
    return (v1 > v0 ? 1 : 0);
}

static int32 sys_pairled_work(struct os_work *work)
{
    //init parameters
    if ((sys_ledctrl.init) == 0) {
        sys_ledctrl.init     = 1;
        sys_ledctrl.wk_delay = 100;
        sys_ledctrl.val      = 1;
        sys_ledctrl.pair     = 0;
        gpio_set_dir(WIFI_RSSI_IO1, GPIO_DIR_OUTPUT);
        gpio_set_dir(WIFI_RSSI_IO2, GPIO_DIR_OUTPUT);
        gpio_set_dir(WIFI_RSSI_IO3, GPIO_DIR_OUTPUT);
        gpio_set_val(WIFI_RSSI_IO1, 0);
        gpio_set_val(WIFI_RSSI_IO2, 0);
        gpio_set_val(WIFI_RSSI_IO3, 0);
    }

    if (!sys_ledctrl.pair) {
        //rssi led show, after connect and stop pair
        if (sys_ledctrl.conn) {
            gpio_set_val(WIFI_RSSI_IO1, sys_ledctrl.rssi1);
            gpio_set_val(WIFI_RSSI_IO2, sys_ledctrl.rssi2);
            gpio_set_val(WIFI_RSSI_IO3, sys_ledctrl.rssi3);
        } else {
            gpio_set_val(WIFI_RSSI_IO1, 0);
            gpio_set_val(WIFI_RSSI_IO2, 0);
            gpio_set_val(WIFI_RSSI_IO3, 0);
        }
    } else {
        //pair state
        gpio_set_val(WIFI_RSSI_IO1, 0);
        if (sys_ledctrl.val) {
            gpio_set_val(WIFI_RSSI_IO2, 1);
            gpio_set_val(WIFI_RSSI_IO3, 0);
        } else {
            gpio_set_val(WIFI_RSSI_IO2, 0);
            gpio_set_val(WIFI_RSSI_IO3, 1);
        }
        sys_ledctrl.val = !sys_ledctrl.val;
    }

    os_run_work_delay(&ledctrl_wk, sys_ledctrl.wk_delay);
    return 0;
}

static int32 sys_pairkey_work(struct os_work *work)
{
    int32 new_val;

    if (!sys_pairctrl.init) {
        sys_pairctrl.init = 1;
        gpio_set_dir(WIFI_PAIRKEY_IO, GPIO_DIR_INPUT);
        gpio_set_mode(WIFI_PAIRKEY_IO, GPIO_PULL_UP, GPIO_PULL_LEVEL_4_7K);
        new_val = sys_pairkey_val();
        sys_pairctrl.def_val  = new_val;
        sys_pairctrl.last_val = new_val;
    } else {
        new_val = sys_pairkey_val();
        if (new_val != sys_pairctrl.last_val) {
            if (new_val == sys_pairctrl.def_val) {
                ieee80211_pairing(sys_cfgs.wifi_mode, 0);
            } else {
                ieee80211_pairing(sys_cfgs.wifi_mode, 1);
            }
            sys_pairctrl.last_val = new_val;
        }
    }
    os_run_work_delay(&pairkey_wk, 100);
    return 0;
}

int32 sys_wifi_event_hdl_pairled(uint8 ifidx, uint16 evt, uint32 param1, uint32 param2)
{
    int32 ret = 0;
    switch (evt) {
        case IEEE80211_EVENT_CONNECTED:
            if (!sys_ledctrl.pair) {
                sys_ledctrl.conn  = 1;
                sys_ledctrl.rssi1 = 1;
                sys_ledctrl.rssi2 = 1;
                sys_ledctrl.rssi3 = 1;
            }
            break;
        case IEEE80211_EVENT_DISCONNECTED:
            if (ifidx == WIFI_MODE_STA && !sys_ledctrl.pair) {
                sys_ledctrl.conn  = 0;
                sys_ledctrl.rssi1 = 0;
                sys_ledctrl.rssi2 = 0;
                sys_ledctrl.rssi3 = 0;
            }
            break;
        case IEEE80211_EVENT_RSSI:
            if (ifidx == WIFI_MODE_STA && !sys_ledctrl.pair) {
                if ((int8)param1 > -48 && sys_ledctrl.conn) {
                    sys_ledctrl.rssi1 = 1;
                    sys_ledctrl.rssi2 = 1;
                    sys_ledctrl.rssi3 = 1;
                } else if ((int8)param1 > -60 && sys_ledctrl.conn) {
                    sys_ledctrl.rssi1 = 1;
                    sys_ledctrl.rssi2 = 1;
                    sys_ledctrl.rssi3 = 0;
                } else {
                    sys_ledctrl.rssi1 = 0;
                    sys_ledctrl.rssi2 = 0;
                    sys_ledctrl.rssi3 = 0;
                }
            }
            break;
        case IEEE80211_EVENT_INTERFACE_DISABLE:
            sys_ledctrl.conn  = 0;
            sys_ledctrl.rssi1 = 0;
            sys_ledctrl.rssi2 = 0;
            sys_ledctrl.rssi3 = 0;
            break;
        case IEEE80211_EVENT_INTERFACE_ENABLE:
            //rssi leds show all if mode is AP
            if (ifidx == WIFI_MODE_AP) {
                sys_ledctrl.conn  = 1;
                sys_ledctrl.rssi1 = 1;
                sys_ledctrl.rssi2 = 1;
                sys_ledctrl.rssi3 = 1;
            }
            break;
        //rate of led overturn determined "sys_ledctrl.wk_delay" changes slowly,
        case IEEE80211_EVENT_PAIR_START:
            sys_ledctrl.pair  = 1;
            sys_ledctrl.conn  = 0;
            sys_ledctrl.rssi1 = 0;
            sys_ledctrl.rssi2 = 0;
            sys_ledctrl.rssi3 = 0;
            sys_ledctrl.wk_delay = 255;
            break;
        //wait for connect done, even though in pairkey lock state.
        //rate of led overturn determined "sys_ledctrl.wk_delay" changes fastly,
        case IEEE80211_EVENT_PAIR_SUCCESS:
            sys_ledctrl.wk_delay = 80;
            break;
        //wait for pairkey unlock
        case IEEE80211_EVENT_PAIR_DONE:
            if (ifidx == WIFI_MODE_AP) {
                sys_ledctrl.conn  = 1;
                sys_ledctrl.rssi1 = 1;
                sys_ledctrl.rssi2 = 1;
                sys_ledctrl.rssi3 = 1;
            } else {
                sys_ledctrl.conn  = 0;
                sys_ledctrl.rssi1 = 0;
                sys_ledctrl.rssi2 = 0;
                sys_ledctrl.rssi3 = 0;
            }

            sys_ledctrl.pair = 0;
            sys_ledctrl.wk_delay = 100;
            break;
        default:
            break;
    }
    return ret;
}

void sys_wifi_pair_led_init(void)
{
    OS_WORK_INIT(&ledctrl_wk, sys_pairled_work, 20);
    OS_WORK_INIT(&pairkey_wk, sys_pairkey_work, 0);
    os_run_work_delay(&ledctrl_wk, 100);
    os_run_work_delay(&pairkey_wk, 100);
}

#endif

