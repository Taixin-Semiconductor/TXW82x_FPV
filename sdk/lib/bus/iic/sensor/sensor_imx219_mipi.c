#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/isp.h"

#if DEV_SENSOR_IMX912

#define IMX219MIPI_MaxGainIndex (98)



#define IMA_H (1224-8)
#define IMA_W (1632-16)


SENSOR_INIT_SECTION const unsigned char IMX219InitTable[]= 
{
    0x01,0x00,0x00,
    0x30,0xEB,0x05,
    0x30,0xEB,0x0C,
    0x30,0x0A,0xFF,
    0x30,0x0B,0xFF,
    0x30,0xEB,0x05,
    0x30,0xEB,0x09,		  
    0x01,0x14,0x01,
    0x01,0x28,0x00,
    0x01,0x2A,0x18,
    0x01,0x2B,0x00,

    0x01,0x60,0x05, //framelength
    0x01,0x61,0x65,
    0x01,0x62,0x13,	//linelength
    0x01,0x63,0x78,

    0x01,0x64,0x00, //x_sta_h
    0x01,0x65,0x00, 
    0x01,0x66,0x0C, //x_end_h
    0x01,0x67,0xBF, 
    0x01,0x68,0x00, //y_sta_h
    0x01,0x69,0x00, 
    0x01,0x6A,0x09, //y_end_h
    0x01,0x6B,0x8F, 
    0x01,0x6C,0x06, //x_size_h
    0x01,0x6D,0x60, 
    0x01,0x6E,0x04, //y_size_h
    0x01,0x6F,0xc8, 

    0x01,0x70,0x01,
    0x01,0x71,0x01,
    0x01,0x72,0x00,//03  //edit by wming for mirror
    0x01,0x74,0x01,
    0x01,0x75,0x01,
    0x01,0x8C,0x0A,
    0x01,0x8D,0x0A,
    0x03,0x01,0x05,
    0x03,0x03,0x01,
    0x03,0x04,0x03,
    0x03,0x05,0x03,
    0x03,0x06,0x00,
    0x03,0x07,0x36,
    0x03,0x09,0x0A,
    0x03,0x0B,0x01,
    0x03,0x0C,0x00,
    0x03,0x0D,0x6c,
    0x45,0x5E,0x00,
    0x47,0x1E,0x4B,
    0x47,0x67,0x0F,
    0x47,0x50,0x14,
    0x45,0x40,0x00,
    0x47,0xB4,0x14,
    0x47,0x13,0x30,
    0x47,0x8B,0x10,
    0x47,0x8F,0x10,
    0x47,0x93,0x10,
    0x47,0x97,0x0E,
    0x47,0x9B,0x0E,
    0x01,0x00,0x01, 

    0xff,0xff,0xff,
};

const _Sensor_CCM imx219_ccm_init = {
    // 347,  -148,  -136,
	// 219,  631, -224,
    // -310, -227,  616,

//    479,   -72,	    -47,
//    -126,	488,	-184,
//    -98, 	-160,	486,

	364,-72,-59,
	85,516,-2,
	-193,-188,317,
    0,      0,      0,

};

const _Sensor_BLC imx219_blc_init =
{
    256, 256, 256, 256,
};

const _Sensor_AWB imx219_awb_init = 
{
    .default_gain   = {427,256,256,447},
    .awb_min_gain   = {300,256,256,400},
    .awb_max_gain   = {550,256,256,720},
 
    .coarse_constraint = {
        .coarse_min_bg =  90,
        .coarse_lb_bg =  130,
        .coarse_rt_bg =  160,
        .coarse_max_bg = 200,
        .coarse_min_rg = 100,
        .coarse_lb_rg =  150,
        .coarse_rt_rg =  170,
        .coarse_max_rg = 230, 
    },

	.constraint = {
		.section_num = 4,


		.color_temp =       {        6500,        5000,        4000,        3000,           0,           0,           0,           0},
		.sec_line_slope =   {  2.37684441,  2.26592493,  1.93668795,  3.02123332,           0,           0,           0,           0},
		.sec_line_offset = {-162.27987671,-191.62303162,-180.4246521,-470.06982422,           0,           0,           0,           0},
		.sec_line_sqrtk2add1 = {  0.38780117,  0.40375081,  0.45879477,  0.31422547,           0,           0,           0,           0},
		.center_line_slope = { -0.52941179, -0.53846157, -0.64516127,           0,           0,           0,           0},
		.center_line_offset = {235.58824158,         237,255.03225708,           0,           0,           0,           0},
		.lower_line_slope = { -0.49800548, -0.60546982, -0.46480513,           0,           0,           0,           0},
		.lower_line_offset = {  206.881073,222.37530518,200.08726501,           0,           0,           0,           0},
		.upper_line_slope = { -0.36376053, -0.61797953, -0.52290577,           0,           0,           0,           0},
		.upper_line_offset = {234.91918945,276.15420532,259.16229248,           0,           0,           0,           0},
		.corner_limit = { 128.4105072,142.93193054,192.24029541,110.73298645, 144.9311676,182.19895935,205.75720215,151.57067871},
	},
    
};

const _Sensor_AE imx219_ae_init = 
{
    .curr_fps              = 20*256,
    .max_frame_length      = 1381,//
    .min_frame_vb          = 4,//
    .max_analog_gain       = 168<<8,//
    .min_analog_gain       = 1<<8,
    .default_exposure_line = 720,
    .max_exposure_line     = 1377,
    .min_exposure_line     = 4,
    .row_time_us           = 28,
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

const _Sensor_DPC imx219_dpc_init = 
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

const _Sensor_GAMMA_BV imx219_gamma_map = 
{
    .bv = {
        29491, 3534, 1599, 347, 222, 115, 57, 34,
    },

    .y_alpha = {
        255,  255, 192, 192, 128, 128, 64, 64,                         
    },

    .rgb_alpha = {
        255,  255, 192, 192, 128, 128, 64, 64,     
    },
};

const _Sensor_CSC imx219_csc_init = 
{
    .rgb2yuv_gamut         = ISP_YUV_GAMUT_BT709,
    .rgb2yuv_range         = ISP_YUV_RANGE_NARROW,
    .yuv2rgb_in_gamut      = ISP_YUV_GAMUT_BT709,
    .yuv2rgb_in_range      = ISP_YUV_RANGE_NARROW,
    .yuv2rgb_out_gamut     = ISP_YUV_GAMUT_BT709,
    .yuv2rgb_out_range     = ISP_YUV_RANGE_NARROW,
    .y_gamma_alpha         = 0xff,
    .rgb_gamma_alpha       = 0xff,
	.gamma_alpha_map       = (void *)&imx219_gamma_map,
};

const _Sensor_GIC imx219_gic_init = 
{
    .w_thres  = 14,
    .w_slope  = 16,
    .w_str    = 127,
    .mu_thres = 5,
    .mu_slope = 16,
};

const _Sensor_CSUPP imx219_csupp_init = {
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

const _Sensor_SHARP imx219_sharp_init = {
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

const _Sensor_YUVNR imx219_yuvnr_init = {
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

const _Sensor_COLENH_BV imx219_ce_map[BV2COLENH_ARRAY_NUM] = {
    {.bv =  48664, .hue = 0, .luma = 50, .contrast = 56, .saturation = 70},
    {.bv =   5993, .hue = 0, .luma = 50, .contrast = 56, .saturation = 70},
    {.bv =   3022, .hue = 0, .luma = 50, .contrast = 56, .saturation = 70},
    {.bv =   1391, .hue = 0, .luma = 50, .contrast = 56, .saturation = 60},
    {.bv =    381, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
    {.bv =    369, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
    {.bv =    184, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
    {.bv =     90, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
};

const _Sensor_COLENH imx219_colenh_init = {
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
    .bv2colenh_map = (void *)imx219_ce_map,
};

const _Sensor_BV2NR imx219_bv2nr_init[BV2RAWNR_ARRAY_NUM] = {
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

const _Sensor_WDR imx219_wdr_init = {
    .wdr_bv           = {791, 1599, 3534,  7000, 10000, 14000, 28000, 56000},
    .max_ns_slope     = {1.0,  1.0,  1.0,   1.0,  1.25,   1.5,   2.0,   3.0},
    .max_shadow_slope = {1.0,  1.0,  1.0,   1.0,   1.0,  1.25,   1.5,   1.5},
};

const uint32 imx219_lsc_tbl[] = {
0x000B5726, 0x0008B272, 0x000701EB, 0x00062599, 0x00061981, 0x0006E998, 0x000851DD, 0x000AF65B, 0x0000031D, 0x000AB304, 0x00082650, 0x00067DCC, 0x0005A57D, 0x00059D64, 0x0006717B, 0x0007DDC3, 
0x000A6A41, 0x000002F6, 0x000A2AE1, 0x0007AA33, 0x0005F5AE, 0x0005295E, 0x00052945, 0x0005ED5B, 0x000785A9, 0x0009FE27, 0x000002D6, 0x0009CAC4, 0x00073A1A, 0x00058D90, 0x0004C543, 0x0004C52B, 
0x00058942, 0x0007218D, 0x0009B613, 0x000002C3, 0x00098AB5, 0x0006E605, 0x0005357A, 0x0004752F, 0x00047518, 0x0005392F, 0x0006D579, 0x00096E00, 0x000002B4, 0x00094EA3, 0x0006A5F6, 0x0004FD6C, 
0x00044122, 0x0004410B, 0x00050122, 0x00069D6C, 0x000949F4, 0x000002A9, 0x0009369C, 0x000689ED, 0x0004E164, 0x0004211A, 0x00042503, 0x0004E51B, 0x00068565, 0x00093DEF, 0x000002A9, 0x00092E9A, 
0x000675EA, 0x0004D160, 0x00041516, 0x00041D00, 0x0004D918, 0x00068162, 0x000945EE, 0x000002AC, 0x00093AA4, 0x000681ED, 0x0004E163, 0x00042519, 0x00042503, 0x0004E91A, 0x00069567, 0x000955F4, 
0x000002B4, 0x000976B4, 0x0006ADFC, 0x00050D6E, 0x00044D23, 0x0004510D, 0x00051526, 0x0006D173, 0x0009AA04, 0x000002C8, 0x0009C6CA, 0x00070610, 0x00054D81, 0x00048934, 0x0004911D, 0x00055D37, 
0x00071185, 0x000A0A1A, 0x000002DE, 0x000A6AED, 0x00078235, 0x0005BD9D, 0x0004F14F, 0x0004F537, 0x0005D151, 0x000799A4, 0x000AA645, 0x00000305, 0x000B171D, 0x00082662, 0x000649C5, 0x00057D72, 
0x00058158, 0x00066575, 0x000845CD, 0x000B5E70, 0x00000338, 0x000BF757, 0x0008FE98, 0x000701F8, 0x0006259D, 0x00063584, 0x000725A2, 0x00092602, 0x000C32A1, 0x0000036E, 0x000CEB8E, 0x0009E6D5, 
0x0007E234, 0x0006F1D2, 0x0006FDB7, 0x000805D8, 0x000A1E3D, 0x000D3AE1, 0x000003B3, 0x000E27E1, 0x000B2B22, 0x00090A7C, 0x0007FA18, 0x00080DF8, 0x00092E1D, 0x000B4E88, 0x000E6730, 0x000003FF, 
0x000F2BFF, 0x000C2B66, 0x000A1EC0, 0x00090A5B, 0x00091A3A, 0x000A3A63, 0x000C6ECD, 0x000F7F7A, 0x000003FF, 

0x0007DE36, 0x000631B6, 0x0005556C, 0x0004E942, 0x0004E135, 0x0005393F, 0x0005ED5F, 0x000771A1, 0x0000021E, 0x00077A19, 0x0005E9A4, 0x00051D5D, 0x0004B136, 0x0004AD28, 0x00051534, 0x0005B956, 
0x00072598, 0x00000206, 0x000725FE, 0x0005B996, 0x0004E150, 0x00047928, 0x0004791C, 0x0004DD26, 0x0005A54D, 0x0006E58B, 0x000001EE, 0x0006F1ED, 0x00058D8A, 0x0004B544, 0x0004511D, 0x00045111, 
0x0004B51D, 0x00058543, 0x0006C585, 0x000001E3, 0x0006D1E0, 0x00056D82, 0x0004913C, 0x00043115, 0x00043109, 0x00049515, 0x0005653B, 0x0006B17F, 0x000001DB, 0x0006BDDD, 0x0005597E, 0x00048137, 
0x00041D10, 0x00041903, 0x00048110, 0x00055937, 0x0006A57C, 0x000001D6, 0x0006BDD9, 0x0005517C, 0x00047535, 0x0004110E, 0x00041101, 0x00047D0E, 0x00055D37, 0x0006B17D, 0x000001DA, 0x0006B5D7, 
0x00054D7B, 0x00047134, 0x00040D0D, 0x00040D00, 0x0004790D, 0x00055D37, 0x0006B97F, 0x000001DC, 0x0006C5DE, 0x0005597F, 0x00048137, 0x0004190F, 0x00041903, 0x00048910, 0x0005713B, 0x0006D184, 
0x000001E3, 0x0006EDEA, 0x00057D89, 0x00049D3F, 0x00043517, 0x0004350A, 0x0004A917, 0x00059543, 0x0006FD8E, 0x000001ED, 0x00071DF6, 0x0005A994, 0x0004C549, 0x00045D21, 0x00045D15, 0x0004D122, 
0x0005BD4D, 0x00072D9A, 0x000001F9, 0x00076E0C, 0x0005F1A6, 0x00050559, 0x00049930, 0x00049D23, 0x00051532, 0x0006095F, 0x000781B0, 0x0000020F, 0x0007D627, 0x000651C1, 0x00055D71, 0x0004F146, 
0x0004F539, 0x00056D48, 0x00066D77, 0x0007E9C9, 0x0000022C, 0x00084E4A, 0x0006C1DC, 0x0005BD8B, 0x0005515F, 0x00055551, 0x0005D561, 0x0006D992, 0x000859E2, 0x0000024F, 0x0008DE72, 0x00073DFC, 
0x000639AC, 0x0005C17C, 0x0005CD6D, 0x0006517F, 0x000759B1, 0x0008F203, 0x0000027B, 0x0009AAA8, 0x0007DE27, 0x0006D9D2, 0x000659A2, 0x00066192, 0x0006E9A5, 0x0007F9D9, 0x0009B22D, 0x000002B2, 
0x000A5ADE, 0x00087A51, 0x000761F6, 0x0006E5C5, 0x0006E9B5, 0x00077DC9, 0x000885FC, 0x000A6A55, 0x000002E7, 

0x0007D636, 0x00062DB6, 0x0005556B, 0x0004E941, 0x0004DD35, 0x00053D3F, 0x0005ED5E, 0x00077DA1, 0x00000221, 0x00077A19, 0x0005F1A3, 0x00051D5D, 0x0004B136, 0x0004AD29, 0x00051534, 0x0005B956, 
0x00072998, 0x00000207, 0x000721FE, 0x0005B995, 0x0004E54F, 0x00047928, 0x0004791C, 0x0004DD27, 0x0005A94D, 0x0006ED8B, 0x000001EF, 0x0006EDEF, 0x00058D8A, 0x0004B545, 0x0004511E, 0x00045111, 
0x0004B51E, 0x00058544, 0x0006C986, 0x000001E3, 0x0006D1E2, 0x00056D82, 0x0004953C, 0x00042D15, 0x00042D08, 0x00049515, 0x0005653B, 0x0006B17E, 0x000001DE, 0x0006BDDD, 0x0005597D, 0x00048138, 
0x00041D10, 0x00041903, 0x00048510, 0x00055D38, 0x0006AD7E, 0x000001DA, 0x0006B9DA, 0x0005557D, 0x00047935, 0x0004110E, 0x00041101, 0x0004810F, 0x00055D38, 0x0006B97E, 0x000001DD, 0x0006B5DA, 
0x00054D7C, 0x00047935, 0x00040D0D, 0x00041100, 0x00047D0D, 0x00056138, 0x0006C181, 0x000001DF, 0x0006C9E0, 0x00055D80, 0x00048538, 0x00041910, 0x00041D03, 0x00048D11, 0x0005713C, 0x0006DD85, 
0x000001E6, 0x0006F1EC, 0x00058189, 0x0004A13F, 0x00043917, 0x0004390B, 0x0004AD18, 0x00059944, 0x0007018F, 0x000001F1, 0x000721F7, 0x0005AD94, 0x0004C94A, 0x00045D21, 0x00046114, 0x0004D522, 
0x0005C54E, 0x0007399C, 0x000001FF, 0x00076A0C, 0x0005F1A7, 0x0005095A, 0x00049930, 0x0004A123, 0x00051532, 0x0006095F, 0x00078DB2, 0x00000213, 0x0007DA29, 0x000655C1, 0x00055D71, 0x0004F146, 
0x0004F538, 0x00057148, 0x00066D77, 0x0007F9CA, 0x00000231, 0x00084A4C, 0x0006C1DD, 0x0005BD8B, 0x0005515F, 0x00055950, 0x0005D561, 0x0006DD93, 0x000861E4, 0x00000253, 0x0008EA76, 0x000741FD, 
0x000641AE, 0x0005C57D, 0x0005CD6E, 0x00065580, 0x000761B2, 0x00090606, 0x0000027E, 0x0009AAAA, 0x0007E22A, 0x0006D9D4, 0x000659A2, 0x00066191, 0x0006F1A6, 0x000801D9, 0x0009BA2F, 0x000002B7, 
0x000A6AE3, 0x00087A53, 0x00076DF8, 0x0006EDC6, 0x0006EDB6, 0x000781CB, 0x000891FE, 0x000A7259, 0x000002EB, 

0x0008224B, 0x00064DBF, 0x00056D73, 0x00050148, 0x0004F53C, 0x00055945, 0x0005FD66, 0x000799A6, 0x0000022D, 0x0007AE29, 0x00060DAD, 0x00053563, 0x0004C93C, 0x0004C52F, 0x00052D3A, 0x0005D15C, 
0x0007499E, 0x00000211, 0x00074E0C, 0x0005D19D, 0x0004F556, 0x00048D2E, 0x00048D21, 0x0004ED2A, 0x0005B551, 0x0007018F, 0x000001F6, 0x00070DF7, 0x0005A190, 0x0004C949, 0x00046522, 0x00046116, 
0x0004C922, 0x00058D47, 0x0006D587, 0x000001E7, 0x0006E9EA, 0x00057985, 0x0004A13F, 0x00043D19, 0x0004390C, 0x0004A119, 0x00056D3D, 0x0006B17F, 0x000001DE, 0x0006D1E3, 0x00056580, 0x0004893A, 
0x00042913, 0x00042506, 0x00048D12, 0x00055D39, 0x0006A97D, 0x000001DA, 0x0006C9DE, 0x0005557D, 0x00048135, 0x00041910, 0x00041903, 0x00048510, 0x00055938, 0x0006AD7D, 0x000001DC, 0x0006C1E1, 
0x0005517C, 0x00047D36, 0x00040D0E, 0x00041100, 0x0004850F, 0x00056138, 0x0006B97E, 0x000001DB, 0x0006D1E4, 0x00055981, 0x00048938, 0x00041D11, 0x00041D03, 0x00048D10, 0x0005693B, 0x0006C982, 
0x000001E0, 0x0006F9F0, 0x00058189, 0x00049D40, 0x00043917, 0x0004390A, 0x0004A517, 0x00058541, 0x0006F18A, 0x000001EB, 0x00072DFB, 0x0005A995, 0x0004C94A, 0x00045921, 0x00045D14, 0x0004D122, 
0x0005AD4B, 0x00072195, 0x000001F9, 0x0007760E, 0x0005E9A6, 0x00050158, 0x00049530, 0x00049923, 0x00050930, 0x0005F55A, 0x00076DAA, 0x0000020A, 0x0007E22B, 0x000645C1, 0x00055970, 0x0004ED44, 
0x0004F137, 0x00055D46, 0x00065171, 0x0007D9C1, 0x00000229, 0x00085A4D, 0x0006B5DC, 0x0005B188, 0x0005495C, 0x0005494E, 0x0005C15D, 0x0006B98A, 0x000841DB, 0x0000024B, 0x0008F27C, 0x000739FD, 
0x000631A9, 0x0005B578, 0x0005BD69, 0x00063D7A, 0x000735A9, 0x0008DDFA, 0x00000277, 0x0009BAB1, 0x0007D228, 0x0006C9CD, 0x0006459D, 0x0006418D, 0x0006C99E, 0x0007D9D0, 0x00099A26, 0x000002AD, 
0x000A7EF1, 0x00087654, 0x000755F3, 0x0006D5C2, 0x0006D5B1, 0x00075DC5, 0x00086DF6, 0x000A5652, 0x000002EA, 

};

const _Sensor_LSC          imx219_lsc_init = {
    .p_lsc_tbl = (uint32 *)imx219_lsc_tbl,
};

const _Sensor_LHS imx219_lhs_map[] = {
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
    {           160,           203,          247,               +20,                      0},  // green enhance(plants),    range:
    {           296,           318,          340,                0,                       00}   // blue enhance,     range:
};

const _Sensor_YGAMMA imx219_ygamma_tbl[NUM_CURVES] = {
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


uint16 IMX219MIPI_sensorGainMapping[IMX219MIPI_MaxGainIndex][2] ={
	{256, 0},
	{272, 12},
	{284, 23},
	{296, 33},
	{308, 42},
	{324, 52},
	{336, 59},
	{348, 66},
	{360, 73},
	{372, 79},
	{384, 85},
	{400, 91},
	{412, 96},
	{424, 101},
	{436, 105},
	{452, 110},
	{464, 114},
	{480, 118},
	{488, 121},
	{500, 125},
	{512, 128},
	{528, 131},
	{540, 134},
	{552, 137},
	{564, 139},
	{576, 142},
	{592, 145},
	{604, 147},
	{612, 149},
	{628, 151},
	{640, 153},
	{656, 156},
	{672, 158},
	{676, 159},
	{692, 161},
	{704, 163},
	{720, 165},
	{728, 166},
	{748, 168},
	{756, 169},
	{772, 171},
	{784, 172},
	{800, 174},
	{812, 175},
	{820, 176},
	{832, 177},
	{852, 179},
	{864, 180},
	{876, 181},
	{888, 182},
	{900, 183},
	{912, 184},
	{928, 185},
	{940, 186},
	{952, 187},
	{964, 188},
	{980, 189},
	{996, 190},
	{1012, 191},
	{1024, 192},
	{1040, 193},
	{1060, 194},
	{1076, 195},
	{1096, 196},
	{1112, 197},
	{1132, 198},
	{1152, 199},
	{1172, 200},
	{1192, 201},
	{1216, 202},
	{1240, 203},
	{1260, 204},
	{1288, 205},
	{1312, 206},
	{1340, 207},
	{1368, 208},
	{1396, 209},
	{1428, 210},
	{1460, 211},
	{1492, 212},
	{1524, 213},
	{1600, 215},
	{1680, 217},
	{1728, 218},
	{1772, 219},
	{1872, 221},
	{1928, 222},
	{1988, 223},
	{2048, 224},
	{2116, 225},
	{2184, 226},
	{2264, 227},
	{2340, 228},
	{2428, 229},
	{2524, 230},
	{2624, 231},
	{2732, 232},
	{0xffff, 232},
};

void imx219_ae_adjust(struct isp_exposure_opt *p_cfg)
{

    uint32 index        = 0;
    uint32 imx219_again = 0;
    float imx219_dgain = 0;
    uint8 upper_byte = 1;
    uint8 lower_byte = 0;
    uint8  *addr        = (uint8 *)p_cfg->data.addr;

    for (int i = 0; i < (IMX219MIPI_MaxGainIndex); i++){
        if(p_cfg->analog_gain <=IMX219MIPI_sensorGainMapping[i][0])
        {
            imx219_again = IMX219MIPI_sensorGainMapping[i][1];
            break;
        }    
    }

    
    if(p_cfg->analog_gain > 2732){
        imx219_dgain = (p_cfg->analog_gain/2732.00f);
        upper_byte = (uint8)(imx219_dgain);
		lower_byte = (uint8)((imx219_dgain-upper_byte)*256.00f);
    }
	
//	imx219_again = 232;
//	p_cfg->exposure_line = 1377;
//	upper_byte = 1;
//	lower_byte = 0;

   printf("=============qagain %d \r\n",imx219_again);
   printf("=============dgain %f \r\n",imx219_dgain);
   printf("=============upper_byte %d \r\n",upper_byte);
   printf("=============lower_byte %d \r\n",lower_byte);
   printf("=============line %d \r\n",p_cfg->exposure_line);   
   printf("=============gain %d \r\n",p_cfg->analog_gain);

	

    /*set analog gain*/
    addr[index++] = 0x01;
    addr[index++] = 0x57;
	addr[index++] = imx219_again;

	/*set digital gain upper*/
    addr[index++] = 0x01;
    addr[index++] = 0x58;
	addr[index++] = upper_byte;
	/*set digital gain lower*/
	addr[index++] = 0x01;
    addr[index++] = 0x59;
	addr[index++] = lower_byte;
	
    /*set exposure time */
    addr[index++] = 0x01;
    addr[index++] = 0x5A;
    addr[index++] = (uint8)(p_cfg->exposure_line >> 8);
    addr[index++] = 0x01;
    addr[index++] = 0x5B;
    addr[index++] = (p_cfg->exposure_line & 0xff);

    // addr[index++] = 0x01;
    // addr[index++] = 0x04;   
    // addr[index++] = 0x00;   

    p_cfg->data.size = index;
    p_cfg->cmd_len   = 2+1;
}

const _Sensor_ISP_Init imx219_isp_init = 
{
    .type         = ISP_INPUT_DAT_SRC_MIPI0,
	.pixel_w = IMA_W,
	.pixel_h= IMA_H,
	
    .bayer_patten = ISP_BAYER_FORMAT_GBRG,
    .input_format = ISP_INPUT_DAT_FORMAT_RAW10,
    .adjust_func  = (isp_ae_func     )imx219_ae_adjust,
    .p_blc        = (_Sensor_BLC    *)&imx219_blc_init,
    .p_ccm        = (_Sensor_CCM    *)&imx219_ccm_init,
    .p_awb        = (_Sensor_AWB    *)&imx219_awb_init,
    .p_ae         = (_Sensor_AE     *)&imx219_ae_init,
	.p_dpc        = (_Sensor_DPC    *)&imx219_dpc_init,
	.p_csc        = (_Sensor_CSC    *)&imx219_csc_init,
	.p_gic        = (_Sensor_GIC    *)&imx219_gic_init,
    .p_csupp      = (_Sensor_CSUPP  *)&imx219_csupp_init,
    .p_sharp      = (_Sensor_SHARP  *)&imx219_sharp_init,
    .p_yuvnr      = (_Sensor_YUVNR  *)&imx219_yuvnr_init,    
    .p_colenh     = (_Sensor_COLENH *)&imx219_colenh_init,	
    .p_bv2nr      = (_Sensor_BV2NR  *)&imx219_bv2nr_init,
    .p_lsc        = (_Sensor_LSC    *)&imx219_lsc_init,
    .p_lhs        = (_Sensor_LHS    *)imx219_lhs_map,
    .p_ygamma     = (_Sensor_YGAMMA *)imx219_ygamma_tbl,
	.p_wdr        = (_Sensor_WDR    *)&imx219_wdr_init,
};

SENSOR_OP_SECTION const _Sensor_Adpt_ imx219_cmd= 
{	
	.typ = 1, //YUV

	.pixelw = IMA_W,
	.pixelh= IMA_H,
	
	.hsyn = 1,
	.vsyn = 1,
	.rduline = 0,//
	.rawwide = 1,//10bit
	.colrarray = 2,//0:_RGRG_ 1:_GRGR_,2:_BGBG_,3:_GBGB_
	.init = (uint8 *)IMX219InitTable,
    .init_len = sizeof(IMX219InitTable),
    .mipi_lane_num = 2,
	.rotate_adapt = {0},
	.hvb_adapt = {0x80,0x0a,0x80,0x0a},
	.mclk = 24000000,
	.p_fun_adapt = {NULL,NULL,NULL},
    .sensor_isp = (_Sensor_ISP_Init *)&imx219_isp_init,
};

const _Sensor_Ident_ imx219_init=
{
    .id = 0x19,
//    .w_cmd = 0x6c,
//    .r_cmd = 0x6d,
	.w_cmd = 0x20,
	.r_cmd = 0x21,
    .addr_num = 2,
    .data_num = 1,
    .id_reg = 0x001,
};




#endif
