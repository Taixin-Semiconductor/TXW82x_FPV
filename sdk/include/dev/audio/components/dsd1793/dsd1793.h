/**
  ******************************************************************************
  * @file    E:\work\venus_v2\SW\trunk\verify\test\audio\dsd1793.h
  * @author  Troy
  * @version V1.0.0
  * @date    01-31-2023
  * @brief   This file contains all the dsd1793 firmware functions.
  ******************************************************************************
  * @attention
  *
  * 
  *
  *
  *
  ******************************************************************************
  */ 
#ifndef __DSD1793_H
#define __DSD1793_H

#ifdef __cplusplus
 extern "C" {
#endif

#if 1

/*!
 * IIC config
 */
 
//PIN ADDR1, PIN ADDR0
//00, 01, 10, 11
#define DSD1793_PIN_ADDR0                        (0)
#define DSD1793_PIN_ADDR1                        (0)
#define DSD1793_DSD_MODE                         (1)
#define DSD1793_PCM_MODE                         (0)


#define DSD1793_IIC                              (HG_I2C1_DEVID)
#define DSD1793_IIC_DEVICE_ADDR                  ((0x4<<4) | (0xC | (DSD1793_PIN_ADDR1) | (DSD1793_PIN_ADDR0)))
#define DSD1793_IIC_SCL_PIN                      (PC_13)
#define DSD1793_IIC_SDA_PIN                      (PC_14)

#if 0
//#define DSD1793_IIC_SCL_GPIO_PORT                (GPIOC)
//#define DSD1793_IIC_SCL_PIN_NUM                  (13)
//#define DSD1793_IIC_SDA_GPIO_PORT                (GPIOC)
//#define DSD1793_IIC_SDA_PIN_NUM                  (14)
//#define DSD1793_IIC_SCL_PIN_BIT                  (BIT(DSD1793_IIC_SCL_PIN_NUM))
//#define DSD1793_IIC_SDA_PIN_BIT                  (BIT(DSD1793_IIC_SDA_PIN_NUM))


#define SMAPLE_FRQUENCY                          (48000)
#define OVER_SAMPLE_RATE                         (192)
#define __SYSCLK                                 (SYS_CLK)

#define SCK_IIS                                  (0)
#define SCK_TIMER                                (0)
#define DSD1793_SCK_GPIO_PORT                    (GPIOD)
#define DSD1793_SCK_GPIO_PIN_NUM                 (14)
#define DSD1793_SCK_GPIO_PIN_BIT                 (BIT(DSD1793_SCK_GPIO_PIN_NUM))


#define __dsd1793_iic_module_init()\
    do{\
        TYPE_LL_IIC_INIT _init;\
        TYPE_LL_IIC_CFG _cfg;\
        memset(&_cfg, 0, sizeof(TYPE_LL_IIC_CFG));\
        memset(&_init, 0, sizeof(TYPE_LL_IIC_INIT));\
        _cfg.presc             =  8;\
        _cfg.scldel            = 15;\
        _cfg.sdadel            = 15;\
        _cfg.scll              = 31;\
        _cfg.sclh              = 31;\
        _cfg.work_mode         = LL_IIC_MASTER_MODE;\
        ll_iic_init(DSD1793_IIC, &_init);\
        ll_iic_config(DSD1793_IIC, &_cfg);\
    }while(0);
    
#define __dsd1793_iic_io_init()\
    do{\
        ll_gpio_iomap_inout_config(DSD1793_IIC_SCL_GPIO_PORT, DSD1793_IIC_SCL_PIN_NUM, LL_GPIO_OUT_SPI2_SCK_OUT,\
                                                                                   LL_GPIO_IN_SPI2_SCK_IN);\
        ll_gpio_iomap_inout_config(DSD1793_IIC_SDA_GPIO_PORT, DSD1793_IIC_SDA_PIN_NUM, LL_GPIO_OUT_SPI2_IO0_OUT,\
                                                                                   LL_GPIO_IN_SPI2_IO0_IN);\
        ll_gpio_ospeed_config(DSD1793_IIC_SCL_GPIO_PORT     , DSD1793_IIC_SCL_PIN_NUM, LL_GPIO_OSPEED_3);\
        ll_gpio_ospeed_config(DSD1793_IIC_SDA_GPIO_PORT     , DSD1793_IIC_SDA_PIN_NUM, LL_GPIO_OSPEED_3);\
        ll_gpio_pull_config(DSD1793_IIC_SCL_GPIO_PORT       , DSD1793_IIC_SCL_PIN_NUM, LL_GPIO_OPENDRAIN_PULL_UP, LL_GPIO_PULL_LEVEL_4_7K);\
        ll_gpio_pull_config(DSD1793_IIC_SDA_GPIO_PORT       , DSD1793_IIC_SDA_PIN_NUM, LL_GPIO_OPENDRAIN_PULL_UP, LL_GPIO_PULL_LEVEL_4_7K);\
        ll_gpio_bit_set(DSD1793_IIC_SCL_GPIO_PORT, DSD1793_IIC_SCL_PIN_BIT);\
        ll_gpio_bit_set(DSD1793_IIC_SDA_GPIO_PORT,DSD1793_IIC_SDA_PIN_BIT);\
    }while(0);

#if (SCK_IIS)
#define __dsd1793_sck_init()\
        do{\
            TYPE_LL_IIS_INIT _init;\
            TYPE_LL_IIS_CFG _cfg;\
            memset(&_init, 0, sizeof(TYPE_LL_IIS_INIT));\
            memset(&_cfg,  0, sizeof(TYPE_LL_IIS_CFG));\
            _cfg.mode            = LL_IIS_MASTER_MODE;\
            _cfg.work_mode       = LL_IIS_SIMPLEX_TX;\
            _cfg.channel_mode    = LL_IIS_STEREO_MODE;\
            _cfg.ws_pol          = LL_IIS_LOW_FOR_LEFT_CHANNEL;\
            _cfg.bck_pol         = LL_IIS_SAMPLE_IN_RISING_EDGE;\
            _cfg.data_format     = LL_IIS_STANDARD_MODE;\
            _cfg.mclk_sel        = LL_IIS_MCLK_SEL_SYS_PLL;\
            _cfg.mclk_output     = LL_IIS_MCLK_OUT_ENABLE;\
            _cfg.mclk_div        = 16;\
            _cfg.baudrate        = 29;\
            _cfg.ws              = 31;\
            _cfg.bit             = 31;\
            ll_gpio_iomap_inout_config(DSD1793_SCK_GPIO_PORT, DSD1793_SCK_GPIO_PIN_NUM, LL_GPIO_OUT_IIS0_MCLK_OUT , LL_GPIO_IN_IIS0_MCLK_IN__LCD_D19_IN_MASK2_6);\
            ll_iis_config(IIS0, &_cfg);\
        }while(0);
#endif

#if (SCK_TIMER)
#define __dsd1793_sck_init()\
        do{\
            TYPE_LL_TIMER_CFG     _timer_cfg;\
            TYPE_LL_TIMER_INIT    _timer_init_cfg;\
            memset(&_timer_cfg, 0, sizeof(TYPE_LL_TIMER_CFG));\
            memset(&_timer_init_cfg, 0, sizeof(TYPE_LL_TIMER_INIT));\
            ll_timer_init(TIMER0, &_timer_init_cfg);\
            _timer_cfg.work_mode_sel = LL_TIMER_MODE_PWM;\
            _timer_cfg.in_src_sel    = LL_TIMER_IN_SRC_SYS_CLK;\
            _timer_cfg.prescaler_sel = 0;\
            _timer_cfg.period_val    = (__SYSCLK/(SMAPLE_FRQUENCY*OVER_SAMPLE_RATE)) - 1;\
            _timer_cfg.pwm_val       = ((__SYSCLK/(SMAPLE_FRQUENCY*OVER_SAMPLE_RATE))/2) - 1;\
            _timer_cfg.count_val     = 0;\
            ll_timer_config(TIMER0, &_timer_cfg);;\
            ll_gpio_ospeed_config(DSD1793_SCK_GPIO_PORT, (DSD1793_SCK_GPIO_PIN_NUM), 7);\
            ll_gpio_iomap_output_config(DSD1793_SCK_GPIO_PORT, (DSD1793_SCK_GPIO_PIN_NUM), LL_GPIO_OUT_M1_9_LCD_DATA_O_12__TMR0_PWM_OUT);\
            ll_timer_start(TIMER0);\
        }while(0);
#endif
        
#define __dsd1793_iic_write_1byte(data)\
    do{\
        ll_iic_master_tx(DSD1793_IIC, data, LL_IIC_NONE_FLAG);\
    }while(0);

#define __dsd1793_iic_read_1byte(data)\
    do{\
        data = ll_iic_master_rx(DSD1793_IIC, LL_IIC_NONE_FLAG);\
    }while(0);


#define __dsd1793_iic_write(reg, p_data, len)\
        do{\
            uint8 *__p_data = (uint8 *)p_data;\
            if(__p_data){\
                ll_iic_master_tx(DSD1793_IIC, DSD1793_IIC_DEVICE_ADDR<<1 | 0, LL_IIC_START_FLAG);\
                __dsd1793_iic_write_1byte(reg);\
                if(len){\
                    for(uint16 __len=0; __len<len-1; __len++){\
                        __dsd1793_iic_write_1byte(*__p_data++);\
                    }\
                }\
                ll_iic_master_tx(DSD1793_IIC, *__p_data, LL_IIC_STOP_FLAG);\
            }\
        }while(0);
    
#define __dsd1793_iic_read(reg, p_data, len)\
        do{\
            uint8 *__p_data = (uint8 *)p_data;\
            if(__p_data){\
                ll_iic_master_tx(DSD1793_IIC, DSD1793_IIC_DEVICE_ADDR<<1 | 0, LL_IIC_START_FLAG);\
                __dsd1793_iic_write_1byte(reg);\
                ll_iic_master_tx(DSD1793_IIC, DSD1793_IIC_DEVICE_ADDR<<1 | 1, LL_IIC_START_FLAG);\
                if(len){\
                    for(uint16 __len=0; __len<len-1; __len++){\
                        __dsd1793_iic_read_1byte(*__p_data++);\
                    }\
                }\
                *__p_data = ll_iic_master_rx(DSD1793_IIC, LL_IIC_STOP_FLAG | LL_IIC_NACK_FLAG);\
            }\
        }while(0);
#endif

#define __dsd1793_delay_ms(x)  os_sleep_ms(x)

/*!
 * DSD1793 reg index define 
 */
#define DSD1793_REG16    16
#define DSD1793_REG17    17
#define DSD1793_REG18    18
#define DSD1793_REG19    19
#define DSD1793_REG20    20
#define DSD1793_REG21    21
#define DSD1793_REG22    22



void dsd1793_reg_write_l8bit(uint16 reg, uint16 val_l8bit);
void dsd1793_reg_read_l8bit(uint16 reg, uint16 *val_l8bit);
void dsd1793_init(void);

#endif


#ifdef __cplusplus
}
#endif


#endif
/******************************** END OF FILE *****/



