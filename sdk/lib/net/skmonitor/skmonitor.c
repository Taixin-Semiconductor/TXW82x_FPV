//#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "osal/task.h"
#include "osal/string.h"
#include "osal/sleep.h"
#include "osal/mutex.h"
#include "osal/semaphore.h"
#include "osal/work.h"
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "lwip/inet_chksum.h"
#include "netif/etharp.h"
#include "netif/ethernetif.h"
#include "lwip/ip.h"
#include "lwip/init.h"
#include "lib/net/skmonitor/skmonitor.h"

#define skmntr_dbg(fmt, ...) //os_printf("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define SKMNTR_STATIS (0)

#define SKMONITOR_PORT (61753)

struct skmonitor_entry {
    struct skmonitor_entry *next;
    uint16 sock;
    uint8  flags;
    uint8  disable: 1, rev: 7;
    uint32 priv;
    skmonitor_cb cb;
#if SKMNTR_STATIS
    uint32 trig_rd, trig_wr, trig_err;
#endif
};

struct skmonitor_mgr {
    struct skmonitor_entry *entrys;
    os_mutex_t lock;
    os_semaphore_t run;
    struct os_work  work;
    os_task_t  task;
    fd_set rset, wset, eset;
    int32  sock;
} g_skmonitor;

static void sock_monitor_reset()
{
    uint8  val = 0;
    struct sockaddr_in addr;
    os_memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(SKMONITOR_PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    sendto(g_skmonitor.sock, &val, 1, 0, (struct sockaddr *)&addr, sizeof(addr));
}

static void sock_monitor_task(void *args)
{
    int32  ret = 0;
    struct timeval tv;
    struct skmonitor_entry *mntr = NULL;

    while (1) {
        os_sema_down(&g_skmonitor.run, osWaitForever);

        FD_ZERO(&g_skmonitor.rset);
        FD_ZERO(&g_skmonitor.wset);
        FD_ZERO(&g_skmonitor.eset);

        ret = g_skmonitor.sock;
        FD_SET(g_skmonitor.sock, &g_skmonitor.rset);
        os_mutex_lock(&g_skmonitor.lock, osWaitForever);
        mntr = g_skmonitor.entrys;
        while (mntr) {
            if (!mntr->disable) {
                if (mntr->flags & SOCK_MONITOR_READ) {
                    FD_SET(mntr->sock, &g_skmonitor.rset);
                }
                if (mntr->flags & SOCK_MONITOR_WRITE) {
                    FD_SET(mntr->sock, &g_skmonitor.wset);
                }
                if (mntr->flags & SOCK_MONITOR_ERROR) {
                    FD_SET(mntr->sock, &g_skmonitor.eset);
                }
                if (mntr->sock > ret) {
                    ret = mntr->sock;
                }

            }
            mntr = mntr->next;
        }
        os_mutex_unlock(&g_skmonitor.lock);

        tv.tv_sec  = 0;
        tv.tv_usec = 100 * 1000;
        ret = select(ret + 1, &g_skmonitor.rset, &g_skmonitor.wset, &g_skmonitor.eset, &tv);
        if (ret == 0) {
            os_sema_up(&g_skmonitor.run);
        } else {
            os_run_work(&g_skmonitor.work);
        }
    }
}

static int32 sock_monitor_work(struct os_work *work)
{
    uint8  val;
    uint8  flags;
    socklen_t len = 0;
    struct sockaddr_in addr;
    struct skmonitor_entry *mntr = NULL;
    struct skmonitor_entry *next = NULL;
    skmonitor_cb cb;
    uint32 priv;
    int32  sock;

    os_mutex_lock(&g_skmonitor.lock, osWaitForever);
    mntr = g_skmonitor.entrys;
    while (mntr) {
        next = mntr->next;
        flags = 0;
        if (FD_ISSET(mntr->sock, &g_skmonitor.rset)) {
            flags |= SOCK_MONITOR_READ;
#if SKMNTR_STATIS
            mntr->trig_rd++;
#endif
        }
        if (FD_ISSET(mntr->sock, &g_skmonitor.wset)) {
            flags |= SOCK_MONITOR_WRITE;
#if SKMNTR_STATIS
            mntr->trig_wr++;
#endif
        }
        if (FD_ISSET(mntr->sock, &g_skmonitor.eset)) {
            flags |= SOCK_MONITOR_ERROR;
#if SKMNTR_STATIS
            mntr->trig_err++;
#endif
        }

        if (flags && mntr->cb) {
            cb   = mntr->cb;
            priv = mntr->priv;
            sock = mntr->sock;
            os_mutex_unlock(&g_skmonitor.lock);

            skmntr_dbg("callback start ..., cb=%p\r\n", cb);
            cb(sock, flags, priv);
            skmntr_dbg("callback done, cb=%p\r\n", cb);
            os_mutex_lock(&g_skmonitor.lock, osWaitForever);
        }
        mntr = next;
    }
    os_mutex_unlock(&g_skmonitor.lock);

    if (FD_ISSET(g_skmonitor.sock, &g_skmonitor.rset)) {
        recvfrom(g_skmonitor.sock, &val, 1, 0, (struct sockaddr *)&addr, &len);
    }
    os_sema_up(&g_skmonitor.run);
    return 0;
}

int32 sock_monitor_add(uint16 sock, skmonitor_flags flags, skmonitor_cb cb, uint32 priv)
{
    int32 ret = -1;
    struct skmonitor_entry *mntr = NULL;

    if ((int16)sock < 0 || cb == NULL) {
        return -EINVAL;
    }

    skmntr_dbg("sock_monitor_add enter ...\r\n");
    os_mutex_lock(&g_skmonitor.lock, osWaitForever);
    mntr = g_skmonitor.entrys;
    while (mntr) {
        if (mntr->sock == sock) {
            mntr->cb   = cb;
            mntr->priv = priv;
            mntr->flags = flags;
            ret = 0;
            break;
        }
        mntr = mntr->next;
    }

    if (ret) {
        mntr = os_zalloc(sizeof(struct skmonitor_entry));
        if (mntr) {
            mntr->cb   = cb;
            mntr->sock = sock;
            mntr->priv = priv;
            mntr->flags = flags;
            mntr->next = g_skmonitor.entrys;
            g_skmonitor.entrys = mntr;
            ret = 0;
        }
    }
    os_mutex_unlock(&g_skmonitor.lock);

    skmntr_dbg("sock_monitor_add leave\r\n");
    sock_monitor_reset();
    return ret;
}

void sock_monitor_del(uint16 sock)
{
    struct skmonitor_entry *mntr = NULL;
    struct skmonitor_entry *prev = NULL;

    skmntr_dbg("sock_monitor_del enter ...\r\n");
    os_mutex_lock(&g_skmonitor.lock, osWaitForever);
    mntr = g_skmonitor.entrys;
    while (mntr) {
        if (mntr->sock == sock) {
            if (prev) {
                prev->next = mntr->next;
            } else {
                g_skmonitor.entrys = mntr->next;
            }
            os_free(mntr);
            break;
        }
        prev = mntr;
        mntr = mntr->next;
    }
    os_mutex_unlock(&g_skmonitor.lock);
    skmntr_dbg("sock_monitor_del leave\r\n");
}

void sock_monitor_disable(uint16 sock, uint8 disable)
{
    struct skmonitor_entry *mntr = NULL;

    skmntr_dbg("sock_monitor_disable enter ...\r\n");
    os_mutex_lock(&g_skmonitor.lock, osWaitForever);
    mntr = g_skmonitor.entrys;
    while (mntr) {
        if (mntr->sock == sock && mntr->disable != disable) {
            mntr->disable = disable;
            if (!disable) {
                sock_monitor_reset();
            }
            break;
        }
        mntr = mntr->next;
    }
    os_mutex_unlock(&g_skmonitor.lock);
    skmntr_dbg("sock_monitor_disable leave\r\n");
}

void sock_monitor_dump(void)
{
#if SKMNTR_STATIS
    struct skmonitor_entry *mntr = NULL;

    skmntr_dbg("sock monitor dump:\r\n");
    os_mutex_lock(&g_skmonitor.lock, osWaitForever);
    mntr = g_skmonitor.entrys;
    while (mntr) {
        if (!mntr->disable) {
            os_printf("   sock %d, cb:%p, read:%d, write:%d, error:%d\r\n", 
                      mntr->sock, mntr->cb, mntr->trig_rd, mntr->trig_wr, mntr->trig_err);
            mntr->trig_rd  = 0;
            mntr->trig_wr  = 0;
            mntr->trig_err = 0;
        }
        mntr = mntr->next;
    }
    os_mutex_unlock(&g_skmonitor.lock);
#endif
}

__init int32 sock_monitor_init(void)
{
    struct sockaddr_in local_addr;
    os_memset(&g_skmonitor, 0, sizeof(g_skmonitor));
    g_skmonitor.sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_skmonitor.sock == -1) {
        os_printf("socket monitor init failed\r\n");
        return RET_ERR;
    }
    local_addr.sin_family = AF_INET;
    local_addr.sin_port   = htons(SKMONITOR_PORT);
    local_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(g_skmonitor.sock, (struct sockaddr *)&local_addr, sizeof(struct sockaddr)) < 0) {
        closesocket(g_skmonitor.sock);
        os_printf("socket monitor init failed\r\n");
        return RET_ERR;
    }
    os_mutex_init(&g_skmonitor.lock);
    os_sema_init(&g_skmonitor.run, 1);
    OS_WORK_INIT(&g_skmonitor.work, sock_monitor_work, 0);
    OS_TASK_INIT("skmntr", &g_skmonitor.task, sock_monitor_task, &g_skmonitor, OS_TASK_PRIORITY_NORMAL, NULL, 1024);
    return RET_OK;
}

