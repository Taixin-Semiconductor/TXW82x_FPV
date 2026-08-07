#ifndef HAL_CSC_H
#define HAL_CSC_H
#ifdef __cplusplus
extern "C" {
#endif

/* User interrupt handle */
typedef int32(*csc_irq_hdl)(uint32 irq_flags, uint32 irq_data, uint32 param);

enum csc_ioctl_cmd {
	CSC_IOCTL_FORMAT_TRAN,

	CSC_IOCTL_PHOTO_SIZE,

	CSC_IOCTL_COEF_TYPE,

	CSC_IOCTL_INPUT_ADDR,

	CSC_IOCTL_OUTPUT_ADDR,	

	CSC_IOCTL_KICK_RUN,	
};


enum csc_format {
	CSC_BGR565,
    CSC_RGB565,
    CSC_BGR888,
    CSC_RGB888,
	CSC_RGB888P,
	CSC_YUYV422,
	CSC_YVYU422,
	CSC_UYVY422,
	CSC_VYUY422,
	CSC_YUV422P,
	CSC_YUV422SP,
	CSC_YVU422SP,
	CSC_YUV420P,
	CSC_YUV444,
	CSC_YVU444,
	CSC_UVY444,
	CSC_VUY444,
	CSC_YUV444P,
	CSC_YUV444SP,
	CSC_YVU444SP,
} ;



struct csc_device {
    struct dev_obj dev;
};

struct csc_hal_ops {
	struct devobj_ops ops;
    int32(*init)(struct csc_device *csc_dev);
    int32(*ioctl)(struct csc_device *csc_dev, enum csc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct csc_device *csc_dev, uint32 irq_flag, csc_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct csc_device *csc_dev, uint32 irq_flag);
};


int32 csc_init(struct csc_device *p_csc);
int32 csc_request_irq(struct csc_device *p_csc, uint32 irq_flags, csc_irq_hdl irq_hdl, uint32 irq_data);
int32 csc_release_irq(struct csc_device *p_csc, uint32 irq_flags);
int32 csc_set_format(struct csc_device *p_csc, uint8_t input_format,uint8_t output_format);
int32 csc_set_photo_size(struct csc_device *p_csc, uint32_t w,uint32_t h);
int32 csc_set_type(struct csc_device *p_csc, uint8_t type);
int32 csc_set_input_addr(struct csc_device *p_csc, uint32_t in0,uint32_t in1,uint32_t in2);
int32 csc_set_output_addr(struct csc_device *p_csc, uint32_t out0,uint32_t out1,uint32_t out2);
int32 csc_start_run(struct csc_device *p_csc);

#ifdef __cplusplus
}
#endif

#endif
