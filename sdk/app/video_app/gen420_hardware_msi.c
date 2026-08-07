#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "hal/h264.h"
#include "lib/video/gen/gen420_dev.h"
#include "dev/gen/hggen420.h"
#include "stream_define.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "gen420_hardware_msi.h"
#include "lib/video/dvp/jpeg/jpg.h"
// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE av_free
#define STREAM_LIBC_ZALLOC av_zalloc


#define GEN420_MAX_MSG (10)

#define MAX_GEN420_HARDWARE_RX 8

enum gen420_hardware_enum
{
    GEN420_HARDWARE_READY = BIT(0),      //将gen420的work停止
};

struct gen420_hardware_s
{
    struct msi *msi;
    struct gen420_device *gen420_dev;
    os_msgqueue_t msgq;
    struct os_event evt;
    struct os_task task;
    uint8_t *gen420_sram;
	uint8_t *gen420_sram2;
	uint8_t *gen420_sram3;
};

os_msgqueue_t *gen420_msgq = NULL;
volatile struct list_head gen420_queue_head;


static struct os_semaphore gen420wq_sem = {0,NULL};

void gen420wq_sema_init()
{
	os_sema_init(&gen420wq_sem,0);
}

void gen420wq_sema_down(int32 tmo_ms)
{
	os_sema_down(&gen420wq_sem,tmo_ms);
}

void gen420wq_sema_up()
{
	os_sema_up(&gen420wq_sem);
}


int32_t h264_gen420_kick()
{
	struct h264_device *h264_dev;
	h264_dev = (struct h264_device *)dev_get(HG_H264_DEVID);
	extern void h264_cfg_srcdat(struct h264_device *p_h264,uint8_t mode);	
	h264_cfg_srcdat(h264_dev,GEN420_DATA);
	return 0;
}




int32_t g_gen420_kick_msg(struct gen420_msg_s *msg,int32_t ms)
{
    if(!msg)
    {
        os_printf("%s:%d err,msg is NULL\n",__FUNCTION__,__LINE__);
        return 1;
    }
    return os_msgq_put(gen420_msgq,(uint32)msg,ms);
}


int register_gen420_queue(uint8_t type,uint32_t w,uint32_t h,gen420_kick_fn kick_fn,gen420_free_fn free_fn,uint32 priv){
	struct gen420_msg_s *wq;
	struct msi *msi = msi_find("gen420_h", 0);
	struct gen420_hardware_s *gen420 = (struct gen420_hardware_s *)msi->priv;
	msi_put(msi);
	if(gen420->gen420_sram == NULL){
		gen420->gen420_sram = (uint8_t *)STREAM_LIBC_MALLOC(8 * 1024);
		gen420->gen420_sram2 = (uint8_t *)STREAM_LIBC_MALLOC(2 * 1024);
		gen420->gen420_sram3 = (uint8_t *)STREAM_LIBC_MALLOC(2 * 1024);
		//任意一个空间申请不到,都算失败
		if(gen420->gen420_sram && gen420->gen420_sram2 && gen420->gen420_sram3){
			gen420_sram_linebuf_adr(gen420->gen420_dev, (uint32)gen420->gen420_sram, (uint32)(gen420->gen420_sram2), (uint32)(gen420->gen420_sram3));
		}
		else
		{
			os_printf(KERN_ERR"[%s] malloc failed!!!\n",__func__);
			if(gen420->gen420_sram)
			{
				STREAM_LIBC_FREE(gen420->gen420_sram);
				gen420->gen420_sram = NULL;
			}
			if(gen420->gen420_sram2)
			{
				STREAM_LIBC_FREE(gen420->gen420_sram2);
				gen420->gen420_sram2 = NULL;
			}
			if(gen420->gen420_sram3)
			{
				STREAM_LIBC_FREE(gen420->gen420_sram3);
				gen420->gen420_sram3 = NULL;
			}
			return -1;
		}
	}
	wq = malloc(sizeof(struct gen420_msg_s));
	wq->type = type;
	wq->kick_fn = kick_fn;
	wq->free_fn = free_fn;
	wq->w       = w;
	wq->h       = h;
	wq->sta     = 0;
	wq->devid  = type;
	wq->fn_data = (void *)priv;
	INIT_LIST_HEAD(&wq->list);
	list_add_tail(&wq->list,(struct list_head*)&gen420_queue_head); 
	
	return 0;
}


int unregister_gen420_queue(uint32_t devid){
	int ret = 0;
	struct list_head *dlist;	
	struct gen420_msg_s* gen420dev;
	struct msi *msi = msi_find("gen420_h", 0);
	struct gen420_hardware_s *gen420 = (struct gen420_hardware_s *)msi->priv;
	msi_put(msi);

	if(list_empty((struct list_head *)&gen420_queue_head) != TRUE){
		dlist = (struct list_head *)&gen420_queue_head;
		do{
			dlist = dlist->next;
			if(dlist == &gen420_queue_head){
				return -1;
			}else{
				gen420dev = list_entry((struct list_head *)dlist,struct gen420_msg_s,list);
				if(gen420dev->devid == devid){
					list_del(&gen420dev->list);
					free(gen420dev);

					if(list_empty((struct list_head *)&gen420_queue_head) == TRUE){
						if(gen420->gen420_sram)
						{
							STREAM_LIBC_FREE(gen420->gen420_sram);
							gen420->gen420_sram = NULL;
						}
						if(gen420->gen420_sram2)
						{
							STREAM_LIBC_FREE(gen420->gen420_sram2);
							gen420->gen420_sram2 = NULL;
						}
						if(gen420->gen420_sram3)
						{
							STREAM_LIBC_FREE(gen420->gen420_sram3);
							gen420->gen420_sram3 = NULL;
						}
					}
					return 1;
				}
			}
		}while(1);
	}else{
		ret = -1;
	}
	return ret;
}

int wake_up_gen420_queue(uint8_t devid,uint8_t *data_rom){
	int ret = 0;
	struct list_head *dlist;
	struct gen420_msg_s* gen420dev;
	if(list_empty((struct list_head *)&gen420_queue_head) != TRUE){
		dlist = (struct list_head *)&gen420_queue_head;
		do{
			dlist = dlist->next;
			if(dlist == &gen420_queue_head){
				return -1;
			}else{
				gen420dev = list_entry((struct list_head *)dlist,struct gen420_msg_s,list);
				if(gen420dev->devid == devid){
					if(gen420dev->sta == 1){
						return 2;          //this id all ready running
					}
					gen420dev->sta   = 1;
					gen420dev->data  = (uint8_t *)data_rom;
					gen420wq_sema_up();
					return 1;
				}
			}
		}while(1);
	}else{
		ret = -1;
	}
	return ret;
}

extern volatile uint8 done_percent;
void gen420_hardware_thread(void *d)
{
	uint32_t have_h264_jpg_stream = 0;
    int32_t res;
//    int32_t err;
    int32_t kick_res;
	struct list_head *dlist;
    struct msi *msi = (struct msi*)d;
    struct gen420_msg_s *msg = NULL;
    struct gen420_hardware_s *gen420 = (struct gen420_hardware_s *)msi->priv;
	struct gen420_msg_s *msgsub[GEN420_MAX_MSG];    //0: jpg for sub stream   1: h264 for sub stream    2-255: user    目前先限定最多3个用户同时使用   
//	int8_t msg_cnt = 0;
	uint8_t itk = 0;
	uint32_t flags;	
    while(1)
    {
rewait:
		gen420wq_sema_down(-1);

		for(itk = 0;itk < GEN420_MAX_MSG;itk++){
			msgsub[itk] = NULL;
		}
		
		flags = disable_irq();
		if(list_empty((struct list_head *)&gen420_queue_head) != TRUE){    //每次都会把链表从头查到尾
			dlist =(struct list_head *) &gen420_queue_head;
			do{
				dlist = dlist->next;
				if(dlist == &gen420_queue_head){
					break;
				}else{
					msg = list_entry((struct list_head *)dlist,struct gen420_msg_s,list);					
					if(msg->type <  2){
						have_h264_jpg_stream = 1;
					}
					
					if(msg->sta == 1){
						//memcpy(msgsub[msg->type],msg,sizeof(struct gen420_msg_s)); 
						msgsub[msg->type] = msg;
						//msg->sta = 0;
					}
				}
			}while(1);

		}else{
			enable_irq(flags);
			continue;
		}
		enable_irq(flags);
		
		for(itk = 0;itk < GEN420_MAX_MSG;itk++)
		{
			if(msgsub[itk] != NULL){


				//如果类型超过2,就是自定义的,那么要看看是否有副码流,如果有副码流,需要等待副码流去编码后再执行自定义的
				if(msgsub[itk]->type >= 2)
				{
					while(done_percent > 80){
						if(have_h264_jpg_stream)
						{
							//os_sleep_ms(1);
							//os_sleep_ms(1);
							goto rewait;
						}else{
							os_sleep_ms(1);
						}
							
					}
				}

				msg = msgsub[itk];

				if(msg->kick_fn){
					kick_res = msg->kick_fn(msg);								//配置对应的h264/jpg的数据源为gen420
					//没有kick成功,直接释放
					if(kick_res == 0)
					{
						gen420_frame_size(gen420->gen420_dev, msg->w, msg->h);
						gen420_psram_adr(gen420->gen420_dev, (uint32)msg->data, (uint32)(msg->data + msg->w * msg->h), (uint32)(msg->data + msg->w * msg->h + msg->w * msg->h / 4));
						gen420_dma_run(gen420->gen420_dev);
						res = os_event_wait(&gen420->evt, GEN420_HARDWARE_READY, NULL, OS_EVENT_WMODE_CLEAR, 1000);
						//有异常?正常不应该1s都没有完成
						if(res)
						{
							os_printf("%s:%d err\n",__FUNCTION__,__LINE__);
						}
					}
					else
					{
						os_printf(KERN_ERR"gen420_hardware kick error:%d\n",kick_res);
					}

				}
				msg->sta = 0;
				//如果副码流已经用完,就代表其他数据流可以使用了
				if(msg->type < 2)
				{
					have_h264_jpg_stream = 0;
				}


				if(msg && msg->free_fn)
				{
					msg->free_fn(msg);
				}
			}

		}
		have_h264_jpg_stream = 0;
    }
//gen420_hardware_thread_end:
    return;
}





static int32 gen420_msi_isr(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2)
{
    struct gen420_hardware_s *gen420 = (struct gen420_hardware_s *)irq_data;
    os_event_set(&gen420->evt, GEN420_HARDWARE_READY, NULL);
    return 0;
}


struct msi *gen420_hardware_msi_init()
{
    uint8_t is_new = 0;
    struct msi *msi = msi_new("gen420_h", 0, &is_new);
    if(is_new)
    {
        struct gen420_hardware_s *gen420 = (struct gen420_hardware_s *)msi->priv;
        gen420 = (struct gen420_hardware_s *)STREAM_LIBC_ZALLOC(sizeof(struct gen420_hardware_s));
        msi->priv = (void*)gen420;
        gen420->gen420_dev = (struct gen420_device *)dev_get(HG_GEN420_DEVID);
        os_msgq_init(&gen420->msgq,MAX_GEN420_HARDWARE_RX);
        os_event_init(&gen420->evt);

        gen420_open(gen420->gen420_dev);
        gen420_request_irq(gen420->gen420_dev, GEN420_DONE_ISR, gen420_msi_isr, (uint32)gen420);

        gen420_dst_h264_and_jpg(gen420->gen420_dev, 0);
		INIT_LIST_HEAD((struct list_head *)&gen420_queue_head);
		gen420wq_sema_init();

        msi->enable = 1;
        //创建线程
        gen420_msgq = &gen420->msgq;
        OS_TASK_INIT("gen420_s", &gen420->task, gen420_hardware_thread, (void*)msi, OS_TASK_PRIORITY_ABOVE_NORMAL+0xf, NULL, 2048);
    }
    return msi;
}
