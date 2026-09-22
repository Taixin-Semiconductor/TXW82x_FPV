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


SENSOR_INIT_SECTION const unsigned char h62_1280x720_25fps_1lane[CMOS_INIT_LEN]= 
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


void h62_ae_adjust(struct isp_exposure_opt *p_cfg)
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

void h62_img_opt(struct isp_sensor_opt *p_opt)
{
    uint8  *addr = (uint8 *)p_opt->data.addr;
    uint8  index = 0;
    addr[index++] = 0x12;
    addr[index++] = (p_opt->reverse_en + p_opt->mirror_en * 2) << 4;
    p_opt->data.size = index; 
    p_opt->cmd_len   = 1 + 1;   // addr length + data lengt
}

void h62_fps_opt(struct isp_sensor_opt *p_opt)
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

static const SensorWorkMode h62_supported_modes[] = {
    {
        .mode = SENSOR_TYPE_MASTER,
        .bayer_patten = ISP_BAYER_FORMAT_BGGR,
        .width = 1280,
        .height = 720,
        .mipi = {
            .mipi_lane_num = 1,
            .mipi_bps = 696,
        },
        .fps_table = {
            {.fps = 25, .vts = 1200},
        },
        .reg_list = h62_1280x720_25fps_1lane,
    },
};

SENSOR_OP_SECTION const _Sensor_Adpt_ h62_cmd = 
{
    .supported_modes = (SensorWorkMode*)h62_supported_modes,
    .mode_num        = ARRAY_SIZE(h62_supported_modes),

    .vts_reg = {0x23,0x22},
    .vts_reg_num = 2,
    .sensor_iic = {
		0x62,0x60,0x61,0x01,0x01,0x0b
    },
    .sensor_isp_cfg = {
        .mirror       = 1,
        .reverse      = 1,
        .input_format = ISP_INPUT_DAT_FORMAT_RAW10,
        .adjust_func  = (isp_ae_func     )h62_ae_adjust,
         .img_opt      = (sensor_img_opt  )h62_img_opt,
         .fps_opt      = (sensor_fps_opt  )h62_fps_opt,
//         .isp_iq_param   = (_Sensor_ISP_Init*)&h62_isp_param_init,
    },
};


#endif
