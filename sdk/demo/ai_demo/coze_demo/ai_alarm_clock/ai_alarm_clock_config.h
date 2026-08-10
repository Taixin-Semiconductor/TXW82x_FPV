#ifndef __AI_ALARM_CLOCK_DEMO_H__
#define __AI_ALARM_CLOCK_DEMO_H__

#define AI_ALARM_CLOCK_DEMO
#define DEFAULT_SYS_CLK                 (240*1000000)  

/*********************************************************************************
 * 内存配置
 ********************************************************************************/
#define PSRAM_HEAP          
#define AV_PSRAM_HEAP    
#define AV_HEAP
#define CONFIG_PSRAM_AVHEAP_SIZE        (2*1024*1024+512*1024)
#define CONFIG_AVHEAP_SIZE              (40*1024)
//#define MEM_TRACE
#define PIN_FROM_PARAM

/*********************************************************************************
 * WIFI 配置
 ********************************************************************************/
#define SYS_WIFI_PAIR                   1

#define WIFI_MODE_DEFAULT	            WIFI_MODE_STA   // 默认WIFI工作模式 (STA模式)

// 速率控制参数选择
#define RATE_CONTROL_ERSHAO             1           // 耳勺设备
#define RATE_CONTROL_HANGPAI            2           // 航拍设备
#define RATE_CONTROL_IPC                3           // 网络摄像头
#define RATE_CONTROL_BABYMPNITOR        4           // 婴儿监视器
#define RATE_CONTROL_SELECT             RATE_CONTROL_BABYMPNITOR

// WIFI TX/RX聚合
#define CONFIG_CORE_RXBUF_SIZE         (18*1024)    //RX聚合需要的RX BUFFER大小
#define WIFI_TX_AGG_EN                  1           //是否允许发送聚合。如果对时延要求不高的，可以打开
#define WIFI_RX_AGG_EN                  1           //是否允许接收聚合。CONFIG_CORE_RXBUF_SIZE 小于 18KB 时 不推荐使能

/*********************************************************************************
 * 网络配置
 ********************************************************************************/
#define DNS_TABLE_SIZE                  10          // DNS缓存表大小
#define SYS_APP_SNTP                    1           // SNTP
#define URLFILE_ENABLE                  1

//#define LWIP_WND_SCALE                  4
//#define TCP_RCV_SCALE                   4

/*********************************************************************************
 * 蓝牙配置
 ********************************************************************************/
#define BLE_SUPPORT                     1           // 使能BLE蓝牙功能
#define BLE_UUID_128                    1           // 使用128位UUID
#define SYS_APP_BLENC                   0           // 1：广播配网（微信小程序），2：BLE配网（需支持共存）

/*********************************************************************************
 * 屏配置
 ********************************************************************************/
#define DMA2D_EN                        1
#define LCD_ST7789_SPI_EN             	1

#define LV_CONF_PATH                    lv_conf_ai_alarm_clock.h

/*********************************************************************************
 * 音频配置
 ********************************************************************************/
#define AURPC_PSRAM_HEAP_SIZE           (30*1024)

#define AUADC_TASK_PRIORITY   			0x50
#define MAX_AUADC_TXBUF                 16
#define AUADC_TIME_INTERVAL             64

/*********************************************************************************
 * 外设配置
 ********************************************************************************/
#define SDH_EN                          0
#define FS_EN                           1

#define FLASHDISK_EN                    1
#define FLASH_FATFS_SIZE                1024*1024
#define ATCMD_UARTDEV                   HG_UART0_DEVID

#define VCAM_EN                        	1
#define VCCSD_33                        1
#define VCAM2_EN                        1
#define VCAM2_VOL                       VCC_LDO_VOL_1V80

/*********************************************************************************
 * TLS 配置
 ********************************************************************************/
#define MBEDTLS_SSL_CIPHERSUITES MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA256, MBEDTLS_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256

/*********************************************************************************
 * AI 方案配置
 ********************************************************************************/
/* 互问ASR */
#define HUWEN_WAKEUP_EN                 0

#define COZE_DEMO                       1
#define COZE_DEMO_GET_EXTERNAL_IP       1   // 获取公网IP，可用于定位

/************************************************************
 // LVGL支持的输入设备类型
************************************************************* */
#define LVGL_INPUTDEV_SUPPORT          (LVGL_INDEY_TOUCHPAD)   


/*************************************************************
 * 支持h264解码:SUPPORT_DECODER_H264
 * 支持jpeg解码:SUPPORT_DECODER_JPEG
 * 支持jpeg编码(通过yuv去编码mjpeg):SUPPORT_ENCODER_JPEG
 * 支持LCD:SUPPORT_LCD(支持video显示到屏)
 * 支持播放器:SUPPORT_TXMPLAYER
************************************************************ */
#define SUPPORT_DECODER_JPEG
#define SUPPORT_ENCODER_JPEG
#define SUPPORT_LCD
#define SUPPORT_TXMPLAYER

/************************************************************
* 音频相关编码解码(参考audio_code_core.h)
*********************************************************** */
#define MP3_DEC_CTRL                    AUCODER_RUN_IN_CPU1
#define OPUS_DEC_CTRL                   AUCODER_RUN_IN_CPU1

#endif  /* __AI_ALARM_CLOCK_DEMO_H__ */