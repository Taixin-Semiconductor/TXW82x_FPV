#ifndef _G_SENSOR_H_
#define _G_SENSOR_H_
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/i2c.h"


#ifndef ALIGNED
#define ALIGNED(x) 				__attribute__ ((aligned(x)))
#endif
#define GSENSOR_INIT_SECTION     ALIGNED(32)
#define GSENSOR_OP_SECTION       ALIGNED(32)



typedef struct {
	uint8 x_l;
	uint8 x_h;
	uint8 y_l;
	uint8 y_h;
	uint8 z_l;
	uint8 z_h;	
} local;


typedef struct {
	uint8 * init;
	uint8 * preread;
	local local_xyz;
} _Gsensor_Adpt_;



typedef struct
{       
	uint8 id,w_cmd,r_cmd,addr_num,data_num;
	uint16 id_reg;
}_Gsensor_Ident_;


#define G_IIC_CLK                       346000UL
 


#define DEV_GSENSOR_DA280  				1
#define DEV_GSENSOR_QMA7981				0
#define DEV_GSENSOR_SC7A20				0


#if DEV_GSENSOR_DA280
extern const _Gsensor_Ident_ da280_init;
extern GSENSOR_OP_SECTION const _Gsensor_Adpt_ da280_cmd;	
#endif

#if DEV_GSENSOR_QMA7981
extern const _Gsensor_Ident_ qma7981_init;
extern GSENSOR_OP_SECTION const _Gsensor_Adpt_ qma7981_cmd;	
#endif

#if DEV_GSENSOR_SC7A20
extern const _Gsensor_Ident_ sc7a20_init;
extern GSENSOR_OP_SECTION const _Gsensor_Adpt_ sc7a20_cmd;	
#endif

#endif
