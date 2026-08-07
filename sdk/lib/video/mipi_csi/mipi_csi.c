#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "lib/video/dvp/cmos_sensor/csi_v2.h"
#include "lib/video/mipi_csi/mipi_csi.h"
#include "devid.h"
#include "hal/gpio.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "dev/vpp/hgvpp.h"
#include "dev/csi/hgdvp.h"
#include "dev/mipi_csi/hgmipi_csi.h"
#include "lib/lcd/lcd.h"
#include "hal/jpeg.h"
#include "hal/csi2.h"
#include "hal/pwm.h"
#include "app_iic/app_iic.h"
#include "lib/video/isp/isp_dev.h"
#include "lib/video/vpp/vpp_dev.h"

#ifdef PIN_FROM_PARAM
#include "pin_param.h"
#endif
#include "syscfg.h"

#define IIC_CLK 250000UL

uint8 u8SensorwriteID2,u8SensorreadID2;
uint8 u8Addrbytnum2,u8Databytnum2;

_Sensor_Ident_ *devSensorInit2=NULL;



#define RESET_HIGH()		{gpio_set_val(rsn,1);}

#define RESET_LOW()		    {gpio_set_val(rsn,0);}

static const _Sensor_Ident_ *devSensorInitTable[] = {

#if DEV_SENSOR_GC0308	
	&gc0308_init,
#endif

#if DEV_SENSOR_GC0309	
	&gc0309_init,
#endif

#if DEV_SENSOR_GC0311	
	&gc0311_init,
#endif

#if DEV_SENSOR_GC0312	
	&gc0312_init,
#endif

#if DEV_SENSOR_GC0328	
	&gc0328_init,
#endif

#if DEV_SENSOR_GC0329
	&gc0329_init,
#endif

#if DEV_SENSOR_BF3A03	
	&bf3a03_init,
#endif

#if DEV_SENSOR_BF3703	
	&bf3703_init,
#endif

#if DEV_SENSOR_OV2640	
	&ov2640_init,
#endif

#if DEV_SENSOR_OV7725	
	&ov7725_init,
#endif

#if DEV_SENSOR_OV7670	
	&ov7670_init,
#endif

#if DEV_SENSOR_BF2013
	&bf2013_init,
#endif

#if DEV_SENSOR_H62
    &h62_init,
#endif

#if DEV_SENSOR_H66
    &h66_init,
#endif

#if DEV_SENSOR_GC1084
   &gc1084_init,
#endif

#if DEV_SENSOR_GC1054
   &gc1054_init,
#endif

#if DEV_SENSOR_SC1336
    &sc1336_init,
#endif

#if DEV_SENSOR_SC1346
    &sc1346_init,
#endif

#if DEV_SENSOR_XC7016_H63	
	&xc7016_h63_init,
#endif

#if DEV_SENSOR_XC7011_H63	
	&xc7011_h63_init,
#endif

#if DEV_SENSOR_XC7011_GC1054	
	&xc7011_gc1054_init,
#endif

#if DEV_SENSOR_GC2053
    &gc2053_init,
#endif

#if DEV_SENSOR_XCG532	
	&xcg532_init,
#endif

#if DEV_SENSOR_GC2145
	&gc2145_init,
#endif

#if DEV_SENSOR_SP0718
	&sp0718_init,
#endif
	
#if DEV_SENSOR_SP0A19
	&sp0a19_init,
#endif
	
#if DEV_SENSOR_BF3720
	&bf3720_init,
#endif

#if DEV_SENSOR_SC2336P
    &sc2336p_init,
#endif

#if DEV_SENSOR_GC2083
    &gc2083_init,
#endif

#if DEV_SENSOR_OV9734
    &ov9734_init,
#endif

#if DEV_SENSOR_GC20C3
    &gc20C3_init,
#endif

#if DEV_SENSOR_XS9950
    &xs9950_init,
#endif

#if DEV_SENSOR_F37P
    &f37p_init,
#endif

#if DEV_SENSOR_F38P
    &f38p_init,
#endif

	NULL,
};

static const _Sensor_Adpt_ *devSensorOPTable[] = {

#if DEV_SENSOR_GC0308	
	&gc0308_cmd,
#endif

#if DEV_SENSOR_GC0309	
	&gc0309_cmd,
#endif

#if DEV_SENSOR_GC0311	
	&gc0311_cmd,
#endif

#if DEV_SENSOR_GC0312	
	&gc0312_cmd,
#endif

#if DEV_SENSOR_GC0328	
	&gc0328_cmd,
#endif

#if DEV_SENSOR_GC0329	
	&gc0329_cmd,
#endif

#if DEV_SENSOR_BF3A03	
	&bf3a03_cmd,
#endif

#if DEV_SENSOR_BF3703	
	&bf3703_cmd,
#endif

#if DEV_SENSOR_OV2640	
	&ov2640_cmd,
#endif

#if DEV_SENSOR_OV7725	
	&ov7725_cmd,
#endif

#if DEV_SENSOR_OV7670	
	&ov7670_cmd,
#endif

#if DEV_SENSOR_BF2013
	&bf2013_cmd,
#endif

#if DEV_SENSOR_H62
    &h62_cmd,
#endif

#if DEV_SENSOR_H66
    &h66_cmd,
#endif

#if DEV_SENSOR_GC1084
   &gc1084_cmd,
#endif

#if DEV_SENSOR_GC1054
   &gc1054_cmd,
#endif

#if DEV_SENSOR_SC1336
    &sc1336_cmd,
#endif

#if DEV_SENSOR_SC1346
    &sc1346_cmd,
#endif

#if DEV_SENSOR_XC7016_H63
	&xc7016_h63_cmd,
#endif

#if DEV_SENSOR_XC7011_H63
	&xc7011_h63_cmd,
#endif

#if DEV_SENSOR_XC7011_GC1054	
	&xc7011_gc1054_cmd,
#endif

#if DEV_SENSOR_GC2053
    &gc2053_cmd,
#endif

#if DEV_SENSOR_XCG532	
	&xcg532_cmd,
#endif

#if DEV_SENSOR_GC2145
	&gc2145_cmd,
#endif

#if DEV_SENSOR_SP0718
	&sp0718_cmd,
#endif

#if DEV_SENSOR_SP0A19
	&sp0a19_cmd,
#endif

#if DEV_SENSOR_BF3720
	&bf3720_cmd,
#endif


#if DEV_SENSOR_SC2336P
    &sc2336p_cmd,
#endif

#if DEV_SENSOR_GC2083
    &gc2083_cmd,
#endif

#if DEV_SENSOR_OV9734
    &ov9734_cmd,
#endif

#if DEV_SENSOR_GC20C3
	&gc20C3_cmd,
#endif

#if DEV_SENSOR_XS9950
    &xs9950_cmd,
#endif

#if DEV_SENSOR_F37P
    &f37p_cmd,
#endif

#if DEV_SENSOR_F38P
    &f38p_cmd,
#endif
};

const _Sensor_Ident_ null_init2={0x00,0x00,0x00,0x00,0x00,0x00};


int sensor2CheckId(uint8_t devid,const _Sensor_Ident_ *p_sensor_ident,const _Sensor_Adpt_ *p_sensor_adpt)
{
	int8 u8Buf[3];
	uint8_t tablebuf[16];
	uint32 id= 0;
	uint32 i = 0;
	uint32 k = 0;

	u8SensorwriteID2 = p_sensor_ident->w_cmd;
	u8SensorreadID2 =p_sensor_ident->r_cmd;
	u8Addrbytnum2 = p_sensor_ident->addr_num;
	u8Databytnum2 = p_sensor_ident->data_num;
	tablebuf[0] = p_sensor_ident->addr_num;
	tablebuf[1] = p_sensor_ident->data_num;
	
//	i2c_ioctl(p_iic,IIC_SET_DEVICE_ADDR,u8SensorwriteID2>>1);
	tablebuf[2] = u8SensorwriteID2>>1;

	if(p_sensor_adpt->preset){
		for(i=0;;i+=p_sensor_ident->data_num+p_sensor_ident->addr_num){
			if((p_sensor_adpt->preset[i]==0xFF)&&(p_sensor_adpt->preset[i+1]==0xFF)){
				os_printf("preset table num:%d\r\n",i);
				break;
			}	
			
			for(k = 0;k < (p_sensor_ident->addr_num + p_sensor_ident->data_num);k++){
				tablebuf[3+k] = p_sensor_adpt->preset[i+k];
			}

			wake_up_iic_queue(devid,tablebuf,0,1,(uint8_t*)NULL);
			while(iic_devid_finish(devid) != 1){
				os_sleep_ms(1);
			}
			//i2c_write(p_iic, (int8*)&p_sensor_adpt->preset[i],p_sensor_ident->addr_num, (int8*)&p_sensor_adpt->preset[i+p_sensor_ident->addr_num], p_sensor_ident->data_num);
			
		}		
	}


	u8Buf[0] = p_sensor_ident->id_reg;
	if(p_sensor_ident->addr_num == 2)
	{
		u8Buf[0] = p_sensor_ident->id_reg>>8;
		u8Buf[1] = p_sensor_ident->id_reg;
	}
	k = 0;
	tablebuf[3+k] = u8Buf[0];
	k++;
	if(p_sensor_ident->addr_num == 2){
		tablebuf[3+k] = u8Buf[1];
		k++;
	}

	tablebuf[3+k] = tablebuf[4+k] = tablebuf[5+k] = tablebuf[6+k] = 0;
	//i2c_read(p_iic,u8Buf,p_sensor_ident->addr_num,(int8*)&id,p_sensor_ident->data_num);
	wake_up_iic_queue(devid,tablebuf,0,0,(uint8_t*)NULL);
	while(iic_devid_finish(devid) != 1){
		os_sleep_ms(1);
	}
	
	id = tablebuf[3+k] | (tablebuf[4+k]<<8) | (tablebuf[5+k]<<16) | (tablebuf[6+k]<<24);
	os_printf("SID: %x, %x, %x, %x,%x\r\n",id,p_sensor_ident->id,u8SensorwriteID2,u8SensorreadID2,p_sensor_ident->id_reg);
	if(id == p_sensor_ident->id){
		iic_devid_set_addr(devid,u8SensorwriteID2>>1);
		return 1;
	}
	else{
		return -1;
	}
}


/*******************************************************************************
* Function Name  : sensor_reset
* Description    : for sensor reset before start
* Input          : nop number
* Output         : None
* Return         : None
*******************************************************************************/
__weak void mipi_sensor_reset(void){
	uint8_t rsn,pdn;

	pdn = MACRO_PIN(PIN_CSI0_PDN);//get_func_io("dvp", "PDN");
	if(pdn != 255){
		gpio_iomap_output(pdn,GPIO_IOMAP_OUTPUT);
		gpio_set_val(pdn,0);
		os_sleep_ms(200);
		gpio_set_val(pdn,1);
		os_sleep_ms(200);
	}


	rsn = MACRO_PIN(PIN_CSI0_RESET);//get_func_io("dvp", "RSN");
	if(rsn != 255){
		gpio_iomap_output(rsn,GPIO_IOMAP_OUTPUT);
	}
	else{
		return;
	}
	
	RESET_HIGH();
	os_sleep_ms(50);
	RESET_LOW();
	os_sleep_ms(200);
	RESET_HIGH();
	os_sleep_ms(50);

}

/*******************************************************************************
* Function Name  : sensor_reset
* Description    : for sensor reset before start
* Input          : nop number
* Output         : None
* Return         : None
*******************************************************************************/
__weak void mipi2_sensor_reset(void){
	uint8_t rsn,pdn;

	pdn = MACRO_PIN(PIN_CSI1_PDN);//get_func_io("dvp", "PDN");
	if(pdn != 255){
		gpio_iomap_output(pdn,GPIO_IOMAP_OUTPUT);
		gpio_set_val(pdn,0);
		os_sleep_ms(1);
		gpio_set_val(pdn,1);
	}


	rsn = MACRO_PIN(PIN_CSI1_RESET);//get_func_io("dvp", "RSN");
	if(rsn != 255){
		gpio_iomap_output(rsn,GPIO_IOMAP_OUTPUT);
	}
	else{
		return;
	}
	
	RESET_HIGH();
	os_sleep_ms(50);
	RESET_LOW();
	os_sleep_ms(200);
	RESET_HIGH();
	os_sleep_ms(50);

}

/*******************************************************************************
* Function Name  : Sensor_ReadRegister
* Description    : read sensor register
* Input          : *pbdata :sensor register addr
                   u8AddrLength:sensor register addr length
                   u8DataLength:sensor register data length
* Output         : None
* Return         : u32i2cReadResult:the result from sensor register
*******************************************************************************/
static _Sensor_Adpt_ * sensorAutoCheck(uint8_t devid,uint8 *init_buf)
{
	uint8 i;
	_Sensor_Adpt_ * devSensor_Struct=NULL;
	for(i=0;devSensorInitTable[i] != NULL;i++)
	{		
		mipi_sensor_reset();
		
		//if(sensor2CheckId(devid,&gc1084_init2,&gc1084_cmd2)>=0)
		if(sensor2CheckId(devid,devSensorInitTable[i],devSensorOPTable[i])>=0)
		{
			os_printf("id =%x num:%d \n",devSensorInitTable[i]->id,i);
			//devSensorInit2 = (_Sensor_Ident_ *)&gc1084_init2;
			devSensorInit2 = (_Sensor_Ident_ *) devSensorInitTable[i];
//#if CMOS_AUTO_LOAD
//			devSensor_Struct = sensor_adpt_load(devSensorInit->id,devSensorInit->w_cmd,devSensorInit->r_cmd,init_buf);
//#else
			devSensor_Struct = (_Sensor_Adpt_ *) devSensorOPTable[i];
//#endif
			break;
		}
	}
	if(devSensor_Struct == NULL)
	{
		os_printf("Er: unkown!");
		devSensorInit2 = (_Sensor_Ident_ *)&null_init2;
		return NULL; // index 0 is test only
	}
	return devSensor_Struct;
}

/*******************************************************************************
* Function Name  : Sensor_ReadRegister
* Description    : read sensor register
* Input          : *pbdata :sensor register addr
                   u8AddrLength:sensor register addr length
                   u8DataLength:sensor register data length
* Output         : None
* Return         : u32i2cReadResult:the result from sensor register
*******************************************************************************/
#if DOUBLE_LANE == 0
static _Sensor_Adpt_ * sensor2AutoCheck(uint8_t devid,uint8 *init_buf)
{
	uint8 i = 0;
	_Sensor_Adpt_ * devSensor_Struct=NULL;
	//for(i=0;devSensorInitTable[i] != NULL;i++)
	{		
		#if DEV_SENSOR_GC1084 == 1
		//mipi2_sensor_reset();
		if(sensor2CheckId(devid,&gc1084_init2,&gc1084_cmd2)>=0)
		{
			os_printf("id =%x num:%d \n",gc1084_init2.id,i);
			devSensorInit2 = (_Sensor_Ident_ *)&gc1084_init2;
//#if CMOS_AUTO_LOAD
//			devSensor_Struct = sensor_adpt_load(devSensorInit->id,devSensorInit->w_cmd,devSensorInit->r_cmd,init_buf);
//#else
			devSensor_Struct = (_Sensor_Adpt_ *) &gc1084_cmd2;
//#endif
//			break;
		}
		#endif
	}
	if(devSensor_Struct == NULL)
	{
		os_printf("Er: unkown!");
		devSensorInit2 = (_Sensor_Ident_ *)&null_init2;
		return NULL; // index 0 is test only
	}
	return devSensor_Struct;
}
#endif

void mipi_csi_fovie_isr(uint32 irq,uint32 dev,uint32 param){
	//dvp_vpp_reset();
	os_printf(KERN_ERR"------------------------------------------------------------------------------mipi fv\r\n");
}

void mipi_csi_sip_isr(uint32 irq,uint32 dev,uint32 param){
	//dvp_vpp_reset();
	os_printf(KERN_ERR"sip reset mipi\r\n");
}

uint32_t io_mipi_csi1_remap_cfg(){
	uint32_t cfg;
	cfg = 0x08040800;
	if((MACRO_PIN(PIN_MIPI_CSI1_CLKN) == PD_1)||((MACRO_PIN(PIN_MIPI_CSI1_CLKN) == PD_0))){          //lane 0 for clk lane
		if(MACRO_PIN(PIN_MIPI_CSI1_CLKN) == PD_1){
			cfg |= (1<<2);
		}
		if(MACRO_PIN(PIN_MIPI_CSI0_D1N_CSI1_D0N) == PA_0){
			cfg |= (1<<4);
		}
	}else if((MACRO_PIN(PIN_MIPI_CSI1_CLKN) == PA_1)||((MACRO_PIN(PIN_MIPI_CSI1_CLKN) == PA_0))){   //lane 1 for clk lane
		cfg |= (1<<0);
		if(MACRO_PIN(PIN_MIPI_CSI1_CLKN) == PA_0){
			cfg |= (1<<2);
		}
		if(MACRO_PIN(PIN_MIPI_CSI0_D1N_CSI1_D0N) == PD_1){
			cfg |= (1<<4);
		}
	}
	return cfg;
}



uint32_t io_mipi_csi0_remap_cfg(uint8_t lane_num){
	uint32_t cfg;
	cfg = 0x08040800;
	if(lane_num == 1){
		if((MACRO_PIN(PIN_MIPI_CSI0_CLKN) == PA_3)||((MACRO_PIN(PIN_MIPI_CSI0_CLKN) == PA_2))){          //lane 0 for clk lane
			if(MACRO_PIN(PIN_MIPI_CSI0_CLKN) == PA_2){
				cfg |= (1<<2);
			}
			if(MACRO_PIN(PIN_MIPI_CSI0_D0N) == PA_4){
				cfg |= (1<<4);
			}
		}else if((MACRO_PIN(PIN_MIPI_CSI0_CLKN) == PA_5)||((MACRO_PIN(PIN_MIPI_CSI0_CLKN) == PA_4))){   //lane 1 for clk lane
			cfg |= (1<<0);
			if(MACRO_PIN(PIN_MIPI_CSI0_CLKN) == PA_4){
				cfg |= (1<<2);
			}
			if(MACRO_PIN(PIN_MIPI_CSI0_D0N) == PA_2){
				cfg |= (1<<4);
			}
		}
	}else{
		if((MACRO_PIN(PIN_MIPI_CSI0_CLKN) == PA_3)||((MACRO_PIN(PIN_MIPI_CSI0_CLKN) == PA_2))){          //lane 0 for clk lane
			if(MACRO_PIN(PIN_MIPI_CSI0_CLKN) == PA_2){
				cfg |= (1<<2);
			}

			if((MACRO_PIN(PIN_MIPI_CSI0_D0N) == PA_5)||((MACRO_PIN(PIN_MIPI_CSI0_D0N) == PA_4))){
				if(MACRO_PIN(PIN_MIPI_CSI0_D0N) == PA_4){
					cfg |= (1<<4);
				}
				if(MACRO_PIN(PIN_MIPI_CSI0_D1N_CSI1_D0N) == PA_0){
					cfg |= (1<<5);
				}
			}else{
				cfg |= (1<<3);
				if(MACRO_PIN(PIN_MIPI_CSI0_D0N) == PA_0){     
					cfg |= (1<<4);
				}
				if(MACRO_PIN(PIN_MIPI_CSI0_D1N_CSI1_D0N) == PA_4){
					cfg |= (1<<5);
				}				
			}
		}else if((MACRO_PIN(PIN_MIPI_CSI0_CLKN) == PA_5)||((MACRO_PIN(PIN_MIPI_CSI0_CLKN) == PA_4))){   //lane 1 for clk lane
			cfg |= (1<<0);
			if(MACRO_PIN(PIN_MIPI_CSI0_CLKN) == PA_4){
				cfg |= (1<<2);
			}

			if((MACRO_PIN(PIN_MIPI_CSI0_D0N) == PA_3)||((MACRO_PIN(PIN_MIPI_CSI0_D0N) == PA_2))){        //lane0
				if(MACRO_PIN(PIN_MIPI_CSI0_D0N) == PA_2){
					cfg |= (1<<4);
				}
				if(MACRO_PIN(PIN_MIPI_CSI0_D1N_CSI1_D0N) == PA_0){
					cfg |= (1<<5);
				}
			}else{
				cfg |= (1<<3);
				if(MACRO_PIN(PIN_MIPI_CSI0_D0N) == PA_0){     
					cfg |= (1<<4);
				}
				if(MACRO_PIN(PIN_MIPI_CSI0_D1N_CSI1_D0N) == PA_2){
					cfg |= (1<<5);
				}				
			}

		}else if((MACRO_PIN(PIN_MIPI_CSI0_CLKN) == PA_1)||((MACRO_PIN(PIN_MIPI_CSI0_CLKN) == PA_0))){   //lane 1 for clk lane
			cfg |= (2<<0);
			if(MACRO_PIN(PIN_MIPI_CSI0_CLKN) == PA_0){
				cfg |= (1<<2);
			}			

			if((MACRO_PIN(PIN_MIPI_CSI0_D0N) == PA_3)||((MACRO_PIN(PIN_MIPI_CSI0_D0N) == PA_2))){        //lane0
				if(MACRO_PIN(PIN_MIPI_CSI0_D0N) == PA_2){
					cfg |= (1<<4);
				}
				if(MACRO_PIN(PIN_MIPI_CSI0_D1N_CSI1_D0N) == PA_4){
					cfg |= (1<<5);
				}
			}else{
				cfg |= (1<<3);
				if(MACRO_PIN(PIN_MIPI_CSI0_D0N) == PA_4){     
					cfg |= (1<<4);
				}
				if(MACRO_PIN(PIN_MIPI_CSI0_D1N_CSI1_D0N) == PA_2){
					cfg |= (1<<5);
				}				
			}
		}
	}
	
	return cfg;
}

struct mipi_csi_priv {
	uint8_t mipi_csi0_en 			: 1,
			mipi_csi1_en 			: 1,
			mipi_csi_init   		: 1,
			mipi_csi0_data_lane_num : 2,
			mipi_csi1_data_lane_num : 2,
			reserve					: 1;
};
static struct mipi_csi_priv g_mipi_csi_priv = {0};

void mipi_csi_debug_config(struct mipi_csi_debug *p_debug, uint32 csi_dev_id)
{
    if (!p_debug->debug_enable)     return;

    sysctrl_ace_peris_access_cpu_all(ACE_MIX_TOP|ACE_GPIO_TOP|ACE_EFUSE_CTRL|ACE_SYS_SEC_TOP|ACE_PMU|ACE_BASEBAND1|ACE_BASEBAND|ACE_RFDIGITAL|ACE_RFDIGCAL_TOP);

    SYSCTRL->SYS_CON0 |= BIT(3);
	SYSCTRL->SYS_CON1 |= BIT(21);  //lmac reset
	SYSCTRL->CLK_CON2 |= BIT(22);  //lmac clk enable

	gpio_iomap_output(p_debug->debug_io0, GPIO_IOMAP_OUT_DBGPATH_DBGO_0); //dbg01
	gpio_iomap_output(p_debug->debug_io1, GPIO_IOMAP_OUT_DBGPATH_DBGO_1); //dbg1
	gpio_iomap_output(p_debug->debug_io2, GPIO_IOMAP_OUT_DBGPATH_DBGO_2); //dbg2
	gpio_iomap_output(p_debug->debug_io3, GPIO_IOMAP_OUT_DBGPATH_DBGO_3); //dbg3
	gpio_iomap_output(p_debug->debug_io4, GPIO_IOMAP_OUT_DBGPATH_DBGO_4); //dbg2
	gpio_iomap_output(p_debug->debug_io5, GPIO_IOMAP_OUT_DBGPATH_DBGO_5); //dbg3

    //dbg0 mipi csi 
    *(volatile uint32 *)0x40062ef0 &= ~(0xff<<0); 
    *(volatile uint32 *)0x40062ef0 |= (128+p_debug->debug_type0)<<0; //6:clk lane dp
    //dbg1
    *(volatile uint32 *)0x40062ef0 &= ~(0xff<<8); 
    *(volatile uint32 *)0x40062ef0 |= (128+p_debug->debug_type1)<<8; //7:clk lane dn
    //dbg2
    *(volatile uint32 *)0x40062ef0 &= ~(0xff<<16); 
    *(volatile uint32 *)0x40062ef0 |= (128+p_debug->debug_type2)<<16;//8:data lane0 dp
    //dbg3
    *(volatile uint32 *)0x40062ef0 &= ~(0xff<<24); 
    *(volatile uint32 *)0x40062ef0 |= (128+p_debug->debug_type3)<<24;//9:data lane0 dn
    //dbg4
    *(volatile uint32 *)0x40062ef4 &= ~(0xff<<0); 
    *(volatile uint32 *)0x40062ef4 |= (128+p_debug->debug_type4)<<0; //10:data lane1 dp
    //dbg5
    *(volatile uint32 *)0x40062ef4 &= ~(0xff<<8); 
    *(volatile uint32 *)0x40062ef4 |= (128+p_debug->debug_type5)<<8; //11:data lane1 dp

    // os_printf("type : %d value : %d\r\n", p_debug->debug_type5, *(volatile uint32 *)0x40062ef4);
    volatile uint32 *debug = (void *)(0x40006784 + (csi_dev_id - (uint32)HG_MIPI_CSI_DEVID) * 0x200);

    for (int i = 0; i < 3; i++)
    {
        os_printf(KERN_DEBUG"****** mipi id : %d debug %08x %08x %08x %08x************\r\n", csi_dev_id, debug[0], debug[1], debug[2], debug[3]);
        os_sleep_ms(1000);
    }
}
extern struct sys_config sys_cfgs;
uint32 mipi_csi_check(uint32 csi_data_lane_num, uint32 csi_dev_id, _Sensor_Adpt_ *p_sensor_cmd)
{
    uint8                  rx_zero_cnt   = 0x08;
    uint8                  thr_buf[10]   = {0};
    uint8                  thr_index     = 0;
    uint32                 state         = 0;
    uint32                 check_time    = 0;
	uint32        		   match_value   = 0;
    uint32                 data_lane0    = 0;
    uint32                 data_lane1    = 0;
    struct mipi_csi_device *p_dev        = (struct mipi_csi_device *)dev_get(csi_dev_id);

    if ((csi_dev_id > HG_MIPI1_CSI_DEVID) || (csi_data_lane_num > 2) || (p_sensor_cmd == NULL) || (p_dev == NULL))
    {
        if ((csi_dev_id > HG_MIPI1_CSI_DEVID))  os_printf(KERN_ERR"%s check csi_dev_id err!\r\n", __func__);
        if (csi_data_lane_num > 2)              os_printf(KERN_ERR"%s check csi data lane_num %d err!\r\n", __func__, csi_data_lane_num);
        if (p_sensor_cmd == NULL)               os_printf(KERN_ERR"%s get sensor_adpt err\r\n", __func__);
        if (p_dev == NULL)               		os_printf(KERN_ERR"%s get mipi csi id : %d err\r\n", __func__, csi_dev_id);
        return FALSE;
    }

    if ((sys_cfgs.mipi_csi0_hs_zero_cnt && (csi_dev_id == HG_MIPI_CSI_DEVID ) && (devSensorInit2->id == sys_cfgs.mipi_csi0_sensor_id)) ||
        (sys_cfgs.mipi_csi1_hs_zero_cnt && (csi_dev_id == HG_MIPI1_CSI_DEVID) && (devSensorInit2->id == sys_cfgs.mipi_csi1_sensor_id)))
    {
        rx_zero_cnt = (csi_dev_id == HG_MIPI_CSI_DEVID) ? (sys_cfgs.mipi_csi0_hs_zero_cnt) : (sys_cfgs.mipi_csi1_hs_zero_cnt);
        mipi_csi_hs_rx_zero_cnt_set(p_dev, rx_zero_cnt);
        os_printf(KERN_DEBUG"mipi id:%d read syscfg head_thr : %d succ!\r\n", csi_dev_id, rx_zero_cnt);
        return TRUE;
    } 

    check_time = os_jiffies();
    while(1)
    {
        mipi_csi_get_stop_state(p_dev, &state);
        if ((state & BIT(0)) || ((os_jiffies() - check_time) > 500))
        {
            break;
        } else {
            delay_us(5);
        }
    }
    check_time = os_jiffies();
    while(1)
    {
        mipi_csi_get_stop_state(p_dev, &state);
        if (!(state & BIT(0)) || ((os_jiffies() - check_time) > 500))
        {
            break;
        } else {
            delay_us(5);
        }
    }

    while(rx_zero_cnt < 0x30)
    {
        mipi_csi_hs_rx_zero_cnt_set(p_dev, rx_zero_cnt);
        mipi_csi_get_match_value(p_dev, (void *)&match_value);
        data_lane0 = (match_value >>  0) & 0xffff;
        data_lane1 = (match_value >> 16) & 0xffff;
        if (csi_data_lane_num == 1)
        {
            if (((data_lane0 >> 3) & 0xff) > 0x3f)
            {
                thr_buf[thr_index++] = rx_zero_cnt;
            }
        } else {
            if ((((data_lane0 >> 3) & 0xff) > 0x3f) && (((data_lane1 >> 3) & 0xff) > 0x3f))
            {
                thr_buf[thr_index++] = rx_zero_cnt;
            }
        }
        rx_zero_cnt += 4;
    }
    
    if (thr_index)
    {
        mipi_csi_hs_rx_zero_cnt_set(p_dev, thr_buf[(thr_index - 1) >> 1]);
        os_printf(KERN_DEBUG"mipi csi index : %d head_thr : %d succ!\r\n", thr_index, thr_buf[thr_index >> 1]);
        if (csi_dev_id == HG_MIPI_CSI_DEVID)
        {
            sys_cfgs.mipi_csi0_hs_zero_cnt = thr_buf[thr_index >> 1];
            sys_cfgs.mipi_csi0_sensor_id   = devSensorInit2->id;
        } else {
            sys_cfgs.mipi_csi1_hs_zero_cnt = thr_buf[thr_index >> 1];
            sys_cfgs.mipi_csi1_sensor_id   = devSensorInit2->id;
        }
        syscfg_save();
        return TRUE;
    } else {
        os_printf(KERN_ERR"mipi csi set head thr err index : %d!\r\n", thr_index);
        return FALSE;
    }
}

int mipi_csi_sensor_init(_Sensor_Adpt_ *p_sensor_cmd, uint8_t mipi_csi_iic)
{
	uint32_t itk = 0;
	uint32_t i = 0;
	uint32_t k = 0;
	uint8_t idbuf[8];
	uint8_t tablebuf[16];
	if(p_sensor_cmd->init!=NULL)
	{
		os_printf("SENSER....init\r\n");
		for(i = 0 ; ; i+=u8Addrbytnum2+u8Databytnum2)
		{
			if((p_sensor_cmd->init[i]==0xFF)&&(p_sensor_cmd->init[i+1]==0xFF)){
				os_printf("init table num:%d\r\n",i);
				break;
			}

			if((p_sensor_cmd->init[i]==0xFE)&&(p_sensor_cmd->init[i+1]==0xFE)){
				if(p_sensor_cmd->init[i+2]==0x01){
					os_sleep_ms(100);
				}
			}
			else{
				for(itk = 0;itk < u8Addrbytnum2+u8Databytnum2;itk++){
					idbuf[itk] = p_sensor_cmd->init[i+itk];
				}

				tablebuf[0] = u8Addrbytnum2;
				tablebuf[1] = u8Databytnum2;
				tablebuf[2] = u8SensorwriteID2>>1;				
				for(k = 0;k < (tablebuf[0] + tablebuf[1]);k++){
					tablebuf[3+k] = idbuf[k];
				}

				wake_up_iic_queue(mipi_csi_iic,tablebuf,0,1,(uint8_t*)NULL);
				while(iic_devid_finish(mipi_csi_iic) != 1){
					os_sleep_ms(1);
				}
				
				if(i==0)
				{
					os_sleep_ms(1);
				}
			}
		}		
	}
	return 0;
}
/***************************************
 * mclk:设置mclk的频率,单位是MHz
 * 
 * 如果是用pwm去设置mclk,设置值如果不能
 * 整数分频,只会设置更小的值
****************************************/
int mipi_csi_hardware_config(uint32_t csi_dev_id, uint8_t init_en, uint8_t csi_data_lane_num, uint8_t dual_en, uint8_t dual_sensor_type,uint8_t mclk, struct mipi_csi_debug *p_debug) 
{
	
	uint32_t cfg = 0;
	_Sensor_Adpt_ *p_sensor_cmd = NULL;
	//struct i2c_setting i2c_setting;
	struct i2c_device *iic_dev = (struct i2c_device *)dev_get(HG_I2C1_DEVID);
	struct mipi_csi_device *mipi_csi_dev = (struct mipi_csi_device *)dev_get(HG_MIPI_CSI_DEVID);
	struct mipi_csi_device *mipi1_csi_dev = (struct mipi_csi_device *)dev_get(HG_MIPI1_CSI_DEVID);

	#if DUAL_EN
	gpio_set_mode(PC_14, GPIO_PULL_DOWN, GPIO_PULL_LEVEL_100K);
	gpio_iomap_output(PC_14,GPIO_IOMAP_OUT_DUAL_ORG_FSYNC);
	#endif
	
	os_printf("mipi_csi_dev:%08x  mipi1_csi_dev:%08x\r\n",mipi_csi_dev,mipi1_csi_dev);

	if(!init_en){
		return 0;
	}

	if(!g_mipi_csi_priv.mipi_csi_init) {
		os_printf("%s %d\n",__FUNCTION__,__LINE__);
		mipi_csi_close(mipi_csi_dev);
		mipi_csi_init(mipi_csi_dev,((DOUBLE_LANE) ? (2) : (csi_data_lane_num)));
		mipi_csi_close(mipi1_csi_dev);
		mipi_csi_init(mipi1_csi_dev,1); 
		#if 1
		struct hgpwm_v0 *global_hgpwm = (struct hgpwm_v0 *)dev_get(HG_PWM0_DEVID);
		gpio_driver_strength(MACRO_PIN(PIN_PWM_CHANNEL_0), GPIO_DS_G1);
		uint32_t mclk_div = (DEFAULT_SYS_CLK/1000000)/mclk;
		//设置分频比,mclk_div-1,然后设置占空比:50%
		pwm_init((struct pwm_device *)global_hgpwm, PWM_CHANNEL_0, mclk_div-1, (mclk_div+1)>>1);
		pwm_start((struct pwm_device *)global_hgpwm, PWM_CHANNEL_0);
		#else
		gpio_iomap_output(PE_0, GPIO_IOMAP_OUT_DVP_MCLK_OUT);
		gpio_driver_strength(PE_0,	GPIO_DS_G1);
		mipi_csi_set_baudrate(mipi_csi_dev,6000000);
		#endif
		os_sleep_ms(300);

		g_mipi_csi_priv.mipi_csi_init = 1;
	}

	if(csi_dev_id == HG_MIPI_CSI_DEVID) {
		
		csi_data_lane_num = (DOUBLE_LANE) ? (2) : (csi_data_lane_num);

		uint8_t mipi_csi0_iic = register_iic_queue(iic_dev,MACRO_PIN(PIN_MIPI0_IIC_CLK),MACRO_PIN(PIN_MIPI0_IIC_SDA),0);
		os_printf("set mipi sensor finish ,Auto Check sensor id\r\n");
		p_sensor_cmd = sensorAutoCheck(mipi_csi0_iic,NULL);
		if(p_sensor_cmd == NULL){
			return FALSE;
		}
		os_printf("Auto Check sensor id finish\r\n");

		sensor_info_add(dual_sensor_type, (dual_en) ? (ISP_INPUT_DAT_SRC_ORG_DMA) : (ISP_INPUT_DAT_SRC_MIPI0), (uint32)p_sensor_cmd->sensor_isp, mipi_csi0_iic, u8SensorwriteID2);
		//	mipi_csi_set_baudrate(mipi_csi_dev,p_sensor_cmd->mclk);

		g_mipi_csi_priv.mipi_csi0_data_lane_num = csi_data_lane_num;
		mipi_csi_set_lane_num(mipi_csi_dev, csi_data_lane_num);
		mipi_csi_open_virtual_channel(mipi_csi_dev,0);

		if(csi_data_lane_num == 1){
			cfg = io_mipi_csi0_remap_cfg(1);
			mipi_csi_dphy_cfg(mipi_csi_dev,cfg,0x17709de7,0x10,0x3def,0,0,0,0);
		}else{
			cfg = io_mipi_csi0_remap_cfg(2);
			mipi_csi_dphy_cfg(mipi_csi_dev,cfg,0x17709de7,0x20,0x3def,BIT(30),0,0,0);
			mipi_csi_dphy_cfg(mipi1_csi_dev,0x08040800,0x17709de7,0x80020,0x3def,0x00000000,0,0,0); //mipi_csi1 bias on
		}
		os_printf("p_sensor_cmd->pixelw:%d p_sensor_cmd->pixelh:%d\n", p_sensor_cmd->pixelw, p_sensor_cmd->pixelh);
		mipi_csi_img_size(mipi_csi_dev,p_sensor_cmd->pixelw,p_sensor_cmd->pixelh);
		//mipi_csi_crop_enable(mipi_csi_dev,0);
		//mipi_csi_crop_img_start(mipi_csi_dev,0,0);
		//mipi_csi_crop_img_end(mipi_csi_dev,1920,1080);
		//mipi_csi_hsync_rec_time(mipi_csi_dev,5);
		mipi_csi_hsync_rec_enable(mipi_csi_dev,1);	
		mipi_csi_input_format(mipi_csi_dev,INPUT_MODE); 
		mipi_csi_request_irq(mipi_csi_dev,CSI2_VSIP_ISR, (mipi_csi_irq_hdl )&mipi_csi_sip_isr,0);
		mipi_csi_request_irq(mipi_csi_dev,CSI2_FOVIE_ISR,(mipi_csi_irq_hdl )&mipi_csi_fovie_isr,0);
		mipi_csi_open(mipi_csi_dev);

		os_printf("mclk:%dMHz\r\n",p_sensor_cmd->mclk);	
		os_printf("init:%x u8Addrbytnum2:%d,u8Databytnum2:%d\r\n",(uint32)p_sensor_cmd->init,u8Addrbytnum2,u8Databytnum2);
		mipi_csi_sensor_init(p_sensor_cmd, mipi_csi0_iic);
	}

	if(csi_dev_id == HG_MIPI1_CSI_DEVID) {
#if DOUBLE_CSI
#if DOUBLE_LANE == 0
		if(g_mipi_csi_priv.mipi_csi0_data_lane_num == 2) {
			os_printf("mipi csi1 unsupport\n");
			return FALSE;
		}

		uint8_t mipi_csi1_iic = register_iic_queue(iic_dev,MACRO_PIN(PIN_MIPI1_IIC_CLK),MACRO_PIN(PIN_MIPI1_IIC_SDA),0);	
		os_printf("set mipi sensor finish ,Auto Check sensor id\r\n");
		p_sensor_cmd =  sensor2AutoCheck(mipi_csi1_iic,NULL);
		os_printf("Auto Check sensor id finish\r\n");

		if(p_sensor_cmd == NULL){
			return FALSE;
		}

		sensor_info_add(dual_sensor_type, (dual_en) ? (ISP_INPUT_DAT_SRC_ORG_DMA) : (ISP_INPUT_DAT_SRC_MIPI1), (uint32)p_sensor_cmd->sensor_isp, mipi_csi1_iic, u8SensorwriteID2);
		//	mipi_csi_set_baudrate(mipi1_csi_dev,p_sensor_cmd->mclk);
		g_mipi_csi_priv.mipi_csi1_data_lane_num = csi_data_lane_num;
		mipi_csi_set_lane_num(mipi1_csi_dev,csi_data_lane_num);
		mipi_csi_open_virtual_channel(mipi1_csi_dev,0);
		cfg = io_mipi_csi1_remap_cfg();
		mipi_csi_dphy_cfg(mipi1_csi_dev,cfg,0x17709de7,0x20,0x3def,0,0,0,0);
		os_printf("p_sensor_cmd->pixelw:%d p_sensor_cmd->pixelh:%d\n", p_sensor_cmd->pixelw, p_sensor_cmd->pixelh);
		mipi_csi_img_size(mipi1_csi_dev,p_sensor_cmd->pixelw,p_sensor_cmd->pixelh);
		mipi_csi_hsync_rec_enable(mipi1_csi_dev,1);	
		mipi_csi_input_format(mipi1_csi_dev,INPUT_MODE);	
		mipi_csi_request_irq(mipi1_csi_dev,CSI2_VSIP_ISR, (mipi_csi_irq_hdl )&mipi_csi_sip_isr,0);
		mipi_csi_request_irq(mipi1_csi_dev,CSI2_FOVIE_ISR,(mipi_csi_irq_hdl )&mipi_csi_fovie_isr,0);
		mipi_csi_open(mipi1_csi_dev);

		os_printf("mclk:%dMHz\r\n",p_sensor_cmd->mclk); 
		os_printf("init:%x u8Addrbytnum2:%d,u8Databytnum2:%d\r\n",(uint32)p_sensor_cmd->init,u8Addrbytnum2,u8Databytnum2);
		mipi_csi_sensor_init(p_sensor_cmd, mipi_csi1_iic);
#endif
#endif
	}

    if (mipi_csi_check(csi_data_lane_num, csi_dev_id, p_sensor_cmd) != TRUE)
    {
        mipi_csi_debug_config(p_debug, csi_dev_id);
		return FALSE;
    } else {
        if (csi_dev_id == HG_MIPI_CSI_DEVID)
        {
            g_mipi_csi_priv.mipi_csi0_en = 1;

			video_msg.csi0_iw = p_sensor_cmd->pixelw;
			video_msg.csi0_ih = p_sensor_cmd->pixelh;
//			video_msg.csi0_ow = p_sensor_cmd->pixelw;
//			video_msg.csi0_oh = p_sensor_cmd->pixelh;
			video_msg.csi0_type = 1;
			video_msg.video_type_cur  = ISP_VIDEO_0;
			video_msg.video_type_last = ISP_VIDEO_0;
			video_msg.video_num++;			
        }

        if (csi_dev_id == HG_MIPI1_CSI_DEVID)
        {
            g_mipi_csi_priv.mipi_csi1_en = 1;

			video_msg.csi1_iw = p_sensor_cmd->pixelw;
			video_msg.csi1_ih = p_sensor_cmd->pixelh;
//			video_msg.csi1_ow = p_sensor_cmd->pixelw;
//			video_msg.csi1_oh = p_sensor_cmd->pixelh;
			video_msg.csi1_type = 1;
			video_msg.video_type_cur  = ISP_VIDEO_0;
			video_msg.video_type_last = ISP_VIDEO_0;
			video_msg.video_num++;			
        }
    }

	return TRUE;
}


void get_single_mipi(uint32_t csi_dev_id,uint16_t *w,uint16_t *h)
{
	if (csi_dev_id == HG_MIPI_CSI_DEVID)
	{
		if(w)
		{
			*w = video_msg.csi0_iw;
		}
		if(h)
		{
			*h = video_msg.csi0_ih;
		}
	}
	else if (csi_dev_id == HG_MIPI1_CSI_DEVID)
	{
		if(w)
		{
			*w = video_msg.csi1_iw;
		}
		if(h)
		{
			*h = video_msg.csi1_ih;
		}
	}
	return;
}
