#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/gen420.h"


int32 gen420_open(struct gen420_device *p_gen)
{
    if (p_gen && ((const struct gen420_hal_ops *)p_gen->dev.ops)->open) {
        return ((const struct gen420_hal_ops *)p_gen->dev.ops)->open(p_gen);
    }
    return RET_ERR;
}

int32 gen420_close(struct gen420_device *p_gen)
{
    if (p_gen && ((const struct gen420_hal_ops *)p_gen->dev.ops)->close) {
        return ((const struct gen420_hal_ops *)p_gen->dev.ops)->close(p_gen);
    }
    return RET_ERR;
}

int32 gen420_request_irq(struct gen420_device *p_gen, uint32 irq_flags, gen420_irq_hdl irq_hdl, uint32 irq_data)
{
    if (p_gen && ((const struct gen420_hal_ops *)p_gen->dev.ops)->request_irq) {
        return ((const struct gen420_hal_ops *)p_gen->dev.ops)->request_irq(p_gen, irq_flags, irq_hdl, irq_data);
    }
    return RET_ERR;
}

int32 gen420_release_irq(struct gen420_device *p_gen, uint32 irq_flags)
{
    if (p_gen && ((const struct gen420_hal_ops *)p_gen->dev.ops)->release_irq) {
        return ((const struct gen420_hal_ops *)p_gen->dev.ops)->release_irq(p_gen, irq_flags);
    }
    return RET_ERR;
}

int32 gen420_sram_linebuf_adr(struct gen420_device *p_gen,uint32 yadr,uint32 uadr,uint32 vadr){
	uint32 yuv_linebuf[3];
	yuv_linebuf[0] = yadr;
	yuv_linebuf[1] = uadr;
	yuv_linebuf[2] = vadr;
    if (p_gen && ((const struct gen420_hal_ops *)p_gen->dev.ops)->ioctl) {
        return ((const struct gen420_hal_ops *)p_gen->dev.ops)->ioctl(p_gen, GEN420_IOCTL_CMD_SET_SRAM_ADR, (uint32)yuv_linebuf, 0);
    }
    return RET_ERR;
}

int32 gen420_psram_adr(struct gen420_device *p_gen,uint32 yadr,uint32 uadr,uint32 vadr){
	uint32 yuv_linebuf[3];
	yuv_linebuf[0] = yadr;
	yuv_linebuf[1] = uadr;
	yuv_linebuf[2] = vadr;
	if (p_gen && ((const struct gen420_hal_ops *)p_gen->dev.ops)->ioctl) {
        return ((const struct gen420_hal_ops *)p_gen->dev.ops)->ioctl(p_gen, GEN420_IOCTL_CMD_SET_PSRAM_ADR, (uint32)yuv_linebuf, 0);
    }
    return RET_ERR;
}

int32 gen420_frame_size(struct gen420_device *p_gen,uint16 w,uint16 h){
    if (p_gen && ((const struct gen420_hal_ops *)p_gen->dev.ops)->ioctl) {
        return ((const struct gen420_hal_ops *)p_gen->dev.ops)->ioctl(p_gen, GEN420_IOCTL_CMD_SET_FRAME_SIZE, w, h);
    }
    return RET_ERR;
}

int32 gen420_dst_h264_and_jpg(struct gen420_device *p_gen,uint8 both){
    if (p_gen && ((const struct gen420_hal_ops *)p_gen->dev.ops)->ioctl) {
        return ((const struct gen420_hal_ops *)p_gen->dev.ops)->ioctl(p_gen, GEN420_IOCTL_CMD_DST_MODE, both,0);
    }
    return RET_ERR;
}

int32 gen420_dma_run(struct gen420_device *p_gen){
    if (p_gen && ((const struct gen420_hal_ops *)p_gen->dev.ops)->ioctl) {
        return ((const struct gen420_hal_ops *)p_gen->dev.ops)->ioctl(p_gen, GEN420_IOCTL_CMD_DMA_RUN,0,0);
    }
    return RET_ERR;
}

