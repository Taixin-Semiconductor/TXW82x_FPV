#ifndef _HGIC_POSIX_PTHREAD_H_
#define _HGIC_POSIX_PTHREAD_H_
#include "typesdef.h"
#include "osal/timer.h"

#define LIB_PTHREAD_DEF
#ifdef LIB_PTHREAD_DEF
#include <pthread.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define pthread_dbg(fmt, ...) os_printf(KERN_NOTICE fmt, ##__VA_ARGS__)
#define pthread_err(fmt, ...) os_printf(KERN_ERR"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#ifndef PTHREAD_CREATE_JOINABLE
#define PTHREAD_CREATE_JOINABLE     0x00
#endif

#ifndef PTHREAD_CREATE_DETACHED
#define PTHREAD_CREATE_DETACHED     0x01
#endif

#ifndef TIMER_ABSTIME
#define TIMER_ABSTIME      0x4
#endif

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME     0x1
#endif

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC    0x4
#endif

#ifndef LIB_PTHREAD_DEF
struct sched_param {
    int sched_priority;
};

/* thread attributes */
typedef struct {
    int                       detachstate;   // 线程的分离状态
    //int                       schedpolicy;   // 线程调度策略
    struct sched_param        schedparam;    // 线程的调度参数
    //int                       inheritsched;  // 线程的继承性
    //int                       scope;         // 线程的作用域
    //size_t                    guardsize;     // 线程栈末尾的警戒缓冲区大小
    //int                       stackaddr_set; // 线程的栈设置
    //void*                     stackaddr;     // 线程栈的位置
    unsigned int              stacksize;     // 线程栈的大小
} pthread_attr_t;
#endif

struct mq_attr {
    long mq_flags;   /**< Message queue flags. */
    long mq_maxmsg;  /**< Maximum number of messages. */
    long mq_msgsize; /**< Maximum message size. */
    long mq_curmsgs; /**< Number of messages currently queued. */
};

#ifndef PTHREAD_PROCESS_PRIVATE
#define PTHREAD_PROCESS_PRIVATE  0
#endif

#ifndef PTHREAD_PROCESS_SHARED
#define PTHREAD_PROCESS_SHARED  1
#endif

#ifndef PTHREAD_SCOPE_PROCESS
#define PTHREAD_SCOPE_PROCESS    0
#endif

#ifndef PTHREAD_SCOPE_SYSTEM
#define PTHREAD_SCOPE_SYSTEM     1
#endif

#ifndef LIB_PTHREAD_DEF
enum {
    PTHREAD_CANCEL_ASYNCHRONOUS = 0,
    PTHREAD_CANCEL_ENABLE,
    PTHREAD_CANCEL_DEFERRED,
    PTHREAD_CANCEL_DISABLE,
    PTHREAD_CANCELED
};

enum {
    PTHREAD_MUTEX_NORMAL = 0,
    PTHREAD_MUTEX_RECURSIVE = 1,
    PTHREAD_MUTEX_ERRORCHECK = 2,
    PTHREAD_MUTEX_ERRORCHECK_NP = PTHREAD_MUTEX_ERRORCHECK,
    PTHREAD_MUTEX_RECURSIVE_NP  = PTHREAD_MUTEX_RECURSIVE,
    PTHREAD_MUTEX_DEFAULT = PTHREAD_MUTEX_NORMAL
};

/* init value for pthread_once_t */
#define PTHREAD_ONCE_INIT       0

enum {
    PTHREAD_PRIO_INHERIT = 0,
    PTHREAD_PRIO_NONE,
    PTHREAD_PRIO_PROTECT,
};
#endif

#ifndef PTHREAD_COND_INITIALIZER
#define PTHREAD_COND_INITIALIZER   0
#endif

#ifndef PTHREAD_MUTEX_INITIALIZER
#define PTHREAD_MUTEX_INITIALIZER   0
#endif

#define PTHREAD_RWLOCK_INITIALIZER  {-1, 0 }

typedef int  pid_t;

#ifndef LIB_PTHREAD_DEF
typedef long pthread_t;
typedef long pthread_condattr_t;
typedef long pthread_mutexattr_t;

typedef int pthread_key_t;
typedef int pthread_once_t;

typedef long pthread_mutex_t;
typedef long pthread_cond_t;
#endif

typedef long pthread_rwlockattr_t;
typedef long pthread_barrierattr_t;

typedef long sem_t;
typedef long mqd_t;


typedef struct {
    int count;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
} pthread_barrier_t;

struct pthread_rwlock {
    pthread_rwlockattr_t attr;

    pthread_mutex_t      rw_mutex;          /* basic lock on this struct */
    pthread_cond_t       rw_condreaders;    /* for reader threads waiting */
    pthread_cond_t       rw_condwriters;    /* for writer threads waiting */

    int rw_nwaitreaders;    /* the number of reader threads waiting */
    int rw_nwaitwriters;    /* the number of writer threads waiting */
    int rw_refcount;        /* 0: unlocked, -1: locked by writer, > 0 locked by n readers */
};
typedef struct pthread_rwlock pthread_rwlock_t;


#define pthread_spin_init(s, p)    pthread_mutex_init(s, NULL)
#define pthread_spin_destroy(s)    pthread_mutex_destroy(s)
#define pthread_spin_lock(s)       pthread_mutex_lock(s)
#define pthread_spin_trylock(s)    pthread_mutex_trylock(s)
#define pthread_spin_unlock(s)     pthread_mutex_unlock(s)


#define PTHREAD_KEY_MAX             8
typedef struct {
    int is_used;
    void (*destructor)(void *parameter);
} pthread_key_data_t;

#define SIGEV_NONE      0 /**< No asynchronous notification is delivered when the event of interest occurs. */
#define SIGEV_SIGNAL    1 /**< A queued signal, with an application-defined value, is generated when the event of interest occurs. Not supported. */
#define SIGEV_THREAD    2 /**< A notification function is called to perform notification. */

union sigval {
    int sival_int;    /**< Integer signal value. */
    void *sival_ptr;  /**< Pointer signal value. */
};

struct sigevent {
    int sigev_notify;                                 /**< Notification type. A value of SIGEV_SIGNAL is not supported. */
    int sigev_signo;                                  /**< Signal number. This member is ignored. */
    union sigval sigev_value;                         /**< Signal value. Only the sival_ptr member is used. */
    void (* sigev_notify_function)(union sigval);     /**< Notification function. */
    pthread_attr_t *sigev_notify_attributes;          /**< Notification attributes. */
};

#define pthread_equal(t1, t2) (t1==t2)

unsigned int pthread_timespec_delta(const struct timespec *abstime);

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start)(void *), void *arg);
pthread_t pthread_self(void);
int pthread_join(pthread_t thread, void **value_ptr);
void pthread_exit(void *value_ptr);
int pthread_detach(pthread_t thread);
void *pthread_tls(pthread_t thread, int create, int size);
int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));

#if !defined(LIB_PTHREAD_DEF) || !defined(_POSIX_THREADS)
int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int state);
int pthread_attr_getdetachstate(pthread_attr_t const *attr, int *state);
int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy);
int pthread_attr_getschedpolicy(pthread_attr_t const *attr, int *policy);
int pthread_attr_setschedparam(pthread_attr_t *attr, struct sched_param const *param);
int pthread_attr_getschedparam(pthread_attr_t const *attr, struct sched_param *param);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stack_size);
int pthread_attr_getstacksize(pthread_attr_t const *attr, size_t *stack_size);
int pthread_attr_setstackaddr(pthread_attr_t *attr, void *stack_addr);
int pthread_attr_getstackaddr(pthread_attr_t const *attr, void **stack_addr);
int pthread_attr_setstack(pthread_attr_t *attr, void *stack_base, size_t stack_size);
int pthread_attr_getstack(pthread_attr_t const *attr, void **stack_base, size_t *stack_size);
int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guard_size);
int pthread_attr_getguardsize(pthread_attr_t const *attr, size_t *guard_size);
int pthread_attr_setscope(pthread_attr_t *attr, int scope);
int pthread_attr_getscope(pthread_attr_t const *attr, int *scope);
#endif

int pthread_barrierattr_destroy(pthread_barrierattr_t *attr);
int pthread_barrierattr_init(pthread_barrierattr_t *attr);
int pthread_barrierattr_getpshared(const pthread_barrierattr_t *attr, int *pshared);
int pthread_barrierattr_setpshared(pthread_barrierattr_t *attr, int pshared);
int pthread_barrier_destroy(pthread_barrier_t *barrier);
int pthread_barrier_init(pthread_barrier_t *barrier, const pthread_barrierattr_t *attr, int count);
int pthread_barrier_wait(pthread_barrier_t *barrier);

#if !defined(LIB_PTHREAD_DEF) || !defined(_POSIX_THREADS)
int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_broadcast(pthread_cond_t *cond);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_condattr_destroy(pthread_condattr_t *attr);
int pthread_condattr_init(pthread_condattr_t *attr);
int pthread_condattr_getclock(const pthread_condattr_t *attr, clockid_t         *clock_id);
int pthread_condattr_setclock(pthread_condattr_t *attr, clockid_t         clock_id);
int pthread_condattr_getpshared(const pthread_condattr_t *attr, int *pshared);
int pthread_condattr_setpshared(pthread_condattr_t *attr, int pshared);

int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *abstime);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type);
int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
#endif

int pthread_rwlockattr_init(pthread_rwlockattr_t *attr);
int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr);
int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *attr, int *pshared);
int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *attr, int pshared);
int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr);
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_timedrdlock(pthread_rwlock_t *rwlock, const struct timespec *abstime);
int pthread_rwlock_timedwrlock(pthread_rwlock_t *rwlock, const struct timespec *abstime);
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);

#ifndef LIB_PTHREAD_DEF
int sched_yield(void);
int sched_get_priority_min(int policy);
int sched_get_priority_max(int policy);
int sched_setscheduler(pid_t pid, int policy);
int sched_setscheduler(pid_t pid, int policy, const struct sched_param *param);
#endif

int sem_destroy(sem_t *sem);
int sem_getvalue(sem_t *sem, int *sval);
int sem_init(sem_t *sem, int pshared, unsigned value);
int sem_post(sem_t *sem);
int sem_timedwait(sem_t *sem, const struct timespec *abstime);
int sem_trywait(sem_t *sem);
int sem_wait(sem_t *sem);

int timer_create(clockid_t clockid, struct sigevent *evp, timer_t *timerid);
int timer_delete(timer_t timerid);
int timer_getoverrun(timer_t timerid);
int timer_settime(timer_t timerid, int flags, const struct itimerspec *value, struct itimerspec *ovalue);
int timer_gettime(timer_t timerid, struct itimerspec *value);

int mq_getattr(mqd_t mqdes, struct mq_attr *mqstat);
int mq_close(mqd_t mqdes);
mqd_t mq_open(const char *name, int oflag, int mode, struct mq_attr *attr);
int mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio);
int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned msg_prio);
int mq_timedreceive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned *msg_prio, const struct timespec *abstime);
int mq_timedsend(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio, const struct timespec *abstime);
int mq_unlink(const char *name);

void pthread_key_destroy(void *tls);
void *pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);
int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
int pthread_key_delete(pthread_key_t key);

#ifdef __cplusplus
}
#endif
#endif

