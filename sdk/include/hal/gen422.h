#ifndef _HAL_GEN422_H_
#define _HAL_GEN422_H_

#ifdef __cplusplus
extern "C" {
#endif

enum gen422_ioctl_cmd {
	GEN422_IOCTL_CMD_SW_ENABLE,

	GEN422_IOCTL_CMD_OUTPUT_SELECT,

	GEN422_IOCTL_CMD_YUV_MODE,

	GEN422_IOCTL_CMD_INPUT_SRC,

	GEN422_IOCTL_CMD_SINGLE_MODE,

	GEN422_IOCTL_CMD_SET_FRAME_SIZE,

	GEN422_IOCTL_CMD_SET_FRAME_TIMING,

	GEN422_IOCTL_CMD_SET_SRAM_ADDR,

	GEN422_IOCTL_CMD_SET_PSRAM_ADDR,

	GEN422_IOCTL_CMD_DMA_RUN,
};


typedef int32(*gen422_irq_hdl)(uint32 irq_flag, uint32 irq_data, uint32 param1, uint32 param2);

struct gen422_device {
    struct dev_obj dev;
};

struct gen422_hal_ops{
    struct devobj_ops ops;
    int32(*open)(struct gen422_device *gen422);
    int32(*close)(struct gen422_device *gen422);
    int32(*ioctl)(struct gen422_device *gen422, enum gen422_ioctl_cmd cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct gen422_device *gen422, uint32 irq_flag, gen422_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct gen422_device *gen422, uint32 irq_flag);
};



int32 gen422_open(struct gen422_device *p_gen);
int32 gen422_close(struct gen422_device *p_gen);
int32 gen422_request_irq(struct gen422_device *p_gen, uint32 irq_flags, gen422_irq_hdl irq_hdl, uint32 irq_data);
int32 gen422_release_irq(struct gen422_device *p_gen, uint32 irq_flags);
int32 gen422_sw_enable(struct gen422_device *p_gen,uint8 enable);
int32 gen422_output_enable(struct gen422_device *p_gen,uint8 vpp_en,uint8 bt_en);
int32 gen422_yuv_mode(struct gen422_device *p_gen,uint8 yuv_mode);
int32 gen422_input_from_sram_or_psram(struct gen422_device *p_gen,uint8 is_sram);
int32 gen422_output_single_mode(struct gen422_device *p_gen,uint8 single);
int32 gen422_frame_size(struct gen422_device *p_gen,uint16 w,uint16 h);
int32 gen422_frame_time(struct gen422_device *p_gen,uint32 vsync,uint32 hsync,uint32 pclk);
int32 gen422_sram_linebuf_adr(struct gen422_device *p_gen,uint32 yadr,uint32 uadr,uint32 vadr);
int32 gen422_psram_adr(struct gen422_device *p_gen,uint32 frame0,uint32 frame1,uint32 w,uint32 h);
int32 gen422_dma_run(struct gen422_device *p_gen);



#ifdef __cplusplus
}
#endif
#endif

