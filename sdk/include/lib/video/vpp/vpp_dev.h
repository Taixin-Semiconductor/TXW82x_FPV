#ifndef _VPP_DEV_H_
#define _VPP_DEV_H_
#include "hal/vpp.h"

#define VPP_INPUT_FORMAT     0   //0:YUV422
								 //1:RGB888/RAW



#define IN_DVP0							0
#define IN_DVP1							1
#define IN_MIPI_CSI0					2
#define IN_MIPI_CSI1					3
#define IN_ISP							4
#define IN_GEN422						5
#define IN_PARA_IN                      6

#ifndef VPP_INPUT_FROM
#define VPP_INPUT_FROM       			IN_ISP   
#endif


enum
{
	VPP_BUF0,
	VPP_BUF1,
};

enum
{
	VPP_BUF_IN_SRAM,
	VPP_BUF_IN_PSRAM,
};

typedef enum{
	ISP_VIDEO_0,
	ISP_VIDEO_1,
	ISP_VIDEO_2,
} ISP_VIDEO_E;

typedef enum {
	SCALER3_DONE = 0,
	SCALE3_KICK,
	JPG0_DONE,
	JPG1_DONE,
	SCALE1_JPG_ENCODE,
	VPP_IFP_EN_CTRL,
	VPP_H264_START,
	VPP_H264_ISR_START,
	VPP_JPEG0_START,
	VPP_JPEG1_START,
	VPP_FUNC_DONE_NUM,
}VPP_FUNC_DONE;

typedef int32_t (*scale3_kick_fn)();


//ret :0 --->keep running   1 --->run one time
typedef int32_t (*func_done_fn)(uint32 irq_data);

struct  video_cfg_t {
	uint8_t camera_mode;
	uint8_t video_num;
	uint8_t video_type_cur;     //cur frame is ISP_VIDEO_0/1/2
	uint8_t video_type_last;    //last frame is ISP_VIDEO_0/1/2
	uint8_t video_type_vpp;    //vpp runing which frame  ,app maybe use
	uint8_t resv;	          
    uint16_t dvp_iw;            /*dvp input image width*/
    uint16_t dvp_ih;            /*dvp input image height*/
    uint16_t dvp_ow;            /*dvp output image width*/
    uint16_t dvp_oh;            /*dvp output image height*/
    uint16_t dvp_type;          /*dvp device type: 0=no device, 1=master device, 2=slave device 0, 3=slave device 1*/
	uint16_t csi0_iw;
	uint16_t csi0_ih;
	uint16_t csi0_ow;
	uint16_t csi0_oh;
	uint16_t csi0_type;        //0:no device   1:master    2:slave0     3:slave1
	uint16_t csi1_iw;
	uint16_t csi1_ih;	
	uint16_t csi1_ow;
	uint16_t csi1_oh;
	uint16_t csi1_type;        //0:no device   1:master    2:slave0     3:slave1
};

struct mdt_coord_msg {	
	uint16_t x0,x1;
	uint16_t y0,y1;
	uint16_t x,y;
	uint8_t blkmv_cnt;
};

enum
{
	VPP_MODE_2N_ADD_16 = 0,        //[16,32] 2N+16
	VPP_MODE_2N,			   //[0,16]	 2N
};


#ifndef SCALE1_FROM_VPPBF
#define SCALE1_FROM_VPPBF         0//0:VPP BUF0    1:VPP BUF1
#endif

#ifndef SCALE3_FROM_VPPBF
#define SCALE3_FROM_VPPBF         0//0:VPP BUF0    1:VPP BUF1
#endif

#ifndef VPP_BUF0_MODE
#define VPP_BUF0_MODE                  VPP_MODE_2N_ADD_16   
#endif

#ifndef VPP_BUF1_MODE
#define VPP_BUF1_MODE                  VPP_MODE_2N_ADD_16  
#endif

//注意这里配置的N,所以实际根据MODE决定申请空间
#ifndef VPP_BUF0_LINEBUF_NUM
#define VPP_BUF0_LINEBUF_NUM           8
#endif

#ifndef VPP_BUF1_LINEBUF_NUM
#define VPP_BUF1_LINEBUF_NUM		   6
#endif


// 取两数最大值
#define VPP_MAX2(x, y)  ((x) > (y) ? (x) : (y))
// 取三数最大值
#define VPP_MAX3(x, y, z) VPP_MAX2(VPP_MAX2(x, y), z)

extern struct video_cfg_t video_msg;
bool vpp_cfg(uint8_t input_from);
bool vpp_cfg_release();
void vpp_itp_save_only(struct vpp_device *p_vpp,uint16_t w,uint16_t h,uint32_t psram_adr);
uint8_t vpp_video_type_map(uint8_t stype);
int32 vppdone_func_register(uint8_t id,func_done_fn func,uint32 arg);
int32 vppdone_func_unregister(uint8_t id);
uint8_t get_vpp_w_h(uint16_t *w, uint16_t *h);
uint8_t get_vpp1_w_h(uint16_t *w, uint16_t *h);
void set_vpp_scale_w_h(uint8_t en, uint16_t w, uint16_t h);
uint8_t get_vpp1_w_h(uint16_t *w, uint16_t *h);
void *get_vpp_buf(uint8_t which);
int8_t vpp_dev_open();
int32 vpp_is_closed(struct vpp_device *p_vpp);
uint32 md_get_pot_x_y(ISP_VIDEO_E sensor_id);
void md_get_table_size(ISP_VIDEO_E sensor_id, uint16_t *w_out, uint16_t *h_out);
int8_t md_get_binary_table(ISP_VIDEO_E sensor_id, uint8_t *bin_buf);
uint32 md_get_relative_pot_x_y(ISP_VIDEO_E sensor_id, uint16_t r_w, uint16_t r_h, uint16_t *gx, uint16_t *gy);
uint32 md_get_relative_pot_x0_y0_x1_y1(uint8_t num, uint16_t r_w, uint16_t r_h, uint16_t *gx0, uint16_t *gy0, uint16_t *gx1, uint16_t *gy1);

//注意当前接口只有buf1才支持写入到psram,buf0只能写入到sram
//写入到psram一般是副码流为h264的时候
int32_t vpp_set_buf_msg(uint8_t which_buf, uint8_t en, uint8_t in_psram, uint16_t w, uint16_t h);
void vpp_set_double_psram_for_buf1(uint8_t en);

#endif

