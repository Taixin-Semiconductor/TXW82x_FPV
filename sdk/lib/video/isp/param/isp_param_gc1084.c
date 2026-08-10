// isp_param.c
#include "sys_config.h"
#include "typesdef.h"
#include "osal/string.h"
#include "hal/isp_param.h"
#include "hal/isp.h"
#include "lib/video/dvp/cmos_sensor/csi.h"


#define GC1084_MASTER_SENSOR	1
#define GC1084_SLAVE0_SENSOR	0
#define GC1084_SLAVE1_SENSOR	0


#if GC1084_MASTER_SENSOR
/* ============================================================
 * GC1084 Master 静态标定参数
 * ============================================================ */

/* ---------- Enable 参数 ---------- */
static const _ISP_FUNC gc1084_master_enable = {
    .test_pattern_en    = 0,
    .dma_flush_en       = 1,
    .blc_en             = 1,
    .awb_en             = 1,
    .ae_en              = 1,
    .hist_en            = 1,
    .ccm_en             = 1,
    .y_gamma_en         = 1,
    .rgb_gamma_en       = 1,
    .ce_en              = 1,
    .sharpen_en         = 1,
    .bnr_en             = 1,
    .ynr_en             = 1,
    .cnr_en             = 1,
    .csupp_en           = 1,
    .adj_hue_en         = 1,
    .lsc_en             = 1,
    .dpc_en             = 1,
    .af_en              = 0,
    .dehaze_en          = 0,
    .gic_en             = 0,
    .md_en              = 0,
    .luma_ca_en         = 0,
};

/* ---------- AWB 参数 (TYPE_HGISP_CFG_AWB) ---------- */
static const TYPE_HGISP_CFG_AWB gc1084_master_awb = {
    .coarse_scale           = (uint32)(256*1.2),
    .coarse_thr             = 3 << 4,
    .fine_step              = 1 << 4, 
    .lock_hi_thr            = 4, 
    .lock_lo_thr            = 0, 
    .stable_thr             = 16, 
    .cbcr_thr               = 3 << 2, 
    .awb_auto_en            = 1, 
    .awb_meas_mode          = 2, 
    .cr_target              = 2048, 
    .cb_target              = 2048,
    .front_cr_val           = 30,
    .front_cb_val           = 30,
    .front_uv_sum           = 15, 
    .back_cr_val            = 30,
    .back_cb_val            = 30,
    .back_uv_sum            = 10, 
    .cons_cr_max            = 40,
    .cons_cb_max            = 40,
    .cons_uv_max            = 30,
    .cons_cr_min            = 10,
    .cons_cb_min            = 10,
    .cons_uv_min            =  2,
    .back_cr_max            = 30,
    .back_cb_max            = 30,
    .back_uv_max            = 10,
    .back_cr_min            = 5,
    .back_cb_min            = 5,
    .back_uv_min            = 5,
    .awb_wp_max             = 0xc0,
    .awb_wp_min             = 32, 
    .awb_r_max              = 0xc0, 
    .awb_g_max              = 0xc0, 
    .awb_b_max              = 0xc0, 
    .awb_precision          = 1,
    .awb_fine_cons_en       = 0,
    .awb_coarse_cons_en     = 0,
    .awb_back_cons_en       = 1,
    .awb_back_wp_min_ratio  = 0.2,
    .manual_gain            = {256, 256, 256, 256},
    .awb_crop_pixel_start_h	= 0,
    .awb_crop_pixel_start_v	= 0,
    .awb_crop_pixel_end_h 	= 0,
    .awb_crop_pixel_end_v 	= 0,

    .sensor_awb = {
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
    },

};

/* ---------- AE 参数 (struct hgisp_cfg_ae) ---------- */
static const struct hgisp_cfg_ae gc1084_master_ae = {
        .ae_manual_en              = 0,
        .luma_target               = 55, 
        .luma_weight_sum           = 610,
        .luma_weight               = { 20, 20, 20, 20, 20,
                                       20, 30, 30, 30, 20,
                                       20, 30, 50, 30, 20,
                                       20, 30, 30, 30, 20,
                                       20, 20, 20, 20, 20},
        .ae_crop_start_h           = 0,
        .ae_crop_start_v           = 0,
        .ae_crop_size_h            = 0,
        .ae_crop_size_v            = 0,
        .hist_crop_start_h         = 1,
        .hist_crop_start_v         = 1,
        .hist_crop_end_h           = 0,
        .hist_crop_end_v           = 0,
        .ae_lock_cnt               = 10,
        .ae_lock_tolerance         = 4,
        .ae_unlock_tolerance       = 12,
		.exposure_alpha            = 16,
        .reduce_fps_en             = 0,
        .lowlight_lsb_gain_en      = 0,
        .lowlight_lsb_gain_4hi_fps = 16,
        .lowlight_lsb_gain_4lo_fps = 16,
        .hist_hs_bin_thr           = 180,
        .hist_upper_hs_pixel_ratio = 0.94,
        .hist_upper_pixel_ratio    = 0.88,
        .hist_lower_pixel_ratio    = 0.00,

        .abl_bv_gain_sel 		   = 1,
        .abl_expo_line_low_ratio   = 0,
        .abl_expo_gain_low_thr     = 0,
        .abl_expo_line_high_ratio  = 0,
        .abl_expo_gain_high_thr    = 0,
        .aoe_expo_gain_thr[0]      = 65535,
        .aoe_expo_gain_thr[1]      = 65535,
        .abl_bv_thr[0]			   = 1e20,
        .abl_bv_thr[1]			   = 1e20,
        .aoe_bv_thr[0]			   = 0,
        .aoe_bv_thr[0]			   = 0,
        .abl_hist_thr[0]		   = 50,			// hist
        .abl_hist_thr[1]		   = 236,
        .dark_pixel_low_ratio	   = 0.60,
        .dark_pixel_high_ratio	   = 0.90,
        .bright_pixel_high_ratio   = 0.15,
        .bright_pixel_sub_ratio    = 0.05,
        .dark_pos_thr_max		   = 50,		    // position
        .bright_pos_adjust_ratio   = 0.90,
        .abl_dark_pos_low_wthr	   = 0.40 * 61,
        .abl_dark_pos_add_wthr	   = 0.10 * 61,
        .aoe_dark_pos_wthr		   = 0.60 * 61,
        .aoe_bright_pos_wthr	   = 0.20 * 61,
        .abl_luma_target_max       = 100,		    // stable
        .abl_diff_ratio			   = 0.05,
        .abl_dark_pos_diff_thr     = 0.15 * 61,
        .abl_bright_pos_diff_thr   = 0.10 * 61,
		
        .stg_mode                  = 0,
        .stg_ratio_slope           = 0.3*256,
        .stg_max_offset            = 20,
        
        .anti_flicker_en		   = 0,
        .flicker_freq 			   = 50,	
        .flicker_gain_th		   = 64<<8,
    
    .sensor_ae = {
        .dark_scene_target_lut = {50, 55},
        .dark_scene_bv_lut     = {47, 363},
        .hs_scene_limit_lut    = {55, 75},
        .hs_scene_bv_lut       = {363, 3022},
        .lowlight_lsb_bv_lut   = {47, 94, 195, 381, 781, 1636, 3225, 1e30},
        .lowlight_lsb_gain_lut = {16, 16,  16,  16,  16,   16,   16,   16},  // u7.4
    },
};

/* ---------- BLC ---------- */
static const _Sensor_BLC gc1084_master_blc = {
    .gr = 256, 
    .gb = 256, 
    .r  = 256, 
    .b  = 256,
};

/* ---------- CCM ---------- */
static const _Sensor_CCM gc1084_master_ccm = {
    480,  -95,  -64,
   -208,  456, -200,
    -16, -105,  520,
      0,    0,    0,
};

/* GC1084 Master Gamma 参数 */
const _Sensor_GAMMA_BV gc1084_master_gamma = 
{
    .adj_by_bv  = 1,
    .bv         = { 29491, 3534, 1599, 347, 222, 115, 57, 34 },
    .y_alpha    = { 255, 255, 192, 192, 128, 128, 64, 64 },
    .rgb_alpha  = { 255, 255, 192, 192, 128, 128, 64, 64 },
};

/* GC1084 Master CSC 参数 */
const _Sensor_CSC gc1084_master_csc = 
{
    .rgb2yuv_gamut         = ISP_YUV_GAMUT_BT709,
    .rgb2yuv_range         = ISP_YUV_RANGE_NARROW,
    .yuv2rgb_in_gamut      = ISP_YUV_GAMUT_BT709,
    .yuv2rgb_in_range      = ISP_YUV_RANGE_NARROW,
    .yuv2rgb_out_gamut     = ISP_YUV_GAMUT_BT709,
    .yuv2rgb_out_range     = ISP_YUV_RANGE_NARROW,
    .y_gamma_alpha         = 0xff,
    .rgb_gamma_alpha       = 0xff,
    .gamma_alpha_map       = (void *)&gc1084_master_gamma,
};

/* ---------- DPC ---------- */
static const _Sensor_DPC gc1084_master_dpc = {
    .static_psram_addr      = (uint32)0,
    .white_threshold        = 115,
    .black_threshold        = 115,
    .white_threshold_min    = 30,
    .black_threshold_min    = 30,
    .sensitivity_value      = 128,
    .dynamic_white_strength = 4,
    .dynamic_black_strength = 4,
};

/* ---------- CE ---------- */
static const _Sensor_BV2COLENH gc1084_master_ce = {
    .sensor_ce = {
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
    },
    .bv2colenh_map = {
        {.bv = 29491, .hue = 0, .luma = 50, .contrast = 56, .saturation = 57},
        {.bv =  3534, .hue = 0, .luma = 50, .contrast = 56, .saturation = 57},
        {.bv =  1599, .hue = 0, .luma = 50, .contrast = 56, .saturation = 57},
        {.bv =   347, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
        {.bv =   222, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
        {.bv =   115, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
        {.bv =    57, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
        {.bv =    34, .hue = 0, .luma = 50, .contrast = 56, .saturation = 50},
    },
};

/* ---------- SHARP ---------- */
static const _Sensor_SHARP gc1084_master_sharp = {
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

/* ---------- NR ---------- */
static const _Sensor_CSUPP gc1084_master_csupp[BV2CSUPP_ARRAY_NUM] = {
// u8  U_luma_thr_lo, U_luma_slop_lo, U_luma_shfb_lo, U_luma_gmin_lo,
// u8  U_luma_thr_hi, U_luma_slop_hi, U_luma_shfb_hi, U_luma_gmin_hi,
// u8  V_luma_thr_lo, V_luma_slop_lo, V_luma_shfb_lo, V_luma_gmin_lo,
// u8  V_luma_thr_hi, V_luma_slop_hi, V_luma_shfb_hi, V_luma_gmin_hi,
// u8  chroma_thr_lo, chroma_slop_lo, chroma_shfb_lo, chroma_gmin_lo,
// group is arranged from bright to dark
//group 0:
   { 31,   2,   0,   0,
    209,   4,   0,   0,
     31,   2,   0,   0,
    209,   4,   0,   0,
      0,   0,   0,   0},

//group 1:
    { 72,   2,   0,   0,
     209,   4,   0,   0,
      72,   2,   0,   0,
     209,   4,   0,   0,
      32,   2,   0,   1},
//group 2:
    { 72,   2,   0,   0,
     209,   4,   0,   0,
      72,   2,   0,   0,
     209,   4,   0,   0,
      64,   7,   1,   1},
};

static const _Sensor_YUVNR gc1084_master_yuvnr[BV2YUVNR_ARRAY_NUM] = {
// group is arranged from bright to dark
// group 0:
    {5,5,5,5,5,5,5,5,
    128,255,0},
// group 1:
    {10,10,10,10,10,10,10,10,
    128,255,0},
// group 2:
    {10,10,10,10,10,10,10,10,
    255,255,0},
// group 3:
    {10,10,10,10,10,10,10,10,
    255,255,1},
// group 4:
    {20,20,20,20,20,20,20,20,
    255,255,1},
// group 5:
    {30,30,30,30,30,30,30,30,
    255,255,1},
};

static const _Sensor_BV2NR gc1084_master_bv2nr[BV2RAWNR_ARRAY_NUM] = {
    //           ev, bnr_range_weight_index, bnr_invksigma, bnr_intensity_threshold, yuvnr_idx, csupp_idx, h264_3dnr_lev, h264_3dnr_en
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

/* ---------- GIC ---------- */
 static const _Sensor_GIC gc1084_master_gic = {
    .w_thres  = 14,
    .w_slope  = 16,
    .w_str    = 127,
    .mu_thres = 5,
    .mu_slope = 16,
    .reserved0 = 0,
    .reserved1 = 0,
    .reserved2 = 0,
};

/* ---------- LHS ---------- */
static const _Sensor_LHS_MAP gc1084_master_lhs = {
    .local_hue_sat_map = {
        // region defination: lower -> center -> upper(direction: anticlockwise)
        // region_lower, region_center, region_upper, hue adjust value, saturation adjust value
        //   (9 bits)      (9 bits)       (9 bits)          (9 bits)           (8 bits)
        {       24,            52,           80,                0,                 0},  // magenta,          range: 28
        {       80,           109,          138,                0,                 0},  // red,              range: 29
        {      140,           171,          202,                0,                 0},  // yellow,           range: 31
        {      204,           232,          260,                0,                 0},  // green,            range: 28
        {      261,           289,          317,                0,                 0},  // cyan,             range: 28
        {      320,           351,           22,                0,                 0},  // blue,             range: 31
        {      109,           132,          156,                0,                 0},  // skin enhance,     range:
        {      160,           203,          247,                0,                 0},  // green enhance(plants),    range:
        {      296,           318,          340,                0,                 0}   // blue enhance,     range:
    },
};



/* ---------- WDR 参数 (TYPE_HGISP_CFG_WDR) ---------- */
static const TYPE_HGISP_CFG_WDR gc1084_master_wdr = {
    .dynamic_gamma_en          = 0,
    .y_gamma_opt               = 0,
    .wdr_en                    = 0,
    .wdr_opt                   = 0,
    .temporal_smooth_alpha     = 0.1,
    .noise_floor               = 128,
    .noise_floor_out           = 128,
    .shadow_boost_target       = 512,
    .highlight_compress_target = 870,
    .auto_noise_floor_out      = 1,
    .min_ns_percentile         = 0.01,
    .max_ns_percentile         = 0.07,

    .sensor_wdr = {
        .wdr_bv           = {791, 1599, 3534, 7000, 10000, 14000, 28000, 56000},
        .max_ns_slope     = {1.0, 1.0, 1.0, 1.0, 1.25, 1.5, 2.0, 3.0},
        .max_shadow_slope = {1.0, 1.0, 1.0, 1.0, 1.0, 1.25, 1.5, 1.5},
    },
};

/* ============================================================
 * GC1084 YGamma 曲线数据 (Master)
 * ============================================================ */
static const _Sensor_YGAMMA_MAP gc1084_master_ygamma = {
    .local_ygamma_map = {
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
    },
};

/* 2.2 RGB Gamma 表 */
static const unsigned int gc1084_2_2_rgbgamma[] = {
    0x04809000,0x0871A848,0x0B628087,0x0DD32CB6,0x1003BCDD,0x11E43D00,0x13A4B11E,0x15451D3A,
    0x16C58154,0x1825DD6C,0x19863582,0x1AC68998,0x1BF6D9AC,0x1D2725BF,0x1E476DD2,0x1F57B5E4,
    0x2067F9F5,0x21683A06,0x22687A16,0x2358B626,0x2448F235,0x25292E44,0x26096652,0x26E99E60,
    0x27B9D26E,0x288A0A7B,0x295A3E88,0x2A2A6E95,0x2AEAA2A2,0x2BAAD2AE,0x2C6B02BA,0x2D2B32C6,
    0x2DDB5ED2,0x2E8B8EDD,0x2F4BBAE8,0x2FFBE6F4,0x309C12FF,0x314C3F09,0x31EC6714,0x329C931E,
    0x333CBB29,0x33DCE333,0x347D0B3D,0x351D3347,0x35AD5751,0x364D7F5A,0x36DDA364,0x376DCB6D,
    0x380DEF76,0x389E1380,0x392E3789,0x39BE5B92,0x3A4E7F9B,0x3ACEA3A4,0x3B5EC7AC,0x3BDEE7B5,
    0x3C6F0BBD,0x3CEF2BC6,0x3D7F4BCE,0x3DFF6FD7,0x3E7F8FDF,0x3EFFAFE7,0x3F7FCFEF,0x3FFFEFF7
};

static const _Sensor_RGBGAMMA gc1084_rgb_gamma_tbl = {
    .p_rgb_gamma_tbl = (unsigned int *)gc1084_2_2_rgbgamma,
};

/* ============================================================
 * GC1084 LSC 数据 (Master)
 * ============================================================ */
static const unsigned int gc1084_master_lsc_tbl[612] = {
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

static const _Sensor_LSC gc1084_master_lsc = {
    .p_lsc_tbl = (unsigned int *)gc1084_master_lsc_tbl,
};

#endif


/* 导出给 gc1084.c 使用的 isp_param_init 结构体数组 */
const _Sensor_ISP_Init gc1084_isp_param_init[ISP_SUPPORT_SENSOR_MAX_NUM] = 
{
#if GC1084_MASTER_SENSOR
    {
        .p_func        = (_ISP_FUNC          *)&gc1084_master_enable,
        .p_blc         = (_Sensor_BLC        *)&gc1084_master_blc,
        .p_ccm         = (_Sensor_CCM        *)&gc1084_master_ccm,
        .p_dpc         = (_Sensor_DPC        *)&gc1084_master_dpc,
        .p_colenh      = (_Sensor_BV2COLENH  *)&gc1084_master_ce, 
        .p_csc         = (_Sensor_CSC        *)&gc1084_master_csc,
        .p_gamma_param = (_Sensor_GAMMA_BV   *)&gc1084_master_gamma,
        .p_gic         = (_Sensor_GIC        *)&gc1084_master_gic,
        .p_csupp       = (_Sensor_CSUPP      *)&gc1084_master_csupp,
        .p_sharp       = (_Sensor_SHARP      *)&gc1084_master_sharp,
        .p_yuvnr       = (_Sensor_YUVNR      *)&gc1084_master_yuvnr,
        .p_bv2nr       = (_Sensor_BV2NR      *)gc1084_master_bv2nr,
        .p_lsc         = (_Sensor_LSC        *)&gc1084_master_lsc,
        .p_lhs         = (_Sensor_LHS_MAP    *)&gc1084_master_lhs,
        .p_ygamma      = (_Sensor_YGAMMA_MAP *)&gc1084_master_ygamma,
        .p_rgb_gamma   = (_Sensor_RGBGAMMA   *)&gc1084_rgb_gamma_tbl,
        .p_awb         = (TYPE_HGISP_CFG_AWB *)&gc1084_master_awb,
        .p_ae          = (TYPE_HGISP_CFG_AE  *)&gc1084_master_ae,
        .p_wdr         = (TYPE_HGISP_CFG_WDR *)&gc1084_master_wdr,
    },
#endif
#if GC1084_SLAVE0_SENSOR
    {
        // .p_func   = (_ISP_FUNC *)&gc1084_slave0_enable,
        // .p_blc    = (_Sensor_BLC *)&gc1084_slave0_blc,
        // .p_ccm    = (_Sensor_CCM *)&gc1084_slave0_ccm,
        // .p_awb    = (TYPE_HGISP_CFG_AWB *)&gc1084_slave0_awb,
        // .p_ae     = (TYPE_HGISP_CFG_AE *)&gc1084_slave0_ae,
        // .p_dpc    = (_Sensor_DPC *)&gc1084_slave0_dpc,
        // .p_csc    = (_Sensor_CSC *)&gc1084_slave0_csc,
        // .p_gic    = (_Sensor_GIC *)&gc1084_slave0_gic,
        // .p_csupp  = (_Sensor_CSUPP *)&gc1084_slave0_csupp,
        // .p_sharp  = (_Sensor_SHARP *)&gc1084_slave0_sharp,
        // .p_yuvnr  = (_Sensor_YUVNR *)&gc1084_slave0_yuvnr,
        // .p_colenh = (_Sensor_BV2COLENH *)&gc1084_slave0_ce,
        // .p_bv2nr  = (_Sensor_BV2NR *)gc1084_slave0_bv2nr,
        // .p_lhs    = (_Sensor_LHS_MAP *)&gc1084_slave0_lhs,
        // .p_lsc    = (_Sensor_LSC *)&gc1084_slave0_lsc,
        // .p_ygamma = (_Sensor_YGAMMA_MAP *)&gc1084_slave0_ygamma,
        // .p_wdr    = (TYPE_HGISP_CFG_WDR *)&gc1084_slave0_wdr,
    },
#endif
#if GC1084_SLAVE1_SENSOR
    {
        // .p_func   = (_ISP_FUNC *)&gc1084_slave1_enable,
        // .p_blc    = (_Sensor_BLC *)&gc1084_slave1_blc,
        // .p_ccm    = (_Sensor_CCM *)&gc1084_slave1_ccm,
        // .p_awb    = (TYPE_HGISP_CFG_AWB *)&gc1084_slave1_awb,
        // .p_ae     = (TYPE_HGISP_CFG_AE *)&gc1084_slave1_ae,
        // .p_dpc    = (_Sensor_DPC *)&gc1084_slave1_dpc,
        // .p_csc    = (_Sensor_CSC *)&gc1084_slave1_csc,
        // .p_gic    = (_Sensor_GIC *)&gc1084_slave1_gic,
        // .p_csupp  = (_Sensor_CSUPP *)&gc1084_slave1_csupp,
        // .p_sharp  = (_Sensor_SHARP *)&gc1084_slave1_sharp,
        // .p_yuvnr  = (_Sensor_YUVNR *)&gc1084_slave1_yuvnr,
        // .p_colenh = (_Sensor_BV2COLENH *)&gc1084_slave1_ce,
        // .p_bv2nr  = (_Sensor_BV2NR *)gc1084_slave1_bv2nr,
        // .p_lhs    = (_Sensor_LHS_MAP *)&gc1084_slave1_lhs,
        // .p_lsc    = (_Sensor_LSC *)&gc1084_slave1_lsc,
        // .p_ygamma = (_Sensor_YGAMMA_MAP *)&gc1084_slave1_ygamma,
        // .p_wdr    = (TYPE_HGISP_CFG_WDR *)&gc1084_slave1_wdr,
    },
#endif
};

