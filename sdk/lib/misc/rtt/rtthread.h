#ifndef _TXW_RTT_DEF_H_
#define _TXW_RTT_DEF_H_
#include "basic_include.h"
#include "hal/netdev.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RTT_OS

#define rt_device dev_obj

typedef struct dev_obj *rt_device_t;
typedef uint8   rt_uint8_t;
typedef int8    rt_int8_t;
typedef uint32  rt_uint32_t;
typedef int32   rt_int32_t;
typedef uint16  rt_uint16_t;
typedef int16   rt_int16_t;
typedef uint32  thread_t;
typedef size_t  rt_size_t;
typedef ssize_t rt_ssize_t;
typedef int32   rt_err_t;
typedef ulong   rt_off_t;
typedef int     rt_bool_t;
typedef long    rt_base_t;
typedef ulong   rt_ubase_t;
typedef ulong   rt_tick_t;
typedef uint64  rt_uint64_t;

typedef os_task_t *rt_thread_t;
typedef os_msgqueue_t *rt_mq_t;
typedef os_event_t    *rt_event_t;
typedef os_semaphore_t *rt_sem_t;
typedef os_timer_t    *rt_timer_t;

#define rt_event        os_event
#define rt_messagequeue os_msgqueue
#define rt_timer        os_timer
#define rt_semaphore    os_semaphore
#define rt_thread       os_task
#define rt_ringbuffer   rbuffer

#define rt_hw_interrupt_disable()    disable_irq()
#define rt_hw_interrupt_enable(f)    enable_irq(f)
#define rt_tick_from_millisecond(m)  os_msecs_to_jiffies(m)

#define INIT_PREV_EXPORT(f)
#define RT_TICK_PER_SECOND  OS_SYSTICK_HZ
#define RT_WAITING_FOREVER  osWaitForever

#define rt_inline       inline
#define rt_align        __aligned
#define rt_weak         __weak
#define RT_ASSERT       ASSERT
#define RT_ERROR        RET_ERR
#define RT_EOK          RET_OK
#define RT_TRUE         1
#define RT_FALSE        0
#define RT_NULL         NULL
#define RT_ENOMEM       ENOMEM
#define RT_EIO          EIO
#define RT_ENOSYS       ENOSYS

#define rt_memset       os_memset
#define rt_memcpy       os_memcpy
#define rt_malloc       os_malloc
#define rt_zalloc       os_zalloc
#define rt_free         os_free
#define rt_strlen       os_strlen

#define rt_kprintf(fmt, ...)       os_printf(fmt, ##__VA_ARGS__)
#define LOG_D(fmt, ...)            //os_printf(fmt"\r\n", ##__VA_ARGS__)
#define LOG_E(fmt, ...)            os_printf(fmt"\r\n", ##__VA_ARGS__)
#define LOG_W(fmt, ...)            os_printf(fmt"\r\n", ##__VA_ARGS__)
#define LOG_I(fmt, ...)            os_printf(fmt"\r\n", ##__VA_ARGS__)

#define rt_device_init(dev)
#define rt_device_find()
#define rt_device_open()
#define rt_device_control()
#define rt_device_write()
#define rt_device_close()

#define eth_device netdev

#define RT_IPC_FLAG_FIFO     (0)
#define RT_EVENT_FLAG_OR     OS_EVENT_WMODE_OR
#define RT_EVENT_FLAG_CLEAR  OS_EVENT_WMODE_CLEAR

#include "rtservice.h"

#define rt_set_errno(err)

/**
 * clock & timer macros
 */
#define RT_TIMER_FLAG_DEACTIVATED       0x0             /**< timer is deactive */
#define RT_TIMER_FLAG_ACTIVATED         0x1             /**< timer is active */
#define RT_TIMER_FLAG_ONE_SHOT          0x0             /**< one shot timer */
#define RT_TIMER_FLAG_PERIODIC          0x2             /**< periodic timer */

#define RT_TIMER_FLAG_HARD_TIMER        0x0             /**< hard timer,the timer's callback function will be called in tick isr. */
#define RT_TIMER_FLAG_SOFT_TIMER        0x4             /**< soft timer,the timer's callback function will be called in timer thread. */

#define RT_TIMER_CTRL_SET_TIME          0x0             /**< set timer control command */
#define RT_TIMER_CTRL_GET_TIME          0x1             /**< get timer control command */
#define RT_TIMER_CTRL_SET_ONESHOT       0x2             /**< change timer to one shot */
#define RT_TIMER_CTRL_SET_PERIODIC      0x3             /**< change timer to periodic */

#ifndef RT_TIMER_SKIP_LIST_LEVEL
#define RT_TIMER_SKIP_LIST_LEVEL          1
#endif

/* 1 or 3 */
#ifndef RT_TIMER_SKIP_LIST_MASK
#define RT_TIMER_SKIP_LIST_MASK         0x3
#endif

#ifndef RT_ALIGN_SIZE
#define RT_ALIGN_SIZE 4
#endif

#define INIT_BOARD_EXPORT(f)

#endif

rt_thread_t rt_thread_create(const char *name, void (*entry)(void *parameter), void *parameter,
                             rt_uint32_t stack_size, rt_uint8_t  priority, rt_uint32_t tick);
rt_err_t rt_thread_delete(rt_thread_t thread);
rt_err_t rt_thread_init(struct rt_thread *thread, const char *name, void (*entry)(void *parameter),
                        void *parameter, void *stack_start, rt_uint32_t stack_size,
                        rt_uint8_t priority, rt_uint32_t tick);
rt_err_t rt_thread_detach(rt_thread_t thread);
rt_thread_t rt_thread_self(void);
rt_thread_t rt_thread_find(char *name);
rt_err_t rt_thread_startup(rt_thread_t thread);
rt_err_t rt_thread_yield(void);
rt_err_t rt_thread_delay(rt_tick_t tick);
rt_err_t rt_thread_delay_until(rt_tick_t *tick, rt_tick_t inc_tick);
rt_err_t rt_thread_mdelay(rt_int32_t ms);
rt_err_t rt_thread_control(rt_thread_t thread, int cmd, void *arg);
rt_err_t rt_thread_suspend(rt_thread_t thread);
rt_err_t rt_thread_resume(rt_thread_t thread);
void rt_thread_timeout(void *parameter);
void rt_enter_critical(void);
void rt_exit_critical(void);

rt_err_t rt_event_init(rt_event_t event, const char *name, rt_uint8_t flag);
rt_err_t rt_event_detach(rt_event_t event);
rt_event_t rt_event_create(const char *name, rt_uint8_t flag);
rt_err_t rt_event_delete(rt_event_t event);
rt_err_t rt_event_send(rt_event_t event, rt_uint32_t set);
rt_err_t rt_event_recv(rt_event_t   event, rt_uint32_t  set, rt_uint8_t opt, rt_int32_t timeout, rt_uint32_t *recved);

rt_err_t rt_mq_init(rt_mq_t mq, const char *name, void *msgpool, rt_size_t msg_size, rt_size_t pool_size, rt_uint8_t flag);
rt_err_t rt_mq_detach(rt_mq_t mq);
rt_mq_t rt_mq_create(const char *name, rt_size_t msg_size, rt_size_t max_msgs, rt_uint8_t flag);
rt_err_t rt_mq_delete(rt_mq_t mq);
rt_err_t rt_mq_send_wait(rt_mq_t mq, const void *buffer, rt_size_t size, rt_int32_t timeout);
rt_err_t rt_mq_send(rt_mq_t mq, const void *buffer, rt_size_t size);
rt_err_t rt_mq_urgent(rt_mq_t mq, const void *buffer, rt_size_t size);
rt_err_t rt_mq_recv(rt_mq_t mq, void *buffer, rt_size_t size, rt_int32_t timeout);

rt_err_t rt_sem_init(rt_sem_t sem, const char *name, rt_uint32_t value, rt_uint8_t flag);
rt_err_t rt_sem_detach(rt_sem_t sem);
rt_sem_t rt_sem_create(const char *name, rt_uint32_t value, rt_uint8_t flag);
rt_err_t rt_sem_delete(rt_sem_t sem);
rt_err_t rt_sem_take(rt_sem_t sem, rt_int32_t time);
rt_err_t rt_sem_trytake(rt_sem_t sem);
rt_err_t rt_sem_release(rt_sem_t sem);

void rt_ringbuffer_init(struct rt_ringbuffer *rb, rt_uint8_t *pool, rt_int16_t size);
void rt_ringbuffer_reset(struct rt_ringbuffer *rb);
void rt_ringbuffer_destroy(struct rt_ringbuffer *rb);
rt_size_t rt_ringbuffer_put(struct rt_ringbuffer *rb, const rt_uint8_t *ptr, rt_uint16_t length);
rt_size_t rt_ringbuffer_put_force(struct rt_ringbuffer *rb, const rt_uint8_t *ptr, rt_uint16_t length);
rt_size_t rt_ringbuffer_putchar(struct rt_ringbuffer *rb, const rt_uint8_t ch);
rt_size_t rt_ringbuffer_putchar_force(struct rt_ringbuffer *rb, const rt_uint8_t ch);
rt_size_t rt_ringbuffer_get(struct rt_ringbuffer *rb, rt_uint8_t *ptr, rt_uint16_t length);
rt_size_t rt_ringbuffer_getchar(struct rt_ringbuffer *rb, rt_uint8_t *ch);
rt_size_t rt_ringbuffer_data_len(struct rt_ringbuffer *rb);

void rt_timer_init(rt_timer_t timer, const char *name, void (*timeout)(void *parameter), void *parameter, rt_tick_t time, rt_uint8_t flag);
rt_err_t rt_timer_detach(rt_timer_t timer);
rt_timer_t rt_timer_create(const char *name, void (*timeout)(void *parameter), void *parameter, rt_tick_t time, rt_uint8_t flag);
rt_err_t rt_timer_delete(rt_timer_t timer);
rt_err_t rt_timer_start(rt_timer_t timer);
rt_err_t rt_timer_stop(rt_timer_t timer);

#ifdef __cplusplus
}
#endif
#endif

