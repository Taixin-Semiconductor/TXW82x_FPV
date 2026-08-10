#include "sys_config.h"
#include "basic_include.h"
#include "lib/posix/stdio.h"

static void sys_mount_sd_card(uint16 dev_id, uint8 umount)
{
    char target[8];
    char source[8];
    uint8 volume = 0;

    volume += (dev_id - HG_SD0_DEVID);
    os_sprintf(target, "/sd%d", volume);
    if (umount) {
        os_printf(KERN_NOTICE"sd%d plugout, umount %s!\r\n", volume, target);
        vfs_umount(target);
    } else {
        os_sprintf(source, "%d:", volume);
        os_printf(KERN_NOTICE"sd%d plugin, mount to %s!\r\n", volume, target);
        int32 ret = vfs_mount((const char *)source, (const char *)target, "fatfs", 0, NULL);
        if (ret) {
            os_printf(KERN_ERR"sd%d mount to %s fail! ret=%d\r\n", volume, target, ret);
        }
    }
}

void sys_mount_device(uint16 dev_id, uint16 dev_type, uint8 umount)
{
    switch (dev_type) {
        case DEV_TYPE_SD:
            sys_mount_sd_card(dev_id, umount);
            break;
        default:
            break;
    }
}

