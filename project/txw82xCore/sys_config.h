#ifndef __SDK_SYS_CONFIG_H__
#define __SDK_SYS_CONFIG_H__
#include "project_config.h"

#define PROJECT_TYPE                    PRO_TYPE_FPV
#define SYS_CACHE_ENABLE                1

#define __ram                           __at_section(".ram.text")

#define CPURPC_TASK_STACKSIZE           1024

#ifndef OS_SYSTICK_HZ
#define OS_SYSTICK_HZ                   1000
#endif

#ifndef OS_IRQ_STACK_SIZE
#define OS_IRQ_STACK_SIZE               1024
#endif

#ifndef OS_IDLE_TASK_STACK
#define OS_IDLE_TASK_STACK             (64 + 128) //=64*4
#endif

#ifndef OS_TIMER_TASK_STACK_SIZE
#define OS_TIMER_TASK_STACK_SIZE        128 //=128*4
#endif

#ifndef OS_TIMER_MSG_NUM
#define OS_TIMER_MSG_NUM                10
#endif

#define BLE_SUPPORT                     1

#define DEFAULT_SYS_CLK                 CoreSetting->cpu_clk

#ifndef SD_MODE_TYPE
#define SD_MODE_TYPE (2)
#endif

#endif

