#include "sys_config.h"
#include "typesdef.h"
#include "errno.h"
#include "osal/semaphore.h"
#include "osal/task.h"
#include "osal/string.h"
#include "osal/timer.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "lib/common/rbuffer.h"
#include "lib/posix/pthread.h"

#ifdef TXWSDK_POSIX

struct pthread_key_data {
    int is_used;
    void (*destructor)(void *args);
};
static struct pthread_key_data thread_keys[PTHREAD_KEY_MAX];

void pthread_key_destroy(void *tls)
{
    uint32 index;
    void **data = (void **)tls;

    if (tls == NULL) {
        return;
    }

    for (index = 0; index < PTHREAD_KEY_MAX; index++) {
        if (thread_keys[index].is_used) {
            if (data[index]) {
                thread_keys[index].destructor(data[index]);
            }
        }
    }
}

void *pthread_getspecific(pthread_key_t key)
{
    void **tls = (void **)pthread_tls(pthread_self(), 0, 0);
    if (tls && key < PTHREAD_KEY_MAX && thread_keys[key].is_used) {
        return tls[key];
    }
    return NULL;
}

int pthread_setspecific(pthread_key_t key, const void *value)
{
    void **tls = (void **)pthread_tls(pthread_self(), 1, PTHREAD_KEY_MAX * sizeof(void *));
    if (tls && key < PTHREAD_KEY_MAX && thread_keys[key].is_used) {
        tls[key] = (void *)value;
        return RET_OK;
    }
    return -EINVAL;
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
    uint32 index;
    uint32 flag = disable_irq();
    for (index = 0; index < PTHREAD_KEY_MAX; index ++) {
        if (thread_keys[index].is_used == 0) {
            thread_keys[index].is_used = 1;
            thread_keys[index].destructor = destructor;
            *key = index;
            enable_irq(flag);
            return 0;
        }
    }
    enable_irq(flag);
    return -EAGAIN;
}

int pthread_key_delete(pthread_key_t key)
{
    if (key >= PTHREAD_KEY_MAX) {
        return -EINVAL;
    }
    uint32 flag = disable_irq();
    thread_keys[key].is_used = 0;
    thread_keys[key].destructor = 0;
    enable_irq(flag);
    return RET_OK;
}

#endif
