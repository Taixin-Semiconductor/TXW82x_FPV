#ifndef __SDFUNC_WORK_H
#define __SDFUNC_WORK_H
#include "osal/sleep.h"
#include "osal/work.h"
#include "osal/irq.h"
#include "basic_include.h"
#include "lib/multimedia/msi.h"
int32 sd_fb_write_work(struct framebuff *fb, const char *filename, uint8_t ishid);
void sd_workqueue_init(uint16 pri, void *stack, uint16 stack_size);
#endif