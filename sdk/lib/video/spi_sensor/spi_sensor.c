#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "devid.h"
#include "hal/gpio.h"
#include "hal/spi.h"
#include "hal/timer_device.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "dev/csi/hgdvp.h"
#include "dev/prc/hgprc.h"
#include "hal/pwm.h"
#include "hal/prc.h"
#include "osal/task.h"
#include <csi_kernel.h>
#include "osal/semaphore.h"

#include "lib/video/dvp/jpeg/jpg.h"
#include "osal/sleep.h"

#ifdef PIN_FROM_PARAM
#include "pin_param.h"
#endif
#if PRC_EN
#define IIC_CLK 120000UL
extern SNSER snser;

int wake_up_iic_queue(uint8_t devid,uint8_t *table,uint32_t len,uint8_t rw_sta,uint8_t *trx_buff);
void spi0_read_irq(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2);
int iic_devid_finish(uint8_t devid);
int register_iic_queue(struct i2c_device *i2c,uint8_t scl_io,uint8_t sda_io,uint8_t id_addr);
int register_iic_queue(struct i2c_device *i2c,uint8_t scl_io,uint8_t sda_io,uint8_t id_addr);
void jpg_cfg(uint8 jpgid,enum JPG_SRC_FROM src_from);


#define RESET_HIGH()		{gpio_set_val(rsn,1);}
#define RESET_LOW()		    {gpio_set_val(rsn,0);}

#define SPI_WIDTH           640   //240
#define SPI_HIGH            480   //320

k_task_handle_t dvp_manual_handle;
extern volatile uint32 outbuff_isr;
volatile uint8 prc_buf_num = 0;

struct	spi_device*   spi0_dev_g; 
struct	prc_device*   prc_dev_g; 

//uint8  spi_dat_buf[2][480*16];    //spi0 & spi1的buf缓存
uint8  *spi_dat_buf[2];

uint8  prc_yuv_line[SPI_WIDTH*16+SPI_WIDTH*16/2]__attribute__ ((aligned(4)));
__psram_data uint8 psram_spi_sensor[3][SPI_WIDTH*SPI_HIGH] __aligned(16);

uint32 read_len  = 0;
uint32 read_app_len  = 0;
uint32 last_len_num  = 0;
extern struct i2c_device *iic_test;
uint32 kick_en = 1;
uint8 spi_to_jpg_open = 0;
extern Vpp_stream photo_msg;
volatile uint8 prc_to_mjpg = 0;

_Sensor_Adpt_ * sensorAutoCheck(struct i2c_device *p_iic,uint8 *init_buf);
extern _Sensor_Ident_ *devSensorInit;
const _Sensor_Ident_ spi_null_init={0x00,0x00,0x00,0x00,0x00,0x00};



void prc_deal_irq(uint32 irq, uint32 irq_data);





static const _Sensor_Ident_ *devSensorInitTable_spi[] = {

#if DEV_SENSOR_BF30A2
	&bf30a2_init,
#endif

#if DEV_SENSOR_BF3A01
	&bf3a01_init,
#endif

#if DEV_SENSOR_BF20A6
	&bf20a6_spi_init,
#endif

	NULL,
};

static const _Sensor_Adpt_ *devSensorOPTable_spi[] = {

#if DEV_SENSOR_BF30A2
	&bf30a2_cmd,
#endif

#if DEV_SENSOR_BF3A01
	&bf3a01_cmd,
#endif

#if DEV_SENSOR_BF20A6
	&bf20a6_spi_cmd,
#endif

};

void prc_module_init(){
	prc_dev_g = (struct prc_device*)dev_get(HG_PRC_DEVID);
	prc_init(prc_dev_g);
	prc_set_width(prc_dev_g,SPI_WIDTH);
	prc_set_yuv_mode(prc_dev_g,0);
	
	
	prc_set_yaddr(prc_dev_g,(uint32)prc_yuv_line);
	prc_set_uaddr(prc_dev_g,(uint32)prc_yuv_line+SPI_WIDTH*16);
	prc_set_vaddr(prc_dev_g,(uint32)prc_yuv_line+SPI_WIDTH*16+SPI_WIDTH*16/4);
	
	prc_request_irq(prc_dev_g,PRC_ISR,(prc_irq_hdl)prc_deal_irq,(uint32)prc_dev_g);
}

uint8 spi_sensor_to_mjpeg_is_run(){
	return prc_to_mjpg;
}

uint8 spi_to_jpg_cfg(uint8 enable){
	if(enable){
		prc_to_mjpg = 1;
		jpg_cfg(HG_JPG0_DEVID,PRC_DATA);
	}else{
		prc_to_mjpg = 0;
	}
	return prc_to_mjpg;
}



void spi_sensor_cfg(){
	spi0_dev_g = (struct spi_device*)dev_get(HG_SPI0_DEVID);
//	spi_open(spi0_dev_g, 10000000, SPI_SLAVE_MODE, SPI_WIRE_SINGLE_MODE, SPI_CPOL_1_CPHA_1);
	spi_open(spi0_dev_g, 10000000, SPI_SLAVE_MODE, SPI_WIRE_QUAD_MODE, SPI_CPOL_0_CPHA_0);
	spi_ioctl(spi0_dev_g,SPI_CFG_SET_NONE_CS,1,0);
	spi_ioctl(spi0_dev_g,SPI_SENSOR_VSYNC_TIMEOUT ,0x7530,0);
	spi_ioctl(spi0_dev_g,SPI_SENSOR_VSYNC_HEAD_CNT,0,0);
	spi_ioctl(spi0_dev_g,SPI_SENSOR_VSYNC_EN,1,0);
	spi_ioctl(spi0_dev_g,SPI_SENSOR_HSYNC_TIMEOUT,2400,0);
	spi_ioctl(spi0_dev_g,SPI_SENSOR_HSYNC_HEAD_CNT,8,0);
	spi_ioctl(spi0_dev_g,SPI_HIGH_SPEED_CFG,0,0);
	spi_ioctl(spi0_dev_g,SPI_SENSOR_PINGPANG_EN,1,0);
	spi_ioctl(spi0_dev_g,SPI_SENSOR_BUF_LEN,SPI_WIDTH*2*16,0);
	spi_ioctl(spi0_dev_g,SPI_SENSOR_LINE_LEN_CFG,SPI_WIDTH*2,0);
	if(spi_dat_buf[0] == NULL){
		spi_dat_buf[0] = (uint8  *)os_malloc(SPI_WIDTH*2*16);
	}

	if(spi_dat_buf[1] == NULL){
		spi_dat_buf[1] = (uint8  *)os_malloc(SPI_WIDTH*2*16);
	}
	if((spi_dat_buf[0] == NULL)||(spi_dat_buf[1] == NULL)){
		_os_printf("spi sensor malloc error!!\r\n");
	}
	spi_ioctl(spi0_dev_g,SPI_SENSOR_SET_ADR0,(uint32)spi_dat_buf[0],0);
	spi_ioctl(spi0_dev_g,SPI_SENSOR_SET_ADR1,(uint32)spi_dat_buf[1],0);
	spi_ioctl(spi0_dev_g,SPI_RX_TIMEOUT_CFG,1,120000);  //500us
	spi_request_irq(spi0_dev_g,SPI_IRQ_FLAG_RX_DONE|SPI_IRQ_FLAG_RX_TIMEOUT,spi0_read_irq,(uint32)spi0_dev_g);		
	spi_ioctl(spi0_dev_g,SPI_KICK_DMA_EN,0,0);
}

volatile uint32 hnum = 0;
volatile uint32 sync_start = 0;
volatile uint32 prc_new_frame;
volatile uint8 prc_deal_num = 0;

uint8 spi_video_run = 0;
//只有主spi才能保证数据是准确的
void spi0_read_irq(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2){
	if(irq == SPI_IRQ_FLAG_RX_DONE){

		prc_new_frame = 0;
		if(sync_start){
			if((hnum%2) == 0){
				prc_set_src_addr(prc_dev_g,(uint32)spi_dat_buf[0]);
			}else{
				prc_set_src_addr(prc_dev_g,(uint32)spi_dat_buf[1]);
			}			
			prc_run(prc_dev_g);
		}
		hnum++;
	}
	
	if(irq == SPI_IRQ_FLAG_RX_TIMEOUT){
		prc_new_frame = 1;
		prc_deal_num = 0;
		sync_start = 1;
		spi_close(spi0_dev_g);
		spi_sensor_cfg();
		prc_first_start(prc_dev_g);
		hnum = 0;
	}	
}

void ybuf_deal(uint32 *sbuf,uint32 *ybuf,uint32 sz){
	uint32 itk;
	for(itk = 0;itk < sz;itk++){
		sbuf[itk] = ybuf[itk];
	}
}

void prc_deal_irq(uint32 irq, uint32 irq_data){
	
	
	static uint8 *prc_buf;
	if(prc_deal_num == 0){
		prc_buf = psram_spi_sensor[prc_buf_num];
		prc_buf_num++;
		if(prc_buf_num == 3)
			prc_buf_num = 0;
	}
	hw_memcpy(prc_buf+prc_deal_num*(16*SPI_WIDTH),prc_yuv_line,16*SPI_WIDTH);
	prc_deal_num++;
	if(prc_deal_num == (SPI_HIGH/16)){
		prc_deal_num = 0;
	}
	
}

uint32 count_timer_get = 0;
uint32 frame_end = 0;
uint32 frame_deal = 0;



void spi_sensor_reset(void){
	uint8_t pdn;
	uint8_t rsn;	
	pdn = MACRO_PIN(PIN_SPI_PDN);
	rsn = MACRO_PIN(PIN_SPI_RESET);
	if(pdn != 255){
		gpio_iomap_output(pdn,GPIO_IOMAP_OUTPUT);
		gpio_set_val(pdn,1);    //pdn up
		os_sleep_ms(100);
		gpio_iomap_output(pdn,GPIO_IOMAP_OUTPUT);
		gpio_set_val(pdn,0);						//pdn down
		os_sleep_ms(50);
	}

	if(rsn != 255){
		gpio_iomap_output(rsn,GPIO_IOMAP_OUTPUT);
	}
	else{
		os_sleep_ms(200);
		return;
	}
	
	RESET_HIGH();
	os_sleep_ms(50);
	RESET_LOW();
	os_sleep_ms(200);
	RESET_HIGH();
	os_sleep_ms(50);	
}

static _Sensor_Adpt_ * sensorAutoCheck_spi(uint8_t devid,uint8 *init_buf)
{
	uint8 i;
	_Sensor_Adpt_ * devSensor_Struct=NULL;
	for(i=0;devSensorInitTable_spi[i] != NULL;i++)
	{		
		spi_sensor_reset();
		if(sensorCheckId(devid,devSensorInitTable_spi[i],devSensorOPTable_spi[i])>=0)
		{
			os_printf("id =%x num:%d \n",devSensorInitTable_spi[i]->id,i);
			devSensorInit = (_Sensor_Ident_ *) devSensorInitTable_spi[i];
//#if CMOS_AUTO_LOAD
//			devSensor_Struct = sensor_adpt_load(devSensorInit->id,devSensorInit->w_cmd,devSensorInit->r_cmd,init_buf);
//#else
			devSensor_Struct = (_Sensor_Adpt_ *) devSensorOPTable_spi[i];
//#endif
			break;
		}
	}
	if(devSensor_Struct == NULL)
	{
		os_printf("Er: unkown!");
		devSensorInit = (_Sensor_Ident_ *)&spi_null_init;
		return NULL; // index 0 is test only
	}
	return devSensor_Struct;
}


void spi_sensor_thread(){
	//struct hgpwm_v0 *global_hgpwm;
	int data_cnt_old = 0;
	//uint8 sensor_staus = 0;    //0:stop  1:run

	
	prc_module_init();
	spi_sensor_cfg();

	while(1){	
	
		if(prc_buf_num == data_cnt_old){
			os_sleep_ms(10);
			continue;
		}

		data_cnt_old = prc_buf_num;
	}
}

void spi_2_jpg();
void i2c_SetSetting(struct i2c_setting *p_i2c_setting);
volatile uint8 spi_sensor_devid;
void spi_senosr_open(){
	//uint8 adr_num,dat_num;
	
	//uint8 sen_w_cmd,sen_r_cmd;
	//uint8 id_c = 0;	
	int8 idbuf[3];
	uint8_t tablebuf[16];
	uint32 i=0;
	uint8_t itk=0;
	uint32 k = 0;
	static _Sensor_Adpt_ *p_sensor_cmd = NULL;
	struct i2c_setting i2c_setting;	
	struct i2c_device *iic_dev;
	struct dvp_device *dvp_dev;


	
	iic_dev = (struct i2c_device *)dev_get(HG_I2C1_DEVID);
	dvp_dev = (struct dvp_device *)dev_get(HG_DVP_DEVID);
	gpio_iomap_output(PB_9, GPIO_IOMAP_OUT_DVP_MCLK_OUT);
	gpio_driver_strength(PB_9,	GPIO_DS_G1);
	dvp_set_baudrate(dvp_dev,6000000); 
	os_sleep_ms(3);
	os_printf("set SPI sensor finish ,Auto Check sensor id\r\n");
	spi_sensor_devid = register_iic_queue(iic_dev,PC_7,PC_6,0);	
	p_sensor_cmd = snser.p_sensor_cmd = sensorAutoCheck_spi(spi_sensor_devid,NULL);

	if(p_sensor_cmd == NULL){
		return;
	}
	os_printf("Auto Check sensor id finish\r\n");
	i2c_setting.u8DevWriteAddr = p_sensor_cmd->w_cmd;
	i2c_setting.u8DevReadAddr = p_sensor_cmd->r_cmd;
	i2c_SetSetting(&i2c_setting);
	dvp_set_baudrate(dvp_dev,24000000); 
	
    if(p_sensor_cmd->init!=NULL)
	{
			
			os_printf("SENSER....init\r\n");
			for(i=0;;i+=u8Addrbytnum+u8Databytnum)
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
					for(itk = 0;itk < u8Addrbytnum+u8Databytnum;itk++){
						idbuf[itk] = p_sensor_cmd->init[i+itk];
					}
					
					tablebuf[0] = u8Addrbytnum;
					tablebuf[1] = u8Databytnum;
					tablebuf[2] = u8SensorwriteID>>1;				
					for(k = 0;k < (tablebuf[0] + tablebuf[1]);k++){
						tablebuf[3+k] = idbuf[k];
					}

					wake_up_iic_queue(spi_sensor_devid,tablebuf,0,1,(uint8_t*)NULL);
					while(iic_devid_finish(spi_sensor_devid) != 1){
						os_sleep_ms(1);
					}

					//i2c_write(iic_test, (int8*)&p_sensor_cmd->init[i], u8Addrbytnum, (int8*)&p_sensor_cmd->init[i+u8Addrbytnum], u8Databytnum);
					if(i==0)
					{
						os_sleep_ms(1);
					}
				}
			}					
			
		csi_kernel_task_new((k_task_entry_t)spi_sensor_thread, "dvp_manual", 0, 40, 0, NULL, 1024, &dvp_manual_handle);
	}

}


#endif

