#include "sys_config.h"
#include "typesdef.h"
#include "devid.h"
#include "list.h"
#include "dev.h"

#include "hal/gpio.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "osal/task.h"
#include "osal/sleep.h"
#include "osal/timer.h"
#include "osal/work.h"

#include "lib/sdhost/sdhost.h"
#include "lib/sdhost/mmc.h"
#include "lib/sdhost/mmc_ops.h"

#include "fatfs/integer.h"
#include "fatfs/diskio.h"
#include "fatfs/ff.h"

#if 0
#define MMCFS_PRIN(fmt, arg...) printf(fmt, ##arg)
#else 
#define MMCFS_PRIN(fmt, arg...)
#endif


static DSTATUS fatfs_status(void *status);
static DSTATUS fatfs_init(void *init_dev);
static DRESULT fatfs_read(void *dev, BYTE* buf, DWORD sector, UINT count);
static DRESULT fatfs_write(void *dev, BYTE* buf, DWORD sector, UINT count);
static DRESULT fatfs_ioctl(void *init_dev, BYTE cmd, void* buf);
void mmc_fatfs_unregister(uint32_t dev_id);
int mmc_fatfs_register(uint32_t dev_id);
uint32_t get_sdhost_status(struct sdh_device *host);
uint32_t sd_tran_stop(struct sdh_device * host);

extern void sdhost_io_func_init(uint32 req);
extern uint32 sd_power_up(struct sdh_device *host,uint8 bus_w);
extern void sd_set_clk(struct sdh_device * host,uint32 clk);


static struct os_work host_wk = {
	.running=0
};

static int32 sdh_loop(struct os_work *work)
{
    struct sdh_device *host = NULL;
    host = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);
    static uint8_t opt = SD_IDLE;
    static uint32_t lba = 0;
    static uint8_t flag = 0;
    static uint8_t count = 0;
    uint32_t sleep_time = 500;
    uint32 ret;

    if(SD_OFF == host->sd_opt)
    {
        os_printf("sdh no online2\r\n");
        if(flag == 1)
        {
            flag = 0;
            os_mutex_del(&host->lock);
            mmc_fatfs_unregister(HG_SDIOHOST_DEVID);
        }

        //判断状态,是否重新挂在文件系统
        mmc_fatfs_register(HG_SDIOHOST_DEVID);
    }
    else
    {
            ret = os_mutex_lock(&host->lock,0);
            if(ret)
            {
                sleep_time = 1;
                //获取锁失败
                goto sdh_loop_end;
            }
            if(SD_IDLE != host->sd_opt)
            {
                if((opt != host->sd_opt)||(lba != host->new_lba))
                {
                    opt = host->sd_opt;
                    lba = host->new_lba;
                    count = 0;
                }
                else
                {
                    count++;
                }
            }
            else
            {
                ret = send_card_status(host);
                if(ret != 0)
                {
                    host->sd_opt = SD_OFF;
                    count = 0;
                    flag = 1;
                }
            }
            
            if(count >= 2){
                count = 0;			
                sd_tran_stop(host);
            }
            os_mutex_unlock(&host->lock);
    }
sdh_loop_end:
    os_run_work_delay(work, sleep_time);
	return 0;
}

int mmc_init(struct sdh_device * host, uint32_t clk)
{
    uint32_t ret;
    uint32_t ocr;
    uint8_t  bw  = 1;
	uint8_t *ext_csd = NULL;
    

    if(host->open)
        host->open(host, 1, SD_MODE_TYPE);
    os_mutex_init(&host->lock);

    sdhost_io_func_init(host->flags);
    
    if(bw == 4)
        sd_power_up(host, MMCSD_BUSWIDTH_4);
    else
        sd_power_up(host,0);

    sd_set_clk(host, 400*1000);

    send_idle(host);

    send_op_cond(host, 0x40FF8080, &ocr);
    while(!(ocr & 0x80000000)) 
    {
        os_sleep_ms(5);
        send_op_cond(host, 0x40FF8080, &ocr);
    }

    send_all_get_cid(host, host->resp_cid);

    host->rca = 1;
    mmc_set_card_addr(host, host->rca);

   //解析csd (to do)
    send_get_csd(host, host->resp_csd);
    host->card_blksize = 1 << GET_BITS(host->resp_csd, 80, 4);
    
   //select card
    send_select_card(host);

   //解析ext_csd (to do)
    ext_csd = os_malloc(512);
    if(!ext_csd) {
        printf("ext_csd malloc fail\r\n");
    }

    ret = mmc_get_ext_csd(host, ext_csd);

    //de select
    host->rca = 0;
    send_select_card(host);
    host->close(host);
    os_free(ext_csd);


    if(ret == 1) {
        //sd 卡
        host->card_type = CARD_TYPE_SD;
        sd_init(host, clk, 0);
    } else {
        host->card_type = CARD_TYPE_MMC;
        emmc_init(host, clk);
    }

    if(host_wk.running == 0)
    {
        OS_WORK_INIT(&host_wk, sdh_loop, 0);
        os_run_work_delay(&host_wk, 500);
    }
    return 0;
}




static struct fatfs_diskio sdcdisk_driver = {
  .status = fatfs_status,
  .init = fatfs_init,
  .read = fatfs_read,
  .write = fatfs_write,
  .ioctl = fatfs_ioctl
}; 


static DSTATUS fatfs_status(void *status){
	//TEST_INFO_SHOW ("fatfs_status_test\r\n");
	DSTATUS err = get_sdhost_status(status);
	return err;
}

static DSTATUS fatfs_init(void *init_dev){
	//TEST_INFO_SHOW ("fatfs_init_test\r\n");
	DSTATUS err;
	err = mmc_init((struct sdh_device*)init_dev, 24*1000*1000);
	return err;
}

static DRESULT fatfs_read(void *dev, BYTE* buf, DWORD sector, UINT count){
	DRESULT err;
	
	err = sd_multiple_read((struct sdh_device*)dev, sector, count*512, buf);
    MMCFS_PRIN ("fatfs_read_test:%d %x %d %d\r\n",sector,buf,count, err);
	//printf("E");
	return err;
}

static DRESULT fatfs_write(void *dev, BYTE* buf, DWORD sector, UINT count){
	DRESULT err;
	
	err = sd_multiple_write((struct sdh_device*)dev,sector,count*512,buf);
    MMCFS_PRIN ("fatfs_write_test:%d %x %d %d\r\n",sector,buf,count, err);
	//printf("E");
	return err;	
}  


extern uint32 fatfs_sd_tran_stop(struct sdh_device * host);
static DRESULT fatfs_ioctl(void *init_dev, BYTE cmd, void* buf){
	uint8 ret	= RES_OK;
	switch(cmd)
    {
		case CTRL_SYNC:
			fatfs_sd_tran_stop(init_dev);
			break;
		case GET_SECTOR_COUNT:
			*(DWORD *)buf = ((struct sdh_device *)init_dev)->card_capacity * 2;
			ret = RES_OK;
			break;

		case GET_SECTOR_SIZE:
			*(WORD *)buf = 512;
			ret = RES_OK;
			
			break;
		case GET_BLOCK_SIZE:
			*(DWORD *)buf = 4;
			// printf("*0B:%d\n",*B);
			ret = RES_OK;
			break;

		default:
			ret = RES_ERROR;            //not finish
			printf("rtos_sd_ioctl err\n");
			break;
    }
	return ret;

}

FATFS fatfs[2];
int mmc_fatfs_register(uint32_t dev_id){

	int ret = 1;
    int ldnum;
    char vpath[8];
    struct sdh_device *fatfs_sdh;
    
	printf("enter %s test\r\n", __func__);
	fatfs_sdh = (struct sdh_device *)dev_get(dev_id);

    if(dev_id == HG_SDIOHOST_DEVID) {
        ldnum = 1;
    } else if(dev_id == HG_SDIOHOST1_DEVID) {
        ldnum = 2;
    } else {
        os_printf("%s err parameter\r\n", __FUNCTION__);
        return 1;
    }
    sprintf(vpath, "%d:", ldnum);
    
	if(fatfs_sdh)
	{
		fatfs_register_drive(ldnum, &sdcdisk_driver, fatfs_sdh);
		ret = f_mount (&fatfs[ldnum-1], vpath, 1);
		if (ret) {
			printf("%s ret:%d\n",__FUNCTION__,ret);
			f_mount (NULL, vpath, 0);
			return ret;
		}	
	}
	
    printf("mmc fatfs mounted\r\n");
	return ret;
}

void mmc_fatfs_unregister(uint32_t dev_id)
{
	int ret = 1;
    int ldnum;
    char vpath[8];
	struct sdh_device *fatfs_sdh;
	fatfs_sdh = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);
    if(dev_id == HG_SDIOHOST_DEVID) {
        ldnum = 1;
    } else if(dev_id == HG_SDIOHOST1_DEVID) {
        ldnum = 2;
    } else {
        os_printf("%s err parameter\r\n", __FUNCTION__);
        return;
    }
    sprintf(vpath, "%d:", ldnum);

	if(fatfs_sdh)
	{
		ret = f_mount (NULL, vpath, 1);
        fatfs_register_drive(ldnum, NULL, NULL);
		if (ret) {
			printf("%s ret:%d\n",__FUNCTION__,ret);
			return ;
		}	
	}
}


