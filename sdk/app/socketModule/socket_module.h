#ifndef __SOCKET_MODULE_H
#define __SOCKET_MODULE_H
#ifndef INVALID_SOCKET
#define INVALID_SOCKET ~0
#endif

int creat_udp_socket(int port);
int creat_tcp_socket(int port);


#endif
