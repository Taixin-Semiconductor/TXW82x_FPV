#ifndef _HAL_GEN420_H_
#define _HAL_GEN420_H_

#ifdef __cplusplus
extern "C" {
#endif

enum gen420_ioctl_cmd {
	GEN420_IOCTL_CMD_SET_SRAM_ADR,

	GEN420_IOCTL_CMD_SET_PSRAM_ADR,

	GEN420_IOCTL_CMD_SET_FRAME_SIZE,

	GEN420_IOCTL_CMD_DST_MODE,

	GEN420_IOCTL_CMD_DMA_RUN,
};


typedef int32(*gen420_irq_hdl)(uint32 irq_flag, uint32 irq_data, uint32 param1, uint32 param2);

struct gen420_device {
    struct dev_obj dev;
};

struct gen420_hal_ops{
    struct devobj_ops ops;
    int32(*open)(struct gen420_device *gen420);
    int32(*close)(struct gen420_device *gen420);
    int32(*ioctl)(struct gen420_device *gen420, enum gen420_ioctl_cmd cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct gen420_device *gen420,uint32 irq_flag,gen420_irq_hdl irq_hdl,  uint32 irq_data);
    int32(*release_irq)(struct gen420_device *gen420, uint32 irq_flag);
};

int32 gen420_open(struct gen420_device *p_gen);
int32 gen420_close(struct gen420_device *p_gen);
int32 gen420_request_irq(struct gen420_device *p_gen, uint32 irq_flags, gen420_irq_hdl irq_hdl, uint32 irq_data);
int32 gen420_release_irq(struct gen420_device *p_gen, uint32 irq_flags);
int32 gen420_sram_linebuf_adr(struct gen420_device *p_gen,uint32 yadr,uint32 uadr,uint32 vadr);
int32 gen420_psram_adr(struct gen420_device *p_gen,uint32 yadr,uint32 uadr,uint32 vadr);
int32 gen420_frame_size(struct gen420_device *p_gen,uint16 w,uint16 h);
int32 gen420_dst_h264_and_jpg(struct gen420_device *p_gen,uint8 both);
int32 gen420_dma_run(struct gen420_device *p_gen);

#ifdef __cplusplus
}
#endif
#endif
