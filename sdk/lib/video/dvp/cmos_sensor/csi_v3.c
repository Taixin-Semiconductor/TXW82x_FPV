#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "lib/video/dvp/cmos_sensor/csi_V2.h"
#include "devid.h"
#include "hal/gpio.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "dev/vpp/hgvpp.h"
#include "dev/csi/hgdvp.h"
#include "lib/lcd/lcd.h"
#include "hal/jpeg.h"
#include "lib/video/isp/isp_dev.h"
#include "app_iic/app_iic.h"
#include "lib/video/vpp/vpp_dev.h"
#include "lib/heap/av_heap.h"
#ifdef PIN_FROM_PARAM
#include "pin_param.h"
#endif

struct dvp_device *dvp_test;
struct i2c_device *iic_test;
struct vpp_device *vpp_test;

volatile uint8 dvp_sensor_iic = 0;


volatile uint8 yuv_buf_head[16]__attribute__ ((section(".sbss")));;

__psram_data uint8 psram_ybuf_src[IMAGE_H*IMAGE_W+IMAGE_H*IMAGE_W/2] __aligned(4);



__psram_data volatile uint8 psram_buf_head[16];
#if ONLY_Y
__psram_data uint8 psram_buf[IMAGE_H][IMAGE_W] __aligned(4);
#endif
__psram_data uint8 psram_buf_tail[2032];


void dvp_line_isr(uint32 irq,uint32 image_h,uint32 param)
{
	
}

void dvp_fhfie_isr(uint32 irq,uint32 dev,uint32 param)
{
	os_printf("fh\r\n");
}

void dvp_fovie_isr(uint32 irq,uint32 dev,uint32 param)
{
	//dvp_vpp_reset();
	os_printf("------------------------------------------------------------------------------dvp fv\r\n");
}

void dvp_sip_isr(uint32 irq,uint32 dev,uint32 param)
{
	//dvp_vpp_reset();
	os_printf("sip reset DVP\r\n");
}

void dvp_line_isr_test(uint32 irq,uint32 image_h,uint32 param)
{
	return;
}

	
bool dvp_sensor_hardware_config(uint8_t camera_mode, uint8_t slave_en, uint8_t sensor_type,uint8_t fps)
{

    uint8_t mode_index = 0;
	const SensorWorkMode *sensor_mode = NULL;

    	uint8_t sensor_src		  = 0;
	struct i2c_device *iic_dev;
	static _Sensor_Adpt_ *p_sensor_cmd = NULL;

	dvp_test = (struct dvp_device *)dev_get(HG_DVP_DEVID);
	iic_dev = (struct i2c_device *)dev_get(HG_I2C1_DEVID);	

	dvp_init(dvp_test);
    dvp_close(dvp_test);

	dvp_sensor_iic  = register_iic_queue(iic_dev,MACRO_PIN(PIN_DVP0_IIC_CLK),MACRO_PIN(PIN_DVP0_IIC_SDA),0);
	gpio_iomap_output(MACRO_PIN(PIN_DVP_MCLK), GPIO_IOMAP_OUT_DVP_MCLK_OUT);
	os_printf("iic init finish,sensor reset & set sensor clk into 6M\r\n");
	dvp_set_baudrate(dvp_test,6000000); 
	os_sleep_ms(3);

    p_sensor_cmd = sensorAutoCheck(HG_MIPI_CSI_DEVID,dvp_sensor_iic);
    if(p_sensor_cmd == NULL){
        unregister_iic_queue(dvp_sensor_iic);
        return FALSE;
    }

    mode_index = sensor_mode_find_index(p_sensor_cmd,camera_mode,0);
    sensor_mode = (SensorWorkMode*)&p_sensor_cmd->supported_modes[mode_index];

    sensor_src = (camera_mode == CAM_DUAL_MASTER_SLAVE_MODE) ? ISP_INPUT_DAT_SRC_ORG_DMA : ISP_INPUT_DAT_SRC_DVP;
    sensor_info_add(sensor_type, sensor_src, (uint32)p_sensor_cmd, dvp_sensor_iic, mode_index);

	os_printf("Auto Check sensor id finish\r\n");

	os_printf("mclk:%dMHz\r\n",24000000);	
	dvp_set_baudrate(dvp_test,24000000);

    os_printf("sensor_mode width:%d height:%d\n", sensor_mode->width,sensor_mode->height);

	dvp_set_vsync_rst_offline(dvp_test,0,30*1000/32);
	dvp_set_size(dvp_test,0,0,sensor_mode->width,sensor_mode->height);
	dvp_set_hsync_polarity(dvp_test,sensor_mode->dvp.hsync_pol);
	dvp_set_hsync_polarity(dvp_test,sensor_mode->dvp.vsync_pol);
	dvp_set_format(dvp_test,sensor_mode->input_format);
	dvp_8BITS_map(dvp_test,0);

	dvp_request_irq(dvp_test,HSIP_ISR, (dvp_irq_hdl )&dvp_sip_isr,0);
	dvp_request_irq(dvp_test,FOVIE_ISR,(dvp_irq_hdl )&dvp_fovie_isr,0);

    dvp_open(dvp_test);

    sensor_write_reg_table(p_sensor_cmd,mode_index,dvp_sensor_iic);
    sensor_fps_to_vts(p_sensor_cmd,mode_index,dvp_sensor_iic,fps);
    if(camera_mode == CAM_DUAL_SPLICE_SLAVE_MODE){
        set_sync_edge(p_sensor_cmd,mode_index,dvp_sensor_iic,SYNC_EDGE_FALL);
    }
	
	video_msg.dvp_iw = sensor_mode->width;
	video_msg.dvp_ih = sensor_mode->height;
	video_msg.dvp_type = 1;
	video_msg.video_type_cur  = ISP_VIDEO_0;
	video_msg.video_type_last = ISP_VIDEO_0;
	video_msg.video_num++;

	return TRUE;
}



void csi_decfg()
{
	dvp_close(dvp_test);
	if (dvp_sensor_iic != 0)
	{
		unregister_iic_queue(dvp_sensor_iic);
		dvp_sensor_iic = 0;
	}
}


void get_single_dvp(uint16_t *w,uint16_t *h)
{
	if(w)
	{
		*w = video_msg.dvp_iw;
	}
	if(h)
	{
		*h = video_msg.dvp_ih;
	}
	return;
}





