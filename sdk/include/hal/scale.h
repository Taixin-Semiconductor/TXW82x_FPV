#ifndef HAL_SCALE_H
#define HAL_SCALE_H

#ifdef __cplusplus
extern "C" {
#endif

/* User interrupt handle */
typedef int32(*scale_irq_hdl)(uint32 irq_flags, uint32 irq_data, uint32 param);

enum scale_ioctl_cmd {
    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    SCALE_IOCTL_CMD_SET_IN_OUT_SIZE,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    SCALE_IOCTL_CMD_SET_STEP,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    SCALE_IOCTL_CMD_OUT2SRAM_FRAME,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    SCALE_IOCTL_CMD_INPUT_STREAM,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    SCALE_IOCTL_CMD_SRAM_ROTATE,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    SCALE_IOCTL_CMD_SRAM_MIRROR,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    SCALE_IOCTL_CMD_PSRAM_BURST,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    SCALE_IOCTL_CMD_SET_START_ADDR,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    SCALE_IOCTL_CMD_SET_SRAMBUF_WLEN,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    SCALE_IOCTL_CMD_SET_IN_Y_ADDR,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    SCALE_IOCTL_CMD_SET_IN_U_ADDR,
    
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	SCALE_IOCTL_CMD_SET_IN_V_ADDR,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    SCALE_IOCTL_CMD_SET_OUT_Y_ADDR,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    SCALE_IOCTL_CMD_SET_OUT_U_ADDR,
    
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	SCALE_IOCTL_CMD_SET_OUT_V_ADDR,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	SCALE_IOCTL_CMD_SET_LINEBUF_YUV_ADDR,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	SCALE_IOCTL_CMD_SET_INBUF_YUV_ADDR,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	SCALE_IOCTL_CMD_SET_LINE_BUF_NUM,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	SCALE_IOCTL_CMD_SET_SRAM_ADDR,		

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	SCALE_IOCTL_CMD_DMA_TO_MEMORY,		

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	SCALE_IOCTL_CMD_SET_DATA_FROM,	
		
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	SCALE_IOCTL_CMD_SET_BOTH_MJPG_H264,
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	SCALE_IOCTL_CMD_SET_INPUT_FORMAT,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	SCALE_IOCTL_CMD_SET_INPUT_IS_FRAMEBUF,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	SCALE_IOCTL_CMD_SET_OUTPUT_FORMAT,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	SCALE_IOCTL_CMD_GET_INBUF_NUM,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	SCALE_IOCTL_CMD_SET_INBUF_NUM,		

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	SCALE_IOCTL_CMD_SET_NEW_FRAME,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	SCALE_IOCTL_CMD_SET_END_FRAME,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	SCALE_IOCTL_CMD_GET_HEIGH_CNT,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */	
	SCALE_IOCTL_CMD_GET_INPUT_WIDTH,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */	
	SCALE_IOCTL_CMD_GET_INPUT_HIGH,	
	
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	SCALE_IOCTL_CMD_GET_IS_OPEN,	
};


struct scale_device {
    struct dev_obj dev;
};

struct scale_hal_ops{
	struct devobj_ops ops;
    int32(*open)(struct scale_device *scale_dev);
    int32(*close)(struct scale_device *scale_dev);
    int32(*ioctl)(struct scale_device *scale_dev, enum scale_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct scale_device *scale_dev, uint32 irq_flag, scale_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct scale_device *scale_dev, uint32 irq_flag);	
};


typedef enum {
	MJPEG_DEC,
	H264_DEC,		
	FRAME_YUV420P,
	FRAME_YUV422P,
	FRAME_YUV444P,
	FRAME_RGB888P
}SCALE2_FORMAT;


int32 scale_open(struct scale_device *p_scale);
int32 scale_close(struct scale_device *p_scale);
int32 scale_request_irq(struct scale_device *p_scale, uint32 irq_flags, scale_irq_hdl irq_hdl, uint32 irq_data);
int32 scale_release_irq(struct scale_device *p_scale, uint32 irq_flags);
int32 scale_set_in_out_size(struct scale_device *p_scale, uint32 s_w,uint32 s_h,uint32 o_w,uint32 o_h);
int32 scale_set_step(struct scale_device *p_scale, uint32 s_w,uint32 s_h,uint32 o_w,uint32 o_h);
int32 scale_set_start_addr(struct scale_device *p_scale, uint32 s_x,uint32 s_y);
int32 scale_set_srambuf_wlen(struct scale_device *p_scale, uint32 wlen);
int32 scale_set_in_yaddr(struct scale_device *p_scale, uint32 yaddr);
int32 scale_set_in_uaddr(struct scale_device *p_scale, uint32 uaddr);
int32 scale_set_in_vaddr(struct scale_device *p_scale, uint32 vaddr);
int32 scale_set_out_yaddr(struct scale_device *p_scale, uint32 yaddr);
int32 scale_set_out_uaddr(struct scale_device *p_scale, uint32 uaddr);
int32 scale_set_out_vaddr(struct scale_device *p_scale, uint32 vaddr);
int32 scale_set_line_buf_num(struct scale_device *p_scale, uint32 num);
int32 scale_set_line_buf_addr(struct scale_device *p_scale, uint32 addr);
int32 scale_set_dma_to_memory(struct scale_device *p_scale, uint32 en);
int32 scale_set_data_from_vpp(struct scale_device *p_scale, uint32 en);
int32 scale_get_inbuf_num(struct scale_device *p_scale);
int32 scale_set_inbuf_num(struct scale_device *p_scale,uint32 num,uint32 start_line);
int32 scale_set_new_frame(struct scale_device *p_scale,uint8 en);
int32 scale_set_end_frame(struct scale_device *p_scale,uint8 en);
int32 scale_get_heigh_cnt(struct scale_device *p_scale);
int32 scale_set_input_stream(struct scale_device *p_scale,uint8 stream);
int32 scale_set_output_sram_or_frame(struct scale_device *p_scale,uint8 out_to_sram);
int32 scale_set_rotate_cfg(struct scale_device *p_scale,uint8 rotate);
int32 scale_set_mirror(struct scale_device *p_scale,uint8 mirror);
int32 scale_set_psram_burst(struct scale_device *p_scale,uint8 pbus);
int32 scale_linebuf_yuv_addr(struct scale_device *p_scale, uint32 yadr,uint32 uadr,uint32 vadr);
int32 scale_set_input_yuv_addr(struct scale_device *p_scale, uint32 yadr,uint32 uadr,uint32 vadr);
int32 scale_set_input_format(struct scale_device *p_scale,uint8 format);
int32 scale_is_allframe_or_linebuf(struct scale_device *p_scale,uint8 isframebuf);
int32 scale_set_output_format(struct scale_device *p_scale,uint8 format);
int32 scale_get_is_open(struct scale_device *p_scale);
int32 scale_set_both_mjpg_h264(struct scale_device *p_scale,uint8 en);
int32 scale_get_input_width(struct scale_device *p_scale);
int32 scale_get_input_high(struct scale_device *p_scale);

#ifdef __cplusplus
}
#endif
#endif
