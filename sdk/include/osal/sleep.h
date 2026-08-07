
#ifndef __OS_SLEEP_H_
#define __OS_SLEEP_H_

#include "typesdef.h"
#include "osal/time.h"

#ifdef __cplusplus
extern "C" {
#endif

/*jiffies is 64bit*/
#ifndef DIFF_JIFFIES
#define DIFF_JIFFIES(j1,j2) ((j2)-(j1))
#endif

#ifndef TIME_AFTER
/* returns true if the time a is after time b. */
#define TIME_AFTER(a,b)     ((int64)(b) - (int64)(a) < 0)
#endif

typedef enum {
    SYSTEM_SLEEP_TYPE_SRAM_WIFI      = 1,
    SYSTEM_SLEEP_TYPE_WIFI_DSLEEP    = 2,
    SYSTEM_SLEEP_TYPE_SRAM_ONLY      = 3,
    SYSTEM_SLEEP_TYPE_LIGHT_SLEEP    = 4,
    SYSTEM_SLEEP_TYPE_RTCC           = 5,
    SYSTEM_SLEEP_TYPE_MAX            = 6,
} SYSTEM_SLEEP_TYPE;

typedef enum {
    DSLEEP_WK_REASON_TIMER              = 1,
    DSLEEP_WK_REASON_TIM                = 2,
    DSLEEP_WK_REASON_BC_TIM             = 3,
    DSLEEP_WK_REASON_IO                 = 4, //discard
    DSLEEP_WK_REASON_BEACON_LOST        = 5,
    DSLEEP_WK_REASON_AP_ERROR           = 6,
    DSLEEP_WK_REASON_HB_TIMEOUT         = 7,
    DSLEEP_WK_REASON_WK_DATA            = 8,
    DSLEEP_WK_REASON_MCLR               = 9,
    DSLEEP_WK_REASON_LVD                = 10,
    DSLEEP_WK_REASON_PIR                = 11,
    DSLEEP_WK_REASON_APWK               = 12,
    DSLEEP_WK_REASON_PS_DISCONNECT      = 13,
    DSLEEP_WK_REASON_STANDBY            = 14,
    DSLEEP_WK_REASON_MAX_IDLE_TMO       = 15,
    DSLEEP_WK_REASON_WEAK_SIGNAL        = 16,

    DSLEEP_WK_REASON_STA_ERROR          = 20,
    DSLEEP_WK_REASON_SLEEPED_STA_ERROR  = 21,
    DSLEEP_WK_REASON_STA_DATA           = 22,
    DSLEEP_WK_REASON_AP_PAIR            = 23,
    DSLEEP_WK_REASON_KEY_UPDATE         = 24,

    DSLEEP_WK_REASON_RTC_ALARM          = 25,
    DSLEEP_WK_REASON_NULL_TX_FAIL       = 26,

    DSLEEP_WK_REASON_IO0                 = 27,
    DSLEEP_WK_REASON_IO1                 = 28,
    DSLEEP_WK_REASON_IO2                 = 29,
    DSLEEP_WK_REASON_IO3                 = 30,
    DSLEEP_WK_REASON_IO4                 = 31,
    DSLEEP_WK_REASON_IO5                 = 32,
    DSLEEP_WK_REASON_WDT                 = 33,
} DSLEEP_WK_REASON;

typedef enum {
    SYS_SLEEPCB_ACTION_RESUME  = 0,
    SYS_SLEEPCB_ACTION_SUSPEND = 1,
} SYS_SLEEPCB_ACTION;

typedef enum {
    SYS_SLEEPCB_LMAC,
    SYS_SLEEPCB_MID,
    SYS_SLEEPCB_UMAC,
    SYS_SLEEPCB_APP,
} SYS_SLEEPCB_STEP;

struct system_sleep_param {
    uint32 sleep_ms;
    uint8  wkup_io_sel[6];//sel IO0 - IO5
    uint8  wkup_io_en;//0: dis; 1 en
    uint8  wkup_io_edge;//0:Rising ; 1: Falling
    uint8  lsleep_enable    :1,
           lsleep_clk_div   :5;
};

struct sys_sleepcb_param {
    uint8 action, step;
    union {
        struct {
            uint8 wkreason;
        } resume;
        struct {
            struct system_sleep_param *param;
        } suspend;
    };
};

typedef int32(*sys_sleepcb)(uint16 type, struct sys_sleepcb_param *args, void *priv);

enum DSLEEP_IOCTL_CMD {
    /*Set CMDs*/
    DSLEEP_IOCTL_SET_ENTER_SLEEP_CB = 0x1, //discard
    DSLEEP_IOCTL_SET_KEEP_ALIVE_CB,
    DSLEEP_IOCTL_SET_USR_WKDET_CB,
    DSLEEP_IOCTL_SET_DSLEEP_BSS_MAX_IDLE,
    DSLEEP_IOCTL_SET_DSLEEP_WKIO_PIN,
    DSLEEP_IOCTL_SET_DSLEEP_WKIO_EDGE,
    DSLEEP_IOCTL_SET_DSLEEP_APLOST_TIME,
    DSLEEP_IOCTL_SET_DSLEEP_SENS_LEVEL,
    DSLEEP_IOCTL_SET_DSLEEP_PWM_SEL,
    DSLEEP_IOCTL_SET_DSLEEP_PWM_MODE,
    DSLEEP_IOCTL_SET_IP_ADDR,
    DSLEEP_IOCTL_SET_GPIOA_RESV,
    DSLEEP_IOCTL_SET_GPIOB_RESV,
    DSLEEP_IOCTL_SET_GPIOC_RESV,
    DSLEEP_IOCTL_SET_DTIM,
    DSLEEP_IOCTL_SET_DEBOUNCE,
    DSLEEP_IOCTL_SET_TXIC_EN,
    DSLEEP_IOCTL_SET_TXIC_WAKEP,
    DSLEEP_IOCTL_SET_WATCHDOG_CLOSE,
    DSLEEP_IOCTL_SET_FAKE_TIM_FADEOUT,
    DSLEEP_IOCTL_SET_CLK_SWITCH_DIS,
    DSLEEP_IOCTL_SET_USE_OLD_LIBFLASH,
    DSLEEP_IOCTL_SET_PA_LATCH,
    DSLEEP_IOCTL_SET_EXT_DCDC,
    DSLEEP_IOCTL_SET_ASSERT_HOLD,
    DSLEEP_IOCTL_SET_DBG_CFG,
    /*Get CMDs*/
    DSLEEP_IOCTL_GET_IP_ADDR = 0x20000000,
    DSLEEP_IOCTL_GET_DTIM,
    DSLEEP_IOCTL_GET_DBG_CFG
};

//ioctrl: set_cfg
#define dsleep_set_keep_alive_cb(priv, func)        dsleep_ioctl(DSLEEP_IOCTL_SET_KEEP_ALIVE_CB, (uint32)priv, (uint32)(func))
#define dsleep_set_usr_wkdet_cb(priv, func)         dsleep_ioctl(DSLEEP_IOCTL_SET_USR_WKDET_CB, (uint32)priv, (uint32)(func))
#define dsleep_set_max_idle(max_idle_sec)           dsleep_ioctl(DSLEEP_IOCTL_SET_DSLEEP_BSS_MAX_IDLE, (uint32)(max_idle_sec), 0)
#define dsleep_set_wkio_pin(pin_idx, pin_num)       dsleep_ioctl(DSLEEP_IOCTL_SET_DSLEEP_WKIO_PIN, (uint32)(pin_idx), (uint32)(pin_num))
#define dsleep_set_wkio_edge(pin_idx, neg_edge)     dsleep_ioctl(DSLEEP_IOCTL_SET_DSLEEP_WKIO_EDGE, (uint32)(pin_idx), (uint32)(neg_edge))
#define dsleep_set_aplost_time(t_sec)               dsleep_ioctl(DSLEEP_IOCTL_SET_DSLEEP_APLOST_TIME, (uint32)(t_sec), 0)
#define dsleep_set_sens_level(sens_level)           dsleep_ioctl(DSLEEP_IOCTL_SET_DSLEEP_SENS_LEVEL, (uint32)(sens_level), 0)
#define dsleep_set_pwm_sel(pwm_idx, pin_num)        dsleep_ioctl(DSLEEP_IOCTL_SET_DSLEEP_PWM_SEL, (uint32)(pwm_idx), (pin_num))
#define dsleep_set_pwm_mode(pwm_idx, mode)          dsleep_ioctl(DSLEEP_IOCTL_SET_DSLEEP_PWM_MODE, (uint32)(pwm_idx), (mode))
#define dsleep_set_ip_addr(ip_addr)                 dsleep_ioctl(DSLEEP_IOCTL_SET_IP_ADDR, (uint32)ip_addr, 0)
#define dsleep_set_user_gioa(user_gioa)             dsleep_ioctl(DSLEEP_IOCTL_SET_GPIOA_RESV, (uint32)user_gioa, 0)
#define dsleep_set_user_giob(user_giob)             dsleep_ioctl(DSLEEP_IOCTL_SET_GPIOB_RESV, (uint32)user_giob, 0)
#define dsleep_set_user_gioc(user_gioc)             dsleep_ioctl(DSLEEP_IOCTL_SET_GPIOC_RESV, (uint32)user_gioc, 0)
#define dsleep_set_dtim(val)                        dsleep_ioctl(DSLEEP_IOCTL_SET_DTIM, (uint32)val, 0)
#define dsleep_set_debounce(val)                    dsleep_ioctl(DSLEEP_IOCTL_SET_DEBOUNCE, (uint32)val, 0)
#define dsleep_set_txic_en(en)                      dsleep_ioctl(DSLEEP_IOCTL_SET_TXIC_EN, (uint32)en, 0)
#define dsleep_set_txic_wakeup(en)                  dsleep_ioctl(DSLEEP_IOCTL_SET_TXIC_WAKEP, (uint32)en, 0)
#define dsleep_set_watchdog_close(en)               dsleep_ioctl(DSLEEP_IOCTL_SET_WATCHDOG_CLOSE, (uint32)en, 0)
#define dsleep_set_tim_fadeout(val)                 dsleep_ioctl(DSLEEP_IOCTL_SET_FAKE_TIM_FADEOUT, (uint32)val, 0)
#define dsleep_set_clk_switch_dis(val)              dsleep_ioctl(DSLEEP_IOCTL_SET_CLK_SWITCH_DIS, (uint32)val, 0)
#define dsleep_set_use_old_libflash(val)            dsleep_ioctl(DSLEEP_IOCTL_SET_USE_OLD_LIBFLASH, (uint32)val, 0)
#define dsleep_set_pa_latch(val)                    dsleep_ioctl(DSLEEP_IOCTL_SET_PA_LATCH, (uint32)val, 0)
#define dsleep_set_ext_dcdc(en)                     dsleep_ioctl(DSLEEP_IOCTL_SET_EXT_DCDC, (uint32)en, 0)
#define dsleep_set_assert_hold(en)                  dsleep_ioctl(DSLEEP_IOCTL_SET_ASSERT_HOLD, (uint32)en, 0)
#define dsleep_set_dbg_cfg(dbg_cfg)                 dsleep_ioctl(DSLEEP_IOCTL_SET_DBG_CFG, (uint32)dbg_cfg, 0)

//ioctrl: get_cfg
#define dsleep_get_ip_addr()                        dsleep_ioctl(DSLEEP_IOCTL_GET_IP_ADDR, 0, 0)
#define dsleep_get_dtim()                           dsleep_ioctl(DSLEEP_IOCTL_GET_DTIM, 0, 0)
#define dsleep_get_dbg_cfg()                        dsleep_ioctl(DSLEEP_IOCTL_GET_DBG_CFG, 0, 0)

int32 dsleep_ioctl(uint32 cmd, uint32 param1, uint32 param2);
void os_sleep(int32 sec);
void os_sleep_ms(int32 msec);
extern void os_sleep_us(int us);

int usleep(useconds_t usec);
unsigned int sleep(unsigned int us);
void delay_us(uint32 n);

int sys_sleepcb_init(void);
int sys_register_sleepcb(sys_sleepcb cb, void *priv);
int sys_suspend(uint16 type, uint8 step, struct system_sleep_param *args);
int sys_resume(uint16 type, uint8 step, uint8 wkreason);

void sys_wakeup_host(void);
void sys_enter_sleep(void);
uint8 sys_wakeup_reason(void);

int32 system_sleep(uint16 type, struct system_sleep_param *args);
void system_resume(uint16 type, uint8 wkreason);

#ifdef __cplusplus
}
#endif

#endif

