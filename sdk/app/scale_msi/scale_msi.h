#ifndef __SCALE_MSI_H
#define __SCALE_MSI_H

#include "stream_frame.h"
#include "basic_include.h"

#include "lib/multimedia/msi.h"

#include "osal/work.h"
#define MAX_SCALE3_TX    3


struct  scale_msg_t {
	uint16_t iw,ih;
	uint16_t ow,oh;
	uint16_t x,y;	
};

struct  scale2_msg_t {
	uint16_t stype;
	uint16_t iw,ih;
	uint16_t ow,oh;
	uint16_t x,y;
};

struct scale3_msi_s
{
    struct os_work work;
    struct scale_device *scale_dev;
    struct msi    *msi;
    uint16_t iw,ih,ow,oh;
    struct framebuff *now_fb;
    uint8_t *scaler3buf;  //scale的buf
    struct os_msgqueue msgq;
    uint8_t del;
	uint8_t txpool_type;
    uint8_t init_txpool_num;
    struct fbpool tx_pool0;
	struct fbpool tx_pool1;
	struct fbpool tx_pool2;
	struct fbpool tx_poolerr;			//用于抛开错误长度或者fb不存在时的抛帧
};

struct const_scale3_msi_s
{
    struct msi    *msi;
	struct scale_device *scale_dev;
    uint8_t *buf;
};
#define MAX_SCALE2_TX    4
struct scale2_msi_s
{
    struct os_work work;
    struct scale_device *scale_dev;
    struct msi    *msi;
    uint16_t iw,ih,ow,oh;
    struct framebuff *now_fb;
    uint8_t *scaler2buf;  //scale的buf
    struct os_msgqueue msgq;
	uint8_t larger;
    uint8_t del;
    uint8_t seq;
    uint16_t type;
    //RBUFFER_DEF(tx_data, struct framebuff *, MAX_SCALE3_TX);
    struct fbpool tx_pool;
};



struct scale1_msi_s
{
    struct os_work work;
    struct scale_device *scale_dev;
    struct dma_device *dma1;
    uint16_t iw,ih,ow,oh;
    struct msi    *msi;
    uint8_t *scaler1buf;  //scale的buf
    uint8_t dma1_lock;
};

struct msi *scale3_msi(const char *name);
struct msi *scale3_msi_const_buf(const char *name, uint8_t *buf,uint16_t iw, uint16_t ih, uint16_t ow, uint16_t oh);
#endif
