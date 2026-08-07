#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/isp.h"

/* lens & sensor config information:
- sensor      : h66
- fstop       : TBD
- mclk        : 24MHz
- max FPS     : 30
- frame length: TBD
- usage       : TBD
- interface   : mipi 1 lane
- mipi_rate   = 216Mbps/lane 
*/

#if DEV_SENSOR_H66

SENSOR_INIT_SECTION const unsigned char H66InitTable[CMOS_INIT_LEN]= 
{	
    0x12, 0x40,
    0x48, 0x85,
    0x48, 0x05,
    0x0E, 0x11,
    0x0F, 0x14,
    0x10, 0x24,
    0x11, 0x80,
    0x0D, 0xF0,
    0x5F, 0x41,
    0x60, 0x20,
    0x58, 0x18,
    0x57, 0x60,
    0x20, 0x84,
    0x21, 0x03,
    0x22, 0x20,
    0x23, 0x03,
    0x24, 0x80,
    0x25, 0xD0,
    0x26, 0x22,
    0x27, 0x3A,
    0x28, 0x15,
    0x29, 0x01,
    0x2A, 0x2E,
    0x2B, 0x11,
    0x2C, 0x00,
    0x2D, 0x1E,
    0x2E, 0xD8,
    0x2F, 0x40,
    0x41, 0x84,
    0x42, 0x12,
    0x46, 0x10,
    0x47, 0x42,
    0x76, 0x40,
    0x77, 0x06,
    0x80, 0x01,
    0xAF, 0x22,
    0xAB, 0x00,
    0x1D, 0x00,
    0x1E, 0x04,
    0x6C, 0x50,
    0x6E, 0x2C,
    0x70, 0x90,
    0x71, 0x8D,
    0x72, 0xAA,
    0x73, 0x56,
    0x74, 0x02,
    0x78, 0x92,
    0x89, 0x01,
    0x6B, 0x20,
    0x86, 0x40,
    0x2F, 0x60,
    0x30, 0x86,
    0x31, 0x04,
    0x32, 0x18,
    0x33, 0x52,
    0x34, 0x24,
    0x35, 0x22,
    0x3A, 0xA0,
    0x3B, 0x00,
    0x3C, 0x36,
    0x3D, 0x40,
    0x3E, 0xD0,
    0x56, 0x1A,
    0x59, 0x3C,
    0x5A, 0x04,
    0x84, 0x28,
    0x85, 0x26,
    0x8A, 0x04,
    0x9C, 0xE1,
    0x5B, 0xA0,
    0x5C, 0x28,
    0x5D, 0xE4,
    0x5E, 0x04,
    0x64, 0xE0,
    0x66, 0x40,
    0x67, 0x74,
    0x68, 0x00,
    0x69, 0x70,
    0x7A, 0x62,
    0xAE, 0x30,
    0x13, 0x81,
    0x96, 0x04,
    0x4A, 0x05,
    0x7E, 0xCD,
    0x50, 0x02,
    0x49, 0x10,
    0x7B, 0x4A,
    0x7C, 0x09,
    0x7F, 0x56,
    0x62, 0x21,
    0x8F, 0x80,
    0x90, 0x00,
    0x8C, 0xFF,
    0x8D, 0xC7,
    0x8E, 0x00,
    0x8B, 0x01,
    0x0C, 0x40,
    0xA3, 0x20,
    0xA0, 0x01,
    0x81, 0x74,
    0xA2, 0x78,
    0x82, 0x01,
    0x19, 0x20,
    0x12, 0x00,
    0x48, 0x85,
    0x48, 0x05,
	0xff, 0xff,//
};

const _Sensor_CCM h66_ccm_init = {
    0x100,  0x000,  0x000,
    0x000,  0x100,  0x000,
    0x000,  0x000,  0x100,
    0x000,  0x000,  0x000,
};

const _Sensor_BLC h66_blc_init =
{
    0, 0, 0, 0,
};

const _Sensor_AWB h66_awb_init = 
{
//    r,  gr,  gb,   b
    256, 256, 256, 256, // default
    256, 256, 256, 256, // min
    800, 800, 800, 800, // max
};

const _Sensor_CSUPP h66_csupp_init = {
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

const _Sensor_SHARP h66_sharp_init = {
    0   , //smooth_filt_alpha
    0   , //shrink_thr
    127 , //filt_clip_hi
    127 , //filt_clip_lo
    
    10  , //sp_thr2
    5   , //sp_thr1
    15  , //enha_clip_hi
    25  , //enha_clip_lo
    
    5   ,10  ,15  ,//e1,e2,e3
    0   ,64  ,128 ,128 ,//k0,k1,k2,k3
    0   ,10  ,30  ,//y1,y2,y3
    
    3   ,4   ,5   , //filt_w11,filt_w12,filt_w13
    4   ,6   ,7   , //filt_w21,filt_w22,filt_w23
    5   ,7   ,12  , //filt_w31,filt_w32,filt_w33
    
    1   , //filt_type
    7   , //filt_sbit
};

const _Sensor_YUVNR h66_yuvnr_init = {
    20,20,20,20,20,20,20,20,
    205,255,0,
};

const _Sensor_ISP_Init h66_isp_init = 
{
    .type         = ISP_INPUT_DAT_SRC_MIPI0,
    .pixel_h      = 720,
    .pixel_w      = 1280,
    .bayer_patten = ISP_BAYER_FORMAT_BGGR,
    .input_format = ISP_INPUT_DAT_FORMAT_RAW10,
    .p_blc        = (_Sensor_BLC   *)&h66_blc_init,
    .p_ccm        = (_Sensor_CCM   *)&h66_ccm_init,
    .p_awb        = (_Sensor_AWB   *)&h66_awb_init,
    .p_csupp      = (_Sensor_CSUPP *)&h66_csupp_init,
    .p_sharp      = (_Sensor_SHARP *)&h66_sharp_init,
    .p_yuvnr      = (_Sensor_YUVNR *)&h66_yuvnr_init,	
};

SENSOR_OP_SECTION const _Sensor_Adpt_ h66_cmd= 
{	
	.typ = 1, //YUV
	.pixelw = 1280,
	.pixelh= 720,
	.hsyn = 1,
	.vsyn = 0,
	.rduline = 0,//
	.rawwide = 1,//10bit
	.colrarray = 2,//0:_RGRG_ 1:_GRGR_,2:_BGBG_,3:_GBGB_
	.init = (uint8 *)H66InitTable,
	.rotate_adapt = {0},
	.hvb_adapt = {0x80,0x0a,0x80,0x0a},
	. mclk = 24000000,
	.p_fun_adapt = {NULL,NULL,NULL},
	.sensor_isp = (_Sensor_ISP_Init *)&h66_isp_init,
};

const _Sensor_Ident_ h66_init=
{
	0x66,0x80,0x81,0x01,0x01,0x0b
};


#endif
