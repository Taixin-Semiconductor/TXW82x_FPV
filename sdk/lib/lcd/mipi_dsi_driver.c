#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "dev.h"
#include "devid.h"
#include "hal/gpio.h"
#include "hal/lcdc.h"
#include "hal/spi.h"
#include "hal/dsi.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "dev/vpp/hgvpp.h"
#include "dev/scale/hgscale.h"
#include "dev/jpg/hgjpg.h"
#include "dev/lcdc/hglcdc.h"
#include "osal/semaphore.h"
#include "lib/lcd/lcd.h"
#include "osal/mutex.h"
#include "osal/sleep.h"
#ifdef PIN_FROM_PARAM
#include "pin_param.h"
#endif


uint8_t *dpipixel_fifo_mipi = NULL;
uint8_t *generic_fifo_mipi  = NULL;


void mipi_set_maxrpack(struct dsi_device *dsi_dev,uint8_t size){
	mipi_dsi_cmd_cfg(dsi_dev,DSI_SP_MAXRPACK,size,0,0);
	mipi_dsi_wait_pkt_fifo_full(dsi_dev);
}


void mipi_read_id(struct dsi_device *dsi_dev,uint8_t vcid,uint8_t cmd,uint32_t *buf,uint8_t len){
	uint8_t i = 0;
	mipi_dsi_cmd_cfg(dsi_dev,DSI_SP_DCSR0P,cmd,vcid,0);
	for(i =0;i < len;i = i+4){
		mipi_dsi_gen_pld_read_empty(dsi_dev);
		buf[i/4] = mipi_dsi_get_gen_pld_data(dsi_dev);
	}
}


void mipi_set_genw0p(struct dsi_device   * dsi_dev,uint8_t vc_id){
	mipi_dsi_cmd_cfg(dsi_dev,DSI_SP_GENW0P,0,vc_id,0);
}

void mipi_set_genw1p(struct dsi_device   * dsi_dev,uint8_t vc_id,uint8_t cmd){
	mipi_dsi_cmd_cfg(dsi_dev,DSI_SP_GENW1P,cmd,vc_id,0);
}

void mipi_set_dcsw1p(struct dsi_device   * dsi_dev,uint8_t vc_id,uint8_t cmd){
	mipi_dsi_cmd_cfg(dsi_dev,DSI_SP_DCSW0P,cmd,vc_id,0);
}


void mipi_set_lp_genw(struct dsi_device   * dsi_dev,uint8_t vc_id,uint8_t *buf,uint32_t len){
	uint32_t i;
	for(i=0;i<len;i=i+4)
	{
		mipi_dsi_gen_pld_write_full(dsi_dev);
		mipi_dsi_set_gen_pld_data(dsi_dev,buf[i+0],buf[i+1],buf[i+2],buf[i+3]);
	}
	mipi_dsi_gen_pld_write_full(dsi_dev);
	mipi_dsi_cmd_cfg(dsi_dev,DSI_LP_GENW,len,vc_id,0);
}

void mipi_set_lp_dcsw(struct dsi_device   * dsi_dev,uint8_t vc_id,uint8_t *buf,uint32_t len){
	uint32_t i;
	for(i=0;i<len;i=i+4)
	{
		mipi_dsi_gen_pld_write_full(dsi_dev);
		mipi_dsi_set_gen_pld_data(dsi_dev,buf[i+0],buf[i+1],buf[i+2],buf[i+3]);
	}

	mipi_dsi_gen_pld_write_full(dsi_dev);
	mipi_dsi_cmd_cfg(dsi_dev,DSI_LP_DCSW,len,vc_id,0);	
}

void mipi_set_genw2p(struct dsi_device   * dsi_dev,uint8_t vc_id,uint8_t cmd,uint8_t dat){
	mipi_dsi_cmd_cfg(dsi_dev,DSI_SP_GENW2P,cmd,vc_id,dat);
}

void mipi_set_dcsw2p(struct dsi_device   * dsi_dev,uint8_t vc_id,uint8_t cmd,uint8_t dat){
	mipi_dsi_cmd_cfg(dsi_dev,DSI_SP_DCSW1P,cmd,vc_id,dat);
}

void mipi_set_cmd(struct dsi_device   * dsi_dev,uint8_t vc_id,uint8_t cmd){
	mipi_dsi_cmd_cfg(dsi_dev,DSI_SP_DCSR0P,cmd,vc_id,0);
}

void mipi_get_data(struct dsi_device * dsi_dev,uint32_t *buf,uint8_t len){
	uint8_t i = 0;
	for(i = 0; i < len;i=i+4){
		mipi_dsi_gen_pld_read_empty(dsi_dev);
		buf[i/4] = mipi_dsi_get_gen_pld_data(dsi_dev);
	}
}

extern lcddev_t  lcdstruct;

void lcd_mipi_table_init(struct dsi_device * dsi_dev,uint8_t vc_id,uint8_t *lcd_table){
	uint32 itk = 0;
	uint8  last_opt = -1;
	uint8 buf[120];
	uint8 code;
	uint8 dly_ms = 0;
	uint8 param_num = 0;


	while(((lcd_table[itk] == LCD_TAB_END) && (lcd_table[itk+1] == LCD_TAB_END)) == 0){
		switch(lcd_table[itk]){
			case LCD_CMD:
				if(last_opt == LCD_CMD){
					_os_printf("cmd:%02x %02x  param_num:%d  \r\n",buf[0],buf[1],param_num);
					if(param_num == 1){
						//mipi_set_genw1p(dsi_dev,vc_id,buf[0]);
						mipi_set_dcsw1p(dsi_dev,vc_id,buf[0]);
						mipi_dsi_wait_pkt_fifo_full(dsi_dev);
					}else if(param_num == 2){
						//mipi_set_genw2p(dsi_dev,vc_id,buf[0],buf[1]);
						mipi_set_dcsw2p(dsi_dev,vc_id,buf[0],buf[1]);
						mipi_dsi_wait_pkt_fifo_full(dsi_dev);
					}else{
						//mipi_set_lp_genw(dsi_dev,vc_id,buf,param_num);
						mipi_set_lp_dcsw(dsi_dev,vc_id,buf,param_num);
						mipi_dsi_wait_pkt_fifo_full(dsi_dev);
					}
					//lcd_register_write_3line(code,buf,param_num);
				}else if(last_opt == DELAY_MS){
					//_os_printf("delay:%d\r\n",dly_ms);
					os_sleep_ms(dly_ms);
				}
				last_opt = LCD_CMD;
				param_num = 0;
				code = lcd_table[itk+1];
				buf[param_num] = code;
				param_num++;
			break;
			case LCD_DAT:
				buf[param_num] = lcd_table[itk+1];
				param_num++;
			break;
			case DELAY_MS:
				if(last_opt == LCD_CMD){
					_os_printf("cmd:%02x %02x  param_num:%d  \r\n",buf[0],buf[1],param_num);
					if(param_num == 1){
						//mipi_set_genw1p(dsi_dev,vc_id,buf[0]);
						mipi_set_dcsw1p(dsi_dev,vc_id,buf[0]);
						mipi_dsi_wait_pkt_fifo_full(dsi_dev);
					}else if(param_num == 2){
						//mipi_set_genw2p(dsi_dev,vc_id,buf[0],buf[1]);
						mipi_set_dcsw2p(dsi_dev,vc_id,buf[0],buf[1]);						
						mipi_dsi_wait_pkt_fifo_full(dsi_dev);
					}else{
						//mipi_set_lp_genw(dsi_dev,vc_id,buf,param_num);
						mipi_set_lp_dcsw(dsi_dev,vc_id,buf,param_num);
						mipi_dsi_wait_pkt_fifo_full(dsi_dev);
					}					
					//lcd_register_write_3line(code,buf,param_num);
				}else if(last_opt == DELAY_MS){
					//_os_printf("delay:%d\r\n",dly_ms);
					os_sleep_ms(dly_ms);
				}
				last_opt = DELAY_MS;
				dly_ms = lcd_table[itk+1];
			break;
			default:
			break;
		}
		itk +=2; 
	}

	if(last_opt == LCD_CMD){
		_os_printf("cmd:%02x %02x  param_num:%d \r\n",buf[0],buf[1],param_num);
		if(param_num == 1){
			//mipi_set_genw1p(dsi_dev,vc_id,buf[0]);
			mipi_set_dcsw1p(dsi_dev,vc_id,buf[0]);
			mipi_dsi_wait_pkt_fifo_full(dsi_dev);
		}else if(param_num == 2){
			//mipi_set_genw2p(dsi_dev,vc_id,buf[0],buf[1]);
			mipi_set_dcsw2p(dsi_dev,vc_id,buf[0],buf[1]);
			mipi_dsi_wait_pkt_fifo_full(dsi_dev);
		}else{
			//mipi_set_lp_genw(dsi_dev,vc_id,buf,param_num);
			mipi_set_lp_dcsw(dsi_dev,vc_id,buf,param_num);
			mipi_dsi_wait_pkt_fifo_full(dsi_dev);
		}
	}else if(last_opt == DELAY_MS){
		//_os_printf("delay:%d\r\n",dly_ms);	
		os_sleep_ms(dly_ms);
	}	
}

//2,0,1,4,3,1,0,0,0,0
void mipi_dsi_io_remap(uint8_t *cfgbuf){
	uint8_t itk = 0;
	uint8_t cfg_num = 0;
	if((MACRO_PIN(PIN_MIPI_DSI_CLKN) == PD_2) || (MACRO_PIN(PIN_MIPI_DSI_CLKN) == PD_3)){    //lane 0
		cfgbuf[0] = 0;
		if(MACRO_PIN(PIN_MIPI_DSI_CLKN) == PD_3){
			cfgbuf[5] = 1;			
		}else{
			cfgbuf[5] = 0;
		}
	}else if((MACRO_PIN(PIN_MIPI_DSI_CLKN) == PD_4) || (MACRO_PIN(PIN_MIPI_DSI_CLKN) == PD_5)){
		cfgbuf[0] = 1;
		if(MACRO_PIN(PIN_MIPI_DSI_CLKN) == PD_5){
			cfgbuf[5] = 1;			
		}else{
			cfgbuf[5] = 0;
		}
	}else if((MACRO_PIN(PIN_MIPI_DSI_CLKN) == PD_6) || (MACRO_PIN(PIN_MIPI_DSI_CLKN) == PD_7)){
		cfgbuf[0] = 2;
		if(MACRO_PIN(PIN_MIPI_DSI_CLKN) == PD_7){
			cfgbuf[5] = 1;			
		}else{
			cfgbuf[5] = 0;
		}
	}else if((MACRO_PIN(PIN_MIPI_DSI_CLKN) == PD_8) || (MACRO_PIN(PIN_MIPI_DSI_CLKN) == PD_9)){
		cfgbuf[0] = 3;
		if(MACRO_PIN(PIN_MIPI_DSI_CLKN) == PD_9){
			cfgbuf[5] = 1;			
		}else{
			cfgbuf[5] = 0;
		}
	}else if((MACRO_PIN(PIN_MIPI_DSI_CLKN) == PD_10) || (MACRO_PIN(PIN_MIPI_DSI_CLKN) == PD_11)){
		cfgbuf[0] = 4;
		if(MACRO_PIN(PIN_MIPI_DSI_CLKN) == PD_11){
			cfgbuf[5] = 1;			
		}else{
			cfgbuf[5] = 0;
		}
	}

	if((MACRO_PIN(PIN_MIPI_DSI_D0N) == PD_2) || (MACRO_PIN(PIN_MIPI_DSI_D0N) == PD_3)){    //lane 1
		cfgbuf[1] = 0;
		if(MACRO_PIN(PIN_MIPI_DSI_D0N) == PD_3){
			cfgbuf[6] = 1;			
		}else{
			cfgbuf[6] = 0;
		}
	}else if((MACRO_PIN(PIN_MIPI_DSI_D0N) == PD_4) || (MACRO_PIN(PIN_MIPI_DSI_D0N) == PD_5)){
		cfgbuf[1] = 1;
		if(MACRO_PIN(PIN_MIPI_DSI_D0N) == PD_5){
			cfgbuf[6] = 1;			
		}else{
			cfgbuf[6] = 0;
		}
	}else if((MACRO_PIN(PIN_MIPI_DSI_D0N) == PD_6) || (MACRO_PIN(PIN_MIPI_DSI_D0N) == PD_7)){
		cfgbuf[1] = 2;
		if(MACRO_PIN(PIN_MIPI_DSI_D0N) == PD_7){
			cfgbuf[6] = 1;			
		}else{
			cfgbuf[6] = 0;
		}
	}else if((MACRO_PIN(PIN_MIPI_DSI_D0N) == PD_8) || (MACRO_PIN(PIN_MIPI_DSI_D0N) == PD_9)){
		cfgbuf[1] = 3;
		if(MACRO_PIN(PIN_MIPI_DSI_D0N) == PD_9){
			cfgbuf[6] = 1;			
		}else{
			cfgbuf[6] = 0;
		}
	}else if((MACRO_PIN(PIN_MIPI_DSI_D0N) == PD_10) || (MACRO_PIN(PIN_MIPI_DSI_D0N) == PD_11)){
		cfgbuf[1] = 4;
		if(MACRO_PIN(PIN_MIPI_DSI_D0N) == PD_11){
			cfgbuf[6] = 1;			
		}else{
			cfgbuf[6] = 0;
		}
	}

	if((MACRO_PIN(PIN_MIPI_DSI_D1N) == PD_2) || (MACRO_PIN(PIN_MIPI_DSI_D1N) == PD_3)){    //lane 2
		cfgbuf[2] = 0;
		if(MACRO_PIN(PIN_MIPI_DSI_D1N) == PD_3){
			cfgbuf[7] = 1;			
		}else{
			cfgbuf[7] = 0;
		}
	}else if((MACRO_PIN(PIN_MIPI_DSI_D1N) == PD_4) || (MACRO_PIN(PIN_MIPI_DSI_D1N) == PD_5)){
		cfgbuf[2] = 1;
		if(MACRO_PIN(PIN_MIPI_DSI_D1N) == PD_5){
			cfgbuf[7] = 1;			
		}else{
			cfgbuf[7] = 0;
		}
	}else if((MACRO_PIN(PIN_MIPI_DSI_D1N) == PD_6) || (MACRO_PIN(PIN_MIPI_DSI_D1N) == PD_7)){
		cfgbuf[2] = 2;
		if(MACRO_PIN(PIN_MIPI_DSI_D1N) == PD_7){
			cfgbuf[7] = 1;			
		}else{
			cfgbuf[7] = 0;
		}
	}else if((MACRO_PIN(PIN_MIPI_DSI_D1N) == PD_8) || (MACRO_PIN(PIN_MIPI_DSI_D1N) == PD_9)){
		cfgbuf[2] = 3;
		if(MACRO_PIN(PIN_MIPI_DSI_D1N) == PD_9){
			cfgbuf[7] = 1;			
		}else{
			cfgbuf[7] = 0;
		}
	}else if((MACRO_PIN(PIN_MIPI_DSI_D1N) == PD_10) || (MACRO_PIN(PIN_MIPI_DSI_D1N) == PD_11)){
		cfgbuf[2] = 4;
		if(MACRO_PIN(PIN_MIPI_DSI_D1N) == PD_11){
			cfgbuf[7] = 1;			
		}else{
			cfgbuf[7] = 0;
		}
	}

	if((MACRO_PIN(PIN_MIPI_DSI_D2N) == PD_2) || (MACRO_PIN(PIN_MIPI_DSI_D2N) == PD_3)){    //lane 3
		cfgbuf[3] = 0;
		if(MACRO_PIN(PIN_MIPI_DSI_D2N) == PD_3){
			cfgbuf[8] = 1;			
		}else{
			cfgbuf[8] = 0;
		}
	}else if((MACRO_PIN(PIN_MIPI_DSI_D2N) == PD_4) || (MACRO_PIN(PIN_MIPI_DSI_D2N) == PD_5)){
		cfgbuf[3] = 1;
		if(MACRO_PIN(PIN_MIPI_DSI_D2N) == PD_5){
			cfgbuf[8] = 1;			
		}else{
			cfgbuf[8] = 0;
		}
	}else if((MACRO_PIN(PIN_MIPI_DSI_D2N) == PD_6) || (MACRO_PIN(PIN_MIPI_DSI_D2N) == PD_7)){
		cfgbuf[3] = 2;
		if(MACRO_PIN(PIN_MIPI_DSI_D2N) == PD_7){
			cfgbuf[8] = 1;			
		}else{
			cfgbuf[8] = 0;
		}
	}else if((MACRO_PIN(PIN_MIPI_DSI_D2N) == PD_8) || (MACRO_PIN(PIN_MIPI_DSI_D2N) == PD_9)){
		cfgbuf[3] = 3;
		if(MACRO_PIN(PIN_MIPI_DSI_D2N) == PD_9){
			cfgbuf[8] = 1;			
		}else{
			cfgbuf[8] = 0;
		}
	}else if((MACRO_PIN(PIN_MIPI_DSI_D2N) == PD_10) || (MACRO_PIN(PIN_MIPI_DSI_D2N) == PD_11)){
		cfgbuf[3] = 4;
		if(MACRO_PIN(PIN_MIPI_DSI_D2N) == PD_11){
			cfgbuf[8] = 1;			
		}else{
			cfgbuf[8] = 0;
		}
	}

	if((MACRO_PIN(PIN_MIPI_DSI_D3N) == PD_2) || (MACRO_PIN(PIN_MIPI_DSI_D3N) == PD_3)){    //lane 4
		cfgbuf[4] = 0;
		if(MACRO_PIN(PIN_MIPI_DSI_D3N) == PD_3){
			cfgbuf[9] = 1;			
		}else{
			cfgbuf[9] = 0;
		}
	}else if((MACRO_PIN(PIN_MIPI_DSI_D3N) == PD_4) || (MACRO_PIN(PIN_MIPI_DSI_D3N) == PD_5)){
		cfgbuf[4] = 1;
		if(MACRO_PIN(PIN_MIPI_DSI_D3N) == PD_5){
			cfgbuf[9] = 1;			
		}else{
			cfgbuf[9] = 0;
		}
	}else if((MACRO_PIN(PIN_MIPI_DSI_D3N) == PD_6) || (MACRO_PIN(PIN_MIPI_DSI_D3N) == PD_7)){
		cfgbuf[4] = 2;
		if(MACRO_PIN(PIN_MIPI_DSI_D3N) == PD_7){
			cfgbuf[9] = 1;			
		}else{
			cfgbuf[9] = 0;
		}
	}else if((MACRO_PIN(PIN_MIPI_DSI_D3N) == PD_8) || (MACRO_PIN(PIN_MIPI_DSI_D3N) == PD_9)){
		cfgbuf[4] = 3;
		if(MACRO_PIN(PIN_MIPI_DSI_D3N) == PD_9){
			cfgbuf[9] = 1;			
		}else{
			cfgbuf[9] = 0;
		}
	}else if((MACRO_PIN(PIN_MIPI_DSI_D3N) == PD_10) || (MACRO_PIN(PIN_MIPI_DSI_D3N) == PD_11)){
		cfgbuf[4] = 4;
		if(MACRO_PIN(PIN_MIPI_DSI_D3N) == PD_11){
			cfgbuf[9] = 1;			
		}else{
			cfgbuf[9] = 0;
		}
	}


	for(itk = 0;itk < 5;itk++){
		if(cfgbuf[5+itk] != 0xff){
			cfg_num++;
		}
	}

	for(itk = cfg_num;itk < 5;itk++){
		cfgbuf[itk] = itk;
		cfgbuf[itk+5] = 0;
	}

}



void mipi_dsi_init(uint32 w,uint32 h,uint32 dclk,uint8 vsa,uint8 vbp,uint8 vfp,uint8 hsa,uint8 hbp,uint8 hfp,uint8 lanenum,uint8 colortype){
	uint8_t itk = 0;
	uint8_t cfgbuf[10];
	int GENERIC_VC_ID = 0x0;
	int dpi2laneclkratio;
	struct dsi_device *dsi_dev;
	dsi_dev = (struct dsi_device *)dev_get(HG_DSI_DEVID); 
	dpi2laneclkratio = 60*1000/(dclk/1000000);
	//dpipixel_fifo_mipi = dpi_pixel_buf;//os_malloc(4*w);//
	generic_fifo_mipi  = os_malloc(w);//

	if(generic_fifo_mipi == NULL){
		_os_printf("generic_fifo_mipi room error....\r\n");
		return;
	}
	dsi_init(dsi_dev,1,4,1,lanenum);
	mipi_dsi_max_clk_time(dsi_dev,0x10);
	mipi_dsi_remain_stop_state(dsi_dev,2);
	mipi_dsi_set_lane_num(dsi_dev,lanenum);
	mipi_dsi_cfg_enable(dsi_dev,0,0,1,1,1,0);
	mipi_dsi_ulps_ctrl(dsi_dev,0,0,0,0);
	mipi_dsi_ulps_min_time(dsi_dev,0x12);
	mipi_dsi_ulps_entry_delay_time(dsi_dev,0x12);
	mipi_dsi_ulps_wakeup_time(dsi_dev,0x12,0x12);
	mipi_dsi_ulps_mode(dsi_dev,0,0,0);
	mipi_dsi_select_mode(dsi_dev,1);
	mipi_dsi_gen_vcid_cfg(dsi_dev,GENERIC_VC_ID,1,1);

	//mipi_dsi_cmd_mode_cfg(dsi_dev,1,1,0,0,0,0,0,0,0,0,0,0,0,0);
	mipi_dsi_cmd_mode_cfg(dsi_dev,1,0,1,1,1,1,1,1,1,1,1,1,1,1);
	//mipi_dsi_cmd_mode_cfg(dsi_dev,0,1,1,1,1,1,1,1,1,1,1,1,1,1);

	mipi_dsi_dpi_vcid(dsi_dev,0);
	if(colortype == LCD_MODE_565){
		mipi_dsi_color_cfg(dsi_dev,0,0);               //5:24BIT  0:16BIT
	}
	else if(colortype == LCD_MODE_888){
		mipi_dsi_color_cfg(dsi_dev,5,0);               //5:24BIT  0:16BIT
	}	

	mipi_dsi_cfg_pol(dsi_dev,0,0,0,0,0);
	//mipi_dsi_vid_mode(dsi_dev,1,1,1,1,1,1,0,0,3);  //3:Burst mode   1:sync events
	mipi_dsi_vid_mode(dsi_dev,1,1,1,1,1,1,0,0,3);
	mipi_dsi_vid_pkt_size(dsi_dev,w);
	mipi_dsi_vid_chunk_num(dsi_dev,0);
	mipi_dsi_vid_null_size(dsi_dev,0);
	mipi_dsi_vid_cfg(dsi_dev,dpi2laneclkratio,vsa,vbp,h,w,vfp,hsa,hbp,hfp);
	mipi_dsi_lpcmd_tmr(dsi_dev,0x10,0x10);
	mipi_dsi_edpi_cfg(dsi_dev,0X20,0,0,0,0);
	mipi_dsi_phy_rst(dsi_dev);	
	mipi_dsi_set_pixel_sram_fifo(dsi_dev,(uint32)generic_fifo_mipi);
	mipi_dsi_set_fifo_depth(dsi_dev,w);
	dsi_open(dsi_dev);

	memset(cfgbuf,0xff,10);
	mipi_dsi_io_remap(cfgbuf);
	for(itk = 0;itk < 10;itk++){
		printf("%d ",cfgbuf[itk]);
	}
	printf("\r\n");
	//mipi_dsi_set_lane_remap(dsi_dev,2,1,0,4,3,1,1,1,0,0); 
	//mipi_dsi_set_lane_remap(dsi_dev,2,0,1,4,3,1,0,0,0,0); 
	mipi_dsi_set_lane_remap(dsi_dev,cfgbuf[0],cfgbuf[1],cfgbuf[2],cfgbuf[3],cfgbuf[4],cfgbuf[5],cfgbuf[6],cfgbuf[7],cfgbuf[8],cfgbuf[9]);
		
	mipi_set_maxrpack(dsi_dev,0x10);   
	//mipi_read_id(dsi_dev,0x0,0x0a,idbuf,4);
	//_os_printf("id:%08x\r\n",idbuf[0]);
	
	lcd_mipi_table_init(dsi_dev,GENERIC_VC_ID,(uint8_t*)lcdstruct.init_table);
	mipi_dsi_wait_pkt_fifo_full(dsi_dev);
	mipi_dsi_cmd_pkt_fifo_empty(dsi_dev);

#if 0
	idbuf[0] = 0;
	idbuf[1] = 0;


	mipi_set_cmd(dsi_dev,GENERIC_VC_ID,0x09);
	mipi_get_data(dsi_dev,idbuf,1);
	_os_printf("id:%02x %02x   \r\n",idbuf[0],idbuf[1]);
	mipi_set_cmd(dsi_dev,GENERIC_VC_ID,0x0A);
	mipi_get_data(dsi_dev,idbuf,1);
	_os_printf("id:%02x %02x   \r\n",idbuf[0],idbuf[1]);
#endif
	//while(1)
	mipi_dsi_select_mode(dsi_dev,0);


}

