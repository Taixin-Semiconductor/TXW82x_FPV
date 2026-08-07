#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/sleep.h"

void os_sleep_us(int us)
{
    uint32 ms = us / 1000;
    us = us % 1000;
    if (ms > 0) {
        os_sleep_ms(ms);
    }
    delay_us(us);
}

