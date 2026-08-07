#ifndef __OS_CONDV_H
#define __OS_CONDV_H
#include "osal/atomic.h"

#ifdef __cplusplus
extern "C" {
#endif

struct os_condv {
    uint32 magic;
    atomic_t waitings;
    void    *sema;
};
typedef struct os_condv os_condv_t;

int32 os_condv_init(os_condv_t *cond);

int32 os_condv_broadcast(os_condv_t *cond);
int32 os_condv_signal(os_condv_t *cond);

int32 os_condv_del(os_condv_t *cond);
int32 os_condv_wait(os_condv_t *cond, os_mutex_t *mutex, uint32 tmo_ms);

#ifdef __cplusplus
}
#endif
#endif

