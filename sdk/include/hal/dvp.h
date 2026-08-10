#ifndef HAL_DVP_H
#define HAL_DVP_H
#ifdef __cplusplus
extern "C" {
#endif

/* User interrupt handle */
typedef int32(*dvp_irq_hdl)(uint32 irq_flags, uint32 irq_data, uint32 param);

/**
  * @brief UART ioctl_cmd type
  */
enum dvp_ioctl_cmd {
    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DVP_IOCTL_CMD_SET_FORMAT,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DVP_IOCTL_CMD_RGB_2_YUV,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DVP_IOCTL_CMD_SET_SIZE,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DVP_IOCTL_CMD_SET_ADR_1,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DVP_IOCTL_CMD_SET_ADR_2,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DVP_IOCTL_CMD_SET_SCEN,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DVP_IOCTL_CMD_SET_HSYNC_VAILD,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DVP_IOCTL_CMD_SET_VSYNC_VAILD,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DVP_IOCTL_CMD_SET_ONE_SAMPLE,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DVP_IOCTL_CMD_SET_DEBOUNCE,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DVP_IOCTL_CMD_SET_YCBCR_MODE,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DVP_IOCTL_CMD_DIS_UV_MODE,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DVP_IOCTL_CMD_SET_FRAME_RATE,


    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DVP_IOCTL_CMD_SET_THRESHOLD,

    DVP_IOCTL_CMD_EX_D5_D6,

    DVP_IOCTL_CMD_SET_JPEG_LEN,


    /*! Compatible version: V2;
     *@ Describe:
     *
     */	
    DVP_IOCTL_CMD_VSYNC_RST_TM,  
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DVP_IOCTL_CMD_10BITS_MAP, 

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DVP_IOCTL_CMD_8BITS_MAP, 

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DVP_IOCTL_CMD_PCLK_EDGE, 

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DVP_IOCTL_CMD_CROP_ENABLE, 

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DVP_IOCTL_CMD_CROP_COORD_START, 

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DVP_IOCTL_CMD_CROP_COORD_END, 
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DVP_IOCTL_CMD_GET_STA,

};

struct dvp_device {
    struct dev_obj dev;
};

struct dvp_hal_ops{
    struct devobj_ops ops;
    int32(*init)(struct dvp_device *dvp_dev);
	int32(*deinit)(struct dvp_device *dvp_dev);
    int32(*baudrate)(struct dvp_device *dvp_dev, uint32 baudrate);
    int32(*open)(struct dvp_device *dvp_dev);
    int32(*close)(struct dvp_device *dvp_dev);
    int32(*ioctl)(struct dvp_device *dvp_dev, enum dvp_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct dvp_device *dvp_dev, uint32 irq_flag, dvp_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct dvp_device *dvp_dev, uint32 irq_flag);
};

int32 dvp_open(struct dvp_device *p_dvp);
int32 dvp_close(struct dvp_device *p_dvp);
int32 dvp_init(struct dvp_device *p_dvp);
int32 dvp_deinit(struct dvp_device *p_dvp);
int32 dvp_set_baudrate(struct dvp_device *p_dvp, uint32 mclk);
int32 dvp_set_size(struct dvp_device *p_dvp, uint32 x_s, uint32 y_s, uint32 x_e, uint32 y_e);
int32 dvp_set_addr1(struct dvp_device *p_dvp, uint32 yuv_addr);
int32 dvp_set_addr2(struct dvp_device *p_dvp, uint32 yuv_addr);
int32 dvp_set_rgb2yuv(struct dvp_device *p_dvp, uint8 en);
int32 dvp_set_format(struct dvp_device *p_dvp, uint8 format);
int32 dvp_set_half_size(struct dvp_device *p_dvp, uint8 en);
int32 dvp_set_vsync_polarity(struct dvp_device *p_dvp, uint8 high_valid);
int32 dvp_set_hsync_polarity(struct dvp_device *p_dvp, uint8 high_valid);
int32 dvp_set_once_sampling(struct dvp_device *p_dvp, uint8 en);
int32 dvp_debounce_enable(struct dvp_device *p_dvp, uint8 en, uint8 pixel);
int32 dvp_set_ycbcr(struct dvp_device *p_dvp, uint8 mode);
int32 dvp_unload_uv(struct dvp_device *p_dvp, uint8 en);
int32 dvp_frame_load_precent(struct dvp_device *p_dvp, uint8 precent);
int32 dvp_low_high_threshold(struct dvp_device *p_dvp, bool low_high);
int32 dvp_jpeg_mode_set_len(struct dvp_device *p_dvp, uint32 len);
int32 dvp_request_irq(struct dvp_device *p_dvp, uint32 irq_flag, dvp_irq_hdl irq_hdl, uint32 irq_data);
int32 dvp_release_irq(struct dvp_device *p_dvp, uint32 irq_flag);
int32 dvp_set_exchange_d5_d6(struct dvp_device *p_dvp, uint8 en);
int32 dvp_10BITS_map(struct dvp_device *p_dvp, uint8 mode);
int32 dvp_8BITS_map(struct dvp_device *p_dvp, uint8 mode);
int32 dvp_set_vsync_rst_offline(struct dvp_device *p_dvp,uint8 enable,uint32 timeout);



#ifdef __cplusplus
}
#endif
#endif
