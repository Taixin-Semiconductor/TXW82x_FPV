#include <flashdb.h>
#include <fal.h>
#include <errno.h>
#include <string.h>
#include "osal/mutex.h"
#include "fdb_def.h"
#include "osal/string.h"
#include "fdb_syscfg.h"

#define FDB_LOG_TAG "[FAL]"

static struct fdb_dev *g_fdb = NULL;

struct fdb_dev *syscfg_get_dev(void)
{
    return g_fdb;
}

static void fdb_lock(fdb_db_t db)
{
    struct fdb_dev *fdb = (struct fdb_dev *)db->user_data;
    if (fdb) {
        os_mutex_lock(&fdb->mutex, osWaitForever);
    }
}

static void fdb_unlock(fdb_db_t db)
{
    struct fdb_dev *fdb = (struct fdb_dev *)db->user_data;
    if (fdb) {
        os_mutex_unlock(&fdb->mutex);
    }
}

#ifdef FDB_USING_TSDB
static fdb_time_t get_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    printf("%s:Check gettimeofday,sec:%lud,usec:%lld\n",__FUNCTION__,tv.tv_sec,tv.tv_usec);
    return tv.tv_sec;
}

/**
 * 初始化 TSDB
 *
 * @param tsdb      TSDB 实例指针（调用者分配）
 * @param name      数据库名称
 * @param path      Flash 分区名称（FAL 模式）
 * @param max_len   每条日志最大长度
 * @param user_data 用户数据（通常传入互斥锁句柄）
 *
 * @return 0 成功，负数失败
 */
static int flashdb_tsdb_init(fdb_tsdb_t tsdb, const char *name,
                      const char *path, size_t max_len,
                      void *user_data)
{
    fdb_err_t result;

    if (!tsdb || !name || !path) {
        return -EINVAL;
    }

    /* 设置锁回调 */
    fdb_tsdb_control(tsdb, FDB_TSDB_CTRL_SET_LOCK,
                     (void *)fdb_lock);
    fdb_tsdb_control(tsdb, FDB_TSDB_CTRL_SET_UNLOCK,
                     (void *)fdb_unlock);

    /* 初始化 TSDB，传递 user_data */
    result = fdb_tsdb_init(tsdb, name, path,
                           get_time, max_len, user_data);
    if (result != FDB_NO_ERR) {
        FDB_PRINT("Error: TSDB(%s) init failed (%d)\n",
                  name, (int)result);
        return -result;
    }

    return 0;
}

#endif

/**
 * 初始化 KVDB
 *
 * @param kvdb       KVDB 实例指针（调用者分配）
 * @param name       数据库名称
 * @param path       Flash 分区名称（FAL 模式）
 * @param default_kv 默认 KV 配置，可为 NULL
 * @param user_data  用户数据（通常传入互斥锁句柄）
 *
 * @return 0 成功，负数失败
 */
#ifdef FDB_USING_KVDB 
static int flashdb_kvdb_init(fdb_kvdb_t kvdb, const char *name,
                      const char *path,
                      struct fdb_default_kv *default_kv,
                      void *user_data)
{
    fdb_err_t result;

    if (!kvdb || !name || !path) {
        return -EINVAL;
    }

    /* 设置锁回调 */
    fdb_kvdb_control(kvdb, FDB_KVDB_CTRL_SET_LOCK,
                     (void *)fdb_lock);
    fdb_kvdb_control(kvdb, FDB_KVDB_CTRL_SET_UNLOCK,
                     (void *)fdb_unlock);

    /* 初始化 KVDB，传递 user_data */
    result = fdb_kvdb_init(kvdb, name, path,
                           default_kv, user_data);
    if (result != FDB_NO_ERR) {
        FDB_PRINT("Error: KVDB(%s) init failed (%d)\n",
                  name, (int)result);
        return -result;
    }

    return 0;
}
#endif
/**
 * 初始化 flashdb（KVDB + TSDB）
 *
 * @return 0 成功，负数失败
 */
int flashdb_init(void)
{
    int result;
    struct fdb_dev *fdb = NULL;

    fdb = FAL_MALLOC(sizeof(struct fdb_dev));
    if (!fdb) {
        FDB_DEBUG("Error, no memory!\n");
        return -ENOMEM;
    }
    memset(fdb, 0, sizeof(struct fdb_dev));

    result = os_mutex_init(&fdb->mutex);
    if (result) {
        FDB_DEBUG("Init mutex Error: %d\n", result);
        goto __exit;
    }

#ifdef FDB_USING_KVDB
    result = flashdb_kvdb_init(&fdb->kvdb, "env", "fdb_kvdb1",
                               NULL, fdb);
    if (result) {
        FDB_DEBUG("Init kvdb Error: %d\n", result);
        goto __exit;
    }
#endif

#ifdef FDB_USING_TSDB
    result = flashdb_tsdb_init(&fdb->tsdb, "log", "fdb_tsdb1",
                               128, fdb);
    if (result) {
        FDB_DEBUG("Init tsdb Error: %d\n", result);
        goto __exit;
    }
#endif
    g_fdb = fdb;
    return 0;

__exit:
#ifdef FDB_USING_KVDB
    fdb_kvdb_deinit(&fdb->kvdb);
#endif
#ifdef FDB_USING_TSDB
    fdb_tsdb_deinit(&fdb->tsdb);
#endif
    os_mutex_del(&fdb->mutex);
    FAL_FREE(fdb);
    return result;
}

void flashdb_deinit(void)
{
    struct fdb_dev *fdb = g_fdb;
    if (!fdb) {
        return;
    }

#ifdef FDB_USING_TSDB
    fdb_tsdb_deinit(&fdb->tsdb);
#endif

#ifdef FDB_USING_KVDB
    fdb_kvdb_deinit(&fdb->kvdb);
#endif
    os_mutex_del(&fdb->mutex);
    FAL_FREE(fdb);
    g_fdb = NULL;
}

