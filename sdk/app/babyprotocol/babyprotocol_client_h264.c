#include "basic_include.h"
#include "csi_kernel.h"
#include "syscfg.h"
#include "lwip/sockets.h"
#include "lib/lcd/lcd.h"
#include "netif/ethernetif.h"
#include "stream_define.h"
#include "stream_frame.h"
#include "babyprotocol_h264.h"
#include "lib/multimedia/msi.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

#ifdef SYS_APP_BBM_CAM

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

frame_msg client_frame;
uint32_t  client_dev_magic = 0;
os_mutex_t      thread_lock;

static struct os_semaphore net_h264_sem = {0,NULL};
static struct os_semaphore net_h264_status_sem = {0,NULL};
static struct os_semaphore net_tcp_sem = {0,NULL};

static k_task_handle_t handle_task_recv;
static k_task_handle_t handle_data_task_recv;
static k_task_handle_t handle_tcp_task_recv;

static int handle_protocol_fd   =  - 1;
static int handle_data_protocol_fd   =  - 1;
static int tcp_connect_fd = -1;
static volatile uint32 status_unlock = 0;

static in_addr_t client_addr = 0;
uint8_t photo_buf[1440] __attribute__ ((aligned(4)));;
uint8_t server_staus_buf[200];
volatile uint32 server_frame_rate;

uint32_t heartbeat = 0;

EVT_HDL tcp_read_ev;

static uint8_t src_filter_type = FSTYPE_H264_GEN420_DATA;

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
	static uint16_t oldw,oldh;
	struct fb_h264_s *h264;
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
			if(src_filter_type != FSTYPE_H264_FILE)	{
				if(oldw != w_gol){
					h264 = (struct fb_h264_s *)fb->priv;			//切换分辨率时要以I帧为基础
					if(h264->type == 1){
						oldw = w_gol;
					}
				}
				
				if(oldw == 1280) {
					src_filter_type = FSTYPE_H264_VPP_DATA0;
				}
				else {
					src_filter_type = FSTYPE_H264_GEN420_DATA;
				}
			}			
			if(fb->stype != src_filter_type)
			{
				ret = RET_ERR;
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

int usr_protocol_create_server(uint16_t port)
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
		printf("get socket err");
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
	char buf[12];
	int  len;
	msg_head = (status_msg *)buf;
	msg_head->framenum = client_frame.framenum;
	msg_head->type     = 1;
	if(client_frame.lost_num != 0){
		len = sendto(handle_protocol_fd, (char*)buf, 2, MSG_DONTWAIT, (struct sockaddr *)d, sizeof(struct sockaddr));
		// os_printf("W");
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
		BABY_DBG("E(%d)",msg_head->framenum);
		return;
	}
	os_mutex_unlock(&thread_lock);
	
	BABY_DBG("server recv:%d  type:%d\r\n",msg_head->framenum,msg_head->type);
	
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

void udp_handle_client_status_thread()
{
	uint32 ie;
	uint8_t loop_run;
	uint16_t port = 6003;
	uint8_t  framenum;
	uint32_t start_tmr = 0;
	struct sockaddr_in addrServer;
	memset(&addrServer,0,sizeof(struct sockaddr_in));
	while(client_addr == 0){
		os_sleep_ms(100);
	}
	addrServer.sin_family=AF_INET;
	addrServer.sin_addr.s_addr=client_addr;//inet_addr("192.168.169.100");
	addrServer.sin_port=htons(6003);

	handle_protocol_fd = usr_protocol_create_server(port);
	eloop_add_fd( handle_protocol_fd, EVENT_READ, EVENT_F_ENABLED, udp_handle_client_status_read_workqueue, 0 );
	while(1){
		// _os_printf("D");
		net_h264_sema_down(-1);  //wait for data send finish
		BABY_DBG("Q(%d)",client_frame.status);
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

void udp_handle_client_data_thread(){
	uint8_t loop_run;	
	int32 ret;
	uint32 ie;
	int init_count = 0;
	void *priv;
	uint16_t port = 6002;
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
	uint32_t start_tmr = 0;
	struct sockaddr_in addrServer;
	struct framebuff *h264_fb = NULL;
	int  len;
	data_head *data_head_msg;
	struct msi *msi;
	uint32_t nal_reserve = 0;

	msi = msi_new("NET_H264", MAX_USER_VIDEO_TX, NULL);
	msi->action = net_video_msi_action;
	msi->enable = 1;

	while(client_addr == 0){
		os_sleep_ms(100);
	}
	framenum = 0;
	memset(&addrServer,0,sizeof(struct sockaddr_in));
	addrServer.sin_family=AF_INET;
	addrServer.sin_addr.s_addr=client_addr;//inet_addr("192.168.169.100");
	addrServer.sin_port=htons(6002);	
	handle_data_protocol_fd = usr_protocol_create_server(port);
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
			BABY_DBG("(%d   %d  %d)",data_head_msg->framenum,h264->type,framelen);

			client_frame.framenum = data_head_msg->framenum;
			client_frame.status = 0;

			os_mutex_unlock(&thread_lock);

			if(lostframe == 1){           //lost frame ,need to wait the I frame for re-send
				if(h264->type != 1){
					goto delete_frame;
				}
			}
		
			if((oldcount + 1) != h264->count){
				if((h264->count == 0)&&(oldcount == 255)){  
					if(lostframe == 1){
						if(h264->type != 1){
							os_printf("send slow,lost frame wait i frame\r\n");
							goto delete_frame;
						}
					}
				}else{
					if(h264->type == 1){
						os_printf("frame lost ,but this frame is i frame ,send it\r\n");
					}else{
						os_printf("send slow ,lost frame ,wait I frame:%d  %d\r\n",oldcount,h264->count);
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

			nal_reserve = 0;
			if(h264_fb->stype == FSTYPE_H264_FILE) {
				if(h264->type == 1) {
					nal_reserve = h264->pps_len + h264->sps_len + 12;
				}
				else if(h264->type == 2) {
					nal_reserve = 4;
				}
			}
			while(framelen != 0){
				data_head_msg->pack = pktcnt;
				if(nal_reserve > 0) {
					if(h264->type == 1) {
						*((uint32_t*)(photo_buf+sizeof(data_head))) = 0x1000000;
						os_memcpy(photo_buf+sizeof(data_head)+4, h264->sps, h264->sps_len);
						*((uint32_t*)(photo_buf+sizeof(data_head)+h264->sps_len+4)) = 0x1000000;
						os_memcpy(photo_buf+sizeof(data_head)+h264->sps_len+8, h264->pps, h264->pps_len);
						*((uint32_t*)(photo_buf+sizeof(data_head)+h264->pps_len+h264->sps_len+8)) = 0x1000000;
					}
					else {
						*((uint32_t*)(photo_buf+sizeof(data_head))) = 0x1000000;
					}
				}
				if(framelen >= (MAX_VIDEO_PKT_LEN - nal_reserve)){
					framelen = framelen - (MAX_VIDEO_PKT_LEN-nal_reserve);
					memcpy(photo_buf+sizeof(data_head)+nal_reserve,h264_fb->data+datoffset,(MAX_VIDEO_PKT_LEN-nal_reserve));
					datoffset += (MAX_VIDEO_PKT_LEN-nal_reserve);
					sendlen = MAX_VIDEO_PKT_LEN;
				}else{					
					memcpy(photo_buf+sizeof(data_head)+nal_reserve,h264_fb->data+datoffset,framelen);
					sendlen = framelen + nal_reserve;
					framelen = 0;
				}
				if(nal_reserve > 0)
					nal_reserve = 0;
				len = sendto(handle_data_protocol_fd, (char*)photo_buf, sendlen+sizeof(data_head), MSG_DONTWAIT, (struct sockaddr *)&addrServer, sizeof(struct sockaddr));
				pktcnt++;
			}
			net_h264_sema_up();
			ie = disable_irq(); 
			status_unlock = 1;
			enable_irq(ie);
			ret = net_h264_status_sema_down(200);
			
			BABY_DBG("status:%d lostnum:%d\r\n",client_frame.status,client_frame.lost_num);

			framelen = h264_fb->len;
			pktcnt = 0;
			datoffset = 0;
			lostidx   = 0;
			lostloop  = 3;    //所有丢包都重传3次,增加接收成功率
			
			if(client_frame.lost_num != 0){
				nal_reserve = 0;
				if(h264_fb->stype == FSTYPE_H264_FILE) {
					if(h264->type == 1) {
						nal_reserve = h264->pps_len + h264->sps_len + 12;
					}
					else if(h264->type == 1) {
						nal_reserve = 4;
					}
				}
				if(nal_reserve > 0) {
					if(h264->type == 1) {
						*((uint32_t*)(photo_buf+sizeof(data_head))) = 0x1000000;
						os_memcpy(photo_buf+sizeof(data_head)+4, h264->sps, h264->sps_len);
						*((uint32_t*)(photo_buf+sizeof(data_head)+h264->sps_len+4)) = 0x1000000;
						os_memcpy(photo_buf+sizeof(data_head)+h264->sps_len+8, h264->pps, h264->pps_len);
						*((uint32_t*)(photo_buf+sizeof(data_head)+h264->pps_len+h264->sps_len+8)) = 0x1000000;
					}
					else {
						*((uint32_t*)(photo_buf+sizeof(data_head))) = 0x1000000;
					}
				}
				while(framelen != 0){
					data_head_msg->pack = pktcnt;
					if(framelen >= (MAX_VIDEO_PKT_LEN - nal_reserve)){
						framelen = framelen - (MAX_VIDEO_PKT_LEN-nal_reserve);
						memcpy(photo_buf+sizeof(data_head)+nal_reserve,h264_fb->data+datoffset,(MAX_VIDEO_PKT_LEN-nal_reserve));
						datoffset += (MAX_VIDEO_PKT_LEN-nal_reserve);
						sendlen = MAX_VIDEO_PKT_LEN;
					}else{					
						memcpy(photo_buf+sizeof(data_head)+nal_reserve,h264_fb->data+datoffset,framelen);
						sendlen = framelen + nal_reserve;
						framelen = 0;
					}
					if(pktcnt == client_frame.lost_packet[lostidx]){
						lostidx++;
						for(itk = 0;itk < lostloop;itk++){
							len = sendto(handle_data_protocol_fd, (char*)photo_buf, sendlen+sizeof(data_head), MSG_DONTWAIT, (struct sockaddr *)&addrServer, sizeof(struct sockaddr));
							os_sleep_ms(1);
						}
					}
					if(nal_reserve > 0)
						nal_reserve = 0;
					pktcnt++;
				}
				client_frame.status = 0;
				net_h264_sema_up();
				
				ie = disable_irq(); 
				status_unlock = 1;
				enable_irq(ie);
				ret = net_h264_status_sema_down(200);

				BABY_DBG("status:%d down:%d\r\n",client_frame.status,ret);
				if(client_frame.status == 3){
					h264_reflash_new_gop(1,1);
					os_printf("frame already lost,wait I frame.......\r\n");
				}
			}
			
delete_frame:
			//_os_printf("U(%d %d)",h264->count,lostframe);
			oldcount = h264->count;
			msi_delete_fb(NULL, h264_fb);			
		}else{
			os_sleep_ms(3);
		}
	}
}


void udp_handle_client_init()
{
	csi_kernel_task_new((k_task_entry_t)udp_handle_client_status_thread, "handle_udp_pkt", 0, 25, 0, NULL, 1024, &handle_task_recv);
	csi_kernel_task_new((k_task_entry_t)udp_handle_client_data_thread, "handle_data_udp_pkt", 0, 25, 0, NULL, 1024, &handle_data_task_recv);	
}



void user_tcp_write_workqueue(void *e, void *d){
	int ret;
	connect_cfg_head tcp_hand;
	if(client_dev_magic == 0)
		tcp_hand.type = 0;
	else
		tcp_hand.type = 1;

	if(src_filter_type == FSTYPE_H264_FILE) {
		tcp_hand.w	          = 1280;
		tcp_hand.h	          = 720;
	}
	else {
		tcp_hand.w	          = w_gol;
		tcp_hand.h	          = h_gol;
	}
	tcp_hand.packet_len	  = MAX_VIDEO_PKT_LEN;
	tcp_hand.ip_grp       = 25;
	tcp_hand.frame_rate   = 20;
	tcp_hand.dev_magic = client_dev_magic;
	ret = send(tcp_connect_fd,&tcp_hand,sizeof(tcp_hand),0);
	BABY_DBG("send tcp heartbeat:%d\r\n",ret);
}

void user_tcpClientread(void *e, void *d)
{
	struct h264_device *h264_dev;
	static struct msi *h264_msi;
	static uint16_t w;
	static uint16_t h;
	int ret;
	uint8_t tcpbuf[64];
	connect_cfg_head tcp_hand;
	h264_dev = (struct h264_device *)dev_get(HG_H264_DEVID);
	ret = read(tcp_connect_fd,tcpbuf,64);
	if(ret > 0) {
		//memcpy(&tcp_hand,tcpbuf,sizeof(tcp_hand));
		memcpy(&tcp_hand,tcpbuf,sizeof(tcp_hand));
		BABY_DBG("get len:%d  tcpbuf:%02x  w:%d  h:%d............\r\n",ret,tcp_hand.type,tcp_hand.w,tcp_hand.h);
		if(tcp_hand.type == 0){
			heartbeat++;
		}else{
			BABY_DBG("start tran photo\r\n");
			if(tcp_hand.type == 2){
				if(client_dev_magic == 0) {    //第一次连接
					client_dev_magic = tcp_hand.dev_magic;
					h264_msi = msi_find(AUTO_H264, 1);
					if(h264_msi) {
						msi_put(h264_msi);
						msi_add_output(h264_msi, NULL, "NET_H264");
					}				
				}
				w = tcp_hand.w;
				h = tcp_hand.h;
				w_gol = w;
				h_gol = h;							
			}
		}		
	}else{
		// _os_printf("%s %d\r\n",__func__,__LINE__);
		eloop_remove_event( tcp_read_ev );
		tcp_read_ev = NULL;
	}
	//net_tcp_sema_up();
}

void tcp_handle_client_thread(){	//摄像头端
	connect_cfg_head tcp_hand;
	int32 isstaconnect;
	int32 ret;
	struct sockaddr_in addr;
    os_memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(6001);
    addr.sin_addr.s_addr = inet_addr("192.168.169.1");
	os_sleep_ms(1000);
reconnect:
	heartbeat = 0;
	ret = connect(tcp_connect_fd, (const struct sockaddr *)&addr, sizeof(struct sockaddr));
	if(ret < 0){
		closesocket(tcp_connect_fd);
		tcp_connect_fd = socket(AF_INET, SOCK_STREAM, 0); 
		printf("connect error(%d)..\r\n",tcp_connect_fd);
		os_sleep_ms(500);
		goto reconnect;
	}else{
		printf("connect finish\r\n");
	}
	
	
	client_addr = addr.sin_addr.s_addr;
	tcp_read_ev = eloop_add_fd( tcp_connect_fd, EVENT_READ, EVENT_F_ENABLED, user_tcpClientread, 0 );
	
#if 1	
	while(1){
      	ip_addr_t ipaddr;
      	ipaddr = lwip_netif_get_ip2("w0");		
		isstaconnect = ieee80211_conf_get_stacnt(WIFI_MODE_STA);

		BABY_DBG("ipaddr:%x  sta:%d\r\n",ipaddr.addr,ieee80211_conf_get_stacnt(WIFI_MODE_STA));

		if(isstaconnect == 0){								//我们为sta,发现连接数为0，则表示当前连接断开，tcp事件关闭并等待重连
			_os_printf("sta lost connect,reconnect TCP \r\n");
			eloop_remove_event( tcp_read_ev );
			tcp_read_ev = NULL;
		}
		
		if(tcp_read_ev == NULL){
			closesocket( tcp_connect_fd );
			tcp_connect_fd = -1;				
			tcp_connect_fd = socket(AF_INET, SOCK_STREAM, 0); 
			os_sleep_ms(500);
			goto reconnect;
		}else{
			eloop_add_alarm(os_jiffies(),EVENT_F_ENABLED,user_tcp_write_workqueue,0);
		}		
		
		os_sleep_ms(1000);	
	}
#endif	
}


void tcp_handle_client_thread_init(){		
	tcp_connect_fd = socket(AF_INET, SOCK_STREAM, 0); 
	csi_kernel_task_new((k_task_entry_t)tcp_handle_client_thread, "handle_connect_tcp_pkt", NULL, 25, 0, NULL, 1024, &handle_tcp_task_recv);
}

void tcp_handle_client_init(){
	tcp_handle_client_thread_init();
}

void client_frame_msg_init(uint8_t frame_num,uint8_t timeout){
	client_frame.framenum = frame_num;
	client_frame.time = 0;
	client_frame.status = 0;
	client_frame.lost_num = 0;
	client_frame.timeout = timeout;
	memset(client_frame.lost_packet,0xff,100);
}

void protocol_client_init(){
	os_mutex_init(&thread_lock);
	net_h264_sema_init();
	net_h264_status_sema_init();
	client_frame_msg_init(0,10);
	udp_handle_client_init();
	//net_tcp_sema_init();
	tcp_handle_client_init();
}

void protocol_client_send_enable(uint8_t enable)
{
	struct msi *h264_msi = NULL;
	h264_msi = msi_find(AUTO_H264, 1);
	if(h264_msi) {
		msi_put(h264_msi);
		if(enable)
			msi_add_output(h264_msi, NULL, "NET_H264");
		else
			msi_del_output(h264_msi, NULL, "NET_H264");
	}	
}

void protocol_client_filtertype(uint8_t type)
{
	src_filter_type = type;
}

void user_protocol()
{
	protocol_client_filtertype(FSTYPE_H264_GEN420_DATA);
    protocol_client_init();        //发送摄像头数据    			STA  
}

#endif