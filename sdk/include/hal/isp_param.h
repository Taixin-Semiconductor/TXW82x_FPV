#ifndef _ISP_PARAM_H_
#define _ISP_PARAM_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "osal/string.h"
#include "sys_config.h"

typedef void *(*ispcfg_malloc)(int size);
typedef void (*ispcfg_free)(void *ptr);

typedef uint32 _Sensor_YGAMMA;

#define ISPCFG_MAGIC  0xb103

#define BV2RAWNR_ARRAY_NUM         11
#define BV2YUVNR_ARRAY_NUM         6
#define BV2CSUPP_ARRAY_NUM         3
#define LOCAL_HUE_SAT_COLOR        9

enum awb_meas_mode{
    AWB_MEAS_MODE_YUV     = 0,
    AWB_MEAS_MODE_RGB     = 1,
    AWB_MEAS_MODE_YUV_NEW = 2,
};

enum awb_config_mode {
    AWB_MODE_MANUAL = 0,
    AWB_MODE_AUTO   = 1,
};

enum awb_gain_type {
    AWB_GAIN_TYPE_AWB  = 0,
    AWB_GAIN_TYPE_RGB  = 1,
    AWB_GAIN_TYPE_BOTH = 2,
};

enum awb_formula_type{
    AWB_FORMULA_SRC,
    AWB_FORMULA_DIFF,
};

enum isp_info_src_type {
    ISP_INFO_SRC_TYPE_CODE_PARAM = 0,
    ISP_INFO_SRC_TYPE_CODE_DOC,
    ISP_INFO_SRC_TYPE_TUNNING,
};

struct isp_func_cfg {
    uint32 test_pattern_en     : 1,     // 0
           dma_flush_en        : 1,
           ifa_table_en        : 1,
           blc_en              : 1,
           lsc_en              : 1,
           af_en               : 1,     // 5
           dehaze_en           : 1,
           dpc_en              : 1,
           bnr_en              : 1,
           gic_en              : 1,
           awb_en              : 1,     // 10
           ccm_en              : 1,
           y_gamma_en          : 1,
           rgb_gamma_en        : 1,
           sharpen_en          : 1,
           ynr_en              : 1,     // 15
           cnr_en              : 1,
           ae_en               : 1,
           hist_en             : 1,
           ce_en               : 1,
           md_en               : 1,     // 20
           csupp_en            : 1,
           luma_ca_en          : 1,
           adj_hue_en          : 1,
           lsc_tab_en          : 1,     // 25
           input_format        : 1,
           reserved            : 2;    
};

#define AWB_CLIP_MAX_SECTION 8
typedef struct hgisp_awb_constraint {
    // awbclip
    uint32 section_num;
	uint32 color_temp[AWB_CLIP_MAX_SECTION];
	float  sec_line_slope[AWB_CLIP_MAX_SECTION];
	float  sec_line_offset[AWB_CLIP_MAX_SECTION];
	float  sec_line_sqrtk2add1[AWB_CLIP_MAX_SECTION];
	float  center_line_slope[AWB_CLIP_MAX_SECTION - 1];
	float  center_line_offset[AWB_CLIP_MAX_SECTION - 1];
	float  lower_line_slope[AWB_CLIP_MAX_SECTION - 1];
	float  lower_line_offset[AWB_CLIP_MAX_SECTION - 1];
	float  upper_line_slope[AWB_CLIP_MAX_SECTION - 1];
	float  upper_line_offset[AWB_CLIP_MAX_SECTION - 1];
	float  corner_limit[AWB_CLIP_MAX_SECTION];
}AWB_CONSTRAINT;

typedef struct hgisp_awb_coarse_constraint {
    float coarse_min_bg, coarse_lb_bg, coarse_rt_bg, coarse_max_bg;
    float coarse_min_rg, coarse_lb_rg, coarse_rt_rg, coarse_max_rg;
}AWB_COARSE_CONSTRAINT;

typedef struct
{
    // channel r gr gb b
    uint16 default_gain[4];
    uint16 awb_min_gain[4];
    uint16 awb_max_gain[4];
    // awb coarse polygon constraint
    AWB_COARSE_CONSTRAINT coarse_constraint;
    AWB_CONSTRAINT        constraint;
}_Sensor_AWB;

typedef struct hgisp_cfg_awb {
    uint32 cr_target    : 16, cb_target   : 16;
    uint32 front_cr_val :  8, back_cr_val : 8, cons_cr_max : 8, cons_cr_min : 8;
    uint32 front_cb_val :  8, back_cb_val : 8, cons_cb_max : 8, cons_cb_min : 8;
    uint32 front_uv_sum :  8, back_uv_sum : 8, cons_uv_max : 8, cons_uv_min : 8;
    uint32 back_cb_max  :  8, back_cr_max : 8, back_uv_max : 8, reserved1  : 8;
    uint32 back_cb_min  :  8, back_cr_min : 8, back_uv_min : 8, reserved2  : 8;
    int32  lock_hi_thr  :  8, lock_lo_thr : 8, reversed0   : 16;
    uint32 coarse_scale :  8, coarse_thr  : 8, fine_step   : 8, stable_thr  : 8;
    uint32 cbcr_thr     :  8, awb_cent_cons_en : 8, awb_back_cons_en : 8, reserved3 : 8;   
    uint32 awb_auto_en  :  4, awb_fine_cons_en : 4, awb_meas_mode : 3, awb_gain_type : 3, awb_formula : 2, awb_wp_max : 8, awb_wp_min : 8;
    uint32 awb_r_max : 8, awb_g_max : 8, awb_b_max : 8, awb_precision : 4, awb_coarse_cons_en : 4; 
    uint16 manual_gain[4];
    float  awb_back_wp_min_ratio;
    uint32 awb_crop_pixel_start_v : 16, awb_crop_pixel_start_h : 16;
    uint32 awb_crop_pixel_end_v   : 16, awb_crop_pixel_end_h   : 16;
    _Sensor_AWB sensor_awb;
}TYPE_HGISP_CFG_AWB;
    
typedef struct
{
    float  wdr_bv[8];                   // bv分段点, 用于控制每段的wdr_max_ns_slope,
    float  max_ns_slope[8];             //  每段bv, 允许的最大的noise floor映射的斜率, 设置范围[1.0~3.0]
    float  max_shadow_slope[8];         //  每段bv, 允许的最大的shadow映射的斜率, 设置范围[1.0~1.5]
}_Sensor_WDR;

typedef struct hgisp_cfg_wdr {
    uint32 auto_noise_floor_out : 8, wdr_en : 8, reserved : 16;        // auto_noise_floor_out : 打开自动计算噪声抑制输出控制点功能 wdr_en:是否使能wdr
    float  temporal_smooth_alpha;                                      // 帧间平滑控制, alpha越小，帧间亮度越稳定，但对场景变化响应越慢。典型值范围:  0.05 ~ 0.2
    uint16 noise_floor;                                                // 噪声抑制输入控制点
    uint16 noise_floor_out;                                            // 噪声抑制输出控制点
    uint16 shadow_boost_target;                                        // 暗部提亮的目标值(10 bit空间)
    uint16 highlight_compress_target;                                  // 高光压缩的目标值(10 bit空间)
    float  min_ns_percentile;                                          // 用于自动计算噪声抑制输出控制点, 获取指定最小百分位处的噪声的统计值
    float  max_ns_percentile;                                          // 用于自动计算噪声抑制输出控制点, 获取指定最大百分位处的噪声的统计值
    _Sensor_WDR sensor_wdr;
} TYPE_HGISP_CFG_WDR;

typedef struct
{
	float  row_time_us;
    uint32 max_analog_gain         : 16,
           min_analog_gain         : 16;
    uint32 max_exposure_line       : 16,
           min_exposure_line       : 16;
    uint32 default_exposure_line   : 16,
           expo_frame_interval     :  8,
           reserved0               :  8;
	float  to_day_bv;
	float  to_night_bv;
    uint8  dark_scene_target_lut[2];
    uint8  hs_scene_limit_lut[2];
	float  dark_scene_bv_lut[2];
	float  hs_scene_bv_lut[2];
    float  lowlight_lsb_bv_lut[8];
	uint8  lowlight_lsb_gain_lut[8];
    uint32 min_frame_vb : 8, curr_fps : 8,  max_frame_length : 16;
}_Sensor_AE;

typedef struct hgisp_cfg_ae {
    uint32 luma_target : 16, luma_weight_sum : 16;
    uint8  luma_weight[25];
    uint8  reserved0, reserved1, reserved2;
    uint32  ae_lock_cnt                 : 8,
            ae_lock_tolerance           : 8,
            ae_manual_en                : 8,
            reduce_fps_en               : 8;
    uint32 lowlight_lsb_gain_en         : 8,
           lowlight_lsb_gain_4hi_fps    : 8,
           lowlight_lsb_gain_4lo_fps    : 8,
           hist_hs_bin_thr              : 8;
    float  hist_upper_hs_pixel_ratio;   // [0, 1]   »ý·ÖÖ±·½Í¼ÁÁ²¿Ö¸¶¨Î»ÖÃµÄÏñËØÊýÁ¿, ÓÃÓÚµ÷ÕûAEÄ¿±êÖµ¡£
    float  hist_upper_pixel_ratio;      // not use
    float  hist_lower_pixel_ratio;      // not use

    uint8  abl_luma_target_max;
    uint8  dark_pos_thr_max;
    uint8  abl_hist_thr[2]; 
    float  abl_diff_ratio;
    uint32 abl_dark_pos_diff_thr   : 8,     // [0,61] ablÆØ¹âËø¶¨ãÐÖµ2£ºµ±µ÷Õûµ½ºÏÊÊÆØ¹âºó£¬Èç¹û°µÇøÈ¨ÖØÖµ±ä»¯Ð¡ÓÚ´ËãÐÖµ  £¬abl Target½«Ëø¶¨£¬±£³Ö²»±ä¡£  
           abl_bright_pos_diff_thr : 8,     // [0,61] ablÆØ¹âËø¶¨ãÐÖµ3£ºµ±µ÷Õûµ½ºÏÊÊÆØ¹âºó£¬Èç¹ûÁÁÇøÈ¨ÖØÖµ±ä»¯Ð¡ÓÚ´ËãÐÖµ  £¬abl Target½«Ëø¶¨£¬±£³Ö²»±ä¡£
           reserved3               : 16;       
    float  bright_pixel_high_ratio;
    float  bright_pixel_sub_ratio;      // [0, 1] Ö±·½Í¼ÁÁÏñËØÊý×ÔÊÊÓ¦Öµ£¨%£©¡£
    float  dark_pixel_low_ratio;
    float  dark_pixel_high_ratio;
    uint32 abl_dark_pos_low_wthr  : 8,     // [0,61] abl°µÇøÎ»ÖÃÈ¨ÖØ×îÐ¡ãÐÖµ¡£
           abl_dark_pos_add_wthr  : 8,     // [0,61] abl°µÇøÎ»ÖÃÈ¨ÖØ×ÔÊÊÓ¦Öµ¡£
           reversed4              : 16;        
    float  bright_pos_adjust_ratio;          
    uint32 aoe_bright_pos_wthr : 8,     // [0,61] aoeÁÁÇøÎ»ÖÃÈ¨ÖØãÐÖµ¡£
           aoe_dark_pos_wthr   : 8,     // [0,61] aoe°µÇøÎ»ÖÃÈ¨ÖØãÐÖµ¡£
           abl_bv_gain_sel     : 8,     // ablÅÐ¶ÏÑ¡Ôñ£º 0£ºÔöÒæ£»  1£ºbv
           reserved5           : 8;       
    float  abl_expo_line_low_ratio; 
    float  abl_expo_line_high_ratio;
    uint32 abl_expo_gain_low_thr;       // [0, ana_gain_max] ablÔöÒæÅÐ¶Ï¡£
    uint32 abl_expo_gain_high_thr;      // [0, ana_gain_max] ablÔöÒæÅÐ¶Ï¡£
    float  abl_bv_thr[2];
    float  aoe_bv_thr[2];
    uint16 aoe_expo_gain_thr[2];        // [0, ana_gain_max] aoeÔöÒæÅÐ¶Ï¡£
    uint32 stg_mode         : 8,        // ×Ô¶¯ÆØ¹â²ßÂÔ 0£º¸ß¹âÓÅÏÈ£» 1£ºµÍ¹âÓÅÏÈ
           stg_max_offset   : 8,        // fix(0,16,8), ÖµÔ½´ó£¬roi¶ÔÈ¨ÖØµÄÓ°ÏìÔ½´ó¡£
           stg_ratio_slope  : 16;       // fix(0,8,0)£¬ roi¶ÔÈ¨ÖØÓ°ÏìµÄ×î´ó³Ì¶È      
           
    uint32 ae_crop_start_v   : 16, ae_crop_start_h   : 16;  // need to reduce 1.
    uint32 ae_crop_size_v    : 16, ae_crop_size_h    : 16;  // NO need to reduce 1.
    uint32 hist_crop_start_v : 16, hist_crop_start_h : 16;  // NO need to reduce 1.
    uint32 hist_crop_end_v   : 16, hist_crop_end_h   : 16;  // NO need to reduce 1.
    _Sensor_AE sensor_ae;

} TYPE_HGISP_CFG_AE;

typedef struct
{
    int16 c00, c01, c02, c10, c11, c12, c20, c21, c22, c30, c31, c32;
}_Sensor_CCM;

typedef struct
{
    uint16 gr, gb, r, b;
}_Sensor_BLC;

typedef struct hgisp_md_param {
    uint32 win_start_h    : 16,
           win_start_v    : 16;
    uint32 win_end_h      : 16,
           win_end_v      : 16;
    uint32 frame_value    : 16,
           block_value    :  8,
           frame_interval :  8;
}_Sensor_MD;

typedef struct hgisp_csc_param {
    uint32 rgb2yuv_gamut    : 8, yuv2rgb_in_gamut  : 8, yuv2rgb_out_gamut : 8, rgb2yuv_range   : 8;
    uint32 yuv2rgb_in_range : 8, yuv2rgb_out_range : 8, y_gamma_alpha     : 8, rgb_gamma_alpha : 8;
}_Sensor_CSC;

typedef struct hgisp_dpc_param {
    uint32 static_psram_addr;
    uint32 white_threshold        : 8,
           black_threshold        : 8,
           sensitivity_value      : 8,
           white_threshold_min    : 8;
    uint32 black_threshold_min    : 8,
           dynamic_white_strength : 4,
           dynamic_black_strength : 4,
           reserved0              : 16;
}_Sensor_DPC;

typedef struct
{
	uint8  U_luma_thr_lo, U_luma_slop_lo, U_luma_shfb_lo, U_luma_gmin_lo;
	uint8  U_luma_thr_hi, U_luma_slop_hi, U_luma_shfb_hi, U_luma_gmin_hi;
	uint8  V_luma_thr_lo, V_luma_slop_lo, V_luma_shfb_lo, V_luma_gmin_lo;
	uint8  V_luma_thr_hi, V_luma_slop_hi, V_luma_shfb_hi, V_luma_gmin_hi;
	uint8  chroma_thr_lo, chroma_slop_lo, chroma_shfb_lo, chroma_gmin_lo;
}_Sensor_CSUPP;

typedef struct
{
    uint32 filt_alpha   : 8,
	       shrink_thr   : 8,
	       filt_clip_hi : 8,
	       filt_clip_lo : 8;
    uint32 sp_thr2      : 8,
	       sp_thr1      : 8,
	       enha_clip_hi : 8,
	       enha_clip_lo : 8;	
    uint32 e1 : 8, e2 : 8, e3 : 8, reserved0 : 8;
	uint32 k0 : 8, k1 : 8, k2 : 8, k3        : 8;
    uint32 y1 : 8, y2 : 8, y3 : 8, reserved1 : 8;
    int32  filt_w11 : 16, filt_w12  : 16;
    int32  filt_w13 : 16, filt_w21  : 16;
    int32  filt_w22 : 16, filt_w23  : 16;
    int32  filt_w31 : 16, filt_w32  : 16;
    int32  filt_w33 : 16, reserved2 : 16;
    uint32 filt_type : 8, filt_sbit : 8, lpf_scale : 8, reserved3 : 8;
	uint8  strength_lut[2]; uint8 reserved4, reserved5;
	float  strength_bv_lut[2];
}_Sensor_SHARP;

typedef struct
{
	uint8 y_thr_tal0,y_thr_tal1,y_thr_tal2,y_thr_tal3,y_thr_tal4,y_thr_tal5,y_thr_tal6,y_thr_tal7; 
	uint8 y_alfa, c_alfa, y_win_size, reserved0;
}_Sensor_YUVNR;

typedef struct
{
    uint8 yuv_range; // narrow : 0 full : 1
	uint8 luma; // range: 0 ~ 100
	uint8 contrast; // range: 0 ~ 100
	uint8 saturation; // range: 0 ~ 100
	int16 hue; // range: -180 ~ 180'
    int16 reserved0;
	int16 ce_in_ofs_y,  ce_in_ofs_cb,  ce_in_ofs_cr; // range: -128 ~ 128
	int16 ce_out_ofs_y, ce_out_ofs_cb, ce_out_ofs_cr; // range: -128 ~ 128
    uint8 adj_sat_by_bv;
    uint8 hi_sat;
    uint8 lo_sat;
    uint8 reserved1;
    float hi_sat_bv; // high brightness for saturation
    float lo_sat_bv; // low brightness for saturation
}_Sensor_COLENH;

typedef struct 
{
    float   bv;
    uint8   bnr_range_weight_index; 
    uint16  bnr_invksigma;
    uint8   bnr_intensity_threshold;
    uint8   yuvnr_idx;
    uint8   csupp_idx;
    uint8   h264_3dnr_lev;
	uint8 	h264_3dnr_en;
}_Sensor_BV2NR;

typedef struct 
{
    uint32  *p_lsc_tbl; 
}_Sensor_LSC;


typedef struct  {
    uint16 region_lower, region_center, region_upper, hue_value, sat_value, reserved0;
} _Sensor_LHS;

struct hgisp_head_info { 
    uint16 magic_num , version   , sensor_flag , sensor_num;
    uint16 basic_size, param_size, y_gamma_size, rgb_gamma_size, lsc_size;
    uint16 basic_crc , param_crc , y_gamma_crc , rgb_gamma_crc , lsc_crc ;
};

typedef struct {
    uint32 bv3dnr : 8, change : 8, reserved0 : 16;
    _Sensor_BV2NR bv2rawnr_map[BV2RAWNR_ARRAY_NUM];
    _Sensor_YUVNR bv2yuvnr_map[BV2YUVNR_ARRAY_NUM];
    _Sensor_CSUPP bv2csupp_map[BV2CSUPP_ARRAY_NUM];
}_Sensor_NR;

typedef struct {
    uint8 w_thres, w_slope, w_str, mu_thres, mu_slope, reserved0, reserved1, reserved2;
}_Sensor_GIC;

typedef struct {
    _Sensor_LHS local_hue_sat_map[LOCAL_HUE_SAT_COLOR];
}_Sensor_LHS_MAP;

struct hgisp_param_info {
    struct isp_func_cfg      enable_param;
    TYPE_HGISP_CFG_AWB       awb_param;
    struct hgisp_cfg_ae      cfg_ae;
    _Sensor_CCM              ccm_param;
    _Sensor_BLC              blc_param;
    _Sensor_MD               md_param;
    _Sensor_CSC              csc_param;
    _Sensor_DPC              dpc_param;
    _Sensor_COLENH           ce_param;
    _Sensor_SHARP            sharp_param;
    _Sensor_NR               nr_param;
    _Sensor_GIC              gic_param;
    _Sensor_LHS_MAP          lhs_param;       
    TYPE_HGISP_CFG_WDR       config_wdr;   
    ispcfg_malloc            malloc;
    ispcfg_free              free;
    uint32                   *y_gamma;
    uint32                   *rgb_gamma;
    uint32                   *lsc_tbl;
};

struct hgisp_basic_info {
    uint32 sensor_name[5];
    uint32 mirror             : 8,
           reverse            : 8,
           curr_mirror        : 8,
           curr_reverse       : 8;
    uint32 img_operation      : 8,
           reserved           : 24;
    uint32 sensor_size_v      : 16,
           sensor_size_h      : 16;
    uint32 input_size_v       : 16,
           input_size_h       : 16;
    uint32 input_size_startv  : 16,
           input_size_starth  : 16;
    uint32 crop_size_v        : 16,
           crop_size_h        : 16;
    uint32 crop_size_startv   : 16,
           crop_size_starth   : 16;
    uint32 bayer_type         :  8,
           input_type         :  8,
           format_type        :  8,
           sensor_index       :  8;
    uint32 init_addr;
    uint32 offset;
};

struct hgisp_sensor_info {
    uint32 info_src;
    struct hgisp_head_info      head_info;
    struct hgisp_basic_info     sensor_basic;
    struct hgisp_param_info     sensor_param;
};

struct hgisp_sensor_init {
    struct hgisp_sensor_info sensor_info[ISP_SUPPORT_SENSOR_MAX_NUM];
};
void *isp_sensor_param_load(uint16 *buff);
#ifdef __cplusplus  
}
#endif

#endif
