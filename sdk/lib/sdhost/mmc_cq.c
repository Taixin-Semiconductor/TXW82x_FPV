#include "sys_config.h"
#include "typesdef.h"
#include "devid.h"
#include "list.h"
#include "dev.h"
#include "osal/task.h"
#include "osal/semaphore.h"
#include "osal/string.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "osal/task.h"
#include "osal/sleep.h"
#include "osal/timer.h"
#include "osal/work.h"
#include "lib/sdhost/sdhost.h"
#include "lib/sdhost/mmc.h"
#include "lib/sdhost/mmc_ops.h"
#include "lib/common/rbuffer.h"
#include "hal/gpio.h"


#define QSR_SET(n, qsr) (qsr) |= (1<<(n))
#define QSR_CLR(n, qsr) (qsr) &= ~(1<<(n))
#define QSR_ISSET(n, qsr) ((qsr) & (1<<(n)))

#define TID_CNT (16)

typedef union {
    struct {

        uint32 blks   :16,
            tid    : 5,
            rev    : 2,
            prio   : 1,
            forced : 1,
            cid    : 4,
            is_tag : 1,
            dir    : 1,
            rwrq   : 1;
    } field;                                 
    uint32 w;         
} TASK_ARG;

struct cq_req {
    TASK_ARG arg;
    uint32 lba;
    uint8 *data;
    void (*cb)(void* arg);
    void *cb_arg; 
};

struct cqe_mgr {
    uint32 tmask;
    struct cq_req * reqs[32];
    struct os_mutex lock;
    struct os_mutex tid_lock;
    struct os_semaphore tid_src;
    struct os_semaphore req_done;
    // RBUFFER_DEF_R(reqs, struct cq_req);
};

struct cqe_mgr g_cqe;

int tid_alloc()
{
    int tid = -1;
    os_sema_down(&g_cqe.tid_src, osWaitForever);
    for(int i = 0;i<TID_CNT;i++)
    {
        if(!QSR_ISSET(i, g_cqe.tmask)) {
            QSR_SET(i, g_cqe.tmask);
            tid = i;
            break;
        }
    }
    printf("tid: %d alloc\r\n", tid);
    return tid;
}

void tid_free(int tid)
{
    QSR_CLR(tid, g_cqe.tmask);
    printf("tid: %d free\r\n", tid);
    os_sema_up(&g_cqe.tid_src);
}

int mmc_enqueue_task(struct sdh_device *host, uint32 lba, TASK_ARG arg)
{
    int err = 0;
    struct rt_mmcsd_cmd cmd44;
    struct rt_mmcsd_cmd cmd45;

    cmd44.cmd_code = MMC_QUE_TASK_PARAMS;
    cmd44.arg = arg.w;
    cmd44.flags = RESP_R1 | CMD_AC;

    cmd45.cmd_code = MMC_QUE_TASK_ADDR;
    cmd45.arg = lba;
    cmd45.flags = RESP_R1 | CMD_AC;

    err = host->cmd(host, &cmd44);
    err |= host->cmd(host, &cmd45);
    if(err) {
        printf("task enqueue fail\r\n");
    }
    return err;
}

int mmc_rtask_exe(struct sdh_device *host, uint8 *buf, TASK_ARG arg)
{
    int err = 0;
    struct rt_mmcsd_cmd cmd46;
    printf("rtask_Exe: tid=[%d]\r\n", arg.field.tid);
    cmd46.cmd_code = MMC_EXECUTE_READ_TASK;
    cmd46.arg = arg.w;
    cmd46.flags = RESP_R1 | CMD_ADTC;

    err = host->cmd(host, &cmd46);

    host->data.blksize = SECTOR_SIZE;
    host->data.buf = buf;
    host->data.blks = arg.field.blks;
    host->read(host, buf);

    if(host->complete) {
        host->complete(host);
    }
    return err;
}

int mmc_wtask_exe(struct sdh_device *host, uint8 *buf, TASK_ARG arg)
{
    int err = 0;
    struct rt_mmcsd_cmd cmd47;

    cmd47.cmd_code = MMC_EXECUTE_WRITE_TASK;
    cmd47.arg = arg.w;
    cmd47.flags = RESP_R1 | CMD_ADTC;

    err = host->cmd(host, &cmd47);

    host->data.blksize = SECTOR_SIZE;
    host->data.buf = buf;
    host->data.blks = arg.field.blks;
    host->write(host, buf);

    if(host->complete) {
        host->complete(host);
    }
    return err;
}

void mmc_dequeue_task(struct sdh_device *host, uint8 *buf, TASK_ARG arg)
{
    if(arg.field.dir == 1) {
        mmc_rtask_exe(host, buf, arg);
    } else {
        mmc_wtask_exe(host, buf, arg);
    }
}
//cmd queue function
int mmc_send_qsr(struct sdh_device *host, uint32* qsr)
{
    if(os_mutex_lock(&g_cqe.lock, 2000)) {
        return 1;
    }
    int err;
    struct rt_mmcsd_cmd cmd;

    cmd.cmd_code = SEND_STATUS;
    cmd.arg = host->rca << 16 | (1<<15);
    cmd.flags = RESP_R1 | CMD_AC;
    err = host->cmd(host, &cmd);
    
    if (err)
        return err;

    if (qsr)
        *qsr = cmd.resp[0];
    os_mutex_unlock(&g_cqe.lock);
    return 0;
}

int mmc_cqe_req(struct sdh_device* host, struct cq_req* req)
{
    if(os_mutex_lock(&g_cqe.lock, 2000)) {
        return 1;
    }
    int tid = tid_alloc();
    int ret = 0;
    if(tid == -1) {
        ret |= 1;
        goto _mmc_cqe_req_end;
    }

    req->arg.field.tid = tid;
    g_cqe.reqs[tid] = req;
    mmc_enqueue_task(host, req->lba, req->arg);
    os_sema_up(&g_cqe.req_done);    
    printf("req cmdq tid: %d lba: %d\t\n", req->arg.field.tid, req->lba);
_mmc_cqe_req_end:
    os_mutex_unlock(&g_cqe.lock);
    return ret;
}

int mmc_cqe_ereq(struct sdh_device* host, uint32 qsr)
{
    os_mutex_lock(&g_cqe.lock, osWaitForever);
    for(int tid = 0;tid<TID_CNT;tid++)
    {
        if(QSR_ISSET(tid, qsr)) {
            printf("qsr: 0x%x, [tid]: [%d], [req_arg]: 0x%x\r\n", qsr, tid, g_cqe.reqs[tid]->arg.w);
            mmc_dequeue_task(host, g_cqe.reqs[tid]->data, (TASK_ARG)(uint32)(tid << 16));
            tid_free(tid);
        }
    }
    os_mutex_unlock(&g_cqe.lock);
    return 0;
}

void cqe_task_func(void *arg)
{
	uint32 qsr;
    struct sdh_device *host = (struct sdh_device*)arg;
    while(1)
    {
        os_sema_down(&g_cqe.req_done, osWaitForever);
        qsr = 0;
        mmc_send_qsr(host, &qsr);
        if(!qsr) {
            os_sema_up(&g_cqe.req_done);
            continue;
        } 
        mmc_cqe_ereq(host, qsr);
        printf(".");
    }
}

struct os_task cqe_task;
int mmc_cqe_init(struct sdh_device *host)
{
    g_cqe.tmask = 0;
    os_mutex_init(&g_cqe.lock);
    os_mutex_init(&g_cqe.tid_lock);
    os_sema_init(&g_cqe.req_done, 0);
    os_sema_init(&g_cqe.tid_src, TID_CNT);

    //cmd queue en
    if(mmc_cmdq_switch(host, 1)) {
        printf("%s fail\r\n", __FUNCTION__);
        return 1;
    }

    OS_TASK_INIT("mmc_cqe", &cqe_task, cqe_task_func, host, OS_TASK_PRIORITY_NORMAL, NULL, 2048);
    return 0;
}


void cqe_test(struct sdh_device* host)
{
    emmc_init(host, 1000*1000);
    if(mmc_cqe_init(host)) {
        return;
    }

    struct cq_req req = {
        .arg.w = (1<<30) | 1,
        .lba = 0,
    };
    req.data = os_malloc(512);
    while(1)
    {
        req.lba *= 1664525;
        req.lba += 1013904223;
        req.lba &= 0x3ff;
        if(mmc_cqe_req(host, &req)) {
            os_sleep_ms(1);
        }
        // os_sleep_ms(5);
    }
    
}


