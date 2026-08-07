/**********************************************************************************************************
 * 虚拟屏幕,支持更多的屏,顶层序号大,底层序号低
 * ********************************************************************************************************/
#include "basic_include.h"

#include "lib/multimedia/msi.h"
#include "osal/work.h"
#include "stream_define.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "app_lcd/app_lcd.h"

extern void yuv_blk_cpy(uint8 *des, uint8* src,uint32 des_w,uint32 des_h, uint32 src_w, uint32_t src_h,uint32 x,uint32 y);
extern void yuv_blk_reduce(uint8 *des, uint8* src,uint32 des_w,uint32 des_h, uint32 src_w, uint32_t src_h,uint32 x,uint32 y);

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE av_free
#define STREAM_LIBC_ZALLOC av_zalloc

#define MAX_RX 8
#define MAX_TX 2

struct window_msg{
	uint16_t w;
	uint16_t h;
	uint16_t x;
	uint16_t y;
};

struct sim_video_more_msi_s
{
    struct os_work work;
    struct msi *msi;
    struct fbpool tx_pool;
    uint16_t w, h;
    uint16_t *filter;          // 数组去进行过滤,0为结束
    struct framebuff **screen; // 不同屏幕的framebuff,屏幕数量与filter数量一致
    uint16_t screen_num;
	uint32_t bufadr[MAX_TX];
    struct framebuff *last_fb;
    struct framebuff *last_input_fb;
    uint32_t subwindow_cnt[10]; 	   //本地buf所记录的cnt
    uint32_t dispwindow_cnt[10];	   //显示的buf所记录的cnt
    struct window_msg dsp_window_msg[10];
    uint8_t lcd_fb_updata;
};

//只做双层
static int32 sim_video_more_work(struct os_work *work)
{
    struct sim_video_more_msi_s *sim_video = (struct sim_video_more_msi_s *)work;
    struct framebuff *fb;
    struct framebuff *output_fb;
	uint32_t dispbuf;
    fb = msi_get_fb(sim_video->msi, 0);
    // 接收到一个fb,就需要判断类型(p0,p1),然后申请空间去进行叠层
    if (fb)
    {
        // 查看类型是否匹配(正常到这里一定匹配,因为接收的时候就进行过滤了)
        uint16_t match_count = 0;
        uint8_t match_flag = 0;
		
        while (match_count < sim_video->screen_num)
        {
            if (fb->stype == sim_video->filter[match_count])				   //类型检查，看一下当前的fb类型是否存在于过滤网内
            {
                if (sim_video->screen[match_count])                            //检查当前的screen是否存在，如果存在，则删除，更新新的fb到screen
                {
                    msi_delete_fb(NULL, sim_video->screen[match_count]);
                }
                sim_video->screen[match_count] = fb;
                match_flag = 1;
                break;
            }
            match_count++;
        }

        // 没有匹配,删除,并且不需要刷新
        if (!match_flag)
        {
            msi_delete_fb(NULL, fb);
            fb = NULL;
            goto sim_video_work_end;
        }

        output_fb = fbpool_get(&sim_video->tx_pool, 0, sim_video->msi);        //从sim_video的发送池里找到对应的fb
        if (!output_fb)
        {
            goto sim_video_work_end;
        }
		
		if((uint32_t)output_fb->data == sim_video->bufadr[0]){
			dispbuf = sim_video->bufadr[1];								//提取当前正在显示的buf地址,用来拷数据
		}else{
			dispbuf = sim_video->bufadr[0];
		}
		
        // 这个模式是默认都是叠层,不再判断有某些情况不需要叠层的情况(效率低,但是既然应用设定多个屏幕,就应该去实现吧)
        // 叠层
        {
            // 开始尝试叠层
            uint16_t tmp = 0;
            uint16_t p_w, p_h;
            struct yuv_arg_s *yuv_msg = NULL;
            while (tmp < sim_video->screen_num)
            {
                if (sim_video->screen[tmp])
                {
                    yuv_msg = (struct yuv_arg_s *)sim_video->screen[tmp]->priv;
                    p_w = yuv_msg->out_w;
                    p_h = yuv_msg->out_h;


					yuv_blk_cpy((uint8_t*)output_fb->data, sim_video->screen[tmp]->data, sim_video->w, sim_video->h, p_w, p_h, yuv_msg->x, yuv_msg->y);
                }
                tmp++;
            }
            output_fb->mtype = F_YUV;
            output_fb->stype = FSTYPE_YUV_P0;
            msi_output_fb(sim_video->msi, output_fb);
        }
    }
sim_video_work_end:
    os_run_work_delay(work, 1);
    return 0;
}


static int32 sim_video_more_work_more(struct os_work *work)
{
	#define REFRESH_LCD_INTERNAL_TIME   10    
    struct sim_video_more_msi_s *sim_video = (struct sim_video_more_msi_s *)work;
    struct framebuff *fb;
    struct framebuff *output_fb;

	
	uint32_t dispbuf;
	uint32_t *cache_buf[10];
	uint8_t push_lcd_updata=0;
	uint16_t p_w, p_h;
	uint16_t max_w;
	uint16_t tmp = 0;
	struct yuv_arg_s *yuv_msg = NULL;
	struct app_lcd_s *lcd_s = &lcd_msg_s;
	
	output_fb = NULL;

	if(sim_video->last_input_fb != NULL){
		fb = sim_video->last_input_fb;
	}else{
		fb = msi_get_fb(sim_video->msi, 0);
	}
    // 接收到一个fb,就需要判断类型(p0,p1),然后申请空间去进行叠层
    if (fb)
    {
        // 查看类型是否匹配(正常到这里一定匹配,因为接收的时候就进行过滤了)
        uint16_t match_count = 0;
        uint8_t match_flag = 0;
        while (match_count < sim_video->screen_num)
        {
            if (fb->stype == sim_video->filter[match_count])				   //类型检查，看一下当前的fb类型是否存在于过滤网内
            {
                yuv_msg = (struct yuv_arg_s *)fb->priv;
				if(sim_video->subwindow_cnt[match_count] != sim_video->dispwindow_cnt[match_count]){   //重复更新，当前要刷的叠层已在备刷区，但又来了新的数据，那要把备刷区推出去
					push_lcd_updata = 1;  					
					sim_video->last_input_fb = fb;
					output_fb = sim_video->last_fb; 
					goto sim_video_push_lcd;
				}
                match_flag = 1;
                break;
            }
            match_count++;
        }

        // 没有匹配,删除,并且不需要刷新
        if (!match_flag)                                                        
        {
            msi_delete_fb(NULL, fb);
            if(fb == sim_video->last_input_fb) {
                sim_video->last_input_fb = NULL;
            }
            fb = NULL;
            goto sim_video_work_end;
        }
		
		if(sim_video->lcd_fb_updata){								
			sim_video->lcd_fb_updata = 0;
			sim_video->last_input_fb = fb;
			
			output_fb = fbpool_get(&sim_video->tx_pool, 0, sim_video->msi); 	   //从sim_video的发送池里找到对应的fb
			if(!output_fb){														   //需要更新时无法申请到fb,则下次重新申请，直到能申请为止
				sim_video->lcd_fb_updata = 1;
				goto sim_video_work_end;
			}else{																   //申请到了，表示当前input_fb可被处理掉，指针清空一下
				if((uint32_t)output_fb->data == sim_video->bufadr[0]){
					dispbuf = sim_video->bufadr[1];								//提取当前正在显示的buf地址,用来拷数据
				}else{
					dispbuf = sim_video->bufadr[0];
				}
				//hw_memcpy(output_fb->data,dispbuf,output_fb->len);
				yuv_blk_cpy((uint8_t*)output_fb->data, (uint8_t*)dispbuf, sim_video->w, sim_video->h, sim_video->w,sim_video->h, 0,0);
				sim_video->last_input_fb = NULL;
			}
		}
		
        if (!output_fb)
        {
            output_fb = sim_video->last_fb; 
        }else{
			sim_video->last_fb = output_fb;            //记录当前要修改的fb
		}


		sim_video->subwindow_cnt[match_count] = yuv_msg->dispcnt;
		sim_video->dsp_window_msg[match_count].w = yuv_msg->out_w;
		sim_video->dsp_window_msg[match_count].h = yuv_msg->out_h;
		sim_video->dsp_window_msg[match_count].x = yuv_msg->x;
		sim_video->dsp_window_msg[match_count].y = yuv_msg->y;


		if((uint32_t)output_fb->data == sim_video->bufadr[0]){
			dispbuf = sim_video->bufadr[1];								//提取当前正在显示的buf地址,用来拷数据
		}else{
			dispbuf = sim_video->bufadr[0];
		}	


		max_w = 0;
		tmp = 0;
		while (tmp < sim_video->screen_num) {						    //选出最大的图片
			if(sim_video->dsp_window_msg[tmp].w > max_w){
				max_w = sim_video->dsp_window_msg[tmp].w;
			}
			tmp++;
		}		

        yuv_msg = (struct yuv_arg_s *)fb->priv;
        p_w = yuv_msg->out_w;
        p_h = yuv_msg->out_h;	

		if(max_w != yuv_msg->out_w){           																					//如果并非大图
			yuv_blk_cpy((uint8_t*)output_fb->data, fb->data, sim_video->w, sim_video->h, p_w, p_h, yuv_msg->x, yuv_msg->y); 	//拷到对应的位置上
			
		}else{																													//如果是大图
			tmp = 0;
			while(tmp < sim_video->screen_num){
				cache_buf[tmp] = NULL;
				if( (match_count != tmp) && (sim_video->dsp_window_msg[tmp].w != 0)){
					if(sim_video->subwindow_cnt[tmp] > sim_video->dispwindow_cnt[tmp]){																//如果备用区数值是更新的，那就拷出来
						cache_buf[tmp] = (uint32_t *)STREAM_MALLOC(sim_video->dsp_window_msg[tmp].w * sim_video->dsp_window_msg[tmp].h * 3 / 2);					//申请小图空间
						if(cache_buf[tmp] != NULL){
							yuv_blk_reduce((uint8_t*)cache_buf[tmp],(uint8_t*)output_fb->data,sim_video->dsp_window_msg[tmp].w,sim_video->dsp_window_msg[tmp].h,sim_video->w,sim_video->h,sim_video->dsp_window_msg[tmp].x,sim_video->dsp_window_msg[tmp].y);
						}else{
							_os_printf("malloc psram for sim video error1\r\n");
						}
					}
				}
				tmp++;
			}
			
			yuv_blk_cpy((uint8_t*)output_fb->data, fb->data, sim_video->w, sim_video->h, p_w, p_h, yuv_msg->x, yuv_msg->y);         //把大图拷好
			
			tmp = 0;
			while(tmp < sim_video->screen_num){
				if( (match_count != tmp) && (sim_video->dsp_window_msg[tmp].w != 0)){
					if(cache_buf[tmp]){	 																			//图片是从之前的备用区提取出来的，在缓存中
						yuv_blk_cpy((uint8_t*)output_fb->data, (uint8_t*)cache_buf[tmp], sim_video->w, sim_video->h, sim_video->dsp_window_msg[tmp].w,sim_video->dsp_window_msg[tmp].h, sim_video->dsp_window_msg[tmp].x,sim_video->dsp_window_msg[tmp].y);  //拷到对应的psram块中
						STREAM_FREE((uint8_t*)cache_buf[tmp]);						
					}
					else{																							//图片得从刷新区那里提取
						cache_buf[tmp] = (uint32_t *)STREAM_MALLOC(sim_video->dsp_window_msg[tmp].w * sim_video->dsp_window_msg[tmp].h * 3 / 2);
						if(cache_buf[tmp] != NULL){
							yuv_blk_reduce((uint8_t*)cache_buf[tmp],(uint8_t*)dispbuf,sim_video->dsp_window_msg[tmp].w,sim_video->dsp_window_msg[tmp].h,sim_video->w,sim_video->h,sim_video->dsp_window_msg[tmp].x,sim_video->dsp_window_msg[tmp].y);
							yuv_blk_cpy((uint8_t*)output_fb->data, (uint8_t*)cache_buf[tmp], sim_video->w, sim_video->h, sim_video->dsp_window_msg[tmp].w,sim_video->dsp_window_msg[tmp].h, sim_video->dsp_window_msg[tmp].x,sim_video->dsp_window_msg[tmp].y);
							STREAM_FREE((uint8_t*)cache_buf[tmp]);
						}else{
							_os_printf("malloc psram for sim video error2\r\n");
						}					
						
					}
				}
				tmp++;
			}
		}

	
		
sim_video_push_lcd:		
		if(push_lcd_updata == 1){	
			
			memcpy(sim_video->dispwindow_cnt,sim_video->subwindow_cnt,10*4);		
            output_fb->mtype = F_YUV;
            output_fb->stype = FSTYPE_YUV_P0;
            msi_output_fb(sim_video->msi, output_fb);	      //将当前推屏的fb推出去
			sim_video->lcd_fb_updata = 1;								  //下次提取新的刷屏帧
            sim_video->last_fb = NULL;
		}else{				
			msi_delete_fb(NULL, fb);										  //删除对应的输入到msi video的fb
			fb = NULL;
		}
    }
sim_video_work_end:
	if((os_jiffies() - lcd_s->start_time) > (lcd_s->refresh_time - REFRESH_LCD_INTERNAL_TIME)){      //当距离结束刷屏时间为REFRESH_LCD_INTERNAL_TIME时，检查是否需要更新，需要的话，就先更新
		if(output_fb){					//先检查推屏的fb是否存在	
			tmp = 0;
			while(tmp < sim_video->screen_num){					     //检查各个叠层
				if(sim_video->dispwindow_cnt[tmp] != sim_video->subwindow_cnt[tmp]){       //发现有叠层出现更新了	
					memcpy(sim_video->dispwindow_cnt,sim_video->subwindow_cnt,10*4);		
		            output_fb->mtype = F_YUV;
		            output_fb->stype = FSTYPE_YUV_P0;
		            msi_output_fb(sim_video->msi, output_fb);	      //将当前推屏的fb推出去
					sim_video->lcd_fb_updata = 1;								  //下次提取新的刷屏帧
                    sim_video->last_fb = NULL;
					break;
				}
				tmp++;
			}
			
		}	
	}	

    os_run_work_delay(work, 1);
    return 0;
}

static int sim_video_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    struct sim_video_more_msi_s *sim_video = (struct sim_video_more_msi_s *)msi->priv;
    int ret = RET_OK;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            struct framebuff *fb;
            // 释放资源fb资源文件,priv是独立申请的
            while (1)
            {
                fb = fbpool_get(&sim_video->tx_pool, 0, NULL);
                if (!fb)
                {
                    break;
                }
                if (fb->data)
                {
                    STREAM_FREE(fb->data);
                    fb->data = NULL;
                }
                // 预分配空间释放
                if (fb->priv)
                {
                    STREAM_LIBC_FREE(fb->priv);
                }
            }
            fbpool_destroy(&sim_video->tx_pool);
            STREAM_LIBC_FREE(sim_video);
        }
        break;
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;

            if (fb->mtype != F_YUV)
            {
                // os_printf("not recv fb:%X\tcount:%d\n",fb,fb->users.counter);
                ret = RET_ERR;
            }
            else
            {
                if (sim_video->filter)
                {
                    // 轮询类型是否有一致
                    ret = RET_ERR;
                    uint16_t *each = sim_video->filter;
                    while (*each)
                    {
                        // 如果一致,返回OK
                        if (*each == fb->stype)
                        {
                            ret = RET_OK;
                            break;
                        }
                        each++;
                    }
                }
            }
        }
        break;

        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;
            // 如果不是克隆的,就自己释放
            if (!fb->clone)
            {
//                if (fb->data)
//                {
//                    STREAM_FREE(fb->data);
//                    fb->data = NULL;
//                }
                fbpool_put(&sim_video->tx_pool, fb);
                // 不需要内核去释放fb
                ret = RET_OK + 1;
            }
        }
        break;
        case MSI_CMD_PRE_DESTROY:
        {
            os_work_cancle2(&sim_video->work, 1);
            uint16_t tmp = 0;
            while (tmp < sim_video->screen_num)
            {
                if (sim_video->screen[tmp])
                {
                    msi_delete_fb(NULL, sim_video->screen[tmp]);
                }
                tmp++;
            }

            while (1)
            {
                struct framebuff *fb = msi_get_fb(msi, 0);
                if (fb) {
                    msi_delete_fb(NULL, fb);
                } else {
                    break;
                }
            }

            if (sim_video->last_fb){
                msi_delete_fb(NULL, sim_video->last_fb);
                sim_video->last_fb = NULL;
            }

            if(sim_video->last_input_fb){
                msi_delete_fb(NULL, sim_video->last_input_fb);
                sim_video->last_input_fb = NULL;
            }
        }
        break;
        default:
        {
        }
        break;
    }

    return ret;
}

struct msi *sim_video_more_msi(const char *name, uint16_t w, uint16_t h, uint16_t *sim_filter)
{
    // 设置1个接收,只是为了可以output_fb给到这个msi
    uint16_t count = 0;
	uint32_t *buf;
    struct msi *msi = msi_new(name, MAX_RX, NULL);
    if (msi)
    {
        struct sim_video_more_msi_s *sim_video = (struct sim_video_more_msi_s *)msi->priv;
        if (!sim_video)
        {
            // 统计sim_filter数量来决定screen的数量

            while (*(sim_filter + count))
            {
                count++;
            }
            sim_video = (struct sim_video_more_msi_s *)STREAM_LIBC_ZALLOC(sizeof(struct sim_video_more_msi_s) + (sizeof(struct framebuff *) * count));
        }

        sim_video->w = w;
        sim_video->h = h;
        sim_video->msi = msi;
        sim_video->filter = sim_filter;
        sim_video->screen_num = count;
        // 申请空间的时候就已经多申请了,所有screen放在末尾
        sim_video->screen = (struct framebuff **)(sim_video + 1);
        sim_video->lcd_fb_updata = 1;
        msi->priv = (void *)sim_video;
        msi->action = sim_video_msi_action;

        // 预分配结构体空间
        uint32_t init_count = 0;
        void *priv;
        fbpool_init(&sim_video->tx_pool, MAX_TX);
        while (init_count < MAX_TX)
        {
            priv = (void *)STREAM_LIBC_ZALLOC(sizeof(struct yuv_arg_s));
            struct yuv_arg_s *yuv_msg = (struct yuv_arg_s *)priv;
			buf = (uint32_t *)STREAM_MALLOC(sim_video->w * sim_video->h * 3 / 2);
            memset((uint8_t*)buf, 0, sim_video->w * sim_video->h);												//清空整个空间，将显示背景显示全黑
            memset((uint8_t*)buf + sim_video->w * sim_video->h, 0x80, sim_video->w * sim_video->h / 2);          
            sys_dcache_clean_range(buf, sim_video->w * sim_video->h * 3 / 2);									//回写psram
            sys_dcache_clean_invalid_range(buf, sim_video->w * sim_video->h * 3 / 2);							//清cache
            yuv_msg->out_w = w;
            yuv_msg->out_h = h;
            yuv_msg->y_size = w * h;
            yuv_msg->y_off = 0;
            yuv_msg->uv_off = 0;
            yuv_msg->del = NULL;
			sim_video->bufadr[init_count] = (uint32_t)buf;
            FBPOOL_SET_INFO(&sim_video->tx_pool, init_count, (uint8_t*)buf, sim_video->w * sim_video->h * 3 / 2, priv);
            init_count++;
        }
		
		
		
        msi->enable = 1;

        // msi_add_output(sim_video->msi, NULL, R_VIDEO_P0);

        OS_WORK_INIT(&sim_video->work, sim_video_more_work_more, 0);
        os_run_work_delay(&sim_video->work, 1);
    }
    return msi;
}
