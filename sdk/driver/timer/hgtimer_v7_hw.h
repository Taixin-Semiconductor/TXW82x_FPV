#ifndef _HGTIMER_V7_HW_H
#define _HGTIMER_V7_HW_H

#ifdef __cplusplus
extern "C" {
#endif

/***** SINPLE TIMERx CTL Register *****/
/*! Capture selection:
 *  00 : GPIO
 *  01 : GPIO OR
 *  10 : compare0 output
 *  11 : compare1 output
 */
#define LL_SIMPLE_TIMER_CAP_SEL(n)                   (((n)&0x3) << 16)
/*!Period break flag   
 */
#define LL_SIMPLE_TIMER_PERIOD_ING                   (1UL << 15)
/*!Mark of capture  
 */
#define LL_SIMPLE_TIMER_CAP_ING                      (1UL << 14)
/*!Periodic interrupt was enabled   
 */
#define LL_SIMPLE_TIMER_PERIOD_IE                    (1UL << 13)
/*!Capture interrupt enable  
 */
#define LL_SIMPLE_TIMER_CAP_IE                       (1UL << 12)
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
#define LL_SIMPLE_TIMER_PSC(n)                       (((n)&0x7) << 8)
/*! Capture source edge selection
 *  0x0: Capture occurs during rising edge
 *  0x1: Capture occurs at falling edge
 *  0x2: Capture occurs along both the rising and falling edges
 *  0x3: Capture occurs along both the rising and falling edges  
 */
#define LL_SIMPLE_TIMER_EDG_SCL(n)                   (((n)&0x3) << 6)

/*! simple timer mode select bits:
 *  01 : timer counter mode
 *  10 : timer pwm mode
 *  11 : timer capture mode
 *  This bit of non-zero time counting is enabled
 */
#define LL_SIMPLE_TIMER_MODE_SEL(n)                  (((n)&0x3) << 4)

/*! Timer counter source select bits:
 8  000 : Select GPIO (CAP PIN) as the clock source Select the rising edge as the count source;
 *  001 : Select the GPIO (CAP PIN) as the clock source and the falling edge as the count source
 *  010 : Select EXT_CLK_SRC1 as the clock source and the rising and falling edges as the count sources
 *  011 : Select EXT_CLK_SRC0 as the clock source and the rising and falling edges as the count sources
 *  100 : Select EXT_CLK_SRC2 as the clock source and the rising and falling edges as the count sources
 *  111 : Select the overflow of the previous timer is selected as the count source. (withString multiple timers into one 64bit counter
 *  Others : system clock
 */
#define LL_SIMPLE_TIMER_INC_SRC_SEL(n)               (((n)&0x7) << 0)

/***** SINPLE TIMERx CNT Register *****/
/*!Counter register  
 */
#define LL_SIMPLE_TIMER_CNT(n)                       (((n)&0xFFFFFFFF) << 0)

/***** SINPLE TIMERx PERIOD Register *****/
/*!Period register  
 */
#define LL_SIMPLE_TIMER_PERIOD(n)                    (((n)&0xFFFFFFFF) << 0)

/***** SINPLE TIMERx PERIOD Register *****/
/*!Comparison value register  
 */
#define LL_SIMPLE_TIMER_PWM(n)                       (((n)&0xFFFFFFFF) << 0)



struct hgtimer_v7_hw {
    __IO uint32_t TMR_CTL;
    __IO uint32_t TMR_CNT;
    __IO uint32_t TMR_PR;
    __IO uint32_t TMR_PWM;
};

#ifdef __cplusplus
}
#endif

#endif /* _HGTIMER_V7_HW_H */


