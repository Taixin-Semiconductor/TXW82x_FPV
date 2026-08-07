#include "sys_config.h"

#include "integer.h"
#include "diskio.h"
#include "ff.h"
#include <stdio.h>
#include "osal/sleep.h"
#include "typesdef.h"
#include "osal/task.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "list.h"
#include "dev.h"
#include "sdhost.h"
#include "devid.h"
#include "vfs.h"
typedef struct
{
    void *dev;
} sd_config_t;

static const char DEVICE_NAME[] = "sd";



static int init(blockdevice_t *device)
{
    (void)device;
    int ret = BD_ERROR_OK;

    if (device->is_initialized)
    {
        return BD_ERROR_OK;
    }
    uint32_t err = sdhost_init(48*1000*1000, SDHC_SINGLE_CODE_SUPPORT_DIS);
    if (err != 0)
    {
        return BD_ERROR_DEVICE_ERROR;
    }
    device->is_initialized = true;
    return ret;
}

static int deinit(blockdevice_t *device)
{


    if (!device->is_initialized)
    {
        return BD_ERROR_OK;
    }
    device->is_initialized = false;
    return BD_ERROR_OK;
}

static int sync(blockdevice_t *device)
{
    (void)device;
    return BD_ERROR_OK;
}

static int read(blockdevice_t *device, const void *buffer, bd_size_t addr, bd_size_t length)
{
    sd_config_t *config = device->config;
    DWORD sector = (DWORD)(addr / device->erase_size);
    DWORD count = (DWORD)(length / device->erase_size);
    int err = sd_multiple_read((struct sdh_device*)config->dev,sector,count*512,(uint8_t*)buffer);
    return err;
}

static int erase(blockdevice_t *device, bd_size_t addr, bd_size_t length)
{
    (void)device;
    (void)addr;
    (void)length;
    return BD_ERROR_OK;
}

static int program(blockdevice_t *device, const void *buffer, bd_size_t addr, bd_size_t length)
{
    sd_config_t *config = device->config;
    DWORD sector = (DWORD)(addr / device->erase_size);
    DWORD count = (DWORD)(length / device->erase_size);

    int err = sd_multiple_write((struct sdh_device*)config->dev,sector,count*512,(uint8_t*)buffer);
    return err;
}

static int trim(blockdevice_t *device, bd_size_t addr, bd_size_t length)
{
    (void)device;
    (void)addr;
    (void)length;
    return BD_ERROR_OK;
}

static bd_size_t size(blockdevice_t *device)
{
    //sd_config_t *config = device->config;
    extern unsigned int sd_dwCap;
    return (bd_size_t)sd_dwCap*1024;
}

blockdevice_t *blockdevice_sd_create()
{

    blockdevice_t *device = VFS_calloc(1, sizeof(blockdevice_t));
    if (device == NULL)
    {
        return NULL;
    }
    sd_config_t *config = calloc(1, sizeof(sd_config_t));
    if (config == NULL)
    {
        VFS_free(device);
        return NULL;
    }

    device->init = init;
    device->deinit = deinit;
    device->read = read;
    device->erase = erase;
    device->program = program;
    device->trim = trim;
    device->sync = sync;
    device->size = size;
    device->read_size = 512;
    device->erase_size = 512;
    device->program_size = 512;
    device->name = DEVICE_NAME;
    device->is_initialized = false;
    device->config = config;
    config->dev = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);
    device->init(device);
    return device;
}

void blockdevice_sd_free(blockdevice_t *device)
{
    device->deinit(device);
    VFS_free(device->config);
    VFS_free(device);
}
