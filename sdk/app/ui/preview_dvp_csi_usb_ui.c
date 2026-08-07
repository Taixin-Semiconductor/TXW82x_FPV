/*********************************************************************************************************** 
这个主要是 dvp + csi + uvc(mjpeg) 实现视频预览，最多支持4个视频源输出预览

S_PREVIEW_SCALE3 ----> R_SIM_VIDEO(320x180) ----> R_VIDEO_P0
(scale3)                (虚拟屏幕)

/dev/video0  ---->  ROUTE_USB  ---->  SR_OTHER_JPG_USB1   ---->   S_JPG_DECODE ----> R_SIM_VIDEO(320x180) ----> R_VIDEO_P0
(uvc)              (uvc中转流)       (uvc的jpg,配置解码size)     (解码成yuv:320x180)    (虚拟屏幕)        


/dev/video1  ---->  ROUTE_USB  ---->  SR_OTHER_JPG_USB2   ---->   S_JPG_DECODE ----> R_SIM_VIDEO(320x180) ----> R_VIDEO_P0
(uvc)              (uvc中转流)       (uvc的jpg,配置解码size)     (解码成yuv:320x180)    (虚拟屏幕)       

********************************************************************************************************** */
#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "stream_frame.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"


//data申请空间函数
#define STREAM_MALLOC     av_psram_malloc
#define STREAM_FREE       av_psram_free
#define STREAM_ZALLOC     av_psram_zalloc

//结构体申请空间函数
#define STREAM_LIBC_MALLOC     av_malloc
#define STREAM_LIBC_FREE       av_free
#define STREAM_LIBC_ZALLOC     av_zalloc


extern lv_indev_t * indev_keypad;
extern lv_style_t g_style;
struct preview_usb_s
{
    lv_group_t  *last_group;
    lv_obj_t    *base_ui;

    lv_group_t    *now_group;
    lv_obj_t    *now_ui;

//这个是需要的流,但命名不一定对应,有可能通过中间流进行中转实现
    struct msi *sim_video;
    struct msi *scale3_s;
    struct msi *decode_s;
    struct msi *USB_P0_jpg_s;
    struct msi *USB_P1_jpg_s;
    struct msi *route_usb_msi;

};

static void exit_preview_dvp_csi_usb_ui(lv_event_t * e)
{
    struct preview_usb_s *ui_s = (struct preview_usb_s *)lv_event_get_user_data(e);
    lv_indev_set_group(indev_keypad, ui_s->last_group);
    lv_obj_clear_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_group_del(ui_s->now_group);
    ui_s->now_group = NULL;
    msi_destroy(ui_s->route_usb_msi);
    msi_destroy(ui_s->sim_video);
    msi_destroy(ui_s->scale3_s);
    msi_destroy(ui_s->decode_s);
    msi_destroy(ui_s->USB_P0_jpg_s);
    msi_destroy(ui_s->USB_P1_jpg_s);
    msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);   
    lv_obj_del(ui_s->now_ui);
}
//进入预览的ui,那么就要创建新的ui去显示预览图了
static void enter_preview_dvp_csi_usb_ui(lv_event_t * e)
{
    struct preview_usb_s *ui_s = (struct preview_usb_s *)lv_event_get_user_data(e);
    lv_obj_add_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *ui = lv_obj_create(lv_scr_act());  
    ui_s->now_ui = ui;
    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));


    static const uint16_t filter[] = { FSTYPE_YUV_P0, FSTYPE_YUV_P1, FSTYPE_YUV_SIM_P0, FSTYPE_USB_CAM0, FSTYPE_USB_CAM1, FSTYPE_NONE };
    ui_s->sim_video = sim_video_more_msi(R_SIM_VIDEO, 800, 480, (uint16_t*)filter);
    if (ui_s->sim_video)
    {
        msi_add_output(ui_s->sim_video, NULL, R_VIDEO_P0);
        msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 1);
    }


    ui_s->scale3_s = scale3_msi(S_PREVIEW_SCALE3);
    //绑定流到Video P1显示
    if (ui_s->scale3_s)
    {
        msi_add_output(ui_s->scale3_s, NULL, R_SIM_VIDEO);
    }


    ui_s->decode_s = jpg_decode_msi(S_JPG_DECODE);
    //将解码的数据推送到Video P0
    if (ui_s->decode_s)
    {
        msi_add_output(ui_s->decode_s, NULL, R_SIM_VIDEO);
    }


    //接收UVC的jpg,配置解码需要的参数(320x180),最后给到解码模块
    ui_s->USB_P0_jpg_s = jpg_decode_msg_msi(SR_OTHER_JPG_USB1,320,180,320,180,FSTYPE_USB_CAM0);
    //将other_jpg的数据给到S_JPG_DECODE进行编码
    if (ui_s->USB_P0_jpg_s)
    {
        // 配置解码的坐标值
        msi_do_cmd(ui_s->USB_P0_jpg_s, MSI_CMD_DECODE_JPEG_MSG, MSI_JPEG_DECODE_X_Y, 0 << 16 | 220);
        msi_add_output(ui_s->USB_P0_jpg_s, NULL, S_JPG_DECODE);
    }
    
    //接收usb的jpg数据,配置需要解码的参数(320x180),然后给到S_JPG_DECODE进行解码,最后给到P1去显示(设置时P1的类型)
    ui_s->USB_P1_jpg_s = jpg_decode_msg_msi(SR_OTHER_JPG_USB2,320,180,320,180,FSTYPE_USB_CAM1);
    //将other_jpg的数据给到S_JPG_DECODE进行编码
    if (ui_s->USB_P1_jpg_s)
    {
        // 配置解码的坐标值
        msi_do_cmd(ui_s->USB_P1_jpg_s, MSI_CMD_DECODE_JPEG_MSG, MSI_JPEG_DECODE_X_Y, 360 << 16 | 220);
        msi_add_output(ui_s->USB_P1_jpg_s, NULL, S_JPG_DECODE);
    }

    ui_s->route_usb_msi = route_msi(ROUTE_USB);
    if (ui_s->route_usb_msi)
    {
        msi_add_output(ui_s->route_usb_msi, NULL, SR_OTHER_JPG_USB1);
        msi_add_output(ui_s->route_usb_msi, NULL, SR_OTHER_JPG_USB2);
    }


    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);


    lv_group_add_obj(group, ui);
    ui_s->now_group = group;
    lv_obj_add_event_cb(ui, exit_preview_dvp_csi_usb_ui, LV_EVENT_PRESSED, ui_s);
}

lv_obj_t *preview_dvp_csi_usb_ui(lv_group_t *group,lv_obj_t *base_ui)
{
    struct preview_usb_s *ui_s = (struct preview_usb_s*)STREAM_LIBC_ZALLOC(sizeof(struct preview_usb_s));
    ui_s->last_group = group;
    ui_s->base_ui = base_ui;
    lv_obj_t *btn =  lv_list_add_btn(base_ui, NULL, "preview_dvp_csi_usb");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_preview_dvp_csi_usb_ui, LV_EVENT_PRESSED, ui_s);
    return btn;
}