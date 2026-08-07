#ifndef _PHOTO_TRAN_PROTOCOL_
#define _PHOTO_TRAN_PROTOCOL_
#include "osal/string.h"

typedef struct
{     
	uint32_t type;       //0:连接请求      1:开始图传      2:cfg
	uint16_t w;
	uint16_t h;
	uint32_t packet_len;  //分包长度
	uint32_t ip_grp;      //IP比例,如25，则表示1个I帧，24个P帧
	uint32_t frame_rate;  //帧率
	uint32_t dev_magic;   //设备特征码
}connect_cfg_head;

typedef struct
{    
	uint8_t  framenum;   //0~255,每帧+1
	uint8_t  cnt;        //当前包需要分多少数据包      
	uint8_t  pack;       //当前包num
	uint8_t  frmtype;    //当前是I帧还是P帧
}data_head;

typedef struct
{    
	uint8_t  framenum;      //当前帧num
	uint8_t  type;          //0:当前帧接收完整            1:请求屏/app发送状态msg              2:当前帧数据缺失
}status_msg;

typedef struct
{    
	uint32_t  time;             //发送完成的时间
	uint32_t  dev_magic;		//设备特征码
	uint8_t   framenum;         //当前帧num
	uint8_t   status;           //0:wait client status   1:sending status to client    2:get client status   3:frame lost
	uint8_t   timeout;          //超时多久后发送状态请求
	uint8_t   lost_num;         //当前获取状态后，丢包情况
	uint8_t   lost_packet[100]; //如果有丢包，分别是哪些包
	
}frame_msg;

typedef struct
{
	uint8_t *addr;
	uint32_t len;
	uint32_t timeinf;
	uint16_t num;
	uint16_t type;
	uint16_t devid;
	uint16_t framerate;
	uint16_t w;
	uint16_t h;
}decode_msg;

typedef struct
{
	uint32_t ipaddr;
	uint32_t tcpfd;
	uint32_t udp_status_fd;
	uint32_t udp_data_fd;
	uint32_t udp_status_task;
	void*  udp_read_status_ev;
	uint32_t udp_data_task;
	uint32_t dev_id;
	uint32_t frame_rate;
	uint8_t *psram_photo;
	uint16_t w;
	uint16_t h;	
}dev_map;

typedef struct
{
	int32_t tx_data;
	int32_t rx_data;
	uint8_t  speed;
	uint8_t  frame_tx_lost;
	uint8_t  frame_tx_success;
	uint8_t  frame_rx_lost;
	uint8_t  frame_rx_success;	
	uint8_t  mcs;
	uint8_t  frmtype;
	uint8_t  have_init;
    uint8_t  run_state;
    uint8_t  run_task;
	struct os_mutex run_state_mutex;
}walkie_msg;

#ifdef SYS_APP_WALKIE_TALKIE
extern walkie_msg walkmsg;
extern uint8_t   apsta_mode;
void user_protocol_task_decrease(void);
void user_protocol_task_increase(void);
void user_protocol2(uint16_t status_port,uint16_t data_port);
void user_protocol2_deinit(void);
void user_protocol3(uint16_t status_port,uint16_t data_port);
void user_protocol3_deinit(void);
void user_protocol_deinit(void);
void user_protocol_reinit(void);
#endif

#define OPEN_DBG       0
#define MODEFORDEV  0    //0:AP   1:STA
#define NET_W       320
#define NET_H       240
 
#endif
