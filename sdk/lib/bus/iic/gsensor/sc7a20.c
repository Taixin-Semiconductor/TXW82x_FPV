#include "typesdef.h"
#include "g_sensor.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/i2c.h"


GSENSOR_INIT_SECTION const unsigned char sc7a20InitTable[28]={
	0X1F,0x00,
	0x20,0x47,
	0x21,0x00,	
	0x22,0x00,
	0x23,0x88,
	0xFC,0X05,      //delay 5ms
	-1,-1,
};






GSENSOR_OP_SECTION const _Gsensor_Adpt_ sc7a20_cmd= {
	.init    = (uint8_t *)sc7a20InitTable,
	.local_xyz = {0x28,0x29,0x2a,0x2b,0x2c,0x2d},
};


const _Gsensor_Ident_ sc7a20_init=
{
	0x11,0x32,0x33,0x01,0x01,0x0f
};


