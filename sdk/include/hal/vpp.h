#ifndef HAL_VPP_H
#define HAL_VPP_H

#ifdef __cplusplus
extern "C" {
#endif

/* User interrupt handle */
typedef int32(*vpp_irq_hdl)(uint32 irq_flags, uint32 irq_data, uint32 param);

/**
  * @brief UART ioctl_cmd type
  */
enum vpp_ioctl_cmd {
    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_DIS_UV_MODE,


    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_THRESHOLD,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_WATERMARK0_RC,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_WATERMARK1_RC,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_WATERMARK0_COLOR,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_WATERMARK1_COLOR,


    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_WATERMARK0_BMPADR,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_WATERMARK1_BMPADR,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_WATERMARK0_LOCATED,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_WATERMARK1_LOCATED,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_WATERMARK0_CONTRAST,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_WATERMARK1_CONTRAST,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_WATERMARK0_CHAR_SIZE_AND_NUM,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_WATERMARK0_CHAR_IDX,
		
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_SET_TAKE_PHOTO_LINEBUF,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_WATERMARK1_PHOTO_SIZE,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_WATERMARK0_MODE,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_WATERMARK1_MODE,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_WATERMARK0_EN,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_WATERMARK1_EN,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_MOTION_DET_EN,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_MOTION_ALL_FRAME_OR_WINDOW,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_MOTION_CALBUF,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_MOTION_RANGE,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_MOTION_BLK_THRESHOLD,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_MOTION_FRAME_THRESHOLD,


    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_MOTION_FRAME_INTERVAL,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_IFP_ADDR,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    VPP_IOCTL_CMD_SET_IFP_EN,
		
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_SET_MODE,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_SET_SIZE,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_SET_YCBCR,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_BUF0_CNT,

    VPP_IOCTL_CMD_BUF0_EN,
		
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_BUF1_CNT,
	
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_BUF1_EN,
	
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_BUF1_SHRINK,
	
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_INPUT_INTERFACE,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_BUF0_Y_ADDR,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_BUF0_U_ADDR,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_BUF0_V_ADDR,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_BUF1_Y_ADDR,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_BUF1_U_ADDR,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_BUF1_V_ADDR,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_ITP_Y_ADDR,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_ITP_U_ADDR,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_ITP_V_ADDR,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_ITP_EN,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_ITP_AUTO_CLOSE,	

    VPP_IOCTL_CMD_ISP_CONFIG,

    VPP_IOCTL_CMD_GET_STA,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_FTUSB3_EN,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_PSRAM_YCNT,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_PSRAM_UVCNT,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_PSRAM1_YCNT,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_PSRAM1_UVCNT,		

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_FRAME_SAVE_NO_SRAM,		

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_F1_SAVE_NO_SRAM,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_GET_F0_IS_PSRAM,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_GET_F1_IS_PSRAM,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_GET_F0_IS_ENABLE,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_GET_F1_IS_ENABLE,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_GET_F1_MODE,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_GET_F1_SHRINK,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_SRAMBUFF_CNT_MODE,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_SCALEBUF_SELECT,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_GET_SCALE1BUF_SELECT,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_GET_SCALE3BUF_SELECT,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_GEN420_BUF_SEL,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_SET_WATERMARK0_AUTO_RC_THRESHOLD,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_SET_WATERMARK0_AUTO_RC_COLOR,	


	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_SET_WATERMARK0_AUTO_RC_HADR,		


	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	VPP_IOCTL_CMD_SET_WATERMARK0_AUTO_RC_MODE,			
};

struct vpp_device {
    struct dev_obj dev;
};

struct vpp_hal_ops {
	struct devobj_ops ops;
    int32(*open)(struct vpp_device *vpp_dev);
    int32(*close)(struct vpp_device *vpp_dev);
    int32(*ioctl)(struct vpp_device *vpp_dev, enum vpp_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct vpp_device *vpp_dev, uint32 irq_flag, vpp_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct vpp_device *vpp_dev, uint32 irq_flag);	
};

int32 vpp_request_irq(struct vpp_device *p_vpp, uint32 irq_flags, vpp_irq_hdl irq_hdl, uint32 irq_data);
int32 vpp_release_irq(struct vpp_device *p_vpp, uint32 irq_flags);
int32 vpp_open(struct vpp_device *p_vpp);
int32 vpp_close(struct vpp_device *p_vpp);
int32 vpp_dis_uv_mode(struct vpp_device *p_vpp,uint8 enable_buf0,uint8 enable_buf1);
int32 vpp_set_threshold(struct vpp_device *p_vpp,uint32 dlt,uint32 dht);
int32 vpp_set_water0_rc(struct vpp_device *p_vpp,uint8 enable);
int32 vpp_set_water1_rc(struct vpp_device *p_vpp,uint8 enable);
int32 vpp_set_water0_color(struct vpp_device *p_vpp,uint8 y,uint8 u,uint8 v);
int32 vpp_set_water1_color(struct vpp_device *p_vpp,uint8 y,uint8 u,uint8 v);
int32 vpp_set_water0_bitmap(struct vpp_device *p_vpp,uint32 bitmap);
int32 vpp_set_water1_bitmap(struct vpp_device *p_vpp,uint32 bitmap);
int32 vpp_set_water0_locate(struct vpp_device *p_vpp,uint8 x,uint8 y);
int32 vpp_set_water1_locate(struct vpp_device *p_vpp,uint8 x,uint8 y);
int32 vpp_set_water0_contrast(struct vpp_device *p_vpp,uint8 contrast);
int32 vpp_set_water1_contrast(struct vpp_device *p_vpp,uint8 contrast);
int32 vpp_set_watermark0_charsize_and_num(struct vpp_device *p_vpp,uint8 w,uint8 h,uint8 num);
int32 vpp_set_watermark0_idx(struct vpp_device *p_vpp,uint8 index_num,uint8 index);
int32 vpp_set_watermark1_size(struct vpp_device *p_vpp,uint8 w,uint8 h);
int32 vpp_set_watermark0_mode(struct vpp_device *p_vpp,uint8 mode);
int32 vpp_set_watermark1_mode(struct vpp_device *p_vpp,uint8 mode);
int32 vpp_set_watermark0_enable(struct vpp_device *p_vpp,uint8 enable);
int32 vpp_set_watermark1_enable(struct vpp_device *p_vpp,uint8 enable);
int32 vpp_set_motion_det_enable(struct vpp_device *p_vpp,uint8 enable);
int32 vpp_set_motion_calbuf(struct vpp_device *p_vpp,uint32 calbuf);
int32 vpp_set_motion_range(struct vpp_device *p_vpp,uint16 x_s,uint16 y_s,uint16 x_e,uint16 y_e);
int32 vpp_set_motion_blk_threshold(struct vpp_device *p_vpp,uint8 threshold);
int32 vpp_set_motion_frame_threshold(struct vpp_device *p_vpp,uint32 threshold);
int32 vpp_set_ifp_addr(struct vpp_device *p_vpp,uint32 addr);
int32 vpp_set_ifp_en(struct vpp_device *p_vpp,uint8 enable);
int32 vpp_set_mode(struct vpp_device *p_vpp,uint8 mode);
int32 vpp_set_video_size(struct vpp_device *p_vpp,uint32 w,uint32 h);
int32 vpp_set_ycbcr(struct vpp_device *p_vpp,uint8 ycbcr);
int32 vpp_set_buf0_count(struct vpp_device *p_vpp,uint8 cnt);
int32 vpp_set_buf0_en(struct vpp_device *p_vpp,uint8 enable);
int32 vpp_set_buf1_count(struct vpp_device *p_vpp,uint8 cnt);
int32 vpp_set_buf1_en(struct vpp_device *p_vpp,uint8 enable);
int32 vpp_set_buf1_shrink(struct vpp_device *p_vpp,uint8 half);
int32 vpp_set_input_interface(struct vpp_device *p_vpp,uint8 mode);
int32 vpp_set_buf0_y_addr(struct vpp_device *p_vpp,uint32 addr);
int32 vpp_set_buf0_u_addr(struct vpp_device *p_vpp,uint32 addr);
int32 vpp_set_buf0_v_addr(struct vpp_device *p_vpp,uint32 addr);
int32 vpp_set_buf1_y_addr(struct vpp_device *p_vpp,uint32 addr);
int32 vpp_set_buf1_u_addr(struct vpp_device *p_vpp,uint32 addr);
int32 vpp_set_buf1_v_addr(struct vpp_device *p_vpp,uint32 addr);
int32 vpp_set_itp_y_addr(struct vpp_device *p_vpp,uint32 addr);
int32 vpp_set_itp_u_addr(struct vpp_device *p_vpp,uint32 addr);
int32 vpp_set_itp_v_addr(struct vpp_device *p_vpp,uint32 addr);
int32 vpp_set_itp_linebuf(struct vpp_device *p_vpp,uint32 linebuf);
int32 vpp_set_itp_enable(struct vpp_device *p_vpp,uint8 en);
int32 vpp_set_itp_auto_close(struct vpp_device *p_vpp,uint8 en);
int32 vpp_set_isp_config(struct vpp_device *p_vpp);
int32 vpp_get_status(struct vpp_device *p_vpp);
int32 vpp_set_motion_all_frame(struct vpp_device *p_vpp,uint32 allframe);
int32 vpp_set_motion_frame_interval(struct vpp_device *p_vpp,uint8 interval);
int32 vpp_set_ftusb30_enable(struct vpp_device *p_vpp,uint8 en);
int32 vpp_set_nosram_buf_enable(struct vpp_device *p_vpp,uint8 vppbuf);
int32 vpp_set_sram_buf_mode(struct vpp_device *p_vpp,uint8 vppbuf0,uint8 vppbuf1);
int32 vpp_set_scale_buf_select(struct vpp_device *p_vpp,uint8 scale1,uint8 scale3);
int32 vpp_set_psram_ycnt(struct vpp_device *p_vpp,uint32 w,uint32 h);
int32 vpp_set_psram1_uvcnt(struct vpp_device *p_vpp,uint32 w,uint32 h);
int32 vpp_set_psram1_ycnt(struct vpp_device *p_vpp,uint32 w,uint32 h);
int32 vpp_set_psram_uvcnt(struct vpp_device *p_vpp,uint32 w,uint32 h);
int32 vpp_set_nosram_buf1_enable(struct vpp_device *p_vpp,uint8 en);
int32 vpp_set_gen420_buf_sel(struct vpp_device *p_vpp,uint8 buf);
int32 vpp_set_watermark0_auto_rc_threshold(struct vpp_device *p_vpp,uint32 cth,uint32 hth);
int32 vpp_set_watermark0_auto_rc(struct vpp_device *p_vpp,uint32 en,uint8 y,uint8 u,uint8 v);
int32 vpp_set_watermark0_auto_rc_sram_adr(struct vpp_device *p_vpp,uint32 adr);
int32 vpp_set_watermark_auto_rc_mode(struct vpp_device *p_vpp,uint8 mode);

int32 vpp_get_f0_is_psram(struct vpp_device *p_vpp);
int32 vpp_get_f1_is_psram(struct vpp_device *p_vpp);
int32 vpp_f0_is_enable(struct vpp_device *p_vpp);
int32 vpp_f1_is_enable(struct vpp_device *p_vpp);
int32 vpp_get_f1_mode(struct vpp_device *p_vpp);
int32 vpp_get_f1_shrink(struct vpp_device *p_vpp);
int32 vpp_get_scale1_buf_select(struct vpp_device *p_vpp);
int32 vpp_get_scale3_buf_select(struct vpp_device *p_vpp);

#ifdef __cplusplus
}
#endif
#endif
