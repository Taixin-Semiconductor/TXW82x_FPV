#ifndef _HGADC_V1_HW_H
#define _HGADC_V1_HW_H

#ifdef __cplusplus
    extern "C" {
#endif

#define LL_ADKEY_SOTF_KICK(hw)                      (hw->ADC_CON_KICK |= BIT(0))

#define LL_ADKEY_GET_IRQ_EN_SAMPLE_DONE(hw)         (hw->ADKEY_CON & BIT(20))

#define LL_ADKEY_GET_DONE_PENDING(hw)               (hw->ADKEY_STA & BIT(0))

#define LL_ADKEY_CLEAR_DONE_PENDING(hw)             (hw->ADKEY_STA = BIT(0))

#define LL_ADKEY_GET_DATA(hw)                       (hw->ADKEY_DATA & 0xFFF)


/** @brief ADC register structure
  * @{
  */
struct hgadc_v1_hw {
    __IO uint32 ADC_ANA_CTRL0;
    __IO uint32 ADC_ANA_CTRL1;
    __IO uint32 ADC_CON_EN;
    __IO uint32 ADC_CON_FUN;
    __IO uint32 ADC_CON_SMP;
    __IO uint32 ADC_CON_SEL0;
    __IO uint32 ADC_CON_SEL1;
    __IO uint32 ADC_CON_KICK;
    __IO uint32 ADC_CON_CMP;
    __IO uint32 ADC_CON_STA;
    __IO uint32 ADC_DMA_STADR;
    __IO uint32 ADC_DMA_CNT;
    __IO uint32 ADC_CON_EXTEND0;
    __IO uint32 ADC_CON_EXTEND1;
};



#ifdef __cplusplus
}
#endif

#endif /* _HGADC_V0_HW_H */