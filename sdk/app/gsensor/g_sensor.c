#include "sys_config.h"
#include "typesdef.h"
#include "g_sensor.h"
#include "devid.h"
#include "sys_config.h"
#include "hal/gpio.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "dev/csi/hgdvp.h"
#include "osal/sleep.h"

static _Gsensor_Ident_ *devGsensorInit=NULL;

uint8_t gSensorwriteID,gSensorreadID;
uint8_t gAddrbytnum,gDatabytnum;

struct i2c_device *g_sensor_iic;

static const _Gsensor_Ident_ *devGsensorInitTable[] = {
#if DEV_GSENSOR_DA280	
	&da280_init,
#endif	

#if DEV_GSENSOR_QMA7981	
	&qma7981_init,
#endif

#if DEV_GSENSOR_SC7A20	
	&sc7a20_init,
#endif

	NULL,
};


static const _Gsensor_Adpt_ *devGsensorOPTable[] = {
	
#if DEV_GSENSOR_DA280
	&da280_cmd,
#endif

#if DEV_GSENSOR_QMA7981	
	&qma7981_cmd,
#endif

#if DEV_GSENSOR_SC7A20	
	&sc7a20_cmd,
#endif

};

uint8_t x_addr_l,x_addr_h,y_addr_l,y_addr_h,z_addr_l,z_addr_h;


void Delay_nopCnt(uint32_t cnt);
void delay_us(uint32 n);

const _Gsensor_Ident_ gnull_init={0x00,0x00,0x00,0x00,0x00,0x00};

static int gsensorCheckId(struct i2c_device *p_iic,const _Gsensor_Ident_ *p_sensor_ident)
{
	uint8_t u8Buf[3];
	uint32_t id= 0;
	

	gSensorwriteID = p_sensor_ident->w_cmd;
	gSensorreadID =p_sensor_ident->r_cmd;
	gAddrbytnum = p_sensor_ident->addr_num;
	gDatabytnum = p_sensor_ident->data_num;

	u8Buf[0] = p_sensor_ident->id_reg;
	if(p_sensor_ident->addr_num == 2)
	{
		u8Buf[0] = p_sensor_ident->id_reg>>8;
		u8Buf[1] = p_sensor_ident->id_reg;
	}

	
	_os_printf("addr:%x %x %x %x\r\n",p_sensor_ident->addr_num,p_sensor_ident->data_num,gSensorwriteID,gSensorreadID);	
	i2c_ioctl(p_iic,IIC_SET_DEVICE_ADDR,gSensorwriteID>>1);
	//id = i2c_sensor_read(p_iic,u8Buf,p_sensor_ident->addr_num,p_sensor_ident->data_num,gSensorwriteID,gSensorreadID);
	i2c_read(p_iic,(int8*)u8Buf,p_sensor_ident->addr_num,(int8*)&id,p_sensor_ident->data_num);
	_os_printf("SID: %x, %x, %x, %x,%x\r\n",id,p_sensor_ident->id,gSensorwriteID,gSensorreadID,p_sensor_ident->id_reg);
	if(id == p_sensor_ident->id)
		return 1;
	else
		return -1;
}




static _Gsensor_Adpt_ * gsensorAutoCheck(struct i2c_device *p_iic,uint8_t *init_buf)
{
	uint8_t i;
	_Gsensor_Adpt_ * devSensor_Struct=NULL;
	for(i=0;devGsensorInitTable[i] != NULL;i++)
	{
		if(gsensorCheckId(p_iic,devGsensorInitTable[i])>=0)
		{
			_os_printf("id =%x num:%d \n",devGsensorInitTable[i]->id,i);
			devGsensorInit = (_Gsensor_Ident_ *) devGsensorInitTable[i];
			devSensor_Struct = (_Gsensor_Adpt_ *) devGsensorOPTable[i];
			break;
		}
	}
	if(devSensor_Struct == NULL)
	{
		_os_printf("Er: unkown!");
		devGsensorInit = (_Gsensor_Ident_ *)&gnull_init;
		return NULL; // index 0 is test only
	}
	return devSensor_Struct;
}

uint8_t get_gsensor_x_y_z(uint8_t* buf){
	int itk = 0;
	buf[0] = 0x04;
	buf[2] = 0x05;
	buf[4] = 0x06;
	buf[6] = 0x07;
	for(itk = 0;itk < 8;itk=itk+2){
		i2c_read(g_sensor_iic,(int8 *)&buf[itk],gAddrbytnum,(int8*)&buf[itk+1],gDatabytnum);
	}
	return 0;
}

uint8_t get_gsensor_x(uint8_t* buf){
	int itk = 0;
	buf[0] = x_addr_l;
	buf[2] = x_addr_h;
	for(itk = 0;itk < 4;itk=itk+2){
		i2c_read(g_sensor_iic,(int8 *)&buf[itk],gAddrbytnum,(int8*)&buf[itk+1],gDatabytnum);
	}
	return 0;
}

uint8_t get_gsensor_y(uint8_t* buf){
	int itk = 0;
	buf[0] = y_addr_l;
	buf[2] = y_addr_h;
	for(itk = 0;itk < 4;itk=itk+2){
		i2c_read(g_sensor_iic,(int8 *)&buf[itk],gAddrbytnum,(int8*)&buf[itk+1],gDatabytnum);
	}	
	return 0;
}

uint8_t get_gsensor_z(uint8_t* buf){
	int itk = 0;
	buf[0] = z_addr_l;
	buf[2] = z_addr_h;
	for(itk = 0;itk < 4;itk=itk+2){
		i2c_read(g_sensor_iic,(int8 *)&buf[itk],gAddrbytnum,(int8*)&buf[itk+1],gDatabytnum);
	}
	return 0;	
}

uint8_t g_sensor_init(){		
	static _Gsensor_Adpt_ *p_gsensor_cmd = NULL;
	int i,j;
	uint8_t buf[5];
	uint8_t ack;
	
	g_sensor_iic = (struct i2c_device *)dev_get(HG_I2C1_DEVID);
	x_addr_l = x_addr_h = y_addr_l = y_addr_h = z_addr_l = z_addr_h = 0;

	_os_printf("g_sensor_init start,iic init:%x\r\n",(int)g_sensor_iic);

	//G_sensor_test();
	//while(1);
	//iic_init_sensor(G_IIC_CLK,GSENSOR_IIC);
	//i2c_SendStop(GSENSOR_IIC);

	i2c_open(g_sensor_iic, IIC_MODE_MASTER, IIC_ADDR_7BIT, 0);
	i2c_set_baudrate(g_sensor_iic,G_IIC_CLK);
	i2c_ioctl(g_sensor_iic,IIC_SDA_OUTPUT_DELAY,20);	
	i2c_ioctl(g_sensor_iic,IIC_FILTERING,20);
	//i2c_send_stop(g_sensor_iic);
	
	_os_printf("init g_sensor,check id\r\n");
	p_gsensor_cmd = gsensorAutoCheck(g_sensor_iic,NULL);
	if(p_gsensor_cmd == NULL){
		return FALSE;
	}

    if(p_gsensor_cmd->preread!=NULL)
    {

		for(i=0;;i+=gAddrbytnum)
		{
			if((p_gsensor_cmd->preread[i]==0xFF)&&(p_gsensor_cmd->preread[i+1]==0xFF)){
				_os_printf("init pre table num:%d\r\n",i);
				break;
			}
			if((p_gsensor_cmd->preread[i]==0xFC)||(p_gsensor_cmd->preread[i]==0xFD)){
				i += 1;
				if(p_gsensor_cmd->preread[i]==0xFC){
					os_sleep_ms(p_gsensor_cmd->preread[i+1]);
				}else{
					delay_us(p_gsensor_cmd->preread[i+1]);
				}
			}else{
				for(j = 0; j < gAddrbytnum;j++){
					buf[j] = p_gsensor_cmd->preread[i+j];
				}
				//ret = i2c_sensor_read(g_sensor_iic,buf,gAddrbytnum,gDatabytnum,gSensorwriteID,gSensorreadID);
				i2c_read(g_sensor_iic,(int8 *)buf,gAddrbytnum,(int8*)&ack,gDatabytnum);
			}
		}
		
	}


	
    if(p_gsensor_cmd->init!=NULL)
    {
		x_addr_l = p_gsensor_cmd->local_xyz.x_l;
		x_addr_h = p_gsensor_cmd->local_xyz.x_h;
		y_addr_l = p_gsensor_cmd->local_xyz.y_l;
		y_addr_h = p_gsensor_cmd->local_xyz.y_h;
		z_addr_l = p_gsensor_cmd->local_xyz.z_l;
		z_addr_h = p_gsensor_cmd->local_xyz.z_h;
		for(i=0;;i+=gAddrbytnum+gDatabytnum)
		{
			if((p_gsensor_cmd->init[i]==0xFF)&&(p_gsensor_cmd->init[i+1]==0xFF)){
				_os_printf("init table num:%d\r\n",i);
				break;
			}
			if((p_gsensor_cmd->init[i]==0xFC)||(p_gsensor_cmd->init[i]==0xFD)){
				if(p_gsensor_cmd->init[i]==0xFC){
					os_sleep_ms(p_gsensor_cmd->init[i+1]);
				}else{
					delay_us(p_gsensor_cmd->init[i+1]);
				}
			}else{
				//i2c_sensor_write(g_sensor_iic,&p_gsensor_cmd->init[i],gAddrbytnum,gDatabytnum,gSensorwriteID,gSensorreadID);
				i2c_write(g_sensor_iic, (int8*)&p_gsensor_cmd->init[i], gAddrbytnum, (int8*)&p_gsensor_cmd->init[i+gAddrbytnum], gDatabytnum);
			}			
		}
		
	}
	
	return TRUE;
}


