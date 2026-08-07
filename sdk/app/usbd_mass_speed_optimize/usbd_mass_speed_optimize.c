#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/string.h"
#include "osal/mutex.h"
#include "osal/semaphore.h"
#include "osal/task.h"
#include "osal/msgqueue.h"
#include "osal/irq.h"
#include "osal/sleep.h"
#include "hal/gpio.h"
#include "hal/usb_device.h"
#include "usbd_mass_speed_optimize.h"
#if SDH_EN
#include "lib/sdhost/sdhost.h"
#endif
#if FLASHDISK_EN
#include "hal/spi_nor.h"
#include "flashdisk/flashdisk.h"
#endif

#if PINGPANG_BUF_EN

struct usbd_mass_speed_dev_t *g_dev = NULL;

int usbd_mass_speed_optimize_send_mq(struct usbd_mass_speed_dev_t *dev, uint32_t sector_addr, uint32_t sector_count, uint32_t read_write_flag)
{
    dev->mq.read_write_flag = read_write_flag;
    dev->mq.sector_addr = sector_addr;
    dev->mq.sector_count = sector_count;

//    os_printf("mq.read_write_flag:%d sector_addr:%x sector_count:%d\n",read_write_flag,sector_addr,sector_count);

    void *ptr = os_zalloc(sizeof(struct usbd_mass_speed_optimize_mq));
    if (ptr) {
        os_memcpy(ptr, (uint8_t *)&dev->mq, sizeof(struct usbd_mass_speed_optimize_mq));
        if (os_msgq_put(dev->msgqueue, (uint32)ptr, osWaitForever)) {
            os_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            os_free(ptr);
            return -1;
        }
        return 0;
    }


    return -1;
}

uint32_t usbd_mass_speed_sec_calc(uint32_t sec)
{
    uint32_t calc_optimize_sec = 0;
    uint32_t i = 0;

    if(sec <= 1)
    {
        return calc_optimize_sec;
    }
#if 1
    for(i = MULTI_SECTOR_COUNT; i > 1; i = i / 2)
    {
        if(sec % i == 0)
        {
            calc_optimize_sec = sec / i;
            break;
        }
    }
#else
    if((sec % MULTI_SECTOR_COUNT) == 0)
    {
        calc_optimize_sec = sec / MULTI_SECTOR_COUNT;
    }
    else
    {
        calc_optimize_sec = 0;
    }
#endif

    return calc_optimize_sec;
}

static void usbd_mass_speed_optimize_thread(void *arg)
{
    struct usbd_mass_speed_dev_t *dev = (struct usbd_mass_speed_dev_t *)arg;
    struct usbd_mass_speed_optimize_mq *mq = NULL;
    uint32_t calc_optimize_sec = 0;
    uint8_t *buff = NULL;
    int ret = 0;

    while(1)
    {
        void *ptr = (void *)os_msgq_get(dev->msgqueue, osWaitForever);
        if(ptr){

            calc_optimize_sec = 0;
            dev->error_flag = 0;

            mq = ptr;

            //此处判断是否结束线程
            if (mq->read_write_flag == RELEASE_FLAG)
            {
                dev->thread_status = 0;
                os_free(ptr);
                break;
            }

            uint32_t sec = mq->sector_count;
            uint32_t lba = mq->sector_addr;
            uint32_t multi_sector_en = 0;       

            calc_optimize_sec = usbd_mass_speed_sec_calc(sec);
            if (calc_optimize_sec) {
                sec = calc_optimize_sec;
                calc_optimize_sec = mq->sector_count / sec;
                multi_sector_en = 1;               
            } else {
                multi_sector_en = 0;
            } 
            
            //先读写一包
            os_sema_down(dev->usb_sem_write, osWaitForever);//保证读和写最后一包都会起这个信号量

            if(dev->pingpang_flag)
            {
                buff = dev->disk->buf_1;
                dev->pingpang_flag = 0;
            }
            else
            {
                buff = dev->disk->buf_2;
                dev->pingpang_flag = 1;
            }


            switch (mq->read_write_flag)
            {
            case READ_FLAG:           
                if(dev->error_flag == 0)
                {
                    if(multi_sector_en)
                    {
                        ret = dev->udisk_read(lba,calc_optimize_sec,buff);
                    }
                    else
                    {
                        ret = dev->udisk_read(lba,1,buff);
                    }
                }

                if(ret){
                    dev->error_flag = 1;
                    os_printf("read err!!!!!!!!!!!!\r\n");
                    os_sema_up(dev->sem);
                    goto __exit_err;
                }       
                os_sema_up(dev->sem); //读：sd线程接收到mq后，就开始读一包，读完起信号量通知usb发送，usb发送前起信号量告诉sd线程先开始接收下一包
                break;

            case WRITE_FLAG:
                os_sema_up(dev->sem); //写：usb读了一包后，获取到mq后开始写卡线程，sd卡写卡前起信号量通知usb开始读下一包

                if(dev->error_flag == 0)
                {
                    // os_printf("sector write\n");
                    if(multi_sector_en)
                    {
                        ret = dev->udisk_write(lba,calc_optimize_sec,buff);
                    }
                    else
                    {
                        ret = dev->udisk_write(lba,1,buff);
                    }
                }

                if(ret){
                    dev->error_flag = 1;
                    os_printf("write err!!!!!!!!!!!!\r\n");
                }
                break;
            
            default:
                break;
            }
            
            if(multi_sector_en)
            {
                lba+=calc_optimize_sec;
            }
            else
            {
                lba++;
            }
            
            sec--;



            //读：紧接着开始读第二包，切换pingpang buff，利用usb正在同步发送的时间进行读sd卡
            //写：usb读完下一包后，起信号量通知sd线程继续写卡，利用sd写卡的时间进行usb同步接收
            while(sec--)
            { 
                os_sema_down(dev->usb_sem_write, osWaitForever);

                if(dev->pingpang_flag)
                {
                    buff = dev->disk->buf_1;
                    dev->pingpang_flag = 0;
                }
                else
                {
                    buff = dev->disk->buf_2;
                    dev->pingpang_flag = 1;
                }
                
                switch (mq->read_write_flag)
                {
                case READ_FLAG:
                    if(dev->error_flag == 0)
                    {
                        if(multi_sector_en)
                        {
                            ret = dev->udisk_read(lba,calc_optimize_sec,buff);
                        }
                        else
                        {
                            ret = dev->udisk_read(lba,1,buff);
                        }
                    }

                    if(ret){
                        dev->error_flag = 1;
                        os_printf("read err!!!!!!!!!!!!\r\n");
                        os_sema_up(dev->sem);
                        goto __exit_err;
                    } 
                    os_sema_up(dev->sem);  
                    break;

                case WRITE_FLAG:
                    os_sema_up(dev->sem);  
                    if(dev->error_flag == 0)
                    {
                        // os_printf("sector write\n");
                        if(multi_sector_en)
                        {
                            ret = dev->udisk_write(lba,calc_optimize_sec,buff);
                        }
                        else
                        {
                            ret = dev->udisk_write(lba,1,buff);
                        }
                    }

                    if(ret){
                        dev->error_flag = 1;
                        os_printf("write err!!!!!!!!!!!!\r\n");
                    }
                    break;
                
                default:
                    break;
                }
                
                if(multi_sector_en)
                {
                    lba+=calc_optimize_sec;
                }
                else
                {
                    lba++;
                }
                              
            }


__exit_err:
            os_free(ptr);
            ptr = NULL;
        }
    }
    
}

void* usbd_mass_speed_optimize_thread_init(void *arg, usbd_mass_read read, usbd_mass_write write)
{
    struct usbd_mass_speed_info *disk = (struct usbd_mass_speed_info *)arg;

    if(read == NULL || write == NULL)
    {
        os_printf("usbd_mass_speed_optimize_thread_init read or write is null\n");
        return 0;
    }

    struct usbd_mass_speed_dev_t *dev = os_zalloc(sizeof(struct usbd_mass_speed_dev_t));
    if(dev == NULL)
    {
        os_printf("usbd_mass_speed_optimize_thread_init malloc fail\n");
        return 0;
    }

    dev->msgqueue = os_zalloc(sizeof(struct os_msgqueue));
    if(dev->msgqueue == NULL)
    {
        os_printf("usbd_mass_speed_optimize_thread_init msgqueue malloc fail\n");
        return 0;
    }
    
    dev->sem = os_zalloc(sizeof(struct os_semaphore));
    if(dev->sem == NULL)
    {
        os_printf("usbd_mass_speed_optimize_thread_init sem malloc fail\n");
        return 0;
    }
    
    dev->usb_sem_write = os_zalloc(sizeof(struct os_semaphore));
    if(dev->usb_sem_write == NULL)
    {
        os_printf("usbd_mass_speed_optimize_thread_init usb_sem_write malloc fail\n");
        return 0;
    }
    
    dev->disk = disk;
    dev->error_flag = 0;
    dev->pingpang_flag = 0;
    dev->thread_status = 1;
    dev->udisk_read  = read;
    dev->udisk_write = write;

    g_dev = dev;
    
    os_sema_init(dev->sem, 0);
    os_sema_init(dev->usb_sem_write, 1);
    os_msgq_init(dev->msgqueue, 16);
    
    OS_TASK_INIT("usbd_mass_speed_optimize", &dev->disk->usbd_mass_speed_task, usbd_mass_speed_optimize_thread, (void *)dev, OS_TASK_PRIORITY_HIGH-1, NULL, 1024);

    return (void *)dev;
}

void usbd_mass_speed_optimize_thread_deinit()
{
    if(g_dev != NULL)
    {
        usbd_mass_speed_optimize_send_mq(g_dev, 0, 0, RELEASE_FLAG);
        do
        {
            os_sleep_ms(1);              //此处等待线程完成读写流程
        } while (g_dev->thread_status);
        

        if(g_dev->disk->usbd_mass_speed_task.hdl)
        {
            os_task_del(&g_dev->disk->usbd_mass_speed_task);
        }

        if(g_dev->msgqueue)
        {
            os_msgq_del(g_dev->msgqueue);
            os_free(g_dev->msgqueue);
        }

        if(g_dev->sem)
        {
            os_sema_del(g_dev->sem);
            os_free(g_dev->sem);
        }

        if(g_dev->usb_sem_write)
        {
            os_sema_del(g_dev->usb_sem_write);
            os_free(g_dev->usb_sem_write);            
        }
        
        os_free(g_dev);
        g_dev = NULL;
        
    }
}

#endif