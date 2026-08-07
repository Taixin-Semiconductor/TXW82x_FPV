#ifndef HAL_DSI_H
#define HAL_DSI_H
#ifdef __cplusplus
extern "C" {
#endif

/* User interrupt handle */
typedef int32(*dsi_irq_hdl)(uint32 irq_flags, uint32 irq_data, uint32 param);

// Data Types for Processor-Sourced Packets 
#define DSI_SP_VSS      0x01                                                                                                                                                                                    
#define DSI_SP_VSE      0x11
#define DSI_SP_HSS      0x21
#define DSI_SP_HSE      0x31
#define DSI_SP_EOTP     0x08
#define DSI_SP_CMOFF    0x02
#define DSI_SP_CMON     0x12
#define DSI_SP_SDON     0x22
#define DSI_SP_SDOFF    0x32
#define DSI_SP_GENW0P   0x03
#define DSI_SP_GENW1P   0x13
#define DSI_SP_GENW2P   0x23
#define DSI_SP_GENR0P   0x04
#define DSI_SP_GENR1P   0x14
#define DSI_SP_GENR2P   0x24
#define DSI_SP_DCSW0P   0x05
#define DSI_SP_DCSW1P   0x15
#define DSI_SP_DCSR0P   0x06
#define DSI_SP_MAXRPACK 0x37
#define DSI_LP_NULL     0x09
#define DSI_LP_BLANK    0x19
#define DSI_LP_GENW     0x29
#define DSI_LP_DCSW     0x39
#define DSI_LP_PPS16B   0x0E
#define DSI_LP_PPS18B   0x1E
#define DSI_LP_LPPS18B  0x2E
#define DSI_LP_PPS24B   0x3E

//display command set
#define DSI_ENTER_IDLE_MODE         0x39                                                                                                                                                                          
#define DSI_ENTER_INVERT_MODE       0x21
#define DSI_ENTER_NORMAL_MODE       0x13
#define DSI_ENTER_PARTIAL_MODE      0x12
#define DSI_ENTER_SLEEP_MODE        0x10
#define DSI_EXIT_IDLE_MODE          0x38
#define DSI_EXIT_INVERT_MODE        0x20
#define DSI_EXIT_SLEEP_MODE         0x11
#define DSI_NOP                     0x00
#define DSI_SET_DISPLAY_OFF         0x28
#define DSI_SET_DISPLAY_ON          0x29
#define DSI_SET_ALLP_ON             0x23
#define DSI_SET_ALLP_OFF            0x22
#define DSI_SET_TEAR_OFF            0x34
#define DSI_SOFT_RESET              0x01
#define DSI_SET_ADDRESS_MODE        0x36
#define DSI_SET_GAMMA_CURVE         0x26
#define DSI_SET_PIXEL_FORMAT        0x3A
#define DSI_SET_TEAR_ON             0x35
#define DSI_SET_SCROLL_START        0x37
#define DSI_SET_TEAR_SCAN_LINE      0x44
#define DSI_SET_COLUMN_ADDRESS      0x2A
#define DSI_SET_PAGE_ADDRESS        0x2B
#define DSI_SET_PARTIAL_COLUMNS     0x31
#define DSI_SET_PARTIAL_ROWS        0x30
#define DSI_SET_SCROLL_AREA         0x33
#define DSI_WRITE_LUT               0x2D
#define DSI_WRITE_MEMORY_CONTINUE   0x3C
#define DSI_WRITE_MEMORY_START      0x2C
#define DSI_GET_ADDRESS_MODE        0x0B
#define DSI_GET_BLUE_CHANNEL        0x08
#define DSI_GET_COMPRESSION_MODE    0x03
#define DSI_GET_DIAGNOSTIC_RESULT   0x0F
#define DSI_GET_DISPLAY_MODE        0x0D
#define DSI_GET_GREEN_CHANNEL       0x07
#define DSI_GET_PIXEL_FORMAT        0x0C
#define DSI_GET_POWER_MODE          0x0A
#define DSI_GET_RED_CHANNEL         0x06
#define DSI_GET_SCAN_LINE           0x45
#define DSI_GET_SIGNAL_MODE         0x0E
#define DSI_READ_DDB_CONTINUE       0xA8
#define DSI_READ_DDB_START          0xA1
#define DSI_READ_MEMORY_CONTINUE    0x3E
#define DSI_READ_MEMORY_START       0x2E

/**
  * @brief UART ioctl_cmd type
  */
enum dsi_ioctl_cmd {
    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DSI_IOCTL_LANE_NUM,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DSI_IOCTL_MAX_CLK_TIM,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DSI_IOCTL_REMAIN_STOP_STATE,		

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DSI_IOCTL_CFG_ENABLE,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DSI_IOCTL_ULPS_CTRL,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DSI_IOCTL_ULPS_MIN_TIME,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DSI_IOCTL_ULPS_ENTRY_DELAY_TIME,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    DSI_IOCTL_ULPS_WAKEUP_TIME,
		
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_ULPS_MODE,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_CMD_VIDEO_SELECT_MODE,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_GEN_VCID_CFG,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_CMD_MODE_CFG,
		
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_DPI_VCID,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_COLOR_CFG,	
		
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_CFG_POL,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_VID_MODE,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_VID_PKT_SIZE,	

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_VID_CHUNK_NUM,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_VID_NULL_SIZE,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_VID_CFG,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_LPCMD_TMR,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_EDPI_CFG,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_DPHY_RST,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_PIXEL_GENERIC_SRAM_FIFO,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_FIFO_DEPTH,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_CMD_PKT_FIFO_EMPTY,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_WAIT_PKT_FIFO_FULL,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_GEN_PLD_WRITE_FULL,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_GEN_PLD_READ_EMPTY,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_CMD_CFG,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_SET_GEN_PLD_DATA,
		
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_GET_GEN_PLD_DATA,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_GET_STA0,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_GET_STA1,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_SET_IO_REMAP,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DSI_IOCTL_RESET_MODULE,

};

struct dsi_device {
    struct dev_obj dev;
};

struct dsi_hal_ops{
    struct devobj_ops ops;
    int32(*init)(struct dsi_device *dsi_dev,uint8 clk_div,uint8_t tx_esc_clk_div,uint8_t auto_clklane,uint8_t lane_num);
	int32(*deinit)(struct dsi_device *dsi_dev);
    int32(*open)(struct dsi_device *dsi_dev);
    int32(*close)(struct dsi_device *dsi_dev);
    int32(*ioctl)(struct dsi_device *dsi_dev, enum dsi_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct dsi_device *dsi_dev, uint32 irq_flag, dsi_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct dsi_device *dsi_dev, uint32 irq_flag);
};

int32 dsi_init(struct dsi_device *p_dsi,uint8 clk_div,uint8_t tx_esc_clk_div,uint8_t auto_clklane,uint8_t lane_num);
int32 dsi_deinit(struct dsi_device *p_dsi);
int32 dsi_open(struct dsi_device *p_dsi);
int32 dsi_close(struct dsi_device *p_dsi);
int32 mipi_dsi_set_lane_num(struct dsi_device *p_dsi,uint8 lane_num);
int32 mipi_dsi_max_clk_time(struct dsi_device *p_dsi,uint32 max_clk);
int32 mipi_dsi_remain_stop_state(struct dsi_device *p_dsi,uint8 state);
int32 mipi_dsi_cfg_enable(struct dsi_device *p_dsi,uint8_t htxeotp_en,uint8_t rxeotp_en,uint8_t bta_en,uint8_t ecc_en,uint8_t crc_en,uint8_t ltxeotp_en);
int32 mipi_dsi_ulps_ctrl(struct dsi_device *p_dsi,uint8_t indatulps,uint8_t outdatulps,uint8_t inclkulps,uint8_t outclkulps);
int32 mipi_dsi_ulps_min_time(struct dsi_device *p_dsi,uint8 time);
int32 mipi_dsi_ulps_entry_delay_time(struct dsi_device *p_dsi,uint32 time);
int32 mipi_dsi_ulps_wakeup_time(struct dsi_device *p_dsi,uint16 twakeup_clk,uint16 twakeup_cnt);
int32 mipi_dsi_ulps_mode(struct dsi_device *p_dsi,uint8_t pll_off,uint8_t auto_ulps,uint8_t pre_pll_off_req);
int32 mipi_dsi_select_mode(struct dsi_device *p_dsi,uint8 cmd);
int32 mipi_dsi_gen_vcid_cfg(struct dsi_device *p_dsi,uint8_t vcid,uint8_t vcid_tear_auto,uint8_t vcid_tx_auto);
int32 mipi_dsi_cmd_mode_cfg(struct dsi_device *p_dsi,uint8_t te_ack,uint8_t ack,uint8_t sw0p_tx,uint8_t sw1p_tx,uint8_t sw2p_tx,uint8_t sr0p_tx,uint8_t sr1p_tx,uint8_t sr2p_tx, uint8_t gen_lw_tx,uint8_t dcs_sw0p_tx,uint8_t dcs_sw1p_tx,uint8_t dcs_sr0p_tx,uint8_t dcs_lw_tx,uint8_t max_rd_pkt_size);
int32 mipi_dsi_dpi_vcid(struct dsi_device *p_dsi,uint8 vcid);							
int32 mipi_dsi_color_cfg(struct dsi_device *p_dsi,uint8_t color_mode,uint8_t loosely18_en);							
int32 mipi_dsi_cfg_pol(struct dsi_device *p_dsi,uint8_t dataen_active,uint8_t vsync_active,uint8_t hsync_active,uint8_t shutd_active,uint8_t colorm_active);
int32 mipi_dsi_vid_mode(struct dsi_device *p_dsi,uint8_t vsa_en,uint8_t vbp_en,uint8_t vfp_en,uint8_t vact_en,uint8_t hbp_en,uint8_t hfp_en,uint8_t frame_bta_ack_en,uint8_t cmd_en,uint8_t vid_mode_type);
int32 mipi_dsi_vid_pkt_size(struct dsi_device *p_dsi,uint32 size);	
int32 mipi_dsi_vid_chunk_num(struct dsi_device *p_dsi,uint32 chunk_num);
int32 mipi_dsi_vid_null_size(struct dsi_device *p_dsi,uint32 null_num);
int32 mipi_dsi_vid_cfg(struct dsi_device *p_dsi,uint32 dpi2laneclkratio,uint32_t vid_vsa,uint32_t vid_vbp,uint32_t vid_vactive,uint32_t w,uint32_t vid_vfp,uint8_t hsa,uint8_t hbp,uint8_t hfp);
int32 mipi_dsi_lpcmd_tmr(struct dsi_device *p_dsi,uint16_t invact_lptime,uint16_t outvact_lptime);
int32 mipi_dsi_edpi_cfg(struct dsi_device *p_dsi,uint16_t edpi_maxpkt,uint8_t hw_te_on,uint8_t hw_te_gen,uint8_t hw_scan_line,uint16_t line_parameter);
int32 mipi_dsi_phy_rst(struct dsi_device *p_dsi);
int32 mipi_dsi_set_pixel_sram_fifo(struct dsi_device *p_dsi,uint32 gen_addr);
int32 mipi_dsi_set_fifo_depth(struct dsi_device *p_dsi,uint32 depth);
int32 mipi_dsi_cmd_pkt_fifo_empty(struct dsi_device *p_dsi);
int32 mipi_dsi_wait_pkt_fifo_full(struct dsi_device *p_dsi);
int32 mipi_dsi_gen_pld_write_full(struct dsi_device *p_dsi);
int32 mipi_dsi_gen_pld_read_empty(struct dsi_device *p_dsi);
int32 mipi_dsi_cmd_cfg(struct dsi_device *p_dsi,uint8_t cmdid,uint8_t cmd,uint8_t vcid,uint8_t dat);
int32 mipi_dsi_set_gen_pld_data(struct dsi_device *p_dsi,uint8_t param0,uint8_t param1,uint8_t param2,uint8_t param3);
int32 mipi_dsi_get_gen_pld_data(struct dsi_device *p_dsi);
int32 mipi_dsi_set_lane_remap(struct dsi_device *p_dsi,uint8_t clklane,uint8_t lane0,uint8_t lane1,uint8_t lane2,uint8_t lane3,uint8_t clk_pol,uint8_t lane0_pol,uint8_t lane1_pol,uint8_t lane2_pol,uint8_t lane3_pol);
int32 mipi_dsi_get_sta0(struct dsi_device *p_dsi);
int32 mipi_dsi_get_sta1(struct dsi_device *p_dsi);
int32 mipi_dsi_reset_module(struct dsi_device *p_dsi);


#ifdef __cplusplus
}
#endif
#endif
