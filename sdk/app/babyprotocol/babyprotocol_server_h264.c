#include "basic_include.h"
#include "csi_kernel.h"
#include "lwip/sockets.h"
#include "lib/lcd/lcd.h"
#include "netif/ethernetif.h"
#include "stream_define.h"
#include "stream_frame.h"
#include "babyprotocol_h264.h"
#include "lib/multimedia/msi.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "scale_msi/scale_msi.h"
#include "lib/video/h264/h264_drv.h"

#ifdef SYS_APP_BBM_LCD

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE av_free
#define STREAM_LIBC_ZALLOC av_zalloc

#define MAX_VIDEO_PKT_LEN 1430

static int8_t next_switch_device = 2;
static int8_t cur_switch_device = 2;

//static struct os_semaphore net_h264_sem = {0,NULL};
struct os_msgqueue net_h264_msg;

static int handle_protocol_fd   =  - 1;
static int handle_data_protocol_fd   =  - 1;

int handle_protocol_table_fd[10];
int handle_data_protocol_table_fd[10];

static int tcp_connect_fd = -1;



//static k_task_handle_t handle_tcp_task_recv;
//static in_addr_t client_addr = 0;



volatile uint32 server_frame_rate;

#define STA_NUM        2
#define DEC_TABLE_NUM  32
decode_msg decmsg[DEC_TABLE_NUM];


uint32_t devnum = 0;
dev_map devtab[11];
frame_msg server_frame[10];

static volatile target_resolution server_resolution[STA_NUM];

uint8_t photo_buf[1440] __attribute__ ((aligned(4)));;
uint8 psram_h264_photo[200*1024] __attribute__ ((aligned(4),section(".psram.src")));
int frame_for_dec = 0;
uint8_t dispnum;
uint16_t rx_speed,tx_speed;
EVT_HDL udp_read_status_ev;
extern struct msi *scale2_msi(const char *name, uint16_t iw, uint16_t ih, uint16_t ow, uint16_t oh, uint16_t type,uint8_t larger);
extern void get_h264_stream_w_h(uint16_t* w,uint16_t* h,uint8_t *h264data);
void net_h264_sema_init()
{
	os_msgq_init(&net_h264_msg,4);
}

uint32 net_h264_sema_down(int32 tmo_ms)
{
	uint32 retval;
	uint32 retdata;
	//os_sema_down(&net_h264_sem,tmo_ms);	
	retdata = os_msgq_get2(&net_h264_msg, tmo_ms, (int32_t*)(&retval));
	if(retval == 0){
		return retdata;
	}else{
		return retval;
	}
}

void net_h264_sema_up(uint32 clientaddr)
{
	int32 ret;
	ret = os_msgq_put(&net_h264_msg, clientaddr, osWaitForever);
	if(ret != 0){
		_os_printf("wakeup msg err:%d\r\n",ret);
	}
	
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

int usr_protocol_create_server_with_ip(uint16_t port,uint32_t ipaddr)
{
	int socket_c, err;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_len = sizeof(struct sockaddr_in);
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htons(INADDR_ANY);//ipaddr;//
	//_os_printf("IP:%x  %x  %x\r\n",ipaddr,htons(INADDR_ANY),inet_addr("192.168.169.100"));
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


void udp_handle_server_status_write_workqueue(void *ei, void *d){
	int tos;
	struct sockaddr_in *addrServer;
	status_msg *msg_head;
	char buf[12];
	int  len;
	uint8_t itk;
	uint8_t id = 0;
	uint8_t fd;
	addrServer = d;
	for(itk = 0;itk < 10;itk++){
		if(addrServer->sin_addr.s_addr == devtab[itk].ipaddr){
			id = devtab[itk].dev_id;
			fd = devtab[itk].udp_status_fd;
		}
	}
	
	msg_head = (status_msg *)buf;
	msg_head->framenum = server_frame[id-1].framenum;
	msg_head->type     = 0;
	//len = sendto(fd, (char*)buf, 2, MSG_DONTWAIT, (struct sockaddr *)d, sizeof(struct sockaddr));
	tos = IPTOS_PREC_NETCONTROL; // 最高优先级
	setsockopt(handle_protocol_fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));	
	len = sendto(handle_protocol_fd, (char*)buf, 2, MSG_DONTWAIT, (struct sockaddr *)d, sizeof(struct sockaddr));
	tos = IPTOS_PREC_ROUTINE; // 最低优先级
	setsockopt(handle_protocol_fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));	
	
}

void udp_handle_server_status_read_workqueue(void *ei, void *status_fd){	
	int tos;
	int retval;
	int ret;
	uint8_t itk;
	uint8_t handlebuf[96];
	struct sockaddr remote_addr;
	struct sockaddr_in *addrServer;
	status_msg *msg_head;
	uint8_t id = 0;
	msg_head = (status_msg *)handlebuf;
	retval = 16;
	int32_t fd = (int32_t)status_fd;
	ret = recvfrom (fd, handlebuf, 24, 0, &remote_addr, (socklen_t*)&retval);
//	_os_printf(" G");
	addrServer = (struct sockaddr_in *)(&remote_addr);
	for(itk = 0;itk < 10;itk++){
		if(addrServer->sin_addr.s_addr == devtab[itk].ipaddr){
			id =  devtab[itk].dev_id;
		}
	}
	
	if((id == 0)||(id > 3)) 				   //当前设备号不存在,先只支持三台设备
	{
		BABY_DBG(" status no this client dev....");
		return;
	}

	id = id-1;								  //设备号是从1开始计算,这里让设备号改成从0开始算
	//os_printf("client read:%02d   type:%d   lost_num:%d \r\n",msg_head->framenum,msg_head->type,server_frame[id-1].lost_num);
	if(server_frame[id].lost_num != 0){
		BABY_DBG("L(%d %d)",id,server_frame[id].lost_num);
	}else{
		BABY_DBG("R%d",id);
	}
	
	if(server_frame[id].framenum != msg_head->framenum){
		BABY_DBG("ag%d",id);
		return;
	}

	if(server_frame[id].lost_num != 0){
		msg_head = (status_msg *)handlebuf;
		msg_head->framenum = server_frame[id].framenum;
		msg_head->type	   = 2;
		for(itk = 0;itk < server_frame[id].lost_num;itk++){
			handlebuf[2+itk] = server_frame[id].lost_packet[itk];
		}
	}else{
		msg_head->framenum = server_frame[id].framenum;
		msg_head->type	   = 0;
	}
	BABY_DBG("S(%d  %d)",id,msg_head->type);
	tos = IPTOS_PREC_NETCONTROL; // 最高优先级
	setsockopt(fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));		
	sendto(fd, handlebuf, 2+server_frame[id].lost_num, MSG_DONTWAIT, &remote_addr, sizeof(struct sockaddr));
	tos = IPTOS_PREC_ROUTINE; // 最低优先级
	setsockopt(fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));	
}


void udp_handle_server_status_thread(void *d){	
	struct sockaddr_in addrServer;
	in_addr_t cli_addr;
	dev_map *dev_tbl;
	dev_tbl = d;
	memset(&addrServer,0,sizeof(struct sockaddr_in));
	
	addrServer.sin_family=AF_INET;
	addrServer.sin_addr.s_addr= inet_addr("255.255.255.255");//client_addr;//inet_addr("192.168.169.1");
	addrServer.sin_port=htons(6003);
	//dev_tbl->udp_read_status_ev = eloop_add_fd( dev_tbl->udp_status_fd, EVENT_READ, EVENT_F_ENABLED, udp_handle_server_status_read_workqueue, &dev_tbl->udp_status_fd );
	dev_tbl->udp_read_status_ev = eloop_add_fd( handle_protocol_fd, EVENT_READ, EVENT_F_ENABLED, udp_handle_server_status_read_workqueue, (void*)handle_protocol_fd );
	
	while(1){
		cli_addr = net_h264_sema_down(-1);
		addrServer.sin_addr.s_addr = cli_addr;
		eloop_add_alarm(os_jiffies(),EVENT_F_ENABLED,udp_handle_server_status_write_workqueue,(void *)&addrServer);   //eventloop send
	}
}

uint8_t mark_lost_pkt(uint8_t pkt,uint8_t *rxbuf){
	uint8_t get_cnt[100];
	uint32_t itk;
	uint8_t pktrx = 0;
	for(itk = 0;itk < 100;itk++){
		if(rxbuf[itk] != 0xff){
			if(pkt == rxbuf[itk]){				 //接收到的清空
				rxbuf[itk] = 0xff;
			}else{
				get_cnt[pktrx] = rxbuf[itk];     //多少个pkt还未接收
				pktrx++;
			}
		}
	}

	memset(rxbuf,0xff,100);
	//server_frame.lost_num = pktrx;
	for(itk = 0;itk < pktrx;itk++){
		rxbuf[itk] = get_cnt[itk];
	}

	return pktrx;
}

void udp_handle_server_data_thread(uint32_t *d){
	uint32 timer_ref = 0;
	uint32 timer_ref2 = 0;
	int retval;
	int ret;
	int len;
	int itk = 0;

	uint16_t w = 0;
	uint16_t h = 0;
	struct sockaddr_in remote_addr;
	uint16_t rx_speed_cnt = 0;
	uint16_t dispnum_cnt = 0;
	uint8_t  push_lcd_success[10];
	uint16_t framenum[10];
	uint32_t framelen[10];
	uint16_t oldframecnt[10];
	uint8_t id;
	uint32_t framerate = 0;
	uint8_t type[10];
	uint8_t cntnum[10];
	uint8_t pktnum[10];
	uint8_t *fbuf;
	uint32 ie;
	uint8_t *psarm_room = NULL;
	data_head *frame_hand;
	retval = 16;

	//framenum = 0xffff;    //初始化值
	memset(framenum,0xff,sizeof(framenum));
	memset(oldframecnt,0,sizeof(oldframecnt));
	memset(framelen,0,sizeof(framelen));
	memset(cntnum,0,sizeof(cntnum));
	memset(pktnum,0,sizeof(pktnum));
	memset(type,0,sizeof(type));
	frame_hand = (data_head *)photo_buf;
	while(1){
		//ret = recvfrom (dev_tbl->udp_data_fd, photo_buf, MAX_VIDEO_PKT_LEN+sizeof(data_head), 0, &remote_addr, (socklen_t*)&retval);
		ret = recvfrom (handle_data_protocol_fd, photo_buf, MAX_VIDEO_PKT_LEN+sizeof(data_head), 0, (struct sockaddr*)(&remote_addr), (socklen_t*)&retval);
		if(next_switch_device == -1) {
			continue;
		}
		id = 0;
		for(itk = 0;itk < 10;itk++){
			if(remote_addr.sin_addr.s_addr == devtab[itk].ipaddr){
				id =  devtab[itk].dev_id;
				psarm_room = devtab[itk].psram_photo;
				framerate  = devtab[itk].frame_rate;
				w = devtab[itk].w;
				h = devtab[itk].h;
				break;
			}
		}

		if((id == 0)||(id > 3))				       //当前设备号不存在,先只支持三台设备
		{
			BABY_DBG("no this client dev....");
			continue;
		}	
		
		id = id-1;                                 //设备号是从1开始计算,这里让设备号改成从0开始算 
		if(ret >0){
			if((os_jiffies() - timer_ref) > 2000){
				rx_speed = rx_speed_cnt;
				rx_speed_cnt = 0;
				timer_ref = os_jiffies();
			}
			rx_speed_cnt += ret;
		}

		if(framenum[id] != frame_hand->framenum){           
			if(framenum[id] != 0xffff){
				if(cntnum[id] != pktnum[id]){
					BABY_DBG("lost frame\r\n");
				}
			}
		    BABY_DBG("frame:%d type:%d id:%d\r\n",frame_hand->framenum,frame_hand->frmtype,id);
			framenum[id] = frame_hand->framenum;
			type[id]     = frame_hand->frmtype;
			cntnum[id]   = frame_hand->cnt;
			pktnum[id] = 0;
			framelen[id] = 0;
			server_frame[id].framenum = framenum[id];
			memset(server_frame[id].lost_packet,0xff,100);
			for(itk = 0;itk < cntnum[id];itk++){
				server_frame[id].lost_packet[itk] = itk;
			}
			pktnum[id]++;                           //首次进来此帧图像
			server_frame[id].lost_num = mark_lost_pkt(frame_hand->pack,server_frame[id].lost_packet);						
		}else{
			for(itk = 0;itk < cntnum[id];itk++){								//后面进来图像均经过这里
				if(frame_hand->pack == server_frame[id].lost_packet[itk]){              //数据没接收过
					goto markdata;
				}
			}
			continue;	
markdata:			
			pktnum[id]++;
			server_frame[id].lost_num = mark_lost_pkt(frame_hand->pack,server_frame[id].lost_packet);		
		}
//		printf("@[%d:%d:%d:%d]@",frame_hand->pack,frame_hand->cnt,frame_hand->framenum,ret);

		len  = ret - sizeof(data_head);
		framelen[id] += len;
		hw_memcpy(psarm_room+frame_hand->pack*MAX_VIDEO_PKT_LEN,photo_buf+sizeof(data_head),len);
		
		
		if(pktnum[id] == cntnum[id]){        				//数据接收完成
			psarm_room[framelen[id]]   = 0x00;
			psarm_room[framelen[id]+1] = 0x00;
			psarm_room[framelen[id]+2] = 0x00;
			psarm_room[framelen[id]+3] = 0x01;		


				
			if((oldframecnt[id]+1) != framenum[id]){				//如果序号不连续
				if((framenum[id] == 0) && (oldframecnt[id] == 255)){	//序号循环了而已
					if(type[id] == 1){								//I帧，当前gop可推屏
						push_lcd_success[id] = 1;
					}
				}else{												//真实意义上的丢失了
					if(type[id] != 1){						        //不是I帧				
						os_printf("error ...................frame code\r\n");	//那当前ID不能再推了
						push_lcd_success[id] = 0;
					}else{											//丢帧了,但当前还是I帧，还是可以推的
						push_lcd_success[id] = 1;							
					}
				}
			}else{
				if(type[id] == 1){								//I帧，当前gop可推屏
					push_lcd_success[id] = 1;
				}				
			}
			if(push_lcd_success[id] == 1){						//可推送gop
	//			sys_dcache_clean_range(psram_h264_photo,framelen+4);
	//			h264_dec_src_264(psram_h264_photo,framelen,640,368);
				for(itk = 0;itk < DEC_TABLE_NUM;itk++){
					if(decmsg[itk].addr == NULL){
						fbuf = av_psram_malloc(framelen[id]+4);
						sys_dcache_invalid_range((uint32_t*)fbuf,framelen[id]+4);
						hw_memcpy(fbuf,psarm_room,framelen[id]+4);
						ie = disable_irq();
						decmsg[itk].addr = fbuf;
						decmsg[itk].len  = framelen[id];
						decmsg[itk].timeinf = os_jiffies(); 
						decmsg[itk].num  = framenum[id];
						decmsg[itk].type = type[id];
						decmsg[itk].devid= id;               //当前完成推送的摄像头ID
						decmsg[itk].framerate = framerate;
						decmsg[itk].w = w;
						decmsg[itk].h = h;	
						frame_for_dec++;
						BABY_DBG("w:%d h:%d  id:%d len:%d\r\n",w,h,id,framelen[id]);
						enable_irq(ie);
						
						break;
					}
				}					
			}
			if(itk == DEC_TABLE_NUM) {
				os_printf("No room for dec!\r\n");
				push_lcd_success[id] = 0;
			}
			if((os_jiffies() - timer_ref2) > 2000){
				dispnum = dispnum_cnt;
				dispnum_cnt = 0;
				timer_ref2 = os_jiffies();
			}			
			dispnum_cnt++;
			oldframecnt[id] = framenum[id];
			net_h264_sema_up(remote_addr.sin_addr.s_addr);
		}
	}
}

extern volatile uint8_t scaler2_dev_id;
void udp_handle_server_decode_to_lcd_thread(){
	int itk = 0;
	uint16 w,h;
	uint16 oldw[STA_NUM];
	uint16 oldh[STA_NUM];
	uint8_t *h264dat;
	uint8_t disp_num;
	uint8_t decframe[3];
	uint32_t timeinf[3];
//	uint32_t dsptime;
//	uint32_t newdspt;
	struct scale_device *scale_dev;
	uint32 ie;
//	uint16 oldframecnt = 0xffff;
//	uint8 frame_gop_lost = 1;
	uint8_t flush_video;
	uint8_t change_pixel = 0;
	uint16_t framerate[3];
	uint16_t framecnt[3];
	uint32_t lasttime[3];
	uint32_t wanttime[3];
	lasttime[0]=lasttime[1]=lasttime[2]=0;
	wanttime[0]=wanttime[1]=wanttime[2]=0;
	scale_dev = (struct scale_device *)dev_get(HG_SCALE2_DEVID);
	while(1){
		
		timeinf[0]=timeinf[1]=timeinf[2]= 0xffffffff;
		decframe[0]=decframe[1]=decframe[2]= 255;
		os_sleep_ms(2);
		
		ie = disable_irq();
		framecnt[0]=framecnt[1]=framecnt[2]=0;
		disp_num = 0;
		for(itk = 0;itk < DEC_TABLE_NUM;itk++){
			if(decmsg[itk].addr != NULL){
				if(timeinf[decmsg[itk].devid] > decmsg[itk].timeinf){
					timeinf[decmsg[itk].devid] = decmsg[itk].timeinf;
					decframe[decmsg[itk].devid] = itk;
				}
				framerate[decmsg[itk].devid] = devtab[decmsg[itk].devid].frame_rate;
				framecnt[decmsg[itk].devid]++;
				disp_num++;
			}			
		}
		enable_irq(ie);


		if(disp_num != 0){
			for(itk = 0;itk < 3;itk++){
				if(framecnt[itk] != 0){						         //当前id存在帧
					flush_video = 0;
					if((os_jiffies() - lasttime[itk]) > 100){ 		 //本次刷图像跟上次刷的时间差了100ms，那就直接推送
						flush_video = 1;
					}else{
						if(os_jiffies() > wanttime[itk]){			 //时间到了推送时间，则发送
							flush_video = 1;
						}else{										 //还没到推送时间，则要检查一下帧缓存有多少，如果多的话，则要适时推送
							if(framecnt[itk] > 12){					 //存够12帧的话，不要想了，发送吧
								flush_video = 1;
							}else if(framecnt[itk] > 6){			 //按照帧率时间减少15ms速度进行播放，以防数据拥堵引起延时
								if( os_jiffies() > (wanttime[itk] - 15)){
									flush_video = 1;
								}
							}else{									 
								if(os_jiffies() >= (wanttime[itk] - 10)){    //按照帧率时间减少10ms速度进行播放，如果严格按照帧率比例来整，肯定会造成数据拥堵到需提前播的位置
									flush_video = 1;
								}
							}						
						}
					}

					if(flush_video){
						
						lasttime[itk] = os_jiffies();
						wanttime[itk] = os_jiffies()+1000/decmsg[decframe[itk]].framerate;
						scaler2_dev_id =decmsg[decframe[itk]].devid;
						sys_dcache_clean_range((uint32_t*)(decmsg[decframe[itk]].addr),decmsg[decframe[itk]].len+4);
						if(decmsg[decframe[itk]].type == 1){
							change_pixel = 0;
						}
						
						if(change_pixel == 0)		
						{	
							h264dat = decmsg[decframe[itk]].addr;

							if(h264dat[4] == 0x67){    //SPS
								get_h264_stream_w_h(&w,&h,h264dat);
								if(h == 368){
									h = 360;
								}
							
								scale2_recfg_input_size(w,h,decmsg[decframe[itk]].devid);
								oldw[decmsg[decframe[itk]].devid] = w;
								oldh[decmsg[decframe[itk]].devid] = h;
							}

							if((oldw[decmsg[decframe[itk]].devid] != server_resolution[decmsg[decframe[itk]].devid].target_width) || (oldh[decmsg[decframe[itk]].devid] != server_resolution[decmsg[decframe[itk]].devid].target_high)){
								BABY_DBG("drop:%d %d\r\n",decmsg[decframe[itk]].devid,decmsg[decframe[itk]].num);
							}else{
								if(next_switch_device == 2) {
										if(cur_switch_device != next_switch_device) {
											msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 1);
											msi_cmd(R_VIDEO_P1, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 1);
											cur_switch_device = next_switch_device;
										}
									if(decmsg[decframe[itk]].devid == 0) {
										scale2_output_larger_local_change(decmsg[decframe[itk]].devid, devtab[decmsg[decframe[itk]].devid].larger);
										scale2_output_size_local_change(decmsg[decframe[itk]].devid,0,0,0,320,360);		
									}	
									else {
										scale2_output_larger_local_change(decmsg[decframe[itk]].devid, devtab[decmsg[decframe[itk]].devid].larger);
										scale2_output_size_local_change(decmsg[decframe[itk]].devid,0,320,0,320,360);	     
									}
									scale2_cfg_run(H264_DEC,decmsg[decframe[itk]].devid);
									h264_dec_src_264(decmsg[decframe[itk]].addr,decmsg[decframe[itk]].len,oldw[decmsg[decframe[itk]].devid],16*((oldh[decmsg[decframe[itk]].devid]+15)/16) ,decmsg[decframe[itk]].devid);
								}
								else if(next_switch_device == 0) {
									if(decmsg[decframe[itk]].devid == 0) {
										if(cur_switch_device != next_switch_device) {
											msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 1);
											msi_cmd(R_VIDEO_P1, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);
											cur_switch_device = next_switch_device;
										}
										scale2_output_larger_local_change(decmsg[decframe[itk]].devid, devtab[decmsg[decframe[itk]].devid].larger);
										scale2_output_size_local_change(decmsg[decframe[itk]].devid,1,0,0,640,360);	
										scale2_cfg_run(H264_DEC,decmsg[decframe[itk]].devid);
										h264_dec_src_264(decmsg[decframe[itk]].addr,decmsg[decframe[itk]].len,oldw[decmsg[decframe[itk]].devid],16*((oldh[decmsg[decframe[itk]].devid]+15)/16) ,decmsg[decframe[itk]].devid);
									}										
								}
								else if(next_switch_device == 1){
									if(decmsg[decframe[itk]].devid == 1) {
										if(cur_switch_device != next_switch_device) {
											msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);
											msi_cmd(R_VIDEO_P1, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 1);
											cur_switch_device = next_switch_device;
										}
										scale2_output_larger_local_change(decmsg[decframe[itk]].devid,devtab[decmsg[decframe[itk]].devid].larger);
										scale2_output_size_local_change(decmsg[decframe[itk]].devid,1,0,0,640,360);
										scale2_cfg_run(H264_DEC,decmsg[decframe[itk]].devid);
										h264_dec_src_264(decmsg[decframe[itk]].addr,decmsg[decframe[itk]].len,oldw[decmsg[decframe[itk]].devid],16*((oldh[decmsg[decframe[itk]].devid]+15)/16) ,decmsg[decframe[itk]].devid);	
									}											
								}
							}

						}
						else{
							
							change_pixel = 1;
						}
						av_psram_free(decmsg[decframe[itk]].addr);	
						ie = disable_irq();
						frame_for_dec--;
						decmsg[decframe[itk]].addr = NULL;
						enable_irq(ie);						
					}
				}
			}
		}
	}
	
}


void udp_photo_handle_server_thread(uint32_t ipaddr,uint8_t tab){
	k_task_handle_t handle_task_recv;
	k_task_handle_t handle_data_task_recv;
	uint16_t port = 6003;	
	handle_protocol_table_fd[tab] = usr_protocol_create_server_with_ip(port,ipaddr);
	port = 6002;
	handle_data_protocol_table_fd[tab] = usr_protocol_create_server_with_ip(port,ipaddr);
	BABY_DBG("build_fd:%d  %d\r\n",handle_protocol_table_fd[tab],handle_data_protocol_table_fd[tab]);
	devtab[tab].udp_status_fd = handle_protocol_table_fd[tab];
	devtab[tab].udp_data_fd   = handle_data_protocol_table_fd[tab];
	
	csi_kernel_task_new((k_task_entry_t)udp_handle_server_status_thread, "handle_udp_pkt", &devtab[tab], 25, 0, NULL, 1024, &handle_task_recv);
	csi_kernel_task_new((k_task_entry_t)udp_handle_server_data_thread, "handle_data_udp_pkt", &devtab[tab], 25, 0, NULL, 1024, &handle_data_task_recv);
	devtab[tab].udp_status_task = (uint32_t)handle_task_recv;
	devtab[tab].udp_data_task = (uint32_t)handle_data_task_recv;

	BABY_DBG("devtab: status->%d  data->%d  status_Task:%x  data_Task:%x\r\n",devtab[tab].udp_status_fd,devtab[tab].udp_data_fd,devtab[tab].udp_status_task,devtab[tab].udp_data_task);
}

void udp_handle_server_init()
{
	k_task_handle_t handle_lcd_task_recv;
	static k_task_handle_t handle_task_recv;
	static k_task_handle_t handle_data_task_recv;
	uint16_t port = 6003;
	handle_protocol_fd = usr_protocol_create_server(port); 
	port = 6002;
	handle_data_protocol_fd = usr_protocol_create_server(port);	
	csi_kernel_task_new((k_task_entry_t)udp_handle_server_status_thread, "handle_udp_pkt", &handle_protocol_fd, 25, 0, NULL, 1024, &handle_task_recv);
	csi_kernel_task_new((k_task_entry_t)udp_handle_server_data_thread, "handle_data_udp_pkt", &handle_data_protocol_fd, 25, 0, NULL, 1024, &handle_data_task_recv);
	BABY_DBG("status:%d  data:%d\r\n",handle_protocol_fd,handle_data_protocol_fd);
	
	csi_kernel_task_new((k_task_entry_t)udp_handle_server_decode_to_lcd_thread, "handle_data_decode_pkt", NULL, 25, 0, NULL, 1024, &handle_lcd_task_recv);	
}

static void tcp_handle_server( void *ei, void *d ){
	int ret;
//	struct msi *scaler_msi;
	int maskid = 0;
	uint8_t itk = 0;
	uint8_t jtk = 0;
	uint32_t ip = 0;
	uint8_t dev = 0;
	uint8_t tab_idx;
//	static uint8_t test_recfg = 0;
	connect_cfg_head tcp_hand;
	uint8_t tcpread[64];
	int tcp_fd = (int)d;
	ret = read(tcp_fd,tcpread,64);
	if(ret > 0){
		memcpy(&tcp_hand,tcpread,sizeof(tcp_hand));
		BABY_DBG("get len:%d  type:%02x  w:%d  h:%d............\r\n",ret,tcp_hand.type,tcp_hand.w,tcp_hand.h);
		if(tcp_hand.type == 2){			
			tcp_hand.type = 2;
			tcp_hand.w	  = tcp_hand.w;
			tcp_hand.h	  = tcp_hand.h;
			tcp_hand.frame_rate = 25;
			tcp_hand.ip_grp 	= 25;
			tcp_hand.packet_len = MAX_VIDEO_PKT_LEN;
			ret = send(tcp_fd,&tcp_hand,sizeof(tcp_hand),0);
			BABY_DBG("set tcp cfg:%d\r\n",ret);
		}else if(tcp_hand.type == 0){
			BABY_DBG("tcp get cfg:%d\r\n",tcp_hand.dev_magic);
			if(tcp_hand.dev_magic == 0){    //新设备刚开机
				tcp_hand.type = 2;
				tcp_hand.w	  = 640;//640;
				tcp_hand.h	  = 360;//360;	
				tcp_hand.frame_rate = 20;
				tcp_hand.ip_grp 	= 25;	
				tcp_hand.packet_len = MAX_VIDEO_PKT_LEN;
				tcp_hand.dev_magic = 0;
				for(itk = 0;itk < 10;itk++){
					if(tcp_fd == devtab[itk].tcpfd){
						if(devtab[itk].dev_id != 0){
							tcp_hand.dev_magic = devtab[itk].dev_id;
						}

						ip = devtab[itk].ipaddr;
					}
				}

				if(tcp_hand.dev_magic == 0){
					tcp_hand.dev_magic = ((ip>>24)&(STA_NUM-1)) + 1;         //控制设备编号
				}


				ret = send(tcp_fd,&tcp_hand,sizeof(tcp_hand),0);
				BABY_DBG("start client run:%d\r\n",ret);	
				
			}else{                          //这个设备已经开机过了
				for(itk = 0;itk < 10;itk++){
					if(tcp_fd == devtab[itk].tcpfd){
						devtab[itk].dev_id = tcp_hand.dev_magic;
						devtab[itk].frame_rate = tcp_hand.frame_rate;
						devtab[itk].w      = tcp_hand.w;
						devtab[itk].h      = tcp_hand.h;
					}
				}				
			}
		}	else if(tcp_hand.type == 1){            //client心跳包
			BABY_DBG("heartbeat...\r\n");
			for(itk = 0;itk < 10;itk++){
				if(tcp_hand.dev_magic == devtab[itk].dev_id){			        //查看client的心跳包是否有记录在table中
					                                                            //magic id存在，但与设备号的不匹配
					for(jtk = 0;jtk < 10;jtk++){
						if(tcp_fd == devtab[jtk].tcpfd){
							ip = devtab[jtk].ipaddr; 
						}
					}
							
 
					os_printf("check ip addr(%d %08x  %d)",tcp_fd,ip,tcp_hand.dev_magic);
					if((((ip>>24)&(STA_NUM-1)) + 1) != tcp_hand.dev_magic){		//根据ip地址分配的设备号匹配不上，那得给对面设备重新配置
						tcp_hand.type = 5;
						tcp_hand.dev_magic = ((ip>>24)&(STA_NUM-1)) + 1;
						ret = send(tcp_fd,&tcp_hand,sizeof(tcp_hand),0);
						BABY_DBG("reset devid:%d ip:%x set id:%d\r\n",ret,ip,tcp_hand.dev_magic);	
						maskid = tcp_hand.dev_magic;	
						dev = maskid;
					}else{
						maskid = tcp_hand.dev_magic;
						devtab[itk].w	   = tcp_hand.w;							//更新心跳过来的设备长度
						devtab[itk].h	   = tcp_hand.h;
						dev = maskid;
					}
					
				}
			}			

			if(maskid == 0){											        //client心跳有，但table中却没有此信息
				for(itk = 0;itk < 10;itk++){
					if(tcp_fd == devtab[itk].tcpfd){
						devtab[itk].dev_id = tcp_hand.dev_magic;		        //把当前设备号记录下来
						devtab[itk].frame_rate = tcp_hand.frame_rate;   
						devtab[itk].w      = tcp_hand.w;					 
						devtab[itk].h      = tcp_hand.h;					
						devtab[itk].psram_photo = av_psram_malloc(200*1024);    //给初始化空间 200K
						dev = devtab[itk].dev_id;
					}
				}
			}

			//dev是对面给过来的,这里是心跳包,对方肯定已经开机并且有了设备号
			dev--;     //设备是从1开始,所以这里得--			
			if((server_resolution[dev].target_width != devtab[itk].w) || (server_resolution[dev].target_high != devtab[itk].h)) {
				tcp_hand.type = 2;
				tcp_hand.w	  = server_resolution[dev].target_width;
				tcp_hand.h	  = server_resolution[dev].target_high;	
				tcp_hand.frame_rate = 20;
				tcp_hand.ip_grp 	= 25;	
				tcp_hand.packet_len = MAX_VIDEO_PKT_LEN;
				ret = send(tcp_fd,&tcp_hand,sizeof(tcp_hand),0);				
			}
//分辨率切换操作			
#if 0
			//if(test_recfg == 0)
			{
				test_recfg++;
				//if(os_jiffies() > 10000)
				if(test_recfg % 30 == 19)
				{
					os_printf("recfg size(%d)...\r\n",dev);
					tcp_hand.type = 2;
					if(tcp_hand.w == 1280){
						tcp_hand.w	  = 640;
						tcp_hand.h	  = 360;
						server_resolution[dev].target_width = 640;
						server_resolution[dev].target_high	= 360;	
					}else{
						tcp_hand.w	  = 1280;
						tcp_hand.h	  = 720;
						server_resolution[dev].target_width = 1280;
						server_resolution[dev].target_high	= 720;
					}

					tcp_hand.frame_rate = 20;
					tcp_hand.ip_grp 	= 25;	
					tcp_hand.packet_len = MAX_VIDEO_PKT_LEN;
					ret = send(tcp_fd,&tcp_hand,sizeof(tcp_hand),0);
					//test_recfg = 1;					
				}else if(test_recfg % 60 == 26){
					_os_printf("recfg size2(%d)...\r\n",dev);
					tcp_hand.type = 2;
					if(tcp_hand.w == 1280){
						tcp_hand.w	  = 640;
						tcp_hand.h	  = 360;
						server_resolution[dev].target_width = 640;
						server_resolution[dev].target_high	= 360;	
					}else{
						tcp_hand.w	  = 1280;
						tcp_hand.h	  = 720;
						server_resolution[dev].target_width = 1280;
						server_resolution[dev].target_high	= 720;
					}
					tcp_hand.frame_rate = 20;
					tcp_hand.ip_grp 	= 25;	
					tcp_hand.packet_len = MAX_VIDEO_PKT_LEN;
					ret = send(tcp_fd,&tcp_hand,sizeof(tcp_hand),0);
					
				}
				
			}
#endif
			
		}		
	}else{
		BABY_DBG("tcp read close fd....\r\n");
		eloop_remove_event( ei );
		closesocket( tcp_fd );
		for(itk = 0;itk < 10;itk++){
			if(tcp_fd == devtab[itk].tcpfd){			//释放的FD是存在于table表的，确定没有进行重新连接
				devtab[itk].tcpfd = 0;
				devtab[itk].ipaddr= 0;
				devtab[itk].dev_id= 0;
				devtab[itk].frame_rate = 0;
				av_psram_free(devtab[itk].psram_photo);
				devtab[itk].psram_photo = NULL;
				tab_idx = itk;
			}
		}	
		devnum--; 
		tcp_fd = -1;
		
	}
	
}


void user_tcpServerAccept(void *e, void *d)    //屏端
{
	uint8_t itk=0;
	uint8_t dnum = 0;
	uint8_t mask = 0;
//	connect_cfg_head tcp_hand;
//	k_task_handle_t task_hdl;
	struct  sockaddr_in client;  
	socklen_t  addrlen;
//	int32 ret;
	addrlen =sizeof(client);  
	int tcp_fd = accept(tcp_connect_fd, (struct sockaddr*)&client, &addrlen);
	if(tcp_fd < 0)
	{
		BABY_DBG("accept()error...............\n");  
		return;
	}

	for(itk = 0;itk < 10;itk++){
		if(devtab[itk].ipaddr == client.sin_addr.s_addr){				//发现当前列表里面已经有此IP地址
			devtab[itk].tcpfd  = tcp_fd;							    //更新此列表的fd
			mask = 1;                                                   //标识,表明当前IP已有设备id分配		
		}
	}
	if(mask == 0){													    //当前IP没被分配
		for(itk = 0;itk < 10;itk++){
			if(devtab[itk].tcpfd == 0){
				dnum = itk; 
				break;
			}
		}
	
		devtab[dnum].ipaddr = client.sin_addr.s_addr;
		devtab[dnum].tcpfd  = tcp_fd;
		devnum++;														//标明下个设备号
	}
	
	eloop_add_fd( tcp_fd, EVENT_READ, EVENT_F_ENABLED, tcp_handle_server, (void*)tcp_fd );
}


void tcp_handle_server_init(){
	int InitSrv(uint16_t port);
	tcp_connect_fd = InitSrv(6001);
	eloop_add_fd( tcp_connect_fd, EVENT_READ, EVENT_F_ENABLED, user_tcpServerAccept, 0 );
}

void protocol_server_init(){
	uint8_t i = 0;
	struct h264_device *h264_dev;
	struct msi *scale2 = scale2_msi("scale2", 640, 360, 640, 360, FSTYPE_YUV_P0, 10);
    if (scale2)
    {
		msi_add_output(scale2, NULL, R_VIDEO_P0);
		msi_add_output(scale2, NULL, R_VIDEO_P1);
		os_printf("%s  %d\r\n",__func__,__LINE__);
    }
	
	for(i = 0;i < STA_NUM;i++){
		server_resolution[i].target_width = 640;
		server_resolution[i].target_high = 360;
	}

	//scaler_msi_gol = scale2;
	h264_dev = (struct h264_device *)dev_get(HG_H264_DEVID);
	memset(decmsg,0x00,sizeof(decmsg));
	for(i=0; i<10; i++) {
		devtab[i].larger = 10;
	}
	h264_drv_init(h264_dev);
	h264_dec_room_init(2,1280,720);
	net_h264_sema_init();	
	udp_handle_server_init();
	tcp_handle_server_init();
}

void protocol_server_change_resolution(uint8_t id, uint32_t width, uint32_t high)
{
	server_resolution[id].target_width = width;
	server_resolution[id].target_high = high;		
}

void user_protocol()
{
	os_sleep_ms(100);
    protocol_server_init();	       //进行推屏					AP
}

static void babyprotocol_switch_device(int8_t device)
{
	next_switch_device = device;
}

int32_t atcmd_babyprotocol_switch_device(const char *cmd, char *argv[], uint32 argc)
{
	int8_t device = 0;
	if(argv[0]) {
		device = os_atoi(argv[0]);
		babyprotocol_switch_device(device);
		return RET_OK;
	}
	return RET_ERR;
}

static void babyprotocol_change_larger(uint8_t device, uint8_t larger)
{
	devtab[device].larger = larger;
}

int32_t atcmd_babyprotocol_change_larger(const char *cmd, char *argv[], uint32 argc)
{
	if(argc >= 2) {
		uint8_t device = os_atoi(argv[0]);
		uint8_t larger = os_atoi(argv[1]);
		babyprotocol_change_larger(device, larger);
		return RET_OK;
	}
	return RET_ERR;
}

#endif