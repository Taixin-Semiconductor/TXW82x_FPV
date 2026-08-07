#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/isp.h"


#if DEV_SENSOR_GC1084
#define SLAVE_MODE      1

SENSOR_INIT_SECTION const unsigned char GC1084InitTable[CMOS_INIT_LEN]=
{
    0x03, 0xfe, 0xf0,
    0x03, 0xfe, 0xf0,
    0x03, 0xfe, 0xf0,
    0x03, 0xfe, 0x00,
    0x03, 0xf2, 0x00,
    0x03, 0xf3, 0x00,
    0x03, 0xf4, 0x36,
    0x03, 0xf5, 0xc0,
    0x03, 0xf6, 0x13,
    0x03, 0xf7, 0x01,
    0x03, 0xf8, 0x32,
    0x03, 0xf9, 0x21,
    0x03, 0xfc, 0xae,
    0x0d, 0x05, 0x08,   // line length[13:8]
    0x0d, 0x06, 0xae,   // line length[07:0]
    0x0d, 0x08, 0x10,
    0x0d, 0x0a, 0x02,   // row start[7:0]  row start{0x0d09}[10:8]
    0x00, 0x0c, 0x03,   // col start[7:0]  col start{0x000b}[10:8]
    0x0d, 0x0d, 0x02,   // window height[10:8]  724
    0x0d, 0x0e, 0xd4,   // window height[07:0] 
    0x00, 0x0f, 0x05,   // window width[11:8]   1288
    0x00, 0x10, 0x08,   // window width[07:0]
    0x00, 0x17, 0x08,
    0x0d, 0x73, 0x92,
    0x00, 0x76, 0x00,
    0x0d, 0x76, 0x00,
    0x0d, 0x41, 0x03,   // frame height[13:8] frame height = window_height+16+VB_val{0x0d79[13:8] 0x0d7a[7:0]}(default:0x10)
    0x0d, 0x42, 0x78,   // frame height[07:0]
    0x0d, 0x7a, 0x0a,
    0x00, 0x6b, 0x18,
    0x0d, 0xb0, 0x9d,
    0x0d, 0xb1, 0x00,
    0x0d, 0xb2, 0xac,
    0x0d, 0xb3, 0xd5,
    0x0d, 0xb4, 0x00,
    0x0d, 0xb5, 0x97,
    0x0d, 0xb6, 0x09,
    0x00, 0xd2, 0xfc,
    0x0d, 0x19, 0x31,
    0x0d, 0x20, 0x40,
    0x0d, 0x25, 0xcb,
    0x0d, 0x27, 0x03,
    0x0d, 0x29, 0x40,
    0x0d, 0x43, 0x20,
    0x00, 0x58, 0x60,
    0x00, 0xd6, 0x66,
    0x00, 0xd7, 0x19,
    0x00, 0x93, 0x02,
    0x00, 0xd9, 0x14,
    0x00, 0xda, 0xc1,       
    0x0d, 0x2a, 0x00,
    0x0d, 0x28, 0x04,
    0x0d, 0xc2, 0x84,
    0x00, 0x15, 0x03, //[1 : 0]     00 : normal             01 : vertical fip
    0x0d, 0x15, 0x03, //            10:horizonral mirror    11 : horizonral & vertical
    0x00, 0x50, 0x30,
    0x00, 0x80, 0x07,
    0x00, 0x8c, 0x05,
    0x00, 0x8d, 0xa8,
    0x00, 0x77, 0x01,
    0x00, 0x78, 0xee,
    0x00, 0x79, 0x02,
    0x00, 0x67, 0xc0,
    0x00, 0x54, 0xff,
    0x00, 0x55, 0x02,
    0x00, 0x56, 0x00,
    0x00, 0x57, 0x04,
    0x00, 0x5a, 0xff,
    0x00, 0x5b, 0x07,
    0x00, 0xd5, 0x03,
    0x01, 0x02, 0xa9,
    0x0d, 0x03, 0x02,
    0x0d, 0x04, 0xd0,
    0x00, 0x7a, 0x60,
    0x04, 0xe0, 0xff,
    0x04, 0x14, 0x75,
    0x04, 0x15, 0x75,
    0x04, 0x16, 0x75,
    0x04, 0x17, 0x75,
    0x01, 0x22, 0x00,
    0x01, 0x21, 0x80,
    0x04, 0x28, 0x10,
    0x04, 0x29, 0x10,
    0x04, 0x2a, 0x10,
    0x04, 0x2b, 0x10,
    0x04, 0x2c, 0x14,
    0x04, 0x2d, 0x14,
    0x04, 0x2e, 0x18,
    0x04, 0x2f, 0x18,
    0x04, 0x30, 0x05,
    0x04, 0x31, 0x05,
    0x04, 0x32, 0x05,
    0x04, 0x33, 0x05,
    0x04, 0x34, 0x05,
    0x04, 0x35, 0x05,
    0x04, 0x36, 0x05,
    0x04, 0x37, 0x05,
    0x01, 0x53, 0x00,
    0x01, 0x8c, 0x10, // bit 2: test image
    0x01, 0x90, 0x01,
    0x01, 0x92, 0x02,
    0x01, 0x94, 0x04,
    0x01, 0x95, 0x02,
    0x01, 0x96, 0xd0,
    0x01, 0x97, 0x05,
    0x01, 0x98, 0x00,
    0x02, 0x01, 0x23,
    0x02, 0x02, 0x53,
    0x02, 0x03, 0xce,
    0x02, 0x08, 0x39,
    0x02, 0x12, 0x06,
    0x02, 0x13, 0x40,
    0x02, 0x15, 0x11, //[1:0],0x1:no data gate clk
//    0x02, 0x25, 0x80, //add
//    0x02, 0x2b, 0x80, //add	
    0x02, 0x29, 0x05,
    0x02, 0x3e, 0x98,
    0x03, 0x1e, 0x3e,
    
    #if SLAVE_MODE
	0x00,0x68, 0x93, //[7]clock en	[4]row_counter	[2]every_frame_master  [1] every_frame_slave [0]en
	0x00,0x69, 0x00, //[6] vsync_mode  [5] fsync_out_polarity  [4] fsync_in_polarity  [3] gpio_mode  [2] gpio_value [1] vsync_out_mode
	0x0d,0x67, 0x00, //[1] fsync_vb_gap_lasts  [0] fsync_every_frame_slave
	0x0d,0x69, 0x04, //[7] fsync_row_force	 [6:0] fsync out position
	0x0d,0x6a, 0x80, //[7] position_FS_D  [3] position_FS_A  [2] position_FE_D
	0x0d,0x6c, 0x00, //[7] fsync_vb_diff_rnd   [6] fsync_exp_change_mode  [5:0] fsync_row_diff_th
	0x0d,0x6d, 0x13, //[7] exp_change_retime  [5:4] fsync_vb_gap   [3:2]gain switch mode   [1:0] fsync vb old
	0x0d,0x6e, 0x00, //fsync_row_diff_big[13:8] 
	0x0d,0x6f, 0x04, //fsync_row_diff_big[7:0]
	0x0d,0x70, 0x00, //fsync_row_diff_big2[13:8]
	0x0d,0x71, 0x12, //fsync_row_diff_big2[7:0]
	0x0d,0x6b, 0x70, //[7] fsync_vb_gap_mode_tmp   [6] fsync_vb_first_mode_tmp	[5] fsync_row_diff_mode  [4] fsync_vb_mode	[3:0]vb_lowbits_disable
    #endif

    //dpc //
    0x01, 0x04, 0x0f,
    0x01, 0x89, 0x03,  //【5】bit=0  dpc enable  on
    0x01, 0x01, 0x0c,  //【7】bit=0 auto //【7】bit=1 manual
    
    0x04, 0x28, 0xff,  //[7:0]越小越强
    0x04, 0x29, 0xff,
    0x04, 0x2a, 0xff,
    0x04, 0x2b, 0xff,
    0x04, 0x2c, 0xff,
    0x04, 0x2d, 0xff,
    0x04, 0x2e, 0xff,
    0x04, 0x2f, 0xff,
    
    0x04, 0x30, 0x01,  //[3:0]越大越强
    0x04, 0x31, 0x01, 
    0x04, 0x32, 0x01,
    0x04, 0x33, 0x01,
    0x04, 0x34, 0x01,     
    0x04, 0x35, 0x01,
    0x04, 0x36, 0x01,
    0x04, 0x37, 0x01, 
        
    0x04, 0x38, 0x01,  //[7:0]越大越强
    0x04, 0x39, 0x01,
    0x04, 0x3a, 0x01,
    0x04, 0x3b, 0x01,
    0x04, 0x3c, 0x01,
    0x04, 0x3d, 0x01,
    0x04, 0x3e, 0x01,
    0x04, 0x3f, 0x01,
	
    0xff, 0xff, 0xff,
};

const _Sensor_CCM gc1084_ccm_init = {
    // 5000k gamma2.2
    480,  -95,  -64,
   -208,  456, -200,
    -16, -105,  520,
      0,    0,    0,
};

const _Sensor_BLC gc1084_blc_init =
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

const _Sensor_AWB gc1084_awb_init = 
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

const _Sensor_AE gc1084_ae_init = 
{
    .max_frame_length      = 888,
    .min_frame_vb          = 1,
    .curr_fps              = 24,
    // .max_analog_gain       = (32<<8),
    .max_analog_gain       = (16<<8),
    .min_analog_gain       =  1<<8,
    .default_exposure_line = 0x2de,//0x2de,
    .max_exposure_line     = 887, //749,
    .min_exposure_line     = 1,
    .row_time_us           = 67,
    .expo_frame_interval   = 3,    
    .to_day_bv             = 1528,
    .to_night_bv           = 369,
    .dark_scene_target_lut = {50, 55},
    .dark_scene_bv_lut     = {47, 363},
    .hs_scene_limit_lut    = {55, 75},
    .hs_scene_bv_lut       = {363, 3022},
    .lowlight_lsb_bv_lut   = {47, 94, 195, 381, 781, 1636, 3225, 1e30},
    .lowlight_lsb_gain_lut = {16, 16,  16,  16,  16,   16,   16,   16},  // u7.4
};

const _Sensor_DPC gc1084_dpc_init = 
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

const _Sensor_CSC gc1084_csc_init = 
{
    .rgb2yuv_gamut         = ISP_YUV_GAMUT_BT709,
    .rgb2yuv_range         = ISP_YUV_RANGE_NARROW,
    .yuv2rgb_in_gamut      = ISP_YUV_GAMUT_BT709,
    .yuv2rgb_in_range      = ISP_YUV_RANGE_NARROW,
    .yuv2rgb_out_gamut     = ISP_YUV_GAMUT_BT709,
    .yuv2rgb_out_range     = ISP_YUV_RANGE_NARROW,
    .y_gamma_alpha         = 0xff,
    .rgb_gamma_alpha       = 0xff,
};

const _Sensor_GIC gc1084_gic_init = 
{
    .w_thres  = 14,
    .w_slope  = 16,
    .w_str    = 127,
    .mu_thres = 5,
    .mu_slope = 16,
};

const _Sensor_CSUPP gc1084_csupp_init = {
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

const _Sensor_SHARP gc1084_sharp_init = {
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

const _Sensor_YUVNR gc1084_yuvnr_init = {
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

const _Sensor_COLENH gc1084_colenh_init = {
    .yuv_range     = 0, // 0: narrow range, 1: full range
    .luma          = 50, // range: 0 ~ 100
    .contrast      = 56, // range: 0 ~ 100
    .saturation    = 55, // range: 0 ~ 100
    .hue           = 0, // range: -180 ~ 180
    .ce_in_ofs_y   = 128,
    .ce_in_ofs_cb  = 128,
    .ce_in_ofs_cr  = 128, // range: -128 ~ 128
    .ce_out_ofs_y  = 128, 
    .ce_out_ofs_cb = 128, 
    .ce_out_ofs_cr = 128, // range: -128 ~ 128
    .adj_sat_by_bv = 1,
    .hi_sat        = 57,
    .lo_sat        = 50,
    .hi_sat_bv     = 1391, // high brightness for saturation
    .lo_sat_bv     = 347, // low brightness for saturation
};



const _Sensor_BV2NR gc1084_bv2nr_init[BV2RAWNR_ARRAY_NUM] = {
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

const uint32 gc1084_lsc_tbl[] = {
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

const _Sensor_LSC          gc1084_lsc_init = {
    .p_lsc_tbl = (uint32 *)gc1084_lsc_tbl,
};

const _Sensor_LHS gc1084_lhs_map[] = {
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

const _Sensor_YGAMMA gc1084_ygamma_tbl[] = {
    0x00801000, 0x01103008, 0x01B05811, 0x0250801B, 0x0300A825, 0x03C0D830, 0x0481083C, 0x05513C48,
    0x06317055, 0x0711A863, 0x0801E071, 0x08F21C80, 0x09E2588F, 0x0AE2989E, 0x0BF2DCAE, 0x0D031CBF, 
    0x0E1360D0, 0x0F33A8E1, 0x1053F0F3, 0x11743905, 0x12948117, 0x13C4CD29, 0x14F5193C, 0x1625654F, 
    0x1765B162, 0x1895FD76, 0x19D64D89, 0x1B169D9D, 0x1C56EDB1, 0x1D973DC5, 0x1ED78DD9, 0x2007DDED, 
    0x21482A00, 0x22887A14, 0x23C8CA28, 0x25091A3C, 0x26496A50, 0x2779BA64, 0x28BA0677, 0x29EA568B,
    0x2B1AA29E, 0x2C4AEEB1, 0x2D7B36C4, 0x2E9B82D7, 0x2FBBCAE9, 0x30DC12FB, 0x31EC5B0D, 0x330C9F1E,
    0x340CE330, 0x351D2340, 0x360D6751, 0x370DA360, 0x37FDDF70, 0x38DE1B7F, 0x39BE538D, 0x3A9E8B9B,
    0x3B6EBFA9, 0x3C2EF3B6, 0x3CEF23C2, 0x3D9F53CE, 0x3E4F7BD9, 0x3EDFA7E4, 0x3F7FCBED, 0x3FFFEFF7,
};

uint8 gc1084_regValTable[25][6] = {
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

uint32 gc1084_gainLevelTable[26] = {
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

const _Sensor_WDR gc1084_wdr_init = {
    .wdr_bv           = {791, 1599, 3534,  7000, 10000, 14000, 28000, 56000},
    .max_ns_slope     = {1.0,  1.0,  1.0,   1.0,  1.25,   1.5,   2.0,   3.0},
    .max_shadow_slope = {1.0,  1.0,  1.0,   1.0,   1.0,  1.25,   1.5,   1.5},
};

void gc1084_ae_adjust(struct isp_ae_func_cfg *p_cfg)
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

    if (p_cfg->img_operation)
    {
        addr[index++] = 0x00;
        addr[index++] = 0x15;
        addr[index++] = p_cfg->reverse_en*2 + p_cfg->mirror_en;

        addr[index++] = 0x0d;
        addr[index++] = 0x15;
        addr[index++] = p_cfg->reverse_en*2 + p_cfg->mirror_en;
    }
    
    p_cfg->data.size = index;
    p_cfg->cmd_len   = 2+1;
    // os_printf("%s %d expo_line : %d\r\n", __func__, __LINE__, p_cfg->exposure_line);
}

void gc1084_img_opt(struct isp_sensor_opt *p_opt)
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

void gc1084_fps_opt(struct isp_sensor_opt *p_opt)
{
    uint8  *addr = (uint8 *)p_opt->data.addr;
    uint8  index = 0;
    uint32 frame_length = p_opt->total_length / p_opt->curr_fps;
    addr[index++] = 0x0d;
    addr[index++] = 0x41;
    addr[index++] = frame_length>> 8;
    addr[index++] = 0x0d;
    addr[index++] = 0x42;
    addr[index++] = frame_length & 0xff;
    p_opt->data.size = index;
    p_opt->cmd_len   = 2+1;
}

const _Sensor_ISP_Init gc1084_isp_init = 
{
    .type         = ISP_INPUT_DAT_SRC_MIPI0,
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


SENSOR_OP_SECTION const _Sensor_Adpt_ gc1084_cmd = 
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
    .mipi_lane_num = 1,
	.rotate_adapt = {0},
	//.hvb_adapt = {0x6a,0x12,0x6a,0x12},
	. mclk = 24000000,
	.p_fun_adapt = {NULL,NULL,NULL},
    .sensor_isp = (_Sensor_ISP_Init *)&gc1084_isp_init,
};

const _Sensor_Ident_ gc1084_init =
{
	0x84, 0x6e, 0x6f, 0x02, 0x01, 0x03f1
};
#endif

