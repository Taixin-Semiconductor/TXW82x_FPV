#include "basic_include.h"
#include "osal_file.h"
#include "lib/ota/fw.h"




#define OTA_FILE_DIR      "ota"
#define OTA_FILE_NAME       "ota.bin"
#define OTA_CACHE_SIZE      (4096)

static uint8_t g_file_ota = 0;
static struct os_work fileota_work;

uint32_t file_ota()
{
    if(g_file_ota) {
        os_printf(KERN_DEBUG"File OTA already done.\n");
        return RET_OK;
    }
    
    char path[64];
    os_sprintf(path, "0:%s/%s", OTA_FILE_DIR, OTA_FILE_NAME);

    void *fp = osal_fopen(path, "rb");
    if (!fp) {
        os_printf(KERN_ERR"OTA file not found: %s\n", path);
        return RET_ERR;
    }

    uint32_t file_size = osal_fsize(fp);
    if (file_size < 1024) {
        osal_fclose(fp);
        os_printf(KERN_ERR"OTA file size is too small: %d bytes\n", file_size);
        return RET_ERR;
    }

    uint8_t *ota_cache_buff = (uint8_t *)os_malloc(OTA_CACHE_SIZE);

    if (!ota_cache_buff) {
        osal_fclose(fp);
        os_printf(KERN_ERR"Failed to allocate memory for OTA cache buffer.\n");
        return RET_ERR;
    }

    uint32_t read_size = 0;
    uint32_t ota_offset = 0;
    uint32_t filesize_tmp = file_size;

    os_printf(KERN_DEBUG"OTA file start processing, size: %d bytes, cache buffer: %x\n", file_size, ota_cache_buff);

    while(file_size)
    {
        if (file_size < OTA_CACHE_SIZE) {
            read_size = file_size;
        } else {
            read_size = OTA_CACHE_SIZE;
        }

        osal_fread(ota_cache_buff, read_size, 1, fp);
        int res = libota_write_fw(filesize_tmp, ota_offset, ota_cache_buff, read_size);
        if (res) {
            os_printf(KERN_DEBUG"libota_write_fw failed at offset %d, res: %d\n", ota_offset, res);
            break;
        }
        os_printf("OTA file size: %d bytes, cache buffer: %x\n", file_size, ota_cache_buff);

        file_size -= read_size;
        ota_offset += read_size;
    }

    if(file_size == 0) {
        g_file_ota = 1;
        os_printf(KERN_DEBUG"OTA file successfully processed.\n");
    } else {
        os_printf(KERN_ERR"OTA file processing incomplete.\n");
    }


    if (fp) {
        osal_fclose(fp);
    }

    if (ota_cache_buff) {
        os_free(ota_cache_buff);
    }

    if (g_file_ota) {
        osal_unlink(path);
        mcu_reset();
    }

    return (file_size == 0) ? RET_OK : RET_ERR;
}

static int32 file_ota_work(struct os_work *work)
{
    if(g_file_ota) {
        os_printf(KERN_DEBUG"File OTA already done.\n");
        goto __file_ota_work_end;
    }
    
    char path[64];
    os_sprintf(path, "0:%s/%s", OTA_FILE_DIR, OTA_FILE_NAME);

    void *fp = osal_fopen(path, "r");
    if (!fp) {
        os_printf(KERN_ERR"OTA file not found: %s\n", path);
        goto __file_ota_work_end;
    }

    uint32_t file_size = osal_fsize(fp);
    if (file_size < 1024) {
        osal_fclose(fp);
        fp = NULL;
        os_printf(KERN_ERR"OTA file size is too small: %d bytes\n", file_size);
        goto __file_ota_work_end;
    }

    uint8_t *ota_cache_buff = (uint8_t *)os_malloc(OTA_CACHE_SIZE);
    if (!ota_cache_buff) {
        os_printf(KERN_ERR"Failed to allocate memory for OTA cache buffer.\n");
        goto __file_ota_end;
    }
    os_printf("OTA file size: %d bytes, cache buffer: %x\n", file_size, ota_cache_buff);
    uint32_t read_size = 0;
    uint32_t ota_offset = 0;
    uint32_t filesize_tmp = file_size;

    while(file_size)
    {
        if (file_size < OTA_CACHE_SIZE) {
            read_size = file_size;
        } else {
            read_size = OTA_CACHE_SIZE;
        }

        osal_fread(ota_cache_buff, read_size, 1, fp);
        int res = libota_write_fw(filesize_tmp, ota_offset, ota_cache_buff, read_size);
        if (res) {
            os_printf(KERN_DEBUG"libota_write_fw failed at offset %d, res: %d\n", ota_offset, res);
            break;
        }
        os_printf("read_size:%d file_size:%d ota_offset:%d\n", read_size, file_size, ota_offset);
        file_size -= read_size;
        ota_offset += read_size;
    }

    if(file_size == 0) {
        g_file_ota = 1;
        os_printf(KERN_DEBUG"OTA file successfully processed.\n");
    } else {
        os_printf(KERN_ERR"OTA file processing incomplete.\n");
    }

    if (g_file_ota) {
        osal_unlink(path);
    }

__file_ota_end:
    if (fp) {
        osal_fclose(fp);
    }

    if (ota_cache_buff) {
        os_free(ota_cache_buff);
    }

__file_ota_work_end:

    os_run_work_delay(work, 2000);

    return 0;
}

void file_ota_work_init()
{
    OS_WORK_INIT(&fileota_work, file_ota_work, 0);
    os_run_work_delay(&fileota_work, 1);
}