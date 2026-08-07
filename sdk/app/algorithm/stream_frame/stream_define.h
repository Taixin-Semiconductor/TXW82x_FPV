#ifndef __STREAM_DEFINE_H
#define __STREAM_DEFINE_H

// 用于定义各种data的类型枚举
enum
{
    DATA_TYPE_ANY,
    DATA_TYPE_AUDIO_PDM,
    DATA_TYPE_AUDIO_I2S,
    DATA_TYPE_AUDIO_ADC,
    DATA_TYPE_AUDIO_DAC,
    DATA_TYPE_JPEG_VGA,
    DATA_TYPE_JPEG_720P,
};

// 定义各种流的名称,R开头代表接收的流,S开头是发送的流,SR代表既要接收又要发送的流
// 名称不能相同

// 没有专门发送和接收,只是流负责管理
#define MANAGE_STREAM_NAME          "manage-stream"
// R
#define R_RECORD_AUDIO              "record-audio" // 录像的音频
#define R_RECORD_JPEG               "record-jpeg"  // 录像的音频
#define R_RECORD_H264               "record-h264"  // 录像的音频
#define R_PHOTO_JPEG                "photo-jpeg"   // 录像的音频
#define R_ALK_JPEG                  "alk-jpeg"
#define R_AUDIO_TEST                "audio-test"
#define R_SPEAKER                   "speaker"
#define R_JPEG_TO_LCD               "JPEG-LCD"
#define R_JPEG_DEMO                 "jpeg_demo"
#define R_USB_AUDIO_MIC             "usb-audio-mic"
#define R_RTP_AUDIO                 "rtp-audio"     // 图传的音频
#define R_RTP_AUDIO2                "rtp-audio2"    // 图传的音频
#define R_RTP_JPEG                  "rtp-jpeg"      // 图传的视频
#define R_RTP_JPEG_H264             "rtp-jpeg-h264" // 图传的视频
#define R_LVGL_JPEG                 "lvgl-jpeg"     // lvgl接收回放适配
#define R_LVGL_TAKEPHOTO            "lvgl_takephoto"
#define R_LOWPOERRESUME_EVENT       "LOWPOERRESUME_EVENT"
#define R_AT_SAVE_OSD               "AT_OSD"
#define R_AT_SAVE_AUDIO             "AT_AUDIO"
#define R_AT_SAVE_PHOTO             "AT_PHOTO"
#define R_AT_AVI_AUDIO              "AT_AVI_AUDIO" // 录像的音频
#define R_AT_AVI_JPEG               "AT_AVI_JPEG"  // 录像的音频
#define R_SAVE_STREAM_DATA          "stream_data"
#define R_INTERMEDIATE_DATA         "R_Intermediate_data"
#define R_AVI_SAVE_VIDEO            "AVI_SAVE_VIDEO"
#define R_AVI_SAVE_AUDIO            "AVI_SAVE_AUDIO"
#define R_INTERCOM_AUDIO            "INTERCOM_SEND"
#define R_PSRAM_JPEG                "PSRM-jpeg" // 接收到psram的视频
#define R_USB_SPK                   "usb_speaker"
#define R_OSD_ENCODE                "osd_encode"
#define R_OSD_SHOW                  "osd_show"
#define R_LCD_OSD                   "LCD_OSD"
#define R_VIDEO_P1                  "VIDEO_P1"
#define R_VIDEO_P0                  "VIDEO_P0"
#define R_SIM_VIDEO                 "SIM_VIDEO"
#define R_JPG_TO_RGB                "JPG_TO_RGB"
#define R_YUV_TO_JPG                "YUV_TO_JPG"
#define R_ZBAR                      "Zbar"
#define R_PRC                       "prc"
#define RS_JPG_CONCAT               "JPG_CONCAT"  // 用于jpg重新组成framebuff
#define RS_JPG_CONCAT1              "JPG_CONCAT1" // 用于jpg1重新组成framebuff
#define R_GEN420_THUMB_JPG          "gen420_thumb_jpg"
#define R_GEN420_THUMB_JPG_OVER_DPI "gen420_thumb_jpg_over_dpi"
#define R_GEN420_JPG_RECODE         "gen420_JPG_RECODE"
#define R_SCALE1_JPG_RECODE         "scale1_JPG_RECODE"
#define R_DEBUG_STREAM              "debug_stream"
#define R_SONIC_PROCESS             "r_sonic_process"
#define R_JPG_THUMB                 "JPG_THUMB"
#define R_MP4_THUMB                 "MP4_THUMB"
#define R_THUMB                     "THUMB"
#define R_JPG_SAVE                  "JPG_SAVE"
#define R_JPG_DECODE_MSG            "JPG_DECODE_MSG"
#define R_THUMB_DECODE_MSG          "THUMB_DECODE_MSG"
#define R_USBD_VIDEO                "usbd_video_msi"
#define R_FILE_MSI                  "file_msi"
#define R_RTP_H264                  "rtp-h264" // 图传的视频
#define R_AVI_ENCODE_MSI            "avi_encode_msi"
// S
#define S_PDM                       "pdm"
#define S_ADC_AUDIO                 "adc_audio"
#define S_MP3_AUDIO                 "mp3_audio"
#define S_AMRNB_AUDIO               "amrnb_audio"
#define S_AMRWB_AUDIO               "amrwb_audio"
#define S_JPEG                      "jpeg"
#define S_USB_JPEG                  "usb-jpeg"
#define S_USB_JPEG_PSRAM            "usb-jpeg-psram"
#define S_WEBJPEG                   "web-jpeg"
#define S_PLAYBACK                  "playback-jpeg"
#define S_WEBAUDIO                  "web-audio"
#define S_NET_JPEG                  "net-jpeg"
#define S_USB_DUAL                  "usb-dual"
#define S_INTERCOM_AUDIO            "INTERCOM_RECV"
#define S_INTERMEDIATE_DATA         "S_Intermediate_data"
#define S_USB_MIC                   "usb_microphone"
#define S_LVGL_OSD                  "lvgl_osd"
#define S_JPG_DECODE                "jpg_decode"
#define S_LVGL_JPG                  "lvgl_jpg"
#define S_PREVIEW_SCALE3            "preview_scale3"
#define S_SCALE1_STREAM             "scale1_stream"
#define S_PREVIEW_ENCODE_QVGA       "preview_encode_qvga"
#define S_PREVIEW_ENCODE_QQVGA      "preview_encode_qqvga"
#define S_PREVIEW_ENCODE_VPP        "preview_encode_vpp"
#define S_SONIC_PROCESS             "s_sonic_process"
#define S_NEWPLAYER                 "newplayer"
#define S_NEWAVI                    "newavi"
#define S_ZBAR_FROM_SD              "zbar_read_sd"
#define S_LVGL_PHOTO                "lvgl_photo"
#define S_LVGL_OSD                  "lvgl_osd"
#define S_SCALE3_OVER_DPI           "scale3_over_dpi"
#define S_H264                      "h264"
#define S_SCALE3_720P               "scale3_720p"
#define S_AVI_PLAYER                "avi_player"
#define S_DEBUG_STREAM              "s_debug_stream"

// SR
#define SR_OTHER_JPG                "other_jpg"
#define SR_OTHER_JPG_USB1           "other_jpg_usb1"
#define SR_OTHER_JPG_USB2           "other_jpg_usb2"
#define SR_OTHER_JPG_USB3           "other_jpg_usb3"
#define SR_USB_RECODE_DEOCDE        "usb_recode_decode"
#define SR_YUV_WATERMARK            "YUV_watermark"
#define SR_VIDEO_USB                "video_usb"
#define SR_ZBAR_JPG                 "zbar_parse"
#define SR_OVER_DPI_JPG             "over_dpi_jpg"
#define SR_OVER_DPI_THUMB_JPG       "over_dpi_thumb"
#define SR_GEN420_720P_JPG          "GEN420_720P_MJPG"
#define ROUTE_USB                   "route-usb"
#define AUTO_JPG                    "auto-jpg"
#define AUTO_H264                    "auto-h264"

// 高16位是大类型(统一宏),低16位是细分类型
#define SET_DATA_TYPE(type1, type2) (type1 << 16 | type2)
#define GET_DATA_TYPE1(type)        (type >> 16)
#define GET_DATA_TYPE2(type)        (type & 0xffff)

enum data_type1
{
    TYPE1_NONE,
    JPEG = 1,
    SOUND,
    H264,

};

// 0保留
enum DATA_TYPE2_RECV_ALL
{
    RESEVER_VALUE,
    RECV_ALL = RESEVER_VALUE,
};

// 定义mjpeg的类型,最好自己将不同类型归类
enum JPEG_data_type2
{
    JPEG_DVP_NODE = RESEVER_VALUE + 1, // 采用特殊的节点方式
    JPEG_DVP_FULL,                     // 采用整张图保存的方式(在空间足够下,采用正常图片形式,正常是带psram才会使用)
    JPEG_USB,
    JPEG_FILE,
};

// 定义声音的类型
enum SOUND_data_type2
{
    SOUND_ALL = RECV_ALL,
    SOUND_NONE, // 不接收声音,源头不要产生这个声音就好了
    SOUND_MIC,
    SOUND_FILE,
    SOUND_USB_SPK,
};

// 设置发送的命令,只有type1是在这里定义,type2由各个模块去定义(这个就不再有固定,因为自定义,所以在要自己在各个STREAM_SEND_CMD里面去处理识别)

#define SET_CMD_TYPE(type1, type2) (type1 << 16 | type2)
#define GET_CMD_TYPE1(type)        (type >> 16)
#define GET_CMD_TYPE2(type)        (type & 0xffff)

enum cmd_type1
{
    CMD_NONE,
    CMD_AUDIO_DAC,
    CMD_JPEG_FILE,
};

struct jpg_node_s
{
    uint16_t w, h;
    uint32_t jpg_len;
};

struct fb_h264_s
{
    uint8_t *pps;
    uint8_t *sps;
    uint16_t pps_len;
    uint16_t sps_len;
    uint8_t  count;
    uint8_t  type;      // 0:无效  1:I帧  2:P帧  3:B帧
    uint8_t  start_len; // 读取实际内容的的起始长度(就是偏移到I帧、P帧、B帧头第一byte)
};

enum YUV_ARG_TYPE
{
    YUV_ARG_NONE,
    YUV_ARG_NORMAL,
    YUV_ARG_DECODE,
    YUV_ARG_TAKEPHOTO,
};
// yuv通用参数
struct yuv_arg_s
{
    uint32_t type; // 不同类型的yuv代表携带的信息不一致?(暂时没有想到更好方法,暂时通过这个去识别,如果某些模块只要yuv,是可以不识别的)
    uint32_t y_size;
    uint32_t y_off;
    uint32_t uv_off;
    uint32_t out_w;
    uint32_t out_h;
    // 状态位,如果发现要求del,那么接收流就要尽快释放了,不要占用当前节点
    uint16_t x, y;
    uint8_t *del;
    uint32_t dispcnt;
    uint32_t magic; // 一个类似随机数?有些msi可以通过识别这个magic来判断是否为自己所需要的数据};
};

// 解码,yuv通用参数放前面,与yuv_arg_s保持一致
struct jpg_decode_arg_s
{
    struct yuv_arg_s yuv_arg;
    uint32_t         rotate;
    uint32_t         decode_w; // 解码图片的size
    uint32_t         decode_h;
    uint32_t         step_w;
    uint32_t         step_h;
};

struct takephoto_yuv_arg_s
{
    struct yuv_arg_s yuv_arg;
    char          name[64];
};

enum YUV_data_type2
{
    YUV_P0    = BIT(0),
    YUV_P1    = BIT(1),
    // 如果YUV需要编码,那么值可能与JPG的值会有关联,这个要好好想想怎么去处理
    YUV_OTHER = BIT(2),
};

struct file_msg_s
{
    uint32_t type;
    uint8_t  path[64];
    uint8_t  ishid;
};

enum JPG_HARDWARE_data_type2
{
    // 暂定双镜头,如果有更多需要区分,可以先添加
    JPG_Camera0  = BIT(0),
    JPG_Camera1  = BIT(1),
    // 下面算是自定义,比如gen420可以从各个地方来源,所以没有固定的名字
    // 这里可能部分需要与YUV一致？因为编码从GEN420过来的,所以类型会与GEN420的源头会有关联
    // 如果有关联,就要考虑映射关系了,暂时先直接用吧
    JPG_GEN420_0 = BIT(2),
};

enum RGB_data_type2
{
    LVGL_RGB = BIT(0),
};

// MSI的命令暂时定义这里,后续需要整理
enum MSI_SELF_CMDs
{
    // osd使用
    MSI_OSD_HARDWARE_REAY_CMD,
    MSI_OSD_HARDWARE_ENCODE_SET_LEN,
    MSI_OSD_HARDWARE_ENCODE_GET_LEN,
    MSI_OSD_HARDWARE_ENCODE_BUF,
    MSI_OSD_HARDWARE_DEV,
    MSI_OSD_HARDWARE_STREAM_RESET,
    MSI_OSD_HARDWARE_STREAM_STOP,
};

enum MSI_SCALE3_CMD
{
    MSI_SCALE3_START,
};

enum MSI_SCALE1_CMD
{
    MSI_SCALE1_RESET_DPI,
    MSI_SCALE1_STOP,
};

enum MSI_SCALE2_CMD
{
    MSI_SCALE2_START,
};

enum MSI_LCD_VIDEO_CMD
{
    // osd使用
    MSI_VIDEO_ENABLE,
};

enum MSI_JPEG0_CMD
{
    MSI_JPEG_STOP,
    MSI_JPEG_RESET_FROM,
    MSI_JPEG_RESET_DPI,
    MSI_PRC_MJPEG_KICK,
    MSI_PRC_REGISTER_ISR,
};

enum MSI_JPEG_CONCAT
{
    MSI_JPEG_OPEN,
    MSI_JPEG_MSG,
    MSI_JPEG_START,
    MSI_JPEG_FROM,
    MSI_JPEG_NODE_COUNT,
    MSI_GET_JPEG_MSI,
    MSI_SET_GEN420_TYPE,
    MSI_SET_SCALE1_TYPE,
    MSI_SET_TIME,
};

enum MSI_JPEG_HARDWARE
{
    MSI_JPEG_HARDWARE_MSG,
    MSI_JPEG_HARDWARE_START,
    MSI_JPEG_HARDWARE_STOP,
    MSI_JPEG_HARDWARE_FROM,
    MSI_JPEG_HARDWARE_SET_GEN420_TYPE,
    MSI_JPEG_HARDWARE_SET_SCALE1_TYPE,
    MSI_JPEG_SET_TIME,
};

enum MSI_AUTO_JPG
{
    MSI_AUTO_JPG_SWITCH_ENCODE_SRC,  
};

enum MSI_JPG_RECODE
{
    MSI_JPG_RECODE_MAGIC,
    MSI_JPG_RECODE_FORCE_TYPE
};

enum MSI_JPEG_DECODE_MSG
{
    MSI_JPEG_DECODE_X_Y,
    MSI_JPEG_DECODE_FORCE_TYPE,
    MSI_JPEG_DECODE_MAGIC,
};

enum MSI_JPG_THUMB
{
    MSI_JPG_THUMB_TAKEPHOTO,
};

enum MSI_DECODE_CMD
{
    MSI_DECODE_SET_W_H,
    MSI_DECODE_SET_IN_OUT_SIZE,
    MSI_DECODE_SET_STEP,
    MSI_DECODE_READY,
    MSI_DECOE_START,
};

enum MSI_PLAYER_CMD
{
    MSI_FORWARD_INDEX,
    MSI_REWIND_INDEX,
    MSI_PLAYER_MAGIC,
    MSI_PLAYER_STATUS,
};

enum MSI_MEDIA_PLAYER_CMD
{
    MSI_MEDIA_ADD_FILE_MSI,
    MSI_MEDIA_ADD_OUTPUT,
    MSI_MEDIA_DEL_OUTPUT,
    MSI_MEDIA_PAUSE,
    MSI_MEDIA_PLAY,
    MSI_MEDIA_PLAY_1FPS,
};

enum MSI_MEDIA_CTRL_CMD
{
    MSI_MEDIA_CTRL_PLAY,
    MSI_MEDIA_CTRL_PLAY_1FPS,
};

enum MSI_VIDEO_DEMUX_CTRL_CMD
{
    MSI_VIDEO_DEMUX_JMP_TIME,
    MSI_VIDEO_DEMUX_FORWARD_TIME,
    MSI_VIDEO_DEMUX_REWIND_TIME,
    MSI_VIDEO_DEMUX_SET_STATUS,
    MSI_VIDEO_DEMUX_GET_STATUS,
    MSI_VIDEO_DEMUX_START,
};

enum MSI_TAKEPHOTO_SCALE3_CMD
{
    MSI_TAKEPHOTO_SCALE3_KICK,
    MSI_TAKEPHOTO_SCALE3_STOP,
};
#endif
