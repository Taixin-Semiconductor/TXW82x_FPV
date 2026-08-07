#ifndef HAL_H264_H
#define HAL_H264_H
#ifdef __cplusplus
extern "C" {
#endif

/* User interrupt handle */
typedef int32(*h264_irq_hdl)(uint32 irq_flags, uint32 irq_data, uint32 param);

/**
  * @brief UART ioctl_cmd type
  */
enum h264_ioctl_cmd {
	/*! Compatible version: V2;
     *@ Describe:
     *
     */
	H264_IOCTL_SRC_FROM,

	/*! Compatible version: V2;
     *@ Describe:
     *
     */
	H264_IOCTL_HW_OPEN,	

	/*! Compatible version: V2;
     *@ Describe:
     *
     */
	H264_IOCTL_IS_RUNNING,		

	/*! Compatible version: V2;
     *@ Describe:
     *
     */
	H264_IOCTL_SW_BUF_READY,	

	/*! Compatible version: V2;
     *@ Describe:
     *
     */
	H264_IOCTL_SW_FRAME_START,	

	/*! Compatible version: V2;
     *@ Describe:
     *
     */
	H264_IOCTL_SW_READY,		

	/*! Compatible version: V2;
     *@ Describe:
     *
     */
	H264_IOCTL_VPP_VSYNC_DEALY_EN,		

	/*! Compatible version: V2;
     *@ Describe:
     *
     */
	H264_IOCTL_WRAP_IMG_SIZE,		

	/*! Compatible version: V2;
     *@ Describe:
     *
     */
	H264_IOCTL_PHY_IMG_SIZE,

	/*! Compatible version: V2;
     *@ Describe:
     *
     */
	H264_IOCTL_FRAME_BASE_CFG,	

	/*! Compatible version: V2;
     *@ Describe:
     *
     */
	H264_IOCTL_FRAME_QP,

	/*! Compatible version: V2;
     *@ Describe:
     *
     */
	H264_IOCTL_TIMEOUT_LIMIT,	

	/*! Compatible version: V2;
     *@ Describe:
     *
     */
	H264_IOCTL_LEN_LIMIT,	

	/*! Compatible version: V2;
     *@ Describe:
     *
     */
	H264_IOCTL_REF_LU_ADDR,	

	/*! Compatible version: V2;
     *@ Describe:
     *
     */
	H264_IOCTL_REF_CHRO_ADDR,

	/*! Compatible version: V2;
     *@ Describe:
     *
     */
	H264_IOCTL_SET_FRM_NUM,
	
	/*! Compatible version: V2;
     *@ Describe:
     *
     */
	H264_IOCTL_SET_ENC_CNT,	

	/*! Compatible version: V2;
     *@ Describe:
     *
     */
	H264_IOCTL_SET_ENC_FRAME_ID,	

	/*! Compatible version: V2;
     *@ Describe:
     *
     */
	H264_IOCTL_SET_DMA_BS_ADDR,	
	
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_FRAME_MB_NUM,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_REF_WB_ADDR,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_REF_RB_ADDR,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_SET_RATE_CTL,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_GET_RATE_CTL,
	
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_RATE_CTL_BYTE,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_RATE_CTL_RDCOST,
	
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_SET_FLT_3DNR,


	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_GET_FLT_3DNR,
		
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_Y_CROP,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_MB_PIPE,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_FLT_3DNR_LUT,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_FLT_MIN_LUT,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_FLT_MAX_LUT,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_FLT_3DNR_SF,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_FLT_3DNR_CON,	
	
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_FLT_3DNR_CON1,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_GET_BANDWIDTH_WR,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_GET_BANDWIDTH_RD,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_GET_DBG_CYC_DONE_NUM,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_GET_FRM_LEN,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_GET_DEC_DEL,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_SET_STA,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_GET_STA,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_GET_ENC_FRM_RDCOST,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_DEC_CLR_ENC_FUNCS,
		
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_SET_DEC_HEADB_NUM,

		
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_SET_OE_SELECT,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_GET_MBL_CALC,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_GET_MBL_CALC_MAX,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_IS_ERR,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	H264_IOCTL_SET_ERR,

};

enum h264_module_clk {
    H264_MODULE_CLK_240M,
    H264_MODULE_CLK_480M,
};

struct h264_device {
    struct dev_obj dev;
};

struct h264_hal_ops {
	struct devobj_ops ops;
    int32(*init)(struct h264_device *h264_dev,enum h264_module_clk clk_type);
    int32(*deinit)(struct h264_device *h264_dev);	
    int32(*open)(struct h264_device *h264_dev);
    int32(*close)(struct h264_device *h264_dev);
	int32(*decode)(struct h264_device *h264_dev);
    int32(*ioctl)(struct h264_device *h264_dev, enum h264_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct h264_device *h264_dev, uint32 irq_flag, h264_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct h264_device *h264_dev, uint32 irq_flag);
};

int32 h264_init(struct h264_device *p_h264,enum h264_module_clk clk_type);
int32 h264_deinit(struct h264_device *p_h264);
int32 h264_open(struct h264_device *p_h264);
int32 h264_close(struct h264_device *p_h264);
int32 h264_decode_start(struct h264_device *p_h264);
int32 h264_request_irq(struct h264_device *p_h264, uint32 irq_flags, h264_irq_hdl irq_hdl, uint32 irq_data);
int32 h264_release_irq(struct h264_device *p_h264, uint32 irq_flags);
int32 h264_set_src_from(struct h264_device *p_h264, uint8_t src_from);
int32 h264_set_hw_open(struct h264_device *p_h264, uint8_t enable);
int32 h264_set_sw_buf_ready(struct h264_device *p_h264);
int32 h264_set_sw_frame_ready(struct h264_device *p_h264);
int32 h264_set_sw_ready(struct h264_device *p_h264);
int32 h264_set_vpp_vsync_delay(struct h264_device *p_h264, uint8_t enable);
int32 h264_set_wrap_img_size(struct h264_device *p_h264, uint16_t w,uint16_t h);
int32 h264_set_phy_img_size(struct h264_device *p_h264, uint16_t w,uint16_t h);
int32 h264_set_frame_base_cfg(struct h264_device *p_h264, uint8_t frm_type,uint16_t enc_mode,uint32_t enc_bs_buf_size,uint32_t timeout_en);
int32 h264_set_frame_qp(struct h264_device *p_h264, uint32_t frame_qp);
int32 h264_set_timeout_limit(struct h264_device *p_h264, uint32_t time);
int32 h264_set_len_limit(struct h264_device *p_h264, uint32_t len);
int32 h264_set_ref_lu_addr(struct h264_device *p_h264, uint32_t saddr, uint32_t eaddr);
int32 h264_set_ref_chro_addr(struct h264_device *p_h264, uint32_t saddr, uint32_t eaddr);
int32 h264_set_frame_num(struct h264_device *p_h264, uint32_t num);
int32 h264_set_enc_cnt(struct h264_device *p_h264, uint32_t cnt);
int32 h264_set_enc_frame_id(struct h264_device *p_h264, uint32_t id);
int32 h264_set_buf_addr(struct h264_device *p_h264, uint32_t addr);
int32 h264_set_frame_mb_num(struct h264_device *p_h264, uint32_t num);
int32 h264_set_ref_wb_addr(struct h264_device *p_h264, uint32_t lpf_lu_base, uint32_t lpf_ch_base);
int32 h264_set_ref_rb_addr(struct h264_device *p_h264, uint32_t last_lpf_lu_base, uint32_t last_lpf_ch_base);
int32 h264_set_frame_rate_ctl(struct h264_device *p_h264, uint32_t rc_grp,uint32_t rc_effort,uint32_t rc_en);
int32 h264_get_rate_ctl(struct h264_device *p_h264);
int32 h264_set_rate_ctl_byte(struct h264_device *p_h264,uint32_t byte);
int32 h264_set_rate_ctl_rdcost(struct h264_device *p_h264,uint32_t rdcost);
int32 h264_set_flt_3dnr(struct h264_device *p_h264,uint32_t dnr);
int32 h264_get_flt_3dnr(struct h264_device *p_h264);
int32 h264_set_y_crop(struct h264_device *p_h264,uint32_t enable,uint32_t linenum);
int32 h264_set_mb_pipe(struct h264_device *p_h264,uint32_t pipe);
int32 h264_set_flt_3dnr_lut(struct h264_device *p_h264,uint32_t num,uint32_t param);
int32 h264_set_flt_min_lut(struct h264_device *p_h264,uint32_t num,uint32_t param);
int32 h264_set_flt_max_lut(struct h264_device *p_h264,uint32_t num,uint32_t param);
int32 h264_set_flt_3dnr_sf(struct h264_device *p_h264,uint32_t param);
int32 h264_set_flt_3dnr_con(struct h264_device *p_h264,uint32_t param);
int32 h264_set_flt_3dnr_con1(struct h264_device *p_h264,uint32_t param);
int32 h264_get_bandwidth_wr(struct h264_device *p_h264);
int32 h264_get_bandwidth_rd(struct h264_device *p_h264);
int32 h264_get_cyc_done_num(struct h264_device *p_h264);
int32 h264_get_frame_len(struct h264_device *p_h264);
int32 h264_get_dec_del(struct h264_device *p_h264);
int32 h264_set_sta(struct h264_device *p_h264,uint32_t sta);
int32 h264_get_sta(struct h264_device *p_h264);
int32 h264_get_enc_frame_rdcost(struct h264_device *p_h264);
int32 h264_set_enc_frame_rdcost(struct h264_device *p_h264);
int32 h264_set_dec_headb_num(struct h264_device *p_h264,uint32_t dec_num);
int32 h264_set_oe_select(struct h264_device *p_h264,uint8_t enable,uint8_t oe);
int32 h264_is_running(struct h264_device *p_h264);

int32 h264_get_mbl_calc(struct h264_device *p_h264);
int32 h264_get_mbl_calc_max(struct h264_device *p_h264);
int32 h264_is_err(struct h264_device *p_h264);
int32 h264_set_err(struct h264_device *p_h264,uint32_t err);


#ifdef __cplusplus
}
#endif
#endif
