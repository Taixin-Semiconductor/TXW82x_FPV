#include "scale_msi.h"
#include "dev/vpp/hgvpp.h"
#include "dev/scale/hgscale.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"


// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE av_free
#define STREAM_LIBC_ZALLOC av_zalloc

uint8 *scaler2buf;
extern uint32 scale_p1_w;
#define SCALE2_SRAMBUF_WLEN   64//64

struct scale2_yuv_arg_s
{
    struct yuv_arg_s yuv_arg;
    uint8_t seq;
};

struct  scale2_msg_t scale2_msg[3] = {
	//ISP_VIDEO_0
	{
		.stype = FSTYPE_YUV_P1,
		.iw = 640,
		.ih = 480,
		.ow = 320,
		.oh = 240,		
		.x  = 0,
		.y  = 0,
		.video_only = 0,
	},
	//ISP_VIDEO_1
	{
		.stype = FSTYPE_YUV_P0,
		.iw = 640,
		.ih = 480,
		.ow = 320,
		.oh = 240,			
		.x	= 320,
		.y	= 0,
		.video_only = 0,
	},
	//ISP_VIDEO_2
	{
		.stype = FSTYPE_YUV_P2,
		.iw = 640,
		.ih = 480,
		.ow = 320,
		.oh = 240,			
		.x	= 0,
		.y	= 180,
		.video_only = 0,
	},
};

volatile uint8_t scaler2_dev_id = 0;

// 这里可能采用信号量的形式,通知到线程去处理数据
static int32_t scale2_stream_done(uint32 irq_flag, uint32 irq_data, uint32 param1)
{
    struct scale2_msi_s *scale2 = (struct scale2_msi_s *)irq_data;
    //struct framebuff *fb;
    struct scale2_yuv_arg_s *arg;
    //判断序号,有可能双镜头
    scale2->seq++;

    if (scale2->now_fb) {
        arg = (struct scale2_yuv_arg_s*)scale2->now_fb->priv;
        arg->seq = scale2->seq;
    
        arg->yuv_arg.y_size = scale2->ow*scale2->oh;
        arg->yuv_arg.x = scale2->x;       
        arg->yuv_arg.y = scale2->y;
        arg->yuv_arg.video_only = scale2_msg[scaler2_dev_id].video_only;
        arg->yuv_arg.out_w =scale2->ow;
        arg->yuv_arg.out_h =scale2->oh;
        
        scale2->now_fb->stype = scale2_msg[scaler2_dev_id].stype;
        //os_printf("D");
    
    
        // 不再直接放now_fb，而是把now_fb放到预分配的槽(now_fb_msg)中，然后把槽地址放入消息队列
        uint8_t idx = scale2->now_fb_msg_idx;
        if (scale2->now_fb_msg[idx] == NULL) {
            scale2->now_fb_msg[idx] = scale2->now_fb;
            // advance index
            scale2->now_fb_msg_idx = (idx + 1) % MAX_SCALE2_TX;
    
            if (os_msgq_put(&scale2->msgq, (uint32_t)&scale2->now_fb_msg[idx], 0)) {
                if (scale2->now_fb_msg[idx] == scale2->now_fb) {
                    scale2->now_fb_msg[idx] = NULL;
                }
                msi_delete_fb(NULL, scale2->now_fb);
            }
        } else {
            msi_delete_fb(NULL, scale2->now_fb);
        }
    }

    scale2->now_fb = NULL;
    scale2->mutex_count = 0;

    return 0;
}


static int32_t scale2_stream_ov(uint32 irq_flag, uint32 irq_data, uint32 param1)
{
    os_printf("%s:%d\n", __FUNCTION__, __LINE__);
    return 0;
}



static int32 scale2_stream_work(struct os_work *work)
{
    struct scale2_msi_s *scale2 = (struct scale2_msi_s *)work;
    struct framebuff *fb;
    struct scale2_yuv_arg_s *arg;
    int32_t err = -1;

    struct framebuff **slot = (struct framebuff **)os_msgq_get2(&scale2->msgq, 0, &err);
    // 没有数据
    if (err)
    {
        goto scale2_stream_work_end;
    }
    fb = NULL;
    if (slot)
    {
        uint32 ie = disable_irq();
        fb = *slot;
        *slot = NULL;
        enable_irq(ie);
    }
    
    arg = (struct scale2_yuv_arg_s*)fb->priv;
	arg->yuv_arg.dispcnt++;
    fb->mtype = F_YUV;
    //fb->stype = scale2->type;
    //_os_printf("scale2 fb:%X\ttype:%d\n",fb,fb->stype);
    if (scale2->filter_type != ~0 && scale2->filter_type != fb->srcID) {
        msi_delete_fb(scale2->msi, fb);
    } else {
        msi_output_fb(scale2->msi, fb, 0);
    }

scale2_stream_work_end:
    // 过1ms就去轮询一遍,实际如果用信号量,可以改成任务形式,等待信号量,可以节约cpu(实际cache影响可能更大,cpu占用很少)
    // 由于workqueue没有支持等待信号量,只能通过1ms轮询一下
    os_run_work_delay(work, 1);
    return 0;
}

static int32_t scale2_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    struct scale2_msi_s *scale2 = (struct scale2_msi_s *)msi->priv;
	//uint32_t ow_n,oh_n;
	//uint32_t iw_n,ih_n;
	uint8_t *yinaddr;
	uint8_t *uinaddr;
	uint8_t *vinaddr;
    switch (cmd_id)
    {

        // 这里msi已经被删除,那么就要考虑tx_pool的资源释放了
        // 能进来这里,就是代表所有fb都已经用完了
        case MSI_CMD_POST_DESTROY:
        {


            // 释放资源fb资源文件
            FBPOOL_FREE(&scale2->tx_pool, STREAM_FREE, STREAM_LIBC_FREE);
			STREAM_LIBC_FREE(scaler2buf);
            scaler2buf = NULL;
            fbpool_destroy(&scale2->tx_pool);
            STREAM_LIBC_FREE(scale2);
        }
        break;

        // 停止硬件,移除没有必要的资源(但是fb的资源不能现在删除,这个时候fb可能外部还在调用)
        case MSI_CMD_PRE_DESTROY:
        {
            struct framebuff *fb = NULL;
            int32_t err = 0;

			os_printf("%s  %d\r\n",__func__,__LINE__);
            scale_close(scale2->scale_dev);
			os_work_cancle2(&scale2->work, 1);
            // os_printf("%s:%d MSI_CMD_PRE_DESTROY\n", __FUNCTION__, __LINE__);

            // 移除信号量里面的数据

            while (!err)
            {
                // 取出的是槽地址，首先获取槽，再释放其中的真实fb
                struct framebuff **slot = (struct framebuff **)os_msgq_get2(&scale2->msgq, 0, &err);
                if (slot)
                {
                    uint32 ie = disable_irq();
                    fb = *slot;
                    *slot = NULL;
                    enable_irq(ie);
                    if (fb)
                    {
                        msi_delete_fb(NULL, fb);
                    }
                }
                fb = NULL;
            }

            os_msgq_del(&scale2->msgq);
			
			
        }
        break;
        // 接收,判断是类型是否可以支持压缩
        case MSI_CMD_TRANS_FB:
        {
        }
        break;
        // 预先分配的,默认不需要释放fb->data,除非是MSI_CMD_DESTROY后,就要释放
        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;
            if (fb->data)
            {
                //sys_dcache_clean_invalid_range((uint32_t*)fb->data, fb->len);
				STREAM_FREE(fb->data);
				fb->data = NULL;
            }
            #if 0 // 添加了 fb->free 和 fb->free_priv
            fbpool_put(&scale2->tx_pool, fb);
            // 不需要内核去释放fb
            ret = RET_OK + 1;
            #endif
        }
        break;

        case MSI_CMD_SCALE2:
        {
            uint32_t cmd_self = (uint32_t)param1;
			uint32 scale2_p1_w;
			uint8_t *data;
            uint8_t decfrom = param2;
            // 自定义命令
            switch (cmd_self)
            {				
                case MSI_SCALE2_SET_FILTER_TYPE:
                {
                    scale2->filter_type = param2;
                }
                break;

                case MSI_SCALE2_START:
                {
                    //注意,这里需要根据vpp那边配置来决定用哪个
                    //注意line buf的数量

                    // 暂时用同样的iw ih ow oh
                    //scale_from_h264_config(scale2->scale_dev,scale2->iw,scale2->ih,scale2->ow,scale2->oh,10);
					//set_lcd_photo1_config(scale2->ow,scale2->oh,0);
					scale2_p1_w    = ((scale2->ow+3)/4)*4;
					
					scale_close(scale2->scale_dev);
					scale_set_input_stream(scale2->scale_dev,decfrom);
					scale_set_output_sram_or_frame(scale2->scale_dev,1);					
					scale_set_in_out_size(scale2->scale_dev,scale2->iw,scale2->ih,scale2->ow,scale2->oh);		
					scale_set_step(scale2->scale_dev,scale2->iw,scale2->ih,scale2->stw,scale2->sth);
					scale_set_start_addr(scale2->scale_dev,0,0);
					
					if(scaler2buf == NULL){
						if(scale2->ow <= scale2->iw){
							scaler2buf = STREAM_LIBC_MALLOC(0x20+scale2_p1_w+20*SCALE2_SRAMBUF_WLEN*4+256 + 0x12+scale2_p1_w/2+11*SCALE2_SRAMBUF_WLEN*2+128+0x12+scale2_p1_w/2+11*SCALE2_SRAMBUF_WLEN*2+128+12);
						}else{
							scaler2buf = STREAM_LIBC_MALLOC(0x20+scale2_p1_w+40*SCALE2_SRAMBUF_WLEN*4+256 + 0x12+scale2_p1_w/2+22*SCALE2_SRAMBUF_WLEN*2+128+0x12+scale2_p1_w/2+22*SCALE2_SRAMBUF_WLEN*2+128+12);
						}
						
						if(scaler2buf == NULL){
							os_printf("malloc scaler2 fail.......\r\n");
							return RET_ERR;
						}else{
							os_printf("scaler 2 malloc finish\r\n");
						}
						
						scale2->scaler2buf = scaler2buf;
						yinaddr = scaler2buf;
						if(scale2->ow <= scale2->iw){
							uinaddr = yinaddr+((0x20+scale2_p1_w+20*SCALE2_SRAMBUF_WLEN*4+256 + 3)/4)*4;
							vinaddr = uinaddr+((0x12+scale2_p1_w/2+11*SCALE2_SRAMBUF_WLEN*2+128 + 3)/4)*4;
						}else{
							uinaddr = yinaddr+((0x20+scale2_p1_w+40*SCALE2_SRAMBUF_WLEN*4+256 + 3)/4)*4;
							vinaddr = uinaddr+((0x12+scale2_p1_w/2+22*SCALE2_SRAMBUF_WLEN*2+128 + 3)/4)*4;
						}
						
						scale_linebuf_yuv_addr(scale2->scale_dev,(uint32)yinaddr,(uint32)uinaddr,(uint32)vinaddr);
					}

					
					scale_set_srambuf_wlen(scale2->scale_dev,SCALE2_SRAMBUF_WLEN);
					scale_request_irq(scale2->scale_dev,FRAME_END,(scale_irq_hdl )&scale2_stream_done,(uint32)scale2);	
					scale_request_irq(scale2->scale_dev,INBUF_OV,(scale_irq_hdl )&scale2_stream_ov,(uint32)scale2);
						
					
                    // 这里分配一下scale2的空间,通过标准接口去分配吧,理论这里一定能获取到,这里就不判断异常情况了
                    struct framebuff *fb;
                    // 正常应该要获取到fb
                    fb = fbpool_get(&scale2->tx_pool, 0, scale2->msi);
                    uint8_t *p_buf;

				    if (!fb)
				    {
				        //os_printf("N scale2->now_fb:%X\r\n",scale2->now_fb);
				        // 找不到新的空间,则返回,使用旧空间
				        return 0;
				    }
                    fb->srcID = decfrom;				
					data = (uint8_t *)STREAM_MALLOC(scale2->ow * scale2->oh * 3 / 2);
					sys_dcache_invalid_range((uint32_t*)data, scale2->ow * scale2->oh * 3 / 2);
					fb->data = data;
					fb->len  = (scale2->ow * scale2->oh * 3) / 2;
                    p_buf = fb->data;
                    scale2->now_fb = fb;
                    scale_set_out_yaddr(scale2->scale_dev, (uint32)p_buf);
                    scale_set_out_uaddr(scale2->scale_dev, (uint32)p_buf + scale2->ow * scale2->oh);
                    scale_set_out_vaddr(scale2->scale_dev, (uint32)p_buf + scale2->ow * scale2->oh + scale2->ow * scale2->oh / 4);	
					scale_open(scale2->scale_dev);
                }
                break;
            }
        }
        break;
        default:
            break;
    }
    return ret;
}

void scale2_output_size_local_change(uint8_t id,uint8_t show_only,uint16 x,uint16 y,uint16 w,uint16 h){
	uint32 ie;
	ie = disable_irq();
	scale2_msg[id].x = x;
	scale2_msg[id].y = y;
	scale2_msg[id].ow = w;
	scale2_msg[id].oh = h;
	scale2_msg[id].video_only = show_only;
	enable_irq(ie);
}

// 参数分别是vpp的图像iw和ih,要scale的ow和oh,如果和屏有关,可以传入屏幕的宽高
struct msi *scale2_msi(const char *name, uint16_t iw, uint16_t ih, uint16_t ow, uint16_t oh, uint16_t type,uint8_t larger)
{
	uint8_t itk;
    struct msi *msi = msi_new(name, 0, NULL);
    struct scale2_msi_s *scale2 = (struct scale2_msi_s *)msi->priv;
	if(msi == NULL)
		return NULL;

    if (!scale2)
    {
        // os_printf("%s:%d new success\n", __FUNCTION__, __LINE__);
        scale2 = (struct scale2_msi_s *)STREAM_LIBC_ZALLOC(sizeof(struct scale2_msi_s));
        msi->action = scale2_msi_action;
        msi->priv = (void *)scale2;
        scale2->msi = msi;
        scale2->scale_dev = (struct scale_device *)dev_get(HG_SCALE2_DEVID);

		for(itk = 0;itk < 3;itk++){
			scale2_msg[itk].iw = iw;
			scale2_msg[itk].ih = ih;
			scale2_msg[itk].ow = ow;
			scale2_msg[itk].oh = oh;
		}
		
        scale2->filter_type = ~0;
        scale2->ow = ow;
        scale2->oh = oh;
        scale2->iw = iw;
        scale2->ih = ih;
        scale2->stw = ow;
        scale2->sth = oh;
        scale2->x = 0;
        scale2->y = 0;		
        scale2->type = type;
		scale2->larger = larger;
        fbpool_init(&scale2->tx_pool, MAX_SCALE2_TX, NULL, NULL);
        // 初始化now_fb_msg槽位为NULL并重置索引
        scale2->now_fb_msg_idx = 0;
        for (itk = 0; itk < MAX_SCALE2_TX; itk++) {
            scale2->now_fb_msg[itk] = NULL;
        }

        uint8_t init_count = 0;
        //uint8_t *data;
        // 初始化fb的一些默认信息
        while (init_count < MAX_SCALE2_TX)
        {
            // 预分配framebuff的空间给到scale
            //data = (uint8_t *)STREAM_MALLOC(ow * oh * 3 / 2);
            // 先配置参数信息
            //配置yuv信息,但是申请空间是申请scale3_yuv_arg_s,这里是特殊的,scale3_yuv_arg_s有自己的参数需要用到
            struct yuv_arg_s *yuv = (struct yuv_arg_s *)STREAM_LIBC_ZALLOC(sizeof(struct scale2_yuv_arg_s));
            // 由于创建流的时候,参数已经分配好了,所以这里可以确定参数
            yuv->y_size = scale2->ow * scale2->oh;
            yuv->uv_off = 0;
            yuv->y_off = 0;
			yuv->dispcnt = 0;
            yuv->out_w = scale2->ow;
            yuv->out_h = scale2->oh;
            yuv->del = &scale2->del;
            //os_printf("malloc data:%X\n",data);
            FBPOOL_SET_INFO(&scale2->tx_pool, init_count, NULL, ow * oh * 3 / 2, (void *)yuv);
            init_count++;
        }
        msi->enable = 1;
        // msi_add_output(msi,NULL, NULL,R_VIDEO_P1);
        // 发送自定义命令,启动scale3
        //msi_do_cmd(msi, MSI_CMD_SCALE2, MSI_SCALE2_START, 0);

        os_msgq_init(&scale2->msgq, MAX_SCALE2_TX);

        OS_WORK_INIT(&scale2->work, scale2_stream_work, 0);
        os_run_work_delay(&scale2->work, 1);		
    }
    os_printf("%s:%d\tmsi:%X\n", __FUNCTION__, __LINE__, msi);
    return msi;
}

int scale2_cfg_run(uint8_t streamfrom,uint8_t id){
	struct msi *scaler_msi = msi_find("scale2",0);
	struct scale2_msi_s *scale2 = (struct scale2_msi_s *)scaler_msi->priv;
	uint32 ie;
	int ret = 0;

	if(scaler_msi) {
		msi_put(scaler_msi);
	}


    if(scale2->mutex_count){
        ret = 1;
        return ret;
    }
    scale2->mutex_count = 1;

	ie = disable_irq();	
    scale2->ow = scale2_msg[id].ow;
    scale2->oh = scale2_msg[id].oh;
    scale2->iw = scale2_msg[id].iw;
    scale2->ih = scale2_msg[id].ih;	
    scale2->stw = scale2_msg[id].ow;
    scale2->sth = scale2_msg[id].oh;
    scale2->x   = scale2_msg[id].x;
    scale2->y   = scale2_msg[id].y;
	enable_irq(ie);
	
	msi_do_cmd(scaler_msi, MSI_CMD_SCALE2, MSI_SCALE2_START, streamfrom);	
	return ret;
}

void scale2_recfg_input_size(uint16_t w,uint16_t h,uint8_t id){
	uint32 ie;
	ie = disable_irq();
	scale2_msg[id].iw =w;
	scale2_msg[id].ih = h;
	enable_irq(ie);	
}
