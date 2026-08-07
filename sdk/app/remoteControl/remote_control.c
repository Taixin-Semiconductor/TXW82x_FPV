#include "event.h"
#include "sys_config.h"
#include "remote_control.h"
#include "socket_module.h"
#include "osal/string.h"


#ifdef  OPENDML_EN 


#if 1





//uint8_t *thread_status;	//音视频录像线程状态, 0:已经停止  1:正在运行 3:等待线程停止,用bit来控制
struct record_status_s
{
	uint8_t type;	//0:停止录像(判断是否在录像) 1:开始录像
	uint8_t video_fps;	//录像的视频帧率
	uint8_t audio_frq;	//录像音频的采样率设置(要与硬件一致)(如果不需要录音,则设置为0),单位为K
	uint8_t reserve;	//保留

};

extern uint8_t get_record_thread_status();
extern uint8_t send_stop_record_cmd();
extern void start_record_thread(uint8_t video_fps,uint8_t audio_frq);

static void record_control_event(struct event *ei, void *d)
{
	int fd = ei->ev.fd.fd;
	int res;
	struct record_status_s *ctrl = (struct record_status_s *)d;
	
	res = recv(fd,ctrl,sizeof(struct record_status_s ),0);

	if(res <= 0)
	{
		_os_printf("%s res:%d\n",__FUNCTION__,res);
		eloop_remove_event(ei);
		if(fd > 0)
		{
			close(fd);
		}
		if(ctrl)
		{
			free(ctrl);
		}
		return;
	}
	send(fd,ctrl,sizeof(struct record_status_s),0);
	_os_printf("type:%d\t%d\n",ctrl->type,res);
	uint8_t thread_status = get_record_thread_status();
	switch(ctrl->type)
	{
		case 0:
			//停止录像,先判断一下录像的状态
			if(!thread_status)
			{
				//已经停止了,不需要任何操作
			}
			//正在录像,并且没有发送过停止录像的命令
			else if(thread_status)
			{
				//发送停止录像
				//set_record_thread_status(0x02);
				send_stop_record_cmd();
			}

		break;

		//开始录像
		case 1:
			//已经运行,则打印错误
			if(thread_status)
			{
				os_printf("record thread already run1\n");
			}
			else
			{
				//创建线程,只是视频
				start_record_thread(ctrl->video_fps,ctrl->audio_frq);
			}
		break;
		default:
			os_printf("record cmd err\n");
		break;
	}


	
}



static void client_accept(void *e, void *d)
{
	int server_fd = (int)d;
	struct sockaddr_in addr;
	socklen_t namelen = sizeof(addr);
	int client_fd;
	client_fd = accept(server_fd, (struct sockaddr*)&addr, &namelen);
	if(client_fd < 0)
	{
		_os_printf("%s accept() error\n",__FUNCTION__);  
	}
	struct record_status_s  *ctrl= (struct record_status_s *)malloc(sizeof(struct record_status_s));
	_os_printf("%s client_fd:%d\n",__FUNCTION__,client_fd);
	eloop_add_fd( client_fd , EVENT_READ, EVENT_F_ENABLED, (void*)record_control_event, (void*)ctrl);

}

//录像接收端口
void recv_record_Server(int port)
{

	_os_printf("%s : port: %d\n", __FUNCTION__,port);

	int fd = creat_tcp_socket(port);
	_os_printf("%s fd:%d\n",__FUNCTION__,fd);
	if(listen(fd,1)== -1)
	{  
		return ;  
	}  

	eloop_add_fd( fd, EVENT_READ, EVENT_F_ENABLED, client_accept,(void*)fd );
}

#endif

#endif
