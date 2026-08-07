#ifndef __OS_IRQ_H_
#define __OS_IRQ_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*irq_handle)(void *data);
struct sys_hwirq {
    void *data;
    irq_handle handle;
#ifdef SYS_IRQ_STAT
    uint32 trig_cnt;
    uint16 max, min;
    uint32 tot_cycle;
#endif
};

#ifdef CONFIG_SLEEP
__dsleeptext void irq_enable(uint32 irq);
__dsleeptext void irq_disable(uint32 irq);
__dsleeptext uint32 disable_irq(void);
__dsleeptext void enable_irq(uint32 flag);
#else
void irq_enable(uint32 irq);
void irq_disable(uint32 irq);
uint32 disable_irq(void);
void enable_irq(uint32 flag);
#endif

void irq_priority(uint32 irq, uint32 prio);
int32 request_irq(uint32 irq_num, irq_handle handle, void *data);
int32 release_irq(uint32 irq_num);
uint32 sysirq_time(void);

#ifdef __cplusplus
}
#endif
#endif
