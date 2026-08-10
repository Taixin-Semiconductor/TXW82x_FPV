/*
 * Copyright (c) 2024, TXW
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief 系统配置接口声明
 *
 * 提供基于 FlashDB KVDB 的配置读写接口，支持字符串、二进制和基础类型。
 */

#ifndef __SYSCFG_H__
#define __SYSCFG_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <flashdb.h>
#include "osal/mutex.h"

#ifdef __cplusplus
extern "C" {
#endif

struct fdb_dev {
    os_mutex_t mutex;
#ifdef FDB_USING_KVDB
    struct fdb_kvdb kvdb;
#endif
#ifdef FDB_USING_TSDB
    struct fdb_tsdb tsdb;
#endif
};

/* 获取全局 fdb 设备实例（内部使用） */
struct fdb_dev *syscfg_get_dev(void);

/* ========== 生命周期管理 ========== */

/**
 * 初始化 flashdb（KVDB + TSDB）
 *
 * @return 0 成功，负数失败
 */
int flashdb_init(void);

/**
 * 反初始化 flashdb
 */
void flashdb_deinit(void);

/* ========== 通用接口 ========== */

/**
 * 写入字符串配置
 *
 * @param key   键名
 * @param value 字符串值，传 NULL 则删除该 key
 *
 * @return FDB_NO_ERR 成功，其他失败
 */
fdb_err_t syscfg_set_string(const char *key, const char *value);

/**
 * 读取字符串配置
 *
 * @param key 键名
 *
 * @return 字符串指针，未找到返回 NULL
 * @note 返回值指向内部缓冲区，需尽快拷贝，下次操作可能覆盖
 */
const char *syscfg_get_string(const char *key);

/**
 * 写入二进制配置
 *
 * @param key   键名
 * @param value 二进制数据缓冲区
 * @param len   数据长度
 *
 * @return FDB_NO_ERR 成功，其他失败
 */
fdb_err_t syscfg_set_binary(const char *key, const void *value,
                             size_t len);

/**
 * 读取二进制配置
 *
 * @param key     键名
 * @param buf     读取缓冲区
 * @param buf_len 缓冲区长度
 *
 * @return 实际读取的字节数，0 表示未找到
 */
size_t syscfg_get_binary(const char *key, void *buf, size_t buf_len);

/**
 * 删除配置
 *
 * @param key 键名
 *
 * @return FDB_NO_ERR 成功，其他失败
 */
fdb_err_t syscfg_delete(const char *key);

/* ========== 基础类型接口 ========== */

/* int8 */
fdb_err_t syscfg_set_i8(const char *key, int8_t value);
fdb_err_t syscfg_get_i8(const char *key, int8_t *value);

/* uint8 */
fdb_err_t syscfg_set_u8(const char *key, uint8_t value);
fdb_err_t syscfg_get_u8(const char *key, uint8_t *value);

/* int16 */
fdb_err_t syscfg_set_i16(const char *key, int16_t value);
fdb_err_t syscfg_get_i16(const char *key, int16_t *value);

/* uint16 */
fdb_err_t syscfg_set_u16(const char *key, uint16_t value);
fdb_err_t syscfg_get_u16(const char *key, uint16_t *value);

/* int32 */
fdb_err_t syscfg_set_i32(const char *key, int32_t value);
fdb_err_t syscfg_get_i32(const char *key, int32_t *value);

/* uint32 */
fdb_err_t syscfg_set_u32(const char *key, uint32_t value);
fdb_err_t syscfg_get_u32(const char *key, uint32_t *value);

/* int64 */
fdb_err_t syscfg_set_i64(const char *key, int64_t value);
fdb_err_t syscfg_get_i64(const char *key, int64_t *value);

/* uint64 */
fdb_err_t syscfg_set_u64(const char *key, uint64_t value);
fdb_err_t syscfg_get_u64(const char *key, uint64_t *value);

/* float */
fdb_err_t syscfg_set_float(const char *key, float value);
fdb_err_t syscfg_get_float(const char *key, float *value);

/* double */
fdb_err_t syscfg_set_double(const char *key, double value);
fdb_err_t syscfg_get_double(const char *key, double *value);

/* bool */
fdb_err_t syscfg_set_bool(const char *key, bool value);
fdb_err_t syscfg_get_bool(const char *key, bool *value);

/* char */
fdb_err_t syscfg_set_char(const char *key, char value);
fdb_err_t syscfg_get_char(const char *key, char *value);

#ifdef __cplusplus
}
#endif

#endif /* __SYSCFG_H__ */
