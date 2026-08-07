#include "typesdef.h"
#include "errno.h"
#include "osal/irq.h"
#include "devid.h"
#include "dev/scale/hgscale.h"
#include "osal/string.h"
#include "osal/sleep.h"
uint32_t in_disable_irq(void);


struct hgscale1_hw
{
    __IO uint32 SCALECON;          //0x00
    __IO uint32 SWINCON;           //0x04
	__IO uint32 SSTART;
    __IO uint32 TWINCON;           //0x08
    __IO uint32 SWIDTH_STEP;       //0x0C
    __IO uint32 SHEIGH_STEP;       //0x10
    __IO uint32 INBUF_LINE_NUM;    //0x14
    __IO uint32 INYDMA_STADR;      //0x18
    __IO uint32 INUDMA_STADR;      //0x1C
    __IO uint32 INVDMA_STADR;      //0x20
    __IO uint32 INBUFCON;          //0x24
    __IO uint32 SHEIGH_CNT;        //0x28  
    __IO uint32 SCALESTA;          //0x2C
};

struct hgscale2_hw
{
    __IO uint32 SCALECON;          //0x00
    __IO uint32 SWINCON;           //0x04
    __IO uint32 SSTART;            //0x08
    __IO uint32 TWINCON;           //0x0C
    __IO uint32 SWIDTH_STEP;       //0x10
    __IO uint32 SHEIGH_STEP;       //0x14
    __IO uint32 YSRAMBUF_STADR;    //0x18
    __IO uint32 USRAMBUF_STADR;    //0x18
    __IO uint32 VSRAMBUF_STADR;    //0x18
    __IO uint32 INYDMA_STADR;      //0x1C
    __IO uint32 INUDMA_STADR;      //0x20
    __IO uint32 INVDMA_STADR;      //0x24    
    __IO uint32 OUTYDMA_STADR;     //0x1C
    __IO uint32 OUTUDMA_STADR;     //0x20
    __IO uint32 OUTVDMA_STADR;     //0x24
    __IO uint32 SHEIGH_CNT;        //0x28
    __IO uint32 SCALESTA;          //0x2C

};

struct hgscale3_hw
{
    __IO uint32 SCALECON;   		//0x00
    __IO uint32 SWINCON;         	//0x04
    __IO uint32 SSTART;         	//0x08
    __IO uint32 TWINCON;         	//0x0C
    __IO uint32 SWIDTH_STEP;       //0x10
    __IO uint32 SHEIGH_STEP;       //0x14
    __IO uint32 INBUF_LINE_NUM;    //0x18
    __IO uint32 INYDMA_STADR;      //0x1C
    __IO uint32 INUDMA_STADR;      //0x20
    __IO uint32 INVDMA_STADR;      //0x24
    __IO uint32 OUTYDMA_STADR;     //0x28
    __IO uint32 OUTUDMA_STADR;     //0x2C
    __IO uint32 OUTVDMA_STADR;     //0x30
    __IO uint32 INBUFCON;          //0x34
    __IO uint32 SHEIGH_CNT;        //0x38	
    __IO uint32 SCALESTA;          //0x3C
};

scale_irq_hdl scaleirq1_vector_table[SCALE_IRQ_NUM];
volatile uint32  scaleirq1_dev_table[SCALE_IRQ_NUM];

scale_irq_hdl scaleirq2_vector_table[SCALE_IRQ_NUM];
volatile uint32 scaleirq2_dev_table[SCALE_IRQ_NUM];

scale_irq_hdl scaleirq3_vector_table[SCALE_IRQ_NUM];
volatile uint32  scaleirq3_dev_table[SCALE_IRQ_NUM];

extern volatile uint8 isp_ov_err;
extern volatile uint8 scaler3_lost; 

static int32 hgscale1_ioctl(struct scale_device *p_scale, enum scale_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2)
{
    int32  ret_val = RET_OK;	
	uint32 s_w,s_h,d_w,d_h;
	uint32 s_x,s_y;
	struct hgscale *scale_hw = (struct hgscale*)p_scale;	
	struct hgscale1_hw *hw  = (struct hgscale1_hw *)scale_hw->hw;

	if(scale_hw->clk_en == 0){
		scale_hw->clk_en = 1;
		if((SYSCTRL->CLK_CON4 & BIT(29)) == 0){      //lcd clk not open
			SYSCTRL->CLK_CON4 |= BIT(29);    //lcdc clk en
			SYSCTRL->CLK_CON4 &= ~BIT(30);
    		SYSCTRL->CLK_CON4 |= BIT(30);    //lcdc rest 
			*(volatile uint32_t*)0x400070d4 |= BIT(1);    //scale1 clk enable
		}else{
			*(volatile uint32_t*)0x400070d4 |= BIT(1);    //scale1 clk enable
		}
	}

	
	switch(ioctl_cmd){
		case SCALE_IOCTL_CMD_SET_IN_OUT_SIZE:
			s_w = param1 & 0xffff;
			s_h = (param1>>16) & 0xffff;
			d_w = param2 & 0xffff;
			d_h = (param2>>16) & 0xffff;
			
			hw->SWINCON = (s_w-1)|((s_h-1)<<16);
			hw->TWINCON = (d_w-1)|((d_h-1)<<16);
		break;
		
		case SCALE_IOCTL_CMD_GET_INPUT_WIDTH:
			ret_val = (hw->SWINCON & 0xffff)+1;			
		break;

		case SCALE_IOCTL_CMD_GET_INPUT_HIGH:
			ret_val = ((hw->SWINCON & 0xffff0000)>>16)+1;			
		break;

		case SCALE_IOCTL_CMD_SET_STEP:
			s_w = param1 & 0xffff;
			s_h = (param1>>16) & 0xffff;
			d_w = param2 & 0xffff;
			d_h = (param2>>16) & 0xffff;
			
			hw->SWIDTH_STEP = 256*s_w/d_w;
			hw->SHEIGH_STEP = 256*s_h/d_h;
		break;

		case SCALE_IOCTL_CMD_SET_START_ADDR:
			s_x = param1 & 0xffff;
			s_y = (param1>>16) & 0xffff;
			hw->SSTART = (s_x) |(s_y<<16);
		break;
				
		case SCALE_IOCTL_CMD_SET_LINE_BUF_NUM:
			hw->INBUF_LINE_NUM = param1;
		break;
	
		case SCALE_IOCTL_CMD_SET_IN_Y_ADDR:
			hw->INYDMA_STADR = param1;
		break;

		case SCALE_IOCTL_CMD_SET_IN_U_ADDR:
			hw->INUDMA_STADR = param1;
		break;

		case SCALE_IOCTL_CMD_SET_IN_V_ADDR:
			hw->INVDMA_STADR = param1;
		break;

		case SCALE_IOCTL_CMD_SET_BOTH_MJPG_H264:
			if(param1)
				hw->SCALECON |= BIT(3);				//both mjpg and h264
			else
				hw->SCALECON &= ~BIT(3);		    //
		break;

		case SCALE_IOCTL_CMD_SET_DATA_FROM:
			if(param1)
				hw->SCALECON &=~BIT(2);				//vpp
			else
				hw->SCALECON |= BIT(2);				//soft
		break;

		case SCALE_IOCTL_CMD_GET_INBUF_NUM:
			ret_val = (hw->INBUFCON>>16)&0x3ff;
		break;

		case SCALE_IOCTL_CMD_SET_INBUF_NUM:
			hw->INBUFCON = ((param1)<<16)|(param2);
		break;

		case SCALE_IOCTL_CMD_SET_NEW_FRAME:
			hw->INBUFCON |= BIT(15);
		break;

		case SCALE_IOCTL_CMD_SET_END_FRAME:
			hw->INBUFCON |= BIT(31);
		break;
		
		case SCALE_IOCTL_CMD_GET_HEIGH_CNT:
			ret_val = hw->SHEIGH_CNT;
		break;

		case SCALE_IOCTL_CMD_GET_IS_OPEN:
			if(scale_hw->clk_en){			//判断scaler是否启动在运行
				ret_val = 1;			
			}else{
				ret_val = 0;
			}
		break;
		
		default:
			os_printf("NO SCALE1 IOCTL:%d\r\n",ioctl_cmd);
            ret_val = -ENOTSUPP;
        break;
	}


	return ret_val;
}


static int32 hgscale2_ioctl(struct scale_device *p_scale, enum scale_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2)
{
    int32  ret_val = RET_OK;
	uint32 s_w,s_h,d_w,d_h;
	uint32 s_x,s_y;
	uint32 *param_buf;
	struct hgscale *scale_hw = (struct hgscale*)p_scale;	
	struct hgscale2_hw *hw  = (struct hgscale2_hw *)scale_hw->hw;

	if(scale_hw->clk_en == 0){
		scale_hw->clk_en = 1;
		if((SYSCTRL->CLK_CON4 & BIT(29)) == 0){      //lcd clk not open
			SYSCTRL->CLK_CON4 |= BIT(29);    //lcdc clk en
			SYSCTRL->CLK_CON4 &= ~BIT(30);
    		SYSCTRL->CLK_CON4 |= BIT(30);    //lcdc rest 
			*(volatile uint32_t*)0x400070d4 |= BIT(2);    //scale2 clk enable
		}else{
			*(volatile uint32_t*)0x400070d4 |= BIT(2);    //scale2 clk enable
		}
	}

	
	switch(ioctl_cmd){
		case SCALE_IOCTL_CMD_SET_IN_OUT_SIZE:
			s_w = param1 & 0xffff;
			s_h = (param1>>16) & 0xffff;

			d_w = param2 & 0xffff;
			d_h = (param2>>16) & 0xffff;
			
			hw->SWINCON = (s_w-1)|((s_h-1)<<16);
			hw->TWINCON = (d_w-1)|((d_h-1)<<16);
		break;

		case SCALE_IOCTL_CMD_GET_INPUT_WIDTH:
			ret_val = (hw->SWINCON & 0xffff)+1;			
		break;

		case SCALE_IOCTL_CMD_GET_INPUT_HIGH:
			ret_val = ((hw->SWINCON & 0xffff0000)>>16)+1;			
		break;
		
		case SCALE_IOCTL_CMD_SET_STEP:
			s_w = param1 & 0xffff;
			s_h = (param1>>16) & 0xffff;	
			d_w = param2 & 0xffff;
			d_h = (param2>>16) & 0xffff;
			
			hw->SWIDTH_STEP = 256*s_w/d_w;
			hw->SHEIGH_STEP = 256*s_h/d_h;
		break;

		case SCALE_IOCTL_CMD_INPUT_STREAM:
			hw->SCALECON &= ~(7<<1);    
			hw->SCALECON |= (param1<<1);   //0:jpg decoder
										   //1:h264 decoder
										   //memory frame buf yuv420
										   //memory frame buf yuv422
										   //memory frame buf yuv444
										   //memory frame buf rgb888
		break;

		case SCALE_IOCTL_CMD_SET_NEW_FRAME:
			hw->SCALECON |= BIT(15);
		break;

		case SCALE_IOCTL_CMD_SET_LINE_BUF_NUM:
			hw->SCALECON &= ~(BIT(7)|BIT(8));    //line 16
			if(param1 == 32){
				hw->SCALECON |= BIT(7);
			 }else if(param1 == 64){
				hw->SCALECON |= BIT(8);
			 }else if(param1 == 128){
				hw->SCALECON |= (BIT(7)|BIT(8));
			 }
			
		break;		

		case SCALE_IOCTL_CMD_OUT2SRAM_FRAME:
			if(param1 == 1){
				hw->SCALECON &= ~BIT(6);  //line buf
			}else{
				hw->SCALECON |= BIT(6);   //frame
			}
		break;

		case SCALE_IOCTL_CMD_SRAM_ROTATE:
			hw->SCALECON &= ~(BIT(9)|BIT(10));    //rotate 0
			if(param1 == 1){
				hw->SCALECON |= BIT(9);           //rotate 90
			}else if(param1 == 2){
				hw->SCALECON |= BIT(10);          //rotate 180
			}else if(param1 == 3){
				hw->SCALECON |= (BIT(9)|BIT(10)); //rotate 270
			}
		break;

		case SCALE_IOCTL_CMD_SRAM_MIRROR:
			if(param1 == 0){
				hw->SCALECON &= ~BIT(11);  //no mirror
			}else{
				hw->SCALECON |= BIT(11);   //mirror
			}
		break;
			
		case SCALE_IOCTL_CMD_PSRAM_BURST:
			hw->SCALECON &= ~(BIT(12)|BIT(13));   //8 word
			if(param1 == 16){
				hw->SCALECON |= BIT(12);          //16 word
			}else if(param1 == 32){
				hw->SCALECON |= BIT(13);          //32 word
			}else if(param1 == 64){
				hw->SCALECON |= (BIT(12)|BIT(13));//64 word
			}
		break;

		case SCALE_IOCTL_CMD_SET_START_ADDR:
			s_x = param1 & 0xffff;
			s_y = (param1>>16) & 0xffff;
			hw->SSTART = (s_x) |(s_y<<16);
		break;
		
		case SCALE_IOCTL_CMD_SET_SRAMBUF_WLEN:
			hw->SCALECON &= ~(BIT(4)|BIT(5));
			if(param1 == 64){
				hw->SCALECON |= BIT(4);
			}else if(param1 == 256){
				hw->SCALECON |= BIT(5);
			}else if(param1 == 512){
				hw->SCALECON |= (BIT(4)|BIT(5));
			}
		break;

		case SCALE_IOCTL_CMD_SET_IN_Y_ADDR:
			hw->INYDMA_STADR = param1;
		break;

		case SCALE_IOCTL_CMD_SET_IN_U_ADDR:
			hw->INUDMA_STADR = param1;
		break;

		case SCALE_IOCTL_CMD_SET_IN_V_ADDR:
			hw->INVDMA_STADR = param1;
		break;


		case SCALE_IOCTL_CMD_SET_OUT_Y_ADDR:
			hw->OUTYDMA_STADR = param1;
		break;

		case SCALE_IOCTL_CMD_SET_OUT_U_ADDR:
			hw->OUTUDMA_STADR = param1;
		break;

		case SCALE_IOCTL_CMD_SET_OUT_V_ADDR:
			hw->OUTVDMA_STADR = param1;
		break;

		case SCALE_IOCTL_CMD_SET_LINEBUF_YUV_ADDR:
			param_buf = (uint32*)param1;
			//printf("YUV:%08x %08x %08x\r\n",param_buf[0],param_buf[1],param_buf[2]);
			hw->YSRAMBUF_STADR = param_buf[0];
			hw->USRAMBUF_STADR = param_buf[1];
			hw->VSRAMBUF_STADR = param_buf[2];
		break;

		case SCALE_IOCTL_CMD_SET_INBUF_YUV_ADDR:
			param_buf = (uint32*)param1;
			hw->INYDMA_STADR = param_buf[0];
			hw->INUDMA_STADR = param_buf[1];
			hw->INVDMA_STADR = param_buf[2];
		break;
	
		case SCALE_IOCTL_CMD_GET_HEIGH_CNT:
			ret_val = hw->SHEIGH_CNT;
		break;

		case SCALE_IOCTL_CMD_GET_IS_OPEN:
			if(scale_hw->clk_en){			//判断scaler是否启动在运行
				ret_val = 1;			
			}else{
				ret_val = 0;
			}
		break;

		default:
			os_printf("NO SCALE2 IOCTL:%d\r\n",ioctl_cmd);
            ret_val = -ENOTSUPP;
        break;
	}


	return ret_val;
}

static int32 hgscale3_ioctl(struct scale_device *p_scale, enum scale_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2)
{
    int32  ret_val = RET_OK;	
	uint32 s_w,s_h,d_w,d_h;
	uint32 s_x,s_y;
//	uint32 *size_array;

	struct hgscale *scale_hw = (struct hgscale*)p_scale;	
	struct hgscale3_hw *hw  = (struct hgscale3_hw *)scale_hw->hw;

	if(scale_hw->clk_en == 0){
		scale_hw->clk_en = 1;
		if((SYSCTRL->CLK_CON4 & BIT(29)) == 0){      //lcd clk not open
			SYSCTRL->CLK_CON4 |= BIT(29);    //lcdc clk en
			SYSCTRL->CLK_CON4 &= ~BIT(30);
    		SYSCTRL->CLK_CON4 |= BIT(30);    //lcdc rest 
			*(volatile uint32_t*)0x400070d4 |= BIT(3);    //scale3 clk enable
		}else{
			*(volatile uint32_t*)0x400070d4 |= BIT(3);    //scale3 clk enable
		}
	}
	
	switch(ioctl_cmd){
		case SCALE_IOCTL_CMD_SET_IN_OUT_SIZE:
			s_w = param1 & 0xffff;
			s_h = (param1>>16) & 0xffff;

			d_w = param2 & 0xffff;
			d_h = (param2>>16) & 0xffff;
			
			hw->SWINCON = (s_w-1)|((s_h-1)<<16);
			hw->TWINCON = (d_w-1)|((d_h-1)<<16);

		break;

		case SCALE_IOCTL_CMD_GET_INPUT_WIDTH:
			ret_val = (hw->SWINCON & 0xffff)+1;			
		break;

		case SCALE_IOCTL_CMD_GET_INPUT_HIGH:
			ret_val = ((hw->SWINCON & 0xffff0000)>>16)+1;			
		break;

		case SCALE_IOCTL_CMD_SET_STEP:
			s_w = param1 & 0xffff;
			s_h = (param1>>16) & 0xffff;	
			d_w = param2 & 0xffff;
			d_h = (param2>>16) & 0xffff;
			//printf("s:%d  %d  d:%d  %d \r\n",s_w,s_h,d_w,d_h);
			hw->SWIDTH_STEP = 256*s_w/d_w;
			hw->SHEIGH_STEP = 256*s_h/d_h;
		break;

		case SCALE_IOCTL_CMD_SET_START_ADDR:
			s_x = param1 & 0xffff;
			s_y = (param1>>16) & 0xffff;
			hw->SSTART = (s_x) |(s_y<<16);
		break;


		case SCALE_IOCTL_CMD_SET_LINE_BUF_NUM:
			hw->INBUF_LINE_NUM = param1;
		break;

		case SCALE_IOCTL_CMD_SET_IN_Y_ADDR:
			hw->INYDMA_STADR = param1;
		break;

		case SCALE_IOCTL_CMD_SET_IN_U_ADDR:
			hw->INUDMA_STADR = param1;
		break;

		case SCALE_IOCTL_CMD_SET_IN_V_ADDR:
			hw->INVDMA_STADR = param1;
		break;

		case SCALE_IOCTL_CMD_SET_OUT_Y_ADDR:
			hw->OUTYDMA_STADR = param1;
			//printf("YO:%08x  %08x\r\n",hw->OUTYDMA_STADR,param1);
			//if(param1 < 0x20000000){
			//	hw->SCALECON &= ~BIT(0);  //disable
			//	__BKPT();
			//}
		break;

		case SCALE_IOCTL_CMD_SET_OUT_U_ADDR:
			hw->OUTUDMA_STADR = param1;
			//printf("UO:%08x  %08x\r\n",hw->OUTYDMA_STADR,param1);
		break;

		case SCALE_IOCTL_CMD_SET_OUT_V_ADDR:
			hw->OUTVDMA_STADR = param1;
			//printf("VO:%08x  %08x\r\n",hw->OUTYDMA_STADR,param1);
		break;

		case SCALE_IOCTL_CMD_SET_OUTPUT_FORMAT:
			if(param1 == 0){
				hw->SCALECON &= ~BIT(5);     //yuv420p
			}else{
				hw->SCALECON |= BIT(5);      //rgb888p
			}
		break;

		case SCALE_IOCTL_CMD_SET_INPUT_IS_FRAMEBUF:
			if(param1 == 0){
				hw->SCALECON &= ~BIT(4);     //line buf
			}else{
				hw->SCALECON |= BIT(4);      //all frame
			}
		break;

		case SCALE_IOCTL_CMD_SET_INPUT_FORMAT:
			if(param1 == 0){
				hw->SCALECON &= ~BIT(3);     //YUV420P
			}else{
				hw->SCALECON |= BIT(3);      //RGB888p
			}
		break;
		
		case SCALE_IOCTL_CMD_DMA_TO_MEMORY:
			if(param1){
				hw->SCALECON |= BIT(1);
			}else{
				hw->SCALECON &= ~BIT(1);
			}
		break;

		case SCALE_IOCTL_CMD_SET_DATA_FROM:
			if(param1)
				hw->SCALECON &=~BIT(2);				//vpp
			else
				hw->SCALECON |= BIT(2);				//soft
		break;

		case SCALE_IOCTL_CMD_GET_INBUF_NUM:
			ret_val = (hw->INBUFCON>>16)&0x3ff;
		break;

		case SCALE_IOCTL_CMD_SET_INBUF_NUM:
			//os_printf("set inbuf_num:%d %d\r\n",param1,param2);
			hw->INBUFCON = ((param1)<<16)|(param2);
		break;

		case SCALE_IOCTL_CMD_SET_NEW_FRAME:
			hw->INBUFCON |= BIT(15);
		break;

		case SCALE_IOCTL_CMD_SET_END_FRAME:
			hw->INBUFCON |= BIT(31);
		break;
		
		case SCALE_IOCTL_CMD_GET_HEIGH_CNT:
			ret_val = hw->SHEIGH_CNT;
		break;

		case SCALE_IOCTL_CMD_GET_IS_OPEN:
			if(scale_hw->clk_en){			//判断scaler是否启动在运行
				ret_val = 1;			
			}else{
				ret_val = 0;
			}
		break;

		default:
			os_printf("NO SCALE3 IOCTL:%d\r\n",ioctl_cmd);
            ret_val = -ENOTSUPP;
        break;
	}

	return ret_val;
}


void irq_scale1_enable(struct scale_device *p_scale,uint8 mode,uint8 irq){
	struct hgscale *scale_hw = (struct hgscale*)p_scale; 
	struct hgscale1_hw *hw  = (struct hgscale1_hw *)scale_hw->hw;
	if(mode){
		hw->SCALECON |= BIT(irq+16);
	}else{
		hw->SCALECON &= ~BIT(irq+16);
	}
}

void irq_scale2_enable(struct scale_device *p_scale,uint8 mode,uint8 irq){
	struct hgscale *scale_hw = (struct hgscale*)p_scale; 
	struct hgscale2_hw *hw  = (struct hgscale2_hw *)scale_hw->hw;
	if(mode){
		hw->SCALECON |= BIT(irq+16);
	}else{
		hw->SCALECON &= ~BIT(irq+16);
	}
}

void irq_scale3_enable(struct scale_device *p_scale,uint8 mode,uint8 irq){
	struct hgscale *scale_hw = (struct hgscale*)p_scale; 
	struct hgscale3_hw *hw  = (struct hgscale3_hw *)scale_hw->hw;
	if(mode){
		hw->SCALECON |= BIT(irq+16);
	}else{
		hw->SCALECON &= ~BIT(irq+16);
	}
}

void SCALE1_IRQHandler_action(void *p_scale)
{
	uint32 sta = 0;
	uint8 loop;
	struct hgscale *scale_hw = (struct hgscale*)p_scale; 
	struct hgscale1_hw *hw  = (struct hgscale1_hw *)scale_hw->hw;
	sta = hw->SCALESTA;
	for(loop = 0;loop < SCALE_IRQ_NUM;loop++){
		if(sta&BIT(loop)){
			hw->SCALESTA = BIT(loop);
			if(scaleirq1_vector_table[loop] != NULL)
				scaleirq1_vector_table[loop] (loop,scaleirq1_dev_table[loop],0);
		}
	}
}


int32 scaleirq1_register(struct scale_device *p_scale,uint32 irq, scale_irq_hdl isr, uint32 dev_id){
	struct hgscale *scale_hw = (struct hgscale*)p_scale; 	
	struct hgscale1_hw *hw  = (struct hgscale1_hw *)scale_hw->hw;
	request_irq(scale_hw->irq_num, SCALE1_IRQHandler_action, p_scale);
	
	irq_scale1_enable(p_scale, 1, irq);
	scaleirq1_vector_table[irq] = isr;
	scaleirq1_dev_table[irq] = dev_id;
	hw->SCALESTA |= BIT(irq);
	os_printf("scaleirq1_register:%d %x  %x\r\n",irq,(uint32)scaleirq1_vector_table[irq],(uint32)isr);
	return 0;
}


int32 scaleirq1_unregister(struct scale_device *p_scale,uint32 irq){
	struct hgscale *scale_hw = (struct hgscale*)p_scale;	
	struct hgscale1_hw *hw  = (struct hgscale1_hw *)scale_hw->hw;
	irq_scale1_enable(p_scale, 0, irq);
	scaleirq1_vector_table[irq] = NULL;
	scaleirq1_dev_table[irq] = 0;
	hw->SCALESTA |= BIT(irq);
	return 0;
}


void SCALE2_IRQHandler_action(void *p_scale)
{
	uint32 sta = 0;
	uint8 loop;
	struct hgscale *scale_hw = (struct hgscale*)p_scale; 
	struct hgscale2_hw *hw  = (struct hgscale2_hw *)scale_hw->hw;
	sta = hw->SCALESTA;
	for(loop = 0;loop < SCALE_IRQ_NUM;loop++){
		if(sta&BIT(loop)){
			hw->SCALESTA = BIT(loop);
			if(scaleirq2_vector_table[loop] != NULL)
				scaleirq2_vector_table[loop] (loop,scaleirq2_dev_table[loop],0);
		}
	}
}


int32 scaleirq2_register(struct scale_device *p_scale,uint32 irq, scale_irq_hdl isr, uint32 dev_id){
	struct hgscale *scale_hw = (struct hgscale*)p_scale; 	
	struct hgscale2_hw *hw  = (struct hgscale2_hw *)scale_hw->hw;
	request_irq(scale_hw->irq_num, SCALE2_IRQHandler_action, p_scale);
	
	irq_scale2_enable(p_scale, 1, irq);
	scaleirq2_vector_table[irq] = isr;
	scaleirq2_dev_table[irq] = dev_id;
	hw->SCALESTA |= BIT(irq);
	return 0;
}


int32 scaleirq2_unregister(struct scale_device *p_scale,uint32 irq){
	struct hgscale *scale_hw = (struct hgscale*)p_scale;	
	struct hgscale2_hw *hw  = (struct hgscale2_hw *)scale_hw->hw;
	irq_scale2_enable(p_scale, 0, irq);
	scaleirq2_vector_table[irq] = NULL;
	scaleirq2_dev_table[irq] = 0;
	hw->SCALESTA |= BIT(irq);
	return 0;
}


void SCALE3_IRQHandler_action(void *p_scale)
{
	uint32 sta = 0;
	uint8 loop;
	struct hgscale *scale_hw = (struct hgscale*)p_scale; 
	struct hgscale3_hw *hw  = (struct hgscale3_hw *)scale_hw->hw;
	sta = hw->SCALESTA;
//	printf("STA:%08x  CNT:%08x\r\n",hw->SCALESTA,hw->SHEIGH_CNT);
	for(loop = 0;loop < SCALE_IRQ_NUM;loop++){
		if(sta&BIT(loop)){
			hw->SCALESTA = BIT(loop);
			
			if(scaleirq3_vector_table[loop] != NULL){
				if(scaler3_lost){
					os_printf("isp error,scaler3 drop\r\n");
					scaler3_lost = 0;
				}else{
					scaleirq3_vector_table[loop] (loop,scaleirq3_dev_table[loop],0);
				}
			}
		}
	}
}


int32 scaleirq3_register(struct scale_device *p_scale,uint32 irq, scale_irq_hdl isr, uint32 dev_id){
	struct hgscale *scale_hw = (struct hgscale*)p_scale; 	
	struct hgscale3_hw *hw  = (struct hgscale3_hw *)scale_hw->hw;
	request_irq(scale_hw->irq_num, SCALE3_IRQHandler_action, p_scale);
	
	irq_scale3_enable(p_scale, 1, irq);
	scaleirq3_vector_table[irq] = isr;
	scaleirq3_dev_table[irq] = dev_id;
	hw->SCALESTA |= BIT(irq);
	//os_printf("scaleirq3_register:%d %x  %x\r\n",irq,(uint32)scaleirq3_vector_table[irq],(uint32)isr);
	return 0;
}


int32 scaleirq3_unregister(struct scale_device *p_scale,uint32 irq){
	struct hgscale *scale_hw = (struct hgscale*)p_scale;	
	struct hgscale3_hw *hw  = (struct hgscale3_hw *)scale_hw->hw;
	irq_scale3_enable(p_scale, 0, irq);
	scaleirq3_vector_table[irq] = NULL;
	scaleirq3_dev_table[irq] = 0;
	hw->SCALESTA |= BIT(irq);
	return 0;
}


static int32 hgscale1_open(struct scale_device *p_scale){
	struct hgscale *scale_hw = (struct hgscale*)p_scale;	
	struct hgscale1_hw *hw  = (struct hgscale1_hw *)scale_hw->hw;	
	irq_enable(scale_hw->irq_num);
	hw->SCALECON |= BIT(0);	 //enable
	return 0;
}

static int32 hgscale1_close(struct scale_device *p_scale){
	struct hgscale *scale_hw = (struct hgscale*)p_scale;	
	struct hgscale1_hw *hw  = (struct hgscale1_hw *)scale_hw->hw;
	//如果关闭中断或者在中断调用,就不再处理
	if(!(__in_interrupt() || in_disable_irq()))
	{
		uint32_t timeout = 100;
		while((hw->SCALESTA &BIT(16)) && --timeout)
		{
			os_sleep_ms(1);
		}
	}
	else
	{
		while(hw->SCALESTA &BIT(16)); 
	}
	hw->SCALECON &= ~BIT(0);  //disable
	hw->SCALESTA = hw->SCALESTA;
	scale_hw->clk_en          = 0;
	*(volatile uint32_t*)0x400070d4 &= ~BIT(1);    //scale1 clk enable	
	
	return 0;
}

static int32 hgscale2_open(struct scale_device *p_scale){
	struct hgscale *scale_hw = (struct hgscale*)p_scale;	
	struct hgscale2_hw *hw  = (struct hgscale2_hw *)scale_hw->hw;	
	irq_enable(scale_hw->irq_num);
	hw->SCALECON |= BIT(0);	 //enable
	return 0;
}

static int32 hgscale2_close(struct scale_device *p_scale){
	struct hgscale *scale_hw = (struct hgscale*)p_scale;	
	struct hgscale2_hw *hw  = (struct hgscale2_hw *)scale_hw->hw;
	//如果关闭中断或者在中断调用,就不再处理
	uint32_t in_disable_irq(void);
	if(!(__in_interrupt() || in_disable_irq()))
	{
		uint32_t timeout = 100;
		while((hw->SCALESTA &BIT(16)) && --timeout)
		{
			os_sleep_ms(1);
		}
	}
	else
	{
		while(hw->SCALESTA &BIT(16)); 
	}
	
	hw->SCALECON &= ~BIT(0);  //disable
	hw->SCALESTA = hw->SCALESTA;
	scale_hw->clk_en          = 0;
	*(volatile uint32_t*)0x400070d4 &= ~BIT(2);    //scale2 clk enable	
	return 0;
}


static int32 hgscale3_open(struct scale_device *p_scale){
	struct hgscale *scale_hw = (struct hgscale*)p_scale;	
	struct hgscale3_hw *hw  = (struct hgscale3_hw *)scale_hw->hw;	
	irq_enable(scale_hw->irq_num);
	hw->SCALECON |= BIT(0);	 //enable	
	return 0;
}

static int32 hgscale3_close(struct scale_device *p_scale){
	struct hgscale *scale_hw = (struct hgscale*)p_scale;	
	struct hgscale3_hw *hw  = (struct hgscale3_hw *)scale_hw->hw;
	//如果关闭中断或者在中断调用,就不再处理
	uint32_t in_disable_irq(void);
	if(!(__in_interrupt() || in_disable_irq()))
	{
		uint32_t timeout = 100;
		while((hw->SCALESTA &BIT(16)) && --timeout)
		{
			os_sleep_ms(1);
		}
	}
	else
	{
		while(hw->SCALESTA &BIT(16)); 
	}
	hw->SCALECON &= ~BIT(0);  //disable
	hw->SCALESTA = hw->SCALESTA;
	scale_hw->clk_en          = 0;
	*(volatile uint32_t*)0x400070d4 &= ~BIT(3);    //scale3 clk enable	
	return 0;
}

static const struct scale_hal_ops dev1_ops = {
    .open        = hgscale1_open,
    .close       = hgscale1_close,
    .ioctl       = hgscale1_ioctl,
    .request_irq = scaleirq1_register,
    .release_irq = scaleirq1_unregister,
};

static const struct scale_hal_ops dev2_ops = {
    .open        = hgscale2_open,
    .close       = hgscale2_close,
    .ioctl       = hgscale2_ioctl,
    .request_irq = scaleirq2_register,
    .release_irq = scaleirq2_unregister,
};

static const struct scale_hal_ops dev3_ops = {
    .open        = hgscale3_open,
    .close       = hgscale3_close,
    .ioctl       = hgscale3_ioctl,
    .request_irq = scaleirq3_register,
    .release_irq = scaleirq3_unregister,
};

int32 hgscale1_attach(uint32 dev_id, struct hgscale *scale){
	scale->clk_en          = 0;
    scale->opened          = 0;
    scale->use_dma         = 0;
    scale->irq_hdl                   = NULL;
    //memset(dvp->irq_hdl,0,sizeof(dvp->irq_hdl));
    scale->irq_data                  = 0;
	//memset(dvp->irq_data,0,sizeof(dvp->irq_data));

	scale->dev.dev.ops = (const struct devobj_ops *)&dev1_ops;

    irq_disable(scale->irq_num);
    dev_register(dev_id, (struct dev_obj *)scale);	
	return 0;
}


int32 hgscale2_attach(uint32 dev_id, struct hgscale *scale){
	scale->clk_en		   = 0;
    scale->opened          = 0;
    scale->use_dma         = 0;
    scale->irq_hdl                   = NULL;
    //memset(dvp->irq_hdl,0,sizeof(dvp->irq_hdl));
    scale->irq_data                  = 0;
	//memset(dvp->irq_data,0,sizeof(dvp->irq_data));

	scale->dev.dev.ops = (const struct devobj_ops *)&dev2_ops;

    irq_disable(scale->irq_num);
    dev_register(dev_id, (struct dev_obj *)scale);	
	return 0;
}


int32 hgscale3_attach(uint32 dev_id, struct hgscale *scale){
	scale->clk_en		   = 0;
    scale->opened          = 0;
    scale->use_dma         = 0;
    scale->irq_hdl                   = NULL;
    //memset(dvp->irq_hdl,0,sizeof(dvp->irq_hdl));
    scale->irq_data                  = 0;
	//memset(dvp->irq_data,0,sizeof(dvp->irq_data));
	scale->dev.dev.ops = (const struct devobj_ops *)&dev3_ops;


    irq_disable(scale->irq_num);
    dev_register(dev_id, (struct dev_obj *)scale);	
	return 0;
}



