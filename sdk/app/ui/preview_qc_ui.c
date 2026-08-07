/*********************************************************************************************************** 
这个主要是 dvp + csi + uvc(mjpeg) 实现视频预览，最多支持4个视频源输出预览

S_PREVIEW_SCALE3 ----> R_SIM_VIDEO(320x180) ----> R_VIDEO_P0
(scale3)                (虚拟屏幕)

/dev/video0  ---->  ROUTE_USB  ---->  SR_OTHER_JPG_USB1   ---->   S_JPG_DECODE ----> R_SIM_VIDEO(320x180) ----> R_VIDEO_P0
(uvc)              (uvc中转流)       (uvc的jpg,配置解码size)     (解码成yuv:320x180)    (虚拟屏幕)        


S_LVGL_PHOTO  ---->  SR_OTHER_JPG_USB2   ---->   S_JPG_DECODE ----> R_SIM_VIDEO(320x180) ----> R_VIDEO_P0
(sd)                (sd的jpg,配置解码size)     (解码成yuv:320x180)    (虚拟屏幕)       

********************************************************************************************************** */
#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "stream_frame.h"
#include "fatfs/osal_file.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

#define CHECK_DIR   "photo"
#define EXT_NAME    "*jpg"

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
struct preview_qc_s
{
    struct os_work work;
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
    struct msi *photo_s;
    struct fbpool tx_pool;
    struct msi *route_usb_msi;

};

static void play_photo(void *d)
{   
    void *fp = NULL;
    void *photo_buf = NULL;
	struct framebuff *data_s = NULL;
    struct preview_qc_s *ui_s = (struct preview_qc_s *)d;
    uint32_t len;
    char path[64];
    os_sprintf(path,"0:%s/%s",CHECK_DIR,"qc.jpg");
	if(!ui_s->photo_s)
	{
        os_printf("%s %d\n",__FUNCTION__,__LINE__);
		goto play_photo_fail;
	}

    //读取图片文件,通过流去发送
    data_s = fbpool_get(&ui_s->tx_pool, 0, ui_s->photo_s);
    if(data_s)
    {
        data_s->data = NULL;
        //开始显示图片
        fp = osal_fopen(path,"rb");
        if(fp)
        {
            uint32_t filesize = osal_fsize(fp);
            photo_buf = (void*)STREAM_MALLOC(filesize);
            if(!photo_buf)
            {
                msi_delete_fb(ui_s->photo_s, data_s);
				data_s = NULL;
                os_printf("%s %d\n",__FUNCTION__,__LINE__);
                goto play_photo_fail;
            }
            else
            {
                len = osal_fread(photo_buf,1,filesize,fp);
                if(len != filesize)
                {
                    os_printf("%s %d\n",__FUNCTION__,__LINE__);
                    goto play_photo_fail;
                }
                data_s->data = (void*)photo_buf;
                data_s->mtype = F_JPG;
                data_s->stype = FSTYPE_YUV_SIM_P0;
                data_s->len = filesize;
                
                msi_output_fb(ui_s->photo_s, data_s);
                photo_buf = NULL;
                data_s = NULL;
                goto play_photo_end;
            }
            
        }
        os_printf("%s %d\n",__FUNCTION__,__LINE__);
        goto play_photo_fail;


    }
play_photo_fail:
    //显示失败
    os_sprintf(path,"0:%s/%s fail",CHECK_DIR,"qc.jpg");
    os_printf("----------------------------------------\n");
    if(photo_buf)
    {
        STREAM_FREE(photo_buf);
    }
    if(data_s)
    {
        msi_delete_fb(NULL, data_s);
    }

play_photo_end:
    if(fp)
    {
        osal_fclose(fp);
        fp = NULL;
    }


}

static int32 photo_msi_work(struct os_work *work)
{
    struct preview_qc_s *ui_s = (struct preview_qc_s *)work;
    play_photo(ui_s);
    os_run_work_delay(&ui_s->work, 200);
	return 0;
}

static int32 photo_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int32_t ret = RET_OK;
    struct preview_qc_s *ui_s = (struct preview_qc_s *)msi->priv;

    switch (cmd_id)
    {
        case MSI_CMD_PRE_DESTROY:
        {
            os_work_cancle2(&ui_s->work, 1);
        }
            break;

        case MSI_CMD_POST_DESTROY:
        {
            fbpool_destroy(&ui_s->tx_pool);
            os_printf("%s %d\n",__FUNCTION__,__LINE__);
        }
            break;
        
        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;
            if (fb->data)
            {
                STREAM_FREE(fb->data);
                fb->data = NULL;
            }
            fbpool_put(&ui_s->tx_pool, fb);
            ret = RET_OK + 1;
        }
            break;

        default:
            break;
    }

    return ret;
}


//创建播放器的流,这个流主要实现的是控制速度转发作用,暂时控制的是视频
//如果要控制音频,需要计算时间来控制速度
static struct msi *photo_stream(const char *name)
{
    struct msi *s = msi_new(name, 0, NULL);
    return s;
}


static void exit_preview_qc_ui(lv_event_t * e)
{
    struct preview_qc_s *ui_s = (struct preview_qc_s *)lv_event_get_user_data(e);
    lv_indev_set_group(indev_keypad, ui_s->last_group);
    lv_obj_clear_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_group_del(ui_s->now_group);
    ui_s->now_group = NULL;
    msi_destroy(ui_s->photo_s);
    msi_destroy(ui_s->route_usb_msi);
    msi_destroy(ui_s->sim_video);
    msi_destroy(ui_s->scale3_s);
    msi_destroy(ui_s->decode_s);
    msi_destroy(ui_s->USB_P0_jpg_s);
    msi_destroy(ui_s->USB_P1_jpg_s);
    msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);   
    msi_cmd(R_VIDEO_P1, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);
    lv_obj_del(ui_s->now_ui);
}
//进入预览的ui,那么就要创建新的ui去显示预览图了
static void enter_preview_qc_ui(lv_event_t * e)
{
    struct preview_qc_s *ui_s = (struct preview_qc_s *)lv_event_get_user_data(e);
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
    
    ui_s->photo_s = photo_stream(S_LVGL_PHOTO);
    if (ui_s->photo_s)
    {
        ui_s->photo_s->priv = (void *)ui_s;
        fbpool_init(&ui_s->tx_pool, 4);
        ui_s->photo_s->action = photo_msi_action;
        msi_add_output(ui_s->photo_s, NULL, SR_OTHER_JPG_USB2);
        ui_s->photo_s->enable = 1;
    }

    //接收usb的jpg数据,配置需要解码的参数(320x180),然后给到S_JPG_DECODE进行解码,最后给到P1去显示(设置时P1的类型)
    ui_s->USB_P1_jpg_s = jpg_decode_msg_msi(SR_OTHER_JPG_USB2,320,180,320,180,FSTYPE_YUV_SIM_P0);
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
    }

    OS_WORK_INIT(&ui_s->work, photo_msi_work, 0);
    os_run_work_delay(&ui_s->work, 1);

    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);


    lv_group_add_obj(group, ui);
    ui_s->now_group = group;
    lv_obj_add_event_cb(ui, exit_preview_qc_ui, LV_EVENT_PRESSED, ui_s);
}

lv_obj_t *preview_qc_ui(lv_group_t *group,lv_obj_t *base_ui)
{
    struct preview_qc_s *ui_s = (struct preview_qc_s*)STREAM_LIBC_ZALLOC(sizeof(struct preview_qc_s));
    ui_s->last_group = group;
    ui_s->base_ui = base_ui;
    lv_obj_t *btn =  lv_list_add_btn(base_ui, NULL, "preview_qc");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_preview_qc_ui, LV_EVENT_PRESSED, ui_s);
    return btn;
}