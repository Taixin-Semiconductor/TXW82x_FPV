#ifndef _MIPI_CSI_H_
#define _MIPI_CSI_H_
#include "sys_config.h"
#include "typesdef.h"


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

#endif

