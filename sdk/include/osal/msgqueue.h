
#ifndef _OS_MSG_QUEUE_H_
#define _OS_MSG_QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

struct os_msgqueue {
    uint32 magic;
    void  *hdl;
};
typedef struct os_msgqueue os_msgqueue_t;

int32 os_msgq_init(os_msgqueue_t *msgq, int32 size);
uint32 os_msgq_get(os_msgqueue_t *msgq, int32 tmo_ms);
uint32 os_msgq_get2(struct os_msgqueue *msgq, int32 tmo_ms, int32 *err);
int32 os_msgq_put(os_msgqueue_t *msgq, uint32 data, int32 tmo_ms);
int32 os_msgq_del(os_msgqueue_t *msgq);
int32 os_msgq_cnt(os_msgqueue_t *msgq);
int32 os_msgq_put_head(os_msgqueue_t *msgq, uint32 data, int32 tmo_ms);

#ifndef OS_MSGQ_DEF
#define OS_MSGQ_DEF(name, size) os_msgqueue_t name;
#endif

#ifdef __cplusplus
}
#endif
#endif

