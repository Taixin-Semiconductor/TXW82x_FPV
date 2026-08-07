#include "sys_config.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "typesdef.h"
#include "devid.h"
#include "osal/irq.h"
#include "osal/string.h"

#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/task.h"
#include "osal/work.h"
#include "osal/event.h"
#include "hal/usb_device.h"

#include "dev/usb/hgusb20_v1_dev_api.h"
#include "dev/usb/uvc_host.h"

#include "osal/semaphore.h"
#include "jpgdef.h"

#include "lib/lcd/lcd.h"

#include "lib/common/common.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

#include "stream_frame.h"
#include "utlist.h"

//兼容有psram和无psram的情况
#ifdef PSRAM_HEAP
#define UVC_MALLOC  av_psram_malloc
#define UVC_FREE    av_psram_free
#else
#define UVC_MALLOC  av_malloc
#define UVC_FREE    av_free
#endif

#if USB_EN
#define UVC_BLANK_LEN       16*1024
#ifdef PSRAM_HEAP
#define UVC_BLANK_NUM       8
#else
#define UVC_BLANK_NUM       8
#endif
#define UVC_FRAME_NUM       2

UVC_BLANK  uvc_blank[UVC_BLANK_NUM];
//uint8 blank_space[UVC_BLANK_NUM][UVC_BLANK_LEN+UVC_HEAD_RESERVER]__attribute__ ((aligned(4)));
uint8 *blank_space[UVC_BLANK_NUM];
struct list_head free_uvc_tab[UVC_HOST_NUM];  
struct list_head *uvc_p[UVC_HOST_NUM];
struct list_head *uvc_bnk[UVC_HOST_NUM];


UVC_MANAGE* uvc_msg[UVC_HOST_NUM];
UVC_MANAGE uvc_msg_frame[UVC_HOST_NUM][UVC_FRAME_NUM];                        //2个frame就够了，一个中断处正在收，一个应用正在使用,带宽不足时一个frame的不同blank进行收发处理

#define NEXT_TGL  ((uvc_msg[0]->uvc_head[1] & BIT(0)) ^ 1)
#define CURT_TGL  (uvc_msg[0]->uvc_head[1] & BIT(0))

#define NEXT_TGL2  ((uvc_msg[1]->uvc_head[1] & BIT(0)) ^ 1)
#define CURT_TGL2  (uvc_msg[1]->uvc_head[1] & BIT(0))

uint32 frame_num[UVC_HOST_NUM];
volatile uint32 rx_packet_len;
volatile uint32 rx_packet_len_2;
volatile uint32 head_len;

int get_node_uvc_len()
{
    return UVC_BLANK_LEN;
}


void *get_usb_jpeg_node_buf(void *d)
{
    UVC_BLANK* uvc_b = (UVC_BLANK*)d;
    return uvc_b->buf_ptr;
}

/**@brief 
 * 将传入的frame根节点下的buf池都送回空间池（free_uvc_tab）中
 */
bool free_get_uvc_node_list(struct list_head *head,struct list_head *free){
    if(list_empty(head)){
        return 0;
    }   

    list_splice_init(head,free);
    return 1;
}


void del_usb_frame(void *get_f)
{
    free_get_uvc_node_list(get_f,&free_uvc_tab[0]); 
}

void del_usb_frame2(void *get_f)
{
    free_get_uvc_node_list(get_f,&free_uvc_tab[1]); 
}


int get_usb_node_count(struct list_head *head)
{
	int count = 0;
	struct list_head *first = (struct list_head *)head;
	while(head->next != first)
	{
		head = head->next;
		count++;
	}

	return count;				//数量
}


/**@brief 
 * 从初始化的frame根节点中抽取其中一个空闲的根节点进行使用，并标记frame的使用状态为正在使用
 * @param 是否开启抢占模式，如果开启，则将上一帧已完成的帧节点删掉，并返回
 */


struct list_head* get_new_uvc_frame_head(int grab){
    uint8 frame_num = 0;
    for(frame_num = 0;frame_num < UVC_FRAME_NUM;frame_num++){
        if(uvc_msg_frame[0][frame_num].state == 0){
            uvc_msg_frame[0][frame_num].state = 1;
            uvc_msg_frame[0][frame_num].frame_end  = 0;
            return &uvc_msg_frame[0][frame_num].list;
        }
    }

    if(grab)                                //是否开启抢占模式,开启抢占后，肯定有frame返回，上一帧没使用的frame肯定usable为2
    {   
        for(frame_num = 0;frame_num < UVC_FRAME_NUM;frame_num++){
            if(uvc_msg_frame[0][frame_num].state == 2){                                
                uvc_msg_frame[0][frame_num].state = 1;                 
                uvc_msg_frame[0][frame_num].frame_end  = 0;
                free_get_uvc_node_list(&uvc_msg_frame[0][frame_num].list,&free_uvc_tab[0]);   
                return &uvc_msg_frame[0][frame_num].list;
            }
        }       
    }
    
    return 0;
}

void set_uvc_frame_ready(UVC_MANAGE* uvc){
    if(uvc->state != 3)                 //表示此frame正在被应用使用，可以使用的状态不用切了
        uvc->state = 2;
}

void set_uvc_frame_using(UVC_MANAGE* uvc){
    uvc->state = 3;
}

void del_uvc_frame(UVC_MANAGE* uvc)
{
    uvc->state = 0;
    uvc->frame_len = 0;
    uvc->frame_end = 0;
}

/*reset所有的frame，包括正在填充的和正在使用的，在填充和获取的线程或中断都关闭后调用*/
void force_reset_uvc_frame()
{
    uint8 frame_num = 0;   
    for(frame_num = 0;frame_num < UVC_FRAME_NUM;frame_num++){
        del_usb_frame(&uvc_msg_frame[0][frame_num].list);
        del_uvc_frame(&uvc_msg_frame[0][frame_num]);
    }
}

/**@brief 
 * 从空间池中提取一个节点，放到队列中，并将此节点作为返回值返回
 */
struct list_head *get_blank_node(struct list_head *head,struct list_head *del){
    if(list_empty(del)){
        return 0;
    }

    list_move(del->prev,head);
    return head->next;              //返回最新的位置
}

bool put_blank_node(struct list_head *head,struct list_head *del){
    irq_disable(USB20DMA_IRQn);
        
    if(list_empty(del)){
        irq_enable(USB20DMA_IRQn);
        return 0;
    }
    list_move(del->prev,head);
    irq_enable(USB20DMA_IRQn);

    return 1;
}

//新增加,将节点归还到节点池
bool free_usb_node(struct list_head *del){
    irq_disable(USB20DMA_IRQn);
    list_move(del,&free_uvc_tab[0]);
    irq_enable(USB20DMA_IRQn);

    return 1;
}

//新增加,将节点归还到节点池
bool free_usb_node2(struct list_head *del){
    irq_disable(USB20DMA_IRQn);
    list_move(del,&free_uvc_tab[1]);
    irq_enable(USB20DMA_IRQn);

    return 1;
}


bool free_node2tab(UVC_BLANK* ubk,struct list_head *head,struct list_head *del)
{
    put_blank_node(head,del);
    return 0;
}


struct list_head* get_uvc_frame()
{
    uint8 frame_num = 0;
    irq_disable(USB20DMA_IRQn);
    for(frame_num = 0;frame_num < UVC_FRAME_NUM;frame_num++){
        if(uvc_msg_frame[0][frame_num].state == 2)             //将当前使用frame或者完成frame返回出去，加以使用
        {
            uvc_msg_frame[0][frame_num].state = 1;
            irq_enable(USB20DMA_IRQn);
            return &uvc_msg_frame[0][frame_num].list;
        }
    }
#ifdef PSRAM_HEAP
    for(frame_num = 0;frame_num < UVC_FRAME_NUM;frame_num++){
        if(uvc_msg_frame[0][frame_num].state == 1)             //将当前使用frame或者完成frame返回出去，加以使用
        {
            uvc_msg_frame[0][frame_num].state = 1;
            irq_enable(USB20DMA_IRQn);
            return &uvc_msg_frame[0][frame_num].list;
        }
    }
#endif  
    irq_enable(USB20DMA_IRQn);

    return 0;   
}

struct list_head* get_uvc_frame2()
{
    uint8 frame_num = 0;
    irq_disable(USB20DMA_IRQn);
    for(frame_num = 0;frame_num < UVC_FRAME_NUM;frame_num++){
        if(uvc_msg_frame[1][frame_num].state == 2)             //将当前使用frame或者完成frame返回出去，加以使用
        {
            uvc_msg_frame[1][frame_num].state = 1;
            irq_enable(USB20DMA_IRQn);
            return &uvc_msg_frame[1][frame_num].list;
        }
    }
#ifdef PSRAM_HEAP
    for(frame_num = 0;frame_num < UVC_FRAME_NUM;frame_num++){
        if(uvc_msg_frame[1][frame_num].state == 1)             //将当前使用frame或者完成frame返回出去，加以使用
        {
            uvc_msg_frame[1][frame_num].state = 1;
            irq_enable(USB20DMA_IRQn);
            return &uvc_msg_frame[1][frame_num].list;
        }
    }
#endif  
    irq_enable(USB20DMA_IRQn);

    return 0;   
}

void uvc_blank_init(uint8* buf, uint32 blank_len, uint8 blank_num)
{
    uvc_blank[blank_num].blank_len     = 0;
    uvc_blank[blank_num].re_space      = blank_len;
    uvc_blank[blank_num].blank_loop    = 0;
    uvc_blank[blank_num].buf_ptr       = buf;//buf+blank_num*(blank_len+UVC_HEAD_RESERVER)+UVC_HEAD_RESERVER;
    uvc_blank[blank_num].busy          = 0;
    uvc_blank[blank_num].frame_counter = 0;
}

void uvc_blank_list_init(struct list_head *ftb,uint8 blank_num)
{
    uint8 bcnt = 0;
    INIT_LIST_HEAD(ftb);    
    for(bcnt = 0;bcnt < blank_num;bcnt++){
        INIT_LIST_HEAD(&uvc_blank[bcnt].list);
        list_add_tail(&uvc_blank[bcnt].list,ftb); 
    }   
}



void uvc_blank_list_init2(struct list_head *ftb,uint8 blank_num)
{
    uint8 bcnt = 0;
    INIT_LIST_HEAD(ftb);    
    for(bcnt = 0;bcnt < blank_num;bcnt++){
        os_printf("uvc blank list init2:%d\r\n",bcnt+UVC_BLANK_NUM/2);
        INIT_LIST_HEAD(&uvc_blank[bcnt+UVC_BLANK_NUM/2].list);
        list_add_tail(&uvc_blank[bcnt+UVC_BLANK_NUM/2].list,ftb); 
    }   
}

void uvc_room_del(){
	uint8 bcnt = 0;
	for(bcnt = 0;bcnt < UVC_BLANK_NUM;bcnt++){
		if(blank_space[bcnt]){
			os_printf("blank del[%d]\r\n",bcnt);
			UVC_FREE(blank_space[bcnt]);
			blank_space[bcnt] = NULL;
		}
	}
}

void uvc_room_del_mjpeg(){
  	uint8 bcnt = 0;
	for(bcnt = 0;bcnt < UVC_BLANK_NUM/2;bcnt++){
		if(blank_space[bcnt]){
			os_printf("h264 blank del[%d]\r\n",bcnt);
			UVC_FREE(blank_space[bcnt]);
			blank_space[bcnt] = NULL;
		}
	}  
}

void uvc_room_del_h264(){
  	uint8 bcnt = 0;
	for(bcnt = UVC_BLANK_NUM/2;bcnt < UVC_BLANK_NUM;bcnt++){
		if(blank_space[bcnt]){
			os_printf("mjpeg blank del[%d]\r\n",bcnt);
			UVC_FREE(blank_space[bcnt]);
			blank_space[bcnt] = NULL;
		}
	}
}

uint8_t *check_uvc_room()
{
    return blank_space[0];
}

uint8_t *check_uvc_room2()
{
    return blank_space[UVC_BLANK_NUM/2];
}

void uvc_room_malloc(){
	uint8 bcnt = 0;
	for(bcnt = 0;bcnt < UVC_BLANK_NUM;bcnt++){
		if(blank_space[bcnt] == NULL){
			blank_space[bcnt] = UVC_MALLOC(UVC_BLANK_LEN);
			if(blank_space[bcnt] == NULL){
				os_printf("uvc malloc fail\r\n");
				return;
			}
			os_printf("blank_space[%d]:%x\r\n",bcnt,(unsigned int)blank_space[bcnt]);
		}

		uvc_blank_init(blank_space[bcnt],UVC_BLANK_LEN,bcnt);
	}
}

void uvc_room_malloc_mjpeg(){
	uint8 bcnt = 0;
	for(bcnt = 0;bcnt < UVC_BLANK_NUM/2;bcnt++){
		if(blank_space[bcnt] == NULL){
			blank_space[bcnt] = UVC_MALLOC(UVC_BLANK_LEN);
			if(blank_space[bcnt] == NULL){
				os_printf("uvc malloc fail\r\n");
				return;
			}
			os_printf("blank_space[%d]:%x\r\n",bcnt,(unsigned int)blank_space[bcnt]);
		}

		uvc_blank_init(blank_space[bcnt],UVC_BLANK_LEN,bcnt);
	}    
}

void uvc_room_malloc_h264(){
	uint8 bcnt = 0;
	for(bcnt = UVC_BLANK_NUM/2;bcnt < UVC_BLANK_NUM;bcnt++){
		if(blank_space[bcnt] == NULL){
			blank_space[bcnt] = UVC_MALLOC(UVC_BLANK_LEN);
			if(blank_space[bcnt] == NULL){
				os_printf("uvc malloc fail\r\n");
				return;
			}
			os_printf("blank_space[%d]:%x\r\n",bcnt,(unsigned int)blank_space[bcnt]);
		}

		uvc_blank_init(blank_space[bcnt],UVC_BLANK_LEN,bcnt);
	}    
}


void uvc_reset_dev(uint8_t en){
	if(en){

		os_sleep_ms(5);
		uvc_room_malloc();
		_os_printf("room set");
	}else{
        uvc_msg_frame[0][0].frame_end = 2;
        uvc_msg_frame[0][1].frame_end = 2;

        uvc_msg_frame[1][0].frame_end = 2;
        uvc_msg_frame[1][1].frame_end = 2;

		os_sleep_ms(5);
		uvc_room_del();
		_os_printf("room del");
	}
}

void uvc_reset_dev_mjpeg(uint8_t en){
    if(en){
        os_sleep_ms(5);
        uvc_room_malloc_mjpeg();
        _os_printf("room mjpeg set");

    }else{
        uvc_msg_frame[0][0].frame_end = 2;
        uvc_msg_frame[0][1].frame_end = 2;

        uvc_room_del_mjpeg();
        _os_printf("room mjpeg del");
    }
}

void uvc_reset_dev_h264(uint8_t en){
    if(en){
        os_sleep_ms(5);
        uvc_room_malloc_h264();
        _os_printf("room h264 set");

    }else{
        uvc_msg_frame[1][0].frame_end = 2;
        uvc_msg_frame[1][1].frame_end = 2;

        uvc_room_del_h264();
        _os_printf("room h264 del");
    }
}

extern void uvc_stream_open(struct hgusb20_dev *p_dev,uint8 enable);
struct list_head* get_new_uvc_frame_head2(int grab);

//初始化双数据流的节点空间
void uvc_room_init(void *p_dev)
{
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);

    //如果已经申请了空间,则不需要管,如果没有申请空间,则关闭
    if(!check_uvc_room())
    {
        uvc_reset_dev(1);
    }

    uvc_blank_list_init(&free_uvc_tab[0],UVC_BLANK_NUM/2);
    INIT_LIST_HEAD(&uvc_msg_frame[0][0].list);
    INIT_LIST_HEAD(&uvc_msg_frame[0][1].list);
    uvc_msg_frame[0][0].state = 0;
    uvc_msg_frame[0][1].state = 0;
    uvc_msg_frame[0][0].sta = 0;
    uvc_msg_frame[0][1].sta = 0;

    uvc_p[0]   = get_new_uvc_frame_head(1);
    uvc_msg[0] = list_entry(uvc_p[0],UVC_MANAGE,list);//&uvc_msg_frame[0];
    

    uvc_blank_list_init2(&free_uvc_tab[1],UVC_BLANK_NUM/2);
    INIT_LIST_HEAD(&uvc_msg_frame[1][0].list);
    INIT_LIST_HEAD(&uvc_msg_frame[1][1].list);
    uvc_msg_frame[1][0].state = 0;
    uvc_msg_frame[1][1].state = 0;
    uvc_msg_frame[1][0].sta = 0;
    uvc_msg_frame[1][1].sta = 0;


    uvc_p[1]   = get_new_uvc_frame_head2(1);
    uvc_msg[1] = list_entry(uvc_p[1],UVC_MANAGE,list);//&uvc_msg_frame[0];


}

//初始化mjpeg的节点空间
void uvc_room_init_mjpeg()
{
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    
    //如果已经申请了空间,则不需要管,如果没有申请空间,则关闭
    if(!check_uvc_room())
    {
        uvc_reset_dev_mjpeg(1);
    }
    
    uvc_blank_list_init(&free_uvc_tab[0],UVC_BLANK_NUM/2);
    INIT_LIST_HEAD(&uvc_msg_frame[0][0].list);
    INIT_LIST_HEAD(&uvc_msg_frame[0][1].list);
    uvc_msg_frame[0][0].state = 0;
    uvc_msg_frame[0][1].state = 0;
    uvc_msg_frame[0][0].sta = 0;
    uvc_msg_frame[0][1].sta = 0;


    uvc_p[0]   = get_new_uvc_frame_head(1);
    uvc_msg[0] = list_entry(uvc_p[0],UVC_MANAGE,list);//&uvc_msg_frame[0];
}

void uvc_room_deinit_mjpeg()
{
    uvc_reset_dev_mjpeg(0);
}

//初始化h264的节点空间
void uvc_room_init_h264()
{
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    
    //如果已经申请了空间,则不需要管,如果没有申请空间,则关闭
    if(!check_uvc_room2())
    {
        uvc_reset_dev_h264(1);
    }


    uvc_blank_list_init2(&free_uvc_tab[1],UVC_BLANK_NUM/2);
    INIT_LIST_HEAD(&uvc_msg_frame[1][0].list);
    INIT_LIST_HEAD(&uvc_msg_frame[1][1].list);
    uvc_msg_frame[1][0].state = 0;
    uvc_msg_frame[1][1].state = 0;
    uvc_msg_frame[1][0].sta = 0;
    uvc_msg_frame[1][1].sta = 0;


    uvc_p[1]   = get_new_uvc_frame_head2(1);
    uvc_msg[1] = list_entry(uvc_p[1],UVC_MANAGE,list);//&uvc_msg_frame[0];
}

void uvc_room_deinit_h264(void)
{
    uvc_reset_dev_h264(0);
}

static uint8 uvc_irq_redy(UVC_MANAGE *uvc)
{   
    UVC_BLANK *blank;
    uvc->tgl        = NEXT_TGL;
    uvc->sta        = 1;
    uvc->frame_end  = 0;
    uvc_bnk[0] = get_blank_node(&uvc->list,&free_uvc_tab[0]);
    if(uvc_bnk[0] == NULL){
        uvc->tgl        = NEXT_TGL;
        uvc->sta        = 0;        
    }
    else
    {
        blank = list_entry(uvc_bnk[0],UVC_BLANK,list);
        blank->blank_loop = 0;
        blank->re_space = UVC_BLANK_LEN;
        blank->busy = 0;
        blank->blank_len = 0;
    }
    return 0;

}


static uint8 uvc_irq_wait_next_frame(UVC_MANAGE *uvc)
{
    if(uvc->tgl == CURT_TGL)
    {
        uvc->sta = 2;
    }
    return 0;
}

/****************************************************
0:立刻进行kcik(包含一些错误)
1:切换bank,不能立刻kick
******************************************************/


static uint8 uvc_data_handle(UVC_MANAGE *uvc,struct hgusb20_dev *p_dev,uint8_t* rx_buff,uint8 drop)
{   
    //static unsigned int uvc_time = 0;
    //static unsigned int in_times = 0;
    UVC_BLANK *blank;
    uint32 data_len;
    uint32 re_space;
    uint32 tgl_mark;
    uint8 blank_loop = 0;
    uint8 ret = 0;
    uint8 end_frame = 0;
    data_len = rx_packet_len-head_len;
    static uint8 last_frame_end = 0;
    if(drop == 2){
		//printf("_E_");
		end_frame = 1;
		drop = 0;
    }
    if(drop)                                                //丢帧处理，帧异常的时候需要处理掉当前帧
    {
        _os_printf("_D3_");
        if(uvc->state != 3){
            uvc->state = 0;
            free_get_uvc_node_list(&uvc->list,&free_uvc_tab[0]);
        }       
        
        uvc_p[0]   = get_new_uvc_frame_head(1);                  //获取新的frame,然后检测下这个frame是不是已有blank，有的话，要先清
        uvc_msg[0] = list_entry(uvc_p[0],UVC_MANAGE,list);//&uvc_msg_frame[0];
        uvc_msg[0]->sta = 0;
        if(uvc_msg[0] == uvc){
            free_get_uvc_node_list(&uvc->list,&free_uvc_tab[0]); 
            del_uvc_frame(uvc);
            uvc_msg[0]->state = 1;
        }else{
            uvc->frame_end = 2;
            uvc->sta = 0;   
        }

        return 0;
    }
    //in_times++;
    if((uvc->tgl != CURT_TGL) || (last_frame_end==1)){
        last_frame_end = 0;
        uvc->sta = 2;
        uvc->tgl = CURT_TGL;
        tgl_mark = CURT_TGL;
#ifdef PSRAM_HEAP   
        set_uvc_frame_ready(uvc);                             //上一次frame已完成
        //_os_printf("_1_");
        blank = list_entry(uvc_bnk[0],UVC_BLANK,list);
        blank->busy = 2;
        //printf("t1:%d\n",in_times);
        uvc->frame_end = 1;

// 无psram
#else   
        if(uvc->state == 1){                                  //防漏
            set_uvc_frame_ready(uvc);
            blank = list_entry(uvc_bnk[0],UVC_BLANK,list);
            blank->busy = 2;
            uvc->frame_end = 1;         
        }
#endif
        // uvc_time = csi_kernel_get_ticks();
        
        
        uvc_p[0]   = get_new_uvc_frame_head(1);                  //获取新的frame,然后检测下这个frame是不是已有blank，有的话，要先清
        uvc_msg[0] = list_entry(uvc_p[0],UVC_MANAGE,list);//&uvc_msg_frame[0];
        uvc_msg[0]->frame_counter = frame_num[0];
        uvc_msg[0]->sta = 2;
        uvc_msg[0]->tgl = tgl_mark;
        uvc_msg[0]->frame_len = 0;
        uvc_bnk[0] = get_blank_node(&uvc_msg[0]->list,&free_uvc_tab[0]);   //提取新的可用blank，旧的blank虽然还有可用空间，但换帧后也直接抛出
//      printf("GB");
        uvc = uvc_msg[0];
        if(uvc_bnk[0] != NULL)
        {
            blank = list_entry(uvc_bnk[0],UVC_BLANK,list);
            blank->blank_len = 0;
            blank->re_space        = UVC_BLANK_LEN;
            blank->busy = 0;
            blank->blank_loop = blank_loop;
            //切换洗一个bank,要延时kick
            ret = 1;
            //printf("nblank:%x %x\r\n",blank,blank_loop);
        }else{
            _os_printf("_D2_");
            if(uvc->state != 3){
                uvc->state = 0;
                free_get_uvc_node_list(&uvc->list,&free_uvc_tab[0]);
            }

            uvc_p[0]   = get_new_uvc_frame_head(1);                  //获取新的frame,然后检测下这个frame是不是已有blank，有的话，要先清
            uvc_msg[0] = list_entry(uvc_p[0],UVC_MANAGE,list);//&uvc_msg_frame[0];
            uvc_msg[0]->sta = 0;   
            if(uvc_msg[0] == uvc){
                free_get_uvc_node_list(&uvc->list,&free_uvc_tab[0]); 
                del_uvc_frame(uvc);
                uvc_msg[0]->state = 1;
            }else{
                uvc->frame_end = 2;
                uvc->sta = 0;   
            }
            return 0;
        }
        frame_num[0]++;
    }
    //当前frame的数据处理
    //1:通过当前frame找到当前blank的结构体
    //printf("uvc_bnk:%x\r\n",uvc_bnk);
    blank = list_entry(uvc_bnk[0],UVC_BLANK,list);
    blank->busy = 1;
    if(blank->re_space < data_len){                                                     //当前blank剩余空间不足
        //printf("a2:%x %d %d\r\n",blank->buf_ptr+blank->blank_len,blank->blank_len,blank->re_space);
        hw_memcpy(blank->buf_ptr+blank->blank_len,&rx_buff[head_len],blank->re_space);
        if((rx_buff[rx_packet_len-2] == 0xFF)&&(rx_buff[rx_packet_len-1] == 0xD9)) {
            last_frame_end = 1;
        }
        //printf("buf:%x\r\n",blank->buf_ptr+blank->blank_len);
        re_space = blank->re_space;
        blank->blank_len +=blank->re_space;
        blank->re_space = 0;
        blank->busy = 2;    
        //printf("t2:%d\n",in_times);
        //uvc_time = csi_kernel_get_ticks();

        blank_loop = blank->blank_loop;

        if(uvc == NULL)
            _os_printf("uvc hehe\r\n");
        
        uvc_bnk[0] = get_blank_node(&uvc->list,&free_uvc_tab[0]);                                    //重新获取一个新的blank，将剩余数据回填进去        
//      _os_printf("GB");
        if(uvc_bnk[0] != 0)
        {
            blank = list_entry(uvc_bnk[0],UVC_BLANK,list);
            blank->blank_len = 0;
            blank->re_space        = UVC_BLANK_LEN;
            blank->busy = 0;
            blank_loop++;
            blank->blank_loop = blank_loop;

            ret = 1;
            //_os_printf("blank:%x %x\r\n",blank,blank_loop);
        }else{
            _os_printf("_D1_");                             //一个frame就将所有的内存都占光了，没有新的内存可以存放此帧图像了
            if(uvc->state != 3){
                uvc->state = 0;             
                free_get_uvc_node_list(&uvc->list,&free_uvc_tab[0]);
            }

            
            uvc_p[0]   = get_new_uvc_frame_head(1);                  //获取新的frame,然后检测下这个frame是不是已有blank，有的话，要先清
            uvc_msg[0] = list_entry(uvc_p[0],UVC_MANAGE,list);//&uvc_msg_frame[0];
            uvc_msg[0]->sta = 0;
            if(uvc == NULL)
                _os_printf("hehe\r\n");
            
            if(uvc_msg[0] == uvc){
                free_get_uvc_node_list(&uvc->list,&free_uvc_tab[0]); 
                del_uvc_frame(uvc);
                uvc_msg[0]->state = 1;
            }else{
                uvc->frame_end = 2;
                uvc->sta = 0;   
            }           


            return 0;
        }

        hw_memcpy(blank->buf_ptr+blank->blank_len,&rx_buff[re_space+head_len],data_len - re_space);   
        if((rx_buff[rx_packet_len-2] == 0xFF)&&(rx_buff[rx_packet_len-1] == 0xD9)) {
            last_frame_end = 1;
        }
        blank->re_space  -=(data_len - re_space);
        blank->blank_len +=(data_len - re_space);
        blank->busy = 1; 
        uvc->frame_len   += data_len;
    }else{                                                                              //当前blank剩余空间足够     
        //printf("a1:%x %d %d\r\n",blank->buf_ptr+blank->blank_len,blank->blank_len,data_len);
        hw_memcpy(blank->buf_ptr+blank->blank_len,&rx_buff[head_len],data_len);      
        if((rx_buff[rx_packet_len-2] == 0xFF)&&(rx_buff[rx_packet_len-1] == 0xD9)) {
            last_frame_end = 1;
        }
        uvc->frame_len += data_len;
        blank->re_space -=data_len;
        blank->blank_len +=data_len;
        //printf(".");  
    }
	
    if(end_frame){
        set_uvc_frame_ready(uvc);
        blank->busy = 2;
        uvc->frame_end = 1;
    }
    return ret;
}



bool uvc_data_deal(struct hgusb20_dev *p_dev)
{   
    uint32 hgusb20_ep_get_dma_rx_len(struct hgusb20_dev *p_dev, uint8 ep);

    uint32 rx_len = hgusb20_ep_get_dma_rx_len(p_dev, UVC_EP);
	
    if(rx_len == 0){
        return 1;
    }
    rx_packet_len = rx_len;
	//printf("rx_len:%d   ",rx_packet_len);
    return 0;
}


struct list_head* get_new_uvc_frame_head2(int grab){
    uint8 frame_num = 0;
    for(frame_num = 0;frame_num < UVC_FRAME_NUM;frame_num++){
        if(uvc_msg_frame[1][frame_num].state == 0){
            uvc_msg_frame[1][frame_num].state = 1;
            uvc_msg_frame[1][frame_num].frame_end  = 0;
            return &uvc_msg_frame[1][frame_num].list;
        }
    }

    if(grab)                                //是否开启抢占模式,开启抢占后，肯定有frame返回，上一帧没使用的frame肯定usable为2
    {   
        for(frame_num = 0;frame_num < UVC_FRAME_NUM;frame_num++){
            if(uvc_msg_frame[1][frame_num].state == 2){                                
                uvc_msg_frame[1][frame_num].state = 1;                 
                uvc_msg_frame[1][frame_num].frame_end  = 0;
                free_get_uvc_node_list(&uvc_msg_frame[1][frame_num].list,&free_uvc_tab[1]);   
                return &uvc_msg_frame[1][frame_num].list;
            }
        }       
    }
    
    return 0;
}

bool uvc_data_deal2(struct hgusb20_dev *p_dev)
{   
    uint32 hgusb20_ep_get_dma_rx_len(struct hgusb20_dev *p_dev, uint8 ep);

    uint32 rx_len = hgusb20_ep_get_dma_rx_len(p_dev, UVC2_EP);
	
    if(rx_len == 0){
        return 1;
    }
    rx_packet_len_2 = rx_len;
	//printf("rx_len:%d   ",rx_packet_len_2);
    return 0;
}

static uint8 uvc_irq_redy2(UVC_MANAGE *uvc)
{   
    UVC_BLANK *blank;
    uvc->tgl        = NEXT_TGL2;
    uvc->sta        = 1;
    uvc->frame_end  = 0;
    uvc_bnk[1] = get_blank_node(&uvc->list,&free_uvc_tab[1]);
    if(uvc_bnk[1] == NULL){
        uvc->tgl        = NEXT_TGL2;
        uvc->sta        = 0;        
    }
    else
    {
        blank = list_entry(uvc_bnk[1],UVC_BLANK,list);
        blank->blank_loop = 0;
        blank->re_space = UVC_BLANK_LEN;
        blank->busy = 0;
        blank->blank_len = 0;
    }
    return 0;
}

static uint8 uvc_irq_wait_next_frame2(UVC_MANAGE *uvc)
{
    if(uvc->tgl == CURT_TGL2)
    {
        uvc->sta = 2;
    }
    return 0;
}

static uint8 uvc_data_handle2(UVC_MANAGE *uvc,struct hgusb20_dev *p_dev,uint8_t* rx_buff,uint8 drop)
{   
    //static unsigned int uvc_time = 0;
    static unsigned int in_times = 0;
    UVC_BLANK *blank;
    uint32 data_len;
    uint32 re_space;
    uint32 tgl_mark;
    uint8 blank_loop = 0;
    uint8 ret = 0;
    uint8 end_frame = 0;
    data_len = rx_packet_len_2-head_len;
    static uint8 last_frame_end = 0;
    if(drop == 2){
		//printf("_E_");
		end_frame = 1;
		drop = 0;
    }
    if(drop)                                                //丢帧处理，帧异常的时候需要处理掉当前帧
    {
        _os_printf("_D3_2");

        if(uvc->state != 3){
            uvc->state = 0;
            free_get_uvc_node_list(&uvc->list,&free_uvc_tab[1]);
        }       
        
        uvc_p[1]   = get_new_uvc_frame_head2(1);                  //获取新的frame,然后检测下这个frame是不是已有blank，有的话，要先清
        uvc_msg[1] = list_entry(uvc_p[1],UVC_MANAGE,list);//&uvc_msg_frame[0];
        uvc_msg[1]->sta = 0;
        if(uvc_msg[1] == uvc){
            free_get_uvc_node_list(&uvc->list,&free_uvc_tab[1]); 
            del_uvc_frame(uvc);
            uvc_msg[1]->state = 1;
        }else{
            uvc->frame_end = 2;
            uvc->sta = 0;   
        }

        return 0;
    }
    in_times++;
    if((uvc->tgl != CURT_TGL2) || (last_frame_end==1)){
        last_frame_end = 0;
        uvc->sta = 2;
        uvc->tgl = CURT_TGL2;
        tgl_mark = CURT_TGL2;
#ifdef PSRAM_HEAP   
        set_uvc_frame_ready(uvc);                             //上一次frame已完成
        //_os_printf("_O_");
        blank = list_entry(uvc_bnk[1],UVC_BLANK,list);
        blank->busy = 2;
        //printf("t1:%d\n",in_times);
        uvc->frame_end = 1;

// 无psram
#else   
        if(uvc->state == 1){                                  //防漏
            set_uvc_frame_ready(uvc);
            blank = list_entry(uvc_bnk[1],UVC_BLANK,list);
            blank->busy = 2;
            uvc->frame_end = 1;         
        }
#endif
        //uvc_time = os_jiffies();
        
        
        uvc_p[1]   = get_new_uvc_frame_head2(1);                  //获取新的frame,然后检测下这个frame是不是已有blank，有的话，要先清
        uvc_msg[1] = list_entry(uvc_p[1],UVC_MANAGE,list);//&uvc_msg_frame[0];
        uvc_msg[1]->frame_counter = frame_num[1];
        uvc_msg[1]->sta = 2;
        uvc_msg[1]->tgl = tgl_mark;
        uvc_msg[1]->frame_len = 0;
        uvc_bnk[1] = get_blank_node(&uvc_msg[1]->list,&free_uvc_tab[1]);   //提取新的可用blank，旧的blank虽然还有可用空间，但换帧后也直接抛出
//      printf("GB");
        uvc = uvc_msg[1];
        if(uvc_bnk[1] != NULL)
        {
            blank = list_entry(uvc_bnk[1],UVC_BLANK,list);
            blank->blank_len = 0;
            blank->re_space        = UVC_BLANK_LEN;
            blank->busy = 0;
            blank->blank_loop = blank_loop;
            //切换洗一个bank,要延时kick
            ret = 1;
            //printf("nblank:%x %x\r\n",blank,blank_loop);
        }else{
            _os_printf("_D2_2");

            if(uvc->state != 3){
                uvc->state = 0;
                free_get_uvc_node_list(&uvc->list,&free_uvc_tab[1]);
            }

            uvc_p[1]   = get_new_uvc_frame_head2(1);                  //获取新的frame,然后检测下这个frame是不是已有blank，有的话，要先清
            uvc_msg[1] = list_entry(uvc_p[1],UVC_MANAGE,list);//&uvc_msg_frame[0];
            uvc_msg[1]->sta = 0;   
            if(uvc_msg[1] == uvc){
                free_get_uvc_node_list(&uvc->list,&free_uvc_tab[1]); 
                del_uvc_frame(uvc);
                uvc_msg[1]->state = 1;
            }else{
                uvc->frame_end = 2;
                uvc->sta = 0;   
            }
            return 0;
        }
        frame_num[1]++;
    }
    //当前frame的数据处理
    //1:通过当前frame找到当前blank的结构体
    //printf("uvc_bnk:%x\r\n",uvc_bnk);
    blank = list_entry(uvc_bnk[1],UVC_BLANK,list);
    blank->busy = 1;
    if(blank->re_space < data_len){                                                     //当前blank剩余空间不足
        //printf("a2:%x %d %d\r\n",blank->buf_ptr+blank->blank_len,blank->blank_len,blank->re_space);
        hw_memcpy(blank->buf_ptr+blank->blank_len,&rx_buff[head_len],blank->re_space);
        if((rx_buff[rx_packet_len_2-2] == 0xFF)&&(rx_buff[rx_packet_len_2-1] == 0xD9)) {
            last_frame_end = 1;
        }
        //printf("buf:%x\r\n",blank->buf_ptr+blank->blank_len);
        re_space = blank->re_space;
        blank->blank_len +=blank->re_space;
        blank->re_space = 0;
        blank->busy = 2;    
        //printf("t2:%d\n",in_times);

        //uvc_time = csi_kernel_get_ticks();

        blank_loop = blank->blank_loop;

        if(uvc == NULL)
            _os_printf("uvc hehe\r\n");
        
        uvc_bnk[1] = get_blank_node(&uvc->list,&free_uvc_tab[1]);                                    //重新获取一个新的blank，将剩余数据回填进去        
//      _os_printf("GB");
        if(uvc_bnk[1] != 0)
        {
            blank = list_entry(uvc_bnk[1],UVC_BLANK,list);
            blank->blank_len = 0;
            blank->re_space        = UVC_BLANK_LEN;
            blank->busy = 0;
            blank_loop++;
            blank->blank_loop = blank_loop;

            ret = 1;
            //_os_printf("blank:%x %x\r\n",blank,blank_loop);
            //return 0;
        }else{
            _os_printf("_D1_2");                             //一个frame就将所有的内存都占光了，没有新的内存可以存放此帧图像了

            if(uvc->state != 3){
                uvc->state = 0;             
                free_get_uvc_node_list(&uvc->list,&free_uvc_tab[1]);
            }

            
            uvc_p[1]   = get_new_uvc_frame_head2(1);                  //获取新的frame,然后检测下这个frame是不是已有blank，有的话，要先清
            uvc_msg[1] = list_entry(uvc_p[1],UVC_MANAGE,list);//&uvc_msg_frame[0];
            uvc_msg[1]->sta = 0;
            if(uvc == NULL)
                _os_printf("hehe\r\n");
            
            if(uvc_msg[1] == uvc){
                free_get_uvc_node_list(&uvc->list,&free_uvc_tab[1]); 
                del_uvc_frame(uvc);
                uvc_msg[1]->state = 1;
            }else{
                uvc->frame_end = 2;
                uvc->sta = 0;   
            }           


            return 0;
        }

        hw_memcpy(blank->buf_ptr+blank->blank_len,&rx_buff[re_space+head_len],data_len - re_space);   
        if((rx_buff[rx_packet_len_2-2] == 0xFF)&&(rx_buff[rx_packet_len_2-1] == 0xD9)) {
            last_frame_end = 1;
        }
        blank->re_space  -=(data_len - re_space);
        blank->blank_len +=(data_len - re_space);
        blank->busy = 1; 
        uvc->frame_len   += data_len;
        //os_printf("1");
    }else{                                                                              //当前blank剩余空间足够     
        //printf("a1:%x %d %d\r\n",blank->buf_ptr+blank->blank_len,blank->blank_len,data_len);
        hw_memcpy(blank->buf_ptr+blank->blank_len,&rx_buff[head_len],data_len);      
        if((rx_buff[rx_packet_len_2-2] == 0xFF)&&(rx_buff[rx_packet_len_2-1] == 0xD9)) {
            last_frame_end = 1;
        }
        uvc->frame_len += data_len;
        blank->re_space -=data_len;
        blank->blank_len +=data_len;
        //printf(".");  
    }
	
    if(end_frame){
        set_uvc_frame_ready(uvc);
        blank->busy = 2;
        uvc->frame_end = 1;
    }

    return ret;
}

int uvc_deal_mjpeg(struct hgusb20_dev *p_dev, uint8_t* rx_buff, uint8_t ep, uint8_t ep_type)
{
    uint8 drop = 0;
    int ret = 0;
	uint32 data_len;
	static uint8_t bulk_head[12];
	static uint8_t recv_start = 0;
    uint8_t *uvc_cache = NULL;
    static uint32_t uvc_cache_buff[256+3];

	if((ep_type & 0X03) == USB_ENDPOINT_XFER_BULK){

        uvc_cache = (uint8_t *)&uvc_cache_buff;

        
		hw_memcpy(uvc_msg[0]->uvc_head,rx_buff+BULK_HEAD,16);
		if((uvc_msg[0]->uvc_head[0] == 0x02)&&((uvc_msg[0]->uvc_head[1]&0X80) == 0x80)&&(uvc_msg[0]->uvc_head[2] == 0xFF)&&(uvc_msg[0]->uvc_head[3] == 0xD8)){
			_os_printf("P2");
			hw_memcpy(bulk_head,rx_buff+BULK_HEAD,BULK_HEAD);
			hw_memcpy(uvc_cache,rx_buff+BULK_HEAD,rx_packet_len);
			hw_memcpy(rx_buff,uvc_cache,rx_packet_len);
			uvc_msg[0]->uvc_head[0] = 12;
			bulk_head[0] = 12;
		}
		else if((uvc_msg[0]->uvc_head[0] == 0x0c)&&((uvc_msg[0]->uvc_head[1]&0XFE) == 0x8c)&&(uvc_msg[0]->uvc_head[12] == 0xFF)&&(uvc_msg[0]->uvc_head[13] == 0xD8)){
			_os_printf("P:%x ",uvc_msg[0]->uvc_head[2]);
			hw_memcpy(bulk_head,rx_buff+BULK_HEAD,BULK_HEAD);
			hw_memcpy(uvc_cache,rx_buff+BULK_HEAD,rx_packet_len);
			hw_memcpy(rx_buff,uvc_cache,rx_packet_len);
			recv_start = 1;
		}else if((uvc_msg[0]->uvc_head[0] == 0x0c)&&((uvc_msg[0]->uvc_head[1]&0XFE) == 0x82)&&
				 (uvc_msg[0]->uvc_head[12] == 0x00)&&(uvc_msg[0]->uvc_head[13] == 0x00)&&
				 (uvc_msg[0]->uvc_head[14] == 0x00)&&(uvc_msg[0]->uvc_head[15] == 0x01)){
			hw_memcpy(bulk_head,rx_buff+BULK_HEAD,BULK_HEAD);
			hw_memcpy(uvc_cache,rx_buff+BULK_HEAD,rx_packet_len);
			hw_memcpy(rx_buff,uvc_cache,rx_packet_len);
		}
		else{
			uint8 same_cnt = 0;
			if(recv_start==1) {
				for(uint8 i=0; i<6; i++)
				{
					if(uvc_msg[0]->uvc_head[i] == bulk_head[i])
						same_cnt++;
				}
			}
			if(same_cnt < 6) {
				hw_memcpy(rx_buff,bulk_head,12);
				rx_packet_len+=12;				
			}
			else {
				hw_memcpy(uvc_cache+BULK_HEAD,rx_buff+BULK_HEAD*2,rx_packet_len-BULK_HEAD);
				hw_memcpy(uvc_cache,bulk_head,BULK_HEAD);
				hw_memcpy(rx_buff, uvc_cache, rx_packet_len);
			}
		}


		
	}
	hw_memcpy(uvc_msg[0]->uvc_head,rx_buff,16);
	head_len = uvc_msg[0]->uvc_head[0];
	if(head_len > rx_packet_len){
		drop = 1; 
	}
	
	data_len = rx_packet_len-head_len;
	if((data_len == 0) && (uvc_msg[0]->uvc_head[1] & (BIT(6))))
    {
        drop = 1;
    }
    else if(data_len == 0)
    {
 		return ret;       
    }
	
//无psram
#ifndef PSRAM_HEAP
    if(uvc_msg[0]->uvc_head[1]&BIT(1)){
        drop = 2;
    }
#endif  


    if (hgusb20_host_is_crc_err(p_dev, ep)) {
        drop = 1; 
    }
   

    if((uvc_msg[0]->uvc_head[0] != 0x0c)&&(uvc_msg[0]->uvc_head[0] != 0x02)){//head[0] is head len 
        drop = 1;   
    }
    
    if(uvc_msg[0]->uvc_head[1] & (BIT(6) | BIT(5))){//error packe
        drop = 1;
    }  
    
    switch(uvc_msg[0]->sta){
        case 0:   uvc_irq_redy(uvc_msg[0]); 
        case 1:   ret = uvc_irq_wait_next_frame(uvc_msg[0]); 
                  if(uvc_msg[0]->sta != 2)  break;
        case 2:   ret = uvc_data_handle(uvc_msg[0],p_dev,rx_buff,drop);  break;
    }

    return ret;

}

int uvc_deal_h264(struct hgusb20_dev *p_dev, uint8_t* rx_buff, uint8_t ep, uint8 ep_type)
{
    uint8 drop = 0;
    int ret = 0;
	uint32 data_len;
	static uint8_t bulk_head[12];
	static uint8_t recv_start = 0;
    uint8_t *uvc_cache = NULL;
    static uint32_t uvc_cache_buff[256+3];
    
	if((ep_type & 0X03) == USB_ENDPOINT_XFER_BULK){

        uvc_cache = (uint8_t *)&uvc_cache_buff;
        
		hw_memcpy(uvc_msg[1]->uvc_head,rx_buff+BULK_HEAD,16);
		if((uvc_msg[1]->uvc_head[0] == 0x02)&&((uvc_msg[1]->uvc_head[1]&0X80) == 0x80)&&(uvc_msg[1]->uvc_head[2] == 0xFF)&&(uvc_msg[1]->uvc_head[3] == 0xD8)){
			_os_printf("P2");
			hw_memcpy(bulk_head,rx_buff+BULK_HEAD,BULK_HEAD);
			hw_memcpy(uvc_cache,rx_buff+BULK_HEAD,rx_packet_len_2);
			hw_memcpy(rx_buff,uvc_cache,rx_packet_len_2);
			uvc_msg[1]->uvc_head[0] = 12;
			bulk_head[0] = 12;
		}
		else if((uvc_msg[1]->uvc_head[0] == 0x0c)&&((uvc_msg[1]->uvc_head[1]&0XFE) == 0x8c)&&(uvc_msg[1]->uvc_head[12] == 0xFF)&&(uvc_msg[1]->uvc_head[13] == 0xD8)){
			_os_printf("P:%x ",uvc_msg[1]->uvc_head[2]);
			hw_memcpy(bulk_head,rx_buff+BULK_HEAD,BULK_HEAD);
			hw_memcpy(uvc_cache,rx_buff+BULK_HEAD,rx_packet_len_2);
			hw_memcpy(rx_buff,uvc_cache,rx_packet_len_2);
			recv_start = 1;
		}else if((uvc_msg[1]->uvc_head[0] == 0x0c)&&((uvc_msg[1]->uvc_head[1]&0XFE) == 0x82)&&
				 (uvc_msg[1]->uvc_head[12] == 0x00)&&(uvc_msg[1]->uvc_head[13] == 0x00)&&
				 (uvc_msg[1]->uvc_head[14] == 0x00)&&(uvc_msg[1]->uvc_head[15] == 0x01)){
			hw_memcpy(bulk_head,rx_buff+BULK_HEAD,BULK_HEAD);
			hw_memcpy(uvc_cache,rx_buff+BULK_HEAD,rx_packet_len_2);
			hw_memcpy(rx_buff,uvc_cache,rx_packet_len_2);
		}
		else{
			uint8 same_cnt = 0;
			if(recv_start==1) {
				for(uint8 i=0; i<6; i++)
				{
					if(uvc_msg[1]->uvc_head[i] == bulk_head[i])
						same_cnt++;
				}
			}
			if(same_cnt < 6) {
				hw_memcpy(rx_buff,bulk_head,12);
				rx_packet_len_2+=12;				
			}
			else {
				hw_memcpy(uvc_cache+BULK_HEAD,rx_buff+BULK_HEAD*2,rx_packet_len_2-BULK_HEAD);
				hw_memcpy(uvc_cache,bulk_head,BULK_HEAD);
				hw_memcpy(rx_buff, uvc_cache, rx_packet_len_2);
			}
		}
        

	}
	hw_memcpy(uvc_msg[1]->uvc_head,rx_buff,16);
	head_len = uvc_msg[1]->uvc_head[0];
	if(head_len > rx_packet_len_2){
		drop = 1; 
	}

	data_len = rx_packet_len_2-head_len;
	if((data_len == 0) && (uvc_msg[1]->uvc_head[1] & (BIT(6))))
    {
        drop = 1;
    }
    else if(data_len == 0)
    {
 		return ret;       
    }
	
//无psram
#ifndef PSRAM_HEAP
    if(uvc_msg[1]->uvc_head[1]&BIT(1)){
        drop = 2;
    }
#endif  

    if (hgusb20_host_is_crc_err(p_dev, ep)) {
        drop = 1; 
    }

    if((uvc_msg[1]->uvc_head[0] != 0x0c)&&(uvc_msg[1]->uvc_head[0] != 0x02)){//head[0] is head len 
        drop = 1;   
    }
    
	
    if(uvc_msg[1]->uvc_head[1] & (BIT(6) | BIT(5))){//error packe
        drop = 1;
    }  
    
    switch(uvc_msg[1]->sta){
        case 0:   uvc_irq_redy2(uvc_msg[1]); 
        case 1:   ret = uvc_irq_wait_next_frame2(uvc_msg[1]); 
                  if(uvc_msg[1]->sta != 2)  break;
        case 2:   ret = uvc_data_handle2(uvc_msg[1],p_dev,rx_buff,drop);  break;
    }

    return ret;

}

int usb_dma_irq_times= 0;

void usb_dma_irq(void * dev, uint8_t* uvc_rx_buff, uint8_t uvc_head_res, uint8_t ep_type)
{   
    uint32 ret;
    usb_dma_irq_times++;
    struct hgusb20_dev *p_dev = (struct hgusb20_dev *)dev;
    if (!p_dev)
        return;

    ret = uvc_data_deal(p_dev); 
    if(ret == 0){
        ret = uvc_deal_mjpeg(p_dev, (uint8_t*)uvc_rx_buff, UVC_EP, ep_type);
    }

    hgusb20_ep_rx_kick(p_dev, UVC_EP, (uint32)(uvc_rx_buff+uvc_head_res),0x400);

}




//暂时会比较简单去实现,定义都是比较随便,如果可以,jpeg与uvc能统一最好
//mjpeg信号量创建
static struct os_semaphore uvc_sem = {0};
static int uvc_sem_init = 0;
void uvc_sema_init()
{
    os_sema_init(&uvc_sem,0);
    uvc_sem_init = 1;
}

void uvc_sema_down(int32 tmo_ms)
{
    if(!uvc_sem_init)
    {
        return;
    }
    os_sema_down(&uvc_sem,tmo_ms);
    //printf("$\n");
}

void uvc_sema_up()
{
    if(!uvc_sem_init)
    {
        return;
    }
    os_sema_up(&uvc_sem);
}

UVC_MANAGE* uvc_message_gloal;

void set_uvc_message_gloal(UVC_MANAGE* uvc_message){
    uvc_message_gloal = uvc_message;
}


UVC_MANAGE* get_uvc_message_gloal(){
    return uvc_message_gloal;
}

void free_uvc_message_gloal(){
    uvc_message_gloal = NULL;
}

uint32 get_uvc_frame_len(){
    return uvc_message_gloal->frame_len;
}

UVC_BLANK* get_uvc_blank(){
    UVC_BLANK* uvc_b;
    if((list_empty(&uvc_message_gloal->list) != TRUE )||(uvc_message_gloal->frame_end == 0)){
        uvc_b = list_entry(uvc_message_gloal->list.prev,UVC_BLANK,list);
        return uvc_b;
    }
    return NULL;
}

uint8 free_uvc_blank(UVC_BLANK* uvc_b){
    if(uvc_b == NULL) return 0;
    free_node2tab(uvc_b,(struct list_head*)&free_uvc_tab,&uvc_message_gloal->list);
    return 1; 
}

uint8 free_uvc_blank_nopsram(UVC_MANAGE* uvc_message){

    UVC_BLANK* uvc_b = NULL;
    if((list_empty(&uvc_message->list) != TRUE )||(uvc_message->frame_end == 0)){
        uvc_b = list_entry(uvc_message->list.prev,UVC_BLANK,list);
    }
    if(uvc_b == NULL) return 0;
    free_node2tab(uvc_b,(struct list_head*)&free_uvc_tab,&uvc_message->list);
    return 1; 
}


uint8* get_uvc_buf(){
    UVC_BLANK* uvc_b;
    uvc_b = get_uvc_blank();
    if(uvc_b == NULL)
        return NULL;

    return uvc_b->buf_ptr;
}

uint8_t uvc_open = 1;

#if 0
UVC_BLANK* uvc_global = NULL;
UVC_MANAGE*uvc_message_global = NULL;
void uvc_stream_open(struct hgusb20_dev *p_dev,uint8 enable);
extern volatile uint8 scale2_finish;
#if DVP_EN
uint8 *psram_uvc_buf[2];
extern uint8 psram_ybuf_src[480*640+480*640/2];
#else
__psram_data uint8 psram_uvc_buf[2][200*1024] __aligned(4);
#endif

#if LCD_EN == 1
void jpg_decode_to_lcd(uint32 photo,uint32 jpg_w,uint32 jpg_h,uint32 video_w,uint32 video_h);
void uvc_user4(void *d)
{
    //uint8 msgbuf[10];
    //u16* pt;
    uint8_t uvc_state = 2;
    os_sleep_ms(5);
    struct uvc_user_arg *arg = (struct uvc_user_arg*)d;
    uint32 tick_num = 0;
    UVC_MANAGE* uvc_message;
    UVC_BLANK* uvc_b;
    struct usb_device *dev = (struct usb_device *)dev_get(HG_USBDEV_DEVID);
    struct list_head* get_f;
    int del_times = 0;
    int err = 0;
	uint32 dnum = 0;
    uint8* buf = NULL;
    int irq_time = 0;
    //struct list_head* get_analy_node = NULL;
    uint8 pingpang = 0;
#if DVP_EN
	psram_uvc_buf[0] = psram_ybuf_src;
	psram_uvc_buf[1] = psram_ybuf_src+200*1024;
#endif
	
	//os_sleep_ms(6000);
    while(1){
        get_f = get_uvc_frame();

        if(get_f == 0) 
        {
            //state=2,代表有通知需要删除任务
            if(arg->state == 2)
            {
                err = 1;
                goto uvc_user4_exit;
            }

			if(uvc_open==1){
				if(uvc_state != uvc_open){
					uvc_stream_open((struct hgusb20_dev *)dev,1);
					uvc_state = uvc_open;
				}
			}
			
            //释放cpu
            os_sleep_ms(1);
            continue;
        }
		
		if(uvc_open==0){
			if(uvc_state != uvc_open){
				uvc_stream_open((struct hgusb20_dev *)dev,0);
				uvc_state = uvc_open;
			}
		}

		
        err = 0;
        irq_time = 0;
        tick_num++;
        uvc_message = list_entry(get_f,UVC_MANAGE,list);
        set_uvc_frame_using(uvc_message);   
        uvc_message_global  = uvc_message;
        while((list_empty(&uvc_message->list) != TRUE )||(uvc_message->frame_end == 0)){
			if(uvc_open==0)
				break;
            irq_disable(USB20DMA_IRQn);
            uvc_b = list_entry(uvc_message->list.prev,UVC_BLANK,list);
            if(uvc_message->frame_end  == 2){
                break;
            }
            irq_enable(USB20DMA_IRQn);

            //if(arg->state == 2)
            //{
            //    err = 1;
            //    goto uvc_user4_exit;
            //}


            if(uvc_message->state != 3)
                _os_printf("state_err\r\n");
            
            uvc_global = uvc_b;
            if(uvc_b && uvc_b->busy == 2)
            {
                //数据头,所以要申请一帧数据
                if(uvc_b->blank_loop == 0)
                {

                    //正常不可能进来这里的
                    //获取下一帧
                    //u = f_get_uvc_frame_current_task();
                    
                    //get_analy_node = get_app_analyze_node(&uvc_app_analyze);
                    //if(get_analy_node)
                    //    buf = get_real_buf_adr(get_analy_node);
                    //else 
                    //   buf = NULL;
                    if(pingpang == 0){
						buf = psram_uvc_buf[0];
						//pingpang = 1;
					}else{
						buf = psram_uvc_buf[1];
						//pingpang = 0;
					}
                }

                //没有帧,那么只能将bank删除,应该是rtsp处理速度不够
                if(1)//(get_analy_node)
                {

                    #if 0
                    if(uvc_b->buf_ptr[0] == 0xFF && uvc_b->buf_ptr[1] == 0xD8 && uvc_b->blank_loop != 0)
                    {
                        //printf("loop:%d\t%X\n",uvc_b->blank_loop,get_f);
                    }
                    #endif
                    hw_memcpy0(buf,uvc_b->buf_ptr,uvc_b->blank_len);
                    buf += uvc_b->blank_len;
                }
                else
                {
                    _os_printf("=");
                }
                  
                free_node2tab(uvc_b,(struct list_head*)&free_uvc_tab,&uvc_message->list);
            }   
            else
            {
                //释放cpu
                os_sleep_ms(1);
                irq_time++;
                if(irq_time>200)
                {
                    irq_time = 0;
                    if(!usb_dma_irq_times)
                    {
                        //err = 1;
                        _os_printf("%s usb maybe disconnect\n",__FUNCTION__);
						uvc_message->frame_end = 2;
						uvc_message->sta       = 0;
                        int32 usb_host_uvc_ioctl(struct usb_device *p_usb_d, uint32 cmd, uint32 param1, uint32 param2);
                        usb_host_uvc_ioctl(dev, USB_HOST_IO_CMD_RESET,0,0);
                        break;
                    }
                    usb_dma_irq_times = 0;
                }
            }
            //continue;
        }

        if(err)
        {

            #if 0
            if(get_analy_node)
            {
                //f_del_uvc_frame(u);
                free_real_msg(get_analy_node,&uvc_app_analyze.free_list);
                get_analy_node = NULL;
            }
            continue;
            #endif
            goto uvc_user4_exit;

        }
        //异常退出,对应frame也del
        if(uvc_message->frame_end == 2)
        {
            //printf("get_F:%x\r\n",get_f);
            free_get_uvc_node_list(get_f,&free_uvc_tab[0]);    
            del_uvc_frame(uvc_message);
            
        
            //if(get_analy_node)
            //{
                //f_del_uvc_frame(u);
            //    free_real_msg(get_analy_node,&uvc_app_analyze.free_list);
            //    get_analy_node = NULL;
            //}

        }

        //正常退出
        else
        {
            //设置状态,等于发送frame
 //           if(get_analy_node)
            {

                //关闭usb中断,打印对应的数据出来   
                //u->frame_len = uvc_message->frame_len;
//                set_app_buf_len(get_analy_node,uvc_message->frame_len);             

                #if 1
                //f_set_uvc_frame_status(u);
                #else
                f_set_uvc_frame_status(u);
                f_del_uvc_frame(u);
                #endif

                //printf("frame_len:%d\n",uvc_message->frame_len);
                
                del_times++;
//                map_realnode_2_app_node_msg(&uvc_app_analyze,get_analy_node);

				if(uvc_open==1){
					if(scale2_finish){

						scale2_finish = 0;
						if(pingpang == 0){
							jpg_decode_to_lcd((uint32)psram_uvc_buf[0],1280,720,160,120);
							pingpang = 1;
						}else{
							jpg_decode_to_lcd((uint32)psram_uvc_buf[1],1280,720,160,120);
							pingpang = 0;
						}
						dnum = 0;
					}else{
						dnum++;
						if(dnum > 8){
							scale2_finish = 1;
						}
					}
				}


				
                _os_printf("M");
//                wake_up_analyze_list_app(&uvc_app_analyze); 
//                get_analy_node = NULL;
//              uvc_sema_up();
            }

            //printf("del frame:%x %d %d\r\n",uvc_message,uvc_message->frame_len);
            //printf("=");
              
            //printf("?");

            del_uvc_frame(uvc_message); 

        }
    }
    uvc_user4_exit:
    if(err)
    {
    
//        if(get_analy_node)
        {
            //f_del_uvc_frame(u);
//            free_real_msg(get_analy_node,&uvc_app_analyze.free_list);
//            get_analy_node = NULL;
        }
        _os_printf("%s exit\n",__FUNCTION__);

        //代表任务已经被删除,可以重新启动
        arg->state = 0;
        return;
    
    }


    
}
#endif
#endif
#if 0
void uvc_user()
{
    UVC_MANAGE* uvc_message;
    UVC_BLANK* uvc_b;
//  uint32 len = 0;
//  uint32 itk = 0;
//  uint8 printf_uvc_jpg = 0;
//  struct list_head *uvc_bk;
    struct list_head* get_f;
    while(1){
        get_f = get_uvc_frame();
        if(get_f == 0) 
        {
            //释放cpu
            os_sleep_ms(1);
            continue;
        }
        uvc_message = list_entry(get_f,UVC_MANAGE,list);
        set_uvc_frame_using(uvc_message);   
        set_uvc_message_gloal(uvc_message);
        uvc_sema_down(-1);
        while((list_empty(&uvc_message->list) != TRUE )||(uvc_message->frame_end == 0)){
            uvc_b = list_entry(uvc_message->list.prev,UVC_BLANK,list);
            if(uvc_b->busy == 2)
            {
                free_node2tab(uvc_b,(struct list_head*)&free_uvc_tab,&uvc_message->list);
            }           
        }
        //printf("del frame:%x %d %d\r\n",uvc_message,uvc_message->frame_len,len);
        free_uvc_message_gloal();
        del_uvc_frame(uvc_message);
    }
}
#endif
#endif
