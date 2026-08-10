/*************************************************************************************** 
这个主要是预览,通过scale3_msi将dvp/csi数据scale到对应的分辨率作为数据流发送出去
然后lcd_msi需要接收对应的数据去显示
这里只是产生对应数据流,数据用途实际需要终端流(比如lcd流)接收后去处理

S_PREVIEW_SCALE3   ---->   R_VIDEO_P0(320x180)
(scale3)                    (lcd_video_p0)

***************************************************************************************/
#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "stream_define.h"
#include "keyWork.h"
#include "keyScan.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "audio_msi/audio_adc.h"
#include "video_record/video_record.h"
#include "dev.h"
#include "demo/app_common.h"

//data申请空间函数
#define STREAM_MALLOC     av_psram_malloc
#define STREAM_FREE       av_psram_free
#define STREAM_ZALLOC     av_psram_zalloc

//结构体申请空间函数
#define STREAM_LIBC_MALLOC     av_malloc
#define STREAM_LIBC_FREE       av_free
#define STREAM_LIBC_ZALLOC     av_zalloc

#define LV_OBJ_AVI_RECORD_FLAG LV_OBJ_FLAG_USER_1

extern lv_indev_t * indev_keypad;
extern lv_style_t g_style;
struct avi_encode_ui_s
{
    lv_group_t  *last_group;
    lv_obj_t    *base_ui;

    lv_group_t    *now_group;
    lv_obj_t    *now_ui;

    struct msi *s;
    struct msi *avi_encode_s;
    struct msi *jpg_s;

    uint32_t record_time;
    uint8_t audio_en;

};

static uint32_t self_key(uint32_t val)
{
    uint32_t key_ret = 0;
	if(val > 0)
	{
		if((val&0xff) ==   KEY_EVENT_SUP)
		{
			switch(val>>8)
			{
				case AD_UP:
					key_ret = LV_KEY_PREV;
				break;
				case AD_DOWN:
					key_ret = LV_KEY_NEXT;
				break;
				case AD_LEFT:
					key_ret = 'q';
				break;
				case AD_RIGHT:
					key_ret = 'e';
				break;
				case AD_PRESS:
					key_ret = LV_KEY_ENTER;
				break;

				default:
				break;

			}
		}
	}
    return key_ret;
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

static void start_avi_record_ui(lv_event_t * e)
{
    struct msi *ret = NULL;
    int32_t c = *((int32_t *)lv_event_get_param(e));
    struct avi_encode_ui_s *ui_s = (struct avi_encode_ui_s *)lv_event_get_user_data(e);
    if(c == LV_KEY_ENTER)
    {
        if(!lv_obj_has_flag(ui_s->now_ui,LV_OBJ_AVI_RECORD_FLAG))
        {
            lv_obj_add_flag(ui_s->now_ui, LV_OBJ_AVI_RECORD_FLAG);
            
			struct dev_obj *audio_dev = NULL;
            dev_walk(DEV_TYPE_MIC, get_dev_cb, &audio_dev);
			struct video_record_cfg rec_cfg = {
                .name         = R_AVI_ENCODE_MSI,
                .srcID        = 0,
                .filter       = (uint8_t) ~0,
                .rec_time     = 1,
                .audio_en     = audio_dev ? 1 : 0,
                .mode         = 0,
                .video_fps    = 25,
                .file_process = NULL,
            };
            ret = avi_record_msi_init(&rec_cfg);
            if (!ret) {
                lv_obj_clear_flag(ui_s->now_ui, LV_OBJ_AVI_RECORD_FLAG);
            }
            else if(rec_cfg.audio_en)
            {
                auadc_msi_add_output(AUSYS_AUAD, R_AVI_ENCODE_MSI);
                ui_s->audio_en = 1;
            }
            ui_s->avi_encode_s = ret;
        }
        else
        {
            //已经在录像了
            os_printf("%s already running\n",__FUNCTION__);
        }
    }
}

static void exit_avi_record_ui(lv_event_t * e)
{
    int32_t c = *((int32_t *)lv_event_get_param(e));
    if(c == 'q')
    {
        struct avi_encode_ui_s *ui_s = (struct avi_encode_ui_s *)lv_event_get_user_data(e);
        lv_indev_set_group(indev_keypad, ui_s->last_group);
        lv_obj_clear_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
        lv_group_del(ui_s->now_group);
        ui_s->now_group = NULL;
        if(ui_s->audio_en)
        {
            auadc_msi_del_output(AUSYS_AUAD, R_AVI_ENCODE_MSI);
            ui_s->audio_en = 0;
        }
        msi_del_output(ui_s->jpg_s, NULL, NULL, R_AVI_ENCODE_MSI);
        msi_put(ui_s->jpg_s);
        msi_destroy(ui_s->avi_encode_s);
        msi_destroy(ui_s->s);
        //关闭VIDEO P0的使能，释放占住的scale3的fb
        msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);   
        lv_obj_del(ui_s->now_ui);
        set_lvgl_get_key_func(NULL);
    }
}

//进入预览的ui,那么就要创建新的ui去显示预览图了
static void enter_avi_encode_ui(lv_event_t * e)
{
    set_lvgl_get_key_func(self_key);
    struct avi_encode_ui_s *ui_s = (struct avi_encode_ui_s *)lv_event_get_user_data(e);
    lv_obj_add_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *ui = lv_obj_create(lv_scr_act());  
    ui_s->now_ui = ui;
    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));
    //绑定流到Video_P0显示
    
    ui_s->s = scale3_msi(S_PREVIEW_SCALE3);
    if (ui_s->s)
    {
        msi_add_output(ui_s->s, NULL, NULL, R_VIDEO_P0);
        msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 1);
        ui_s->s->enable = 1;
    }

    ui_s->jpg_s = msi_find(AUTO_JPG, 1);
    if (ui_s->jpg_s)
    {
        msi_add_output(ui_s->jpg_s, NULL, NULL, R_AVI_ENCODE_MSI);
    }

    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);

    lv_group_add_obj(group, ui);
    ui_s->now_group = group;

    lv_obj_add_event_cb(ui, start_avi_record_ui, LV_EVENT_KEY, ui_s);
    lv_obj_add_event_cb(ui, exit_avi_record_ui, LV_EVENT_KEY, ui_s);
}

lv_obj_t *avi_loop_record_ui(lv_group_t *group,lv_obj_t *base_ui)
{
    struct avi_encode_ui_s *ui_s = (struct avi_encode_ui_s*)STREAM_LIBC_ZALLOC(sizeof(struct avi_encode_ui_s));
    ui_s->last_group = group;
    ui_s->base_ui = base_ui;
    lv_obj_t *btn =  lv_list_add_btn(base_ui, NULL, "avi_loop_record_ui");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_avi_encode_ui, LV_EVENT_PRESSED, ui_s);
    return btn;
}
