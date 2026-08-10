#include "basic_include.h"
#include "lowPower_app.h"

static struct os_msgqueue *g_resume_msgq = NULL;

static void lowPower_app_resume_task(void *d)
{
    struct os_msgqueue *resume_msgq = (struct os_msgqueue *)d;
    uint32_t res = os_msgq_get(resume_msgq, osWaitForever);
    if (res == 0) {
        os_printf("%s:%d\tget resume msg failed:%d\n", __FUNCTION__, __LINE__, res);
        goto resume_done;
    }

    os_printf("%s:%d\twakeup resume task\n",__FUNCTION__,__LINE__);

    if (lowPower_app_has_registered_modules()) {
        lowPower_app_resume_modules();
        goto resume_done;
    }

resume_done:
    lowPower_app_suspend_exit();

    g_resume_msgq = NULL;
    os_msgq_del(resume_msgq);
    os_free(resume_msgq);


}

int lowPower_app_resume_preinit(void)
{
    int ret;
    struct os_msgqueue *resume_msgq = (struct os_msgqueue *)os_zalloc(sizeof(struct os_msgqueue));
    if (!resume_msgq) {
        goto init_failed;
    }

    ret = os_msgq_init(resume_msgq, 1);
    if (ret != RET_OK) {
        os_printf("%s:%d\tinit resume msgq failed\n", __FUNCTION__, __LINE__);
        os_free(resume_msgq);
        goto init_failed;
    }

    g_resume_msgq = resume_msgq;

    os_task_create("lowpower_resume", lowPower_app_resume_task, (void*)resume_msgq, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 2048);

    return RET_OK;

init_failed:
    return RET_ERR;
}


void lowPower_app_resume_wakeup(void)
{
    int ret = RET_ERR;
    struct os_msgqueue *resume_msgq = g_resume_msgq;

    os_printf("%s:%d\twakeup resume task\n",__FUNCTION__,__LINE__);

    ret = os_msgq_put(resume_msgq, (uint32)&resume_msgq, 0);
    if (ret != RET_OK) {
        os_msgq_del(resume_msgq);
        os_free(resume_msgq);
        g_resume_msgq = NULL;
        os_printf("%s:%d\tput resume msg failed:%d\n", __FUNCTION__, __LINE__, ret);
    }
}
