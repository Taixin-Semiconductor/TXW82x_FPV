#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/isp.h"

#if DEV_SENSOR_IMX219

#define IMX219MIPI_MaxGainIndex (98)


SENSOR_INIT_SECTION const unsigned char imx219_binning_1600x1200_25fps[]= 
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

    p_cfg->data.size = index;
    p_cfg->cmd_len   = 2+1;
}

static const SensorWorkMode imx219_supported_modes[] = {
    {
        .mode = CAM_SINGLE_MASTER_MODE,
        .width = 1600,
        .height = 1200,
        .mipi = {
            .mipi_lane_num = 2,
            .mipi_bps = 360,
        },
        .fps_table = {
            {.fps = 25, .vts = 1381},
        },
        .reg_list = imx219_binning_1600x1200_25fps,
    },

};


SENSOR_OP_SECTION const _Sensor_Adpt_ imx219_cmd= 
{	
    .supported_modes = (SensorWorkMode*)imx219_supported_modes,
    .mode_num        = ARRAY_SIZE(imx219_supported_modes),

    .vts_reg = {0x0160,0x0161},
    .vts_reg_num = 2,
    .sensor_iic = {
        .id = 0x19,     .w_cmd = 0x20,
        // .w_cmd = 0x6c, .r_cmd = 0x6d,
        .r_cmd = 0x21,  .addr_num = 2,
        .data_num = 1,  .id_reg = 0x001,
    },
    .sensor_isp_cfg = {
        .mirror       = 1,
        .reverse      = 1,
        .expo_max_gain = 168<<8,
        .expo_update_delay = 2,
        .min_frame_vb = 4,
        .bayer_patten = ISP_BAYER_FORMAT_GBRG,
        .input_format = ISP_INPUT_DAT_FORMAT_RAW10,
        .adjust_func  = (isp_ae_func     )imx219_ae_adjust,
        // .img_opt      = (sensor_img_opt  )h63p_img_opt,
        // .fps_opt      = (sensor_fps_opt  )h63p_fps_opt,
        // .isp_iq_param   = (_Sensor_ISP_Init*)&imx219_isp_param_init,
    },
};


#endif
