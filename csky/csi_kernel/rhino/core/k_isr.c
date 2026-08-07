#include "sys_config.h"
#include "tx_platform.h"
#include <csi_kernel.h>
#include <k_api.h>
#include <csi_core.h>
#include <stdio.h>

#if RHINO_CONFIG_ISR_TASK
#define RB_NPOS(rb, pos, i) ({ \
        uint32 __pos__ = (rb)->pos;\
        ((__pos__+(i)>=(rb)->qsize) ? (__pos__+(i)-(rb)->qsize) : (__pos__+(i))); \
    })

/* rpos == wpos 表示 rbuffer 为空 */
#define RB_EMPTY(rb) ((rb)->wpos == (rb)->rpos)

/* wpos+1 == rpos 表示 rbuffer 已满 */
#define RB_FULL(rb) ({ \
        uint32 _rpos_ = ((rb)->rpos);\
        uint32 _wpos_ = ((rb)->wpos)+1;\
        ((_wpos_>=(rb)->qsize) ? (_wpos_-(rb)->qsize) : (_wpos_)) == (_rpos_);\
    })

/* rbuffer 中未被读取的数据长度 */
#define RB_COUNT(rb) ({ \
        uint32 _rpos_ = ((rb)->rpos);\
        uint32 _wpos_ = ((rb)->wpos);\
        ((_rpos_<=_wpos_)? (_wpos_-_rpos_): ((rb)->qsize-_rpos_+_wpos_))\
    })

/* rbuffer中剩余空间长度 */
#define RB_IDLE(rb) ({ \
        uint32 _rpos_ = ((rb)->rpos);\
        uint32 _wpos_ = ((rb)->wpos);\
        ((_wpos_<_rpos_)? (_rpos_-_wpos_-1): ((rb)->qsize-_wpos_+_rpos_-1))\
    })

/*get a value from ringbuffer*/
#define RB_GET(rb, val) do{\
        if(!RB_EMPTY(rb)){\
            val = (rb)->rbq[(rb)->rpos];\
            (rb)->rpos = RB_NPOS((rb), rpos, 1);\
        }\
    } while(0)

/*set a value into ringbuffer*/
#define RB_SET(rb, val) do{\
        if(!RB_FULL(rb)){\
            (rb)->rbq[(rb)->wpos] = val;\
            (rb)->wpos = RB_NPOS((rb), wpos, 1);\
        }else{ k_err_proc(RHINO_INTRPT_ISR_OVF); }\
    } while (0)

typedef struct {
    void (*hdl)(void *data);
    void *data;
} k_isr_data;

struct k_isr_buff {
    uint8_t rpos, wpos, qsize, busy;
    k_isr_data *rbq;
};

static void ISR_task(void *pa)
{
    kstat_t     ret;
    k_isr_data  isr;
    struct k_isr_buff *rb = (struct k_isr_buff *)g_isr_buff;

    while (1) {
        ret = krhino_sem_take(&g_isr_sem, 0xffffffff);
        if (ret != RHINO_SUCCESS) {
            k_err_proc(RHINO_SYS_FATAL_ERR);
        }

        isr.hdl = 0;
        RB_GET(rb, isr);
        if (isr.hdl) {
            isr.hdl(isr.data);
        }
    }
}

__init void kisr_init(void)
{
    struct k_isr_buff *rb = (struct k_isr_buff *)g_isr_buff;
    rb->wpos = 0;
    rb->rpos = 0;
    rb->qsize = RHINO_CONFIG_ISR_BUFF_SIZE;
    rb->rbq   = (k_isr_data *)(rb + 1);
    krhino_sem_create(&g_isr_sem, "isr_sem", 0);
    krhino_task_create(&g_isr_task, "isr", NULL,
                       RHINO_CONFIG_ISR_TASK_PRI, 0u, g_isr_task_stack,
                       RHINO_CONFIG_ISR_TASK_STACK_SIZE, ISR_task, 1u);
}

void isr_run(void (*hdl)(void *data), void *data)
{
    k_isr_data isr = { hdl, data };
    struct k_isr_buff *rb = (struct k_isr_buff *)g_isr_buff;
    uint32_t flag = __disable_irq();
    RB_SET(rb, isr);
    if (!flag) __enable_irq();
    krhino_sem_give(&g_isr_sem);
}

#endif

