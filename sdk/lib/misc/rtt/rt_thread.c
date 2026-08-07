#include "rtthread.h"

rt_thread_t rt_thread_create(const char *name, void (*entry)(void *parameter), void *parameter,
                             rt_uint32_t stack_size, rt_uint8_t  priority, rt_uint32_t tick)
{
    struct rt_thread *thd = os_malloc(sizeof(struct rt_thread));
    void *stack = os_malloc(stack_size+124);                        //124 = sizeof(ktask_t)
    if(stack == RT_NULL)
    {
        os_printf("[%s]:stack malloc failed!!!\n",__FUNCTION__);    //usb任务栈使用sram空间，避免打到psram动态申请导致usb异常
        return NULL;
    }
    if (thd) {
        os_task_init((const uint8 *)name, thd, entry, (uint32)parameter);
        os_task_set_priority(thd, priority);
        os_task_set_stack(thd, stack, stack_size+124);          //124 = sizeof(ktask_t)
    }
    return thd;
}

rt_err_t rt_thread_delete(rt_thread_t thread)
{
    rt_err_t ret;
    ret = os_task_del(thread);
    if(thread->stack)
    {
        os_free(thread->stack);
    }
    os_free(thread);
    return ret;
}

rt_err_t rt_thread_init(struct rt_thread *thread, const char *name, void (*entry)(void *parameter),
                        void *parameter, void *stack_start, rt_uint32_t stack_size,
                        rt_uint8_t priority, rt_uint32_t tick)
{
    void *stack = RT_NULL;
    rt_uint32_t stacksize = 0;

    stacksize = stack_size+124;                          //124 = sizeof(ktask_t)
    stack = os_malloc(stacksize);                       //usb任务栈使用sram空间，避免打到psram动态申请导致usb异常
    if(stack == RT_NULL)
    {
        os_printf("[%s]:stack malloc failed!!!\n",__FUNCTION__);
        return RT_ERROR;
    }

    if(stack_start)
    {
        os_printf("[%s]:This api auto malloc stack, you can free your stack space!!!\r\n");
    }

    os_task_init((const uint8 *)name, thread, entry, (uint32)parameter);
    os_task_set_priority(thread, priority);
    os_task_set_stack(thread, stack_start, stack_size);
    return RT_EOK;
}

rt_err_t rt_thread_detach(rt_thread_t thread)
{
    rt_err_t ret;
    ret = os_task_del(thread);
    if(thread->stack)
    {
        os_free(thread->stack);
        thread->stack = NULL;
    }
    return ret; 
}

rt_thread_t rt_thread_self(void)
{
    return (rt_thread_t)os_task_data(os_task_current());
}

rt_thread_t rt_thread_find(char *name)
{
    return NULL;
}

rt_err_t rt_thread_startup(rt_thread_t thread)
{
    return os_task_run(thread);
}

rt_err_t rt_thread_yield(void)
{
    return os_task_yield();
}

rt_err_t rt_thread_delay(rt_tick_t tick)
{
    os_sleep_ms(os_jiffies_to_msecs(tick));
    return RET_OK;
}

rt_err_t rt_thread_delay_until(rt_tick_t *tick, rt_tick_t inc_tick)
{
    return RT_ERROR;
}

rt_err_t rt_thread_mdelay(rt_int32_t ms)
{
    os_sleep_ms(ms);
    return RET_OK;
}

rt_err_t rt_thread_control(rt_thread_t thread, int cmd, void *arg)
{
    return RT_ERROR;
}

rt_err_t rt_thread_suspend(rt_thread_t thread)
{
    return os_task_suspend(thread);
}

rt_err_t rt_thread_resume(rt_thread_t thread)
{
    return os_task_resume(thread);
}

void rt_thread_timeout(void *parameter)
{
}

void rt_enter_critical(void)
{
    os_sched_disable();
}

void rt_exit_critical(void)
{
    os_sched_enbale();
}

