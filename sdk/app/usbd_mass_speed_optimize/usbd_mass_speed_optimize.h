#ifndef USBD_MASS_SPEED_OPTIMIZE_H
#define USBD_MASS_SPEED_OPTIMIZE_H
#include "sys_config.h"

#define PINGPANG_BUF_EN	        1       //usbd mass读写速度优化线程使能

//MULTI_SECTOR_COUNT的值决定了读写速度优化线程的读写最大扇区数，该值越大，DMA可同时操作的扇区数越大 (注意：配置需为2的倍数)
//SD   : malloc sram = 512  x MULTI_SECTOR_COUNT x 2 (默认MULTI_SECTOR_COUNT = 4)
//FLASH: malloc sram = 4096 x MULTI_SECTOR_COUNT x 2 (默认MULTI_SECTOR_COUNT = 2)
//SRAM : malloc sram = 512  x MULTI_SECTOR_COUNT x 2 (默认MULTI_SECTOR_COUNT = 64)  测速使用
#if PINGPANG_BUF_EN
#if USBDISK == 1
#define MULTI_SECTOR_COUNT      4       
#elif USBDISK == 2
#define MULTI_SECTOR_COUNT      2
#elif USBDISK == 3
#define MULTI_SECTOR_COUNT      64
#endif
#else
#define MULTI_SECTOR_COUNT      1
#endif


#ifndef MULTI_SECTOR_COUNT
    #define MULTI_SECTOR_COUNT      4 
#endif

#if PINGPANG_BUF_EN
enum read_write_flag
{
    READ_FLAG = 0,
    WRITE_FLAG,
    RELEASE_FLAG,
};

typedef int (*usbd_mass_read)(uint32 lba, uint32 count, uint8* buf);
typedef int (*usbd_mass_write)(uint32 lba, uint32 count, uint8* buf);

struct usbd_mass_speed_info
{
    uint8_t *buf_1;
    uint8_t *buf_2;
    struct os_task usbd_mass_speed_task;
};

struct usbd_mass_speed_optimize_mq
{
    uint32_t read_write_flag;   //0:read, 1:write, 2:delete thread
    uint32_t sector_addr;
    uint32_t sector_count;
};

struct usbd_mass_speed_dev_t
{
    struct usbd_mass_speed_info *disk;
    struct os_msgqueue *msgqueue;
    struct os_semaphore *sem;
    struct os_semaphore *usb_sem_write;
    struct usbd_mass_speed_optimize_mq mq;

    volatile uint32_t error_flag;
    volatile uint32_t pingpang_flag;
    uint32_t thread_status;

    usbd_mass_read  udisk_read;
    usbd_mass_write udisk_write;
};

extern struct usbd_mass_speed_dev_t *g_dev;

int usbd_mass_speed_optimize_send_mq(struct usbd_mass_speed_dev_t *dev, uint32_t sector_addr, uint32_t sector_count, uint32_t read_write_flag);
uint32_t usbd_mass_speed_sec_calc(uint32_t sec);
void* usbd_mass_speed_optimize_thread_init(void *arg, usbd_mass_read read, usbd_mass_write write);
void  usbd_mass_speed_optimize_thread_deinit();

#endif

#endif
