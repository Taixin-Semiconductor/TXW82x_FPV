#include "basic_include.h"
#include "osal/string.h"
#include "lib/ota/fw.h"
#include "txw82x/boot_lib.h"
#include "lib/posix/stdio.h"
#include <stdio.h>

extern off_t fsize(FILE *stream);
#define OTA_CACHE_SIZE (4096)
void ota_task(void *fp)
{
    // 偏移到原来位置
    fseek(fp, SEEK_SET, 0);
    uint32_t file_size      = fsize(fp);
    uint8_t *ota_cache_buff = (uint8_t *) os_malloc(OTA_CACHE_SIZE);
    uint32_t read_size      = 0;
    uint32_t ota_offset     = 0;
    uint32_t filesize_tmp   = file_size;
    os_printf("###ota size:%d\n", file_size);
    while (file_size)
    {
        if (file_size < OTA_CACHE_SIZE)
        {
            read_size = file_size;
        }
        else
        {
            read_size = OTA_CACHE_SIZE;
        }
        fread(ota_cache_buff, read_size, 1, fp);
        int res = libota_write_fw(filesize_tmp, ota_offset, ota_cache_buff, read_size);
        if (res)
        {
            os_printf(KERN_DEBUG "libota_write_fw failed at offset %d, res: %d\n", ota_offset, res);
            break;
        }
        os_printf("OTA file size: %d bytes, cache buffer: %x\n", file_size, ota_cache_buff);

        file_size -= read_size;
        ota_offset += read_size;
    }
    fclose(fp);
    mcu_reset();
}

int32_t file_ota(const char *ota_bin)
{
    void    *fp = fopen(ota_bin, "rb");
    uint8_t  header[256];
    uint16_t ota_crc, now_crc;
    int16_t  ret;
    if (fp)
    {
        fread(header, 1, sizeof(header), fp);
        ret = get_code_crc(header, sizeof(header), &ota_crc);
        if (ret)
        {
            fclose(fp);
        }
        else
        {
            now_crc = get_code_crc16();
            os_printf("ota_crc:%X,now_crc:%X\n", ota_crc, now_crc);
            extern void ota_task(void *fp);
            if (now_crc != ota_crc)
            {
                // 创建任务
                os_task_create("sd_ota", ota_task, fp, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2048);
            }
            return RET_OK;
        }
    }
    return RET_ERR;
}