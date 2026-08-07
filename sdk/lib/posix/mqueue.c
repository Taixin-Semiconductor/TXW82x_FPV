#include "sys_config.h"
#include "typesdef.h"
#include "errno.h"
#include "osal/semaphore.h"
#include "osal/irq.h"
#include "osal/task.h"
#include "osal/string.h"
#include "osal/timer.h"
#include "osal/mutex.h"
#include "osal/msgqueue.h"
#include "lib/common/rbuffer.h"
#include "lib/posix/pthread.h"

#ifdef TXWSDK_POSIX

struct pthread_mq {
    char name[32];
    uint32 unlink: 1, rev: 31;
    struct pthread_mq *next;
    os_msgqueue_t msgq;
};

struct pthread_mq_data {
    void  *data;
    uint32 len;
};

static struct pthread_mq *mq_list = NULL;

static struct pthread_mq *mq_list_find(const char *name, uint8 unlink)
{
    struct pthread_mq *prev = NULL;
    struct pthread_mq *next = NULL;
    uint32 flag = disable_irq();
    next = mq_list;
    while (next) {
        if (os_strcmp(next->name, name) == 0) {
            if (unlink) {
                if (prev == NULL) {
                    mq_list = next->next;
                } else {
                    prev->next = next->next;
                }
                next->unlink = 1;
            }
            break;
        }
        prev = next;
        next = next->next;
    }
    enable_irq(flag);
    return next;
}

static void mq_list_add(struct pthread_mq *mq)
{
    uint32 flag = disable_irq();
    mq->next = mq_list;
    mq_list = mq;
    enable_irq(flag);
}

int mq_close(mqd_t mqdes)
{
    return 0;
}

int mq_getattr(mqd_t mqdes, struct mq_attr *mqstat)
{
    return 0;
}

mqd_t mq_open(const char *name, int oflag, int mode, struct mq_attr *attr)
{
    struct pthread_mq *mq = mq_list_find(name, 0);
    if (mq) {
        return (mqd_t)mq;
    } else {
        mq = (struct pthread_mq *)os_zalloc(sizeof(struct pthread_mq));
        if (mq == NULL) {
            pthread_err("alloc fail\r\n");
            return -ENOMEM;
        }
        os_strncpy(mq->name, name, 31);
        os_msgq_init(&mq->msgq, attr ? attr->mq_msgsize : 32);
        mq_list_add(mq);
        return (mqd_t)mq;
    }
}

static uint32 _mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int tmo)
{
    struct pthread_mq_data *val;
    struct pthread_mq *mq = (struct pthread_mq *)mqdes;

    if (mq->unlink) {
        return -ENODEV;
    }

    val = (struct pthread_mq_data *)os_msgq_get(&mq->msgq, tmo);
    if (val) {
        if (msg_len >= val->len) {
            os_memcpy(msg_ptr, val->data, val->len);
        } else {
            pthread_err("msg_len(%d) < val_len(%d)\r\n", msg_len, val->len);
        }
        os_free(val->data);
        os_free(val);
        return 0;
    }
    return -1;
}

static int _mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int tmo)
{
    int ret;
    struct pthread_mq *mq = (struct pthread_mq *)mqdes;
    struct pthread_mq_data *val;

    if (mq->unlink) {
        return -ENODEV;
    }

    val = os_malloc(sizeof(struct pthread_mq_data));
    if (val) {
        val->data = os_memdup(msg_ptr, msg_len);
        if (val->data) {
            ret = os_msgq_put(&mq->msgq, (uint32)val, tmo);
            if (ret == 0) {
                return RET_OK;
            }
            os_free(val->data);
        } else {
            pthread_err("alloc fail\r\n");
        }
        os_free(val);
    } else {
        pthread_err("alloc fail\r\n");
    }
    return -ENOMEM;
}

int mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio)
{
    return _mq_receive(mqdes, msg_ptr, msg_len, osWaitForever);
}

int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned msg_prio)
{
    return _mq_send(mqdes, msg_ptr, msg_len, osWaitForever);
}

int mq_timedreceive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned *msg_prio, const struct timespec *abstime)
{
    return _mq_receive(mqdes, msg_ptr, msg_len, pthread_timespec_delta(abstime));
}

int mq_timedsend(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio, const struct timespec *abstime)
{
    return _mq_send(mqdes, msg_ptr, msg_len, pthread_timespec_delta(abstime));
}

int mq_unlink(const char *name)
{
    struct pthread_mq_data *val;
    struct pthread_mq *mq = mq_list_find(name, 1);
    if (mq) {
        while (os_msgq_cnt(&mq->msgq) > 0) {
            val = (struct pthread_mq_data *)os_msgq_get(&mq->msgq, 0);
            if (val) {
                os_free(val->data);
                os_free(val);
            }
        }
        os_msgq_del(&mq->msgq);
        os_free(mq);
        return 0;
    }
    return -ENODEV;
}

#endif
