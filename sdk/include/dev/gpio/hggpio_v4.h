#ifndef _HUGEIC_GPIO_V4_H_
#define _HUGEIC_GPIO_V4_H_
#include "hal/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HGGPIO_V4_MAX_PINS            (16)


#ifdef HG_GPIOA_BASE
    #ifndef GPIOA_BASE
        #define GPIOA_BASE HG_GPIOA_BASE
    #endif
#endif

#ifdef HG_GPIOB_BASE
    #ifndef GPIOB_BASE
        #define GPIOB_BASE HG_GPIOB_BASE
    #endif
#endif

#ifdef HG_GPIOC_BASE
    #ifndef GPIOC_BASE
        #define GPIOC_BASE HG_GPIOC_BASE
    #endif
#endif

#ifdef HG_GPIOD_BASE
    #ifndef GPIOD_BASE
        #define GPIOD_BASE HG_GPIOD_BASE
    #endif
#endif

#ifdef HG_GPIOE_BASE
    #ifndef GPIOE_BASE
        #define GPIOE_BASE HG_GPIOE_BASE
    #endif
#endif

#ifdef TXW80X
#define INMAP_CONST_1 0x28
#define INMAP_CONST_0 0x29
#elif defined(TXW81X)
#define INMAP_CONST_1 0x2e
#define INMAP_CONST_0 0x2f
#elif defined(TXW82X)
#define INMAP_CONST_1 0x44
#define INMAP_CONST_0 0x45
#endif

struct gpio_bk{
    __IO uint32_t MODE;
    __IO uint32_t OTYPE;
    __IO uint32_t OSPEEDL;
    __IO uint32_t OSPEEDH;
    __IO uint32_t PUPL;
    __IO uint32_t PUPH;
    __IO uint32_t PUDL;
    __IO uint32_t PUDH;
    __IO uint32_t ODAT;
    __IO uint32_t AFRL;
    __IO uint32_t AFRH;
    __IO uint32_t IMK;
    __IO uint32_t DEBEN;
    __IO uint32_t AIOEN;
    __IO uint32_t TRG0;
    __IO uint32_t IEEN;
    __IO uint32_t IOFUNCOUTCON0;
    __IO uint32_t IOFUNCOUTCON1;
    __IO uint32_t IOFUNCOUTCON2;
    __IO uint32_t IOFUNCOUTCON3;
};

struct hggpio_v4 {
    struct gpio_device dev;
    uint32 hw;
	uint32 comm_irq_num;
    int32 pin_id[HGGPIO_V4_MAX_PINS];
    gpio_irq_hdl irq_hdl[HGGPIO_V4_MAX_PINS];
    uint32 pin_num[2];
    uint8 irq_num;
    uint32_t flag;
#ifdef CONFIG_SLEEP
    struct gpio_bk       bk;
#endif    
};

struct hggpio_v4_hw;

int32 hggpio_v4_attach(uint32 dev_id, struct hggpio_v4 *gpio);
uint32_t hggpio_v4_mask_opa(struct hggpio_v4_hw *reg, uint32_t pin_num, enum gpio_iomap_out_func func_sel, uint32_t flag);

#ifdef __cplusplus
}
#endif


#endif

