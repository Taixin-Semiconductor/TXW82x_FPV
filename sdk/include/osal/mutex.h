#ifndef _OS_MUTEX_H_
#define _OS_MUTEX_H_

#ifdef __cplusplus
extern "C" {
#endif

struct os_mutex {
    uint32 magic;
    void  *hdl;
};
typedef struct os_mutex os_mutex_t;

int32 os_mutex_init(os_mutex_t *mutex);
int32 os_mutex_lock(os_mutex_t *mutex, int32 tmo);
int32 os_mutex_unlock(os_mutex_t *mutex);
int32 os_mutex_del(os_mutex_t *mutex);
void *os_mutex_owner(os_mutex_t *mutex);

#ifdef __cplusplus
}
#endif

#endif
