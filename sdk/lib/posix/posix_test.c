#include "sys_config.h"
#include "typesdef.h"
#include "errno.h"
#include "osal/semaphore.h"
#include "osal/task.h"
#include "osal/string.h"
#include "osal/timer.h"
#include "osal/mutex.h"
#include "osal/msgqueue.h"
#include "lib/common/rbuffer.h"
#include "lib/posix/pthread.h"
#include "osal/sleep.h"

#ifdef TXWSDK_POSIX

pthread_cond_t c;
void posix_mutex_test(void)
{
    pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t m2;
    pthread_mutex_init(&m2, NULL);
    for (int i = 0; i < 100; ++i)
    {
        os_printf("posix_mutex_test %d\n", i);
        pthread_mutex_lock(&m1);
        pthread_mutex_unlock(&m1);

        pthread_mutex_lock(&m2);
        pthread_mutex_unlock(&m2);
    }
}

void posix_cond_test(void)
{
    pthread_mutex_t m;
    pthread_mutex_init(&m, NULL);

    pthread_cond_init(&c, NULL);

    for (int i = 0; i < 100; ++i)
    {
        os_printf("posix_mutex_test %d\n", i);
        pthread_mutex_lock(&m);
        pthread_cond_wait(&c, &m);
        pthread_mutex_unlock(&m);
    }
}

static void *pthread_test_task(void *arg)
{
    os_printf(KERN_NOTICE"pthread_test_task running ...\r\n");
    if (arg) {
        os_sleep(10);
        os_printf(KERN_NOTICE"pthread_test_task join!\r\n");
        pthread_join((pthread_t)arg, NULL);
    } else {
        os_sleep(20);
    }
    os_printf(KERN_NOTICE"pthread_test_task exit!\r\n");
	return NULL;
}

void posix_pthread_test(void)
{
    pthread_t pthd1, pthd2;
    pthread_attr_t attr1, attr2;
    pthread_attr_init(&attr1);
    pthread_attr_init(&attr2);
    pthread_attr_setdetachstate(&attr1, PTHREAD_CREATE_DETACHED);
    pthread_attr_setdetachstate(&attr2, PTHREAD_CREATE_JOINABLE);
    os_printf("posix test start!\r\n");
    pthread_create(&pthd2, &attr2, pthread_test_task, 0);
    pthread_create(&pthd1, &attr1, pthread_test_task, (void *)pthd2);
}

#endif

