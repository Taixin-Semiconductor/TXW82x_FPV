#ifndef __SDK_SYS_CONFIG_H__
#define __SDK_SYS_CONFIG_H__

// ============================================================================
// sys_config.h提供了SDK各个宏定义 的默认配置，优先使用 projec_config.h 的 配置定义。
// 需要修改某个宏定义时 应该在 project_config.h里面进行重定义
// ============================================================================
#include "project_config.h"

#define PROJECT_TYPE                    PRO_TYPE_FPV
#define SYS_CACHE_ENABLE                1
#define SYSCFG_ENABLE                   1

// 标记需要放在SRAM的代码段
#define __ram                           __at_section(".ram.text")

// 标记需要放在PSRAM的数据段
#define __psram_data                    __at_section(".psram.data")

// ============================================================================
// Core CPU(CPU1)的配置
// ============================================================================
// Core CPU(CPU1)的默认时钟频率
#ifndef CONFIG_CORE_CPU_CLK
#define CONFIG_CORE_CPU_CLK             DEFAULT_SYS_CLK
#endif

// Core CPU(CPU1)使用的uart
#ifndef CONFI_CORE_UARTDEV
#define CONFI_CORE_UARTDEV              UART0_BASE
#endif

// Core CPU(CPU1)使用的M2M DMA
#ifndef CONFI_CORE_M2M_DMA
#define CONFI_CORE_M2M_DMA              M2M_DMA2_BASE
#endif

#ifndef CONFI_CORE_DCACHE_MAINT_EN
#define CONFI_CORE_DCACHE_MAINT_EN      1
#endif

// Core CPU(CPU1)的 Heap 大小. 【默认从 SRAM内存池 中分配】
#ifndef CONFIG_CORE_HEAP_SIZE
#define CONFIG_CORE_HEAP_SIZE          (40*1024)
#endif

// Core CPU(CPU1)的 LMAC协议栈 RX Buffer Size.【默认从 SRAM内存池 中分配】
#ifndef CONFIG_CORE_RXBUF_SIZE
#define CONFIG_CORE_RXBUF_SIZE         (10*1024)
#endif

// Core CPU(CPU1)的 WiFi协议栈Buffer Size. 【默认从 PSRAM内存池 中分配】
#ifndef CONFIG_CORE_SKB_POOL_SIZE
#define CONFIG_CORE_SKB_POOL_SIZE      200*1024//0x100000//
#endif

// ============================================================================
// SDK基础配置
// ============================================================================

// SRAM 总的可用地址和大小（减去 data段和bss段 后的剩余空间）
#define SRAM_POOL_START                 (srampool_start)
#define SRAM_POOL_SIZE                  (srampool_end - srampool_start)

// PSRAM 总的可用地址和大小
#define PSRAM_POOL_START                (psrampool_start)
#define PSRAM_POOL_SIZE                 (psrampool_end - psrampool_start)

// 应用CPU(CPU0)的 SRAM内存池 起始地址
// SRAM内存池 作为默认的分配空间，默认的 malloc/zalloc/os_malloc/... 都从 SRAM内存池 分配
#define SYS_HEAP_START                 (SRAM_POOL_START)
// 应用CPU(CPU0)的 SRAM内存池 默认Size
#ifndef SYS_HEAP_SIZE
#define  SYS_HEAP_SIZE                 (SRAM_POOL_SIZE)
#endif

// 应用CPU(CPU0)的 PSRAM内存池 起始地址
// PSRAM内存池 作为特殊分配空间，使用 os_malloc_psram 时从 PSRAM内存池 分配
#define SYS_PSRAM_HEAP_START           (PSRAM_POOL_START)
// 应用CPU(CPU0)的 PSRAM内存池 默认Size
#ifndef SYS_PSRAM_HEAP_SIZE
#define SYS_PSRAM_HEAP_SIZE            (PSRAM_POOL_SIZE)
#endif

// SDK系统滴答定时器频率 (Hz)：1000Hz 即 1ms 节拍
#ifndef OS_SYSTICK_HZ
#define OS_SYSTICK_HZ                  (1000)
#endif

// SDK中断服务程序 (IRQ) 栈大小：1024
#ifndef OS_IRQ_STACK_SIZE
#define OS_IRQ_STACK_SIZE              (1024)
#endif

// SDK空闲任务 (Idle Task) 栈大小：64 * 3 = 192
#ifndef OS_IDLE_TASK_STACK
#define OS_IDLE_TASK_STACK             (64*3)
#endif

// SDK RTOS定时器任务栈大小：256
#ifndef OS_TIMER_TASK_STACK_SIZE
#define OS_TIMER_TASK_STACK_SIZE       (256) //实际大小是：256*4
#endif

// SDK双核通信Task的堆栈大小
#define CPURPC_TASK_STACKSIZE           1024

// SDK RTOS定时器消息队列长度：10 条消息
#ifndef OS_TIMER_MSG_NUM
#define OS_TIMER_MSG_NUM               (10)
#endif

// 默认系统时钟：60 MHz
#ifndef DEFAULT_SYS_CLK
#define DEFAULT_SYS_CLK                 60000000UL
#endif

// 启用TXSDK 提供的 Posix 封装API
#ifndef TXWSDK_POSIX
#define TXWSDK_POSIX
#endif

// AT 命令使用的 UART 设备 ID
#ifndef ATCMD_UARTDEV
#define ATCMD_UARTDEV                   HG_UART0_DEVID
#endif

// 是否 默认关闭打印输出 -- 开机后无任何打印
#ifndef SYS_DISABLE_PRINT
#define SYS_DISABLE_PRINT               0
#endif

// ============================================================================
// WiFi 基础默认配置 (WiFi Basic Defaults)
// ============================================================================

// WiFi SSID 前缀 (热点名称将以 "82X_" 开头)
#ifndef WIFI_SSID_PREFIX
#define WIFI_SSID_PREFIX                "82X_"
#endif

// WiFi 默认密码
#ifndef WIFI_PASSWD_DEFAULT
#define WIFI_PASSWD_DEFAULT             "12345678"
#endif

// WiFi 默认工作模式：AP模式
#ifndef WIFI_MODE_DEFAULT
#define WIFI_MODE_DEFAULT               WIFI_MODE_AP
#endif

// WiFi 默认模式：802.11n
#ifndef WIFI_HWMODE_DEFAULT
#define WIFI_HWMODE_DEFAULT             IEEE80211_HWMODE_11N
#endif

// WiFi 默认信道：0，表示自动选择信道
#ifndef WIFI_CHANNEL_DEFAULT
#define WIFI_CHANNEL_DEFAULT            0
#endif

// WiFi 默认Beacon周期：100
#ifndef BEACON_INTVAL_DEFAULT
#define BEACON_INTVAL_DEFAULT           100 //单位：毫秒
#endif

// WiFi 默认DTIM周期：10 -- 休眠STA每隔10个Beacon周期醒来一次
#ifndef DTIM_PERIOD_DEFAULT
#define DTIM_PERIOD_DEFAULT             10
#endif

// WiFi 默认BSS最大空闲时间：300 秒 (超时可能断开连接)
#ifndef BSS_MAX_IDLE_DEFAULT
#define BSS_MAX_IDLE_DEFAULT            300 //单位：秒
#endif

// WiFi 默认密钥管理方式：WPA-PSK
#ifndef KEY_MGMT_DEFAULT
#define KEY_MGMT_DEFAULT                WPA_KEY_MGMT_PSK
#endif

// WiFi 默认BSS带宽：20M
#ifndef WIFI_BSSBW_DEFAULT
#define WIFI_BSSBW_DEFAULT              20
#endif

// ============================================================================
// WiFi 高级特性配置 (WiFi Advanced Features)
// ============================================================================

// RTS 阈值：-1 (禁用 RTS/CTS 握手)
#ifndef WIFI_RTS_THRESHOLD
#define WIFI_RTS_THRESHOLD              -1
#endif

// RTS 最大重试次数
#ifndef WIFI_RTS_MAX_RETRY
#define WIFI_RTS_MAX_RETRY              2
#endif

// TX (发送) 最大重试次数
#ifndef WIFI_TX_MAX_RETRY
#define WIFI_TX_MAX_RETRY               15
#endif

//TX速率支持，每1bit对应一种速率
#ifndef WIFI_TX_SUPP_RATE
#define WIFI_TX_SUPP_RATE               0x0FFFFF
#endif

//组播帧传输次数
#ifndef WIFI_MULICAST_RETRY
#define WIFI_MULICAST_RETRY             0
#endif

//自动选择信道时扫描的信道。每1bit对应1个信道(bit 0~11 -> chan 1~12)
#ifndef WIFI_ACS_CHAN_LISTS
#define WIFI_ACS_CHAN_LISTS             0x03FF
#endif

//每个信道的扫描时间，单位ms
#ifndef WIFI_ACS_SCAN_TIME
#define WIFI_ACS_SCAN_TIME              150
#endif

//tx发送占空比，单位是%，范围是0~100
#ifndef WIFI_TX_DUTY_CYCLE
#define WIFI_TX_DUTY_CYCLE              100
#endif

//是否使能SSID过滤功能。使能后，只有隐藏SSID和指定SSID的beacon才会上传
#ifndef WIFI_SSID_FILTER_EN
#define WIFI_SSID_FILTER_EN             0
#endif

//是否尽可能的阻止sta进入休眠
#ifndef WIFI_PREVENT_PS_MODE_EN
#define WIFI_PREVENT_PS_MODE_EN         0
#endif

//FEM芯片类型。LMAC_FEM_NONE以外的值会进行对应的FEM初始化
#ifndef WIFI_FEM_CHIP
#define WIFI_FEM_CHIP                   LMAC_FEM_NONE
#endif

//频偏跟踪功能：默认打开
#ifndef WIFI_FREQ_OFFSET_TRACK_MODE
#define WIFI_FREQ_OFFSET_TRACK_MODE     LMAC_FREQ_OFFSET_TRACK_ALWAYS_ON
#endif

//是否使能温度补偿
#ifndef WIFI_TEMPERATURE_COMPESATE_EN
#define WIFI_TEMPERATURE_COMPESATE_EN   1
#endif

//缓存的TX休眠帧是否不允许丢弃
#ifndef WIFI_PS_NO_FRM_LOSS_EN
#define WIFI_PS_NO_FRM_LOSS_EN          0
#endif

//是否允许发送聚合。如果对时延要求不高的，可以打开
#ifndef WIFI_TX_AGG_EN
#define WIFI_TX_AGG_EN                  0
#endif

//是否允许接收聚合。CONFIG_CORE_RXBUF_SIZE 小于 18KB 时 不推荐使能
#ifndef WIFI_RX_AGG_EN
#define WIFI_RX_AGG_EN                  0
#endif

//RF TX Power 档位。默认第0档
#ifndef WIFI_RF_PWR_LEVEL
#define WIFI_RF_PWR_LEVEL               0
#endif

//是否使能多MAC地址支持功能
#ifndef WIFI_MODULE_MULTI_MAC_EN
#define WIFI_MODULE_MULTI_MAC_EN        0
#endif

//是否使能重排序模块
#ifndef WIFI_MODULE_RX_REORDER_EN
#define WIFI_MODULE_RX_REORDER_EN       0
#endif

#ifndef CHANNEL_DEFAULT
#define CHANNEL_DEFAULT                 0
#endif

#ifndef SSID_DEFAULT
#define SSID_DEFAULT                    "82X_"
#endif

#ifndef RATE_CONTROL_SELECT
#define RATE_CONTROL_SELECT             3   //默认IPC模式
#endif

// ============================================================================
// 网络默认参数
// ============================================================================

// 默认本机 IP：192.168.1.1
#ifndef NET_IP_ADDR_DEFAULT
#define NET_IP_ADDR_DEFAULT             0x0101A8C0  //192.168.1.1
#endif

// 默认子网掩码：255.255.255.0
#ifndef NET_MASK_DEFAULT
#define NET_MASK_DEFAULT                0x00FFFFFF  //255.255.255.0
#endif

// 默认网关：192.168.1.1
#ifndef NET_GW_IP_DEFAULT
#define NET_GW_IP_DEFAULT               0x0101A8C0  //192.168.1.1
#endif

// DHCP服务器的 起始IP：192.168.1.100
#ifndef DHCPD_START_IP_DEFAULT
#define DHCPD_START_IP_DEFAULT          0x6401A8C0  //192.168.1.100
#endif

// DHCP服务器的 结束IP：192.168.1.254
#ifndef DHCPD_END_IP_DEFAULT
#define DHCPD_END_IP_DEFAULT            0xFE01A8C0  //192.168.1.254
#endif

// DHCP服务器的 租约时间：7200 秒 (2 小时)
#ifndef DHCPD_LEASETIME_DEFAULT
#define DHCPD_LEASETIME_DEFAULT         7200 //单位: 秒
#endif

// DHCP服务器的 DNS服务器1
#ifndef DHCPD_DNS1_DEFAULT
#define DHCPD_DNS1_DEFAULT              0x0101A8C0  //192.168.1.1
#endif

// DHCP服务器的 DNS服务器2
#ifndef DHCPD_DNS2_DEFAULT
#define DHCPD_DNS2_DEFAULT              0x0101A8C0  //192.168.1.1
#endif

// DHCP服务器的 路由器参数
#ifndef DHCPD_ROUTER_DEFAULT
#define DHCPD_ROUTER_DEFAULT            0x0101A8C0  //192.168.1.1
#endif

// 默认是否 启动DHCP服务器
#ifndef DHCPD_EN
#define DHCPD_EN                       (1) //开启
#endif

// 默认是否 启动DHCP客户端
#ifndef DHCPC_EN
#define DHCPC_EN                       (1) //开启
#endif

// ============================================================================
// IPv6 默认配置 (IPv6 Defaults)
// ============================================================================
#ifndef IP6_ADDR_DEFAULT_STR
#define IP6_ADDR_DEFAULT_STR "fe80::1:2:3:4%20"
#endif

#ifndef IP6_GW_DEFAULT_STR
#define IP6_GW_DEFAULT_STR   "fe80::1%20"   // fe80::1%20
#endif

#ifndef IP6_ADDR_LOOPBACK
#define IP6_ADDR_LOOPBACK    "::1"          //::1
#endif

#ifndef IP6_ADDR_ALLROUTERS
#define IP6_ADDR_ALLROUTERS     "ff02::2"
#endif

#ifndef IP6_PREFIX_LENGTH_DEFAULT
#define IP6_PREFIX_LENGTH_DEFAULT 64
#endif

#ifndef DHCP6_START_IP_DEFAULT
#define DHCP6_START_IP_DEFAULT  "fe80::2"  // fe80::2
#endif

#ifndef DHCP6_END_IP_DEFAULT
#define DHCP6_END_IP_DEFAULT    "fe80::fe" // fe80::fe
#endif

// ============================================================================
// SDK功能模块开关
// ============================================================================

// 是否开启 单WiFi设备模式：-- 只注册一个 lwip netif: w0
#ifndef WIFI_SINGLE_DEV
#define WIFI_SINGLE_DEV                 1
#endif

// 是否开启 网络功能
#ifndef SYS_NETWORK_SUPPORT
#define SYS_NETWORK_SUPPORT             1
#endif

// 是否 支持WiFi AP模式
#ifndef WIFI_AP_SUPPORT
#define WIFI_AP_SUPPORT                 1
#endif

// 是否 支持WiFi STA模式
#ifndef WIFI_STA_SUPPORT
#define WIFI_STA_SUPPORT                1
#endif

// 是否 支持WiFi RMESH模式
#ifndef WIFI_RMESH_SUPPORT
#define WIFI_RMESH_SUPPORT              0
#endif

// WiFi RMESH模式支持的节点数量
#ifndef WIFI_RMESH_DEVMAX
#define WIFI_RMESH_DEVMAX               4
#endif

// 是否支持 DHCP服务器 功能
#ifndef SYS_APP_DHCPD
#define SYS_APP_DHCPD                   1
#endif

// 是否支持 NTP客户端 功能
#ifndef SYS_APP_SNTP
#define SYS_APP_SNTP                    0
#endif

// 是否支持 HTTP服务器 功能
#ifndef SYS_APP_UHTTPD
#define SYS_APP_UHTTPD                  0
#endif

// 是否支持 BLE配网 功能
#ifndef SYS_APP_BLENC
#define SYS_APP_BLENC                   0
#endif

// 是否支持 IoT AT命令 功能
#ifndef SYS_IOT_ATCMD
#define SYS_IOT_ATCMD                   0
#endif

// ============================================================================
// ISP (图像信号处理) 配置
// ============================================================================

// 支持的最大的Sensor数量
#ifndef ISP_SUPPORT_SENSOR_MAX_NUM
#define ISP_SUPPORT_SENSOR_MAX_NUM      3
#endif

#ifndef ISP_PLL1_2X_CLK_SEL_VALUE     
#define ISP_PLL1_2X_CLK_SEL_VALUE       6
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

// ============================================================================
// 启用PSRAM后，重定义Lwip的Buffer配置
// ============================================================================
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

#define LWIP_DHCP_DOES_ACD_CHECK 0  //关闭lwip的acd模块功能

#ifndef VIDEO_YUV_RANGE_TYPE
#define VIDEO_YUV_RANGE_TYPE            (1)
#endif

#ifndef FATFS_EN
#define FATFS_EN 1
#endif

#ifndef URLFILE_ENABLE
#define URLFILE_ENABLE 0
#endif


// 应用层的默认宏
#ifndef SUB_STREAM_EN
#define SUB_STREAM_EN 0
#define SUB_STREAM_WIDTH 0
#endif

#ifndef VCAM_EN
#define VCAM_EN  0
#endif

#ifndef VCAM_VOL
#define VCAM_VOL VCAM_VOL_2V80
#endif

#ifndef VCAM2_OC
#define VCAM2_OC VCAM_OC_200MA
#endif

#ifndef VCAM2_EN
#define VCAM2_EN  0
#endif

#ifndef VCAM2_VOL
#define VCAM2_VOL VCC_LDO_VOL_1V80
#endif

#ifndef VCCSD_33
#define VCCSD_33 0
#endif

#ifndef AURPC_PSRAM_HEAP_SIZE
#define AURPC_PSRAM_HEAP_SIZE (30*1024)
#endif

#ifndef STARTUP_OTA
#define STARTUP_OTA   0
#endif

#ifndef LVGL_INPUTDEV_SUPPORT
#define LVGL_INPUTDEV_SUPPORT (0)
#endif

#endif

