#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/isp.h"

/* lens & sensor config information:
- sensor      : gc2053
- fstop       : f2.2
- mclk        : 24M
- max FPS     : 25
- frame length: 1230
- usage       : TBD
- interface   : mipi 2 lane
*/

#if DEV_SENSOR_GC2053

SENSOR_INIT_SECTION const unsigned char GC2053InitTable[CMOS_INIT_LEN] =
{
    0xfe,0x80,
    0xfe,0x80,
    0xfe,0x80,
    0xfe,0x00,
    0xf2,0x00,//[1]I2C_open_ena [0]pwd_dn
    0xf3,0x00,//0f//00[3]Sdata_pad_io [2:0]Ssync_pad_io 
    0xf4,0x36,//[6:4]pll_ldo_set
    0xf5,0xc0,//[7]soc_mclk_enable [6]pll_ldo_en [5:4]cp_clk_sel [3:0]cp_clk_div
    0xf6,0x44,//[7:3]wpllclk_div [2:0]refmp_div
    0xf7,0x01,//[7]refdiv2d5_en [6]refdiv1d5_en [5:4]scaler_mode [3]refmp_enb [1]div2en [0]pllmp_en
    0xf8,0x2c,//[7:0]pllmp_div
    0xf9,0x42,//[7:3]rpllclk_div [2:1]pllmp_prediv [0]analog_pwc
    0xfc,0x8e,
    /****CISC,L & ANALOG****/
    0xfe,0x00,
    0x87,0x18,//[6]aec_delay_mode
    0xee,0x30,//[5:4]dwen_sramen
    0xd0,0xb7,//ramp_en
    0x03,0x04,
    0x04,0x60,
    0x05,0x04,//05
    0x06,0x4c,//60//[11:0]hb
    0x07,0x00,
    0x08,0x11,//19
    0x09,0x00, 
    0x0a,0x02, //cisctl row start
    0x0b,0x00,
    0x0c,0x02, //cisctl col start
    0x0d,0x04,
    0x0e,0x40,
    0x12,0xe2, //vsync_ahead_mode
    0x13,0x16,
    0x19,0x0a, //ad_pipe_num
    0x21,0x1c,//eqc1fc_eqc2fc_sw
    0x28,0x0a,//16//eqc2_c2clpen_sw
    0x29,0x24,//eq_post_width
    0x2b,0x04,//c2clpen --eqc2
    0x32,0xf8, //[5]txh_en ->avdd28
    0x37,0x03, //[3:2]eqc2sel=0
    0x39,0x15,//17 //[3:0]rsgl
    0x43,0x07,//vclamp
    0x44,0x40, //0e//post_tx_width
    0x46,0x0b, //txh¡ª¡ª3.2v
    0x4b,0x20, //rst_tx_width
    0x4e,0x08, //12//ramp_t1_width
    0x55,0x20, //read_tx_width_pp
    0x66,0x05, //18//stspd_width_r1
    0x67,0x05, //40//5//stspd_width_r
    0x77,0x01, //dacin offset x31
    0x78,0x00, //dacin offset
    0x7c,0x93, //[1:0] co1comp
    0x8c,0x12, //12 ramp_t1_ref
    0x8d,0x92, //90
    0x90,0x01,
    0x9d,0x10,
    0xce,0x7c,//70//78//[4:2]c1isel
    0xd2,0x41,//[5:3]c2clamp
    0xd3,0xdc,//ec//0x39[7]=0,0xd3[3]=1 rsgh=vref
    0xe6,0x50,//ramps offset
    /*gain*/ 
    0xb6,0xc0,
    0xb0,0x70,
    0xb1,0x01,
    0xb2,0x00,
    0xb3,0x00,
    0xb4,0x00,
    0xb8,0x01,
    0xb9,0x00,
    /*blk*/ 
    0x26,0x30,//23 //[4]Ð´0£¬È«n mode
    0xfe,0x01,
    0x40,0x23,
    0x55,0x07,
    0x60,0x40, //[7:0]WB_offset
    0xfe,0x04,
    0x14,0x78, //g1 ratio
    0x15,0x78, //r ratio
    0x16,0x78, //b ratio
    0x17,0x78, //g2 ratio
    /*window*/
    0xfe,0x01,
    0x92,0x00, //win y1
    0x94,0x03, //win x1
    0x95,0x04,
    0x96,0x38, //[10:0]out_height
    0x97,0x07,
    0x98,0x80, //[11:0]out_width
    /*ISP*/  
    0xfe,0x01,
    0x01,0x05,//03//[3]dpc blending mode [2]noise_mode [1:0]center_choose 2b'11:median 2b'10:avg 2'b00:near
    0x02,0x89, //[7:0]BFF_sram_mode
    0x04,0x01, //[0]DD_en
    0x07,0xa6,
    0x08,0xa9,
    0x09,0xa8,
    0x0a,0xa7,
    0x0b,0xff,
    0x0c,0xff,
    0x0f,0x00,
    0x50,0x1c,
    0x89,0x03,
    0xfe,0x04,
    0x28,0x86,
    0x29,0x86,
    0x2a,0x86,
    0x2b,0x68,
    0x2c,0x68,
    0x2d,0x68,
    0x2e,0x68,
    0x2f,0x68,
    0x30,0x4f,
    0x31,0x68,
    0x32,0x67,
    0x33,0x66,
    0x34,0x66,
    0x35,0x66,
    0x36,0x66,
    0x37,0x66,
    0x38,0x62,
    0x39,0x62,
    0x3a,0x62,
    0x3b,0x62,
    0x3c,0x62,
    0x3d,0x62,
    0x3e,0x62,
    0x3f,0x62,
    /****DVP , MIPI****/
    0xfe,0x01,
    0x9a,0x06,//[5]OUT_gate_mode [4]hsync_delay_half_pclk [3]data_delay_half_pclk [2]vsync_polarity [1]hsync_polarity [0]pclk_out_polarity
    0xfe,0x00,
    0x7b,0x2a,//[7:6]updn [5:4]drv_high_data [3:2]drv_low_data [1:0]drv_pclk
    0x23,0x2d,//[3]rst_rc [2:1]drv_sync [0]pwd_rc
    0xfe,0x03,
    0x01,0x27,//20//27[6:5]clkctr [2]phy-lane1_en [1]phy-lane0_en [0]phy_clk_en
    0x02,0x56,//[7:6]data1ctr [5:4]data0ctr [3:0]mipi_diff
    0x03,0xce,//b2//b6[7]clklane_p2s_sel [6:5]data0hs_ph [4]data0_delay1s [3]clkdelay1s [2]mipi_en [1:0]clkhs_ph
    0x12,0x80,
    0x13,0x07,//LWC
    0x15,0x12,//[1:0]clk_lane_mode
    0xfe,0x00,
    0x3e,0x93,//40//91[7]lane_ena [6]DVPBUF_ena [5]ULPEna [4]MIPI_ena [3]mipi_set_auto_disable [2]RAW8_mode [1]ine_sync_mode [0]double_lane_en
    0x41,0x04,
    0x42,0xce,
    0xff,0xff,
};

const _Sensor_CCM gc2053_ccm_init = {
    // 5500k, gamma2p2
    //0x20e,  0xf79,  0xfe6,
    //0xf31,  0x1e9,  0xf59,
    //0xfc7,  0xfa7,  0x1b2,
    //0xffc,  0xffe,  0xffc,
    404, -163,  -45,
      0,  546,  -59,
   -148, -127,  360,
      0,    0,    0,
};

const _Sensor_BLC gc2053_blc_init =
{
    17*16, 17*16, 17*16, 17*16,
};

const _Sensor_AWB gc2053_awb_init = 
{
    .default_gain   = {383, 256, 256, 478},
    .awb_min_gain   = {256, 256, 256, 350},
    .awb_max_gain   = {550, 256, 256, 768},
 
    .coarse_constraint = {
        .coarse_min_bg =  70,
        .coarse_lb_bg  = 110,
        .coarse_rt_bg  = 120,
        .coarse_max_bg = 190,
        .coarse_min_rg = 100,
        .coarse_lb_rg  = 145,
        .coarse_rt_rg  = 170,
        .coarse_max_rg = 270,
    },

    .constraint = {
        .section_num = 6,
        //   temp:   7500, 6500, 5000, 4000, 3000, 2300
        //  rgain:    457,  448,  406,  363,  319,  275
        //  bgain:    402,  436,  470,  518,  578,  634
        // lower,upper range
        // lower,upper
        //    15,   10
        //    15,   15
        //    20,   15
        //    20,   15
        //    15,   15
        //    15,   15
        .color_temp          = {        7500,        6500,        5000,        4000,        3000,        2300,           0,           0},
        .sec_line_slope      = {  0.23076923,        0.75,   1.5217391,   1.6923077,       2.375,         3.3,           0,           0},
        .sec_line_offset     = {         130,        40.5,        -106,  -179.30769,    -373.875,      -682.4,           0,           0},
        .sec_line_sqrtk2add1 = {   0.9743912,         0.8,  0.54917789,  0.50872931,    0.388057,   0.2900074,           0,           0},
        .center_line_slope   = {  -4.3333333, -0.73333333,        -0.6, -0.58333333,  -0.3030303,           0,           0},
        .center_line_offset  = {   782.66667,   257.06667,       235.6,   232.58333,   175.12121,           0,           0},
        .lower_line_slope    = {  -3.3168706,  -1.1684311,  -0.6009149, -0.37406012, -0.30550067,           0,           0},
        .lower_line_offset   = {   585.46066,   297.56977,   212.43301,   173.68044,   160.02483,           0,           0},
        .upper_line_slope    = {    -1.18883,  -0.6642319, -0.59926374,  -0.5898791, -0.30032946,           0,           0},
        .upper_line_offset   = {   346.83514,   263.94864,   252.95358,   251.18335,   190.14025,           0,           0},
        .corner_limit        = {   128.38413,   159.62711,   152.74391,    165.2486,   233.64989,   88.644634,   242.35011,   117.35537},
    },
};

const _Sensor_AE gc2053_ae_init = 
{
    .max_frame_length      = 1230,
    .min_frame_vb          = 1,
    .max_analog_gain       = 24<<8,
    .min_analog_gain       = 1<<8,
    .default_exposure_line = 1229,
    .max_exposure_line     = 1229,
    .min_exposure_line     = 1,
    .row_time_us           = 32,
    .expo_frame_interval   = 2,
    .to_day_bv             = 962,       // 10lux
    .to_night_bv           = 465,
    .dark_scene_target_lut = {35, 55},
    .dark_scene_bv_lut     = {34, 300},
    .hs_scene_limit_lut    = {55, 65},
    .hs_scene_bv_lut       = {791, 3534},
    .lowlight_lsb_bv_lut   = {34, 80, 204, 400, 791, 1599, 3534, 1e30},
    .lowlight_lsb_gain_lut = {64, 32,  16,  16,  16,  16,   16,   16},  // u7.4
};

const _Sensor_CSUPP gc2053_csupp_init = {
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

const _Sensor_SHARP gc2053_sharp_init = {
    .filt_alpha      = 128,
    .shrink_thr      = 5  ,
    .filt_clip_hi    = 127,
    .filt_clip_lo    = 127,

	.sp_thr2 		 = 20 ,    
    .sp_thr1 	 	 = 10 ,
    .enha_clip_hi 	 = 127,
    .enha_clip_lo	 = 127,

	.e1 = 8,  .e2 = 28, .e3 = 40,
    .k0 = 96, .k1 = 32, .k2 = 32, .k3 = 16,
    .y1 = 24,             // y1 = k0*e1
    .y2 = 44,             // y2 = k1*e2 + (y1 - k1*e1) = k1 * (e2 - e1) + y1
    .y3 = 56,             // y3 = k2*e3 + (y2 - k2*e2) = k2 * (e3 - e2) + y2

    // --- Unsharp Mask ---
	.filt_w11 = 7 , .filt_w12 = 9 , .filt_w13 = 10,
    .filt_w21 = 9 , .filt_w22 = 11, .filt_w23 = 12,
    .filt_w31 = 10, .filt_w32 = 12, .filt_w33 = 24,
    .filt_type = 1,
    .filt_sbit = 8,
    .lpf_scale = 1,

    .strength_lut    = {32,  255},
    .strength_bv_lut = {700, 9930},
};

const _Sensor_YUVNR gc2053_yuvnr_init = {
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

const _Sensor_COLENH gc2053_colenh_init = {
    .yuv_range  = 0,
    .luma       = 50, // range: 0 ~ 100
    .contrast   = 55, // range: 0 ~ 100
    .saturation = 70, // range: 0 ~ 100
    .hue        = 0, // range: -180 ~ 180
    .ce_in_ofs_y   = 128,
    .ce_in_ofs_cb  = 128,
    .ce_in_ofs_cr  = 128, // range: -128 ~ 128
    .ce_out_ofs_y  = 128, 
    .ce_out_ofs_cb = 128, 
    .ce_out_ofs_cr = 128, // range: -128 ~ 128
    .adj_sat_by_bv = 1,
    .hi_sat     = 70,
    .lo_sat     = 50,
    .hi_sat_bv  = 3531, // high brightness for saturation
    .lo_sat_bv  = 400,  // low brightness for saturation
};

const _Sensor_BV2NR gc2053_bv2nr_init[BV2RAWNR_ARRAY_NUM] = {
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
const uint32 gc2053_lsc_tbl[] = {
// R channel
0x00087A70, 0x0005B5B5, 0x0004A945, 0x0004491C, 0x00045D13, 0x0004F127, 0x00069160, 0x000A3208, 0x00000350, 0x0008064C, 0x000595AC, 0x0004893D, 0x00042512, 0x0004390A, 0x0004C91E, 0x00064554, 
0x000995E7, 0x0000030A, 0x0007AA41, 0x0005699B, 0x00047134, 0x00041D0A, 0x00042508, 0x0004A516, 0x00060549, 0x00090DCF, 0x000002C1, 0x0007522A, 0x00056190, 0x00046530, 0x0004190A, 0x00042106, 
0x00049911, 0x0005D943, 0x0008B9BE, 0x0000028B, 0x0007261B, 0x0005558B, 0x0004592F, 0x00041107, 0x00042905, 0x00049511, 0x0005BD42, 0x00087DB4, 0x0000026A, 0x00072610, 0x00054989, 0x00046131, 
0x00041509, 0x00043108, 0x00048D15, 0x0005BD41, 0x000849B2, 0x00000259, 0x00071E0F, 0x00055988, 0x00047135, 0x0004310F, 0x00044D0E, 0x00049D1A, 0x0005C942, 0x000839B5, 0x00000251, 0x00072A14, 
0x0005618C, 0x00047D36, 0x00045517, 0x00046918, 0x0004BD1F, 0x0005E549, 0x000841BA, 0x00000251, 0x0007521F, 0x00057D95, 0x0004A53F, 0x00047D20, 0x00048520, 0x0004E129, 0x00060151, 0x00088DC3, 
0x00000260, 0x00079632, 0x0005A5A1, 0x0004C947, 0x0004A52B, 0x0004AD2A, 0x0004FD33, 0x0006355B, 0x0008ADCF, 0x00000263, 0x0007DA44, 0x0005E9B0, 0x0004ED51, 0x0004CD35, 0x0004D532, 0x00052539, 
0x0006716D, 0x0008F1E1, 0x00000282, 0x00082A40, 0x00062DBF, 0x0005155E, 0x0004E13B, 0x0004F13A, 0x0005593E, 0x0006B57D, 0x000931F1, 0x000002A9, 0x00089246, 0x00066DD2, 0x00054D71, 0x0004ED42, 
0x0005053E, 0x00059D4B, 0x0007018F, 0x0009820A, 0x000002D3, 0x0008E668, 0x0006C1E4, 0x00059984, 0x00052952, 0x00055950, 0x0005F562, 0x000745A4, 0x0009D224, 0x00000301, 0x00093A98, 0x00071E05, 
0x0005F59A, 0x00058569, 0x0005B165, 0x00064579, 0x0007B9B7, 0x000A6E39, 0x0000032C, 0x0009B2DB, 0x0007822A, 0x000651B6, 0x0005FD80, 0x00060980, 0x0006A18E, 0x000845CB, 0x000B264F, 0x00000355, 
0x000A5316, 0x0007F242, 0x00069DC7, 0x00064594, 0x00065992, 0x0006E9A1, 0x000895E2, 0x000BC65F, 0x0000037E,
// Gr channel
0x00087A70, 0x0005B5B5, 0x0004A945, 0x0004491C, 0x00045D13, 0x0004F127, 0x00069160, 0x000A3208, 0x00000350, 0x0008064C, 0x000595AC, 0x0004893D, 0x00042512, 0x0004390A, 0x0004C91E, 0x00064554, 
0x000995E7, 0x0000030A, 0x0007AA41, 0x0005699B, 0x00047134, 0x00041D0A, 0x00042508, 0x0004A516, 0x00060549, 0x00090DCF, 0x000002C1, 0x0007522A, 0x00056190, 0x00046530, 0x0004190A, 0x00042106, 
0x00049911, 0x0005D943, 0x0008B9BE, 0x0000028B, 0x0007261B, 0x0005558B, 0x0004592F, 0x00041107, 0x00042905, 0x00049511, 0x0005BD42, 0x00087DB4, 0x0000026A, 0x00072610, 0x00054989, 0x00046131, 
0x00041509, 0x00043108, 0x00048D15, 0x0005BD41, 0x000849B2, 0x00000259, 0x00071E0F, 0x00055988, 0x00047135, 0x0004310F, 0x00044D0E, 0x00049D1A, 0x0005C942, 0x000839B5, 0x00000251, 0x00072A14, 
0x0005618C, 0x00047D36, 0x00045517, 0x00046918, 0x0004BD1F, 0x0005E549, 0x000841BA, 0x00000251, 0x0007521F, 0x00057D95, 0x0004A53F, 0x00047D20, 0x00048520, 0x0004E129, 0x00060151, 0x00088DC3, 
0x00000260, 0x00079632, 0x0005A5A1, 0x0004C947, 0x0004A52B, 0x0004AD2A, 0x0004FD33, 0x0006355B, 0x0008ADCF, 0x00000263, 0x0007DA44, 0x0005E9B0, 0x0004ED51, 0x0004CD35, 0x0004D532, 0x00052539, 
0x0006716D, 0x0008F1E1, 0x00000282, 0x00082A40, 0x00062DBF, 0x0005155E, 0x0004E13B, 0x0004F13A, 0x0005593E, 0x0006B57D, 0x000931F1, 0x000002A9, 0x00089246, 0x00066DD2, 0x00054D71, 0x0004ED42, 
0x0005053E, 0x00059D4B, 0x0007018F, 0x0009820A, 0x000002D3, 0x0008E668, 0x0006C1E4, 0x00059984, 0x00052952, 0x00055950, 0x0005F562, 0x000745A4, 0x0009D224, 0x00000301, 0x00093A98, 0x00071E05, 
0x0005F59A, 0x00058569, 0x0005B165, 0x00064579, 0x0007B9B7, 0x000A6E39, 0x0000032C, 0x0009B2DB, 0x0007822A, 0x000651B6, 0x0005FD80, 0x00060980, 0x0006A18E, 0x000845CB, 0x000B264F, 0x00000355, 
0x000A5316, 0x0007F242, 0x00069DC7, 0x00064594, 0x00065992, 0x0006E9A1, 0x000895E2, 0x000BC65F, 0x0000037E,
// Gb channel
0x00087A70, 0x0005B5B5, 0x0004A945, 0x0004491C, 0x00045D13, 0x0004F127, 0x00069160, 0x000A3208, 0x00000350, 0x0008064C, 0x000595AC, 0x0004893D, 0x00042512, 0x0004390A, 0x0004C91E, 0x00064554, 
0x000995E7, 0x0000030A, 0x0007AA41, 0x0005699B, 0x00047134, 0x00041D0A, 0x00042508, 0x0004A516, 0x00060549, 0x00090DCF, 0x000002C1, 0x0007522A, 0x00056190, 0x00046530, 0x0004190A, 0x00042106, 
0x00049911, 0x0005D943, 0x0008B9BE, 0x0000028B, 0x0007261B, 0x0005558B, 0x0004592F, 0x00041107, 0x00042905, 0x00049511, 0x0005BD42, 0x00087DB4, 0x0000026A, 0x00072610, 0x00054989, 0x00046131, 
0x00041509, 0x00043108, 0x00048D15, 0x0005BD41, 0x000849B2, 0x00000259, 0x00071E0F, 0x00055988, 0x00047135, 0x0004310F, 0x00044D0E, 0x00049D1A, 0x0005C942, 0x000839B5, 0x00000251, 0x00072A14, 
0x0005618C, 0x00047D36, 0x00045517, 0x00046918, 0x0004BD1F, 0x0005E549, 0x000841BA, 0x00000251, 0x0007521F, 0x00057D95, 0x0004A53F, 0x00047D20, 0x00048520, 0x0004E129, 0x00060151, 0x00088DC3, 
0x00000260, 0x00079632, 0x0005A5A1, 0x0004C947, 0x0004A52B, 0x0004AD2A, 0x0004FD33, 0x0006355B, 0x0008ADCF, 0x00000263, 0x0007DA44, 0x0005E9B0, 0x0004ED51, 0x0004CD35, 0x0004D532, 0x00052539, 
0x0006716D, 0x0008F1E1, 0x00000282, 0x00082A40, 0x00062DBF, 0x0005155E, 0x0004E13B, 0x0004F13A, 0x0005593E, 0x0006B57D, 0x000931F1, 0x000002A9, 0x00089246, 0x00066DD2, 0x00054D71, 0x0004ED42, 
0x0005053E, 0x00059D4B, 0x0007018F, 0x0009820A, 0x000002D3, 0x0008E668, 0x0006C1E4, 0x00059984, 0x00052952, 0x00055950, 0x0005F562, 0x000745A4, 0x0009D224, 0x00000301, 0x00093A98, 0x00071E05, 
0x0005F59A, 0x00058569, 0x0005B165, 0x00064579, 0x0007B9B7, 0x000A6E39, 0x0000032C, 0x0009B2DB, 0x0007822A, 0x000651B6, 0x0005FD80, 0x00060980, 0x0006A18E, 0x000845CB, 0x000B264F, 0x00000355, 
0x000A5316, 0x0007F242, 0x00069DC7, 0x00064594, 0x00065992, 0x0006E9A1, 0x000895E2, 0x000BC65F, 0x0000037E,
// B channel
0x00087A70, 0x0005B5B5, 0x0004A945, 0x0004491C, 0x00045D13, 0x0004F127, 0x00069160, 0x000A3208, 0x00000350, 0x0008064C, 0x000595AC, 0x0004893D, 0x00042512, 0x0004390A, 0x0004C91E, 0x00064554, 
0x000995E7, 0x0000030A, 0x0007AA41, 0x0005699B, 0x00047134, 0x00041D0A, 0x00042508, 0x0004A516, 0x00060549, 0x00090DCF, 0x000002C1, 0x0007522A, 0x00056190, 0x00046530, 0x0004190A, 0x00042106, 
0x00049911, 0x0005D943, 0x0008B9BE, 0x0000028B, 0x0007261B, 0x0005558B, 0x0004592F, 0x00041107, 0x00042905, 0x00049511, 0x0005BD42, 0x00087DB4, 0x0000026A, 0x00072610, 0x00054989, 0x00046131, 
0x00041509, 0x00043108, 0x00048D15, 0x0005BD41, 0x000849B2, 0x00000259, 0x00071E0F, 0x00055988, 0x00047135, 0x0004310F, 0x00044D0E, 0x00049D1A, 0x0005C942, 0x000839B5, 0x00000251, 0x00072A14, 
0x0005618C, 0x00047D36, 0x00045517, 0x00046918, 0x0004BD1F, 0x0005E549, 0x000841BA, 0x00000251, 0x0007521F, 0x00057D95, 0x0004A53F, 0x00047D20, 0x00048520, 0x0004E129, 0x00060151, 0x00088DC3, 
0x00000260, 0x00079632, 0x0005A5A1, 0x0004C947, 0x0004A52B, 0x0004AD2A, 0x0004FD33, 0x0006355B, 0x0008ADCF, 0x00000263, 0x0007DA44, 0x0005E9B0, 0x0004ED51, 0x0004CD35, 0x0004D532, 0x00052539, 
0x0006716D, 0x0008F1E1, 0x00000282, 0x00082A40, 0x00062DBF, 0x0005155E, 0x0004E13B, 0x0004F13A, 0x0005593E, 0x0006B57D, 0x000931F1, 0x000002A9, 0x00089246, 0x00066DD2, 0x00054D71, 0x0004ED42, 
0x0005053E, 0x00059D4B, 0x0007018F, 0x0009820A, 0x000002D3, 0x0008E668, 0x0006C1E4, 0x00059984, 0x00052952, 0x00055950, 0x0005F562, 0x000745A4, 0x0009D224, 0x00000301, 0x00093A98, 0x00071E05, 
0x0005F59A, 0x00058569, 0x0005B165, 0x00064579, 0x0007B9B7, 0x000A6E39, 0x0000032C, 0x0009B2DB, 0x0007822A, 0x000651B6, 0x0005FD80, 0x00060980, 0x0006A18E, 0x000845CB, 0x000B264F, 0x00000355, 
0x000A5316, 0x0007F242, 0x00069DC7, 0x00064594, 0x00065992, 0x0006E9A1, 0x000895E2, 0x000BC65F, 0x0000037E,
};

const _Sensor_LSC       gc2053_lsc_init = {
    .p_lsc_tbl = (uint32 *)gc2053_lsc_tbl,
};

const _Sensor_LHS gc2053_lhs_map[] = {
    // region defination: lower -> center -> upper(direction: anticlockwise)
    // region_lower, region_center, region_upper, hue adjust value, saturation adjust value
    //   (9 bits)      (9 bits)       (9 bits)          (9 bits)           (8 bits)
    {            24,            52,           80,                0,                       0},  // magenta,          range: 28
    {            80,           109,          138,                0,                       0},  // red,              range: 29
    {           140,           171,          202,                0,                       0},  // yellow,           range: 31
    {           204,           232,          260,                0,                       0},  // green,            range: 28
    {           261,           289,          317,                0,                       0},  // cyan,             range: 28
    {           320,           351,           22,                0,                       0},  // blue,             range: 31
    {           109,           132,          156,                0,                       0},  // skin enhance,     range:
    {           160,           203,          247,              +32,                       0},  // green enhance(plants),    range:
    {           296,           318,          340,                0,                       0}   // blue enhance,     range:
};

const _Sensor_WDR gc2053_wdr_init = {
    .wdr_bv           = {791, 1599, 3534,  7000, 10000, 14000, 28000, 56000},
    .max_ns_slope     = {1.0,  1.0,  1.0,   1.0,  1.25,   1.5,   2.0,   3.0},
    .max_shadow_slope = {1.0,  1.0,  1.0,   1.0,   1.0,  1.25,   1.5,   1.5},
};

uint8 gc2053_regValTable[29][4] = {
    // 0xb4  0xb3 0xb8 0xb9
    {0x00, 0x00, 0x01, 0x00},
    {0x00, 0x10, 0x01, 0x0c},
    {0x00, 0x20, 0x01, 0x1b},
    {0x00, 0x30, 0x01, 0x2c},
    {0x00, 0x40, 0x01, 0x3f},
    {0x00, 0x50, 0x02, 0x16},
    {0x00, 0x60, 0x02, 0x35},
    {0x00, 0x70, 0x03, 0x16},
    {0x00, 0x80, 0x04, 0x02},
    {0x00, 0x90, 0x04, 0x31},
    {0x00, 0xa0, 0x05, 0x32},
    {0x00, 0xb0, 0x06, 0x35},
    {0x00, 0xc0, 0x08, 0x04},
    {0x00, 0x5a, 0x09, 0x19},
    {0x00, 0x83, 0x0b, 0x0f},
    {0x00, 0x93, 0x0d, 0x12},
    {0x00, 0x84, 0x10, 0x00},
    {0x00, 0x94, 0x12, 0x3a},
    {0x01, 0x2c, 0x1a, 0x02},
    {0x01, 0x3c, 0x1b, 0x20},
    {0x00, 0x8c, 0x20, 0x0f},
    {0x00, 0x9c, 0x26, 0x07},
    {0x02, 0x64, 0x36, 0x21},
    {0x02, 0x74, 0x37, 0x3a},
    {0x00, 0xc6, 0x3d, 0x02},
    {0x00, 0xdc, 0x3f, 0x3f},
    {0x02, 0x85, 0x3f, 0x3f},
    {0x02, 0x95, 0x3f, 0x3f},
    {0x00, 0xce, 0x3f, 0x3f},
};

uint32 gc2053_gainLevelTable[30] = {
    64,
    74,
    89,
    102,
    127,
    147,
    177,
    203,
    260,
    300,
    361,
    415,
    504,
    581,
    722,
    832,
    1027,
    1182,
    1408,
    1621,
    1990,
    2291,
    2850,
    3282,
    4048,
    5180,
    5500,
    6744,
    7073,
    0xffffffff
};

void GC2053_ae_adjust(struct isp_ae_func_cfg *p_cfg)
{
    int i;
    int    gc2053_total;
    uint32 tol_dig_gain = 0;
    uint8  index        = 0;
    uint32 gain         = (p_cfg->analog_gain) >> 2;
    uint8  *addr        = (uint8 *)p_cfg->data.addr;

    gc2053_total = sizeof(gc2053_gainLevelTable) / sizeof(uint32);

    for (i = 0; i < gc2053_total; i++)
    {
        if ((gc2053_gainLevelTable[i] <= gain) && (gain < gc2053_gainLevelTable[i + 1]))
            break;
    }

    tol_dig_gain = gain * 64 / gc2053_gainLevelTable[i];

    addr[index++] = 0xfe;
    addr[index++] = 0x00;
    addr[index++] = 0xb4;
    addr[index++] = gc2053_regValTable[i][0];
    addr[index++] = 0xb3;
    addr[index++] = gc2053_regValTable[i][1];
    addr[index++] = 0xb8;
    addr[index++] = gc2053_regValTable[i][2];
    addr[index++] = 0xb9;
    addr[index++] = gc2053_regValTable[i][3];
    addr[index++] = 0xb1;
    addr[index++] = (tol_dig_gain >> 6);
    addr[index++] = 0xb2;
    addr[index++] = ((tol_dig_gain & 0x3f) << 2);
    addr[index++] = 0xfe;
    addr[index++] = 00;
    addr[index++] = 0x03;
    addr[index++] = (p_cfg->exposure_line >> 8);
    addr[index++] = 0x04;
    addr[index++] = (p_cfg->exposure_line & 0xff);
    p_cfg->data.size = index;
    p_cfg->cmd_len   = 1+1;
}

const _Sensor_ISP_Init gc2053_isp_init = 
{
    .type         = ISP_INPUT_DAT_SRC_MIPI0,
    .pixel_h      = 1080,
    .pixel_w      = 1920,
    .bayer_patten = ISP_BAYER_FORMAT_GRBG,
    .input_format = ISP_INPUT_DAT_FORMAT_RAW10,
    .adjust_func  = (isp_ae_func     )GC2053_ae_adjust,
    .p_blc        = (_Sensor_BLC    *)&gc2053_blc_init,
    .p_ccm        = (_Sensor_CCM    *)&gc2053_ccm_init,
    .p_awb        = (_Sensor_AWB    *)&gc2053_awb_init,
    .p_ae         = (_Sensor_AE     *)&gc2053_ae_init,
    .p_csupp      = (_Sensor_CSUPP  *)&gc2053_csupp_init,
    .p_sharp      = (_Sensor_SHARP  *)&gc2053_sharp_init,
    .p_yuvnr      = (_Sensor_YUVNR  *)&gc2053_yuvnr_init,
    .p_colenh     = (_Sensor_COLENH *)&gc2053_colenh_init,       
    .p_bv2nr      = (_Sensor_BV2NR  *)gc2053_bv2nr_init,
    .p_lsc        = (_Sensor_LSC    *)&gc2053_lsc_init,
    .p_lhs        = (_Sensor_LHS    *)gc2053_lhs_map,
    // .p_ygamma     = (_Sensor_YGAMMA *)gc2083_ygamma_tbl,
    .p_wdr        = (_Sensor_WDR    *)&gc2053_wdr_init,
};

SENSOR_OP_SECTION const _Sensor_Adpt_ gc2053_cmd = 
{
	.typ = 1, //YUV
	.pixelw = 1920,
	.pixelh= 1080,
	.hsyn = 1,
	.vsyn = 1,
	.rduline = 0,//
	.rawwide = 1,//10bit
	.colrarray = 1,//0:_RGRG_ 1:_GRGR_,2:_BGBG_,3:_GBGB_
	.init = (uint8 *)GC2053InitTable,
    .init_len = sizeof(GC2053InitTable),
    .mipi_lane_num = 2,
	.rotate_adapt = {0},
	//.hvb_adapt = {0x6a,0x12,0x6a,0x12},
	. mclk = 12000000,
	.p_fun_adapt = {NULL,NULL,NULL},
    .sensor_isp = (_Sensor_ISP_Init *)&gc2053_isp_init,
};

const _Sensor_Ident_ gc2053_init =
{
	0x53, 0x6e, 0x6f, 0x01, 0x01, 0x00f1
};

#endif
