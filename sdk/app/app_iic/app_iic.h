#ifndef __APP_IIC_H
#define __APP_IIC_H

#include "sys_config.h"
#include "typesdef.h"
#include "stream_frame.h"


#include "basic_include.h"
#include "hal/i2c.h"



void iic_thread_init();
void iic_run_thread(void *d);
int iic_devid_finish(uint8_t devid);
int wake_up_iic_queue(uint8_t devid,uint8_t *table,uint32_t len,uint8_t rw_sta,uint8_t *trx_buff);
int unregister_iic_queue(uint8_t devid);
int register_iic_queue(struct i2c_device *i2c,uint8_t scl_io,uint8_t sda_io,uint8_t id_addr);
int iic_devid_set_addr(uint8_t devid,uint8_t addr);


#endif
