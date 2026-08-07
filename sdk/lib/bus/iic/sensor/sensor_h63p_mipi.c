#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/isp.h"

#if DEV_SENSOR_H63P


SENSOR_INIT_SECTION const unsigned char H63PInitTable[CMOS_INIT_LEN]= 
{	
	0x12,0x40,
	0x48,0x85,
	0x48,0x05,
	0x0E,0x11,
	0x0F,0x84,
	0x10,0x1E,
	0x11,0x80,
	0x57,0x60,
	0x58,0x18,
	0x61,0x10,
	0x46,0x08,
	0x0D,0xA0,
	0x20,0xC0,
	0x21,0x03,
	0x22,0xEE,
	0x23,0x02,
	0x24,0x80,
	0x25,0xD0,
	0x26,0x22,
	0x27,0x36,
	0x28,0x15,
	0x29,0x03,
	0x2A,0x2B,
	0x2B,0x13,
	0x2C,0x00,
	0x2D,0x00,
	0x2E,0xBA,
	0x2F,0x60,
	0x41,0x84,
	0x42,0x02,
	0x47,0x46,
	0x76,0x40,
	0x77,0x06,
	0x80,0x01,
	0xAF,0x22,
	0x8A,0x00,
	0xA6,0x00,
	0x8D,0x49,
	0xAB,0x00,
	0x1D,0x00,
	0x1E,0x04,
	0x6C,0x50,
	0x9E,0xF8,
	0x6E,0x2C,
	0x70,0x8C,
	0x71,0x6D,
	0x72,0x6A,
	0x73,0x46,
	0x74,0x02,
	0x78,0x8E,
	0x89,0x01,
	0x6B,0x20,
	0x86,0x40,
	0x9C,0xE1,
	0x3A,0xAC,
	0x3B,0x18,
	0x3C,0x5D,
	0x3D,0x80,
	0x3E,0x6E,
	0x31,0x07,
	0x32,0x14,
	0x33,0x12,
	0x34,0x1C,
	0x35,0x1C,
	0x56,0x12,
	0x59,0x20,
	0x85,0x14,
	0x64,0xD2,
	0x8F,0x90,
	0xA4,0x87,
	0xA7,0x80,
	0xA9,0x48,
	0x45,0x01,
	0x5B,0xA0,
	0x5C,0x6C,
	0x5D,0x44,
	0x5E,0x81,
	0x63,0x0F,
	0x65,0x12,
	0x66,0x43,
	0x67,0x79,
	0x68,0x00,
	0x69,0x78,
	0x6A,0x28,
	0x7A,0x66,
	0xA5,0x03,
	0x94,0xC0,
	0x13,0x81,
	0x96,0x84,
	0xB7,0x4A,
	0x4A,0x01,
	0xB5,0x0C,
	0xA1,0x0F,
	0xA3,0x40,
	0xB1,0x00,
	0x93,0x00,
	0x7E,0x4C,
	0x50,0x02,
	0x49,0x10,
	0x8E,0x40,
	0x7F,0x56,
	0x0C,0x00,
	0xBC,0x11,
	0x82,0x00,
	0x19,0x20,
	0x1F,0x10,
	0x1B,0x4F,
	0x12,0x00,
    0xff, 0xff,
};

const _Sensor_CCM h63p_ccm_init = {
    // 347,  -148,  -136,
	// 219,  631, -224,
    // -310, -227,  616,

    479,   -72,	    -47,
    -126,	488,	-184,
    -98, 	-160,	486,
    0,      0,      0,

};

const _Sensor_BLC h63p_blc_init =
{
    64, 64, 64, 64,
};

const _Sensor_AWB h63p_awb_init = 
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


//    .constraint = {
//		.section_num = 4,
//		//   色温:   7500, 6500, 5000, 4000, 3000
//		// rgain:    542,  531,  466,  427,  349
//		// bgain:    433,  484,  533,  566,  695
//		// 下范围,上范围
//		// 下,上
//		//    15,   10
//		//    15,   15
//		//    20,   15
//		//    15,   15
//		//    15,   10
//		.color_temp          = {         6500,        5000,        4000,        3000,           0,           0,           0},
//		.sec_line_slope      = {   0.71428571,   1.5789474,   1.6206897,   1.5909091,           0,           0,           0},
//		.sec_line_offset     = {    47.142857,  -99.631579,  -131.96552,  -205.09091,           0,           0,           0},
//		.sec_line_sqrtk2add1 = {   0.81373347,  0.53505208,  0.52510733,  0.53217155,           0,           0,           0},
//		.center_line_slope   = {  -0.66666667, -0.58333333, -0.62857143,           0,           0,           0},
//		.center_line_offset  = {          217,      205.25,   212.17143,           0,           0,           0},
//		.lower_line_slope    = {   -1.0344963, -0.19354257, -0.62858901,           0,           0,           0},
//		.lower_line_offset   = {     240.8974,   131.32201,   194.45743,           0,           0,           0},
//		.upper_line_slope    = {  -0.58222977,   -0.582807, -0.81057448,           0,           0,           0},
//		.upper_line_offset   = {    222.43953,   222.52555,   259.16802,           0,           0,           0},
//		.corner_limit        = {     149.13948,    130.92278,    152.24035,    180.01743,    81.300452,    193.32172,    102.46637},
//    },

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

const _Sensor_AE h63p_ae_init = 
{
    .max_frame_length      = 740,//
    .min_frame_vb          = 4,//
    .max_analog_gain       = 7936,//
    .min_analog_gain       =  1<<8,
    .default_exposure_line = 720,
    .max_exposure_line     = 720,
    .min_exposure_line     = 4,
    .row_time_us           = 50,
    .expo_frame_interval   = 3,
    .to_day_bv             = 669,
    .to_night_bv           = 197,
    .dark_scene_target_lut = {50, 55},
    .dark_scene_bv_lut     = {47, 197},
    .hs_scene_limit_lut    = {55, 75},
    .hs_scene_bv_lut       = {197, 2784},
    .lowlight_lsb_bv_lut   = {72, 88, 129, 197, 347,  669, 1391, 1e30},
    .lowlight_lsb_gain_lut = {64, 64,  48,  40,  32,   24,   16,   16},  // u7.4
};

const _Sensor_CSUPP h63p_csupp_init = {
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

const _Sensor_SHARP h63p_sharp_init = {
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

const _Sensor_YUVNR h63p_yuvnr_init = {
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

const _Sensor_COLENH h63p_colenh_init = {
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
    .adj_sat_by_bv = 1,
    .hi_sat        = 60,
    .lo_sat        = 50,
    .hi_sat_bv     = 1391, // high brightness for saturation
    .lo_sat_bv     = 347, // low brightness for saturation
};

const _Sensor_BV2NR h63p_bv2nr_init[BV2RAWNR_ARRAY_NUM] = {
    //           bv, bnr_range_weight_index, bnr_invksigma, bnr_intensity_threshold, yuvnr_idx, csupp_idx
    {         24647,                      8,           511,                      63,         0,         0},    // 320lux
    {          2784,                      8,           511,                      63,         0,         0},    // 40lux
    {          1391,                      8,           350,                      63,         0,         0},    // 20lux
    {           669,                      8,           260,                      63,         1,         0},    // 10lux
    {           347,                      8,           260,                      63,         2,         1},    // 5p03lux
    {           197,                      8,           260,                      63,         3,         1},    // 2p5lux
    {           129,                      8,           200,                      63,         3,         2},    // 1p25lux
    {            88,                     19,           160,                      63,         3,         2},    // 0p62lux
    {            72,                     19,           128,                      63,         3,         2},    // 0p31lux
    {            64,                     19,           128,                      63,         3,         2},    // 0p1lux
    {             0,                     19,           128,                      63,         3,         2},    // 0p01lux
};
const uint32 h63p_lsc_tbl[] = {
0x0007560F, 0x00059D97, 0x0004A143, 0x00041113, 0x00041105, 0x00045D0D, 0x00057535, 0x0007358F, 0x00000216, 0x0007520D, 0x00059195, 0x00049942, 0x00041112, 0x00041D04, 0x0004690B, 0x00057534, 
0x0007358F, 0x0000020C, 0x00073606, 0x00059192, 0x00049943, 0x00041510, 0x00041104, 0x0004690A, 0x00057135, 0x0007198C, 0x00000208, 0x00073202, 0x00059592, 0x00049D41, 0x00041D12, 0x00041D06, 
0x0004790C, 0x00056D36, 0x00071589, 0x00000204, 0x00073600, 0x0005A592, 0x0004AD46, 0x00043517, 0x00042909, 0x00048510, 0x0005753C, 0x0007158B, 0x00000200, 0x00072DFF, 0x00059D93, 0x0004AD45, 
0x00044119, 0x0004310C, 0x00049113, 0x0005793C, 0x0006FD89, 0x000001FB, 0x000725FC, 0x00059594, 0x0004B545, 0x0004511D, 0x0004390D, 0x00049D17, 0x0005793D, 0x0006F987, 0x000001FB, 0x000725FD, 
0x0005A595, 0x0004C949, 0x00045922, 0x00044D11, 0x0004AD1D, 0x00057D42, 0x0007018A, 0x000001F9, 0x00073E03, 0x0005B19A, 0x0004D94E, 0x00046525, 0x00046115, 0x0004BD20, 0x00058D46, 0x0006FD8C, 
0x000001F8, 0x0007760F, 0x0005E1A4, 0x0004F958, 0x0004892C, 0x0004851E, 0x0004E12B, 0x0005A94A, 0x00071D90, 0x000001FD, 0x0007B21E, 0x00061DB3, 0x00052564, 0x0004B53A, 0x0004BD2A, 0x00051536, 
0x0005D558, 0x0007519E, 0x0000020C, 0x0007E62C, 0x00064DC1, 0x00055571, 0x0004E544, 0x0004E936, 0x00053542, 0x00060163, 0x000789A9, 0x0000021C, 0x00083E4A, 0x000699D5, 0x00059981, 0x00052153, 
0x00052546, 0x00057551, 0x00063D6F, 0x0007CDB8, 0x00000231, 0x0008B266, 0x0006E9EA, 0x0005D593, 0x00056564, 0x00055D54, 0x0005B160, 0x00067D7E, 0x000815C8, 0x00000249, 0x0008FE7D, 0x00072DFB, 
0x000615A3, 0x0005A173, 0x00059563, 0x0005E96F, 0x0006B98E, 0x000851D9, 0x00000258, 0x00096E9C, 0x00078214, 0x000659B6, 0x0005E586, 0x0005D974, 0x00062D7E, 0x000709A1, 0x0008ADEE, 0x00000271, 
0x0009CEBA, 0x0007DA2D, 0x0006ADCD, 0x00061994, 0x00062185, 0x0006758F, 0x000761B6, 0x00090E04, 0x0000028C, 

0x000635B9, 0x00050562, 0x00045929, 0x00040108, 0x00040501, 0x00043504, 0x0004ED20, 0x0006195D, 0x000001B6, 0x000631B5, 0x00050962, 0x00045D2A, 0x00040509, 0x00040901, 0x00043905, 0x0004E921, 
0x0006095B, 0x000001AF, 0x000631B5, 0x00051163, 0x0004692C, 0x0004090A, 0x00041102, 0x00044106, 0x0004E922, 0x0006055A, 0x000001AD, 0x000639B5, 0x00051D64, 0x0004712F, 0x0004150C, 0x00041506, 
0x00044D09, 0x0004F525, 0x0006015B, 0x000001AB, 0x000645B8, 0x00052D69, 0x00048534, 0x00043113, 0x0004250A, 0x0004610D, 0x0005052A, 0x0006095E, 0x000001AD, 0x000645B8, 0x00053569, 0x00049135, 
0x00044117, 0x00042D0D, 0x00047110, 0x0005092D, 0x0006055E, 0x000001AB, 0x000649B8, 0x0005396B, 0x00049D37, 0x0004511B, 0x00043D10, 0x00048514, 0x00051531, 0x00060961, 0x000001AB, 0x000659BD, 
0x0005496F, 0x0004AD3C, 0x00046520, 0x00045514, 0x0004991B, 0x00052535, 0x00061564, 0x000001AE, 0x00066DC1, 0x00056174, 0x0004C140, 0x00047124, 0x00046518, 0x0004A920, 0x00053138, 0x00062166, 
0x000001AE, 0x000699CC, 0x0005857E, 0x0004E94B, 0x0004912C, 0x00048D20, 0x0004C929, 0x00054D40, 0x00063D6E, 0x000001B6, 0x0006D5DB, 0x0005B98D, 0x00051556, 0x0004B936, 0x0004BD2C, 0x0004F134, 
0x00057149, 0x00066D78, 0x000001C1, 0x000709EC, 0x0005E998, 0x00053962, 0x0004E941, 0x0004E536, 0x00051D3E, 0x00059552, 0x00069981, 0x000001CF, 0x00075A01, 0x000629AA, 0x00057972, 0x00052550, 
0x00052146, 0x0005514D, 0x0005C95F, 0x0006D18E, 0x000001E1, 0x0007B215, 0x000671BE, 0x0005BD82, 0x00055D61, 0x00055955, 0x0005895B, 0x0005FD6C, 0x0007119C, 0x000001F3, 0x0007F629, 0x0006B1CD, 
0x0005F592, 0x00059970, 0x00059163, 0x0005BD68, 0x0006357B, 0x000749A9, 0x00000205, 0x00084E42, 0x0006F9E4, 0x00063DA4, 0x0005DD81, 0x0005D173, 0x00060178, 0x00067D8B, 0x000791BC, 0x00000216, 
0x0008B65E, 0x00074DFA, 0x00068DB7, 0x00062993, 0x00061585, 0x0006498A, 0x0006CD9F, 0x0007E5D2, 0x00000222, 

0x000645BF, 0x00051566, 0x0004652D, 0x0004090B, 0x00040100, 0x00042902, 0x0004C918, 0x0005C94F, 0x0000019D, 0x000645BC, 0x00051968, 0x0004692E, 0x00040D0C, 0x00040902, 0x00042D03, 0x0004CD1B, 
0x0005CD4F, 0x00000198, 0x000649BC, 0x00052568, 0x00047530, 0x0004150E, 0x00040D04, 0x00043905, 0x0004D51E, 0x0005D552, 0x0000019A, 0x00064DBB, 0x0005316A, 0x00048134, 0x00042111, 0x00041D08, 
0x00044909, 0x0004E523, 0x0005D954, 0x0000019D, 0x00065DC0, 0x00053D6F, 0x00049538, 0x00043D17, 0x0004310D, 0x0004650E, 0x00050129, 0x0005F15A, 0x000001A2, 0x000661C1, 0x00054570, 0x0004A13A, 
0x00044D1B, 0x00043910, 0x00047512, 0x00050D2D, 0x0005F95C, 0x000001A5, 0x000665C2, 0x00054D72, 0x0004AD3C, 0x00045D1F, 0x00044914, 0x00048D18, 0x00051D32, 0x00060D60, 0x000001A9, 0x000679C7, 
0x00056176, 0x0004C541, 0x00047524, 0x00046519, 0x0004A51E, 0x00053138, 0x00062566, 0x000001B0, 0x000689CB, 0x0005757B, 0x0004D546, 0x00048529, 0x0004791D, 0x0004BD24, 0x0005453E, 0x0006356A, 
0x000001B3, 0x0006C1D6, 0x00059D87, 0x0004F950, 0x0004A131, 0x0004A125, 0x0004E12E, 0x00056946, 0x00066575, 0x000001C0, 0x0006F9E6, 0x0005D194, 0x0005255D, 0x0004D13C, 0x0004D131, 0x00050D39, 
0x00059951, 0x0006A182, 0x000001D1, 0x000729F5, 0x000605A1, 0x00055568, 0x0004FD48, 0x0005013D, 0x00053945, 0x0005C15C, 0x0006DD8D, 0x000001E1, 0x00077E0B, 0x000649B2, 0x00059579, 0x00053957, 
0x00053D4C, 0x00057554, 0x0005FD6A, 0x0007299E, 0x000001FB, 0x0007DA20, 0x000691C7, 0x0005D58B, 0x00057D67, 0x00057D5D, 0x0005B965, 0x00063D7A, 0x000775AF, 0x00000214, 0x00082235, 0x0006D1D9, 
0x00061999, 0x0005B978, 0x0005B56B, 0x0005F173, 0x0006818A, 0x0007C5C1, 0x0000022B, 0x00088252, 0x00071DEF, 0x00065DAC, 0x0006018A, 0x0006017D, 0x00064185, 0x0006D99E, 0x000825D6, 0x00000245, 
0x0008EA6C, 0x00077607, 0x0006ADC1, 0x00064D9C, 0x00064D8F, 0x00069599, 0x00073DB4, 0x00088DF3, 0x0000025A, 

0x0005D592, 0x0004E552, 0x00045123, 0x0003FD07, 0x0003ECFD, 0x000410FD, 0x00049912, 0x0005613E, 0x00000177, 0x0005D596, 0x0004F155, 0x00045527, 0x0004050A, 0x0003FD00, 0x00041D01, 0x0004A915, 
0x00057543, 0x00000177, 0x0005E596, 0x0005015A, 0x0004692A, 0x0004190D, 0x00040D05, 0x00043105, 0x0004BD1C, 0x00058145, 0x00000180, 0x0005F99C, 0x0005155D, 0x00047D30, 0x00043112, 0x00042109, 
0x00044D0A, 0x0004D121, 0x0005994B, 0x00000182, 0x0006119F, 0x00053162, 0x00049936, 0x00044518, 0x00043510, 0x00046D11, 0x0004F12A, 0x0005B953, 0x0000018D, 0x000611A0, 0x00053965, 0x0004A539, 
0x00045D1E, 0x00044913, 0x00048115, 0x0005012F, 0x0005CD59, 0x00000193, 0x000615A4, 0x00053967, 0x0004A93B, 0x00046D21, 0x00045517, 0x00049119, 0x00051134, 0x0005E95B, 0x00000199, 0x00062DA9, 
0x0005516D, 0x0004C141, 0x00048126, 0x0004691C, 0x0004B521, 0x0005313A, 0x00061166, 0x000001A3, 0x000645AE, 0x00056570, 0x0004D544, 0x0004912A, 0x0004851F, 0x0004C525, 0x00054940, 0x00061D6B, 
0x000001AA, 0x000671B6, 0x0005917C, 0x0004F94F, 0x0004A933, 0x0004A927, 0x0004E52E, 0x00057147, 0x00065976, 0x000001B8, 0x0006A9C9, 0x0005BD86, 0x0005215B, 0x0004D13E, 0x0004D131, 0x0005193A, 
0x00059D54, 0x00069584, 0x000001CC, 0x0006E5D7, 0x0005ED93, 0x00055165, 0x00050146, 0x0005013C, 0x00054545, 0x0005D15E, 0x0006D591, 0x000001DF, 0x00072DEC, 0x00062DA7, 0x00058D75, 0x00053555, 
0x0005394D, 0x00057D54, 0x0006056B, 0x0007299F, 0x000001F8, 0x00078A02, 0x000675B9, 0x0005D185, 0x00057D66, 0x00056D5B, 0x0005B564, 0x0006417C, 0x000779B0, 0x00000211, 0x0007CA14, 0x0006B9CA, 
0x00060193, 0x0005AD73, 0x0005AD69, 0x0005F171, 0x0006858B, 0x0007BDC2, 0x0000022B, 0x00082228, 0x0006F9DC, 0x000655A6, 0x0005F587, 0x0005F17A, 0x00064183, 0x0006E59E, 0x00082DDB, 0x00000246, 
0x00087A44, 0x000749F5, 0x00069DB9, 0x00063597, 0x0006398A, 0x00069196, 0x000735B5, 0x0008A9F7, 0x0000025C, 
};

const _Sensor_LSC          h63p_lsc_init = {
    .p_lsc_tbl = (uint32 *)h63p_lsc_tbl,
};

const _Sensor_LHS h63p_lhs_map[] = {
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

const _Sensor_YGAMMA h63p_ygamma_tbl[] = {
    0x01002000, 0x02006010, 0x0300A020, 0x0400E030, 0x05012040, 0x06016050, 0x0701A060, 0x0801E070, 
    0x09022080, 0x0A026090, 0x0B02A0A0, 0x0C02E0B0, 0x0D0320C0, 0x0E0360D0, 0x0F03A0E0, 0x1003E0F0, 
    0x11042100, 0x12046110, 0x1304A120, 0x1404E130, 0x15052140, 0x16056150, 0x1705A160, 0x1805E170, 
    0x19062180, 0x1A066190, 0x1B06A1A0, 0x1C06E1B0, 0x1D0721C0, 0x1E0761D0, 0x1F07A1E0, 0x2007E1F0, 
    0x21082200, 0x22086210, 0x2308A220, 0x2408E230, 0x25092240, 0x26096250, 0x2709A260, 0x2809E270, 
    0x290A2280, 0x2A0A6290, 0x2B0AA2A0, 0x2C0AE2B0, 0x2D0B22C0, 0x2E0B62D0, 0x2F0BA2E0, 0x300BE2F0, 
    0x310C2300, 0x320C6310, 0x330CA320, 0x340CE330, 0x350D2340, 0x360D6350, 0x370DA360, 0x380DE370, 
    0x390E2380, 0x3A0E6390, 0x3B0EA3A0, 0x3C0EE3B0, 0x3D0F23C0, 0x3E0F63D0, 0x3F0FA3E0, 0x3FFFE3F0, 
};

uint32 h63p_gainLevelTable[] = {
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


void h63p_ae_adjust(struct isp_ae_func_cfg *p_cfg)
{

    uint32 index        = 0;
    uint32 tol_dig_gain = 0;
    uint8  *addr        = (uint8 *)p_cfg->data.addr;
	int   h63p_total  = sizeof(h63p_gainLevelTable) / sizeof(uint32);
	uint16 gain = (p_cfg->analog_gain<<2);
    for(uint16 i=0; i<h63p_total; i++)
    {
        if(h63p_gainLevelTable[i] >= gain)
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

const _Sensor_ISP_Init h63p_isp_init = 
{
    .type         = ISP_INPUT_DAT_SRC_MIPI0,
    .pixel_h      = 720,
    .pixel_w      = 1280,
    .bayer_patten = ISP_BAYER_FORMAT_BGGR,
    .input_format = ISP_INPUT_DAT_FORMAT_RAW10,
    .adjust_func  = (isp_ae_func     )h63p_ae_adjust,
    .p_blc        = (_Sensor_BLC    *)&h63p_blc_init,
    .p_ccm        = (_Sensor_CCM    *)&h63p_ccm_init,
    .p_awb        = (_Sensor_AWB    *)&h63p_awb_init,
    .p_ae         = (_Sensor_AE     *)&h63p_ae_init,
    .p_csupp      = (_Sensor_CSUPP  *)&h63p_csupp_init,
    .p_sharp      = (_Sensor_SHARP  *)&h63p_sharp_init,
    .p_yuvnr      = (_Sensor_YUVNR  *)&h63p_yuvnr_init,    
    .p_colenh     = (_Sensor_COLENH *)&h63p_colenh_init,	
    .p_bv2nr      = (_Sensor_BV2NR  *)&h63p_bv2nr_init,
    .p_lsc        = (_Sensor_LSC    *)&h63p_lsc_init,
    .p_lhs        = (_Sensor_LHS    *)h63p_lhs_map,
    .p_ygamma     = (_Sensor_YGAMMA *)h63p_ygamma_tbl,
};

SENSOR_OP_SECTION const _Sensor_Adpt_ h63p_cmd= 
{	
	.typ = 1, //YUV
	.pixelw = 1280,
	.pixelh= 720,
	.hsyn = 1,
	.vsyn = 0,
	.rduline = 0,//
	.rawwide = 1,//10bit
	.colrarray = 2,//0:_RGRG_ 1:_GRGR_,2:_BGBG_,3:_GBGB_
	.init = (uint8 *)H63PInitTable,
	.rotate_adapt = {0},
	.hvb_adapt = {0x80,0x0a,0x80,0x0a},
	. mclk = 24000000,
	.p_fun_adapt = {NULL,NULL,NULL},
    .sensor_isp = (_Sensor_ISP_Init *)&h63p_isp_init,
};

const _Sensor_Ident_ h63p_init=
{
	0x48,0x80,0x81,0x01,0x01,0x0b
};


#endif
