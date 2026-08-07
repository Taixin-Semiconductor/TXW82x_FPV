#include "typesdef.h"
#include "errno.h"
#include "osal/irq.h"
#include "devid.h"
#include "dev/osd_enc/hgosd_enc.h"
#include "osal/string.h"

struct hgosdenc_hw
{
    __IO uint32 OSD_ENC_CON;
    __IO uint32 OSD_ENC_STA;    
    __IO uint32 OSD_ENC_SADR;  
    __IO uint32 OSD_ENC_TADR;   
    __IO uint32 OSD_ENC_RLEN;  
    __IO uint32 OSD_ENC_DLEN;  
    __IO uint32 OSD_ENC_IDENT0; 
    __IO uint32 OSD_ENC_IDENT1;
    __IO uint32 OSD_ENC_TRANS0;  
    __IO uint32 OSD_ENC_TRANS1;
};

static int32 hgosd_ioctl(struct osdenc_device *p_osd, enum osdenc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2)
{
	int32  ret_val = RET_OK;
	struct hgosd *osd_hw = (struct hgosd*)p_osd; 
	struct hgosdenc_hw *hw  = (struct hgosdenc_hw *)osd_hw->hw;
	uint32_t *p32;
	switch(ioctl_cmd){
		case OSD_IOCTL_TRAN_IDENT_TRANS:
			p32 = (uint32_t *)param1;
			hw->OSD_ENC_IDENT0 = p32[0];
			hw->OSD_ENC_IDENT1 = p32[1];
			hw->OSD_ENC_TRANS0 = p32[2];
			hw->OSD_ENC_TRANS1 = p32[3];		
		break;

		case OSD_IOCTL_ENC_ADR:
			hw->OSD_ENC_SADR = param1;
			hw->OSD_ENC_TADR = param2;			
		break;

		case OSD_IOCTL_SET_ENC_RLEN:
			hw->OSD_ENC_RLEN = ((param1/4 + 1)/2)*2 - 1; //word 偶对齐 -1
		break;

		case OSD_IOCTL_SET_ENC_FORMAT:
			if(param1 == 1){
				hw->OSD_ENC_CON &= ~BIT(1);    //565
			}else{
				hw->OSD_ENC_CON |= BIT(1);     //888
			}
		break;

		case OSD_IOCTL_GET_ENC_DLEN:
			return hw->OSD_ENC_DLEN*4;
		break;

		case OSD_IOCTL_SET_ENC_RUN:
			hw->OSD_ENC_CON |= BIT(0);     
		break;
		default:
			os_printf("NO OSD IOCTL:%d\r\n",ioctl_cmd);
			ret_val = -ENOTSUPP;
		break;
	}

	
	return ret_val;
}


osdenc_irq_hdl encirq_vector_table[ENC_IRQ_NUM];
void * encirq_dev_table[ENC_IRQ_NUM];

void irq_osdenc_enable(struct osdenc_device *p_osd,uint8 mode,uint8 irq){
	struct hgosd *osd_hw = (struct hgosd*)p_osd; 
	struct hgosdenc_hw *hw  = (struct hgosdenc_hw *)osd_hw->hw;	
	if(mode){
		hw->OSD_ENC_CON |= BIT(irq+2);
	}else{
		hw->OSD_ENC_CON &= ~BIT(irq+2);
	}
}

void OSD_ENC_IRQHandler_action(void *p_osd){
	uint32 sta = 0;
	uint8 loop;
	struct hgosd *osd_hw = (struct hgosd*)p_osd; 
	struct hgosdenc_hw *hw  = (struct hgosdenc_hw *)osd_hw->hw;	
	sta = hw->OSD_ENC_STA;
	for(loop = 0;loop < ENC_IRQ_NUM;loop++){
		if(sta&BIT(loop)){
			hw->OSD_ENC_STA = BIT(loop);
			if(encirq_vector_table[loop] != NULL)
				encirq_vector_table[loop] (loop,(uint32)encirq_dev_table[loop],0);
		}
	}
}

int32 osd_enc_irq_register(struct osdenc_device *p_osd,uint32 irq, osdenc_irq_hdl isr, uint32 dev_id){
	struct hgosd *osd_hw = (struct hgosd*)p_osd;	
	struct hgosdenc_hw *hw  = (struct hgosdenc_hw *)osd_hw->hw;
	request_irq(osd_hw->irq_num, OSD_ENC_IRQHandler_action, p_osd);
	irq_osdenc_enable(p_osd, 1, irq);
	encirq_vector_table[irq] = isr;
	encirq_dev_table[irq] = (void*)dev_id;
	hw->OSD_ENC_STA |= BIT(irq);
	os_printf("osdencirq_register:%d %x  %x\r\n",irq,(uint32)encirq_vector_table[irq],(uint32)isr);
	return 0;
}


int32 osd_enc_irq_unregister(struct osdenc_device *p_osd,uint32 irq){
	struct hgosd *osd_hw = (struct hgosd*)p_osd;	
	struct hgosdenc_hw *hw  = (struct hgosdenc_hw *)osd_hw->hw;
	irq_osdenc_enable(p_osd, 0, irq);
	encirq_vector_table[irq] = NULL;
	encirq_dev_table[irq] = 0;
	hw->OSD_ENC_STA |= BIT(irq);
	return 0;
}

static int32 hgosdenc_open(struct osdenc_device *p_osd){
	struct hgosd *osd_hw = (struct hgosd*)p_osd;	
	//struct hgosdenc_hw *hw  = (struct hgosdenc_hw *)osd_hw->hw;
#ifdef 	FPGA_SUPPORT
	SYSCTRL->SYS_CON0 |= BIT(12); 
#else
	sysctrl_osd_enc_clk_open();
	SYSCTRL->SYS_CON7 |= BIT(15);
#endif
	irq_enable(osd_hw->irq_num);
	return 0;
}

static int32 hgosdenc_close(struct osdenc_device *p_osd){
	struct hgosd *osd_hw = (struct hgosd*)p_osd;	
	//struct hgosdenc_hw *hw  = (struct hgosdenc_hw *)osd_hw->hw;
	sysctrl_osd_enc_clk_close();
	irq_disable(osd_hw->irq_num);
	return 0;
}

static const struct osdenc_hal_ops dev_ops = {
    .open        = hgosdenc_open,
    .close       = hgosdenc_close,
    .ioctl       = hgosd_ioctl,
    .request_irq = osd_enc_irq_register,
    .release_irq = osd_enc_irq_unregister,
};


int32 hgosdenc_attach(uint32 dev_id, struct hgosd *hgosd){
    hgosd->opened          = 0;
    hgosd->use_dma         = 0;
    hgosd->irq_hdl                   = NULL;
    hgosd->irq_data                  = 0;
	hgosd->dev.dev.ops = (const struct devobj_ops *)&dev_ops;
    irq_disable(hgosd->irq_num);
    dev_register(dev_id, (struct dev_obj *)hgosd);	
	return 0;
}


