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
 * Author: Simon Goldschmdit <goldsimon@gmx.de>
 *
 */
#ifndef LWIP_ARCH_SYS_ARCH_H
#define LWIP_ARCH_SYS_ARCH_H

#include "lwip/opt.h"
#include "lwip/arch.h"

/* unify os includes. */
#include "osal/msgqueue.h"
#include "osal/mutex.h"
#include "osal/semaphore.h"
#include "osal/task.h"
#include "osal/timer.h"
#include "hal/dma.h"

typedef u32_t sys_prot_t;
#define LWIP_SYS_ARCH_VAILD_MAGIC   (0x1A2B3C4D)
#define LWIP_SYS_ARCH_INVAILD_MAGIC (0x4D3C2B1A)

void sys_arch_msleep(u32_t delay_ms);
#define sys_msleep(ms) sys_arch_msleep(ms)

typedef struct {
    struct os_mutex mutex;
    s32_t mutex_vaild;
} sys_mutex_t;

#define sys_mutex_valid_val(mutex)   ((mutex).mutex_vaild == LWIP_SYS_ARCH_VAILD_MAGIC)
#define sys_mutex_valid(mutex)       (((mutex) != NULL) && sys_mutex_valid_val(*(mutex)))
#define sys_mutex_set_invalid(mutex) ((mutex)->mutex_vaild = LWIP_SYS_ARCH_INVAILD_MAGIC)

typedef struct {
    struct os_semaphore sem;
    s32_t sem_vaild;
} sys_sem_t;

#define sys_sem_valid_val(sema)   ((sema).sem_vaild == LWIP_SYS_ARCH_VAILD_MAGIC)
#define sys_sem_valid(sema)       (((sema) != NULL) && sys_sem_valid_val(*(sema)))
#define sys_sem_set_invalid(sema) ((sema)->sem_vaild = LWIP_SYS_ARCH_INVAILD_MAGIC)

typedef struct {
    struct os_msgqueue msgq;
    s32_t mbox_vaild;
} sys_mbox_t;

#define sys_mbox_valid_val(mbox)   ((mbox).mbox_vaild == LWIP_SYS_ARCH_VAILD_MAGIC)
#define sys_mbox_valid(mbox)       (((mbox) != NULL) && sys_mbox_valid_val(*(mbox)))
#define sys_mbox_set_invalid(mbox) ((mbox)->mbox_vaild = LWIP_SYS_ARCH_INVAILD_MAGIC)

typedef struct os_task *sys_thread_t;

#endif /* LWIP_ARCH_SYS_ARCH_H */
