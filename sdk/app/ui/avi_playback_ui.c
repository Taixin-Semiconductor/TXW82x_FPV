/*************************************************************************************** 
***************************************************************************************/
#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "keyWork.h"
#include "keyScan.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "avi_player_msi.h"

#define CHECK_DIR   "AVI"
#define EXT_NAME    "*avi"
#define PLAY_AVI_STREAM_NAME     (64)
#define ROUTE_PLAYER_MSI_TX_NUM  (16)

enum
{
    PLAY_STATUS_STOP_PLAY = BIT(0),
    PLAY_STATUS_PLAY_1FPS = BIT(1),  //播放一帧,然后自动暂停
    PLAY_STATUS_PLAY_END =  BIT(2),  //播放完毕,需要重新播放   
};

//转发流的播放流结构体
struct player_forward_stream_s
{
	struct os_work work;
    struct msi *s;
    struct framebuff *data_s;

    uint32_t magic;
    uint32_t start_time;
    uint32_t last_fps_play_time;       //上一帧播放的时间(非系统时间,是视频帧的时间)
    uint8_t play_status;    //自己的播放状态,第一bit代表播放或者暂停
};

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

struct avi_player_ui_s
{
    lv_group_t  *last_group;
    lv_obj_t    *base_ui;

    lv_group_t    *now_group;
    lv_obj_t    *now_ui;

    lv_timer_t *timer;
    lv_obj_t * label_time;

    struct msi *P0_jpg_msi;
    struct msi *decode_msi;
    struct msi *player_msi;
    struct msi *avi_s;

    uint8_t *play_name;
};

struct avi_list_param
{
    lv_group_t *group;
    lv_obj_t *ui;
    struct avi_player_ui_s *ui_s;
};

typedef int (*creat_avi_list_ui)(const char *filename,void *data);

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
                    key_ret = 'q';
				break;
				case AD_DOWN:
                    key_ret = 'e';
				break;
				case AD_LEFT:
					key_ret = 'a';
				break;
				case AD_RIGHT:
					key_ret = 'd';
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

static void exit_show_filelist(lv_event_t * e)
{
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    set_lvgl_get_key_func(self_key);
    struct avi_player_ui_s *ui_s = (struct avi_player_ui_s *)lv_event_get_user_data(e);
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

//进入回放
static void enter_playback(lv_event_t * e)
{
    set_lvgl_get_key_func(self_key);
    struct avi_player_ui_s *ui_s = (struct avi_player_ui_s *)lv_event_get_user_data(e);
    lv_obj_t *list_item = lv_event_get_current_target(e);
    lv_obj_t *list = list_item->user_data;
    //将当前的流关闭,内部判断是否=NULL,这里就不判断了

    if(ui_s->play_name)
    {
        STREAM_FREE(ui_s->play_name);
        ui_s->play_name = NULL;
    }

    os_printf("ui_s->label_time:%X\n",ui_s->label_time);
    lv_label_set_text(ui_s->label_time, "00:00");

    //重新打开一个视频文件
    char path[64];
    const char *filename = lv_list_get_btn_text(list,list_item);
    os_printf("play filename:%s\n",filename);
    os_sprintf(path,"0:%s/%s",CHECK_DIR,filename);

    ui_s->play_name = (uint8_t*)STREAM_MALLOC(PLAY_AVI_STREAM_NAME);
    os_sprintf((char*)ui_s->play_name,"%s_%04d",filename,(uint32_t)os_jiffies());
    os_printf("struct msi play_name:%s\n",ui_s->play_name);
    ui_s->avi_s = avi_player_init(S_AVI_PLAYER, (const char *)path);
    if(ui_s->avi_s)
    {
        //设置播放一帧,先去停止播放器
        struct player_forward_stream_s *player_msg = ui_s->player_msi->priv;
        os_work_cancle2(&player_msg->work, 1);

        msi_add_output(ui_s->avi_s, NULL, S_NEWPLAYER);
        ui_s->avi_s->enable = 1;

        //重新开始播放,修改启动时间
        player_msg->last_fps_play_time = 0;
        player_msg->start_time = os_jiffies();
        player_msg->magic = os_jiffies() & 0xFF;
        msi_do_cmd(ui_s->avi_s, MSI_CMD_PLAYER, MSI_PLAYER_MAGIC, player_msg->magic);
        player_msg->play_status = (PLAY_STATUS_STOP_PLAY|PLAY_STATUS_PLAY_1FPS);

        OS_WORK_REINIT(&player_msg->work);
        os_run_work_delay(&player_msg->work, 1);

        //文件开始解析
    }   
    exit_show_filelist(e);
}



static int avi_list_show(const char *filename,void *data)
{
    struct avi_list_param *param = (struct avi_list_param*)data;
	lv_obj_t * btn = lv_list_add_btn(param->ui, NULL, (const char *)filename);
    btn->user_data = (void*)param->ui;
    if(param->group)
    {
        lv_group_add_obj(param->group, btn);
    }
	
    //回调函数,进入回放功能
	lv_obj_add_event_cb(btn, enter_playback, LV_EVENT_CLICKED, param->ui_s);
	return 0;
}

//遍历文件夹
static int each_read_file(creat_avi_list_ui fn,void *param)
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
        set_lvgl_get_key_func(NULL);
        struct avi_player_ui_s *ui_s = (struct avi_player_ui_s *)lv_event_get_user_data(e);
        struct avi_list_param param;
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

        each_read_file(avi_list_show,(void*)&param);

        lv_obj_add_event_cb(list, clear_list_group_ui, LV_EVENT_DELETE, param.group);
    }

}

//这里要创建一个流,专门是控制avi播放流的,由jpg解码,发送到这里缓存起来,然后控制播放速度、暂停等
//这里使用lvgl的timer来代替workqueue
static int32_t player_work(struct os_work *work)
{
    struct player_forward_stream_s *player_msg  = (struct player_forward_stream_s *)work;
    struct framebuff *data_s = NULL;
    uint32_t time;
	//static int count = 0;
    if(!player_msg->s->enable)
    {
        goto player_work_get_data;
    }
    
    if(!(player_msg->play_status & PLAY_STATUS_STOP_PLAY))
    {
        goto player_work_get_data;
    }


    if(player_msg->data_s)
    {

        time = player_msg->data_s->time;
        //先检查时间是否合适
        if(player_msg->start_time + time > os_jiffies())
        {
            if(player_msg->magic != player_msg->data_s->datatag)
            {
                msi_delete_fb(NULL, player_msg->data_s);
                player_msg->data_s = NULL;
            }
            goto player_work_end;
        }
        //再次检查是否符合需要播放(有可能是缓存或者快进导致进入这里)
        if(player_msg->magic != player_msg->data_s->datatag)
        {
            msi_delete_fb(NULL, player_msg->data_s);
            player_msg->data_s = NULL;
            _os_printf("=");
            goto player_work_end;
        }

        if(player_msg->play_status & PLAY_STATUS_PLAY_1FPS)
        {
            player_msg->play_status &= ~(PLAY_STATUS_STOP_PLAY|PLAY_STATUS_PLAY_1FPS);
        }
        
        //os_printf("time:%d\tstart_time:%d\tmagic:%d\n",time,player_msg->start_time,player_msg->data_s->datatag);
        player_msg->last_fps_play_time = time;
        //判断一下时间是否需要播放
        data_s = fb_clone(player_msg->data_s, (player_msg->data_s->mtype << 8 | player_msg->data_s->stype), player_msg->s);
        //准备填好显示、解码参数信息,然后发送到解码的模块去解码,主要如果没有成功要如何处理,这里没有考虑
        if(data_s)
        {
            
            //os_printf("time:%d\ttype:%X\n",time,player_msg->data_s->mtype);

            //如果不是音频,就是视频,就显示
            if(data_s->mtype != F_AUDIO)
            {
                msi_output_fb(player_msg->s, data_s);
            }
            else
            {
                //如果是音频,看看是否需要暂停的,如果是,则不播放,因为可能是快进快退,这个时候是不需要去播音频
                if(!(player_msg->play_status & PLAY_STATUS_STOP_PLAY))
                {
                    msi_delete_fb(player_msg->s, data_s);
                }
                else
                {
                    msi_output_fb(player_msg->s, data_s);
                }
            }

            msi_delete_fb(player_msg->s, player_msg->data_s);
            player_msg->data_s = NULL;
        }

    }

player_work_get_data:
    if(!player_msg->data_s)
    {
        player_msg->data_s = msi_get_fb(player_msg->s, 0);
    }
    else
    {
        if(player_msg->magic != player_msg->data_s->datatag)
        {
            msi_delete_fb(NULL, player_msg->data_s);
            player_msg->data_s = NULL;
            goto player_work_end;
        }
    }
    
player_work_end:
    os_run_work_delay(work, 1);
	return 0;
}


static int32_t route_player_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    struct player_forward_stream_s *msg = (struct player_forward_stream_s *)msi->priv;
    switch (cmd_id)
    {
        // 帮忙转发,但是永远不需要放在队列中
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *data = (struct framebuff *)param1;
            //过滤一下没有用的数据包,只是接收play_time_start之后的数据包
            //os_printf("msg->magic:%d data->datatag:%d\n",msg->magic,data->datatag);
            if(!(msg->magic == data->datatag))
            {
                ret = RET_OK + 1;
            }
        }
        break;

        case MSI_CMD_PRE_DESTROY:
        {
            if (msg) {

                os_work_cancle2(&msg->work, 1);

                if(msg->data_s)
                {
                    os_printf("msg->data_s:%X\n",msg->data_s);
                    msi_delete_fb(NULL, msg->data_s);
                    msg->data_s = NULL;
                }

            }
        }
            break;

        case MSI_CMD_POST_DESTROY:
        {
            if(msg)
            {
                STREAM_FREE(msg);

                msi->priv = NULL;
            }
        }
            break;

        case MSI_CMD_FREE_FB:
        {
            //struct framebuff *fb = (struct framebuff *)param1;
            //os_printf("ui free fb:0x%x\n",fb);
        }
            break;

        default:
            break;
    }

    return ret;
}

//创建播放器的流,这个流主要实现的是控制速度转发作用,暂时控制的是视频
//如果要控制音频,需要计算时间来控制速度
static struct msi *route_player_msi(const char *name)
{
    struct msi *msi = msi_new(name, 16, NULL);

    if (!msi) {
        return NULL;
    }

    struct player_forward_stream_s *msg = (struct player_forward_stream_s*)msi->priv;

    if (!msg) {
        msg = (struct player_forward_stream_s *)STREAM_ZALLOC(sizeof(struct player_forward_stream_s));
        msg->s = msi;
        msi->priv = msg;
        msg->start_time = os_jiffies();
        msg->data_s = NULL;
        msg->magic = 0;
        msg->play_status = (PLAY_STATUS_STOP_PLAY|PLAY_STATUS_PLAY_1FPS);
        msg->last_fps_play_time = 0;

        //fbpool_init(&msg->tx_pool, ROUTE_PLAYER_MSI_TX_NUM);
        msi->action = route_player_action;
        msi->enable = 1;

        OS_WORK_INIT(&msg->work, player_work, 0);
        os_run_work_delay(&msg->work, 1);
    }
    else
    {
        msi_destroy(msi);
        msi = NULL;
    }

    return msi;
}






static void player_locate_ctrl_ui(lv_event_t * e)
{
    struct avi_player_ui_s *ui_s = (struct avi_player_ui_s *)lv_event_get_user_data(e);
    struct player_forward_stream_s *player_msg = ui_s->player_msi->priv;
    int32_t c = *((int32_t *)lv_event_get_param(e));
    //printf("c:%d\n",c);
    switch(c)
    {
        //后退
        case 'a':
        {
            os_work_cancle2(&player_msg->work, 1);
            uint32_t now_play_time = msi_do_cmd(ui_s->avi_s, MSI_CMD_PLAYER, MSI_REWIND_INDEX, player_msg->last_fps_play_time);
            //重新开始播放,修改启动时间
            player_msg->last_fps_play_time = now_play_time;
            player_msg->start_time = os_jiffies() - now_play_time;
            player_msg->magic = os_jiffies() & 0xFF;
            msi_do_cmd(ui_s->avi_s, MSI_CMD_PLAYER, MSI_PLAYER_MAGIC, player_msg->magic);
            player_msg->play_status = (PLAY_STATUS_STOP_PLAY|PLAY_STATUS_PLAY_1FPS);

            OS_WORK_REINIT(&player_msg->work);
            os_run_work_delay(&player_msg->work, 1);
        }

        break;
        //前进
        case 'd':
        {
            os_work_cancle2(&player_msg->work, 1);
            uint32_t now_play_time = msi_do_cmd(ui_s->avi_s, MSI_CMD_PLAYER, MSI_FORWARD_INDEX, player_msg->last_fps_play_time);
            //重新开始播放,修改启动时间
            player_msg->last_fps_play_time = now_play_time;
            player_msg->start_time = os_jiffies() - now_play_time;
            player_msg->magic = os_jiffies() & 0xFF;
            msi_do_cmd(ui_s->avi_s, MSI_CMD_PLAYER, MSI_PLAYER_MAGIC, player_msg->magic);
            player_msg->play_status = (PLAY_STATUS_STOP_PLAY|PLAY_STATUS_PLAY_1FPS);
            OS_WORK_REINIT(&player_msg->work);
            os_run_work_delay(&player_msg->work, 1);
        }

        break;
        
        default:
        break;
    }
}

static void player_ctrl_ui(lv_event_t * e)
{
    struct avi_player_ui_s *ui_s = (struct avi_player_ui_s *)lv_event_get_user_data(e);
    struct player_forward_stream_s *player_msg = ui_s->player_msi->priv;
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    //根据播放状态设置
    if(player_msg->play_status&PLAY_STATUS_STOP_PLAY)
    {
        player_msg->play_status &= (~PLAY_STATUS_STOP_PLAY);
    }
    else
    {
        //重新开始播放,修改启动时间
        player_msg->start_time = os_jiffies() - player_msg->last_fps_play_time;
        player_msg->play_status |= PLAY_STATUS_STOP_PLAY;
        os_printf("################## stop time:%d\tlast_time:%d\n",player_msg->start_time,player_msg->last_fps_play_time);
    }
}
static void exit_player_ui(lv_event_t * e)
{
    struct avi_player_ui_s *ui_s = (struct avi_player_ui_s *)lv_event_get_user_data(e);
    int32_t c = *((int32_t *)lv_event_get_param(e));
    if(c == 'q')
    {
        lv_timer_del(ui_s->timer);
        struct avi_player_ui_s *ui_s = (struct avi_player_ui_s *)lv_event_get_user_data(e);
        lv_indev_set_group(indev_keypad, ui_s->last_group);
        lv_obj_clear_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
        lv_group_del(ui_s->now_group);
        ui_s->now_group = NULL;
        
        msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);
        msi_destroy(ui_s->P0_jpg_msi);
        msi_destroy(ui_s->decode_msi);
        msi_destroy(ui_s->player_msi);
        msi_destroy(ui_s->avi_s);

        lv_obj_del(ui_s->now_ui);
        if(ui_s->play_name)
        {
            STREAM_FREE(ui_s->play_name);
            ui_s->play_name = NULL;
        }
        set_lvgl_get_key_func(NULL);
    }
}

static void player_ui_timer(lv_timer_t *t)
{
    struct avi_player_ui_s *ui_s = (struct avi_player_ui_s *)t->user_data;
    if(ui_s->avi_s)
    {
        //获取当前播放器的流结构体
        struct player_forward_stream_s *player_msg = ui_s->player_msi->priv;
        //os_printf("play time:%d\tstart_time:%d\n",player_msg->last_fps_play_time,player_msg->start_time);
        char show_time[64];
        sprintf(show_time,"%02d:%02d",(player_msg->last_fps_play_time/1000)/60,(player_msg->last_fps_play_time/1000)%60);
        lv_label_set_text(ui_s->label_time, show_time);

        //判断文件已经播放完毕,如果需要重新播放,则重新建立播放文件
        int status = msi_do_cmd(ui_s->avi_s, MSI_CMD_PLAYER, MSI_PLAYER_STATUS, 0);
        if(status)
        {
            os_printf("status:%d\n",status);
        }
    }

}



static void enter_player_ui(lv_event_t * e)
{
    set_lvgl_get_key_func(self_key);
    struct avi_player_ui_s *ui_s = (struct avi_player_ui_s *)lv_event_get_user_data(e);
    lv_obj_t *base_ui = ui_s->base_ui;
    lv_obj_add_flag(base_ui, LV_OBJ_FLAG_HIDDEN);

    
    lv_obj_t *ui = lv_obj_create(lv_scr_act()); 
    ui_s->now_ui = ui; 
    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));

    //播放转发发送到P0,控制速度
    ui_s->player_msi = route_player_msi(S_NEWPLAYER);
    if (ui_s->player_msi) {
        msi_add_output(ui_s->player_msi, NULL, "R_AUDAC");
        msi_add_output(ui_s->player_msi, NULL, SR_OTHER_JPG);
        ui_s->player_msi->enable = 1;
    }

    //SR_OTHER_JPG接收数据,然后发送到decode
    ui_s->P0_jpg_msi = jpg_decode_msg_msi(SR_OTHER_JPG,640,360,640,360,FSTYPE_YUV_P0);
    if (ui_s->P0_jpg_msi) {
        msi_add_output(ui_s->P0_jpg_msi, NULL, S_JPG_DECODE);
        ui_s->P0_jpg_msi->enable = 1;
    }

    //decode发送到S_NEWPLAYER控制速度后,转发到P0
    ui_s->decode_msi = jpg_decode_msi(S_JPG_DECODE);
    if (ui_s->decode_msi) {
        msi_add_output(ui_s->decode_msi, NULL, R_VIDEO_P0);
        msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 1);
        ui_s->decode_msi->enable = 1;
    }

    //创建lvgl的timer,用于获取播放的时间

    ui_s->label_time = lv_label_create(ui);
    lv_label_set_text(ui_s->label_time, "00:00");
    lv_obj_align(ui_s->label_time,LV_ALIGN_TOP_MID,0,0);
    ui_s->timer = lv_timer_create(player_ui_timer, 100,  (void*)ui_s);
    

    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);
    lv_group_add_obj(group, ui);
    ui_s->now_group = group;
    lv_obj_add_event_cb(ui, exit_player_ui, LV_EVENT_KEY, ui_s); 
    //控制暂停、播放
    lv_obj_add_event_cb(ui, player_ctrl_ui, LV_EVENT_SHORT_CLICKED, ui_s); 

    //快进快退,一次尝试快进5秒,一次尝试快退5秒
    lv_obj_add_event_cb(ui, player_locate_ctrl_ui, LV_EVENT_KEY, ui_s); 

    //弹出菜单栏,用于切换分辨率
    lv_obj_add_event_cb(ui, show_filelist, LV_EVENT_KEY, ui_s);
}

lv_obj_t *avi_playback_ui(lv_group_t *group,lv_obj_t *base_ui)
{
    struct avi_player_ui_s *ui_s = (struct avi_player_ui_s*)STREAM_LIBC_ZALLOC(sizeof(struct avi_player_ui_s));
    ui_s->last_group = group;
    ui_s->base_ui = base_ui;
    ui_s->play_name = NULL;
    
    lv_obj_t *btn =  lv_list_add_btn(base_ui, NULL, "avi_playback_ui");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_player_ui, LV_EVENT_SHORT_CLICKED, ui_s);
    return btn;
}