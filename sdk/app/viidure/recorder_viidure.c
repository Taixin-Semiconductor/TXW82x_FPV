#include <csi_config.h>
#include "csi_kernel.h"
#include "typesdef.h"
#include "event.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "jpgdef.h"
#include "syscfg.h"
#include "stdio.h"
#include "sys_config.h"
#include "basic_include.h"
#include "recorder_viidure.h"
#include "lwip/netif.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "lwip/def.h"
#include "lwip/stats.h"
#include "lwip/etharp.h"
#include "lwip/ip.h"
#include "lwip/snmp.h"
#include "lwip/tcpip.h"
#include "netif/ethernet.h"
#include "netif/ethernetif.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "osal/mutex.h"
#include "osal/sleep.h"
#include "hal/vpp.h"
#include "hal/isp.h"
#include "sys/time.h"
#include "cjson/cJSON.h"
#include "osal/msgqueue.h"
#include "fs/fatfs/osal_file.h"
#include "dev/csi/hgdvp.h"
#include "dev/jpg/hgjpg.h"
#include "lib/net/http/http.h"
#include "lib/net/eloop/eloop.h"
#include "lib/umac/ieee80211.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "lib/common/timezone.h"
#include "video_app/file_thumb.h"
#include "video_app_h264_msi.h"
#include "video_app/file_common_api.h"
#include "video_record/file_process.h"
#include "audio_msi/audio_adc.h"
#include "sysevt_usb/sysevt_usb.h"
#include "app/takephoto_module/takephoto.h"

static struct os_work recorder_wk;
static int itemcfg_save(uint8_t type);
uint8_t get_mipi_video_status(void);
uint8_t get_mipi2_video_status(void);
uint8_t get_usb_video_status(void);
void rec_lock_file(char *file_name, char *file_path);
static void itemcfg_handle(void);

//需要与items_list中设置分辨的一致才行
static const int http_dpi[][2] = {
    {1280, 720},  // 720P
    {1920, 1080}, // 1080P
    {2560, 1440}, // 2K
    {3840, 2160}, // 4K
    {7680, 4320}, // 8K
};

#define addWeb(web, url, type,reponse,queue) { url,type,(reponse),queue }

/******************************************************************************
 *                       这个文件用于适配录风者app的功能                      *
 *                                                                            *
 ******************************************************************************/

// 结构体申请空间函数
#ifdef MORE_SRAM 
#define STREAM_LIBC_MALLOC os_malloc_psram
#define STREAM_LIBC_FREE os_free_psram
#define STREAM_LIBC_ZALLOC os_zalloc_psram
#else
#define STREAM_LIBC_MALLOC os_malloc
#define STREAM_LIBC_FREE os_free
#define STREAM_LIBC_ZALLOC os_zalloc
#endif

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 消息推送端口
#define PUSH_MESSAGE_PORT       60000
// 开启后，SD剩余容量小于 SD_FREE_SIZE_MIN 则推送SD卡满消息 
#define SD_FREE_SIZE_EN         0   
#define SD_FREE_SIZE_MIN        0   

// 停车监控相册
#define PARKING_MOR_ALBUM_EN    0
// APP 锁存功能开关
#define APP_LOCK_VIDEO_EN       0
// 多目
#define MULTI_CAMERA_EN         1

#define MIPI                    0
#define USB                     1
#define MP4                     0
#define AVI                     1

#define CAM_MASK_SINGLE         ( 1 << 0 )  // 单目
#define CAM_MASK_FRONT          ( 1 << 1 )
#define CAM_MASK_INTERIOR       ( 1 << 2 )
#define CAM_MASK_BACK           ( 1 << 3 )
// 常用组合
#define CAM_ALL                 (CAM_MASK_FRONT | CAM_MASK_BACK | CAM_MASK_INTERIOR)
#define CAM_FRONT_BACK          (CAM_MASK_FRONT | CAM_MASK_BACK)
#define CAM_FRONT_INTERIOR      (CAM_MASK_FRONT | CAM_MASK_INTERIOR)
// 组合配置
#define CAM_MASK_ENABLE         CAM_MASK_SINGLE

static struct cam_cfg *g_cam_configs = NULL;
// 镜头配置表
static const struct cam_cfg g_cam_configs_f[] = {
    [0] = {
        .msi = NULL,
        .msi_name = "rec",
        .src_msi = NULL,
        .src_msi_name = AUTO_H264,
        .aac_msi = NULL,
        .srcID = FRAMEBUFF_SOURCE_CAMERA0,
        .filter_type = FSTYPE_H264_VPP_DATA0,
        .mode = MP4_MODE_NORMAL,
        .mask = CAM_MASK_SINGLE,
        .type = MP4,
        .src_type = MIPI,
        .rec_path = REC_PATH,
        .img_path = IMG_PATH,
        .enable = 0,
        .media = {
            #if SUB_STREAM_EN
            .rtsp_url = "h264?1",
            .tran_mode = "tcp",
            #else
            .rtsp_url = "webcam",
            .tran_mode = "udp",
            #endif
        },
        .file_process = {
            .loop = NULL,
            .rec_path = REC_PATH,
            .ext_name = MP4_EXTENSION_NAME,
            .create_file = rec_create_file,
            .loop_free = rec_loop_free,
            .lock_file = rec_lock_file,
            .frame_time = 0,
            .need_lock = 0
        },
        .create_func = mp4_record_msi_init,
        .get_status = get_mipi_video_status
    },
    [1] = {
        .msi = NULL,
        .msi_name = "rec_front",
        .src_msi = NULL,
        .src_msi_name = AUTO_H264,
        .aac_msi = NULL,
        .srcID = FRAMEBUFF_SOURCE_CAMERA0,
        .filter_type = FSTYPE_H264_VPP_DATA0,
        .mode = MP4_MODE_NORMAL,
        .mask = CAM_MASK_FRONT,
        .type = MP4,
        .src_type = MIPI,
        .rec_path = RECA_PATH,
        .img_path = IMGA_PATH,
        .enable = 0,
        .media = {
            .rtsp_url = "h264?0",
            .tran_mode = "tcp",
        },
        .file_process = {
            .loop = NULL,
            .rec_path = RECA_PATH,
            .ext_name = MP4_EXTENSION_NAME,
            .create_file = rec_create_file,
            .loop_free = rec_loop_free,
            .lock_file = rec_lock_file,
            .frame_time = 0,
            .need_lock = 0
        },
        .create_func = mp4_record_msi_init,
        .get_status = get_mipi_video_status
    },
    [2] = {
        .msi = NULL,
        .msi_name = "rec_int",
        .src_msi = NULL,
        .src_msi_name = AUTO_H264,
        .aac_msi = NULL,
        .srcID = FRAMEBUFF_SOURCE_NONE,
        .filter_type = FSTYPE_H264_VPP_DATA1,
        .mode = MP4_MODE_NORMAL,
        .mask = CAM_MASK_INTERIOR,
        .type = MP4,
        .src_type = MIPI,
        .rec_path = RECB_PATH,
        .img_path = IMGB_PATH,
        .enable = 0,
        .media = {
            .rtsp_url = "h264?1",
            .tran_mode = "tcp",
        },
        .file_process = {
            .loop = NULL,
            .rec_path = RECB_PATH,
            .ext_name = MP4_EXTENSION_NAME,
            .create_file = rec_create_file,
            .loop_free = rec_loop_free,
            .lock_file = rec_lock_file,
            .frame_time = 0,
            .need_lock = 0
        },
        .create_func = mp4_record_msi_init,
        .get_status = get_mipi2_video_status
    },
    [3] = {
        .msi = NULL,
        .msi_name = "rec_rear",
        .src_msi = NULL,
        .src_msi_name = ROUTE_USB,
        .aac_msi = NULL,
        .srcID = 0,
        .filter_type = (uint8_t) ~0,
        .mode = 0,
        .mask = CAM_MASK_BACK,
        .type = AVI,
        .src_type = USB,
        .rec_path = RECC_PATH,
        .img_path = IMGC_PATH,
        .enable = 0,
        .media = {
            .rtsp_url = "custom?route-usb",
            .tran_mode = "tcp",
        },
        .file_process = {
            .loop = NULL,
            .rec_path = RECC_PATH,
            .ext_name = AVI_EXTENSION_NAME,
            .create_file = rec_create_file,
            .loop_free = rec_loop_free,
            .lock_file = rec_lock_file,
            .frame_time = 0,
            .need_lock = 0
        },
        .create_func = avi_record_msi_init,
        .get_status = get_usb_video_status
    }
};

#define REC_ARRAY_SIZE(a)       (sizeof(a) / sizeof((a)[0]))
// 镜头配置数量
#define CAMERAS_CONFIG_COUNT    REC_ARRAY_SIZE(g_cam_configs_f)

// 配置项数量
#define ITEMS_CONFIG_COUNT      REC_ARRAY_SIZE(items_cfg_f)
#define ITEM_MAX_NUM            8

typedef enum {
    ITEM_ENABLE,
    ITEM_DISABLE
}ItemEnable;

struct items_config;
static struct items_config *items_cfg = NULL;

typedef void (*param_handler_t)(struct items_config *item, uint8_t value, struct httpClient *httpClient);

typedef struct {
    const char *name;
	const uint8_t default_value;
    const ItemEnable enable;
    const param_handler_t handler;
    const char *items_list[ITEM_MAX_NUM];
} config_item_t;

struct items_config {
    uint8_t value;
	const config_item_t *const_item;
};

#define ITEMS_CONFIG_INIT(_name, _default_value, _value, _enable, _handler, ...)    \
    {                                                               \
        .value = (_value),                                          \
        .const_item = &(const config_item_t) {                      \
            .name = (_name),                                        \
            .default_value = (_default_value),                      \
            .enable = (_enable),                                    \
            .handler = (_handler),                                  \
            .items_list = {__VA_ARGS__}                             \
        }                                                           \
    }

typedef enum {
    SET_ITEMS_VALUE,
    GET_ITEMS_VALUE,
    GET_ITEMS_DEFAULT_VALUE,
    GET_ITEMS_ENABLE
} ItemsProcessType;

static void set_default(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_mic(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_osd(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_rec_resolution(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_rec_split_duration(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_encodec(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_speaker(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_gsr_sensitivity(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_rec(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_language(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_timelapse_rate(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_park_record_time(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_park_gsr_sensitivity(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_light_fre(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_rear_mirror(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_wdr(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_parking_mode(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_parking_monitor(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_voice_control(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_screen_standby(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_auto_poweroff(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_adas(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_boot_sound(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_video_flip(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_video_mirror(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_key_tone(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_hour_type(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_time_format(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_ev(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_ir_mode(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_speed_unit(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_wb(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_anti_shake(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_fast_record(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_denoise(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_auto_record(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_image_size(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_delay_shot(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_continue_shot(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_interior_record(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_shot_timelapse_rate(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_shot_timelapse_time(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_video_flip_mirror(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_five_g_wifi(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_gps_osd(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_speed_osd(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_rear_flip(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_summer_timer(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_screen_brightness(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_sound_indicator(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_gps_data(struct items_config *item, uint8_t value, struct httpClient *httpClient);
// static void set_vi_lowfps_rec(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_coordinate_osd(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_inside_mirror(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_status_light(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_osd_option(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_wifi_channel(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_fill_light(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_screensaver(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_front_rotate(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_hdr(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_wifi_boot_starter(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_screen_saver_type(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_work_mode(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_frame_zoom(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_rotate(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_motion_detect(struct items_config *item, uint8_t value, struct httpClient *httpClient);
// static void set_image_quality(struct items_config *item, uint8_t value, struct httpClient *httpClient);
// static void set_beauty(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_file_format(struct items_config *item, uint8_t value, struct httpClient *httpClient);
// static void set_video_frame_rate(struct items_config *item, uint8_t value, struct httpClient *httpClient);
// static void set_fov(struct items_config *item, uint8_t value, struct httpClient *httpClient);
// static void set_video_watermark(struct items_config *item, uint8_t value, struct httpClient *httpClient);
// static void set_motion_detect_rec_duration(struct items_config *item, uint8_t value, struct httpClient *httpClient);
// static void set_record_light(struct items_config *item, uint8_t value, struct httpClient *httpClient);
// static void set_motion_detect_sensitivity(struct items_config *item, uint8_t value, struct httpClient *httpClient);
// static void set_sharpness(struct items_config *item, uint8_t value, struct httpClient *httpClient);
// static void set_brightness(struct items_config *item, uint8_t value, struct httpClient *httpClient);
// static void set_contrast(struct items_config *item, uint8_t value, struct httpClient *httpClient);
// static void set_saturation(struct items_config *item, uint8_t value, struct httpClient *httpClient);
// static void set_color(struct items_config *item, uint8_t value, struct httpClient *httpClient);
// static void set_tv_video_out(struct items_config *item, uint8_t value, struct httpClient *httpClient);
// static void set_interval_shot(struct items_config *item, uint8_t value, struct httpClient *httpClient);
static void set_switchcam(struct items_config *item, uint8_t value, struct httpClient *httpClient);

static const struct items_config items_cfg_f[] = {
/*                    功能名                       默认值   当前值      使能       执行函数                功能选项          */
    ITEMS_CONFIG_INIT("mic",                         1,      1,     ITEM_ENABLE,  set_mic,                              
        "off", "on"),                                                                                 // 录像声音
    ITEMS_CONFIG_INIT("osd",                         1,      1,     ITEM_ENABLE,  set_osd,                              
        "off", "on"),                                                                                 // 时间水印
    ITEMS_CONFIG_INIT("rec_resolution",              0,      0,     ITEM_ENABLE,  set_rec_resolution,                   
        "2K", "4K"),                                                                                  // 录像分辨率
    ITEMS_CONFIG_INIT("rec_split_duration",          1,      1,     ITEM_ENABLE,  set_rec_split_duration,               
        "off", "1MIN", "2MIN", "3MIN"),                                                               // 录像文件时长
    ITEMS_CONFIG_INIT("encodec",                     0,      0,     ITEM_DISABLE, set_encodec,                          
        "h.264", "h.265"),                                                                            // 视频编码格式
    ITEMS_CONFIG_INIT("speaker",                     0,      0,     ITEM_DISABLE, set_speaker,                          
        "off", "lowest", "lower", "low", "middle", "high", "very high"),                              // 扬声器音量
    ITEMS_CONFIG_INIT("gsr_sensitivity",             2,      2,     ITEM_ENABLE,  set_gsr_sensitivity,                  
        "off", "lowest", "lower", "low", "middle", "high"),                                           // 碰撞感应灵敏度
    ITEMS_CONFIG_INIT("rec",                         0,      0,     ITEM_DISABLE, set_rec,                              
        "off", "on"),                                                                                 // 录像开关，只记录状态，不使能
    ITEMS_CONFIG_INIT("language",                    1,      1,     ITEM_ENABLE,  set_language,                         
        "en_US", "zh_CN", "zh_CHT", "ja", "ko", "ru", "fr", "de"),                                    // 记录仪语言
    ITEMS_CONFIG_INIT("timelapse_rate",              0,      0,     ITEM_ENABLE,  set_timelapse_rate,                   
        "off", "1fps", "2fps", "5fps"),                                                               // 缩时录影帧率
    ITEMS_CONFIG_INIT("park_record_time",            0,      0,     ITEM_ENABLE,  set_park_record_time,                 
        "off", "12hour", "24hour", "48hour"),                                                         // 停车监控时长
    ITEMS_CONFIG_INIT("park_gsr_sensitivity",        0,      0,     ITEM_ENABLE,  set_park_gsr_sensitivity,             
        "off", "lowest", "lower", "low", "middle", "high"),                                           // 停车监控碰撞感应
    ITEMS_CONFIG_INIT("light_fre",                   0,      0,     ITEM_ENABLE,  set_light_fre,                        
        "50HZ", "60HZ"),                                                                              // 光源频率
    ITEMS_CONFIG_INIT("rear_mirror",                 0,      0,     ITEM_DISABLE, set_rear_mirror,                      
        "off", "on"),                                                                                 // 后路镜像
    ITEMS_CONFIG_INIT("wdr",                         0,      0,     ITEM_DISABLE, set_wdr,                              
        "off", "on"),                                                                                 // 宽动态
    ITEMS_CONFIG_INIT("parking_mode",                0,      0,     ITEM_DISABLE, set_parking_mode,                     
        "off", "timelapse", "normrec"),                                                               // 停车监控模式
    ITEMS_CONFIG_INIT("parking_monitor",             0,      0,     ITEM_DISABLE, set_parking_monitor,                  
        "off", "on"),                                                                                 // 停车监控开关
    ITEMS_CONFIG_INIT("voice_control",               0,      0,     ITEM_DISABLE, set_voice_control,                    
        "off", "on"),                                                                                 // 声控开关
    ITEMS_CONFIG_INIT("screen_standby",              0,      0,     ITEM_ENABLE,  set_screen_standby,                   
        "off", "1MIN", "2MIN", "3MIN"),                                                               // 自动息屏时间
    ITEMS_CONFIG_INIT("auto_poweroff",               0,      0,     ITEM_ENABLE,  set_auto_poweroff,                    
        "off", "1H", "12H"),                                                                          // 延时关机时间
    ITEMS_CONFIG_INIT("adas",                        0,      0,     ITEM_DISABLE, set_adas,                             
        "off", "on"),                                                                                 // 驾驶辅助
    ITEMS_CONFIG_INIT("boot_sound",                  0,      0,     ITEM_DISABLE, set_boot_sound,                       
        "off", "on"),                                                                                 // 开机启动音
    ITEMS_CONFIG_INIT("video_flip",                  0,      0,     ITEM_DISABLE, set_video_flip,                       
        "off", "on"),                                                                                 // 视频垂直翻转
    ITEMS_CONFIG_INIT("video_mirror",                0,      0,     ITEM_DISABLE, set_video_mirror,                     
        "off", "on"),                                                                                 // 视频水平镜像
    ITEMS_CONFIG_INIT("key_tone",                    0,      0,     ITEM_ENABLE,  set_key_tone,                         
        "off", "on"),                                                                                 // 按键音
    ITEMS_CONFIG_INIT("hour_type",                   0,      0,     ITEM_DISABLE, set_hour_type,                        
        "off", "on"),                                                                                 // 24 小时制
    ITEMS_CONFIG_INIT("time_format",                 0,      0,     ITEM_DISABLE, set_time_format,                      
        "yyyy-MM-dd", "yyyy/MM/dd"),                                                                  // 时间格式
    ITEMS_CONFIG_INIT("ev",                          0,      0,     ITEM_DISABLE, set_ev,                               
        "-1", "0", "1"),                                                                              // 曝光补偿
    ITEMS_CONFIG_INIT("ir_mode",                     0,      0,     ITEM_DISABLE, set_ir_mode,                          
        "auto", "manual"),                                                                            // 红外模式
    ITEMS_CONFIG_INIT("speed_unit",                  0,      0,     ITEM_DISABLE, set_speed_unit,                       
        "km/h", "mph"),                                                                               // 速度单位
    ITEMS_CONFIG_INIT("wb",                          0,      0,     ITEM_DISABLE, set_wb,                               
        "off", "on"),                                                                                 // 白平衡
    ITEMS_CONFIG_INIT("anti_shake",                  0,      0,     ITEM_DISABLE, set_anti_shake,                       
        "off", "on"),                                                                                 // 防抖
    ITEMS_CONFIG_INIT("fast_record",                 0,      0,     ITEM_DISABLE, set_fast_record,                      
        "off", "2x", "4x", "6x"),                                                                     // 快速录像
    ITEMS_CONFIG_INIT("denoise",                     0,      0,     ITEM_DISABLE, set_denoise,                          
        "off", "on"),                                                                                 // 图像降噪
    ITEMS_CONFIG_INIT("auto_record",                 0,      0,     ITEM_DISABLE, set_auto_record,                      
        "off", "on"),                                                                                 // 强制录像
    ITEMS_CONFIG_INIT("image_size",                  0,      0,     ITEM_ENABLE,  set_image_size,                       
        "720P", "1080P", "2K", "4K", "8K"),                                                           // 拍照分辨率
    ITEMS_CONFIG_INIT("delay_shot",                  0,      0,     ITEM_DISABLE, set_delay_shot,                       
        "off", "2s", "10s"),                                                                          // 延迟拍照
    ITEMS_CONFIG_INIT("continue_shot",               0,      0,     ITEM_ENABLE,  set_continue_shot,                    
        "off", "2", "10"),                                                                            // 连拍张数
    ITEMS_CONFIG_INIT("interior_record",             0,      0,     ITEM_DISABLE, set_interior_record,                  
        "off", "on"),                                                                                 // 车内录像
    ITEMS_CONFIG_INIT("shot_timelapse_rate",         0,      0,     ITEM_DISABLE, set_shot_timelapse_rate,              
        "off", "2s", "10s"),                                                                          // 拍照缩时间隔
    ITEMS_CONFIG_INIT("shot_timelapse_time",         0,      0,     ITEM_DISABLE, set_shot_timelapse_time,              
        "off", "2s", "10s"),                                                                          // 拍照缩时间长
    ITEMS_CONFIG_INIT("video_flip_mirror",           0,      0,     ITEM_ENABLE,  set_video_flip_mirror,                
        "off", "on"),                                                                                 // 图像翻转
    ITEMS_CONFIG_INIT("five_g_wifi",                 0,      0,     ITEM_DISABLE, set_five_g_wifi,                      
        "off", "on"),                                                                                 // 5GWiFi
    ITEMS_CONFIG_INIT("gps_osd",                     0,      0,     ITEM_DISABLE, set_gps_osd,                          
        "off", "on"),                                                                                 // GPS 水印
    ITEMS_CONFIG_INIT("speed_osd",                   0,      0,     ITEM_DISABLE, set_speed_osd,                        
        "off", "on"),                                                                                 // 速度水印
    ITEMS_CONFIG_INIT("rear_flip",                   0,      0,     ITEM_DISABLE, set_rear_flip,                        
        "off", "on"),                                                                                 // 后路翻转
    ITEMS_CONFIG_INIT("summer_timer",                0,      0,     ITEM_DISABLE, set_summer_timer,                     
        "off", "on"),                                                                                 // 夏令时
    ITEMS_CONFIG_INIT("screen_brightness",           0,      0,     ITEM_DISABLE, set_screen_brightness,                
        "off", "low", "middle", "high", "very high"),                                                 // 屏幕亮度
    ITEMS_CONFIG_INIT("sound_indicator",             0,      0,     ITEM_DISABLE, set_sound_indicator,                  
        "off", "on"),                                                                                 // 提示音
    ITEMS_CONFIG_INIT("gps_data",                    0,      0,     ITEM_DISABLE, set_gps_data,                         
        "off", "on"),                                                                                 // GPS 数据
    // ITEMS_CONFIG_INIT("vi_lowfps_rec",               0,      0,      ITEM_DISABLE, set_vi_lowfps_rec,                     
        // "1hour", "2hour", "4hour"),                                                                   // 缩时录像时长
    ITEMS_CONFIG_INIT("coordinate_osd",              0,      0,     ITEM_DISABLE, set_coordinate_osd,                   
        "off", "on"),                                                                                 // 经纬度水印
    ITEMS_CONFIG_INIT("inside_mirror",               0,      0,     ITEM_DISABLE, set_inside_mirror,                    
        "off", "on"),                                                                                 // 车内镜像
    ITEMS_CONFIG_INIT("status_light",                0,      0,     ITEM_DISABLE, set_status_light,                     
        "off", "on"),                                                                                 // 状态灯
    ITEMS_CONFIG_INIT("osd_option",                  0,      0,     ITEM_DISABLE, set_osd_option,                       
        "off", "on", "logo&date", "logo&model&date&time"),                                            // 视频水印选项
    ITEMS_CONFIG_INIT("wifi_channel",                0,      0,     ITEM_DISABLE, set_wifi_channel,                     
        "0", "1", "2"),                                                                               // WiFi 信道
    ITEMS_CONFIG_INIT("fill_light",                  0,      0,     ITEM_DISABLE, set_fill_light,                       
        "off", "on"),                                                                                 // 补光
    ITEMS_CONFIG_INIT("screensaver",                 0,      0,     ITEM_DISABLE, set_screensaver,                      
        "off", "on"),                                                                                 // 屏幕保护
    ITEMS_CONFIG_INIT("front_rotate",                0,      0,     ITEM_DISABLE, set_front_rotate,                     
        "off", "on", "auto"),                                                                         // 前路旋转
    ITEMS_CONFIG_INIT("hdr",                         0,      0,     ITEM_DISABLE, set_hdr,                              
        "off", "on"),                                                                                 // 高动态
    ITEMS_CONFIG_INIT("wifi_boot_starter",           0,      0,     ITEM_DISABLE, set_wifi_boot_starter,                
        "off", "on"),                                                                                 // 开机启动 WiFi
    ITEMS_CONFIG_INIT("screen_saver_type",           0,      0,     ITEM_DISABLE, set_screen_saver_type,                
        "screen_off", "screen_saver_on"),                                                             // 屏保类型
    ITEMS_CONFIG_INIT("work_mode",                   0,      0,     ITEM_DISABLE, set_work_mode,                        
        "record", "capture", "delaycapture", "intervalcapture", "continuouscapture", "slowrecord", "looprecord", "timelapserecord"),      // 预览多模式切换
    ITEMS_CONFIG_INIT("frame_zoom",                  0,      0,     ITEM_DISABLE, set_frame_zoom,                       
        "1x", "2x", "3x"),                                                                            // 预览画面缩放切换
    ITEMS_CONFIG_INIT("rotate",                      0,      0,     ITEM_DISABLE, set_rotate,                           
        "off", "on", "auto"),                                                                         // 图像旋转
    ITEMS_CONFIG_INIT("motion_detect",               0,      0,     ITEM_DISABLE, set_motion_detect,                    
        "off", "on"),                                                                                 // 移动侦测
    // ITEMS_CONFIG_INIT("image_quality",               0,      0,      ITEM_DISABLE, set_image_quality,                 
        // "off", "on"),                                                                                 // 拍照质量
    // ITEMS_CONFIG_INIT("beauty",                      0,      0,      ITEM_DISABLE, set_beauty,                        
        // "off", "on"),                                                                                 // 美颜
    ITEMS_CONFIG_INIT("file_format",                 0,      0,     ITEM_DISABLE, set_file_format,                      
        "MP4", "AVI"),                                                                                // 录像格式
    // ITEMS_CONFIG_INIT("video_frame_rate",            0,      0,      ITEM_DISABLE, set_video_frame_rate,                  
        // "30", "60"),                                                                                  // 视频帧率
    // ITEMS_CONFIG_INIT("fov",                         0,      0,      ITEM_DISABLE, set_fov,                               
        // "wide", "middle", "narrow"),                                                                  // 视角
    // ITEMS_CONFIG_INIT("video_watermark",             0,      0,      ITEM_DISABLE, set_video_watermark,                   
        // "off", "on"),                                                                                 // 视频水印
    // ITEMS_CONFIG_INIT("motion_detect_rec_duration",  0,      0,      ITEM_DISABLE, set_motion_detect_rec_duration,        
        // "off", "12hour", "24hour"),                                                                   // 移动侦测录像时间
    // ITEMS_CONFIG_INIT("record_light",                0,      0,      ITEM_DISABLE, set_record_light,                      
        // "off", "on"),                                                                                 // 录像警示灯
    // ITEMS_CONFIG_INIT("motion_detect_sensitivity",   0,      0,      ITEM_DISABLE, set_motion_detect_sensitivity,         
        // "off", "low", "middle", "high"),                                                              // 移动侦测灵敏度
    // ITEMS_CONFIG_INIT("sharpness",                   0,      0,      ITEM_DISABLE, set_sharpness,                         
        // "off", "low", "middle", "high"),                                                              // 锐度
    // ITEMS_CONFIG_INIT("brightness",                  0,      0,      ITEM_DISABLE, set_brightness,                        
        // "off", "low", "middle", "high"),                                                              // 亮度
    // ITEMS_CONFIG_INIT("contrast",                    0,      0,      ITEM_DISABLE, set_contrast,                          
        // "off", "low", "middle", "high"),                                                              // 对比度
    // ITEMS_CONFIG_INIT("saturation",                  0,      0,      ITEM_DISABLE, set_saturation,                        
        // "off", "low", "middle", "high"),                                                              // 饱和度
    // ITEMS_CONFIG_INIT("color",                       0,      0,      ITEM_DISABLE, set_color,                             
        // "default", "mono", "sepia", "cool", "warm", "vivid"),                                         // 色彩
    // ITEMS_CONFIG_INIT("tv_video_out",                0,      0,      ITEM_DISABLE, set_tv_video_out,                      
        // "NTSC", "PAL"),                                                                               // TV 模式
    // ITEMS_CONFIG_INIT("interval_shot",               0,      0,      ITEM_DISABLE, set_interval_shot,                     
        // "2s", "5s"),                                                                                  // 间隔拍照
    ITEMS_CONFIG_INIT("switchcam",                   0,      0,     ITEM_DISABLE, set_switchcam,                         
        NULL),                                                                                        // 切换摄像头，记录状态，不使能
    ITEMS_CONFIG_INIT("timezone",                    0,      0,     ITEM_DISABLE,          NULL,                         
        NULL),                                                                                        // 时区，记录状态，不使能
    ITEMS_CONFIG_INIT("saveflag",                    0,      0,     ITEM_DISABLE,          NULL,                         
        NULL),                                                                                        // 保存状态标志位，记录状态，不使能
    ITEMS_CONFIG_INIT("lockvideo",                   0,      0,     ITEM_DISABLE,          NULL,                         
        NULL),                                                                                        // 锁存，记录状态，不使能
    ITEMS_CONFIG_INIT("cameranum",                   0,      0,     ITEM_DISABLE,          NULL,                         
        NULL),                                                                                        // 镜头数量，记录状态，不使能
    ITEMS_CONFIG_INIT("sdstatus",                    0,      0,     ITEM_DISABLE,          NULL,                            
        NULL),                                                                                        // SD卡状态，记录状态，不使能
};

int get_items_counts(int num) {
    int count = 0;
    if(num < ITEMS_CONFIG_COUNT) {
        while((count < ITEM_MAX_NUM) && (items_cfg[num].const_item->items_list[count] != NULL)) {
            count++;
        }
        return count;
    }
    return 0;
}

int items_value_process(const char* name, uint8_t value, ItemsProcessType type) {
    int name_len = 0;
    int items_cfg_len = 0;
    if(items_cfg == NULL)
        return -1;
    for (int i = 0; i < ITEMS_CONFIG_COUNT; i++) {
        name_len = strlen(name);
        items_cfg_len = strlen(items_cfg[i].const_item->name);
        if (strncmp(items_cfg[i].const_item->name, name, (name_len > items_cfg_len) ? name_len : items_cfg_len) == 0) {
            switch (type)
            {
            case SET_ITEMS_VALUE:
                items_cfg[i].value = value;
                break;
            case GET_ITEMS_VALUE:
                return items_cfg[i].value;
                break;
            case GET_ITEMS_DEFAULT_VALUE:
                return items_cfg[i].const_item->default_value;
                break;
            case GET_ITEMS_ENABLE:
                return items_cfg[i].const_item->enable;
                break;
            default:
                os_printf("Invalid items process type\r\n");
                return -1;
            }
            return 0;
        }
    }
    os_printf("Invalid config name: %s\r\n", name);
    return -1;
}

static char *remove_prefix(char *filepath)
{
    if (!filepath) 
        return NULL;
    
    char* slash = strchr(filepath, '/');
    if(slash == NULL)
    {
        slash = filepath;
    }
    else
    {
        slash++;
    }

    char* underscore = strchr(filepath, '_');
    if (underscore) 
    {
        if(os_strstr(slash, SOS_PREFIX))
        {
            os_memmove(slash, underscore + 1, strlen(underscore) + 1);
        }
    }
    
    return filepath;
}

static char* remove_suffix(char* filepath) 
{
    if(!filepath) 
        return NULL;

    char* underscore = strrchr(filepath, '_');
    char* dot = strrchr(filepath, '.');

    if(underscore && dot && underscore < dot) 
    {
        if(os_strstr(underscore, FRONT_SUFFIX) || os_strstr(underscore, BACK_SUFFIX) || os_strstr(underscore, INTER_SUFFIX)) 
        {
            os_memmove(underscore, dot, strlen(dot) + 1);
        }
    }
    return filepath;
}

static void http_reponse_getproductinfo(struct httpClient *httpClient);
static void http_reponse_settimezone(struct httpClient *httpClient);
static void http_reponse_setsystime(struct httpClient *httpClient);
static void http_reponse_getmediainfo(struct httpClient *httpClient);
static void http_reponse_getdeviceattr(struct httpClient *httpClient);
static void http_reponse_getparamvalue(struct httpClient *httpClient);
static void http_reponse_capability(struct httpClient *httpClient);
static void http_reponse_getparamitems_all(struct httpClient *httpClient);
static void http_reponse_getsdinfo(struct httpClient *httpClient);
static void http_reponse_getbatteryinfo(struct httpClient *httpClient);
static void http_reponse_getadasitems(struct httpClient *httpClient);
static void http_reponse_getstorageinfo(struct httpClient *httpClient);
static void http_reponse_lockvideo(struct httpClient *httpClient);
static void http_reponse_getlockvideostatus(struct httpClient *httpClient);
static void http_reponse_setting(struct httpClient *httpClient);
static void http_reponse_setparamvalue(struct httpClient *httpClient);
static void http_reponse_enterrecorder(struct httpClient *httpClient);
static void http_reponse_exitrecorder(struct httpClient *httpClient);
static void http_reponse_getrecduration(struct httpClient *httpClient);
static void http_reponse_getadasvalue(struct httpClient *httpClient);
static void http_reponse_playback(struct httpClient *httpClient);
static void http_reponse_getfilelist(struct httpClient *httpClient);
static void http_reponse_thumbnail(struct httpClient *httpClient);
static void http_reponse_setwifi(struct httpClient *httpClient);
static void http_reponse_reset(struct httpClient *httpClient);
static void http_reponse_sdformat(struct httpClient *httpClient);
static void http_reponse_wifireboot(struct httpClient *httpClient);
static void http_reponse_video(struct httpClient *httpClient);
static void http_reponse_event(struct httpClient *httpClient);
static void http_reponse_deletefile(struct httpClient *httpClient);
static void http_reponse_snapshot(struct httpClient *httpClient);
static void http_reponse_rec(struct httpClient *httpClient);
static void http_getparamvalue_rec(struct httpClient *httpClient);


static const struct url getweb[] = 
{
    addWeb(web, "app/getproductinfo", 1, http_reponse_getproductinfo, NULL),
	addWeb(web, "app/settimezone?timezone=", 2, http_reponse_settimezone, NULL),
	addWeb(web, "app/setsystime?date=", 2, http_reponse_setsystime, NULL),
	addWeb(web, "app/getmediainfo", 1, http_reponse_getmediainfo, NULL),
	addWeb(web, "app/getdeviceattr", 1, http_reponse_getdeviceattr, NULL),
	addWeb(web, "app/getparamvalue?", 2, http_reponse_getparamvalue, NULL),
	addWeb(web, "app/getparamitems?param=all", 1, http_reponse_getparamitems_all, NULL),
	addWeb(web, "app/getsdinfo", 1, http_reponse_getsdinfo, NULL),
	addWeb(web, "app/getbatteryinfo", 1, http_reponse_getbatteryinfo, NULL),
	addWeb(web, "app/capability", 1, http_reponse_capability, NULL),
    addWeb(web, "app/lockvideo", 1, http_reponse_lockvideo, NULL),
	addWeb(web, "app/getlockvideostatus", 1, http_reponse_getlockvideostatus, NULL),
	addWeb(web, "app/getstorageinfo", 1, http_reponse_getstorageinfo, NULL),
	addWeb(web, "app/setting?", 2, http_reponse_setting, NULL),
	addWeb(web, "app/setparamvalue?", 2, http_reponse_setparamvalue, NULL),
	addWeb(web, "app/enterrecorder", 1, http_reponse_enterrecorder, NULL),
	addWeb(web, "app/exitrecorder", 1, http_reponse_exitrecorder, NULL),
	addWeb(web, "app/getrecduration", 1, http_reponse_getrecduration, NULL),
	addWeb(web, "app/snapshot", 1, http_reponse_snapshot, NULL),
	addWeb(web, "app/playback", 2, http_reponse_playback, NULL),
	addWeb(web, "app/getfilelist", 1, http_reponse_getfilelist, NULL),
	addWeb(web, "app/getthumbnail?file=", 2, http_reponse_thumbnail, NULL),
	addWeb(web, "app/setwifi?", 2, http_reponse_setwifi, NULL),
	addWeb(web, "app/reset", 1, http_reponse_reset, NULL),
	addWeb(web, "app/sdformat", 2, http_reponse_sdformat, NULL),
	addWeb(web, "app/wifireboot", 1, http_reponse_wifireboot, NULL),
	addWeb(web, "LOOP/", 2, http_reponse_video, NULL),
	addWeb(web, "EVENT/", 2, http_reponse_event, NULL),
    addWeb(web, "EMR/", 2, http_reponse_video, NULL),
    addWeb(web, "PARK/", 2, http_reponse_video, NULL),
	addWeb(web, "app/deletefile?file=", 2, http_reponse_deletefile, NULL),
    addWeb(web, "app/getadasitems", 2, http_reponse_getadasitems, NULL),
    addWeb(web, "app/getadasvalue", 2, http_reponse_getadasvalue, NULL),

    //搜索到url为NULL代表结束
    addWeb(web, NULL, 1, http_send_error, NULL),
};

static uint32_t json_setsuccess(cJSON **root2, char **post_content)
{
    *root2 = cJSON_CreateObject();
    cJSON_AddNumberToObject(*root2, "result", 0);
    cJSON_AddStringToObject(*root2, "info", "success.");
    *post_content = cJSON_PrintUnformatted(*root2);
    _os_printf("postcontent: %s\r\n", *post_content);
    _os_printf("postlen: %d\r\n", strlen(*post_content));
    return strlen(*post_content);
}

static void http_reponse_success(struct httpClient *httpClient)
{
    if(httpClient == NULL)
	{
		return;
	}
        
	cJSON *root = NULL;
    char *post_content = NULL;
    uint32_t sendlen = 0;
    struct httpresp *head;
    int fd = httpClient->fdClient;

    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Connection, "close");
    http_add_header(head, httpIndex.Type, "application/json");
    sendlen = json_setsuccess(&root, &post_content);
    http_header_send(head, httpIndex.Length, sendlen, fd);
    send(fd, post_content, sendlen, 0);
    closeRes(httpClient);
    cJSON_free(post_content);
    cJSON_Delete(root);
}

static uint32_t json_setunsupport(cJSON **root2, char **post_content)
{
    *root2 = cJSON_CreateObject();
    cJSON_AddNumberToObject(*root2, "result", 98);
    cJSON_AddStringToObject(*root2, "info", "unsupport.");
    *post_content = cJSON_PrintUnformatted(*root2);
    _os_printf("postcontent: %s\r\n", *post_content);
    _os_printf("postlen: %d\r\n", strlen(*post_content));
    return strlen(*post_content);
}

static void http_reponse_unsupport(struct httpClient *httpClient)
{
	cJSON *root = NULL;
    char *post_content = NULL;
    uint32_t sendlen = 0;
    struct httpresp *head;
    int fd = httpClient->fdClient;

    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Connection, "close");
    http_add_header(head, httpIndex.Type, "application/json");
    sendlen = json_setunsupport(&root, &post_content);
    http_header_send(head, httpIndex.Length, sendlen, fd);
    send(fd, post_content, sendlen, 0);
    
    closeRes(httpClient);
    cJSON_free(post_content);
    cJSON_Delete(root);
}

static uint32_t json_batteryinfo(cJSON **root2, char **post_content2)
{
    cJSON *root = NULL;
    root = cJSON_CreateObject();
    *root2 = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "capacity", 100);
    cJSON_AddNumberToObject(root, "charge", 1); // 0:未充电 1:充电中

    cJSON_AddNumberToObject(*root2, "result", 0);
    cJSON_AddItemToObject(*root2, "info", root);
    *post_content2 = cJSON_PrintUnformatted(*root2);
    _os_printf("postcontent: %s\r\n", *post_content2);
    _os_printf("postlen: %d\r\n", strlen(*post_content2));
    return strlen(*post_content2);
}

static void http_reponse_getbatteryinfo(struct httpClient *httpClient)
{
    cJSON *root = NULL;
    char *post_content = NULL;
    uint32_t sendlen = 0;
    struct httpresp *head;
    int fd = httpClient->fdClient;

    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Connection, "close");
    http_add_header(head, httpIndex.Type, "application/json");

    sendlen = json_batteryinfo(&root, &post_content);

    http_header_send(head, httpIndex.Length, sendlen, fd);
    send(fd, post_content, sendlen, 0);

    closeRes(httpClient);
    cJSON_free(post_content);
    cJSON_Delete(root);
}

static uint32_t json_productinfo(cJSON **root2, char **post_content2)
{
    cJSON *root = NULL;
    root = cJSON_CreateObject();
    *root2 = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", "recorder");
    cJSON_AddStringToObject(root, "company", "taixin");
    cJSON_AddStringToObject(root, "soc", "taixin");
    cJSON_AddStringToObject(root, "sp", "taixin");

    cJSON_AddNumberToObject(*root2, "result", 0);
    cJSON_AddItemToObject(*root2, "info", root);

    *post_content2 = cJSON_PrintUnformatted(*root2);
    _os_printf("postcontent: %s\r\n", *post_content2);
    _os_printf("postlen: %d\r\n", strlen(*post_content2));
    return strlen(*post_content2);
}

static void http_reponse_getproductinfo(struct httpClient *httpClient)
{
    cJSON *root = NULL;
    char *post_content = NULL;
    uint32_t sendlen = 0;
    struct httpresp *head;
    int fd = httpClient->fdClient;

    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Connection, "close");
    http_add_header(head, httpIndex.Type, "application/json");

    sendlen = json_productinfo(&root, &post_content);

    http_header_send(head, httpIndex.Length, sendlen, fd);
    send(fd, post_content, sendlen, 0);

    closeRes(httpClient);
    cJSON_free(post_content);
    cJSON_Delete(root);
}

static uint32_t json_deviceattr(cJSON **root2, char **post_content)
{
    cJSON *root = NULL;
    char ssid[32] = {0};
    char mac[13] = {0};
    os_sprintf(ssid, "%s", sys_cfgs.ssid);
    os_sprintf(mac, "%02X%02X%02X%02X%02X%02X", sys_cfgs.mac[0], sys_cfgs.mac[1], sys_cfgs.mac[2], sys_cfgs.mac[3], sys_cfgs.mac[4], sys_cfgs.mac[5]);
    
    // 进入录风者默认是主摄出图
    items_value_process("switchcam", 0, SET_ITEMS_VALUE);

    root = cJSON_CreateObject();
    *root2 = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "uuid", mac);
    cJSON_AddStringToObject(root, "softver", "v0.0.8");
    cJSON_AddStringToObject(root, "otaver", "v1.20260119.0");
    cJSON_AddStringToObject(root, "hwver", "v1.2");
    cJSON_AddStringToObject(root, "ssid", ssid);
    cJSON_AddStringToObject(root, "bssid", mac);
    cJSON_AddNumberToObject(root, "camnum", items_value_process("cameranum", 0, GET_ITEMS_VALUE));
    cJSON_AddNumberToObject(root, "curcamid", items_value_process("switchcam", 0, GET_ITEMS_VALUE));
    cJSON_AddNumberToObject(root, "wifireboot", 0);

    cJSON_AddNumberToObject(*root2, "result", 0);
    cJSON_AddItemToObject(*root2, "info", root);

    *post_content = cJSON_PrintUnformatted(*root2);
    _os_printf("postcontent: %s\r\n", *post_content);
    _os_printf("postlen: %d\r\n", strlen(*post_content));
    return strlen(*post_content);
}

static void http_reponse_getdeviceattr(struct httpClient *httpClient)
{
    cJSON *root = NULL;
    char *post_content = NULL;
    uint32_t sendlen = 0;
    struct httpresp *head;
    int fd = httpClient->fdClient;

    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Connection, "close");
    http_add_header(head, httpIndex.Type, "application/json");
    sendlen = json_deviceattr(&root, &post_content);

    http_header_send(head, httpIndex.Length, sendlen, fd);
    send(fd, post_content, sendlen, 0);

    closeRes(httpClient);
    cJSON_free(post_content);
    cJSON_Delete(root);
}

void rec_lock_file(char *file_name, char *file_path)
{
#if APP_LOCK_VIDEO_EN
    if(file_name == NULL || file_path == NULL)
    {
        os_printf("file_name or file_path is NULL\r\n");
        return;
    }

    uint8_t video_num = items_value_process("lockvideo", 0, GET_ITEMS_VALUE);
    if(video_num)
    {
        int res = 0;
        char new_file_path[64];
        char *last_slash = os_strrchr(file_path, '/');
        int front_len = last_slash - file_path;
        os_snprintf(new_file_path, sizeof(new_file_path), "%.*s/%s%s", front_len, file_path, SOS_PREFIX, file_name);

        res = osal_rename(file_path, new_file_path);
        if(res != FR_OK) {
            os_printf("lock %s failed, res: %d\r\n", file_path, res);
        } else {
            os_printf("lock success, rename file %s to %s\r\n", file_path, new_file_path);
            osal_fchmod(new_file_path, AM_RDO, AM_RDO);
        }
        items_value_process("lockvideo", --video_num, SET_ITEMS_VALUE);
    }
#endif
}

void open_audio(struct dev_obj *audio_dev)
{
    os_printf("%s\n", __FUNCTION__);
    struct dev_hotplug_info *hotplug_info = (struct dev_hotplug_info *) audio_dev->info;
    txAudioInfo_t *audio_info = (txAudioInfo_t *) hotplug_info->priv;
    txAudioInfo_t codec_info;
    struct msi *aac_msi = NULL;
    uint8_t aac_init = 0;
    for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++)
    {
        if(g_cam_configs[i].msi && !g_cam_configs[i].aac_msi)
        {
            if(g_cam_configs[i].type == MP4)
            {
                if(!aac_init)
                {
                    codec_info.sample_rate = audio_info->sample_rate;
                    codec_info.channels = audio_info->channels;
                    codec_info.frame_size = 1024;
                    aac_msi = aenc_get_msi(AUDIO_CODEC_AAC, "adc", &codec_info);
                }
                if(aac_msi)
                {
                    aac_init = 1;
                    g_cam_configs[i].aac_msi = aac_msi;
                    auadc_msi_add_output(AUSYS_AUAD, aac_msi->name);
                    msi_do_cmd(g_cam_configs[i].aac_msi, MSI_CMD_START, 0, 0);
                    msi_add_output(g_cam_configs[i].aac_msi, NULL, g_cam_configs[i].msi, NULL);
                }
            }
            else if(g_cam_configs[i].type == AVI)
            {
                g_cam_configs[i].aac_msi = NULL;
                auadc_msi_add_output(AUSYS_AUAD, g_cam_configs[i].msi->name);
            }
        }
    }
}

void close_audio(void)
{
    os_printf("%s\n", __FUNCTION__);
    struct msi *aac_msi = NULL;
    for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++)
    {
        if(g_cam_configs[i].msi)
        {
            if(g_cam_configs[i].aac_msi && g_cam_configs[i].type == MP4)
            {
                msi_del_output(g_cam_configs[i].aac_msi, NULL, g_cam_configs[i].msi, NULL);
                aac_msi = g_cam_configs[i].aac_msi;
                g_cam_configs[i].aac_msi = NULL;
            }
            else if(g_cam_configs[i].type == AVI)
            {
                auadc_msi_del_output(AUSYS_AUAD, g_cam_configs[i].msi->name);
            }
        }
    }

    if(aac_msi)
    {
        msi_put(aac_msi);
        aac_msi = NULL;
    }
}

static int32 get_dev_cb(const struct dev_obj *dev, void *arg)
{
    struct dev_obj **dev_param = (struct dev_obj **)arg;
    if(dev && dev_param)
    {
        *dev_param = (struct dev_obj *)dev;
    }

    return 1;
}

void open_rec(void)
{
    os_printf("%s\n", __FUNCTION__);

    uint8_t audio_en = 0;
    struct dev_obj *audio_dev = NULL;
    dev_walk(DEV_TYPE_MIC, get_dev_cb, (void *)&audio_dev);
    if(audio_dev)
    {
        audio_en = items_value_process("mic", 0, GET_ITEMS_VALUE);
    }

#if APP_LOCK_VIDEO_EN
    items_value_process("lockvideo", 0, SET_ITEMS_VALUE);
#endif
    int rec_split_duration = items_value_process("rec_split_duration", 0, GET_ITEMS_VALUE);
    

    /* 多路录卡时判断是否需要加写卡锁 */
    uint8_t rec_num = 0;
    for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++)
    {
        if(g_cam_configs[i].enable)
        {
            rec_num++;
        }
    }

    for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++)
    {
        if((g_cam_configs[i].mask & CAM_MASK_ENABLE) && g_cam_configs[i].enable && !g_cam_configs[i].msi)
        {
            if(!g_cam_configs[i].src_msi)
            {
                g_cam_configs[i].src_msi = msi_find(g_cam_configs[i].src_msi_name, 1);
                if(g_cam_configs[i].src_msi)
                {
					if(rec_num > 1)
					{
						g_cam_configs[i].file_process.need_lock = 1;
					}
					else
					{
						g_cam_configs[i].file_process.need_lock = 0;
					}
                    os_printf("open src msi: %s success\r\n", g_cam_configs[i].src_msi_name);
                    
                    struct video_record_cfg rec_cfg = {
                        .name = g_cam_configs[i].msi_name,
                        .srcID = g_cam_configs[i].srcID,
                        .filter = g_cam_configs[i].filter_type,
                        .rec_time = rec_split_duration,
                        .audio_en = audio_en,
                        .mode = g_cam_configs[i].mode,
                        .video_fps = 25,
                        .file_process = &g_cam_configs[i].file_process,
                    };
                    g_cam_configs[i].msi = g_cam_configs[i].create_func(&rec_cfg);

                    if(g_cam_configs[i].msi)
                    {
                        os_printf("open msi: %s success\r\n", g_cam_configs[i].msi->name);
                        msi_add_output(g_cam_configs[i].src_msi, NULL, g_cam_configs[i].msi, NULL);
                    }
                    else
                    {
                        os_printf("open msi: %s fail\r\n", g_cam_configs[i].msi_name);
                        msi_put(g_cam_configs[i].src_msi);
                        g_cam_configs[i].src_msi = NULL;
                    }
                }
            }
        }
    }

    if(audio_en)
    {
        open_audio(audio_dev);
    }

    for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++)
    {
        if(g_cam_configs[i].msi)
        {
            msi_do_cmd(g_cam_configs[i].msi, MSI_CMD_MEDIA_CTRL, MSI_MEDIA_CTRL_RECORD_START, 0);
        }
    }
}

void close_rec(void)
{
    os_printf("%s\n", __FUNCTION__);

    for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++)
    {
        if(g_cam_configs[i].msi)
        {
            g_cam_configs[i].msi->enable = 0;
            msi_do_cmd(g_cam_configs[i].msi, MSI_CMD_PRE_DESTROY, 0, 0);
            if(g_cam_configs[i].src_msi)
            {
                msi_del_output(g_cam_configs[i].src_msi, NULL, g_cam_configs[i].msi, NULL);
            }
        }
    }

    close_audio();

    for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++)
    {
        if(g_cam_configs[i].msi)
        {
            msi_destroy(g_cam_configs[i].msi);
            g_cam_configs[i].msi = NULL;
            if(g_cam_configs[i].src_msi)
            {
                msi_put(g_cam_configs[i].src_msi);
                g_cam_configs[i].src_msi = NULL;
            }
            os_printf("close msi: %s, src msi: %s\r\n", g_cam_configs[i].msi_name, g_cam_configs[i].src_msi_name);
        }
    }
}

static uint8_t rec_get_running_status(uint8_t *has_msi)
{
    uint8_t rec_status = 1;
    uint32_t rec_status_g = 0;
    *has_msi = 0;

    for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++)
    {
        if(g_cam_configs[i].msi)
        {
            *has_msi = 1;
            msi_do_cmd(g_cam_configs[i].msi, MSI_CMD_GET_RUNNING, (uint32_t)&rec_status_g, 0);
            rec_status &= rec_status_g ? 1 : 0;
        }
    }

    return *has_msi ? rec_status : 0;
}

/* 以第一个 type 类型的录卡时间为准 */
static uint32_t rec_get_duration(uint8_t type)
{
    uint32_t rec_second = 0;
    for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++)
    {
        if(g_cam_configs[i].msi && g_cam_configs[i].type == type)
        {
            msi_do_cmd(g_cam_configs[i].msi, MSI_CMD_MEDIA_CTRL, MSI_MEDIA_CTRL_GET_RECTIME, (uint32_t)&rec_second);
            return rec_second;
        }
    }
    return 0;
}

int32_t sd_ready(const struct dev_obj *dev,void *arg)
{
    uint8_t *ready = arg;
    if(ready)
    {
        *ready = 1;
    }
    return 1;
}

static uint32_t json_rec_status(cJSON **root2, char **post_content)
{
    cJSON *root = NULL;
    root = cJSON_CreateObject();
    *root2 = cJSON_CreateObject();
    uint8_t rec_status = 0;
	uint8_t sd_status = 0;
    uint8_t has_msi = 0;

#if FS_EN
    dev_walk(DEV_TYPE_SD, sd_ready, &sd_status);
#endif

    if (sd_status == 0)
    {
        rec_status = 0;
    }
    else
    {
        rec_status = rec_get_running_status(&has_msi);
        if (has_msi && !rec_status)
        {
            close_rec();
        }
    }

    items_value_process("rec", rec_status, SET_ITEMS_VALUE);
    cJSON_AddNumberToObject(root, "value", rec_status);
    cJSON_AddNumberToObject(*root2, "result", 0);
    cJSON_AddItemToObject(*root2, "info", root);
    *post_content = cJSON_PrintUnformatted(*root2);
    _os_printf("postcontent: %s\r\n", *post_content);
    _os_printf("postlen: %d\r\n", strlen(*post_content));
    return strlen(*post_content);
}

static void http_getparamvalue_rec(struct httpClient *httpClient)
{
    cJSON *root = NULL;
    char *post_content = NULL;
    uint32_t sendlen = 0;
    struct httpresp *head;
    int fd = httpClient->fdClient;

    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Connection, "close");
    http_add_header(head, httpIndex.Type, "application/json");

    sendlen = json_rec_status(&root, &post_content);
    http_header_send(head, httpIndex.Length, sendlen, fd);
    send(fd, post_content, sendlen, 0);

    closeRes(httpClient);
    cJSON_free(post_content);
    cJSON_Delete(root);
}

static void http_getparamvalue_car_plate(struct httpClient *httpClient)
{
	http_reponse_unsupport(httpClient);
}

static uint32_t json_all_status(cJSON **root2, char **post_content)
{
    cJSON *root = NULL;
	cJSON *item = NULL;
    *root2 = cJSON_CreateObject();

	// 创建info数组
    root = cJSON_CreateArray();

    // 遍历配置项，创建数组元素
    for (int i = 0; i < ITEMS_CONFIG_COUNT; i++) 
    {
        if(items_cfg[i].const_item->enable == ITEM_ENABLE)
        {
            item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "name", items_cfg[i].const_item->name);
            cJSON_AddNumberToObject(item, "value", items_cfg[i].value);
            cJSON_AddItemToArray(root, item);
        }
    }
    cJSON_AddNumberToObject(*root2, "result", 0);
    cJSON_AddItemToObject(*root2, "info", root);
    *post_content = cJSON_PrintUnformatted(*root2);
    _os_printf("postcontent: %s\r\n", *post_content);
    _os_printf("postlen: %d\r\n", strlen(*post_content));
    return strlen(*post_content);
}

static void http_getparamvalue_all(struct httpClient *httpClient)
{
	cJSON *root = NULL;
    char *post_content = NULL;
	uint32_t sendlen = 0;
    struct httpresp *head;
    int fd = httpClient->fdClient;

    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Connection, "close");
    http_add_header(head, httpIndex.Type, "application/json");

    sendlen = json_all_status(&root, &post_content);
	
    http_header_send(head, httpIndex.Length, sendlen, fd);
    send(fd, post_content, sendlen, 0);

    closeRes(httpClient);
    cJSON_free(post_content);
    cJSON_Delete(root);
}

static void http_reponse_getparamvalue(struct httpClient *httpClient)
{
	char *param_start, *params;
	int param_size = 0;

    // 查找参数字符串起始位置
    param_start = strstr(httpClient->pattern, "param=");
    if(param_start) {
        param_start += strlen("param=");
    }
    
    if(param_start == NULL) {
        os_printf("%s: %d, request error\r\n", __FUNCTION__, __LINE__);
        closeRes(httpClient);
        return;
    }

	param_size = strlen(param_start);
	params = STREAM_LIBC_MALLOC(param_size + 1);
	if (params == NULL) {
        os_printf("FUNCTION:%s Line:%d, STREAM_LIBC_MALLOC failed\r\n", __FUNCTION__, __LINE__);
        return;
	}
    strncpy(params, param_start, param_size);
    params[param_size] = '\0';

    // 根据param值调用对应函数
    if (strcmp(params, "rec") == 0) {
        http_getparamvalue_rec(httpClient);
    }
    else if (strcmp(params, "car_plate") == 0) {
        // 车牌识别 不支持
        http_getparamvalue_car_plate(httpClient);
    }
    else if (strcmp(params, "all") == 0) {
        http_getparamvalue_all(httpClient);
    }
    else {
        closeRes(httpClient);
        os_printf("Unknown parameter: %s\r\n", params);
    }
	STREAM_LIBC_FREE(params);
}

static uint32_t json_mediainfo(cJSON **root2, char **post_content2)
{
    char *ip8;
    cJSON *root = NULL;
    char rtsp_path[100];
    char path_for_file[32];
    ip8 = (char*)&sys_cfgs.ipaddr;

    uint8_t video_num = items_value_process("switchcam", 0, GET_ITEMS_VALUE);
    uint8_t camera_num = items_value_process("cameranum", 0, GET_ITEMS_VALUE);
    //设置超过范围,则直接配置第一个摄像头
    if(video_num >= camera_num)
    {
        video_num = 0;
    }

    struct media_s *media = NULL;
    for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++)
    {
        if((g_cam_configs[i].mask & CAM_MASK_ENABLE) && g_cam_configs[i].enable)
        {
            if(video_num == 0)
            {
                media = &g_cam_configs[i].media;
                break;
            }
            video_num--;
        }
    }

    root = cJSON_CreateObject();
    *root2 = cJSON_CreateObject();

    if(media)
    {
        memset(rtsp_path, 0, sizeof(rtsp_path));
        memset(path_for_file, 0, sizeof(path_for_file));
        snprintf(path_for_file, sizeof(path_for_file), "%s", media->rtsp_url);
        snprintf(rtsp_path, sizeof(rtsp_path), "rtsp://%d.%d.%d.%d/%s", ip8[0], ip8[1], ip8[2], ip8[3], path_for_file);
        cJSON_AddStringToObject(root, "rtsp", rtsp_path);
        cJSON_AddStringToObject(root, "transport", media->tran_mode);
        cJSON_AddNumberToObject(*root2, "result", 0);
    }
    else
    {
        _os_printf("media is null\n");
        cJSON_AddNumberToObject(*root2, "result", 99);
    }

    cJSON_AddNumberToObject(root, "port", PUSH_MESSAGE_PORT);
    cJSON_AddNumberToObject(root, "rectime", 1);
    cJSON_AddItemToObject(*root2, "info", root);

    *post_content2 = cJSON_PrintUnformatted(*root2);
    _os_printf("postcontent: %s\r\n", *post_content2);
    _os_printf("postlen: %d\r\n", strlen(*post_content2));
    return strlen(*post_content2);
}

static void http_reponse_getmediainfo(struct httpClient *httpClient)
{
    cJSON *root = NULL;
    char *post_content = NULL;
    uint32_t sendlen = 0;
    struct httpresp *head;
    int fd = httpClient->fdClient;

    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Connection, "close");
    http_add_header(head, httpIndex.Type, "application/json");

    sendlen = json_mediainfo(&root, &post_content);

    http_header_send(head, httpIndex.Length, sendlen, fd);
    send(fd, post_content, sendlen, 0);
    closeRes(httpClient);
    cJSON_free(post_content);
    cJSON_Delete(root);
}

uint32_t json_capability(cJSON **root2, char **post_content2)
{
    cJSON *root = NULL;
    root = cJSON_CreateObject();
    *root2 = cJSON_CreateObject();
    if(PARKING_MOR_ALBUM_EN) {
        if(APP_LOCK_VIDEO_EN) {
            cJSON_AddStringToObject(root, "value", "000101000000010");
        } else {
            cJSON_AddStringToObject(root, "value", "000001000000010");
        }
    } else {
        if(APP_LOCK_VIDEO_EN) {
            cJSON_AddStringToObject(root, "value", "001100000000010");
        } else {
            cJSON_AddStringToObject(root, "value", "001000000000010");
        }
    }

    cJSON_AddNumberToObject(*root2, "result", 0);
    cJSON_AddItemToObject(*root2, "info", root);

    *post_content2 = cJSON_PrintUnformatted(*root2);
    _os_printf("postcontent: %s\r\n", *post_content2);
    _os_printf("postlen: %d\r\n", strlen(*post_content2));
    return strlen(*post_content2);
}

static void http_reponse_capability(struct httpClient *httpClient)
{
    cJSON *root = NULL;
    char *post_content = NULL;
    uint32_t sendlen = 0;
    struct httpresp *head;
    int fd = httpClient->fdClient;

    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Connection, "close");
    http_add_header(head, httpIndex.Type, "application/json");

    sendlen = json_capability(&root, &post_content);

    http_header_send(head, httpIndex.Length, sendlen, fd);
    send(fd, post_content, sendlen, 0);
    closeRes(httpClient);
    cJSON_free(post_content);
    cJSON_Delete(root);
}

static int recorder_set_timezone_offset(int offset_hour)
{
    timezone_info_t tz = {0};

    if (offset_hour < -12 || offset_hour > 14) {
        return -1;
    }

    tz.std_offset_min = offset_hour * 60;
    tz.dst_offset_min = 0;
    tz.is_dst_active  = 0;
    tz.dst_rule_type  = DST_RULE_NONE;

    return timezone_set_custom(&tz);
}

static void http_reponse_settimezone(struct httpClient *httpClient)
{
    cJSON *root = NULL;
    char *post_content = NULL;
    uint32_t sendlen = 0;
    struct httpresp *head;
    int fd = httpClient->fdClient;

    int timezone = atoi(httpClient->pattern);
    
    timezone = atoi(httpClient->pattern);

    if (recorder_set_timezone_offset(timezone) != 0) {
        _os_printf("invalid timezone: %s\r\n", httpClient->pattern);
        closeRes(httpClient);
        return;
    }
    items_value_process("timezone", timezone, SET_ITEMS_VALUE);
    _os_printf("set timezone: %d\r\n", timezone);

    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Connection, "close");
    http_add_header(head, httpIndex.Type, "application/json");

    sendlen = json_setsuccess(&root, &post_content);

    http_header_send(head, httpIndex.Length, sendlen, fd);
    send(fd, post_content, sendlen, 0);

    closeRes(httpClient);
    cJSON_free(post_content);
    cJSON_Delete(root);
}

time_t timeToSeconds(const char* timeStr) 
{
    struct tm tm = {0};
    int year, month, day, hour, minute, second;
    uint8_t ret = 0;
    
    // 解析时间字符串
    ret = sscanf(timeStr, "%4d%2d%2d%2d%2d%2d", 
                 &year, &month, &day, &hour, &minute, &second);
                 
    if(ret != 6)
    {
        return 0;
    }
    
    // 设置tm结构体
    tm.tm_year = year - 1900;  // 年份是从1900开始的
    tm.tm_mon = month - 1;     // 月份是0-11
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tm.tm_isdst = -1;          // 让系统自动判断夏令时
    
    // 转换为时间戳（秒数）
    time_t timestamp = mktime(&tm);

    return timestamp;
}

static void http_reponse_setsystime(struct httpClient *httpClient)
{
    cJSON *root = NULL;
    char *post_content = NULL;
    uint32_t sendlen = 0;
    struct httpresp *head;
    int fd = httpClient->fdClient;
    struct timeval ptimeval;
    time_t local_time = 0;
    time_t utc_time = 0;

    local_time = timeToSeconds(httpClient->pattern);
    if(local_time == 0)
    {
        _os_printf("params error: %s\r\n", httpClient->pattern);
        closeRes(httpClient);
        return;
    }
    timezone_local_to_utc(local_time, &utc_time);
    ptimeval.tv_sec = utc_time;
    ptimeval.tv_usec = 0;
    settimeofday(&ptimeval, NULL);

    _os_printf("set systime: %s\r\n", httpClient->pattern);

    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Connection, "close");
    http_add_header(head, httpIndex.Type, "application/json");

    sendlen = json_setsuccess(&root, &post_content);

    http_header_send(head, httpIndex.Length, sendlen, fd);
    send(fd, post_content, sendlen, 0);

    closeRes(httpClient);
    cJSON_free(post_content);
    cJSON_Delete(root);
}

static void http_reponse_enterrecorder(struct httpClient *httpClient)
{
    // 进入录风者界面后开始录像，可根据需要修改
    if(items_value_process("rec" , 0, GET_ITEMS_ENABLE) == ITEM_ENABLE) {
        if(items_value_process("rec", 0, GET_ITEMS_DEFAULT_VALUE)) {
            open_rec();
            items_value_process("rec", 1, SET_ITEMS_VALUE);
        }
    }
    http_reponse_success(httpClient);
}

static void http_reponse_exitrecorder(struct httpClient *httpClient)
{
    int saveflag = 0;
    saveflag = items_value_process("saveflag", 0, GET_ITEMS_VALUE);
    if(saveflag) {
        itemcfg_save(1);
        saveflag = 0;
        items_value_process("saveflag", saveflag, SET_ITEMS_VALUE);
    }
	http_reponse_success(httpClient);
}

static uint32_t json_recduration(cJSON **root2, char **post_content)
{
    cJSON *root = NULL;
    root = cJSON_CreateObject();
    *root2 = cJSON_CreateObject();
    uint32_t rec_second = 0;

    // 默认以第一个镜头的录像时间作为当前录卡的持续时间
    for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++)
    {
        if(g_cam_configs[i].msi)
        {
            rec_second = rec_get_duration(g_cam_configs[i].type);
            break;
        }
    }
    
    cJSON_AddNumberToObject(root, "duration", rec_second);
    cJSON_AddNumberToObject(*root2, "result", 0);
    cJSON_AddItemToObject(*root2, "info", root);

    *post_content = cJSON_PrintUnformatted(*root2);
    _os_printf("postcontent: %s\r\n", *post_content);
    _os_printf("postlen: %d\r\n", strlen(*post_content));
    return strlen(*post_content);
}

static void http_reponse_getrecduration(struct httpClient *httpClient)
{
	cJSON *root = NULL;
    char *post_content = NULL;
    uint32_t sendlen = 0;
    struct httpresp *head;
    int fd = httpClient->fdClient;

    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Connection, "close");
    http_add_header(head, httpIndex.Type, "application/json");
    sendlen = json_recduration(&root, &post_content);

    http_header_send(head, httpIndex.Length, sendlen, fd);
    send(fd, post_content, sendlen, 0);

    closeRes(httpClient);
    cJSON_free(post_content);
    cJSON_Delete(root);
}

static void http_reponse_playback(struct httpClient *httpClient)
{
	char *params;

    // 查找参数字符串起始位置
    params = strstr(httpClient->pattern, "param=");
    if(params) {
        params += strlen("param=");
    }

    // 根据param值调用对应函数
    if (strcmp(params, "enter") == 0) {
		_os_printf("playback enter\r\n");
    }
    else if (strcmp(params, "exit") == 0) {
        _os_printf("playback exit\r\n");
        // 退出回放界面后打开录像，可根据需要修改
        // open_rec();
        // items_value_process("rec", 1, SET_ITEMS_VALUE);
    }
    else {
        _os_printf("unknown parameter: %s\n", params);
        closeRes(httpClient);
		return;
    }

	http_reponse_success(httpClient);
}

static void http_reponse_getadasitems(struct httpClient *httpClient)
{
    http_reponse_unsupport(httpClient);
}

static void http_reponse_getadasvalue(struct httpClient *httpClient)
{
    http_reponse_unsupport(httpClient);
}

static void http_reponse_lockvideo(struct httpClient *httpClient)
{
#if APP_LOCK_VIDEO_EN
    if (items_value_process("lockvideo", 0, GET_ITEMS_VALUE)) {
        items_value_process("lockvideo", 0, SET_ITEMS_VALUE);
    } else {
        uint8_t video_num = 0;
        for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++)
        {
            if((g_cam_configs[i].mask & CAM_MASK_ENABLE) && g_cam_configs[i].msi)
            {
                video_num++;
            }
        }
        items_value_process("lockvideo", video_num, SET_ITEMS_VALUE);
    }
    http_reponse_success(httpClient);
#else
    http_reponse_unsupport(httpClient);
#endif
}

static uint32_t json_lockvideostatus(cJSON **root2, char **post_content2)
{
    cJSON *root = NULL;
    root = cJSON_CreateObject();
    *root2 = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "status", items_value_process("lockvideo", 0, GET_ITEMS_VALUE));

    cJSON_AddNumberToObject(*root2, "result", 0);
    cJSON_AddItemToObject(*root2, "info", root);

    *post_content2 = cJSON_PrintUnformatted(*root2);
    _os_printf("postcontent: %s\r\n", *post_content2);
    _os_printf("postlen: %d\r\n", strlen(*post_content2));
    return strlen(*post_content2);
}

static void http_reponse_getlockvideostatus(struct httpClient *httpClient)
{
#if APP_LOCK_VIDEO_EN
    cJSON *root = NULL;
    char *post_content = NULL;
    uint32_t sendlen = 0;
    struct httpresp *head;
    int fd = httpClient->fdClient;

    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Connection, "close");
    http_add_header(head, httpIndex.Type, "application/json");

    sendlen = json_lockvideostatus(&root, &post_content);

    http_header_send(head, httpIndex.Length, sendlen, fd);
    send(fd, post_content, sendlen, 0);
    closeRes(httpClient);
    cJSON_free(post_content);
    cJSON_Delete(root);
#else
	http_reponse_unsupport(httpClient);
#endif
}

static void http_reponse_getstorageinfo(struct httpClient *httpClient)
{
    http_reponse_unsupport(httpClient);
}

static void http_reponse_setting(struct httpClient *httpClient)
{
	char *params;
    // 查找参数字符串起始位置
    params = strstr(httpClient->pattern, "param=");
    if(params) {
        params += strlen("param=");
    }

    // 根据param值调用对应函数
    if (strcmp(params, "enter") == 0) {
		_os_printf("setting enter\r\n");
    }
    else if (strcmp(params, "exit") == 0) {
        int saveflag = 0;
        saveflag = items_value_process("saveflag", 0, GET_ITEMS_VALUE);
        if(saveflag) {
            itemcfg_save(1);
            saveflag = 0;
            items_value_process("saveflag", saveflag, SET_ITEMS_VALUE);
        }
        _os_printf("setting exit\r\n");
        // 退出设置界面后打开录像，可根据需要修改
        // open_rec();
        // items_value_process("rec", 1, SET_ITEMS_VALUE);
    }
    else {
        _os_printf("Unknown parameter: %s\n", params);
        closeRes(httpClient);
		return;
    }

	http_reponse_success(httpClient);
}

static void set_rec(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 录像
	if(value == 0)
	{
		// 关闭录像
		_os_printf("rec close\r\n");
        close_rec();
	}
	else if(value == 1)
	{
		// 打开录像
        _os_printf("rec open\r\n");
        open_rec();
	}
	else
	{
		_os_printf("set rec error!!!\r\n");
        closeRes(httpClient);
        return;
	}

	item->value = value;
    http_reponse_success(httpClient);
}

static void set_default(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    item->value = value;
    http_reponse_success(httpClient);
}

static void set_mic(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 录像声音
	if(value == 0)
	{
		// 关闭录像声音
		_os_printf("mic close\r\n");
	}
	else if(value == 1)
	{
		// 打开录像声音
		_os_printf("mic open\r\n");
    }
	else
	{
		_os_printf("set mic error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_osd(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 时间水印
	if(value == 0)
	{
		// 关闭时间水印
		_os_printf("set osd close\r\n");

        for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++)
        {
            if((g_cam_configs[i].mask & CAM_MASK_ENABLE) && g_cam_configs[i].enable && g_cam_configs[i].src_type == MIPI)
            {
                struct vpp_device *vpp_dev;
                vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
                vpp_set_watermark0_enable(vpp_dev,value);
        	    // vpp_set_watermark1_enable(vpp_dev,value);
            }
            // else if((g_cam_configs[i].mask & CAM_MASK_ENABLE) && g_cam_configs[i].enable && g_cam_configs[i].src_type == USB)
            // {
				
            // }
        }
	}
	else if(value == 1)
	{
		// 打开时间水印
		_os_printf("set osd open\r\n");

        for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++)
        {
            if((g_cam_configs[i].mask & CAM_MASK_ENABLE) && g_cam_configs[i].enable && g_cam_configs[i].src_type == MIPI)
            {
                struct vpp_device *vpp_dev;
                vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
                vpp_set_watermark0_enable(vpp_dev,value);
        	    // vpp_set_watermark1_enable(vpp_dev,value);
            }
            // else if((g_cam_configs[i].mask & CAM_MASK_ENABLE) && g_cam_configs[i].enable && g_cam_configs[i].src_type == USB)
            // {
				
            // }
        }
	}
	else
	{
		_os_printf("set osd error!!!\r\n");
        closeRes(httpClient);
		return;
	}

	item->value = value;
	http_reponse_success(httpClient);
}

static void set_rec_resolution(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 录像分辨率
	if(value == 0)
	{
		// 2K
		_os_printf("set rec resolution 2K\r\n");
	}
	else if(value == 1)
	{
		// 4K
		_os_printf("set rec resolution 4K\r\n");
	}
	else
	{
		_os_printf("set mic error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_rec_split_duration(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 录像文件时长	 {"off", "1MIN", "2MIN", "3MIN"}
	switch(value)
	{
		case 0:
			_os_printf("set rec split duration off\r\n");
			break;
		case 1:
			_os_printf("set rec split duration 1MIN\r\n");
			break;
		case 2:
			_os_printf("set rec split duration 2MIN\r\n");
			break;
		case 3:
			_os_printf("set rec split duration 3MIN\r\n");
			break;
		default:
			_os_printf("set rec split duration error!!!\r\n");
            closeRes(httpClient);
			return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_encodec(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 录像编码器  {"h264", "h265"}
    if(value == 0)
	{
		_os_printf("set encodec h264\r\n");
	}
	else if(value == 1)
	{
		_os_printf("set encodec h265\r\n");
	}
	else
	{
		_os_printf("set encodec error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_speaker(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 扬声器音量   {"off", "lowest", "lower", "low", "middle", "high", "very high"}
    switch(value)
	{
		case 0:
			_os_printf("set speaker off\r\n");
			break;
		case 1:
			_os_printf("set speaker lowest\r\n");
			break;
		case 2:
			_os_printf("set speaker lower\r\n");
			break;
		case 3:
			_os_printf("set speaker low\r\n");
			break;
		case 4:
			_os_printf("set speaker middle\r\n");
			break;
		case 5:
			_os_printf("set speaker high\r\n");
			break;
		case 6:
			_os_printf("set speaker very high\r\n");
            break;
    	default:
    		_os_printf("set speaker error!!!\r\n");
            closeRes(httpClient);
    		return;
	}
    item->value = value;
	http_reponse_success(httpClient);
}

static void set_gsr_sensitivity(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 碰撞感应  {"off", "lowest", "lower", "low", "middle", "high"}
    switch(value)
	{
		case 0:
			_os_printf("set gsr sensitivity off\r\n");
			break;
		case 1:
			_os_printf("set gsr sensitivity lowest\r\n");
			break;
		case 2:
			_os_printf("set gsr sensitivity lower\r\n");
			break;
		case 3:
			_os_printf("set gsr sensitivity low\r\n");
			break;
        case 4:
			_os_printf("set gsr sensitivity middle\r\n");
			break;
		case 5:
			_os_printf("set gsr sensitivity high\r\n");
			break;
		default:
			_os_printf("set gsr sensitivity error!!!\r\n");
            closeRes(httpClient);
			return;
	}

    item->value = value;
    http_reponse_success(httpClient);
}

static void set_language(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 记录仪语言	{"en_US", "zh_CN", "zh_CHT", "ja", "ko", "ru", "fr", "de"}
	switch(value)
	{
		case 0:
			_os_printf("set language en_US\r\n");
			break;
		case 1:
			_os_printf("set language zh_CN\r\n");
			break;
		case 2:
			_os_printf("set language zh_CHT\r\n");
			break;
		case 3:
			_os_printf("set language ja\r\n");
			break;
		case 4:
			_os_printf("set language ko\r\n");
			break;
		case 5:
			_os_printf("set language ru\r\n");
			break;
		case 6:
			_os_printf("set language fr\r\n");
			break;
		case 7:
			_os_printf("set language de\r\n");
			break;
		default:
			_os_printf("set language error!!!\r\n");
            closeRes(httpClient);
			return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_timelapse_rate(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 缩时录影帧率 {"off", "1fps", "2fps", "5fps"}
    switch(value)
	{
		case 0:
			_os_printf("set timelapse rate off\r\n");
			break;
		case 1:
			_os_printf("set timelapse rate 1fps\r\n");
			break;
		case 2:
			_os_printf("set timelapse rate 2fps\r\n");
			break;
		case 3:
			_os_printf("set timelapse rate 5fps\r\n");
			break;
		default:
			_os_printf("set timelapse rate off error!!!\r\n");
            closeRes(httpClient);
			return;
	}

    item->value = value;
    http_reponse_success(httpClient);
}

static void set_park_record_time(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 停车监控时长 {"off", "12hour", "24hour", "48hour"}
    switch(value)
	{
		case 0:
			_os_printf("set park record time off\r\n");
			break;
		case 1:
			_os_printf("set park record time 12hour\r\n");
			break;
		case 2:
			_os_printf("set park record time 24hour\r\n");
			break;
		case 3:
			_os_printf("set park record time 48hour\r\n");
			break;
		default:
			_os_printf("set park record time error!!!\r\n");
            closeRes(httpClient);
			return;
	}

    item->value = value;
    http_reponse_success(httpClient);
}

static void set_park_gsr_sensitivity(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 停车监控碰撞感应  {"off", "lowest", "lower", "low", "middle", "high"}
    switch(value)
	{
		case 0:
			_os_printf("set park gsr sensitivity off\r\n");
			break;
		case 1:
			_os_printf("set park gsr sensitivity lowest\r\n");
			break;
		case 2:
			_os_printf("set park gsr sensitivity lower\r\n");
			break;
		case 3:
			_os_printf("set park gsr sensitivity low\r\n");
			break;
        case 4:
			_os_printf("set park gsr sensitivity middle\r\n");
			break;
		case 5:
			_os_printf("set park gsr sensitivity high\r\n");
			break;
		default:
			_os_printf("set park gsr sensitivity error!!!\r\n");
            closeRes(httpClient);
			return;
	}

    item->value = value;
    http_reponse_success(httpClient);
}

static void set_light_fre(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 光源频率	{"50HZ", "60HZ"}
	if(value == 0)
	{
		// 50HZ
		_os_printf("set light fre 50HZ\r\n");
	}
	else if(value == 1)
	{
		// 60HZ
		_os_printf("set light fre 60HZ\r\n");
	}
	else
	{
		_os_printf("set light fre error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_rear_mirror(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 后路镜像
    if(value == 0)
	{
		// 50HZ
		_os_printf("set rear_mirror off\r\n");
	}
	else if(value == 1)
	{
		// 60HZ
		_os_printf("set rear_mirror on\r\n");
	}
	else
	{
		_os_printf("set rear_mirror error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_wdr(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 宽动态
	if(value == 0)
	{
		// 关闭WDR
		_os_printf("set wdr off\r\n");
	}
	else if(value == 1)
	{
		// 打开WDR
		_os_printf("set wdr on\r\n");
	}
	else
	{
	    _os_printf("set wdr error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_parking_mode(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 停车监控模式 {"off", "timelapse", "normrec"}
    switch(value)
	{
		case 0:
			_os_printf("set parking_mode off\r\n");
			break;
		case 1:
			_os_printf("set parking_mode timelapse\r\n");
			break;
		case 2:
			_os_printf("set parking_mode normrec\r\n");
			break;
		default:
			_os_printf("set parking_mode error!!!\r\n");
            closeRes(httpClient);
			return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_parking_monitor(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 停车监控开关
	if(value == 0)
	{
		// 关闭停车监控
		_os_printf("set parking_monitor off\r\n");
	}
	else if(value == 1)
	{
		// 打开停车监控
		_os_printf("set parking_monitor on\r\n");
	}
	else
	{
	    _os_printf("set parking_monitor error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_voice_control(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 声控开关
	if(value == 0)
	{
		// 关闭声控
		_os_printf("set voice control off\r\n");
	}
	else if(value == 1)
	{
		// 打开声控
		_os_printf("set voice control on\r\n");
	}
	else
	{
	    _os_printf("set voice control error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_screen_standby(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 自动息屏时间	{"off", "1MIN", "2MIN", "3MIN"}
	switch(value)
	{
		case 0:
			_os_printf("set screen standby off\r\n");
			break;
		case 1:
			_os_printf("set screen standby 1MIN\r\n");
			break;
		case 2:
			_os_printf("set screen standby 2MIN\r\n");
			break;
		case 3:
			_os_printf("set screen standby 3MIN\r\n");
			break;
		default:
			_os_printf("set screen standby error!!!\r\n");
            closeRes(httpClient);
			return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_auto_poweroff(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 延时关机时间	{"off", "1MIN", "2MIN", "3MIN"}
	switch(value)
	{
		case 0:
			_os_printf("set auto poweroff off\r\n");
			break;
		case 1:
			_os_printf("set auto poweroff 1MIN\r\n");
			break;
		case 2:
			_os_printf("set auto poweroff 2MIN\r\n");
			break;
		case 3:
			_os_printf("set auto poweroff 3MIN\r\n");
			break;
		default:
			_os_printf("set auto poweroff error!!!\r\n");
            closeRes(httpClient);
			return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_adas(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 驾驶辅助
	if(value == 0)
	{
		// 关闭驾驶辅助
		_os_printf("set adas off\r\n");
	}
	else if(value == 1)
	{
		// 打开驾驶辅助
		_os_printf("set adas on\r\n");
	}
	else
	{
	    _os_printf("set adas error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_boot_sound(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 开机启动音
	if(value == 0)
	{
		// 关闭开机启动音
		_os_printf("set boot sound off\r\n");
	}
	else if(value == 1)
	{
		// 打开机启动音
		_os_printf("set boot sound on\r\n");
	}
	else
	{
	    _os_printf("set boot sound error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_video_flip(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 视频垂直镜像
	if(value == 0)
	{
		// 关闭垂直镜像
		_os_printf("set video flip off\r\n");
        struct isp_device *isp = (struct isp_device  *)dev_get(HG_ISP_DEVID);
        isp_reverse_enable(isp, 0, SENSOR_TYPE_MASTER);
	}
	else if(value == 1)
	{
		// 打开垂直镜像
		_os_printf("set video flip on\r\n");
        struct isp_device *isp = (struct isp_device  *)dev_get(HG_ISP_DEVID);
        isp_reverse_enable(isp, 1, SENSOR_TYPE_MASTER);
	}
	else
	{
	    _os_printf("set video flip error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_video_mirror(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 视频水平镜像
	if(value == 0)
	{
		// 关闭视频镜像
		_os_printf("set video mirror off\r\n");
        struct isp_device *isp = (struct isp_device  *)dev_get(HG_ISP_DEVID);
        isp_mirror_enable(isp, 0, SENSOR_TYPE_MASTER);
	}
	else if(value == 1)
	{
		// 打开视频镜像
		_os_printf("set video mirror on\r\n");
        struct isp_device *isp = (struct isp_device  *)dev_get(HG_ISP_DEVID);
        isp_mirror_enable(isp, 1, SENSOR_TYPE_MASTER);
	}
	else
	{
	    _os_printf("set video mirror error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_key_tone(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 按键音
	if(value == 0)
	{
		// 关闭按键音
		_os_printf("set key tone off\r\n");
	}
	else if(value == 1)
	{
		// 打开按键音
		_os_printf("set key tone on\r\n");
	}
	else
	{
		_os_printf("set key tone error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_hour_type(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 24小时制
	if(value == 0)
	{
		// 关闭24小时制
		_os_printf("set 24hour type off\r\n");
	}
	else if(value == 1)
	{
		// 打开24小时制
		_os_printf("set 24hour type on\r\n");
	}
	else
	{
		_os_printf("set 24hour type error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_time_format(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 时间格式
	if(value == 0)
	{
		// yyyy-MM-dd
		_os_printf("set time format yyyy-MM-dd\r\n");
	}
	else if(value == 1)
	{
		// yyyy/MM/dd
		_os_printf("set time format yyyy/MM/dd\r\n");
	}
	else
	{
	    _os_printf("set time format error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_ev(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 曝光补偿 {"-1", "0", "1"},
    switch(value)
	{
	case 0:
		_os_printf("set ev -1\r\n");
		break;
	case 1:
		_os_printf("set ev 0\r\n");
		break;
	case 2:
		_os_printf("set ev 1\r\n");
		break;
	default:
		_os_printf("set ev error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_ir_mode(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 红外模式 {"auto", "manual"}
	if(value == 0)
	{
		// 自动
		_os_printf("set ir mode auto\r\n");
	}
	else if(value == 1)
	{
		// 手动
		_os_printf("set ir mode manual\r\n");
	}
	else
	{
		_os_printf("set ir mode error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_speed_unit(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 速度单位 {"km/h", "mph"}
	if(value == 0)
	{
		// km/h
		_os_printf("set speed unit km/h\r\n");
	}
	else if(value == 1)
	{
		// mph
		_os_printf("set speed unit mph\r\n");
	}
	else
	{
        _os_printf("set speed unit error!!!\r\n");
        closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_wb(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 白平衡 {"off", "on"}
	if(value == 0)
	{
		// off
		_os_printf("set wb off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set wb on\r\n");
	}
	else
	{
		_os_printf("set wb error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_anti_shake(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 防抖 {"off", "on"}
	if(value == 0)
	{
		// off
		_os_printf("set anti shake off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set anti shake on\r\n");
	}
	else
	{
		_os_printf("set anti shake error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_fast_record(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 快速录像 {"off", "2x", "4x", "6x"}
    switch(value)
	{
	case 0:
		// off
		_os_printf("set fast record off\r\n");
		break;
	case 1:
		// 2x
		_os_printf("set fast record 2x\r\n");
		break;
	case 2:
		// 4x
		_os_printf("set fast record 4x\r\n");
		break;
	case 3:
		// 6x
		_os_printf("set fast record 6x\r\n");
		break;
	default:
		_os_printf("set fast record error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_denoise(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 图像降噪 {"off", "on"}
	if(value == 0)
	{
		// off
		_os_printf("set denoise off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set denoise on\r\n");
	}
	else
	{
	    _os_printf("set denoise error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_auto_record(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 自动录像 {"off", "on"}
	if(value == 0)
	{
		// off
		_os_printf("set auto record off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set auto record on\r\n");
	}
	else
	{
        _os_printf("set auto record error!!!\r\n");
        closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_image_size(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 拍照分辨率   {"720P", "1080P", "2K", "4K", "8K"}
    switch(value)
    {
    case 0:
		// 720P
		_os_printf("set image size 720P\r\n");
		break;
	case 1:
		// 1080P
		_os_printf("set image size 1080P\r\n");
		break;
	case 2:
		// 2K
		_os_printf("set image size 2K\r\n");
		break;
    case 3:
		// 4K
		_os_printf("set image size 4K\r\n");
		break;
	case 4:
		// 8K
		_os_printf("set image size 8K\r\n");
		break;
	default:
		_os_printf("set image size error!!!\r\n");
        closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_delay_shot(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 延时拍照 {"off", "2s", "10s"}
	switch(value)
	{
	case 0:
		// off
		_os_printf("set delay shot off\r\n");
		break;
	case 1:
		// 2s
		_os_printf("set delay shot 2s\r\n");
		break;
	case 2:
		// 10s
		_os_printf("set delay shot 10s\r\n");
		break;
	default:
		_os_printf("set delay shot error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_continue_shot(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 连拍张数 {"off", "2", "10"}
	switch(value)
	{
	case 0:
		// off
		_os_printf("set continue shot off\r\n");
		break;
	case 1:
		// 2
		_os_printf("set continue shot 2\r\n");
		break;
	case 2:
		// 10
		_os_printf("set continue shot 10\r\n");
		break;
	default:
		_os_printf("set continue shot error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_interior_record(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 车内录像 {"off", "on"}
	if(value == 0)
	{
		// off
		_os_printf("set interior record off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set interior record on\r\n");
	}
	else
	{
        _os_printf("set interior record error!!!\r\n");
        closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_shot_timelapse_rate(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 拍照缩时间隔 {"off", "2s", "10s"}
	switch(value)
	{
	case 0:
		// off
		_os_printf("set shot timelapse rate off\r\n");
		break;
	case 1:
		// 2s
		_os_printf("set shot timelapse rate 2s\r\n");
		break;
	case 2:
		// 10s
		_os_printf("set shot timelapse rate 10s\r\n");
		break;
	default:
		_os_printf("set shot timelapse rate error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_shot_timelapse_time(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 拍照缩时间长 {"off", "2s", "10s"}
	switch(value)
	{
	case 0:
		// off
		_os_printf("set shot timelapse rate off\r\n");
		break;
	case 1:
		// 2s
		_os_printf("set shot timelapse rate 2s\r\n");
		break;
	case 2:
		// 10s
		_os_printf("set shot timelapse rate 10s\r\n");
		break;
	default:
		_os_printf("set shot timelapse rate error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_video_flip_mirror(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 图像翻转 {"off", "on"}
	if(value == 0)
	{
		// off
		_os_printf("set video flip mirror off\r\n");
        // 关闭垂直镜像
        struct isp_device *isp = (struct isp_device  *)dev_get(HG_ISP_DEVID);
        isp_reverse_enable(isp, 0, SENSOR_TYPE_MASTER);
        isp_mirror_enable(isp, 0, SENSOR_TYPE_MASTER);
	}
	else if(value == 1)
	{
		// on
        _os_printf("set video flip mirror on\r\n");
        struct isp_device *isp = (struct isp_device  *)dev_get(HG_ISP_DEVID);
        isp_reverse_enable(isp, 1, SENSOR_TYPE_MASTER);
        isp_mirror_enable(isp, 1, SENSOR_TYPE_MASTER);
	}
	else
	{
        _os_printf("set video flip mirror error!!!\r\n");
        closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_five_g_wifi(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 5G WiFi {"off", "on"}
	if(value == 0)
	{
		// off
		_os_printf("set five g wifi off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set five g wifi on\r\n");
	}
	else
	{
        _os_printf("set five g wifi error!!!\r\n");
        closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_gps_osd(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // GPS 水印 {"off", "on"}
	if(value == 0)
	{
		// off
		_os_printf("set gps osd off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set gps osd on\r\n");
	}
	else
	{
        _os_printf("set gps osd error!!!\r\n");
        closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_speed_osd(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 速度水印 {"off", "on"}
	if(value == 0)
	{
		// off
		_os_printf("set speed osd off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set speed osd on\r\n");
	}
	else
	{
        _os_printf("set speed osd error!!!\r\n");
        closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_rear_flip(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 后路翻转 {"off", "on"}
	if(value == 0)
	{
		// off
		_os_printf("set rear flip off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set rear flip on\r\n");
	}
	else
	{
        _os_printf("set rear flip error!!!\r\n");
        closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_summer_timer(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 夏令时 {"off", "on"}
	if(value == 0)
	{
		// off
		_os_printf("set summer timer off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set summer timer on\r\n");
	}
	else
	{
        _os_printf("set summer timer error!!!\r\n");
        closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_screen_brightness(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 屏幕亮度 {"off", "low", "middle", "high", "very high"}
    switch(value)
	{
	case 0:
		// off
		_os_printf("set screen brightness off\r\n");
		break;
	case 1:
		// low
		_os_printf("set screen brightness low\r\n");
		break;
	case 2:
		// middle
		_os_printf("set screen brightness middle\r\n");
		break;
	case 3:
		// high
		_os_printf("set screen brightness high\r\n");
		break;
	case 4:
		// very high
		_os_printf("set screen brightness very high\r\n");
		break;
	default:
		_os_printf("set screen brightness error!!!\r\n");
        closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_sound_indicator(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 提示音 {"off", "on"}
	if(value == 0)
	{
		// off
		_os_printf("set sound indicator off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set sound indicator on\r\n");
	}
	else
	{
        _os_printf("set sound indicator error!!!\r\n");
        closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_gps_data(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // GPS 数据 {"off", "on"}
	if(value == 0)
	{
		// off
		_os_printf("set gps data off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set gps data on\r\n");
	}
	else
	{
	    _os_printf("set gps data error!!!\r\n");
        closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_coordinate_osd(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 经纬度水印 {"off", "on"}
	if(value == 0)
	{
		// off
		_os_printf("set coordinate osd off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set coordinate osd on\r\n");
	}
	else
	{
        _os_printf("set coordinate osd error!!!\r\n");
        closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_inside_mirror(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 车内镜像 {"off", "on"}
	if(value == 0)
	{
		// off
		_os_printf("set inside mirror off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set inside mirror on\r\n");
	}
	else
	{
        _os_printf("set inside mirror error!!!\r\n");
        closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_status_light(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 状态灯 {"off", "on"}
	if(value == 0)
	{
		// off
		_os_printf("set status light off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set status light on\r\n");
	}
	else
	{
        _os_printf("set status light error!!!\r\n");
        closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_osd_option(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 视频水印选项 {"off", "on", "logo&date", "logo&model&date&time"}
    switch(value)
	{
	case 0:
		// off
		_os_printf("set osd option off\r\n");
		break;
	case 1:
		// on
		_os_printf("set osd option on\r\n");
		break;
	case 2:
		// logo&date
		_os_printf("set osd option logo&date\r\n");
        break;
	case 3:
		// logo&model&date&time
		_os_printf("set osd option logo&model&date&time\r\n");
		break;
	default:
		_os_printf("set osd option error!!!\r\n");
		closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_wifi_channel(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // wifi 信道 {"0", 1", "2"}
    switch(value)
	{
	case 0:
		// 0
		_os_printf("set wifi channel 0\r\n");
		break;
	case 1:
		// 1
		_os_printf("set wifi channel 1\r\n");
		break;
	case 2:
		// 2
		_os_printf("set wifi channel 2\r\n");
		break;
	default:
		_os_printf("set wifi channel error!!!\r\n");
		closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_fill_light(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 补光 {"off", "on"}
	if(value == 0)
	{
		// off
		_os_printf("set fill light off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set fill light on\r\n");
	}
	else
	{
        _os_printf("set fill light error!!!\r\n");
		closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_screensaver(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 屏保 {"off", "on"}
	if(value == 0)
	{
		// off
		_os_printf("set screensaver off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set screensaver on\r\n");
	}
	else
	{
        _os_printf("set screensaver error!!!\r\n");
		closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_front_rotate(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 前路旋转 {"off", "on", "auto"}
    switch(value)
	{
	case 0:
		// off
		_os_printf("set front rotate off\r\n");
		break;
	case 1:
		// on
		_os_printf("set front rotate on\r\n");
		break;
	case 2:
		// auto
		_os_printf("set front rotate auto\r\n");
		break;
	default:
		_os_printf("set front rotate error!!!\r\n");
		closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_hdr(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 高动态 {"off", "on"}
	if(value == 0)
	{
		// off
		_os_printf("set hdr off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set hdr on\r\n");
	}
	else
	{
        _os_printf("set hdr error!!!\r\n");
		closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_wifi_boot_starter(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 开机启动 WiFi    {"off", "on"}
	if(value == 0)
	{
		// off
		_os_printf("set wifi boot starter off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set wifi boot starter on\r\n");
	}
	else
	{
        _os_printf("set wifi boot starter error!!!\r\n");
		closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_screen_saver_type(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 屏保类型 {"screen_off", "screen_saver_on"}
	if(value == 0)
	{
		// off
		_os_printf("set screen saver type off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set screen saver type on\r\n");
	}
	else
	{
        _os_printf("set screen saver type error!!!\r\n");
		closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_work_mode(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 预览多模式切换   {"record", "capture", "delaycapture", "intervalcapture", 
    // "continuouscapture", "slowrecord", "looprecord", "timelapserecord"}
	switch(value)
	{
	case 0:
		// record
		_os_printf("set work mode record\r\n");
		break;
	case 1:
		// capture
		_os_printf("set work mode capture\r\n");
		break;
	case 2:
		// delaycapture
		_os_printf("set work mode delaycapture\r\n");
		break;
	case 3:
		// intervalcapture
		_os_printf("set work mode intervalcapture\r\n");
		break;
	case 4:
		// continuouscapture
		_os_printf("set work mode continuouscapture\r\n");
		break;
	case 5:
		// slowrecord
		_os_printf("set work mode slowrecord\r\n");
		break;
	case 6:
		// looprecord
		_os_printf("set work mode looprecord\r\n");
		break;
	case 7:
		// timelapserecord
		_os_printf("set work mode timelapserecord\r\n");
		break;
	default:
		_os_printf("set work mode error!!!\r\n");
		closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_frame_zoom(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 预览画面缩放切换 {"1x", "2x", "3x"}
    switch(value)
	{
	case 0:
		// 1x
		_os_printf("set frame zoom 1x\r\n");
		break;
	case 1:
		// 2x
		_os_printf("set frame zoom 2x\r\n");
		break;
	case 2:
		// 3x
		_os_printf("set frame zoom 3x\r\n");
		break;
	default:
		_os_printf("set frame zoom error!!!\r\n");
		closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_rotate(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 图像旋转 {"off", "on", "auto"}
    switch(value)
	{
	case 0:
		// off
		_os_printf("set rotate off\r\n");
		break;
	case 1:
		// on
		_os_printf("set rotate on\r\n");
		break;
	case 2:
		// auto
		_os_printf("set rotate auto\r\n");
		break;
	default:
		_os_printf("set rotate error!!!\r\n");
		closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_motion_detect(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 移动侦测 {"off", "on"}
    if(value == 0)
	{
		// off
		_os_printf("set motion detect off\r\n");
	}
	else if(value == 1)
	{
		// on
		_os_printf("set motion detect on\r\n");
	}
	else
	{
        _os_printf("set motion detect error!!!\r\n");
		closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_file_format(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
    // 录像格式 {"MP4", "AVI"}
    switch(value)
	{
	case 0:
		// MP4
		_os_printf("set file fromat mp4\r\n");
		break;
	case 1:
		// AVI
		_os_printf("set file fromat avi\r\n");
		break;
	default:
		_os_printf("set file fromat error!!!\r\n");
		closeRes(httpClient);
		return;
	}

    item->value = value;
	http_reponse_success(httpClient);
}

static void set_switchcam(struct items_config *item, uint8_t value, struct httpClient *httpClient)
{
	// 镜头切换 前 中 后 画中画
    switch(value)
    {
    case 0:
		_os_printf("switch cam 0\r\n");
		break;
	case 1:
		_os_printf("switch cam 1\r\n");
		break;
	case 2:
		_os_printf("switch cam 2\r\n");
		break;
	default:
		_os_printf("switch cam error\r\n");
        closeRes(httpClient);
		return;
    }

    item->value = value;
	http_reponse_success(httpClient);
}

static void http_reponse_setparamvalue(struct httpClient *httpClient)
{
	char *param_start, *param_end, *params, *value_str;
	int param_size = 0;
    uint8_t value = 0;

    // 查找参数字符串起始位置
    param_start = strstr(httpClient->pattern, "param=");
    if(param_start) {
        param_start += strlen("param=");
    }
	param_end = strchr(httpClient->pattern, '&');
    value_str = strstr(httpClient->pattern, "value=");
    if(value_str) {
        value_str += strlen("value=");
    }

    if(param_start == NULL || param_end == NULL || value_str == NULL) {
        os_printf("%s: %d, request error\r\n", __FUNCTION__, __LINE__);
        closeRes(httpClient);
        return;
    }
    
	param_size = param_end - param_start;
	params = STREAM_LIBC_MALLOC(param_size + 1);
	if (params == NULL) {
        os_printf("%s: %d, malloc failed\r\n", __FUNCTION__, __LINE__);
        closeRes(httpClient);
        return;
	}
    strncpy(params, param_start, param_size);
    params[param_size] = '\0';

    // 转换为整数
    value = (uint8_t)atoi(value_str);

	// 在参数表中查找对应的处理函数
    for (int i = 0; i < ITEMS_CONFIG_COUNT; i++) {
        int items_cfg_len = strlen(items_cfg[i].const_item->name);
        if (strncmp(items_cfg[i].const_item->name, params, (items_cfg_len > param_size) ? items_cfg_len : param_size) == 0) {
            if(items_cfg[i].const_item->handler != NULL) {
                items_cfg[i].const_item->handler(&items_cfg[i], value, httpClient);
                if(strncmp(items_cfg[i].const_item->name, "rec", items_cfg_len) != 0 && items_cfg[i].const_item->enable == ITEM_ENABLE)
                    items_value_process("saveflag", 1, SET_ITEMS_VALUE);
            } else {
                closeRes(httpClient);
            }
            STREAM_LIBC_FREE(params);
            return;
        }
    }
	// 未找到
    _os_printf("Unknown parameter: %s\r\n", params);
	closeRes(httpClient);
	STREAM_LIBC_FREE(params);
}

void send_chunk_dynamic(int fd, const char* data, uint16_t data_len) {
	uint16_t len = data_len + 32;
    char *chunk_buffer = STREAM_LIBC_MALLOC(len);
	if(chunk_buffer == NULL)
	{
		os_printf("chunk_buffer STREAM_LIBC_MALLOC fail\r\n");
		return;
	}
	int chunk_len = os_snprintf(chunk_buffer, len - 1, "%x\r\n%s\r\n", data_len, data);
	send(fd, chunk_buffer, chunk_len, 0);
    STREAM_LIBC_FREE(chunk_buffer);
}

static int get_extension_string(int fd, const char *search_dir, const char *extension_name, const char *prefix)
{
    int         file_count = 0;
    char        send_buffer[256];
    char        path[64];
    char        filename_suffix[32];
    const char  *suffix = NULL;
    uint8_t     extension_name_len = strlen(extension_name);

    uint8_t type = (os_strcmp(prefix, EVENT_PREFIX) == 0) ? 1 : 
                   (os_strcmp(prefix, LOOP_PREFIX) == 0) ? 2 : 
                   (os_strcmp(prefix, EMR_PREFIX) == 0) ? 3 : 4;
    
    if(os_strcmp(search_dir, REC_PATH) == 0 || os_strcmp(search_dir, IMG_PATH) == 0) {
        suffix = NULL;
    }
    else if(os_strcmp(search_dir, RECA_PATH) == 0 || os_strcmp(search_dir, IMGA_PATH) == 0) {
        suffix = FRONT_SUFFIX;
    } else if(os_strcmp(search_dir, RECB_PATH) == 0 || os_strcmp(search_dir, IMGB_PATH) == 0) {
        suffix = INTER_SUFFIX;
    } else if(os_strcmp(search_dir, RECC_PATH) == 0 || os_strcmp(search_dir, IMGC_PATH) == 0) {
        suffix = BACK_SUFFIX;
    }
    
    FILINFO *fil;
    void *dir = osal_opendir((char *) search_dir);
    if (dir)
    {
        do
        {
            fil = osal_readdir(dir);
            if(!fil) break;
            if(fil->fname[0] == 0) break;
            if (fil->fname[0] == '.') continue;

            if (osal_dirent_isdir(fil) && (type == 2 || type == 3 || type == 4))
            {
                snprintf(path, sizeof(path), "%s/%s", search_dir, fil->fname);
                void *subdir = osal_opendir(path);

                // 第二层遍历：获取子目录中的文件
                FILINFO *sub_fil;
                if (subdir)
                {
                    do
                    {
                        sub_fil = osal_readdir(subdir);
                        if(!sub_fil) break;
                        if(sub_fil->fname[0] == 0) break;
                        if (sub_fil->fname[0] == '.') continue;
                        if (osal_dirent_isdir(sub_fil))
                            continue;
                        
                        char    *filename     = osal_dirent_name(sub_fil);
                        uint32_t filesize     = osal_dirent_size(sub_fil);
                        uint8_t  filename_len = strlen(filename);

                        if(type == 3) {
                            if(os_strstr(filename, SOS_PREFIX) == NULL)
                                continue;
                        } else {
                            if(os_strstr(filename, SOS_PREFIX) != NULL)
                                continue;
                        }

                        // 检查后缀名是否匹配
                        uint8_t extension_filename[16];
                        
                        // 进行全部转换成大写
                        if (filename_len > extension_name_len)
                        {
                            for (uint8_t i = 0; i < extension_name_len; i++)
                            {
                                extension_filename[i] = toupper(filename[filename_len - extension_name_len + i]);
                            }
                            // 后缀名匹配
                            if (memcmp(extension_name, extension_filename, extension_name_len) == 0)
                            {
                                if(suffix)
                                {
                                    os_snprintf(filename_suffix, sizeof(filename_suffix), "%.*s%s%s", filename_len - extension_name_len, filename, suffix, extension_name);
                                    os_snprintf(send_buffer, sizeof(send_buffer), "%s{\"name\":\"%s/%s\",\"size\":%d,\"createtimestr\":\"%.14s\",\"type\":%d}",
                                           (file_count++ > 0) ? "," : "",
                                            prefix,
                                            filename_suffix,
                                            filesize / 1024,
                                            filename,
                                            2);
                                }
                                else
                                {
                                    os_snprintf(send_buffer, sizeof(send_buffer), "%s{\"name\":\"%s/%s\",\"size\":%d,\"createtimestr\":\"%.14s\",\"type\":%d}",
                                           (file_count++ > 0) ? "," : "",
                                            prefix,
                                            filename,
                                            filesize / 1024,
                                            filename,
                                            2);
                                }
                                // 发送数据
                                send_chunk_dynamic(fd, send_buffer, strlen(send_buffer));
                            }
                        }
                    } while(sub_fil);
                    osal_closedir(subdir);
                }
            }
            else if(!osal_dirent_isdir(fil) && (type == 1))
            {
                char    *filename     = osal_dirent_name(fil);
                uint32_t filesize     = osal_dirent_size(fil);
                uint8_t  filename_len = strlen(filename);

                // 检查后缀名是否匹配
                uint8_t extension_filename[16];

                // 进行全部转换成大写
                if (filename_len > extension_name_len)
                {
                    for (uint8_t i = 0; i < extension_name_len; i++)
                    {
                        extension_filename[i] = toupper(filename[filename_len - extension_name_len + i]);
                    }
                    // 后缀名匹配
                    if (memcmp(extension_name, extension_filename, extension_name_len) == 0)
                    {
                        if(suffix)
                        {
                            os_snprintf(filename_suffix, sizeof(filename_suffix), "%.*s%s%s", filename_len - extension_name_len, filename, suffix, extension_name);
                            os_snprintf(send_buffer, sizeof(send_buffer), "%s{\"name\":\"%s/%s\",\"size\":%d,\"createtimestr\":\"%.14s\",\"type\":%d}",
                                   (file_count++ > 0) ? "," : "",
                                    prefix,
                                    filename_suffix,
                                    filesize / 1024,
                                    filename,
                                    1);
                        }
                        else
                        {
                            os_snprintf(send_buffer, sizeof(send_buffer), "%s{\"name\":\"%s/%s\",\"size\":%d,\"createtimestr\":\"%.14s\",\"type\":%d}",
                                   (file_count++ > 0) ? "," : "",
                                    prefix,
                                    filename,
                                    filesize / 1024,
                                    filename,
                                    1);
                        }
                        // 发送数据
                        send_chunk_dynamic(fd, send_buffer, strlen(send_buffer));
                    }
                }
            }
        } while (fil);
        osal_closedir(dir);
    }
    return file_count;
}

static void stream_response(int fd)
{
    int     video_count = 0;
    int     photo_count = 0;
    int     sos_count = 0;
    int     park_count = 0;
    int     last_count = 0;
    char    send_buffer[256];

    // 创建HTTP响应头
    snprintf(send_buffer, sizeof(send_buffer), 
        "HTTP/1.0 200 OK\r\n"
        "Connection: close\r\n"
        "Content-Type: application/json\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n");
    send(fd, send_buffer, strlen(send_buffer), 0);
    
    // 获取视频
    os_snprintf(send_buffer, sizeof(send_buffer), "{\"result\":0,\"info\":[{\"folder\":\"%s\",\"files\":[", LOOP_PREFIX);
    send_chunk_dynamic(fd, send_buffer, strlen(send_buffer));

    for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++) 
    {
        if(g_cam_configs[i].mask & CAM_MASK_ENABLE)
        {
            if(last_count != video_count)
            {
                send_chunk_dynamic(fd, ",", strlen(","));
                last_count += video_count;
            }
            video_count += get_extension_string(fd, g_cam_configs[i].rec_path, g_cam_configs[i].file_process.ext_name, LOOP_PREFIX);
        }
    }

    // 获取照片
    os_snprintf(send_buffer, sizeof(send_buffer), "],\"count\":%d},{\"folder\":\"%s\",\"files\":[", video_count, EVENT_PREFIX);
    send_chunk_dynamic(fd, send_buffer, strlen(send_buffer));
    last_count = 0;
    for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++) 
    {
        if(g_cam_configs[i].mask & CAM_MASK_ENABLE)
        {
            if(last_count != photo_count)
            {
                send_chunk_dynamic(fd, ",", strlen(","));
                last_count += photo_count;
            }
            photo_count += get_extension_string(fd, g_cam_configs[i].img_path, JPG_EXTENSION_NAME, EVENT_PREFIX);
        }
    }

    // 紧急事件
    os_snprintf(send_buffer, sizeof(send_buffer), "],\"count\":%d},{\"folder\":\"%s\",\"files\":[", photo_count, EMR_PREFIX);
    send_chunk_dynamic(fd, send_buffer, strlen(send_buffer));
    last_count = 0;
    for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++) 
    {
        if(g_cam_configs[i].mask & CAM_MASK_ENABLE)
        {
            if(last_count != sos_count)
            {
                send_chunk_dynamic(fd, ",", strlen(","));
                last_count += sos_count;
            }
            sos_count += get_extension_string(fd, g_cam_configs[i].rec_path, g_cam_configs[i].file_process.ext_name, EMR_PREFIX);
        }
    }

    if(PARKING_MOR_ALBUM_EN) {
        os_snprintf(send_buffer, sizeof(send_buffer), "],\"count\":%d},{\"folder\":\"%s\",\"files\":[", sos_count, PARK_PREFIX);
        send_chunk_dynamic(fd, send_buffer, strlen(send_buffer));
        last_count = 0;
        for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++) 
        {
            if(g_cam_configs[i].mask & CAM_MASK_ENABLE)
            {
                if(last_count != park_count)
                {
                    send_chunk_dynamic(fd, ",", strlen(","));
                    last_count += park_count;
                }
                park_count += get_extension_string(fd, PARK_PATH, g_cam_configs[i].file_process.ext_name, PARK_PREFIX);
            }
        }
        os_snprintf(send_buffer, sizeof(send_buffer), "],\"count\":%d}]}", park_count);
        send_chunk_dynamic(fd, send_buffer, strlen(send_buffer));
    } else {
        os_snprintf(send_buffer, sizeof(send_buffer), "],\"count\":%d}]}", sos_count);
        send_chunk_dynamic(fd, send_buffer, strlen(send_buffer));
    }

    // 发送分块结束标记
    send_chunk_dynamic(fd, "", 0);  // 0长度块表示结束
}

static void http_reponse_getfilelist(struct httpClient *httpClient)
{
	int fd = httpClient->fdClient;
    stream_response(fd);
    closeRes(httpClient);
}

static void http_reponse_thumbnail(struct httpClient *httpClient)
{
    char *post_content = NULL;
    uint32_t sendlen = 0;
    struct httpresp *head;
    int fd = httpClient->fdClient;
    char *filepath = httpClient->pattern;

    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Connection, "close");
    http_add_header(head, httpIndex.Type, "image/jpeg");

#if APP_LOCK_VIDEO_EN
    if(strstr(filepath, EMR_PREFIX))
    {
        filepath = remove_prefix(filepath);
        if(filepath == NULL)
        {
            os_printf("%s remove_prefix err\r\n", __FUNCTION__);
            closeRes(httpClient);
            return;
        }
    }
#endif

    char *filename = remove_suffix(filepath);
    if(filename == NULL)
    {
        os_printf("%s remove_suffix err\r\n", __FUNCTION__);
        closeRes(httpClient);
        return;
    }

    // 获取缩略图
    post_content = (char *)jpg_file_read(filename, 1, (int32_t*)&sendlen);
    os_printf("post_content:%x, sendlen:%d\r\n", post_content, sendlen);
    http_header_send(head, httpIndex.Length, sendlen, fd);
    if (post_content)
    {
        send(fd, post_content, sendlen, 0);
        jpg_file_free(post_content);
    }
    closeRes(httpClient);
}

static int32 set_wifi_ssid(const char *ssid)
{
    uint8_t ifidx = (sys_cfgs.wifi_mode == WIFI_MODE_APSTA ? WIFI_MODE_STA : sys_cfgs.wifi_mode);
    
    os_memset(sys_cfgs.bssid, 0, 6);
    os_strncpy(sys_cfgs.ssid, ssid, SSID_MAX_LEN);
    ieee80211_conf_set_bssid(ifidx, NULL);
    if (os_strlen(sys_cfgs.passwd) > 0) {
        wpa_passphrase(sys_cfgs.ssid, (char*)sys_cfgs.passwd, sys_cfgs.psk);
    }
    ieee80211_conf_set_ssid(ifidx, sys_cfgs.ssid);
    ieee80211_conf_set_psk(ifidx, sys_cfgs.psk);
    os_printf("set new ssid:%s\r\n", sys_cfgs.ssid);
    syscfg_save();
    return 0;
}

static int32 set_wifi_passwd(char *passwd)
{
    uint8_t ifidx = (sys_cfgs.wifi_mode == WIFI_MODE_APSTA ? WIFI_MODE_STA : sys_cfgs.wifi_mode);
    
    os_strncpy(sys_cfgs.passwd, passwd, PASSWD_MAX_LEN);
    wpa_passphrase(sys_cfgs.ssid, (char*)sys_cfgs.passwd, sys_cfgs.psk);
    ieee80211_conf_set_psk(ifidx, sys_cfgs.psk);
    ieee80211_conf_set_passwd(ifidx, (char*)sys_cfgs.passwd);
    os_printf("set new key:%s\r\n", sys_cfgs.passwd);
    syscfg_save();

    return 0;
}

static void http_reponse_setwifi(struct httpClient *httpClient)
{
	char *equal_pos;
    char value[64];

	equal_pos = strchr(httpClient->pattern, '=');
    if(equal_pos == NULL)
    {
        os_printf("params format error\r\n", __FUNCTION__, __LINE__);
        closeRes(httpClient);
        return;
    }
    os_strncpy(value, equal_pos + 1, sizeof(value));

    if (os_strstr(httpClient->pattern, "wifissid")) {
        os_printf("set wifi ssid: %s\r\n", value);
        http_reponse_success(httpClient);
		os_sleep_ms(500);
        set_wifi_ssid(value);
    }
    else if (os_strstr(httpClient->pattern, "wifipwd")) {
        os_printf("set wifi pwd: %s\r\n",value);
        // psk need 8 bytes at less
        if (os_strlen(value) < 8) {
            os_printf("psk need 8 bytes at less\r\n");
            closeRes(httpClient);
            return;
        }
        http_reponse_success(httpClient);
		os_sleep_ms(500);
        set_wifi_passwd(value);
    }
    else {
        os_printf("unknown params: %s\r\n", httpClient->pattern);
		closeRes(httpClient);
		return;
    }
}

static void http_reponse_reset(struct httpClient *httpClient)
{
    for (int i = 0; i < ITEMS_CONFIG_COUNT; i++) {
        items_cfg[i].value = items_cfg[i].const_item->default_value;
    }
    
    itemcfg_save(0);
    itemcfg_handle(); // 重新加载配置

    char ssid_default[SSID_MAX_LEN + 1];
    char passwd_default[PASSWD_MAX_LEN + 1];
    os_snprintf(ssid_default, SSID_MAX_LEN, WIFI_SSID_PREFIX"%02X%02X%02X", sys_cfgs.mac[3], sys_cfgs.mac[4], sys_cfgs.mac[5]);
    os_strcpy(passwd_default, WIFI_PASSWD_DEFAULT);
    set_wifi_ssid(ssid_default);
    set_wifi_passwd(passwd_default);

    struct vpp_device *vpp_dev;
    vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
    vpp_set_watermark0_enable(vpp_dev, 1);

	http_reponse_success(httpClient);
}

static void http_reponse_sdformat(struct httpClient *httpClient)
{
    vfs_mkfs("0:","fatfs",NULL);
	http_reponse_success(httpClient);
}

static void http_reponse_wifireboot(struct httpClient *httpClient)
{
	http_reponse_unsupport(httpClient);
}

static void http_reponse_video(struct httpClient *httpClient)
{
    uint32_t sendlen = 0;
    char path[64];
    char range_buf[64];
    uint32_t read_len;
    struct httpresp *head = NULL;
    F_FILE *fp = NULL;
    uint8_t *tmp_buf = NULL;
    int res = 0;
    int fd = httpClient->fdClient;
    uint32_t offset_start = 0;
    uint32_t offset_end = 0;
    uint32_t remain_len = 0;
    uint8_t flag = 0;
    uint8_t file_type = 0;
    struct httpEntry *next = httpClient->http_request;
    char *filepath = httpClient->pattern;
    int type = (strncmp(next->value, "/" LOOP_PREFIX, strlen("/" LOOP_PREFIX)) == 0) ? 2 :
               (strncmp(next->value, "/" EMR_PREFIX, strlen("/" EMR_PREFIX)) == 0) ? 3 :
               (strncmp(next->value, "/" PARK_PREFIX, strlen("/" PARK_PREFIX)) == 0) ? 4 : 0;

    if(os_strstr(httpClient->pattern, BACK_SUFFIX))
    {
        file_type = 2;
    }
    else
    {
        file_type = 1;
    }

    _os_printf("file_type: %d\r\n", file_type);

    if(type == 0)
    {
        _os_printf("type error\r\n");
        goto reponse_video_end;
    }

    gen_video_path(filepath, path, sizeof(path));

    fp = osal_fopen((const char*)path, "rb");
    if(!fp)
    {
        os_printf("open file failed\r\n");
        head = http_create_reply(404, "Not Found");
        http_add_header(head, httpIndex.Connection, "close");
        http_header_send(head, httpIndex.Length, 0, fd);
        goto reponse_video_end;
    }

    sendlen = osal_fsize(fp);
    offset_start = 0;
    offset_end = sendlen;

    while(next)
    {
        if(strcmp(next->key, "Range") == 0)
        {
            uint32_t start = 0;
            uint32_t end = 0;

            flag = 1;

            if(sscanf(next->value, "bytes=%u-%u", &start, &end) == 2)
            {
                offset_start = start;
                offset_end = end + 1;
            }
            else if(sscanf(next->value, "bytes=%u-", &start) == 1)
            {
                offset_start = start;
                offset_end = sendlen;
            }
            else
            {
                os_printf("invalid range:%s\r\n", next->value);
                head = http_create_reply(416, "Range Not Satisfiable");
                http_add_header(head, httpIndex.Connection, "close");
                snprintf(range_buf, sizeof(range_buf), "bytes */%u", sendlen);
                http_add_header(head, "Content-Range", range_buf);
                http_header_send(head, httpIndex.Length, 0, fd);
                goto reponse_video_end;
            }
            break;
        }
        next = next->next;
    }

    os_printf("offset_start:%u\toffset_end:%u\tsendlen:%u\r\n", offset_start, offset_end, sendlen);

    if(offset_start >= sendlen || offset_start >= offset_end)
    {
        os_printf("offset_start out of range\r\n");
        head = http_create_reply(416, "Range Not Satisfiable");
        http_add_header(head, httpIndex.Connection, "close");
        snprintf(range_buf, sizeof(range_buf), "bytes */%u", sendlen);
        http_add_header(head, "Content-Range", range_buf);
        http_header_send(head, httpIndex.Length, 0, fd);
        goto reponse_video_end;
    }

    if(offset_end > sendlen)
    {
        offset_end = sendlen;
    }

    remain_len = offset_end - offset_start;

    if(flag)
    {
        head = http_create_reply(206, "Partial Content");
    }
    else
    {
        head = http_create_reply(200, "OK");
    }

    http_add_header(head, httpIndex.Connection, "close");

    if(file_type == 1)
    {
        http_add_header(head, httpIndex.Type, "video/mp4");
    }
    else
    {
        http_add_header(head, httpIndex.Type, "video/x-msvideo");
    }

    http_add_header(head, "Accept-Ranges", "bytes");

    if(flag)
    {
        snprintf(range_buf, sizeof(range_buf), "bytes %u-%u/%u", offset_start, offset_end - 1, sendlen);
        http_add_header(head, "Content-Range", range_buf);
    }

    http_header_send(head, httpIndex.Length, remain_len, fd);

    osal_fseek(fp, offset_start);

    tmp_buf = STREAM_MALLOC(4096);
    if(tmp_buf)
    {
        uint32_t total_len = 0;
        while(remain_len > 0)
        {
            uint32_t sent_len = 0;
            uint32_t need_len = (remain_len > 4096) ? 4096 : remain_len;
            read_len = osal_fread(tmp_buf, 1, need_len, fp);
            if(read_len == 0)
            {
                break;
            }

            while(sent_len < read_len)
            {
                res = send(fd, tmp_buf + sent_len, read_len - sent_len, 0);
                if(res <= 0)
                {
                    os_printf("send error, res: %d, total len: %d\r\n", res, total_len);
                    goto reponse_video_end;
                }
                sent_len += res;
				total_len += res;
            }

            remain_len -= read_len;
        }
        os_printf("send over, total len: %d\r\n", total_len);
    }

reponse_video_end:
    if(fp)
    {
        osal_fclose(fp);
        fp = NULL;
    }
    if(tmp_buf)
    {
        STREAM_FREE(tmp_buf);
        tmp_buf = NULL;
    }
    closeRes(httpClient);
}

static void http_reponse_event(struct httpClient *httpClient)
{
	char *post_content = NULL;
    uint32_t sendlen = 0;
    struct httpresp *head;
    int fd = httpClient->fdClient;

    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Connection, "close");
    http_add_header(head, httpIndex.Type, "image/jpeg");

    post_content = (char *)jpg_file_read(httpClient->pattern, 0, (int32_t*)&sendlen);
    os_printf("normal post_content:%x sendlen:%d\r\n", post_content, sendlen);
    http_header_send(head, httpIndex.Length, sendlen, fd);
    if (post_content)
    {
        int send_len = send(fd, post_content, sendlen, 0);
        os_printf("send_len: %d\n",send_len);
        jpg_file_free(post_content);
    }
    closeRes(httpClient);
}

static uint8_t file_delete(char *filename)
{
    FRESULT res;
    char jpg_path[64];
    char thumb_path[64];
    char *file_name = filename;
    uint8_t flag = 0;

    gen_photo_video_path(filename, jpg_path, sizeof(jpg_path));

#if APP_LOCK_VIDEO_EN
    if(strstr(filename, SOS_PREFIX))
    {
        file_name = remove_prefix(file_name);
        if(file_name == NULL)
        {
            os_printf("%s remove_prefix err\r\n", __FUNCTION__);
            return 1;
        }
        flag = 1;
    }
#endif

    file_name = remove_suffix(file_name);
    if(file_name == NULL)
    {
        os_printf("%s remove_suffix err\r\n", __FUNCTION__);
        return 1;
    }

    if(flag)
    {
        res = osal_fchmod(jpg_path, 0, AM_RDO);  // 清除只读属性
        if (res != FR_OK) {
            _os_printf("Failed to clean file %s AM_RDO attr, error: %d\n", jpg_path, res);
            return 1;
        }
    }
    
    res = osal_unlink(jpg_path);
    if (res != FR_OK) {
        _os_printf("Failed to delete file: %s, error: %d\n", jpg_path, res);
        return 1;
    }
    _os_printf("File deleted successfully: %s\n", jpg_path);

    gen_thumb_path(file_name, thumb_path, sizeof(thumb_path));
    res = osal_unlink(thumb_path);
    if (res != FR_OK) {
        os_printf("Failed to delete thumbnail: %s, error: %d\n", thumb_path, res);
        return 1;
    }
    _os_printf("Thumbnail deleted successfully: %s\n", thumb_path);
    
    return 0; // 成功
}

static void http_reponse_deletefile(struct httpClient *httpClient)
{
    uint8_t offset = 0;

    if (strncmp(httpClient->pattern, EVENT_PREFIX, strlen(EVENT_PREFIX)) == 0) {
		offset = strlen(EVENT_PREFIX) + 1;
    }
    else if (strncmp(httpClient->pattern, LOOP_PREFIX, strlen(LOOP_PREFIX)) == 0) {
        offset = strlen(LOOP_PREFIX) + 1;
    }
    else if(strncmp(httpClient->pattern, EMR_PREFIX, strlen(EMR_PREFIX)) == 0) {
        offset = strlen(EMR_PREFIX) + 1;
    }
	else if(strncmp(httpClient->pattern, PARK_PREFIX, strlen(PARK_PREFIX)) == 0) {
        offset = strlen(PARK_PREFIX) + 1;
    }
    else {
        _os_printf("Unknown parameter: %s\n", httpClient->pattern);
		closeRes(httpClient);
		return;
    }

    if(file_delete(httpClient->pattern + offset) == 0)
    {
        _os_printf("delete file success\r\n");
        http_reponse_success(httpClient);
        return;
    }
    _os_printf("delete file failed\r\n");
    closeRes(httpClient);
}

typedef void (*takephoto_fn)(uint8_t takephoto_num, uint16_t w, uint16_t h, const char *path);
void takephoto_over_dpi_func(uint8_t takephoto_num, uint16_t w, uint16_t h, const char *path)
{
    struct msi *over_dpi_jpg_msi = msi_find(SR_OVER_DPI_JPG, 1);
    if(over_dpi_jpg_msi)
    {
        msi_do_cmd(over_dpi_jpg_msi, MSI_CMD_JPG_THUMB, MSI_JPG_THUMB_TAKEPHOTO_SETPATH, (uint32_t)path);
    }

    struct msi *over_dpi_recode_msi = msi_find(R_SCALE1_JPG_RECODE, 1);
    uint32_t    dpi_w_h             = w << 16 | h;
    if (over_dpi_recode_msi)
    {
        msi_do_cmd(over_dpi_recode_msi, MSI_CMD_SCALE1, MSI_SCALE1_RESET_DPI, dpi_w_h);
        msi_put(over_dpi_recode_msi);
    }

    struct msi *over_dpi_msi = msi_find(S_SCALE3_OVER_DPI, 1);
    if (over_dpi_msi)
    {
        msi_do_cmd(over_dpi_msi, MSI_CMD_TAKEPHOTO_SCALE3, MSI_TAKEPHOTO_SCALE3_KICK, takephoto_num);
        msi_put(over_dpi_msi);
    }
}

//这里的w和h是没有用的,一位内是原来分辨率拍照
void takephoto_normal_func(uint8_t takephoto_num, uint16_t w, uint16_t h, const char *path)
{
    struct msi *jpg_thumb_msi = msi_find(R_JPG_THUMB, 1); // 需要预先创建,否则不会真正拍照
    if (jpg_thumb_msi)
    {
        msi_do_cmd(jpg_thumb_msi, MSI_CMD_JPG_THUMB, MSI_JPG_THUMB_TAKEPHOTO_SETPATH, (uint32_t)path);
        msi_do_cmd(jpg_thumb_msi, MSI_CMD_JPG_THUMB, MSI_JPG_THUMB_TAKEPHOTO, takephoto_num);
        msi_put(jpg_thumb_msi);
    }
}

void usb_takephoto_normal_func(uint8_t takephoto_num, const char *path)
{
    struct msi *usb_jpg_thumb_msi = msi_find(R_JPG_PHOTO_USB, 1);
    if (usb_jpg_thumb_msi)
    {
        msi_do_cmd(usb_jpg_thumb_msi, MSI_CMD_JPG_THUMB, MSI_JPG_THUMB_TAKEPHOTO_SETPATH, (uint32_t)path);
        msi_do_cmd(usb_jpg_thumb_msi, MSI_CMD_JPG_THUMB, MSI_JPG_THUMB_TAKEPHOTO, takephoto_num);
        msi_put(usb_jpg_thumb_msi);
    }
}

static void takephoto_mipi(const char *path)
{
    uint8_t dpi_v = items_value_process("image_size", 0, GET_ITEMS_VALUE);
    takephoto_fn t_fn;
    uint16_t w,h;
    switch(dpi_v)
    {
        case 0:
            t_fn = takephoto_normal_func;
            w = http_dpi[dpi_v][0];
            h = http_dpi[dpi_v][1];
        break;
        case 1:
            t_fn = takephoto_over_dpi_func;
            w = http_dpi[dpi_v][0];
            h = http_dpi[dpi_v][1];
        break;
        case 2:
            t_fn = takephoto_over_dpi_func;
            w = http_dpi[dpi_v][0];
            h = http_dpi[dpi_v][1];
        break;
        case 3:
            t_fn = takephoto_over_dpi_func;
            w = http_dpi[dpi_v][0];
            h = http_dpi[dpi_v][1];
        break;
        case 4:
            t_fn = takephoto_over_dpi_func;
            w = http_dpi[dpi_v][0];
            h = http_dpi[dpi_v][1];
        break;
        default:
            t_fn = takephoto_normal_func;
            w = http_dpi[0][0];
            h = http_dpi[0][1];
        break;
    }

    uint8_t take_photo_num = 1;
    int continue_shot = items_value_process("continue_shot", 0, GET_ITEMS_VALUE);
    switch (continue_shot)
    {
    case 0:
        take_photo_num = 1;
        break;
    case 1:
        take_photo_num = 2;
        break;
    case 2:
        take_photo_num = 10;
        break;
    }

    for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++)
    {
        if(g_cam_configs[i].enable && g_cam_configs[i].src_type == MIPI)
        {
            msi_do_cmd(g_cam_configs[i].photo_msi, MSI_CMD_START, take_photo_num, 0);
            break;  // 暂不考虑两路 mipi 的情况
        }
    }
}

static void takephoto_usb(const char *path)
{
    uint8_t take_photo_num = 1;
    int continue_shot = items_value_process("continue_shot", 0, GET_ITEMS_VALUE);
    switch (continue_shot)
    {
    case 0:
        take_photo_num = 1;
        break;
    case 1:
        take_photo_num = 2;
        break;
    case 2:
        take_photo_num = 10;
        break;
    }
    usb_takephoto_normal_func(take_photo_num, path);
}

static void http_reponse_snapshot(struct httpClient *httpClient)
{
    uint8_t video_num = items_value_process("switchcam", 0, GET_ITEMS_VALUE);

    for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++)
    {
        if((g_cam_configs[i].mask & CAM_MASK_ENABLE) && g_cam_configs[i].enable)
        {
            if(video_num == 0)
            {
                if(g_cam_configs[i].src_type == MIPI)
                {
                    takephoto_mipi(g_cam_configs[i].img_path);
                }
                else if(g_cam_configs[i].src_type == USB)
                {
                    takephoto_usb(g_cam_configs[i].img_path);
                }
                break;
            }
            video_num--;
        }
    }

    http_reponse_success(httpClient);
}

static uint32_t json_return_sdmsg(cJSON **root2, char **post_content)
{
    int res = 1;
	cJSON *root = NULL;
    root = cJSON_CreateObject();
    *root2 = cJSON_CreateObject();
    uint32_t totalsize = 0, freesize = 0;
	int sd_status = 0;
    uint8_t real_status = 0; // 0：SD 卡正常 1：SD 卡异常或未格式化 2：SD 卡未插入 4: SD 卡存储已满
#if FS_EN
    uint8_t sd_status2 = 0;
    dev_walk(DEV_TYPE_SD, sd_ready, &sd_status2);
    sd_status = sd_status2 ? 0 : 2;  // 0: 正常 2: 未插入
#endif
    if(sd_status == 0) {
        uint8_t try_counts = 0;
get_again:
        res = osal_fatfsfree("0:", &totalsize, &freesize);
        if (res == FR_OK) {
        #if SD_FREE_SIZE_EN
            if(freesize < SD_FREE_SIZE_MIN) {
                real_status = 4;
            } else {
                real_status = 0;
            }
        #else
            real_status = 0;
        #endif
            os_printf("sd free size: %d, total size: %d\r\n", freesize, totalsize);
        } else if (res == FR_TIMEOUT) {
            try_counts++;
            if (try_counts <= 3) {
                os_printf("get sd free size timeout, try again\r\n");
                os_sleep_ms(500);
                goto get_again;
            } else {
                real_status = 1;
                os_printf("get sd free size timeout, exit\r\n");
            }
        } else {
            real_status = 1;
        }
    } else {
        real_status = 2;
        _os_printf("sd is not inserted\r\n");
    }

    items_value_process("sdstatus", real_status, SET_ITEMS_VALUE);

    cJSON_AddNumberToObject(root, "status", real_status);
    cJSON_AddNumberToObject(root, "free", freesize);
    cJSON_AddNumberToObject(root, "total", totalsize);

    cJSON_AddNumberToObject(*root2, "result", 0);
    cJSON_AddItemToObject(*root2, "info", root);

    *post_content = cJSON_PrintUnformatted(*root2);
    _os_printf("postcontent: %s\r\n", *post_content);
    _os_printf("postlen: %d\r\n", strlen(*post_content));
    return strlen(*post_content);
}

static void http_reponse_getsdinfo(struct httpClient *httpClient)
{
    cJSON *root = NULL;
    char *post_content = NULL;
    uint32_t sendlen = 0;
    struct httpresp *head;
    int fd = httpClient->fdClient;

    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Connection, "close");
    http_add_header(head, httpIndex.Type, "application/json");
    sendlen = json_return_sdmsg(&root, &post_content);

    if(sendlen) {
        http_header_send(head, httpIndex.Length, sendlen, fd);
        send(fd, post_content, sendlen, 0);
    }

    closeRes(httpClient);
    cJSON_free(post_content);
    cJSON_Delete(root);
}

// 定义一个函数来创建 info 数组中的每个对象
static cJSON *create_info_object(const char *name, const char * const *items, int item_count)
{
    cJSON *info_obj = cJSON_CreateObject();
    if (info_obj == NULL)
    {
        return NULL;
    }

    // 添加 name 字段
    cJSON_AddStringToObject(info_obj, "name", name);

    // 创建 items 数组
    cJSON *items_array = cJSON_CreateArray();
    if (items_array == NULL)
    {
        cJSON_Delete(info_obj);
        return NULL;
    }
    for (int i = 0; i < item_count; i++)
    {
        cJSON_AddItemToArray(items_array, cJSON_CreateString(items[i]));
    }
    cJSON_AddItemToObject(info_obj, "items", items_array);

    // 创建 index 数组
    cJSON *index_array = cJSON_CreateArray();
    if (index_array == NULL)
    {
        cJSON_Delete(info_obj);
        return NULL;
    }
    for (int i = 0; i < item_count; i++)
    {
        cJSON_AddItemToArray(index_array, cJSON_CreateNumber(i));
    }
    cJSON_AddItemToObject(info_obj, "index", index_array);

    return info_obj;
}

uint32_t json_getparamitem(cJSON **root, char **post_content2)
{
    int items_counts = 0;
    // 创建根对象
    *root = cJSON_CreateObject();
    if (*root == NULL)
    {
        return 0;
    }
    // 添加 result 字段
    cJSON_AddNumberToObject(*root, "result", 0);

    // 创建 info 数组
    cJSON *info_array = cJSON_CreateArray();
    if (info_array == NULL)
    {
        cJSON_Delete(*root);
        return 0;
    }
    // 为 info 数组添加每个对象
    for (int i = 0; i < ITEMS_CONFIG_COUNT; i++)
    {
        if(items_cfg[i].const_item->enable == ITEM_ENABLE)
        {
            items_counts = get_items_counts(i);
            cJSON *info_obj = create_info_object(items_cfg[i].const_item->name, items_cfg[i].const_item->items_list, items_counts);
            if (info_obj == NULL)
            {
                cJSON_Delete(*root);
                return 0;
            }
            cJSON_AddItemToArray(info_array, info_obj);
        }
    }
    // 将 info 数组添加到根对象中
    cJSON_AddItemToObject(*root, "info", info_array);

    // 将 JSON 对象转换为字符串
    *post_content2 = cJSON_PrintUnformatted(*root);

    // 打印 JSON 字符串
    _os_printf("postcontent: %s\n", *post_content2);
    _os_printf("postlen: %d\r\n", strlen(*post_content2));
    return strlen(*post_content2);
}

static void http_reponse_getparamitems_all(struct httpClient *httpClient)
{
    cJSON *root = NULL;
    char *post_content = NULL;
    uint32_t sendlen = 0;
    struct httpresp *head;
    int fd = httpClient->fdClient;

    sendlen = json_getparamitem(&root, &post_content);
    if(sendlen == 0)
    {
        closeRes(httpClient);
        return;
    }

    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Connection, "close");
    http_add_header(head, httpIndex.Type, "application/json");

    http_header_send(head, httpIndex.Length, sendlen, fd);
    send(fd, post_content, sendlen, 0);

    closeRes(httpClient);
    cJSON_free(post_content);
    cJSON_Delete(root);
}

void sd_status_push(int fd)
{
	uint8_t new_status = 0;
    uint8_t real_status = 0;
    int last_status = items_value_process("sdstatus", 0, GET_ITEMS_VALUE);
    if(last_status == -1)
    {
        os_printf("get sdstatus failed!\r\n");
        return;
    }
#if FS_EN
    uint8_t new_status2 = 0;
    dev_walk(DEV_TYPE_SD, sd_ready,&new_status2);
    new_status = new_status2 ? 0 : 2;
#endif
    char *push_content = NULL;
    if(new_status == 0) {
    #if SD_FREE_SIZE_EN
        int res = 0;
        uint32_t freesize = 0;
        uint8_t try_counts = 0;
    get_again:
        res = osal_fatfsfree("0:", NULL, &freesize);
        if (res == FR_OK) {
            if(freesize < SD_FREE_SIZE_MIN) {
                real_status = 4;
            } else {
                real_status = 0;
            }
        } else if (res == FR_TIMEOUT) {
            try_counts++;
            if (try_counts <= 3) {
                os_printf("get sd free size timeout, try again\r\n");
                os_sleep_ms(500);
                goto get_again;
            } else {
                real_status = 1;
                os_printf("get sd free size timeout, exit\r\n");
            }
        } else {
            real_status = 1;
            _os_printf("get sd failed\r\n");
        }
    #else
        real_status = 0;
    #endif
    } else {
        real_status = 2;
    }

    if(last_status == real_status)
        return;

    cJSON *root = NULL;
    struct timeval ptimeval;
    gettimeofday(&ptimeval, NULL);
    root = cJSON_CreateObject();
    if (!root)
        goto clean_up;
    cJSON *info = cJSON_CreateObject();
    if (!info)
        goto clean_up;
    
    items_value_process("sdstatus", real_status, SET_ITEMS_VALUE);

    cJSON_AddStringToObject(root, "msgid", "sd");
    cJSON_AddItemToObject(root, "info", info);
    cJSON_AddNumberToObject(root, "time", ptimeval.tv_sec);
    cJSON_AddNumberToObject(info, "status", real_status);
    push_content = cJSON_PrintUnformatted(root);
    _os_printf("postcontent: %s\r\n", push_content);
    _os_printf("postlen: %d\r\n", strlen(push_content));
    if (push_content && fd > 0) {
        send(fd, push_content, strlen(push_content), 0);
    }

clean_up:
    if (push_content) cJSON_free(push_content);
    if (root) cJSON_Delete(root);
}

void lock_status_push(int fd)
{
    static uint8_t lock_status = 0;   // 0：停止加锁 1：开始加锁
    uint8_t lock_status_new = 0;
    char *push_content = NULL;
    lock_status_new = items_value_process("lockvideo", 0, GET_ITEMS_VALUE);
    if(lock_status != lock_status_new)
    {
        cJSON *root = NULL;
        lock_status = lock_status_new;
        root = cJSON_CreateObject();
        if (!root)
            goto clean_up;
        cJSON_AddStringToObject(root, "msgid", "rec_lock");
        cJSON *info = cJSON_CreateObject();
        if (!info)
            goto clean_up;
        cJSON_AddItemToObject(root, "info", info);
        cJSON_AddNumberToObject(info, "value", lock_status > 0 ? 1 : 0);
        push_content = cJSON_PrintUnformatted(root);
        os_printf("postcontent: %s\r\n", push_content);
        os_printf("postlen: %d\r\n", strlen(push_content));
        if (push_content && fd > 0) {
            send(fd, push_content, strlen(push_content), 0);
        }
    clean_up:
        if (push_content) cJSON_free(push_content);
        if (root) cJSON_Delete(root);
    }
}

uint8_t get_mipi_video_status(void)
{
    return 1;
}

uint8_t get_mipi2_video_status(void)
{
    return 1;
}

uint8_t get_usb_video_status(void)
{
    uint8_t camera_num = 0;
    struct sysevt_usbh_video_priv_data info;
    for(int i = 0; i < CONFIG_SYSEVT_USB_VIDEO_MAX_NUM; i++)
    {
        system_event_usbh_video_get_info(i, &info);
        if(info.status == SYSEVT_USB_STATUS_CONNECT)
        {
            camera_num++;
        }
    }
    return camera_num;
}

uint8_t get_camera_status(uint8_t *num)
{
    uint8_t camera_num = 0;
    uint8_t enable = 0;
    for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++)
    {
        if(g_cam_configs[i].mask & CAM_MASK_ENABLE)
        {
            enable = g_cam_configs[i].get_status();
            g_cam_configs[i].enable = enable;
            if(enable)
            {
                camera_num++;
            }
        }
    }

    if(camera_num != *num)
    {
        _os_printf("camera num: %d\r\n", camera_num);
        *num = camera_num;
        items_value_process("cameranum", camera_num, SET_ITEMS_VALUE);
        return 1;
    }

    return 0;
}

void usb_event_disconnect_hdl(void)
{
    os_printf("%s %d\n", __FUNCTION__, __LINE__);
    close_rec();
    uint8_t num = items_value_process("cameranum", 0, GET_ITEMS_VALUE);
    get_camera_status(&num);
    /* uvc 镜头拔出后重新录卡 */
    // open_rec();
}

void usb_event_connect_hdl(void)
{
    os_printf("%s %d\n", __FUNCTION__, __LINE__);
    uint8_t num = items_value_process("cameranum", 0, GET_ITEMS_VALUE);
    get_camera_status(&num);
    /* usb 镜头插入后录卡 */
    // open_rec();
}

void camera_status_push(int fd)
{
    uint8_t camera_status = 0;   // 0：拔出 1：插入
    static uint8_t count = 0;
    static uint8_t last_camera_num = 0;
    if(!count)
    {
        last_camera_num = items_value_process("cameranum", 0, GET_ITEMS_VALUE);
        count = 1;
    }
    uint8_t camera_num = last_camera_num;
    uint8_t change_flag = 0;
    char *push_content = NULL;
    change_flag = get_camera_status(&camera_num);
    if(change_flag)
    {
        cJSON *root = NULL;
        if(camera_num > last_camera_num) {
            camera_status = 1;
        } else if(camera_num < last_camera_num) {
            camera_status = 0;
        } else {
            goto clean_up;
        }
        last_camera_num = camera_num;

        root = cJSON_CreateObject();
        if (!root)
            goto clean_up;
        cJSON_AddStringToObject(root, "msgid", "cam_plugin");
        cJSON *info = cJSON_CreateObject();
        if (!info)
            goto clean_up;
        cJSON_AddItemToObject(root, "info", info);
        cJSON_AddNumberToObject(info, "action", camera_status);
        cJSON_AddNumberToObject(info, "curcamid", 0);
        cJSON_AddNumberToObject(info, "camnum", camera_num);
        push_content = cJSON_PrintUnformatted(root);
        os_printf("postcontent: %s\r\n", push_content);
        os_printf("postlen: %d\r\n", strlen(push_content));
        if (push_content && fd > 0) {
            send(fd, push_content, strlen(push_content), 0);
        }
    clean_up:
        if (push_content) cJSON_free(push_content);
        if (root) cJSON_Delete(root);
    }
}

// 若需要按键或其他APP外的操作更新录卡状态，可以启用
void rec_status_push(int fd)
{
    static uint8_t rec_status = 1;   // 0：录制停止 1：录制启动
    uint8_t rec_status_new = 0;
    char *push_content = NULL;
    // rec_status_new = get_rec_status(sdh);    // rec状态获取函数
    if(rec_status != rec_status_new)
    {
        cJSON *root = NULL;
        struct timeval ptimeval;
        rec_status = rec_status_new;
        root = cJSON_CreateObject();
        if (!root)
            goto clean_up;
        cJSON_AddStringToObject(root, "msgid", "rec");
        cJSON *info = cJSON_CreateObject();
        if (!info)
            goto clean_up;
        cJSON_AddItemToObject(root, "info", info);
        cJSON_AddNumberToObject(info, "status", rec_status);
        gettimeofday(&ptimeval, NULL);
        cJSON_AddNumberToObject(root, "time", ptimeval.tv_sec);
        push_content = cJSON_PrintUnformatted(root);
        os_printf("postcontent: %s\r\n", push_content);
        os_printf("postlen: %d\r\n", strlen(push_content));
        if (push_content && fd > 0) {
            send(fd, push_content, strlen(push_content), 0);
        }
    clean_up:
        if (push_content) cJSON_free(push_content);
        if (root) cJSON_Delete(root);
    }
}

typedef void (*message_handler_t)(int fd);

message_handler_t handlers[] = {
    sd_status_push,
#if APP_LOCK_VIDEO_EN
    lock_status_push,
#endif
    camera_status_push,
    // rec_status_push,   // rec消息推送，根据需要启用
};

static void detect_loop(void *ei, void *d)
{
    int fd = (int)d;
    int handles_num = sizeof(handlers) / sizeof(handlers[0]);

    int err = 0;
    socklen_t len = sizeof(err);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
    if (err != 0) {
        os_printf("socket error: %d, close socket\n", err);
        closesocket(fd);
        eloop_remove_event(ei);
    }

    for(int i = 0; i < handles_num; i++)
    {
        handlers[i](fd);
    }
}

/********************* Initial ********************/

struct item_config {
    /******* 参数区头部: 这部分区域不能修改  *****/
    uint16 magic_num, crc;
    uint16 size, rev1, rev2, rev3;
    /*******************************************/
    uint32_t front_check;
    uint8_t value[ITEMS_CONFIG_COUNT];
    uint32_t rear_check;
};

static struct item_config *item_cfg = NULL;

static void itemcfg_getdefault(void)
{
    for(int i = 0; i < ITEMS_CONFIG_COUNT; i++) {
        if(items_cfg[i].const_item->enable == ITEM_ENABLE)
            item_cfg->value[i] = items_cfg_f[i].const_item->default_value;
    }
}

static void itemcfg_getvalue(void)
{
    for(int i = 0; i < ITEMS_CONFIG_COUNT; i++) {
        if(items_cfg[i].const_item->enable == ITEM_ENABLE)
            item_cfg->value[i] = items_cfg[i].value;
    }
}

static void itemcfg_setvalue(void)
{
    for(int i = 0; i < ITEMS_CONFIG_COUNT; i++) {
        if(items_cfg[i].const_item->enable == ITEM_ENABLE)
            items_cfg[i].value = item_cfg->value[i];
    }
}

static void itemcfg_handle(void)
{
    for(int i = 0; i < ITEMS_CONFIG_COUNT; i++) {
        if(items_cfg[i].const_item->enable == ITEM_ENABLE)
            items_cfg[i].const_item->handler(&items_cfg[i], item_cfg->value[i], NULL);
    }
}

static int itemcfg_save(uint8_t type)
{
    item_cfg->front_check = 0x66666666;
    item_cfg->rear_check = 0x88888888;
    if(type == 1) {
        itemcfg_getvalue();
    } else {
        itemcfg_getdefault();
    }

    return syscfg_write("recorder", item_cfg, sizeof(struct item_config));
}

static void item_cfg_load(void)
{
    if (syscfg_read("recorder", item_cfg, sizeof(struct item_config)) == RET_OK) {
        if(item_cfg->front_check == 0x66666666 && item_cfg->rear_check == 0x88888888) {
            itemcfg_handle();

            os_printf("item_cfg load ok\r\n");
            return;
        }
        os_printf("item_cfg load fail, front_check: 0x%x\trear_check: 0x%x\r\n", item_cfg->front_check, item_cfg->rear_check);
    }

    os_printf("recorder use default params.\r\n");
    itemcfg_save(0);
    itemcfg_handle();
}

static void thread_rec_pool_queue(struct httpserver *httpserver)
{
    int32 ret;
    struct httpClient *httpClient;
    while (1)
    {
        httpClient = (struct httpClient *)os_msgq_get2(&httpserver->thread_pool_queue,-1,&ret);
        if(httpClient && ret == 0)
        {
            _os_printf("%s %d   httpserver->thread_pool_queue:%x\r\n", __FUNCTION__, ret, httpserver->thread_pool_queue);
            http_thread_pool(httpClient, (struct url*)getweb, NULL);
            STREAM_LIBC_FREE(httpClient);
            httpClient = NULL;
        }
    }
}

// 发送有个队列消息
static uint8_t httpqueue_thread_send_pool(struct httpClient *httpClient)
{
    long res;
    res = os_msgq_put(&httpClient->httpserver->thread_pool_queue, (uint32_t)httpClient,-1);
    _os_printf("QueueSend finish:%d!!!\r\n", res);
    if (res != 0)
    {
        closeRes(httpClient);
        STREAM_LIBC_FREE(httpClient);
        _os_printf("QueueSend fail!\r\n");
    }
    return 0;
}

static void do_accept(void *ei, void *d)
{
    SOCK_HDL fd;
    unsigned int i;
    struct sockaddr_in addr;
    struct httpClient *httpClient = initHttpClient();
    if (!httpClient)
    {
        return;
    }
    struct httpserver *httpserver = (struct httpserver *)d;
    _os_printf("httpserver fd:%d\r\n", httpserver->fdServer);

    i = sizeof(addr);
    if ((fd = accept(httpserver->fdServer, (struct sockaddr *)&addr, &i)) < 0)
    {
        os_printf("accept error\r\n");
        STREAM_LIBC_FREE(httpClient);
        return;
    }
    _os_printf("accepted connection from %s: %d\r\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    _os_printf("create new thread fd: %d\r\n", fd);
    httpClient->fdClient = fd;
    httpClient->httpserver = httpserver;
    httpClient->time = os_jiffies();
    httpClient->pattern = NULL;
    httpClient->http_request = NULL;

    struct timeval recv_timeout = { .tv_sec = 1, .tv_usec = 0 };
    int ret = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&recv_timeout, sizeof(recv_timeout));
    if(ret < 0)
    {
        os_printf("error setsockopt TCP recv timeout, ret: %d\r\n", ret);
    }
    struct timeval send_timeout = { .tv_sec = 6, .tv_usec = 0 };
    ret = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&send_timeout, sizeof(send_timeout));
    if(ret < 0)
    {
        os_printf("error setsockopt TCP send timeout, ret: %d\r\n", ret);
    }
    
    httpqueue_thread_send_pool(httpClient);
}

static int Viidure_listen(int port)
{
    struct sockaddr_in addr;
    SOCK_HDL fd;
    struct httpserver *httpserver = STREAM_LIBC_ZALLOC(sizeof(struct httpserver));
    if(httpserver == NULL)
    {
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = 0;
    addr.sin_port = htons(port);

    if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        STREAM_LIBC_FREE(httpserver);
        return -1;
    }

    if (osal_set_reuseaddr(fd, 1) < 0) {}
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        close(fd);
        STREAM_LIBC_FREE(httpserver);
        return -1;
    }
    if (listen(fd, 8) < 0)
    {
        close(fd);
        STREAM_LIBC_FREE(httpserver);
        return -1;
    }
    httpserver->fdServer = fd;
    os_msgq_init(&httpserver->thread_pool_queue,32);

    os_task_create("queue1", (void*)thread_rec_pool_queue, httpserver, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2048);
    os_task_create("queue2", (void*)thread_rec_pool_queue, httpserver, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2048);

    eloop_add_fd(fd, EVENT_READ, EVENT_F_ENABLED, do_accept, (void *)httpserver);

    _os_printf("listening on http port %s:%d\r\n", inet_ntoa(addr.sin_addr), port);

    return 0;
}

static void push_do_accept(void *ei, void *d)
{
    SOCK_HDL fd;
    struct sockaddr_in cliaddr;
    socklen_t clienlen = sizeof(cliaddr);

    if((fd = accept((int)d, (struct sockaddr *)&cliaddr, &clienlen)) < 0)
    {
        return;
    }

    // 设置 TCP keepalive
    int keepalive = 1;          // 启用 keepalive
    int keepidle = 3;           // 5 秒无数据后开始探测
    int keepintvl = 3;          // 每隔 5 秒探测一次
    int keepcnt = 3;            // 探测 3 次失败后判定连接断开

    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive)) < 0) {
        os_printf("setsockopt SO_KEEPALIVE failed\n");
        close(fd);
        return;
    }

    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle)) < 0) {
        os_printf("setsockopt TCP_KEEPIDLE failed\n");
    }

    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl)) < 0) {
        os_printf("setsockopt TCP_KEEPINTVL failed\n");
    }

    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt)) < 0) {
        os_printf("setsockopt TCP_KEEPCNT failed\n");
    }

    _os_printf("accepted push connection from %s: %d\r\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
    _os_printf("create new push fd: %d\r\n", fd);

    eloop_add_timer(1500, EVENT_F_ENABLED, detect_loop, (void *)fd);
}

static int push_tcp_server(int port)
{
    SOCK_HDL fd;
    struct sockaddr_in servaddr;

    // 创建socket
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
       os_printf("tcp socket create failed\n");
       return -1;
    }

    // 绑定本地地址
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
       os_printf("tcp bind failed\n");
       close(fd);
       return -1;
    }

    // 监听端口
    if (listen(fd, 5) < 0) {
       os_printf("tcp listen failed\n");
       close(fd);
       return -1;
    }

    eloop_add_fd(fd, EVENT_READ, EVENT_F_ENABLED, push_do_accept, (void *)fd);

    _os_printf("listening on push port %s:%d\r\n", inet_ntoa( servaddr.sin_addr ), port );

    return 0;
}

static int32_t recorder_save_loop(struct os_work *work)
{
    int saveflag = 0;
    saveflag = items_value_process("saveflag", 0, GET_ITEMS_VALUE);
    if(saveflag) {
        if(itemcfg_save(1) != -1)
        {
            saveflag = 0;
            items_value_process("saveflag", saveflag, SET_ITEMS_VALUE);
        }
    }

    os_run_work_delay(&recorder_wk, 3000);
	return 0;
}

/********************* GLOBAL CONFIGURATION DIRECTIVES ********************/
int config_Viidure(int port)
{
    if (port <= 0 || port > 65535)
    {
        _os_printf("invalid listen port %d", port);
        return -1;
    }

    if(!items_cfg)
    {
        items_cfg = STREAM_LIBC_MALLOC(sizeof(struct items_config) * ITEMS_CONFIG_COUNT);
        if(!items_cfg) {
            _os_printf("items_cfg malloc failed\r\n");
            return -1;
        }
        os_memcpy(items_cfg, items_cfg_f, sizeof(struct items_config) * ITEMS_CONFIG_COUNT);
    }

    if(!item_cfg)
    {
        item_cfg = STREAM_LIBC_ZALLOC(sizeof(struct item_config));
        if(!item_cfg)
        {
            _os_printf("item_cfg malloc failed\r\n");
            STREAM_LIBC_FREE(items_cfg);
            items_cfg = NULL;
            return -1;
        }
    }

    if(!g_cam_configs)
    {
        g_cam_configs = STREAM_LIBC_ZALLOC(sizeof(struct cam_cfg) * CAMERAS_CONFIG_COUNT);
        if(!g_cam_configs)
        {
            _os_printf("g_cam_configs malloc failed\r\n");
            STREAM_LIBC_FREE(item_cfg);
            item_cfg = NULL;
            STREAM_LIBC_FREE(items_cfg);
            items_cfg = NULL;
            return -1;
        }
        os_memcpy(g_cam_configs, g_cam_configs_f, sizeof(struct cam_cfg) * CAMERAS_CONFIG_COUNT);
    }

    item_cfg_load();

    uint8_t camera_num = 0;
    get_camera_status(&camera_num);

    OS_WORK_INIT(&recorder_wk, recorder_save_loop, 0);
    os_run_work_delay(&recorder_wk, 3000);

    if(push_tcp_server(PUSH_MESSAGE_PORT) != 0)
    {
        _os_printf("push tcp server init failed, port: %d\r\n", PUSH_MESSAGE_PORT);
    }

    //接收某个镜头的数据
    for(int i = 0; i < CAMERAS_CONFIG_COUNT; i++)
    {
        if(g_cam_configs[i].enable && g_cam_configs[i].src_type == MIPI)
        {
            g_cam_configs[i].photo_msi = new_takephoto_msi(FRAMEBUFF_SOURCE_CAMERA0, FSTYPE_NONE);
            msi_add_output(NULL, AUTO_JPG, g_cam_configs[i].photo_msi, NULL);
            break; // 暂不考虑两路 mipi 的情况
        }
    }

    return Viidure_listen(port);
}
