#ifndef _CSI_MODE_DEFINE_
#define _CSI_MODE_DEFINE_
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/i2c.h"
#include "dev/csi/hgdvp.h"
#include "hal/isp.h"

#define SENSOR_PRESET_FPS_NUM     8U

#define SENSOR_MCLK     24

typedef struct {
	uint32 rotate;
} Rotate_Adapt;

typedef struct {
	uint32 pclk;
	uint32 v_len;
	uint32 step_val;
	uint32 step_max;
} Hvb_Adapt;

typedef struct {
	void (*fp_rotate)(uint32 rotate);

	uint32 (*fp_hvblank)(int8_t hb,int8_t vb);

	void (*fp_exp_gain_wr)(uint32 exp,uint32 gain);
} P_Fun_Adapt;

typedef struct {
	uint8 *bypass_on;
	uint8 *bypass_off;
	//_Sensor_Ident_ * sensor_init;
	uint8 *sensor_init_table;
	uint8 id,w_cmd,r_cmd,addr_num,data_num;
	uint16 id_reg;	
} P_XC7016_Fun;

typedef struct isp_param_init {
    _ISP_FUNC             *p_func;
    _Sensor_BLC           *p_blc;
    _Sensor_CCM           *p_ccm;
	_Sensor_DPC           *p_dpc;	
	_Sensor_CSC           *p_csc;
    _Sensor_GAMMA_BV      *p_gamma_param;
	_Sensor_GIC           *p_gic;
	_Sensor_CSUPP         *p_csupp;
	_Sensor_SHARP         *p_sharp;
	_Sensor_YUVNR         *p_yuvnr;
	_Sensor_BV2COLENH     *p_colenh;
    _Sensor_BV2NR         *p_bv2nr;
    _Sensor_LHS_MAP       *p_lhs;
    _Sensor_LSC           *p_lsc;
    _Sensor_YGAMMA_MAP    *p_ygamma;
    _Sensor_RGBGAMMA      *p_rgb_gamma;
	TYPE_HGISP_CFG_AWB    *p_awb;
    TYPE_HGISP_CFG_AE     *p_ae;
	TYPE_HGISP_CFG_WDR    *p_wdr;
}_Sensor_ISP_Init;


typedef struct {
    uint16 bayer_patten, input_format;
    uint8  mirror, reverse;

    uint32 expo_max_gain         : 16, /* 最大增益 (256=1X) */
           expo_min_gain         : 16; /* 最小增益 (256=1X) */
    // uint32 expo_max_line         : 16, /* 最大曝光行 (通常=VTS-min_frame_vb) */
    //        expo_min_line         : 16; /* 最小曝光行 (通常=min_frame_vb) */
    uint32 expo_update_delay     :  8, /* 曝光参数生效延迟(帧)*/
           min_frame_vb 		 :  8; 

    isp_ae_func     adjust_func;
    sensor_img_opt  img_opt;
    sensor_fps_opt  fps_opt;
    _Sensor_ISP_Init  *isp_iq_param;
}_Sensor_ISP_CFG;


typedef struct {
    uint8_t  mipi_lane_num;    // mipi lane数 1/2
    uint16_t mipi_bps;
} SensorMipiParam;


typedef struct {
    uint8_t hsync_pol;
    uint8_t vsync_pol;
    uint32_t mclk;
} SensorDvpParam;

// FSYNC边沿配置
typedef struct {
    uint8  role;
    uint16 sync_reg;        // 同步极性配置寄存器地址
    uint8  edge_rise;   // 上升沿模式写入值
    uint8  edge_fall;   // 下降沿模式写入值
} SlaveSyncCfg_t;

// 帧率-VTS映射
typedef struct {
    uint8  fps;
    uint16 vts;
} FpsVtsMap_t;




typedef enum {
    SYNC_EDGE_FALL = 0,  // 下降沿
    SYNC_EDGE_RISE = 1   // 上升沿
} SyncEdgeType;


typedef struct {
    uint8_t mode;
	uint8_t bayer_patten;
    uint16_t width;
    uint16_t height;

    uint16_t input_format;
    uint16_t interface_type;
    float row_time_us;
    FpsVtsMap_t     fps_table[SENSOR_PRESET_FPS_NUM];
    SlaveSyncCfg_t  sync_cfg;
    SensorMipiParam mipi;
    SensorDvpParam  dvp;
    const unsigned char *reg_list;

} SensorWorkMode;

typedef struct
{       
	uint8 id,w_cmd,r_cmd,addr_num,data_num;
	uint16 id_reg;
}_Sensor_Ident_;



typedef struct {
	uint8 w_cmd,r_cmd,id,addr_num,data_num;
	uint16 id_reg;//id ¼Ä´æÆ÷µØÖ·
	uint8 *init;
	uint8 *slave_init;
	uint8 *nigth_mode_init;
	uint8 *sensor_stop_stream;
	uint32 init_len;
	uint8 *preset;
	uint16 typ; //0: raw 1:yuv 2:mipi
	uint32 mclk;
	uint32 pclk_fir_en;
	uint16 pixelw; 
	uint16 pixelh;
    uint8  mipi_lane_num;
	uint16 vts_reg[3];//FRAME_LENGTH: H - L
	uint8  vts_reg_num; 
	uint8 colrarray; 
	bool hsyn; 
	bool vsyn;
	bool rduline;
	bool rawwide; 
	
	Rotate_Adapt rotate_adapt;
	Hvb_Adapt hvb_adapt;
	volatile uint32 isp_all_mod;
	uint32 allow_miss_dots;
	uint32 align[1];		// only for 16byte align

	P_Fun_Adapt p_fun_adapt;
	P_XC7016_Fun p_xc7016_adapt;
    
    _Sensor_ISP_CFG   sensor_isp_cfg;

    SensorWorkMode *supported_modes;
	uint8 mode_num;

	_Sensor_Ident_ sensor_iic;
    


} _Sensor_Adpt_;




typedef struct
{
	uint8 online;
	uint32 pclk;
	uint32 iclk;
	_Sensor_Ident_ * p_sensor_ident;
	_Sensor_Adpt_ * p_sensor_cmd;
}SNSER;

typedef struct
{
	uint16 in_w;			//sensor pixel
	uint16 in_h;
	uint16 out0_w; 			//to jpg encode
	uint16 out0_h;	
	uint16 out1_w; 			//to jpg encode
	uint16 out1_h;
}Vpp_stream;


typedef struct
{
	uint16 out_w; 
	uint16 out_h;
}Output_photo;

struct i2c_setting
{
	uint8 u8DevWriteAddr;
	uint8 u8DevReadAddr;
};

typedef struct {
   const _Sensor_Ident_ *ident;
   const _Sensor_Adpt_  *adpt;
} SensorTable;


#define SENSOR_INIT_SECTION    __attribute__ ((aligned(32)))
#define SENSOR_OP_SECTION      __attribute__ ((aligned(32)))

#define  CMOS_INIT_LEN


#define DVP                     ((struct hgdvp_hw *) DVP_BASE)


extern uint8 u8SensorwriteID,u8SensorreadID;
extern uint8 u8Addrbytnum,u8Databytnum;

void Delay_nopCnt(uint32 cnt);

void get_single_dvp(uint16_t *w,uint16_t *h);


//////////////////////////////////////////COMMON//////////////////////////////////////////////////////////////////////

#define END    0xff  

#define IMAGE_W    1280     
#define IMAGE_H    720

#define IMAGE_FORMAT					0   //0:YUV422
											//1:RAW8
											//2:RAW10
											//3:RAW12

#define SCEN_EN                         0   

#define FRAME_RATE                      0   //0:  100%
											//1:  75%
											//2:  50%
											//3:  25%

#define ONCE_SAMPLE						0   //这个只处理整帧中断的情况,即RGB/Y ONLY

////////////////////////////////////////////////////////RGB//////////////////////////////////////////////////////////////////


#define RGB2YUV							0


////////////////////////////////////////////////////////YUV//////////////////////////////////////////////////////////////////


#define HSYS							0    //0:h->1 v->1
											 //1:h->1 v->0
											 //2:h->0 v->1
											 //3:h->0 v->0


#define YUV_MODE                        0    //0：YUYV
											 //1：YVYU
											 //2：UYVY
											 //3：VYUY


											
#define ONLY_Y							0

////////////////////////////////////////////////////////JPEG//////////////////////////////////////////////////////////////////
#define JPEG_LEN                        2*1024
#ifndef VPP_SCALE_WIDTH
#define VPP_SCALE_WIDTH				1920//1920//1280	
#endif


#ifndef VPP_SCALE_HIGH
#define VPP_SCALE_HIGH				1080//1080//720
#endif


#ifndef IPF_EN
#define IPF_EN					0
#endif
#define DET_EN					1


///////////////////////////////////////////////////////sensor//////////////////////////////////////////////////////////////////////

extern const _Sensor_ISP_Init default_isp_param_init[ISP_SUPPORT_SENSOR_MAX_NUM];
extern const _Sensor_ISP_Init gc1084_isp_param_init[ISP_SUPPORT_SENSOR_MAX_NUM];


_Sensor_Adpt_ *sensorAutoCheck(uint8_t csi_dev_id, uint8_t iic_devid);
int sensor_write_reg_table(_Sensor_Adpt_ *sensor_cmd, uint8_t index ,uint8_t mipi_csi_iic);
int sensor_mode_find_index(_Sensor_Adpt_ *sensor_cmd,uint8_t target_mode, uint8_t target_lane);
uint16 sensor_fps_to_vts(const _Sensor_Adpt_ *sensor_cmd, uint8_t index, uint8_t mipi_csi_iic, uint8_t target_fps);
uint8 set_sync_edge(const _Sensor_Adpt_ *sensor_cmd, uint8_t index ,uint8_t mipi_csi_iic, SyncEdgeType edge);



#endif


