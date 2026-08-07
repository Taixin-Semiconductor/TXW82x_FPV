#ifndef __FLASH_READ_DEMO_H
#define __FLASH_READ_DEMO_H
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "errno.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "osal/task.h"
#include "osal/string.h"
#include "osal/semaphore.h"
#include "hal/spi.h"
#include "hal/spi_nor.h"
#include "hal/crc.h"
#include "lib/syscfg/syscfg.h"

#include "dev/spi/hgspi_xip.h"


uint8_t flash_read_uuid_demo();
#endif