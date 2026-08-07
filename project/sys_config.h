#ifndef __SDK_SYS_CONFIG_H__
#define __SDK_SYS_CONFIG_H__
#include "project_config.h"

#define PROJECT_TYPE                    PRO_TYPE_FPV
#define SYS_CACHE_ENABLE                1
#define SYSCFG_ENABLE                   1

//#define OHOS          //使用鸿蒙LiteOS，需要CDK排除csky目录: 右键Exclude
#define CSKY_OS     //使用 AliOS，需要CDK排除ohos目录: 右键Exclude

#define __ram                           __at_section(".ram.text")
#define __psram_data                    __at_section(".psram.data")

#define SRAM_POOL_START                 (srampool_start)
#define SRAM_POOL_SIZE                  (srampool_end - srampool_start)

#define PSRAM_POOL_START                 (psrampool_start)
#define PSRAM_POOL_SIZE                  (psrampool_end - psrampool_start)

#define CPURPC_TASK_STACKSIZE           1024

#ifndef CONFIG_CORE_CPU_CLK
#define CONFIG_CORE_CPU_CLK             DEFAULT_SYS_CLK
#endif

#ifndef CONFI_CORE_UARTDEV
#define CONFI_CORE_UARTDEV              UART0_BASE
#endif

#ifndef CONFI_CORE_M2M_DMA
#define CONFI_CORE_M2M_DMA              M2M_DMA2_BASE
#endif

#ifndef CONFI_CORE_DCACHE_MAINT_EN
#define CONFI_CORE_DCACHE_MAINT_EN      1
#endif

/////////////////////////////////////////////////////////////////////////////
// SRAM Layout :
// --------------------------------------------------------------------------
// |..core heap..|..core rxbuff..|..av heap ..|..sys heap..|
// --------------------------------------------------------------------------
#ifndef CONFIG_CORE_HEAP_START
#define CONFIG_CORE_HEAP_START          SRAM_POOL_START
#endif
#ifndef CONFIG_CORE_HEAP_SIZE
#define CONFIG_CORE_HEAP_SIZE          (40*1024)
#endif

#ifndef CONFIG_CORE_RXBUF_ADDR
#define CONFIG_CORE_RXBUF_ADDR         (CONFIG_CORE_HEAP_START+CONFIG_CORE_HEAP_SIZE)
#endif
#ifndef CONFIG_CORE_RXBUF_SIZE
#define CONFIG_CORE_RXBUF_SIZE         (10*1024)
#endif



#define SYS_HEAP_START                 (CONFIG_CORE_RXBUF_ADDR+CONFIG_CORE_RXBUF_SIZE)
#ifndef SYS_HEAP_SIZE
#define  SYS_HEAP_SIZE                 (SRAM_POOL_SIZE - (SYS_HEAP_START-SRAM_POOL_START) - CONFIG_AVHEAP_SIZE)
#endif

#define CONFIG_AVHEAP_START            (SYS_HEAP_START+SYS_HEAP_SIZE)//(CONFIG_CORE_RXBUF_ADDR+CONFIG_CORE_RXBUF_SIZE)//
#ifndef CONFIG_AVHEAP_SIZE
#define CONFIG_AVHEAP_SIZE             (0)
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// PSRAM Layout :
// --------------------------------------------------------------------------
// |..core skbpool..|.. psram avheap ..|.. psram heap ..| ....                               |
// --------------------------------------------------------------------------


#ifndef CONFIG_CORE_SKB_POOL_ADDR
#define CONFIG_CORE_SKB_POOL_ADDR      (PSRAM_POOL_START)//0x28000000//
#endif
#ifndef CONFIG_CORE_SKB_POOL_SIZE
#define CONFIG_CORE_SKB_POOL_SIZE      (200*1024)//0x100000//
#endif

#ifndef CONFIG_PSRAM_AVHEAP_START
#define CONFIG_PSRAM_AVHEAP_START       (CONFIG_CORE_SKB_POOL_ADDR+CONFIG_CORE_SKB_POOL_SIZE)
#endif

#ifndef CONFIG_PSRAM_AVHEAP_SIZE
#define CONFIG_PSRAM_AVHEAP_SIZE       (0)
#endif

#define SYS_PSRAM_HEAP_START           (CONFIG_PSRAM_AVHEAP_START+CONFIG_PSRAM_AVHEAP_SIZE)
#ifndef SYS_PSRAM_HEAP_SIZE
#define SYS_PSRAM_HEAP_SIZE            (PSRAM_POOL_SIZE-(SYS_PSRAM_HEAP_START-PSRAM_POOL_START))
#endif
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#ifndef TXWSDK_POSIX
#define TXWSDK_POSIX
#endif

#ifndef OS_SYSTICK_HZ
#define OS_SYSTICK_HZ                  (1000)
#endif

#ifndef OS_IRQ_STACK_SIZE
#define OS_IRQ_STACK_SIZE              (1024)
#endif

#ifndef OS_IDLE_TASK_STACK
#define OS_IDLE_TASK_STACK             (64*3)
#endif

#ifndef OS_TIMER_TASK_STACK_SIZE
#define OS_TIMER_TASK_STACK_SIZE       (128) //=128*4
#endif

#ifndef OS_TIMER_MSG_NUM
#define OS_TIMER_MSG_NUM               (10)
#endif

#ifndef DEFAULT_SYS_CLK
#define DEFAULT_SYS_CLK                 60000000UL
#endif

#ifndef ATCMD_UARTDEV
#define ATCMD_UARTDEV                   HG_UART0_DEVID
#endif

#ifndef WIFI_SSID_PREFIX
#define WIFI_SSID_PREFIX                "82X_"
#endif

#ifndef WIFI_PASSWD_DEFAULT
#define WIFI_PASSWD_DEFAULT             "12345678"
#endif

#ifndef WIFI_MODE_DEFAULT
#define WIFI_MODE_DEFAULT               WIFI_MODE_AP
#endif

#ifndef WIFI_HWMODE_DEFAULT
#define WIFI_HWMODE_DEFAULT             IEEE80211_HWMODE_11N
#endif

#ifndef WIFI_CHANNEL_DEFAULT
#define WIFI_CHANNEL_DEFAULT            0
#endif

#ifndef BEACON_INTVAL_DEFAULT
#define BEACON_INTVAL_DEFAULT           100 //单位：毫秒
#endif

#ifndef DTIM_PERIOD_DEFAULT
#define DTIM_PERIOD_DEFAULT             10 //单位：beacon个数
#endif

#ifndef BSS_MAX_IDLE_DEFAULT
#define BSS_MAX_IDLE_DEFAULT            300 //单位：秒
#endif

#ifndef KEY_MGMT_DEFAULT
#define KEY_MGMT_DEFAULT                WPA_KEY_MGMT_PSK
#endif

#ifndef WIFI_BSSBW_DEFAULT
#define WIFI_BSSBW_DEFAULT              20
#endif

#ifndef NET_IP_ADDR_DEFAULT
#define NET_IP_ADDR_DEFAULT             0x0101A8C0  //192.168.1.1
#endif

#ifndef NET_MASK_DEFAULT
#define NET_MASK_DEFAULT                0x00FFFFFF  //255.255.255.0
#endif

#ifndef NET_GW_IP_DEFAULT
#define NET_GW_IP_DEFAULT               0x0101A8C0  //192.168.1.1
#endif

#ifndef DHCPD_START_IP_DEFAULT
#define DHCPD_START_IP_DEFAULT          0x6401A8C0  //192.168.1.100
#endif

#ifndef DHCPD_END_IP_DEFAULT
#define DHCPD_END_IP_DEFAULT            0xFE01A8C0  //192.168.1.254
#endif

#ifndef DHCPD_LEASETIME_DEFAULT
#define DHCPD_LEASETIME_DEFAULT         7200 //单位: 秒
#endif

#ifndef DHCPD_DNS1_DEFAULT
#define DHCPD_DNS1_DEFAULT              0x0101A8C0  //192.168.1.1
#endif

#ifndef DHCPD_DNS2_DEFAULT
#define DHCPD_DNS2_DEFAULT              0x0101A8C0  //192.168.1.1
#endif

#ifndef DHCPD_ROUTER_DEFAULT
#define DHCPD_ROUTER_DEFAULT            0x0101A8C0  //192.168.1.1
#endif

#ifndef DHCPD_EN
#define DHCPD_EN                       (1)
#endif

#ifndef DHCPC_EN
#define DHCPC_EN                       (1)
#endif

#ifndef WIFI_RTS_THRESHOLD
#define WIFI_RTS_THRESHOLD              -1
#endif

#ifndef WIFI_RTS_MAX_RETRY
#define WIFI_RTS_MAX_RETRY              2
#endif

#ifndef WIFI_TX_MAX_RETRY
#define WIFI_TX_MAX_RETRY               15
#endif

#ifndef WIFI_TX_SUPP_RATE
#define WIFI_TX_SUPP_RATE               0x0FFFFF    //TX速率支持，每1bit对应一种速率
#endif

#ifndef WIFI_MULICAST_RETRY
#define WIFI_MULICAST_RETRY             0           //组播帧传输次数
#endif

#ifndef WIFI_ACS_CHAN_LISTS
#define WIFI_ACS_CHAN_LISTS             0x03FF      //要扫描的信道。每1bit对应1个信道(bit 0~11 -> chan 1~12)
#endif

#ifndef WIFI_ACS_SCAN_TIME
#define WIFI_ACS_SCAN_TIME              150         //每个信道的扫描时间，单位ms
#endif

#ifndef WIFI_TX_DUTY_CYCLE
#define WIFI_TX_DUTY_CYCLE              100         //tx发送占空比，单位是%，范围是0~100
#endif

#ifndef WIFI_SSID_FILTER_EN
#define WIFI_SSID_FILTER_EN             0           //是否使能SSID过滤功能。使能后，只有隐藏SSID和指定SSID的beacon才会上传
#endif

#ifndef WIFI_PREVENT_PS_MODE_EN
#define WIFI_PREVENT_PS_MODE_EN         0           //是否尽可能的阻止sta进入休眠
#endif

#ifndef WIFI_FEM_CHIP
#define WIFI_FEM_CHIP                   LMAC_FEM_NONE   //FEM芯片类型。LMAC_FEM_NONE以外的值会进行对应的FEM初始化
#endif

#ifndef WIFI_FREQ_OFFSET_TRACK_MODE
#define WIFI_FREQ_OFFSET_TRACK_MODE     LMAC_FREQ_OFFSET_TRACK_ALWAYS_ON    //默认一直打开频偏跟踪功能
#endif

#ifndef WIFI_TEMPERATURE_COMPESATE_EN
#define WIFI_TEMPERATURE_COMPESATE_EN   1           //是否使能温度补偿
#endif

#ifndef WIFI_PS_NO_FRM_LOSS_EN
#define WIFI_PS_NO_FRM_LOSS_EN          0           //tx缓存的休眠帧是否不允许丢弃
#endif

#ifndef WIFI_RF_PWR_LEVEL
#define WIFI_RF_PWR_LEVEL               0
#endif

#ifndef WIFI_SINGLE_DEV
#define WIFI_SINGLE_DEV                 1
#endif

#ifndef SYS_NETWORK_SUPPORT
#define SYS_NETWORK_SUPPORT             1
#endif

#ifndef WIFI_AP_SUPPORT
#define WIFI_AP_SUPPORT                 1
#endif

#ifndef WIFI_STA_SUPPORT
#define WIFI_STA_SUPPORT                1
#endif

#ifndef SYS_APP_DHCPD
#define SYS_APP_DHCPD                   1
#endif

#ifndef SYS_APP_SNTP
#define SYS_APP_SNTP                    0
#endif

#ifndef SYS_APP_UHTTPD
#define SYS_APP_UHTTPD                  0
#endif

#ifndef SYS_APP_BLENC
#define SYS_APP_BLENC                   0
#endif

#ifndef SYS_IOT_ATCMD
#define SYS_IOT_ATCMD                   0
#endif

#ifndef SYS_DISABLE_PRINT
#define SYS_DISABLE_PRINT               0
#endif

#ifndef ISP_SUPPORT_SENSOR_MAX_NUM
#define ISP_SUPPORT_SENSOR_MAX_NUM      3
#endif

#ifndef ISP_SENSOR_REG_MAX_LEN
#define ISP_SENSOR_REG_MAX_LEN         (100)
#endif

#ifndef ISP_AE_CROP_ZONE_NUM
#define ISP_AE_CROP_ZONE_NUM (5)
#endif

#ifndef DUAL_EN
#define DUAL_EN (0)
#endif

#ifndef SD_MODE_TYPE
#define SD_MODE_TYPE (2)
#endif

#ifndef UBLE_UUID_128_SUPPORT
#define UBLE_UUID_128_SUPPORT           1
#endif

#ifdef PSRAM_HEAP
#define TCPIP_MBOX_SIZE                 128
#define DEFAULT_UDP_RECVMBOX_SIZE       64
#define DEFAULT_TCP_RECVMBOX_SIZE       64
#define DEFAULT_ACCEPTMBOX_SIZE         16

//#define MEM_LIBC_MALLOC 1
//#define MEMP_MEM_MALLOC 1 
#define MEM_SIZE                        80*1024
#define MEMP_NUM_PBUF                   40
#define MEMP_NUM_NETCONN                16
#define MEMP_NUM_NETBUF                 64
#define MEMP_NUM_UDP_PCB                8
#define MEMP_NUM_TCP_PCB                16
#define MEMP_NUM_TCP_SEG                320
#define PBUF_POOL_SIZE                  80

#define TCP_SND_BUF                    (40 * TCP_MSS)
#define TCP_WND                        (40 * TCP_MSS)
#define TCP_TMR_INTERVAL                50
#endif

#ifndef VIDEO_YUV_RANGE_TYPE
#define VIDEO_YUV_RANGE_TYPE            (1)
#endif

#define LWIP_DHCP_DOES_ACD_CHECK 0  //关闭lwip的acd模块功能
#endif

