#include "sys_config.h"
#include "tx_platform.h"
#ifdef TXW80X
#include "txw80x/ticker.h"
#endif

#ifdef TXW81X
#include "txw83x/ticker.h"
#endif
#include "osal/irq.h"
#include "list.h"
#include "dev.h"

#if 0
#define SIMPLE_TIMER0(offset) (*((uint32 *)(0x40015B00 + offset)))

/**
  * @brief  The tick timer is initialized.
  * @param  None
  * @retval None
  * @note   Use the lowest priority interrupt.
  */
void hw_timer_init(void)
{
    //STMR0_CTL
    SIMPLE_TIMER0(0x00) = (4 << 8) | (7 << 0);
    //STMR0_CNT
    SIMPLE_TIMER0(0x04) = 0;
    //STMR0_PR
    SIMPLE_TIMER0(0x04) = 0xFFFFFFFF;    
}

/**
  * @brief  get the tick of the timer.
  * @param  None
  * @retval None
  */
uint32 hw_timer_get_tick(uint8 start)
{
    uint32 cnt = 0;
    uint32_t v = sys_get_sysclk()/1000000;

    if(start){
        __disable_irq();
        //STMR0_CNT
        SIMPLE_TIMER0(0x04) = 0;
        //TMR_EN
        SIMPLE_TIMER0(0x00) |= (1 << 4);
        //*(uint32_t *)(HG_GPIOC_BASE+36) |= BIT(7);
        __enable_irq();
        return 0;
    }else{
        __disable_irq();
        //*(uint32_t *)(HG_GPIOC_BASE+36) &= ~ BIT(7);
        //TMR_CNT
        cnt = SIMPLE_TIMER0(0x04);
        //TMR_EN
        SIMPLE_TIMER0(0x00) &= ~(1 << 4);        
        __enable_irq();
        return (16*cnt)/v;
    }
}
#endif

/**
  * @brief  Use the CPU precision delay function
  * @param  n :
  * @retval None
  * @note   for CK803 : period_count = 3*n+1
  */
__attribute__((noinline, naked)) void __delay_asm(uint32 n)
{
    __asm__("                   \
delay_asm_L1:                   \
    DECGT32 R0, R0, 1         \n\
    BT32 delay_asm_L1         \n\
    rts                       \n\
    ");
}

/**
  * @brief  Use the CPU to delay approximately us
  * @param  n : the number of us to delay
  * @retval None
  * @note   only work for SYS_CLK
  */
void delay_us(uint32 n)
{
    uint32 temp = sys_get_sysclk()/1000000*n;
    
    if(temp > 47) {
        __delay_asm((temp - 47)/3);
    }
}

