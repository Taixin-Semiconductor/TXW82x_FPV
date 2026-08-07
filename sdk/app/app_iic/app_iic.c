#include "sys_config.h"
#include "basic_include.h"
#include "hal/lcdc.h"
#include "hal/spi.h"
#include "hal/i2c.h"
#include "dev/lcdc/hglcdc.h"
#include "lib/lcd/lcd.h"
#include "lib/multimedia/msi.h"
#include "stream_frame.h"

typedef struct 
{
	struct list_head list;				//iic queue hand
	struct i2c_device *i2c;			    //iic dev
	uint32_t len;
	uint8_t *table;
	uint8_t scl_io;							
	uint8_t sda_io;
	uint8_t id_addr;
	uint8_t rw_sta;                     //0:normal read   1:normal write   2:table write
	uint8_t sta;						//0:idle    1:data ready
	uint8_t devid;
	uint8_t *trx_buff;					//用户自定义的buff空间
}iic_queue_wq;


volatile struct list_head iic_queue_head;

static struct os_semaphore iicwq_sem = {0,NULL};

void iicwq_sema_init()
{
	os_sema_init(&iicwq_sem,0);
}

void iicwq_sema_down(int32 tmo_ms)
{
	os_sema_down(&iicwq_sem,tmo_ms);
}

void iicwq_sema_up()
{
	os_sema_up(&iicwq_sem);
}

int register_iic_queue(struct i2c_device *i2c,uint8_t scl_io,uint8_t sda_io,uint8_t id_addr){
	static uint8_t idnum= 1;
	iic_queue_wq *wq;
	wq = malloc(sizeof(iic_queue_wq));
	wq->scl_io = scl_io;
	wq->sda_io = sda_io;
	wq->id_addr= id_addr;
	wq->sta    = 0;
	wq->i2c    = i2c;
	wq->devid  = idnum;
	wq->trx_buff = NULL;
	INIT_LIST_HEAD(&wq->list);
	list_add_tail(&wq->list,(struct list_head*)&iic_queue_head); 
	return idnum++;
}

int unregister_iic_queue(uint8_t devid){
	int ret = 0;
	struct list_head *dlist;
	
	iic_queue_wq* iicdev;
	if(list_empty((struct list_head *)&iic_queue_head) != TRUE){
		dlist = (struct list_head *)&iic_queue_head;
		do{
			dlist = dlist->next;
			if(dlist == &iic_queue_head){
				return -1;
			}else{
				iicdev = list_entry((struct list_head *)dlist,iic_queue_wq,list);
				if(iicdev->devid == devid){
					list_del(&iicdev->list);
					free(iicdev);
					return 1;
				}
			}
		}while(1);
	}else{
		ret = -1;
	}
	return ret;
}

int iic_devid_set_addr(uint8_t devid,uint8_t addr){
	int ret = 0;
	struct list_head *dlist;
	iic_queue_wq* iicdev;	
	if(list_empty((struct list_head *)&iic_queue_head) != TRUE){
		dlist = (struct list_head *)&iic_queue_head;
		do{
			dlist = dlist->next;
			if(dlist == &iic_queue_head){
				return -2;                 //no this device
			}else{
				iicdev = list_entry((struct list_head *)dlist,iic_queue_wq,list);
				if(iicdev->devid == devid){
					iicdev->id_addr= addr;
					return 1;          //this id all ready finish
				}
			}
		}while(1);		
	}else{
		ret = -1;
	}
	return ret;
}


int wake_up_iic_queue(uint8_t devid,uint8_t *table,uint32_t len,uint8_t rw_sta,uint8_t *trx_buff){
	int ret = 0;
	struct list_head *dlist;
	iic_queue_wq* iicdev;
	if(list_empty((struct list_head *)&iic_queue_head) != TRUE){
		dlist = (struct list_head *)&iic_queue_head;
		do{
			dlist = dlist->next;
			if(dlist == &iic_queue_head){
				return -1;
			}else{
				iicdev = list_entry((struct list_head *)dlist,iic_queue_wq,list);
				if(iicdev->devid == devid){
					iicdev->table = table;
					iicdev->len   = len;
					if(iicdev->sta == 1){
						return 2;          //this id all ready running
					}
					iicdev->sta   = 1;
					iicdev->rw_sta= rw_sta;
					iicdev->trx_buff = trx_buff;
					iicwq_sema_up();
					return 1;
				}
			}
		}while(1);
	}else{
		ret = -1;
	}
	return ret;
}

int iic_devid_finish(uint8_t devid){
	int ret = 0;
	struct list_head *dlist;
	iic_queue_wq* iicdev;	
	if(list_empty((struct list_head *)&iic_queue_head) != TRUE){
		dlist = (struct list_head *)&iic_queue_head;
		do{
			dlist = dlist->next;
			if(dlist == &iic_queue_head){
				return -2;                 //no this device
			}else{
				iicdev = list_entry((struct list_head *)dlist,iic_queue_wq,list);
				if(iicdev->devid == devid){
					if(iicdev->sta == 0){
						return 1;          //this id all ready finish
					}else{
						return 0;
					}
				}
			}
		}while(1);		
	}else{
		ret = -1;
	}
	return ret;
}

void iic_run_thread(void *d){
	// int32 iic_table_finish; 
	// int out_time;
	scatter_data *table_scatter_data;
	struct i2c_device *iic_dev;
	struct i2c_device *iic2_dev;
	struct list_head *dlist;
	iic_queue_wq* iicdev;
	iic_dev = (struct i2c_device *)dev_get(HG_I2C1_DEVID);
	iic2_dev = (struct i2c_device *)dev_get(HG_I2C2_DEVID);
	while(1){
		iicwq_sema_down(-1);
		if(list_empty((struct list_head *)&iic_queue_head) != TRUE){
			dlist =(struct list_head *) &iic_queue_head;
			do{
				dlist = dlist->next;
				if(dlist == &iic_queue_head){
					break;
				}else{
					iicdev = list_entry((struct list_head *)dlist,iic_queue_wq,list);
					if(iicdev->sta == 1){
						gpio_driver_strength(iicdev->scl_io, GPIO_DS_G3);
						gpio_driver_strength(iicdev->sda_io, GPIO_DS_G3);
						gpio_set_mode(iicdev->scl_io, GPIO_OPENDRAIN_PULL_UP, GPIO_PULL_LEVEL_4_7K);
						gpio_set_mode(iicdev->sda_io, GPIO_OPENDRAIN_PULL_UP, GPIO_PULL_LEVEL_4_7K);
						if(iicdev->i2c == iic_dev){
							gpio_iomap_inout(iicdev->scl_io, GPIO_IOMAP_IN_SPI1_SCK_IN, GPIO_IOMAP_OUT_SPI1_SCK_OUT);
							gpio_iomap_inout(iicdev->sda_io, GPIO_IOMAP_IN_SPI1_IO0_IN, GPIO_IOMAP_OUT_SPI1_IO0_OUT);

						}else{
							gpio_iomap_inout(iicdev->scl_io, GPIO_IOMAP_IN_SPI2_SCK_IN, GPIO_IOMAP_OUT_SPI2_SCK_OUT);
							gpio_iomap_inout(iicdev->sda_io, GPIO_IOMAP_IN_SPI2_IO0_IN, GPIO_IOMAP_OUT_SPI2_IO0_OUT);
						}
						if(iicdev->id_addr == 0){
							i2c_ioctl(iicdev->i2c,IIC_SET_DEVICE_ADDR,iicdev->table[2]);
						}
						//printf("scl_io:%x  sda_io:%x\r\n",iicdev->scl_io,iicdev->sda_io);
						if(iicdev->rw_sta == 0){
							if (iicdev->trx_buff != NULL) {
								//处理DMA地址不对齐的情况，使用外部传入的地址空间
								i2c_read(iicdev->i2c,(int8*)&iicdev->table[3],iicdev->table[0],(int8*)iicdev->trx_buff,iicdev->table[1]);
							} else {
								i2c_read(iicdev->i2c,(int8*)&iicdev->table[3],iicdev->table[0],(int8*)&iicdev->table[3+iicdev->table[0]],iicdev->table[1]);
							}
						}else if(iicdev->rw_sta == 1){
							i2c_write(iicdev->i2c, (int8*)&iicdev->table[3],iicdev->table[0], (int8*)&iicdev->table[3+iicdev->table[0]], iicdev->table[1]);
						}else if(iicdev->rw_sta == 2){
							table_scatter_data = (scatter_data*)iicdev->table;
							// out_time = 0x1ff;
							// iic_table_finish = 0;
							i2c_master_write_table(iicdev->i2c, iicdev->len, iicdev->id_addr, table_scatter_data);		
							// while((iic_table_finish == 0)&&(out_time != 0)){
							// 	iic_table_finish = i2c_ioctl(iic_dev,IIC_GET_TX_STATUS, 0);
							// 	out_time--;
							// 	os_sleep_ms(1);
							// }
						}

						gpio_set_dir(iicdev->scl_io, GPIO_DIR_INPUT);
                		gpio_set_dir(iicdev->sda_io, GPIO_DIR_INPUT);
						gpio_iomap_input(iicdev->scl_io,GPIO_IOMAP_INPUT);
						gpio_iomap_input(iicdev->sda_io,GPIO_IOMAP_INPUT);						
						iicdev->sta = 0;
					}
				}
			}while(1);

		}else{
			continue;
		}		
	
	}
}

void iic_thread_init(){
	struct i2c_device *iic_dev;
	iic_dev = (struct i2c_device *)dev_get(HG_I2C1_DEVID);

	struct i2c_device *iic_dev2;
	iic_dev2 = (struct i2c_device *)dev_get(HG_I2C2_DEVID);

	i2c_open(iic_dev, IIC_MODE_MASTER, IIC_ADDR_7BIT, 0);
	i2c_set_baudrate(iic_dev,250000UL);
	i2c_ioctl(iic_dev,IIC_SDA_OUTPUT_DELAY,20);	
	i2c_ioctl(iic_dev,IIC_FILTERING,20);
	i2c_ioctl(iic_dev,IIC_SET_WRITE_TABLE_MODE, 1);
	
	// i2c_open(iic_dev2, IIC_MODE_MASTER, IIC_ADDR_7BIT, 0);
	// i2c_set_baudrate(iic_dev2,250000UL);
	// i2c_ioctl(iic_dev2,IIC_FILTERING,20);
	// i2c_ioctl(iic_dev2,IIC_SET_WRITE_TABLE_MODE, 1);

	INIT_LIST_HEAD((struct list_head *)&iic_queue_head);
	iicwq_sema_init();
	os_task_create("iic_thread", iic_run_thread, NULL, OS_TASK_PRIORITY_HIGH, 0, NULL, 1024);
}

