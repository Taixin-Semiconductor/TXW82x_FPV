/*
 * Copyright (C) 2016 YunOS Project. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <k_api.h>
#include <csi_config.h>
#include <soc.h>
//#include <drv_timer.h>
#include "osal/irq.h"

#define OS_MS_PERIOD_TICK (1000/OS_SYSTICK_HZ)

typedef struct osTimespec {
    long    tv_sec;
    long    tv_msec;
} osTimespec_t;

extern uint64_t g_sys_tick_count;
extern uint32_t g_cpuloading;
extern uint32_t g_cpuloading_int;
extern volatile uint64_t g_sys_time_last;

void systick_handler(void)
{
    g_cpuloading_int++;
    g_sys_tick_count++;
    krhino_tick_proc();
    if((g_cpuloading_int*OS_MS_PERIOD_TICK) >= 2000){
        g_cpuloading = 100*(g_cpuloading_int - g_idle_task[cpu_cur_get()].runtime) / g_cpuloading_int;
        g_idle_task[cpu_cur_get()].runtime = 0;
        g_cpuloading_int = 0;
    }
}

uint64_t krhino_curr_nanosec(void)
{
    uint32_t flag;
    uint32_t cycles;
    uint64_t tick;
    uint64_t now;

    flag   = __disable_irq();
    tick   = g_sys_tick_count;
    cycles = csi_coret_get_value();
    if(csi_vic_get_pending_irq(CORET_IRQn)) { //tick中断被delay
        tick++;
    }
    if (!flag)  __enable_irq();

    cycles = csi_coret_get_load() - cycles;
    now    = (tick * OS_MS_PERIOD_TICK * 1000000ULL) + 
             (uint64_t)cycles * 1000000000ULL / DEFAULT_SYS_CLK;

    if(now < g_sys_time_last){
        now += (OS_MS_PERIOD_TICK * 1000000ULL);
    }
    g_sys_time_last = now;
    return now;
}

