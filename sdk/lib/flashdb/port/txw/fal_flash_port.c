/*
 * Copyright (c) 2024, TXW
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief External NOR Flash driver porting for TXW platform
 *
 * 使用 sdk/include/hal/spi_nor.h 提供的接口
 */

#include <fal.h>
#include <string.h>
#include "dev.h"
#include "devid.h"
#include "hal/spi_nor.h"
#include <fal.h>
#include "chip/tx_platform.h"
#include "fdb_def.h"

#define TXW_FAL_STAGING_BUFFER_SIZE        256

static int txw_fal_init(void);
static int txw_fal_read(long offset, uint8_t *buf, size_t size);
static int txw_fal_write(long offset, const uint8_t *buf, size_t size);
static int txw_fal_erase(long offset, size_t size);

/**
 * Flash 设备参数配置
 *
 * len、blk_size 会在 init() 中从驱动获取并更新
 * user_data 在 init() 中指向 spi_nor_flash 设备实例
 */
struct fal_flash_dev txw_nor_flash = {
    .name       = "norflash0",
    .addr       = 0,
    .len        = 0,                 /* init 中从驱动获取 */
    .blk_size   = 0,                 /* init 中从驱动获取 */
    .ops        = {txw_fal_init, txw_fal_read, txw_fal_write, txw_fal_erase},
    .write_gran = 32,                 /* NOR Flash 写粒度为 1 bit */
    .user_data  = NULL,              /* init 中设置为 spi_nor_flash 指针 */
};

/**
 * 初始化 SPI NOR Flash
 *
 * @return 0 成功，-1 失败
 */
static int txw_fal_init(void)
{
    struct spi_nor_flash *flash = NULL;

    flash = (void *)dev_get(HG_FLASH0_DEVID);
    if (!flash) {
        FDB_DEBUG("%s,%d:Error, can not get norflash device!id:%d\n",
                  __FUNCTION__, __LINE__, HG_FLASH0_DEVID);
        return -1;
    }

    /* 保存 flash 设备指针到 user_data */
    txw_nor_flash.user_data = flash;

    /* 从驱动获取 Flash 参数 */
    txw_nor_flash.len      = flash->size;
    txw_nor_flash.blk_size = flash->sector_size;

    FDB_DEBUG("Check flash %p,size:%d, block_size:%d,write_gran:%d)\n",
              txw_nor_flash.user_data, txw_nor_flash.len,
              txw_nor_flash.blk_size, txw_nor_flash.write_gran);

    return 0;
}

/**
 * 从 SPI Flash 读取数据
 *
 * @param offset 相对于 Flash 起始地址的偏移量
 * @param buf    读取缓冲区
 * @param size   读取大小
 *
 * @return 实际读取的大小，-1 表示失败
 */
static int txw_fal_read(long offset, uint8_t *buf, size_t size)
{
    struct spi_nor_flash *flash =
        (struct spi_nor_flash *)txw_nor_flash.user_data;
    uint32_t stage_buf[TXW_FAL_STAGING_BUFFER_SIZE / sizeof(uint32_t)];
    uint8_t *stage = (uint8_t *)stage_buf;
    size_t read = 0;

    if (!flash) {
        FDB_PRINT("Error, flash device not initialized!\n");
        return -1;
    }

    if (!buf) {
        FDB_PRINT("Error, read buffer is NULL!\n");
        return -1;
    }

    spi_nor_open(flash);

    while (read < size) {
        size_t chunk = size - read;
        if (chunk > sizeof(stage_buf)) {
            chunk = sizeof(stage_buf);
        }

        spi_nor_read(flash, (uint32)(offset + read), stage, (uint32)chunk);
        memcpy(buf + read, stage, chunk);
        read += chunk;
    }

    spi_nor_close(flash);

    return size;
}

static int txw_fal_verify_chunk(struct spi_nor_flash *flash, uint32 offset,
                                const uint8_t *expect, size_t size)
{
    uint8_t verify_buf[TXW_FAL_STAGING_BUFFER_SIZE];
    size_t i;

    spi_nor_read(flash, offset, verify_buf, (uint32)size);

    for (i = 0; i < size; i++) {
        if (verify_buf[i] != expect[i]) {
            FDB_PRINT("Error: write verify failed at 0x%x, "
                      "chunk_off:%d, expected 0x%02x, got 0x%02x\n",
                      (uint32)(offset + i), (int)i,
                      expect[i], verify_buf[i]);
            return -1;
        }
    }

    return 0;
}

/**
 * 写入数据到 SPI Flash
 *
 * @param offset 相对于 Flash 起始地址的偏移量
 * @param buf    写入数据缓冲区
 * @param size   写入大小
 *
 * @return 实际写入的大小，-1 表示失败
 */
static int txw_fal_write(long offset, const uint8_t *buf, size_t size)
{
    struct spi_nor_flash *flash =
        (struct spi_nor_flash *)txw_nor_flash.user_data;
    uint32_t stage_buf[TXW_FAL_STAGING_BUFFER_SIZE / sizeof(uint32_t)];
    uint8_t *stage = (uint8_t *)stage_buf;
    size_t written = 0;

    if (!flash) {
        FDB_PRINT("Error, flash device not initialized!\n");
        return -1;
    }

    if (!buf) {
        FDB_PRINT("Error, write buffer is NULL!\n");
        return -1;
    }

    spi_nor_open(flash);

    while (written < size) {
        size_t chunk = size - written;

        if (chunk > sizeof(stage_buf)) {
            chunk = sizeof(stage_buf);
        }

        memcpy(stage, buf + written, chunk);
        spi_nor_write(flash, (uint32)(offset + written),
                      stage, (uint32)chunk);

        if (txw_fal_verify_chunk(flash, (uint32)(offset + written),
                                 stage, chunk) < 0) {
            spi_nor_close(flash);
            return -1;
        }

        written += chunk;
    }

    spi_nor_close(flash);

    return size;
}

static int txw_fal_erase(long offset, size_t size)
{
    struct spi_nor_flash *flash = (struct spi_nor_flash *)txw_nor_flash.user_data;
    //uint32_t erased = 0;
    uint32 sector_cnt;
    uint32 i = 0;

    if (!flash) {
        FDB_PRINT("Error, flash device not initialized!\n");
        return -1;
    }

    sector_cnt = (size + flash->sector_size - 1) / flash->sector_size;

    spi_nor_open(flash);
    for (i = 0; i < sector_cnt; i++) {
        spi_nor_sector_erase(flash, offset + i * flash->sector_size);
    }
    spi_nor_close(flash);
    return size;
}

int fal_partition_table_update(struct fal_partition *table, size_t len)
{
    const struct fal_flash_dev *flash = fal_flash_device_find(NOR_FLASH_DEV_NAME);
    size_t i;
    int updated = 0;

    if (!flash || flash->len == 0) {
        FAL_PRINTF("[Error/FAL] %s: flash '%s' not ready\n",
                   __FUNCTION__, NOR_FLASH_DEV_NAME);
        return -1;
    }

    if (flash->blk_size > 0) {
        if (FDB_KVDB_PART_SIZE % flash->blk_size != 0) {
            FAL_PRINTF("[Error/FAL] %s: KVDB size(0x%x) not aligned "
                       "to blk_size(0x%x)\n",
                       __FUNCTION__, FDB_KVDB_PART_SIZE,
                       flash->blk_size);
            return -1;
        }
        if (FDB_TSDB_PART_SIZE % flash->blk_size != 0) {
            FAL_PRINTF("[Error/FAL] %s: TSDB size(0x%x) not aligned "
                       "to blk_size(0x%x)\n",
                       __FUNCTION__, FDB_TSDB_PART_SIZE,
                       flash->blk_size);
            return -1;
        }

        if (flash->len % flash->blk_size != 0) {
            FAL_PRINTF("[Error/FAL] %s: flash len(0x%x) not aligned "
                       "to blk_size(0x%x)\n",
                       __FUNCTION__, flash->len, flash->blk_size);
            return -1;
        }
    }

    for (i = 0; i < len; i++) {
        if (!strcmp(table[i].name, "fdb_kvdb1")) {
            table[i].offset = flash->len - FDB_KVDB_PART_SIZE;
            table[i].len    = FDB_KVDB_PART_SIZE;
            updated++;
        } else if (!strcmp(table[i].name, "fdb_tsdb1")) {
            table[i].offset = flash->len
                              - FDB_KVDB_PART_SIZE
                              - FDB_TSDB_PART_SIZE;
            table[i].len    = FDB_TSDB_PART_SIZE;
            updated++;
        }
    }

    if (updated > 0) {
        FAL_PRINTF("[I/FAL] Partitions updated: flash=0x%x, "
                   "kvdb@0x%x, tsdb@0x%x\n",
                   flash->len,
                   flash->len - FDB_KVDB_PART_SIZE,
                   flash->len - FDB_KVDB_PART_SIZE - FDB_TSDB_PART_SIZE);
    }

    return updated;
}
