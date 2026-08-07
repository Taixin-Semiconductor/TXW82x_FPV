#include "basic_include.h"

#ifdef CSKY_OS
#include "csi_core.h"
#include <k_api.h>
#include <csi_kernel.h>
#endif

#ifdef OHOS
#include "los_task.h"
#endif

__bobj uint8_t assert_holdup;

void assert_internal(const char *__function, unsigned int __line, const char *__assertion, void *lr)
{
    int print_loop = 0;
    uint32_t in_int = __in_interrupt();
#ifdef CSKY_OS
#define OS_TSKNAME task_name
    ktask_t *task = (ktask_t *)os_task_current();
#endif
#ifdef OHOS
#define OS_TSKNAME taskName
    LosTaskCB *task = (LosTaskCB *)os_task_current();
#endif

    disable_print(0);
    if (assert_holdup) {
        disable_irq();
        mcu_watchdog_timeout(0); //disable watchdog
        jtag_map_set(1);
    }

    os_printf(KERN_ERR"[%s:%p]: assertation \"%s\" failed: function: %s, line %d, LR:%p\r\n",
              (in_int ? "Interrupt" : task->OS_TSKNAME), (in_int ? 0 : task),
              __assertion, __function, __line, lr);

    sys_errlog_flush(0xffffffff, 0, 0);

    if (!in_int && task) {
        os_task_dump(task, 0);
    }

    do {
        os_printf(KERN_ERR"[%s:%p]: assertation \"%s\" failed: function: %s, line %d, LR:%p\r\n",
                  (in_int ? "Interrupt" : task->OS_TSKNAME), (in_int ? 0 : task),
                  __assertion, __function, __line, lr);
        delay_us(1000 * 1000);
    } while (assert_holdup || print_loop++ < 5);

    mcu_reset();
}

