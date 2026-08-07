#ifndef _HAL_ISP_H_
#define _HAL_ISP_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "hal/isp_param.h"

#define EVENT_ISP_CALC      (BIT(0))
#define EVENT_ISP_IMG_OPT   (BIT(1))
#define EVENT_ISP_FPS_OPT   (BIT(2))

enum sensor_type {
    SENSOR_TYPE_MASTER = 0,
    SENSOR_TYPE_SLAVE0,
    SENSOR_TYPE_SLAVE1,
};

enum isp_input_dat_src{
    ISP_INPUT_DAT_SRC_DVP = 0,
    ISP_INPUT_DAT_SRC_DVP1,
    ISP_INPUT_DAT_SRC_MIPI0,
    ISP_INPUT_DAT_SRC_MIPI1,
    ISP_INPUT_DAT_SRC_ORG_DMA,
    ISP_INPUT_DAT_SRC_GEN422,
    ISP_INPUT_DAT_SRC_PARA,
    ISP_INPUT_DAT_SRC_RESERVED,
};

enum isp_yuv_range {
    ISP_YUV_RANGE_NARROW = 0,
    ISP_YUV_RANGE_FULL,
};

enum isp_yuv_gamut {
    ISP_YUV_GAMUT_BT601,
    ISP_YUV_GAMUT_BT709,
};

enum isp_input_dat_format{
    ISP_INPUT_DAT_FORMAT_RAW08,
    ISP_INPUT_DAT_FORMAT_RAW10,
    ISP_INPUT_DAT_FORMAT_RAW12,
    ISP_INPUT_DAT_FORMAT_YUV422,
};

enum isp_bayer_format{
    ISP_BAYER_FORMAT_RGGB,
    ISP_BAYER_FORMAT_GRBG,
    ISP_BAYER_FORMAT_GBRG,
    ISP_BAYER_FORMAT_BGGR,
};

enum isp_ycrcb_gamut_type{
    ISP_YCRCB_GAMUT_TYPE_BT601,
    ISP_YCRCB_GAMUT_TYPE_BT709,
};

enum isp_ycrcb_range{
    ISP_YCRCB_RANGE_NARROW,
    ISP_YCRCB_RANGE_FULL,
};

enum isp_detect_frame_interval{
    ISP_DETECT_FRAME_INR_00,
    ISP_DETECT_FRAME_INR_01,
    ISP_DETECT_FRAME_INR_02,
    ISP_DETECT_FRAME_INR_03,
    ISP_DETECT_FRAME_INR_04,
    ISP_DETECT_FRAME_INR_05,
    ISP_DETECT_FRAME_INR_06,
    ISP_DETECT_FRAME_INR_07,
    ISP_DETECT_FRAME_INR_08,
    ISP_DETECT_FRAME_INR_09,
    ISP_DETECT_FRAME_INR_10,
    ISP_DETECT_FRAME_INR_11,
    ISP_DETECT_FRAME_INR_12,
    ISP_DETECT_FRAME_INR_13,
    ISP_DETECT_FRAME_INR_14,
    ISP_DETECT_FRAME_INR_15,
    ISP_DETECT_FRAME_INR_16,
    ISP_DETECT_FRAME_INR_17,
    ISP_DETECT_FRAME_INR_18,
    ISP_DETECT_FRAME_INR_19,
    ISP_DETECT_FRAME_INR_20,
    ISP_DETECT_FRAME_INR_21,
    ISP_DETECT_FRAME_INR_22,
    ISP_DETECT_FRAME_INR_23,
    ISP_DETECT_FRAME_INR_24,
    ISP_DETECT_FRAME_INR_25,
    ISP_DETECT_FRAME_INR_26,
    ISP_DETECT_FRAME_INR_27,
    ISP_DETECT_FRAME_INR_28,
    ISP_DETECT_FRAME_INR_29,
    ISP_DETECT_FRAME_INR_30,
    ISP_DETECT_FRAME_INR_31,
};

enum isp_irq_flag {
    ISP_IRQ_FLAG_INTF_SLOW = 0,
    ISP_IRQ_FLAG_FRM_FAST,
    ISP_IRQ_FLAG_DAT_OF,
    ISP_IRQ_FLAG_FRM_END,
    ISP_IRQ_FLAG_AWB_DONE,
    ISP_IRQ_FLAG_AE_DONE,
    ISP_IRQ_FLAG_MD_DONE,
    ISP_IRQ_FLAG_DMA_DONE,
    ISP_IRQ_FLAG_AF_DONE,
    ISP_IRQ_FLAG_DPC_ERR,
    ISP_IRQ_FLAG_IIC_CFG,
    ISP_IRQ_FLAG_TOTAL_NUM,
};

enum isp_clk_divide{
   ISP_CLK_DIVIDE_1 = 0,
   ISP_CLK_DIVIDE_2,
   ISP_CLK_DIVIDE_3,
   ISP_CLK_DIVIDE_4,
   ISP_CLK_DIVIDE_5,
   ISP_CLK_DIVIDE_6,
   ISP_CLK_DIVIDE_7,
};

enum isp_input_fifo{
    ISP_INPUT_FIFO_FULL,
    ISP_INPUT_FIFO_HALF,
    ISP_INPUT_FIFO_QUARTER,
    ISP_INPUT_FIFO_EIGHTHS,
};

enum isp_filter_mode {
    ISP_FILTER_MODE_NORMAL,
    ISP_FILTER_MODE_BLACK_WHITE,
    ISP_FILTER_MODE_BINARYZATION,
    ISP_FILTER_MODE_RETRO,
};

enum isp_ioctl_cmd {
    ISP_IOCTL_CMD_GET_STA,                                                  
    ISP_IOCTL_CMD_GET_VERSION,
    ISP_IOCTL_CMD_DUMP_PARAM,
    ISP_IOCTL_CMD_PROGRAM_PARAM,
    ISP_IOCTL_CMD_INIT_SENSOR_PARAM,
    ISP_IOCTL_CMD_SENSOR_INIT,
    ISP_IOCTL_CMD_CLK_DIVIDE,
    ISP_IOCTL_CMD_FIFO_TYPE,
    ISP_IOCTL_CMD_CROP_INPUT,
    ISP_IOCTL_CMD_CROP_OUTPUT,
    ISP_IOCTL_CMD_Y_GAMMA_ADDR,
    ISP_IOCTL_CMD_RGB_GAMMA_ADDR,
    ISP_IOCTL_CMD_SENSOR_IIC_DEVID,
    ISP_IOCTL_CMD_SENSOR_IIC_OPTCMD,
    ISP_IOCTL_CMD_SLAVE_INDEX,
    ISP_IOCTL_CMD_BLACK_WHITE_MODE,
    ISP_IOCTL_CMD_IMG_MIRROR,
    ISP_IOCTL_CMD_IMG_REVERSE,
    ISP_IOCTL_CMD_GET_IMG,
    ISP_IOCTL_CMD_FILTER_MODE,
    ISP_IOCTL_CMD_AWB_MANNUL_MODE_MAP,
    ISP_IOCTL_CMD_AWB_MANNUL_MODE_TYPE,
    ISP_IOCTL_CMD_AWB_CONTROL_STEP,      
    ISP_IOCTL_CMD_AWB_CONTROL_THR,      
    ISP_IOCTL_CMD_AWB_AUTO_CTRL,
    ISP_IOCTL_CMD_AWB_FINE_CONSTRAINT_CTRL, 
    ISP_IOCTL_CMD_AWB_COARSE_CONSTRAINT_CTRL,       
    ISP_IOCTL_CMD_AWB_MEAS_MODE,        
    ISP_IOCTL_CMD_AWB_WP_EXPECT,        
    ISP_IOCTL_CMD_AWB_WP_RANGE_CONSTRAINT, 
    ISP_IOCTL_CMD_AWB_WP_RANGE_CONSTRAINTF,
    ISP_IOCTL_CMD_AWB_WP_RANGE_CONSTRAINT_RESTRAIN,   
    ISP_IOCTL_CMD_AWB_RGB_CONSTRAINT,   
    ISP_IOCTL_CMD_AWB_WP_NUM_RESTRAIN,    
	ISP_IOCTL_CMD_AWB_GAIN_TYPE,      
    ISP_IOCTL_CMD_AWB_GAIN_CONSTRAINT, 
    ISP_IOCTL_CMD_AWB_GAIN_COARSE_CONSTRAINT,   
    ISP_IOCTL_CMD_AWB_GAIN_FINE_CONSTRAINT,
    ISP_IOCTL_CMD_AWB_CROP_RANGE,   
    ISP_IOCTL_CMD_AE_BV,
    ISP_IOCTL_CMD_AE_MANUAL_PARAM,
    ISP_IOCTL_CMD_AE_ROW_TIME_US,
    ISP_IOCTL_CMD_AE_ANALOG_GAIN,
    ISP_IOCTL_CMD_AE_EXPOSURE_LINE,
    ISP_IOCTL_CMD_AE_CONFIG_INTERVAL,
    ISP_IOCTL_CMD_AE_DAY_NIGHT_BV,
    ISP_IOCTL_CMD_AE_SCENE_LUT,
    ISP_IOCTL_CMD_AE_LOWLIGHT_PARAM,
    ISP_IOCTL_CMD_AE_FRAME_MAX,
    ISP_IOCTL_CMD_AE_LOCK_PARAM,
    ISP_IOCTL_CMD_AE_REDUCE_FPS,
    ISP_IOCTL_CMD_LOWLIGHT_LSB_GAIN_EN,
    ISP_IOCTL_CMD_LOWLIGHT_LSB_GAIN_4HI_FPS,
    ISP_IOCTL_CMD_LOWLIGHT_LSB_GAIN_4LO_FPS,
    ISP_IOCTL_CMD_HIST_HS_BIN_THR,
    ISP_IOCTL_CMD_AE_LUMA_TARGET,
    ISP_IOCTL_CMD_AE_LUMA_WEIGHT,
    ISP_IOCTL_CMD_AE_ABL_LUMA_MAX,
    ISP_IOCTL_CMD_AE_POS_THR,
    ISP_IOCTL_CMD_AE_HIST_THR,
    ISP_IOCTL_CMD_AE_AOE_EXPO_GAIN,
    ISP_IOCTL_CMD_AE_ABL_LOCK_THR,
    ISP_IOCTL_CMD_AE_BRIGHT_DARK_PIXEL_RATIO,
    ISP_IOCTL_CMD_AE_ABL_DARK_BRIGHT_POS_THR,
    ISP_IOCTL_CMD_AE_AOE_DARK_BRIGHT_POS_THR,
    ISP_IOCTL_CMD_AE_ABL_GAIN_MODE,
    ISP_IOCTL_CMD_AE_ABL_EXPO_LINE_RATIO,
    ISP_IOCTL_CMD_AE_ABL_EXPO_GAIN_THR,
    ISP_IOCTL_CMD_AE_ABL_BV_THR,
    ISP_IOCTL_CMD_AE_AOE_BV_THR,
    ISP_IOCTL_CMD_AE_HIST_PIXEL_THR,
    ISP_IOCTL_CMD_AE_STG_PARAM,
    ISP_IOCTL_CMD_AE_EV_OFFSET_LUT,
    ISP_IOCTL_CMD_AE_EV_OFFSET_TYPE,
    ISP_IOCTL_CMD_AE_CROP_RANGE,
    ISP_IOCTL_CMD_HIST_CROP_RANGE,
    ISP_IOCTL_CMD_CCM_ARRAY,
    ISP_IOCTL_CMD_BLC_PARAM,
    ISP_IOCTL_CMD_MD_WINDOW_PARAM,
    ISP_IOCTL_CMD_MD_DETECT_PARAM,
    ISP_IOCTL_CMD_CSC_PARAM,
    ISP_IOCTL_CMD_DPC_PARAM,
    ISP_IOCTL_CMD_CE_YUV_RANGE,
    ISP_IOCTL_CMD_CE_LUMA,
    ISP_IOCTL_CMD_CE_SATURATION,
    ISP_IOCTL_CMD_CE_CONTRAST,
    ISP_IOCTL_CMD_CE_HUE,
    ISP_IOCTL_CMD_CE_OFFSET_PARAM,
    ISP_IOCTL_CMD_CE_SATURATION_BV_PARAM,
    ISP_IOCTL_CMD_SHARP_PARAM,
    ISP_IOCTL_CMD_RAWNR_MAP,
    ISP_IOCTL_CMD_YUVNR_MAP,
    ISP_IOCTL_CMD_CSUPP_MAP,
    ISP_IOCTL_CMD_Y_GAMMA_TUNNING,
    ISP_IOCTL_CMD_RGB_GAMMA_TUNNING,
    ISP_IOCTL_CMD_LSC_TUNNING,
    ISP_IOCTL_CMD_GET_Y_GAMMA,
    ISP_IOCTL_CMD_GET_RGB_GAMMA,
    ISP_IOCTL_CMD_GET_LSC,
    ISP_IOCTL_CMD_GIC_PARAM,
    ISP_IOCTL_CMD_LUMA_CA_ADDR,
    ISP_IOCTL_CMD_3DNR_ENABLE,
    ISP_IOCTL_CMD_LHS_MAP,
    ISP_IOCTL_CMD_GET_SENSOR_RAW,
    ISP_IOCTL_CMD_DUMP_SRAM_PARAM,
    ISP_IOCTL_CMD_DUMP_REG_PARAM,
    ISP_IOCTL_CMD_CONFIG_SRAM_PARAM,
    ISP_IOCTL_CMD_FUNC_ENABLE,
    ISP_IOCTL_CMD_WDR_EN,
    ISP_IOCTL_CMD_WDR_NOISE_FLOOR,
    ISP_IOCTL_CMD_WDR_TUNNING,
    ISP_IOCTL_CMD_YUV_RANGE,
};

enum isp_module_clk {
    ISP_MODULE_CLK_320M = 1,
    ISP_MODULE_CLK_240M,
    ISP_MODULE_CLK_160M,
    ISP_MODULE_CLK_120M,
    ISP_MODULE_CLK_080M,
    ISP_MODULE_CLK_060M,
};

enum isp_ioctl_ret_type {
    RET_TYPE_SUCC,
    RET_TYPE_INVALID_VALUE,//invalid_value,
    RET_TYPE_
};

enum isp_ev_offset {
    ISP_EV_OFFSET_SUB_6,
    ISP_EV_OFFSET_SUB_5,
    ISP_EV_OFFSET_SUB_4,
    ISP_EV_OFFSET_SUB_3,
    ISP_EV_OFFSET_SUB_2,
    ISP_EV_OFFSET_SUB_1,
    ISP_EV_OFFSET_ADD_0,
    ISP_EV_OFFSET_ADD_1,
    ISP_EV_OFFSET_ADD_2,
    ISP_EV_OFFSET_ADD_3,
    ISP_EV_OFFSET_ADD_4,
    ISP_EV_OFFSET_ADD_5,
    ISP_EV_OFFSET_ADD_6,
};

enum isp_awb_mode {
    ISP_AWB_MODE_AUTO,
    ISP_AWB_MODE_OVERCAST_SKY,
    ISP_AWB_MODE_CLOUDY_SKY,
    ISP_AWB_MODE_SUNNY_SKY,
};

struct isp_awb_mode_param {
    uint16 awb_mode;
    uint16 awb_gain_r, awb_gain_gr, awb_gain_gb, awb_gain_b;
};

struct isp_ae_func_cfg {
    scatter_data data;
    uint32 analog_gain   : 16,
           cmd_len       :  8,
           devid_id      :  8;
    uint32 slave_id      : 16,
           reverse_en    :  4,
           mirror_en     :  4,
           img_operation :  8;
    uint32 exposure_line;
    uint8  chg_fps_flag;
    uint16 frame_length;
    int32  config_flag;
};

struct isp_sensor_opt
{
    scatter_data data;
    uint32 devid_id   : 8,
           slave_id   : 8,
           reverse_en : 4,
           mirror_en  : 4,    
           cmd_len    : 8;     
    uint32 curr_fps;
    uint32 total_length;
};  


/* User interrupt handle */
typedef void (*isp_irq_hdl)(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2);
typedef void (*isp_ae_func)(struct isp_ae_func_cfg *param);
typedef void (*sensor_img_opt)(struct isp_sensor_opt *p_opt);
typedef void (*sensor_fps_opt)(struct isp_sensor_opt *p_opt);

/* ISP api for user */
struct isp_device {
    struct dev_obj dev;
};

struct isp_hal_ops{
    struct devobj_ops ops;
    int32(*open)(struct isp_device *isp, enum isp_module_clk clk_type, enum isp_input_fifo fifo_type);
    int32(*close)(struct isp_device *isp);
    int32(*ioctl)(struct isp_device *isp, uint32 cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct isp_device *isp, uint32 irq_flag, isp_irq_hdl irqhdl, uint32 irq_data);
    int32(*release_irq)(struct isp_device *isp, uint32 irq_flag);
    int32(*calculate)(struct isp_device *isp, struct isp_ae_func_cfg *cfg);
};


int32 isp_open(struct isp_device *isp, enum isp_module_clk clk_type, enum isp_input_fifo fifo_type);
int32 isp_close(struct isp_device *isp);
int32 isp_ioctl(struct isp_device *isp, uint32 cmd, uint32 param1, uint32 param2);
int32 isp_calculate(struct isp_device *isp, struct isp_ae_func_cfg *p_cfg);
int32 isp_request_irq(struct isp_device *isp, uint32 irq_flag, isp_irq_hdl irqhdl, uint32 irq_data);
int32 isp_release_irq(struct isp_device *isp, uint32 irq_flag);
int32 isp_get_status(struct isp_device *isp);
int32 isp_y_gamma_init(struct isp_device *isp, uint32 y_gamma_addr);
int32 isp_rgb_gamma_init(struct isp_device *isp, uint32 rgb_gamma_addr);
int32 isp_clk_divide_init(struct isp_device *isp, uint32 clk_type);
int32 isp_fifo_init(struct isp_device *isp, uint32 fifo_type);
int32 isp_sensor_init(struct isp_device *isp, enum sensor_type type, enum isp_input_dat_src sensor_src, uint32 sensor_config);
int32 isp_crop_input_size(struct isp_device *isp, uint32 start_v, uint32 start_h, uint32 end_v, uint32 end_h, enum sensor_type type);
int32 isp_crop_output_size(struct isp_device *isp, uint32 start_v, uint32 start_h, uint32 end_v, uint32 end_h, enum sensor_type type);
int32 isp_sensor_iic_devid_init(struct isp_device *isp, uint32 devid_id, enum sensor_type type);
int32 isp_sensor_iic_cmd_init(struct isp_device *isp, uint32 cmd_id, enum sensor_type type);
int32 isp_sensor_slave_index(struct isp_device *isp);
int32 isp_black_white_enable(struct isp_device *isp, uint32 enable, enum sensor_type type);
int32 isp_mirror_enable(struct isp_device *isp, uint32 enable, enum sensor_type type);
int32 isp_reverse_enable(struct isp_device *isp, uint32 enable, enum sensor_type type);

int32 isp_awb_mannul_mode_map(struct isp_device *isp, uint32 map_addr);
int32 isp_awb_mannul_mode_type(struct isp_device *isp, enum isp_awb_mode mannul_type, enum sensor_type type);
int32 isp_awb_gain_type(struct isp_device *isp, uint32 gain_type, enum sensor_type type);
int32 isp_awb_control_step_config(struct isp_device *isp, uint16 coarse, uint16 fine, enum sensor_type type);
int32 isp_awb_control_thr_config(struct isp_device *isp, uint16 coarse_thr, uint16 hi_thr, uint16 lo_thr, uint16 rgb_thr, uint16 yuv_thr, enum sensor_type type);
int32 isp_awb_wp_expect_val(struct isp_device *isp, uint32 cb_target, uint32 cr_target, enum sensor_type type);
int32 isp_awb_rgb_constraint(struct isp_device *isp, uint32 r_max, uint32 g_max, uint32 b_max, enum sensor_type type);
int32 isp_awb_cbcr_constraint(struct isp_device *isp, uint32 cb_min, uint32 cr_min, uint32 sum_val, enum sensor_type type);
int32 isp_awb_cbcr_constraintf(struct isp_device *isp, uint32 cb_min, uint32 cr_min, uint32 sum_val, enum sensor_type type);
int32 isp_awb_cbcr_constraint_restrain(struct isp_device *isp, uint32 param, enum sensor_type type);
int32 isp_awb_wp_constraint(struct isp_device *isp, uint32 wp_min, uint32 wp_max, enum sensor_type type);
int32 isp_awb_measure_mode_config(struct isp_device *isp, uint32 mode, enum sensor_type type);
int32 isp_awb_auto_config(struct isp_device *isp, uint32 enable, uint32 *gain_buf, enum sensor_type type);
int32 isp_awb_fine_constraint_enable(struct isp_device *isp, uint32 enable, enum sensor_type type);
int32 isp_awb_coarse_constraint_enable(struct isp_device *isp, uint32 enable, enum sensor_type type);
int32 isp_awb_gain_constraint(struct isp_device *isp, uint32 addr, enum sensor_type type);
int32 isp_awb_gain_coarse_constraint(struct isp_device *isp, uint32 addr, enum sensor_type type);
int32 isp_awb_gain_fine_constraint(struct isp_device *isp, uint32 addr, enum sensor_type type);
int32 isp_awb_crop_range(struct isp_device *isp, uint32 start_v, uint32 start_h, uint32 end_v, uint32 end_h, enum sensor_type type);

int32 isp_get_current_bv(struct isp_device *isp, uint32 *bv, enum sensor_type type);
int32 isp_ae_manul_config(struct isp_device *isp, uint32 target, uint32 analog_gain, uint32 expo_line, enum sensor_type type);
int32 isp_ae_row_time(struct isp_device *isp, uint32 row_us, enum sensor_type type);
int32 isp_ae_analog_gain(struct isp_device *isp, uint32 max_gain, uint32 min_gain, enum sensor_type type);
int32 isp_ae_exposure_line(struct isp_device *isp, uint32 max_line, uint32 min_line, uint32 def_line, enum sensor_type type);
int32 isp_ae_interval_config(struct isp_device *isp, uint32 cnt, enum sensor_type type);
int32 isp_ae_day_night_bv(struct isp_device *isp, uint32 day_bv, uint32 night_bv, enum sensor_type type);
int32 isp_ae_scene_lut(struct isp_device *isp, uint32 addr, enum sensor_type type);
int32 isp_ae_lowlight_param(struct isp_device *isp, uint32 addr, enum sensor_type type);
int32 isp_ae_frame_max(struct isp_device *isp, uint32 frame_max, uint32 vb, enum sensor_type type);
int32 isp_ae_lock_param(struct isp_device *isp, uint32 cnt, uint32 diff, enum sensor_type type);
int32 isp_ae_reduce_fps(struct isp_device *isp, uint32 enable, enum sensor_type type);
int32 isp_ae_lowlight_gain_enable(struct isp_device *isp, uint32 enable, enum sensor_type type);
int32 isp_ae_lowlight_gain_high_gain(struct isp_device *isp, uint32 gain, enum sensor_type type);
int32 isp_ae_lowlight_gain_low_gain(struct isp_device *isp, uint32 gain, enum sensor_type type);
int32 isp_ae_hist_hs_bin_thr(struct isp_device *isp, uint32 hs_bin_thr, enum sensor_type type);

int32 isp_ae_luma_target_config(struct isp_device *isp, uint32 luma_target, enum sensor_type type);
int32 isp_ae_luma_weight_config(struct isp_device *isp, uint32 luma_weight_sum, uint32 *luma_weight, enum sensor_type type);
int32 isp_ae_abl_luma_max(struct isp_device *isp, uint32 max, enum sensor_type type);
int32 isp_ae_pos_thr(struct isp_device *isp, uint32 dark_thr, enum sensor_type type);
int32 isp_ae_hist_thr(struct isp_device *isp, uint32 val1, uint32 val2, enum sensor_type type);
int32 isp_ae_aoe_gain(struct isp_device *isp, uint32 val1, uint32 val2, enum sensor_type type);
int32 isp_ae_abl_lock(struct isp_device *isp, uint32 diff_ratio, uint32 dark_pos_thr, uint32 bright_pos_thr, enum sensor_type type);
int32 isp_ae_brihgt_dark_pixel_ratio(struct isp_device *isp, uint32 bright_low, uint32 bright_high, uint32 dark_low, uint32 dark_high, enum sensor_type type);
int32 isp_ae_abl_dark_bright_pos_ratio(struct isp_device *isp, uint32 dark_low, uint32 dark_high, uint32 bright_thr, enum sensor_type type);
int32 isp_ae_aoe_dark_bright_pos_ratio(struct isp_device *isp, uint32 dark_thr, uint32 bright_thr, enum sensor_type type);
int32 isp_ae_abl_gain_type(struct isp_device *isp, uint32 abl_gain_type, enum sensor_type type);
int32 isp_ae_abl_expo_gain_set(struct isp_device *isp, uint32 low, uint32 high, enum sensor_type type);
int32 isp_ae_abl_expo_line_ratio(struct isp_device *isp, uint32 low, uint32 high, enum sensor_type type);
int32 isp_ae_abl_bv_thr(struct isp_device *isp, uint32 low, uint32 high, enum sensor_type type);
int32 isp_ae_aoe_bv_thr(struct isp_device *isp, uint32 low, uint32 high, enum sensor_type type);
int32 isp_ae_hist_pixel_thr(struct isp_device *isp, uint32 upper, uint32 upper_hs, uint32 lower, enum sensor_type type);
int32 isp_ae_stg_param(struct isp_device *isp, uint32 stg_mode, uint32 ratio_slope, uint32 max_offset, enum sensor_type type);
int32 isp_ae_ev_offset_lut(struct isp_device *isp, uint32 lut_addr);
int32 isp_ae_ev_offset_type(struct isp_device *isp, enum isp_ev_offset offset_type, enum sensor_type type);
int32 isp_ae_crop_range(struct isp_device *isp, uint32 data, enum sensor_type type);
int32 isp_hist_crop_range(struct isp_device *isp, uint32 data, enum sensor_type type);

int32 isp_ccm_config(struct isp_device *isp, uint32 *ccm_array, enum sensor_type type);
int32 isp_blc_config(struct isp_device *isp, uint32 *blc_array, enum sensor_type type);
int32 isp_md_window_config(struct isp_device *isp, uint32 win_start_h, uint32 win_start_v, uint32 win_end_h, uint32 win_end_v, enum sensor_type type);
int32 isp_md_param_config(struct isp_device *isp, uint32 frame_val, uint32 block_val, uint32 frame_cnt, enum sensor_type type);
int32 isp_csc_config(struct isp_device *isp, uint32 *config_addr, enum sensor_type type);
int32 isp_dpc_config(struct isp_device *isp, uint32 *config_addr, enum sensor_type type);
int32 isp_ce_yuv_range(struct isp_device *isp, uint32 range, enum sensor_type type);
int32 isp_ce_luma_config(struct isp_device *isp, uint32 luma_val, enum sensor_type type);
int32 isp_ce_saturation_config(struct isp_device *isp, uint32 luma_val, enum sensor_type type);
int32 isp_ce_contrast_config(struct isp_device *isp, uint32 contrast, enum sensor_type type);
int32 isp_ce_hue_config(struct isp_device *isp, uint32 hue, enum sensor_type type);
int32 isp_ce_offset_config(struct isp_device *isp, uint32 addr, enum sensor_type type);
int32 isp_ce_bv_stauration(struct isp_device *isp, uint32 addr, enum sensor_type type);
int32 isp_sharp_param(struct isp_device *isp, uint32 addr, enum sensor_type type);
int32 isp_rawnr_map(struct isp_device *isp, uint32 map_num, uint32 map_start, uint32 map_data_addr, enum sensor_type type);
int32 isp_yuvnr_map(struct isp_device *isp, uint32 map_num, uint32 map_start, uint32 map_data_addr, enum sensor_type type);
int32 isp_csupp_map(struct isp_device *isp, uint32 map_num, uint32 map_start, uint32 map_data_addr, enum sensor_type type);
int32 isp_y_gamma_tunning(struct isp_device *isp, uint32 addr, enum sensor_type type);
int32 isp_rgb_gamma_tunning(struct isp_device *isp, uint32 addr, enum sensor_type type);
int32 isp_lsc_tunning(struct isp_device *isp, uint32 addr, enum sensor_type type);
int32 isp_3dnr_enable(struct isp_device *isp, uint32 enable, enum sensor_type type);
int32 isp_gic_init(struct isp_device *isp, uint32 param_addr, enum sensor_type type);
int32 isp_lhs_map(struct isp_device *isp, uint32 map_num, uint32 map_start, uint32 map_data_addr, enum sensor_type type);
int32 isp_sensor_param_config(struct isp_device *isp, uint32 addr);
int32 isp_sensor_sram_param_config(struct isp_device *isp, uint32 channel, uint32 addr);
int32 isp_sensor_func_enable(struct isp_device *isp, uint32 channel, uint32 data);
int32 isp_wdr_en_config(struct isp_device *isp, uint32 wdr_en, enum sensor_type type);
int32 isp_wdr_noise_floor_config(struct isp_device *isp, uint32 cfg_noise_floor, uint32 cfg_noise_floor_out, enum sensor_type type);
int32 isp_wdr_tunning(struct isp_device *isp, uint32 addr, enum sensor_type type);
int32 isp_yuv_range(struct isp_device *isp, enum isp_yuv_range range_type);
#ifdef __cplusplus
}
#endif

#endif
