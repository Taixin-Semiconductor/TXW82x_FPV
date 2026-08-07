#ifndef HG_JPG_H
#define HG_JPG_H
#include "hal/jpeg.h"
//typedef int (*jgp_isr_func)(uint32);
//typedef int32 (*jpg_irq_hdl)(uint32 irq, uint32 irq_data, uint32 param);


#ifdef TXW80X
typedef enum {
	DONE_IRQ,
	JPG_OUTBUF_FULL,
	JPG_BUF_ERR,
	
//	JPG_READ_EMPTY,	
//	JPG_WRITE_FULL,
	JPG_IRQ_NUM
}JPG_IRQ_E;
#endif

#ifdef TXW81X
typedef enum {
	DONE_IRQ,
	JPG_OUTBUF_FULL,
	JPG_BUF_ERR,
	JPG_PIXEL_DONE,
	JPG_TIME_OUT,
	JPG_IRQ_NUM
}JPG_IRQ_E;
#endif

#if TXW82X
typedef enum {
	DONE_IRQ,
	JPG_OUTBUF_FULL,
	JPG_BUF_ERR,
	JPG_PIXEL_DONE,
	JPG_TIME_OUT,
	JPG_IRQ_NUM
}JPG_IRQ_E;
#endif

struct hgjpg {
    struct jpg_device       dev;
	uint32              hw;
	uint32              thw;
	uint32              huf_hw;
    //jpg_irq_hdl             irq_hdl;
    uint32                  irq_num;
    uint8                   opened       : 1,
                            jpg_run      : 1,
							decode       : 1,
							set_buf_len  : 1;
	uint8  addr_count;
	uint8  deal_time;
	uint32 *cfg_backup;
};

//int32 jpg_open(struct jpg_device *p_jpg);
//int32 jpg_close(struct jpg_device *p_jpg);
//int32 jpg_init(struct jpg_device *p_jpg,uint32 table_index,uint32 qt,uint32 buflen,uint32 head_reserver);
//int32 jpg_set_size(struct jpg_device *p_jpg,uint32 h,uint32 w);
//int32 jpg_set_len(struct jpg_device *p_jpg,uint32 buflen,uint32 head_reserver);
//int32 jpg_updata_dqt(struct jpg_device *p_jpg,uint32* dqtbuf);
//int32 jpg_set_addr(struct jpg_device *p_jpg,uint32 addr);
//int32 jpg_set_qt(struct jpg_device *p_jpg,uint32 qt);
//int32 jpg_get_len(struct jpg_device *p_jpg);
//int32 jpg_get_outbuf_num(struct jpg_device *p_jpg);
//int32 jpg_request_irq(struct jpg_device *p_jpg,uint32 irq, jgp_isr_func isr, void *dev_id);
//int32 jpg_release_irq(struct jpg_device *p_jpg,uint32 irq);
void hgjpg_attach(uint32 dev_id, struct hgjpg *jpg);







#endif
