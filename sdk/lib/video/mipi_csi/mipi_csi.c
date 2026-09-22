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
#include "hal/capture.h"
#include "app_iic/app_iic.h"
#include "lib/video/isp/isp_dev.h"
#include "lib/video/vpp/vpp_dev.h"

#ifdef PIN_FROM_PARAM
#include "pin_param.h"
#endif
#include "syscfg.h"


uint32 sensor0_ident,sensor1_ident;

_Sensor_Ident_ *devSensorInit2=NULL;
const _Sensor_Ident_ null_init2={0x00,0x00,0x00,0x00,0x00,0x00};

void   *set_fps_task_hdl = NULL;
volatile struct os_msgqueue set_fps_msg;
volatile struct os_event    set_fps_event;
static uint32_t fsync_cnt       = 0;
static uint8_t  fsync_pending   = 0;

void dual_mipi_csi_reset(enum fps_mode mode, float fps);

void mipi_csi_fovie_isr(uint32 irq,uint32 dev,uint32 param){
	os_printf(KERN_ERR"------------------------------------------------------------------------------mipi fv\r\n");
}

//MIPI CSI 接收端收到的行数和配置预期行数不匹配
void mipi_csi_vsip_isr(uint32 irq,uint32 dev,uint32 param){
	os_printf(KERN_ERR"vsip reset mipi csi_dev %08x \r\n",param);
}

//MIPI CSI 接收端检测到单行内部数据异常
void mipi_csi_hsip_isr(uint32 irq,uint32 dev,uint32 param){
	os_printf(KERN_ERR"hsip reset mipi csi_dev %08x \r\n",param);
}

capture_irq_hdl fsync_capture_irq_hdl(uint32 irq, uint32 irq_data){
    if (irq == CAPTURE_IRQ_FLAG_CAPTURE) {
		os_event_set((void *)&set_fps_event, EVENT_FYSNC_CNT, NULL); 
    }
	return 0;
}

/**
 * @brief 设置双路MIPI传感器的帧率
 * 
 * @param mode  帧率模式，仅支持2种预设的帧率切换（正常模式、夜视降帧模式）
 * @param fps   目标帧率值，用于配置fsync信号，必须和预设配置表的帧率对应
 * 
 */
void dual_mipi_sensor_set_fps(enum fps_mode mode, float fps)
{
    if(video_msg.camera_mode != CAM_DUAL_SPLICE_SLAVE_MODE) {
        os_printf(KERN_ERR"%s This camera mode is not supported! \r\n", __func__);
        return;
    }
    int32 msg_ret   = 0;
    struct isp_sensor_opt   *sensor_opt = malloc(sizeof(struct isp_sensor_opt));
    if (!sensor_opt) return;   
    sensor_opt->fps_mode = mode;
    sensor_opt->fps_f    = fps;
    // os_printf("%s ===> %d %f \r\n",__func__,sensor_opt->fps_mode,sensor_opt->fps_f);
    msg_ret = os_msgq_put((void *)&set_fps_msg, (uint32)sensor_opt, 0);
    if(msg_ret != RET_OK) {
        free(sensor_opt);
    }
}

int32 set_fps_task(void)
{
    uint32 flag = 0;
    int32 event_ret = 0;
    int32 msg_ret   = 0;

    struct isp_sensor_opt   *opt = NULL;
    
    while (1)
    {
        event_ret = os_event_wait((void *)&set_fps_event, EVENT_ISP_DOEN_OPT | EVENT_FYSNC_CNT , &flag, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 50);
        if (event_ret){
            continue;
        }

        if(flag & EVENT_FYSNC_CNT){
            //gpio_set_val(PA_15, (cnt++)&0x01);
            if(fsync_pending){
                if(fsync_cnt++ > 3){
                    isp_cfg_dev();
                    fsync_pending = 0;
                }
            }
        }
		
		if (flag & EVENT_ISP_DOEN_OPT){
            opt = (struct isp_sensor_opt *)os_msgq_get2((void *)&set_fps_msg, 0, &msg_ret);
            if (msg_ret == 0){
                dual_mipi_csi_reset(opt->fps_mode,opt->fps_f);
                fsync_cnt = 0;
                fsync_pending = 1;
                free(opt);
            } 
		}
    }
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
			mipi_csi0_init   		: 1,
			mipi_csi1_init   		: 1,
			mipi_mclk_init          : 1,
			mipi_fsync_init			: 1,
			mipi_csi0_data_lane_num : 2;
	uint8_t mipi_csi0_iic_id;
	uint8_t mipi_csi1_iic_id;
};
static struct mipi_csi_priv g_mipi_csi_priv = {0};

void mipi_csi_debug_config(struct mipi_csi_debug *p_debug, uint32 csi_dev_id)
{
    if (!p_debug || !p_debug->debug_enable)     return;

    sysctrl_ace_peris_access_cpu_all(ACE_MIX_TOP|ACE_GPIO_TOP|ACE_EFUSE_CTRL|ACE_SYS_SEC_TOP|ACE_PMU|ACE_BASEBAND1|ACE_BASEBAND|ACE_RFDIGITAL|ACE_RFDIGCAL_TOP);

    SYSCTRL_REG_OPT(
        SYSCTRL->SYS_CON0 |= BIT(3);
        SYSCTRL->SYS_CON1 |= BIT(21);  //lmac reset
        SYSCTRL->CLK_CON2 |= BIT(22);  //lmac clk enable
    );
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
        // os_sleep_ms(1000);
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



void dual_mipi_csi_reset(enum fps_mode mode, float fps)
{
	uint32_t cfg = 0;
	uint32 period_sysclkpd_cnt = 0;
	struct mipi_csi_device *mipi_csi_dev 	= (struct mipi_csi_device *)dev_get(HG_MIPI_CSI_DEVID);
	struct mipi_csi_device *mipi1_csi_dev 	= (struct mipi_csi_device *)dev_get(HG_MIPI1_CSI_DEVID);
	struct hgpwm_v0 	   *sensor_fsync	= (struct hgpwm_v0 *)dev_get(HG_PWM0_DEVID);
	struct isp_device 	   *isp_dev 		= (struct isp_device  *)dev_get(HG_ISP_DEVID);

	struct isp_sensor_opt sensor0_opt 		= {0};
	struct isp_sensor_opt sensor1_opt 		= {0};

	_Sensor_Adpt_  			*sensor0_adapt = NULL;
	_Sensor_Adpt_  			*sensor1_adapt = NULL;

	isp_get_sensor_opt(isp_dev, &sensor0_opt, SENSOR_TYPE_MASTER);
	sensor0_adapt  = (_Sensor_Adpt_ *)sensor0_opt.sensor_cmd;  

	isp_get_sensor_opt(isp_dev, &sensor1_opt, SENSOR_TYPE_SLAVE0);
	sensor1_adapt  = (_Sensor_Adpt_ *)sensor1_opt.sensor_cmd;  

	if(sensor0_adapt == NULL || sensor1_adapt == NULL) return;

	sensor_info_add(SENSOR_TYPE_MASTER, ISP_INPUT_DAT_SRC_MIPI0, (uint32)sensor0_adapt, sensor0_opt.devid_id, (uint32)sensor0_ident);
	sensor_info_add(SENSOR_TYPE_SLAVE0, ISP_INPUT_DAT_SRC_MIPI1, (uint32)sensor1_adapt, sensor1_opt.devid_id, (uint32)sensor1_ident);

	// sensor_power_on(HG_MIPI_CSI_DEVID);
	// sensor_power_on(HG_MIPI1_CSI_DEVID);

	isp_dev_close();

	if(mode == FPS_MODE_NIGHT_LOW){
		period_sysclkpd_cnt = DEFAULT_SYS_CLK/fps;
		pwm_ioctl((struct pwm_device *)sensor_fsync, PWM_CHANNEL_0, PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, period_sysclkpd_cnt, period_sysclkpd_cnt*3/5);   
	}else if(mode == FPS_MODE_DAY_NORMAL){
		period_sysclkpd_cnt = DEFAULT_SYS_CLK/fps;
		pwm_ioctl((struct pwm_device *)sensor_fsync, PWM_CHANNEL_0, PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, period_sysclkpd_cnt, period_sysclkpd_cnt*3/5);   		
	}

	// sensor_write_reg_table(sensor0_adapt->sensor_stop_stream,sensor0_opt.devid_id);
	// sensor_write_reg_table(sensor1_adapt->sensor_stop_stream,sensor1_opt.devid_id);

	mipi_csi_close(mipi1_csi_dev);
	mipi_csi_init(mipi1_csi_dev,1); 

	mipi_csi_close(mipi_csi_dev);
	mipi_csi_init(mipi_csi_dev,1); 

	mipi_csi_set_lane_num(mipi_csi_dev, g_mipi_csi_priv.mipi_csi0_data_lane_num);
	mipi_csi_open_virtual_channel(mipi_csi_dev,0);

	cfg = io_mipi_csi0_remap_cfg(1);
	mipi_csi_dphy_cfg(mipi_csi_dev,cfg,0x17709de7,0x10,0x3def,0,0,0,0);

	mipi_csi_img_size(mipi_csi_dev,sensor0_adapt->pixelw, sensor0_adapt->pixelh);
	//mipi_csi_crop_enable(mipi_csi_dev,0);
	//mipi_csi_crop_img_start(mipi_csi_dev,0,0);
	//mipi_csi_crop_img_end(mipi_csi_dev,1920,1080);
	//mipi_csi_hsync_rec_time(mipi_csi_dev,5);
	mipi_csi_hsync_rec_enable(mipi_csi_dev,1);	
	mipi_csi_input_format(mipi_csi_dev,sensor0_adapt->sensor_isp_cfg.input_format); 
	mipi_csi_request_irq(mipi_csi_dev,CSI2_VSIP_ISR, (mipi_csi_irq_hdl )&mipi_csi_vsip_isr,0);
	mipi_csi_request_irq(mipi_csi_dev,CSI2_HSIP_ISR, (mipi_csi_irq_hdl )&mipi_csi_hsip_isr,0);
	mipi_csi_request_irq(mipi_csi_dev,CSI2_FOVIE_ISR,(mipi_csi_irq_hdl )&mipi_csi_fovie_isr,0);
	mipi_csi_open(mipi_csi_dev);
	
//	sensor_write_reg_table(sensor0_adapt->init,sensor0_opt.devid_id);
//	if(mode == FPS_MODE_NIGHT_LOW){
//		sensor_write_reg_table(sensor0_adapt->nigth_mode_init,sensor0_opt.devid_id);
//	}else if(mode == FPS_MODE_DAY_NORMAL){
//		sensor_write_reg_table(sensor0_adapt->slave_init,sensor0_opt.devid_id);
//	}
	mipi_csi_hs_rx_zero_cnt_set(mipi_csi_dev, sys_cfgs.mipi_csi0_hs_zero_cnt);

	mipi_csi_set_lane_num(mipi1_csi_dev,1);
	mipi_csi_open_virtual_channel(mipi1_csi_dev,0);
	cfg = io_mipi_csi1_remap_cfg();
	mipi_csi_dphy_cfg(mipi1_csi_dev,cfg,0x17709de7,0x20,0x3def,0,0,0,0);

	//os_printf("pixelw:%d pixelh:%d\n", sensor1_adapt->pixelw, sensor1_adapt->pixelh);
	mipi_csi_img_size(mipi1_csi_dev,sensor1_adapt->pixelw, sensor1_adapt->pixelh);
	mipi_csi_hsync_rec_enable(mipi1_csi_dev,1);	
	mipi_csi_input_format(mipi1_csi_dev,sensor1_adapt->sensor_isp_cfg.input_format); 
	mipi_csi_request_irq(mipi1_csi_dev,CSI2_VSIP_ISR, (mipi_csi_irq_hdl )&mipi_csi_vsip_isr,0);
	mipi_csi_request_irq(mipi1_csi_dev,CSI2_HSIP_ISR, (mipi_csi_irq_hdl )&mipi_csi_hsip_isr,0);
	mipi_csi_request_irq(mipi1_csi_dev,CSI2_FOVIE_ISR,(mipi_csi_irq_hdl )&mipi_csi_fovie_isr,0);
	mipi_csi_open(mipi1_csi_dev);

	
//	sensor_write_reg_table(sensor1_adapt->init,sensor1_opt.devid_id,cmd1_len);
//	if(mode == FPS_MODE_NIGHT_LOW){
//		sensor_write_reg_table(sensor1_adapt->nigth_mode_init,sensor1_opt.devid_id,cmd1_len);
//	}else if(mode == FPS_MODE_DAY_NORMAL){
//		sensor_write_reg_table(sensor1_adapt->slave_init,sensor1_opt.devid_id,cmd1_len);
//	}
	mipi_csi_hs_rx_zero_cnt_set(mipi1_csi_dev, sys_cfgs.mipi_csi1_hs_zero_cnt);

}


int mipi_csi_hardware_config(uint32_t csi_dev_id, uint8_t csi_lane_num, uint8_t camera_mode, uint8_t slave_en, uint8_t sensor_type,uint8_t fps, struct mipi_csi_debug *p_debug) 
{
    uint8_t mode_index = 0;
	uint32_t cfg = 0;
	uint8_t csi_data_lane_num = 0;
	uint8_t sensor_src		  = 0;

	_Sensor_Adpt_ 	*p_sensor_cmd  	= NULL;
	const SensorWorkMode *sensor_mode = NULL;

	struct pwm_device *global_hgpwm = (struct pwm_device *)dev_get(HG_PWM0_DEVID);
    struct capture_device *p_capture = (struct capture_device*)dev_get(HG_CAPTURE0_DEVID);
	struct i2c_device *iic_dev = (struct i2c_device *)dev_get(HG_I2C1_DEVID);
	struct mipi_csi_device *mipi_csi_dev = (struct mipi_csi_device *)dev_get(HG_MIPI_CSI_DEVID);
	struct mipi_csi_device *mipi1_csi_dev = (struct mipi_csi_device *)dev_get(HG_MIPI1_CSI_DEVID);

	if(camera_mode == CAM_DUAL_MASTER_SLAVE_MODE)
	{
		gpio_set_mode(MACRO_PIN(PIN_MIPI_FSYNC), GPIO_PULL_DOWN, GPIO_PULL_LEVEL_100K);
		gpio_iomap_output(MACRO_PIN(PIN_MIPI_FSYNC),GPIO_IOMAP_OUT_DUAL_ORG_FSYNC);
	}
	os_printf("mipi_csi_dev:%08x  mipi1_csi_dev:%08x\r\n",mipi_csi_dev,mipi1_csi_dev);


	if(!g_mipi_csi_priv.mipi_fsync_init && camera_mode == CAM_DUAL_SPLICE_SLAVE_MODE){
		uint32_t period_sysclkpd_cnt = DEFAULT_SYS_CLK/15;
		gpio_driver_strength(MACRO_PIN(PIN_PWM_CHANNEL_0), GPIO_DS_G1);
		pwm_init((struct pwm_device *)global_hgpwm, PWM_CHANNEL_0, period_sysclkpd_cnt-1, period_sysclkpd_cnt*3/5);
		pwm_start((struct pwm_device *)global_hgpwm, PWM_CHANNEL_0);
		
		gpio_ioctl(MACRO_PIN(PIN_CAPTURE_CHANNEL_2),GPIO_CMD_SET_IEEN,1,0);//同时支持输入和输出
    	capture_init(p_capture, CAPTURE_CHANNEL_2, CAPTURE_MODE_ALL);
    	capture_request_irq(p_capture, CAPTURE_CHANNEL_2, CAPTURE_IRQ_FLAG_CAPTURE, (capture_irq_hdl)fsync_capture_irq_hdl, 0);
    	gpio_ioctl(MACRO_PIN(PIN_PWM_CHANNEL_0),GPIO_CMD_SET_ONLY_MODE_BIT,0x01,0);//设为输出
    	capture_start(p_capture, CAPTURE_CHANNEL_2);
	
		os_event_init((void *)&set_fps_event);
		os_msgq_init((void *)&set_fps_msg, 1);
		set_fps_task_hdl = os_task_create("set_fps_task", (void *)set_fps_task, NULL, OS_TASK_PRIORITY_HIGH+1, 0, NULL, 1024);

		g_mipi_csi_priv.mipi_fsync_init = 1;
	}

	if(!g_mipi_csi_priv.mipi_mclk_init) {
		gpio_driver_strength(MACRO_PIN(PIN_PWM_CHANNEL_1), GPIO_DS_G1);
		uint32_t mclk_div = (DEFAULT_SYS_CLK/1000000)/SENSOR_MCLK;
		//设置分频比,mclk_div-1,然后设置占空比:50%
		pwm_init((struct pwm_device *)global_hgpwm, PWM_CHANNEL_1, mclk_div-1, (mclk_div+1)>>1);
		pwm_start((struct pwm_device *)global_hgpwm, PWM_CHANNEL_1);
		os_sleep_ms(1);
		g_mipi_csi_priv.mipi_mclk_init = 1;
	}

	if(csi_dev_id == HG_MIPI_CSI_DEVID && !g_mipi_csi_priv.mipi_csi0_en) {
		uint8_t mipi_csi0_iic = register_iic_queue(iic_dev,MACRO_PIN(PIN_MIPI0_IIC_CLK),MACRO_PIN(PIN_MIPI0_IIC_SDA),0);
		os_printf("set mipi0 sensor finish ,Auto Check sensor id\r\n");

		p_sensor_cmd = sensorAutoCheck(csi_dev_id,mipi_csi0_iic);
		if(p_sensor_cmd == NULL){
			unregister_iic_queue(mipi_csi0_iic);
			return FALSE;
		}

        mode_index = sensor_mode_find_index(p_sensor_cmd,camera_mode,csi_lane_num);
        sensor_mode = (SensorWorkMode*)&p_sensor_cmd->supported_modes[mode_index];

		csi_data_lane_num = sensor_mode->mipi.mipi_lane_num;
		os_printf("Auto Check sensor id finish,mipi_csi0 lane num:%d\r\n",csi_data_lane_num);
		mipi_csi_close(mipi_csi_dev);
		mipi_csi_init(mipi_csi_dev, csi_data_lane_num);
		if (csi_data_lane_num > 1){
			mipi_csi_close(mipi1_csi_dev);
			mipi_csi_init(mipi1_csi_dev,1); 
		}
        
        g_mipi_csi_priv.mipi_csi0_iic_id = mipi_csi0_iic;
		sensor_src = (camera_mode == CAM_DUAL_MASTER_SLAVE_MODE) ? ISP_INPUT_DAT_SRC_ORG_DMA : ISP_INPUT_DAT_SRC_MIPI0;
		sensor_info_add(sensor_type, sensor_src, (uint32)p_sensor_cmd, mipi_csi0_iic, mode_index);
        
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
		os_printf("sensor_mode width:%d height:%d\n", sensor_mode->width,sensor_mode->height);
		mipi_csi_img_size(mipi_csi_dev,sensor_mode->width,sensor_mode->height);
		//mipi_csi_crop_enable(mipi_csi_dev,0);
		//mipi_csi_crop_img_start(mipi_csi_dev,0,0);
		//mipi_csi_crop_img_end(mipi_csi_dev,1920,1080);
		//mipi_csi_hsync_rec_time(mipi_csi_dev,5);
		mipi_csi_hsync_rec_enable(mipi_csi_dev,1);	
		mipi_csi_input_format(mipi_csi_dev,p_sensor_cmd->sensor_isp_cfg.input_format); 
		mipi_csi_request_irq(mipi_csi_dev,CSI2_VSIP_ISR, (mipi_csi_irq_hdl )&mipi_csi_vsip_isr,0);
		mipi_csi_request_irq(mipi_csi_dev,CSI2_HSIP_ISR, (mipi_csi_irq_hdl )&mipi_csi_hsip_isr,0);
		mipi_csi_request_irq(mipi_csi_dev,CSI2_FOVIE_ISR,(mipi_csi_irq_hdl )&mipi_csi_fovie_isr,0);
		mipi_csi_open(mipi_csi_dev);

		sensor_write_reg_table(p_sensor_cmd,mode_index,mipi_csi0_iic);
		sensor_fps_to_vts(p_sensor_cmd,mode_index,mipi_csi0_iic,fps);
		if(camera_mode == CAM_DUAL_SPLICE_SLAVE_MODE){
			set_sync_edge(p_sensor_cmd,mode_index,mipi_csi0_iic,SYNC_EDGE_FALL);
		}
	}

	if(csi_dev_id == HG_MIPI1_CSI_DEVID && !g_mipi_csi_priv.mipi_csi1_en) {
		if(g_mipi_csi_priv.mipi_csi0_data_lane_num == 2) {
			os_printf("mipi csi1 unsupport\n");
			return FALSE;
		}
		csi_data_lane_num = 1;
        mipi_csi_close(mipi1_csi_dev);
        mipi_csi_init(mipi1_csi_dev, csi_data_lane_num); 
		uint8_t mipi_csi1_iic = register_iic_queue(iic_dev,MACRO_PIN(PIN_MIPI1_IIC_CLK),MACRO_PIN(PIN_MIPI1_IIC_SDA),0);	
		os_printf("set mipi1 sensor finish ,Auto Check sensor id\r\n");
		p_sensor_cmd = sensorAutoCheck(csi_dev_id,mipi_csi1_iic);
		if(p_sensor_cmd == NULL){
			unregister_iic_queue(mipi_csi1_iic);
			return FALSE;
		}

        mode_index = sensor_mode_find_index(p_sensor_cmd,camera_mode,csi_lane_num);
        sensor_mode = (SensorWorkMode*)&p_sensor_cmd->supported_modes[mode_index];
        
		os_printf("Auto Check sensor id finish,mipi_csi1 lane num:%d\r\n",p_sensor_cmd->mipi_lane_num);
        
        g_mipi_csi_priv.mipi_csi1_iic_id = mipi_csi1_iic;
		sensor_src = (camera_mode == CAM_DUAL_MASTER_SLAVE_MODE) ? ISP_INPUT_DAT_SRC_ORG_DMA : ISP_INPUT_DAT_SRC_MIPI1;
		sensor_info_add(sensor_type, sensor_src, (uint32)p_sensor_cmd, mipi_csi1_iic, mode_index);

		mipi_csi_set_lane_num(mipi1_csi_dev,1);
		mipi_csi_open_virtual_channel(mipi1_csi_dev,0);
		cfg = io_mipi_csi1_remap_cfg();
		mipi_csi_dphy_cfg(mipi1_csi_dev,cfg,0x17709de7,0x20,0x3def,0,0,0,0);

		os_printf("sensor_mode width:%d height:%d\n", sensor_mode->width,sensor_mode->height);
		mipi_csi_img_size(mipi1_csi_dev,sensor_mode->width,sensor_mode->height);
		mipi_csi_hsync_rec_enable(mipi1_csi_dev,1);	
		mipi_csi_input_format(mipi1_csi_dev,p_sensor_cmd->sensor_isp_cfg.input_format); 
		mipi_csi_request_irq(mipi1_csi_dev,CSI2_VSIP_ISR, (mipi_csi_irq_hdl )&mipi_csi_vsip_isr,0);
		mipi_csi_request_irq(mipi1_csi_dev,CSI2_HSIP_ISR, (mipi_csi_irq_hdl )&mipi_csi_hsip_isr,0);
		mipi_csi_request_irq(mipi1_csi_dev,CSI2_FOVIE_ISR,(mipi_csi_irq_hdl )&mipi_csi_fovie_isr,0);
		mipi_csi_open(mipi1_csi_dev);

		sensor_write_reg_table(p_sensor_cmd,mode_index,mipi_csi1_iic);
		sensor_fps_to_vts(p_sensor_cmd,mode_index,mipi_csi1_iic,fps);
		if(camera_mode == CAM_DUAL_SPLICE_SLAVE_MODE){
			set_sync_edge(p_sensor_cmd,mode_index,mipi_csi1_iic,SYNC_EDGE_RISE);
		}
	}

	if(csi_dev_id == HG_MIPI_CSI_DEVID){
		mipi_csi_debug_config(p_debug, csi_dev_id);
	}

    if (mipi_csi_check(csi_data_lane_num, csi_dev_id, p_sensor_cmd) != TRUE)
    {
		return FALSE;
    } else {

        if (csi_dev_id == HG_MIPI_CSI_DEVID)
        {
            g_mipi_csi_priv.mipi_csi0_en = 1;
			video_msg.csi0_iw = sensor_mode->width;
			video_msg.csi0_ih = sensor_mode->height;
			video_msg.csi0_type = 1;
			video_msg.video_type_cur  = ISP_VIDEO_0;
			video_msg.video_type_last = ISP_VIDEO_0;
			video_msg.video_num++;			
        }

        if (csi_dev_id == HG_MIPI1_CSI_DEVID)
        {
            g_mipi_csi_priv.mipi_csi1_en = 1;
			video_msg.csi1_iw = sensor_mode->width;
			video_msg.csi1_ih = sensor_mode->height;
			video_msg.csi1_type = 1;
			video_msg.video_type_cur  = ISP_VIDEO_0;
			video_msg.video_type_last = ISP_VIDEO_0;
			video_msg.video_num++;			
        }
		if(video_msg.video_num == 2){
			video_msg.camera_mode = camera_mode;
		}
    }

	return TRUE;
}

void mipi_csi_hardware_deconfig()
{
	struct mipi_csi_device *mipi_csi_dev = (struct mipi_csi_device *)dev_get(HG_MIPI_CSI_DEVID);
	struct mipi_csi_device *mipi1_csi_dev = (struct mipi_csi_device *)dev_get(HG_MIPI1_CSI_DEVID);

	struct pwm_device *global_hgpwm = (struct pwm_device *)dev_get(HG_PWM0_DEVID);
    struct capture_device *p_capture = (struct capture_device*)dev_get(HG_CAPTURE0_DEVID);

	if(mipi_csi_dev){
		mipi_csi_close(mipi_csi_dev);
		if (g_mipi_csi_priv.mipi_csi0_iic_id != 0) {
			unregister_iic_queue(g_mipi_csi_priv.mipi_csi0_iic_id);
		}
	}
	if (mipi1_csi_dev) {
		mipi_csi_close(mipi1_csi_dev);
		if (g_mipi_csi_priv.mipi_csi1_iic_id != 0) {
			unregister_iic_queue(g_mipi_csi_priv.mipi_csi1_iic_id);
		}
	}

    if(g_mipi_csi_priv.mipi_fsync_init == 1){
        capture_release_irq(p_capture, CAPTURE_CHANNEL_2);
        capture_deinit(p_capture, CAPTURE_CHANNEL_2);
        pwm_deinit(global_hgpwm,PWM_CHANNEL_0);
    }
    if(g_mipi_csi_priv.mipi_mclk_init){
        pwm_deinit(global_hgpwm,PWM_CHANNEL_1);
    }

    if (set_fps_task_hdl)   os_task_destroy(set_fps_task_hdl);
    if (set_fps_event.hdl)  os_event_del((void *)&set_fps_event);
    if (set_fps_msg.hdl)    os_msgq_del((void *)&set_fps_msg);

	os_memset(&video_msg, 0, sizeof(video_msg));
	os_memset(&g_mipi_csi_priv, 0, sizeof(g_mipi_csi_priv));
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
