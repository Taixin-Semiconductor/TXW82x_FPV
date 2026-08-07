#include "lwip/api.h"
#include "lwip/sockets.h"
#include "socket_module.h"




//udp服务器创建
int creat_udp_socket(int port)
{
	int t_socket;
	struct sockaddr_in addr;
	int err;
	t_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (t_socket == INVALID_SOCKET)
	{
		_os_printf("socket error !");
		return 0;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htons(INADDR_ANY);
     

	err = bind(t_socket, (struct sockaddr*) &addr, sizeof(struct sockaddr_in));
	if (err ==  - 1)
	{
		_os_printf("%s bind err:%d,maybe port %d had be bind\n",__FUNCTION__,err,port);
		closesocket(t_socket);
		return  -1;
	}
	return t_socket;
}


int creat_tcp_socket(int port)
{
	int t_socket;
	struct sockaddr_in addr;
	int err;
	t_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (t_socket == INVALID_SOCKET)
	{
		_os_printf("socket error !");
		return 0;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htons(INADDR_ANY);
     
	err = bind(t_socket, (struct sockaddr*) &addr, sizeof(struct sockaddr_in));
	if (err ==  - 1)
	{
		_os_printf("%s bind err:%d,maybe port %d had be bind\n",__FUNCTION__,err,port);
		closesocket(t_socket);
		return  0;
	}
	return t_socket;
}

