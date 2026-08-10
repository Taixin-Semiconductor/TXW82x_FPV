#ifndef HAL_MIPI_CSI_H
#define HAL_MIPI_CSI_H

#ifdef __cplusplus
extern "C" {
#endif

/* User interrupt handle */
typedef int32(*mipi_csi_irq_hdl)(uint32 irq_flags, uint32 irq_data, uint32 param);

/**
  * @brief UART ioctl_cmd type
  */
enum mipi_csi_ioctl_cmd {
    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	MIPI_CSI_LANE_NUM,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	MIPI_CSI_IMG_SIZE,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	MIPI_CSI1_IMG_SIZE,		

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	MIPI_CSI_HSYNC_REC_TIME,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	MIPI_CSI1_HSYNC_REC_TIME,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	MIPI_CSI_HSYNC_REC_EN,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	MIPI_CSI1_HSYNC_REC_EN,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	MIPI_CSI_CROP_EN,		

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	MIPI_CSI1_CROP_EN,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	MIPI_CSI_CROP_COORD_START, 

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	MIPI_CSI1_CROP_COORD_START, 

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	MIPI_CSI_CROP_COORD_END, 

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	MIPI_CSI1_CROP_COORD_END, 

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	MIPI_CSI_FORMAT_INPUT,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	MIPI_CSI1_FORMAT_INPUT,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	MIPI_CSI_DPHY_CFG,
	
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	MIPI_CSI_DPHY_DBG,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	MIPI_CSI_OPEN_VIRTUAL_CHANNEL,	

    MIPI_CSI_FTUSB3_OPEN,

    MIPI_CSI_HS_RX_ZERO_CNT,

    MIPI_CSI_GET_MATCH_VALUE,

    MIPI_CSI_STOP_STATE,
};

struct mipi_csi_device {
    struct dev_obj dev;
};

struct mipi_csi_hal_ops{
    struct devobj_ops ops;
    int32(*init)(struct mipi_csi_device *mipi_csi_dev,uint8_t lane_num);
	int32(*deinit)(struct mipi_csi_device *mipi_csi_dev);
    int32(*open)(struct mipi_csi_device *mipi_csi_dev);
	int32(*baudrate)(struct mipi_csi_device *mipi_csi_dev, uint32 baudrate);
    int32(*close)(struct mipi_csi_device *mipi_csi_dev);
    int32(*ioctl)(struct mipi_csi_device *mipi_csi_dev, enum mipi_csi_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct mipi_csi_device *mipi_csi_dev, uint32 irq_flag, mipi_csi_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct mipi_csi_device *mipi_csi_dev, uint32 irq_flag);	
    int32(*request_irq1)(struct mipi_csi_device *mipi_csi_dev, uint32 irq_flag, mipi_csi_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq1)(struct mipi_csi_device *mipi_csi_dev, uint32 irq_flag);	
};

int32 mipi_csi_open(struct mipi_csi_device *p_mipi_csi);
int32 mipi_csi_close(struct mipi_csi_device *p_mipi_csi);
int32 mipi_csi_init(struct mipi_csi_device *p_mipi_csi,uint8_t lane_num);
int32 mipi_csi_deinit(struct mipi_csi_device *p_mipi_csi);
int32 mipi_csi_set_baudrate(struct mipi_csi_device *p_mipi_csi, uint32 mclk);
int32 mipi_csi_request_irq(struct mipi_csi_device *p_mipi_csi, uint32 irq_flags, mipi_csi_irq_hdl irq_hdl, uint32 irq_data);
int32 mipi_csi_release_irq(struct mipi_csi_device *p_mipi_csi, uint32 irq_flags);
int32 mipi_csi1_request_irq(struct mipi_csi_device *p_mipi_csi, uint32 irq_flags, mipi_csi_irq_hdl irq_hdl, uint32 irq_data);
int32 mipi_csi1_release_irq(struct mipi_csi_device *p_mipi_csi, uint32 irq_flags);
int32 mipi_csi_set_lane_num(struct mipi_csi_device *p_mipi_csi,uint32 lane_num);
int32 mipi_csi_open_virtual_channel(struct mipi_csi_device *p_mipi_csi,uint8 open);
int32 mipi_csi_img_size(struct mipi_csi_device *p_mipi_csi,uint32 w,uint32 h);
int32 mipi_csi_hsync_rec_time(struct mipi_csi_device *p_mipi_csi,uint8 time);
int32 mipi_csi_hsync_rec_enable(struct mipi_csi_device *p_mipi_csi,uint8 enable);
int32 mipi_csi_crop_enable(struct mipi_csi_device *p_mipi_csi,uint8 enable);
int32 mipi_csi_crop_img_start(struct mipi_csi_device *p_mipi_csi,uint32 x,uint32 y);
int32 mipi_csi_crop_img_end(struct mipi_csi_device *p_mipi_csi,uint32 x,uint32 y);
int32 mipi_csi_input_format(struct mipi_csi_device *p_mipi_csi,uint8 format);
int32 mipi_csi_dphy_cfg(struct mipi_csi_device *p_mipi_csi,uint32 c0,uint32 c1,uint32 c2,uint32 c3,uint32 c4,uint32 c5,uint32 c6,uint32 c7);
int32 mipi_csi_dphy_dbg(struct mipi_csi_device *p_mipi_csi,uint32 d0,uint32 d1,uint32 d2,uint32 d3);
int32 mipi_csi_ftusb3_open(struct mipi_csi_device *p_mipi_csi, uint8 enable);
int32 mipi_csi1_img_size(struct mipi_csi_device *p_mipi_csi,uint32 w,uint32 h);
int32 mipi_csi1_hsync_rec_time(struct mipi_csi_device *p_mipi_csi,uint8 time);
int32 mipi_csi1_hsync_rec_enable(struct mipi_csi_device *p_mipi_csi,uint8 enable);
int32 mipi_csi1_crop_enable(struct mipi_csi_device *p_mipi_csi,uint8 enable);
int32 mipi_csi1_crop_img_start(struct mipi_csi_device *p_mipi_csi,uint32 x,uint32 y);
int32 mipi_csi1_crop_img_end(struct mipi_csi_device *p_mipi_csi,uint32 x,uint32 y);
int32 mipi_csi1_input_format(struct mipi_csi_device *p_mipi_csi,uint8 format);
int32 mipi_csi_hs_rx_zero_cnt_set(struct mipi_csi_device *p_mipi_csi, uint32 value);
int32 mipi_csi_get_match_value(struct mipi_csi_device *p_mipi_csi, uint32 *value);
int32 mipi_csi_get_stop_state(struct mipi_csi_device *p_mipi_csi, uint32 *value);
#ifdef __cplusplus
}
#endif

#endif

