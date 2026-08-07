#ifndef __GEN420_HARDWARE_MSI_H
#define __GEN420_HARDWARE_MSI_H
#include "basic_include.h"

//设置队列的宏,H264固定,其他是自定义
enum GEN420_QUEUE_ENUM
{
    GEN420_QUEUE_NONE,
    GEN420_QUEUE_H264,
    GEN420_QUEUE_JPEG,
    GEN420_QUEUE_JPEG_RECODE,
    GEN420_QUEUE_THUMB_JPEG,
    GEN420_QUEUE_THUMB_JPEG_OVER_DPI,
};
extern os_msgqueue_t *gen420_msgq;

struct gen420_msg_s;
typedef int32_t (*gen420_kick_fn)(struct gen420_msg_s *msg);
typedef int32_t (*gen420_free_fn)(struct gen420_msg_s *msg);

//一个回调函数,私有结构,以及一些固定参数
struct gen420_msg_s
{
	struct list_head list;		  //gen420 queue hand	
    gen420_kick_fn kick_fn;       //启动kick,然后等待完成,返回值代表需要kick
    gen420_free_fn free_fn;       //内存释放?应该什么时候调用,不应该在中断调用才对
    void *fn_data;                //函数的私有结构
	uint8_t type;                 //0: jpg for Sub stream   1:h264 for Sub stream   2-255:user   
    uint8_t *data;
    uint16_t w;
    uint16_t h;
	uint32_t devid;
	uint8_t sta;			      //0:idle    1:data ready
	
};

int32_t g_gen420_kick_msg(struct gen420_msg_s *msg,int32_t ms);
struct msi *gen420_hardware_msi_init();
int wake_up_gen420_queue(uint8_t devid,uint8_t *data_rom);
int unregister_gen420_queue(uint32_t devid);
int register_gen420_queue(uint8_t type,uint32_t w,uint32_t h,gen420_kick_fn kick_fn,gen420_free_fn free_fn,uint32 priv);
int32_t h264_gen420_kick();
#endif