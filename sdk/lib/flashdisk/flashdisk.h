#ifndef __FLASHDISK_H
#define __FLASHDISK_H
#include "sys_config.h"
#include "typesdef.h"
#include "diskio.h"
#include "ff.h"
#ifndef FLASH_FATFS_SIZE
#define FLASH_FATFS_SIZE    512*1024
#endif

void flash_fatfs_init(uint32_t abs_addr, uint32_t size);
int flash_udisk_read(void *dev, uint32_t lba, uint32_t block_count, uint8* buf);
int flash_udisk_write(void *dev, uint32_t lba, uint32_t block_count, uint8* buf);
struct spi_nor_flash *flashdisk_usb_getdev();
uint8_t flash_udisk_get_status(void *dev);
uint32_t flash_udisk_get_capacity(void *dev);
uint32_t flash_udisk_get_sector_size(void *dev);
#endif