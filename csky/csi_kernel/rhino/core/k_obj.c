/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include <k_api.h>

__bobj int32_t      errno;
__bobj uint64_t     g_sys_tick_count;
__bobj uint32_t     g_sys_tick_cycles;
__bobj uint32_t     g_cpuloading;
__bobj uint32_t     g_cpuloading_int;

__bobj kstat_t      g_sys_stat;
__bobj uint8_t      g_idle_task_spawned[RHINO_CONFIG_CPU_NUM];

__bobj runqueue_t   g_ready_queue;

/* schedule lock counter */
__bobj uint8_t      g_sched_lock[RHINO_CONFIG_CPU_NUM];
__bobj uint8_t      g_intrpt_nested_level[RHINO_CONFIG_CPU_NUM];

/* highest pri task in ready queue */
__bobj ktask_t     *g_preferred_ready_task[RHINO_CONFIG_CPU_NUM];

/* current active task */
__bobj ktask_t     *g_active_task[RHINO_CONFIG_CPU_NUM];

/* idle task attribute */
__bobj ktask_t      g_idle_task[RHINO_CONFIG_CPU_NUM];
__bobj idle_count_t g_idle_count[RHINO_CONFIG_CPU_NUM];
__bobj cpu_stack_t  g_idle_task_stack[RHINO_CONFIG_CPU_NUM][RHINO_CONFIG_IDLE_TASK_STACK_SIZE];

/* tick attribute */
__bobj tick_t       g_tick_count;
__bobj klist_t      g_tick_head;

#if (RHINO_CONFIG_SYSTEM_STATS > 0)
__bobj kobj_list_t  g_kobj_list;
#endif

#if (RHINO_CONFIG_TIMER > 0)
__bobj klist_t          g_timer_head;
__bobj sys_time_t       g_timer_count;
__bobj ktask_t          g_timer_task;
__bobj cpu_stack_t      g_timer_task_stack[RHINO_CONFIG_TIMER_TASK_STACK_SIZE];
__bobj kbuf_queue_t     g_timer_queue;
__bobj k_timer_queue_cb timer_queue_cb[RHINO_CONFIG_TIMER_MSG_NUM];
#endif

#if (RHINO_CONFIG_ISR_TASK > 0)
__bobj ktask_t          g_isr_task;
__bobj ksem_t           g_isr_sem;
__bobj uint32_t         g_isr_buff[4+2*RHINO_CONFIG_ISR_BUFF_SIZE];
__bobj cpu_stack_t      g_isr_task_stack[RHINO_CONFIG_ISR_TASK_STACK_SIZE];
#endif

#if (RHINO_CONFIG_DISABLE_SCHED_STATS > 0)
__bobj hr_timer_t   g_sched_disable_time_start;
__bobj hr_timer_t   g_sched_disable_max_time;
__bobj hr_timer_t   g_cur_sched_disable_max_time;
#endif

#if (RHINO_CONFIG_DISABLE_INTRPT_STATS > 0)
__bobj uint16_t     g_intrpt_disable_times;
__bobj hr_timer_t   g_intrpt_disable_time_start;
__bobj hr_timer_t   g_intrpt_disable_max_time;
__bobj hr_timer_t   g_cur_intrpt_disable_max_time;
#endif

#if (RHINO_CONFIG_HW_COUNT > 0)
__bobj hr_timer_t   g_sys_measure_waste;
#endif

#if (RHINO_CONFIG_CPU_USAGE_STATS > 0)
__bobj ktask_t      g_cpu_usage_task;
__bobj cpu_stack_t  g_cpu_task_stack[RHINO_CONFIG_CPU_USAGE_TASK_STACK];
__bobj idle_count_t g_idle_count_max;
__bobj uint32_t     g_cpu_usage;
#endif

#if (RHINO_CONFIG_TASK_SCHED_STATS > 0)
__bobj ctx_switch_t g_sys_ctx_switch_times;
#endif

#if (RHINO_CONFIG_KOBJ_DYN_ALLOC > 0)
__bobj ksem_t       g_res_sem;
__bobj klist_t      g_res_list;
__bobj ktask_t      g_dyn_task;
__bobj cpu_stack_t  g_dyn_task_stack[RHINO_CONFIG_K_DYN_TASK_STACK];
#endif

#if (RHINO_CONFIG_WORKQUEUE > 0)
__bobj klist_t       g_workqueue_list_head;
__bobj kmutex_t      g_workqueue_mutex;
__bobj kworkqueue_t  g_workqueue_default;
__bobj cpu_stack_t   g_workqueue_stack[RHINO_CONFIG_WORKQUEUE_STACK_SIZE];
#endif

#if (RHINO_CONFIG_MM_TLF > 0)

//k_mm_head    *g_kmm_head;

#endif

__bobj kspinlock_t   g_sys_lock;

__bobj uint8_t g_in_interrupt;

