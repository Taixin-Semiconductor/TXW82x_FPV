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

#define VPP_INPUT_FROM       			IN_ISP   

enum
{
	ISP_VIDEO_0,
	ISP_VIDEO_1,
	ISP_VIDEO_2,
};


typedef int32_t (*scale3_kick_fn)();


struct  video_cfg_t {
	uint8_t video_num;
	uint8_t video_type_cur;     //cur frame is ISP_VIDEO_0/1/2
	uint8_t video_type_last;    //last frame is ISP_VIDEO_0/1/2
	uint8_t video_type_vpp;    //vpp runing which frame  ,app maybe use
	uint8_t resv;	          
	uint16_t dvp_iw;
	uint16_t dvp_ih;
	uint16_t dvp_ow;
	uint16_t dvp_oh;
	uint16_t dvp_type;          //0:no device   1:master    2:slave0     3:slave1
	uint16_t csi0_iw;
	uint16_t csi0_ih;
	uint16_t csi0_ow;
	uint16_t csi0_oh;
	uint16_t csi0_type;         //0:master 1:slave0  2:slave1
	uint16_t csi1_iw;
	uint16_t csi1_ih;	
	uint16_t csi1_ow;
	uint16_t csi1_oh;
	uint16_t csi1_type;         //0:master 1:slave0  2:slave1
};
enum
{
	VPP_MODE_2N_ADD_16,        //[16,32] 2N+16
	VPP_MODE_2N,			   //[0,16]	 2N
};

#define SCALE1_FROM_VPPBF         0//0:VPP BUF0    1:VPP BUF1
#define SCALE3_FROM_VPPBF         0//0:VPP BUF0    1:VPP BUF1

#define VPP_BUF0_MODE                  VPP_MODE_2N_ADD_16        
#define VPP_BUF1_MODE                  VPP_MODE_2N_ADD_16        

//注意这里配置的N,所以实际根据MODE决定申请空间
#define VPP_BUF0_LINEBUF_NUM           8
#define VPP_BUF1_LINEBUF_NUM		   6



extern struct video_cfg_t video_msg;
bool vpp_cfg(uint32_t w,uint32_t h,uint8_t input_from);
void vpp_itp_save_only(struct vpp_device *p_vpp,uint16_t w,uint16_t h,uint32_t psram_adr);
uint8_t vpp_video_type_map(uint8_t stype);
#endif

