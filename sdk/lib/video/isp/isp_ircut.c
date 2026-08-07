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

// 全局唯一IRCUT管理器
IRCUT_MGR g_ircut_mgr = {0};

/**
 * @brief 加载通道默认阈值
 */
static void ircut_load_thresh(uint8 ch_id, IRCUT_INFO *ircut_info)
{
    if (!ircut_info) return;
    switch(ch_id)
    {
        case 0: // sensor 0
            ircut_info->irled_detect_mode          = IRCUT_DET_MODE_MANUAL;
            ircut_info->thresh.frame_to_switch     = 3;
            ircut_info->thresh.switch_max          = 30;
            ircut_info->thresh.to_day_bv           = 8000.0f;
            ircut_info->thresh.to_night_bv         = 3500.0f;
            ircut_info->thresh.to_day_sat          = 30;
            ircut_info->thresh.to_day_bv_max       = 10000.0f;
            ircut_info->thresh.to_day_diff_rb_gain = 45;
            ircut_info->thresh.to_day_diff_b_gain  = 30;
            break;
        case 1: // sensor 1
            ircut_info->irled_detect_mode          = IRCUT_DET_MODE_MANUAL;
            ircut_info->thresh.frame_to_switch     = 3;
            ircut_info->thresh.switch_max          = 20;
            ircut_info->thresh.to_day_bv           = 7500.0f;
            ircut_info->thresh.to_night_bv         = 4200.0f;
            ircut_info->thresh.to_day_sat          = 25;
            ircut_info->thresh.to_day_bv_max       = 10000.0f;
            ircut_info->thresh.to_day_diff_rb_gain = 40;
            ircut_info->thresh.to_day_diff_b_gain  = 25;
            break;
        case 2: // sensor 2
            ircut_info->irled_detect_mode          = IRCUT_DET_MODE_MANUAL;
            ircut_info->thresh.frame_to_switch     = 4;
            ircut_info->thresh.switch_max          = 35;
            ircut_info->thresh.to_day_bv           = 8500.0f;
            ircut_info->thresh.to_night_bv         = 3000.0f;
            ircut_info->thresh.to_day_sat          = 35;
            ircut_info->thresh.to_day_bv_max       = 11000.0f;
            ircut_info->thresh.to_day_diff_rb_gain = 50;
            ircut_info->thresh.to_day_diff_b_gain  = 35;
            break;
        default: 
            os_printf(KERN_ERR"ircut load thresh invalid ch_id:%d\n", ch_id);
            break;
    }
}

/**
 * @brief 根据通道号加载对应IO引脚
 */
static void ircut_load_ch_pin(uint8 ch_id, IRCUT_CH_PIN *pin)
{
    os_memset(pin, 0, sizeof(IRCUT_CH_PIN));
    switch(ch_id)
    {
        case 0: // sensor 0
            pin->pin_ircut_det  = 255;//PIN_IRCUT_DETECT_CH0;
            pin->pin_irled      = 255;//PIN_IRCUT_LED_CH0;
            pin->pin_whiteled   = 255;//PIN_WHITE_LED_CH0;
            pin->pin_ircut_in1  = 255;//PIN_IRCUT_IN1_CH0;
            pin->pin_ircut_in2  = 255;//PIN_IRCUT_IN2_CH0;
            break;
        case 1: // sensor 1
            pin->pin_ircut_det  = 255;
            pin->pin_irled      = 255;
            pin->pin_whiteled   = 255;
            pin->pin_ircut_in1  = 255;
            pin->pin_ircut_in2  = 255;
            break;
        case 2: // sensor 2
            pin->pin_ircut_det  = 255;
            pin->pin_irled      = 255;
            pin->pin_whiteled   = 255;
            pin->pin_ircut_in1  = 255;
            pin->pin_ircut_in2  = 255;
            break;
        default:
            os_printf(KERN_ERR"ircut load pin invalid ch_id:%d\n", ch_id);
            break;
    }
}

/**
 * @brief 红外灯控制，直接使用通道变量引脚
 */
static void irled_control(IRCUT_INFO *ircut_info, uint8 led_state)
{
    gpio_set_val(ircut_info->pin.pin_irled, led_state);
}

/**
 * @brief 白光灯控制
 */
static void whiteled_control(IRCUT_INFO *ircut_info, uint8 led_state)
{
    gpio_set_val(ircut_info->pin.pin_whiteled, led_state);
}

/**
 * @brief IRCUT电机状态机控制
 */
static void ircut_control(IRCUT_INFO *ircut_info)
{
    uint8 pin1 = ircut_info->pin.pin_ircut_in1;
    uint8 pin2 = ircut_info->pin.pin_ircut_in2;
    switch (ircut_info->ircut_opt_status)
    {
        case IRCUT_OPT_STATE_ON:
            if (ircut_info->ircut_status == IRCUT_OFF)
            {
                gpio_set_val(pin1, 1);
                gpio_set_val(pin2, 0);
                ircut_info->ircut_opt_status = IRCUT_OPT_STATE_SW_OFF;
            }
            break;

        case IRCUT_OPT_STATE_OFF:
            if (ircut_info->ircut_status == IRCUT_ON)
            {
                gpio_set_val(pin1, 0);
                gpio_set_val(pin2, 1);
                ircut_info->ircut_opt_status = IRCUT_OPT_STATE_SW_ON;
            }
            break;

        case IRCUT_OPT_STATE_IDLE:
            if (ircut_info->ircut_status == IRCUT_ON)
            {
                gpio_set_val(pin1, 0);
                gpio_set_val(pin2, 1);
                ircut_info->ircut_opt_status = IRCUT_OPT_STATE_SW_ON;
            }
            else if (ircut_info->ircut_status == IRCUT_OFF)
            {
                gpio_set_val(pin1, 1);
                gpio_set_val(pin2, 0);
                ircut_info->ircut_opt_status = IRCUT_OPT_STATE_SW_ON;
            }
            break;

        case IRCUT_OPT_STATE_SW_ON:
            if (++ircut_info->frame_cnt >= ircut_info->thresh.frame_to_switch)
            {
                ircut_info->frame_cnt = 0;
                gpio_set_val(pin1, 0);
                gpio_set_val(pin2, 0);
                ircut_info->ircut_opt_status = IRCUT_OPT_STATE_ON;
                ircut_info->action_status    = IRCUT_ACTION_STOP;
            }
            break;

        case IRCUT_OPT_STATE_SW_OFF:
            if (++ircut_info->frame_cnt >= ircut_info->thresh.frame_to_switch)
            {
                ircut_info->frame_cnt = 0;
                gpio_set_val(pin1, 0);
                gpio_set_val(pin2, 0);
                ircut_info->ircut_opt_status = IRCUT_OPT_STATE_OFF;
                ircut_info->action_status    = IRCUT_ACTION_STOP;
            }
            break;

        default:
            ircut_info->ircut_opt_status = IRCUT_OPT_STATE_OFF;
            gpio_set_val(pin1, 0);
            gpio_set_val(pin2, 0);
            break;
    }
}

/**
 * @brief 单通道业务处理逻辑
 */
static void ircut_ch_process(IRCUT_MGR *mgr, IRCUT_INFO *ircut_info)
{
    if (!mgr || !ircut_info || !ircut_info->valid)
        return;

    switch (ircut_info->irled_detect_mode)
    {
        case IRCUT_DET_MODE_HW:
            if (ircut_info->irdet_gpio_en)
            {
                ircut_info->irdet_status = gpio_get_val(ircut_info->pin.pin_ircut_det);
                if (ircut_info->irdet_status && (ircut_info->irled_status == IRLED_OFF))
                {
                    if (++ircut_info->switch_cnt >= ircut_info->thresh.switch_max)
                    {
                        ircut_info->switch_cnt = 0;
                        ircut_info->whiteled_status = WHITELED_ON;
                        ircut_info->irled_status  = IRLED_ON;
                        ircut_info->ircut_status  = IRCUT_OFF;
                        ircut_info->action_status = IRCUT_ACTION_START;
                        isp_black_white_enable(mgr->isp_dev, ircut_info->irled_status, ircut_info->ch_id);
                    }
                }
                else if (!ircut_info->irdet_status && (ircut_info->irled_status == IRLED_ON))
                {
                    if (++ircut_info->switch_cnt >= ircut_info->thresh.switch_max)
                    {
                        ircut_info->switch_cnt = 0;
                        ircut_info->whiteled_status = WHITELED_OFF;
                        ircut_info->irled_status  = IRLED_OFF;
                        ircut_info->ircut_status  = IRCUT_ON;
                        ircut_info->action_status = IRCUT_ACTION_START;
                        isp_black_white_enable(mgr->isp_dev, ircut_info->irled_status, ircut_info->ch_id);
                    }
                }
                else
                {
                    ircut_info->switch_cnt = 0;
                }
            }
            break;

        case IRCUT_DET_MODE_SW:
        {
            ISP_IRCUT_STAT isp_stat;
            isp_get_isp_ircut_statistics(mgr->isp_dev, &isp_stat, ircut_info->ch_id);

			// os_printf("sensor %d r_mean=%d  g_mean=%d  b_mean=%d \r\n",ircut_info->ch_id, isp_stat.r_mean,isp_stat.g_mean,isp_stat.b_mean);
			// os_printf("sensor %d sat=%d  bv=%f \r\n",ircut_info->ch_id, isp_stat.saturation,isp_stat.curr_bv);	  

            switch (ircut_info->sw_state)
            {
                case IRCUT_STAT_DAY_TO_NIGHT:
                    if (isp_stat.curr_bv < ircut_info->thresh.to_night_bv)//白天彩色切夜视黑白,只需判断bv值
                    {
                        ircut_info->switch_cnt++;
                        if (ircut_info->switch_cnt >= ircut_info->thresh.switch_max)
                        {
                            ircut_info->switch_cnt = 0;
                            ircut_info->whiteled_status = WHITELED_ON;
                            ircut_info->irled_status = IRLED_ON;
                            ircut_info->ircut_status = IRCUT_OFF;
                            ircut_info->action_status = IRCUT_ACTION_START;
                            isp_black_white_enable(mgr->isp_dev, ircut_info->irled_status, ircut_info->ch_id);
                            // isp_awb_measure_mode_config(mgr->isp_dev, AWB_MEAS_MODE_RGB, ircut_info->ch_id);
                            ircut_info->sw_state = IRCUT_STAT_WB_REC;
                        }
                    }
                    else
                    {
                        ircut_info->switch_cnt = 0;
                    }
                    break;

                case IRCUT_STAT_WB_REC:
                    ircut_info->switch_cnt++;
                    if (ircut_info->switch_cnt >= 20)//稳定计数
                    {
                        ircut_info->switch_cnt = 0;
                        ircut_info->last_r_gain = isp_stat.r_gain;
                        ircut_info->last_b_gain = isp_stat.b_gain;
                        ircut_info->sw_state = IRCUT_STAT_NIGHT_TO_DAY;
                    }
                    break;

                case IRCUT_STAT_NIGHT_TO_DAY:
                {
                    uint16 diff_r_gain = IR_ABS(isp_stat.r_gain - ircut_info->last_r_gain);
                    uint16 diff_b_gain = IR_ABS(isp_stat.b_gain - ircut_info->last_b_gain);
                    if (((isp_stat.curr_bv > ircut_info->thresh.to_day_bv) && (isp_stat.saturation > ircut_info->thresh.to_day_sat)
                        && ((diff_r_gain + diff_b_gain) > ircut_info->thresh.to_day_diff_rb_gain || diff_b_gain > ircut_info->thresh.to_day_diff_b_gain))
                        || ((isp_stat.saturation > ircut_info->thresh.to_day_sat) && (isp_stat.curr_bv > ircut_info->thresh.to_day_bv_max)))
                    {
                        ircut_info->switch_cnt++;
                        if (ircut_info->switch_cnt >= ircut_info->thresh.switch_max)
                        {
                            ircut_info->switch_cnt = 0;
                            ircut_info->whiteled_status = WHITELED_OFF;
                            ircut_info->irled_status = IRLED_OFF;
                            ircut_info->ircut_status = IRCUT_ON;
                            ircut_info->action_status = IRCUT_ACTION_START;
                            isp_black_white_enable(mgr->isp_dev, ircut_info->irled_status, ircut_info->ch_id);
//                            isp_awb_measure_mode_config(mgr->isp_dev, AWB_MEAS_MODE_YUV_NEW, ircut_info->ch_id);
                            ircut_info->sw_state = IRCUT_STAT_DAY_TO_NIGHT;
                        }
                    }
                    else
                    {
                        ircut_info->switch_cnt = 0;
                    }
                    break;
                }
            }
            break;
        }

        case IRCUT_DET_MODE_MANUAL:
            ircut_info->action_status = IRCUT_ACTION_STOP;
            break;

        default:
            os_printf(KERN_ERR "ch%d ircut detect mode err:%d\n", ircut_info->ch_id, ircut_info->irled_detect_mode);
            break;
    }

    // 执行硬件动作
    if (ircut_info->action_status == IRCUT_ACTION_START)
    {
        if (ircut_info->ircut_gpio_en)
            ircut_control(ircut_info);
        if (ircut_info->irled_gpio_en)
            irled_control(ircut_info, ircut_info->irled_status);
        if (ircut_info->whiteled_gpio_en)
           whiteled_control(ircut_info, ircut_info->whiteled_status);
    }
}

/**
 * @brief 全局共用工作队列回调，遍历所有通道
 */
static void ircut_action(struct os_work *work)
{
    (void)work;
    IRCUT_MGR *mgr = &g_ircut_mgr;
    if (!mgr->ircut_init)
        return;

    for (uint8 ch = 0; ch < IRCUT_MAX_CH; ch++)
    {
        IRCUT_INFO *p_info = &mgr->ch_info[ch];
        ircut_ch_process(mgr, p_info);
    }

    os_run_work_delay(&mgr->ircut_work, 50);
}

/**
 * @brief 单通道初始化
 */
uint8 ircut_ch_init(uint8 ch_id)
{
    IRCUT_MGR *mgr = &g_ircut_mgr;
    if (ch_id >= IRCUT_MAX_CH)
        return 0;

    IRCUT_INFO *ircut_info = &mgr->ch_info[ch_id];
    os_memset(ircut_info, 0, sizeof(IRCUT_INFO));
    ircut_info->ch_id = ch_id;

    // 加载当前通道独立引脚
    ircut_load_ch_pin(ch_id, &ircut_info->pin);
    // 加载阈值
    ircut_load_thresh(ch_id,ircut_info);

    // 运行状态初始化
    ircut_info->frame_cnt         = 0;
    ircut_info->switch_cnt        = 0;
    ircut_info->ircut_opt_status  = IRCUT_OPT_STATE_IDLE;
    ircut_info->action_status     = IRCUT_ACTION_START;

    ircut_info->ircut_status      = IRCUT_ON;
    ircut_info->irled_status      = IRLED_OFF;
    ircut_info->irdet_status      = IRDET_OFF;
    ircut_info->whiteled_status   = WHITELED_OFF;

    // 光敏IO初始化
    uint8 det_pin = ircut_info->pin.pin_ircut_det;
    if (det_pin != 255)
    {
        gpio_set_dir(det_pin, GPIO_DIR_INPUT);
        ircut_info->irdet_gpio_en = 1;
    }

    // 红外灯IO初始化
    uint8 ir_pin = ircut_info->pin.pin_irled;
    if (ir_pin != 255)
    {
        gpio_iomap_output(ir_pin, GPIO_IOMAP_OUTPUT);
        irled_control(ircut_info, ircut_info->irled_status);
        ircut_info->irled_gpio_en = 1;
    }

    // 白光灯IO初始化
    uint8 wl_pin = ircut_info->pin.pin_whiteled;
    if (wl_pin != 255)
    {
        gpio_iomap_output(wl_pin, GPIO_IOMAP_OUTPUT);
        whiteled_control(ircut_info, ircut_info->whiteled_status);
        ircut_info->whiteled_gpio_en = 1;
    }

    // IRCUT电机IN1/IN2初始化
    uint8 pin_ircut1 = ircut_info->pin.pin_ircut_in1;
    uint8 pin_ircut2 = ircut_info->pin.pin_ircut_in2;
    if (pin_ircut1 != 255 && pin_ircut2 != 255)
    {
        gpio_iomap_output(pin_ircut1, GPIO_IOMAP_OUTPUT);
        gpio_iomap_output(pin_ircut2, GPIO_IOMAP_OUTPUT);
        gpio_set_val(pin_ircut1, 1);
        gpio_set_val(pin_ircut2, 0);
        ircut_info->ircut_en      = 1;
        ircut_info->ircut_gpio_en = 1;
        ircut_info->valid = 1;
    }

    return ircut_info->valid;
}

/**
 * @brief IRCUT总初始化，防重复初始化
 */
void ircut_init(void)
{   
    uint8 ret = 0;
    IRCUT_MGR *mgr = &g_ircut_mgr;
    if (mgr->ircut_init)
    {
        os_printf("ircut already initialized, skip\r\n");
        return;
    }

    os_memset(mgr, 0, sizeof(IRCUT_MGR));
    
    mgr->ircut_init = 1;
    mgr->isp_dev    = (struct isp_device *)dev_get(HG_ISP_DEVID);	

    for (uint8 ch = 0; ch < IRCUT_MAX_CH; ch++)
    {
        ret |= ircut_ch_init(ch);
    }

    if(ret != 0)//只要任意一路存在 IRCUT 硬件
    {
        OS_WORK_INIT(&mgr->ircut_work, (void *)ircut_action, 0);
        os_run_work_delay(&mgr->ircut_work, 50);
        os_printf("ircut mgr init ok\n");
    }
    else
    {
        os_printf("ircut init skip, no ircut hardware\r\n");
    }
}

/**
 * @brief 完整去初始化：停止任务、关硬件、清空内存
 */
void ircut_deinit(void)
{
    IRCUT_MGR *mgr = &g_ircut_mgr;
    if (!mgr->ircut_init)
    {
        os_printf("ircut not init, skip deinit\n");
        return;
    }

    // 停止全局工作队列
    os_work_cancle(&mgr->ircut_work,1);

    // 遍历所有通道关闭硬件输出
    for (uint8 ch = 0; ch < IRCUT_MAX_CH; ch++)
    {
        IRCUT_INFO *ircut_info = &mgr->ch_info[ch];
        if (!ircut_info->valid)
            continue;
        // 关闭红外灯
        if (ircut_info->irled_gpio_en)
            gpio_set_val(ircut_info->pin.pin_irled, 0);
        // IRCUT电机断电
        if (ircut_info->ircut_gpio_en)
        {
            gpio_set_val(ircut_info->pin.pin_ircut_in1, 0);
            gpio_set_val(ircut_info->pin.pin_ircut_in2, 0);
        }
        ircut_info->valid = 0;
    }

    // 清空整个管理器
    os_memset(mgr, 0, sizeof(IRCUT_MGR));
    os_printf("ircut mgr deinit ok\n");
}



