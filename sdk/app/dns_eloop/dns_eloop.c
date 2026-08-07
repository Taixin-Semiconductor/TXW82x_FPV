/**
  ******************************************************************************
  * @file    dns_eloop.c
  * @author  HUGE-IC Application Team
  * @version V1.0.0
  * @date    2021-07-16
  * @brief   a simple dns server
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2021 HUGE-IC</center></h2>
  *
  *
  ******************************************************************************
  */

#include "typesdef.h"
#include "list.h"
#include "osal/task.h"
#include "osal/string.h"
#include "osal/sleep.h"
#include "lib/common/sysevt.h"
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "netif/ethernetif.h"
#include "lwip/ip.h"
#include "lwip/init.h"
#include "lwip/prot/dns.h"
//#include "lib/net/dns/dns.h"
#include "dns_eloop.h"
#include "event.h"


#define dns_dbg(fmt, ...) os_printf(fmt, ##__VA_ARGS__)
#define dns_err(fmt, ...) os_printf(fmt, ##__VA_ARGS__)

#define DNS_RECVBUF_SIZE (512)

struct dns_mgr {
    struct netif *netif;
    int32  sock;
    uint8  recvbuf[DNS_RECVBUF_SIZE];
    uint8  stop: 1;
};

static struct dns_mgr *dns;

static void dns_query_refuse(struct sockaddr_in *addr, uint8 *data, uint32 len)
{
    struct dns_hdr *hdr = (struct dns_hdr *)data;
    // 置起flags中response，并且返回query失败原因, 我们并未实现dns功能，只是拒绝了查询
    hdr->flags1 |= DNS_FLAG1_RESPONSE;
    hdr->flags2 &= ~DNS_FLAG2_ERR_MASK;
    hdr->flags2 |= 5; // 5: Refused - 服务器拒绝执行请求操作
    sendto(dns->sock, data, len, 0, (const struct sockaddr *)addr, sizeof(struct sockaddr));
    //dns_dbg("dns query refused - %s:%d\r\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
}

static void dns_task_eloop(void *ei, void *d)
{
    int32 ret = 0;
    socklen_t len = 16; // 接收socket地址需要用
    struct sockaddr_in addr;
    struct dns_hdr *hdr;

    ret = recvfrom(dns->sock, dns->recvbuf, DNS_RECVBUF_SIZE, 0, (struct sockaddr *)&addr, &len);
    // 最少dns_hdr长度12
    if (ret >= 12 && !dns->stop) {
        hdr = (struct dns_hdr *)dns->recvbuf;
        // 最少dns_hdr + dns_query = 12 + 4
        if (hdr->numquestions > 0 && ret >= (12 + 4)) {
            dns_query_refuse(&addr, dns->recvbuf, ret);
        }
    }
}

__init static int32 _dns_start(char *ifname)
{
    int32 ret  = -1;
    int32 sock = -1;
    int32 optval = 1;
    struct sockaddr_in local_addr;
    struct ifreq nif;

    dns = os_zalloc(sizeof(struct dns_mgr));
    if(dns == NULL)
        return -ENOMEM;
    
    dns->netif = netif_find(ifname);
    if (dns->netif == NULL) {
        dns_err("netif:%s is not exist\r\n", ifname);
        return RET_ERR;
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        dns_err("create socket failed\r\n");
        return RET_ERR;
    }

    fcntl(sock, F_SETFL, O_NONBLOCK);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));
    os_strncpy(nif.ifr_name, dns->netif->name, 2);
    setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, &nif, sizeof(nif));
    os_memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(53);
    local_addr.sin_addr.s_addr = dns->netif->ip_addr.addr;
    ret = bind(sock, (struct sockaddr *)&local_addr, sizeof(struct sockaddr));
    if (ret == -1) {
        dns_err("bind fail\r\n");
        closesocket(sock);
        return RET_ERR;
    }

    dns->sock = sock;
    dns_dbg("dns sock :%x\r\n", (uint32)sock);
    eloop_add_fd(dns->sock, EVENT_READ, EVENT_F_ENABLED, dns_task_eloop, NULL);
    
    return RET_OK;
}

int32 dns_start_eloop(char *ifname)
{
    int32 ret = 0;

    if (ifname == NULL) {
        dns_err("invalid param!\r\n");
        return RET_ERR;
    }
    
    if (dns) {
        dns->stop = 0;
        dns_err("dns already running ...\r\n");
        return RET_OK;
    }
    
    ret = _dns_start(ifname);
    ASSERT(!ret);
    return ret;
}

void dns_stop_eloop(char *ifname)
{
    if(dns){
        dns->stop = 1;
    }
}


