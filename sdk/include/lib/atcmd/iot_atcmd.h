#ifndef _TXW_IOT_ATCMD_H_
#define _TXW_IOT_ATCMD_H_
#include "basic_include.h"

#ifdef __cplusplus
extern "C" {
#endif

#define iot_atcmd_dbg(fmt, ...) //os_printf("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define iot_atcmd_err(fmt, ...) os_printf(KERN_ERR"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

struct iot_atcmd_link {
    uint8  type: 4, parent: 4;
    uint8  mode: 3, connected: 1, svr: 1, rx_pending:1;
    int8   sock; 
    uint8  connection_cnt;
    uint16 remote_port;
    uint32 remote_ip;
    uint32 send_len;
    uint8 *rx_buff;
    uint16 rx_len;
    uint16 rd_off;
};

struct iot_atcmd_mgr {
    uint32 uart_baudrate;
    uint8  uart_databits;
    uint8  uart_stopbits;
    uint8  uart_parity;
    uint8  uart_flowcontrol;

    uint8  CIPMUX: 1, //多连接模式
           CIPMODE: 1, //透传模式
           CIPRECVMODE: 1 //数据接收模式: 0-主动， 1-被动
           ;
    uint8  link_max;
    uint8  cur_txlink, svr_link;
    uint8  max_connection, rev;
    uint16 tx_len;
    struct uart_device *uart;

    ////////////////////////////////
    struct iot_atcmd_link links[0];
};

extern struct iot_atcmd_mgr *iot_atcmd;

int32 atcmd_TEST(const char *cmd, char *argv[], uint32 count);
int32 atcmd_RESTOR(const char *cmd, char *argv[], uint32 count);
int32 atcmd_RESET(const char *cmd, char *argv[], uint32 count);
int32 atcmd_GMR(const char *cmd, char *argv[], uint32 count);
int32 atcmd_GSLP(const char *cmd, char *argv[], uint32 count);
int32 atcmd_ATE(const char *cmd, char *argv[], uint32 count);
int32 atcmd_UART_CUR(const char *cmd, char *argv[], uint32 count);
int32 atcmd_UART_DEF(const char *cmd, char *argv[], uint32 count);
int32 atcmd_SLEEP(const char *cmd, char *argv[], uint32 count);
int32 atcmd_WAKEUPGPIO(const char *cmd, char *argv[], uint32 count);
int32 atcmd_RFPOWER(const char *cmd, char *argv[], uint32 count);
int32 atcmd_RFVDD(const char *cmd, char *argv[], uint32 count);
int32 atcmd_SYSRAM(const char *cmd, char *argv[], uint32 count);
int32 atcmd_SYSADC(const char *cmd, char *argv[], uint32 count);
int32 atcmd_SYSIOSETCFG(const char *cmd, char *argv[], uint32 count);
int32 atcmd_SYSIOGETCFG(const char *cmd, char *argv[], uint32 count);
int32 atcmd_SYSGPIODIR(const char *cmd, char *argv[], uint32 count);
int32 atcmd_SYSGPIOWRITE(const char *cmd, char *argv[], uint32 count);
int32 atcmd_SYSGPIOREAD(const char *cmd, char *argv[], uint32 count);

int32 atcmd_CIPSTATUS(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPDOMAIN(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPSTART(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPSSLSIZE(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPSSLCCONF(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPSEND(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPSENDEX(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPSENDBUF(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPBUFRESET(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPBUFSTATUS(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPCHECKSEQ(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPCLOSE(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIFSR(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPMUX(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPSERVER(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPSERVERMAXCONN(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPMODE(const char *cmd, char *argv[], uint32 count);
int32 atcmd_SAVETRANSLINK(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPSTO(const char *cmd, char *argv[], uint32 count);
int32 atcmd_PING(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIUPDATE(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPDINFO(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPRECVMODE(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPRECVDATA(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPRECVLEN(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPSNTPCFG(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPSNTPTIME(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPDNS_CUR(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPDNS_DEF(const char *cmd, char *argv[], uint32 count);

int32 atcmd_CWMODE(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CWJAP(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CWLAP(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CWQAP(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CWSAP(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CWLIF(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CWDHCP_CUR(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CWDHCP_DEF(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CWDHCPS_CUR(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CWDHCPS_DEF(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CWAUTOCONN(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPSTAMAC_CUR(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPSTAMAC_DEF(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPAPMAC_CUR(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPAPMAC_DEF(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPSTA_CUR(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPSTA_DEF(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPAP_CUR(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CIPAP_DEF(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CWHOSTNAME(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CWCOUNTRY_CUR(const char *cmd, char *argv[], uint32 count);
int32 atcmd_CWCOUNTRY_DEF(const char *cmd, char *argv[], uint32 count);

#ifdef __cplusplus
}
#endif
#endif

