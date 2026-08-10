#include "typesdef.h"
#include "g_sensor.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/i2c.h"


GSENSOR_INIT_SECTION const unsigned char qma7981InitTable[28]={
	0x36,0xb6,
	0xFC,0X05,      //delay 5ms
	0x36,0x00,
	0xFC,0X05,		//delay 5ms
	0x11,0x80,
	0x0F,0x02,
	0x10,0xE1,
	-1,-1,
};






GSENSOR_OP_SECTION const _Gsensor_Adpt_ qma7981_cmd= {
	.init    = (uint8_t *)qma7981InitTable,
	.local_xyz = {0x01,0x02,0x03,0x04,0x05,0x06},
};


const _Gsensor_Ident_ qma7981_init=
{
	0xe7,0x26,0x27,0x01,0x01,0x00
};


