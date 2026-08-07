
#include "scale_msi.h"
#include "dev/vpp/hgvpp.h"
#include "lib/video/vpp/vpp_dev.h"
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
struct scale3_yuv_arg_s
{
    struct yuv_arg_s yuv_arg;
	uint16_t ow,oh;
	uint8_t tx_type;    //0~3  :video     4:error fb   
};

//暂时这里去控制scale3从哪个buf过来
//请配合VPP那边配置
#define SCALE3_YUVBUF_0_1           SCALE3_FROM_VPPBF

extern uint8 *yuvbuf;
extern uint8 *yuvbuf1;
#if SCALE3_YUVBUF_0_1 == 0
#define SCALE3_LINE_BUF             yuvbuf
#define SCALE3_YUVBUF_Y_OFF_LINE    ((VPP_BUF0_MODE == VPP_MODE_2N) ? (VPP_BUF0_LINEBUF_NUM * 2) : (VPP_BUF0_LINEBUF_NUM * 2 + 16))
#elif SCALE3_YUVBUF_0_1 == 1
#define SCALE3_LINE_BUF             yuvbuf1
#define SCALE3_YUVBUF_Y_OFF_LINE    ((VPP_BUF1_MODE == VPP_MODE_2N) ? (VPP_BUF1_LINEBUF_NUM * 2) : (VPP_BUF1_LINEBUF_NUM * 2 + 16))
#endif


// 暂定这个scale3是在存在dvp的时候使用,所以用宏隔离
// 实际scale3还有另一种模式不需要dvp那边启动的


// 这里可能采用信号量的形式,通知到线程去处理数据

struct  scale_msg_t scale3_msg[3] = {
	//ISP_VIDEO_0
	{
		.iw = 640,
		.ih = 480,
		.ow = 320,
		.oh = 180,
		.x  = 0,
		.y  = 0,
	},
	//ISP_VIDEO_1
	{
		.iw = 640,
		.ih = 360,
		.ow = 320,
		.oh = 180,
		.x	= 360,
		.y	= 0,
	},
	//ISP_VIDEO_2
	{
		.iw = 640,
		.ih = 360,
		.ow = 320,
		.oh = 180,
		.x	= 0,
		.y	= 200,
	},
};




static int32_t scale3_stream_done(uint32 irq_flag, uint32 irq_data, uint32 param1)
{
    struct scale3_msi_s *scale3 = (struct scale3_msi_s *)irq_data;
    struct framebuff *fb = NULL;
    uint8_t *p_buf;
	uint8_t sennum = 0;
    struct scale3_yuv_arg_s *arg;
	uint8_t errfb;
//	static uint32_t  scaler_cnt;
    //判断序号,有可能双镜头
//    scale3->seq++;
	if(scale3->txpool_type == 1){
		fb = fbpool_get(&scale3->tx_pool0, 0, scale3->msi);
	}else{
		if(video_msg.video_type_cur == ISP_VIDEO_0){
			if(video_msg.video_num == 2){
				fb = fbpool_get(&scale3->tx_pool1, 0, scale3->msi);
			}else{
				if(video_msg.video_type_last == ISP_VIDEO_1){
					fb = fbpool_get(&scale3->tx_pool2, 0, scale3->msi);
				}else{
					fb = fbpool_get(&scale3->tx_pool1, 0, scale3->msi);
				}
			}
		}
		
		if(video_msg.video_type_cur == ISP_VIDEO_1){
			fb = fbpool_get(&scale3->tx_pool0, 0, scale3->msi);
		}

		if(video_msg.video_type_cur == ISP_VIDEO_2){
			fb = fbpool_get(&scale3->tx_pool0, 0, scale3->msi);
		}
	}
	
	errfb = 0;
	if(fb == NULL){
		fb = fbpool_get(&scale3->tx_poolerr, 0, scale3->msi);
		errfb = 1;
	}

	if(video_msg.video_num == 1){
		sennum = 0;
	}else if(video_msg.video_num == 2){
		if(video_msg.video_type_cur == ISP_VIDEO_0){
			sennum = 1;
		}else{
			sennum = 0;
		}
	}else{
		if((video_msg.video_type_cur == ISP_VIDEO_1)||(video_msg.video_type_cur == ISP_VIDEO_2)){
			sennum = 0;
		}else if(video_msg.video_type_last == ISP_VIDEO_1){
			sennum = 2;	
		}else if(video_msg.video_type_last == ISP_VIDEO_2){
			sennum = 1;
		}
	}

	
  	arg = (struct scale3_yuv_arg_s*)fb->priv;
	if(errfb){
		scale3->ow = 128;//ow;
		scale3->oh = 96;//oh;
	}else{
		if(fb->len != (scale3_msg[sennum].ow * scale3_msg[sennum].oh * 3)/2 ){
			scale3->ow = arg->ow;
			scale3->oh = arg->oh;
		}else{
			scale3->ow = scale3_msg[sennum].ow;//ow;
			scale3->oh = scale3_msg[sennum].oh;//oh;
		}
	}
	scale3->iw = scale3_msg[sennum].iw;//iw;
	scale3->ih = scale3_msg[sennum].ih;//ih;					
	arg->yuv_arg.x = scale3_msg[sennum].x;
	arg->yuv_arg.y = scale3_msg[sennum].y;
	arg->yuv_arg.out_w = scale3->ow;
	arg->yuv_arg.out_h = scale3->oh;
	
	// 暂时用同样的iw ih ow oh
	scale_set_in_out_size(scale3->scale_dev, scale3->iw, scale3->ih, scale3->ow, scale3->oh);
	scale_set_step(scale3->scale_dev, scale3->iw, scale3->ih, scale3->ow, scale3->oh);
	scale_set_in_yaddr(scale3->scale_dev, (uint32)scale3->scaler3buf);
	scale_set_in_uaddr(scale3->scale_dev, (uint32)scale3->scaler3buf + scale3->iw * SCALE3_YUVBUF_Y_OFF_LINE);
	scale_set_in_vaddr(scale3->scale_dev, (uint32)scale3->scaler3buf + scale3->iw * SCALE3_YUVBUF_Y_OFF_LINE + scale3->iw * SCALE3_YUVBUF_Y_OFF_LINE/4);
	_os_printf("S");
//	_os_printf("M(%x  %x)",fb,fb->data);
	fb->len  = scale3->ow * scale3->oh * 3 / 2;
    // 配置新的空间地址
    p_buf = (uint8_t *)fb->data;
    scale_set_out_yaddr(scale3->scale_dev, (uint32)p_buf);
    scale_set_out_uaddr(scale3->scale_dev, (uint32)p_buf + scale3->ow * scale3->oh);
    scale_set_out_vaddr(scale3->scale_dev, (uint32)p_buf + scale3->ow * scale3->oh + scale3->ow * scale3->oh / 4);

	if(errfb){
		msi_delete_fb(NULL, fb);
		return 0;
	}
	
	scale3->now_fb->stype = FSTYPE_YUV_P0+video_msg.video_type_cur;
    // 发送now_data,发送失败也要返回
    if (os_msgq_put(&scale3->msgq, (uint32_t)scale3->now_fb, 0))
    {
        // 正常不能中断del,但是这个模块是内部,只要del没有一些等待信号量操作,问题不大
        msi_delete_fb(NULL, scale3->now_fb);
        scale3->now_fb = NULL;
        //return 0;
    }
    
    scale3->now_fb = fb;
    // os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    return 0;
}

static int32_t scale3_stream_ov(uint32 irq_flag, uint32 irq_data, uint32 param1)
{
    os_printf("%s:%d\n", __FUNCTION__, __LINE__);
    return 0;
}


static int32 scale3_stream_work(struct os_work *work)
{
	int32_t ret;
    struct scale3_msi_s *scale3 = (struct scale3_msi_s *)work;
    struct framebuff *fb;
    struct scale3_yuv_arg_s *arg;
    int32_t err = -1;
    fb = (struct framebuff *)os_msgq_get2(&scale3->msgq, 0, &err);
    // 没有数据
    if (err)
    {
        goto scale3_stream_work_end;
    }
	
    arg = (struct scale3_yuv_arg_s*)fb->priv;
	arg->yuv_arg.dispcnt++;
    fb->mtype = F_YUV;
	//_os_printf("P(%d x:%d y:%d)",fb->stype,arg->yuv_arg.x,arg->yuv_arg.y);
    //os_printf("scale3 fb:%X  type:%d  x:%d  y:%d   w:%d   h:%d\r\n",fb,fb->stype,arg->yuv_arg.x,arg->yuv_arg.y,arg->yuv_arg.out_w,arg->yuv_arg.out_h);
	if(arg->tx_type == 4){
		fbpool_put(&scale3->tx_poolerr, fb);
	}else{
		ret = msi_output_fb(scale3->msi, fb);
	}
	
scale3_stream_work_end:
    // 过1ms就去轮询一遍,实际如果用信号量,可以改成任务形式,等待信号量,可以节约cpu(实际cache影响可能更大,cpu占用很少)
    // 由于workqueue没有支持等待信号量,只能通过1ms轮询一下
    os_run_work_delay(work, 1);
    return 0;
}

static int32_t scale3_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    struct scale3_msi_s *scale3 = (struct scale3_msi_s *)msi->priv;
	struct scale3_yuv_arg_s *arg;
    switch (cmd_id)
    {

        // 这里msi已经被删除,那么就要考虑tx_pool的资源释放了
        // 能进来这里,就是代表所有fb都已经用完了
        case MSI_CMD_POST_DESTROY:
        {

            struct framebuff *fb;
            // 释放资源fb资源文件
			int temp_video_num = scale3->init_txpool_num;
			for(int init_txpool_num = 0; init_txpool_num < temp_video_num ; init_txpool_num++) {
            while (1)
            {
					fb = fbpool_get(&scale3->tx_pool0 + init_txpool_num, 0, NULL);
                if (!fb)
                {
                    break;
                }
					if(fb->data)
					{
						//os_printf("D(%x  %x)\n",fb,fb->data);
						STREAM_FREE(fb->data);
						fb->data = NULL;
					}
                // 预分配空间释放
                if (fb->priv)
                {
						//os_printf("D(%x  %x)\n",fb,fb->priv);
                    STREAM_LIBC_FREE(fb->priv);
						fb->priv = NULL;
                }
            }
				fbpool_destroy(&scale3->tx_pool0 + init_txpool_num);
			}

				while (1)
				 {
				fb = fbpool_get(&scale3->tx_poolerr, 0, NULL);
					 if (!fb)
					 {
						 break;
					 }
				if(fb->data)
					 {
					//os_printf("E(%x  %x)\n",fb,fb->data);
					STREAM_FREE(fb->data);
					fb->data = NULL;
					 }
					 // 预分配空间释放
					 if (fb->priv)
					 {
					//os_printf("E(%x  %x)\n",fb,fb->priv);
						 STREAM_LIBC_FREE(fb->priv);
					fb->priv = NULL;
					 }
				 }
			fbpool_destroy(&scale3->tx_poolerr);
            STREAM_LIBC_FREE(scale3);
			msi->priv = NULL;
        }
        break;

        // 停止硬件,移除没有必要的资源(但是fb的资源不能现在删除,这个时候fb可能外部还在调用)
        case MSI_CMD_PRE_DESTROY:
        {
            os_work_cancle2(&scale3->work, 1);
            scale_close(scale3->scale_dev);
            // os_printf("%s:%d MSI_CMD_PRE_DESTROY\n", __FUNCTION__, __LINE__);

            // 移除信号量里面的数据
            struct framebuff *fb = NULL;
            int32_t err = 0;
            while (!err)
            {
                fb = (struct framebuff *)os_msgq_get2(&scale3->msgq, 0, &err);
                if (fb)
                {
                    msi_delete_fb(NULL, fb);
                }
                fb = NULL;
            }

			if (scale3->msgq.hdl) {
				os_msgq_del(&scale3->msgq);
			}

            fb = scale3->now_fb;
            if (scale3->now_fb)
            {
                msi_delete_fb(NULL, scale3->now_fb);
                scale3->now_fb = NULL;
            }
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
			uint8_t type = 0;
			arg = (struct scale3_yuv_arg_s*)fb->priv;   
			if(arg->tx_type == 0){
				type = 0;
			}else if(arg->tx_type == 1){
				type = 1;
			}else if(arg->tx_type == 2){
				type = 2;
			}else{
				type = 4;
			}
			
			if (fb->data && (type < 4))
			{
				if(fb->len != (scale3_msg[type].ow * scale3_msg[type].oh * 3) / 2){
					STREAM_FREE(fb->data);																		  //释放之前的空间
					fb->data = (uint8_t *)STREAM_MALLOC((scale3_msg[type].ow * scale3_msg[type].oh * 3) / 2);   //申请到新的大空间
					fb->len = (scale3_msg[type].ow * scale3_msg[type].oh * 3) / 2;
					arg->ow = scale3_msg[type].ow;
					arg->oh = scale3_msg[type].oh;
				}
			}
			//这里要更改一下，看put到哪里去
			if(type == 0){
	            fbpool_put(&scale3->tx_pool0, fb);
			}else if(type == 1){
				fbpool_put(&scale3->tx_pool1, fb);
			}else if(type == 2){
				fbpool_put(&scale3->tx_pool2, fb);
			}else{
				fbpool_put(&scale3->tx_poolerr, fb);
			}
            // 不需要内核去释放fb
            ret = RET_OK + 1;
        }
        break;

        case MSI_CMD_SCALE3:
        {
            uint32_t cmd_self = (uint32_t)param1;
            //uint32_t arg = param2;
            // 自定义命令
            switch (cmd_self)
            {
                case MSI_SCALE3_START:
                {
                    //注意,这里需要根据vpp那边配置来决定用哪个
                    //注意line buf的数量
					uint8_t video_type = 0;
					uint8_t *p_buf;
					struct fbpool * tx_pool;
					struct framebuff *fb;

                    scale3->scaler3buf = SCALE3_LINE_BUF;
                    os_msgq_init(&scale3->msgq, MAX_SCALE3_TX);

                    OS_WORK_INIT(&scale3->work, scale3_stream_work, 0);
                    os_run_work_delay(&scale3->work, 1);

					if(video_msg.video_num == 1){
						video_type = ISP_VIDEO_0;
					}else if(video_msg.video_num == 2){
						video_type = video_msg.video_type_last;    //
					}else{
						if((video_msg.video_type_cur == ISP_VIDEO_1)||(video_msg.video_type_cur == ISP_VIDEO_2)){
							video_type = ISP_VIDEO_0;
						}else if(video_msg.video_type_last == ISP_VIDEO_1){
							video_type = ISP_VIDEO_2;
						}else if(video_msg.video_type_last == ISP_VIDEO_2){
							video_type = ISP_VIDEO_1;
						}
					} 


                    scale_set_start_addr(scale3->scale_dev, 0, 0);
                    // 暂时固定,如果遇到需要动态修改的,可以通过参数之类来切换
                    scale_set_dma_to_memory(scale3->scale_dev, 1);
                    scale_set_data_from_vpp(scale3->scale_dev, 1);
                    scale_set_line_buf_num(scale3->scale_dev, SCALE3_YUVBUF_Y_OFF_LINE);

				
			        scale3->ow = scale3_msg[video_type].ow;//ow;
			        scale3->oh = scale3_msg[video_type].oh;//oh;
			        scale3->iw = scale3_msg[video_type].iw;//iw;
			        scale3->ih = scale3_msg[video_type].ih;//ih;	
			        
					
                    // 暂时用同样的iw ih ow oh
                    scale_set_in_out_size(scale3->scale_dev, scale3->iw, scale3->ih, scale3->ow, scale3->oh);
                    scale_set_step(scale3->scale_dev, scale3->iw, scale3->ih, scale3->ow, scale3->oh);
                    scale_set_in_yaddr(scale3->scale_dev, (uint32)scale3->scaler3buf);
                    scale_set_in_uaddr(scale3->scale_dev, (uint32)scale3->scaler3buf + scale3->iw * SCALE3_YUVBUF_Y_OFF_LINE);
                    scale_set_in_vaddr(scale3->scale_dev, (uint32)scale3->scaler3buf + scale3->iw * SCALE3_YUVBUF_Y_OFF_LINE + scale3->iw * SCALE3_YUVBUF_Y_OFF_LINE/4);

                    // 这里分配一下scale3的空间,通过标准接口去分配吧,理论这里一定能获取到,这里就不判断异常情况了
                    
                    // 正常应该要获取到fb
					if(scale3->txpool_type == 1){
						fb = fbpool_get(&scale3->tx_pool0, 0, scale3->msi);
					}else{
						if(video_type == ISP_VIDEO_0){
							tx_pool = &scale3->tx_pool0;
						}else if(video_type == ISP_VIDEO_1){
							tx_pool = &scale3->tx_pool1;
						}else{
							tx_pool = &scale3->tx_pool2;
						}
						fb = fbpool_get(tx_pool, 0, scale3->msi);
					}
					arg = (struct scale3_yuv_arg_s*)fb->priv; 
					arg->yuv_arg.x = scale3_msg[video_type].x;
					arg->yuv_arg.y = scale3_msg[video_type].y;
					os_printf("arg w:%d  arg h:%d\r\n",arg->yuv_arg.out_w,arg->yuv_arg.out_h);

                    p_buf = fb->data;
                    scale3->now_fb = fb;

                    scale_set_out_yaddr(scale3->scale_dev, (uint32)p_buf);
                    scale_set_out_uaddr(scale3->scale_dev, (uint32)p_buf + scale3->ow * scale3->oh);
                    scale_set_out_vaddr(scale3->scale_dev, (uint32)p_buf + scale3->ow * scale3->oh + scale3->ow * scale3->oh / 4);
                    scale_request_irq(scale3->scale_dev, FRAME_END, scale3_stream_done, (uint32)scale3);
                    scale_request_irq(scale3->scale_dev, INBUF_OV, scale3_stream_ov, (uint32)scale3);
                    scale_open(scale3->scale_dev);
                    // 由于硬件需要vpp参与,所以这里会耦合了其他硬件模块
                    struct vpp_device *vpp_dev;
                    vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
                    vpp_open(vpp_dev);
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

extern scale3_kick_fn scale3_kick_func;
int32_t scaler3_kick_start(){
	struct msi *msi = msi_find(S_PREVIEW_SCALE3, 0);
	msi_do_cmd(msi, MSI_CMD_SCALE3, MSI_SCALE3_START, 0);
	msi_put(msi);
	scale3_kick_func = NULL;
	return 0;
}


// 参数分别是vpp的图像iw和ih,要scale的ow和oh,如果和屏有关,可以传入屏幕的宽高
struct msi *scale3_msi(const char *name)
{
    struct msi *msi = msi_new(name, 0, NULL);

	if(msi == NULL){
		return NULL;
	}

    struct scale3_msi_s *scale3 = (struct scale3_msi_s *)msi->priv;
	struct  scale_msg_t *scale_msg[3];
	uint8_t *fbuf;	
	
	scale_msg[0] = &scale3_msg[0];
	scale_msg[1] = &scale3_msg[1];
	scale_msg[2] = &scale3_msg[2];
	
    if (!scale3)
    {
        // os_printf("%s:%d new success\n", __FUNCTION__, __LINE__);
        scale3 = (struct scale3_msi_s *)STREAM_LIBC_ZALLOC(sizeof(struct scale3_msi_s));
        msi->action = scale3_msi_action;
        msi->priv = (void *)scale3;
        scale3->msi = msi;
        scale3->scale_dev = (struct scale_device *)dev_get(HG_SCALE3_DEVID);
		scale3->txpool_type = 0;
		
		int temp_video_num = video_msg.video_num;
		scale3->init_txpool_num = temp_video_num;

		for(int init_txpool_num = 0; init_txpool_num < temp_video_num ; init_txpool_num++) {
			int fbpool_init_size = 0;

			if (SCALE3_YUVBUF_0_1 == 0) 
			{
				extern uint8_t get_vpp_w_h(uint16_t *w, uint16_t *h);
				get_vpp_w_h(&scale_msg[init_txpool_num]->iw, &scale_msg[init_txpool_num]->ih);
			}
			else if (SCALE3_YUVBUF_0_1 == 1)
			{
				extern uint8_t get_vpp1_w_h(uint16_t *w, uint16_t *h);
				get_vpp1_w_h(&scale_msg[init_txpool_num]->iw, &scale_msg[init_txpool_num]->ih);
			}

			switch (init_txpool_num)
			{
				/* txpool0 */
				case 0:
				{
					fbpool_init_size = 2;
					if (temp_video_num == 1) {
						fbpool_init_size++;
						scale3->txpool_type = 1;
					}
				}
					break;

				/* txpool1 */
				case 1:
				{
					fbpool_init_size = 2;
					if(temp_video_num > 2)
						fbpool_init_size--;
				}
					break;

				/* txpool2 */
				case 2:
				{
					fbpool_init_size = 1;
				}
					break;

				default:
					break;
			}
			os_printf("scale3 init txpool num:%d  size:%d txpool0:0x%x txpool1:0x%x txpool2:0x%x\r\n",init_txpool_num,fbpool_init_size, &scale3->tx_pool0 + 0, &scale3->tx_pool0 + 1, &scale3->tx_pool0 + 2);
			fbpool_init(&scale3->tx_pool0 + init_txpool_num, fbpool_init_size);
			for(int i = 0; i < fbpool_init_size; i++) {
				struct scale3_yuv_arg_s *yuv_priv = (struct scale3_yuv_arg_s *)STREAM_LIBC_ZALLOC(sizeof(struct scale3_yuv_arg_s));
				struct yuv_arg_s *yuv = (struct yuv_arg_s *)yuv_priv;
				scale3->ow = scale_msg[init_txpool_num]->ow;
				scale3->oh = scale_msg[init_txpool_num]->oh;
				yuv->y_size = scale3->ow * scale3->oh;
		        yuv->uv_off = 0;
		        yuv->y_off = 0;	
				yuv->out_w = scale3->ow;
				yuv->out_h = scale3->oh;
				yuv->del = &scale3->del;

				yuv_priv->tx_type = init_txpool_num;
				yuv_priv->ow = scale3->ow;
				yuv_priv->oh = scale3->oh;
				
				fbuf = (uint8_t *)STREAM_MALLOC(scale3->ow * scale3->oh * 3 / 2);
				FBPOOL_SET_INFO(&scale3->tx_pool0 + init_txpool_num, i, fbuf, scale3->ow * scale3->oh * 3 / 2, (void *)yuv);
			}

		}

		fbpool_init(&scale3->tx_poolerr, 1);
		struct scale3_yuv_arg_s * yuv_priv = (struct scale3_yuv_arg_s *)STREAM_LIBC_ZALLOC(sizeof(struct scale3_yuv_arg_s));
		struct yuv_arg_s *yuv = (struct yuv_arg_s *)yuv_priv;
		yuv->y_size = 128 * 96;
        yuv->uv_off = 0;
        yuv->y_off = 0;	
		yuv->out_w = 128;
		yuv->out_h = 96;
		yuv->del = &scale3->del;
		scale3->ow = 128;
		scale3->oh = 96;
		yuv_priv->tx_type = 4;		
		fbuf = (uint8_t *)STREAM_MALLOC(128 * 96 * 3 / 2);
		FBPOOL_SET_INFO(&scale3->tx_poolerr, 0, fbuf, 128 * 96 * 3 / 2, (void *)yuv); 

        msi->enable = 1;
		
		scale3_kick_func = scaler3_kick_start;
		
    }
    os_printf("%s:%d\tmsi:%X\n", __FUNCTION__, __LINE__, msi);
    return msi;
}



static int32_t const_scale3_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    struct const_scale3_msi_s *scale3 = (struct const_scale3_msi_s *)msi->priv;
    switch (cmd_id)
    {

        // 这里msi已经被删除,那么就要考虑tx_pool的资源释放了
        // 能进来这里,就是代表所有fb都已经用完了
        case MSI_CMD_POST_DESTROY:
        {
            STREAM_LIBC_FREE(scale3);
        }
        break;

        // 停止硬件,移除没有必要的资源(但是fb的资源不能现在删除,这个时候fb可能外部还在调用)
        case MSI_CMD_PRE_DESTROY:
        {

        }
        break;


        default:
            break;
    }
    return ret;
}

static int32_t const_scale3_stream_done(uint32 irq_flag, uint32 irq_data, uint32 param1)
{
    _os_printf("CO");
    return 0;
}

void scale3_output_size_local_change(uint8_t id,uint16 x,uint16 y,uint16 w,uint16 h){
	uint32 ie;
	ie = disable_irq();
	scale3_msg[id].x = x;
	scale3_msg[id].y = y;
	scale3_msg[id].ow = w;
	scale3_msg[id].oh = h;
	enable_irq(ie);
}

//固定空间,由外部给空间,不再采用流的方式
//buf需要给够空间,根据ow和oh来计算
struct msi *scale3_msi_const_buf(const char *name, uint8_t *buf,uint16_t iw, uint16_t ih, uint16_t ow, uint16_t oh)
{
    uint8_t isnew;
    struct msi *msi = msi_new(name, 0, &isnew);
    if (isnew)
    {
        struct scale_device *scale_dev = (struct scale_device *)dev_get(HG_SCALE3_DEVID);
        os_printf("%s:%d new success\n", __FUNCTION__, __LINE__);
        struct const_scale3_msi_s *scale3 = (struct const_scale3_msi_s *)STREAM_LIBC_ZALLOC(sizeof(struct const_scale3_msi_s));
		scale3->scale_dev = scale_dev;
        msi->priv = (void *)scale3;
        msi->action = const_scale3_msi_action;
        msi->enable = 0;
		
        // 暂时用同样的iw ih ow oh
        scale_set_in_out_size(scale_dev, iw, ih, ow, oh);
        scale_set_step(scale_dev, iw, ih, ow, oh);
        scale_set_start_addr(scale_dev, 0, 0);
        // 暂时固定,如果遇到需要动态修改的,可以通过参数之类来切换
        scale_set_dma_to_memory(scale_dev, 1);
        scale_set_data_from_vpp(scale_dev, 1);
        scale_set_line_buf_num(scale_dev, 28);
        scale_set_in_yaddr(scale_dev, (uint32)yuvbuf);
        scale_set_in_uaddr(scale_dev, (uint32)yuvbuf + iw * 28);
        scale_set_in_vaddr(scale_dev, (uint32)yuvbuf + iw * 28 + iw * 28/4);



        scale_set_out_yaddr(scale_dev, (uint32)buf);
        scale_set_out_uaddr(scale_dev, (uint32)buf + ow * oh);
        scale_set_out_vaddr(scale_dev, (uint32)buf + ow * oh + ow * oh / 4);
        scale_request_irq(scale_dev, FRAME_END, const_scale3_stream_done, (uint32)scale3);
        scale_request_irq(scale_dev, INBUF_OV, scale3_stream_ov, (uint32)scale3);
        scale_open(scale_dev);
        // 由于硬件需要vpp参与,所以这里会耦合了其他硬件模块
        struct vpp_device *vpp_dev;
        vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
        vpp_open(vpp_dev);
        
    }
    os_printf("%s:%d\tmsi:%X\n", __FUNCTION__, __LINE__, msi);
    return msi;
}
