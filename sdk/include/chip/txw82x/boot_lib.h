#ifndef __BOOT_LIB_H
#define __BOOT_LIB_H
#include "typesdef.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    CODE_CRC_OK,
    INPUT_TOO_SHORT,
    HEAD_CHECK_FAIL,
    HEAD_CRC_CHECK_FAIL,
};
uint32 get_boot_loader_addr();
void   save_boot_loader_addr();
uint32 get_boot_total_size();
uint32 get_boot_version();
void   increase_boot_version();
void   set_boot_msg(uint8 *msg);
uint32 get_boot_svn_version();

uint8_t get_psram_status();
void    set_psram_status(uint8_t res);

uint32 get_boot_loader_offset();
/**********************************************
返回值:
CODE_CRC_OK:校验通过
INPUT_TOO_SHORT:输入长度过短
HEAD_CHECK_FAIL:头部校验不过
HEAD_CRC_CHECK_FAIL:头部crc校验不过
// 注意:不是返回CODE_CRC_OK,*crc的值是无效的
************************************************/
int16  get_code_crc(uint8 *buf, uint32 len, uint16 *crc); // 返回ota代码的crc(只需要前面256byte)
uint16 get_code_crc16();                                  // 获取当前代码的crc

void rom_qspi_reboot_trampoline(int al_loader_is_exist);

#ifdef __cplusplus
}
#endif

#endif
