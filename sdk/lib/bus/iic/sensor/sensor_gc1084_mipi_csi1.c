#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/isp.h"


#if DEV_SENSOR_GC1084_CSI1

static SENSOR_INIT_SECTION const unsigned char GC1084InitTable[CMOS_INIT_LEN]=
{
    //mclk=24mhz
    //Mipi_clk=648Mbps/lane
    //rame rate=25FPS
    //window size:1280x720
    //bayer :GRBG
    0x03,0xfe,0xf0,
    0x03,0xfe,0xf0,
    0x03,0xfe,0xf0,
    0x03,0xfe,0x00,
    0x03,0xf2,0x00,
    0x03,0xf3,0x00,
    0x03,0xf4,0x36,
    0x03,0xf5,0xc0,
    0x03,0xf6,0x13,
    0x03,0xf7,0x01,
    0x03,0xf8,0x51,
    0x03,0xf9,0x21,
    0x03,0xfc,0xae,
    0x0d,0x05,0x09,//line_length
    0x0d,0x06,0xc4,
    0x0d,0x08,0x10,
    0x0d,0x0a,0x02,
    0x00,0x0c,0x03,
    0x0d,0x0d,0x02,
    0x0d,0x0e,0xd4,
    0x00,0x0f,0x05,
    0x00,0x10,0x08,
    0x00,0x17,0x08,
    0x0d,0x73,0x92,
    0x00,0x76,0x00,
    0x0d,0x76,0x00,
    0x0d,0x41,0x05,
    0x0d,0x42,0x10,  //vts=1296=25fps
    0x0d,0x7a,0x0a,
    0x00,0x6b,0x18,
    0x0d,0xb0,0x9d,
    0x0d,0xb1,0x00,
    0x0d,0xb2,0xac,
    0x0d,0xb3,0xd5,
    0x0d,0xb4,0x00,
    0x0d,0xb5,0x97,
    0x0d,0xb6,0x09,
    0x00,0xd2,0xfc,
    0x0d,0x19,0x31,
    0x0d,0x20,0x40,
    0x0d,0x25,0xcb,
    0x0d,0x27,0x03,
    0x0d,0x29,0x40,
    0x0d,0x43,0x20,
    0x00,0x58,0x60,
    0x00,0xd6,0x66,
    0x00,0xd7,0x19,
    0x00,0x93,0x02,
    0x00,0xd9,0x14,
    0x00,0xda,0xc1,
    0x0d,0x2a,0x00,
    0x0d,0x28,0x04,
    0x0d,0xc2,0x84,
    0x00,0x50,0x30,
    0x00,0x80,0x07,
    0x00,0x8c,0x05,
    0x00,0x8d,0xa8,
    0x00,0x77,0x01,
    0x00,0x78,0xee,
    0x00,0x79,0x02,
    0x00,0x67,0xc0,
    0x00,0x54,0xff,
    0x00,0x55,0x02,
    0x00,0x56,0x00,
    0x00,0x57,0x04,
    0x00,0x5a,0xff,
    0x00,0x5b,0x07,
    0x00,0xd5,0x03,
    0x01,0x02,0xa9,
    0x0d,0x03,0x02,
    0x0d,0x04,0xd0,
    0x00,0x7a,0x60,
    0x04,0xe0,0xff,
    0x04,0x14,0x75,
    0x04,0x15,0x75,
    0x04,0x16,0x75,
    0x04,0x17,0x75,
    0x01,0x22,0x00,
    0x01,0x21,0x80,
    0x04,0x28,0x10,
    0x04,0x29,0x10,
    0x04,0x2a,0x10,
    0x04,0x2b,0x10,
    0x04,0x2c,0x14,
    0x04,0x2d,0x14,
    0x04,0x2e,0x18,
    0x04,0x2f,0x18,
    0x04,0x30,0x05,
    0x04,0x31,0x05,
    0x04,0x32,0x05,
    0x04,0x33,0x05,
    0x04,0x34,0x05,
    0x04,0x35,0x05,
    0x04,0x36,0x05,
    0x04,0x37,0x05,
    0x01,0x53,0x00,
    0x01,0x90,0x01,
    0x01,0x92,0x02,
    0x01,0x94,0x04,
    0x01,0x95,0x02,
    0x01,0x96,0xd0,
    0x01,0x97,0x05,
    0x01,0x98,0x00,
    0x02,0x01,0x23,
    0x02,0x02,0x53,
    0x02,0x03,0xce,
    0x02,0x08,0x39,
    0x02,0x12,0x06,
    0x02,0x13,0x40,
    0x02,0x15,0x12,
    0x02,0x29,0x05,
    0x02,0x3e,0x98,
    0x03,0x1e,0x3e,
    0x00,0x15,0x03,
    0x0d,0x15,0x03,
	0x02,0x15,0x11, //[1:0],0x1:no data gate clk	   
    0xff,0xff,0xff,
};

static SENSOR_INIT_SECTION const unsigned char initTable_slave_15fps[CMOS_INIT_LEN] ={
    /* 15 fps */
    0x0d,0x41,0x08,//frame_length 
    0x0d,0x42,0x70,  

    /*slave mode*/
    0x00,0x68,0x93,
    0x00,0x69,0x00,//[5] fsync_out_polarity
    0x0d,0x67,0x00,
    0x0d,0x69,0x30,
    0x0d,0x6a,0x08,
    0x0d,0x6c,0x00,
    0x0d,0x6d,0x13,
    0x0d,0x6e,0x00,
    0x0d,0x6f,0x04,
    0x0d,0x70,0x00,
    0x0d,0x71,0x12,
    0x0d,0x6b,0x70,

    0xff,0xff,0xff,
};

static SENSOR_INIT_SECTION const unsigned char initTable_slave_7fps[CMOS_INIT_LEN] ={
    0x0d,0x41,0x12,//frame_length 
    0x0d,0x42,0x14,  
 
    /*slave mode*/
    0x00,0x68,0x93,
    0x00,0x69,0x00,//[5] fsync_out_polarity
    0x0d,0x67,0x00,
    0x0d,0x69,0x30,
    0x0d,0x6a,0x08,
    0x0d,0x6c,0x00,
    0x0d,0x6d,0x13,
    0x0d,0x6e,0x00,
    0x0d,0x6f,0x04,
    0x0d,0x70,0x00,
    0x0d,0x71,0x12,
    0x0d,0x6b,0x70,

    0xff,0xff,0xff,
};

static SENSOR_INIT_SECTION const unsigned char gc1084_stop_stream[]=
{
    0x03,0xfe,0xf0,
	0xff,0xff,0xff,
};

static const _Sensor_CCM gc1084_ccm_init = {
    // 5000k gamma2.2
    480,  -95,  -64,
   -208,  456, -200,
    -16, -105,  520,
      0,    0,    0,
};

static const _Sensor_BLC gc1084_blc_init =
{
    // 17*16, 17*16, 17*16, 17*16,
    // 17*16, 17*16, 17*16, 17*16,
    // 259, 259, 259, 259,
    256, 256, 256, 256,
    // 260, 260, 260, 260,
    // 260, 260, 260, 260,
    // 257, 257, 257, 257, 
    // 259, 259, 259, 259,
};

static const _Sensor_AWB gc1084_awb_init = 
{
    .default_gain   = {427,256,256,447},
    .awb_min_gain   = {260,256,256,360},
    .awb_max_gain   = {551,256,256,768},
 
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
        .color_temp = { 7500, 6500, 5000, 4000, 2856, 0, 0, 0},
        .sec_line_slope = {0.13043478,0.57894737,1.33333333,1.44897959,1.47058824, 0, 0, 0},
        .sec_line_offset = {163.17391304,80.57894737,-59.33333333,-122.22448980,-233.47058824, 0, 0, 0},
        .sec_line_sqrtk2add1 = {0.99160041,0.86542629,0.60000000,0.56800381,0.56231002, 0, 0, 0},
        .center_line_slope = {-7.66666667,-0.78947368,-0.71428571,-0.68000000, 0, 0, 0},
        .center_line_offset = {1169.00000000,261.21052632,249.85714286,243.96000000, 0, 0, 0},
        .lower_line_slope = {14.39078405,-2.25188242,-0.71452001,-0.28621527, 0, 0, 0},
        .lower_line_offset = {-1563.28659702,417.50036139,213.03116117,146.66110778, 0, 0, 0},
        .upper_line_slope = {-2.76616134,-0.70016780,-0.71416035,-0.80835805, 0, 0, 0},
        .upper_line_offset = {565.55747149,266.02684729,268.26565555,285.27022947, 0, 0, 0},
        .corner_limit = {121.06719671,178.96528653,138.91600411,181.29339184,216.37689979,84.73073498,227.62310021,101.26926502},
    },
};

static const _Sensor_AE gc1084_ae_init = 
{
    .max_frame_length      = 2160,               //default value , Automatically read the frame length
    .curr_fps              = (uint32)(15*256),   //default value, During initialization, calculate the frame rate
    .default_exposure_line = 2150,               //default value, Equal to max_frame_length - min_frame_vb
    .max_exposure_line     = 2150,               //default value, Equal to max_frame_length - min_frame_vb
    .to_day_bv             = 1528,               //not use
    .to_night_bv           = 369,                //not use
    
    .min_frame_vb          = 16,
    .max_analog_gain       = (48<<8),
    .min_analog_gain       = 1<<8,
    .min_exposure_line     = 16,
    .row_time_us           = 30.87,
    .expo_frame_interval   = 3,    
    .dark_scene_target_lut = {50, 55},
    .dark_scene_bv_lut     = {47, 363},
    .hs_scene_limit_lut    = {55, 75},
    .hs_scene_bv_lut       = {363, 3022},
    .lowlight_lsb_bv_lut   = {47, 94, 195, 381, 781, 1636, 3225, 1e30},
    .lowlight_lsb_gain_lut = {16, 16,  16,  16,  16,   16,   16,   16},  // u7.4
};

static const _Sensor_DPC gc1084_dpc_init = 
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

static const _Sensor_GAMMA_BV gc1084_gamma_map = 
{
    .adj_by_bv = 1,

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

static const _Sensor_CSC gc1084_csc_init = 
{
    .rgb2yuv_gamut         = ISP_YUV_GAMUT_BT709,
    .rgb2yuv_range         = ISP_YUV_RANGE_NARROW,
    .yuv2rgb_in_gamut      = ISP_YUV_GAMUT_BT709,
    .yuv2rgb_in_range      = ISP_YUV_RANGE_NARROW,
    .yuv2rgb_out_gamut     = ISP_YUV_GAMUT_BT709,
    .yuv2rgb_out_range     = ISP_YUV_RANGE_NARROW,
    .y_gamma_alpha         = 0xff,
    .rgb_gamma_alpha       = 0xff,
    .gamma_alpha_map       = (void *)&gc1084_gamma_map,
};

static const _Sensor_GIC gc1084_gic_init = 
{
    .w_thres  = 14,
    .w_slope  = 16,
    .w_str    = 127,
    .mu_thres = 5,
    .mu_slope = 16,
};

static const _Sensor_CSUPP gc1084_csupp_init = {
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

static const _Sensor_SHARP gc1084_sharp_init = {
    .filt_alpha      = 128,
    .shrink_thr      = 0  ,
    .filt_clip_hi    = 127,
    .filt_clip_lo    = 127,
    .sp_thr2 		 = 10 ,
    .sp_thr1 	 	 = 5  ,
    .enha_clip_hi 	 = 127,
    .enha_clip_lo	 = 127, 
    .e1				 = 5 ,
	.e2				 = 10,
	.e3				 = 15,	
    .k0				 = 0,
	.k1				 = 128,
	.k2				 = 128,
	.k3				 = 128,  
    .y1				 = 0,
	.y2				 = 20,
	.y3				 = 40,
    .filt_w11		 = 7,
	.filt_w12        = 9,
	.filt_w13        = 10,
    .filt_w21        = 9,
	.filt_w22        = 12,
	.filt_w23        = 13,
    .filt_w31        = 10,
	.filt_w32        = 13,
	.filt_w33        = 16,
    .filt_type       = 1,
    .filt_sbit       = 8,
    .lpf_scale		 = 1,
    .strength_lut    = { 64, 128},
    .strength_bv_lut = {369,1528},
};

static const _Sensor_YUVNR gc1084_yuvnr_init = {
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

static const _Sensor_COLENH_BV gc1084_ce_map[BV2COLENH_ARRAY_NUM] = {
    {.bv = 29491, .hue = 0, .luma = 50, .contrast = 56, .saturation = 57},
    {.bv =  3534, .hue = 0, .luma = 50, .contrast = 56, .saturation = 57},
    {.bv =  1599, .hue = 0, .luma = 50, .contrast = 56, .saturation = 57},
    {.bv =   347, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
    {.bv =   222, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
    {.bv =   115, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
    {.bv =    57, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
    {.bv =    34, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
};

static const _Sensor_COLENH gc1084_colenh_init = {
    .yuv_range     = 0,    // 0: narrow range, 1: full range
    .luma          = 50,   // range: 0 ~ 100
    .contrast      = 56,   // range: 0 ~ 100
    .saturation    = 55,   // range: 0 ~ 100
    .hue           = 0,    // range: -180 ~ 180
    .ce_in_ofs_y   = 128,
    .ce_in_ofs_cb  = 128,
    .ce_in_ofs_cr  = 128, // range: -128 ~ 128
    .ce_out_ofs_y  = 128, 
    .ce_out_ofs_cb = 128, 
    .ce_out_ofs_cr = 128, // range: -128 ~ 128
    .adj_by_bv_en  = 1,
    .bv2colenh_map = (void *)gc1084_ce_map,
};


static const _Sensor_BV2NR gc1084_bv2nr_init[BV2RAWNR_ARRAY_NUM] = {
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

static const uint32 gc1084_lsc_tbl[] = {
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

static const _Sensor_LSC          gc1084_lsc_init = {
    .p_lsc_tbl = (uint32 *)gc1084_lsc_tbl,
};

static const _Sensor_LHS gc1084_lhs_map[] = {
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
static const _Sensor_YGAMMA gc1084_ygamma_tbl[NUM_CURVES] = {
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



static uint8 gc1084_regValTable[25][6] = {
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

static uint32 gc1084_gainLevelTable[26] = {
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

static const _Sensor_WDR gc1084_wdr_init = {
    .wdr_bv           = {791, 1599, 3534,  7000, 10000, 14000, 28000, 56000},
    .max_ns_slope     = {1.0,  1.0,  1.0,   1.0,  1.25,   1.5,   2.0,   3.0},
    .max_shadow_slope = {1.0,  1.0,  1.0,   1.0,   1.0,  1.25,   1.5,   1.5},
};

static void gc1084_ae_adjust(struct isp_exposure_opt *p_cfg)
{
    uint32 i            = 0;
    uint32 index        = 0;
    uint32 total        = sizeof(gc1084_gainLevelTable) / sizeof(uint32);
    uint32 tol_dig_gain = 0;
    uint32 gain         = p_cfg->analog_gain >> 2;
    uint8  *addr        = (uint8 *)p_cfg->data.addr;

    for(i = 0; i < total; i++)
    {
        if((gc1084_gainLevelTable[i] <= gain)&&(gain < gc1084_gainLevelTable[i+1]))
            break;
    }

    tol_dig_gain = (gain)*64/gc1084_gainLevelTable[i];

    addr[index++] = 0x00;
    addr[index++] = 0xd1;
    addr[index++] = gc1084_regValTable[i][0];
    addr[index++] = 0x00;
    addr[index++] = 0xd0;
    addr[index++] = gc1084_regValTable[i][1];
    addr[index++] = 0x03;
    addr[index++] = 0x1d;
    addr[index++] = 0x2e;
    addr[index++] = 0x0d;
    addr[index++] = 0xc1;
    addr[index++] = gc1084_regValTable[i][2];
    addr[index++] = 0x03;
    addr[index++] = 0x1d;
    addr[index++] = 0x28;
    addr[index++] = 0x00;
    addr[index++] = 0xb8;
    addr[index++] = gc1084_regValTable[i][3];
    addr[index++] = 0x00;
    addr[index++] = 0xb9;
    addr[index++] = gc1084_regValTable[i][4];
    addr[index++] = 0x01;
    addr[index++] = 0x55;
    addr[index++] = gc1084_regValTable[i][5];
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

static void gc1084_img_opt(struct isp_sensor_opt *p_opt)
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

static void gc1084_fps_opt(struct isp_sensor_opt *p_opt)
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

static const _Sensor_ISP_Init gc1084_isp_init = 
{
    .type         = ISP_INPUT_DAT_SRC_MIPI1,
    .pixel_h      = 720,
    .pixel_w      = 1280,
    .mirror       = 1,
    .reverse      = 1,
    .bayer_patten = ISP_BAYER_FORMAT_GRBG,
    .input_format = ISP_INPUT_DAT_FORMAT_RAW10,
    .adjust_func  = (isp_ae_func     )gc1084_ae_adjust,
    .p_blc        = (_Sensor_BLC    *)&gc1084_blc_init,
    .p_ccm        = (_Sensor_CCM    *)&gc1084_ccm_init,
    .p_awb        = (_Sensor_AWB    *)&gc1084_awb_init,
    .p_ae         = (_Sensor_AE     *)&gc1084_ae_init,
	.p_dpc        = (_Sensor_DPC    *)&gc1084_dpc_init,
	.p_csc        = (_Sensor_CSC    *)&gc1084_csc_init,
	.p_gic        = (_Sensor_GIC    *)&gc1084_gic_init,
    .p_csupp      = (_Sensor_CSUPP  *)&gc1084_csupp_init,
    .p_sharp      = (_Sensor_SHARP  *)&gc1084_sharp_init,
    .p_yuvnr      = (_Sensor_YUVNR  *)&gc1084_yuvnr_init,    
    .p_colenh     = (_Sensor_COLENH *)&gc1084_colenh_init,	
    .p_bv2nr      = (_Sensor_BV2NR  *)gc1084_bv2nr_init,
    .p_lsc        = (_Sensor_LSC    *)&gc1084_lsc_init,
	.p_lhs        = (_Sensor_LHS    *)gc1084_lhs_map,
	.p_ygamma     = (_Sensor_YGAMMA *)gc1084_ygamma_tbl,
	.p_wdr        = (_Sensor_WDR    *)&gc1084_wdr_init,
    .img_opt      = (sensor_img_opt  )gc1084_img_opt,
    .fps_opt      = (sensor_fps_opt  )gc1084_fps_opt,
};


SENSOR_OP_SECTION const _Sensor_Adpt_ gc1084_cmd_csi1 = 
{
	.typ = 1, //YUV
	.pixelw = 1280,
	.pixelh= 720,
	.hsyn = 1,
	.vsyn = 1,
	.rduline = 0,//
	.rawwide = 1,//10bit
	.colrarray = 1,//0:_RGRG_ 1:_GRGR_,2:_BGBG_,3:_GBGB_
	.init = (uint8 *)GC1084InitTable,
    .init_len = sizeof(GC1084InitTable),
    .slave_init = (uint8 *)initTable_slave_15fps,
    .nigth_mode_init = (uint8 *)initTable_slave_7fps,
    .sensor_stop_stream = (uint8 *)gc1084_stop_stream,
    .vts_reg = {0x0d41,0x0d42},
    .vts_reg_num = 2,
    .mipi_lane_num = 1,
	.rotate_adapt = {0},
	//.hvb_adapt = {0x6a,0x12,0x6a,0x12},
	. mclk = 24000000,
	.p_fun_adapt = {NULL,NULL,NULL},
	.sensor_isp  = (_Sensor_ISP_Init *)&gc1084_isp_init,
};

const _Sensor_Ident_ gc1084_init_csi1 =
{
	0x84, 0x6e, 0x6f, 0x02, 0x01, 0x03f1
};
#endif

