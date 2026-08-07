#include "vfs.h"
#include "fat.h"
#include "heap.h"

/*****************************************************************
 * 从内存申请一片空间,然后格式化成一个fat的文件系统,这里假装是sd
 ***************************************************************/
int vfs_demo_fs_heap_init(uint8_t *heap_buf,uint32_t heap_size)
{
	blockdevice_t *heap = blockdevice_heap_create(heap_buf,heap_size);
	filesystem_t *fat = filesystem_fat_create();

    uint8_t *src = (uint8_t*)0x10100000;
    hw_memcpy(heap_buf,src,heap_size);

    os_printf("Heap:%X\n",heap);
	int err = fs_mount("/sd", fat, heap);
    if (err == -1) {
        os_printf("format / with FAT\n");
        err = fs_format(fat, heap);
        if (err == -1) {
            os_printf("fs_format error: %s", strerror(errno));
            return false;
        }
        err = fs_mount("/sd", fat, heap);
		os_printf("errno:%d\n",errno);
        if (err == -1) {
            os_printf("fs_mount error: %s", strerror(errno));
            return false;
        }
    }
    return true;
}