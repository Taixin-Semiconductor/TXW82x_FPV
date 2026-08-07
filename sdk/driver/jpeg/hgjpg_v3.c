#include "sys_config.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "dev/csi/hgdvp.h"
#include "devid.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "dev/jpg/hgjpg.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "hal/jpeg.h"
#include "osal/task.h"
#include "osal/sleep.h"



struct hgjpg_hw
{
    __IO uint32 CSR0;
    __IO uint32 CSR1;
    __IO uint32 CSR2;
    __IO uint32 CSR3;   
    __IO uint32 DMA_CON;
	__IO uint32 DMA_CON1;
    __IO uint32 DMA_STA; 
    __IO uint32 DMA_TADR0;
    __IO uint32 DMA_TADR1;
    __IO uint32 DMA_DLEN; 
    __IO uint32 DMA_DHT_ADR; 	
    __IO uint32 DMA_DADR; 
	__IO uint32 DMA_DTO;
	__IO uint32 DMA_SF_YADR;
	__IO uint32 DMA_SF_UADR;
	__IO uint32 DMA_SF_VADR;		
};


struct hgjpg_table_hw
{
	__IO uint32 DQT[32];	
};

struct hgjpg_huff_hw
{
	__IO uint32_t HUFF[192];	
};

struct jpg_device *p_jpg_global[JPG_NUM];
volatile uint8_t jpg_ready[JPG_NUM];


volatile uint8_t jpg_oe_enable[JPG_NUM];
volatile uint8_t jpg_oe_used[JPG_NUM];
volatile uint8_t jpg_oe_ok[JPG_NUM];

extern volatile uint8 isp_ov_err;


jpg_irq_hdl jpgirq_vector_table[JPG_NUM][JPG_IRQ_NUM];
void * jpgirq_dev_table[JPG_NUM][JPG_IRQ_NUM];

void irq_jpg_enable(struct hgjpg_hw *p_jpg,uint8 mode,uint8 irq){
	if(mode){
		p_jpg->DMA_CON |= BIT(irq+13);
	}else{
		p_jpg->DMA_CON &= ~BIT(irq+13);
	}
}

extern volatile uint8_t vpp_oe;
void JPG_IRQHandler_action(void *p_jpg)
{
	uint32 jpgr = 0;
	uint32 sta = 0;
	uint8_t jpg_err = 0;
	uint8  dec_err = 0;
	uint32 arg = 0;
	uint32 arg2 = 0;
	int8_t loop;
	struct hgjpg *jpg_hw;// = (struct hgjpg*)p_jpg; 
	struct hgjpg_hw *hw;//  = (struct hgjpg_hw *)jpg_hw->hw;
	jpgr = *(volatile uint32_t*)0x40005200;     //fix jpg reg error bug,imp	
	uint8_t src_from;
	if(p_jpg_global[0]){
		jpg_hw = (struct hgjpg*)p_jpg_global[0]; 
		hw	   = (struct hgjpg_hw *)jpg_hw->hw;
		sta = (hw->DMA_STA & ((hw->DMA_CON >> 13) &0x01f));
		src_from = (hw->DMA_CON& 0xE0)>>5;
		for(loop = JPG_IRQ_NUM-1;loop >= 0;loop--)	{
			if(sta&BIT(loop)){
				if(jpg_hw->opened)
				{
					if(loop == DONE_IRQ){
						//_os_printf("--(%d)",SCHED->BW_STA_CNT);
						jpg_hw->addr_count = 0;
						arg = hw->DMA_DLEN;
						if(jpg_hw->deal_time != ((hw->DMA_STA>>16)&0xff)){
							arg2 = 1;
						}
						jpg_hw->deal_time = 0;
					}
					else if(loop == JPG_BUF_ERR){
						_os_printf(KERN_INFO"hw->DMA_STA0:%x\r\n",hw->DMA_STA);
						jpg_hw->addr_count = 0;
						arg = hw->DMA_DLEN;
						jpg_hw->deal_time = 0;
					}else if(loop == JPG_OUTBUF_FULL){
						jpg_hw->deal_time++;
					}

					if(isp_ov_err && (jpg_hw->decode== 0 && (src_from == 0 || src_from == 1 || src_from == 3))){
						if(loop == DONE_IRQ){
							jpgirq_vector_table[0][JPG_BUF_ERR] (loop,(uint32)jpgirq_dev_table[0][JPG_BUF_ERR],arg,arg2);
							jpg_err = 1;
						}
					}
					
					//如果有错,就不再继续执行其他的中断
					if(jpg_err==0 && jpgirq_vector_table[0][loop] != NULL)
						jpgirq_vector_table[0][loop] (loop,(uint32)jpgirq_dev_table[0][loop],arg,arg2);

					if(loop == JPG_BUF_ERR)
					{
						jpg_err = 1;
					}
				}
				hw->DMA_STA = BIT(loop);
			}
		}
	}

	if(p_jpg_global[1]){
		jpg_hw = (struct hgjpg*)p_jpg_global[1]; 
		hw	   = (struct hgjpg_hw *)jpg_hw->hw;
		sta = (hw->DMA_STA & ((hw->DMA_CON >> 13) &0x1f));
		src_from = (hw->DMA_CON& 0xE0)>>5;
		for(loop = JPG_IRQ_NUM-1;loop >= 0;loop--)	{			
			if(sta&BIT(loop)){	
				if(jpg_hw->opened)
				{
					if(loop == DONE_IRQ){				
						jpg_hw->addr_count = 0;
						arg = hw->DMA_DLEN;
						if(jpg_hw->deal_time != ((hw->DMA_STA>>16)&0xff)){
							arg2 = 1;
						}
						jpg_hw->deal_time = 0;
					}
					else if(loop == JPG_BUF_ERR){
						_os_printf(KERN_INFO"hw->DMA_STA1:%x\r\n",hw->DMA_STA);
						jpg_hw->addr_count = 0;
						arg = hw->DMA_DLEN;
						jpg_hw->deal_time = 0;
						if(hw->DMA_STA & BIT(8)){
							dec_err = 1;
						}					
					}else if(loop == JPG_OUTBUF_FULL){
						jpg_hw->deal_time++;
					}
					if(isp_ov_err && (jpg_hw->decode== 0 && (src_from == 0 || src_from == 1 || src_from == 3))){
						if(loop == DONE_IRQ){
							jpgirq_vector_table[1][JPG_BUF_ERR] (loop,(uint32)jpgirq_dev_table[1][JPG_BUF_ERR],arg,arg2);
							jpg_err = 1;
						}
					}				

					if(jpgirq_vector_table[1][loop] != NULL)
						jpgirq_vector_table[1][loop] (loop,(uint32)jpgirq_dev_table[1][loop],arg,arg2);

				
					if(dec_err == 1){
						hw->DMA_CON |= BIT(12);	//复位
						__NOP();__NOP();__NOP();					
					}	
				}
		
				hw->DMA_STA = BIT(loop);
			}
		}	
	}

}

/*
int32 hgjpg_set_len(struct jpg_device *p_jpg,uint32 buflen,uint32 head_reserver){
	struct hgjpg *jpg_hw = (struct hgjpg*)p_jpg; 
	struct hgjpg_hw *hw  = (struct hgjpg_hw *)jpg_hw->hw;
	hw->DMA_CON |= ((buflen - head_reserver)<<16);
	return 0;
}
*/


void jpg_table_init(struct hgjpg_hw *p_jpg,struct hgjpg_table_hw* p_jpg_dqt,struct hgjpg_huff_hw *p_jpg_huff,uint32 table_index){
	uint32 itk = 0;
	uint32 *ptable;
	ptable = (uint32*)quality_tab[table_index];
	for(itk = 0;itk <32;itk++){
		p_jpg_dqt->DQT[itk] = ptable[itk];
	}
	#if 0
	uint16 *huftbl;
	huftbl = (uint16_t*)htable;
	for(itk = 0;itk <384;itk=itk+2){
		p_jpg_huff->HUFF[itk/2] = (uint32)huftbl[itk]|(uint32)huftbl[itk+1]<<12;

	}
	#endif
	p_jpg->DMA_DHT_ADR = (uint32)dhtable;
}

static void jpg_huff_init(struct hgjpg* jpg)
{
	struct hgjpg_huff_hw *p_jpg_huff  = (struct hgjpg_huff_hw *)jpg->huf_hw;
	uint32 itk = 0;
	uint16 *huftbl;
	huftbl = (uint16_t*)htable;
	for(itk = 0;itk <384;itk=itk+2)
	{
		p_jpg_huff->HUFF[itk/2] = (uint32)huftbl[itk]|(uint32)huftbl[itk+1]<<12;
	}
}

void jpg_csr_encode_config(struct hgjpg_hw *p_jpg,uint32 image_h,uint32 image_w){
	p_jpg->CSR1 = (image_h<<16)|BIT(2)|BIT(8);
	if((image_h%16) != 0)
		image_h = ((image_h/16)+1)*16;
	p_jpg->CSR2 = ((image_h * image_w) /256)-1;
	p_jpg->CSR3 = (image_w<<16)|(image_w/16 - 1);

}

int32 hgjpg_open(struct jpg_device *p_jpg){
	struct hgjpg *jpg_hw = (struct hgjpg*)p_jpg; 
	struct hgjpg_hw *hw  = (struct hgjpg_hw *)jpg_hw->hw;
	uint8 jpg_chose;
	if(hw == (void *)MJPEG0_BASE){
		jpg_chose = 0;
	}

	if(hw == (void *)MJPEG1_BASE){
		jpg_chose = 1;
	}

	//SCHED->BW_STA_CYCLE = 60000000;
	//SCHED->CTRL_CON      |=BIT(1);
	//open之前先将pending清除(预防之前pengding有残留)
	hw->DMA_STA = hw->DMA_STA;	
	jpg_hw->decode = 0;
	jpg_hw->opened	= 1;
	jpg_ready[jpg_chose] = 1;
	hw->DMA_CON |= BIT(0);				//enable jpg
	hw->DMA_CON1 |= BIT(5);
	//hw->CSR0 = 1;	
	irq_enable(jpg_hw->irq_num);
	return 0;
}

int32 hgjpg_close(struct jpg_device *p_jpg){
	struct hgjpg *jpg_hw = (struct hgjpg*)p_jpg; 
	struct hgjpg_hw *hw  = (struct hgjpg_hw *)jpg_hw->hw;
	uint8_t jpg_chose = 0;
	uint32_t flag;
    uint32_t in_disable_irq(void);
	if(!(__in_interrupt() || in_disable_irq()))
	{
		flag = disable_irq();
		hw->DMA_CON &= ~(7<<5);
		hw->DMA_CON |= (SOFT_DATA <<5);
		enable_irq(flag);
		os_sleep_ms(1);
	}
	else
	{
		while(hw->DMA_STA&(0x7<<25));
	}
	//jpg_open(jpg_hw->hw,0);
	
	flag = disable_irq();
	hw->DMA_CON &= ~BIT(0);				//disable jpg
	hw->DMA_STA = hw->DMA_STA;
	if(hw == (void *)MJPEG0_BASE){
		jpg_chose = 0;
	}

	if(hw == (void *)MJPEG1_BASE){
		jpg_chose = 1;
	}
	jpg_ready[jpg_chose] = 0;
	enable_irq(flag);
	jpg_hw->opened	= 0;
	jpg_hw->decode = 0;
	jpg_hw->addr_count = 0;
	
	//hw->CSR0 = 0;	
	jpg_hw->jpg_run = 0;
	//os_printf("close hw:%X\n",hw);
	return 0;
}

int32 hgjpg_suspend(struct dev_obj *obj){
	struct hgjpg *jpg_hw = (struct hgjpg*)obj;
	struct hgjpg_hw *hw;
	struct hgjpg_hw *hw_cfg;
	jpg_hw->cfg_backup = (uint32 *)os_malloc(sizeof(struct hgjpg_hw));
	//memcpy((uint8 *)p_jpg->cfg_backup,(uint8 *)jpg_hw->hw,sizeof(struct hgjpg_hw));
	SYSCTRL->CLK_CON3 &= ~BIT(14);					//close jpg clk
	hw_cfg = (struct hgjpg_hw*)jpg_hw->cfg_backup;
	hw     = (struct hgjpg_hw*)jpg_hw->hw;
	//hw_cfg->CSR0 	= hw->CSR0;
	hw_cfg->CSR1 	= hw->CSR1;
	hw_cfg->CSR2 	= hw->CSR2;
	hw_cfg->CSR3 	= hw->CSR3;
	hw_cfg->DMA_CON = hw->DMA_CON;
	hw_cfg->DMA_STA = hw->DMA_STA;
	hw_cfg->DMA_TADR0 = hw->DMA_TADR0;
	hw_cfg->DMA_TADR1 = hw->DMA_TADR1;
	hw_cfg->DMA_DLEN  = hw->DMA_DLEN;
	
	irq_disable(jpg_hw->irq_num);
	return 0;
}

int32 hgjpg_resume(struct dev_obj *obj){
	struct hgjpg *jpg_hw = (struct hgjpg*)obj;
	struct hgjpg_hw *hw  = (struct hgjpg_hw *)jpg_hw->hw;	
	struct hgjpg_hw *hw_cfg;
	struct hgjpg_table_hw *thw  = (struct hgjpg_table_hw *)jpg_hw->thw;
	struct hgjpg_huff_hw *hufhw  = (struct hgjpg_huff_hw *)jpg_hw->huf_hw;
	SYSCTRL->CLK_CON3 |= BIT(14);					//open jpg clk	
	//memcpy((uint8 *)jpg_hw->hw,(uint8 *)p_jpg->cfg_backup,sizeof(struct hgjpg_hw));	
	hw_cfg = (struct hgjpg_hw*)jpg_hw->cfg_backup;
	jpg_table_init(hw,thw,hufhw,0x01);
	//hw->CSR0 	= hw_cfg->CSR0;
	hw->CSR1 	= hw_cfg->CSR1;
	hw->CSR2 	= hw_cfg->CSR2;
	hw->CSR3 	= hw_cfg->CSR3;
	hw->DMA_STA = hw_cfg->DMA_STA;
	hw->DMA_TADR0 = hw_cfg->DMA_TADR0;
	hw->DMA_TADR1 = hw_cfg->DMA_TADR1;
	hw->DMA_DLEN  = hw_cfg->DMA_DLEN;
	hw->DMA_CON = hw_cfg->DMA_CON;
	
	irq_enable(jpg_hw->irq_num);
	os_free(jpg_hw->cfg_backup);
	return 0;
}


int32 hgjpg_init(struct jpg_device *p_jpg,uint32 table_index,uint32 qt){
	struct hgjpg *jpg_hw = (struct hgjpg*)p_jpg; 
	struct hgjpg_hw *hw  = (struct hgjpg_hw *)jpg_hw->hw;
	struct hgjpg_table_hw *thw  = (struct hgjpg_table_hw *)jpg_hw->thw;
	struct hgjpg_huff_hw *hufhw  = (struct hgjpg_huff_hw *)jpg_hw->huf_hw;
	SYSCTRL->CLK_CON3 |= BIT(14);   
	SYSCTRL->SYS_CON0 |= BIT(2);	//
	jpg_table_init(hw,thw,hufhw,table_index);
	hw->DMA_CON = (qt<<1);
	jpg_hw->addr_count = 0;

	return 0;
}



int32 hgjpg_set_addr(struct jpg_device *p_jpg,uint32 param,uint32 buflen){
	struct hgjpg *jpg_hw = (struct hgjpg*)p_jpg; 
	struct hgjpg_hw *hw  = (struct hgjpg_hw *)jpg_hw->hw;
	uint8 jpg_chose;
	if(hw == (void *)MJPEG0_BASE){
		jpg_chose = 0;
	}

	if(hw == (void *)MJPEG1_BASE){
		jpg_chose = 1;
	}

	if(jpg_hw->jpg_run  == 0){
		//printf("#################hw:%X\n",hw);
		//os_printf("jpg_hw->addr_count:%d\n",jpg_hw->addr_count);
		//os_printf("jpg_ready[jpg_chose]:%d\t%d\n",jpg_ready[jpg_chose],jpg_chose);
		//os_printf("jpg_chose:%d\n",jpg_chose);
		jpg_hw->jpg_run = 1;
		jpg_hw->set_buf_len = 1;

	}
	if(jpg_hw->addr_count%2){
		hw->DMA_TADR1 = param;
	}else{
		if(jpg_ready[jpg_chose]){
			
			hw->DMA_TADR0 = param;
			//hw->DMA_CON1 &= ~BIT(5);
			hw->DMA_CON1 |= BIT(5);
			jpg_ready[jpg_chose] = 0;
		}else{
			hw->DMA_TADR0 = param;
		}
	}
	if(jpg_hw->set_buf_len){
		hw->DMA_CON |= ((buflen/4-1)<<20);
		jpg_hw->set_buf_len = 0;
	}
	jpg_hw->addr_count++;
	return 0;
}

//decode to scaler
int32 hgjpg_decode(struct jpg_device *p_jpg,uint32 photo,uint32_t len){
	struct hgjpg *jpg_hw = (struct hgjpg*)p_jpg; 
	struct hgjpg_hw *hw  = (struct hgjpg_hw *)jpg_hw->hw;	
	if(hw->DMA_CON & BIT(0)){
		hw->DMA_CON &= ~BIT(0);
	}	
	hw->DMA_CON &= ~(7<<5);
	hw->DMA_CON |= BIT(12);	//复位
	__NOP();__NOP();__NOP();
	//hw->DMA_CON &= ~BIT(12);
	hw->CSR1 = BIT(3)|BIT(8);
	hw->DMA_DADR  = photo;
	jpg_hw->decode = 1;

	if (len % 4) {
		len = len + 4;
	}

	if(len != 0){
		hw->DMA_DLEN = len;		//需要word对齐
		hw->DMA_CON1 |= BIT(1);
	}	
	else
	{
		hw->DMA_CON1 &= ~BIT(1);
	}
	hw->DMA_CON &= ~(0xf<<1);	
	hw->DMA_CON |= BIT(4);	
	hw->DMA_CON |= BIT(0);	
	return 0;
}

int32 hgjpg_ioctl(struct jpg_device *p_jpg,uint32 cmd,uint32 param1,uint32 param2){
	int32  ret_val = RET_OK;
	struct hgjpg *jpg_hw = (struct hgjpg*)p_jpg; 
	struct hgjpg_hw *hw  = (struct hgjpg_hw *)jpg_hw->hw;
	struct hgjpg_table_hw *thw  = (struct hgjpg_table_hw *)jpg_hw->thw;
	uint8 jpg_chose;
	uint32* dqt;
	uint32 itk;
	
	if(hw == (void *)MJPEG0_BASE){
		jpg_chose = 0;
	}

	if(hw == (void *)MJPEG1_BASE){
		jpg_chose = 1;
	}

	
	switch(cmd){
	case JPG_IOCTL_CMD_IS_ONLINE:
		return jpg_hw->opened;
	break;
	
	case JPG_IOCTL_CMD_SET_ADR:
		hgjpg_set_addr(p_jpg,param1,param2);
	break;
	
	case JPG_IOCTL_CMD_SET_QT:
		hw->DMA_CON &= ~(0xf<<1);
		hw->DMA_CON |= (param1<<1);		
	break;

	case JPG_IOCTL_CMD_SET_SIZE:
		jpg_csr_encode_config(hw,param1,param2);
	break;
	
	case JPG_IOCTL_CMD_UPDATE_QT:
		dqt = (uint32*)param1;
		for(itk = 0;itk <32;itk++){
			thw->DQT[itk] = dqt[itk];
		}	
	break;
	
	case JPG_IOCTL_CMD_DECODE_TAG:
		if(param1){
			hw->DMA_CON |= BIT(8);
		}else{
			hw->DMA_CON &= ~BIT(8);
		}
	break;
/*		
	case JPG_IOCTL_CMD_OPEN_DBG:
		if(param1){
			hw->DMA_CON |= BIT(10);
		}else{
			hw->DMA_CON &= ~BIT(10);
		}
	break;
*/		
	case JPG_IOCTL_CMD_SOFT_FRAME_START:
		hw->DMA_CON |= BIT(10);
	break;
	
	case JPG_IOCTL_CMD_SOFT_KICK:
		hw->DMA_CON |= BIT(9);
	break;

	case JPG_IOCTL_CMD_SET_SRC_FROM:
		hw->DMA_CON &= ~(7<<5);
		hw->DMA_CON |= (param1 <<5);
	break;
	
	case JPG_IOCTL_CMD_HW_CHK:
		if(param1){
			hw->DMA_CON |= BIT(11);
		}else{
			hw->DMA_CON &= ~BIT(11);
		}		
	break;		

	case JPG_IOCTL_CMD_IS_IDLE:
		if(hw->DMA_STA&(0x7<<25)){
			ret_val = 0; 
		}else{
			ret_val = 1; 
		}
	break;
	case JPG_IOCTL_CMD_BUFF_INIT:
		if(param1 == 1){
			hw->DMA_CON1 |= BIT(5);
		}else{
			hw->DMA_CON1 &= ~BIT(5);
		}
	break;
	case JPG_IOCTL_CMD_VSYNC_DLY:
		if(param1 == 1){
			hw->DMA_CON1 |= BIT(4);
		}else{
			hw->DMA_CON1 &= ~BIT(4);
		}
	break;
	case JPG_IOCTL_CMD_SET_SOFT_Y:
		hw->DMA_SF_YADR = param1;
	break;

	case JPG_IOCTL_CMD_SET_SOFT_UV:
		hw->DMA_SF_UADR = param1;
		hw->DMA_SF_VADR = param2;
	break;
	
	case JPG_IOCTL_CMD_TIMEOUT_CNT:
		hw->DMA_DTO = param1;
	break;

	case JPG_IOCTL_CMD_TIMEOUT_EN:
		hw->DMA_CON1 |= BIT(3);
	break;	

	case JPG_IOCTL_CMD_DECAUTO_RUN_EN:
		hw->DMA_CON1 |= BIT(2);
	break;
	
	case JPG_IOCTL_CMD_DEC_LEN_CFG_EN:
		//SET this bit ,may set the dlen len reg, otherwise while auto dec until finish or timeout err happen
		if(param1){
			hw->DMA_CON1 |= BIT(1);
		}else{
			hw->DMA_CON1 &= ~BIT(1);
		}
	break;	
	case JPG_IOCTL_CMD_DEC_FLUSH_EN:
		//set this bit,dec while run ,but not set data to memory,just check dec right or false
		if(param1){
			hw->DMA_CON1 |= BIT(0);
		}else{
			hw->DMA_CON1 &= ~BIT(0);
		}
	break;

	case JPG_IOCTL_CMD_CODEC_RESET:
		hw->DMA_CON |= BIT(12);
	break;

	case JPG_IOCTL_CMD_SET_DLEN:
		hw->DMA_DLEN = param1;
	break;

	case JPG_IOCTL_CMD_SET_READY:
		jpg_ready[jpg_chose] = 1;
	break;

	case JPG_IOCTL_CMD_SET_OE_SELECT:
		if(param1 == 1){
			jpg_oe_enable[jpg_chose] = 1;
			jpg_oe_used[jpg_chose]   = param2;
		}else{
			jpg_oe_enable[jpg_chose] = 0;
		}
	break;
	case JPG_IOCTL_CMD_SET_OE:
		jpg_oe_ok[jpg_chose] = param1;
	break;
	case JPG_IOCTL_CMD_GET_OE:
		if(jpg_oe_enable[jpg_chose] == 1){
			if(jpg_oe_ok[jpg_chose] == jpg_oe_used[jpg_chose]){
				ret_val = 1;
			}else{
				ret_val = 0;
			}
		}else{
			ret_val = 1;
		}
		
	break;
	default:
		ret_val = -ENOTSUPP;
	break;
	}
	return ret_val;	
}


int32 jpgirq_register(struct jpg_device *p_jpg, jpg_irq_hdl isr,uint32 irq_flag, uint32 irq_data)
{
	uint8 jpg_chose;
	struct hgjpg *jpg_hw = (struct hgjpg*)p_jpg; 
	struct hgjpg_hw *hw  = (struct hgjpg_hw *)jpg_hw->hw;
	uint32 irq_num = 0;
	if(hw == (void *)MJPEG0_BASE){
		jpg_chose = 0;
	}

	if(hw == (void *)MJPEG1_BASE){
		jpg_chose = 1;
	}	
	p_jpg_global[jpg_chose] = p_jpg;
	request_irq(MJPEG01_IRQn, JPG_IRQHandler_action, NULL);
	if(irq_flag == JPG_IRQ_FLAG_JPG_DONE ){
		irq_num = DONE_IRQ;
	}else if(irq_flag == JPG_IRQ_FLAG_JPG_BUF_FULL ){
		irq_num = JPG_OUTBUF_FULL;
	}else if(irq_flag == JPG_IRQ_FLAG_ERROR ){
		irq_num = JPG_BUF_ERR;
	}else if(irq_flag == JPG_IRQ_FLAG_PIXEL_DONE ){
		irq_num = JPG_PIXEL_DONE;
	}else if(irq_flag == JPG_IRQ_FLAG_TIME_OUT){
		irq_num = JPG_TIME_OUT;
	}
	irq_jpg_enable(hw, 1, irq_num);
	
	jpgirq_vector_table[jpg_chose][irq_num] = isr;
	jpgirq_dev_table[jpg_chose][irq_num] = (void *)irq_data;
	hw->DMA_STA |= BIT(irq_num);
	return 0;
}

int32 jpgirq_unregister(struct jpg_device *p_jpg,uint32 irq_flag){
	uint8 jpg_chose;
	struct hgjpg *jpg_hw = (struct hgjpg*)p_jpg; 
	struct hgjpg_hw *hw  = (struct hgjpg_hw *)jpg_hw->hw;
	uint32 irq_num = 0;
	if(hw == (void *)MJPEG0_BASE){
		jpg_chose = 0;
	}

	if(hw == (void *)MJPEG1_BASE){
		jpg_chose = 1;
	}
	
	if(irq_flag == JPG_IRQ_FLAG_JPG_DONE ){
		irq_num = DONE_IRQ;
	}else if(irq_flag == JPG_IRQ_FLAG_JPG_BUF_FULL ){
		irq_num = JPG_OUTBUF_FULL;
	}else if(irq_flag == JPG_IRQ_FLAG_ERROR ){
		irq_num = JPG_BUF_ERR;
	}else if(irq_flag == JPG_IRQ_FLAG_PIXEL_DONE ){
		irq_num = JPG_PIXEL_DONE;
	}else if(irq_flag == JPG_IRQ_FLAG_TIME_OUT){
		irq_num = JPG_TIME_OUT;
	}
	
	irq_jpg_enable(hw, 0, irq_num);
	jpgirq_vector_table[jpg_chose][irq_num] = NULL;
	jpgirq_dev_table[jpg_chose][irq_num] = NULL;
	hw->DMA_STA |= BIT(irq_num);
	return 0;	
}

static const struct jpeg_hal_ops jpeg_ops = {
    .open                  = hgjpg_open,
    .close                 = hgjpg_close,
	.init                  = hgjpg_init,
	.decode                = hgjpg_decode,
	.ioctl				   = hgjpg_ioctl,
    .request_irq           = jpgirq_register,
    .release_irq           = jpgirq_unregister,
#ifdef CONFIG_SLEEP	
	.ops.suspend		   = hgjpg_suspend,
	.ops.resume 		   = hgjpg_resume,
#endif
};



void hgjpg_attach(uint32 dev_id, struct hgjpg *jpg)
{
	jpg->dev.dev.ops = (const struct devobj_ops *)&jpeg_ops;
    irq_disable(jpg->irq_num);
    dev_register(dev_id, (struct dev_obj *)jpg);
	jpg_huff_init(jpg);
}





