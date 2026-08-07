#ifndef __FLASHDISK_H
#define __FLASHDISK_H
#include "sys_config.h"
#include "typesdef.h"
#include "diskio.h"
#include "ff.h"
#ifndef FLASH_FATFS_SIZE
#define FLASH_FATFS_SIZE    512*1024
#endif

void flash_fatfs_init();
int flashdisk_usb_read(uint32_t sector,uint32_t count,uint8* buf);
int flashdisk_usb_write(uint32_t sector,uint32_t count,uint8* buf);
struct spi_nor_flash *flashdisk_usb_getdev();
uint32_t flashdisk_get_sec_count();
uint32_t flashdisk_get_sec_size();
#endif