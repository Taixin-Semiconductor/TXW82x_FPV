#include <stdio.h>
#include "sys_config.h"
#include "lwip\sockets.h"
#include "lwip\netif.h"
#include "lwip\dns.h"
//#include "lwip\fcntl.h"
#include "lwip\api.h"
#include "lwip\tcp.h"

#include <event.h>
#include <log.h>
#include <frame.h>
#include <rtp.h>
#include "spook_config.h"
//#include <linux/linux_mutex.h>
#include "lwip/api.h"
#include <csi_kernel.h>
#include "ota.h"
//#include "flash.h"
#include "socket_module.h"
#include "dev/csi/hgdvp.h"
#include "hal/spi_nor.h"
#include "lib/ota/fw.h"

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC os_malloc
#define STREAM_LIBC_FREE os_free
#define STREAM_LIBC_ZALLOC os_zalloc

#define RECV_MAX_SIZE	1460

enum TcpServerCmd
{
	REQ_NET_UPGRADE = 0xA0,
	ACK_NET_UPGRADE = 0xA1,
};

typedef struct tcpServerHeader
{
	unsigned int Command;
	unsigned int Reserved[8];
}TCP_SERVER_H;

typedef struct tcpNetUpgradeInfo
{
	char version[16];
	int fileSize;
}NetUpgrade_Info;


int InitSrv(uint16_t port)
{
	int srvsock;
	srvsock = creat_tcp_socket(port);   
	if(listen(srvsock,3)== -1)
	{  
		//perror("listen()error\n");  
		return -1;  
	}  
	return srvsock;
}



void tcpNetUpgrade(int tcp_connect_fd)
{
	int res = 0;
	uint8 *tcp_rcv_buff = NULL;
	int size = 0;
	int offset = 0;
	tcp_rcv_buff = (uint8*)STREAM_LIBC_MALLOC(RECV_MAX_SIZE + 4);
	memset(tcp_rcv_buff, 0, RECV_MAX_SIZE + 4);
	TCP_SERVER_H tcpHeader;
	memset(&tcpHeader, 0, sizeof(TCP_SERVER_H));
	tcpHeader.Command = ACK_NET_UPGRADE;

	
	//spi_nor_open(flash);
	if(send(tcp_connect_fd, (char*)&tcpHeader, sizeof(TCP_SERVER_H), 0) > 0)
	{
		NetUpgrade_Info netUpgradeInfo;
		memset(&netUpgradeInfo, 0, sizeof(NetUpgrade_Info));
		recv(tcp_connect_fd, (char*)&netUpgradeInfo, sizeof(NetUpgrade_Info), 0);
		while(offset < netUpgradeInfo.fileSize)
		{
			memset(tcp_rcv_buff, 0, RECV_MAX_SIZE + 4);
			size = recv(tcp_connect_fd, tcp_rcv_buff, RECV_MAX_SIZE, 0);
			if(size <= 0)
			{
				_os_printf("%s:%d err,offset:%d\n",__FUNCTION__,__LINE__,offset);
				break;
			}

			//res = libotaV2_write_fw(netUpgradeInfo.fileSize,1,0,0,offset, tcp_rcv_buff, size);
			res = libota_write_fw(netUpgradeInfo.fileSize,offset,tcp_rcv_buff,size);
			if(res)
			{
				send(tcp_connect_fd, "Upgrade_err", 12, 0);
				_os_printf("%s:%d libota_write_fw err,offset:%d,res:%d\n",__FUNCTION__,__LINE__,offset,res);
				break;
			}
			
			offset += size;
			//_os_printf("recv size: %d, offset: %d(%d%%)\n", size, offset, offset*100/netUpgradeInfo.fileSize);
		
		};
		if(offset == netUpgradeInfo.fileSize)
		{
			//将数据头回写,代表已经完成
			//spi_nor_write(flash,update_adr+0,(unsigned char*)&ota_flag,sizeof(ota_flag));
			size = send(tcp_connect_fd, "Upgrade_ok", 11, 0);
			_os_printf("NetUpgrade ok ###\n");
		}
		else
		{
			size = send(tcp_connect_fd, "Upgrade_err", 12, 0);
			_os_printf("NetUpgrade err offset = %d, fileSize = %d ###\n", offset, netUpgradeInfo.fileSize);
		}
	}
	STREAM_LIBC_FREE(tcp_rcv_buff);
	close(tcp_connect_fd);
	//spi_nor_close(flash);
}



extern void tcp_update(void *d);
void tcpServerAccept(void *e, void *d)
{
	int tcp_server_fd = (int)d;
	int tcp_connect_fd = -1;
	struct  sockaddr_in client;  
	socklen_t  addrlen;

	TCP_SERVER_H tcpHeader;
	memset(&tcpHeader, 0, sizeof(TCP_SERVER_H));

	addrlen =sizeof(client);  
	tcp_connect_fd = accept(tcp_server_fd, (struct sockaddr*)&client, &addrlen);
	if(tcp_connect_fd < 0)
	{
		_os_printf("accept()error\n");  
	}
	os_task_create("tcp_update", tcp_update, (void*)tcp_connect_fd, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2048);

}


void ota_Tcp_Server()
{
	uint16_t port = 5007;
	int tcp_server_fd;

	_os_printf("[test] init tcp server: port: %d\n", port);

	tcp_server_fd = InitSrv(port);
	
	eloop_add_fd( tcp_server_fd, EVENT_READ, EVENT_F_ENABLED, tcpServerAccept, (void*)tcp_server_fd );
}


void tcp_recv_data(void *d)
{

	int tcp_connect_fd = (int)d;
	uint8_t *tcp_rcv_buff;
	int size;
	tcp_rcv_buff = (uint8_t*)STREAM_LIBC_MALLOC(1024 + 4);
	memset(tcp_rcv_buff, 0, sizeof(1024+4));

	
#if DVP_EN	
		//dvp_close();
		void *dvp = (void *)dev_get(HG_DVP_DEVID);
		if(dvp)
		{
			dvp_close(dvp);
		}
#endif
	
#if USB_EN	
		SYSCTRL_REG_OPT(sysctrl_usb20_clk_close());
#endif
		while(1){
			size = recv(tcp_connect_fd, tcp_rcv_buff, 1024, 0);
			if(size > 0){
				//_os_printf("tcp_rcv_buff(%d):%x %x %x %x\r\n",size,tcp_rcv_buff[0],tcp_rcv_buff[1],tcp_rcv_buff[2],tcp_rcv_buff[3]);
			}else{
				_os_printf("recv error\r\n");
				goto end;
			}
		}

end:
		close(tcp_connect_fd);


}



void tcpAccept(void *e, void *d)
{
	int fd = (int)d;
	int tcp_connect_fd = -1;
	struct  sockaddr_in client;  
	socklen_t  addrlen;

	TCP_SERVER_H tcpHeader;
	memset(&tcpHeader, 0, sizeof(TCP_SERVER_H));

	addrlen =sizeof(client);  
	tcp_connect_fd = accept(fd, (struct sockaddr*)&client, &addrlen);
	if(tcp_connect_fd < 0)
	{
		_os_printf("accept()error\n");  
	}
	os_task_create("tcp_recv", tcp_recv_data, (void*)tcp_connect_fd, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2048);
}


void Tcp_Server_test()
{
	uint16_t port = 8090;

	_os_printf("[test] init tcp server: port: %d\n", port);

	int fd = InitSrv(port);
	
	eloop_add_fd( fd, EVENT_READ, EVENT_F_ENABLED, tcpAccept, (void*)fd );
}



static int8_t tcp_update_running = 0;
int get_tcp_update_status()
{
	return tcp_update_running;
}

void tcp_update(void *d)
{

	int tcp_connect_fd = (int)d;

	if(tcp_update_running)
	{
		goto end;
	}
	tcp_update_running = 1;
	TCP_SERVER_H tcpHeader;
	memset(&tcpHeader, 0, sizeof(TCP_SERVER_H));
 
	
#if DVP_EN	
		//dvp_close();
		void *dvp = (void *)dev_get(HG_DVP_DEVID);
		if(dvp)
		{
			dvp_close(dvp);
		}
#endif
	
#if USB_EN	
		SYSCTRL_REG_OPT(sysctrl_usb20_clk_close());
#endif
		if(recv(tcp_connect_fd, (char*)&tcpHeader, sizeof(TCP_SERVER_H), 0) > 0)
		{
			_os_printf("tcpHeader.Command = %x ###\n", tcpHeader.Command);
			switch(tcpHeader.Command)
			{
				case REQ_NET_UPGRADE:
				{
					tcpNetUpgrade(tcp_connect_fd);
					break;
				}
				default:
					break;
			}
		}
		tcp_update_running = 0;
end:
		
		close(tcp_connect_fd);

}






