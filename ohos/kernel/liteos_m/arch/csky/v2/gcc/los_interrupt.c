/*
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdarg.h>
#include "securec.h"
#include "los_context.h"
#include "los_arch_context.h"
#include "los_arch_interrupt.h"
#include "los_debug.h"
#include "los_hook.h"
#include "los_task.h"
#include "los_sched.h"
#include "los_memory.h"
#include "los_membox.h"
#include "osal/string.h"

#define INT_OFFSET       6
#define PRI_OFF_PER_INT  8
#define PRI_PER_REG      4
#define PRI_OFF_IN_REG   6
#define PRI_BITS         2
#define PRI_HI           0
#define PRI_LOW          7
#define MASK_8_BITS      0xFF
#define MASK_32_BITS     0xFFFFFFFF
#define BYTES_OF_128_INT 4


VIC_TYPE *VIC_REG = (VIC_TYPE *)VIC_REG_BASE;

UINT32 HalGetPsr(VOID)
{
    UINT32 intSave;
    __asm__ volatile("mfcr %0, psr" : "=r"(intSave) : : "memory");
    return intSave;
}

UINT32 HalSetVbr(UINT32 intSave)
{
    __asm__ volatile("mtcr %0, vbr" : : "r"(intSave)  : "memory");
    return intSave;
}

UINT32 ArchIntLock(VOID)
{
    UINT32 intSave;
    __asm__ __volatile__(
        "mfcr    %0, psr \n"
        "psrclr ie"
        : "=r"(intSave)
        :
        : "memory");
    return intSave;
}

UINT32 ArchIntUnLock(VOID)
{
    UINT32 intSave;
    __asm__ __volatile__(
        "mfcr   %0, psr \n"
        "psrset ie"
        : "=r"(intSave)
        :
        : "memory");
    return intSave;
}

VOID ArchIntRestore(UINT32 intSave)
{
    __asm__ __volatile__("mtcr %0, psr" : : "r"(intSave));
}

UINT32 ArchIntLocked(VOID)
{
    UINT32 intSave;
    __asm__ volatile("mfcr %0, psr" : "=r"(intSave) : : "memory");
    return !(intSave & (1 << INT_OFFSET));
}

STATIC UINT32 HwiUnmask(HWI_HANDLE_T hwiNum)
{
    UINT32 intSave;

    intSave = LOS_IntLock();
    VIC_REG->ISER[hwiNum / 32] = (UINT32)(1UL << (hwiNum % 32));
    VIC_REG->ISSR[hwiNum / 32] = (UINT32)(1UL << (hwiNum % 32));
    LOS_IntRestore(intSave);

    return LOS_OK;
}

STATIC UINT32 HwiSetPriority(HWI_HANDLE_T hwiNum, UINT8 priority)
{
    UINT32 intSave;

    intSave = LOS_IntLock();
    VIC_REG->IPR[hwiNum / PRI_PER_REG] |= (((priority << PRI_OFF_IN_REG) << (hwiNum % PRI_PER_REG)) * PRI_OFF_PER_INT);
    LOS_IntRestore(intSave);

    return LOS_OK;
}

STATIC UINT32 HwiMask(HWI_HANDLE_T hwiNum)
{
    UINT32 intSave;

    intSave = LOS_IntLock();
    VIC_REG->ICER[hwiNum / OS_SYS_VECTOR_CNT] = (UINT32)(1UL << (hwiNum % OS_SYS_VECTOR_CNT));
    LOS_IntRestore(intSave);

    return LOS_OK;
}

STATIC UINT32 HwiPending(HWI_HANDLE_T hwiNum)
{
    UINT32 intSave;

    intSave = LOS_IntLock();
    VIC_REG->ISPR[hwiNum / OS_SYS_VECTOR_CNT] = (UINT32)(1UL << (hwiNum % OS_SYS_VECTOR_CNT));
    LOS_IntRestore(intSave);

    return LOS_OK;
}

STATIC UINT32 HwiClear(HWI_HANDLE_T hwiNum)
{
    VIC_REG->ICPR[hwiNum / OS_SYS_VECTOR_CNT] = (UINT32)(1UL << (hwiNum % OS_SYS_VECTOR_CNT));

    return LOS_OK;
}

/* ****************************************************************************
 Function    : HwiNumGet
 Description : Get an interrupt number
 Input       : None
 Output      : None
 Return      : Interrupt Indexes number
 **************************************************************************** */
STATIC UINT32 HwiNumGet(VOID)
{
    return (HalGetPsr() >> PSR_VEC_OFFSET) & MASK_8_BITS;
}

STATIC UINT32 HwiCreate(HWI_HANDLE_T hwiNum, HWI_PRIOR_T hwiPrio)
{
    HwiSetPriority(hwiNum, (UINT8)hwiPrio);
    HwiUnmask(hwiNum);
    return LOS_OK;
}

STATIC HwiControllerOps g_archHwiOps = {
    .enableIrq      = HwiUnmask,
    .disableIrq     = HwiMask,
    .setIrqPriority = HwiSetPriority,
    .getCurIrqNum   = HwiNumGet,
    .triggerIrq     = HwiPending,
    .clearIrq       = HwiClear,
    .createIrq      = HwiCreate,
};

HwiControllerOps *ArchIntOpsGet(VOID)
{
    return &g_archHwiOps;
}

