#ifndef _ISP_DEV_H_
#define _ISP_DEV_H_

#include "lib/video/dvp/cmos_sensor/csi.h"
#include "hal/isp.h"

typedef struct sensor_isp_info {
    struct list_head list;	
    uint16 sensor_src;
    uint16 sensor_type;
    uint16 sensor_dev_id;
    uint32 mode_index;
    uint32 sensor_config;
} SENSOR_BASIC_INFO;

void sensor_info_init();
void sensor_info_destory();
void sensor_info_add(enum sensor_type type, enum isp_input_dat_src sensor_src, uint32 sensor_config, uint32 iic_id, uint32 opt_cmd);

void isp_cfg_dev();
void isp_dev_close();
uint32 sensor_read_frame_length(_Sensor_Adpt_ *sensor_cmd, uint8 addr_num , uint8 data_num , uint8_t devid);

#endif
