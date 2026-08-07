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

#ifdef SYS_APP_WALKIE_TALKIE

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE av_free
#define STREAM_LIBC_ZALLOC av_zalloc

#define MAX_VIDEO_PKT_LEN 1430
//#define SENSOR_DEV_NUM  1

#if OPEN_DBG
#define CHILDREN_DBG(fmt, ...)   _os_printf(fmt, ##__VA_ARGS__)
#else
#define CHILDREN_DBG(fmt, ...)   //_os_printf(fmt, ##__VA_ARGS__)
#endif

struct os_msgqueue net_h264_msg;

static int handle_protocol_fd   =  - 1;
static int handle_data_protocol_fd   =  - 1;

int handle_protocol_table_fd[10];
int handle_data_protocol_table_fd[10];


volatile uint32 server_frame_rate;

#define DEC_TABLE_NUM  32
decode_msg decmsg[DEC_TABLE_NUM];


uint32_t devnum = 0;
dev_map devtab[10];
frame_msg server_frame[10];

static uint8_t photo_buf[1440] __attribute__ ((aligned(4)));;
uint8 psram_h264_photo[200*1024] __attribute__ ((aligned(4),section(".psram.src")));
int frame_for_dec = 0;
uint8_t dispnum;
uint16_t rx_speed,tx_speed;
extern struct msi *scale2_msi(const char *name, uint16_t iw, uint16_t ih, uint16_t ow, uint16_t oh, uint16_t type,uint8_t larger);
void net_s_h264_sema_init()
{
	os_msgq_init(&net_h264_msg,1);
}

uint32 net_s_h264_sema_down(int32 tmo_ms)
{
	uint32 retval;
	uint32 retdata;
	//os_sema_down(&net_h264_sem,tmo_ms);	
	retdata = os_msgq_get2(&net_h264_msg, tmo_ms, (int32*)&retval);
	if(retval == 0){
		return retdata;
	}else{
		return retval;
	}
}

void net_s_h264_sema_up(uint32 clientaddr)
{
	os_msgq_put(&net_h264_msg, clientaddr, osWaitForever);
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


void udp_handle_server_status_write_workqueue(void *ei, void *d){
	struct sockaddr_in *addrServer;
	status_msg *msg_head;
	char buf[12];
	int  len;
	uint8_t id;
	uint8_t fd;
	addrServer = d;
	//for(itk = 0;itk < 10;itk++){
	//	if(addrServer->sin_addr.s_addr == devtab[itk].ipaddr){
			id = devtab[0].dev_id;
			fd = devtab[0].udp_status_fd;
	//	}
	//}
	msg_head = (status_msg *)buf;
	msg_head->framenum = server_frame[id-1].framenum;
	msg_head->type     = 0;
	//len = sendto(fd, (char*)buf, 2, MSG_DONTWAIT, (struct sockaddr *)d, sizeof(struct sockaddr));
	len = sendto(handle_protocol_fd, (char*)buf, 2, MSG_DONTWAIT, (struct sockaddr *)d, sizeof(struct sockaddr));
}

static void udp_handle_server_status_read_workqueue(void *ei, uint32_t *status_fd){	
	int retval;
	int ret;
	int tos;
	uint8_t itk;
	uint8_t handlebuf[64];
	struct sockaddr remote_addr;
	struct sockaddr_in *addrServer;
	status_msg *msg_head;
	uint8_t id;
	msg_head = (status_msg *)handlebuf;
	retval = 16;
	ret = recvfrom (*status_fd, handlebuf, 24, 0, &remote_addr, (socklen_t*)&retval);

	addrServer = (struct sockaddr_in *)&remote_addr;

//	for(itk = 0;itk < 10;itk++){
//		if(addrServer->sin_addr.s_addr == devtab[itk].ipaddr){
//			id =  devtab[itk].dev_id;
//		}
//	}
	id = 1;
	if((id == 0)&&(id > 3)) 				   //当前设备号不存在,先只支持三台设备
	{
		CHILDREN_DBG(" status no this client dev....");
		return;
	}

	id = id-1;								  //设备号是从1开始计算,这里让设备号改成从0开始算
	//os_printf("client read:%02d   type:%d   lost_num:%d \r\n",msg_head->framenum,msg_head->type,server_frame[id-1].lost_num);
	if(server_frame[id].lost_num != 0){
		CHILDREN_DBG("L(%d %d)",id,server_frame[id].lost_num);
	}else{
		CHILDREN_DBG("R%d",id);
	}
	
	if(server_frame[id].framenum != msg_head->framenum){
		CHILDREN_DBG("ag%d",id);
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
	CHILDREN_DBG("S");
	tos = IPTOS_PREC_NETCONTROL; // 最高优先级
	setsockopt(*status_fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));	
	sendto(*status_fd, handlebuf, 2+server_frame[id].lost_num, MSG_DONTWAIT, &remote_addr, sizeof(struct sockaddr));
	tos = IPTOS_PREC_ROUTINE; // 最低优先级
	setsockopt(*status_fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));	
	
	CHILDREN_DBG("RE");
}


void udp_handle_server_status_thread(void *d){	
	uint16_t *port;
	struct sockaddr_in addrServer;
	in_addr_t cli_addr;
	memset(&addrServer,0,sizeof(struct sockaddr_in));

	port = d;
	CHILDREN_DBG("data   server:%d\r\n",*port);
	addrServer.sin_family=AF_INET;
	addrServer.sin_addr.s_addr= inet_addr("255.255.255.255");//client_addr;//inet_addr("192.168.169.1");
	addrServer.sin_port=htons(*port);
	eloop_add_fd( handle_protocol_fd, EVENT_READ, EVENT_F_ENABLED, (void*)udp_handle_server_status_read_workqueue, &handle_protocol_fd );
	
	while(1){
		cli_addr = net_s_h264_sema_down(-1);
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

void scale3_output_size_local_change(uint8_t id,uint8_t show_only,uint16 x,uint16 y,uint16 w,uint16 h);
void scale2_output_size_local_change(uint8_t id,uint8_t show_only,uint16 x,uint16 y,uint16 w,uint16 h);

uint8_t switch_mode = 0;
void udp_handle_server_data_thread(uint32_t *d){
	uint32 timer_ref = 0;
	uint32 timer_ref2 = 0;
	int retval;
	int ret;
	int len;
	int itk = 0;

	uint16_t w,h;
	struct sockaddr_in remote_addr;
	uint16_t rx_speed_cnt = 0;
	uint16_t dispnum_cnt = 0;
	uint8_t  push_lcd_success[10];
	uint16_t framenum[10];
	uint32_t framelen[10];
	uint16_t oldframecnt[10];
	uint8_t id;
	uint32_t framerate;
	uint8_t type[10];
	uint8_t cntnum[10];
	uint8_t pktnum[10];
	uint8_t *fbuf;
	uint32 ie;
	uint8_t *psarm_room;
	data_head *frame_hand;
	retval = 16;
	uint32 frame_dec_num = 0;
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
		ret = recvfrom (handle_data_protocol_fd, photo_buf, MAX_VIDEO_PKT_LEN+sizeof(data_head), 0, (struct sockaddr*)&remote_addr, (socklen_t*)&retval);
		id = 0;
//		for(itk = 0;itk < 10;itk++){
//			if(remote_addr.sin_addr.s_addr == devtab[itk].ipaddr){
				id =  devtab[0].dev_id;
				psarm_room = devtab[0].psram_photo;
				framerate  = devtab[0].frame_rate;
				w = devtab[0].w;
				h = devtab[0].h;
//				break;
//			}
//		}

		if((id == 0)||(id > 3))				       //当前设备号不存在,先只支持三台设备
		{
			CHILDREN_DBG("no this client dev(%d)....",id);
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

		walkmsg.rx_data += ret;
		
		if(framenum[id] != frame_hand->framenum){           
			if(framenum[id] != 0xffff){
				if(cntnum[id] != pktnum[id]){
					CHILDREN_DBG("lost frame\r\n");
				}
			}
		    CHILDREN_DBG("frame:%d type:%d id:%d\r\n",frame_hand->framenum,frame_hand->frmtype,id);
			
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
					walkmsg.frame_rx_lost++;
					if(type[id] != 1){						        //不是I帧				
						CHILDREN_DBG("error ...................frame code\r\n");	//那当前ID不能再推了
						push_lcd_success[id] = 0;
					}else{											//丢帧了,但当前还是I帧，还是可以推的
						push_lcd_success[id] = 1;							
					}
				}
			}else{
				if(type[id] == 1){								//I帧，当前gop可推屏
					push_lcd_success[id] = 1;
				}
				walkmsg.frame_rx_success++;
			}

			if(push_lcd_success[id] == 1){						//可推送gop
	//			sys_dcache_clean_range(psram_h264_photo,framelen+4);
	//			h264_dec_src_264(psram_h264_photo,framelen,640,368);
				for(itk = 0;itk < DEC_TABLE_NUM;itk++){
					if(decmsg[itk].addr == NULL){
						fbuf = av_psram_malloc(framelen[id]+4);
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
						CHILDREN_DBG("w:%d h:%d  id:%d len:%d\r\n",w,h,id,decmsg[itk].len);						
						enable_irq(ie);
						//frame_dec_num++;
						if(frame_dec_num == 0) {
							scale2_output_size_local_change(0,0,0,0,320,240);
							scale3_output_size_local_change(0,0,0,0,160,120);
							frame_dec_num++;
						}
						if((frame_dec_num%600) == 200){
							_os_printf("%s  %d\r\n",__func__,__LINE__);
							scale2_output_size_local_change(0,0,0,0,320,240);
							scale3_output_size_local_change(0,0,6,6,160,120);
						}else if((frame_dec_num%600) == 400){
							_os_printf("%s  %d\r\n",__func__,__LINE__);	
							scale2_output_size_local_change(0,0,6,6,160,120);
							scale3_output_size_local_change(0,0,0,0,320,240);							
							
						}else if((frame_dec_num%600) == 500){
							//scale2_output_size_local_change(0,0,0,10,160,120);
							//scale3_output_size_local_change(0,1,30,0,320,240);							
						}						
						break;
					}
				}				
			}

			if((os_jiffies() - timer_ref2) > 2000){
				dispnum = dispnum_cnt;
				dispnum_cnt = 0;
				timer_ref2 = os_jiffies();
			}			
			dispnum_cnt++;
			oldframecnt[id] = framenum[id];
			net_s_h264_sema_up(remote_addr.sin_addr.s_addr);
		}
	}
}

extern volatile uint8_t scaler2_dev_id;
void udp_handle_server_decode_to_lcd_thread(){
	int itk = 0;
	uint8_t disp_num;
	uint8_t decframe[3];
	uint32_t timeinf[3];
	uint32_t listaddr;
	uint8_t *h264dat;
	struct scale_device *scale_dev;
	uint32 ie;
	uint16 oldw,oldh;
	uint16 w,h;
	uint8_t flush_video;
	uint8_t change_pixel = 1;
	uint16_t framerate[3];
	uint16_t framecnt[3];
	uint32_t lasttime[3];
	uint32_t wanttime[3];
	lasttime[0]=lasttime[1]=lasttime[2]=0;
	wanttime[0]=wanttime[1]=wanttime[2]=0;
	scale_dev = (struct scale_device *)dev_get(HG_SCALE2_DEVID);

	static uint32_t last_update_time = 0;
	static uint32_t total_disp_num = 0;

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
						total_disp_num++;
						lasttime[itk] = os_jiffies();
						wanttime[itk] = os_jiffies()+1000/decmsg[decframe[itk]].framerate;
						scaler2_dev_id =decmsg[decframe[itk]].devid;
						sys_dcache_clean_range((uint32_t *)decmsg[decframe[itk]].addr,decmsg[decframe[itk]].len+4);
						if(decmsg[decframe[itk]].type == 1){
							change_pixel = 0;
						}

#if 0						
						if((decmsg[decframe[itk]].w == scale_get_input_width(scale_dev)||(1 == scale_get_input_width(scale_dev))) && (decmsg[decframe[itk]].h == scale_get_input_high(scale_dev)||(1 == scale_get_input_high(scale_dev))) && (change_pixel == 0)){//确认scale模块配置的源size跟目前要解码的size是一致
							//h264_dec_src_264(decmsg[decframe[itk]].addr,decmsg[decframe[itk]].len,decmsg[decframe[itk]].w,16*((decmsg[decframe[itk]].h + 15)/16) ,decmsg[decframe[itk]].devid);
							

							listaddr = put_h264msg_to_queue(1,decmsg[decframe[itk]].w,decmsg[decframe[itk]].h,decmsg[decframe[itk]].addr,decmsg[decframe[itk]].len);					
							while(h264msg_queue_done(listaddr) == 0){
								os_sleep_ms(2);
							}
						}		
						else{
							change_pixel = 1;
						}
#else
						if(change_pixel == 0)		
						{	
							h264dat = decmsg[decframe[itk]].addr;

							if(h264dat[4] == 0x67){    //SPS
								get_h264_stream_w_h(&w,&h,h264dat);
								if(h == 368){
									h = 360;
								}
								if(h == 128){
									h = 120;
								}
								
								scale2_recfg_input_size(w,h,0);
								oldw = w;
								oldh = h;
							}
							//_os_printf("w:%d,h:%d\r\n",w,h);
							listaddr = put_h264msg_to_queue(1,w,h,decmsg[decframe[itk]].addr,decmsg[decframe[itk]].len);					
							while(h264msg_queue_done(listaddr) == 0){
								os_sleep_ms(2);
							}
						}
						else{
							change_pixel = 1;
						}					
#endif						
						av_psram_free(decmsg[decframe[itk]].addr);	
						ie = disable_irq();
						frame_for_dec--;
						decmsg[decframe[itk]].addr = NULL;
						enable_irq(ie);						
					}
				}
			}
		}
		if((os_jiffies()-last_update_time) > 5000) {
			_os_printf("\n");
			os_printf("---disp num:%d---",(uint32_t)((float)total_disp_num/((float)(os_jiffies()-last_update_time)/1000.0f)));
			_os_printf("\n");
			last_update_time = os_jiffies();
			total_disp_num = 0;
		}
	}
}

static uint16_t port_data;
static uint16_t port_status;
void udp_handle_server_init(uint16_t status_port,uint16_t data_port)
{
	k_task_handle_t handle_lcd_task_recv;
	static k_task_handle_t handle_task_recv;
	static k_task_handle_t handle_data_task_recv;
	port_status = status_port;
	handle_protocol_fd = usr_protocol_create_server(port_status); 
	port_data = data_port;
	handle_data_protocol_fd = usr_protocol_create_server(port_data);	
	csi_kernel_task_new((k_task_entry_t)udp_handle_server_status_thread, "handle_udp_pkt", &port_status, 25, 0, NULL, 1024, &handle_task_recv);
	csi_kernel_task_new((k_task_entry_t)udp_handle_server_data_thread, "handle_data_udp_pkt", &port_data, 25, 0, NULL, 1024, &handle_data_task_recv);
	os_printf("status:%d  data:%d\r\n",handle_protocol_fd,handle_data_protocol_fd);
	csi_kernel_task_new((k_task_entry_t)udp_handle_server_decode_to_lcd_thread, "handle_data_decode_pkt", NULL, 25, 0, NULL, 1024, &handle_lcd_task_recv);	
}


void protocol_server_init(uint16_t status_port,uint16_t data_port){
#if 0
	struct h264_device *h264_dev;
	struct msi *scale2 = scale2_msi("scale2", 640, 360, 320, 240, FSTYPE_YUV_P0, 10);
    if (scale2)
    {
		msi_add_output(scale2, NULL, "sim_video");
		os_printf("%s  %d\r\n",__func__,__LINE__);
    }
	//scaler_msi_gol = scale2;
	h264_dev = (struct h264_device *)dev_get(HG_H264_DEVID);
	memset(decmsg,0x00,sizeof(decmsg));
	h264_drv_init(h264_dev);
	h264_dec_room_init(2,1280,720);
#endif	

	devtab[0].dev_id = 1;				//把当前设备号记录下来
	devtab[0].frame_rate = 25;	
	devtab[0].w	   = NET_W;					 
	devtab[0].h	   = NET_H;					
	devtab[0].psram_photo = av_psram_malloc(200*1024);	//给初始化空间 200K

	net_s_h264_sema_init();	
	udp_handle_server_init(status_port,data_port);
//	tcp_handle_server_init();
}

void user_protocol2(uint16_t status_port,uint16_t data_port)
{
    protocol_server_init(status_port,data_port);	       //进行推屏					AP
}

#endif