/***************************************************
    该demo主要是使用AT命令拍一张照片,前提要将jpeg打开
***************************************************/
#include "sys_config.h"	
#include "tx_platform.h"
#include "osal/string.h"
#include "stream_frame.h"
#include "osal/task.h"
#include "osal_file.h"
#include "video_app/video_app.h"
#include "lwip/api.h"
#include "lwip/sockets.h"
#include "lwip/etharp.h"
#include "utlist.h"
#include "jpgdef.h"
#include "lib/lcd/lcd.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#include "lwip/tcpip.h"
#include "netif/ethernetif.h"
#include "lib/common/sysevt.h"
#include "syscfg.h"
#include <event.h>
#include <csi_kernel.h>
#include "lib/video/dvp/jpeg/jpg.h"
#include "walkie_talkie.h"
#include "stream_define.h"
#include "lib/multimedia/msi.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "dev/vpp/hgvpp.h"
#include "lib/umac/ieee80211.h"
#include "lib/video/h264/h264_drv.h"
#include "lib/lmac/lmac.h"

#ifdef SYS_APP_WALKIE_TALKIE

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE av_free
#define STREAM_LIBC_ZALLOC av_zalloc


#define MAX_USER_VIDEO_TX 16
#define MAX_VIDEO_PKT_LEN 1430


#if OPEN_DBG
#define CHILDREN_DBG(fmt, ...)   _os_printf(fmt, ##__VA_ARGS__)
#else
#define CHILDREN_DBG(fmt, ...)   //_os_printf(fmt, ##__VA_ARGS__)
#endif

frame_msg client_frame;
uint32_t  client_dev_magic = 0;
os_mutex_t      thread_lock;

static struct os_semaphore net_h264_sem = {0,NULL};
static struct os_semaphore net_h264_status_sem = {0,NULL};
static struct os_semaphore net_tcp_sem = {0,NULL};

static k_task_handle_t handle_task_recv;
static k_task_handle_t handle_data_task_recv;

static int handle_protocol_fd   =  - 1;
static int handle_data_protocol_fd   =  - 1;
static volatile uint32 status_unlock = 0;

static uint8_t photo_buf[1440] __attribute__ ((aligned(4)));;
uint8_t server_staus_buf[200];
volatile uint32 server_frame_rate;

uint32_t heartbeat = 0;

EVT_HDL tcp_read_ev;

static uint8_t current_bss_bw = 20;
static uint8_t ctrl_mode = 4;

void net_h264_status_sema_init()
{
	os_sema_init(&net_h264_status_sem,0);
}

int32 net_h264_status_sema_down(int32 tmo_ms)
{
	return os_sema_down(&net_h264_status_sem,tmo_ms);
}

void net_h264_status_sema_up()
{
	os_sema_up(&net_h264_status_sem);
}

void net_h264_sema_init()
{
	os_sema_init(&net_h264_sem,0);
}

void net_h264_sema_down(int32 tmo_ms)
{
	os_sema_down(&net_h264_sem,tmo_ms);
}

void net_h264_sema_up()
{
	os_sema_up(&net_h264_sem);
}

void net_tcp_sema_init()
{
	os_sema_init(&net_tcp_sem,0);
}

int32 net_tcp_sema_down(int32 tmo_ms)
{
	return os_sema_down(&net_tcp_sem,tmo_ms);
}

void net_tcp_sema_up()
{
	os_sema_up(&net_tcp_sem);
}
uint16_t w_gol,h_gol;
static int net_video_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int ret = RET_OK;
    switch (cmd_id)
    {

        // 暂时没有考虑释放
        case MSI_CMD_POST_DESTROY:
        {
        }
        break;
        // 接收,判断是否已经压缩了
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;
			//_os_printf("&&");
			if(w_gol == 1280){
				if(fb->stype != FSTYPE_H264_VPP_DATA0)
				{
					ret = RET_ERR;
				}
			}else{
				if(fb->stype != FSTYPE_H264_GEN420_DATA)
				{
					ret = RET_ERR;
				}
			}
        }
        break;
        case MSI_CMD_FREE_FB:
        {
        }
        break;
        default:
            break;
    }
    return ret;
}

int usr_protocol_create_client(uint16_t port)
{
	int socket_c, err;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_len = sizeof(struct sockaddr_in);
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htons(INADDR_ANY);

	socket_c = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_c < 0)
	{
		CHILDREN_DBG("get socket err");
		return  - 1;
	} 

	err = bind(socket_c, (struct sockaddr*) &addr, sizeof(struct sockaddr_in));

	if (err ==  - 1)
	{
		close(socket_c);
		return  - 1;

	}	
	return socket_c;
}


void udp_handle_client_status_write_workqueue(void *ei, void *d){
	uint32 ie;
	status_msg *msg_head;
	int tos;
	char buf[12];
	int  len;
	msg_head = (status_msg *)buf;
	msg_head->framenum = client_frame.framenum;
	msg_head->type     = 1;
	if(client_frame.lost_num != 0){
		tos = IPTOS_PREC_NETCONTROL; // 最高优先级
    	setsockopt(handle_protocol_fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));
		len = sendto(handle_protocol_fd, (char*)buf, 2, MSG_DONTWAIT, (struct sockaddr *)d, sizeof(struct sockaddr));
		tos = IPTOS_PREC_ROUTINE; // 最低优先级
		setsockopt(handle_protocol_fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));
		
		CHILDREN_DBG("W(%x   %x)",buf[0],buf[1]);
	}else{
		ie = disable_irq();
		if(status_unlock){
			status_unlock = 0;
			net_h264_status_sema_up();
		}	
		enable_irq(ie);
		client_frame.status = 2;      //已经收到完整数据，没必要重发，只修改状态就可以
	}
}

void udp_handle_client_status_read_workqueue(){	
	int retval;
	int ret;
	uint32 ie;
	uint8_t itk;
	uint8_t new_status;
	status_msg *msg_head;
	
	struct sockaddr remote_addr;
	retval = 16;
	ret = recvfrom (handle_protocol_fd, server_staus_buf, 200, 0, &remote_addr, (socklen_t*)&retval);

	os_mutex_lock(&thread_lock, osWaitForever);
	msg_head = (status_msg *)server_staus_buf;
	new_status = client_frame.status;
	client_frame.status = 2;
	
	if(msg_head->framenum != client_frame.framenum){
		client_frame.status = new_status;
		os_mutex_unlock(&thread_lock);
		CHILDREN_DBG("E(%d)",msg_head->framenum);
		return;
	}
	os_mutex_unlock(&thread_lock);
	
	CHILDREN_DBG("server recv:%d  type:%d\r\n",msg_head->framenum,msg_head->type);
	
	if(msg_head->type == 0){
		client_frame.lost_num = 0;
	}else if(msg_head->type == 2){
		client_frame.lost_num = ret - 2;
		memset(client_frame.lost_packet,0xff,100);
		for(itk = 0;itk < client_frame.lost_num;itk++){
			client_frame.lost_packet[itk] = server_staus_buf[2+itk];
		}
	}

	ie = disable_irq();
	if(status_unlock){
		status_unlock = 0;
		net_h264_status_sema_up();
	}	
	enable_irq(ie);
}

extern in_addr_t send_addr;
void udp_handle_client_status_thread(void *d)
{
	uint32 ie;
	uint8_t loop_run;
	//uint16_t port = 6003;
	uint8_t  framenum;
	uint32_t start_tmr = 0;
	struct sockaddr_in addrServer;
	uint16_t *port;
	port = d;
	memset(&addrServer,0,sizeof(struct sockaddr_in));
	while(send_addr == 0){
		os_sleep_ms(10);
	}
	addrServer.sin_family=AF_INET;
	addrServer.sin_addr.s_addr=send_addr;//inet_addr("192.168.169.1");//client_addr;//
	addrServer.sin_port=htons(*port);

	handle_protocol_fd = usr_protocol_create_client(*port);
	eloop_add_fd( handle_protocol_fd, EVENT_READ, EVENT_F_ENABLED, udp_handle_client_status_read_workqueue, 0 );
	while(1){
		CHILDREN_DBG("D");
		net_h264_sema_down(-1);  //wait for data send finish
		CHILDREN_DBG("Q(%d)",client_frame.status);
		start_tmr = client_frame.time;
		framenum  = client_frame.framenum;
		os_sleep_ms(client_frame.timeout);
		loop_run = 0;
		while((client_frame.status == 0)&&(framenum  == client_frame.framenum)){         //如果当前frame还处于等待client状态的情况,发送请求状态的要求
			eloop_add_alarm(os_jiffies(),EVENT_F_ENABLED,udp_handle_client_status_write_workqueue,(void *)&addrServer);   //eventloop send
			//client_frame.timeout = 5;       //5ms都读不到对回复的状态,重发吧
			os_sleep_ms(5);
			loop_run++;
			if(loop_run > 8)   //重发8次后还是读不到状态,认命吧,你掉线了    
				break;
		}
		
		if(client_frame.status == 0){
			if(client_frame.framenum == framenum){    //要是新的frame来了,表示旧的frame已经正常结束,否则要唤醒当前帧
				ie = disable_irq();
				if(status_unlock){
					status_unlock = 0;
					net_h264_status_sema_up();
				}
				client_frame.status = 3;		 //frame timeout,lost
				enable_irq(ie);
			}
		}
		
	}
}

#if 1
void  recfg_mclk_msg(struct dvp_device * mclkdev,uint8_t success){
	static uint8_t  framecnt = 0;
	static uint8_t  lost_frame_num = 0;
	static uint8_t  success_frame_num = 0;
	static uint32_t timeout = 0;   
	static uint8_t  speed_level = 0;   //
	static uint8_t  last_speed_level = 0xff;
	int32 isstaconnect;
	if(apsta_mode == 2)	     //STA
	{
		isstaconnect = ieee80211_conf_get_stacnt(WIFI_MODE_STA);
		if(isstaconnect == 0){
			return;
		}
	}
	else{
		isstaconnect = ieee80211_conf_get_stacnt(WIFI_MODE_AP);
		if(isstaconnect == 0){
			return;
		}
	}

	if(framecnt > 15){        //15帧内丢帧比例
		if((os_jiffies() - timeout) > 1000){     //如果刚调整过mclk,那等3秒后再进行下次调整
			if(lost_frame_num > 12){
				timeout = os_jiffies();
				speed_level = 3;			
			}else if(lost_frame_num > 6){		         //15帧丢了6帧以上
				timeout = os_jiffies();
				speed_level = 2;
			}else if(lost_frame_num > 3){	     //15帧丢了3帧以上
				timeout = os_jiffies();
				speed_level = 1;			
			}else if(lost_frame_num == 0){	     //没丢帧
				if(speed_level == 0){
					speed_level = 0;
				}else{
					speed_level--;
				}	
			}

//			speed_level = ctrl_mode;

			if (ctrl_mode != 4) {
				speed_level = ctrl_mode;
			}

			if(last_speed_level != speed_level){
				last_speed_level = speed_level;
#if 0				
				if(speed_level == 0){
					h264_recfg_bsp(1,200);	
					h264_recfg_rate(1,25);
					h264_recfg_frm_gop(1,25);
					h264_recfg_ini_qp(1,26);
					//dvp_set_baudrate(mclkdev,24000000);	
				}else if(speed_level == 1){
					h264_recfg_bsp(1,150);	
					h264_recfg_rate(1,12);
					h264_recfg_frm_gop(1,12);
					h264_recfg_ini_qp(1,31);
					//dvp_set_baudrate(mclkdev,12000000);
				}else if(speed_level == 2){
					h264_recfg_bsp(1,50);	
					h264_recfg_rate(1,6);
					h264_recfg_frm_gop(1,6);
					h264_recfg_ini_qp(1,37);
					//dvp_set_baudrate(mclkdev,6000000);
				}
#endif				
				walkmsg.speed = speed_level;
			}
		}
		framecnt = 0;
		lost_frame_num = 0;
		success_frame_num = 0;
	}else{
		if(success){
			success_frame_num++;
		}else{
			lost_frame_num++;
		}
		framecnt++;
	}
	walkmsg.speed = speed_level;
}
#endif
void recfg_mclk_by_connect(){
	static int32 lastconnect;
	int32 isstaconnect;
	if(apsta_mode == 2){
		isstaconnect = ieee80211_conf_get_stacnt(WIFI_MODE_STA);
	}else{
		isstaconnect = ieee80211_conf_get_stacnt(WIFI_MODE_AP);
	}

	if((isstaconnect != lastconnect)&&(isstaconnect == 0)){     //连接掉线，时钟恢复成高帧率,重连之后再恢复mcs控制
		//dvp_set_baudrate(mclkdev,24000000);
		lastconnect = isstaconnect;
		return;
	}

	lastconnect = isstaconnect;

}


void recfg_mclk_by_mcs(uint8_t mcs,uint8_t frmtype)
{
	static uint8_t lastmcs;
	static uint8_t lastfrmtype;
	static uint8_t  speed_level = 0;

	if((lastfrmtype != frmtype) && (frmtype == 0)) {
		lastfrmtype = frmtype;
		if(walkmsg.speed == 4)
			return;
		speed_level = 4;
		h264_recfg_bsp(1,50);	
		h264_recfg_rate(1,6);
		h264_recfg_frm_gop(1,6);
		h264_recfg_ini_qp(1,37);
		walkmsg.speed = speed_level;
		h264_reflash_new_gop(1,1);
	}
	else if(lastmcs != mcs){
		lastmcs = mcs;
		if(current_bss_bw == 20) {
			if(lastmcs > 5){       //mcs 6 7
				if(walkmsg.speed == 0)
					return;
				speed_level = 0;
				h264_recfg_bsp(1,200);	
				h264_recfg_rate(1,25);
				h264_recfg_frm_gop(1,25);
				h264_recfg_ini_qp(1,26);
				h264_reflash_new_gop(1,1);
				//dvp_set_baudrate(mclkdev,24000000);				
			}else if(lastmcs > 3){  //mcs 4 5
				if(walkmsg.speed == 1)
					return;
				speed_level = 1;
				h264_recfg_bsp(1,150);	
				h264_recfg_rate(1,20);
				h264_recfg_frm_gop(1,20);
				h264_recfg_ini_qp(1,29);
				h264_reflash_new_gop(1,1);
				//dvp_set_baudrate(mclkdev,20000000);
			}else if(lastmcs > 2){  //mcs 3
				if(walkmsg.speed == 2)
					return;
				speed_level = 2;
				h264_recfg_bsp(1,100);	
				h264_recfg_rate(1,12);
				h264_recfg_frm_gop(1,12);
				h264_recfg_ini_qp(1,33);
				h264_reflash_new_gop(1,1);
				//dvp_set_baudrate(mclkdev,12000000);
			}else if(lastmcs > 1){   //mcs 2
				if(walkmsg.speed == 3)
					return;
				speed_level = 3;
				h264_recfg_bsp(1,50);	
				h264_recfg_rate(1,6);
				h264_recfg_frm_gop(1,6);
				h264_recfg_ini_qp(1,37);
				h264_reflash_new_gop(1,1);
				//dvp_set_baudrate(mclkdev,6000000);
			}else {                  //mcs 1 0
				if(walkmsg.speed == 4)  
					return;
				speed_level = 4;
				h264_recfg_bsp(1,50);	
				h264_recfg_rate(1,6);
				h264_recfg_frm_gop(1,6);
				h264_recfg_ini_qp(1,37);
				h264_reflash_new_gop(1,1);
			}
		}
		else if(current_bss_bw == 5) {
			if(lastmcs > 5) {
				if(walkmsg.speed == 2)
					return;	
				speed_level = 2;
				h264_recfg_bsp(1,100);	
				h264_recfg_rate(1,12);
				h264_recfg_frm_gop(1,12);
				h264_recfg_ini_qp(1,33);	
				h264_reflash_new_gop(1,1);		
			}
			else if(lastmcs > 3) {
				if(walkmsg.speed == 3)
					return;
				speed_level = 3;
				h264_recfg_bsp(1,50);	
				h264_recfg_rate(1,6);
				h264_recfg_frm_gop(1,6);
				h264_recfg_ini_qp(1,37);	
				h264_reflash_new_gop(1,1);			
			}
			else {
				if(walkmsg.speed == 4)  
					return;
				speed_level = 4;
				h264_recfg_bsp(1,50);	
				h264_recfg_rate(1,6);
				h264_recfg_frm_gop(1,6);
				h264_recfg_ini_qp(1,37);
				h264_reflash_new_gop(1,1);			
			}
		}
		walkmsg.speed = speed_level;
	}

}

void udp_handle_client_data_thread(void *d){
	int32 ret;
	uint32 ie;
	uint8_t framesuc = 0;
	uint8_t mcs = 0;
	uint8_t frmtype = 0;
	uint32_t temp;
	uint8_t mcslop=0;
	struct dvp_device *dvp_dev;	
	uint8_t  framenum;
	uint8_t  oldcount = 0;
	uint8_t  lostframe = 0;
	uint8_t  itk;
	uint8_t  lostidx = 0;
	uint8_t  lostloop = 0;
	uint32_t framelen = 0;
	uint32_t datoffset = 0;
	uint32_t sendlen = 0;
	uint8_t pktcnt = 0;
	struct fb_h264_s *h264;

	struct sockaddr_in addrServer;
	struct framebuff *h264_fb = NULL;
	int  len;
	data_head *data_head_msg;
	struct msi *msi;
	uint16_t *port;
	port = d;
	dvp_dev = (struct dvp_device *)dev_get(HG_DVP_DEVID);

	msi = msi_new("NET_H264", MAX_USER_VIDEO_TX, NULL);
	msi->action = net_video_msi_action;
	msi->enable = 1;
    msi_add_output(0, S_H264, "NET_H264");      //lvgl的msi输出到R_OSD_ENCODE的msi	

	while(send_addr == 0){
		os_sleep_ms(10);
	}
	framenum = 0;
	memset(&addrServer,0,sizeof(struct sockaddr_in));
	addrServer.sin_family=AF_INET;
	addrServer.sin_addr.s_addr=send_addr;//inet_addr("192.168.169.1");;//inet_addr("192.168.169.100");
	addrServer.sin_port=htons(*port);	
	handle_data_protocol_fd = usr_protocol_create_client(*port);
	data_head_msg = (data_head *)photo_buf;
	while(1){
		
	
		h264_fb = msi_get_fb(msi, 0);
			
	
		if (h264_fb){
			os_mutex_lock(&thread_lock, osWaitForever);
			h264 = (struct fb_h264_s *)h264_fb->priv;
			memset(data_head_msg,0,sizeof(data_head));
			data_head_msg->framenum = framenum;
			framenum++;
			data_head_msg->cnt = (h264_fb->len+MAX_VIDEO_PKT_LEN-1)/MAX_VIDEO_PKT_LEN;
			data_head_msg->frmtype = h264->type;
			framelen = h264_fb->len;
			CHILDREN_DBG("(%d   %d  %d)",data_head_msg->framenum,h264->type,framelen);
			client_frame.framenum = data_head_msg->framenum;
			client_frame.status = 0;

			os_mutex_unlock(&thread_lock);

			if(lostframe == 1){           //lost frame ,need to wait the I frame for re-send
				if(h264->type != 1){
					goto delete_frame;
				}
			}
		
			if((oldcount + 1) != h264->count){
				if((h264->count == 2)&&(oldcount == 255)){
					if(lostframe == 1){
						if(h264->type != 1){
							CHILDREN_DBG("send slow,lost frame wait i frame\r\n");
							goto delete_frame;
						}
					}
				}else{
					if(h264->type == 1){
						CHILDREN_DBG("frame lost ,but this frame is i frame ,send it\r\n");
					}else{
						CHILDREN_DBG("send slow ,lost frame ,wait I frame:%d  %d\r\n",oldcount,h264->count);
						h264_reflash_new_gop(1,1);
						lostframe = 1;
						goto delete_frame;
					}
				}
			}
			
			lostframe = 0;

			pktcnt = 0;
			datoffset = 0;

			client_frame.lost_num = data_head_msg->cnt; 
			framesuc = 1;
			while(framelen != 0){
				data_head_msg->pack = pktcnt;
				if(framelen >= MAX_VIDEO_PKT_LEN){
					framelen = framelen-MAX_VIDEO_PKT_LEN;
					memcpy(photo_buf+sizeof(data_head),h264_fb->data+datoffset,MAX_VIDEO_PKT_LEN);
					datoffset += MAX_VIDEO_PKT_LEN;
					sendlen = MAX_VIDEO_PKT_LEN;
				}else{					
					memcpy(photo_buf+sizeof(data_head),h264_fb->data+datoffset,framelen);
					sendlen = framelen;
					framelen = 0;
				}
				len = sendto(handle_data_protocol_fd, (char*)photo_buf, sendlen+sizeof(data_head), MSG_DONTWAIT, (struct sockaddr *)&addrServer, sizeof(struct sockaddr));
				walkmsg.tx_data += len;
				pktcnt++;
			}
			net_h264_sema_up();
			ie = disable_irq(); 
			status_unlock = 1;
			enable_irq(ie);
			ret = net_h264_status_sema_down(200);
			
			CHILDREN_DBG("status:%d lostnum:%d\r\n",client_frame.status,client_frame.lost_num);

			framelen = h264_fb->len;
			pktcnt = 0;
			datoffset = 0;
			lostidx   = 0;
			lostloop  = 3;    //所有丢包都重传3次,增加接收成功率

			//if((walkmsg.speed >= 4) || (current_bss_bw == 5)) {
			if((walkmsg.speed == 3) || (current_bss_bw == 5)) {
				client_frame.lost_num = 0;
			}else if(walkmsg.speed == 2) {
				lostloop = 1;
			}
			
			if(client_frame.lost_num != 0){
				while(framelen != 0){
					data_head_msg->pack = pktcnt;
					if(framelen >= MAX_VIDEO_PKT_LEN){
						framelen = framelen-MAX_VIDEO_PKT_LEN;
						memcpy(photo_buf+sizeof(data_head),h264_fb->data+datoffset,MAX_VIDEO_PKT_LEN);
						datoffset += MAX_VIDEO_PKT_LEN;
						sendlen = MAX_VIDEO_PKT_LEN;
					}else{					
						memcpy(photo_buf+sizeof(data_head),h264_fb->data+datoffset,framelen);
						sendlen = framelen;
						framelen = 0;
					}
					if(pktcnt == client_frame.lost_packet[lostidx]){
						lostidx++;
						for(itk = 0;itk < lostloop;itk++){
							len = sendto(handle_data_protocol_fd, (char*)photo_buf, sendlen+sizeof(data_head), MSG_DONTWAIT, (struct sockaddr *)&addrServer, sizeof(struct sockaddr));
							os_sleep_ms(1);
							walkmsg.tx_data += len;
						}
					}
					pktcnt++;
				}
				client_frame.status = 0;
				net_h264_sema_up();
				
				ie = disable_irq(); 
				status_unlock = 1;
				enable_irq(ie);
				ret = net_h264_status_sema_down(200);

				CHILDREN_DBG("status:%d down:%d\r\n",client_frame.status,ret);
				if(client_frame.status == 3){
					h264_reflash_new_gop(1,1);
					CHILDREN_DBG("frame already lost,wait I frame.......\r\n");
					framesuc = 0;
				}
			}

			if(framesuc){
				walkmsg.frame_tx_success++;
			}else{
				walkmsg.frame_tx_lost++;
			}

#if 0
			if(apsta_mode == 2){
				//ieee80211_conf_get_bssid(WIFI_MODE_STA,&connectmsg.addr);
				temp = ieee80211_conf_get_tx_mcs(WIFI_MODE_STA,NULL,0)&0xff;
				frmtype += (temp>>4)&0x0f;
				mcs += (temp&0x0f);
			}else{
				//ieee80211_conf_get_stalist(WIFI_MODE_AP,&connectmsg,1);
				temp = ieee80211_conf_get_tx_mcs(WIFI_MODE_AP,NULL,1)&0xff;
				frmtype += (temp>>4)&0x0f;
				mcs += (temp&0x0f);
			}
			mcslop++;
			if(mcslop == 10){
				mcslop = 0;
				walkmsg.mcs = mcs/10;
				walkmsg.frmtype = frmtype/10;
				mcs = 0;
				frmtype = 0;
				recfg_mclk_by_mcs(walkmsg.mcs,walkmsg.frmtype);
			}
			
#else			
			recfg_mclk_msg(dvp_dev,framesuc);

#endif
			
delete_frame:
			//_os_printf("U(%d %d)",h264->count,lostframe);
			oldcount = h264->count;
			msi_delete_fb(NULL, h264_fb);			
		}else{
			os_sleep_ms(3);
			recfg_mclk_by_connect();
		}
	}
}

static uint16_t port_data;
static uint16_t port_status;
void udp_handle_client_init(uint16_t status_port,uint16_t data_port)
{
	port_data   = data_port; 
	port_status = status_port;
	csi_kernel_task_new((k_task_entry_t)udp_handle_client_status_thread, "handle_udp_pkt", &port_status, 25, 0, NULL, 1024, &handle_task_recv);
	csi_kernel_task_new((k_task_entry_t)udp_handle_client_data_thread, "handle_data_udp_pkt", &port_data, 25, 0, NULL, 1024, &handle_data_task_recv);	
}

void client_frame_msg_init(uint8_t frame_num,uint8_t timeout){
	client_frame.framenum = frame_num;
	client_frame.time = 0;
	client_frame.status = 0;
	client_frame.lost_num = 0;
	client_frame.timeout = timeout;
	memset(client_frame.lost_packet,0xff,100);
}


void protocol_client_init(uint16_t status_port,uint16_t data_port){
	os_mutex_init(&thread_lock);
	net_h264_sema_init();
	net_h264_status_sema_init();
	client_frame_msg_init(0,10);
	udp_handle_client_init(status_port,data_port);
	//net_tcp_sema_init();
	//tcp_handle_client_init();
}

void user_protocol3(uint16_t status_port,uint16_t data_port)
{
    protocol_client_init(status_port,data_port);        //发送摄像头数据    			STA  
}

int32 atcmd_current_bss_bw(const char *cmd, char *argv[], uint32 argc)
{
	if(argc < 1) {
        os_printf("%s argc err:%d,enter the bss_bw\n",__FUNCTION__,argc);
        return 0;
    }
	if(argv[0]) {
		current_bss_bw = os_atoi(argv[0]);
		os_printf("\n***current_bss_bw:%d***\n",current_bss_bw);
		if(current_bss_bw == 20) {
			atcmd_recv((uint8_t*)"AT+BSS_BW=20",0);    
			lmac_set_beacon_modulation(NULL, LMAC_RATE_DSSS_CCK_RATE0);
			lmac_set_supp_rate(NULL, WIFI_TX_SUPP_RATE);
		}
		else {
			atcmd_recv((uint8_t*)"AT+BSS_BW=5",0);
			lmac_set_beacon_modulation(NULL, LMAC_RATE_NON_HT_RATE0);
			lmac_set_supp_rate(NULL, WIFI_TX_SUPP_RATE & 0xFFFFFFF0);
		}		
	}
	return 0;
}

void protocol_client_send_enable(uint8_t enable)
{
	struct msi *h264_msi = NULL;
	h264_msi = msi_find(S_H264, 1);
	if(h264_msi) {
		msi_put(h264_msi);
		if(enable)
			msi_add_output(h264_msi, NULL, "NET_H264");
		else
			msi_del_output(h264_msi, NULL, "NET_H264");
	}	
}

int32 atcmd_client_send_enable(const char *cmd, char *argv[], uint32 argc)
{
	if(argv[0]) {
		uint8_t enable = os_atoi(argv[0]);
		protocol_client_send_enable(enable);
	}
	return 0;
}

int32 atcmd_client_send_mode(const char *cmd, char *argv[], uint32 argc)
{
	if(argv[0]) {
		ctrl_mode = os_atoi(argv[0]);
	}
	return 0;	
}

#endif