#ifndef __BOOT_LIB_H
#define __BOOT_LIB_H
#include "typesdef.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32 get_boot_loader_addr();
void save_boot_loader_addr();
uint32 get_boot_total_size();
uint32 get_boot_version();
void increase_boot_version();
void set_boot_msg(uint8 *msg);
uint32 get_boot_svn_version();

uint8_t get_psram_status();
void set_psram_status(uint8_t res);

uint32 get_boot_loader_offset();
//返回 0:是代表检查通过   -1:输入长度过短  -2:头部校验不过  -3:头部crc校验不过
//注意:返回-2或者-3,crc都会被写入一个值(但不一定有效)
int16 get_code_crc(uint8 *buf,uint32 len,uint16 *crc);//返回ota代码的crc(只需要前面256byte)
uint16 get_code_crc16();    //获取当前代码的crc

#ifdef __cplusplus
}
#endif

#endif
