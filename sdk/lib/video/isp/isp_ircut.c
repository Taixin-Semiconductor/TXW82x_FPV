#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "lib/video/dvp/cmos_sensor/csi_V2.h"
#include "devid.h"
#include "hal/gpio.h"
#include "hal/isp.h"
#include "hal/i2c.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "dev/vpp/hgvpp.h"
#include "dev/csi/hgdvp.h"
#include "lib/lcd/lcd.h"
#include "hal/jpeg.h"
#include "hal/gpio.h"
#include "lib/video/isp/isp_ircut.h"

#ifdef PIN_FROM_PARAM
#include "pin_param.h"
#endif
#include "syscfg.h"

IRCUT_INFO ircut_info;


void irled_control(uint8 led_state)
{
    gpio_set_val(MACRO_PIN(PIN_IRCUT_LED), led_state);
}

void whiteled_control(uint8 led_state)
{
    gpio_set_val(MACRO_PIN(PIN_WHITE_LED), led_state);
}

void ircut_control(IRCUT_INFO *info)
{
    switch (info->ircut_opt_status)
    {
        case IRCUT_OPT_STATE_ON:
            if (info->ircut_status == IRCUT_OFF)
            {
                gpio_set_val(MACRO_PIN(PIN_IRCUT_IN1), 1);
                gpio_set_val(MACRO_PIN(PIN_IRCUT_IN2), 0);
                info->ircut_opt_status = IRCUT_OPT_STATE_SW_OFF;
            }
            break;

        case IRCUT_OPT_STATE_OFF:
            if (info->ircut_status == IRCUT_ON)
            {
                gpio_set_val(MACRO_PIN(PIN_IRCUT_IN1), 0);
                gpio_set_val(MACRO_PIN(PIN_IRCUT_IN2), 1);
                info->ircut_opt_status = IRCUT_OPT_STATE_SW_ON;
            }
            break;

        case IRCUT_OPT_STATE_IDLE:
            if (info->ircut_status == IRCUT_ON)
            {
                gpio_set_val(MACRO_PIN(PIN_IRCUT_IN1), 0);
                gpio_set_val(MACRO_PIN(PIN_IRCUT_IN2), 1);
                info->ircut_opt_status = IRCUT_OPT_STATE_SW_ON;
            } else if (info->ircut_status == IRCUT_OFF) {
                gpio_set_val(MACRO_PIN(PIN_IRCUT_IN1), 1);
                gpio_set_val(MACRO_PIN(PIN_IRCUT_IN2), 0);
                info->ircut_opt_status = IRCUT_OPT_STATE_SW_ON;
            }
            break;

        case IRCUT_OPT_STATE_SW_ON:
            if (++info->frame_cnt >= info->frame_to_switch)
            {
                info->frame_cnt = 0;
                gpio_set_val(MACRO_PIN(PIN_IRCUT_IN1), 0);
                gpio_set_val(MACRO_PIN(PIN_IRCUT_IN2), 0);
                info->ircut_opt_status = IRCUT_OPT_STATE_ON;
                info->action_status    = IRCUT_ACTION_STOP;
            }
            break;

        case IRCUT_OPT_STATE_SW_OFF:
            if (++info->frame_cnt >= info->frame_to_switch)
            {
                info->frame_cnt = 0;
                gpio_set_val(MACRO_PIN(PIN_IRCUT_IN1), 0);
                gpio_set_val(MACRO_PIN(PIN_IRCUT_IN2), 0);
                info->ircut_opt_status = IRCUT_OPT_STATE_OFF;
                info->action_status    = IRCUT_ACTION_STOP;
            }
            break;     
        
        default:
            info->ircut_opt_status = IRCUT_OPT_STATE_OFF;
            gpio_set_val(MACRO_PIN(PIN_IRCUT_IN1), 0);
            gpio_set_val(MACRO_PIN(PIN_IRCUT_IN2), 0);
            break;
    }
}

void ircut_action(struct os_work *work)
{
    switch (ircut_info.irled_detect_mode)
    {
        case IRCUT_DET_MODE_HW:
            if (ircut_info.irdet_gpio_en)
            {
                ircut_info.irdet_status = gpio_get_val(MACRO_PIN(PIN_IRCUT_DETECT));
                if (ircut_info.irdet_status && (ircut_info.irled_status == IRLED_OFF)) {
                    if (++ircut_info.switch_cnt >= ircut_info.switch_max)
                    {
                        ircut_info.switch_cnt = 0;
                        ircut_info.whiteled_status = WHITELED_ON;
                        ircut_info.irled_status  = IRLED_ON;
                        ircut_info.ircut_status  = IRCUT_OFF;
                        ircut_info.action_status = IRCUT_ACTION_START;
                        isp_black_white_enable(ircut_info.dev, ircut_info.irled_status, SENSOR_TYPE_MASTER);
                    }
                } else if (!ircut_info.irdet_status && (ircut_info.irled_status == IRLED_ON)) {
                    if (++ircut_info.switch_cnt >= ircut_info.switch_max)
                    {
                        ircut_info.switch_cnt = 0;
                        ircut_info.whiteled_status = WHITELED_OFF;
                        ircut_info.irled_status  = IRLED_OFF;
                        ircut_info.ircut_status  = IRCUT_ON;
                        ircut_info.action_status = IRCUT_ACTION_START;
                        isp_black_white_enable(ircut_info.dev, ircut_info.irled_status, SENSOR_TYPE_MASTER);
                    }
                } else {
                    ircut_info.switch_cnt = 0;
                }
            }
            break;

        case IRCUT_DET_MODE_SW:
        {
            ISP_IRCUT_STAT isp_stat;
			isp_get_isp_ircut_statistics(ircut_info.dev,&isp_stat,SENSOR_TYPE_MASTER);
			
//			os_printf("r_mean=%d  g_mean=%d  b_mean=%d \r\n",isp_stat.r_mean,isp_stat.g_mean,isp_stat.b_mean);
//
//			os_printf("sat=%d  bv=%f \r\n",isp_stat.saturation,isp_stat.curr_bv);	  

            switch (ircut_info.sw_state)
            {
				//Status 1: from day to night
                case IRCUT_STAT_DAY_TO_NIGHT:
                {
                    if (isp_stat.curr_bv < ircut_info.to_night_bv){
                        ircut_info.switch_cnt++;
                        if (ircut_info.switch_cnt >= ircut_info.switch_max ){
                            ircut_info.switch_cnt = 0;
                            ircut_info.whiteled_status = WHITELED_ON;
                            ircut_info.irled_status = IRLED_ON;
                            ircut_info.ircut_status = IRCUT_OFF;
                            ircut_info.action_status = IRCUT_ACTION_START;
                            isp_black_white_enable(ircut_info.dev, ircut_info.irled_status, SENSOR_TYPE_MASTER);
                            isp_awb_measure_mode_config(ircut_info.dev, AWB_MEAS_MODE_RGB, SENSOR_TYPE_MASTER);
                            ircut_info.sw_state = IRCUT_STAT_WB_REC;
                        }
                    }else{
                        ircut_info.switch_cnt = 0;
                    }
                    break;
                }
                // State 2: Save stable white balance statistics
                case IRCUT_STAT_WB_REC:
                {
                    ircut_info.switch_cnt++;
                    if (ircut_info.switch_cnt >= 20){//after one second
                        ircut_info.switch_cnt = 0;
                        //os_printf("REC r_gain=%d  b_gain=%d \r\n",isp_stat.r_gain,isp_stat.b_gain);
                        ircut_info.last_r_gain = isp_stat.r_gain;
                        ircut_info.last_b_gain = isp_stat.b_gain;
                        ircut_info.sw_state = IRCUT_STAT_NIGHT_TO_DAY;
                    }
                    break;
                }
                //Status 3: from night to day
                case IRCUT_STAT_NIGHT_TO_DAY:
                {
                    uint16 diff_r_gain = IR_ABS(isp_stat.r_gain - ircut_info.last_r_gain);
				    uint16 diff_b_gain = IR_ABS(isp_stat.b_gain - ircut_info.last_b_gain);
                    //os_printf("diff_r_gain=%d  diff_b_gain=%d \r\n",diff_r_gain,diff_b_gain);
                    if (((isp_stat.curr_bv > ircut_info.to_day_bv) &&(isp_stat.saturation > ircut_info.to_day_sat)
						&& ((diff_r_gain + diff_b_gain) > ircut_info.to_day_diff_rb_gain || diff_b_gain > ircut_info.to_day_diff_b_gain))
                         || ((isp_stat.saturation > ircut_info.to_day_sat) && (isp_stat.curr_bv > ircut_info.to_day_bv_max))){
                        ircut_info.switch_cnt++;
                        if (ircut_info.switch_cnt >= ircut_info.switch_max)
                        {
                            ircut_info.switch_cnt = 0;
                            ircut_info.whiteled_status = WHITELED_OFF;
                            ircut_info.irled_status = IRLED_OFF;
                            ircut_info.ircut_status = IRCUT_ON;
                            ircut_info.action_status = IRCUT_ACTION_START;
                            isp_black_white_enable(ircut_info.dev, ircut_info.irled_status, SENSOR_TYPE_MASTER);
                            isp_awb_measure_mode_config(ircut_info.dev, AWB_MEAS_MODE_YUV_NEW, SENSOR_TYPE_MASTER);
                            ircut_info.sw_state = IRCUT_STAT_DAY_TO_NIGHT;
                        }
                    }else{
                        ircut_info.switch_cnt = 0;
                    }
                    break;
                }
            }
            break;
        }
        case IRCUT_DET_MODE_MANUAL:
            ircut_info.action_status = IRCUT_ACTION_STOP;
            break;

        default :
            os_printf(KERN_ERR"ircut detect mode : %d err", ircut_info.irled_detect_mode);
    }

    if (ircut_info.action_status)
    {
        if (ircut_info.ircut_gpio_en)       ircut_control(&ircut_info);
        if (ircut_info.irled_gpio_en)       irled_control(ircut_info.irled_status);
        //if (ircut_info.whiteled_gpio_en)    whiteled_control(ircut_info.whiteled_status);
    } 

    os_run_work_delay(&ircut_info.ircut_action_work, 50);
}

void ircut_init()
{
    os_memset(&ircut_info, 0, sizeof(IRCUT_INFO));
    ircut_info.dev = (struct isp_device *)dev_get(HG_ISP_DEVID);
    if (ircut_info.dev == NULL)
    {
        os_printf(KERN_ERR"ircut get isp device err!\r\n");
        return;
    }
     
    ircut_info.frame_cnt         = 0;
    ircut_info.frame_to_switch   = 3;
    ircut_info.switch_cnt        = 0;
    ircut_info.switch_max        = 30;
    ircut_info.to_day_bv         = 8000;
    ircut_info.to_night_bv       = 3500;
    ircut_info.to_day_sat        = 30;
    ircut_info.to_day_bv_max     = 10000;
    ircut_info.to_day_diff_rb_gain  = 45;
    ircut_info.to_day_diff_b_gain  = 30;
    ircut_info.ircut_opt_status  = IRCUT_OPT_STATE_IDLE;
    ircut_info.ircut_status      = IRCUT_ON;
    ircut_info.irled_status      = IRLED_OFF;
    ircut_info.irdet_status      = IRDET_OFF;
    ircut_info.whiteled_status   = WHITELED_OFF;
    ircut_info.action_status     = IRCUT_ACTION_START;
    ircut_info.irled_detect_mode = IRCUT_DET_MODE_MANUAL;

    if (MACRO_PIN(PIN_IRCUT_DETECT) != 255)
    {
        gpio_set_dir(MACRO_PIN(PIN_IRCUT_DETECT), GPIO_DIR_INPUT);
        ircut_info.irled_detect_mode = IRCUT_DET_MODE_HW;
        ircut_info.irdet_gpio_en = 1;
    } else {
        ircut_info.irdet_gpio_en = 0;
    }

    if (MACRO_PIN(PIN_IRCUT_LED) != 255)
    {
        gpio_iomap_output(MACRO_PIN(PIN_IRCUT_LED), GPIO_IOMAP_OUTPUT);
        irled_control(ircut_info.irled_status);
        ircut_info.irled_gpio_en = 1;
    }

    if (MACRO_PIN(PIN_WHITE_LED) != 255)
    {
        gpio_iomap_output(MACRO_PIN(PIN_WHITE_LED), GPIO_IOMAP_OUTPUT);
        whiteled_control(ircut_info.whiteled_status);
        ircut_info.whiteled_gpio_en = 1;
    }

    if ((MACRO_PIN(PIN_IRCUT_IN1) != 255) && (MACRO_PIN(PIN_IRCUT_IN2) != 255))
    {
        gpio_iomap_output(MACRO_PIN(PIN_IRCUT_IN1), GPIO_IOMAP_OUTPUT);
        gpio_iomap_output(MACRO_PIN(PIN_IRCUT_IN2), GPIO_IOMAP_OUTPUT);
        gpio_set_val(MACRO_PIN(PIN_IRCUT_IN1), 1);
        gpio_set_val(MACRO_PIN(PIN_IRCUT_IN2), 0);
        ircut_info.ircut_en      = 1;
        ircut_info.ircut_gpio_en = 1;
    } else {
        ircut_info.ircut_en = 0;
        ircut_info.ircut_gpio_en = 0;
    }

    if (ircut_info.ircut_en)
    {
        OS_WORK_INIT(&ircut_info.ircut_action_work, (void *)ircut_action, 0);
        os_run_work_delay(&ircut_info.ircut_action_work, 50);
    }
}
