#ifndef _HAL_DMA2D_H_
#define _HAL_DMA2D_H_

#ifdef __cplusplus
extern "C" {
#endif

enum dma2d_color_type {
    DMA2D_COLOR_TYPE_ARGB8888, 
    DMA2D_COLOR_TYPE_RGB888  , 
    DMA2D_COLOR_TYPE_RGB565  , 
    DMA2D_COLOR_TYPE_ARGB1555, 
    DMA2D_COLOR_TYPE_ARGB4444, 
    DMA2D_COLOR_TYPE_L8      , 
    DMA2D_COLOR_TYPE_AL44    , 
    DMA2D_COLOR_TYPE_AL88    , 
    DMA2D_COLOR_TYPE_L4      , 
    DMA2D_COLOR_TYPE_A8      , 
    DMA2D_COLOR_TYPE_A4      , 
    DMA2D_COLOR_TYPE_RESERVED,
};

enum dma2d_ioctl_type {
    DMA2D_IOCTL_CHECK_DONE,
    DMA2D_IOCTL_CHECK_RESULT,
    DMA2D_IOCTL_SUSPEND,
    DMA2D_IOCTL_SUSPEND_CHECK,
    DMA2D_IOCTL_RESUME,
    DMA2D_IOCTL_RESUME_CHECK,
    DMA2D_IOCTL_ABORT,
    DMA2D_IOCTL_WATERMARK,
    DMA2D_IOCTL_CONFIG_FG,
    DMA2D_IOCTL_CONFIG_BG,
};

enum dma2d_irq_type {
    DMA2D_IRQ_TYPE_TRANS_ERR,
    DMA2D_IRQ_TYPE_TRANS_DONE,
    DMA2D_IRQ_TYPE_TRANS_WM,
    DMA2D_IRQ_TYPE_CLUT_ERR,
    DMA2D_IRQ_TYPE_CLUT_DONE,
    DMA2D_IRQ_TYPE_CONFIG_ERR,
};

enum dma2d_clut_mode_type {
    DMA2D_CLUT_MODE_ARGB8888 = 0,
    DMA2D_CLUT_MODE_RGB888,
};

enum dma2d_work_status {
    DMA2D_STATUS_DONE   = 0,
    DMA2D_STATUS_WORKING,
    DMA2D_STATUS_SUSPEND,
    DMA2D_STATUS_CLUT_ERR,
    DMA2D_STATUS_CONFIG_ERR,
    DMA2D_STATUS_TRANS_ERR,
    DMA2D_STATUS_TIMEOUT,
};

enum dma2d_alpha_type {
    DMA2D_ALPHA_MODE_SAVE       = 0,
    DMA2D_ALPHA_MODE_FIXED_VAL,
    DMA2D_ALPHA_MODE_PRESENTAGE,
	DMA2D_ALPHA_MODE_RESERVED,
};

struct dma2d_src_param {
    uint32 alpha_val : 8, alpha_mode : 8, alpha_reverse : 1, r_b_swap : 1, clut_mode : 4, reserved : 10;
};

struct dma2d_dst_param {
    uint32 r_b_swap : 1, alpha_reverse : 1, byte_swap : 1, reserved : 29;
};

struct dma2d_blkcpy_param {
    uint32 color_mode;
    uint32 src_addr;
    uint32 dst_addr;
    uint32 src_pixel_width        : 16,
           dst_pixel_width        : 16;
    uint32 blk_pixel_width        : 16,
           blk_pixel_height       : 16;
    uint32 src_pixel_start_width  : 16,
           src_pixel_start_height : 16;
    uint32 dst_pixel_start_width  : 16,
           dst_pixel_start_height : 16;
};

struct dma2d_memcpy_param {
    uint32 color_mode;
    uint32 src_addr;
    uint32 dst_addr;
    uint32 src_pixel_width  : 16,
           src_pixel_height : 16;
};

struct dma2d_memset_param {
    uint32 color_mode;
    uint32 color_set;
    uint32 dst_addr;
    uint32 dst_pixel_width    : 16,
           dst_pixel_height   : 16;
    uint32 pixel_start_width  : 16,
           pixel_start_height : 16;
    uint32 set_pixel_width    : 16,
           set_pixel_height   : 16;
};

struct dma2d_convert_param {
    uint32 src_addr;
    uint32 dst_addr;
    uint32 photo_pixel_width  : 16,
           photo_pixel_height : 16;
    uint32 src_color_mode     : 16,
           dst_color_mode     : 16;
    struct dma2d_src_param    src_param;
    struct dma2d_dst_param    dst_param;
};

struct dma2d_mixture_param {
    uint32 photo0_addr;
    uint32 photo1_addr;
    uint32 output_addr;
    uint32 photo0_color_val;
    uint32 photo1_color_val;
    uint32 photo_pixel_width  : 16,
           photo_pixel_height : 16;
    uint32 photo0_color_mode  : 10,
           photo1_color_mode  : 10,
           output_color_mode  : 10,
           reserved           :  2;
    struct dma2d_src_param    src_param[2];
    struct dma2d_dst_param    dst_param;
};


struct dma2d_clut_param {
    uint32 clut_addr;
    uint32 clut_size;
    enum dma2d_clut_mode_type clut_mode;
};

/* User interrupt handle */
typedef void (*dma2d_irq_hdl)(uint32 irq, uint32 irq_data);

/* DMA2D api for user */
struct dma2d_device {
    struct dev_obj dev;
};

struct dma2d_hal_ops{
    struct devobj_ops ops;
    int32(*blkcpy) (struct dma2d_device *dma2d, struct dma2d_blkcpy_param  *p_blkcpy);
    int32(*memcpy) (struct dma2d_device *dma2d, struct dma2d_memcpy_param  *p_memcpy);
    int32(*memset) (struct dma2d_device *dma2d, struct dma2d_memset_param  *p_memset);
	int32(*convert)(struct dma2d_device *dma2d, struct dma2d_convert_param *p_convert);
	int32(*mixture)(struct dma2d_device *dma2d, struct dma2d_mixture_param *p_mixture);
    int32(*ioctl)(struct dma2d_device *dma2d, uint32 cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct dma2d_device *dma2d, uint32 irq_flag, dma2d_irq_hdl irqhdl, uint32 irq_data);
    int32(*release_irq)(struct dma2d_device *dma2d, uint32 irq_flag);
};

/* DMA2d API functions */
int32 dma2d_blkcpy (struct dma2d_device *dma2d, struct dma2d_blkcpy_param  *p_blkcpy);
int32 dma2d_memcpy (struct dma2d_device *dma2d, struct dma2d_memcpy_param  *p_memcpy);
int32 dma2d_memset (struct dma2d_device *dma2d, struct dma2d_memset_param  *p_memset);
int32 dma2d_convert(struct dma2d_device *dam2d, struct dma2d_convert_param *p_convert);
int32 dma2d_mixture(struct dma2d_device *dma2d, struct dma2d_mixture_param *p_mixture);
int32 dma2d_clut_init(struct dma2d_device *dam2d);
int32 dma2d_ioctl(struct dma2d_device *dam2d);
int32 dma2d_request_irq(struct dma2d_device *dam2d);
int32 dma2d_release_irq(struct dma2d_device *dam2d);
int32 dma2d_check_status(struct dma2d_device *dma2d);
int32 dma2d_abort_trans(struct dma2d_device *dma2d);
int32 dma2d_suspend_trans(struct dma2d_device *dma2d);
int32 dma2d_suspend_status(struct dma2d_device *dma2d);
int32 dma2d_resume_trans(struct dma2d_device *dma2d);
int32 dma2d_resume_status(struct dma2d_device *dma2d);
#ifdef __cplusplus
}
#endif

#endif
