#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "osal/string.h"
#include "osal/task.h"
#include <csi_kernel.h>
#include <k_api.h>

extern void *console_handle;

void os_kernel_init(void)
{
    csi_kernel_init();
}

void os_kernel_start(void)
{
    csi_kernel_start();
}

void os_console_uart(void *uart)
{
    console_handle = uart;
}

static void csi_intrpt_exit_do(uint32 irqn)
{
    csi_kernel_intrpt_exit();
    if (__in_disable_irq(__get_PSR())) {
        os_printf(KERN_ERR"!!! sys_irq%d disable irq\r\n", irqn);
        __enable_irq();
    }
}

void OS_INTRPT_ENTER(int irqn)
{
    csi_kernel_intrpt_enter();
}

void OS_INTRPT_EXIT(int irqn)
{
    csi_intrpt_exit_do(irqn);
}

