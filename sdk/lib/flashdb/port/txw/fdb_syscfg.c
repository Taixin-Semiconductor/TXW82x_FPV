#include <flashdb.h>
#include <fal.h>
#include <errno.h>
#include <string.h>
#include "osal/mutex.h"
#include "fdb_def.h"
#include "osal/string.h"
#include "fdb_syscfg.h"

#ifdef FDB_USING_KVDB
/**
 * 写入字符串配置
 *
 * @param key   键名
 * @param value 字符串值，传 NULL 则删除该 key
 *
 * @return 0 成功，非 0 失败
 */
fdb_err_t syscfg_set_string(const char *key, const char *value)
{
    struct fdb_dev *fdb = syscfg_get_dev();
    if (!fdb) {
        FDB_PRINT("%s,%d:Error,not inited!\n",__FUNCTION__,__LINE__);
        return FDB_INIT_FAILED;
    }
    return fdb_kv_set(&fdb->kvdb, key, value);
}

/**
 * 读取字符串配置
 *
 * @param key 键名
 *
 * @return 字符串指针，未找到返回 NULL
 * @note 返回值指向内部缓冲区，需尽快拷贝，下次操作可能覆盖
 */
const char *syscfg_get_string(const char *key)
{
    struct fdb_dev *fdb = syscfg_get_dev();
    if (!fdb) {
        FDB_PRINT("%s,%d:Error,not inited!\n",__FUNCTION__,__LINE__);
        return NULL;
    }
    return fdb_kv_get(&fdb->kvdb, key);
}

/**
 * 写入二进制配置
 *
 * @param key   键名
 * @param value 二进制数据缓冲区
 * @param len   数据长度
 *
 * @return 0 成功，非 0 失败
 */
fdb_err_t syscfg_set_binary(const char *key, const void *value,
                             size_t len)
{
    struct fdb_dev *fdb = syscfg_get_dev();
    struct fdb_blob blob;

    if (!fdb) {
        FDB_PRINT("%s,%d:Error,not inited!\n",__FUNCTION__,__LINE__);
        return FDB_INIT_FAILED;
    }
    return fdb_kv_set_blob(&fdb->kvdb, key,
                           fdb_blob_make(&blob, value, len));
}

/**
 * 读取二进制配置
 *
 * @param key     键名
 * @param buf     读取缓冲区
 * @param buf_len 缓冲区长度
 *
 * @return 实际读取的字节数，0 表示未找到
 */
size_t syscfg_get_binary(const char *key, void *buf, size_t buf_len)
{
    struct fdb_dev *fdb = syscfg_get_dev();
    struct fdb_blob blob;

    if (!fdb) {
        FDB_PRINT("%s,%d:Error,not inited!\n",__FUNCTION__,__LINE__);
        return 0;
    }
    return fdb_kv_get_blob(&fdb->kvdb, key,
                           fdb_blob_make(&blob, buf, buf_len));
}

/**
 * 删除配置
 *
 * @param key 键名
 *
 * @return 0 成功，非 0 失败
 */
fdb_err_t syscfg_delete(const char *key)
{
    struct fdb_dev *fdb = syscfg_get_dev();
    if (!fdb) {
        FDB_PRINT("%s,%d:Error,not inited!\n",__FUNCTION__,__LINE__);
        return FDB_INIT_FAILED;
    }
    return fdb_kv_del(&fdb->kvdb, key);
}

/* ========== 基础类型 set/get 接口 ========== */

/* int8 */
fdb_err_t syscfg_set_i8(const char *key, int8_t value)
{
    return syscfg_set_binary(key, &value, sizeof(value));
}
fdb_err_t syscfg_get_i8(const char *key, int8_t *value)
{
    size_t len = syscfg_get_binary(key, value, sizeof(*value));
    if (len == sizeof(*value)) {
        return FDB_NO_ERR;
    }
    return FDB_READ_ERR;
}

/* uint8 */
fdb_err_t syscfg_set_u8(const char *key, uint8_t value)
{
    return syscfg_set_binary(key, &value, sizeof(value));
}
fdb_err_t syscfg_get_u8(const char *key, uint8_t *value)
{
    size_t len = syscfg_get_binary(key, value, sizeof(*value));
    if (len == sizeof(*value)) {
        return FDB_NO_ERR;
    }
    return FDB_READ_ERR;
}

/* int16 */
fdb_err_t syscfg_set_i16(const char *key, int16_t value)
{
    return syscfg_set_binary(key, &value, sizeof(value));
}
fdb_err_t syscfg_get_i16(const char *key, int16_t *value)
{
    size_t len = syscfg_get_binary(key, value, sizeof(*value));
    if (len == sizeof(*value)) {
        return FDB_NO_ERR;
    }
    return FDB_READ_ERR;
}

/* uint16 */
fdb_err_t syscfg_set_u16(const char *key, uint16_t value)
{
    return syscfg_set_binary(key, &value, sizeof(value));
}
fdb_err_t syscfg_get_u16(const char *key, uint16_t *value)
{
    size_t len = syscfg_get_binary(key, value, sizeof(*value));
    if (len == sizeof(*value)) {
        return FDB_NO_ERR;
    }
    return FDB_READ_ERR;
}

/* uint32 */
fdb_err_t syscfg_set_u32(const char *key, uint32_t value)
{
    return syscfg_set_binary(key, &value, sizeof(value));
}
fdb_err_t syscfg_get_u32(const char *key, uint32_t *value)
{
    size_t len = syscfg_get_binary(key, value, sizeof(*value));
    if (len == sizeof(*value)) {
        return FDB_NO_ERR;
    }
    return FDB_READ_ERR;
}

/* uint64 */
fdb_err_t syscfg_set_u64(const char *key, uint64_t value)
{
    return syscfg_set_binary(key, &value, sizeof(value));
}
fdb_err_t syscfg_get_u64(const char *key, uint64_t *value)
{
    size_t len = syscfg_get_binary(key, value, sizeof(*value));
    if (len == sizeof(*value)) {
        return FDB_NO_ERR;
    }
    return FDB_READ_ERR;
}

/* int32 */
fdb_err_t syscfg_set_i32(const char *key, int32_t value)
{
    return syscfg_set_binary(key, &value, sizeof(value));
}
fdb_err_t syscfg_get_i32(const char *key, int32_t *value)
{
    size_t len = syscfg_get_binary(key, value, sizeof(*value));
    if (len == sizeof(*value)) {
        return FDB_NO_ERR;
    }
    return FDB_READ_ERR;
}

/* int64 */
fdb_err_t syscfg_set_i64(const char *key, int64_t value)
{
    return syscfg_set_binary(key, &value, sizeof(value));
}
fdb_err_t syscfg_get_i64(const char *key, int64_t *value)
{
    size_t len = syscfg_get_binary(key, value, sizeof(*value));
    if (len == sizeof(*value)) {
        return FDB_NO_ERR;
    }
    return FDB_READ_ERR;
}

/* float */
fdb_err_t syscfg_set_float(const char *key, float value)
{
    return syscfg_set_binary(key, &value, sizeof(value));
}
fdb_err_t syscfg_get_float(const char *key, float *value)
{
    size_t len = syscfg_get_binary(key, value, sizeof(*value));
    if (len == sizeof(*value)) {
        return FDB_NO_ERR;
    }
    return FDB_READ_ERR;
}

/* double */
fdb_err_t syscfg_set_double(const char *key, double value)
{
    return syscfg_set_binary(key, &value, sizeof(value));
}
fdb_err_t syscfg_get_double(const char *key, double *value)
{
    size_t len = syscfg_get_binary(key, value, sizeof(*value));
    if (len == sizeof(*value)) {
        return FDB_NO_ERR;
    }
    return FDB_READ_ERR;
}

/* bool */
fdb_err_t syscfg_set_bool(const char *key, bool value)
{
    return syscfg_set_binary(key, &value, sizeof(value));
}
fdb_err_t syscfg_get_bool(const char *key, bool *value)
{
    size_t len = syscfg_get_binary(key, value, sizeof(*value));
    if (len == sizeof(*value)) {
        return FDB_NO_ERR;
    }
    return FDB_READ_ERR;
}

/* char */
fdb_err_t syscfg_set_char(const char *key, char value)
{
    return syscfg_set_binary(key, &value, sizeof(value));
}
fdb_err_t syscfg_get_char(const char *key, char *value)
{
    size_t len = syscfg_get_binary(key, value, sizeof(*value));
    if (len == sizeof(*value)) {
        return FDB_NO_ERR;
    }
    return FDB_READ_ERR;
}

#endif /* FDB_USING_KVDB */

