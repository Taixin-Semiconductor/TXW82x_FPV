#ifndef _DUAL_ORG_DEV_H_
#define _DUAL_ORG_DEV_H_

#include "typesdef.h"
#include "hal/dual_org.h"

void dorg_double_sensor(uint32 src0_w,uint32 src0_h,uint32 src1_w,uint32 src1_h,uint32 src0_fmt,uint32 src1_fmt,uint8_t dvp_role, uint8_t csi0_role, uint8_t csi1_role);
void dual_org_debug_config(uint8 dbg_io0,uint8 dbg_io1,uint8 dbg_io2,uint8 dbg_io3);

#endif
