#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/isp.h"

/* lens & sensor config information:
- sensor      : H62
- fstop       : TBD
- mclk        : TBD
- max FPS     : TBD
- frame length: TBD
- usage       : TBD
- interface   : TBD
*/

#if DEV_SENSOR_H62


SENSOR_INIT_SECTION const unsigned char H62InitTable[CMOS_INIT_LEN]= 
{	
		//sensor inittab 初始化表
		0x12, 0x40, //sys start
		0x0c, 0x00,
//		0x0c, 0x01,  //10 bit walking '1' pattern
		0x0D, 0x50,
		0x0e, 0x11, //pll control1
		0x0f, 0x09, //pll control2
		0x10, 0x1e, //pll control3
		0x11, 0x80,  //clk
		0x19, 0x68, //luminance control
	#define FRAME_WIDTH		(2200)
	#define FRAME_HEIGHT	(789)		
	//	0x20, 0xac, // sensor frame time width [7:0]
		0x20, FRAME_WIDTH&0XFF, // sensor frame time width [7:0]
		0x21, FRAME_WIDTH>>8,  // sensor frame time width [15:8] :1920
		0x22, FRAME_HEIGHT&0XFF, // sensor frame time height [7:0]
		//0x22, 0x20, // sensor frame time height [7:0]
		0x23, FRAME_HEIGHT>>8, // sensor frame time height [15:8] 750
		
	//	0x20, 0x20, // sensor frame time width [7:0]
	//	0x21, 0x05,  // sensor frame time width [15:8] :1920
		//0x22, 0x15, // sensor frame time height [7:0]
	//	0x22, 0x10, // sensor frame time height [7:0]
	//	0x23, 0x03, // sensor frame time height [15:8] 750
		
	//	0x20,0x90,//;60  
	//	0x21,0x09,//;09
	//	0x22,0x40,
	//	0x23,0x06,		
		
		0x24, 0x00, //HWin [7:0]
		0x25, 0xd0, //VWin [7:0]
		0x26, 0x25, //HVWin

		
		0x27,0xd4,
		0x28,0x15,
		0x29,0x02,//*///
		0x2a, 0x63, 
		0x2b, 0x21,
		0x2c, 0x08,
		0x2d, 0x01,
		0x2e, 0xbc,
		0x2f, 0xc0,
		
		
		0x41, 0x88,
		0x42, 0x12,
		0x39, 0x90,
		0x1d, 0x00,
		0x1e, 0x04,
		0x7a, 0x4c, //DPHY2 mipi interface :normal operation
		0x70, 0x49, //Mipi1: timing control [7:5]Tlpx
		0x71, 0x2a, //Mipi2: [7:5] Ths-zero [4:0] RSVD
		0x72, 0x48, //Mipi3 :[7:5] Ths-prepare
		0x73, 0x33, //Mipi4 [6:4] Ths-trail
		0x74, 0x12, //Mipi5: [7] mipi_sleep off, [6] mipi continus mode
		0x75, 0x2b, //Mipi6: mipi data type ID
		0x76, 0x40, //Mipi word count LSBs
		0x77, 0x06,//Mipi word count MSBs
		0x78, 0x18,//Mipi9 [6:0] RSVD
		0x66, 0x38,
		0x1f, 0x20,//GLat rsvd
		0x30, 0x90,
		0x31, 0x0c,
		0x32, 0xff,
		0x33, 0x0c,
		0x34, 0x4b,
		0x35, 0xa3,
		0x36, 0x06,
		0x38, 0x40,
		0x3a, 0x08,
		0x56, 0x02,
		0x60, 0x01,
		0x0d, 0x50,
		0x57, 0x80,
		0x58, 0x33,
		0x5a, 0x04,
		0x5b, 0xb6,
		0x5c, 0x08,
		0x5d, 0x67,
		0x5e, 0x04,
		0x5f, 0x08,
		0x66, 0x28,
		0x67, 0xf8,
		0x68, 0x00,
		0x69, 0x74,
		0x6a, 0x1f,
		0x63, 0x82,
		0x6c, 0xc0,
		0x6e, 0x5c,
		0x82, 0x01,
		
	
		0x46, 0xc2,
		0x48, 0x7e,
		0x62, 0x40,
		0x7d, 0x57,
		0x7e, 0x28,
		0x80, 0x00,
		0x4a, 0x05,
		0x4C, 0x08,
		0x4D, 0x08,
		0x49, 0x08,//0x10,
		0x13, 0x81,
		0x59, 0x9C,//0x97,
		0x12, 0x00, //sleep out
		0x47, 0x47,
		//sleep 500
		0x0b, 0x62,
		0x0b, 0x62,
		0x0b, 0x62,
		0x0b, 0x62,
		0x0b, 0x62,
		0x0b, 0x62,
		0x0b, 0x62,
		0x0b, 0x62,
		0x0b, 0x62,
		0x0b, 0x62,
		0x0b, 0x62,
		0x0b, 0x62,
		
		0x47, 0x44, 
		0x1f, 0x21, 
		//0x17, 0x00, 
		//0x16, 0x20,
	//	0xc0, 0x01,
	//	0xc1, 0xaa,
	//	0xc2, 0x02,
	//	0xc3, 0x03,
	//	0xc4, 0x00,
	//	0xc5, 0x20,
	//	0x1f, 0x80,
		0x02, 0x02,
		0x01, 0x20,
		0x00, 0x03,
	//	0x13, 0x80,
	//	0x14, 0x40,

	0xff,0xff//
};

const _Sensor_CCM h62_ccm_init = {
	// 5500k, gamma2p2
    0x209,  0xf8b,  0xfe7,
    0xf0d,  0x1d7,  0xf0c,
    0xfe8,  0xf9a,  0x1f6,
    0xffb,  0xffd,  0xffb,
};

const _Sensor_BLC h62_blc_init =
{
    33, 33, 33, 33,
};

const _Sensor_AWB h62_awb_init = 
{
//    r,  gr,  gb,   b
    .default_gain   = {454, 256, 256, 466},
    .awb_min_gain   = {306, 256, 256, 466},
    .awb_max_gain   = {454, 256, 256, 644},
 
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

const _Sensor_AE h62_ae_init = 
{
    .max_frame_length      = 789,
    .min_frame_vb          = 9,
    .max_analog_gain       = 32 << 8,
    .min_analog_gain       = 1 << 8,
    .default_exposure_line = 780, //0x2e9,//0x2de,
    .max_exposure_line     = 780, //749,
    .min_exposure_line     = 2,
    .row_time_us           = 2448/24,//89,
    .expo_frame_interval   = 2,
    .to_day_bv             = 1528,
    .to_night_bv           = 369,
    .dark_scene_target_lut = {50, 55},
    .dark_scene_bv_lut     = {47, 363},
    .hs_scene_limit_lut    = {55, 75},
    .hs_scene_bv_lut       = {363, 3022},
    .lowlight_lsb_bv_lut   = {47, 94, 195, 381, 781, 1636, 3225, 1e30},
    .lowlight_lsb_gain_lut = {16, 16,  16,  16,  16,   16,   16,   16},  // u7.4
};

const _Sensor_CSUPP h62_csupp_init = {
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

const _Sensor_SHARP h62_sharp_init = {
    .filt_alpha      = 128,
    .shrink_thr      = 0  ,
    .filt_clip_hi    = 127,
    .filt_clip_lo    = 127,
    .sp_thr2 		 = 10 ,
    .sp_thr1 	 	 = 5  ,
    .enha_clip_hi 	 = 127,
    .enha_clip_lo	 = 127, 
    .e1 = 5, .e2 =  10, .e3 = 15,
    .k0 = 0, .k1 = 128, .k2 = 128, .k3 = 128,
    .y1 = 0,              // y1 = k0*e1
    .y2 = 20,              // y2 = k1*e2 + (y1 - k1*e1) = k1 * (e2 - e1) + y1
    .y3 = 40,              // y3 = k2*e3 + (y2 - k2*e2) = k2 * (e3 - e2) + y2
    .filt_w11 = 7 ,  .filt_w12 = 9,  .filt_w13 = 10,
    .filt_w21 = 9 ,  .filt_w22 = 12, .filt_w23 = 13,
    .filt_w31 = 10,  .filt_w32 = 13, .filt_w33 = 16,
    .filt_type       = 1,
    .filt_sbit       = 8,
    .lpf_scale		 = 1,
    .strength_lut    = { 64, 128},
    .strength_bv_lut = {369,1528},

};

const _Sensor_YUVNR h62_yuvnr_init = {
    20,20,20,20,20,20,20,20,
    205,255,0,
};

const _Sensor_COLENH h62_colenh_init = {
    .yuv_range      = 0,
    .luma           = 50,
    .contrast       = 60,
    .saturation     = 70,
    .hue            = 0,
    .ce_in_ofs_y    = 128,
    .ce_in_ofs_cb   = 128,
    .ce_in_ofs_cr   = 128,
    .ce_out_ofs_y   = 128,
    .ce_out_ofs_cb  = 128,
    .ce_out_ofs_cr  = 128,
    .adj_sat_by_bv  = 1,
    .hi_sat         = 70,
    .lo_sat         = 50,
    .hi_sat_bv      = 1638, // high brightness for saturation
    .lo_sat_bv      = 381, // low brightness for saturation

};

const _Sensor_BV2NR h62_bv2nr_init[BV2RAWNR_ARRAY_NUM] = {
    //           ev, bnr_range_weight_index, bnr_invksigma, bnr_intensity_threshold, yuvnr_idx, csupp_idx
    {         48664,                      0,             0,                      63,         0,         0,          0,          0},    // 320lux
    {          5993,                      4,           407,                      63,         0,         0,          1,          1},    // 40lux
    {          3022,                      4,           407,                      50,         0,         0,          1,          1},    // 20lux
    {          1528,                      4,           407,                      40,         1,         0,          2,          1},    // 10lux
    {           754,                      4,           244,                      35,         2,         1,          2,          1},    // 5p03lux
    {           369,                      8,           244,                      35,         3,         1,          2,          1},    // 2p5lux
    {           184,                      8,           244,                      35,         3,         2,          2,          1},    // 1p25lux
    {            90,                     19,            48,                      35,         3,         2,          2,          1},    // 0p62lux
    {            46,                     19,            25,                      35,         3,         2,          2,          1},    // 0p31lux
    {            32,                     19,            25,                      35,         3,         2,          2,          1},    // 0p1lux
    {            24,                     19,            25,                      35,         3,         2,          2,          1},    // 0p01lux
};

const uint32 h62_lsc_tbl[] = {
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

const _Sensor_LSC          h62_lsc_init = {
    .p_lsc_tbl = (uint32 *)h62_lsc_tbl,
};

const _Sensor_LHS h62_lhs_map[] = {
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

const _Sensor_YGAMMA h62_ygamma_tbl[] = {
    0x00801000, 0x01103008, 0x01B05811, 0x0250801B, 0x0300A825, 0x03C0D830, 0x0481083C, 0x05513C48,
    0x06317055, 0x0711A863, 0x0801E071, 0x08F21C80, 0x09E2588F, 0x0AE2989E, 0x0BF2DCAE, 0x0D031CBF, 
    0x0E1360D0, 0x0F33A8E1, 0x1053F0F3, 0x11743905, 0x12948117, 0x13C4CD29, 0x14F5193C, 0x1625654F, 
    0x1765B162, 0x1895FD76, 0x19D64D89, 0x1B169D9D, 0x1C56EDB1, 0x1D973DC5, 0x1ED78DD9, 0x2007DDED, 
    0x21482A00, 0x22887A14, 0x23C8CA28, 0x25091A3C, 0x26496A50, 0x2779BA64, 0x28BA0677, 0x29EA568B,
    0x2B1AA29E, 0x2C4AEEB1, 0x2D7B36C4, 0x2E9B82D7, 0x2FBBCAE9, 0x30DC12FB, 0x31EC5B0D, 0x330C9F1E,
    0x340CE330, 0x351D2340, 0x360D6751, 0x370DA360, 0x37FDDF70, 0x38DE1B7F, 0x39BE538D, 0x3A9E8B9B,
    0x3B6EBFA9, 0x3C2EF3B6, 0x3CEF23C2, 0x3D9F53CE, 0x3E4F7BD9, 0x3EDFA7E4, 0x3F7FCBED, 0x3FFFEFF7,
};

void h62_ae_adjust(struct isp_ae_func_cfg *p_cfg)
{
    uint8  gain_segment[]    = {1, 2, 4, 8, 16, 32};
    uint8  sensor_gain_part1 = 0;
    uint8  i                 = 0;
    uint8  sensor_gain_reg   = 0;
    uint8  analog_gain       = p_cfg->analog_gain>>8;
    uint16 sensor_gain_part2 = 0;
    uint32 exposure_line     = p_cfg->exposure_line;
    uint8  *addr             = (uint8 *)p_cfg->data.addr;

    // convert the analog gain to match the SFR configure value of H62
    for(i=0;i<5;i++){
        // note: ae_param->analog_gain is UQ16.8
        if(analog_gain < gain_segment[i+1]){
            sensor_gain_part1 = i;
            break;
        }
    }
    sensor_gain_part2 = (((p_cfg->analog_gain >> sensor_gain_part1) - 256) >> 4) & 0x0f;
    sensor_gain_reg = (sensor_gain_part1 << 4) | sensor_gain_part2;
    i = 0;
    addr[i++] = 0x02;
    addr[i++] = (exposure_line >> 8) & 0xff;
    addr[i++] = 0x01;
    addr[i++] = exposure_line & 0xff;
    addr[i++] = 0x00;
    addr[i++] = sensor_gain_reg;
    p_cfg->data.size = i;
    p_cfg->cmd_len   = 1+1;
}

const _Sensor_ISP_Init h62_isp_init = 
{
    .type         = ISP_INPUT_DAT_SRC_MIPI0,
    .pixel_h      = 720,
    .pixel_w      = 1280,
    .bayer_patten = ISP_BAYER_FORMAT_BGGR,
    .input_format = ISP_INPUT_DAT_FORMAT_RAW10,
    .adjust_func  = (isp_ae_func     )h62_ae_adjust,
    .p_blc        = (_Sensor_BLC    *)&h62_blc_init,
    .p_ccm        = (_Sensor_CCM    *)&h62_ccm_init,
    .p_awb        = (_Sensor_AWB    *)&h62_awb_init,
    .p_ae         = (_Sensor_AE     *)&h62_ae_init,
    .p_csupp      = (_Sensor_CSUPP  *)&h62_csupp_init,
    .p_sharp      = (_Sensor_SHARP  *)&h62_sharp_init,
    .p_yuvnr      = (_Sensor_YUVNR  *)&h62_yuvnr_init,
	.p_colenh     = (_Sensor_COLENH *)&h62_colenh_init,		
    .p_bv2nr      = (_Sensor_BV2NR  *)h62_bv2nr_init,
    .p_lsc        = (_Sensor_LSC    *)&h62_lsc_init,
    .p_lhs        = (_Sensor_LHS    *)h62_lhs_map,
    .p_ygamma     = (_Sensor_YGAMMA *)h62_ygamma_tbl,
};

SENSOR_OP_SECTION const _Sensor_Adpt_ h62_cmd= 
{	
	.typ = 1, //YUV
	.pixelw = 1280,
	.pixelh= 720,
	.hsyn = 1,
	.vsyn = 0,
	.rduline = 0,//
	.rawwide = 1,//10bit
	.colrarray = 2,//0:_RGRG_ 1:_GRGR_,2:_BGBG_,3:_GBGB_
	.init = (uint8 *)H62InitTable,
    .init_len = sizeof(H62InitTable),
    .mipi_lane_num = 1,
	.rotate_adapt = {0},
	.hvb_adapt = {0x80,0x0a,0x80,0x0a},
	. mclk = 20000000,
	.p_fun_adapt = {NULL,NULL,NULL},
    .sensor_isp = (_Sensor_ISP_Init *)&h62_isp_init,
};

const _Sensor_Ident_ h62_init=
{
	0x62,0x60,0x61,0x01,0x01,0x0b
};
#endif
