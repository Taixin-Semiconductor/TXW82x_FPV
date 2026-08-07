#ifndef __OS_SEMAPHORE_H
#define __OS_SEMAPHORE_H

#ifdef __cplusplus
extern "C" {
#endif

struct os_semaphore {
    uint32 magic;
    void  *hdl;
};

typedef struct os_semaphore os_semaphore_t;

int32 os_sema_init(os_semaphore_t *sem, int32 val);
int32 os_sema_del(os_semaphore_t *sem);
int32 os_sema_down(os_semaphore_t *sem, int32 tmo_ms);
int32 os_sema_up(os_semaphore_t *sem);
int32 os_sema_count(os_semaphore_t *sem);
void os_sema_eat(os_semaphore_t *sem);

#ifdef __cplusplus
}
#endif
#endif

