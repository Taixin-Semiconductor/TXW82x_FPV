#ifndef _PHOTO_TRAN_PROTOCOL_
#define _PHOTO_TRAN_PROTOCOL_

#include "osal/string.h"
#include "lib/net/eloop/eloop.h"

//#define OPEN_DBG 1
#if OPEN_DBG
#define BABY_DBG(fmt, ...)   _os_printf(fmt, ##__VA_ARGS__)
#else
#define BABY_DBG(fmt, ...)
#endif

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
	EVT_HDL  udp_read_status_ev;
	uint32_t udp_data_task;
	uint32_t dev_id;
	uint32_t frame_rate;
	uint8_t *psram_photo;
	uint8_t  larger;
	uint16_t w;
	uint16_t h;	
}dev_map;


typedef struct
{
	uint8_t  speed;
	uint8_t  frame_tx_lost;
	uint8_t  frame_tx_success;
	uint8_t  frame_rx_lost;
	uint8_t  frame_rx_success;	
}babymonitor_msg;

typedef struct
{
	uint32_t target_width;
	uint32_t target_high;
}target_resolution;


#define MAIN_SENSOR_STILL_MIN    300
#define MAIN_SENSOR_STILL_MAX    600
#define MAIN_SENSOR_MOVE_MIN     800
#define MAIN_SENSOR_MOVE_MAX     1500

#define SEC_SENSOR_STILL_MIN     200
#define SEC_SENSOR_STILL_MAX     400
#define SEC_SENSOR_MOVE_MIN      400
#define SEC_SENSOR_MOVE_MAX      1000


#define BPS_STEP                 50
#define UP_BPS_FRM_NUM           25

#ifdef SYS_APP_BBM_LCD
extern void protocol_server_change_resolution(uint8_t id,uint32_t width, uint32_t high);
#endif

#endif