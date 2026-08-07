#ifndef __SDK_PROJECT_CONFIG_H__
#define __SDK_PROJECT_CONFIG_H__


#define CUSTOMER_ID 1

/*
 * CUSTOMER_ID :
 *
 * 1 82xApp_Demo
 * 2 82xApp_ISP_Tunning_Demo
 * 3 82xApp_720P_Demo
 * 4 82xApp_1080P_Demo
 * 5 82xApp_720P_to_1080P_Demo
 * 6 82xApp_UVC_Demo
*/
#if (CUSTOMER_ID == 1)
#define SYS_APP_FPV
#define DEFAULT_SYS_CLK                 (192*1000000) 
#define PSRAM_HEAP          //如果需要psram当作heap,需要打开这个宏
#define AV_PSRAM_HEAP    
#define AV_HEAP
#define CONFIG_PSRAM_AVHEAP_SIZE        (7*1024*1024)
#define CONFIG_AVHEAP_SIZE              (80*1024)
//#define MEM_TRACE
#define PIN_FROM_PARAM

/* ------ USB1.1 ----- */
#define USB11_EN                        1
#define USB11_HOST_EN                   1
/* ------ USB2.0 ----- */
#define USB_EN                          1
#define USB_HOST_EN                     1
#define MACBUS_USB
#define USBDISK                         1   //1代表将sd卡作为u盘   2代表将flash作为usb盘,需要配合USB_EN使用,并且其他宏不能有冲突

/*=========== RTT USB架构宏定义 ==========*/
#define RTT_USB_EN                      1   //RTT USB 架构使能 (USB_EN打开)
#define USB_DETECT_EN                   1   //USB 主从检测使能 (USB_HOST_EN关闭、USB_EN打开)

/*            USB HOST            */

#define RT_USBH                     //RTT USB HOST 使能

//  #define RT_USBH_CDC
 #define RT_USBH_UVC
// #define RT_USBH_UAC
// #define RT_USBH_WIRELESS
// #define RT_USBH_VENDOR_QUECTEL
// #define RT_USBH_VENDOR_CHINAMOBILE
//  #define RT_USBH_MSTORAGE

/*           USB DEVICE           */

// #define RT_USING_USB_DEVICE          //RTT USB DEVICE 使能

// #define RT_USB_DEVICE_COMPOSITE       //USB DEVICE 复合设备使能

// #define RT_USB_DEVICE_CDC
// #define RT_USB_DEVICE_AUDIO_MIC
// #define RT_USB_DEVICE_AUDIO_SPEAKER
// #define RT_USB_DEVICE_MSTORAGE
// #define RT_USB_DEVICE_VIDEO
// #define RT_USB_DEVICE_RNDIS
// #define RT_USB_DEVICE_HID
// #define RT_USB_DEVICE_WINUSB

/*================== end =================*/

/* BLE蓝牙配网模式选择 */
#define BLE_PROV_MODE                   2   // 1：广播配网（微信小程序），2： BLE 配网（需支持共存）
#define WIRELESS_PAIR_CODE              0


#define PRC_EN                          1
#define OF_EN                           0
#define PARA_IN_EN						0
#define DVP_EN                          1
#define MIPI_CSI_EN                     1
#define DUAL_EN                         1
#define VPP_EN                          1
#define ISP_EN                          1
#define FTUSB3_EN                       (0 && ISP_EN)
#define JPG_EN                          1
#define LCD_EN                          1
#define SCALE_EN                        1
#define H264_EN                         1
#define SDH_EN                          1
#define FS_EN                           1

#define VCAM_EN                        (1 || DVP_EN)

#define DMA2D_EN                        1

#define OPENDML_EN                      0
#define UART_FLY_CTRL_EN                0
#define PWM_EN                          0
#define KEY_MODULE_EN                   0
#define TOUCH_PAD_EN                    0
#define FLASHDISK_EN                    0
#define MP3_EN                          0
#define AMR_EN                          0
#define WIFI_PAIR_SUPPORT                        0
#define PRINTER_EN                      0

#define AUDIO_EN                        1

#define RTP_SOUND                       (1&&AUDIO_EN)

#define MJPEG_VIDEO                     (1 &&OPENDML_EN&&FS_EN&&SDH_EN&&JPG_EN)          //基于框架的mjpeg录像    
#define UVC_VIDEO                       (1 &&OPENDML_EN&&FS_EN&&SDH_EN&&USB_EN)          //基于框架的uvc录像

#define VFS_EN                          0           //VFS的文件系统,支持多个文件系统共用同一套接口(littlefs、fatfs)

///////////////wifi parameter////////////
#define WIFI_RF_PWR_LEVEL               0           //选择WIFI功率
#define WIFI_RTS_THRESHOLD              1600        //RTS阈值，2304等效于不用RTS
#define WIFI_RTS_MAX_RETRY              2           //RTS重试次数，范围为2~16
#define WIFI_TX_MAX_RETRY               15          //最大传输次数，范围为1~31
/* 每1bit代表一种速率。每bit代表的格式：
 * bit 0  : DSSS 1M
 * bit 1  : DSSS 2M
 * bit 2  : DSSS 5.5M
 * bit 3  : DSSS 11M
 * bit 4  : NON-HT 6M
 * bit 5  : NON-HT 9M
 * bit 6  : NON-HT 12M
 * bit 7  : NON-HT 18M
 * bit 8  : NON-HT 24M
 * bit 9  : NON-HT 36M
 * bit 10 : NON-HT 48M
 * bit 11 : NON-HT 54M
 * bit 12 : HT MCS0
 * bit 13 : HT MCS1
 * bit 14 : HT MCS2
 * bit 15 : HT MCS3
 * bit 16 : HT MCS4
 * bit 17 : HT MCS5
 * bit 18 : HT MCS6
 * bit 19 : HT MCS7
 */
#define WIFI_TX_SUPP_RATE               0x0FFFFF    //TX速率支持，每1bit对应一种速率
#define WIFI_MULICAST_RETRY             0           //组播帧传输次数
#define WIFI_ACS_CHAN_LISTS             0x1FFF      //要扫描的信道。每1bit对应1个信道(bit 0~11 -> chan 1~12)
#define WIFI_ACS_SCAN_TIME              150         //每个信道的扫描时间，单位ms
#define CHANNEL_DEFAULT                 0
#define SSID_DEFAULT                    "150X1-"   //"150X1-0768c1a3"
#define WIFI_TX_DUTY_CYCLE              100         //tx发送占空比，单位是%，范围是0~100
#define WIFI_SSID_FILTER_EN             0           //是否使能SSID过滤功能。使能后，只有隐藏SSID和指定SSID的beacon才会上传
#define WIFI_PREVENT_PS_MODE_EN         1           //是否尽可能的阻止sta进入休眠
#define WIFI_PS_NO_FRM_LOSS_EN          1           //tx缓存的休眠帧是否不允许丢弃
#define NET_IP_ADDR_DEFAULT             0x01A9A8C0  //192.168.169.1
#define NET_MASK_DEFAULT                0x00FFFFFF  //255.255.255.0
#define NET_GW_IP_DEFAULT               0x01A9A8C0  //192.168.169.1
#define DHCPD_START_IP_DEFAULT          0x64A9A8C0  //192.168.169.100
#define DHCPD_END_IP_DEFAULT            0xFEA9A8C0  //192.168.169.254
#define DHCPD_DNS1_DEFAULT              0x01A9A8C0  //192.168.169.1
#define DHCPD_DNS2_DEFAULT              0x01A9A8C0  //192.168.169.1
#define DHCPD_ROUTER_DEFAULT            0x01A9A8C0  //192.168.169.1


#define ATCMD_UARTDEV       HG_UART0_DEVID







//设定自定义空间,前提heap要足够,main的时候会从heap申请这样一大片空间分配到custom_malloc中
//空间组成大概是  jpg空间+csi空间(VGA 30kB)+ 其他模块的空间
//系统尽量留有20K左右,防止其他地方malloc申请不到

//#define VCAM_33
#define VCCSD_33 0


//#define WIFI_BRIDGE_EN  1
//#define WIFI_BRIDGE_DEV HG_GMAC_DEVID

//速率调整参数选择，默认是耳勺
#define RATE_CONTROL_ERSHAO         1
#define RATE_CONTROL_HANGPAI        2
#define RATE_CONTROL_IPC            3
#define RATE_CONTROL_BABYMPNITOR    4

#define RATE_CONTROL_SELECT     RATE_CONTROL_IPC



//是能5m/20m共存和自动带宽切换，
//#define WIFI_FEM_CHIP     LMAC_FEM_GSR2401C
//#define LMAC_BGN_PCF
            
/***********************************************************
 * BLE
 * ********************************************************/
#define BLE_SUPPORT         1
#define BLE_UUID_128        1

#define DEV_SENSOR_GC1084               1
#define DEV_SENSOR_GC2053               1
#define LCD_ST7701S_MIPI_EN 1

#elif (CUSTOMER_ID == 2)
/***************************************************************
 * 打开PIN_FROM_PARAM,通过脚本和config.cfg去生成对应的io配置信息
 * 请查看重要的文件:pin_param.h、config.cfg两个文件
 *************************************************************/
#define PIN_FROM_PARAM

/*****************************************
 * 打开对应demo的宏
 ****************************************/
#define SYS_APP_ISP_TUNNING

/**********************************************************************
 * 系统必要信息宏
 * CONFIG_PSRAM_AVHEAP_SIZE:为应用分配的psram宏,需要根据应用场景分配
 * PSRAM_HEAP:如果需要用到psram,需要打开PSRAM_HEAP
 ********************************************************************/
#define DEFAULT_SYS_CLK                 (192*1000000) 
#define PSRAM_HEAP          //如果需要psram当作heap,需要打开这个宏
#define AV_PSRAM_HEAP    
#define AV_HEAP
#define CONFIG_PSRAM_AVHEAP_SIZE        (3*1024*1024 + 600*1024)
#define CONFIG_AVHEAP_SIZE              (120*1024)

/*****************************************************************
 * VCAM开关,部分io电源域需要打开才有电
 *****************************************************************/
#define VCAM_EN                         1
#define VCCSD_33                        0

/*******************************************************************
 * 图像编码相关参数
 * 根据sensor的类型打开 DVP_EN 或者是 MIPI_CSI_EN
 * DUAL_EN     : 多个sensor打开，空间不足保持关闭
 * H264_I_ONLY : 1080p 打开
 ******************************************************************/
#define DVP_EN                          0
#define MIPI_CSI_EN                     1
#define DUAL_EN                         1
#define VPP_EN                          1
#define ISP_EN                          1
#define ISP_TUNNING_EN                  1
#define JPG_EN                          1
#define H264_EN                         1
#define H264_I_ONLY                     1

/***************************************************
 * sensor型号配置
#define DEV_SENSOR_OV7725               0
#define DEV_SENSOR_OV7670               0
#define DEV_SENSOR_OV9734				0
#define DEV_SENSOR_GC0308               0
#define DEV_SENSOR_OV2640               0
#define DEV_SENSOR_BF3A03               0
#define DEV_SENSOR_BF2013               0
#define DEV_SENSOR_OV2685               0
#define DEV_SENSOR_BF30A2               0
#define DEV_SENSOR_H62                  0
#define DEV_SENSOR_H63P                 0
#define DEV_SENSOR_SC1346               0
#define DEV_SENSOR_GC1084               0
#define DEV_SENSOR_GC2083               0
#define DEV_SENSOR_GC2053               0
#define DEV_SENSOR_GC20C3               0
#define DEV_SENSOR_SC2336P              0
#define DEV_SENSOR_SC2331               0
#define DEV_SENSOR_F38P                 0
#define DEV_SENSOR_F37P                 0
#define DEV_SENSOR_TP9950               0
 **************************************************/
#define DEV_SENSOR_GC2053               1
#define DEV_SENSOR_GC1084               1

/************************************************************ 
 * mipi相关
INPUT_MODE:
#define YUV422                          0
#define RAW8                            1
#define RAW10                           2
#define RAW12                           3

DOUBLE_LANE ：
1 : 双个数据lane 
0 : 单个数据lane
************************************************************/
#define INPUT_MODE                      2   //RAW10
#define DOUBLE_CSI                      0
#define DOUBLE_LANE                     0

/***********************************************************
 * USB相关宏配置（必须打开）
 ************************************************************/
 /* ------ USB1.1 ----- */
#define USB11_EN                        0
#define USB11_HOST_EN                   0
/* ------ USB2.0 ----- */
#define USB_EN                          1
#define USB_HOST_EN                     0
#define MACBUS_USB
#define USBDISK                         1   //1代表将sd卡作为u盘   2代表将flash作为usb盘,需要配合USB_EN使用,并且其他宏不能有冲突

/*=========== RTT USB架构宏定义 ==========*/
#define RTT_USB_EN                      1   //RTT USB 架构使能 (USB_EN打开)
#define USB_DETECT_EN                   0   //USB 主从检测使能 (USB_HOST_EN关闭、USB_EN打开)

/*           USB DEVICE           */

#define RT_USING_USB_DEVICE          //RTT USB DEVICE 使能

#define RT_USB_DEVICE_COMPOSITE       //USB DEVICE 复合设备使能

#define RT_USB_DEVICE_CDC
#define RT_USB_DEVICE_VIDEO
/***********************************************************
 *默认mjpeg的节点数量,要根据mjpeg启动的分辨率去考虑
 默认节点大小:16*1024
 720P:算mjpeg大小50-80K,给10个节点足够
 1080P:算mjpeg大小100-150K,给20个节点足够
 TARGET_JPG_LEN ： 目标mjpeg的大小
 其他分辨率,根据实际情况去配置
 * ********************************************************/
#define JPG_NODE_COUNT      20
#define TARGET_JPG_LEN      100000   
/*================== end =================*/

#elif (CUSTOMER_ID == 3)
/*****************************************************************************
 * 720P摄像头(帧率主要看摄像头配置表,SDK默认1084 25帧)
 * 主码流(H264):1280x720 @ 25fps
 * 辅码流(H264):640x360  @ 25fps
 * 录卡(MP4):主码流+aac音频(8KHz/16bit)
 * 拍照(MJPG):1280x720
 * 图传:RTSP  辅码流  默认地址:rtsp://ip:554/h264?1
 * 回放:录风者采用下载模式去解码mp4回放
 * 文件系统:FAT32、EXFAT(配合ffconf.h设置)
 ****************************************************************************/

/***************************************************************
 * 打开PIN_FROM_PARAM,通过脚本和config.cfg去生成对应的io配置信息
 * 请查看重要的文件:pin_param.h、config.cfg两个文件
 *************************************************************/
#define PIN_FROM_PARAM
/*****************************************
 * 打开对应demo的宏
 ****************************************/
#define SYS_APP_FPV

/**********************************************************************
 * 系统必要信息宏
 * CONFIG_PSRAM_AVHEAP_SIZE:为应用分配的psram宏,需要根据应用场景分配
 * PSRAM_HEAP:如果需要用到psram,需要打开PSRAM_HEAP
 ********************************************************************/
#define DEFAULT_SYS_CLK                 (192*1000000) 
#define PSRAM_HEAP          //如果需要psram当作heap,需要打开这个宏
#define AV_PSRAM_HEAP    
#define AV_HEAP


/*******************************************************************************************************************************************
1080P摄像头空间分配(如果没有其他要求,会将大部分空间给到视频)
H264：  I/P帧:3.1M+0.5M(默认,根据I帧P帧最大值进行调整)
		纯I帧:0.5M
		h264数据缓冲size: 64帧(最大缓冲)*100K(平局每一帧size) = 6M左右  (如果终端应用不会卡太久,这里可以分配2M-3M左右,这个是动态的)
		(64帧是最大缓冲数量,100K需要根据自己h264配置调整,缓冲帧数大是解决录卡不会轻易丢帧,参数都是可以调整,根据产品来调整)

MJPG(1路):  MJPG:  10-30(mjpg节点)*16K = 160K-480K  (不同分辨率以及质量需要调整,如果是大分辨率拍照4K、8K,需要1M

所以这里需要空间粗略计算就是 3.1M+0.5M+0.5M + 3M(h264+mjpg数据共用动态) ≈ 7M

由于MJPG与H264共用,会造成一定碎片化,可以适当将空间h264和MJPG独立分开,默认SDK没有分开
*********************************************************************************************************************************************/
#define CONFIG_PSRAM_AVHEAP_SIZE        (7*1024*1024+512*1024)

/**********************************************************************************************************************************************
 * 不同镜头以及功能不一样
 * 没有考虑大分辨拍照模式
 * 720P镜头:  60K(VPP_DATA0)(1080P h264) = 60k
			  辅码流:  12K(gen420) = 12K
			  其他:  10K(解码) + 其他(某些结构体用到) ≈ 15K
				
    total:  90K(没有额外功能,可以运行,如果空间足够,尽量到100K)
	
				
 
 *********************************************************************************************************************************************/
#define CONFIG_AVHEAP_SIZE              (90*1024)

/******************************************************************************
 * wifi的必要参数配置
 ******************************************************************************/
///////////////wifi parameter////////////
#define WIFI_RF_PWR_LEVEL               0           //选择WIFI功率。0：普通功率；1：比0的功率小；2：比1更小；10：大电流
#define WIFI_RTS_THRESHOLD              1600        //RTS阈值，2304等效于不用RTS
#define WIFI_RTS_MAX_RETRY              2           //RTS重试次数，范围为2~16
#define WIFI_TX_MAX_RETRY               15          //最大传输次数，范围为1~31
#define WIFI_TX_MAX_POWER               7           //TX最大发射功率，有0~7档

#define WIFI_TX_SUPP_RATE               0x0FFFFF    //TX速率支持，每1bit对应一种速率
#define WIFI_MAX_STA_CNT                8           //最多连接sta的数量。有效值为1~8
#define WIFI_MULICAST_RETRY             0           //组播帧传输次数
#define WIFI_ACS_CHAN_LISTS             0x1FFF      //要扫描的信道。每1bit对应1个信道(bit 0~11 -> chan 1~12)
#define WIFI_ACS_SCAN_TIME              150         //每个信道的扫描时间，单位ms
#define WIFI_MAX_PS_CNT                 30           //底层为休眠sta缓存的帧最大数量。0代表sta休眠由umac全程管理，底层不缓存
#define CHANNEL_DEFAULT                 0
#define SSID_DEFAULT                    "150X1-"   //"150X1-0768c1a3"
#define WIFI_TX_DUTY_CYCLE              100         //tx发送占空比，单位是%，范围是0~100
#define WIFI_SSID_FILTER_EN             0           //是否使能SSID过滤功能。使能后，只有隐藏SSID和指定SSID的beacon才会上传
#define WIFI_PREVENT_PS_MODE_EN         1           //是否尽可能的阻止sta进入休眠
#define NET_IP_ADDR_DEFAULT             0x01A9A8C0  //192.168.169.1
#define NET_MASK_DEFAULT                0x00FFFFFF  //255.255.255.0
#define NET_GW_IP_DEFAULT               0x01A9A8C0  //192.168.169.1
#define DHCPD_START_IP_DEFAULT          0x64A9A8C0  //192.168.169.100
#define DHCPD_END_IP_DEFAULT            0xFEA9A8C0  //192.168.169.254
#define DHCPD_DNS1_DEFAULT              0x01A9A8C0  //192.168.169.1
#define DHCPD_DNS2_DEFAULT              0x01A9A8C0  //192.168.169.1
#define DHCPD_ROUTER_DEFAULT            0x01A9A8C0  //192.168.169.1


/***************************************************************
 * 蓝牙
 **************************************************************/
//#define BLE_SUPPORT                 1

/****************************************************************
 * wifi速率配置,根据不同场景需要距离等去配置特定速率算法
#define RATE_CONTROL_ERSHAO         1
#define RATE_CONTROL_HANGPAI        2
#define RATE_CONTROL_IPC            3
#define RATE_CONTROL_BABYMPNITOR    4
 ***************************************************************/
#define RATE_CONTROL_SELECT             3


/*****************************************************************
 * VCAM开关,部分io电源域需要打开才有电
 *****************************************************************/
#define VCAM_EN                         1
#define VCCSD_33                        0

/*******************************************************************
 * 图像编码相关参数
 ******************************************************************/
#define MIPI_CSI_EN                     1
#define VPP_EN                          1
#define H264_EN                         1
#define ISP_EN                          1
#define JPG_EN                          1
#define SCALE_EN                        1

/*******************************************************************
 * 打开拍照模式(紧紧支持录风者模式)
 ******************************************************************/
#define TAKEPHOTO_EN                      1


/*******************************************************************
 * 支持h264的副码流
 ******************************************************************/
#define SUB_STREAM_EN 					1


/*****************************************************************
 * sd使能
 ***************************************************************/
#define SDH_EN                          1
#define FS_EN                           1



/************************************************************************************************************************
 * sensor型号配置,配置多个,支持自动识别(当前SDK只能支持全部单line或者双line,包含单line和双line需要对sdk初始化修改)
#define DEV_SENSOR_OV7725               0
#define DEV_SENSOR_OV7670               0
#define DEV_SENSOR_GC0308               0
#define DEV_SENSOR_OV2640               0
#define DEV_SENSOR_BF3A03               0
#define DEV_SENSOR_BF2013               0
#define DEV_SENSOR_OV2685               0
#define DEV_SENSOR_BF30A2               0
#define DEV_SENSOR_H62                  0
#define DEV_SENSOR_H63P                 0
#define DEV_SENSOR_SC1346               0
#define DEV_SENSOR_GC1084               0
#define DEV_SENSOR_GC2083               0
#define DEV_SENSOR_GC2053               0
#define DEV_SENSOR_SC2336P              0
#define DEV_SENSOR_SC2331               0
#define DEV_SENSOR_F38P                 0
#define DEV_SENSOR_F37P                 0
#define DEV_SENSOR_TP9950               0
 ***************************************************************************************************************************/
//#define DEV_SENSOR_SC1346               1
#define DEV_SENSOR_GC1084               1
#define DEV_SENSOR_GC2053               1



/************************************************************ 
 * mipi相关
INPUT_MODE:
#define YUV422                          0
#define RAW8                            1
#define RAW10                           2
#define RAW12                           3
************************************************************/
#define INPUT_MODE                      2   //RAW10
#define DOUBLE_CSI                      0
#define DOUBLE_LANE                     0


/***********************************************************
 *音频及功放使能io配置
 * ********************************************************/
#define AUDIO_EN                        1

/***********************************************************
 *默认mjpeg的节点数量,要根据mjpeg启动的分辨率去考虑
 默认节点大小:16*1024
 720P:算mjpeg大小50-80K,给10个节点足够
 1080P:算mjpeg大小100-150K,给20个节点足够
 其他分辨率,根据实际情况去配置
 * ********************************************************/
#define JPG_NODE_COUNT 30

/*************************************************************
 * 节省sram内存,将部分模块强制使用psram
 * 优点:节省sram内存
 * 缺点:psram读写慢,可能会影响性能
 
 1080P的sram内存不足,将部分数据放到了psram
 ************************************************************/
#define MORE_SRAM

/*************************************************************
 * 720P摄像头的mjpeg辅码流需要打开VPP_BUF1_EN
 * 720P的辅码流是h264(gen420),注意SUB_STREAM_WIDTH需要是与镜头
   等比例关系,包括是H(sdk默认这个是等比例,所以不需要配置H)
 ************************************************************/
#define VPP_BUF1_EN 1
#define PSRAM_FRAME_SAVE 1
#define SUB_STREAM_WIDTH    640

/************************************************************************
* 配置解码最大的size,如果不需要特殊size解码,这个配置320x180(缩略图用)
* 配置解码缓冲区节点数量(预分配空间,没有解码要求,默认1个节点就够了)
 ************************************************************************/
#define DECODE_MAX_W    320
#define DECODE_MAX_H    180
#define MAX_DECODE_YUV_TX 1

/************************************************************************
* 配置MP4录制最大文件的size
 ************************************************************************/
#define MAX_SINGLE_MP4_SIZE (100 * 1024 * 1024)

/************************************************************************
 * 720P摄像头的H264辅码流
 * 录风者设置图传是H264,则RECORDER_MODE等于0即可
 ************************************************************************/
#define RECORDER_MODE 0

/************************************************************************
* 开启文件系统优化
 ************************************************************************/
#define USE_FAT_CACHE 1

#elif (CUSTOMER_ID == 4)
/*****************************************************************************
 * 1080P摄像头(帧率主要看摄像头配置表,SDK默认2053 25帧)
 * 主码流(H264):1920x1080 @ 25fps
 * 辅码流(MJPG):1280x720  @ 25fps
 * 录卡(MP4):主码流+aac音频(8KHz/16bit)
 * 拍照(MJPG):1280x720
 * 图传:RTSP  辅码流  默认地址:rtsp://ip:554/webcam
 * 回放:录风者采用下载模式去解码mp4回放
 * 文件系统:FAT32、EXFAT(配合ffconf.h设置)
 ****************************************************************************/

/***************************************************************
 * 打开PIN_FROM_PARAM,通过脚本和config.cfg去生成对应的io配置信息
 * 请查看重要的文件:pin_param.h、config.cfg两个文件
 *************************************************************/
#define PIN_FROM_PARAM
/*****************************************
 * 打开对应demo的宏
 ****************************************/
#define SYS_APP_FPV

/**********************************************************************
 * 系统必要信息宏
 * CONFIG_PSRAM_AVHEAP_SIZE:为应用分配的psram宏,需要根据应用场景分配
 * PSRAM_HEAP:如果需要用到psram,需要打开PSRAM_HEAP
 ********************************************************************/
#define DEFAULT_SYS_CLK                 (192*1000000) 
#define PSRAM_HEAP          //如果需要psram当作heap,需要打开这个宏
#define AV_PSRAM_HEAP    
#define AV_HEAP


/*******************************************************************************************************************************************
1080P摄像头空间分配(如果没有其他要求,会将大部分空间给到视频)
H264：  I/P帧:3.1M+0.5M(默认,根据I帧P帧最大值进行调整)
		纯I帧:0.5M
		h264数据缓冲size: 64帧(最大缓冲)*100K(平局每一帧size) = 6M左右  (如果终端应用不会卡太久,这里可以分配2M-3M左右,这个是动态的)
		(64帧是最大缓冲数量,100K需要根据自己h264配置调整,缓冲帧数大是解决录卡不会轻易丢帧,参数都是可以调整,根据产品来调整)

MJPG(1路):  MJPG:  10-30(mjpg节点)*16K = 160K-480K  (不同分辨率以及质量需要调整,如果是大分辨率拍照4K、8K,需要1M

所以这里需要空间粗略计算就是 3.1M+0.5M+0.5M + 3M(h264+mjpg数据共用动态) ≈ 7M

由于MJPG与H264共用,会造成一定碎片化,可以适当将空间h264和MJPG独立分开,默认SDK没有分开
*********************************************************************************************************************************************/
#define CONFIG_PSRAM_AVHEAP_SIZE        (7*1024*1024+512*1024)

/**********************************************************************************************************************************************
 * 不同镜头以及功能不一样
 * 没有考虑大分辨拍照模式
 * 1080P镜头:  90K(VPP_DATA0)(1080P h264) + 52.5K(VPP_DATA1)(720P拍照) ≈ 142.5K
				其他:  12K(gen420)+10K(解码) + 其他(某些结构体用到) ≈ 22K(动态使用,gen420和解码可以分时复用)
				
    total:  165K
				
 
 *********************************************************************************************************************************************/
#define CONFIG_AVHEAP_SIZE              (100*1024 + 65*1024)


/******************************************************************************
 * wifi的必要参数配置
 ******************************************************************************/
///////////////wifi parameter////////////
#define WIFI_RF_PWR_LEVEL               0           //选择WIFI功率。0：普通功率；1：比0的功率小；2：比1更小；10：大电流
#define WIFI_RTS_THRESHOLD              1600        //RTS阈值，2304等效于不用RTS
#define WIFI_RTS_MAX_RETRY              2           //RTS重试次数，范围为2~16
#define WIFI_TX_MAX_RETRY               15          //最大传输次数，范围为1~31
#define WIFI_TX_MAX_POWER               7           //TX最大发射功率，有0~7档

#define WIFI_TX_SUPP_RATE               0x0FFFFF    //TX速率支持，每1bit对应一种速率
#define WIFI_MAX_STA_CNT                8           //最多连接sta的数量。有效值为1~8
#define WIFI_MULICAST_RETRY             0           //组播帧传输次数
#define WIFI_ACS_CHAN_LISTS             0x1FFF      //要扫描的信道。每1bit对应1个信道(bit 0~11 -> chan 1~12)
#define WIFI_ACS_SCAN_TIME              150         //每个信道的扫描时间，单位ms
#define WIFI_MAX_PS_CNT                 30           //底层为休眠sta缓存的帧最大数量。0代表sta休眠由umac全程管理，底层不缓存
#define CHANNEL_DEFAULT                 0
#define SSID_DEFAULT                    "150X1-"   //"150X1-0768c1a3"
#define WIFI_TX_DUTY_CYCLE              100         //tx发送占空比，单位是%，范围是0~100
#define WIFI_SSID_FILTER_EN             0           //是否使能SSID过滤功能。使能后，只有隐藏SSID和指定SSID的beacon才会上传
#define WIFI_PREVENT_PS_MODE_EN         1           //是否尽可能的阻止sta进入休眠
#define NET_IP_ADDR_DEFAULT             0x01A9A8C0  //192.168.169.1
#define NET_MASK_DEFAULT                0x00FFFFFF  //255.255.255.0
#define NET_GW_IP_DEFAULT               0x01A9A8C0  //192.168.169.1
#define DHCPD_START_IP_DEFAULT          0x64A9A8C0  //192.168.169.100
#define DHCPD_END_IP_DEFAULT            0xFEA9A8C0  //192.168.169.254
#define DHCPD_DNS1_DEFAULT              0x01A9A8C0  //192.168.169.1
#define DHCPD_DNS2_DEFAULT              0x01A9A8C0  //192.168.169.1
#define DHCPD_ROUTER_DEFAULT            0x01A9A8C0  //192.168.169.1


/***************************************************************
 * 蓝牙
 **************************************************************/
//#define BLE_SUPPORT                 1

/****************************************************************
 * wifi速率配置,根据不同场景需要距离等去配置特定速率算法
#define RATE_CONTROL_ERSHAO         1
#define RATE_CONTROL_HANGPAI        2
#define RATE_CONTROL_IPC            3
#define RATE_CONTROL_BABYMPNITOR    4
 ***************************************************************/
#define RATE_CONTROL_SELECT             3


/*****************************************************************
 * VCAM开关,部分io电源域需要打开才有电
 *****************************************************************/
#define VCAM_EN                         1
#define VCCSD_33                        0

/*******************************************************************
 * 图像编码相关参数
 ******************************************************************/
#define MIPI_CSI_EN                     1
#define VPP_EN                          1
#define H264_EN                         1
#define ISP_EN                          1
#define JPG_EN                          1
#define SCALE_EN                        1

/*******************************************************************
 * 打开拍照模式(紧紧支持录风者模式)
 ******************************************************************/
#define TAKEPHOTO_EN                      1


/*****************************************************************
 * sd使能
 ***************************************************************/
#define SDH_EN                          1
#define FS_EN                           1



/************************************************************************************************************************
 * sensor型号配置,配置多个,支持自动识别(当前SDK只能支持全部单line或者双line,包含单line和双line需要对sdk初始化修改)
#define DEV_SENSOR_OV7725               0
#define DEV_SENSOR_OV7670               0
#define DEV_SENSOR_GC0308               0
#define DEV_SENSOR_OV2640               0
#define DEV_SENSOR_BF3A03               0
#define DEV_SENSOR_BF2013               0
#define DEV_SENSOR_OV2685               0
#define DEV_SENSOR_BF30A2               0
#define DEV_SENSOR_H62                  0
#define DEV_SENSOR_H63P                 0
#define DEV_SENSOR_SC1346               0
#define DEV_SENSOR_GC1084               0
#define DEV_SENSOR_GC2083               0
#define DEV_SENSOR_GC2053               0
#define DEV_SENSOR_SC2336P              0
#define DEV_SENSOR_SC2331               0
#define DEV_SENSOR_F38P                 0
#define DEV_SENSOR_F37P                 0
#define DEV_SENSOR_TP9950               0
 ***************************************************************************************************************************/
//#define DEV_SENSOR_SC1346               1
#define DEV_SENSOR_GC1084               1
#define DEV_SENSOR_GC2053               1



/************************************************************ 
 * mipi相关
INPUT_MODE:
#define YUV422                          0
#define RAW8                            1
#define RAW10                           2
#define RAW12                           3
************************************************************/
#define INPUT_MODE                      2   //RAW10
#define DOUBLE_CSI                      0
#define DOUBLE_LANE                     0


/***********************************************************
 *音频及功放使能io配置
 * ********************************************************/
#define AUDIO_EN                        1

/***********************************************************
 *默认mjpeg的节点数量,要根据mjpeg启动的分辨率去考虑
 默认节点大小:16*1024
 720P:算mjpeg大小50-80K,给10个节点足够
 1080P:算mjpeg大小100-150K,给20个节点足够
 其他分辨率,根据实际情况去配置
 * ********************************************************/
#define JPG_NODE_COUNT 30


/*************************************************************
 * 节省sram内存,将部分模块强制使用psram
 * 优点:节省sram内存
 * 缺点:psram读写慢,可能会影响性能
 
 1080P的sram内存不足,将部分数据放到了psram
 ************************************************************/
#define MORE_SRAM


/************************************************************************
 * 1080P摄像头的mjpeg辅码流需要打开VPP_BUF1_EN
 * 设置图传辅码流的size(1080P辅码流是mjpg),注意SUB_STREAM_WIDTH需要是与镜头
   等比例关系,包括是H(sdk默认这个是等比例,所以不需要配置H)
 ************************************************************************/
#define VPP_BUF1_EN 1
#define SUB_STREAM_WIDTH    1280

/************************************************************************
* 配置解码最大的size,如果不需要特殊size解码,这个配置320x180(缩略图用)
* 配置解码缓冲区节点数量(预分配空间,没有解码要求,默认1个节点就够了)
 ************************************************************************/
#define DECODE_MAX_W    320
#define DECODE_MAX_H    180
#define MAX_DECODE_YUV_TX 1

/************************************************************************
* 配置MP4录制最大文件的size
 ************************************************************************/
#define MAX_SINGLE_MP4_SIZE (100 * 1024 * 1024)

/************************************************************************
 * 1080P摄像头的mjpeg辅码流
 * 录风者设置图传是mjpeg,则RECORDER_MODE等于1即可
 ************************************************************************/
#define RECORDER_MODE 1


/************************************************************************
* 开启文件系统优化
 ************************************************************************/
#define USE_FAT_CACHE 1

#elif (CUSTOMER_ID == 5)
/*****************************************************************************
 * 720P摄像头,插值到1080P(帧率主要看摄像头配置表,SDK默认1084 25帧)
 * 主码流(H264):1920x1080 @ 25fps
 * 辅码流(H264):640x360   @ 25fps 
 * 录卡(MP4):主码流+aac音频(8KHz/16bit)
 * 拍照(MJPG):1280x720
 * 图传:RTSP  辅码流  默认地址:rtsp://ip:554/h264?1
 * 回放:录风者采用下载模式去解码mp4回放
 * 文件系统:FAT32、EXFAT(配合ffconf.h设置)
 ****************************************************************************/

/***************************************************************
 * 打开PIN_FROM_PARAM,通过脚本和config.cfg去生成对应的io配置信息
 * 请查看重要的文件:pin_param.h、config.cfg两个文件
 *************************************************************/
#define PIN_FROM_PARAM
/*****************************************
 * 打开对应demo的宏
 ****************************************/
#define SYS_APP_FPV

/**********************************************************************
 * 系统必要信息宏
 * CONFIG_PSRAM_AVHEAP_SIZE:为应用分配的psram宏,需要根据应用场景分配
 * PSRAM_HEAP:如果需要用到psram,需要打开PSRAM_HEAP
 ********************************************************************/
#define DEFAULT_SYS_CLK                 (192*1000000) 
#define PSRAM_HEAP          //如果需要psram当作heap,需要打开这个宏
#define AV_PSRAM_HEAP    
#define AV_HEAP


/*******************************************************************************************************************************************
1080P摄像头空间分配(如果没有其他要求,会将大部分空间给到视频)
H264：  I/P帧:3.1M+0.5M(默认,根据I帧P帧最大值进行调整)
		纯I帧:0.5M
		h264数据缓冲size: 64帧(最大缓冲)*100K(平局每一帧size) = 6M左右  (如果终端应用不会卡太久,这里可以分配2M-3M左右,这个是动态的)
		(64帧是最大缓冲数量,100K需要根据自己h264配置调整,缓冲帧数大是解决录卡不会轻易丢帧,参数都是可以调整,根据产品来调整)

MJPG(1路):  MJPG:  10-30(mjpg节点)*16K = 160K-480K  (不同分辨率以及质量需要调整,如果是大分辨率拍照4K、8K,需要1M

所以这里需要空间粗略计算就是 3.1M+0.5M+0.5M + 3M(h264+mjpg数据共用动态) ≈ 7M

由于MJPG与H264共用,会造成一定碎片化,可以适当将空间h264和MJPG独立分开,默认SDK没有分开
*********************************************************************************************************************************************/
#define CONFIG_PSRAM_AVHEAP_SIZE        (7*1024*1024+512*1024)

/**********************************************************************************************************************************************
 * 不同镜头以及功能不一样
 * 没有考虑大分辨拍照模式
 * 720P镜头:  60K(VPP_DATA0)(1080P h264) = 60k
			  辅码流:  12K(gen420) = 12K
			  其他:  10K(解码) + 其他(某些结构体用到) ≈ 15K
				
    total:  90K(没有额外功能,可以运行,如果空间足够,尽量到100K)
	
				
 
 *********************************************************************************************************************************************/
#define CONFIG_AVHEAP_SIZE              (90*1024)

/******************************************************************************
 * wifi的必要参数配置
 ******************************************************************************/
///////////////wifi parameter////////////
#define WIFI_RF_PWR_LEVEL               0           //选择WIFI功率。0：普通功率；1：比0的功率小；2：比1更小；10：大电流
#define WIFI_RTS_THRESHOLD              1600        //RTS阈值，2304等效于不用RTS
#define WIFI_RTS_MAX_RETRY              2           //RTS重试次数，范围为2~16
#define WIFI_TX_MAX_RETRY               15          //最大传输次数，范围为1~31
#define WIFI_TX_MAX_POWER               7           //TX最大发射功率，有0~7档

#define WIFI_TX_SUPP_RATE               0x0FFFFF    //TX速率支持，每1bit对应一种速率
#define WIFI_MAX_STA_CNT                8           //最多连接sta的数量。有效值为1~8
#define WIFI_MULICAST_RETRY             0           //组播帧传输次数
#define WIFI_ACS_CHAN_LISTS             0x1FFF      //要扫描的信道。每1bit对应1个信道(bit 0~11 -> chan 1~12)
#define WIFI_ACS_SCAN_TIME              150         //每个信道的扫描时间，单位ms
#define WIFI_MAX_PS_CNT                 30           //底层为休眠sta缓存的帧最大数量。0代表sta休眠由umac全程管理，底层不缓存
#define CHANNEL_DEFAULT                 0
#define SSID_DEFAULT                    "150X1-"   //"150X1-0768c1a3"
#define WIFI_TX_DUTY_CYCLE              100         //tx发送占空比，单位是%，范围是0~100
#define WIFI_SSID_FILTER_EN             0           //是否使能SSID过滤功能。使能后，只有隐藏SSID和指定SSID的beacon才会上传
#define WIFI_PREVENT_PS_MODE_EN         1           //是否尽可能的阻止sta进入休眠
#define NET_IP_ADDR_DEFAULT             0x01A9A8C0  //192.168.169.1
#define NET_MASK_DEFAULT                0x00FFFFFF  //255.255.255.0
#define NET_GW_IP_DEFAULT               0x01A9A8C0  //192.168.169.1
#define DHCPD_START_IP_DEFAULT          0x64A9A8C0  //192.168.169.100
#define DHCPD_END_IP_DEFAULT            0xFEA9A8C0  //192.168.169.254
#define DHCPD_DNS1_DEFAULT              0x01A9A8C0  //192.168.169.1
#define DHCPD_DNS2_DEFAULT              0x01A9A8C0  //192.168.169.1
#define DHCPD_ROUTER_DEFAULT            0x01A9A8C0  //192.168.169.1


/***************************************************************
 * 蓝牙
 **************************************************************/
//#define BLE_SUPPORT                 1

/****************************************************************
 * wifi速率配置,根据不同场景需要距离等去配置特定速率算法
#define RATE_CONTROL_ERSHAO         1
#define RATE_CONTROL_HANGPAI        2
#define RATE_CONTROL_IPC            3
#define RATE_CONTROL_BABYMPNITOR    4
 ***************************************************************/
#define RATE_CONTROL_SELECT             3


/*****************************************************************
 * VCAM开关,部分io电源域需要打开才有电
 *****************************************************************/
#define VCAM_EN                         1
#define VCCSD_33                        0

/*******************************************************************
 * 图像编码相关参数
 ******************************************************************/
#define MIPI_CSI_EN                     1
#define VPP_EN                          1
#define H264_EN                         1
#define ISP_EN                          1
#define JPG_EN                          1
#define SCALE_EN                        1

/*******************************************************************
 * 打开拍照模式(紧紧支持录风者模式)
 ******************************************************************/
#define TAKEPHOTO_EN                      1


/*******************************************************************
 * 支持h264的副码流
 ******************************************************************/
#define SUB_STREAM_EN 					1


/*****************************************************************
 * sd使能
 ***************************************************************/
#define SDH_EN                          1
#define FS_EN                           1



/************************************************************************************************************************
 * sensor型号配置,配置多个,支持自动识别(当前SDK只能支持全部单line或者双line,包含单line和双line需要对sdk初始化修改)
#define DEV_SENSOR_OV7725               0
#define DEV_SENSOR_OV7670               0
#define DEV_SENSOR_GC0308               0
#define DEV_SENSOR_OV2640               0
#define DEV_SENSOR_BF3A03               0
#define DEV_SENSOR_BF2013               0
#define DEV_SENSOR_OV2685               0
#define DEV_SENSOR_BF30A2               0
#define DEV_SENSOR_H62                  0
#define DEV_SENSOR_H63P                 0
#define DEV_SENSOR_SC1346               0
#define DEV_SENSOR_GC1084               0
#define DEV_SENSOR_GC2083               0
#define DEV_SENSOR_GC2053               0
#define DEV_SENSOR_SC2336P              0
#define DEV_SENSOR_SC2331               0
#define DEV_SENSOR_F38P                 0
#define DEV_SENSOR_F37P                 0
#define DEV_SENSOR_TP9950               0
 ***************************************************************************************************************************/
//#define DEV_SENSOR_SC1346               1
#define DEV_SENSOR_GC1084               1
#define DEV_SENSOR_GC2053               1



/************************************************************ 
 * mipi相关
INPUT_MODE:
#define YUV422                          0
#define RAW8                            1
#define RAW10                           2
#define RAW12                           3
************************************************************/
#define INPUT_MODE                      2   //RAW10
#define DOUBLE_CSI                      0
#define DOUBLE_LANE                     0


/***********************************************************
 *音频及功放使能io配置
 * ********************************************************/
#define AUDIO_EN                        1

/***********************************************************
 *默认mjpeg的节点数量,要根据mjpeg启动的分辨率去考虑
 默认节点大小:16*1024
 720P:算mjpeg大小50-80K,给10个节点足够
 1080P:算mjpeg大小100-150K,给20个节点足够
 其他分辨率,根据实际情况去配置
 * ********************************************************/
#define JPG_NODE_COUNT 30



/*************************************************************
 * 节省sram内存,将部分模块强制使用psram
 * 优点:节省sram内存
 * 缺点:psram读写慢,可能会影响性能
 
 1080P的sram内存不足,将部分数据放到了psram
 ************************************************************/
#define MORE_SRAM

/*************************************************************
 * 720P摄像头的mjpeg辅码流需要打开VPP_BUF1_EN
 * 720P的辅码流是h264(gen420),注意SUB_STREAM_WIDTH需要是与镜头
   等比例关系,包括是H(sdk默认这个是等比例,所以不需要配置H)
 ************************************************************/
#define VPP_BUF1_EN 1
#define PSRAM_FRAME_SAVE 1
#define SUB_STREAM_WIDTH    640

/************************************************************************
* 配置解码最大的size,如果不需要特殊size解码,这个配置320x180(缩略图用)
* 配置解码缓冲区节点数量(预分配空间,没有解码要求,默认1个节点就够了)
 ************************************************************************/
#define DECODE_MAX_W    320
#define DECODE_MAX_H    180
#define MAX_DECODE_YUV_TX 1

/************************************************************************
* 配置MP4录制最大文件的size
 ************************************************************************/
#define MAX_SINGLE_MP4_SIZE (100 * 1024 * 1024)

/************************************************************************
 * 720P摄像头的H264辅码流
 * 录风者设置图传是H264,则RECORDER_MODE等于0即可
 ************************************************************************/
#define RECORDER_MODE 0



/***********************************************************
 * 插值打开相关的宏
 * FORCE_SCALE_TO_H264:强行将h264插值到对应分辨率
 * VPP_SCALE_EN: 插值需要打开scale1的宏
 * VPP_SCALE_WIDTH:插值后的size
 * VPP_SCALE_HIGH:插值后的size
 * *********************************************************/
#define FORCE_SCALE_TO_H264
#define VPP_SCALE_EN 1
#define VPP_SCALE_WIDTH 1920
#define VPP_SCALE_HIGH 1080

/************************************************************************
* 开启文件系统优化
 ************************************************************************/
#define USE_FAT_CACHE 1

#elif (CUSTOMER_ID == 6)
/***************************************************************************************************************************************
 * UVC摄像头
 * UVC镜头: 与usb支持分辨率有关
 * 水印码流:与uvc镜头分辨率一致
 * 图传:RTSP  辅码流  默认地址:rtsp://ip:554/custom?route-usb(uvc镜头直接图传) 或者 rtsp://ip:554/custom?gen420_JPG_RECODE(uvc增加水印图传)
 * 文件系统:FAT32、EXFAT(配合ffconf.h设置)
 ***************************************************************************************************************************************/

/***************************************************************
 * 打开PIN_FROM_PARAM,通过脚本和config.cfg去生成对应的io配置信息
 * 请查看重要的文件:pin_param.h、config.cfg两个文件
 *************************************************************/
#define PIN_FROM_PARAM
/*****************************************
 * 打开对应demo的宏
 ****************************************/
#define SYS_APP_FPV

//#define SYS_APP_DEMO

/**********************************************************************
 * 系统必要信息宏
 * CONFIG_PSRAM_AVHEAP_SIZE:为应用分配的psram宏,需要根据应用场景分配
 * PSRAM_HEAP:如果需要用到psram,需要打开PSRAM_HEAP
 ********************************************************************/
#define DEFAULT_SYS_CLK                 (192*1000000) 
#define PSRAM_HEAP          //如果需要psram当作heap,需要打开这个宏
#define AV_PSRAM_HEAP    
#define AV_HEAP

/*********************************************************************
uvc: 节点固定占用空间默认:16K*8 = 128K

mjpeg解码(1280*720)预分配空间:  1.3M

mjpeg重新编码:   30*16K = 480K

约 需要2M

mjpeg的data缓冲:1M(动态,多缓冲多,少就是概率出现某些情况申请不到空间)

total:3M
**********************************************************************/
#define CONFIG_PSRAM_AVHEAP_SIZE        (3*1024*1024)

/************************************************************
 * 不同镜头以及功能不一样
 
 gen420:12K
 mjpeg解码:10K
 
 total≈20K(相当于分时复用,如果空间足够需要多给点)
 ************************************************************/
#define CONFIG_AVHEAP_SIZE              (20*1024)

/******************************************************************************
 * wifi的必要参数配置
 ******************************************************************************/
///////////////wifi parameter////////////
#define WIFI_RF_PWR_LEVEL               0           //选择WIFI功率。0：普通功率；1：比0的功率小；2：比1更小；10：大电流
#define WIFI_RTS_THRESHOLD              1600        //RTS阈值，2304等效于不用RTS
#define WIFI_RTS_MAX_RETRY              2           //RTS重试次数，范围为2~16
#define WIFI_TX_MAX_RETRY               15          //最大传输次数，范围为1~31
#define WIFI_TX_MAX_POWER               7           //TX最大发射功率，有0~7档

#define WIFI_TX_SUPP_RATE               0x0FFFFF    //TX速率支持，每1bit对应一种速率
#define WIFI_MAX_STA_CNT                8           //最多连接sta的数量。有效值为1~8
#define WIFI_MULICAST_RETRY             0           //组播帧传输次数
#define WIFI_ACS_CHAN_LISTS             0x1FFF      //要扫描的信道。每1bit对应1个信道(bit 0~11 -> chan 1~12)
#define WIFI_ACS_SCAN_TIME              150         //每个信道的扫描时间，单位ms
#define WIFI_MAX_PS_CNT                 30           //底层为休眠sta缓存的帧最大数量。0代表sta休眠由umac全程管理，底层不缓存
#define CHANNEL_DEFAULT                 0
#define SSID_DEFAULT                    "150X1-"   //"150X1-0768c1a3"
#define WIFI_TX_DUTY_CYCLE              100         //tx发送占空比，单位是%，范围是0~100
#define WIFI_SSID_FILTER_EN             0           //是否使能SSID过滤功能。使能后，只有隐藏SSID和指定SSID的beacon才会上传
#define WIFI_PREVENT_PS_MODE_EN         1           //是否尽可能的阻止sta进入休眠
#define NET_IP_ADDR_DEFAULT             0x01A9A8C0  //192.168.169.1
#define NET_MASK_DEFAULT                0x00FFFFFF  //255.255.255.0
#define NET_GW_IP_DEFAULT               0x01A9A8C0  //192.168.169.1
#define DHCPD_START_IP_DEFAULT          0x64A9A8C0  //192.168.169.100
#define DHCPD_END_IP_DEFAULT            0xFEA9A8C0  //192.168.169.254
#define DHCPD_DNS1_DEFAULT              0x01A9A8C0  //192.168.169.1
#define DHCPD_DNS2_DEFAULT              0x01A9A8C0  //192.168.169.1
#define DHCPD_ROUTER_DEFAULT            0x01A9A8C0  //192.168.169.1


/***************************************************************
 * 蓝牙
 **************************************************************/
//#define BLE_SUPPORT                 1

/****************************************************************
 * wifi速率配置,根据不同场景需要距离等去配置特定速率算法
#define RATE_CONTROL_ERSHAO         1
#define RATE_CONTROL_HANGPAI        2
#define RATE_CONTROL_IPC            3
#define RATE_CONTROL_BABYMPNITOR    4
 ***************************************************************/
#define RATE_CONTROL_SELECT             3


/*****************************************************************
 * VCAM开关,部分io电源域需要打开才有电
 *****************************************************************/
#define VCAM_EN                         1
#define VCCSD_33                        0

/*******************************************************************
 * 图像编码相关参数
 ******************************************************************/
#define JPG_EN                          1
#define SCALE_EN                        1

/*******************************************************************
UVC的配置
********************************************************************/
/* ------ USB2.0 ----- */
#define USB_EN                          1
#define USB_HOST_EN                     1
/*=========== RTT USB架构宏定义 ==========*/
#define RTT_USB_EN                      1   //RTT USB 架构使能 (USB_EN打开)
/*            USB HOST            */
#define RT_USBH                     //RTT USB HOST 使能
#define RT_USBH_UVC


/*******************************************************************
 * usb摄像头增加水印后重新编码使能
 ******************************************************************/
#define USB_JPG_ADD_WATERMARK             1


/*****************************************************************
 * sd使能
 ***************************************************************/
#define SDH_EN                          1
#define FS_EN                           1

/***********************************************************
 *默认mjpeg的节点数量,要根据mjpeg启动的分辨率去考虑
 默认节点大小:16*1024
 720P:算mjpeg大小50-80K,给10个节点足够
 1080P:算mjpeg大小100-150K,给20个节点足够
 其他分辨率,根据实际情况去配置
 * ********************************************************/
#define JPG_NODE_COUNT 30


/************************************************************************
* 开启文件系统优化
 ************************************************************************/
#define USE_FAT_CACHE 1


/************************************************************************
* 配置解码最大的size,usb摄像头支持最大分辨率1280x720
* 配置解码缓冲区节点数量(预分配空间,默认1个节点,可以调节)
 ************************************************************************/
#define DECODE_MAX_W    1280
#define DECODE_MAX_H    720
#define MAX_DECODE_YUV_TX 1




#endif

#endif
