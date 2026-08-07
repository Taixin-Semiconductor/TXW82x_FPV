#ifndef _PARA_IN_DEV_H_
#define _PARA_IN_DEV_H_

#include "basic_include.h"
#include "hal/para_in.h"

#define TP9950_HDA_720P_25FPS

#if DEV_SENSOR_TP9950

#ifdef TP9950_HDA_720P_25FPS
extern const _Sensor_Para_in_Init tp9950_HDA_720P_25FPS_para_in_init;
#endif

#endif

void para_in_hareware_init();

#endif