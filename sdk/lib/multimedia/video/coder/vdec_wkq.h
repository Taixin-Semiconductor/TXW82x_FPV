#ifndef _VIDEO_DECODER_WKQ_H
#define _VIDEO_DECODER_WKQ_H
#include "basic_include.h"

//支持多次调用
int32 vdec_wkq_init(uint32 priority, void *stack, uint16 stack_size);

//支持多次调用
int32 vdec_wkq_deinit(void);

int32 vdec_work_run(struct os_work *work);

int32 vdec_work_delay_run(struct os_work *work, uint32 delay_ms);

#endif