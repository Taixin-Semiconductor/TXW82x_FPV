#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/isp.h"

/* lens & sensor config information:
- sensor      : gc20C3
- fstop       : 1.6
- mclk        : 24MHz
- max FPS     : TBD
- frame length: TBD
- usage       : TEST
- interface   : mipi 2 lane
*/

#if DEV_SENSOR_GC20C3


SENSOR_INIT_SECTION const unsigned char GC20C3InitTable[]={
	//version a2b7df
	//00_GC20C3_MIPI2L_24M_1920x1080_24.000fps_raw10_linear
	//RESOLUTION width="1920" height="1080"
	//BAYERMODE type= GrRBGb
	//mipi_rate = 540 Mbps/lane

	//vts = 1125,row time=37.037037us  hts=4000
	0x03,0xfe,0xff, 
	0x03,0xfe,0x00,
	0x03,0xfe,0x10, 
	0x01,0x90,0x03,
	0x0b,0x4d,0x02,		
	0x0d,0x40,0x01, 
	0x00,0x87,0x50, 
	0x02,0x09,0x00,
	0x03,0xb5,0x11,
	0x03,0x1c,0x18,
	0x03,0xb2,0x03,
	0x03,0xbb,0xff, 
	0x03,0xbe,0xff, 
	0x0d,0x10,0x06, 
	0x0d,0x11,0x0b, 
	0x0d,0x10,0x07,  //
	0x0d,0x1a,0x02, 
	0x0d,0x15,0x03,  //
	0x0d,0x12,0x05,  //
	0x0d,0x16,0x00,  //
	0x0d,0x17,0x87,  //
	0x0d,0x18,0x01,  //
	0x0d,0x19,0x48,  //
	0x01,0x45,0x0d, 
	0x01,0x44,0x02, 
	0x01,0x42,0x04, 
	0x01,0x43,0x14, 
	0x01,0x46,0x05, 
	0x01,0x41,0x05, 
	0x01,0x49,0x05, 
	0x01,0x4a,0x07, 
	0x01,0x4b,0x06, 

	//0x04,0x9c,0x04,

	0x0b,0x4e,0x88, 
	0x0e,0x0c,0x00, 
	0x0e,0x0f,0x00, 
	0x0e,0x3a,0x98,
	0x0b,0x45,0x06,
	0x0b,0x47,0xf0, 
	0x0d,0x30,0x06, 

	0x0d,0x2f,0x05, 
	0x0b,0x40,0x57, 
	0x0b,0x43,0x00, 
	0x0b,0x41,0x0c, 
	0x0d,0x31,0x03, 
	0x0c,0x23,0x40, 
	0x0e,0x23,0x1a, 
	0x0e,0x2a,0x09, 
	0x0e,0x2b,0xc9, 
	0x0e,0x41,0x87, 
	0x0e,0x3b,0xb5, 
	0x0e,0x3a,0x15, 
	0x0e,0x37,0x1f, 
	0x02,0x17,0x02, 
	0x02,0x13,0x04,
	0x02,0x19,0xc2,
	0x02,0x59,0x04,
	0x02,0x5a,0x5e,  //1118
	0x02,0x11,0x01,
	0x03,0x40,0x04,
	0x03,0x41,0x65,  //1125
	0x03,0x42,0x04,
	0x03,0x43,0xe2,  //1000//1250
	0x02,0x12,0x14,
	0x03,0x50,0x06,
	0x03,0x48,0x07,
	0x03,0x49,0x88,
	0x03,0x4a,0x04,
	0x03,0x4b,0x40,
	0x03,0x47,0x00,
	0x0b,0x0c,0x00,
	0x0b,0x0d,0x02,
	0x0b,0x0e,0x07,
	0x0b,0x0f,0x8a,
	0x03,0x4e,0x07,
	0x03,0x4f,0xa8,
	0x00,0x04,0x0f, 
	0x04,0x44,0x00, 
	0x00,0x38,0x20, 
	0x00,0x39,0x20, 
	0x00,0x3a,0x20, 
	0x00,0x3b,0x20, 
	0x04,0x92,0x00, 
	0x04,0x93,0x00, 
	0x00,0x70,0x00, 
	0x00,0x94,0x07, 
	0x00,0x95,0x80, 
	0x00,0x96,0x04, 
	0x00,0x97,0x38, 
	0x00,0x99,0x04, 
	0x00,0x9b,0x04, 
	0x04,0x38,0x0f,
	0x04,0x39,0xf0,
	0x02,0x1a,0x10,
	0x04,0x76,0x01,
	0x04,0x30,0x23,
	0x04,0x43,0x02,
	0x00,0x38,0x00,
	0x00,0x39,0x00,
	0x00,0x3a,0x00,
	0x00,0x3b,0x00,
	0x00,0x70,0x80,
	0x04,0x48,0x0d,
	0x04,0x49,0x0d,
	0x04,0x4a,0x0d,
	0x04,0x4b,0x0d,
	0x04,0x4c,0x74,
	0x04,0x4d,0x74,
	0x04,0x4e,0x74,
	0x04,0x4f,0x74,		
	0x04,0x85,0x68,
	0x0d,0x38,0x07, 
	0x0d,0x39,0x57, 
	0x0e,0x4e,0xa9,  
	0x00,0x72,0x09,
	0x00,0x73,0x05,
	0x0c,0x20,0x09,
	0x0c,0x1d,0x02,
	0x0c,0x1e,0x2c,
	0x0c,0x1f,0xe6,
	0x0c,0x19,0x00,
	0x0c,0x1a,0x11,
	0x0c,0x1b,0x00,
	0x0c,0x1c,0x80,
	0x02,0x61,0x13,
	0x00,0x04,0x0f,
	0x00,0x52,0x00, 
	0x00,0x53,0x20, 
	0x00,0x55,0x20, 
	0x01,0x52,0x14,		
	0x01,0x00,0x03,
	0x03,0x1c,0x1f, 

	0x03,0x36,0x01,
	0x03,0x36,0x00, 
	0x03,0xfe,0x00,

    0xff, 0xff, 0xff,
};

const _Sensor_CCM gc20C3_ccm_init = {
    // 5500k, gamma2p2
    // 0x17A,  0xfc9,  0xffd,
    // 0xfa4,  0x148,  0xf0b,
    // 0xfe1,  0xfed,  0x1f6,
    // 0x000,  0x000,  0x000, 

    311,    -130,	-59,
    26 ,	509	,   -42,
    -81, 	-123,	356,
    0   ,   0   ,   0  ,
};

const _Sensor_BLC gc20C3_blc_init =
{
    // 251, 251, 251, 251,
    249,249, 249, 249,
};

const _Sensor_AWB gc20C3_awb_init = 
{
	.default_gain = {383, 256, 256, 478},
	.awb_min_gain = {305, 256, 256, 350},
	.awb_max_gain = {528, 256, 256, 650},
	
 	.coarse_constraint = {
 	    .coarse_min_bg = 50.000000f,
 	    .coarse_lb_bg = 100.000000f,
 	    .coarse_rt_bg = 150.000000f,
 	    .coarse_max_bg = 210.000000f,
 	    .coarse_min_rg = 90.000000f,
 	    .coarse_lb_rg = 150.000000f,
 	    .coarse_rt_rg = 175.000000f,
 	    .coarse_max_rg = 270.000000f,
 	},
	
	.constraint = {
		.section_num = 4,
		.color_temp = {6500, 5000, 4000, 3000, 0, 0, 0, 0},
		.sec_line_slope = {2.906283f, 4.151833f, 5.258988f, 3.349774f, 0.000000f, 0.000000f, 0.000000f, 0.000000f},
		.sec_line_offset = {-248.055008f, -527.689392f, -851.933899f, -629.473267f, 0.000000f, 0.000000f, 0.000000f, 0.000000f},
		.sec_line_offset = {-248.055008f, -527.689392f, -851.933899f, -629.473267f, 0.000000f, 0.000000f, 0.000000f, 0.000000f},
		.sec_line_sqrtk2add1 = {0.325361f, 0.234161f, 0.186803f, 0.286053f, 0.000000f, 0.000000f, 0.000000f, 0.000000f},
		.center_line_slope = {-0.333333f, -0.466667f, -0.452381f, 0.000000f, 0.000000f, 0.000000f, 0.000000f},
		.center_line_offset = {189.333328f, 210.666672f, 208.166672f, 0.000000f, 0.000000f, 0.000000f, 0.000000f},
		.lower_line_slope = {-0.502238f, -0.421670f, -0.392741f, 0.000000f, 0.000000f, 0.000000f, 0.000000f},
		.upper_line_slope = {-0.373665f, -0.481909f, -0.448209f, 0.000000f, 0.000000f, 0.000000f, 0.000000f},
		.upper_line_offset = {231.678986f, 249.842117f, 243.374527f, 0.000000f, 0.000000f, 0.000000f, 0.000000f},
		.corner_limit = {124.727631f, 114.438744f, 210.867920f, 76.886543f, 146.262711f, 177.025742f, 229.818771f, 140.367645f},
	},

};

const _Sensor_AE gc20C3_ae_init = 
{
    .max_frame_length      = 1120,
    .min_frame_vb          = 1,
    .max_analog_gain       = 24<<8,
    .min_analog_gain       = 1<<8,
    .default_exposure_line = 1120,
    .max_exposure_line     = 1120,
    .min_exposure_line     = 1,
    .row_time_us           = 37,
    .expo_frame_interval   = 2,
    .to_day_bv             = 962,       // 10lux
    .to_night_bv           = 465,
    .dark_scene_target_lut = {35, 54},
    .dark_scene_bv_lut     = {34, 280},
    .hs_scene_limit_lut    = {55, 75},
    .hs_scene_bv_lut       = {280, 3534},
    .lowlight_lsb_bv_lut   = {34, 115, 222, 400, 791, 1599, 3534,  1e30},
    .lowlight_lsb_gain_lut = {64, 52,  46,   35,  25,  20,   16,   16},  // u7.4
};


const _Sensor_DPC gc20C3_dpc_init = 
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

const _Sensor_GAMMA_BV gc20C3_gamma_map = 
{
    .bv 		= { 15000,  8000,  3000, 1200,  600,  300, 200, 100, },
    .y_alpha 	= {	  255,   255,   255,  192,  160,  128,  64,  32, },
    .rgb_alpha 	= {   255,   255,   255,  192,  160,  128,  64,  32, },
};

const _Sensor_CSC gc20C3_csc_init = 
{
    .rgb2yuv_gamut         = ISP_YUV_GAMUT_BT709,
    .rgb2yuv_range         = ISP_YUV_RANGE_NARROW,
    .yuv2rgb_in_gamut      = ISP_YUV_GAMUT_BT709,
    .yuv2rgb_in_range      = ISP_YUV_RANGE_NARROW,
    .yuv2rgb_out_gamut     = ISP_YUV_GAMUT_BT709,
    .yuv2rgb_out_range     = ISP_YUV_RANGE_NARROW,
    .y_gamma_alpha         = 0xff,
    .rgb_gamma_alpha       = 0xff,
	.gamma_alpha_map       = (void *)&gc20C3_gamma_map,
};


const _Sensor_GIC gc20C3_gic_init = 
{
    .w_thres  = 14,
    .w_slope  = 16,
    .w_str    = 127,
    .mu_thres = 5,
    .mu_slope = 16,
};

const _Sensor_BV2NR gc20C3_bv2nr_init[BV2RAWNR_ARRAY_NUM] = {
    //           ev, bnr_range_weight_index, bnr_invksigma, bnr_intensity_threshold, yuvnr_idx, csupp_idx
    {      29491,                     6 ,           511,                      63,         0,         0,         0,              1},    // 320lux
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

const _Sensor_CSUPP gc20C3_csupp_init = {
// u8  U_luma_thr_lo, U_luma_slop_lo, U_luma_shfb_lo, U_luma_gmin_lo,
// u8  U_luma_thr_hi, U_luma_slop_hi, U_luma_shfb_hi, U_luma_gmin_hi,
// u8  V_luma_thr_lo, V_luma_slop_lo, V_luma_shfb_lo, V_luma_gmin_lo,
// u8  V_luma_thr_hi, V_luma_slop_hi, V_luma_shfb_hi, V_luma_gmin_hi,
// u8  chroma_thr_lo, chroma_slop_lo, chroma_shfb_lo, chroma_gmin_lo,
     31,   4,   0,   0,
    209,   4,   0,   0,
     31,   4,   0,   0,
    209,   4,   0,   0,
     31,   4,   0,   0,
};

const _Sensor_SHARP gc20C3_sharp_init = {

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

const _Sensor_YUVNR gc20C3_yuvnr_init = {
    .y_thr_tal0 = 20,
	.y_thr_tal1 = 20,
	.y_thr_tal2 = 20,
	.y_thr_tal3 = 20,
	.y_thr_tal4 = 20,
	.y_thr_tal5 = 20,
	.y_thr_tal6 = 20,
	.y_thr_tal7 = 20,
	.y_alfa     = 205,
	.c_alfa     = 255,
	.y_win_size = 0,
};

const _Sensor_COLENH_BV gc20C3_ce_map[BV2COLENH_ARRAY_NUM] = {
    {.bv =  29491, .hue = 0, .luma = 50, .contrast = 56, .saturation = 70},
    {.bv =  10000, .hue = 0, .luma = 50, .contrast = 56, .saturation = 70},
    {.bv =   3534, .hue = 0, .luma = 50, .contrast = 56, .saturation = 70},
    {.bv =    400, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
    {.bv =    222, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
    {.bv =    115, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
    {.bv =     57, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
    {.bv =     34, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
};

const _Sensor_COLENH gc20C3_colenh_init = {
    .yuv_range     = 0,
    .luma          = 50, // range: 0 ~ 100
    .contrast      = 50, // range: 0 ~ 100
    .saturation    = 70, // range: 0 ~ 100
    .hue           = 0, // range: -180 ~ 180
    .ce_in_ofs_y   = 128,
    .ce_in_ofs_cb  = 128,
    .ce_in_ofs_cr  = 128, // range: -128 ~ 128
    .ce_out_ofs_y  = 128, 
    .ce_out_ofs_cb = 128, 
    .ce_out_ofs_cr = 128, // range: -128 ~ 128
    .adj_by_bv_en  = 1,
    .bv2colenh_map = (void *)gc20C3_ce_map,
};

const uint32 gc20C3_lsc_tbl[] = {
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

const _Sensor_LSC          gc20C3_lsc_init = {
    .p_lsc_tbl = (uint32 *)gc20C3_lsc_tbl,
};

const _Sensor_LHS gc20C3_lhs_map[] = {
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
const _Sensor_YGAMMA gc20C3_ygamma_tbl[NUM_CURVES] = {
    {
     .bv = 200,
     .packed_lut = {
        0x01002000, 0x02006010, 0x0300A020, 0x0400E030, 0x05012040, 0x06016050, 0x0701A060, 0x0801E070, 
        0x09022080, 0x0A026090, 0x0B02A0A0, 0x0C02E0B0, 0x0D0320C0, 0x0E0360D0, 0x0F03A0E0, 0x1003E0F0, 
        0x11042100, 0x12046110, 0x1304A120, 0x1404E130, 0x15052140, 0x16056150, 0x1705A160, 0x1805E170, 
        0x19062180, 0x1A066190, 0x1B06A1A0, 0x1C06E1B0, 0x1D0721C0, 0x1E0761D0, 0x1F07A1E0, 0x2007E1F0, 
        0x21082200, 0x22086210, 0x2308A220, 0x2408E230, 0x25092240, 0x26096250, 0x2709A260, 0x2809E270, 
        0x290A2280, 0x2A0A6290, 0x2B0AA2A0, 0x2C0AE2B0, 0x2D0B22C0, 0x2E0B62D0, 0x2F0BA2E0, 0x300BE2F0, 
        0x310C2300, 0x320C6310, 0x330CA320, 0x340CE330, 0x350D2340, 0x360D6350, 0x370DA360, 0x380DE370, 
        0x390E2380, 0x3A0E6390, 0x3B0EA3A0, 0x3C0EE3B0, 0x3D0F23C0, 0x3E0F63D0, 0x3F0FA3E0, 0x3FFFE3F0,}},
    {
     .bv = 500,
     .packed_lut = {
        0x01002000, 0x02006010, 0x0300A020, 0x0400E030, 0x05012040, 0x06016050, 0x0701A060, 0x0801E070, 
        0x09022080, 0x0A026090, 0x0B02A0A0, 0x0C02E0B0, 0x0D0320C0, 0x0E0360D0, 0x0F03A0E0, 0x1003E0F0, 
        0x11042100, 0x12046110, 0x1304A120, 0x1404E130, 0x15052140, 0x16056150, 0x1705A160, 0x1805E170, 
        0x19062180, 0x1A066190, 0x1B06A1A0, 0x1C06E1B0, 0x1D0721C0, 0x1E0761D0, 0x1F07A1E0, 0x2007E1F0, 
        0x21082200, 0x22086210, 0x2308A220, 0x2408E230, 0x25092240, 0x26096250, 0x2709A260, 0x2809E270, 
        0x290A2280, 0x2A0A6290, 0x2B0AA2A0, 0x2C0AE2B0, 0x2D0B22C0, 0x2E0B62D0, 0x2F0BA2E0, 0x300BE2F0, 
        0x310C2300, 0x320C6310, 0x330CA320, 0x340CE330, 0x350D2340, 0x360D6350, 0x370DA360, 0x380DE370, 
        0x390E2380, 0x3A0E6390, 0x3B0EA3A0, 0x3C0EE3B0, 0x3D0F23C0, 0x3E0F63D0, 0x3F0FA3E0, 0x3FFFE3F0,}},
    {
     .bv = 1000,
     .packed_lut = {
        0x01002000, 0x02006010, 0x0300A020, 0x0400E030, 0x05012040, 0x06016050, 0x0701A060, 0x0801E070, 
        0x09022080, 0x0A026090, 0x0B02A0A0, 0x0C02E0B0, 0x0D0320C0, 0x0E0360D0, 0x0F03A0E0, 0x1003E0F0, 
        0x11042100, 0x12046110, 0x1304A120, 0x1404E130, 0x15052140, 0x16056150, 0x1705A160, 0x1805E170, 
        0x19062180, 0x1A066190, 0x1B06A1A0, 0x1C06E1B0, 0x1D0721C0, 0x1E0761D0, 0x1F07A1E0, 0x2007E1F0, 
        0x21082200, 0x22086210, 0x2308A220, 0x2408E230, 0x25092240, 0x26096250, 0x2709A260, 0x2809E270, 
        0x290A2280, 0x2A0A6290, 0x2B0AA2A0, 0x2C0AE2B0, 0x2D0B22C0, 0x2E0B62D0, 0x2F0BA2E0, 0x300BE2F0, 
        0x310C2300, 0x320C6310, 0x330CA320, 0x340CE330, 0x350D2340, 0x360D6350, 0x370DA360, 0x380DE370, 
        0x390E2380, 0x3A0E6390, 0x3B0EA3A0, 0x3C0EE3B0, 0x3D0F23C0, 0x3E0F63D0, 0x3F0FA3E0, 0x3FFFE3F0,}},
    {
     .bv = 1500,
     .packed_lut = {
        0x01002000, 0x02006010, 0x0300A020, 0x0400E030, 0x05012040, 0x06016050, 0x0701A060, 0x0801E070, 
        0x09022080, 0x0A026090, 0x0B02A0A0, 0x0C02E0B0, 0x0D0320C0, 0x0E0360D0, 0x0F03A0E0, 0x1003E0F0, 
        0x11042100, 0x12046110, 0x1304A120, 0x1404E130, 0x15052140, 0x16056150, 0x1705A160, 0x1805E170, 
        0x19062180, 0x1A066190, 0x1B06A1A0, 0x1C06E1B0, 0x1D0721C0, 0x1E0761D0, 0x1F07A1E0, 0x2007E1F0, 
        0x21082200, 0x22086210, 0x2308A220, 0x2408E230, 0x25092240, 0x26096250, 0x2709A260, 0x2809E270, 
        0x290A2280, 0x2A0A6290, 0x2B0AA2A0, 0x2C0AE2B0, 0x2D0B22C0, 0x2E0B62D0, 0x2F0BA2E0, 0x300BE2F0, 
        0x310C2300, 0x320C6310, 0x330CA320, 0x340CE330, 0x350D2340, 0x360D6350, 0x370DA360, 0x380DE370, 
        0x390E2380, 0x3A0E6390, 0x3B0EA3A0, 0x3C0EE3B0, 0x3D0F23C0, 0x3E0F63D0, 0x3F0FA3E0, 0x3FFFE3F0, }},
    {// 线性曲线，BV=500
     .bv = 2000,
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

const _Sensor_WDR gc20C3_wdr_init = {
    .wdr_bv           = {791, 1599, 3534,  7000, 10000, 14000, 28000, 56000},
    .max_ns_slope     = {1.0,  1.0,  1.0,   1.0,  1.25,   1.5,   2.0,   3.0},
    .max_shadow_slope = {1.0,  1.0,  1.0,   1.0,   1.0,  1.25,   1.5,   1.5},
};

uint8 gc20C3_regValTable[13][7] = {   
  //   0d04  0d05  0e36  0e39   04a8   04a9   0052      |  实际倍数   | Again dB|
     { 0x00, 0x01, 0x15, 0x15,  0x01,  0x00,  0x64},    //|  X1        | 0.00    |
     { 0x00, 0x02, 0x15, 0x15,  0x01,  0x1b,  0x64},    //|  X1.43     | 3.09    |
     { 0x00, 0x03, 0x16, 0x16,  0x02,  0x00,  0x64},    //|  X2.01     | 6.05    |
     { 0x00, 0x04, 0x17, 0x17,  0x02,  0x37,  0x64},    //|  X2.87     | 9.17    |
     { 0x00, 0x05, 0x17, 0x17,  0x04,  0x02,  0x84},    //|  X4.03     | 12.11   |
     { 0x00, 0x06, 0x18, 0x18,  0x05,  0x32,  0x84},    //|  X5.79     | 15.25   |
     { 0x00, 0x07, 0x19, 0x19,  0x08,  0x05,  0x84},    //|  X8.09     | 18.16   |
     { 0x04, 0x97, 0x1a, 0x1a,  0x0b,  0x10,  0x84},    //|  X11.25    | 21.03   |
     { 0x08, 0x07, 0x1b, 0x1b,  0x10,  0x04,  0x84},    //|  X16.07    | 24.12   |
     { 0x0a, 0x4f, 0x1c, 0x1c,  0x16,  0x22,  0x88},    //|  X22.54    | 27.06   |
     { 0x0c, 0x07, 0x1d, 0x1d,  0x20,  0x06,  0x88},    //|  X32.10    | 30.13   |
     { 0x0d, 0x2f, 0x1e, 0x1e,  0x2d,  0x10,  0x88},    //|  X45.26    | 33.11   |
     { 0x0e, 0x07, 0x20, 0x20,  0x3f,  0x23,  0x72},    //|  X63.56    | 36.06   |
};

uint32 gc20C3_gainLevelTable[14] = {	
	64  ,
	91  ,
	128 ,
	183 ,
	257 ,
	370 ,
	517 ,
	720 ,
	1028,
	1442,
	2054,
	2896,
	4067,
	0xffffffff,
};



void GC20C3_ae_adjust(struct isp_exposure_opt *p_cfg)
{
    uint8 i;
    uint8 index         = 0;
    int   gc20C3_total  = sizeof(gc20C3_gainLevelTable) / sizeof(uint32);
    uint32 tol_dig_gain = 0;
    uint32 gain         = (p_cfg->analog_gain) >> 2; 
    uint8  *addr        = (uint8 *)p_cfg->data.addr;

    for (i = 0; i < gc20C3_total; i++)
    {
        if ((gc20C3_gainLevelTable[i] <= gain) && (gain < gc20C3_gainLevelTable[i + 1]))
            break;
    }

    tol_dig_gain = gain * 1024 / gc20C3_gainLevelTable[i];

    addr[index++] = 0x0d;//0
    addr[index++] = 0x04;
    addr[index++] = gc20C3_regValTable[i][0];
    addr[index++] = 0x0d;
    addr[index++] = 0x05;
    addr[index++] = gc20C3_regValTable[i][1];
    addr[index++] = 0x0e;
    addr[index++] = 0x36;
    addr[index++] = gc20C3_regValTable[i][2];
    addr[index++] = 0x0e;
    addr[index++] = 0x39;
    addr[index++] = gc20C3_regValTable[i][3];
    addr[index++] = 0x04;
    addr[index++] = 0xa8;
    addr[index++] = gc20C3_regValTable[i][4];
    addr[index++] = 0x04;
    addr[index++] = 0xa9;
    addr[index++] = gc20C3_regValTable[i][5];
    addr[index++] = 0x00;
    addr[index++] = 0x52;
    addr[index++] = gc20C3_regValTable[i][6];
	
    addr[index++] = 0x04;
    addr[index++] = 0x74;
	addr[index++] = (uint8)((tol_dig_gain>>8)&0xff);

    addr[index++] = 0x04;
    addr[index++] = 0x75;
    addr[index++] = (uint8)(tol_dig_gain&0xff);

    addr[index++] = 0x02;
    addr[index++] = 0x02;
    addr[index++] = (uint8)(p_cfg->exposure_line >> 8);
    addr[index++] = 0x02;
    addr[index++] = 0x03;
    addr[index++] = (uint8)(p_cfg->exposure_line & 0xff);
	
    p_cfg->data.size = index;
    p_cfg->cmd_len   = 2+1;
}

const _Sensor_ISP_Init gc20C3_isp_init = 
{
    .type         = ISP_INPUT_DAT_SRC_MIPI0,
    .pixel_h      = 1080,
    .pixel_w      = 1920,
    .bayer_patten = ISP_BAYER_FORMAT_GRBG,
    .input_format = ISP_INPUT_DAT_FORMAT_RAW10,
    .adjust_func  = (isp_ae_func     )GC20C3_ae_adjust,
    .p_blc        = (_Sensor_BLC    *)&gc20C3_blc_init,
    .p_ccm        = (_Sensor_CCM    *)&gc20C3_ccm_init,
    .p_awb        = (_Sensor_AWB    *)&gc20C3_awb_init,
    .p_ae         = (_Sensor_AE     *)&gc20C3_ae_init,
    .p_dpc        = (_Sensor_DPC    *)&gc20C3_dpc_init,
	.p_csc        = (_Sensor_CSC    *)&gc20C3_csc_init,
	.p_gic        = (_Sensor_GIC    *)&gc20C3_gic_init,
    .p_csupp      = (_Sensor_CSUPP  *)&gc20C3_csupp_init,
    .p_sharp      = (_Sensor_SHARP  *)&gc20C3_sharp_init,
    .p_yuvnr      = (_Sensor_YUVNR  *)&gc20C3_yuvnr_init,    
    .p_colenh     = (_Sensor_COLENH *)&gc20C3_colenh_init,
    .p_bv2nr      = (_Sensor_BV2NR  *)&gc20C3_bv2nr_init,    
    .p_lsc        = (_Sensor_LSC    *)&gc20C3_lsc_init,
    .p_lhs        = (_Sensor_LHS    *)gc20C3_lhs_map,
    .p_ygamma     = (_Sensor_YGAMMA *)gc20C3_ygamma_tbl,
    .p_wdr        = (_Sensor_WDR    *)&gc20C3_wdr_init,

};

SENSOR_OP_SECTION const _Sensor_Adpt_ gc20C3_cmd = 
{
	.typ = 1, //YUV
	.pixelw = 1920,
	.pixelh= 1080,
	.hsyn = 1,
	.vsyn = 1,
	.rduline = 0,//
	.rawwide = 1,//10bit
	.colrarray = 0,//0:_RGRG_ 1:_GRGR_,2:_BGBG_,3:_GBGB_
	.init = (uint8 *)GC20C3InitTable,
    .init_len = sizeof(GC20C3InitTable),
    .mipi_lane_num = 2,
    .vts_reg = {0x0340,0x0341},
    .vts_reg_num = 2,
	.rotate_adapt = {0},
	//.hvb_adapt = {0x6a,0x12,0x6a,0x12},
	. mclk = 24000000,
	.p_fun_adapt = {NULL,NULL,NULL},
    .sensor_isp = (_Sensor_ISP_Init *)&gc20C3_isp_init,
};

const _Sensor_Ident_ gc20C3_init =
{
	0xc3, 0x62, 0x63, 0x02, 0x01, 0x03f1
};

#endif



