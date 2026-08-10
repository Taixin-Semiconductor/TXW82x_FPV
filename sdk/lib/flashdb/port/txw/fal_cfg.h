/*
 * Copyright (c) 2024, TXW
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FAL (Flash Abstraction Layer) configuration for TXW platform
 *
 * Flash 布局由 fal_partition_table_update() 在运行时计算：
 * ┌─────────────────────────────────┐ 0x000000
 * │        Bootloader + 其他        │
 * ├─────────────────────────────────┤ flash_len - KVDB_SIZE - TSDB_SIZE
 * │        TSDB 分区 (fdb_tsdb1)    │ FDB_TSDB_PART_SIZE
 * ├─────────────────────────────────┤ flash_len - KVDB_SIZE
 * │        KVDB 分区 (fdb_kvdb1)    │ FDB_KVDB_PART_SIZE
 * └─────────────────────────────────┤ flash_len (运行时探测)
 *
 * 分区偏移量不再硬编码，由 hook 根据实际 Flash 大小自动计算。
 */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

/* ===================== Flash Device Configuration ========================= */

#define FAL_PART_HAS_TABLE_CFG
#define NOR_FLASH_DEV_NAME              "norflash0"

#ifndef FAL_MALLOC
#define FAL_MALLOC                     os_malloc
#endif

#ifndef FAL_CALLOC
#define FAL_CALLOC                     os_calloc
#endif

#ifndef FAL_REALLOC
#define FAL_REALLOC                    os_realloc
#endif

#ifndef FAL_FREE
#define FAL_FREE                       os_free
#endif

#ifndef FAL_PRINTF
#define FAL_PRINTF                     printf
#endif

#ifndef FAL_DEBUG
#define FAL_DEBUG                      1
#endif

//inttypes
#include <typesdef.h>
#ifndef PRIx8
#define PRIx8       "x"
#endif
#ifndef PRIu16
#define PRIu16      "u"
#endif
#ifndef PRId16
#define PRId16      "d" 
#endif
#ifndef PRIx16
#define PRIx16      "x"
#endif
#ifndef PRIu32
#define PRIu32      "u"
#endif
#ifndef PRId32
#define PRId32      "d"
#endif
#ifndef PRIx32
#define PRIx32      "x"
#endif
#ifndef PRIuPTR
#define PRIuPTR     "u"
#endif
#ifndef X8_F
#define X8_F  "02" PRIx8
#endif
#ifndef U16_F
#define U16_F PRIu16
#endif
#ifndef S16_F
#define S16_F PRId16
#endif
#ifndef X16_F
#define X16_F PRIx16
#endif
#ifndef U32_F
#define U32_F PRIu32
#endif
#ifndef S32_F
#define S32_F PRId32
#endif
#ifndef X32_F
#define X32_F PRIx32
#endif
#ifndef SZT_F
#define SZT_F PRIuPTR
#endif 
#ifndef PRIX32
#define PRIX32      PRIx32
#endif   
#ifndef PRIdLEAST16
#define PRIdLEAST16 PRId32    
#endif
#ifndef PRIuLEAST16
#define PRIuLEAST16 PRIu32
#endif
#ifndef PRIdMAX
#define PRIdMAX     PRId32
#endif
#ifdef FDB_USING_TIMESTAMP_64BIT
#define __PRITS "lld"
#else
#define __PRITS "d"
#endif

/*
 * Flash 总大小 — 仅作为编译期占位，运行时由 hook 使用真实值
 * 如需在其他地方引用 Flash 大小，请使用 txw_nor_flash.len
 */
#define FLASH_TOTAL_SIZE                (0x4000)   

/* 声明外部 NOR Flash 设备（在 fal_flash_port.c 中定义） */
extern struct fal_flash_dev txw_nor_flash;

/* Flash 设备表 */
#define FAL_FLASH_DEV_TABLE                                          \
{                                                                    \
    &txw_nor_flash,                                                  \
}

/* ====================== Partition Configuration ========================== */

/*
 * 分区大小配置（按需修改）
 * 注意：大小必须是 Flash block size (4KB = 0x1000) 的整数倍，且大于2片用于备份
 */
#define FDB_KVDB_PART_SIZE              (8 * 1024)        
#define FDB_TSDB_PART_SIZE              (8 * 1024)        
/*
 * 分区偏移量占位值 — 运行时由 fal_partition_table_update() 覆盖
 * 实际布局：KVDB 在 Flash 末尾，TSDB 紧邻其前
 */
#define FDB_KVDB_PART_OFFSET            (FLASH_TOTAL_SIZE - FDB_KVDB_PART_SIZE)
#define FDB_TSDB_PART_OFFSET            (FDB_KVDB_PART_OFFSET - FDB_TSDB_PART_SIZE)

/* 分区表配置 */
#ifdef FAL_PART_HAS_TABLE_CFG
#define FAL_PART_TABLE                                                               \
{                                                                                    \
    {FAL_PART_MAGIC_WORD, "fdb_tsdb1", NOR_FLASH_DEV_NAME, FDB_TSDB_PART_OFFSET, FDB_TSDB_PART_SIZE, 0}, \
    {FAL_PART_MAGIC_WORD, "fdb_kvdb1", NOR_FLASH_DEV_NAME, FDB_KVDB_PART_OFFSET, FDB_KVDB_PART_SIZE, 0}, \
}

#endif
#endif /* _FAL_CFG_H_ */
