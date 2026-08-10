#ifndef _HGTIMER_V4_HW_H
#define _HGTIMER_V4_HW_H

#ifdef __cplusplus
extern "C" {
#endif


/***** TIMERx CON Register *****/
/*! Timer IR selection whether lsb first send (for TIMER1/2 only)
 */
#define LL_TIMER_CON_IR_TMR_FST_LSB_SEL           (1UL << 29)
/*! Timer IR function logic 0 PWM polarity selection (for TIMER1/2 only)
 */
#define LL_TIMER_CON_IR_ZERO_PWMPOL               (1UL << 28)
/*! Timer IR function logic 1 PWM polarity selection (for TIMER1/2 only)
 */
#define LL_TIMER_CON_IR_ONE_PWMPOL                (1UL << 27)
/*! Timer IR function enable (for TIMER1/2 only)
 */
#define LL_TIMER_CON_IR_EN                        (1UL << 26)
/*! PWM polarity selection in TIMER module
 */
#define LL_TIMER_CON_PWMPOL                       (1UL << 25)
/*! The polarity of capture event 1 is selected: 0 = rising edge, 1 = falling edge.
 */
#define LL_TIMER_CON_CAP1POL(n)                   ((n & 0x1) << 21)
/*! When the capture event 1 occurs, the value of the CNT is automatically cleared.
 */
#define LL_TIMER_CON_CTRRST1                      (1UL << 17)
/*! The number of the Capture register:
 *  00 : Capture data store in CAP1
 *  01 : Capture data store in CAP1 CAP2
 *  10 : Capture data store in CAP1 CAP2 CAP3
 *  11 : Capture data store in CAP1 CAP2 CAP3 CAP4
 */
#define LL_TIMER_CON_CAP_CNT(n)                   (((n)&0x3) << 15)
/*! Capture selection:
 *  00 : GPIO
 *  01 : GPIO OR
 *  10 : compare0 output
 *  11 : compare1 output
 */
#define LL_TIMER_CON_CAP_SEL(n)                   (((n)&0x3) << 13)
/*! Output sync signal selection:
 *  00 : CNT value = PRD value
 *  01 : CNT value = CMP value
 *  10 : Output SYNCI value to SYNCO
 *  11 : PWM output is assigned to SYNCO
 */
#define LL_TIMER_CON_SYNCO_SEL(n)                 (((n)&0x3) << 11)
/*! Synci polarity inversion:
 *  1 : Invert
 *  0 : not reversed
 */
#define LL_TIMER_CON_SYNCI_POL                    (1UL << 10)
/*! Synci function selection
 *  00 : disable
 *  01 : kick start
 *  10 : reset
 *  11 : gating
 */
#define LL_TIMER_CON_SLAVE_MODE(n)                (((n)&0x3) << 8)
/*! Timer prescaler settings:
 *  000 : 0 frequency division
 *  001 : 2 frequency division
 *  010 : 4 frequency division
 *  011 : 8 frequency division
 *  100 : 16 frequency division
 *  101 : 32 frequency division
 *  110 : 64 frequency division
 *  111 : 128 frequency division
 */
#define LL_TIMER_CON_PSC(n)                       (((n)&0x7) << 5)
/*! Timer counter source select bits:
 *  001 : Internal high speed RC
 *  010 : Internal low speed RC
 *  011 : External crystal oscillator divided by 2 clocks
 *  100 : timer inc pin rising
 *  101 : timer inc pin falling
 *  110 : timer inc pin rising and falling
 *  Others : system clock
 */
#define LL_TIMER_CON_INC_SRC_SEL(n)               (((n)&0x7) << 2)
/*! Timer mode select bits:
 *  00 : timer counter mode
 *  01 : timer pwm mode
 *  10 : timer capture mode
 *  Others : reservation
 */
#define LL_TIMER_CON_MODE_SEL(n)                  (((n)&0x3) << 0)


/***** TIMERx EN Register *****/
/*! TMR enable signal, active high.
 */
#define LL_TIMER_EN_TMREN                         (1UL << 0)


/***** TIMERx IE Register *****/
/*! Timer IR TX word done interrupt enable (for TIMER1/2 only)
 */
#define LL_TIMER_IE_IR_TX_WORD_DONE_IE            (1UL << 11)
/*! Timer IR TX done interrupt enable (for TIMER1/2 only)
 */
#define LL_TIMER_IE_IR_TX_DONE_IE                 (1UL << 10)
/*! Dma buffer full interrupt enable.
 */
#define LL_TIMER_IE_DMA_FL_IE                     (1UL << 9)
/*! Dma buffer half full interrupt enable.
 */
#define LL_TIMER_IE_DMA_HF_IE                     (1UL << 8)
/*! The slave mode trigger mode or reset mode interrupt enable.
 */
#define LL_TIMER_IE_SLAVE_IE                      (1UL << 7)
/*! When the CNT value is equal to the CMP value, the interrupt is enabled and is valid only in pwm mode.
 */
#define LL_TIMER_IE_CMP_IE                        (1UL << 6)
/*! When the CNT value is equal to the PRD value, the interrupt is enabled and is valid only in the counter mode/PWM mode.
 */
#define LL_TIMER_IE_PRD_IE                        (1UL << 5)
/*! When the CNT value overflows (16'hffff), the interrupt is enabled.
 */
#define LL_TIMER_IE_OVF_IE                        (1UL << 4)
/*! When the capture event 1 occurs, the interrupt is enabled.
 @note: none this capture event interrupt
 */
#define LL_TIMER_IE_CAP1_IE                       (1UL << 0)


/***** TIMERx CNT Register *****/
/*! Count register.
 */
#define LL_TIMER_CNT(n)                           (((n)&0xFFFFFFFF) << 0)


/***** TIMERx FLG Register *****/
/*! Timer IR TX word done interrupt flag (for TIMER1/2 only)
 */
#define LL_TIMER_IE_IR_TX_WORD_DONE_FLG           (1UL << 11)
/*! Timer IR TX done interrupt flag (for TIMER1/2 only)
 */
#define LL_TIMER_IE_IR_TX_DONE_FLG                (1UL << 10)
/*! Dma buffer full sign.
 */
#define LL_TIMER_IE_DMA_FL_FLG                    (1UL << 9)
/*! Dma buffer half full sign.
 */
#define LL_TIMER_IE_DMA_HF_FLG                    (1UL << 8)
/*! The slave mode flag (reset or trigger only).
 */
#define LL_TIMER_IE_SLAVE_FLG                     (1UL << 7)
/*! The CNT value is equal to the CMP value flag and is valid only in pwm mode.
 */
#define LL_TIMER_IE_CMP_FLG                       (1UL << 6)
/*! The CNT value is equal to the PRD value flag and is valid only in counter mode/PWM mode.
 */
#define LL_TIMER_IE_PRD_FLG                       (1UL << 5)
/*! CNT value overflow (16'hffff) flag.
 */
#define LL_TIMER_IE_OVF_FLG                       (1UL << 4)
/*! Capture event 1 occurs.
 */
#define LL_TIMER_IE_CAP1_FLG                      (1UL << 0)


/***** TIMERx CLR Register *****/
/*! Timer IR TX word done interrupt clear (for TIMER1/2 only)
 */
#define LL_TIMER_IE_IR_TX_WORD_DONE_CLR           (1UL << 11)
/*! Timer IR TX done interrupt clear (for TIMER1/2 only)
 */
#define LL_TIMER_IE_IR_TX_DONE_CLR                (1UL << 10)
/*! Dma buffer full sign clear.
 */
#define LL_TIMER_IE_DMA_FL_CLR                    (1UL << 9)
/*! Dma buffer half full sign clear.
 */
#define LL_TIMER_IE_DMA_HF_CLR                    (1UL << 8)
/*! The slave mode flag (reset or trigger only) clear.
 */
#define LL_TIMER_IE_SLAVE_CLR                     (1UL << 7)
/*! The CNT value is equal to the CMP value flag clear.
 */
#define LL_TIMER_IE_CMP_CLR                       (1UL << 6)
/*! The CNT value is equal to the PRD value flag clear.
 */
#define LL_TIMER_IE_PRD_CLR                       (1UL << 5)
/*! The CNT value overflow (16'hffff) flag is cleared.
 */
#define LL_TIMER_IE_OVF_CLR                       (1UL << 4)
/*! Capture event 1 occurs flag clear.
 */
#define LL_TIMER_IE_CAP1_CLR                      (1UL << 0)


/***** TIMERx CAP1/PR Register *****/
/*! Capture mode        : capture register 1
 *  Timing mode/PWM mode: Count period register
 */
#define LL_TIMER_CMP1_PR(n)                       (((n)&0xFFFFFFFF) << 0)


/***** TIMERx CAP2/CMP Register *****/
/*! Capture mode        : capture register 2
 *  Timing mode/PWM mode: compare register
 */
#define LL_TIMER_CMP2_CMP(n)                      (((n)&0xFFFFFFFF) << 0)


/***** TIMERx CAP3/PR_SD Register *****/
/*! Capture mode        : capture register 3
 *  Timing mode/PWM mode: counting period shadow register
 */
#define LL_TIMER_CMP3_PR_SD(n)                    (((n)&0xFFFFFFFF) << 0)


/***** TIMERx CAP4/CMP_SD Register *****/
/*! Capture mode        : capture register 4
 *  Timing mode/PWM mode: Compare shadow registers
 */
#define LL_TIMER_CMP4_CMP_SD(n)                   (((n)&0xFFFFFFFF) << 0)

/***** TIMERx DCCTL Register *****/
/*! Dma mode selection
 *  0 : Single mode, after the specified dma length is completed, disable TMR.
 *  1 : Loop mode, after the dma length is specified, restart from the start address.
 */
#define LL_TIMER_DMA_LPBK                         (1UL << 1)
/*! Dma enabled.
 */
#define LL_TIMER_DMA_EN                           (1UL << 0)


/***** TIMERx DADR Register *****/
/*! Dma starting address.
 */
#define LL_TIMER_DADR_STADR(n)                    (((n)&0xFFFF) << 0)


/***** TIMERx DLEN Register *****/
/*! Dma buffer length (32bit), if the buffer is n, the configuration is n-1;
 */
#define LL_TIMER_DLEN_LEN(n)                      (((n)&0xFFFF) << 0)


/***** TIMERx DCNT Register *****/
/*! The number of dma data is valid.
 */
#define LL_TIMER_DCNT_CNT(n)                      (((n)&0xFFFF) << 0)


/***** TIMER ALLCON Register *****/
/*! The timer3 sync count value is cleared.
 */
#define LL_TIMER_ALLCON_TMR3_SYNC                 (1UL << 11)
/*! The timer2 sync count value is cleared.
 */
#define LL_TIMER_ALLCON_TMR2_SYNC                 (1UL << 10)
/*! The timer1 sync count value is cleared.
 */
#define LL_TIMER_ALLCON_TMR1_SYNC                 (1UL << 9)
/*! The timer0 sync count value is cleared.
 */
#define LL_TIMER_ALLCON_TMR0_SYNC                 (1UL << 8)
/*! Timer3 starts counting.
 */
#define LL_TIMER_ALLCON_TMR3_KICK                 (1UL << 3)
/*! Timer2 starts counting.
 */
#define LL_TIMER_ALLCON_TMR2_KICK                 (1UL << 2)
/*! Timer1 starts counting.
 */
#define LL_TIMER_ALLCON_TMR1_KICK                 (1UL << 1)
/*! Timer0 starts counting.
 */
#define LL_TIMER_ALLCON_TMR0_KICK                 (1UL << 0)
/*! Configure the trigger path for the sync count of TIMER.
 */
#define LL_TIMER_ALLCON_SYNC_COUNT_ALL(n)         (((n)&0x3F) << 8)
/*! Configure the synchronous trigger path of TIMER.
 */
#define LL_TIMER_ALLCON_KICK_ALL(n)               (((n)&0x3F) << 0)



struct hgtimer_v4_hw {
    __IO uint32_t TMR_CON;
    __IO uint32_t TMR_EN;
    __IO uint32_t TMR_IE;
    __IO uint32_t TMR_CNT;
    __IO uint32_t TMR_FLG;
    __IO uint32_t TMR_CLR;
    __IO uint32_t TMR_CAP1;
    __IO uint32_t TMR_CAP2;
    __IO uint32_t TMR_CAP3;
    __IO uint32_t TMR_CAP4;
    /* The following registers only for timer1 & timer2 */
    __IO uint32_t TMR_DCTL;
    __IO uint32_t TMR_DADR;
    __IO uint32_t TMR_DLEN;
    __IO uint32_t TMR_DCNT;
    __IO uint32_t TMR_IR_BCNT;
};

#ifdef __cplusplus
}
#endif

#endif /* _HGTIMER_V4_HW_H */

