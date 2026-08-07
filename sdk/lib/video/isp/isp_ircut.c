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

IRCUT_INFO ircut_info;


// #undef PIN_IRCUT_IN1           
// #undef PIN_IRCUT_IN2           
// #undef PIN_IRCUT_DETECT        
// #undef PIN_IRCUT_LED           
// #undef PIN_WHITE_LED           

// #define PIN_IRCUT_IN1           PC_15
// #define PIN_IRCUT_IN2           PC_14
// #define PIN_IRCUT_DETECT        PA_15
// #define PIN_IRCUT_LED           255
// #define PIN_WHITE_LED           255

void irled_control(uint8 led_state)
{
    gpio_set_val(PIN_IRCUT_LED, led_state);
}

void whiteled_control(uint8 led_state)
{
    gpio_set_val(PIN_WHITE_LED, led_state);
}

void ircut_control(IRCUT_INFO *info)
{
    switch (info->ircut_opt_status)
    {
        case IRCUT_OPT_STATE_ON:
            if (info->ircut_status == IRCUT_OFF)
            {
                gpio_set_val(PIN_IRCUT_IN1, 1);
                gpio_set_val(PIN_IRCUT_IN2, 0);
                info->ircut_opt_status = IRCUT_OPT_STATE_SW_OFF;
            }
            break;

        case IRCUT_OPT_STATE_OFF:
            if (info->ircut_status == IRCUT_ON)
            {
                gpio_set_val(PIN_IRCUT_IN1, 0);
                gpio_set_val(PIN_IRCUT_IN2, 1);
                info->ircut_opt_status = IRCUT_OPT_STATE_SW_ON;
            }
            break;

        case IRCUT_OPT_STATE_IDLE:
            if (info->ircut_status == IRCUT_ON)
            {
                gpio_set_val(PIN_IRCUT_IN1, 0);
                gpio_set_val(PIN_IRCUT_IN2, 1);
                info->ircut_opt_status = IRCUT_OPT_STATE_SW_ON;
            } else if (info->ircut_status == IRCUT_OFF) {
                gpio_set_val(PIN_IRCUT_IN1, 1);
                gpio_set_val(PIN_IRCUT_IN2, 0);
                info->ircut_opt_status = IRCUT_OPT_STATE_SW_ON;
            }
            break;

        case IRCUT_OPT_STATE_SW_ON:
            if (++info->frame_cnt >= info->frame_to_switch)
            {
                info->frame_cnt = 0;
                gpio_set_val(PIN_IRCUT_IN1, 0);
                gpio_set_val(PIN_IRCUT_IN2, 0);
                info->ircut_opt_status = IRCUT_OPT_STATE_ON;
                info->action_status    = IRCUT_ACTION_STOP;
            }
            break;

        case IRCUT_OPT_STATE_SW_OFF:
            if (++info->frame_cnt >= info->frame_to_switch)
            {
                info->frame_cnt = 0;
                gpio_set_val(PIN_IRCUT_IN1, 0);
                gpio_set_val(PIN_IRCUT_IN2, 0);
                info->ircut_opt_status = IRCUT_OPT_STATE_OFF;
                info->action_status    = IRCUT_ACTION_STOP;
            }
            break;     
        
        default:
            info->ircut_opt_status = IRCUT_OPT_STATE_OFF;
            gpio_set_val(PIN_IRCUT_IN1, 0);
            gpio_set_val(PIN_IRCUT_IN2, 0);
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
                ircut_info.irdet_status = gpio_get_val(PIN_IRCUT_DETECT);
                if (ircut_info.irdet_status && (ircut_info.irled_status == IRLED_OFF))
                {
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
            isp_get_current_bv(ircut_info.dev, (void *)&ircut_info.curr_bv, SENSOR_TYPE_MASTER);
            if ((ircut_info.curr_bv > ircut_info.to_day_bv) && (ircut_info.irled_status == IRLED_ON))
            {
                if (++ircut_info.switch_cnt >= ircut_info.switch_max)
                {
                    ircut_info.switch_cnt = 0;
                    ircut_info.whiteled_status = WHITELED_OFF;
                    ircut_info.irled_status = IRLED_OFF;
                    ircut_info.ircut_status = IRCUT_ON;
                    ircut_info.action_status = IRCUT_ACTION_START;
                    isp_black_white_enable(ircut_info.dev, ircut_info.ircut_status, SENSOR_TYPE_MASTER);
                }
            } else if ((ircut_info.curr_bv < ircut_info.to_night_bv) && (ircut_info.irled_status == IRLED_OFF)) {
                if (++ircut_info.switch_cnt >= ircut_info.switch_max)
                {
                    ircut_info.switch_cnt = 0;
                    ircut_info.whiteled_status = WHITELED_ON;
                    ircut_info.irled_status = IRLED_ON;
                    ircut_info.ircut_status = IRCUT_OFF;
                    ircut_info.action_status = IRCUT_ACTION_START;
                    isp_black_white_enable(ircut_info.dev, ircut_info.ircut_status, SENSOR_TYPE_MASTER);
                }
            } else {
                ircut_info.switch_cnt = 0;
            }
            break;

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
        if (ircut_info.whiteled_gpio_en)    whiteled_control(ircut_info.whiteled_status);
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
    ircut_info.to_day_bv         = 800;
    ircut_info.to_night_bv       = 195;
    ircut_info.ircut_opt_status  = IRCUT_OPT_STATE_IDLE;
    ircut_info.ircut_status      = IRCUT_ON;
    ircut_info.irled_status      = IRLED_OFF;
    ircut_info.irdet_status      = IRDET_OFF;
    ircut_info.whiteled_status   = WHITELED_OFF;
    ircut_info.action_status     = IRCUT_ACTION_START;
    ircut_info.irled_detect_mode = IRCUT_DET_MODE_MANUAL;

    if (PIN_IRCUT_DETECT != 255)
    {
        gpio_set_dir(PIN_IRCUT_DETECT, GPIO_DIR_INPUT);
        ircut_info.irled_detect_mode = IRCUT_DET_MODE_HW;
        ircut_info.irdet_gpio_en = 1;
    } else {
        ircut_info.irdet_gpio_en = 0;
    }

    if (PIN_IRCUT_LED != 255)
    {
        gpio_iomap_output(PIN_IRCUT_LED, GPIO_IOMAP_OUTPUT);
        irled_control(ircut_info.irled_status);
        ircut_info.irled_gpio_en = 1;
    }

    if (PIN_WHITE_LED != 255)
    {
        gpio_iomap_output(PIN_WHITE_LED, GPIO_IOMAP_OUTPUT);
        whiteled_control(ircut_info.whiteled_status);
        ircut_info.whiteled_gpio_en = 1;
    }

    if ((PIN_IRCUT_IN1 != 255) && (PIN_IRCUT_IN2 != 255))
    {
        gpio_iomap_output(PIN_IRCUT_IN1, GPIO_IOMAP_OUTPUT);
        gpio_iomap_output(PIN_IRCUT_IN2, GPIO_IOMAP_OUTPUT);
        gpio_set_val(PIN_IRCUT_IN1, 0);
        gpio_set_val(PIN_IRCUT_IN2, 0);
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
