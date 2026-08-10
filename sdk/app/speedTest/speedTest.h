#ifndef __SPEEDTEST_H
#define __SPEEDTEST_H
void speedTest_udp_server(int port);
void speedTest_udp_client(const char *ip,int port);
void speedTest_tcp_server(int port);


#endif
