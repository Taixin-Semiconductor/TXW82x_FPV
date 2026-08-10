#ifndef _MIPI_CSI_H_
#define _MIPI_CSI_H_
#include "sys_config.h"
#include "typesdef.h"
#include "hal/isp.h"


#define RAW8							1
#define RAW10							2
#define RAW12							3
#define YUV422							5


#ifndef INPUT_MODE
#define INPUT_MODE						RAW10
#endif

#ifndef DOUBLE_CSI
#define DOUBLE_CSI                      1      //主机
#endif

#ifndef DOUBLE_LANE
#define DOUBLE_LANE                     0
#endif

struct mipi_csi_debug {
    uint8 debug_enable;
	uint8 debug_io0  , debug_io1  , debug_io2  , debug_io3  , debug_io4  , debug_io5  ;
	uint8 debug_type0, debug_type1, debug_type2, debug_type3, debug_type4, debug_type5;
};

int mipi_csi_hardware_config(uint32_t csi_dev_id, uint8_t csi_lane_num, uint8_t camera_mode, uint8_t slave_en, uint8_t sensor_type,uint8_t fps, struct mipi_csi_debug *p_debug); 
void get_single_mipi(uint32_t csi_dev_id,uint16_t *w,uint16_t *h);
void mipi_csi_hardware_deconfig();

void dual_mipi_sensor_set_fps(enum fps_mode mode, float fps);

#endif

