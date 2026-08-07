/*
 * @Author: jiejie
 * @Github: https://github.com/jiejieTop
 * @Date: 2019-12-23 19:26:27
 * @LastEditTime: 2020-09-23 08:53:43
 * @Description: the code belongs to jiejie, please keep the author information and source code according to the license.
 */
#include "platform_thread.h"
#include "platform_memory.h"

platform_thread_t *platform_thread_init( const char *name,
                                        void (*entry)(void *),
                                        void * const param,
                                        unsigned int stack_size,
                                        unsigned int priority,
                                        unsigned int tick)
{
    platform_thread_t *thread = NULL;
    void *stack = NULL;          //124 = sizeof(ktask_t)
    
    thread = platform_memory_alloc(sizeof(platform_thread_t));
    if(thread == NULL) {
        return NULL;
    }

    thread->task = (struct os_task *)platform_memory_alloc(sizeof(struct os_task));
    if (thread->task == NULL) {
        os_printf("[%s]:task malloc failed!!!\n",__FUNCTION__);
        platform_memory_free(thread->task);
        return NULL;
    }
    stack = platform_memory_alloc(stack_size+124);
    if (stack == NULL) {
        os_printf("[%s]:stack malloc failed!!!\n",__FUNCTION__);
        platform_memory_free(thread->task);
        platform_memory_free(thread);
        return NULL;
    }
    os_task_init((const uint8 *)name, thread->task, entry, (uint32)param);
    os_task_set_priority(thread->task, priority);
    os_task_set_stack(thread->task, stack, stack_size+124); //124 = sizeof(ktask_t)

    return thread;
}

void platform_thread_startup(platform_thread_t* thread)
{
    if (thread && thread->task) {
        os_task_run(thread->task);
    }
}


void platform_thread_stop(platform_thread_t* thread)
{
    if (thread && thread->task) {
        os_task_suspend(thread->task);
    }
}

void platform_thread_start(platform_thread_t* thread)
{
    if (thread && thread->task) {
        os_task_resume(thread->task);
    }
}

void platform_thread_destroy(platform_thread_t* thread)
{
    if (thread && thread->task) {
        os_task_del(thread->task);
        if (thread->task->stack) {
            platform_memory_free(thread->task->stack);
        }
        platform_memory_free(thread->task);
        platform_memory_free(thread);
    }
}
