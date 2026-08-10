#ifndef _HGGPIO_V4_HW_H_
#define _HGGPIO_V4_HW_H_

#ifdef __cplusplus
extern "C" {
#endif


/* Switches for uncommon functions */
#define HGGPIO_V4_DIR_ATOMIC_EN       (1)
#define HGGPIO_V4_DRIVER_STRENGTH_EN  (1)
#define HGGPIO_V4_DEBUNCE_EN          (1)
#define HGGPIO_V4_TOGGLE_EN           (1)
#define HGGPIO_V4_SET_ATOMIC_EN       (1)
#define HGGPIO_V4_ANALOG_EN           (1)
#define HGGPIO_V4_LOCK_EN             (0)
#define HGGPIO_V4_INPUT_LAG_EN        (0)
#define HGGPIO_V4_ADC_ANALOG_INPUT_EN (0)
#define HGGPIO_V4_TK_ANALOG_INPUT_EN  (0)


/**
  * @breif huge-ic gpio register definition
  */
struct hggpio_v4_hw {
    __IO uint32_t MODE;
    __IO uint32_t OTYPE;
    __IO uint32_t OSPEEDL;
    __IO uint32_t OSPEEDH;
    __IO uint32_t PUPL;
    __IO uint32_t PUPH;
    __IO uint32_t PUDL;
    __IO uint32_t PUDH;
    __IO uint32_t IDAT;
    __IO uint32_t ODAT;
    __IO uint32_t BSR;
    __IO uint32_t RES0;
    __IO uint32_t AFRL;
    __IO uint32_t AFRH;
    __IO uint32_t TGL;
    __IO uint32_t IMK;
    __IO uint32_t HY;
    __IO uint32_t RES2;
    __IO uint32_t RES3;
    __IO uint32_t DEBEN;
    __IO uint32_t AIOEN;
    __IO uint32_t PND;
    __IO uint32_t PNDCLR;
    __IO uint32_t TRG0;
    __IO uint32_t RES4;
    __IO uint32_t RES5;
    __IO uint32_t RES6;
    __IO uint32_t RES7;
    __IO uint32_t IEEN;
    __IO uint32_t IOFUNCOUTCON0;
    __IO uint32_t IOFUNCOUTCON1;
    __IO uint32_t IOFUNCOUTCON2;
    __IO uint32_t IOFUNCOUTCON3;
} ;


#ifdef __cplusplus
}
#endif

#endif /* _HGGPIO_V4_HW_H_ */

