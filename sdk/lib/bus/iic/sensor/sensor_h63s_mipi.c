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

#if DEV_SENSOR_H63S


SENSOR_INIT_SECTION const unsigned char H63SInitTable[CMOS_INIT_LEN]= 
{	
	0x12,0x40,
	0x48,0xA2,
	0x48,0x22,
	0x0E,0x11,
	0x0F,0x2C,
	0x10,0x24,
	0x11,0x80,
	0x57,0x60,
	0x58,0x18,
	0x5F,0x01,
	0x46,0x18,
	0xB6,0x00,
	0x0D,0xF0,
	0x20,0x80,
	0x21,0x04,
	0x22,0xEE,
	0x23,0x02,
	0x24,0x80,
	0x25,0xD0,
	0x26,0x22,
	0x27,0x61,
	0x28,0x15,
	0x29,0x03,
	0x2A,0x52,
	0x2B,0x13,
	0x2C,0x00,
	0x2D,0x00,
	0x2E,0xBA,
	0x2F,0x00,
	0x41,0x84,
	0x42,0x02,
	0x47,0x42,
	0x76,0x40,
	0x77,0x06,
	0x80,0x01,
	0xAF,0x22,
	0xAB,0x00,
	0x1D,0x00,
	0x1E,0x04,
	0x6C,0x50,
	0x08,0x00,
	0x9E,0xF0,
	0x70,0x8D,
	0x71,0x4D,
	0x72,0x6C,
	0x73,0x66,
	0x74,0x02,
	0x78,0x8D,
	0x89,0x01,
	0x6E,0x2C,
	0x6B,0x60,
	0x86,0x00,
	0x30,0x87,
	0x31,0x0A,
	0x32,0x18,
	0x33,0x10,
	0x34,0x20,
	0x35,0x20,
	0x3A,0xA0,
	0x56,0x80,
	0x59,0x2A,
	0x5A,0x88,
	0x61,0x18,
	0x64,0xC2,
	0x85,0x1A,
	0x8A,0x20,
	0x90,0x04,
	0x91,0x04,
	// 0x94,0xA0,
    0x94,0xE0,
	0x9B,0x8F,
	0xA6,0x00,
	0xA7,0x80,
	0xA9,0x48,
	0xBF,0x01,
	0x5A,0x19,
	0x5D,0x84,
	0x5E,0x90,
	0x5F,0x40,
	0x5E,0x90,
	0x5F,0x40,
	0x6F,0x40,
	0x64,0x84,
	0x65,0x90,
	0x66,0x40,
	0xBF,0x00,
	0x45,0x09,
	0x5B,0xB1,
	0x5C,0x46,
	0x5D,0x43,
	0x5E,0xC3,
	0x65,0x32,
	0x66,0xD0,
	0x67,0x32,
	0x68,0x40,
	0x69,0x70,
	0x6A,0x22,
	0x7A,0x44,
	0x8D,0x67,
	0x8F,0x90,
	0x9C,0x11,
	0xA4,0x87,
	0xA5,0xAB,
	0xB8,0x01,
	0xBF,0x01,
	0x67,0xB0,
	0xBF,0x00,
	0x13,0x81,
	0x4A,0x01,
	0xB1,0x04,
	0x50,0x02,
	0xA1,0x0F,
	0x49,0x40,
	0xBF,0x01,
	0x5C,0x00,
	0x5E,0x10,
	0x65,0x10,
	0x6F,0x53,
	0xBF,0x00,
	0xBC,0x11,
	0x82,0x00,
	0x19,0x20,
	0x12,0x00,

	0XFF,0XFF,
};
const _Sensor_CCM h63s_ccm_init = {
    480,  -95,  -64,
   -208,  456, -200,
    -16, -105,  520,
      0,    0,    0,
};

const _Sensor_BLC h63s_blc_init =
{
    62<<2, 62<<2, 62<<2, 62<<2,
};

const _Sensor_AWB h63s_awb_init = 
{
    .default_gain   = {256, 256, 256, 256},
    .awb_min_gain   = {256, 256, 256, 256},
    .awb_max_gain   = {800, 800, 800, 800},
 
    .coarse_constraint = {
        .coarse_min_bg =  60,
        .coarse_lb_bg  = 110,
        .coarse_rt_bg  = 120,
        .coarse_max_bg = 200,
        .coarse_min_rg = 100,
        .coarse_lb_rg  = 160,
        .coarse_rt_rg  = 160,
        .coarse_max_rg = 270,   
    },

    .constraint = {
        .section_num = 5,      
        .color_temp = { 6500, 5000, 4000, 2856, 0, 0, 0, 0},
        .sec_line_slope = {1.26666667, 1.33333333, 1.44897959, 1.47058824, 0, 0, 0, 0},
        .sec_line_offset = {-10.20000000, -59.33333333, -122.22448980, -233.47058824, 0, 0, 0, 0},
        .sec_line_sqrtk2add1 = {0.61964429, 0.60000000, 0.56800381, 0.56231002, 0, 0, 0, 0},
        .center_line_slope = {-0.78947368, -0.71428571, -0.68000000, 0, 0, 0, 0},
        .center_line_offset = {261.21052632, 249.85714286, 243.96000000, 0, 0, 0, 0},
        .lower_line_slope = {-4.32869028, -0.71452001, -0.28621527, 0, 0, 0, 0},
        .lower_line_offset = {693.71580677, 213.03116117, 146.66110778, 0, 0, 0, 0},
        .upper_line_slope = {-0.78978756, -0.71416035, -0.80835805, 0, 0, 0, 0},
        .upper_line_offset = {280.36601015, 268.26565555, 285.27022947, 0, 0, 0, 0},
        .corner_limit = {125.80355711, 149.15117234, 141.29466433, 168.77324148, 216.37689979, 84.73073498, 227.62310021, 101.26926502},
    },
    
};

const _Sensor_AE h63s_ae_init = 
{
    .curr_fps              = (uint32)25*256,
    .max_frame_length      = 740,//
    .min_frame_vb          = 4,//
    .max_analog_gain       = 7936,//
    .min_analog_gain       =  1<<8,
    .default_exposure_line = 720,
    .max_exposure_line     = 720,
    .min_exposure_line     = 1,
    .row_time_us           = 50,
    .expo_frame_interval   = 2,
    .to_day_bv             = 669,
    .to_night_bv           = 197,
    .dark_scene_target_lut = {50, 55},
    .dark_scene_bv_lut     = {47, 197},
    .hs_scene_limit_lut    = {55, 75},
    .hs_scene_bv_lut       = {197, 2784},
    .lowlight_lsb_bv_lut   = {72, 88, 129, 197, 347,  669, 1391, 1e30},
    .lowlight_lsb_gain_lut = {64, 64,  48,  40,  32,   24,   16,   16},  // u7.4
};

const _Sensor_DPC h63s_dpc_init = 
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

const _Sensor_GAMMA_BV h63s_gamma_map = 
{
    .bv 		= { 15000,  8000,  3000, 1200,  600,  300, 200, 100, },
    .y_alpha 	= {	  255,   255,   255,  192,  160,  128,  64,  32, },
    .rgb_alpha 	= {   255,   255,   255,  192,  160,  128,  64,  32, },
};

const _Sensor_CSC h63s_csc_init = 
{
    .rgb2yuv_gamut         = ISP_YUV_GAMUT_BT709,
    .rgb2yuv_range         = ISP_YUV_RANGE_NARROW,
    .yuv2rgb_in_gamut      = ISP_YUV_GAMUT_BT709,
    .yuv2rgb_in_range      = ISP_YUV_RANGE_NARROW,
    .yuv2rgb_out_gamut     = ISP_YUV_GAMUT_BT709,
    .yuv2rgb_out_range     = ISP_YUV_RANGE_NARROW,
    .y_gamma_alpha         = 0xff,
    .rgb_gamma_alpha       = 0xff,
	.gamma_alpha_map       = (void *)&h63s_gamma_map,
};

const _Sensor_GIC h63s_gic_init = 
{
    .w_thres  = 14,
    .w_slope  = 16,
    .w_str    = 127,
    .mu_thres = 5,
    .mu_slope = 16,
};

const _Sensor_CSUPP h63s_csupp_init = {
// uint8  U_luma_thr_lo, U_luma_slop_lo, U_luma_shfb_lo, U_luma_gmin_lo,
// uint8  U_luma_thr_hi, U_luma_slop_hi, U_luma_shfb_hi, U_luma_gmin_hi,
// uint8  V_luma_thr_lo, V_luma_slop_lo, V_luma_shfb_lo, V_luma_gmin_lo,
// uint8  V_luma_thr_hi, V_luma_slop_hi, V_luma_shfb_hi, V_luma_gmin_hi,
// uint8  chroma_thr_lo, chroma_slop_lo, chroma_shfb_lo, chroma_gmin_lo,
     31,   4,   0,   0,
    209,   4,   0,   0,
     31,   4,   0,   0,
    209,   4,   0,   0,
     31,   4,   0,   0,
};

const _Sensor_SHARP h63s_sharp_init = {
    .filt_alpha      = 128,
    .shrink_thr      = 0,
    .filt_clip_hi    = 127,
    .filt_clip_lo    = 127,
    .sp_thr2 		 = 25,
    .sp_thr1 	 	 = 10,
    .enha_clip_hi 	 = 64,
    .enha_clip_lo	 = 64, 
    .e1				 = 5,
	.e2				 = 10,
	.e3				 = 15,	
    .k0				 = 0,
	.k1				 = 128,
	.k2				 = 128,
	.k3				 = 128,
    .y1				 = 0,
	.y2				 = 20,
	.y3				 = 40,
    .filt_w11		 =  7,
	.filt_w12        =  9,
	.filt_w13        = 10,
    .filt_w21        =  9,
	.filt_w22        = 12,
	.filt_w23        = 13,
    .filt_w31        = 10,
	.filt_w32        = 13,
	.filt_w33        = 16,
    .filt_type       = 1,
    .filt_sbit       = 8,
    .lpf_scale		 = 1,
    .strength_lut    = {64, 128},
    .strength_bv_lut = {347, 2784},
};

const _Sensor_YUVNR h63s_yuvnr_init = {
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

const _Sensor_COLENH_BV h63s_ce_map[BV2COLENH_ARRAY_NUM] = {
    {.bv =  48664, .hue = 0, .luma = 50, .contrast = 56, .saturation = 70},
    {.bv =   5993, .hue = 0, .luma = 50, .contrast = 56, .saturation = 70},
    {.bv =   3022, .hue = 0, .luma = 50, .contrast = 56, .saturation = 70},
    {.bv =   1391, .hue = 0, .luma = 50, .contrast = 56, .saturation = 60},
    {.bv =    381, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
    {.bv =    369, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
    {.bv =    184, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
    {.bv =     90, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
};

const _Sensor_COLENH h63s_colenh_init = {
    .yuv_range     = 0, // 0: narrow range, 1: full range
    .luma          = 52, // range: 0 ~ 100
    .contrast      = 52, // range: 0 ~ 100
    .saturation    = 52, // range: 0 ~ 100
    .hue           = 0, // range: -180 ~ 180
    .ce_in_ofs_y   = 128,
    .ce_in_ofs_cb  = 128,
    .ce_in_ofs_cr  = 128, // range: -128 ~ 128
    .ce_out_ofs_y  = 128, 
    .ce_out_ofs_cb = 128, 
    .ce_out_ofs_cr = 128, // range: -128 ~ 128
    .adj_by_bv_en  = 1,
    .bv2colenh_map = (void *)h63s_ce_map,
};
const _Sensor_BV2NR h63s_bv2nr_init[BV2RAWNR_ARRAY_NUM] = {
    //           ev, bnr_range_weight_index, bnr_invksigma, bnr_intensity_threshold, yuvnr_idx, csupp_idx
    {    29491,                     6 ,           511,                      63,         0,         0,             0,             0},    // 320lux
    {     3534,                     8 ,           460,                      63,         0,         0,             1,             1},    // 40lux
    {     1599,                     8 ,           350,                      63,         1,         0,             1,             1},    // 20lux
    {      791,                     16,           271,                      63,         1,         1,             2,             1},    // 10lux
    {      222,                     16,           165,                      63,         2,         1,             2,             1},    // 5p03lux
    {      115,                     16,           135,                      63,         2,         1,             2,             1},    // 2p5lux
    {       57,                     20,           101,                      63,         3,         1,             2,             1},    // 1p25lux
    {       34,                     24,            62,                      63,         4,         2,             2,             1},    // 0p62lux
    {       26,                     26,            50,                      63,         4,         2,             2,             1},    // 0p31lux
    {       21,                     28,            40,                      63,         5,         2,             2,             1},    // 0p1lux
    {       16,                     31,            25,                      63,         5,         2,             2,             1},    // 0p01lux
};
const uint32 h63s_lsc_tbl[] = {
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

const _Sensor_LSC          h63s_lsc_init = {
    .p_lsc_tbl = (uint32 *)h63s_lsc_tbl,
};

const _Sensor_LHS h63s_lhs_map[] = {
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

const _Sensor_YGAMMA h63s_ygamma_tbl[5] = {
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

const _Sensor_WDR h63s_wdr_init = {
    .wdr_bv           = {791, 1599, 3534,  7000, 10000, 14000, 28000, 56000},
    .max_ns_slope     = {1.0,  1.0,  1.0,   1.0,  1.25,   1.5,   2.0,   3.0},
    .max_shadow_slope = {1.0,  1.0,  1.0,   1.0,   1.0,  1.25,   1.5,   1.5},
};

uint32 h63s_gainLevelTable[] = {
    1024, 1088, 1152, 1216, 1280, 1344, 1408, 1472, 1536, 1600, 
    1664, 1728, 1792, 1856, 1920, 1984, 2048, 2176, 2304, 2432, 
    2560, 2688, 2816, 2944, 3072, 3200, 3328, 3456, 3584, 3712, 
    3840, 3968, 4096, 4352, 4608, 4864, 5120, 5376, 5632, 5888, 
    6144, 6400, 6656, 6912, 7168, 7424, 7680, 7936, 8192, 8704, 
    9216, 9728, 10240, 10752, 11264, 11776, 12288, 12800, 13312, 
    13824, 14336, 14848, 15360, 15872, 16384, 17408, 18432, 19456, 
    20480, 21504, 22528, 23552, 24576, 25600, 26624, 27648, 28672, 
    29696, 30720, 31744, 0xffffffff,
};


void h63s_ae_adjust(struct isp_exposure_opt *p_cfg)
{

    uint32 index        = 0;
    uint32 tol_dig_gain = 0;
    uint8  *addr        = (uint8 *)p_cfg->data.addr;
	int   h63s_total  = sizeof(h63s_gainLevelTable) / sizeof(uint32);
	uint16 gain = (p_cfg->analog_gain<<2);
    for(uint16 i=0; i<h63s_total; i++)
    {
        if(h63s_gainLevelTable[i] >= gain)
        {
            tol_dig_gain = i;
            break;
        }
    }

	addr[index++] = 0x00;
	addr[index++] = tol_dig_gain;
    addr[index++] = 0x02;
    addr[index++] = (uint8)(p_cfg->exposure_line >> 8);
    addr[index++] = 0x01;
    addr[index++] = (p_cfg->exposure_line & 0xff);
    p_cfg->data.size = index;
    p_cfg->cmd_len   = 1+1;
}

void h63s_img_opt(struct isp_sensor_opt *p_opt)
{
    uint8  *addr = (uint8 *)p_opt->data.addr;
    uint8  index = 0;
    addr[index++] = 0x12;
    addr[index++] = (p_opt->reverse_en + p_opt->mirror_en * 2) << 4;
    p_opt->data.size = index; 
    p_opt->cmd_len   = 1 + 1;   // addr length + data lengt
}

void h63s_fps_opt(struct isp_sensor_opt *p_opt)
{
    uint8  *addr        = (uint8 *)p_opt->data.addr;
    uint8  index        = 0;
    addr[index++]       = 0x23;
    addr[index++]       = p_opt->curr_length >> 8;
    addr[index++]       = 0x22;
    addr[index++]       = p_opt->curr_length & 0xff;
    p_opt->data.size    = index;
    p_opt->cmd_len      = 1+1;
}

const _Sensor_ISP_Init h63s_isp_init = 
{
    .type         = ISP_INPUT_DAT_SRC_MIPI0,
    .pixel_h      = 720,
    .pixel_w      = 1280,
    .bayer_patten = ISP_BAYER_FORMAT_BGGR,
    .input_format = ISP_INPUT_DAT_FORMAT_RAW10,
    .adjust_func  = (isp_ae_func     )h63s_ae_adjust,
    .p_blc        = (_Sensor_BLC    *)&h63s_blc_init,
    .p_ccm        = (_Sensor_CCM    *)&h63s_ccm_init,
    .p_awb        = (_Sensor_AWB    *)&h63s_awb_init,
    .p_ae         = (_Sensor_AE     *)&h63s_ae_init,
	.p_dpc        = (_Sensor_DPC    *)&h63s_dpc_init,
	.p_csc        = (_Sensor_CSC    *)&h63s_csc_init,
	.p_gic        = (_Sensor_GIC    *)&h63s_gic_init,
    .p_csupp      = (_Sensor_CSUPP  *)&h63s_csupp_init,
    .p_sharp      = (_Sensor_SHARP  *)&h63s_sharp_init,
    .p_yuvnr      = (_Sensor_YUVNR  *)&h63s_yuvnr_init,    
    .p_colenh     = (_Sensor_COLENH *)&h63s_colenh_init,	
    .p_bv2nr      = (_Sensor_BV2NR  *)&h63s_bv2nr_init,
    .p_lsc        = (_Sensor_LSC    *)&h63s_lsc_init,
    .p_lhs        = (_Sensor_LHS    *)h63s_lhs_map,
    .p_ygamma     = (_Sensor_YGAMMA *)h63s_ygamma_tbl,
	.p_wdr        = (_Sensor_WDR    *)&h63s_wdr_init,
    .fps_opt      = (sensor_fps_opt  )h63s_fps_opt,
    .img_opt      = (sensor_img_opt  )h63s_img_opt,

};


SENSOR_OP_SECTION const _Sensor_Adpt_ h63s_cmd= 
{	
	.typ = 1, //YUV
	.pixelw = 1280,
	.pixelh= 720,
	.hsyn = 1,
	.vsyn = 0,
	.rduline = 0,//
	.rawwide = 1,//10bit
	.colrarray = 2,//0:_RGRG_ 1:_GRGR_,2:_BGBG_,3:_GBGB_
	.init = (uint8 *)H63SInitTable,
//	.init_len = sizeof(H63SInitTable),
	.mipi_lane_num = 1,
	.rotate_adapt = {0},
	//.hvb_adapt = {0x80,0x0a,0x80,0x0a},
	. mclk = 24000000,
	.p_fun_adapt = {NULL,NULL,NULL},
    .sensor_isp = (_Sensor_ISP_Init*)&h63s_isp_init,
};

const _Sensor_Ident_ h63s_init=
{
	0x73,0x80,0x81,0x01,0x01,0x0b
};

#endif

