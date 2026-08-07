/*************************************************************************************** 
***************************************************************************************/
#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "stream_frame.h"
#include "keyWork.h"
#include "keyScan.h"
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


extern lv_style_t g_style;
extern lv_indev_t * indev_keypad;

struct photo_ui_s
{
    lv_group_t  *last_group;
    lv_obj_t    *base_ui;
    lv_group_t    *now_group;
    lv_obj_t    *now_ui;
    

    struct msi *photo_s;
    struct fbpool tx_pool;
    struct msi *other_s;
    struct msi *decode_s;
    lv_obj_t * label_path;

};

struct photo_list_param
{
    lv_group_t *group;
    lv_obj_t *ui;
    struct photo_ui_s *ui_s;
};
typedef int (*list_ui)(const char *filename,void *data);




static void exit_show_filelist(lv_event_t * e)
{
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    struct photo_ui_s *ui_s = (struct photo_ui_s *)lv_event_get_user_data(e);
    lv_obj_t *list_item = lv_event_get_current_target(e);
    if(ui_s->now_group)
    {
        lv_indev_set_group(indev_keypad, ui_s->now_group);
    }
    if(list_item->user_data)
    {
        //由于是子控件回调函数需要删除父控件,所以需要用到异步删除,否则删除内部链表会有异常
        lv_obj_del_async(list_item->user_data);
    }
}



//读取图片,发送出去显示
static void play_photo(lv_event_t * e)
{   
    void *fp = NULL;
    void *photo_buf = NULL;
	struct framebuff *data_s = NULL;
    uint32_t len;
    struct photo_ui_s *ui_s = (struct photo_ui_s *)lv_event_get_user_data(e);
    lv_obj_t *list_item = lv_event_get_current_target(e);
    lv_obj_t *list = list_item->user_data;
    char path[64];
    const char *filename = lv_list_get_btn_text(list,list_item);
    os_sprintf(path,"0:%s/%s",CHECK_DIR,filename);
	if(!ui_s->photo_s)
	{
		goto play_photo_fail;
	}

    //读取图片文件,通过流去发送
    data_s = fbpool_get(&ui_s->tx_pool, 0, ui_s->photo_s);
    if(data_s)
    {
        data_s->data = NULL;
        lv_label_set_text(ui_s->label_path, path);
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
                goto play_photo_fail;
            }
            else
            {
                len = osal_fread(photo_buf,1,filesize,fp);
                if(len != filesize)
                {
                    goto play_photo_fail;
                }
                data_s->data = (void*)photo_buf;
                data_s->mtype = F_JPG;
                data_s->stype = FSTYPE_YUV_P0;
                data_s->len = filesize;
                
                msi_output_fb(ui_s->photo_s, data_s);
                photo_buf = NULL;
                data_s = NULL;
                goto play_photo_end;
            }
            
        }

        goto play_photo_fail;

    }
play_photo_fail:
    //显示失败
    os_sprintf(path,"0:%s/%s fail",CHECK_DIR,filename);
    lv_label_set_text(ui_s->label_path, path);
    if(photo_buf)
    {
        STREAM_FREE(photo_buf);
        photo_buf = NULL;
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

    exit_show_filelist(e);
}



static int photo_list_show(const char *filename,void *data)
{
    struct photo_list_param *param = (struct photo_list_param*)data;
	lv_obj_t * btn = lv_list_add_btn(param->ui, NULL, (const char *)filename);
    btn->user_data = (void*)param->ui;
    if(param->group)
    {
        lv_group_add_obj(param->group, btn);
    }
	
    //回调函数,进入回放功能
	lv_obj_add_event_cb(btn, play_photo, LV_EVENT_CLICKED, param->ui_s);
	return 0;
}

//遍历文件夹
//遍历文件夹
static int each_read_file(list_ui fn,void *param)
{
    DIR  dir;
    FILINFO finfo;
    FRESULT fr;
    if(!fn)
    {
        goto  each_read_file_end;
    }

    fr = f_findfirst(&dir, &finfo, CHECK_DIR, EXT_NAME);


	while(fr == FR_OK && finfo.fname[0] != 0)
	{
        fn(finfo.fname,param);
        fr = f_findnext(&dir, &finfo);
	}

    f_closedir(&dir);
each_read_file_end:
    os_printf("%s:%d\tret:%d\n",__FUNCTION__,__LINE__,fr);
    return fr;
}

static void clear_list_group_ui(lv_event_t * e)
{
    lv_group_t *group = (lv_group_t *)lv_event_get_user_data(e);
    os_printf("group:%X\n",group);
    if(group)
    {
        lv_group_del(group);
    }
}



static void show_filelist(lv_event_t * e)
{
    int32_t c = *((int32_t *)lv_event_get_param(e));
    if(c == 'e')
    {
        struct photo_ui_s *ui_s = (struct photo_ui_s *)lv_event_get_user_data(e);
        struct photo_list_param param;
        lv_obj_t *list = lv_list_create(ui_s->now_ui);
        lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
        param.ui_s = ui_s;
        param.ui = list;
        param.group = lv_group_create();
        lv_indev_set_group(indev_keypad, param.group);
        //创建exit的控件
        lv_obj_t *list_item = lv_list_add_btn(list, NULL, "exit");
        list_item->user_data = (void*)list;
        lv_group_add_obj(param.group, list_item);
        lv_obj_add_event_cb(list_item, exit_show_filelist, LV_EVENT_SHORT_CLICKED, ui_s);

        each_read_file(photo_list_show,(void*)&param);

        lv_obj_add_event_cb(list, clear_list_group_ui, LV_EVENT_DELETE, param.group);
    }

}









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


				break;
				default:
				break;

			}
		}
	}
    return key_ret;
}

static int32 photo_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int32_t ret = RET_OK;
    struct photo_ui_s *ui_s = (struct photo_ui_s *)msi->priv;

    switch (cmd_id)
    {
        case MSI_CMD_PRE_DESTROY:
        {

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

static void exit_player_ui(lv_event_t * e)
{
    int32_t c = *((int32_t *)lv_event_get_param(e));
    if(c == 'q')
    {
        struct photo_ui_s *ui_s = (struct photo_ui_s *)lv_event_get_user_data(e);
        lv_indev_set_group(indev_keypad, ui_s->last_group);
        lv_obj_clear_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
        lv_group_del(ui_s->now_group);
        ui_s->now_group = NULL;
        msi_destroy(ui_s->photo_s);
        msi_destroy(ui_s->other_s);
        msi_destroy(ui_s->decode_s);
        msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);

        ui_s->decode_s = NULL;
        ui_s->other_s = NULL;
        ui_s->photo_s = NULL;

        
        lv_obj_del(ui_s->now_ui);

        set_lvgl_get_key_func(NULL);

    }

}



static void enter_player_ui(lv_event_t * e)
{
    set_lvgl_get_key_func(self_key);
    struct photo_ui_s *ui_s = (struct photo_ui_s *)lv_event_get_user_data(e);
    lv_obj_t *base_ui = ui_s->base_ui;
    lv_obj_add_flag(base_ui, LV_OBJ_FLAG_HIDDEN);
    os_printf("%s ui_s:%x ui_s->decode_s:%x\n",__FUNCTION__,ui_s,&ui_s->decode_s);
    
    lv_obj_t *ui = lv_obj_create(lv_scr_act()); 
    ui_s->now_ui = ui; 
    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));


    ui_s->label_path = lv_label_create(ui);
    lv_label_set_text(ui_s->label_path, "");
    lv_obj_align(ui_s->label_path,LV_ALIGN_TOP_MID,0,0);
    os_printf("%s %d\n",__FUNCTION__,__LINE__);

    ui_s->photo_s = photo_stream(S_LVGL_PHOTO);
    if (ui_s->photo_s)
    {
        ui_s->photo_s->priv = (void *)ui_s;
        fbpool_init(&ui_s->tx_pool, 4);
        ui_s->photo_s->action = photo_msi_action;
        msi_add_output(ui_s->photo_s, NULL, SR_OTHER_JPG);
        ui_s->photo_s->enable = 1;
    }
    else
    {
        os_printf("photo ui new msi failed\n");
    }
    os_printf("%s %d\n",__FUNCTION__,__LINE__);
    //SR_OTHER_JPG接收数据,然后发送到decode
    ui_s->other_s = jpg_decode_msg_msi(SR_OTHER_JPG,640,360,640,360,FSTYPE_YUV_P0);
    if (ui_s->other_s)
    {
        msi_add_output(ui_s->other_s, NULL, S_JPG_DECODE);
    }
    os_printf("%s %d\n",__FUNCTION__,__LINE__);
    //decode发送到S_NEWPLAYER控制速度后,转发到P0
    ui_s->decode_s = jpg_decode_msi(S_JPG_DECODE);
    if (ui_s->decode_s)
    {
        msi_add_output(ui_s->decode_s, NULL, R_VIDEO_P0);
        msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 1);
    }
    os_printf("%s %d\n",__FUNCTION__,__LINE__);
    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);
    lv_group_add_obj(group, ui);
    ui_s->now_group = group;

    //弹出菜单栏,用于切换分辨率
    lv_obj_add_event_cb(ui, show_filelist, LV_EVENT_KEY, ui_s);
    lv_obj_add_event_cb(ui, exit_player_ui, LV_EVENT_KEY, ui_s);
}

lv_obj_t *photo_ui(lv_group_t *group,lv_obj_t *base_ui)
{
    struct photo_ui_s *ui_s = (struct photo_ui_s*)STREAM_LIBC_ZALLOC(sizeof(struct photo_ui_s));
    os_printf("%s ui_s:%x ui_s->decode_s:%x\n",__FUNCTION__,ui_s,&ui_s->decode_s);
    ui_s->last_group = group;
    ui_s->base_ui = base_ui;
    
    lv_obj_t *btn =  lv_list_add_btn(base_ui, NULL, "photo");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_player_ui, LV_EVENT_SHORT_CLICKED, ui_s);
    return btn;
}