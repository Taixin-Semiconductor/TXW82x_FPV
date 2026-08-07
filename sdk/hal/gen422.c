#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/gen422.h"


int32 gen422_open(struct gen422_device *p_gen)
{
    if (p_gen && ((const struct gen422_hal_ops *)p_gen->dev.ops)->open) {
        return ((const struct gen422_hal_ops *)p_gen->dev.ops)->open(p_gen);
    }
    return RET_ERR;
}

int32 gen422_close(struct gen422_device *p_gen)
{
    if (p_gen && ((const struct gen422_hal_ops *)p_gen->dev.ops)->close) {
        return ((const struct gen422_hal_ops *)p_gen->dev.ops)->close(p_gen);
    }
    return RET_ERR;
}

int32 gen422_request_irq(struct gen422_device *p_gen, uint32 irq_flags, gen422_irq_hdl irq_hdl, uint32 irq_data)
{
    if (p_gen && ((const struct gen422_hal_ops *)p_gen->dev.ops)->request_irq) {
        return ((const struct gen422_hal_ops *)p_gen->dev.ops)->request_irq(p_gen, irq_flags, irq_hdl, irq_data);
    }
    return RET_ERR;
}

int32 gen422_release_irq(struct gen422_device *p_gen, uint32 irq_flags)
{
    if (p_gen && ((const struct gen422_hal_ops *)p_gen->dev.ops)->release_irq) {
        return ((const struct gen422_hal_ops *)p_gen->dev.ops)->release_irq(p_gen, irq_flags);
    }
    return RET_ERR;
}

int32 gen422_sw_enable(struct gen422_device *p_gen,uint8 enable){
    if (p_gen && ((const struct gen422_hal_ops *)p_gen->dev.ops)->ioctl) {
        return ((const struct gen422_hal_ops *)p_gen->dev.ops)->ioctl(p_gen, GEN422_IOCTL_CMD_SW_ENABLE, enable, 0);
    }
    return RET_ERR;
}

int32 gen422_output_enable(struct gen422_device *p_gen,uint8 vpp_en,uint8 bt_en){
    if (p_gen && ((const struct gen422_hal_ops *)p_gen->dev.ops)->ioctl) {
        return ((const struct gen422_hal_ops *)p_gen->dev.ops)->ioctl(p_gen, GEN422_IOCTL_CMD_OUTPUT_SELECT, vpp_en, bt_en);
    }
    return RET_ERR;
}

//0 :yuyv  1 :yvyu  2 :uyvy  3 :vyuy
int32 gen422_yuv_mode(struct gen422_device *p_gen,uint8 yuv_mode){
    if (p_gen && ((const struct gen422_hal_ops *)p_gen->dev.ops)->ioctl) {
        return ((const struct gen422_hal_ops *)p_gen->dev.ops)->ioctl(p_gen, GEN422_IOCTL_CMD_YUV_MODE, yuv_mode, 0);
    }
    return RET_ERR;
}

int32 gen422_input_from_sram_or_psram(struct gen422_device *p_gen,uint8 is_sram){
    if (p_gen && ((const struct gen422_hal_ops *)p_gen->dev.ops)->ioctl) {
        return ((const struct gen422_hal_ops *)p_gen->dev.ops)->ioctl(p_gen, GEN422_IOCTL_CMD_INPUT_SRC, is_sram, 0);
    }
    return RET_ERR;
}

int32 gen422_output_single_mode(struct gen422_device *p_gen,uint8 single){
    if (p_gen && ((const struct gen422_hal_ops *)p_gen->dev.ops)->ioctl) {
        return ((const struct gen422_hal_ops *)p_gen->dev.ops)->ioctl(p_gen, GEN422_IOCTL_CMD_SINGLE_MODE, single, 0);
    }
    return RET_ERR;
}

int32 gen422_frame_size(struct gen422_device *p_gen,uint16 w,uint16 h){
    if (p_gen && ((const struct gen422_hal_ops *)p_gen->dev.ops)->ioctl) {
        return ((const struct gen422_hal_ops *)p_gen->dev.ops)->ioctl(p_gen, GEN422_IOCTL_CMD_SET_FRAME_SIZE, w, h);
    }
    return RET_ERR;
}

int32 gen422_frame_time(struct gen422_device *p_gen,uint32 vsync,uint32 hsync,uint32 pclk){
	uint32 time[3];
	time[0] = vsync;
	time[1] = hsync;
	time[2] = pclk;	
	if (p_gen && ((const struct gen422_hal_ops *)p_gen->dev.ops)->ioctl) {
		return ((const struct gen422_hal_ops *)p_gen->dev.ops)->ioctl(p_gen, GEN422_IOCTL_CMD_SET_FRAME_TIMING, (uint32)time, 0);
	}
	return RET_ERR;
}

int32 gen422_sram_linebuf_adr(struct gen422_device *p_gen,uint32 yadr,uint32 uadr,uint32 vadr){
	uint32 yuv_linebuf[3];
	yuv_linebuf[0] = yadr;
	yuv_linebuf[1] = uadr;
	yuv_linebuf[2] = vadr;
    if (p_gen && ((const struct gen422_hal_ops *)p_gen->dev.ops)->ioctl) {
        return ((const struct gen422_hal_ops *)p_gen->dev.ops)->ioctl(p_gen, GEN422_IOCTL_CMD_SET_SRAM_ADDR, (uint32)yuv_linebuf, 0);
    }
    return RET_ERR;
}

int32 gen422_psram_adr(struct gen422_device *p_gen,uint32 frame0,uint32 frame1,uint32 w,uint32 h){
	uint32 framebuf[2];
	framebuf[0] = frame0;
	framebuf[1] = frame1;
	

    if (p_gen && ((const struct gen422_hal_ops *)p_gen->dev.ops)->ioctl) {
        return ((const struct gen422_hal_ops *)p_gen->dev.ops)->ioctl(p_gen, GEN422_IOCTL_CMD_SET_PSRAM_ADDR, (uint32)framebuf, (w<<16)|h);
    }
    return RET_ERR;
}

int32 gen422_dma_run(struct gen422_device *p_gen){
    if (p_gen && ((const struct gen422_hal_ops *)p_gen->dev.ops)->ioctl) {
        return ((const struct gen422_hal_ops *)p_gen->dev.ops)->ioctl(p_gen, GEN422_IOCTL_CMD_DMA_RUN,0,0);
    }
    return RET_ERR;
}


