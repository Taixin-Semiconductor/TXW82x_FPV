/**
 * @file hggpio_v4.c
 * @author bxd
 * @brief gpio
 * @version
 * TXW81X
 * @date 2023-08-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "dev.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "hal/gpio.h"
#include "dev/gpio/hggpio_v4.h"
#include "hggpio_v4_hw.h"
#include "osal/sleep.h"


#if 0
    #define GPIO_PRINTF(fmt, args...) printf(fmt, ##args)
#else 
    #define GPIO_PRINTF(fmt, args...)
#endif

/*  debug enable  */
#define GPIO_ENBUG_EN 0

/* mode define */
#define _GPIO_PULL_NONE  0x1
#define _GPIO_PULL_UP    0x2
#define _GPIO_PULL_DOWN  0x4

#define _GPIO_OPENDRAIN_PULL_MASK 0x80
#define _GPIO_OPENDRAIN_PULL_NONE (_GPIO_OPENDRAIN_PULL_MASK | _GPIO_PULL_NONE)
#define _GPIO_OPENDRAIN_PULL_UP   (_GPIO_OPENDRAIN_PULL_MASK | _GPIO_PULL_UP  )
#define _GPIO_OPENDRAIN_PULL_DOWN (_GPIO_OPENDRAIN_PULL_MASK | _GPIO_PULL_DOWN)

#define _GPIO_OPENDRAIN_DROP_MASK 0x40
#define _GPIO_OPENDRAIN_DROP_NONE (_GPIO_OPENDRAIN_DROP_MASK | _GPIO_PULL_NONE)
#define _GPIO_OPENDRAIN_DROP_DOWN (_GPIO_OPENDRAIN_DROP_MASK | _GPIO_PULL_DOWN)


#define GPIO_PULL_LEVEL_MASK  0xF


/* 一些软件标志位 */
#define GPIO_FLAG_IS_INTR_EN         0x1
#define GPIO_FLAG_IS_SUSPENDED       0x2


#define SYSCTRL_FLAG_IS_SUSPEND      0x1
#define SYSCTRL_FLAG_IS_USB2GPIO     0X80

#ifdef CONFIG_SLEEP
struct gpio_sysctrl {
    uint32_t                  flag;
    uint32_t                  iofuncin[15];
#if defined(TXW82X)
    uint32_t                  iomask[9];
#else
    uint32_t                  iomask[2];    
#endif
};
    static struct gpio_sysctrl gpio_sys = {
        .flag = 0,
    }; 
#endif    

#define IOMASK_OPA_CLEAR   0
#define IOMASK_OPA_WRITE   1
#define IOMASK_OPA_READ    2
#define IOMASK_OPA_CHECK   3

uint32_t hggpio_v4_mask_opa(struct hggpio_v4_hw *reg, uint32_t pin_num, enum gpio_iomap_out_func func_sel, uint32_t flag)
{
    uint8_t  mask_offset[8] = {(&SYSCTRL->IOFUNCMASK0)-(&SYSCTRL->IOFUNCMASK0),
                               (&SYSCTRL->IOFUNCMASK1)-(&SYSCTRL->IOFUNCMASK0),
                               (&SYSCTRL->IOFUNCMASK2)-(&SYSCTRL->IOFUNCMASK0),
                               (&SYSCTRL->IOFUNCMASK3)-(&SYSCTRL->IOFUNCMASK0),
                               (&SYSCTRL->IOFUNCMASK4)-(&SYSCTRL->IOFUNCMASK0),
                               (&SYSCTRL->IOFUNCMASK5)-(&SYSCTRL->IOFUNCMASK0),
                               (&SYSCTRL->IOFUNCMASK6)-(&SYSCTRL->IOFUNCMASK0),
                               (&SYSCTRL->IOFUNCMASK7)-(&SYSCTRL->IOFUNCMASK0),};
    uint32_t group_func_sel   = ((func_sel+4)/5);
    uint32_t mask_base      = (uint32_t)&(SYSCTRL->IOFUNCMASK0);
    uint32_t mask_bit       = (group_func_sel-1) * 4;
    uint32_t *maskp         = (uint32_t*)(mask_base+(mask_offset[mask_bit/32]*4)); 
    uint32_t mask_tmp = *maskp;
    //Config IOMASKFUNCOUTCONx Reg

    switch(flag) {
        case IOMASK_OPA_CLEAR:
            mask_tmp &= ~((0xf) << ((mask_bit % 32)));
            break;
        case IOMASK_OPA_WRITE:
            mask_tmp &= ~((0xf) << ((mask_bit % 32)));
            if (((func_sel-1)%5) < 4) {
                mask_tmp |= (BIT((func_sel-1)%5))  << ((mask_bit %32));
            }
            break;
        case IOMASK_OPA_READ:
            if ((mask_tmp & ((0xf << (mask_bit %32)))) >> (mask_bit %32) == 0) 
                return 0;
            else 
                return __builtin_ctz((mask_tmp & ((0xf << (mask_bit %32)))) >> (mask_bit %32));
        case IOMASK_OPA_CHECK:
            if (((func_sel-1)%5) < 4) {
                return ((BIT((func_sel-1)%5))  << ((mask_bit %32))) == 
                    (mask_tmp & ((0xf) << ((mask_bit % 32))));
            } else {
                return !(mask_tmp & ((0xf) << ((mask_bit % 32))));
            }
        default:
            ;
    }

    *maskp = mask_tmp;
	return 0;
}

static inline uint32 hggpio_v4_pin_num(struct hggpio_v4 *gpio, uint32 pin)
{
    return (pin & 0xf);
}

static int32 hggpio_v4_set_pull(struct hggpio_v4_hw *reg, uint32 pin, enum gpio_pin_mode pull_mode, enum gpio_pull_level level)
{
    int32  ret               = RET_OK;
    uint32 l_pin_pos         = (pin * 4);
    uint32 h_pin_pos         = (pin & 0x07) * 4;
    uint32 pull_level_to_reg = 0;

    if (pin >= HGGPIO_V4_MAX_PINS) {
        return -EINVAL;
    }

    switch (level) {
        case (GPIO_PULL_LEVEL_NONE):
            pull_level_to_reg = 0;
            break;
        case (GPIO_PULL_LEVEL_4_7K):
            pull_level_to_reg = 2;
            break;
        case (GPIO_PULL_LEVEL_100K):
            pull_level_to_reg = 1;
            break;
        
        default:
            return -EINVAL;
    }

    if((reg == (void*)GPIOF_BASE && pin <= 9) || (reg == (void*)GPIOE_BASE && pin >= 4)) {
        if(_GPIO_PULL_DOWN & pull_mode) {
            pull_level_to_reg &= ~1;
        }
    }

    sysctrl_unlock();

    if (pin < 8) {
        if (_GPIO_PULL_NONE == (pull_mode & 0xF)) {
                reg->PUDL &= ~(GPIO_PULL_LEVEL_MASK << l_pin_pos);
                reg->PUPL &= ~(GPIO_PULL_LEVEL_MASK << l_pin_pos);
        }else if (_GPIO_PULL_DOWN & pull_mode) {
                reg->PUDL  =  (reg->PUDL & ~ (GPIO_PULL_LEVEL_MASK << l_pin_pos)) | (pull_level_to_reg << l_pin_pos);
                reg->PUPL &= ~(GPIO_PULL_LEVEL_MASK << l_pin_pos);
        } else if (_GPIO_PULL_UP & pull_mode) {
                reg->PUDL &= ~(GPIO_PULL_LEVEL_MASK << l_pin_pos);
                reg->PUPL  =  (reg->PUPL & ~ (GPIO_PULL_LEVEL_MASK << l_pin_pos)) | (pull_level_to_reg << l_pin_pos);
        }
    } else {
        if (_GPIO_PULL_NONE == (pull_mode & 0xF)) {
                reg->PUDH &= ~(GPIO_PULL_LEVEL_MASK << h_pin_pos);
                reg->PUPH &= ~(GPIO_PULL_LEVEL_MASK << h_pin_pos);
        }else if (_GPIO_PULL_DOWN & pull_mode) {
                reg->PUDH  =  (reg->PUDH & ~ (GPIO_PULL_LEVEL_MASK << h_pin_pos)) | (pull_level_to_reg << h_pin_pos);
                reg->PUPH &= ~(GPIO_PULL_LEVEL_MASK << h_pin_pos);
        } else if (_GPIO_PULL_UP & pull_mode) {
                reg->PUDH &= ~(GPIO_PULL_LEVEL_MASK << h_pin_pos);
                reg->PUPH  =  (reg->PUPH & ~ (GPIO_PULL_LEVEL_MASK << h_pin_pos)) | (pull_level_to_reg << h_pin_pos);
        }
    }

    sysctrl_lock();
    
    return ret;
}

static int32 hggpio_v4_set_driver_strength(struct hggpio_v4_hw *reg, uint32 pin, enum pin_driver_strength drive)
{
    int32  ret = RET_OK;
    uint32 pin_pos = ((pin & 0x7) * 4);
    uint32 drive_to_reg = 0;

    if (pin >= HGGPIO_V4_MAX_PINS) {
        return -EINVAL;
    }

    drive_to_reg = drive;

    sysctrl_unlock();

#ifdef TXW82X
    // PE4-15 PF0-8的驱动等级由各自低位PD,OSPEED控制 {PD[0],DRV[0]}
	if((reg == (void*)GPIOF_BASE && pin <= 9) || (reg == (void*)GPIOE_BASE && pin >= 4)) {
		if (pin < 8) {
            reg->OSPEEDL = (reg->OSPEEDL & ~(0xF << pin_pos)) | ((drive_to_reg&1) << pin_pos);
			reg->PUDL = (reg->PUDL & ~(0xF << pin_pos)) | ((drive_to_reg>>1) << pin_pos);
		} else {
            reg->OSPEEDH = (reg->OSPEEDH & ~(0xF << pin_pos)) | ((drive_to_reg&1) << pin_pos);
			reg->PUDH = (reg->PUDH & ~(0xF << pin_pos)) | ((drive_to_reg>>1) << pin_pos);
		}
	}else {
		if (pin < 8) {
			reg->OSPEEDL = (reg->OSPEEDL & ~(0xF << pin_pos)) | (drive_to_reg << pin_pos);
		} else {
			reg->OSPEEDH = (reg->OSPEEDH & ~(0xF << pin_pos)) | (drive_to_reg << pin_pos);
		}
	}
#else
	if (pin < 8) {
		reg->OSPEEDL = (reg->OSPEEDL & ~(0xF << pin_pos)) | (drive_to_reg << pin_pos);
	} else {
		reg->OSPEEDH = (reg->OSPEEDH & ~(0xF << pin_pos)) | (drive_to_reg << pin_pos);
	}
#endif

    GPIO_PRINTF("%s, GPIO: 0x%x PIN: %d STRENTH: %d\r\npudl: 0x%x pudh: 0x%x spl: 0x%x sph: 0x%x\r\n", 
    __FUNCTION__, reg, pin%16, drive, reg->PUDL, reg->PUDH, reg->OSPEEDL, reg->OSPEEDH);
    sysctrl_lock();
    
    return ret;
}



static int32 hggpio_v4_set_mode(struct hggpio_v4_hw *reg, uint32 pin, enum gpio_pin_mode mode, enum gpio_pull_level level)
{

    uint32 pin_bit  = 1UL << pin;
    uint32 mode_tmp = 0;

    if (pin >= HGGPIO_V4_MAX_PINS) {
        return -EINVAL;
    }
    
    switch (mode) {
        
        case GPIO_PULL_NONE:
            mode_tmp = _GPIO_PULL_NONE;
            SYSCTRL_REG_OPT(reg->OTYPE  &= ~pin_bit);
            break;
            
        case GPIO_PULL_UP  :
            mode_tmp = _GPIO_PULL_UP;
            SYSCTRL_REG_OPT(reg->OTYPE  &= ~pin_bit);
            break;
            
        case GPIO_PULL_DOWN: 
            mode_tmp = _GPIO_PULL_DOWN;
            SYSCTRL_REG_OPT(reg->OTYPE  &= ~pin_bit);
            break;
            
        case GPIO_OPENDRAIN_PULL_NONE:
            mode_tmp = _GPIO_OPENDRAIN_PULL_NONE;
            SYSCTRL_REG_OPT(reg->OTYPE  |=  pin_bit);
            break;

        case GPIO_OPENDRAIN_PULL_UP:
            mode_tmp = _GPIO_OPENDRAIN_PULL_UP;
            SYSCTRL_REG_OPT(reg->OTYPE  |=  pin_bit);
            break;
            
        default:
            return -EINVAL;
    }

    return hggpio_v4_set_pull(reg, pin, mode_tmp, level);
}

static int32_t hggpio_v4_get_omap(struct gpio_device *dev, uint32_t pin)
{
    struct hggpio_v4 *gpio        = (struct hggpio_v4*)dev;
    struct hggpio_v4_hw *hw      = (struct hggpio_v4_hw *)gpio->hw;
    uint32_t iomap_out_pin_pos;
    uint32_t mode_pin_pos;
    uint32_t group_func;
    uint32_t iofunc;
    volatile uint32_t *operate_ptr = &(hw->IOFUNCOUTCON0);

    if (pin < gpio->pin_num[0] || pin > gpio->pin_num[1]) {
        return -EINVAL;
    }

    pin = hggpio_v4_pin_num(gpio, pin);
    iomap_out_pin_pos = ((pin & 0x3) << 3);
    mode_pin_pos      = pin *  2;
    operate_ptr      += pin >> 2;

    group_func = (((*operate_ptr) >> iomap_out_pin_pos)&0xff);

    if (group_func == 0) {
        return 0;
    }

    iofunc = group_func*5 - 4;
    if (hggpio_v4_mask_opa(hw, pin, iofunc, IOMASK_OPA_READ))
        iofunc += hggpio_v4_mask_opa(hw, pin, iofunc, IOMASK_OPA_READ);
    else 
        iofunc += 4;

    return iofunc;
}

static int32_t hggpio_v4_get_imap(struct gpio_device *dev, uint32_t pin)
{
//    struct hggpio_v4 *gpio         = (struct hggpio_v4*)dev;
    volatile uint32_t *operate_ptr = &(SYSCTRL->IOFUNCINCON0);
    uint32_t reg;
    for(int i = 0;i<=14;i++)
    {
        reg = *operate_ptr;
        for(int j = 0;j<4;j++)
        {
            if((reg & (0xff<<(j*8))) == (pin<<(j*8))) {
                return i*4+j;
            }
        }
        operate_ptr++;
    }

    return 0xF000;
}

/*
 * @brief:  
 * @ret: 返回值是16个bit, 对应16个IO(Px0->Px15). 复用了对应func则为1  
*/
static int32_t hggpio_v4_get_omap_pin(struct gpio_device *dev, enum gpio_iomap_out_func iomap_out_func_sel)
{
    struct hggpio_v4 *gpio        = (struct hggpio_v4*)dev;
    struct hggpio_v4_hw *hw      = (struct hggpio_v4_hw *)gpio->hw;    
    unsigned int gio_func = (iomap_out_func_sel+4)/5;
    uint8_t *cur_gio_func = (uint8_t *)&hw->IOFUNCOUTCON0;
    int ret = 0;


    if (!((hggpio_v4_mask_opa(hw, 0, iomap_out_func_sel, IOMASK_OPA_CHECK))
        || (iomap_out_func_sel == GPIO_IOMAP_OUTPUT))) 
        return 0;

    for(int i = 0;i<16;i++)
    {
        if (cur_gio_func[i] == gio_func) 
            ret |= 1<<i;
    }

    return ret;
}
///////////////////////////////attach function///////////////////////

static int32_t hggpio_v4_iomap_output(struct gpio_device *gpio, uint32_t pin, enum gpio_iomap_out_func iomap_out_func_sel) {

    int32 ret_val = RET_OK;
    struct hggpio_v4 *dev        = (struct hggpio_v4*)gpio;
    struct hggpio_v4_hw *hw      = (struct hggpio_v4_hw *)dev->hw;
    uint32_t iomap_out_pin_pos     = 0;
    uint32_t mode_pin_pos          = 0;
    uint32_t new_func_sel   = ((iomap_out_func_sel+4)/5);
    volatile uint32_t *operate_ptr = &(hw->IOFUNCOUTCON0);

    if (pin < dev->pin_num[0] || pin > dev->pin_num[1]) {
        return -EINVAL;
    }

    pin = hggpio_v4_pin_num(dev, pin);
    iomap_out_pin_pos = ((pin & 0x3) << 3);
    mode_pin_pos      = pin *  2;
    operate_ptr      += pin >> 2;

    sysctrl_unlock();

    if (GPIOC_BASE == dev->hw) {
        if ((6 == pin) || (7 == pin)) {
            //PC6->DP; PC7->DM change to GPIO
            SYSCTRL->SYS_CON1 &= ~ BIT(18); //USB20_PHY reset
            __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
            SYSCTRL->USB20_PHY_CFG3 |= (0xf << 0);
            __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
            SYSCTRL->SYS_CON1 |= BIT(18);
        }
    }

	/* clear simulation enablement for corresponding pins */
    hw->AIOEN &= ~(1 << pin);
#ifdef TXW82X
	//Config IOFUNCOUTCONx Reg
    uint32_t tmp;
	//如果这个IO已经配置了IOFUNC， 则清空IOFUNC以及对应的IOFUNCMASK
    if( (((*operate_ptr) >> iomap_out_pin_pos)&0xff)) {
		tmp = ((*operate_ptr) >> iomap_out_pin_pos)&0xff;
		*operate_ptr = (*operate_ptr & ~(0xFF << iomap_out_pin_pos));
		hggpio_v4_mask_opa(hw, pin, tmp*5, IOMASK_OPA_CLEAR);
    }

	//iomap_out_func_sel = 0(LL_GPIO_OUTPUT)时不需要配mask
	if(iomap_out_func_sel != 0) {
    	hggpio_v4_mask_opa(hw, pin, iomap_out_func_sel, IOMASK_OPA_WRITE);
 	}

    *operate_ptr = (*operate_ptr & ~(0xFF << iomap_out_pin_pos)) | ((new_func_sel & 0x000000FF) << iomap_out_pin_pos);
#else
    //Config IOFUNCOUTCONx Reg
    *operate_ptr = (*operate_ptr & ~(0xFF << iomap_out_pin_pos)) | ((iomap_out_func_sel & 0x000000FF) << iomap_out_pin_pos);
#endif
    //Config MODE
    hw->MODE = (hw->MODE & ~(0x3 << mode_pin_pos)) | (0x01 << mode_pin_pos);
    
    sysctrl_lock();
    return ret_val;
}

static int32 hggpio_v4_iomap_input(struct gpio_device *gpio, uint32 pin, enum gpio_iomap_in_func iomap_in_func_sel) {

    struct hggpio_v4 *dev     = (struct hggpio_v4*)gpio;
    struct hggpio_v4_hw *hw   = (struct hggpio_v4_hw *)dev->hw;
    int32 ret_val             = RET_OK;
    uint32 pin_num_temp       = 0;
    uint32 pin_pos            = (iomap_in_func_sel & 0x3) << 3;
    volatile uint32 *operate_ptr = &(SYSCTRL->IOFUNCINCON0);

    if (pin < dev->pin_num[0] || pin > dev->pin_num[1]) {
        return -EINVAL;
    }

    pin = hggpio_v4_pin_num(dev, pin);
    pin_num_temp = pin;

    sysctrl_unlock();

    if (GPIOC_BASE == dev->hw) {
        if ((6 == pin) || (7 == pin)) {
            //PC6->DP; PC7->DM change to GPIO
            SYSCTRL->SYS_CON1 &= ~ BIT(18); //USB20_PHY reset
            __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
            SYSCTRL->USB20_PHY_CFG3 |= (0xf << 0);
            __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
            SYSCTRL->SYS_CON1 |= BIT(18);
        }
    }

    if (hw) {
        hw->AIOEN &= ~(1 << pin);
    }

    if (0xF000 != (0xF000 & iomap_in_func_sel)) {
        
        //Config IOMAP_INPUT
        if (GPIOA_BASE == (uint32_t)dev->hw) {
            pin_num_temp = pin_num_temp + 0x00;
        } else if (GPIOB_BASE == (uint32_t)dev->hw) {
            pin_num_temp = pin_num_temp + 0x10;
        } else if (GPIOC_BASE == (uint32_t)dev->hw) {
            pin_num_temp = pin_num_temp + 0x20;
        } else if (GPIOD_BASE == (uint32_t)dev->hw) {
            pin_num_temp = pin_num_temp + 0x30;
        } else  if (GPIOE_BASE == (uint32_t)dev->hw) {
            pin_num_temp = pin_num_temp + 0x40;
        } else if ((!dev->hw) && (!pin)) {
            pin_num_temp = INMAP_CONST_1;
        } else if ((!dev->hw) && ( pin)) {
            pin_num_temp = INMAP_CONST_0;
        }
        
        operate_ptr  = operate_ptr + (iomap_in_func_sel >> 2);
        *operate_ptr = (*operate_ptr & ~(0xFF << pin_pos)) | pin_num_temp << pin_pos;
    }

    //Config input mode
    if (hw) {
        hw->MODE = hw->MODE & ~(0x03 << (pin * 2));
    }

    sysctrl_lock();

    return ret_val;

}

static int32 hggpio_v4_iomap_inout(struct gpio_device *gpio, uint32 pin, enum gpio_iomap_in_func iomap_in_func_sel, enum gpio_iomap_out_func iomap_out_func_sel) {

    if (RET_OK != hggpio_v4_iomap_input(gpio,  pin,  iomap_in_func_sel)) {
        return RET_ERR;
    }
    if (RET_OK != hggpio_v4_iomap_output(gpio, pin,  iomap_out_func_sel)) {
        return RET_ERR;
    }

    return RET_OK;
}

static int32 hggpio_v4_dir(struct gpio_device *gpio,uint32 pin, enum gpio_pin_direction direction)
{
    if (GPIO_DIR_INPUT != direction && GPIO_DIR_OUTPUT != direction) {
        return -EINVAL;
    }

    if (GPIO_DIR_INPUT == direction) {
        if (RET_OK != hggpio_v4_iomap_input(gpio,  pin, GPIO_IOMAP_INPUT)) {
            return RET_ERR;
        }
    } else {
        if (RET_OK != hggpio_v4_iomap_output(gpio, pin, GPIO_IOMAP_OUTPUT)) {
            return RET_ERR;
        }
    }

    return RET_OK;
}

static int32_t hggpio_v4_somap_config(struct gpio_device *gpio, uint32_t pin, enum smap_node node, enum smap_output smap)
{	
	uint16_t iomap[4] = {
		GPIO_IOMAP_OUT_SPARE0,
		GPIO_IOMAP_OUT_SPARE1,
		GPIO_IOMAP_OUT_SPARE2,
		GPIO_IOMAP_OUT_SPARE3,
	};
    hggpio_v4_iomap_output(gpio, pin, iomap[node]);

	sysctrl_unlock();
	SYSCTRL->SPAREIO_CON &= ~(0xff << (node<<3));
	SYSCTRL->SPAREIO_CON |= ((176-smap)<< (node<<3));
    sysctrl_lock();
    //同步
    delay_us(10);
	return 0;
}

static int32_t hggpio_v4_simap_config(struct gpio_device *gpio, uint32_t pin, enum smap_node node, enum smap_output smap)
{
	volatile uint32_t *smap_icon = &SYSCTRL->SPARE_I_CON0;
	node -= 4;
	
    //0-2 刚好 是3个 SPARE IN MAP
	hggpio_v4_iomap_input(gpio, pin, node);	

    sysctrl_unlock();
	smap_icon += (smap/16);
	*smap_icon = (node+1) << (2*(smap%16));
    sysctrl_lock();
    //同步
    delay_us(10);
	return 0;
}

#if HGGPIO_V4_DIR_ATOMIC_EN
static int32 hggpio_v4_dir_atomic(struct gpio_device *gpio,uint32 pin, enum gpio_pin_direction direction)
{
#if defined(__CORTEX_M) && ((__CORTEX_M == 0x03) || (__CORTEX_M == 0x04))
    if (RET_OK != hggpio_v4_dir(gpio, pin, direction)) {
        return RET_ERR;
    }
#else
    uint32 flags = disable_irq();
    if (RET_OK != hggpio_v4_dir(gpio, pin, direction)) {
        enable_irq(flags);
        return RET_ERR;
    }
    enable_irq(flags);
#endif
    return RET_OK;
}
#endif

static int32 hggpio_v4_mode(struct gpio_device *gpio, uint32 pin, enum gpio_pin_mode mode, enum gpio_pull_level level)
{
    struct hggpio_v4 *dev   = (struct hggpio_v4 *)gpio;
    struct hggpio_v4_hw *hw = (struct hggpio_v4_hw *)dev->hw;

    if (pin < dev->pin_num[0] || pin > dev->pin_num[1]) {
        return -EINVAL;
    }

    pin = hggpio_v4_pin_num(dev, pin);

    sysctrl_unlock();

    if (GPIOC_BASE == dev->hw) {
        if ((6 == pin) || (7 == pin)) {
            //PC6->DP; PC7->DM change to GPIO
            SYSCTRL->SYS_CON1 &= ~ BIT(18); //USB20_PHY reset
            __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
            SYSCTRL->USB20_PHY_CFG3 |= (0xf << 0);
            __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
            SYSCTRL->SYS_CON1 |= BIT(18);
        }
    }

    sysctrl_lock();

    
    
    return hggpio_v4_set_mode(hw, pin, mode, level);
}

#if HGGPIO_V4_DRIVER_STRENGTH_EN
static int32 hggpio_v4_driver_strength(struct gpio_device *gpio, uint32 pin, enum pin_driver_strength strength)
{
    struct hggpio_v4 *dev   = (struct hggpio_v4 *)gpio;
    struct hggpio_v4_hw *hw = (struct hggpio_v4_hw *)dev->hw;

    if (pin < dev->pin_num[0] || pin > dev->pin_num[1]) {
        return -EINVAL;
    }

    pin = hggpio_v4_pin_num(dev, pin);
    return hggpio_v4_set_driver_strength(hw, pin, strength);
}
#endif

static int32 hggpio_v4_get(struct gpio_device *gpio, uint32 pin)
{
    struct hggpio_v4 *dev = (struct hggpio_v4 *)gpio;
    struct hggpio_v4_hw *hw = (struct hggpio_v4_hw *)dev->hw;

    if (pin < dev->pin_num[0] || pin > dev->pin_num[1]) {
        return -EINVAL;
    }

    pin = hggpio_v4_pin_num(dev, pin);

    sysctrl_unlock();

    if (GPIOC_BASE == dev->hw) {
        if ((6 == pin) || (7 == pin)) {
            //PC6->DP; PC7->DM change to GPIO
            SYSCTRL->SYS_CON1 &= ~ BIT(18); //USB20_PHY reset
            __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
            SYSCTRL->USB20_PHY_CFG3 |= (0xf << 0);
            __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
            SYSCTRL->SYS_CON1 |= BIT(18);
        }
    }

    sysctrl_lock();

    return (hw->IDAT & (1UL << pin)) ? 1 : 0;
}

static int32 hggpio_v4_set(struct gpio_device *gpio, uint32 pin, uint32 val)
{
    struct hggpio_v4 *dev   = (struct hggpio_v4 *)gpio;
    struct hggpio_v4_hw *hw = (struct hggpio_v4_hw *)dev->hw;

    if (pin < dev->pin_num[0] || pin > dev->pin_num[1]) {
        return -EINVAL;
    }
    pin = hggpio_v4_pin_num(dev, pin);

    sysctrl_unlock();

    if (GPIOC_BASE == dev->hw) {
        if ((6 == pin) || (7 == pin)) {
            //PC6->DP; PC7->DM change to GPIO
            SYSCTRL->SYS_CON1 &= ~ BIT(18); //USB20_PHY reset
            __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
            SYSCTRL->USB20_PHY_CFG3 |= (0xf << 0);
            __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
            SYSCTRL->SYS_CON1 |= BIT(18);
        }
    }

    sysctrl_lock();

    if (val) {
        hw->ODAT |=  (1UL << pin);
    } else {
        hw->ODAT &= ~(1UL << pin);
    }

    return RET_OK;
}

int32 hggpio_v4_set_ieen(struct gpio_device *gpio, uint32 pin, int32 val)
{
    struct hggpio_v4 *dev   = (struct hggpio_v4 *)gpio;
    struct hggpio_v4_hw *hw = (struct hggpio_v4_hw *)dev->hw;

    if (pin < dev->pin_num[0] || pin > dev->pin_num[1]) {
        return -EINVAL;
    }
    pin = hggpio_v4_pin_num(dev, pin);

    if (val) {
        hw->IEEN |=  (1UL << pin);
    } else {
        hw->IEEN &= ~(1UL << pin);
    }

    return RET_OK;
}

#if HGGPIO_V4_LOCK_EN
static int32 hggpio_v4_lock(struct gpio_device *gpio, uint32 pin)
{
    struct hggpio_v4 *dev   = (struct hggpio_v4 *)gpio;
    struct hggpio_v4_hw *hw = (struct hggpio_v4_hw *)dev->hw;
    uint32 tmp = 0x00010000;

    if (pin < dev->pin_num[0] || pin > dev->pin_num[1]) {
        return -EINVAL;
    }
    pin = hggpio_v4_pin_num(dev, pin);

    tmp |= (1UL << pin);
    /* Set LCKK bit */
    hw->LCK = tmp;
    /* Reset LCKK bit */
    hw->LCK = (1UL << pin);
    /* Set LCKK bit */
    hw->LCK = tmp;
    /* Read LCKK bit*/
    tmp = hw->LCK;
    /* Read LCKK bit*/
    tmp = hw->LCK;
    if (0 == (tmp & 0x00010000)) {
        return RET_ERR;
    }

    return RET_OK;
}
#endif

#if HGGPIO_V4_DEBUNCE_EN
static int32 hggpio_v4_debunce(struct gpio_device *gpio, uint32 pin, int32 enable)
{
    struct hggpio_v4 *dev   = (struct hggpio_v4 *)gpio;
    struct hggpio_v4_hw *hw = (struct hggpio_v4_hw *)dev->hw;

    if (pin < dev->pin_num[0] || pin > dev->pin_num[1]) {
        return -EINVAL;
    }
    pin = hggpio_v4_pin_num(dev, pin);
    if (enable) {
        hw->DEBEN |= (1UL << pin);
    } else {
        hw->DEBEN &= ~(1UL << pin);
    }

    return RET_OK;
}
#endif

#if HGGPIO_V4_TOGGLE_EN
static int32 hggpio_v4_toggle(struct gpio_device *gpio, uint32 pin)
{
    struct hggpio_v4 *dev   = (struct hggpio_v4 *)gpio;
    struct hggpio_v4_hw *hw = (struct hggpio_v4_hw *)dev->hw;

    if (pin < dev->pin_num[0] || pin > dev->pin_num[1]) {
        return -EINVAL;
    }
    pin = hggpio_v4_pin_num(dev, pin);

    hw->TGL = (1UL << pin);

    return RET_OK;
}
#endif

#if HGGPIO_V4_SET_ATOMIC_EN
static int32 hggpio_v4_set_atomic(struct gpio_device *gpio, uint32 pin, int32 value)
{
#if defined(__CORTEX_M) && ((__CORTEX_M == 0x03) || (__CORTEX_M == 0x04))
    struct hggpio_v4 *dev   = (struct hggpio_v4 *)gpio;
    struct hggpio_v4_hw *hw = (struct hggpio_v4_hw *)dev->hw;

    if (pin < dev->pin_num[0] || pin > dev->pin_num[1]) {
        return -EINVAL;
    }
    pin = hggpio_v4_pin_num(dev, pin);
    REG_BIT_BAND((uint32)&hw->ODAT, pin) = value;
#else
    uint32 flags = disable_irq();
    hggpio_v4_set(gpio, pin, value);
    enable_irq(flags);
#endif
    return RET_OK;
}
#endif

/**********************************************************************************/
/*******************     GPIO EXTERNAL FUNCTIONAL     *****************************/
/**********************************************************************************/
#if HGGPIO_V4_ANALOG_EN
static int32 hggpio_v4_analog(struct gpio_device *gpio, uint32 pin, int32 enable, int32 aioen)
{
    struct hggpio_v4 *dev = (struct hggpio_v4 *)gpio;
    struct hggpio_v4_hw *hw = (struct hggpio_v4_hw *)dev->hw;

    if (pin < dev->pin_num[0] || pin > dev->pin_num[1]) {
        return -EINVAL;
    }

    pin = hggpio_v4_pin_num(dev, pin);
    
    sysctrl_unlock();
    if (enable) {
        hw->MODE = (hw->MODE &~ (0x3 << pin*2)) | (0x3 << pin*2);
    } else {
        hw->MODE = (hw->MODE & ~(3UL << (pin*2)));
    }

    if (aioen) {
        hw->AIOEN |= (1UL << (pin));
    } else {
        hw->AIOEN &= ~(1UL << (pin));
    }
 
    sysctrl_lock();
    
    return RET_OK;
}
#endif

#if HGGPIO_V4_INPUT_LAG_EN
static int32 hggpio_v4_input_lag(struct gpio_device *gpio, uint32 pin, int32 enable)
{
    struct hggpio_v4 *dev = (struct hggpio_v4 *)gpio;
    struct hggpio_v4_hw *hw = (struct hggpio_v4_hw *)dev->hw;

    if (pin < dev->pin_num[0] || pin > dev->pin_num[1]) {
        return -EINVAL;
    }

    pin = hggpio_v4_pin_num(dev, pin);

    sysctrl_unlock();
    if (enable) {
        hw->HY |=  (1UL << pin);
    } else {
        hw->HY &= ~(1UL << pin);
    }
    sysctrl_lock();
    
    return RET_OK;
}
#endif

static int32 hggpio_v4_afio(struct gpio_device *gpio, uint32 pin, enum gpio_afio_set afio)
{
    uint32 pin_loc;
    struct hggpio_v4 *dev = (struct hggpio_v4 *)gpio;
    struct hggpio_v4_hw *hw = (struct hggpio_v4_hw *)dev->hw;

    if (pin < dev->pin_num[0] || pin > dev->pin_num[1]) {
        return -EINVAL;
    }
    pin = hggpio_v4_pin_num(dev, pin);
    
    pin_loc = (pin & 0x7) * 4;
    
    sysctrl_unlock();
    
    hw->AIOEN &= ~(1UL << (pin));

    /* set alternate function mode */
    hw->MODE = (hw->MODE & ~(3UL << (pin*2))) | (2UL << (pin*2));

    if (pin < 8) {
        hw->AFRL = (hw->AFRL & ~(0xFUL << (pin_loc))) | (afio << (pin_loc));
    } else {
        hw->AFRH = (hw->AFRH & ~(0xFUL << (pin_loc))) | (afio << (pin_loc));
    }

    sysctrl_lock();
    
    return RET_OK;
}

static int32 hggpio_v4_int(struct gpio_device *gpio, uint32 pin, enum gpio_irq_event evt)
{
    uint32 pin_bit = 0;
    int32  ret_val = RET_OK;
    struct hggpio_v4 *dev = (struct hggpio_v4 *)gpio;
    struct hggpio_v4_hw *hw = (struct hggpio_v4_hw *)dev->hw;

    if (pin > HGGPIO_V4_MAX_PINS) {
        return -EINVAL;
    }

    pin_bit = (1 << pin);

    sysctrl_unlock();

    switch(evt) {
        case (GPIO_IRQ_EVENT_NONE): {
            hw->IMK  &= ~pin_bit;
            hw->TRG0 = (hw->TRG0 &~ ((0x3) << pin*2)) | ((0x3) << pin*2);
        };break;
        case (GPIO_IRQ_EVENT_ALL): {
            hw->IMK  |=  pin_bit;
            hw->TRG0 = (hw->TRG0 &~ ((0x3) << pin*2)) | ((0x0) << pin*2);
        };break;
        case (GPIO_IRQ_EVENT_RISE): {
            hw->IMK  |=  pin_bit;
            hw->TRG0 = (hw->TRG0 &~ ((0x3) << pin*2)) | ((0x2) << pin*2);
        };break;
        case (GPIO_IRQ_EVENT_FALL): {
            hw->IMK  |=  pin_bit;
            hw->TRG0 = (hw->TRG0 &~ ((0x3) << pin*2)) | ((0x1) << pin*2);
        };break;
        default: {
            ret_val = -ENOTSUPP;
            break;
        }
    }

    sysctrl_lock();

    return RET_OK;
}

#if HGGPIO_V4_ADC_ANALOG_INPUT_EN
static int32 hggpio_v4_adc_analog_input(struct gpio_device *gpio, uint32 pin, int32 value) {

    uint32 pin_bit = 0;
    struct hggpio_v4 *dev = (struct hggpio_v4*)gpio;
    struct hggpio_v4_hw *hw = (struct hggpio_v4_hw *)dev->hw;

    if (pin < dev->pin_num[0] || pin > dev->pin_num[1]) {
        return -EINVAL;
    }

    pin = hggpio_v4_pin_num(dev, pin);
    pin_bit = (1 << pin);

    sysctrl_unlock();

    if (value) {
        hw->MODE   = (hw->MODE &~ (0x3 << pin*2)) | (0x3 << pin*2);
        hw->AIOEN &= ~pin_bit;
        hw->ADC_AIOEN |= pin_bit;
    } else {
        hw->ADC_AIOEN &= ~pin_bit;
        hw->MODE = (hw->MODE &~ (0x3 << pin*2));
    }

    sysctrl_lock();

    return RET_OK;

}
#endif

#if HGGPIO_V4_TK_ANALOG_INPUT_EN
static int32 hggpio_v4_tk_analog_input(struct gpio_device *gpio, uint32 pin, int32 value) {

    uint32 pin_bit = 0;
    struct hggpio_v4 *dev = (struct hggpio_v4*)gpio;
    struct hggpio_v4_hw *hw = (struct hggpio_v4_hw *)dev->hw;

    if (pin < dev->pin_num[0] || pin > dev->pin_num[1]) {
        return -EINVAL;
    }

    pin = hggpio_v4_pin_num(dev, pin);
    pin_bit = (1 << pin);

    sysctrl_unlock();

    if (value) {
        hw->MODE   = (hw->MODE &~ (0x3 << pin*2)) | (0x3 << pin*2);
        //hw->AIOEN &= ~pin_bit;
        //hw->TK_AIOEN |= pin_bit;
    } else {
        //hw->TK_AIOEN &= ~pin_bit;
        hw->MODE = (hw->MODE &~ (0x3 << pin*2));
    }

    sysctrl_lock();

    return RET_OK;
}
#endif

static int32 hggpio_v4_ioctl(struct gpio_device *gpio, uint32 pin, uint32 cmd, uint32 param1, uint32 param2)
{
    int32 ret_val = RET_OK;
    
    switch (cmd) {
        #if HGGPIO_V4_INPUT_LAG_EN
            case GPIO_INPUT_DELAY_ON_OFF:
                ret_val = hggpio_v4_input_lag(gpio, pin, param1);
                break;
        #endif
        
        #if HGGPIO_V4_LOCK_EN
            case GPIO_LOCK:
                ret_val = hggpio_v4_lock(gpio, pin);
                break;
        #endif
        
        #if HGGPIO_V4_DEBUNCE_EN
            case GPIO_DEBUNCE:
                ret_val = hggpio_v4_debunce(gpio, pin, param1);
                break;
        #endif
        
        #if HGGPIO_V4_TOGGLE_EN
            case GPIO_OUTPUT_TOGGLE:
                ret_val = hggpio_v4_toggle(gpio, pin);
                break;
        #endif

        #if HGGPIO_V4_DIR_ATOMIC_EN
            case GPIO_DIR_ATOMIC:
                ret_val = hggpio_v4_dir_atomic(gpio, pin, param1);
                break;
        #endif

        #if HGGPIO_V4_SET_ATOMIC_EN
            case GPIO_VALUE_ATOMIC:
                ret_val = hggpio_v4_set_atomic(gpio, pin, param1);
                break;
        #endif

        #if HGGPIO_V4_ADC_ANALOG_INPUT_EN
            case GPIO_CMD_ADC_ANALOG:
                ret_val = hggpio_v4_adc_analog_input(gpio, pin, param1);
                break;
        #endif

        #if HGGPIO_V4_TK_ANALOG_INPUT_EN
            case GPIO_CMD_TK_ANALOG:
                ret_val = hggpio_v4_tk_analog_input(gpio, pin, param1);
                break;
        #endif

            case GPIO_CMD_DRIVER_STRENGTH:
                ret_val = hggpio_v4_driver_strength(gpio, pin, param1);
                break;

            case GPIO_CMD_AFIO_SET:
                ret_val = hggpio_v4_afio(gpio, pin, param1);
                break;

            case GPIO_CMD_IOMAP_OUT_FUNC:
                ret_val = hggpio_v4_iomap_output(gpio, pin, param1);
                break;

            case GPIO_CMD_IOMAP_IN_FUNC:
                ret_val = hggpio_v4_iomap_input(gpio, pin, param1);
                break;

            case GPIO_CMD_IOMAP_INOUT_FUNC:
                ret_val = hggpio_v4_iomap_inout(gpio, pin, param1, param2);
                break;

            case GPIO_GENERAL_ANALOG:
                ret_val = hggpio_v4_analog(gpio, pin, param1, 1);
                break;
            
            case GPIO_AIODIS_ANALOG:
                ret_val = hggpio_v4_analog(gpio, pin, param1, 0);
                break;

            case GPIO_CMD_SET_IEEN:
                ret_val = hggpio_v4_set_ieen(gpio, pin, param1);
				break;
#ifdef TXW82X
            case GPIO_GET_OUTMAP:
               ret_val = hggpio_v4_get_omap(gpio, pin);
               break;
            
            case GPIO_GET_INMAP:
               ret_val = hggpio_v4_get_imap(gpio, pin);
               break;

            case GPIO_SET_SMAP:
                if(param1 < SMAP_IN_1) {
                    ret_val = hggpio_v4_somap_config(gpio, pin, param1, param2);
                } else {
                    ret_val = hggpio_v4_simap_config(gpio, pin, param1, param2);
                }
            
            case GPIO_GET_OUTMAP_PIN:
                ret_val = hggpio_v4_get_omap_pin(gpio, param1);
                break;
#endif

        default:
            ret_val = -ENOTSUPP;
            break;
    }
    return ret_val;
}

/**********************************************************************************/
/*******************     GPIO    IRQ   FUNCTIONAL     *****************************/
/**********************************************************************************/
static void hggpio_v4_irq_handler(void *data)
{
    int32 i = 0;
    struct hggpio_v4 *dev = (struct hggpio_v4 *)data;
    struct hggpio_v4_hw *hw = (struct hggpio_v4_hw *)dev->hw;
    enum gpio_irq_event evt = GPIO_IRQ_EVENT_NONE;
    uint32 pending  = hw->PND;
    uint32 trg0     = hw->TRG0;
    hw->PNDCLR = 0xFFFFFFFF;

    for (i = 0; i < HGGPIO_V4_MAX_PINS; i++) {
        if (pending & BIT(i)) {
            if ((trg0&(0x3 << i)) == (0x1)) {
                evt = GPIO_IRQ_EVENT_FALL;
            } else if ((trg0&(0x3 << i)) == (0x2)) {
                evt = GPIO_IRQ_EVENT_RISE;
            } else {
                evt = (hw->IDAT & BIT(i)) ? GPIO_IRQ_EVENT_RISE : GPIO_IRQ_EVENT_FALL;
            }
            if (dev->irq_hdl[i]) {
                dev->irq_hdl[i](dev->pin_id[i], evt, 0, 0);
            }
        }
    }
}

static int32 hggpio_v4_request_pin_irq(struct gpio_device *gpio, uint32 pin, gpio_irq_hdl handler, uint32 data, enum gpio_irq_event evt)
{
    int32 ret = RET_OK;
    struct hggpio_v4 *dev = (struct hggpio_v4 *)gpio;

    if (pin < dev->pin_num[0] || pin > dev->pin_num[1]) {
        return -EINVAL;
    }

    pin = hggpio_v4_pin_num(dev, pin);
    dev->irq_hdl[pin] = handler;
    dev->pin_id[pin]  = data;
    ret = request_irq(dev->irq_num, hggpio_v4_irq_handler, (void *)dev);
    ASSERT(ret == RET_OK);
    hggpio_v4_int(gpio, pin, evt);
    irq_enable(dev->irq_num);
    dev->flag |= GPIO_FLAG_IS_INTR_EN;
    return RET_OK;
}

static int32 hggpio_v4_release_pin_irq(struct gpio_device *gpio, uint32 pin, enum gpio_irq_event evt)
{
    struct hggpio_v4 *dev = (struct hggpio_v4 *)gpio;

    if (pin < dev->pin_num[0] || pin > dev->pin_num[1]) {
        return -EINVAL;
    }
    pin = hggpio_v4_pin_num(dev, pin);
    dev->irq_hdl[pin] = NULL;
    dev->pin_id[pin]  = 0;
    return hggpio_v4_int(gpio, pin, 0);
}

static int32 hggpio_v4_enable_irq(struct gpio_device *gpio, uint32 pin, uint32 enable)
{
    struct hggpio_v4 *dev = (struct hggpio_v4 *)gpio;

    if (pin < dev->pin_num[0] || pin > dev->pin_num[1]) {
        return -EINVAL;
    }

    pin = hggpio_v4_pin_num(dev, pin);
    if (enable) {
        irq_enable(dev->irq_num);
    } else {
        irq_disable(dev->irq_num);
    }
    return RET_OK;
}



#ifdef CONFIG_SLEEP
int32 hggpio_v4_suspend(struct hggpio_v4 *gpio)
{
    struct hggpio_v4_hw *hw = (struct hggpio_v4_hw *)gpio->hw;

    gpio->bk.MODE          = hw->MODE;         
    gpio->bk.OTYPE         = hw->OTYPE;      
    gpio->bk.OSPEEDL       = hw->OSPEEDL;      
    gpio->bk.OSPEEDH       = hw->OSPEEDH;      
    gpio->bk.PUPL          = hw->PUPL;         
    gpio->bk.PUPH          = hw->PUPH;         
    gpio->bk.PUDL          = hw->PUDL;         
    gpio->bk.PUDH          = hw->PUDH;               
    gpio->bk.ODAT          = hw->ODAT;         
    gpio->bk.AFRL          = hw->AFRL;         
    gpio->bk.AFRH          = hw->AFRH;         
    gpio->bk.IMK           = hw->IMK;          
    gpio->bk.DEBEN         = hw->DEBEN;        
    gpio->bk.AIOEN         = hw->AIOEN;        
    gpio->bk.TRG0          = hw->TRG0;        
    gpio->bk.IEEN          = hw->IEEN;         
    gpio->bk.IOFUNCOUTCON0 = hw->IOFUNCOUTCON0;
    gpio->bk.IOFUNCOUTCON1 = hw->IOFUNCOUTCON1;
    gpio->bk.IOFUNCOUTCON2 = hw->IOFUNCOUTCON2;
    gpio->bk.IOFUNCOUTCON3 = hw->IOFUNCOUTCON3;  

    gpio->flag |= GPIO_FLAG_IS_SUSPENDED;

    //irq_disable(gpio->comm_irq_num);

    memcpy(gpio_sys.iofuncin, (void*)&SYSCTRL->IOFUNCINCON0, 15*4);
#if defined(TXW82X)
    gpio_sys.iomask[0] = SYSCTRL->IOFUNCMASK0;
    gpio_sys.iomask[1] = SYSCTRL->IOFUNCMASK1;
    gpio_sys.iomask[2] = SYSCTRL->IOFUNCMASK2;
    gpio_sys.iomask[3] = SYSCTRL->IOFUNCMASK3;
    gpio_sys.iomask[4] = SYSCTRL->IOFUNCMASK4;
    gpio_sys.iomask[5] = SYSCTRL->IOFUNCMASK5;
    gpio_sys.iomask[6] = SYSCTRL->IOFUNCMASK6;
    gpio_sys.iomask[7] = SYSCTRL->IOFUNCMASK7;
    gpio_sys.iomask[8] = SYSCTRL->IOFUNCMASK8;
#else
    gpio_sys.iomask[0] = SYSCTRL->IOFUNCMASK0;
    gpio_sys.iomask[1] = SYSCTRL->IOFUNCMASK1;
#endif

    gpio_sys.flag |= SYSCTRL_FLAG_IS_SUSPEND;

    if(SYSCTRL->USB20_PHY_CFG3 & 0x8) {
        gpio_sys.flag |= SYSCTRL_FLAG_IS_USB2GPIO;
    } else {
        gpio_sys.flag &= ~SYSCTRL_FLAG_IS_USB2GPIO;
    }
    
#if GPIO_ENBUG_EN
    uint32_t *p32;
    if(hw == GPIOE_BASE) {
        p32 = &SYSCTRL->IOFUNCINCON0;
        os_printf("IOFUNCIN:\r\n");
        for(int i = 0;i<15;i++)
        {
            os_printf("0x%x\r\n", p32[i]);
        }
        os_printf("\r\n");
        os_printf("IOFUNCMASK: [0x%x, 0x%x]\r\n", SYSCTRL->IOFUNCMASK0, SYSCTRL->IOFUNCMASK1);
    }

    os_printf("*********gpio_suspend: 0x%x\r\n", hw);

    p32 = (void*)hw;
    for(int j = 0;j<0x20+1;j+=4)
    {
        os_printf("0x%08x  0x%08x  0x%08x  0x%08x\r\n", 
        p32[j], p32[j+1], p32[j+2], p32[j+3]);
    }
#endif

    return 0;
}

int32 hggpio_v4_resume(struct hggpio_v4 *gpio)
{
    struct hggpio_v4_hw *hw = (struct hggpio_v4_hw *)gpio->hw;
    if(gpio->flag & GPIO_FLAG_IS_SUSPENDED) {
        hw->MODE          = gpio->bk.MODE;         
        hw->OTYPE         = gpio->bk.OTYPE;      
        hw->OSPEEDL       = gpio->bk.OSPEEDL;      
        hw->OSPEEDH       = gpio->bk.OSPEEDH;      
        hw->PUPL          = gpio->bk.PUPL;         
        hw->PUPH          = gpio->bk.PUPH;         
        hw->PUDL          = gpio->bk.PUDL;         
        hw->PUDH          = gpio->bk.PUDH;              
        hw->ODAT          = gpio->bk.ODAT;               
        hw->AFRL          = gpio->bk.AFRL;         
        hw->AFRH          = gpio->bk.AFRH;                 
        hw->IMK           = gpio->bk.IMK;          
        hw->DEBEN         = gpio->bk.DEBEN;        
        hw->AIOEN         = gpio->bk.AIOEN;        
        hw->TRG0          = gpio->bk.TRG0;        
        hw->IEEN          = gpio->bk.IEEN;         
        hw->IOFUNCOUTCON0 = gpio->bk.IOFUNCOUTCON0;
        hw->IOFUNCOUTCON1 = gpio->bk.IOFUNCOUTCON1;
        hw->IOFUNCOUTCON2 = gpio->bk.IOFUNCOUTCON2;
        hw->IOFUNCOUTCON3 = gpio->bk.IOFUNCOUTCON3;   
        gpio->flag &= ~GPIO_FLAG_IS_SUSPENDED;

        //irq_enable(gpio->comm_irq_num);
    } 

    if(gpio_sys.flag & SYSCTRL_FLAG_IS_SUSPEND) {
        sysctrl_unlock();
        memcpy((void*)&SYSCTRL->IOFUNCINCON0, gpio_sys.iofuncin, 15*4);
        #if defined(TXW82X)
            SYSCTRL->IOFUNCMASK0 = gpio_sys.iomask[0];
            SYSCTRL->IOFUNCMASK1 = gpio_sys.iomask[1];
            SYSCTRL->IOFUNCMASK2 = gpio_sys.iomask[2];
            SYSCTRL->IOFUNCMASK3 = gpio_sys.iomask[3];
            SYSCTRL->IOFUNCMASK4 = gpio_sys.iomask[4];
            SYSCTRL->IOFUNCMASK5 = gpio_sys.iomask[5];
            SYSCTRL->IOFUNCMASK6 = gpio_sys.iomask[6];
            SYSCTRL->IOFUNCMASK7 = gpio_sys.iomask[7];
            SYSCTRL->IOFUNCMASK8 = gpio_sys.iomask[8];
        #else
            SYSCTRL->IOFUNCMASK0 = gpio_sys.iomask[0];
            SYSCTRL->IOFUNCMASK1 = gpio_sys.iomask[1];
        #endif
        if(gpio_sys.flag & SYSCTRL_FLAG_IS_USB2GPIO) {
            //PC6->DP; PC7->DM change to GPIO
            SYSCTRL->SYS_CON1 &= ~ BIT(18); //USB20_PHY reset
            __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
            SYSCTRL->USB20_PHY_CFG3 |= (0xf << 0);
            __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
            SYSCTRL->SYS_CON1 |= BIT(18);
        }
        sysctrl_lock();
        gpio_sys.flag &= ~SYSCTRL_FLAG_IS_SUSPEND;
    }


#if GPIO_ENBUG_EN
    uint32_t *p32;
    if(hw == (void*)GPIOE_BASE) {
        p32 = (void*)&SYSCTRL->IOFUNCINCON0;
        os_printf("IOFUNCIN:\r\n");
        for(int i = 0;i<15;i++)
        {
            os_printf("0x%x\r\n", p32[i]);
        }
        os_printf("\r\n");
        os_printf("IOFUNCMASK: [0x%x, 0x%x]\r\n", SYSCTRL->IOFUNCMASK0, SYSCTRL->IOFUNCMASK1);
    }

    os_printf("*********gpio_resume 0x%x\r\n", hw);
    p32 = (void*)hw;
    for(int j = 0;j<0x20+1;j+=4)
    {
        os_printf("0x%08x  0x%08x  0x%08x  0x%08x\r\n", 
        p32[j], p32[j+1], p32[j+2], p32[j+3]);
    }
#endif

    return 0;
}
#else
int32_t hggpio_v4_suspend(struct hggpio_v4 *gpio)
{
    return 0;
}

int32_t hggpio_v4_resume(struct hggpio_v4 *gpio)
{
    return 0;
}
#endif

static const struct gpio_hal_ops gpio_v4_ops = {
    .mode            = hggpio_v4_mode,
    .dir             = hggpio_v4_dir,
    .set             = hggpio_v4_set,
    .get             = hggpio_v4_get,
    .ioctl           = hggpio_v4_ioctl,
    .request_pin_irq = hggpio_v4_request_pin_irq,
    .release_pin_irq = hggpio_v4_release_pin_irq,
#ifdef CONFIG_SLEEP
    .ops.suspend = hggpio_v4_suspend,
    .ops.resume  = hggpio_v4_resume,
#endif
};

int32 hggpio_v4_attach(uint32 dev_id, struct hggpio_v4 *gpio)
{
    int32 i = 0;
//    struct hggpio_v4 *dev     = (struct hggpio_v4*)gpio;
//    struct hggpio_v4_hw *hw   = (struct hggpio_v4_hw *)dev->hw;

    //V2 IOFUNCOUTCON0-3无复位值，故全写0       ----- !初始值的初始化迁移到其他地方了
//    memset((void *)&hw->IOFUNCOUTCON0, 0, sizeof(hw->IOFUNCOUTCON0)*4);
    
    gpio->dev.dev.ops = (const struct devobj_ops *)&gpio_v4_ops;

    for (i = 0; i < HGGPIO_V4_MAX_PINS; i++) {
        gpio->irq_hdl[i] = NULL;
    }
    irq_disable(gpio->irq_num);
    dev_register(dev_id, (struct dev_obj *)gpio);
    
    return RET_OK;
}




