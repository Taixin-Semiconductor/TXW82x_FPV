#ifndef _ISP_DEV_H_
#define _ISP_DEV_H_

#include "lib/video/dvp/cmos_sensor/csi.h"
#include "hal/isp.h"
#include "hal/capture.h"

extern void   *set_fps_task_hdl;
extern volatile struct os_msgqueue set_fps_msg;
extern volatile struct os_event    set_fps_event;

typedef struct sensor_isp_info {
    struct list_head list;	
    uint16 sensor_src;
    uint16 sensor_type;
    uint16 sensor_dev_id;
    uint32 sensor_opt_cmd;
    uint32 sensor_config;
} SENSOR_BASIC_INFO;

void sensor_info_init();
void sensor_info_destory();
void sensor_info_add(enum sensor_type type, enum isp_input_dat_src sensor_src, uint32 sensor_config, uint32 iic_id, uint32 opt_cmd);

void isp_cfg_dev();
void isp_dev_close();
void dual_mipi_csi_reset(enum fps_mode mode, float fps);
void dual_mipi_sensor_set_fps(enum fps_mode mode, float fps);
uint32 sensor_read_frame_length(_Sensor_Adpt_ *sensor_cmd, uint8 addr_num , uint8 data_num , uint8_t devid);
int32 set_fps_task(void);
capture_irq_hdl fsync_capture_irq_hdl(uint32 irq, uint32 irq_data);


#endif
