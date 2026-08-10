#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/isp.h"

#if DEV_SENSOR_H63P


SENSOR_INIT_SECTION const unsigned char h63p_1280x720_25fps_reg[CMOS_INIT_LEN]= 
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


void h63p_ae_adjust(struct isp_exposure_opt *p_cfg)
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

void h63p_img_opt(struct isp_sensor_opt *p_opt)
{
    uint8  *addr = (uint8 *)p_opt->data.addr;
    uint8  index = 0;
    addr[index++] = 0x12;
    addr[index++] = (p_opt->reverse_en + p_opt->mirror_en * 2) << 4;
    p_opt->data.size = index; 
    p_opt->cmd_len   = 1 + 1;   // addr length + data lengt
}

void h63p_fps_opt(struct isp_sensor_opt *p_opt)
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

static const SensorWorkMode h63p_supported_modes[] = {

    {
        .mode = CAM_SINGLE_MASTER_MODE,
        .bayer_patten = ISP_BAYER_FORMAT_BGGR,
        .width = 1280,
        .height = 720,
        .mipi = {
            .mipi_lane_num = 1,
            .mipi_bps = 360,
        },
        .fps_table = {
            {.fps = 25, .vts = 750},
            {.fps = 20, .vts = 937},
            {.fps = 15, .vts = 1250},
        },
        .reg_list = h63p_1280x720_25fps_reg,
    },

};

SENSOR_OP_SECTION const _Sensor_Adpt_ h63p_cmd= 
{	
    .supported_modes = (SensorWorkMode*)h63p_supported_modes,
    .mode_num        = ARRAY_SIZE(h63p_supported_modes),

    .vts_reg = {0x23,0x22},
    .vts_reg_num = 2,
    .sensor_iic = {
        0x48,0x80,0x81,0x01,0x01,0x0b
    },
    .sensor_isp_cfg = {
        .mirror       = 1,
        .reverse      = 1,
        .input_format = ISP_INPUT_DAT_FORMAT_RAW10,
        .adjust_func  = (isp_ae_func     )h63p_ae_adjust,
        .img_opt      = (sensor_img_opt  )h63p_img_opt,
        .fps_opt      = (sensor_fps_opt  )h63p_fps_opt,
        // .isp_iq_param   = (_Sensor_ISP_Init*)&h63p_isp_param_init,
    },
};


#endif
