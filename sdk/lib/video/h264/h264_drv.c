#include "sys_config.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "lib/video/h264/h264_drv.h"
#include "devid.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "hal/h264.h"
#include "dev/scale/hgscale.h"
#include "dev/h264/hg264.h"
#include "osal/semaphore.h"
#include "openDML.h"
#include "osal/mutex.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "gen420_hardware_msi.h"
#include "lib/video/vpp/vpp_dev.h"
#include "lib/multimedia/msi.h"


#define H264_MALLOC av_psram_malloc
#define H264_FREE   av_psram_free
#define H264_ZALLOC av_psram_zalloc



#ifndef H264_HARDWARE_CLK
#define H264_HARDWARE_CLK	H264_MODULE_CLK_480M
#endif

#ifndef H264_I_ONLY
#define H264_I_ONLY         0
#endif

volatile uint32_t h264_dev_num = 1;
volatile struct	h264_cfg_t    enc_cfg;
volatile struct h264_ctl_t	  enc_ctl;
volatile struct h264_rc_ctl_t rc_ctl;

volatile struct	h264_cfg_t    enc_2_cfg;
volatile struct h264_ctl_t	  enc_2_ctl;
volatile struct h264_rc_ctl_t rc_2_ctl;

volatile struct	h264_cfg_t    *penc_cfg;
volatile struct h264_ctl_t	  *penc_ctl;
volatile struct h264_rc_ctl_t *prc_ctl;

uint8_t *h264_ref_lu_base_psram = NULL;
uint8_t *h264_ref_ch_base_psram = NULL;

uint8_t *h264_ref_lu_base_psram2 = NULL;
uint8_t *h264_ref_ch_base_psram2 = NULL;

uint8_t *h264_room_psram = NULL;


uint8_t *h264_ref_memory_base[10];


__psram_data uint8_t h264_room[H264_NODE_NUM*H264_NODE_LEN] __aligned(1024);

volatile h264_frame h264_frame_point[H264_FRAME_NUM];
volatile struct list_head *h264_module_p;		//264模块的节点指针
volatile struct list_head *h264_app_p;			//应用的节点指针
volatile struct list_head* h264_f_p;			//当前264所使用的frame
volatile h264_node h264_node_src[H264_NODE_NUM];
volatile struct list_head h264_free_tab;			//空闲列表，存放空间节点
volatile uint8 h264_error = 0;
volatile uint8 frame_done = 0;

static struct os_semaphore h264_sem = {0,NULL};

volatile struct list_head h264_queue_head;

static struct os_semaphore h264wq_sem = {0,NULL};
void h264wq_sema_init()
{
	os_sema_init(&h264wq_sem,0);
}

void h264wq_sema_down(int32 tmo_ms)
{
	os_sema_down(&h264wq_sem,tmo_ms);
}

void h264wq_sema_up()
{
	os_sema_up(&h264wq_sem);
}


/**@brief 
 * 将传入的frame根节点下的buf池都送回空间池（free_tab）中
 */
static bool free_get_node_list(volatile struct list_head *head,volatile struct list_head *free){
	if(list_empty((struct list_head *)head)){
		return 0;
	}	

	list_splice_init((struct list_head *)head,(struct list_head *)free);
	return 1;
}

/**@brief 
 * 从初始化的frame根节点中抽取其中一个空闲的根节点进行使用，并标记frame的使用状态为正在使用
 * @param 是否开启抢占模式，如果开启，则将上一帧已完成的帧节点删掉，并返回
 */


volatile struct list_head* get_h264_new_frame_head(int grab,uint8_t devid,uint8_t is_iframe){
	uint8 frame_num = 0;
	for(frame_num = 0;frame_num < H264_FRAME_NUM;frame_num++){
		if(h264_frame_point[frame_num].usable == 0){
			h264_frame_point[frame_num].usable = 1;
			if(is_iframe){
				h264_frame_point[frame_num].h264_type = 1;
			}else{
				h264_frame_point[frame_num].h264_type = 2;
			}	
			h264_frame_point[frame_num].h264_dev_id = devid;
			return &h264_frame_point[frame_num].list;
		}

	}

	if(grab)								//是否开启抢占模式,开启抢占后，肯定有frame返回，上一帧没使用的frame肯定usable为2
	{
		
		for(frame_num = 0;frame_num < H264_FRAME_NUM;frame_num++){
			if(h264_frame_point[frame_num].usable == 2){
				h264_frame_point[frame_num].usable = 1;
				if(is_iframe){
					h264_frame_point[frame_num].h264_type = 1;
				}else{
					h264_frame_point[frame_num].h264_type = 2;
				}				
				h264_frame_point[frame_num].h264_dev_id = devid;
				free_get_node_list(&h264_frame_point[frame_num].list,&h264_free_tab);
				return &h264_frame_point[frame_num].list;
			}
		}
	}
	
	return 0;
}

/**@brief 
 * 初始化free_tab池，将mjpeg_node的节点都放到空闲池中，供frame后面提取使用
 * @param ftb 空闲池头指针
 * @param jpn mjpeg_node节点源
 * @param use_num  mjpeg_node的数量，即多少个mjpeg_node放到空闲池
 * @param addr   mjpeg_node的总buf起始地址
 * @param buf_len  每个mjpeg_node所关联到的数据量
 */
void h264_free_table_init(volatile struct list_head *ftb, volatile h264_node* h264n,int use_num,uint32 addr,uint32 buf_len){
	int itk;
	for(itk = 0;itk < use_num;itk++){
		h264n->buf_addr = (uint8*)(addr + itk*buf_len);
		list_add_tail((struct list_head *)&h264n->list,(struct list_head *)ftb); 
		h264n++;
	}
}

static void set_frame_ready(h264_frame *hf){
	hf->timestamp = os_jiffies();
	hf->usable = 2;
}


/**@brief 
 * 从空间池中提取一个节点，放到队列中，并将此节点作为返回值返回
 */
static struct list_head *get_node(volatile struct list_head *head,volatile struct list_head *del){
	if(list_empty((struct list_head *)del)){
		return 0;
	}

	list_move((struct list_head *)del->next,(struct list_head *)head);

	return head->next;				//返回最新的位置
}

/**@brief 
 * 从当前使用的节点回放到消息池中
 */
static bool put_node(volatile struct list_head *head,volatile struct list_head *del){
	if(list_empty((struct list_head *)del)){
		return 0;
	}
	uint32_t flags;
	flags = disable_irq();
	list_move(del->next,(struct list_head *)head);
	enable_irq(flags);

	return 1;
}


/**@brief 
 * 获取当前jpeg节点buf地址进行返回
 */
static uint32 get_addr(volatile struct list_head *list){
	h264_node* h264n;
	h264n = list_entry((struct list_head *)list,h264_node,list);
	return (uint32)h264n->buf_addr;
}

/**@brief 
 * 将传入的frame根节点状态调整为空闲，以供下次重新获取
 */
void del_264_frame(volatile struct list_head* frame_list)
{


	h264_frame* fl;
	uint32_t flags;
	if(list_empty((struct list_head *)frame_list) != TRUE){	
		flags = disable_irq();
		free_get_node_list(frame_list,&h264_free_tab);
		fl = list_entry((struct list_head *)frame_list,h264_frame,list);
		fl->usable = 0;
		enable_irq(flags);
		return;		

	}else{
		flags = disable_irq();
		fl = list_entry((struct list_head *)frame_list,h264_frame,list);
		fl->usable = 0;
		enable_irq(flags);
		return;
	}


}

uint32 get_h264_node_len_new(void *get_f)
{
	return H264_NODE_LEN;
}


void del_h264_frame(void *d)
{
	h264_frame* get_f = (h264_frame*)d;
	del_264_frame((struct list_head*)get_f);

}

struct list_head* get_h264_frame()
{
	uint8 frame_num = 0;
	
	for(frame_num = 0;frame_num < H264_FRAME_NUM;frame_num++){
		if(h264_frame_point[frame_num].usable == 2){
			h264_frame_point[frame_num].usable = 1;
			return (struct list_head *)&h264_frame_point[frame_num].list;
		}
	}

	return NULL;	
}


uint32 get_h264_timestamp(void *d)
{
	struct list_head* get_f = (struct list_head*)d;
	h264_frame* hf;
	hf = list_entry(get_f,h264_frame,list);
	return hf->timestamp;
}


uint32 get_h264_which(void *d)
{
	struct list_head* get_f = (struct list_head*)d;
	h264_frame* hf;
	hf = list_entry(get_f,h264_frame,list);
	return hf->which;
}

uint32 get_h264_srcID(void *d)
{
	struct list_head* get_f = (struct list_head*)d;
	h264_frame* hf;
	hf = list_entry(get_f,h264_frame,list);
	return hf->srcID;
}

uint32 get_h264_len(void *d)
{
	uint32 flen;
	struct list_head* get_f = (struct list_head*)d;
	h264_frame* hf;
	hf = list_entry(get_f,h264_frame,list);
	flen = hf->frame_len;
	return flen;
}

uint8 get_h264_type(void *d)
{
	uint8 type;
	struct list_head* get_f = (struct list_head*)d;
	h264_frame* hf;
	hf = list_entry(get_f,h264_frame,list);
	type = hf->h264_type;
	return type;
}

uint8 get_h264_loop_num(void *d)
{
	uint8 num;
	struct list_head* get_f = (struct list_head*)d;
	h264_frame* hf;
	hf = list_entry(get_f,h264_frame,list);
	num = hf->h264_num;
	return num;
}

void *get_h264_first_buf(void *d)
{
	struct list_head* get_f = (struct list_head*)d;
	h264_node* hn;
	hn = list_entry(get_f->next,h264_node,list);
	if(hn)
	{
		return hn->buf_addr;
	}
	return NULL;
} 
 
int del_h264_first_node(void *d)
{	
	h264_frame* get_f = (h264_frame*)d;

	uint8_t *h264_buf = get_h264_first_buf(get_f);
	uint32_t h264_buf_node_len = get_h264_node_len_new((void *)get_f);
	sys_dcache_clean_invalid_range((uint32_t *)h264_buf, h264_buf_node_len);
	put_node(&h264_free_tab,(struct list_head*)get_f);
	
	return 0;	
}
 
void *get_h264_node_buf(void *d)
{
	struct list_head* get_f = (struct list_head*)d;
	h264_node* hn;
	hn = list_entry(get_f,h264_node,list);
	if(hn)
	{
		return hn->buf_addr;
	}
	return NULL;
}


//删除节点,是实际节点
void del_h264_node(void *d)
{
	uint32_t flags;
	struct list_head* get_f = (struct list_head *)d;
	flags = disable_irq();
	list_add_tail(get_f,(struct list_head*)&h264_free_tab);
	enable_irq(flags);
}


uint32 get_h264_w_h(void *d,uint16_t *w,uint16_t *h)
{
	h264_frame* get_f = (h264_frame*)d;
	if(w)
	{
		*w = get_f->w;
	}

	if(h)
	{
		*h = get_f->h;
	}
	return 0;
}





void h264_room_init(){
	uint8_t i;
	for(i = 0;i < H264_FRAME_NUM;i++){
		INIT_LIST_HEAD((struct list_head *)&h264_frame_point[i].list);
	}

	INIT_LIST_HEAD((struct list_head *)&h264_free_tab);
	uint32_t h264_rom_addr = ((uint32_t)h264_room_psram + 0x3ff)&(~0x3ff);
	h264_free_table_init(&h264_free_tab,(h264_node*)&h264_node_src,H264_NODE_NUM,(uint32)h264_rom_addr,H264_NODE_LEN);

	for(i = 0;i < H264_FRAME_NUM;i++){
		h264_frame_point[i].usable      = 0;
		h264_frame_point[i].h264_num    = 0;
		h264_frame_point[i].h264_dev_id = 0;
	}
}

void h264_3dnr_ini(struct h264_device  *p_h264,struct h264_cfg_t *enc_cfg)
{
	uint8_t      h264_noise_level = enc_cfg->flt_noise_lev;
	uint16_t     i;
	uint32_t     wdata   ; 
	uint8_t      d0, d1, d2, d3;

	uint32_t NoiseLevel              = 8     ;   // range:0 ~ 15
	uint32_t BackGroundNoiseLevel    = 8     ;   // range:0 ~ 15
	uint32_t LightFlowLevel          = 8     ;   // range:0 ~ 15
	uint32_t MV_Threshold            = 3     ;   // 0~15, 3 bits, 0: disable mv_threshold_check, user modify
	uint32_t MV_Trust_Cut            = 3840  ;   // if max_mcost<MV_Trust_Cut then force best_mv=(0,0), and 
	                                        //min_mcost=max_mcost; set MV_Trust_Cut=0, if you want to disable.
	uint32_t Merge_Fusion_Limit      = 16    ;   // max_dif(abs(flt-org))
	uint32_t TemporalFilterParameter = 8     ;   // range 0 ~ 64, user modify
	uint32_t MD_Cmp_Level            = 9     ;  
	uint32_t MV_Bias_SF              = 1     ;

 	uint8_t min_beta_by_max_mcost[64]={
         1,  1,   6,  8,    // 1024*0 ~ 1024*1
         8, 10,  10, 12,    // 1024*1 ~ 1024*2   
        16, 18,  20, 24,    // 1024*2 ~ 1024*3
        28, 32,  36, 40,    // 1024*3 ~ 1024*4
        44, 48,  52, 56,    // 1024*4 ~ 1024*5
        60, 60,  62, 62,    // 1024*5 ~ 1024*6
        64, 64,  64, 64,    // 1024*6 ~ 1024*7
        64, 64,  64, 64,    // 1024*7 ~ 1024*8
                            
        64, 64,  64, 64,    // 1024*8 ~ 1024*9   
        64, 64,  64, 64,    // 1024*9 ~ 1024*10
        64, 64,  64, 64,    // 1024*10 ~ 1024*11
        64, 64,  64, 64,    // 1024*11 ~ 1024*12
        64, 64,  64, 64,    // 1024*12 ~ 1024*13
        64, 64,  64, 64,    // 1024*13 ~ 1024*14
        64, 64,  64, 64,    // 1024*14 ~ 1024*15
        64, 64,  64, 64,    // 1024*15 ~ 1024*16
	};

    uint8_t min_beta_by_min_mcost[32]={
         1,  1,   1,  1,    //    0 ~ 1024
         1,  1,   1,  1,    // 1024 ~ 2048
         1,  4,   8, 12,    // 2048 ~ 3072
        16, 20,  24, 28,    // 3072 ~ 4096

        28, 28,  28, 29,    // 4096 ~ 5120
        32, 35,  38, 41,    // 5120 ~ 6144
        44, 46,  49, 52,    // 6144 ~ 7168
        55, 58,  61, 64,    // 7168 ~ 8192
	};

    uint8_t min_beta_by_mdcost00[16]={
         1,  7,  15, 23,    //    0 ~ 1024
        31, 39,  47, 55,    // 1024 ~ 2048
        64, 64,  64, 64,    // 2048 ~ 3072
        64, 64,  64, 64,    // 3072 ~ 4096 
    };

	if(h264_noise_level == 4) {
	    //very high noise
	    NoiseLevel              = 9     ;   // range:0 ~ 15
	    BackGroundNoiseLevel    = 9     ;   // range:0 ~ 15
	    LightFlowLevel          = 8     ;   // range:0 ~ 15
	    MV_Threshold            = 3     ;   // 0~15, 3 bits, 0: disable mv_threshold_check, user modify
	    Merge_Fusion_Limit      = 32    ;   // max_dif(abs(flt-org))
	    MV_Trust_Cut            = 4500  ;   // if max_mcost<MV_Trust_Cut then force best_mv=(0,0), and                                                             // min_mcost=max_mcost; set MV_Trust_Cut=0, if you want to disable.
	} else if(h264_noise_level == 3) {
	    //high noise
	    NoiseLevel              = 9     ;   // range:0 ~ 15
	    BackGroundNoiseLevel    = 8     ;   // range:0 ~ 15
	    LightFlowLevel          = 8     ;   // range:0 ~ 15
	    MV_Threshold            = 3     ;   // 0~15, 3 bits, 0: disable mv_threshold_check, user modify
	    Merge_Fusion_Limit      = 24    ;   // max_dif(abs(flt-org))
	    MV_Trust_Cut            = 4500  ;   // if max_mcost<MV_Trust_Cut then force best_mv=(0,0), and                                                             // min_mcost=max_mcost; set MV_Trust_Cut=0, if you want to disable.
	} else if(h264_noise_level == 2) {
	    //medium
	    NoiseLevel              = 8     ;   // range:0 ~ 15
	    BackGroundNoiseLevel    = 7     ;   // range:0 ~ 15
	    LightFlowLevel          = 7     ;   // range:0 ~ 15
	    MV_Threshold            = 3     ;   // 0~15, 3 bits, 0: disable mv_threshold_check, user modify
	    Merge_Fusion_Limit      = 16    ;   // max_dif(abs(flt-org))
	    MV_Trust_Cut            = 3840  ;   // if max_mcost<MV_Trust_Cut then force best_mv=(0,0), and                                                             // min_mcost=max_mcost; set MV_Trust_Cut=0, if you want to disable.
	} else if(h264_noise_level == 1) {
	    //low
	    NoiseLevel              = 7     ;   // range:0 ~ 15
	    BackGroundNoiseLevel    = 6     ;   // range:0 ~ 15
	    LightFlowLevel          = 7     ;   // range:0 ~ 15
	    MV_Threshold            = 2     ;   // 0~15, 3 bits, 0: disable mv_threshold_check, user modify
	    Merge_Fusion_Limit      = 12    ;   // max_dif(abs(flt-org))
	    MV_Trust_Cut            = 2800  ;   // if max_mcost<MV_Trust_Cut then force best_mv=(0,0), and                                                             // min_mcost=max_mcost; set MV_Trust_Cut=0, if you want to disable.
	} else {
	    //very low
	    NoiseLevel              = 7     ;   // range:0 ~ 15
	    BackGroundNoiseLevel    = 5     ;   // range:0 ~ 15
	    LightFlowLevel          = 6     ;   // range:0 ~ 15
	    MV_Threshold            = 2     ;   // 0~15, 3 bits, 0: disable mv_threshold_check, user modify
	    Merge_Fusion_Limit      = 9     ;   // max_dif(abs(flt-org))
	    MV_Trust_Cut            = 1800  ;   // if max_mcost<MV_Trust_Cut then force best_mv=(0,0), and                                                             // min_mcost=max_mcost; set MV_Trust_Cut=0, if you want to disable.
	}


	//--- 1: configure each tables
	for(i=0; i<4; i++) {
	    d0 = min_beta_by_mdcost00[i*4 + 0] - 1;
	    d1 = min_beta_by_mdcost00[i*4 + 1] - 1;
	    d2 = min_beta_by_mdcost00[i*4 + 2] - 1;
	    d3 = min_beta_by_mdcost00[i*4 + 3] - 1;
	    
	    wdata = ((uint32_t)d3 << 24) | ((uint32_t)d2 << 16) | ((uint32_t)d1 << 8) | ((uint32_t)d0 << 0);
		h264_set_flt_3dnr_lut(p_h264,i,wdata);
	}

	for(i=0; i<8; i++) {
	    d0 = min_beta_by_min_mcost[i*4 + 0] - 1;
	    d1 = min_beta_by_min_mcost[i*4 + 1] - 1;
	    d2 = min_beta_by_min_mcost[i*4 + 2] - 1;
	    d3 = min_beta_by_min_mcost[i*4 + 3] - 1;
	    
	    wdata = ((uint32_t)d3 << 24) | ((uint32_t)d2 << 16) | ((uint32_t)d1 << 8) | ((uint32_t)d0 << 0);
		h264_set_flt_min_lut(p_h264,i,wdata);
	}
	    
	for(i=0; i<16; i++) {
	    d0 = min_beta_by_max_mcost[i*4 + 0] - 1;
	    d1 = min_beta_by_max_mcost[i*4 + 1] - 1;
	    d2 = min_beta_by_max_mcost[i*4 + 2] - 1;
	    d3 = min_beta_by_max_mcost[i*4 + 3] - 1;
	    
	    wdata = ((uint32_t)d3 << 24) | ((uint32_t)d2 << 16) | ((uint32_t)d1 << 8) | ((uint32_t)d0 << 0);
		h264_set_flt_max_lut(p_h264,i,wdata);
	}


	//--- 3DNR ctrl_2
	wdata = MV_Bias_SF;
	h264_set_flt_3dnr_sf(p_h264,wdata);

	//--- 3DNR ctrl_1
	wdata = (MV_Trust_Cut << 16) | ((BackGroundNoiseLevel -5) << 12) | ((NoiseLevel-5) << 8) |
	        (MD_Cmp_Level << 4)  | (MV_Threshold << 0);

	h264_set_flt_3dnr_con1(p_h264,wdata);
	//--- 3DNR ctrl_0
	wdata = (0 << 28) | (TemporalFilterParameter << 20) | ((LightFlowLevel -5) << 16) |
	        (1 << 8)  | (Merge_Fusion_Limit << 0);	
	h264_set_flt_3dnr_con(p_h264,wdata);
}


volatile uint16 recfg_h264_bps[2];
volatile uint16 recfg_h264_rate[2];
volatile uint16 recfg_h264_gop[2];
volatile uint8  recfg_h264_ini_qp[2];
volatile uint8  recfg_h264_new_grop[2];

void h264_reflash_new_gop(uint8_t dev_num,uint8_t en){
	uint32_t ie;	
	ie = disable_irq();
	recfg_h264_new_grop[dev_num-1] = en;
	enable_irq(ie);
}

void h264_recfg_bsp(uint8_t dev_num,uint16_t bps){
	uint32_t ie;	
	ie = disable_irq();
	recfg_h264_bps[dev_num-1] = bps;
	enable_irq(ie);
}

void h264_recfg_rate(uint8_t dev_num,uint16_t rate){
	uint32_t ie;	
	ie = disable_irq();
	recfg_h264_rate[dev_num-1] = rate;
	enable_irq(ie);
}

void h264_recfg_frm_gop(uint8_t dev_num,uint16_t gop){
	uint32_t ie;	
	ie = disable_irq();
	recfg_h264_gop[dev_num-1] = gop;
	enable_irq(ie);
}

void h264_recfg_ini_qp(uint8_t dev_num,uint8_t qp){
	uint32_t ie;	
	ie = disable_irq();
	recfg_h264_ini_qp[dev_num-1] = qp;
	enable_irq(ie);
}

uint8_t h264_cfg_modify(struct h264_cfg_t *enc_cfg,uint8_t dev_num){
	uint8_t result = 0;
	if(enc_cfg->enc_bps != recfg_h264_bps[dev_num-1]){
		enc_cfg->enc_bps = recfg_h264_bps[dev_num-1];
		result = 1;
	}

	if(enc_cfg->frm_rate != recfg_h264_rate[dev_num-1]){
		enc_cfg->frm_rate = recfg_h264_rate[dev_num-1];
		result = 1;
	}

	if(enc_cfg->frm_gop != recfg_h264_gop[dev_num-1]){
		enc_cfg->frm_gop = recfg_h264_gop[dev_num-1];
		result = 1;
	}

	if(enc_cfg->ini_qp != recfg_h264_ini_qp[dev_num-1]){
		enc_cfg->ini_qp = recfg_h264_ini_qp[dev_num-1];
		result = 1;
	}

	return result;
}

void h264_ini_recfg(struct h264_cfg_t *enc_cfg, struct h264_ctl_t *enc_ctl, struct h264_rc_ctl_t *rc_ctl){
	enc_ctl->frm_qp    = enc_cfg->ini_qp;
	enc_ctl->target_gop = ( ((uint32_t)enc_cfg->enc_bps * 1000 * (uint32_t)enc_cfg->frm_gop) / enc_cfg->frm_rate ) >> 3;  //byte target
	rc_ctl->i_qp      = enc_cfg->ini_qp;
	rc_ctl->p_qp      = enc_cfg->ini_qp;
	//--- cal I frame target byte
	rc_ctl->i_byte    = (enc_ctl->target_gop * enc_cfg->frm_ip_rate)     / (enc_cfg->frm_gop - 1 + enc_cfg->frm_ip_rate);
	rc_ctl->i_frm_max = (enc_ctl->target_gop * enc_cfg->frm_ip_rate_max) / (enc_cfg->frm_gop - 1 + enc_cfg->frm_ip_rate_max);
	rc_ctl->i_frm_min = (enc_ctl->target_gop * enc_cfg->frm_ip_rate_min) / (enc_cfg->frm_gop - 1 + enc_cfg->frm_ip_rate_min);	
}


void h264_ini_seting_cfg(struct h264_device *p_h264,struct h264_cfg_t *enc_cfg, struct h264_ctl_t *enc_ctl, struct h264_rc_ctl_t *rc_ctl,uint8_t dev_num) {
  h264_reflash_new_gop(dev_num,0);
  h264_recfg_bsp(dev_num,enc_cfg->enc_bps);
  h264_recfg_rate(dev_num,enc_cfg->frm_rate);
  h264_recfg_frm_gop(dev_num,enc_cfg->frm_gop);
  h264_recfg_ini_qp(dev_num,enc_cfg->ini_qp);
  
  enc_ctl->enc_mode  = enc_cfg->enc_mode;
  enc_ctl->frm_qp    = enc_cfg->ini_qp;
  enc_ctl->frm_width = ((enc_cfg->frm_width + 15) >> 4) << 4;   //16 pixel align
  enc_ctl->frm_height= ((enc_cfg->frm_height + 15) >> 4) << 4;  //16 pixel align
  enc_ctl->frm_luma_size = enc_ctl->frm_width * enc_ctl->frm_height;  
  enc_ctl->frm_mblines= enc_ctl->frm_height >> 4;
  enc_ctl->timeout_limit = (enc_ctl->frm_luma_size << 10) >> 17;//18
  enc_ctl->bs_byte_limit = H264_BS_SIZE-4;
  
  enc_ctl->frm_cnt    = 0;
  enc_ctl->gop_frm_cnt= 0;
  enc_ctl->idrid      = 0;
  enc_ctl->target_gop = ( ((uint32_t)enc_cfg->enc_bps * 1000 * (uint32_t)enc_cfg->frm_gop) / enc_cfg->frm_rate ) >> 3;  //byte target

  //_os_printf("frm_luma_size:%d   frm_width:%d\r\n",enc_ctl->frm_luma_size,enc_ctl->frm_width);
  if(dev_num == 1){
	//申请空间多申请了4K,主要是为了对齐使用
	  enc_ctl->ref_lu_base= ((uint32_t)h264_ref_lu_base_psram + 0xfff) & (~0xfff);    //must be 4KB aligned
	  enc_ctl->ref_lu_end = ((enc_ctl->ref_lu_base + enc_ctl->frm_luma_size + (48*enc_ctl->frm_width) + 4095) >> 12) << 12;  //4KB align
	  enc_ctl->ref_ch_base= ((uint32_t)h264_ref_ch_base_psram + 0xfff)& (~0xfff);//enc_ctl->ref_lu_end;
	  enc_ctl->ref_ch_end = ((enc_ctl->ref_ch_base + (enc_ctl->frm_luma_size >>1) + (24*enc_ctl->frm_width) + 4095) >> 12) << 12; //4KB align
  }else if(dev_num == 2){
	  enc_ctl->ref_lu_base= ((uint32_t)h264_ref_lu_base_psram2 + 0xfff) & (~0xfff);    //must be 4KB aligned
	  enc_ctl->ref_lu_end = ((enc_ctl->ref_lu_base + enc_ctl->frm_luma_size + (48*enc_ctl->frm_width) + 4095) >> 12) << 12;  //4KB align
	  enc_ctl->ref_ch_base= ((uint32_t)h264_ref_ch_base_psram2 + 0xfff)& (~0xfff);//enc_ctl->ref_lu_end;
	  enc_ctl->ref_ch_end = ((enc_ctl->ref_ch_base + (enc_ctl->frm_luma_size >>1) + (24*enc_ctl->frm_width) + 4095) >> 12) << 12; //4KB align
  }

  
  enc_ctl->last_lpf_lu_base = enc_ctl->ref_lu_base;
  enc_ctl->last_lpf_ch_base = enc_ctl->ref_ch_base;
  
//  enc_ctl->bs_base0		= (uint32_t)h264_dat_buf[0];//h264_dat_buf + (1<<20) - 1024;
//  enc_ctl->bs_base1		= (uint32_t)h264_dat_buf[1];//enc_ctl->bs_base0 + 16*1024;

  enc_ctl->enc_first_frm=1;
  
  rc_ctl->frm_rdcost= (enc_ctl->frm_luma_size >> 8) * (256*48 >> 4);//not critial initial value
  rc_ctl->i_qp      = enc_cfg->ini_qp;
  rc_ctl->p_qp      = enc_cfg->ini_qp;
  
  //--- cal I frame target byte
  rc_ctl->i_byte    = (enc_ctl->target_gop * enc_cfg->frm_ip_rate)     / (enc_cfg->frm_gop - 1 + enc_cfg->frm_ip_rate);
  rc_ctl->i_frm_max = (enc_ctl->target_gop * enc_cfg->frm_ip_rate_max) / (enc_cfg->frm_gop - 1 + enc_cfg->frm_ip_rate_max);
  rc_ctl->i_frm_min = (enc_ctl->target_gop * enc_cfg->frm_ip_rate_min) / (enc_cfg->frm_gop - 1 + enc_cfg->frm_ip_rate_min);
  
}

void h264_enc_eof_update(struct h264_cfg_t *enc_cfg, struct h264_ctl_t *enc_ctl, struct h264_rc_ctl_t *rc_ctl) {
  
  uint8    rc_p_qp;
  uint8    i_qp;
  uint32   frm_target_step;
  uint32   i_byte;
  
  
  enc_ctl->frm_cnt ++;
  enc_ctl->gop_frm_cnt ++;
  enc_ctl->enc_first_frm = 0;
    
  if(enc_ctl->gop_frm_cnt == enc_cfg->frm_gop) {
      enc_ctl->gop_frm_cnt = 0;
      enc_ctl->idrid = (enc_ctl->idrid + 1) & 0x07;
      
      if(enc_cfg->rc_en) {
        //--- I frame target byte update
        rc_p_qp= (rc_ctl->p_qp_acc + (rc_ctl->p_acc_num >> 1)) / rc_ctl->p_acc_num;
        frm_target_step = (rc_ctl->i_byte >> 4) + (rc_ctl->i_byte >> 5);  //about 10%
        i_qp = rc_ctl->i_qp;
        i_byte= rc_ctl->i_byte;
        
        if(i_qp > rc_p_qp) {            //need add more byte to I frame
            if(i_qp >= (rc_p_qp + 3))
                i_byte += 2*frm_target_step;
            else if(i_qp > (rc_p_qp + 1))
                i_byte += frm_target_step;
        } else if(i_qp < rc_p_qp) {     //need add more byte to P frame
            if(i_qp <= (rc_p_qp - 3))
                i_byte -= 2*frm_target_step;
            else if(i_qp < (rc_p_qp - 1))
                i_byte -= frm_target_step;
        }
        
        //clip
        if(i_byte > rc_ctl->i_frm_max)
          i_byte = rc_ctl->i_frm_max;
        else if(i_byte < rc_ctl->i_frm_min)
          i_byte = rc_ctl->i_frm_min;
        
        rc_ctl->i_byte = i_byte;      
      } 
  }
  
}

void h264_rc_tune_eof(struct h264_device*p_h264,struct h264_cfg_t *enc_cfg, struct h264_ctl_t *enc_ctl, struct h264_rc_ctl_t *rc_ctl) {

  uint32   rdata;
  uint32   bs_len          = enc_ctl->enc_bs_byte;
  uint32   frm_target      = enc_ctl->frm_target;
  uint32   frm_target_step = (enc_ctl->frm_target >> 4) + (enc_ctl->frm_target >> 5);  //about 10%

  rc_ctl->gop_remain_byte -= enc_ctl->enc_bs_byte;
  if(rc_ctl->gop_remain_byte <=0)
    rc_ctl->gop_remain_byte = 1;

  //--- just accumulate the average QP for last 4 P frame within a GOP
  if((rc_ctl->gop_remain_frame <= 3) && (enc_ctl->frm_type == 0)) {
      rc_ctl->p_qp_acc += enc_ctl->hw_cal_qp;
      rc_ctl->p_acc_num++;
  }
  
  //--- next frame QP modify for I/P frames
  if(enc_ctl->frm_type == 2) {
      rc_ctl->i_qp = enc_ctl->hw_cal_qp;
      //must within [16, 50] range, no need clip
      
  } else {
    uint8  hw_cal_qp = enc_ctl->hw_cal_qp;
    
    if(enc_cfg->rc_effort == 1) {
      //high effort
      if(bs_len > (frm_target + 1*frm_target_step)) {       //real > target
          if(bs_len > (frm_target + 4*frm_target_step))         //>= 1.4*target
              hw_cal_qp += 3;
          else if(bs_len > (frm_target + 3*frm_target_step))    //>= 1.3*target
              hw_cal_qp += 3;
          else if(bs_len > (frm_target + 2*frm_target_step))    //>= 1.2*target
              hw_cal_qp += 2;
          else if(bs_len >= (frm_target + 1*frm_target_step))   //>= 1.1*target
              hw_cal_qp += 1;
      } else if(bs_len < (frm_target - 1*frm_target_step)) {//real < target
          if(bs_len <= (frm_target - 3*frm_target_step))        //<= 0.7*target
              hw_cal_qp -= 2;
          else if(bs_len < (frm_target - 1*frm_target_step))    //<= 0.9*target
              hw_cal_qp -= 1;
      }
    } else {
      //low or relax mode
      //start QP change between each P frame is [-2, 3]
      if((hw_cal_qp >= (rc_ctl->p_qp + 3))) {
        hw_cal_qp = rc_ctl->p_qp + 3;
      } else if((hw_cal_qp <= (rc_ctl->p_qp - 3))) {
        hw_cal_qp = rc_ctl->p_qp - 2;
      }
    }
  
    //clip to [16, 50]
    if(hw_cal_qp > 50)
        hw_cal_qp = 50;
    else if(hw_cal_qp < 16)
        hw_cal_qp = 16;  
  
    rc_ctl->p_qp = hw_cal_qp;
  }

  if(enc_cfg->rc_effort == 2) {
    rdata   = h264_get_enc_frame_rdcost(p_h264);
    rc_ctl->frm_rdcost = (rdata * enc_cfg->rc_grp + (enc_ctl->frm_mblines >> 1)) / ((uint32)enc_ctl->frm_mblines);       
  } 
}


void h264_read_bs(struct str_info *str)
{
	uint8_t    bs    ;
	uint8_t    sf_bit  ;

	//make sure this is str->bit_len >=25
	while(str->bit_len <= 24) {
		bs      = *(str->ptr);
		str->ptr  += 1;
		sf_bit    = 24 - str->bit_len;
		str->stream = str->stream | (((uint32_t) bs) << sf_bit);
		str->bit_len+= 8;
	}
}

void h264_byte_align(struct str_info *str)
{
	uint8_t    sf_bit  ;
	sf_bit  = str->bit_len & 0x07;
	if(sf_bit != 0) {
		str->stream = str->stream << sf_bit;
		str->bit_len -= sf_bit;
		str->used_bits += sf_bit;
		h264_read_bs(str);
	}
}

uint32_t dec_fix(struct str_info *str, uint8_t bit_num)
{
	uint32_t   val     ;
	uint8_t    sf_bit  ;

	if(bit_num > 25) {
	//print("dec_fix error, bit_num is too big.\n\r");
	}

	sf_bit= 32 - bit_num;
	val   = str->stream >> sf_bit;
	str->stream = str->stream << bit_num;
	str->bit_len -= bit_num;
	str->used_bits += bit_num;
	h264_read_bs(str);

	return(val);
}

int32_t dec_glb(struct str_info *str, uint8_t mode)
{
	uint8_t    leading_0 = 0;
	uint16_t   tmp_val   = 0;
	uint16_t   tail_val  ;
	int32_t   syn       ;

	//-- find leading zero
	while (((str->stream >> 31) & 0x01) == 0x0) {
		str->stream = str->stream << 1;
		str->bit_len -= 1;
		str->used_bits += 1;
		leading_0++;
	}

	//--- shift 1
	str->stream = str->stream << 1;
	str->bit_len -= 1;
	str->used_bits += 1;

	h264_read_bs(str);

	//--- get tail
	if(leading_0 == 0)
		tail_val = 0;
	else
		tail_val = (uint16_t) (str->stream >> (32 - leading_0));

	str->stream = str->stream << leading_0;
	str->bit_len -= leading_0;
	str->used_bits += leading_0;

	tmp_val   = (1 << leading_0) - 1 + tail_val;
	h264_read_bs(str);

	if(mode == 0)
	syn = tmp_val;
	else {
	if(tmp_val == 0)
	  syn = 0;
	else if(tmp_val & 0x01)
	  syn = (tmp_val + 1) >> 1;
	else
	  syn = 0 - (tmp_val >> 1);
	}

	return(syn);

}


void dec_pic(struct str_info *str, struct h264_header *head)
{
	uint32_t   syn_unsign;
	int32_t   syn_sign;
	uint8_t    slice_type;
	uint8_t    ref_pic_list_modification_flag_l0;
	uint8_t    modify_of_pic_muns_idc;
	int16_t   qp_delta;


	syn_sign  = dec_glb(str, 0);    //first_mb_in_slice

	syn_sign  = dec_glb(str, 0);    //slice_type
	slice_type  = syn_sign;
	head->pic_slice_type = slice_type;

	syn_sign  = dec_glb(str, 0);    //pps_id
	syn_unsign  = dec_fix(str, (head->sps_log2_max_fnum_minus4 + 4)); //frame_num

	if(head->idr_flag) {
	syn_sign  = dec_glb(str, 0);  //idr_pic_id
	}

	syn_unsign  = dec_fix(str, (head->sps_log2_max_poc_lsb_minus4 + 4));  //pic_order_cnt_lsb

	if(slice_type == 0) {
	syn_unsign  = dec_fix(str, 1);  //num_ref_idx_active_override_flag
	}

	if(slice_type == 0) {
	syn_unsign  = dec_fix(str, 1);  //ref_pic_list_modification_flag_l0
	ref_pic_list_modification_flag_l0 = syn_unsign;
	} else {
	ref_pic_list_modification_flag_l0 = 0;
	}

	if(ref_pic_list_modification_flag_l0) {
	modify_of_pic_muns_idc = 0;
	while(modify_of_pic_muns_idc != 3) {
	  syn_sign  = dec_glb(str, 0);    //modify_of_pic_nums_idc
	  modify_of_pic_muns_idc = syn_sign;

	  if(modify_of_pic_muns_idc == 0 || modify_of_pic_muns_idc == 1) {
	    syn_sign  = dec_glb(str, 0);  //abs_diff_pic_num_minus1
	  }
	}
	}

	if(head->idr_flag) {
	syn_unsign  = dec_fix(str, 1);  //no_output_of_prior_pics_flag
	syn_unsign  = dec_fix(str, 1);  //long_term_reference_flag
	} else {
	syn_unsign  = dec_fix(str, 1);  //adaptive_ref_pic_marking_mode_flag
	}

	if((head->pps_cavlc_mode == 0) && (head->idr_flag == 0)) {
	syn_unsign  = dec_glb(str, 0);  //cabac_init_idc
	}

	syn_sign  = dec_glb(str, 1);    //slice_qp_delta
	qp_delta  = syn_sign;
	head->pic_qp= (uint8_t) ((int8_t)head->pps_qp_ini + qp_delta);

	if(head->pps_cavlc_mode == 0) {   //cabac mode, align to 8bit
		str->used_bits = ((str->used_bits + 7) >> 3) << 3; 
	}
}

void dec_sps(struct str_info *str, struct h264_header *head)
{
	uint32_t   syn_unsign;
	int32_t   syn_sign;

	syn_unsign  = dec_fix(str, 8);  //profile_idc
	syn_unsign  = dec_fix(str, 8);  //
	syn_unsign  = dec_fix(str, 8);  //level_idc
	syn_sign  = dec_glb(str, 0);  //sps_id

	syn_sign  = dec_glb(str, 0);  //log2_max_frame_num_minus4
	head->sps_log2_max_fnum_minus4 = syn_sign;

	syn_sign  = dec_glb(str, 0);  //pic_order_cnt_type

	syn_sign  = dec_glb(str, 0);  //log2_max_pic_order_cnt_lsb_minus4
	head->sps_log2_max_poc_lsb_minus4 = syn_sign;

	syn_sign  = dec_glb(str, 0);  //max_mun_ref_frames
	syn_unsign  = dec_fix(str, 1);  //gaps_in_frame_num_value_allowed_flag

	syn_sign  = dec_glb(str, 0);  //pic_width_in_mbs_minus1
	head->sps_pic_width = (syn_sign + 1) * 16;

	syn_sign  = dec_glb(str, 0);  //pic_height_in_map_units_minus1
	head->sps_pic_height= (syn_sign + 1) * 16;

	h264_byte_align(str);
}

void dec_pps(struct str_info *str, struct h264_header *head)
{
	uint32_t   syn_unsign;
	int32_t   syn_sign;

	syn_sign  = dec_glb(str, 0);    //pps_id
	syn_sign  = dec_glb(str, 0);    //sps_id
	syn_unsign  = dec_fix(str, 1);    //entropy_coding_mode_flag
	head->pps_cavlc_mode = (syn_unsign == 0)? 1 : 0;

	syn_unsign  = dec_fix(str, 1);    //bottom_filed_pic_order_in_frame...
	syn_sign  = dec_glb(str, 0);    //num_slice_groups_minus1
	syn_sign  = dec_glb(str, 0);    //num_ref_idx_l0_default_active_minus1
	syn_sign  = dec_glb(str, 0);    //num_ref_idx_l1_default_active_minus1
	syn_unsign  = dec_fix(str, 1);    //weighted_pred_flag
	syn_unsign  = dec_fix(str, 2);    //weighted_bipred_idc

	syn_sign  = dec_glb(str, 1);    //pic_init_qp_minus26
	head->pps_qp_ini = syn_sign + 26;

	syn_sign  = dec_glb(str, 1);    //pic_init_qs_minus26

	syn_sign  = dec_glb(str, 1);    //chroma_qp_index_offset
	head->pps_chrom_qp_offset = syn_sign;

	h264_byte_align(str);
}

uint8_t nal_parse(struct str_info *str, struct h264_header *head)
{
	uint32_t   syn_unsign;
	uint8_t    nal_type;

	//initial read in 4 byte
	str->used_bits = 0;
	str->bit_len = 0;
	str->stream =0;
	h264_read_bs(str);

	//--- find the 0x00000001 nal start code
	while( str->stream != 0x00000001) {
		str->stream = str->stream << 8;
		str->bit_len -= 8;
		str->used_bits += 8;
		h264_read_bs(str);
	}

	//discard 0x00000001 start code
	str->bit_len = 0;
	str->used_bits += 32;
	str->stream = 0;
	h264_read_bs(str);

	syn_unsign  = dec_fix(str, 3);
	syn_unsign  = dec_fix(str, 5);
	nal_type  = syn_unsign;
	h264_read_bs(str);
	if(nal_type == pic_idr) {
		head->idr_flag = 1;
		dec_pic(str, head);
	} else if(nal_type == pic_data) {
		head->idr_flag = 0;
		dec_pic(str, head);
	} else if(nal_type == sps) {
		dec_sps(str, head);
	} else if(nal_type == pps) {
		dec_pps(str, head);
	} else {
		_os_printf("Unsupported NAL Unit type.\n\r");
	}

	return(nal_type);
}

void  h264_dec_flag_chk(uint32_t flags) {
  if((flags & 0x10) != 0) {	//check dec_err_flag
  	_os_printf("decoder syntax error.\n\r");
  }
  
  if((flags & 0x100) != 0) {//check bitstream read length error flag
  	_os_printf("decoder stream length error.\n\r");
  }
}

//return ptr point to byte location after 0x00000001
uint8_t* find_start_code(uint8_t* ptr) {

	struct str_info   str ;

	str.ptr= ptr;
	str.bit_len=0;
	str.used_bits=0;
	str.stream=0;

	//initial with 32bit
	h264_read_bs(&str);

	//--- find the 0x00000001 nal start code
	while(str.stream != 0x00000001) {
		str.stream  = str.stream << 8;
		str.bit_len -= 8;
		str.used_bits += 8;
		h264_read_bs(&str);

		//debug only
		//if((uint32_t)str.ptr >= (H264_FILE_BS_BASE + bs_size)) {
		//  print("Start code not find.\n\r");
		//}
	}
	return(str.ptr);
}

void  h264_enc_flag_chk(uint32 flags) {
  
}


void h264_clr_intr(struct h264_device *p_h264) {
  h264_set_sta(p_h264,0);
}

void h264_cfg_srcdat(struct h264_device *p_h264,uint8_t mode){
	h264_set_src_from(p_h264,mode); 
}

void h264_get_intr_status(struct h264_device *p_h264,struct h264_cfg_t *enc_cfg, struct h264_ctl_t *enc_ctl, struct h264_rc_ctl_t *rc_ctl) {
  uint32   rdata ;
  rdata = h264_get_sta(p_h264);
  enc_ctl->enc_end_flags= rdata;
  
  if(enc_cfg->rc_en) {
    //rate control info
    rdata = h264_get_rate_ctl(p_h264);
    rdata = rdata >> 16;
    enc_ctl->hw_cal_qp = (rdata + (enc_ctl->frm_mblines >> 1)) / (enc_ctl->frm_mblines);    
  }
}

uint32  h264_cal_ref_base(uint32 buf_base, uint32 buf_end, uint32 last_base, uint32 last_offset) {
  uint32   addr  ;

  addr  = ((last_base + last_offset + 4095) >> 12) << 12; //4KB align
  //ring-buffer
  if(addr >= buf_end) {
    addr = addr - buf_end;
    addr = buf_base + addr;
  }
  return(addr);
}

//--- cal RC MB Line target byte for IPPPPPPP.... GOP mode
//--- target bitrate is traced through a GOP
uint32 h264_rc_cal(struct h264_cfg_t *enc_cfg, struct h264_ctl_t *enc_ctl, struct h264_rc_ctl_t *rc_ctl)
{

	uint32 grp_byte      ;   //target byte number of a RC MB line group
	uint32 frm_byte      ;   //target byte number of a frame

	if(enc_ctl->frm_type == 2) {  //I frame, first Intra frame in a GOP
	    rc_ctl->gop_remain_frame = enc_cfg->frm_gop;
	    rc_ctl->p_qp_acc  = 0;
	    rc_ctl->p_acc_num = 0;
	    rc_ctl->p_avg     = (rc_ctl->gop_remain_byte - rc_ctl->i_byte) / (enc_cfg->frm_gop - 1);
	    rc_ctl->p_frm_max = rc_ctl->p_avg + (rc_ctl->p_avg >>3);    //+0.125 times variation within a GOP
	    rc_ctl->p_frm_min = rc_ctl->p_avg - (rc_ctl->p_avg >>3);    //-0.125 times variation within a GOP
	    frm_byte          = rc_ctl->i_byte;
	} else {      //other P frame in a GOP
	    frm_byte = rc_ctl->gop_remain_byte / rc_ctl->gop_remain_frame;
	    
	    if(frm_byte > rc_ctl->p_frm_max)
	      frm_byte = rc_ctl->p_frm_max;
	    else if(frm_byte < rc_ctl->p_frm_min)
	      frm_byte = rc_ctl->p_frm_min;
	}

	if(enc_cfg->rc_effort != 2)     //low/high-effort
	  frm_byte += (frm_byte >> 4);  //add 6% more to compensate the HW inaccurate

	enc_ctl->frm_target = frm_byte;
	grp_byte = (frm_byte*enc_cfg->rc_grp + (enc_ctl->frm_mblines >> 1)) / enc_ctl->frm_mblines;

	rc_ctl->gop_remain_frame --;
	return (grp_byte);
}

extern volatile uint32_t vpp_md_cnt;
void h264_start_enc_frm(struct h264_device *p_h264,struct h264_cfg_t *penc_cfg, struct h264_ctl_t *enc_ctl, struct h264_rc_ctl_t *rc_ctl) {
  uint32   rc_grp_byte     ;
  //uint32   frm_target_step ;
  uint32   lpf_lu_base     ;
  uint32   lpf_ch_base     ;
  uint32   wdata           ;
  uint32   rdata           ;
  uint32   con             ;
//  static  uint8_t  frm_num = 2;
  h264_frame* hf;
  uint32 addr;
  h264_error = 0;
  if(enc_ctl->gop_frm_cnt == 0) {
	if(penc_cfg->auto_ctl_3dnr == 1){
		if(penc_cfg->md_3dnr_timer == 0){
			if(penc_cfg->last_vpp_md != vpp_md_cnt){
			  if(penc_cfg->md_set_3dnr == 0){
				  penc_cfg->md_3dnr_en = penc_cfg->flt3d_en;
				  penc_cfg->md_set_3dnr = 1;
			  }
			  penc_cfg->flt3d_en = 0;		 //发现有在动,3d降噪先关闭
			  penc_cfg->md_3dnr_timer = 3;
			}else{
				penc_cfg->md_set_3dnr = 0;
				penc_cfg->flt3d_en = penc_cfg->md_3dnr_en;
			}
			penc_cfg->last_vpp_md = vpp_md_cnt;
		}else{
			penc_cfg->md_3dnr_timer--;
		}
	}	

  	if(penc_cfg->flt3d_en){
		if(penc_cfg->run_gop_3dnr == 0){
			enc_ctl->enc_first_frm = 1;
  			penc_cfg->run_gop_3dnr = penc_cfg->gop_rst_3dnr;
		}
		else
			penc_cfg->run_gop_3dnr--;
	}
	
    enc_ctl->frm_type = 2;
    rc_ctl->gop_remain_byte = enc_ctl->target_gop;
    enc_ctl->frm_qp   = rc_ctl->i_qp;
  } else {	
    enc_ctl->frm_type = 0;
    enc_ctl->frm_qp   = rc_ctl->p_qp;
  }

  	// --- ISP denoise turning ---
  	if(penc_cfg->md_3dnr_timer == 0 && penc_cfg->last_vpp_md == vpp_md_cnt){
		if ((penc_cfg->flt_noise_lev != penc_cfg->flt_noise_lev_last) || (penc_cfg->flt3d_en != penc_cfg->flt3d_en_last))
		{
			h264_3dnr_ini(p_h264, penc_cfg);
			enc_ctl->enc_first_frm = 1;
			penc_cfg->flt_noise_lev_last = penc_cfg->flt_noise_lev;
			penc_cfg->flt3d_en			 = penc_cfg->flt3d_en_last;
			//_os_printf("h264 nr: %d %d\r\n", penc_cfg->flt3d_en, penc_cfg->flt_noise_lev);
		}
	}    

  //--- write cfg regs
  
  wdata = enc_ctl->frm_type | (1 << 3) | (enc_ctl->enc_mode << 4) | (penc_cfg->enc_bs_buf_size << 16);
  con               = wdata;
  h264_set_frame_base_cfg(p_h264,enc_ctl->frm_type,enc_ctl->enc_mode,penc_cfg->enc_bs_buf_size,1); 
  h264_set_phy_img_size(p_h264,enc_ctl->frm_width,enc_ctl->frm_height);

  if(penc_cfg->rc_en){
	  h264_set_frame_qp(p_h264,enc_ctl->frm_qp);
  }
  else{
	  h264_set_frame_qp(p_h264,penc_cfg->ini_qp);
  }
  
  //--- cfg reg not change(if frame x/y size change, may need change)
  h264_set_timeout_limit(p_h264,enc_ctl->timeout_limit);
  h264_set_len_limit(p_h264,enc_ctl->bs_byte_limit);
#if H264_I_ONLY  
  h264_set_ref_lu_addr(p_h264,0X40000000,0X40000000);
  h264_set_ref_chro_addr(p_h264,0X40000000,0X40000000);

  //h264_set_ref_lu_addr(p_h264,enc_ctl->ref_lu_base,enc_ctl->ref_lu_base+(IMAGE_W_H264*(IMAGE_H_H264+48))/2);
  //h264_set_ref_chro_addr(p_h264,enc_ctl->ref_lu_base,enc_ctl->ref_lu_base+(IMAGE_W_H264*(IMAGE_H_H264+48))/2);  
#else
  h264_set_ref_lu_addr(p_h264,enc_ctl->ref_lu_base,enc_ctl->ref_lu_end);
  h264_set_ref_chro_addr(p_h264,enc_ctl->ref_ch_base,enc_ctl->ref_ch_end);
#endif

  h264_set_frame_num(p_h264,enc_ctl->gop_frm_cnt);  
  h264_set_enc_cnt(p_h264,enc_ctl->gop_frm_cnt);
  h264_set_enc_frame_id(p_h264,enc_ctl->idrid);
  
  h264_f_p		= get_h264_new_frame_head(1,H264_DEV_0_ID,con&BIT(1)); 							  //预先分配第一frame，待帧结束的时候再分配下一frame 		  
  h264_module_p = h264_f_p;   
  h264_module_p = get_node(h264_module_p,&h264_free_tab);
  if(h264_module_p == NULL){
	  _os_printf("need more node for new h264 frame start1\r\n");
	  while(1);
  }
  addr = get_addr(h264_module_p);
  h264_set_buf_addr(p_h264,addr);
  //sys_dcache_clean_invalid_range((uint32_t *)addr, H264_NODE_LEN);
  h264_module_p = get_node(h264_module_p,&h264_free_tab);
  if(h264_module_p == NULL){
	  _os_printf("need more node for new h264 frame start2\r\n");
	  while(1);
  }  
  addr = get_addr(h264_module_p);   
  h264_set_buf_addr(p_h264,addr);
  //sys_dcache_clean_invalid_range((uint32_t *)addr, H264_NODE_LEN);

  hf = list_entry((struct list_head *)h264_f_p,h264_frame,list);
  //hf->h264_num = frm_num;
  hf->h264_num = penc_cfg->count;
  penc_cfg->count++;
  //Xil_Out32((H264_REG_BASE + 12*4), (enc_ctl->frm_luma_size) >> 8); //MB number of a frame
  //p_h264->FRM_MB_NUM = (enc_ctl->frm_luma_size) >> 8;
  h264_set_frame_mb_num(p_h264,enc_ctl->frm_luma_size);
  //--- ref/lpf buf base
  //--- cal lpf write base
  lpf_lu_base = h264_cal_ref_base(enc_ctl->ref_lu_base, enc_ctl->ref_lu_end, enc_ctl->last_lpf_lu_base, enc_ctl->frm_luma_size);
  lpf_ch_base = h264_cal_ref_base(enc_ctl->ref_ch_base, enc_ctl->ref_ch_end, enc_ctl->last_lpf_ch_base, (enc_ctl->frm_luma_size >> 1));

#if H264_I_ONLY   
  h264_set_ref_wb_addr(p_h264,0X40000000,0X40000000);
  h264_set_ref_rb_addr(p_h264,0X40000000,0X40000000);

  //h264_set_ref_wb_addr(p_h264,enc_ctl->ref_lu_base,enc_ctl->ref_lu_base);
  //h264_set_ref_rb_addr(p_h264,enc_ctl->ref_lu_base,enc_ctl->ref_lu_base);  
#else
  h264_set_ref_wb_addr(p_h264,lpf_lu_base,lpf_ch_base);
  h264_set_ref_rb_addr(p_h264,enc_ctl->last_lpf_lu_base,enc_ctl->last_lpf_ch_base);
#endif
  enc_ctl->last_lpf_lu_base = lpf_lu_base;
  enc_ctl->last_lpf_ch_base = lpf_ch_base;
  
  //--- rate contrl
  wdata = ((penc_cfg->rc_grp - 1)<<0) | (penc_cfg->rc_effort << 4) | (penc_cfg->rc_en <<7);
  //Xil_Out32((H264_REG_BASE + 16*4), wdata);
  //p_h264->RC_CTL   = wdata;
  h264_set_frame_rate_ctl(p_h264,penc_cfg->rc_grp,penc_cfg->rc_effort,penc_cfg->rc_en);

  
  
  if(penc_cfg->rc_en) {
    rc_grp_byte = h264_rc_cal(penc_cfg, enc_ctl, rc_ctl);    
    //p_h264->RC_BYTE   = rc_grp_byte;
    h264_set_rate_ctl_byte(p_h264,rc_grp_byte);
    if(penc_cfg->rc_effort == 2) {
      //p_h264->RC_RDCOST   = rc_ctl->frm_rdcost;
	  h264_set_rate_ctl_rdcost(p_h264,rc_ctl->frm_rdcost);
    } 
  }

  //--- 3DNR
  if(penc_cfg->flt3d_en) {
    rdata   = h264_get_flt_3dnr(p_h264);     //获取上一帧结果 
    wdata   = rdata & 0x0fffffff;
  	if(h264_dev_num == 2){
		if(penc_cfg == &enc_cfg){               //当前是主镜头，那表示上次获取到的是副镜头
			enc_2_cfg.r3dnr  = wdata;
			//os_printf("Y0:%08x cont:%d r:%d\r\n",wdata,penc_cfg->count,penc_cfg->run_gop_3dnr);
			penc_cfg->r3dnr  = enc_cfg.r3dnr;   //拿回主镜头的参数
		}else{								    //当前是副镜头，那表示上次获取到的是主镜头
			enc_cfg.r3dnr    = wdata;
			//os_printf("Y1:%08x cont:%d r:%d\r\n",wdata,penc_cfg->count,penc_cfg->run_gop_3dnr);
			penc_cfg->r3dnr  = enc_2_cfg.r3dnr; //拿回副镜头的参数
		}
	}else{
		penc_cfg->r3dnr  = wdata;
	}
	
    if(enc_ctl->enc_first_frm == 1){
		penc_cfg->r3dnr = penc_cfg->r3dnr; 		   //disable 3DNR for first frame	
	}
    else{
    	penc_cfg->r3dnr = penc_cfg->r3dnr | (3 << 28);//enable 3DNR   //结果保存下来
		
	}
	h264_set_flt_3dnr(p_h264,penc_cfg->r3dnr);
  }else{
	  penc_cfg->r3dnr = penc_cfg->r3dnr & 0x0fffffff;
	  h264_set_flt_3dnr(p_h264,penc_cfg->r3dnr);
  }

  //configure mb_pipe_cnt
  if((penc_cfg->flt3d_en == 0) && (enc_ctl->frm_type == 2))    //I frame and 3DNR disable
    wdata = ((460-2) << 16) | 460;
  else
    wdata = ((502-2) << 16) | 502; 

  //crop_en
  if((penc_cfg->frm_height != penc_cfg->wrap_height)){
	  	//p_h264->Y_CROP = BIT(31)|4;
	  	h264_set_y_crop(p_h264,1,penc_cfg->frm_height - penc_cfg->wrap_height);
  }
  else
  {
		h264_set_y_crop(p_h264,0,8);
  }

  penc_cfg->enc_runing = 1;

  //p_h264->MB_PIPE = wdata;
  h264_set_mb_pipe(p_h264,wdata);
  //configure ROI window
  //p_h264->WRAP_CON |= BIT(7);
  h264_set_sw_ready(p_h264);
//  if(con&BIT(1)){
//	  _os_printf("is iframe \r\n");
//  }else{
//	  _os_printf("is pframe \r\n");
//  }
//  ll_gpio_bit_set(GPIOD,BIT(9));
//  ll_gpio_bit_reset(GPIOD,BIT(9));
//  _os_printf("  [CON:%x ] ",con);
  //--- start encode a frame
  //enc_ctl->bs_buf_id = 0;
}

void h264_frame_done_isr(uint32 irq_flags, uint32 irq_data, uint32 param){
	h264_frame* hf;
	uint32 	  rdata;
	//uint32 	dat_ofs=0;
	uint32     bw_cnt_wr,bw_cnt_rd,enc_cnt_cyc;
	struct h264_device *p_h264 = (struct h264_device *)irq_data;

	bw_cnt_wr = h264_get_bandwidth_wr(p_h264);
	bw_cnt_rd = h264_get_bandwidth_rd(p_h264);
	enc_cnt_cyc = h264_get_cyc_done_num(p_h264);
	
	rdata	 = h264_get_frame_len(p_h264);

	//如果超过某个size,就认为图片太大,就重新生成吧
	if(rdata*3/2 > (H264_NODE_LEN*H264_NODE_NUM))
	{
		h264_set_err(p_h264,1);
		os_printf(KERN_ERR"h264 size over:%d\n",rdata);
	}
//	dat_ofs = rdata%(16*1024);
//	if(dat_atbuf == 0)
//	{
		//ll_m2m_memcpy(M2M_DMA0,frame_cache+frame_offset,h264_dat_buf[0],dat_ofs,0);
//	}else{
		//ll_m2m_memcpy(M2M_DMA0,frame_cache+frame_offset,h264_dat_buf[1],dat_ofs,0);
//	}
	//帧OK
		
	hf = list_entry((struct list_head *)h264_f_p,h264_frame,list);
	hf->frame_len = rdata;
	hf->which = penc_cfg->src_from;
	hf->srcID = video_msg.video_type_cur + FRAMEBUFF_SOURCE_CAMERA0; 
	
	//因为有预分配机制，所以要先去掉最后一个节点
	//_os_printf("bw_rd:%d   bw_wr:%d\r\n",h264_get_bandwidth_rd(p_h264),h264_get_bandwidth_wr(p_h264));
	//os_printf("---H(%d  %d)",h264_dev_num,penc_cfg->src_from);

	if(h264_module_p)
	{
		put_node(&h264_free_tab,h264_module_p->prev);
	}
	
	if(h264_is_running(p_h264) == 1){
		hf->w = penc_cfg->wrap_width;
		hf->h = penc_cfg->wrap_height;
		set_frame_ready(hf);
	}
	
	enc_ctl.enc_bs_byte = rdata;
	h264_get_intr_status(p_h264,(struct	h264_cfg_t*)penc_cfg, (struct h264_ctl_t*)penc_ctl, (struct h264_rc_ctl_t *)prc_ctl);
	h264_clr_intr(p_h264);
	
	//--- user add function to check the flags
	h264_enc_flag_chk(penc_ctl->enc_end_flags);
	
	if(penc_cfg->rc_en)
	  h264_rc_tune_eof(p_h264,(struct h264_cfg_t *)penc_cfg, (struct h264_ctl_t *)penc_ctl, (struct h264_rc_ctl_t *)prc_ctl);
	
	h264_enc_eof_update((struct h264_cfg_t *)penc_cfg, (struct h264_ctl_t *)penc_ctl, (struct h264_rc_ctl_t *)prc_ctl);

	if(h264_is_running(p_h264) == 0){
		del_264_frame(h264_f_p);
		return;
	}

	if(param || h264_is_err(p_h264)){
		os_printf("isp ov,h264 frame drop\r\n");
		h264_set_err(p_h264,0);
		del_264_frame(h264_f_p);
		if(h264_dev_num == 2){
			if(penc_ctl == &enc_ctl){
				recfg_h264_new_grop[1] = 1; 
			}else{
				recfg_h264_new_grop[0] = 1;
			}
		}else{
			recfg_h264_new_grop[0] = 1; 	  //重新配置成I帧
		}
	}

///////////////////////////////////////////////////////
		
	//p_h264->WRAP_CON |= 0x01|0x10;
///////////////////////////////////////////////////////
	if(h264_dev_num == 2){
		if(penc_ctl == &enc_ctl){
			penc_ctl = &enc_2_ctl;
			penc_cfg = &enc_2_cfg;
			prc_ctl  = &rc_2_ctl; 

			if(recfg_h264_new_grop[1] == 1){
				recfg_h264_new_grop[1] = 0;
				penc_ctl->gop_frm_cnt = 0;
			}
			
			if(penc_ctl->gop_frm_cnt == 0){               //I帧再做调整，保证上个gop完成
				if(h264_cfg_modify((struct h264_cfg_t *)penc_cfg,2)){
					h264_ini_recfg((struct h264_cfg_t *)penc_cfg,(struct h264_ctl_t *)penc_ctl,(struct h264_rc_ctl_t *)prc_ctl);
				}
			}
		}else{
			penc_ctl = &enc_ctl;
			penc_cfg = &enc_cfg;
			prc_ctl  = &rc_ctl;

			if(recfg_h264_new_grop[0] == 1){
				recfg_h264_new_grop[0] = 0;
				penc_ctl->gop_frm_cnt = 0;
			}
			
			if(penc_ctl->gop_frm_cnt == 0){
				if(h264_cfg_modify((struct h264_cfg_t *)penc_cfg,1)){
					h264_ini_recfg((struct h264_cfg_t *)penc_cfg,(struct h264_ctl_t *)penc_ctl,(struct h264_rc_ctl_t *)prc_ctl);
				}
			}
		}
	}else{
		if(recfg_h264_new_grop[0] == 1){
			recfg_h264_new_grop[0] = 0;
			penc_ctl->gop_frm_cnt = 0;
		}

		if(penc_ctl->gop_frm_cnt == 0){
			if(h264_cfg_modify((struct h264_cfg_t *)penc_cfg,1)){
				h264_ini_recfg((struct h264_cfg_t *)penc_cfg,(struct h264_ctl_t *)penc_ctl,(struct h264_rc_ctl_t *)prc_ctl);
			}
		}
	}	
	#if 0
	if(penc_cfg->frm_height == 1088){
		h264_set_wrap_img_size(p_h264,penc_cfg->frm_width,(penc_cfg->frm_height-8));
	}else{
		h264_set_wrap_img_size(p_h264,penc_cfg->frm_width,penc_cfg->frm_height);
	}
	#else
	h264_set_wrap_img_size(p_h264,penc_cfg->wrap_width,penc_cfg->wrap_height);
	#endif
	h264_start_enc_frm(p_h264,(struct h264_cfg_t *)penc_cfg, (struct h264_ctl_t *)penc_ctl, (struct h264_rc_ctl_t *)prc_ctl);
	enc_ctl.bs_buf_id = 0;	
	if(penc_cfg->src_from == GEN420_DATA)
	{
		//唤醒任务去kick gen420
		extern void *get_vpp_psram_buf();
		wake_up_gen420_queue(1,get_vpp_psram_buf());
	}else{
		h264_cfg_srcdat(p_h264,penc_cfg->src_from);
	}	
	_os_printf(KERN_DEBUG"Z%d",penc_cfg->src_from);

}

void h264_frame_done_norekick_isr(uint32 irq_flags, uint32 irq_data, uint32 param){
	h264_frame* hf;
	uint32 	  rdata;
	//uint32 	dat_ofs=0;
	uint32     bw_cnt_wr,bw_cnt_rd,enc_cnt_cyc;
	struct h264_device *p_h264 = (struct h264_device *)irq_data;

	bw_cnt_wr = h264_get_bandwidth_wr(p_h264);
	bw_cnt_rd = h264_get_bandwidth_rd(p_h264);
	enc_cnt_cyc = h264_get_cyc_done_num(p_h264);
	
	rdata	 = h264_get_frame_len(p_h264);
//	dat_ofs = rdata%(16*1024);
//	if(dat_atbuf == 0)
//	{
		//ll_m2m_memcpy(M2M_DMA0,frame_cache+frame_offset,h264_dat_buf[0],dat_ofs,0);
//	}else{
		//ll_m2m_memcpy(M2M_DMA0,frame_cache+frame_offset,h264_dat_buf[1],dat_ofs,0);
//	}
	//帧OK
		
	hf = list_entry((struct list_head *)h264_f_p,h264_frame,list);
	hf->frame_len = rdata;
	hf->which = penc_cfg->src_from;
	hf->srcID = video_msg.video_type_cur + FRAMEBUFF_SOURCE_CAMERA0; 
	
	//因为有预分配机制，所以要先去掉最后一个节点
	//_os_printf("bw_rd:%d   bw_wr:%d\r\n",h264_get_bandwidth_rd(p_h264),h264_get_bandwidth_wr(p_h264));
	//os_printf("---H(%d  %d)",h264_dev_num,penc_cfg->src_from);

	put_node(&h264_free_tab,h264_module_p->prev);
	if(h264_is_running(p_h264) == 1){
		set_frame_ready(hf);
	}
	
	enc_ctl.enc_bs_byte = rdata;
	h264_get_intr_status(p_h264,(struct	h264_cfg_t*)penc_cfg, (struct h264_ctl_t*)penc_ctl, (struct h264_rc_ctl_t *)prc_ctl);
	h264_clr_intr(p_h264);
	
	//--- user add function to check the flags
	h264_enc_flag_chk(penc_ctl->enc_end_flags);
	
	if(penc_cfg->rc_en)
	  h264_rc_tune_eof(p_h264,(struct h264_cfg_t *)penc_cfg, (struct h264_ctl_t *)penc_ctl, (struct h264_rc_ctl_t *)prc_ctl);
	
	h264_enc_eof_update((struct h264_cfg_t *)penc_cfg, (struct h264_ctl_t *)penc_ctl, (struct h264_rc_ctl_t *)prc_ctl);

	if(h264_is_running(p_h264) == 0){
		del_264_frame(h264_f_p);
		return;
	}

	if(param){
		os_printf("isp ov,h264 frame drop\r\n");
		del_264_frame(h264_f_p);
		if(h264_dev_num == 2){
			if(penc_ctl == &enc_ctl){
				recfg_h264_new_grop[1] = 1; 
			}else{
				recfg_h264_new_grop[0] = 1;
			}
		}else{
			recfg_h264_new_grop[0] = 1; 	  //重新配置成I帧
		}
	}
	penc_cfg->enc_runing = 0;
	os_printf(KERN_DEBUG"Z%d",penc_cfg->src_from);
	h264_sema_up();
}


void h264_sema_init()
{
	if(h264_sem.magic == 0){
		os_sema_init(&h264_sem,0);
	}
}

int32 h264_sema_down(int32 tmo_ms)
{
	int32 ret;
	ret = os_sema_down(&h264_sem,tmo_ms);
	return ret;
}

void h264_sema_up()
{
	os_sema_up(&h264_sem);
}

void h264_drv_init(){
	struct h264_device *p_h264;
	p_h264 = (struct h264_device *)dev_get(HG_H264_DEVID);

	h264_sema_init();
	h264_init(p_h264,H264_HARDWARE_CLK);
}


void h264_frame_dec_done_isr(uint32 irq_flags, uint32 irq_data, uint32 param){
	//struct h264_device *p_h264 = (struct h264_device *)irq_data;
	//frame_done = 1;
	h264_sema_up();
	//_os_printf("h264 dec end\r\n");
}


void h264_buf0_done_isr(uint32 irq_flags, uint32 irq_data, uint32 param){
	struct h264_device *p_h264 = (struct h264_device *)irq_data;
	uint32 addr;
	//_os_printf("B0");
	h264_module_p = get_node(h264_module_p,&h264_free_tab);
	if(h264_module_p == NULL){
		_os_printf("need more node for new h264 frame start\r\n");
		h264_set_err(p_h264,1);
		return ;
	}
	addr = get_addr(h264_module_p);
	h264_set_buf_addr(p_h264,addr);
	//sys_dcache_clean_invalid_range((uint32_t *)addr, H264_NODE_LEN);
	//_os_printf("B0");
	enc_ctl.bs_buf_id = (penc_ctl->bs_buf_id + 1) & 0x01;	
}

void h264_buf1_done_isr(uint32 irq_flags, uint32 irq_data, uint32 param){
	struct h264_device *p_h264 = (struct h264_device *)irq_data;
	uint32 addr;
	//_os_printf("B1");
	h264_module_p = get_node(h264_module_p,&h264_free_tab);
	if(h264_module_p == NULL){
		_os_printf("need more node for new h264 frame start\r\n");
		h264_set_err(p_h264,1);
		return ;
	}
	addr = get_addr(h264_module_p);
	h264_set_buf_addr(p_h264,addr);
	//sys_dcache_clean_invalid_range((uint32_t *)addr, H264_NODE_LEN);
	//_os_printf("B1");
	enc_ctl.bs_buf_id = (penc_ctl->bs_buf_id + 1) & 0x01;	
}

void h264_frame_fast_isr(uint32 irq_flags, uint32 irq_data, uint32 param){
	//struct h264_device *p_h264 = (struct h264_device *)irq_data;
	//_os_printf("********************************%s  %d\r\n",__func__,__LINE__);
	_os_printf(KERN_INFO"(F)");
}


void h264_pixel_fast_isr(uint32 irq_flags, uint32 irq_data, uint32 param){
//	struct h264_device *p_h264 = (struct h264_device *)irq_data;
	
	printf(KERN_INFO"(P)");
//	printf("******************************** %s  %d\r\n",__func__,__LINE__);
//	h264_error = 1;
}

void h264_soft_slow_isr(uint32 irq_flags, uint32 irq_data, uint32 param){
	//struct h264_device *p_h264 = (struct h264_device *)irq_data;
	_os_printf(KERN_INFO"******************************** %s  %d\r\n",__func__,__LINE__);
}


void h264_pixel_done_isr(uint32 irq_flags, uint32 irq_data, uint32 param){
	//struct h264_device *p_h264 = (struct h264_device *)irq_data;
	//_os_printf("mbl_calc:%d   mb_calc_max:%d\r\n",h264_get_mbl_calc(p_h264),h264_get_mbl_calc_max(p_h264));

}

void h264_enc_timeout_isr(uint32 irq_flags, uint32 irq_data, uint32 param){
	//struct h264_device *p_h264 = (struct h264_device *)irq_data;
	//_os_printf("(ENC TIMOUT)");
	//_os_printf("mbl_calc:%d   mb_calc_max:%d\r\n",h264_get_mbl_calc(p_h264),h264_get_mbl_calc_max(p_h264));

}

void h264_isr_init(struct h264_device *p_h264){
	h264_request_irq(p_h264,H264_FRAME_DONE,(h264_irq_hdl )&h264_frame_done_isr,(uint32)p_h264);
	h264_request_irq(p_h264,H264_BUF0_FULL ,(h264_irq_hdl )&h264_buf0_done_isr,(uint32)p_h264);		
	h264_request_irq(p_h264,H264_BUF1_FULL ,(h264_irq_hdl )&h264_buf1_done_isr,(uint32)p_h264);	
	h264_request_irq(p_h264,H264_SOFT_SLOW ,(h264_irq_hdl )&h264_soft_slow_isr,(uint32)p_h264);
	h264_request_irq(p_h264,H264_FRAME_FAST,(h264_irq_hdl )&h264_frame_fast_isr,(uint32)p_h264);	
	h264_request_irq(p_h264,H264_PIXEL_FAST,(h264_irq_hdl )&h264_pixel_fast_isr,(uint32)p_h264);
	h264_request_irq(p_h264,H264_PIXEL_DONE,(h264_irq_hdl )&h264_pixel_done_isr,(uint32)p_h264);		
	h264_request_irq(p_h264,H264_ENC_TIME_OUT,(h264_irq_hdl )&h264_enc_timeout_isr,(uint32)p_h264);
}

void h264_main_sensor_cfg(struct h264_device *p_h264,uint32_t w,uint32_t h,uint8_t src_from){
	enc_cfg.enc_mode		= 1;	//0:dec, 1:enc
	enc_cfg.enc_bs_buf_size = 4;	//0:1KB; 1:2KB; 2:4KB; 3:8KB; 4:16KB
	enc_cfg.frm_width		= (w+0xf)&(~0xf);
	enc_cfg.frm_height		= (h+0xf)&(~0xf);
	enc_cfg.wrap_width      = w;
	enc_cfg.wrap_height     = h;
	enc_cfg.src_from        = src_from;
	enc_cfg.enc_bps 		= 1500; //Kbit pre second
	enc_cfg.frm_rate		= 25;	//fps
#if H264_I_ONLY 
	enc_cfg.frm_gop 		= 1;	//IPPPP frame number of a gop
	enc_cfg.rc_en			= 0;	//enc rate control enable
#else
	enc_cfg.frm_gop 		= 25;	//IPPPP frame number of a gop
	enc_cfg.rc_en			= 1;	//enc rate control enable
#endif	
	enc_cfg.rc_grp			= 2;	//mb line number when RC change qp
	enc_cfg.rc_effort		= 2;	//0:low; 1: high; 2:relax;
	//still video: bigger frm_ip_rate; moving video: smaller frm_ip_rate;
	enc_cfg.frm_ip_rate 	= 10;	//initial (Intra MB line)/(Inter MB line) target bit rate times; not critical parameter.
	enc_cfg.frm_ip_rate_max = 15;
	enc_cfg.frm_ip_rate_min = 5;
	enc_cfg.flt3d_en		= 1;	//3D filter enable    对讲机一定要关掉
	enc_cfg.flt3d_en_last	= 1;	//3D filter enable
	enc_cfg.flt_noise_lev	= 2;	//0: very low nosie; 1: low; 2: medium; 3: high; 4: very high; naomal usage is: 2
    enc_cfg.flt_noise_lev_last = enc_cfg.flt_noise_lev;
	enc_cfg.ini_qp			= 26;	//initial QP for this sequence, not critical
	enc_cfg.roi_qp			= 22;	//QP used in ROI windows
	enc_cfg.gop_rst_3dnr    = 10;   //10个ip gop后，重新使能一下3dnr
	enc_cfg.auto_ctl_3dnr   = 1;    //需要打开vpp移动帧测，否则无效

	enc_cfg.run_gop_3dnr    = enc_cfg.gop_rst_3dnr;
	h264_ini_seting_cfg(p_h264,(struct h264_cfg_t *)&enc_cfg, (struct h264_ctl_t *)&enc_ctl, (struct h264_rc_ctl_t *)&rc_ctl,1);
	if(enc_cfg.flt3d_en) {
	  enc_cfg.r3dnr = h264_get_flt_3dnr(p_h264);
	  enc_cfg.r3dnr = enc_cfg.r3dnr & 0x0fffffff;
	  h264_3dnr_ini(p_h264,(struct h264_cfg_t *)&enc_cfg);
//	  os_printf("Z:%08x\r\n",enc_cfg.r3dnr);
	}
	
}

void h264_second_sensor_cfg(struct h264_device *p_h264,uint32_t w,uint32_t h,uint8_t src_from){

	enc_2_cfg.enc_mode		= 1;	//0:dec, 1:enc
	enc_2_cfg.enc_bs_buf_size = 4;	//0:1KB; 1:2KB; 2:4KB; 3:8KB; 4:16KB
	enc_2_cfg.frm_width		= (w+0xf)&(~0xf);//w;//
	enc_2_cfg.frm_height	= (h+0xf)&(~0xf);//h;//
	enc_2_cfg.wrap_width    = w;
	enc_2_cfg.wrap_height   = h;
	enc_2_cfg.src_from        = src_from;	
	enc_2_cfg.enc_bps 		= 1000; //Kbit pre second
	enc_2_cfg.frm_rate		= 25;	//fps
#if H264_I_ONLY
	enc_2_cfg.frm_gop		= 1;	//IPPPP frame number of a gop
	enc_2_cfg.rc_en 		= 0;	//enc rate control enable
#else
	enc_2_cfg.frm_gop 		= 25;	//IPPPP frame number of a gop
	enc_2_cfg.rc_en			= 1;	//enc rate control enable
#endif	
	enc_2_cfg.rc_grp		= 2;	//mb line number when RC change qp
	enc_2_cfg.rc_effort		= 2;	//0:low; 1: high; 2:relax;
	//still video: bigger frm_ip_rate; moving video: smaller frm_ip_rate;
	enc_2_cfg.frm_ip_rate 	= 10;	//initial (Intra MB line)/(Inter MB line) target bit rate times; not critical parameter.
	enc_2_cfg.frm_ip_rate_max = 15;
	enc_2_cfg.frm_ip_rate_min = 5;
	enc_2_cfg.flt3d_en		= 1;	//3D filter enable
    enc_2_cfg.flt3d_en_last = 1;	
	enc_2_cfg.flt_noise_lev	= 2;	//0: very low nosie; 1: low; 2: medium; 3: high; 4: very high; naomal usage is: 2
    enc_2_cfg.flt_noise_lev_last = enc_2_cfg.flt_noise_lev;
	enc_2_cfg.ini_qp			= 26;	//initial QP for this sequence, not critical
	enc_2_cfg.roi_qp			= 22;	//QP used in ROI windows
	enc_2_cfg.gop_rst_3dnr	= 10;	//10个ip gop后，重新使能一下3dnr
	enc_2_cfg.auto_ctl_3dnr	= 1;   

	enc_2_cfg.run_gop_3dnr	= enc_2_cfg.gop_rst_3dnr;
	h264_ini_seting_cfg(p_h264,(struct h264_cfg_t *)&enc_2_cfg, (struct h264_ctl_t *)&enc_2_ctl, (struct h264_rc_ctl_t *)&rc_2_ctl,2);
	if(enc_2_cfg.flt3d_en) {
	  enc_2_cfg.r3dnr = h264_get_flt_3dnr(p_h264);
	  enc_2_cfg.r3dnr = enc_cfg.r3dnr & 0x0fffffff;
	  h264_3dnr_ini(p_h264,(struct h264_cfg_t *)&enc_2_cfg);
//	  os_printf("N:%08x\r\n",enc_2_cfg.r3dnr);
	}
	
}


//init就初始化,需要申请释放配合使用,否则内存泄漏
//暂时没有返回值
//申请空间需要16对齐
int h264_mem_init(uint8_t init,uint32_t drv1_w,uint32_t drv1_h,uint32_t drv2_w,uint32_t drv2_h)
{
	uint8_t err = 0;
	uint32_t lu_base_buf_size,ch_base_size,h264_room_size;
	if(init)
	{
		if(drv1_w && drv1_h)
		{
			drv1_w = (drv1_w+0xf)&(~0xf);
			drv1_h = (drv1_h+0xf)&(~0xf);
#if H264_I_ONLY
			lu_base_buf_size = 4096 + 4096;
			ch_base_size = 4096 + 4096;
#else
			lu_base_buf_size = (drv1_w*(drv1_h+48)) + 4096 + 4096;
			ch_base_size = (drv1_w*(drv1_h+48))/2  + 4096 + 4096;
#endif		
			h264_ref_lu_base_psram = H264_MALLOC(lu_base_buf_size);
			h264_ref_ch_base_psram = H264_MALLOC(ch_base_size);

			if(!h264_ref_lu_base_psram || !h264_ref_ch_base_psram)
			{
				err = 1;
				goto h264_mem_init_end;
			}
			else
			{
				sys_dcache_clean_invalid_range((uint32_t *) h264_ref_lu_base_psram, lu_base_buf_size);
				sys_dcache_clean_invalid_range((uint32_t *) h264_ref_ch_base_psram, ch_base_size);
			}
		}

		if(drv2_w && drv2_h)
		{
			drv2_w = (drv2_w+0xf)&(~0xf);
			drv2_h = (drv2_h+0xf)&(~0xf);
#if H264_I_ONLY
			lu_base_buf_size = 4096 + 4096;
			ch_base_size = 4096 + 4096;
#else
			lu_base_buf_size = (drv2_w*(drv2_h+48)) + 4096 + 4096;
			ch_base_size = (drv2_w*(drv2_h+48))/2  + 4096 + 4096;
#endif		
			h264_ref_lu_base_psram2 = H264_MALLOC(lu_base_buf_size);
			h264_ref_ch_base_psram2 = H264_MALLOC(ch_base_size);


			if(!h264_ref_lu_base_psram2 || !h264_ref_ch_base_psram2)
			{

				err = 1;
				goto h264_mem_init_end;
			}
			else
			{
				sys_dcache_clean_invalid_range((uint32_t *) h264_ref_lu_base_psram2, lu_base_buf_size);
				sys_dcache_clean_invalid_range((uint32_t *) h264_ref_ch_base_psram2, ch_base_size);
			}
		}

		h264_room_size = H264_NODE_NUM*H264_NODE_LEN + 1024;
		h264_room_psram = H264_MALLOC(h264_room_size);;
		if(!h264_room_psram)
		{
			err = 1;
			goto h264_mem_init_end;
		}
		else
		{
			sys_dcache_clean_invalid_range((uint32_t *) h264_room_psram, h264_room_size);
		}
	}
	else
	{
		err = 2;
	}

	h264_mem_init_end:
	if(err)
	{
		if(h264_ref_lu_base_psram)
		{
			H264_FREE(h264_ref_lu_base_psram);
			h264_ref_lu_base_psram = NULL;
		}

		if(h264_ref_ch_base_psram)
		{
			H264_FREE(h264_ref_ch_base_psram);
			h264_ref_ch_base_psram = NULL;
		}


		if(h264_ref_lu_base_psram2)
		{
			H264_FREE(h264_ref_lu_base_psram2);
			h264_ref_lu_base_psram2 = NULL;
		}

		if(h264_ref_ch_base_psram2)
		{
			H264_FREE(h264_ref_ch_base_psram2);
			h264_ref_ch_base_psram2 = NULL;
		}

		if(h264_room_psram)
		{
			H264_FREE(h264_room_psram);
			h264_room_psram = NULL;
		}
	}
	return err;

}

int h264_enc(uint32_t drv1_from,uint32_t drv1_w,uint32_t drv1_h,uint32_t drv2_from,uint32_t drv2_w,uint32_t drv2_h){
//	uint32_t 	cnt=0;
//	uint32 start_time;
//	uint32_t 	dat_ofs=0;
//	uint32_t 	yuv_offset=0;
//	uint32_t 	bs_offset=0;	  //BS file offset
//	uint32_t 	x, y;
	//uint32_t	  dma_cmd_base = DMA_CMD_BASE;
//	uint32_t 	null_dma_da  = 0xffff0000;	//no use, write to h264 core directly
	//--- h264 working mode config
	//struct	  h264_cfg_t  enc_cfg;
//	uint32_t 	  rdata;
//	uint32_t       room_ofset[2];
//	uint32_t 		  bs_size;
//	uint32_t 		  w_size = 0;
//	uint8_t			  frame_end;
//	uint32_t 		  temp;
//	uint32_t 		  w_dat = 0;

	struct h264_device *h264_dev;
	h264_dev = (struct h264_device *)dev_get(HG_H264_DEVID);	

	_os_printf("%s  %d  drv2_w:%d\r\n",__func__,__LINE__,drv2_w);
	
	if(drv2_w != 0){
		h264_dev_num = 2;
	}
	else{
		h264_dev_num = 1;

	
	}
	uint8_t err = h264_mem_init(1,drv1_w,drv1_h,drv2_w,drv2_h);
	if(err)
	{
		return err;
	}
	h264_room_init();
	h264_drv_init(h264_dev);
	h264_main_sensor_cfg(h264_dev,drv1_w,drv1_h,drv1_from);	
	if(h264_dev_num == 2)
		h264_second_sensor_cfg(h264_dev,drv2_w,drv2_h,drv2_from);
	
	h264_isr_init(h264_dev);
	//h264_wrap_init
	penc_ctl = &enc_ctl;
	penc_cfg = &enc_cfg;
	prc_ctl  = &rc_ctl;
	
	penc_ctl->bs_buf_id = 0;
	#if 0
	if(penc_cfg->frm_height == 1088){
		h264_set_wrap_img_size(h264_dev,penc_cfg->frm_width,(penc_cfg->frm_height-8));
	}else{
		h264_set_wrap_img_size(h264_dev,penc_cfg->frm_width,penc_cfg->frm_height);
	}
	#else
		h264_set_wrap_img_size(h264_dev,penc_cfg->wrap_width,penc_cfg->wrap_height);
	#endif


	h264_cfg_srcdat(h264_dev,penc_cfg->src_from);
	
	//p_h264->WRAP_CON |= 0x01|0x10|0x100;
	h264_set_vpp_vsync_delay(h264_dev,1);
	h264_set_hw_open(h264_dev,1);
	//h264_open(h264_dev);
	h264_start_enc_frm(h264_dev,(struct h264_cfg_t *)penc_cfg, (struct h264_ctl_t *)penc_ctl, (struct h264_rc_ctl_t *)prc_ctl);
#if 0	
	h264_open(h264_dev);
	os_sleep_ms(10000);
	h264_close(h264_dev);
	os_sleep_ms(10000);
	h264_open(h264_dev);
	os_sleep_ms(10000);
#endif	
	return 0;
}


void h264_dec_intr_status(struct h264_device *p_h264,struct h264_ctl_t *dec_ctl) {
    uint32_t   rdata 	;
    uint32_t		hw_byte	;

	//decoder used bitstream length check
	hw_byte = h264_get_frame_len(p_h264);
	hw_byte += h264_get_dec_del(p_h264);
	if((hw_byte < (dec_ctl->bs_byte_limit -8)) || (hw_byte > dec_ctl->bs_byte_limit)) {
		_os_printf("decoder stream length error.:%d  %d\n\r",hw_byte,dec_ctl->bs_byte_limit);
	}		

	rdata = h264_get_sta(p_h264);

	dec_ctl->enc_end_flags= rdata;
}

void h264_dec_clr_enc_funcs(struct h264_device *p_h264) {
  h264_set_enc_frame_rdcost(p_h264);
}


void h264_dec_refbuf_set(struct h264_device *p_h264,uint32_t      buf_base, struct h264_cfg_t *dec_cfg, struct h264_ctl_t *dec_ctl) {
  uint32_t   max_luma_size;
  max_luma_size = dec_cfg->frm_width * dec_cfg->frm_height;
  
  
  dec_ctl->ref_lu_base= buf_base;   //must be 4KB aligned
  dec_ctl->ref_lu_end = ((buf_base + max_luma_size + (48*dec_cfg->frm_width) + 4095) >> 12) << 12;  //4KB align
  dec_ctl->ref_ch_base= dec_ctl->ref_lu_end;
  dec_ctl->ref_ch_end = ((dec_ctl->ref_ch_base + (max_luma_size>>1) + (24*dec_cfg->frm_width) + 4095) >> 12) << 12;  //4KB align
  dec_ctl->last_lpf_lu_base = dec_ctl->ref_lu_base;
  dec_ctl->last_lpf_ch_base = dec_ctl->ref_ch_base;

  //p_h264->REF_LU_ST_ADDR   = dec_ctl->ref_lu_base >> 12;
  //p_h264->REF_LU_ED_ADDR   = dec_ctl->ref_lu_end  >> 12;
  //p_h264->REF_CHRO_ST_ADDR = dec_ctl->ref_ch_base >> 12;
  //p_h264->REF_CHRO_ED_ADDR = dec_ctl->ref_ch_end  >> 12; 

//  h264_set_ref_lu_addr(p_h264,dec_ctl->ref_lu_base,dec_ctl->ref_lu_end);
//  h264_set_ref_chro_addr(p_h264,dec_ctl->ref_ch_base,dec_ctl->ref_ch_end);

  
}

void h264_dec_src_room_set(struct h264_device *p_h264,uint32_t       buf_base, struct h264_cfg_t *dec_cfg,struct h264_ctl_t *dec_ctl){
	uint32_t   max_luma_size;
	max_luma_size = dec_cfg->frm_width * dec_cfg->frm_height;

	dec_ctl->ref_lu_base= buf_base;   //must be 4KB aligned
	dec_ctl->ref_lu_end = ((buf_base + max_luma_size + (48*dec_cfg->frm_width) + 4095) >> 12) << 12;  //4KB align
	dec_ctl->ref_ch_base= dec_ctl->ref_lu_end;
	dec_ctl->ref_ch_end = ((dec_ctl->ref_ch_base + (max_luma_size>>1) + (24*dec_cfg->frm_width) + 4095) >> 12) << 12;  //4KB align
	
	h264_set_ref_lu_addr(p_h264,dec_ctl->ref_lu_base,dec_ctl->ref_lu_end);
	h264_set_ref_chro_addr(p_h264,dec_ctl->ref_ch_base,dec_ctl->ref_ch_end);
}

void h264_dec_a_frame(struct h264_device *p_h264,uint32_t nal_length, struct h264_ctl_t *dec_ctl, struct h264_header *head, struct str_info *str,uint32 dataroom) {
      uint32_t wdata;
      uint32_t base_tmp;
	  uint32_t base_tmp2;
  
      wdata = dec_ctl->frm_type | (0 << 4);
	  //p_h264->CON  = wdata;
	  h264_set_frame_base_cfg(p_h264,dec_ctl->frm_type,0,0,0);
      
	  //p_h264->IMG_WIDTH  = dec_ctl->frm_width;
	  //p_h264->IMG_HEIGH  = dec_ctl->frm_height;
	  h264_set_phy_img_size(p_h264,dec_ctl->frm_width,dec_ctl->frm_height);
	  
	  //p_h264->FRM_QP     = head->pic_qp;
	  h264_set_frame_qp(p_h264,head->pic_qp);

	  
      //ref read base
//	  p_h264->REF_LU_RB_ADDR    = dec_ctl->last_lpf_lu_base >> 12;
//	  p_h264->REF_CHRO_RB_ADDR  = dec_ctl->last_lpf_ch_base >> 12;
	  h264_set_ref_rb_addr(p_h264,dec_ctl->last_lpf_lu_base,dec_ctl->last_lpf_ch_base);
      //lpf write base
      //lpf luma base
      base_tmp = h264_cal_ref_base(dec_ctl->ref_lu_base, dec_ctl->ref_lu_end, dec_ctl->last_lpf_lu_base, dec_ctl->frm_luma_size);
      //Xil_Out32((H264_REG_BASE + 84*4), base_tmp >> 12);
      //p_h264->REF_LU_WB_ADDR    = base_tmp >> 12;
      dec_ctl->last_lpf_lu_base = base_tmp;
      
      //lpf chroma base
      base_tmp2 = h264_cal_ref_base(dec_ctl->ref_ch_base, dec_ctl->ref_ch_end, dec_ctl->last_lpf_ch_base, (dec_ctl->frm_luma_size >> 1));
      //Xil_Out32((H264_REG_BASE + 85*4), base_tmp >> 12);
      //p_h264->REF_CHRO_WB_ADDR   = base_tmp2 >> 12;
      dec_ctl->last_lpf_ch_base = base_tmp2;

	  h264_set_ref_wb_addr(p_h264,base_tmp,base_tmp2);

//      p_h264->LEN_LIMIT   = (nal_length+4);
	  h264_set_len_limit(p_h264,nal_length+4);
      dec_ctl->bs_byte_limit = nal_length+4;
	  
      //p_h264->DMA_BS_ADDR0 = ((uint32_t)&frame_cache>>10);    
	  h264_set_buf_addr(p_h264,dataroom);
	  //p_h264->FRM_MB_NUM = (dec_ctl->frm_luma_size >> 8);
	  h264_set_frame_mb_num(p_h264,dec_ctl->frm_luma_size);
      //p_h264->DEC_HEADB_NUM = str->used_bits;
	  h264_set_dec_headb_num(p_h264,str->used_bits);
}

struct	str_info	h264_str[3]; 
struct	h264_header h264_head[3];
struct	h264_cfg_t	dec_cfg[3];  
struct	h264_ctl_t	dec_ctl[3];

__psram_data uint8_t h264_1_room[100*1024] __aligned(4096);


void h264_dec_room_init(uint8_t devnum,uint16_t drv2_w,uint16_t drv2_h){
	uint8_t i = 0;
	for(i = 0;i < devnum;i++){
		h264_ref_memory_base[i] = H264_MALLOC((drv2_w*(drv2_h+48))+(drv2_w*(drv2_h+48))/2 + 4096 + 4096);
		h264_ref_memory_base[i] = ((uint32_t)h264_ref_memory_base[i] + 0xfff) & (~0xfff); 
	}
}



void h264_dec_src_264(uint8 *src_file,uint32 file_size,uint32 w,uint32 h,uint32_t devid){
	//--- platform initial 
	int32 ret;
	uint8_t* 	mbs_ptr; //main bs ptr
	struct h264_device *p_h264;
	p_h264 = (struct h264_device *)dev_get(HG_H264_DEVID);

    mbs_ptr = src_file;   //文件地址	
	//decoder reference frame addr setting

	dec_cfg[devid].enc_mode  = 0;	  //0:dec, 1:enc  
	dec_cfg[devid].frm_width = w; //maximal support x-pixel
	dec_cfg[devid].frm_height= h;  //maximal support y-pixel
	
	h264_request_irq(p_h264,H264_FRAME_DONE,(h264_irq_hdl )&h264_frame_dec_done_isr,(uint32)p_h264);
	while(1){
	    uint8_t      nal_type  ;
	    uint32_t     nal_start   ; //nal bitstream start addr
	    uint32_t     nal_length  ; //nal byte length
	    


	    mbs_ptr   = find_start_code(mbs_ptr); //find first 0x0000_0001    
	    nal_start = (uint32_t) mbs_ptr - 4;        //return the 4byte "00000001"
	    mbs_ptr   = find_start_code(mbs_ptr); //find second 0x0000_0001
	    mbs_ptr   -= 4; 
	    
	    nal_length  = (uint32_t) mbs_ptr - nal_start;
//		if((mbs_ptr - src_file) > file_size-30000){
//			break;
//		}
		
	    //--- copy bitstream to BS buffer for decode

		hw_memcpy((void*) h264_1_room,(const void*) nal_start,nal_length);
		sys_dcache_clean_range((uint32_t*)h264_1_room,nal_length);
		
		h264_str[devid].ptr = (uint8_t *)h264_1_room;
		
	    //decoder nal header
	    nal_type = nal_parse(&h264_str[devid], &h264_head[devid]);
	    
	    if(nal_type == sps) {
	      //frame size check      
	      if((h264_head[devid].sps_pic_width > dec_cfg[devid].frm_width) || (h264_head[devid].sps_pic_height > dec_cfg[devid].frm_height)) {
	        _os_printf("frame x/y pixel size is too big.(%d  %d  === > %d  %d)\n\r",h264_head[devid].sps_pic_width,h264_head[devid].sps_pic_height,dec_cfg[devid].frm_width,dec_cfg[devid].frm_height);
	        return ;
	      }
	      
	      dec_ctl[devid].frm_width = h264_head[devid].sps_pic_width;   //should be 16*N
	      dec_ctl[devid].frm_height= h264_head[devid].sps_pic_height;  //should be 16*N
	      dec_ctl[devid].frm_luma_size   = dec_ctl[devid].frm_width * dec_ctl[devid].frm_height;
	          
	      dec_ctl[devid].timeout_limit   = (dec_ctl[devid].frm_luma_size >> 8) * 512 * 2;     

		  
		  //_os_printf("h264_ref:%08x\r\n",h264_ref_memory_base[devid]);
		  h264_dec_refbuf_set(p_h264,((uint32_t)h264_ref_memory_base[devid] + 0xfff) & (~0xfff), (struct h264_cfg_t *)&dec_cfg[devid], (struct h264_ctl_t *)&dec_ctl[devid]);
		  
		  h264_dec_clr_enc_funcs(p_h264);
		  h264_set_mb_pipe(p_h264,((502-2) << 16) | 502);

		  h264_set_timeout_limit(p_h264,(dec_ctl[devid].timeout_limit) >> 10);
	    } else if(nal_type == pps) {
	    
	    } else if(nal_type == pic_idr) {
	      
	    }     

		h264_dec_src_room_set(p_h264,((uint32_t)h264_ref_memory_base[devid] + 0xfff) & (~0xfff), (struct h264_cfg_t *)&dec_cfg[devid], (struct h264_ctl_t *)&dec_ctl[devid]);
		
	    //there is a frame need decode
	    if((nal_type == pic_idr) || (nal_type == pic_data)) {
	      //decode a picture
	      if(nal_type == pic_idr)
	        dec_ctl[devid].frm_type = 2;
	      else
	        dec_ctl[devid].frm_type = 0;
	  
	      h264_dec_a_frame(p_h264,nal_length, &dec_ctl[devid], &h264_head[devid], &h264_str[devid],(uint32_t)h264_1_room);
		  h264_decode_start(p_h264);

		  ret = h264_sema_down(100);
	      h264_dec_intr_status(p_h264,&dec_ctl[devid]);
	      //-- clear the frm end flag
	      h264_clr_intr(p_h264);
	      h264_dec_flag_chk(dec_ctl[devid].enc_end_flags);	

		}
		file_size -= nal_length;
		
		if(file_size == 0)   //sps+pps
			return;
	}
}

void h264_dec_nal_264(uint8 *src_file,uint32 file_size,uint32 w,uint32 h,uint32_t devid){
	struct h264_device *p_h264;
	p_h264 = (struct h264_device *)dev_get(HG_H264_DEVID);
	
	dec_cfg[devid].enc_mode  = 0;	  //0:dec, 1:enc  
	dec_cfg[devid].frm_width = w; //maximal support x-pixel
	dec_cfg[devid].frm_height= h;  //maximal support y-pixel
}

void h264_frame_gen420_kick_run(struct h264_device *p_h264,uint32 addr){
	if(recfg_h264_new_grop[0] == 1){
		recfg_h264_new_grop[0] = 0;
		penc_ctl->gop_frm_cnt = 0;
	}
	h264_init(p_h264,H264_HARDWARE_CLK);
	
	if(penc_ctl->gop_frm_cnt == 0){
		if(h264_cfg_modify((struct h264_cfg_t *)penc_cfg,1)){
			h264_ini_recfg((struct h264_cfg_t *)penc_cfg,(struct h264_ctl_t *)penc_ctl,(struct h264_rc_ctl_t *)prc_ctl);
		}
	}

	h264_request_irq(p_h264,H264_FRAME_DONE,  (h264_irq_hdl )&h264_frame_done_norekick_isr,(uint32)p_h264);
	h264_request_irq(p_h264,H264_BUF0_FULL ,  (h264_irq_hdl )&h264_buf0_done_isr,(uint32)p_h264);		
	h264_request_irq(p_h264,H264_BUF1_FULL ,  (h264_irq_hdl )&h264_buf1_done_isr,(uint32)p_h264);	
	h264_request_irq(p_h264,H264_SOFT_SLOW ,  (h264_irq_hdl )&h264_soft_slow_isr,(uint32)p_h264);
	h264_request_irq(p_h264,H264_FRAME_FAST,  (h264_irq_hdl )&h264_frame_fast_isr,(uint32)p_h264);	
	h264_request_irq(p_h264,H264_PIXEL_FAST,  (h264_irq_hdl )&h264_pixel_fast_isr,(uint32)p_h264);
	h264_request_irq(p_h264,H264_PIXEL_DONE,  (h264_irq_hdl )&h264_pixel_done_isr,(uint32)p_h264);		
	h264_request_irq(p_h264,H264_ENC_TIME_OUT,(h264_irq_hdl )&h264_enc_timeout_isr,(uint32)p_h264);

	h264_set_oe_select(p_h264, 0, 0);
	h264_set_wrap_img_size(p_h264,penc_cfg->wrap_width,penc_cfg->wrap_height);
	h264_cfg_srcdat(p_h264,penc_cfg->src_from);
	h264_set_vpp_vsync_delay(p_h264,1);
	h264_set_hw_open(p_h264,1);
	h264_start_enc_frm(p_h264,(struct h264_cfg_t *)penc_cfg, (struct h264_ctl_t *)penc_ctl, (struct h264_rc_ctl_t *)prc_ctl);
	enc_ctl.bs_buf_id = 0;	
	h264_open(p_h264);

	wake_up_gen420_queue(1,addr);

}

struct h264_hardware_s
{
    struct msi *msi;
    struct h264_device *h264_dev;
    os_msgqueue_t msgq;
    struct os_event evt;
    struct os_task task;
};

//一个回调函数,私有结构,以及一些固定参数
struct h264_msg_s
{
	struct list_head list;		  //h264 queue hand	
    void *fn_data;                //函数的私有结构
	uint8_t type;                 //0: h264 enc  1:h264 dec   
    uint32_t data;
	uint32_t len;
    uint16_t w;
    uint16_t h;
	uint8_t sta;			      //0:idle    1:data ready
	
};




int put_h264msg_to_queue(uint8_t type,uint32_t w,uint32_t h,uint32 data,uint32 len){
	struct h264_msg_s *wq;
	uint32_t flags;	
	wq = malloc(sizeof(struct h264_msg_s));
	wq->type   = type;
	wq->w      = w;
	wq->h      = h;
	wq->sta    = 0;
	wq->data   = data;
	wq->len    = len;
	
	flags = disable_irq();
	INIT_LIST_HEAD(&wq->list);
	list_add_tail(&wq->list,(struct list_head*)&h264_queue_head); 
	enable_irq(flags);
	
	h264wq_sema_up();
	return &wq->list;
}

uint8_t h264msg_queue_done(uint32 list){
	struct list_head *dlist;
	uint32_t flags;	
	flags = disable_irq();
	dlist = (struct list_head *)&h264_queue_head;
	while(dlist->next != &h264_queue_head){
		if(dlist->next == list){
			enable_irq(flags);
			return 0;
		}
		dlist = dlist->next;
	}
	enable_irq(flags);
	return 1;
}

void h264_wq_thread(){
	int32 ret;
	uint32_t flags;	
	struct list_head *dlist;
	struct h264_msg_s* h264dev;	
	struct h264_device *p_h264;
	p_h264 = (struct h264_device *)dev_get(HG_H264_DEVID);	
	while(1){
		h264wq_sema_down(-1);
		flags = disable_irq();
		if(list_empty((struct list_head *)&h264_queue_head) != TRUE){
			enable_irq(flags);
			do{
				flags = disable_irq();
				dlist = (struct list_head *)&h264_queue_head;
				dlist = dlist->next;
				if(dlist == &h264_queue_head){
					enable_irq(flags);
					break;
					//return -1;
				}else{
					h264dev = list_entry((struct list_head *)dlist,struct h264_msg_s,list);
					enable_irq(flags);
					if(h264dev->type == 0){				//h264 enc
						//os_printf("Enc\r\n");
						h264_frame_gen420_kick_run(p_h264,h264dev->data);
						ret = h264_sema_down(100);
						if(ret == 0){
							os_printf("enc error..\r\n");
						}
					}else{	
						//_os_printf("Dec\r\n");              //h264 dec
						h264_dec_src_264(h264dev->data,h264dev->len,h264dev->w,16*((h264dev->h + 15)/16) ,0);
					}
					flags = disable_irq();
					list_del(&h264dev->list);
					enable_irq(flags);
					free(h264dev);
				}
			}while(1);
		}else{
			enable_irq(flags);
		}
	
	}
}


void h264wq_queue_init(uint16_t w,uint16_t h){
	//h264_drv_init();
	h264wq_sema_init();
	INIT_LIST_HEAD((struct list_head *)&h264_queue_head);
	h264_msi_init_with_mode_for_264wq(GEN420_DATA,w,h);
	h264_dec_room_init(1,w,16*((h+15)/16));
	os_task_create("h264_thread", h264_wq_thread, NULL, OS_TASK_PRIORITY_HIGH, 0, NULL, 1024);
}

