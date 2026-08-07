#ifndef _JPG_MODE_DEFINE_
#define _JPG_MODE_DEFINE_


#define DQT_DEF				5

#define DQT_MAX_INDEX       11

#ifndef TARGET_JPG_LEN
#define TARGET_JPG_LEN      50000   
#endif
#define QUALITY_CTRL_P      131
#define QUALITY_CTRL_I      8
#define QUALITY_CTRL_D      0
#define LIMITING(x,x1,x2)     (x>x1)?x1:(x<x2?x2:x)


typedef struct 
{
	struct list_head list;				//jpeg_frame的节点头
	uint32 frame_len;						//帧长度
	uint8 usable;							//判断是否可用       0:空闲
										//			   1:节点补充中或正在使用中
										//			   2:可用
	uint8 jpg_num;
	
}jpeg_frame;


typedef struct
{     
	struct list_head list;
	uint8* buf_addr;
}mjpeg_node;

enum JPG_ID
{
	JPGID0,
	JPGID1,
};

//通过定义宏去设置jpg的lock的值,只要有复用,就需要添加一个,防止与其他冲突
enum JPG_LOCK_VALUE
{
	JPG_LOCK_NONE,
	JPG_LOCK_ENCODE,
	JPG_LOCK_SCALE1_ENCODE,
	JPG_LOCK_DECODE,
	JPG_LOCK_GEN420_ENCODE,
	JPG_LOCK_GEN420_RECODE,
	JPG_LOCK_GEN420_THUBM_ENCODE,
	JPG_LOCK_GEN420_THUBM_ENCODE_OVER_DPI,
	JPG_LOCK_NORMAL_ENCODE,
	JPG_LOCK_MAX = 0xff,
};


enum JPG_SRC_FROM {
	VPP_DATA0,
	VPP_DATA1,		
	PRC_DATA,
	SCALER_DATA,
	GEN420_DATA,
	SOFT_DATA,	
};

void jpg_start(uint8 jpgid);
void jpeg_user();
//void jpg_open(struct hgjpg_hw *p_jpg,uint8 mode);
//void hgjpg_attach(uint32 dev_id, struct hgjpg *jpg);
#ifdef TXW81X
void jpg_cfg(uint8 jpgid,enum JPG_SRC_FROM src_from);
#endif

#ifdef TXW80X
void jpg_cfg();
#endif

//bool put_node(volatile struct list_head *head,volatile struct list_head *del);
void del_frame(uint8 jpg_num,volatile struct list_head* frame_list);
struct list_head* get_frame(uint8 jpgid);

void mjpeg_sema_init();
void mjpeg_sema_down(int32 tmo_ms);
void mjpeg_sema_up();
void mjpeg_avi_thread(void *d);


extern const uint16  htable[384];
extern const uint8  dhtable[412];

extern char quality_tab[6][128];

#if SCEN_EN
#define IMAGE_W_J    IMAGE_W/2
#define IMAGE_H_J    IMAGE_H/2
#else
#define IMAGE_W_J    IMAGE_W
#define IMAGE_H_J    IMAGE_H
#endif

#endif

