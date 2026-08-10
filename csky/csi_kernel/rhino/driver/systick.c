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

typedef struct osTimespec {
    long    tv_sec;
    long    tv_msec;
} osTimespec_t;

extern uint64_t g_sys_tick_count;
extern uint32_t g_cpuloading;
extern uint32_t g_cpuloading_int;
extern uint64_t g_sys_time_last;

void systick_handler(void)
{
    g_cpuloading_int++;
    g_sys_tick_count++;
    krhino_tick_proc();
    if ((g_cpuloading_int * OS_MS_PERIOD_TICK) >= 2000) {
        g_cpuloading = 100 * (g_cpuloading_int - g_idle_task[cpu_cur_get()].runtime) / g_cpuloading_int;
        g_idle_task[cpu_cur_get()].runtime = 0;
        g_cpuloading_int = 0;
    }
}

uint64_t krhino_curr_nanosec(void)
{
    uint32_t irq;
    uint64_t tick;
    uint32_t cycles, load;
    uint32_t countflag;
    uint64_t total_cycles;
    uint64_t now;

    irq = disable_irq();
    tick   = g_sys_tick_count;
    cycles = csi_coret_get_value();
    load   = csi_coret_get_load();
    countflag = (CORET->CTRL >> 16) & 0x01;
    enable_irq(irq);

    uint32_t period_len = load + 1;

    if (countflag) {
        // 关中断期间发生了回绕：已过周期数 = (tick+1) * period_len - cycles
        // 注意：cycles 是回绕后的当前值（从 load 重新开始递减）
        total_cycles = (tick + 1) * (uint64_t)period_len - cycles;
    } else {
        // 无回绕：已过周期数 = tick * period_len + (load - cycles)
        // 修正：从 load 递减到 cycles，已过周期数为 load - cycles
        total_cycles = tick * (uint64_t)period_len + (load - cycles);
    }

    // 计算秒和纳秒，避免所有问题
    uint64_t seconds = total_cycles / DEFAULT_SYS_CLK;
    uint64_t rem = total_cycles % DEFAULT_SYS_CLK;
    uint64_t nanoseconds = (rem * 1000000000ULL) / DEFAULT_SYS_CLK;
    
    now = seconds * 1000000000ULL + nanoseconds;

    // 单调性保护
    if (now < g_sys_time_last) {
        now = g_sys_time_last + 1;
    }
    g_sys_time_last = now;
    return now;
}

