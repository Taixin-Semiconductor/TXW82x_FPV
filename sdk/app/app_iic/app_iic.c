#include "sys_config.h"
#include "basic_include.h"
#include "hal/lcdc.h"
#include "hal/spi.h"
#include "hal/i2c.h"
#include "dev/lcdc/hglcdc.h"
#include "lib/lcd/lcd.h"
#include "lib/multimedia/msi.h"
#include "stream_define.h"

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
	volatile uint8_t sta;					//0:idle    1:data ready
	volatile uint8_t deleted;				//0:normal    1:pending delete
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

	if (idnum == 0) {
		idnum = 1;
	}

	iic_queue_wq *wq;
	wq = malloc(sizeof(iic_queue_wq));

	if(wq == NULL){
		os_printf("register_iic_queue malloc fail\n");
		return 0;
	}

	wq->scl_io = scl_io;
	wq->sda_io = sda_io;
	wq->id_addr= id_addr;
	wq->sta    = 0;
	wq->deleted = 0;
	wq->i2c    = i2c;
	wq->devid  = idnum;
	wq->trx_buff = NULL;
	uint32 irq_flags = disable_irq();
	INIT_LIST_HEAD(&wq->list);
	list_add_tail(&wq->list,(struct list_head*)&iic_queue_head); 
	enable_irq(irq_flags);

	os_printf("register_iic_queue:devid:%d\n",wq->devid);
	return idnum++;
}

int unregister_iic_queue(uint8_t devid){
	int ret = 0;
	struct list_head *dlist;
	
	iic_queue_wq* iicdev;
	// os_printf("unregister_iic_queue:devid:%d\n",devid);
	uint32 irq_flags = disable_irq();
	if(list_empty((struct list_head *)&iic_queue_head) != TRUE){
		dlist = (struct list_head *)&iic_queue_head;
		do{
			dlist = dlist->next;
			if(dlist == &iic_queue_head){
				enable_irq(irq_flags);
				return -1;
			}else{
				iicdev = list_entry((struct list_head *)dlist,iic_queue_wq,list);
				if(iicdev->devid == devid){
					// 标记删除，不从链表摘除，不释放内存
					// 由iic_run_thread在安全时机执行list_del和free
					iicdev->deleted = 1;
					// 如果iic_thread尚未开始处理(sta==1但未快照)，主动置sta=0
					// 如果iic_thread正在处理(已快照参数)，它完成后会自行置sta=0
					// 如果iic_thread还未处理(sta==0)，无影响
					iicdev->sta = 0;
					enable_irq(irq_flags);
					os_printf("unregister_iic_queue:devid:%d marked deleted\n",devid);
					iicwq_sema_up(); // 唤醒iic_thread执行清理
					return 1;
				}
			}
		}while(1);
	}else{
		ret = -1;
	}
	enable_irq(irq_flags);
	return ret;
}

int iic_devid_set_addr(uint8_t devid,uint8_t addr){
	int ret = 0;
	struct list_head *dlist;
	iic_queue_wq* iicdev;	
	uint32 irq_flags = disable_irq();
	if(list_empty((struct list_head *)&iic_queue_head) != TRUE){
		dlist = (struct list_head *)&iic_queue_head;
		do{
			dlist = dlist->next;
			if(dlist == &iic_queue_head){
				enable_irq(irq_flags);
				return -2;                 //no this device
			}else{
				iicdev = list_entry((struct list_head *)dlist,iic_queue_wq,list);
				if(iicdev->devid == devid){
					iicdev->id_addr= addr;
					enable_irq(irq_flags);
					return 1;          //this id all ready finish
				}
			}
		}while(1);		
	}else{
		ret = -1;
	}
	enable_irq(irq_flags);
	return ret;
}


int wake_up_iic_queue(uint8_t devid,uint8_t *table,uint32_t len,uint8_t rw_sta,uint8_t *trx_buff){
	int ret = 0;
	struct list_head *dlist;
	iic_queue_wq* iicdev;
	
	uint32 irq_flags = disable_irq();
	if(list_empty((struct list_head *)&iic_queue_head) != TRUE){
		dlist = (struct list_head *)&iic_queue_head;
		do{
			dlist = dlist->next;
			if(dlist == &iic_queue_head){
				enable_irq(irq_flags);
				return -1;
			}else{
				iicdev = list_entry((struct list_head *)dlist,iic_queue_wq,list);
				if(iicdev->devid == devid){				if(iicdev->deleted){
					enable_irq(irq_flags);
					return -1;          //设备已标记删除
				}					iicdev->table = table;
					iicdev->len   = len;
					if(iicdev->sta == 1){
						enable_irq(irq_flags);
						return 2;          //this id all ready running
					}
					// os_printf("wake_up_iic_queue:devid:%d, rw_sta:%d, trx_buff:0x%x func:0x%x\n",devid,rw_sta,trx_buff,RETURN_ADDR());
					iicdev->sta   = 1;
					iicdev->rw_sta= rw_sta;
					iicdev->trx_buff = trx_buff;
					enable_irq(irq_flags);
					// os_printf("iicwq_sema_up in wake_up_iic_queue\n");
					iicwq_sema_up();
					return 1;
				}
			}
		}while(1);
	}else{
		ret = -1;
	}
	enable_irq(irq_flags);
	return ret;
}

int iic_devid_finish(uint8_t devid){
	int ret = 0;
	struct list_head *dlist;
	iic_queue_wq* iicdev;	
	uint32 irq_flags = disable_irq();
	if(list_empty((struct list_head *)&iic_queue_head) != TRUE){
		dlist = (struct list_head *)&iic_queue_head;
		do{
			dlist = dlist->next;
			if(dlist == &iic_queue_head){
				enable_irq(irq_flags);
				return -2;                 //no this device
			}else{
				iicdev = list_entry((struct list_head *)dlist,iic_queue_wq,list);
				if(iicdev->devid == devid){
					if(iicdev->sta == 0){
						enable_irq(irq_flags);
						return 1;          //this id all ready finish
					}else{
						enable_irq(irq_flags);
						return 0;
					}
				}
			}
		}while(1);		
	}else{
		ret = -1;
	}
	enable_irq(irq_flags);
	return ret;
}

void iic_run_thread(void *d){
	// int32 iic_table_finish; 
	// int out_time;
	scatter_data *table_scatter_data;
	struct i2c_device *iic_dev;
	struct i2c_device *iic2_dev;
	iic_queue_wq* iicdev;
	uint8_t cur_scl_io, cur_sda_io;
	struct i2c_device *cur_i2c;
	uint8_t cur_id_addr, cur_rw_sta;
	uint32_t cur_len;
	uint8_t *cur_table, *cur_trx_buff;
	uint32 irq_flags = 0;
	iic_dev = (struct i2c_device *)dev_get(HG_I2C1_DEVID);
	iic2_dev = (struct i2c_device *)dev_get(HG_I2C2_DEVID);
	while(1){
		iicwq_sema_down(-1);
		// os_printf("iic_run_thread iicwq_sema_down\n");
		if(list_empty((struct list_head *)&iic_queue_head) != TRUE){
			// 遍历查找需要处理的设备，处理完后从链表头重新开始
			while(1){
				irq_flags = disable_irq();
				struct list_head *dlist;
				struct list_head *next;
				iic_queue_wq *found_dev = NULL;

				// 第一步：清理已标记删除且sta==0的节点（安全释放）
				dlist = iic_queue_head.next;
				while(dlist != &iic_queue_head){
					next = dlist->next;
					iicdev = list_entry(dlist, iic_queue_wq, list);
					if(iicdev->deleted && iicdev->sta == 0){
						list_del(dlist);
						enable_irq(irq_flags);
						os_printf("iic_run_thread free deleted dev devid:%d\n",iicdev->devid);
						free(iicdev);
						irq_flags = disable_irq();
						dlist = iic_queue_head.next; // 从头重新扫描
						continue;
					}
					dlist = next;
				}

				// 第二步：查找sta==1且未删除的节点进行处理
				dlist = iic_queue_head.next;
				while(dlist != &iic_queue_head){
					iicdev = list_entry(dlist, iic_queue_wq, list);
					if(!iicdev->deleted && iicdev->sta == 1){
						// 关中断期间快照需要的参数，避免开中断后访问可能被释放的节点
						cur_scl_io    = iicdev->scl_io;
						cur_sda_io    = iicdev->sda_io;
						cur_i2c       = iicdev->i2c;
						cur_id_addr   = iicdev->id_addr;
						cur_rw_sta    = iicdev->rw_sta;
						cur_table     = iicdev->table;
						cur_len       = iicdev->len;
						cur_trx_buff  = iicdev->trx_buff;
						found_dev = iicdev;
						break;
					}
					dlist = dlist->next;
				}
				enable_irq(irq_flags);

				if(found_dev == NULL){
					break;  // 没有找到需要处理的设备
				}

				// 开中断后执行耗时的I2C操作
				gpio_driver_strength(cur_scl_io, GPIO_DS_G3);
				gpio_driver_strength(cur_sda_io, GPIO_DS_G3);
				gpio_set_mode(cur_scl_io, GPIO_OPENDRAIN_PULL_UP, GPIO_PULL_LEVEL_4_7K);
				gpio_set_mode(cur_sda_io, GPIO_OPENDRAIN_PULL_UP, GPIO_PULL_LEVEL_4_7K);
				if(cur_i2c == iic_dev){
					gpio_iomap_inout(cur_scl_io, GPIO_IOMAP_IN_SPI1_SCK_IN, GPIO_IOMAP_OUT_SPI1_SCK_OUT);
					gpio_iomap_inout(cur_sda_io, GPIO_IOMAP_IN_SPI1_IO0_IN, GPIO_IOMAP_OUT_SPI1_IO0_OUT);
				}else{
					gpio_iomap_inout(cur_scl_io, GPIO_IOMAP_IN_SPI2_SCK_IN, GPIO_IOMAP_OUT_SPI2_SCK_OUT);
					gpio_iomap_inout(cur_sda_io, GPIO_IOMAP_IN_SPI2_IO0_IN, GPIO_IOMAP_OUT_SPI2_IO0_OUT);
				}
				if(cur_id_addr == 0){
					i2c_ioctl(cur_i2c,IIC_SET_DEVICE_ADDR,cur_table[2]);
				}
				if(cur_rw_sta == 0){
					if (cur_trx_buff != NULL) {
						i2c_read(cur_i2c,(int8*)&cur_table[3],cur_table[0],(int8*)cur_trx_buff,cur_table[1]);
					} else {
						i2c_read(cur_i2c,(int8*)&cur_table[3],cur_table[0],(int8*)&cur_table[3+cur_table[0]],cur_table[1]);
					}
				}else if(cur_rw_sta == 1){
					i2c_write(cur_i2c, (int8*)&cur_table[3],cur_table[0], (int8*)&cur_table[3+cur_table[0]], cur_table[1]);
				}else if(cur_rw_sta == 2){
					table_scatter_data = (scatter_data*)cur_table;
					i2c_master_write_table(cur_i2c, cur_len, cur_id_addr, table_scatter_data);		
				}

				gpio_set_dir(cur_scl_io, GPIO_DIR_INPUT);
				gpio_set_dir(cur_sda_io, GPIO_DIR_INPUT);
				gpio_iomap_input(cur_scl_io,GPIO_IOMAP_INPUT);
				gpio_iomap_input(cur_sda_io,GPIO_IOMAP_INPUT);
				// 关中断设置sta=0
				irq_flags = disable_irq();
				found_dev->sta = 0;
				// 如果此节点已被标记删除，立即清理（unregister不会free，由这里负责）
				if(found_dev->deleted){
					list_del(&found_dev->list);
					enable_irq(irq_flags);
					os_printf("iic_run_thread free deleted dev after processing devid:%d\n",found_dev->devid);
					free(found_dev);
				}else{
					enable_irq(irq_flags);
				}
			}
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
	os_task_create("iic_thread", iic_run_thread, NULL, OS_TASK_PRIORITY_HIGH-1, 0, NULL, 1024);
}

