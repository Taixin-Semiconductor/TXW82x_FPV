/*************************************************************************************** 
S_PREVIEW_SCALE3    ---->   R_VIDEO_P0(320x180)
(scale3)                      (lcd_video_p0)
                    
RS_JPG_CONCAT       ---->   R_LVGL_TAKEPHOTO(1280x720)
(mjpeg)                            
***************************************************************************************/
#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "stream_frame.h"
#include "keyWork.h"
#include "keyScan.h"
#include "fatfs/osal_file.h"
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

#define LV_OBJ_TAKEPHOTO_FLAG LV_OBJ_FLAG_USER_1

extern lv_indev_t * indev_keypad;
extern lv_style_t g_style;

struct preview_encode_takephoto_ui_s
{
    lv_group_t  *last_group;
    lv_obj_t    *base_ui;
    lv_group_t    *now_group;
    lv_obj_t    *now_ui;
    lv_timer_t *timer;

    struct msi *s;
    struct msi *jpg_s;
    struct msi *takephoto_s;
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
					key_ret = 'a';
				break;
				case AD_DOWN:
					key_ret = 's';
				break;
				case AD_LEFT:
					key_ret = 'q';
				break;
				case AD_RIGHT:
					key_ret = 'd';
				break;
				case AD_PRESS:
					key_ret = LV_KEY_ENTER;
				break;


				break;
				default:
				break;

			}
		}
		else if((val&0xff) ==   KEY_EVENT_LUP)
		{
			

				switch(val>>8)
				{
					case AD_PRESS:
						key_ret = 'e';
					break;

					default:
					break;

				}
		}
	}
    return key_ret;
}


//生成一个流,专门接收图片,用于拍照的,拍照之前可以将缓冲区的删除,这样可以保证得到最新的图片
//要考虑一些问题,如果拍照放在workqueue,要考虑写卡时间是否影响workqueue
//这里尝试使用lvgl的timer事件去保存(影响lvgl,如果需要更加好,可能要创建任务去保存)

static int32 takephoto_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int32_t ret = RET_OK;
    //struct preview_encode_takephoto_ui_s *ui_s = (struct preview_encode_takephoto_ui_s *)msi->priv;

    switch (cmd_id)
    {
        case MSI_CMD_PRE_DESTROY:
        {

        }
            break;

        case MSI_CMD_POST_DESTROY:
        {

        }
            break;
        
        case MSI_CMD_FREE_FB:
        {

        }
            break;

        default:
            break;
    }

    return ret;
}


static struct msi *takephoto_stream(const char *name)
{
    //设置接收1,是为了不要接收太多的图片,因为是拍照,需要保持实时性
    //如果接收过多,如果不处理,会导致源头没有空间产生新的图片
    //如果要结合录像,则需要在应用层去处理(实际录像时可以创建另一个独立的流)
    struct msi *msi = msi_new(name, 1, NULL);
    return msi;
}

static void takephoto_timer(lv_timer_t *t)
{
    struct preview_encode_takephoto_ui_s *ui_s = (struct preview_encode_takephoto_ui_s *)t->user_data;
    struct framebuff *fb = NULL;
    //看看对应的流有没有图片,如果有,则保存
    fb = msi_get_fb(ui_s->takephoto_s, 0);
    if(fb)
    {
        ui_s->takephoto_s->enable = 0;
        char filename[64];
        sprintf(filename,"0:/photo/%08d.jpg",(uint32_t)os_jiffies());
        void *fp = osal_fopen(filename,"wb");
        if(fp)
        {
            osal_fwrite(fb->data,1,fb->len,fp);
            osal_fclose(fp);
        }
        else
        {
            os_printf("takephoto fail,filename:%s\n",filename);
        }
        //保存图片
        msi_delete_fb(NULL, fb);

        while(fb)
        {
            fb = msi_get_fb(ui_s->takephoto_s, 0);
            if(fb)
            {
                msi_delete_fb(NULL, fb);
            }
        }
        os_printf("takephoto success\n");
        lv_obj_clear_flag(ui_s->now_ui, LV_OBJ_TAKEPHOTO_FLAG); 
        ui_s->timer = NULL;
        lv_timer_del(t);
    }
}

//使能拍照
static void encode_takephoto_ui(lv_event_t * e)
{
    struct preview_encode_takephoto_ui_s *ui_s = (struct preview_encode_takephoto_ui_s *)lv_event_get_user_data(e);
    struct msi *s = ui_s->takephoto_s;
    int32_t c = *((int32_t *)lv_event_get_param(e));
    if(c == LV_KEY_ENTER)
    {
        if(!lv_obj_has_flag(ui_s->now_ui,LV_OBJ_TAKEPHOTO_FLAG))
        {
            lv_obj_add_flag(ui_s->now_ui, LV_OBJ_TAKEPHOTO_FLAG); 
            s->enable = 1;
            //启动timer,10ms检查一次
            ui_s->timer = lv_timer_create(takephoto_timer, 10,  (void*)ui_s);
        }
        else
        {
            //已经在拍照了
            os_printf("%s already running\n",__FUNCTION__);
        }
    }

}

static void exit_preview_encode_takephoto_ui(lv_event_t * e)
{
	struct preview_encode_takephoto_ui_s *ui_s = (struct preview_encode_takephoto_ui_s *)lv_event_get_user_data(e);
    int32_t c = *((int32_t *)lv_event_get_param(e));
    if(c == 'q')
    {
        if(ui_s->timer)
        {
            lv_timer_del(ui_s->timer);
        }
        struct preview_encode_takephoto_ui_s *ui_s = (struct preview_encode_takephoto_ui_s *)lv_event_get_user_data(e);
        lv_indev_set_group(indev_keypad, ui_s->last_group);
        lv_obj_clear_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
        lv_group_del(ui_s->now_group);
        ui_s->now_group = NULL;
        msi_destroy(ui_s->s);
        msi_del_output(ui_s->jpg_s, NULL, R_LVGL_TAKEPHOTO);
        msi_put(ui_s->jpg_s);
        msi_destroy(ui_s->takephoto_s);
        msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);   
        lv_obj_del(ui_s->now_ui);

        set_lvgl_get_key_func(NULL);
    }

}
//进入预览的ui,那么就要创建新的ui去显示预览图了
static void enter_preview_encode_takephoto_ui(lv_event_t * e)
{
    set_lvgl_get_key_func(self_key);
    struct preview_encode_takephoto_ui_s *ui_s = (struct preview_encode_takephoto_ui_s *)lv_event_get_user_data(e);
    lv_obj_add_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *ui = lv_obj_create(lv_scr_act());  
    ui_s->now_ui = ui;
    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));
    ui_s->s = scale3_msi(S_PREVIEW_SCALE3);
    //绑定流到Video P0显示
    if (ui_s->s)
    {
        msi_add_output(ui_s->s, NULL, R_VIDEO_P0);
        msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 1);
        ui_s->s->enable = 1;
    }

    //创建jpg流,设置从vpp_buf0取数据
    ui_s->jpg_s = msi_find(AUTO_JPG, 1);
    if(ui_s->jpg_s)
    {
        msi_add_output(ui_s->jpg_s, NULL, R_LVGL_TAKEPHOTO);
        ui_s->jpg_s->enable = 1;
    }

    ui_s->takephoto_s = takephoto_stream(R_LVGL_TAKEPHOTO);
    if(ui_s->takephoto_s)
    {
        ui_s->takephoto_s->action = takephoto_msi_action;
        ui_s->takephoto_s->priv = ui_s;
    }

    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);

    lv_group_add_obj(group, ui);
    ui_s->now_group = group;
    lv_obj_add_event_cb(ui, exit_preview_encode_takephoto_ui, LV_EVENT_KEY, ui_s);

    //拍照的按键事件
    lv_obj_add_event_cb(ui, encode_takephoto_ui, LV_EVENT_KEY, ui_s);
}

lv_obj_t *preview_encode_takephoto_ui(lv_group_t *group,lv_obj_t *base_ui)
{
    struct preview_encode_takephoto_ui_s *ui_s = (struct preview_encode_takephoto_ui_s*)STREAM_LIBC_ZALLOC(sizeof(struct preview_encode_takephoto_ui_s));
    ui_s->last_group = group;
    ui_s->base_ui = base_ui;
    lv_obj_t *btn =  lv_list_add_btn(base_ui, NULL, "preview_encode_takephoto");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_preview_encode_takephoto_ui, LV_EVENT_SHORT_CLICKED, ui_s);
    return btn;
}