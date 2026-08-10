#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/isp.h"

/* lens & sensor config information:
- sensor      : sensor_name
- fstop       : TBD
- mclk        : TBD
- max FPS     : TBD
- frame length: TBD
- usage       : TBD
- interface   : TBD
*/

#if DEV_SENSOR_NAME
#define SLAVE_MODE      0

SENSOR_INIT_SECTION const unsigned char nameInitTable[CMOS_INIT_LEN]=
{
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
	//..............
	//..............
	//..............
	
    0xff, 0xff, 0xff,
};

const _Sensor_CCM name_ccm_init = {
    // 5500k, gamma2p2
	0x17A,  0xfc9,  0xffd,
	0xfa4,  0x148,  0xf0b,
	0xfe1,  0xfed,  0x1f6,
	0x000,  0x000,  0x000,
};

const _Sensor_BLC name_blc_init =
{
    255,  255,  255,  255,
};


const _Sensor_AWB name_awb_init = 
{
    .default_gain   = {383, 256, 256, 478},
    .awb_min_gain   = {305, 256, 256, 350},
    .awb_max_gain   = {528, 256, 256, 650},

    .coarse_constraint = {
        .coarse_min_bg =  70,
        .coarse_lb_bg  = 100,
        .coarse_rt_bg  = 120,
        .coarse_max_bg = 210,
        .coarse_min_rg = 100,
        .coarse_lb_rg  = 150,
        .coarse_rt_rg  = 145,
        .coarse_max_rg = 270,
    },

    .constraint = {
        .section_num = 4,
        .color_temp = { 6500, 5000, 4000, 2856, 0, 0, 0, 0},
        .sec_line_slope = {1.05882353, 1.28571429, 1.63157895, 1.62962963, 0, 0, 0, 0},
        .sec_line_offset = {22.05882353, -48.71428571, -142.00000000, -240.37037037, 0, 0, 0, 0},
        .sec_line_sqrtk2add1 = {0.68662353, 0.61394061, 0.52256206, 0.52301622, 0, 0, 0, 0},
        .center_line_slope = {-0.94444444, -0.61111111, -0.61363636, 0, 0, 0, 0},
        .center_line_offset = {292.50000000, 241.50000000, 241.93181818, 0, 0, 0, 0},
        .lower_line_slope = {-4.95081397, -0.62333997, -0.18264122, 0, 0, 0, 0},
        .lower_line_offset = {771.46450775, 202.34940780, 135.05015736, 0, 0, 0, 0},
        .upper_line_slope = {-0.95003818, -0.60443022, -0.61363631, 0, 0, 0, 0},
        .upper_line_offset = {313.94513029, 257.88437005, 259.53077303, 0, 0, 0, 0},
        .corner_limit = {124.70064701, 154.09480271, 145.29935299, 175.90519729, 207.15475670, 97.21515907, 222.84524330, 122.78484093},
    },
};

const _Sensor_AE name_ae_init = 
{
    .curr_fps              = (uint32)25*256,
    .max_frame_length      = 0,
    .min_frame_vb          = 0,
    .max_analog_gain       = 0,
    .min_analog_gain       = 0,
    .default_exposure_line = 0, //0x2e9,//0x2de,
    .max_exposure_line     = 0, //749,
    .min_exposure_line     = 0,
    .row_time_us           = 0,//89,
    .expo_frame_interval   = 0,
    .to_day_bv             = 962,       // 10lux
    .to_night_bv           = 465,
    .dark_scene_target_lut = {35, 54},
    .dark_scene_bv_lut     = {34, 280},
    .hs_scene_limit_lut    = {55, 75},
    .hs_scene_bv_lut       = {280, 3534},
    .lowlight_lsb_bv_lut   = {34, 115, 222, 400, 791, 1599, 3534,  1e30},
    .lowlight_lsb_gain_lut = {64, 52,  46,   35,  25,  20,   16,   16},  // u7.4
};

const _Sensor_DPC name_dpc_init = 
{
    .static_psram_addr      = (uint32)0,
    .white_threshold        = 115,
    .black_threshold        = 115,
    .white_threshold_min    = 30,
    .black_threshold_min    = 30,
    .sensitivity_value      = 128,
    .dynamic_white_strength = 4,
    .dynamic_black_strength = 4,
};

const _Sensor_GAMMA_BV name_gamma_map = 
{	
	.adj_by_bv  = 1,
	
	.bv 		= { 15000,  8000,  3000, 1200,  600,  300, 200, 100, },
    .y_alpha 	= {	  255,   255,   255,  192,  160,  128,  64,  32, },
    .rgb_alpha 	= {   255,   255,   255,  192,  160,  128,  64,  32, },
};

const _Sensor_CSC name_csc_init = 
{
    .rgb2yuv_gamut         = ISP_YUV_GAMUT_BT709,
    .rgb2yuv_range         = ISP_YUV_RANGE_NARROW,
    .yuv2rgb_in_gamut      = ISP_YUV_GAMUT_BT709,
    .yuv2rgb_in_range      = ISP_YUV_RANGE_NARROW,
    .yuv2rgb_out_gamut     = ISP_YUV_GAMUT_BT709,
    .yuv2rgb_out_range     = ISP_YUV_RANGE_NARROW,
    .y_gamma_alpha         = 0xff,
    .rgb_gamma_alpha       = 0xff,
    .gamma_alpha_map       = (void *)&gamma_alpha_map;
};

const _Sensor_GIC name_gic_init = 
{
    .w_thres  = 14,
    .w_slope  = 16,
    .w_str    = 127,
    .mu_thres = 5,
    .mu_slope = 16,
};

const _Sensor_CSUPP name_csupp_init = {
// uint8  U_luma_thr_lo, U_luma_slop_lo, U_luma_shfb_lo, U_luma_gmin_lo,
// uint8  U_luma_thr_hi, U_luma_slop_hi, U_luma_shfb_hi, U_luma_gmin_hi,
// uint8  V_luma_thr_lo, V_luma_slop_lo, V_luma_shfb_lo, V_luma_gmin_lo,
// uint8  V_luma_thr_hi, V_luma_slop_hi, V_luma_shfb_hi, V_luma_gmin_hi,
// uint8  chroma_thr_lo, chroma_slop_lo, chroma_shfb_lo, chroma_gmin_lo,
    0,   0,   0,   0,
    0,   0,   0,   0,
    0,   0,   0,   0,
    0,   0,   0,   0,
    0,   0,   0,   0,
};

const _Sensor_SHARP name_sharp_init = {
    .filt_alpha      = 128 ,
    .shrink_thr      = 5   ,
    .filt_clip_hi    = 127 ,
    .filt_clip_lo    = 127 ,
    
    .sp_thr2 		 = 20  ,    
    .sp_thr1 	 	 = 10  ,
    .enha_clip_hi 	 = 127 ,
    .enha_clip_lo	 = 127 ,
    
    .e1 = 8,  .e2 = 28, .e3 = 40,
    .k0 = 96, .k1 = 32, .k2 = 32, .k3 = 16,
    .y1 = 24,              // y1 = k0*e1
    .y2 = 44,              // y2 = k1*e2 + (y1 - k1*e1) = k1 * (e2 - e1) + y1
    .y3 = 56,              // y3 = k2*e3 + (y2 - k2*e2) = k2 * (e3 - e2) + y2

    // --- Unsharp Mask ---
    .filt_w11 = 7,  .filt_w12 = 9,  .filt_w13 = 10,
    .filt_w21 = 9,  .filt_w22 = 11, .filt_w23 = 12,
    .filt_w31 = 10, .filt_w32 = 12, .filt_w33 = 24,
    .filt_type = 1,
    .filt_sbit = 8,
    .lpf_scale = 1,

    // --- Sharpen Mask --- 
    // .filt_w11 = -2,  .filt_w12 = -12, .filt_w13 = -19,
    // .filt_w21 = -12, .filt_w22 = -24, .filt_w23 =  20,
    // .filt_w31 = -19, .filt_w32 =  20, .filt_w33 = 196,
    // .filt_type = 0,
    // .filt_sbit = 8,
    // .lpf_scale = 0,

    // .filt_w11 = 0,  .filt_w12 = 0,  .filt_w13 = -1,
    // .filt_w21 = 0,  .filt_w22 = -1, .filt_w23 = -2,
    // .filt_w31 = -1, .filt_w32 = -2, .filt_w33 = 16,
    // .filt_type = 0,
    // .filt_sbit = 4,
    // .lpf_scale = 0,

    .strength_lut    = {32,  255},
    .strength_bv_lut = {791, 9930},


};
};

const _Sensor_YUVNR name_yuvnr_init = {
	.y_thr_tal0 = 0,
	.y_thr_tal1 = 0,
	.y_thr_tal2 = 0,
	.y_thr_tal3 = 0,
	.y_thr_tal4 = 0,
	.y_thr_tal5 = 0,
	.y_thr_tal6 = 0,
	.y_thr_tal7 = 0,
	.y_alfa     = 0,
	.c_alfa     = 0,
	.y_win_size = 0,
};

const _Sensor_COLENH_BV name_ce_map[BV2COLENH_ARRAY_NUM] = {
	{.bv =    28000, .hue = 0, .luma = 50, .contrast = 60, .saturation = 70},
    {.bv =    14000, .hue = 0, .luma = 50, .contrast = 59, .saturation = 70},
    {.bv =    10000, .hue = 0, .luma = 50, .contrast = 58, .saturation = 70},
    {.bv =     7000, .hue = 0, .luma = 50, .contrast = 57, .saturation = 70},
    {.bv =     3534, .hue = 0, .luma = 50, .contrast = 55, .saturation = 70},
    {.bv =     1599, .hue = 0, .luma = 50, .contrast = 53, .saturation = 70},
    {.bv =      791, .hue = 0, .luma = 50, .contrast = 52, .saturation = 70},
    {.bv =      115, .hue = 0, .luma = 50, .contrast = 50, .saturation = 70},
};

const _Sensor_COLENH name_colenh_init = {
    .yuv_range  = 0,
    .luma       = 50, // range: 0 ~ 100
    .contrast   = 50, // range: 0 ~ 100
    .saturation = 70, // range: 0 ~ 100
    .hue        = 0, // range: -180 ~ 180
    .ce_in_ofs_y   = 128,
    .ce_in_ofs_cb  = 128,
    .ce_in_ofs_cr  = 128, // range: -128 ~ 128
    .ce_out_ofs_y  = 128, 
    .ce_out_ofs_cb = 128, 
    .ce_out_ofs_cr = 128, // range: -128 ~ 128
    .adj_by_bv_en  = 1,
    .bv2colenh_map = (void *)name_ce_map,
};

const _Sensor_BV2NR name_bv2nr_init[BV2RAWNR_ARRAY_NUM] = {
    //        bv, bnr_range_weight_index, bnr_invksigma, bnr_intensity_threshold, yuvnr_idx, csupp_idx,  h264_3dnr_lev, h264_3dnr_en
    {      29491,                     6 ,           511,                      63,         0,         0,         0,              0},    // 320lux
    {       3534,                     8 ,           407,                      63,         0,         0,         1,              1},    // 40lux
    {       1599,                     8 ,           271,                      63,         1,         0,         1,              1},    // 20lux
    {        791,                     16,           271,                      63,         1,         1,         2,              1},    // 10lux
    {        222,                     16,           135,                      48,         2,         1,         2,              1},    // 5p03lux
    {        115,                     16,           135,                      48,         2,         1,         2,              1},    // 2p5lux
    {         57,                     20,           101,                      32,         3,         1,         2,              1},    // 1p25lux
    {         34,                     24,            62,                      32,         4,         1,         2,              1},    // 0p62lux
    {         26,                     26,            50,                      32,         4,         1,         2,              1},    // 0p31lux
    {         21,                     28,            40,                      32,         5,         2,         2,              1},    // 0p1lux
    {         16,                     31,            25,                      32,         5,         2,         2,              1},    // 0p01lux	

};
const uint32 name_lsc_tbl[] = {
//R channel
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
//GR channel
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
//GB channel
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
//B channel
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,0x00040100,
};

const _Sensor_LSC          name_lsc_init = {
    .p_lsc_tbl = (uint32 *)name_lsc_tbl,
};

const _Sensor_LHS name_lhs_map[] = {
    // region defination: lower -> center -> upper(direction: anticlockwise)
    // region_lower, region_center, region_upper, hue adjust value, saturation adjust value
    //   (9 bits)      (9 bits)       (9 bits)          (9 bits)           (8 bits)
    {            24,            52,           80,                0,                       0},  // magenta,          range: 28
    {            80,           109,          138,                0,                       0},  // red,              range: 29
    {           140,           171,          202,               +0,                     +00},  // yellow,           range: 31
    {           204,           232,          260,              + 0,                       0},  // green,            range: 28
    {           261,           289,          317,                0,                       0},  // cyan,             range: 28
    {           320,           351,           22,                0,                       0},  // blue,             range: 31
    {           109,           132,          156,                0,                       0},  // skin enhance,     range:
    {           160,           203,          247,               +00,                      0},  // green enhance(plants),    range:
    {           296,           318,          340,                0,                       00}   // blue enhance,     range:
};


// 预设的Gamma曲线和对应的BV值
const _Sensor_YGAMMA name_ygamma_tbl[NUM_CURVES] = {
    {// gamma2.4，BV=100
     .bv = 790,
     .packed_lut = {
        0x08715C00, 0x0C62A887, 0x0F537CC6, 0x11C424F5, 0x13D4B51C, 0x15A5313D, 0x1755A15A, 0x18E60575, 
        0x1A46658E, 0x1BA6BDA4, 0x1CE711BA, 0x1E1761CE, 0x1F37A9E1, 0x2057F1F3, 0x21583605, 0x22587615, 
        0x2358B625, 0x2448F235, 0x25292E44, 0x26096652, 0x26E99E60, 0x27B9D66E, 0x288A0A7B, 0x295A3E88, 
        0x2A1A6E95, 0x2ADA9EA1, 0x2B9ACEAD, 0x2C5AFEB9, 0x2D0B2AC5, 0x2DBB56D0, 0x2E6B82DB, 0x2F0BAEE6, 
        0x2FBBDAF0, 0x305C02FB, 0x30FC2B05, 0x319C530F, 0x323C7B19, 0x32CCA323, 0x336CC72C, 0x33FCEB36, 
        0x348D133F, 0x351D3748, 0x35AD5B51, 0x363D7F5A, 0x36CD9F63, 0x374DC36C, 0x37DDE774, 0x385E077D, 
        0x38DE2785, 0x396E478D, 0x39EE6B96, 0x3A6E8B9E, 0x3ADEABA6, 0x3B5EC7AD, 0x3BDEE7B5, 0x3C5F07BD, 
        0x3CCF23C5, 0x3D4F43CC, 0x3DBF5FD4, 0x3E2F7FDB, 0x3EAF9BE2, 0x3F1FB7EA, 0x3F8FD3F1, 0x3FFFEFF8, 
}},
    {// Gamma 2.2，BV=200
     .bv = 1599,
     .packed_lut = {
        0x04809000, 0x0871A848, 0x0B628087, 0x0DD32CB6, 0x1003BCDD, 0x11E43D00, 0x13A4B11E, 0x15451D3A, 
        0x16C58154, 0x1825DD6C, 0x19863582, 0x1AC68998, 0x1BF6D9AC, 0x1D2725BF, 0x1E476DD2, 0x1F57B5E4, 
        0x2067F9F5, 0x21683A06, 0x22687A16, 0x2358B626, 0x2448F235, 0x25292E44, 0x26096652, 0x26E99E60, 
        0x27B9D26E, 0x288A0A7B, 0x295A3E88, 0x2A2A6E95, 0x2AEAA2A2, 0x2BAAD2AE, 0x2C6B02BA, 0x2D2B32C6, 
        0x2DDB5ED2, 0x2E8B8EDD, 0x2F4BBAE8, 0x2FFBE6F4, 0x309C12FF, 0x314C3F09, 0x31EC6714, 0x329C931E, 
        0x333CBB29, 0x33DCE333, 0x347D0B3D, 0x351D3347, 0x35AD5751, 0x364D7F5A, 0x36DDA364, 0x376DCB6D, 
        0x380DEF76, 0x389E1380, 0x392E3789, 0x39BE5B92, 0x3A4E7F9B, 0x3ACEA3A4, 0x3B5EC7AC, 0x3BDEE7B5, 
        0x3C6F0BBD, 0x3CEF2BC6, 0x3D7F4BCE, 0x3DFF6FD7, 0x3E7F8FDF, 0x3EFFAFE7, 0x3F7FCFEF, 0x3FFFEFF7, }},
    {// Gamma 1.8，BV=300
     .bv = 3500,
     .packed_lut = {
        0x06511400, 0x0951FC65, 0x0BB2A495, 0x0DB330BB, 0x0F83A8DB, 0x113418F8, 0x12B47D13, 0x1424DD2B, 
        0x15853542, 0x16D58D58, 0x1815DD6D, 0x19462981, 0x1A667594, 0x1B86BDA6, 0x1C9701B8, 0x1DA745C9, 
        0x1EA789DA, 0x1FA7C9EA, 0x209805FA, 0x21884609, 0x22787E18, 0x2358BA27, 0x2438F235, 0x25192A43, 
        0x25F96251, 0x26C99A5F, 0x2799CE6C, 0x286A0279, 0x293A3686, 0x2A0A6693, 0x2ACA9AA0, 0x2B8ACAAC, 
        0x2C4AFAB8, 0x2D0B2AC4, 0x2DCB5AD0, 0x2E7B86DC, 0x2F3BB6E7, 0x2FEBE2F3, 0x309C0EFE, 0x314C3B09, 
        0x31FC6714, 0x32AC931F, 0x334CBF2A, 0x33FCE734, 0x349D133F, 0x354D3B49, 0x35ED6754, 0x368D8F5E, 
        0x372DB768, 0x37CDDF72, 0x386E077C, 0x390E2F86, 0x399E5390, 0x3A3E7B99, 0x3ACEA3A3, 0x3B6EC7AC, 
        0x3BFEEFB6, 0x3C9F13BF, 0x3D2F37C9, 0x3DBF5BD2, 0x3E4F83DB, 0x3EDFA7E4, 0x3F6FCBED, 0x3FFFEFF6, }},
    {// Gamma 1.4，BV=400
     .bv = 5000,
     .packed_lut = {
        0x02604C00, 0x04B0E426, 0x06E1744B, 0x08E1FC6E, 0x0AC2748E, 0x0C72E8AC, 0x0E2354C7, 0x0FB3BCE2, 
        0x114420FB, 0x12C48114, 0x1434E12C, 0x15A53943, 0x16F5915A, 0x1835E56F, 0x19763583, 0x1AA68197, 
        0x1BD6D1AA, 0x1D071DBD, 0x1E3769D0, 0x1F67B1E3, 0x2087FDF6, 0x21984608, 0x22B88A19, 0x23C8CE2B, 
        0x24C9123C, 0x25C9524C, 0x26B98E5C, 0x27A9CA6B, 0x288A067A, 0x296A3E88, 0x2A3A7296, 0x2B0AAAA3, 
        0x2BDADEB0, 0x2C9B0EBD, 0x2D6B42C9, 0x2E1B72D6, 0x2EDB9EE1, 0x2F8BCEED, 0x304BFAF8, 0x30EC2704, 
        0x319C530E, 0x324C7F19, 0x32FCA724, 0x339CD32F, 0x344CFF39, 0x34ED2744, 0x358D4F4E, 0x363D7B58, 
        0x36DDA363, 0x377DCB6D, 0x381DF377, 0x38BE1B81, 0x395E438B, 0x39FE6B95, 0x3A9E939F, 0x3B2EB7A9, 
        0x3BCEDFB2, 0x3C6F07BC, 0x3CFF2FC6, 0x3D9F53CF, 0x3E3F7BD9, 0x3ECFA3E3, 0x3F6FC7EC, 0x3FFFEFF6, }},
    {// 线性曲线，BV=500
     .bv = 6000,
     .packed_lut = {
        0x01002000, 0x02006010, 0x0300A020, 0x0400E030, 0x05012040, 0x06016050, 0x0701A060, 0x0801E070, 
        0x09022080, 0x0A026090, 0x0B02A0A0, 0x0C02E0B0, 0x0D0320C0, 0x0E0360D0, 0x0F03A0E0, 0x1003E0F0, 
        0x11042100, 0x12046110, 0x1304A120, 0x1404E130, 0x15052140, 0x16056150, 0x1705A160, 0x1805E170, 
        0x19062180, 0x1A066190, 0x1B06A1A0, 0x1C06E1B0, 0x1D0721C0, 0x1E0761D0, 0x1F07A1E0, 0x2007E1F0, 
        0x21082200, 0x22086210, 0x2308A220, 0x2408E230, 0x25092240, 0x26096250, 0x2709A260, 0x2809E270, 
        0x290A2280, 0x2A0A6290, 0x2B0AA2A0, 0x2C0AE2B0, 0x2D0B22C0, 0x2E0B62D0, 0x2F0BA2E0, 0x300BE2F0, 
        0x310C2300, 0x320C6310, 0x330CA320, 0x340CE330, 0x350D2340, 0x360D6350, 0x370DA360, 0x380DE370, 
        0x390E2380, 0x3A0E6390, 0x3B0EA3A0, 0x3C0EE3B0, 0x3D0F23C0, 0x3E0F63D0, 0x3F0FA3E0, 0x3FFFE3F0,}}
};

const _Sensor_WDR name_wdr_init = {
    .wdr_bv           = {791, 1599, 3534,  7000, 10000, 14000, 28000, 56000},
    .max_ns_slope     = {1.0,  1.0,  1.0,   1.0,  1.25,   1.5,   2.0,   3.0},
    .max_shadow_slope = {1.0,  1.0,  1.0,   1.0,   1.0,  1.25,   1.5,   1.5},
};
uint8 name_regValTable[25][6] = {
    // 00d1  00d0  0dc1  00b8  00b9  0155 
    {  0x00, 0x00, 0x00, 0x01, 0x00, 0x00},
    {  0x0A, 0x00, 0x00, 0x01, 0x0c, 0x00},
    {  0x00, 0x01, 0x00, 0x01, 0x1a, 0x00},
    {  0x0A, 0x01, 0x00, 0x01, 0x2a, 0x00},
    {  0x00, 0x02, 0x00, 0x02, 0x00, 0x00},
    {  0x0A, 0x02, 0x00, 0x02, 0x18, 0x00},
    {  0x00, 0x03, 0x00, 0x02, 0x33, 0x00},
    {  0x0A, 0x03, 0x00, 0x03, 0x14, 0x00},
    {  0x00, 0x04, 0x00, 0x04, 0x00, 0x02},
    {  0x0A, 0x04, 0x00, 0x04, 0x2f, 0x02},
    {  0x00, 0x05, 0x00, 0x05, 0x26, 0x02},
    {  0x0A, 0x05, 0x00, 0x06, 0x29, 0x02},
    {  0x00, 0x06, 0x00, 0x08, 0x00, 0x02},
    {  0x0A, 0x06, 0x00, 0x09, 0x1f, 0x04},
    {  0x12, 0x46, 0x00, 0x0b, 0x0d, 0x04},
    {  0x19, 0x66, 0x00, 0x0d, 0x12, 0x06},
    {  0x00, 0x04, 0x01, 0x10, 0x00, 0x06},				
    {  0x0A, 0x04, 0x01, 0x12, 0x3e, 0x08},
    {  0x00, 0x05, 0x01, 0x16, 0x1a, 0x0a},
    {  0x0A, 0x05, 0x01, 0x1a, 0x23, 0x0c},
    {  0x00, 0x06, 0x01, 0x20, 0x00, 0x0c},
    {  0x0A, 0x06, 0x01, 0x25, 0x3b, 0x0f},
    {  0x12, 0x46, 0x01, 0x2c, 0x33, 0x12},
    {  0x19, 0x66, 0x01, 0x35, 0x06, 0x14},
    {  0x20, 0x06, 0x01, 0x3f, 0x3f, 0x15},
};

uint32 name_gainLevelTable[26] = {
    64,  
    76,  
    90,  
    106, 
    128, 
    152, 
    
    179,
    212, 
    256, 
    303, 
    358, 
    425, 
    
    512, 
    607, 
    717, 
    849, 
        
    1024,
    1213,
    1434,
    1699,
    2048,			
    2427,
    2867,
    3398,
    4096,							
    0xffffffff,
};

void name_ae_adjust(struct isp_exposure_opt *p_cfg)
{
    uint32 i            = 0;
    uint32 index        = 0;
    uint32 total        = sizeof(name_gainLevelTable) / sizeof(uint32);
    uint32 tol_dig_gain = 0;
    uint32 gain         = p_cfg->analog_gain >> 2;
    uint8  *addr        = (uint8 *)p_cfg->data.addr;

    for(i = 0; i < total; i++)
    {
        if((name_gainLevelTable[i] <= gain)&&(gain < name_gainLevelTable[i+1]))
            break;
    }

    //tol_dig_gain = (gain)*64/name_gainLevelTable[i];
    tol_dig_gain = (p_cfg->analog_gain)*(64>>2)/name_gainLevelTable[i];

    if (p_cfg->chg_fps_flag) {
        addr[index++] = 0x0d;
        addr[index++] = 0x41;
        addr[index++] = p_cfg->frame_length >> 8;
        addr[index++] = 0x0d;
        addr[index++] = 0x42;
        addr[index++] = p_cfg->frame_length & 0xff;
    }
    addr[index++] = 0x00;
    addr[index++] = 0xd1;
    addr[index++] = name_regValTable[i][0];
    addr[index++] = 0x00;
    addr[index++] = 0xd0;
    addr[index++] = name_regValTable[i][1];
    addr[index++] = 0x03;
    addr[index++] = 0x1d;
    addr[index++] = 0x2e;
    addr[index++] = 0x0d;
    addr[index++] = 0xc1;
    addr[index++] = name_regValTable[i][2];
    addr[index++] = 0x03;
    addr[index++] = 0x1d;
    addr[index++] = 0x28;
    addr[index++] = 0x00;
    addr[index++] = 0xb8;
    addr[index++] = name_regValTable[i][3];
    addr[index++] = 0x00;
    addr[index++] = 0xb9;
    addr[index++] = name_regValTable[i][4];
    addr[index++] = 0x01;
    addr[index++] = 0x55;
    addr[index++] = name_regValTable[i][5];
    addr[index++] = 0x00;
    addr[index++] = 0xb1;
    addr[index++] = (uint8)(tol_dig_gain>>6);
    addr[index++] = 0x00;
    addr[index++] = 0xb2;
    addr[index++] = (uint8)((tol_dig_gain&0x3f)<<2);
    addr[index++] = 0x0d;
    addr[index++] = 0x03;
    addr[index++] = (uint8)(p_cfg->exposure_line >> 8);
    addr[index++] = 0x0d;
    addr[index++] = 0x04;
    addr[index++] = (p_cfg->exposure_line & 0xff);
    p_cfg->data.size = index;
    p_cfg->cmd_len   = 2+1;
}

void name_img_opt(struct isp_sensor_opt *p_opt)
{
    uint8  *addr = (uint8 *)p_opt->data.addr;
    uint8  index = 0;
    addr[index++] = 0x00;
    addr[index++] = 0x15;
    addr[index++] = p_opt->reverse_en*2 + p_opt->mirror_en;

    addr[index++] = 0x0d;
    addr[index++] = 0x15;
    addr[index++] = p_opt->reverse_en*2 + p_opt->mirror_en;
    p_opt->data.size = index;
    p_opt->cmd_len   = 2+1;
}

void name_fps_opt(struct isp_sensor_opt *p_opt)
{
    uint8  *addr        = (uint8 *)p_opt->data.addr;
    uint8  index        = 0;
    addr[index++]       = 0x0d;
    addr[index++]       = 0x41;
    addr[index++]       = p_opt->curr_length >> 8;
    addr[index++]       = 0x0d;
    addr[index++]       = 0x42;
    addr[index++]       = p_opt->curr_length & 0xff;
    p_opt->data.size    = index;
    p_opt->cmd_len      = 2+1;
}

const _Sensor_ISP_CFG name_isp_init = 
{
    .type         = ISP_INPUT_DAT_SRC_MIPI0,
    .pixel_h      = 720,
    .pixel_w      = 1280,
    .bayer_patten = ISP_BAYER_FORMAT_GRBG,
    .input_format = ISP_INPUT_DAT_FORMAT_RAW10,
    .adjust_func  = (isp_ae_func     )name_ae_adjust,
    .img_opt      = (sensor_img_opt  )name_img_opt,
    .fps_opt      = (sensor_fps_opt  )name_fps_opt,	
};


SENSOR_OP_SECTION const _Sensor_Adpt_ name_cmd = 
{
	.typ = 1, //YUV
	.pixelw = 1280,
	.pixelh= 720,
	.hsyn = 1,
	.vsyn = 1,
	.rduline = 0,//
	.rawwide = 1,//10bit
	.colrarray = 1,//0:_RGRG_ 1:_GRGR_,2:_BGBG_,3:_GBGB_
	.init = (uint8 *)nameInitTable,
    .init_len = sizeof(nameInitTable),
    .mipi_lane_num = 1,
	.rotate_adapt = {0},
	//.hvb_adapt = {0x6a,0x12,0x6a,0x12},
	. mclk = 24000000,
	.p_fun_adapt = {NULL,NULL,NULL},
    .sensor_isp_cfg = (_Sensor_ISP_CFG *)&name_isp_init,
};

const _Sensor_Ident_ name_init =
{
	0x84, 0x6e, 0x6f, 0x02, 0x01, 0x03f1
};
#endif

