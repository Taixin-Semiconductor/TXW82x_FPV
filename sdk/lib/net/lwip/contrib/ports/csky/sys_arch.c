/*
 * Copyright (c) 2017 Simon Goldschmidt
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Simon Goldschmidt <goldsimon@gmx.de>
 *
 */

/* lwIP includes. */
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"
#include "lwip/tcpip.h"

/* unify os includes. */
#include "osal/msgqueue.h"
#include "osal/mutex.h"
#include "osal/semaphore.h"
#include "osal/task.h"
#include "osal/timer.h"
#include "osal/sleep.h"
#include "osal/irq.h"

/* Initialize this module (see description in sys.h) */

static struct os_mutex lwip_sys_mutex;
static const u32_t null_msg = 0;
static u8_t thread_pool_index = 0;
static struct os_task thread_pool[LWIP_SYS_THREAD_POOL_N];

void
sys_init(void)
{
    s32_t ret = 0;
    ret = os_mutex_init(&lwip_sys_mutex);
    ASSERT(!ret);
    memset(&thread_pool,0,sizeof(thread_pool));
    thread_pool_index = 0;
}

u32_t
sys_jiffies(void)
{
    return (u32_t)os_jiffies();
}

u32_t
sys_now(void)
{
    return (u32_t)os_jiffies_to_msecs(os_jiffies());
}

sys_prot_t
sys_arch_protect(void)
{
    sys_prot_t ret = 1;
    os_mutex_lock(&lwip_sys_mutex, osWaitForever);
    return ret;
}

void
sys_arch_unprotect(sys_prot_t pval)
{
    os_mutex_unlock(&lwip_sys_mutex);
    LWIP_UNUSED_ARG(pval);
}

void
sys_arch_msleep(u32_t delay_ms)
{
    os_sleep_ms(delay_ms);
}

/* Create a new mutex*/
err_t
sys_mutex_new(sys_mutex_t *mutex)
{
    LWIP_ASSERT("mutex != NULL", mutex != NULL);
    memset(mutex, 0, sizeof(sys_mutex_t));
    if (RET_OK != os_mutex_init(&mutex->mutex)) {
        lwip_printf("sys_mutex_new error\n");
        return ERR_MEM;
    }
    mutex->mutex_vaild = LWIP_SYS_ARCH_VAILD_MAGIC;
    return ERR_OK;
}

void
sys_mutex_lock(sys_mutex_t *mutex)
{
    int32 ret = 0;
    /* nothing to do, no multithreading supported */
    LWIP_ASSERT("mutex != NULL", mutex != NULL);
    LWIP_ASSERT("mutex->mutex_vaild invaild!", mutex->mutex_vaild == LWIP_SYS_ARCH_VAILD_MAGIC);
    /* check that the mutext is valid and unlocked (no nested locking) */
    ret = os_mutex_lock(&mutex->mutex, osWaitForever);
    LWIP_ASSERT("failed to take the mutex", ret == 0);
}

void
sys_mutex_unlock(sys_mutex_t *mutex)
{
    /* nothing to do, no multithreading supported */
    LWIP_ASSERT("mutex != NULL", mutex != NULL);
    LWIP_ASSERT("mutex->mutex_vaild invaild", mutex->mutex_vaild == LWIP_SYS_ARCH_VAILD_MAGIC);
    /* we count down just to check the correct pairing of lock/unlock */
    os_mutex_unlock(&mutex->mutex);
}

void
sys_mutex_free(sys_mutex_t *mutex)
{
    /* parameter check */
    LWIP_ASSERT("mutex != NULL", mutex != NULL);
    LWIP_ASSERT("mutex->mutex_vaild invaild", mutex->mutex_vaild == LWIP_SYS_ARCH_VAILD_MAGIC);
    if (RET_OK != os_mutex_del(&mutex->mutex)) {
        lwip_printf("sys_mutex_free error\n");
    }
    mutex->mutex_vaild = LWIP_SYS_ARCH_INVAILD_MAGIC;
}

err_t
sys_sem_new(sys_sem_t *sem, u8_t initial_count)
{
    LWIP_ASSERT("sem != NULL", sem != NULL);
    LWIP_ASSERT("initial_count invalid (not 0 or 1)",
                (initial_count == 0) || (initial_count == 1));
    memset(sem, 0, sizeof(sys_sem_t));
    if (RET_OK != os_sema_init(&sem->sem, initial_count)) {
        lwip_printf("sys_sem_new create error\n");
        return ERR_MEM;
    }
    sem->sem_vaild = LWIP_SYS_ARCH_VAILD_MAGIC;
    return ERR_OK;
}

void
sys_sem_signal(sys_sem_t *sem)
{
    LWIP_ASSERT("sem != NULL", sem != NULL);
    LWIP_ASSERT("sem->sem_vaild invaild", sem->sem_vaild == LWIP_SYS_ARCH_VAILD_MAGIC);
    os_sema_up(&sem->sem);
}

u32_t
sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout_ms)
{
    LWIP_ASSERT("sem != NULL", sem != NULL);
    LWIP_ASSERT("sem->sem_vaild invaild", sem->sem_vaild == LWIP_SYS_ARCH_VAILD_MAGIC);
    //u32_t start = sys_now();

    if (os_sema_down(&sem->sem, (timeout_ms != 0) ? (timeout_ms) : (osWaitForever)) < 1) {
        return SYS_ARCH_TIMEOUT;
    }
    /* Old versions of lwIP required us to return the time waited.
    This is not the case any more. Just returning != SYS_ARCH_TIMEOUT
    here is enough. */
    return 1;//(DIFF_JIFFIES(start, sys_now())) / 1000;
}

void
sys_sem_free(sys_sem_t *sem)
{
    LWIP_ASSERT("sem != NULL", sem != NULL);
    LWIP_ASSERT("sem->sem_vaild invaild", sem->sem_vaild == LWIP_SYS_ARCH_VAILD_MAGIC);
    if (RET_OK != os_sema_del(&sem->sem)) {
        lwip_printf("sys_sem_free error!");
    }
    sem->sem_vaild = LWIP_SYS_ARCH_INVAILD_MAGIC;
}

err_t
sys_mbox_new(sys_mbox_t *mbox, int size)
{
    LWIP_ASSERT("mbox != NULL", mbox != NULL);
    LWIP_ASSERT("size > 0", size > 0);
    memset(mbox, 0, sizeof(sys_mbox_t));
    if (RET_OK == os_msgq_init(&mbox->msgq, size)) {
        mbox->mbox_vaild = LWIP_SYS_ARCH_VAILD_MAGIC;
        return ERR_OK;
    } else {
        lwip_printf("os_msgq_init error!\n");
        return ERR_MEM;
    }
}

void
sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
    int ret = 0;
    LWIP_ASSERT("mbox != NULL", mbox != NULL);
    LWIP_ASSERT("q->mbox_vaild invaild", mbox->mbox_vaild == LWIP_SYS_ARCH_VAILD_MAGIC);
    if (msg == NULL) {
        msg = (void *)&null_msg;
    }
    ret = os_msgq_put(&mbox->msgq, (u32_t)msg, osWaitForever);
    LWIP_ASSERT("mbox post failed", ret == 0);
}

err_t
sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    LWIP_ASSERT("mbox!= NULL", mbox != NULL);
    LWIP_ASSERT("q->mbox_vaild invaild", mbox->mbox_vaild == LWIP_SYS_ARCH_VAILD_MAGIC);
    if (msg == NULL) {
        msg = (void *)&null_msg;
    }
    return (ERR_OK == os_msgq_put(&mbox->msgq, (u32_t)msg, 0)) ? (ERR_OK) : (ERR_MEM);
}

err_t
sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg)
{
    return sys_mbox_trypost(mbox, msg);
}

u32_t
sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout_ms)
{
    u32_t handle    = 0;
    LWIP_ASSERT("mbox!= NULL", mbox != NULL);
    LWIP_ASSERT("q->mbox_vaild invaild", mbox->mbox_vaild == LWIP_SYS_ARCH_VAILD_MAGIC);

    handle = os_msgq_get(&mbox->msgq, (timeout_ms != 0) ? (timeout_ms) : (osWaitForever));
    if (handle != 0) {
        if (handle == (u32_t)&null_msg) {
            *msg = NULL;
        } else {
            *msg = (void *)handle;
        }
    } else {
        *msg = NULL;
        return SYS_ARCH_TIMEOUT;
    }
    return 1;//(DIFF_JIFFIES(sys_now(), start)) >> 10;
}

u32_t
sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
    uint32 handle = 0;

    LWIP_ASSERT("mbox!= NULL", mbox != NULL);
    LWIP_ASSERT("q->mbox_vaild invaild", mbox->mbox_vaild == LWIP_SYS_ARCH_VAILD_MAGIC);

    handle = os_msgq_get(&mbox->msgq, 0);

    if (handle != 0) {
        if (handle == (u32_t)&null_msg) {
            *msg = NULL;
        } else {
            *msg = (void *)handle;
        }
    } else {
        *msg = NULL;
        return SYS_MBOX_EMPTY;
    }
    return ERR_OK;
}

void
sys_mbox_free(sys_mbox_t *mbox)
{
    /* parameter check */
    LWIP_ASSERT("mbox != NULL", mbox != NULL);
    LWIP_ASSERT("q->mbox_vaild invaild", mbox->mbox_vaild == LWIP_SYS_ARCH_VAILD_MAGIC);

    os_msgq_del(&mbox->msgq);
    mbox->mbox_vaild = LWIP_SYS_ARCH_INVAILD_MAGIC;
}

u32_t
sys_mbox_get_num(sys_mbox_t *q) 
{
    if(q && q->mbox_vaild == LWIP_SYS_ARCH_VAILD_MAGIC) {
        return os_msgq_cnt(&q->msgq);        
    } else {
        return 0;
    }
}

sys_thread_t
sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
    sys_thread_t task_hdl = NULL;
    LWIP_ASSERT("invalid stacksize", stacksize > 0);

    if (thread_pool_index >= LWIP_SYS_THREAD_POOL_N) {
        //error handle?
        lwip_printf("calling sys_thread_new too much\n");
        return NULL;
    }
    task_hdl = (sys_thread_t)&thread_pool[thread_pool_index];
    thread_pool_index++;

    LWIP_DEBUGF(SYS_DEBUG, ("New Thread: %s\n", name));

    OS_TASK_INIT(name, task_hdl, thread, *((uint32 *)arg), prio, NULL, stacksize);
    return task_hdl;
}

