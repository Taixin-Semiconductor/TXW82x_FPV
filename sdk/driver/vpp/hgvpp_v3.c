#include "typesdef.h"
#include "errno.h"
#include "osal/irq.h"
#include "devid.h"
#include "dev/vpp/hgvpp.h"
#include "osal/string.h"
#include "video_err.h"

typedef struct hgvpp_hw
{
    __IO uint32_t CON;
    __IO uint32_t CON1;
    __IO uint32_t SIZE;  
    __IO uint32_t DLT;  
    __IO uint32_t DHT; 
    __IO uint32_t STA;  
    __IO uint32_t DMA_YADR;   
    __IO uint32_t DMA_UADR;
    __IO uint32_t DMA_VADR;
    __IO uint32_t DMA_YADR1;   
    __IO uint32_t DMA_UADR1;
    __IO uint32_t DMA_VADR1; 
    __IO uint32_t IWM0_CON;
	__IO uint32_t IWM0_CON1;
    __IO uint32_t IWM0_SIZE;    
    __IO uint32_t IWM0_YUV;
    __IO uint32_t IWM0_YUV1;
    __IO uint32_t IWM0_LADR;
    __IO uint32_t IWM0_HADR;
    __IO uint32_t IWM0_IDX0;    
    __IO uint32_t IWM0_IDX1;
    __IO uint32_t IWM0_IDX2;    
    __IO uint32_t IWM1_CON;
    __IO uint32_t IWM1_SIZE;
    __IO uint32_t IWM1_YUV;
    __IO uint32_t IWM1_LADR;  
    __IO uint32_t IPF_SADR;
    __IO uint32_t MD_CON;
    __IO uint32_t MD_WIN_CON0;
    __IO uint32_t MD_WIN_CON1; 
    __IO uint32_t MD_BASE_ADDR;
	__IO uint32_t ITP_PSRAM_YADR;
	__IO uint32_t ITP_PSRAM_UADR;
	__IO uint32_t ITP_PSRAM_VADR; 
	__IO uint32_t FRM_PSRAM_YCNT;
	__IO uint32_t FRM_PSRAM_UVCNT; 
	__IO uint32_t FRM1_PSRAM_YCNT;
	__IO uint32_t FRM1_PSRAM_UVCNT; 	
}SDK_HGVPP_HW;


vpp_irq_hdl vppirq_vector_table[VPP_IRQ_NUM];
volatile uint32 vppirq_dev_table[VPP_IRQ_NUM];

extern volatile uint8 isp_ov_err;

static int32 hgvpp_ioctl(struct vpp_device *p_vpp, enum vpp_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2)
{
    int32  ret_val = RET_OK;
	uint32  w,h,num;
	uint32 x_s,y_s,x_e,y_e;
	//uint32 *size_array;
	struct hgvpp *vpp_hw = (struct hgvpp*)p_vpp;	
	SDK_HGVPP_HW *hw  = (SDK_HGVPP_HW *)vpp_hw->hw;
	
	if(vpp_hw->clk_en == 0){
		vpp_hw->clk_en = 1;
        SYSCTRL_REG_OPT(
    		SYSCTRL->CLK_CON5 |= BIT(2);     //vpp clk en
    		SYSCTRL->SYS_CON7 &= ~BIT(26);   
    		SYSCTRL->SYS_CON7 |= BIT(26);    //vpp rst  		
		);
	}


	switch(ioctl_cmd){
		case VPP_IOCTL_CMD_ITP_AUTO_CLOSE:
			if(param1)
				hw->CON &= ~BIT(30);		
			else
				hw->CON |= BIT(30);		
		break;

		case VPP_IOCTL_CMD_ITP_Y_ADDR:
			hw->ITP_PSRAM_YADR = param1;
		break;

		case VPP_IOCTL_CMD_ITP_U_ADDR:
			hw->ITP_PSRAM_UADR = param1;
		break;

		case VPP_IOCTL_CMD_ITP_V_ADDR:
			hw->ITP_PSRAM_VADR = param1;
		break;

		case VPP_IOCTL_CMD_ITP_EN:
			if(param1)
				hw->CON |= BIT(29);
			else
				hw->CON &= ~BIT(29);
		break;		
			
		case VPP_IOCTL_CMD_SET_TAKE_PHOTO_LINEBUF:
			if(param1 < 8){
				hw->CON1 &= ~(0x7<<10);
				hw->CON1 |= (param1<<10);				
			}
		break;
	
		case VPP_IOCTL_CMD_DIS_UV_MODE:
			if(param1)
				hw->CON |= BIT(4);		
			else
				hw->CON &= ~BIT(4);	

			if(param2){
				hw->CON1 |= BIT(5);		
			}else{
				hw->CON1 &= ~BIT(5);
			}
		break;
			
		case VPP_IOCTL_CMD_SET_THRESHOLD:
			hw->DLT = param1;	
			hw->DHT = param2;
		break;

		case VPP_IOCTL_CMD_SET_WATERMARK0_RC:
			if(param1)
				hw->IWM0_CON |= BIT(5);
			else
				hw->IWM0_CON &= ~BIT(5);	
		break;
			
		case VPP_IOCTL_CMD_SET_WATERMARK1_RC:
			if(param1)
				hw->IWM1_CON |= BIT(5);
			else
				hw->IWM1_CON &= ~BIT(5);	
		break;

		case VPP_IOCTL_CMD_SET_WATERMARK0_COLOR:
			hw->IWM0_YUV = 	param1;
		break;
		
		case VPP_IOCTL_CMD_SET_WATERMARK1_COLOR:
			hw->IWM1_YUV =	param1;
		break;
				
		case VPP_IOCTL_CMD_SET_WATERMARK0_BMPADR:
			hw->IWM0_LADR =	param1;
		break;

		case VPP_IOCTL_CMD_SET_WATERMARK1_BMPADR:
			hw->IWM1_LADR =	param1;
		break;
		
		case VPP_IOCTL_CMD_SET_WATERMARK0_LOCATED:
			hw->IWM0_CON &= ~(0x1ff<<15);
			hw->IWM0_CON &= ~(0xff<<24);
			hw->IWM0_CON |= (((param1&0Xff00)>>8)<<24);
			hw->IWM0_CON |= ((param1&0Xff)<<15);			
		break;
		
		case VPP_IOCTL_CMD_SET_WATERMARK1_LOCATED:
			hw->IWM1_CON &= ~(0xff<<16);
			hw->IWM1_CON &= ~(0xff<<24);
			hw->IWM1_CON |= (((param1&0Xff00)>>8)<<24);
			hw->IWM1_CON |= ((param1&0Xff)<<16);			
		break;
		
		case VPP_IOCTL_CMD_SET_WATERMARK0_CONTRAST:
			hw->IWM0_CON &= ~(0x7<<2);
			hw->IWM0_CON |= (param1<<2);
		break;

		case VPP_IOCTL_CMD_SET_WATERMARK1_CONTRAST:
			
			hw->IWM1_CON &= ~(0x7<<2);
			hw->IWM1_CON |= (param1<<2);
		break;		
		
		case VPP_IOCTL_CMD_SET_WATERMARK0_CHAR_SIZE_AND_NUM:
			w = param1&0xff;
			h = ((param1&0xff00)>>8)&0xff; 
			num = param2&0xff;
			hw->IWM0_SIZE &= ~(0x1f<<0);
			hw->IWM0_SIZE |= (num<<0);
			hw->IWM0_SIZE &= ~(0x3f<<5);
			hw->IWM0_SIZE |= (w<<5);
			hw->IWM0_SIZE &= ~(0x3ff<<12);
			hw->IWM0_SIZE |= ((w*num)<<12);
			hw->IWM0_SIZE &= ~(0xff<<23);
			hw->IWM0_SIZE |= (h<<23);
		break;
		
		case VPP_IOCTL_CMD_SET_WATERMARK0_CHAR_IDX:	
			if(param2 > 23)
				return ret_val;
			
			if(param2 < 8){
				hw->IWM0_IDX0 &= ~(0xf<<(param2*4));
				hw->IWM0_IDX0 |= (param1<<(param2*4));
			}
			else if(param2 < 16){
				hw->IWM0_IDX1 &= ~(0xf<<((param2-8)*4));
				hw->IWM0_IDX1 |= (param1<<((param2-8)*4));
			
			}else{
				hw->IWM0_IDX2 &= ~(0xf<<((param2-16)*4));
				hw->IWM0_IDX2 |= (param1<<((param2-16)*4));
			}
		break;
			
		case VPP_IOCTL_CMD_SET_WATERMARK1_PHOTO_SIZE:
			w = param1&0xff;
			h = ((param1&0xff00)>>8)&0xff;	
		
			hw->IWM1_SIZE &= ~(0xff<<12);
			hw->IWM1_SIZE |= (w<<12);
			hw->IWM1_SIZE &= ~(0xff<<23);
			hw->IWM1_SIZE |= (h<<23);
		break;
		
		case VPP_IOCTL_CMD_SET_WATERMARK0_MODE:
			if(param1)
				hw->IWM0_CON |= BIT(1);
			else
				hw->IWM0_CON &= ~BIT(1);	
		break;

		case VPP_IOCTL_CMD_SET_WATERMARK1_MODE:
			if(param1)
				hw->IWM1_CON |= BIT(1);
			else
				hw->IWM1_CON &= ~BIT(1);	
		break;		

		case VPP_IOCTL_CMD_SET_WATERMARK0_EN:
			if(param1)
				hw->IWM0_CON |= BIT(0);
			else
				hw->IWM0_CON &= ~BIT(0);	
		break;		

		case VPP_IOCTL_CMD_SET_WATERMARK1_EN:
			if(param1)
				hw->IWM1_CON |= BIT(0);
			else
				hw->IWM1_CON &= ~BIT(0);	
		break;		
			
		case VPP_IOCTL_CMD_SET_MOTION_DET_EN:
			if(param1){
				//hw->MD_CON &= ~(0x1f<<18);
				//hw->MD_CON |= (BIT(23));				
				hw->MD_CON |= BIT(0);
			}
			else{
				hw->MD_CON &= ~BIT(0);	
			}
		break;

		case VPP_IOCTL_CMD_SET_MOTION_ALL_FRAME_OR_WINDOW:
			if(param1 == 0){
				hw->MD_CON |= (BIT(26));				
			}
			else{
				hw->MD_CON &= ~BIT(26);	
			}			
		break;
			
		case VPP_IOCTL_CMD_SET_MOTION_CALBUF:
			hw->MD_BASE_ADDR = param1;
		break;

		case VPP_IOCTL_CMD_SET_MOTION_RANGE:
			x_s = param1&0xfff;
			y_s = ((param1&0xfff0000)>>16)&0xfff; 
			x_e = param2&0xfff;
			y_e = ((param2&0xfff0000)>>16)&0xfff; 		
			//hw->MD_WIN_CON0 = param1;
			//hw->MD_WIN_CON1 = param1;	
			hw->MD_WIN_CON0 = (x_s | (y_s<<16));
			hw->MD_WIN_CON1 = ((x_e-1) | ((y_e-1)<<16));			
		break;
		
		case VPP_IOCTL_CMD_SET_MOTION_BLK_THRESHOLD:
			param1 = param1&0xff;
			hw->MD_CON &= ~(0xff<<1);
			hw->MD_CON |= (param1<<1);
		break;
		
		case VPP_IOCTL_CMD_SET_MOTION_FRAME_THRESHOLD:
			param1 = param1&0xfff;
			hw->MD_CON &= ~(0xfff<<9);
			hw->MD_CON |= (param1<<9);
		break;

		case VPP_IOCTL_CMD_SET_MOTION_FRAME_INTERVAL:
			param1 = param1&0x1f;
			hw->MD_CON &= ~(0x1f<<21);
			hw->MD_CON |= (param1<<21);
		break;

		case VPP_IOCTL_CMD_SET_IFP_ADDR:
			hw->IPF_SADR = param1;
		break;
		
		case VPP_IOCTL_CMD_SET_IFP_EN:
			if(param1){
				hw->CON |= BIT(28);
			}
			else{
				hw->CON &= ~BIT(28);	
			}
		break;	
			
		case VPP_IOCTL_CMD_SET_MODE:
			if(param1){
				hw->CON |= BIT(1);
				//os_printf("vpp input raw or rgb888\r\n");
			}
			else{
				hw->CON &= ~BIT(1);	
				//os_printf("vpp input YUV422\r\n");
			}
		break;	
			
		case VPP_IOCTL_CMD_SET_SIZE:
			w = param1;
			h = param2;	
		
            if(w%16 != 0){
                os_printf("###############################warning image_w no align 16 Byte##################################################\r\n");	
            }

            if(h%8 != 0){
                os_printf("###############################warning image_h no align 8 Byte##################################################\r\n");	
            }	
            hw->SIZE = (h<<12) | (w<<0);
		break;
		
		case VPP_IOCTL_CMD_SET_YCBCR:
			if(param1 > 3){
				os_printf("set ycbcr error:%d\r\n",param1);
			}
			hw->CON &= ~(BIT(2)|BIT(3));
			hw->CON |= param1<<2;
		break;
		
		case VPP_IOCTL_CMD_BUF0_CNT:
			if(param1 > 16){
				os_printf("set buf0 error:%d\r\n",param1);
			}
			hw->CON &= ~(0x0f<<9);
			hw->CON |= (param1<<9);
		break;

		case VPP_IOCTL_CMD_BUF1_CNT:
			if(param1 > 16){
				os_printf("set buf1 error:%d\r\n",param1);
			}
			hw->CON1 &= ~(0x0f<<6);
			hw->CON1 |= (param1<<6);
		break;		
			
		case VPP_IOCTL_CMD_BUF0_EN:
			if(param1)
				hw->CON |= BIT(8);
			else
				hw->CON &= ~BIT(8);	
		break;	

		case VPP_IOCTL_CMD_BUF1_EN:
			if(param1)
				hw->CON1 |= BIT(1);
			else
				hw->CON1 &= ~BIT(1);	
		break;	

		case VPP_IOCTL_CMD_FTUSB3_EN:
			if(param1)
				hw->CON1 |= BIT(0);
			else
				hw->CON1 &= ~BIT(0);	
		break;
		
		case VPP_IOCTL_CMD_BUF1_SHRINK:
			hw->CON1 &= ~(0x07<<2);
			hw->CON1 |= (param1<<2);    //0: 1/2   
										//1: 1/3  
										//2: 1/4  
										//3: 1/6
										//4: 2/3
										//5: 1/1
		break;	
			
		case VPP_IOCTL_CMD_INPUT_INTERFACE:
			hw->CON &= ~(0x07<<5);
			hw->CON |= (param1<<5);
		break;

		case VPP_IOCTL_CMD_BUF0_Y_ADDR:
			hw->DMA_YADR = param1;
		break;

		case VPP_IOCTL_CMD_BUF0_U_ADDR:
			hw->DMA_UADR = param1;
		break;

		case VPP_IOCTL_CMD_BUF0_V_ADDR:
			hw->DMA_VADR = param1;
		break;

		case VPP_IOCTL_CMD_BUF1_Y_ADDR:
			hw->DMA_YADR1 = param1;
		break;

		case VPP_IOCTL_CMD_BUF1_U_ADDR:
			hw->DMA_UADR1 = param1;
		break;

		case VPP_IOCTL_CMD_BUF1_V_ADDR:
			hw->DMA_VADR1 = param1;
		break;

//        case VPP_IOCTL_CMD_ISP_CONFIG:
//            hw->CON   = 0;
//           hw->CON  |= 0x3 << 20;
//            hw->CON  |= 0x3 << 5;
//            hw->CON  |= 0x1 << 1;
//            hw->CON1 |= 0x1 << 0;
//            hw->CON  |= BIT(0);
//        break;

        case VPP_IOCTL_CMD_GET_STA:
            ret_val = hw->STA;
        break;

		case VPP_IOCTL_CMD_PSRAM_YCNT:
			w = param1;
			h = param2;	
			hw->FRM_PSRAM_YCNT = (w*h)/4 -1;
		break;

		case VPP_IOCTL_CMD_PSRAM_UVCNT:
			w = param1;
			h = param2;
			hw->FRM_PSRAM_UVCNT = (w*h)/16 -1;
		break;

		case VPP_IOCTL_CMD_PSRAM1_YCNT:
			w = param1;
			h = param2;	
			hw->FRM1_PSRAM_YCNT = (w*h)/4 -1;
		break;

		case VPP_IOCTL_CMD_PSRAM1_UVCNT:
			w = param1;
			h = param2;
			hw->FRM1_PSRAM_UVCNT = (w*h)/16 -1;
		break;

		case VPP_IOCTL_CMD_FRAME_SAVE_NO_SRAM:
			hw->CON1 &= ~(3<<13);
			if(param1 == 1){
				hw->CON1 |= (1<<13);
			}else if(param1 == 2){
				hw->CON1 |= (2<<13);
			}			
		break;
			
		case VPP_IOCTL_CMD_F1_SAVE_NO_SRAM:
			if(param1 == 1){			   //frame1 save 
				hw->CON1 |= BIT(15);	   //enable 
			}else{
				hw->CON1 &= ~BIT(15);      //disable
			}
		break;

		case VPP_IOCTL_CMD_GET_F0_IS_PSRAM:
			if((hw->CON1 & (3<<13)) == (1<<13)){
				ret_val = 1;
			}else{
				ret_val = 0; 
			}
		break;

		case VPP_IOCTL_CMD_GET_F1_IS_PSRAM:
			if((hw->CON1 & (3<<13)) == (2<<13)){
				ret_val = 1;
			}else if((hw->CON1 & BIT(15)) == BIT(15)){
				ret_val = 1;
			}else{
				ret_val = 0; 
			}
		break;

		case VPP_IOCTL_CMD_GET_F0_IS_ENABLE:
			if(hw->CON&BIT(8)){
				ret_val = 1; 
			}else{
				ret_val = 0; 
			}
		break;

		case VPP_IOCTL_CMD_GET_F1_IS_ENABLE:
			if(hw->CON1&BIT(1)){
				ret_val = 1; 
			}else{
				ret_val = 0; 
			}
		break;

		case VPP_IOCTL_CMD_GET_F1_MODE:
			if(hw->CON1&BIT(17)){
				ret_val = 1;     //2N mode
			}else{
				ret_val = 0;     //16+2n mode
			}
		break;

		case VPP_IOCTL_CMD_GET_F1_SHRINK:
			ret_val = (hw->CON1&0x07)>>2;			
		break;

		case VPP_IOCTL_CMD_SRAMBUFF_CNT_MODE:
			if(param1 == 1){			   //VPP BUF0
				hw->CON1 |= BIT(16);	   //2N
			}else{
				hw->CON1 &= ~BIT(16);      //16+2N
			}

			if(param2 == 1){			   //VPP BUF1
				hw->CON1 |= BIT(17);	   //2N
			}else{
				hw->CON1 &= ~BIT(17);      //16+2N
			}			
		break;

		case VPP_IOCTL_CMD_SCALEBUF_SELECT:
			if(param1 == 1){			   //SCALE1
				hw->CON1 |= BIT(18);	   //BUF 1
			}else{
				hw->CON1 &= ~BIT(18);      //BUF 0
			}

			if(param2 == 1){			   //SCALE3
				hw->CON1 |= BIT(19);	   //BUF 1
			}else{
				hw->CON1 &= ~BIT(19);      //BUF 0
			}			
		break;

		case VPP_IOCTL_CMD_GET_SCALE1BUF_SELECT:
			if(hw->CON1&BIT(18)){
				ret_val = 1;              //用的是buf1
			}else{
				ret_val = 0;			  //用的是buf0
			}
		break;

		case VPP_IOCTL_CMD_GET_SCALE3BUF_SELECT:
			if(hw->CON1&BIT(19)){
				ret_val = 1;              //用的是buf1
			}else{
				ret_val = 0;			  //用的是buf0
			}
		break;

		case VPP_IOCTL_CMD_GEN420_BUF_SEL:
			if(param1 == 0){			   
				hw->CON1 &= ~BIT(20);	   //BUF 0
			}else{
				hw->CON1 |= BIT(20);       //BUF 1
			}

		break;

		case VPP_IOCTL_CMD_SET_WATERMARK0_AUTO_RC_THRESHOLD:
			hw->IWM0_CON1  = 0;
			hw->IWM0_CON1  = param1/8;
			hw->IWM0_CON1 |= (param2/8)<<18;
		break;

		case VPP_IOCTL_CMD_SET_WATERMARK0_AUTO_RC_COLOR:
			if(param1)
				hw->IWM0_CON |= BIT(6);
			else
				hw->IWM0_CON &= ~BIT(6);
			
			hw->IWM0_YUV1 = param2;
		break;

		case VPP_IOCTL_CMD_SET_WATERMARK0_AUTO_RC_HADR:
			hw->IWM0_HADR = param1;    //sram for moudle cal threshold
		break;

		case VPP_IOCTL_CMD_SET_WATERMARK0_AUTO_RC_MODE:
			if(param1)
				hw->IWM0_CON |= BIT(7);
			else
				hw->IWM0_CON &= ~BIT(7);

		break;
		case VPP_IOCTL_CMD_IS_CLOSED:
			ret_val = vpp_hw->opened?0:1;
		break;
		default:
			os_printf("NO VPP IOCTL:%d\r\n",ioctl_cmd);
            ret_val = -ENOTSUPP;
        break;
	}


	return ret_val;
}

void VPP_IRQHandler_action(void *p_vpp)
{
	uint32 jpgr = 0;
	uint32 sta = 0;
	uint8 loop;
	struct hgvpp *vpp_hw = (struct hgvpp*)p_vpp; 
	struct hgvpp_hw *hw  = (struct hgvpp_hw *)vpp_hw->hw;
	jpgr = *(volatile uint32_t*)0x40005200;     //fix jpg reg error bug,imp
	sta = hw->STA;
	uint8_t isp_err = 0;
	for(loop = 0;loop < VPP_IRQ_NUM;loop++){
		if(sta&BIT(loop)){
			hw->STA = BIT(loop);
			if(loop == IPF_OV_ISR){
				isp_ov_err = ~0;
			}
			isp_err = isp_ov_err & BIT(ISP_VPP_ERR);			
			if(isp_err){
				if(loop == FRAME_DONE_ISR){
					os_printf("isp ov,vpp drop\r\n");
					isp_ov_err &= (~BIT(ISP_VPP_ERR));
				}
			}else{
				if(vppirq_vector_table[loop] != NULL)
					vppirq_vector_table[loop] (loop,vppirq_dev_table[loop],0);
			}
		}
	}
}


void irq_vpp_enable(struct vpp_device *p_vpp,uint8 mode,uint8 irq){
	struct hgvpp *vpp_hw = (struct hgvpp*)p_vpp; 
	struct hgvpp_hw *hw  = (struct hgvpp_hw *)vpp_hw->hw;
	if(mode){
		hw->CON |= BIT(irq+19);
	}else{
		hw->CON &= ~BIT(irq+19);
	}
}


int32 vppirq_register(struct vpp_device *p_vpp,uint32 irq, vpp_irq_hdl isr, uint32 dev_id){
	struct hgvpp *vpp_hw = (struct hgvpp*)p_vpp; 	
	struct hgvpp_hw *hw  = (struct hgvpp_hw *)vpp_hw->hw;
	request_irq(vpp_hw->irq_num, VPP_IRQHandler_action, p_vpp);
	
	irq_vpp_enable(p_vpp, 1, irq);
	vppirq_vector_table[irq] = isr;
	vppirq_dev_table[irq] = dev_id;
	hw->STA |= BIT(irq);
	os_printf("vppirq_register:%d %x  %x\r\n",irq,(uint32)vppirq_vector_table[irq],(uint32)isr);
	return 0;
}


int32 vppirq_unregister(struct vpp_device *p_vpp,uint32 irq){
	struct hgvpp *vpp_hw = (struct hgvpp*)p_vpp;	
	struct hgvpp_hw *hw  = (struct hgvpp_hw *)vpp_hw->hw;
	irq_vpp_enable(p_vpp, 0, irq);
	vppirq_vector_table[irq] = NULL;
	vppirq_dev_table[irq] = 0;
	hw->STA |= BIT(irq);
	return 0;
}


static int32 hgvpp_open(struct vpp_device *p_vpp){
	struct hgvpp *vpp_hw = (struct hgvpp*)p_vpp;	
	struct hgvpp_hw *hw  = (struct hgvpp_hw *)vpp_hw->hw;
	vpp_hw->opened = 1;
	hw->CON |= BIT(0);
	irq_enable(vpp_hw->irq_num);
	return 0;
}

static int32 hgvpp_close(struct vpp_device *p_vpp){
	struct hgvpp *vpp_hw = (struct hgvpp*)p_vpp; 	
	struct hgvpp_hw *hw  = (struct hgvpp_hw *)vpp_hw->hw;
	vpp_hw->opened = 0;
	vpp_hw->clk_en = 0;
	hw->CON &= ~BIT(0);
	irq_disable(vpp_hw->irq_num);
	return 0;
}


int32 hgvpp_suspend(struct dev_obj *obj){
	struct hgvpp *vpp_hw = (struct hgvpp*)obj;
	struct hgvpp_hw *hw;
	struct hgvpp_hw *hw_cfg;

	if(!vpp_hw->opened || vpp_hw->dsleep)
	{
		return RET_OK;
	}
	vpp_hw->dsleep = 1;
	vpp_hw->cfg_backup = (uint32 *)os_malloc(sizeof(struct hgvpp_hw));
	hw_cfg = (struct hgvpp_hw*)vpp_hw->cfg_backup;
	hw     = (struct hgvpp_hw*)vpp_hw->hw;
	
	hw_cfg->CON = hw->CON;
	hw_cfg->CON1 = hw->CON1;
	hw_cfg->SIZE = hw->SIZE;  
	hw_cfg->DLT = hw->DLT;	
	hw_cfg->DHT = hw->DHT; 
	hw_cfg->STA = hw->STA;	
	hw_cfg->DMA_YADR = hw->DMA_YADR;   
	hw_cfg->DMA_UADR = hw->DMA_UADR;
	hw_cfg->DMA_VADR = hw->DMA_VADR;
	hw_cfg->DMA_YADR1 = hw->DMA_YADR1;	 
	hw_cfg->DMA_UADR1 = hw->DMA_UADR1;
	hw_cfg->DMA_VADR1 = hw->DMA_VADR1; 
	hw_cfg->IWM0_CON = hw->IWM0_CON;
	hw_cfg->IWM0_CON1 = hw->IWM0_CON1;
	hw_cfg->IWM0_SIZE = hw->IWM0_SIZE;	  
	hw_cfg->IWM0_YUV = hw->IWM0_YUV;
	hw_cfg->IWM0_YUV1 = hw->IWM0_YUV1;
	hw_cfg->IWM0_LADR = hw->IWM0_LADR;
	hw_cfg->IWM0_HADR = hw->IWM0_HADR;
	hw_cfg->IWM0_IDX0 = hw->IWM0_IDX0;	  
	hw_cfg->IWM0_IDX1 = hw->IWM0_IDX1;
	hw_cfg->IWM0_IDX2 = hw->IWM0_IDX2;	  
	hw_cfg->IWM1_CON = hw->IWM1_CON;
	hw_cfg->IWM1_SIZE = hw->IWM1_SIZE;
	hw_cfg->IWM1_YUV = hw->IWM1_YUV;
	hw_cfg->IWM1_LADR = hw->IWM1_LADR;	
	hw_cfg->IPF_SADR = hw->IPF_SADR;
	hw_cfg->MD_CON = hw->MD_CON;
	hw_cfg->MD_WIN_CON0 = hw->MD_WIN_CON0;
	hw_cfg->MD_WIN_CON1 = hw->MD_WIN_CON1; 
	hw_cfg->MD_BASE_ADDR = hw->MD_BASE_ADDR;
	hw_cfg->ITP_PSRAM_YADR = hw->ITP_PSRAM_YADR;
	hw_cfg->ITP_PSRAM_UADR = hw->ITP_PSRAM_UADR;
	hw_cfg->ITP_PSRAM_VADR = hw->ITP_PSRAM_VADR; 
	hw_cfg->FRM_PSRAM_YCNT = hw->FRM_PSRAM_YCNT;
	hw_cfg->FRM_PSRAM_UVCNT = hw->FRM_PSRAM_UVCNT; 
	hw_cfg->FRM1_PSRAM_YCNT = hw->FRM1_PSRAM_YCNT;
	hw_cfg->FRM1_PSRAM_UVCNT = hw->FRM1_PSRAM_UVCNT;

	
	irq_disable(vpp_hw->irq_num);
	return 0;
}

int32 hgvpp_resume(struct dev_obj *obj){
	struct hgvpp *vpp_hw = (struct hgvpp*)obj;
	struct hgvpp_hw *hw;
	struct hgvpp_hw *hw_cfg;

	if(!vpp_hw->opened || !vpp_hw->dsleep)
	{
		vpp_hw->clk_en = 0;	
		return RET_OK;
	}

	vpp_hw->clk_en = 1;
    SYSCTRL_REG_OPT(
    	SYSCTRL->CLK_CON5 |= BIT(2);     //vpp clk en
    	SYSCTRL->SYS_CON7 &= ~BIT(26);   
    	SYSCTRL->SYS_CON7 |= BIT(26);    //vpp rst  		
	);

	vpp_hw->dsleep = 0;	
	hw_cfg = (struct hgvpp_hw*)vpp_hw->cfg_backup;
	hw     = (struct hgvpp_hw*)vpp_hw->hw;
	hw->CON = hw_cfg->CON;
	hw->CON1 = hw_cfg->CON1;
	hw->SIZE = hw_cfg->SIZE;  
	hw->DLT = hw_cfg->DLT;  
	hw->DHT = hw_cfg->DHT; 
	hw->STA = hw_cfg->STA;  
	hw->DMA_YADR = hw_cfg->DMA_YADR;   
	hw->DMA_UADR = hw_cfg->DMA_UADR;
	hw->DMA_VADR = hw_cfg->DMA_VADR;
	hw->DMA_YADR1 = hw_cfg->DMA_YADR1;   
	hw->DMA_UADR1 = hw_cfg->DMA_UADR1;
	hw->DMA_VADR1 = hw_cfg->DMA_VADR1; 
	hw->IWM0_CON = hw_cfg->IWM0_CON;
	hw->IWM0_CON1 = hw_cfg->IWM0_CON1;
	hw->IWM0_SIZE = hw_cfg->IWM0_SIZE;    
	hw->IWM0_YUV = hw_cfg->IWM0_YUV;
	hw->IWM0_YUV1 = hw_cfg->IWM0_YUV1;
	hw->IWM0_LADR = hw_cfg->IWM0_LADR;
	hw->IWM0_HADR = hw_cfg->IWM0_HADR;
	hw->IWM0_IDX0 = hw_cfg->IWM0_IDX0;    
	hw->IWM0_IDX1 = hw_cfg->IWM0_IDX1;
	hw->IWM0_IDX2 = hw_cfg->IWM0_IDX2;    
	hw->IWM1_CON = hw_cfg->IWM1_CON;
	hw->IWM1_SIZE = hw_cfg->IWM1_SIZE;
	hw->IWM1_YUV = hw_cfg->IWM1_YUV;
	hw->IWM1_LADR = hw_cfg->IWM1_LADR;  
	hw->IPF_SADR = hw_cfg->IPF_SADR;
	hw->MD_CON = hw_cfg->MD_CON;
	hw->MD_WIN_CON0 = hw_cfg->MD_WIN_CON0;
	hw->MD_WIN_CON1 = hw_cfg->MD_WIN_CON1; 
	hw->MD_BASE_ADDR = hw_cfg->MD_BASE_ADDR;
	hw->ITP_PSRAM_YADR = hw_cfg->ITP_PSRAM_YADR;
	hw->ITP_PSRAM_UADR = hw_cfg->ITP_PSRAM_UADR;
	hw->ITP_PSRAM_VADR = hw_cfg->ITP_PSRAM_VADR; 
	hw->FRM_PSRAM_YCNT = hw_cfg->FRM_PSRAM_YCNT;
	hw->FRM_PSRAM_UVCNT = hw_cfg->FRM_PSRAM_UVCNT; 
	hw->FRM1_PSRAM_YCNT = hw_cfg->FRM1_PSRAM_YCNT;
	hw->FRM1_PSRAM_UVCNT = hw_cfg->FRM1_PSRAM_UVCNT;  
	irq_enable(vpp_hw->irq_num);
	os_free(vpp_hw->cfg_backup);
	return 0;
}

static const struct vpp_hal_ops dev_ops = {
    .open        = hgvpp_open,
    .close       = hgvpp_close,
    .ioctl       = hgvpp_ioctl,
    .request_irq = vppirq_register,
    .release_irq = vppirq_unregister,
#ifdef CONFIG_SLEEP	
	.ops.suspend = hgvpp_suspend,
	.ops.resume  = hgvpp_resume,
#endif  
};



int32 hgvpp_attach(uint32 dev_id, struct hgvpp *vpp){
    vpp->opened          = 0;
	vpp->clk_en          = 0;
    vpp->use_dma         = 0;
    vpp->irq_hdl                   = NULL;
    //memset(dvp->irq_hdl,0,sizeof(dvp->irq_hdl));
    vpp->irq_data                  = 0;
	//memset(dvp->irq_data,0,sizeof(dvp->irq_data));
	vpp->dev.dev.ops = (const struct devobj_ops *)&dev_ops;

    irq_disable(vpp->irq_num);
    dev_register(dev_id, (struct dev_obj *)vpp);	
	return 0;
}


