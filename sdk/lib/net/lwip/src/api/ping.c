#include "sys_config.h"
#include "typesdef.h"
#include "osal/string.h"

#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/inet_chksum.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/ip.h"
#include "lwip/icmp6.h"
#include "lwip/raw.h"
#include "lwip/ip6_addr.h"

static int ping_recv(int sockfd, void *buf, int len, int flags, struct sockaddr *src_addr,
                     socklen_t *addrlen, unsigned int timeout_ms)
{
    int             ret = 0;
    struct timeval  to;
    fd_set          fdset;
    FD_ZERO(&fdset);
    FD_SET(sockfd, &fdset);
    to.tv_sec = timeout_ms / 1000;
    to.tv_usec = (timeout_ms % 1000) * 1000;
    ret = lwip_select(sockfd + 1, &fdset, NULL, NULL, timeout_ms == 0 ? NULL : &to);

    if (ret > 0) {
        if (FD_ISSET(sockfd, &fdset)) {
            ret = lwip_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);

            if (ret <= 0) {
                ret = -1;
            }
        } else {
            ret = -1;
        }
    }

    return ret;
}

void lwip_ping4(char *ip_domain, int pktsize, unsigned int send_times)
{
    int isdomain = 0;
    int sock = -1;
    uint16 seqno = 0;
    uint16 echo_id;
    uint32 send_cnt = 0;
    uint32 recv_cnt = 0;
    uint32 timeout_ms = 3000;
    uint64 ping_tick = 0;
    uint64 recv_tick = 0;
    struct sockaddr_in from;
    struct sockaddr_in to;
    struct icmp_echo_hdr *echo;
    int ipaddr = inet_addr(ip_domain);
    socklen_t addr_len  = sizeof(struct sockaddr_in);
    int buff_len  = sizeof(struct icmp_echo_hdr) + pktsize;
    char *recvbuf = NULL;

    if (ipaddr == INADDR_NONE) {
        struct hostent *host = lwip_gethostbyname(ip_domain, 1);
        if (host == NULL) {
            os_printf("can not resolve domain name:%s\n", ip_domain);
            return;
        }

        ipaddr = *(int *)host->h_addr;
        isdomain = 1;
    }

    if ((sock = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP)) < 0) {
        os_printf("create socket failed!\n");
        return;
    }

    echo = os_malloc(buff_len);
    if (echo == NULL) {
        os_printf("malloc failed, request to malloc size:0x%x\n", buff_len);
        lwip_close(sock);
        return ;
    }

    recvbuf = os_malloc(buff_len + sizeof(struct ip_hdr));

    if (recvbuf == NULL) {
        os_printf("malloc failed, request to malloc size:0x%x\n", buff_len);
        os_free(echo);
        lwip_close(sock);
        return ;
    }

    to.sin_addr.s_addr = ipaddr;
    to.sin_family = AF_INET;
    to.sin_len = sizeof(to);

    if (isdomain) {
        os_printf("\n\nPinging %s[%s] with %d bytes of data:\n", ip_domain, inet_ntoa(ipaddr), pktsize);
    } else {
        os_printf("\n\nPinging %s with %d bytes of data:\n", ip_domain, pktsize);
    }

    os_random_bytes((uint8 *)&seqno, 2);
    os_random_bytes((uint8 *)&echo_id, 2);
    while (send_cnt++ < send_times || send_times == 0) {
        ICMPH_TYPE_SET(echo, ICMP_ECHO);
        ICMPH_CODE_SET(echo, 0);
        echo->chksum = 0;
        echo->id     = echo_id;
        echo->seqno  = htons(++seqno);
        echo->chksum = inet_chksum(echo, buff_len);
        ping_tick = os_jiffies();

        if (lwip_sendto(sock, (char *)echo, buff_len, 0, (const struct sockaddr *)&to, addr_len) > 0) {
            while (ping_recv(sock, recvbuf, buff_len + sizeof(struct ip_hdr), 0, (struct sockaddr *)&from, &addr_len, timeout_ms) >
                (int)(sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr))) {
                recv_tick = os_jiffies();
                struct ip_hdr *iphdr = (struct ip_hdr *)recvbuf;
                struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr *)(recvbuf + (IPH_HL(iphdr) * 4));
                // ¨°?¡¤¨¤¨°??¡À¨º?¦Ì?????icmp¡ã¨¹¦Ì???2??¨¹3?¨º¡À
                if (TIME_AFTER(recv_tick, ping_tick + os_msecs_to_jiffies(timeout_ms))) {
                    os_printf("Request timed out.\n");
                    break;
                }
                if ((iecho->id == echo_id) && (iecho->seqno == echo->seqno)) {
                    recv_cnt++;
                    os_printf("Reply from %s: bytes=%d time:%dms TTL=255\n",
                              inet_ntoa(from.sin_addr.s_addr), pktsize, recv_tick - ping_tick);
                    break;
                }
                mcu_watchdog_feed();
            }
        } else {
            os_printf("Ping %s error!!\n", inet_ntoa(ipaddr));
        }

        os_sleep_ms(1000);
    }

    os_printf("----------------------------------------------------------\n"\
              "Ping statistics for %s:\n"\
              "Packets: Sent = %d, Received = %d, Lost = %d (%d%% loss)\n",
              inet_ntoa(ipaddr), send_times, recv_cnt, (send_times - recv_cnt),
              (send_times - recv_cnt) * 100 / send_times);
    lwip_close(sock);
    os_free(echo);
    os_free(recvbuf);
}
/*
int get_netif_ip6_global_addr(struct netif *netif, ip_addr_t *ipaddr)
{
    char ipaddr_str[IP6ADDR_STRLEN_MAX];
    int i = 0;

    if (netif == NULL || ipaddr == NULL) {
        os_printf("input param error!\n");
        return -1;
    }
    if (!netif_is_up(netif) || !netif_is_link_up(netif)) {
        os_printf("Network interface is not up or link is down\n");
        return -2;
    }
    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i))) {
            //ip6addr_ntoa_r(netif_ip6_addr(netif, i), addr_str, sizeof(addr_str));
            if (ip6_addr_isglobal(netif_ip6_addr(netif, i))) {
                memset(ipaddr, 0, sizeof(ip_addr_t));
                //memcpy(&ipaddr->u_addr.ip6,netif_ip6_addr(netif, i),sizeof(ip6_addr_t));
                //IP_SET_TYPE(ipaddr, IPADDR_TYPE_V6);
                IP_ADDR6(ipaddr, netif_ip6_addr(netif, i)->addr[0], netif_ip6_addr(netif, i)->addr[1],
                         netif_ip6_addr(netif, i)->addr[2], netif_ip6_addr(netif, i)->addr[3]);
                os_printf("%s:Check netif %c%c global ip6addr:%s\n",
                          __FUNCTION__, netif->name[0], netif->name[1], ipaddr_ntoa_r(ipaddr,
                                  ipaddr_str, sizeof(ipaddr_str)));
                return 0;
            }
        }
    }
    return -3;
}
*/
static void ipv6_print_hex(char *buf, int len)
{
    int i = 0;

    for (i = 0; i < len; i++) {
        if (i > 0 && i % 16 == 0) { printf("\r\n"); }
        else if (i > 0 && i % 8 == 0) { printf("  "); }
        printf("index:%d of total %d:%02x\n ", i, len, buf[i] & 0xff);
    }
    printf("\r\n\r\n");
}

//#define PING6_DATA_SIZE 56  // ICMPv6 Echo Requestçææè½½è·å¤§å°
//#define PING6_TIMEOUT 1000  // è¶æ¶æ¶é´ï¼æ¯«ç§ï¼

int lwip_ipv6_ping(const ip_addr_t *dest_addr, int pktsize, unsigned int send_times)
{
#if LWIP_IPV6
    int32 sock = -1;
    struct sockaddr_in6 dest_sockaddr;
    struct sockaddr from;
    socklen_t addr_len  = sizeof(struct sockaddr);
    struct icmp6_echo_hdr *iecho = NULL;
    struct ip6_hdr *ip6hdr = NULL;
    struct icmp_echo_hdr *recho = NULL;
    uint16_t seqno = 0;
    uint16_t id    = 0xAFAF;
    int32 ret = 0;
    uint8 *pkt = NULL;
    int32 pkt_len = sizeof(struct icmp6_echo_hdr) + pktsize;
    uint8 *reply = NULL;
    int32 recv_len = sizeof(struct ip6_hdr) + sizeof(struct icmp6_echo_hdr) + pktsize;
    uint32 send_cnt = 0;
    uint32 recv_cnt = 0;
    uint64 recv_tick   = 0;
    uint64 ping_tick   = 0;
    uint32 ping6_timeout = 3000;

    // æ£æ¥ç®æ å°åæ¯å¦ä¸ºIPv6å°å
    if (!IP_IS_V6(dest_addr)) {
        os_printf("Invalid IPv6 destination address!\n");
        ret = -1;
        goto __finish;
    }

    // åå»º ICMPv6 åå§å¥æ¥å­
    sock = lwip_socket(AF_INET6, SOCK_RAW, IP6_NEXTH_ICMP6);
    if (sock < 0) {
        os_printf("Failed to create socket\n");
        ret = -1;
        goto __finish;
    }

    // è®¾ç½®ç®æ å°å
    memset(&dest_sockaddr, 0, sizeof(dest_sockaddr));
    dest_sockaddr.sin6_family = AF_INET6;
    memcpy(&dest_sockaddr.sin6_addr, &dest_addr->u_addr.ip6, sizeof(ip6_addr_t));

    pkt = os_malloc(sizeof(struct icmp6_echo_hdr) + pktsize);
    if (!pkt) {
        os_printf("%s,%d:Error!No memory!\n", __FUNCTION__, __LINE__);
        lwip_close(sock);
        ret = -2;
        goto __finish;
    }
    reply = os_malloc(recv_len);
    if (!reply) {
        os_printf("%s:Error!No memory!\n", __FUNCTION__, __LINE__);
        ret = -2;
        goto __finish;
    }

    while (send_cnt++ < send_times || send_times == 0) {
        iecho = (struct icmp6_echo_hdr *)pkt;
        iecho->type = ICMP6_TYPE_EREQ;
        iecho->code = 0;
        iecho->chksum = 0;
        iecho->id = lwip_htons(id);
        iecho->seqno = lwip_htons(++seqno);
        memset((char *)iecho + sizeof(struct icmp6_echo_hdr), 0xA5, pktsize);  //å¡«åææè½½è·
        ping_tick = os_jiffies();

        if (lwip_sendto(sock, pkt, pkt_len, 0, (const struct sockaddr *)&dest_sockaddr, addr_len)
            > 0) {
            while (ping_recv(sock, reply, recv_len, 0, (struct sockaddr *)&from, &addr_len,
                             ping6_timeout) >
                   (int)(sizeof(struct ip6_hdr) + sizeof(struct icmp6_echo_hdr))) {
                recv_tick = os_jiffies();
                ip6hdr = (struct ip6_hdr *)reply;
                recho = (struct icmp_echo_hdr *)(reply + sizeof(struct ip6_hdr));
                if (TIME_AFTER(recv_tick, ping_tick + os_msecs_to_jiffies(ping6_timeout))) {
                    os_printf("Request timed out.\n");
                    break;
                }
                if ((recho->id == 0xAFAF) && (recho->seqno == iecho->seqno)) {
                    recv_cnt++;
                    os_printf("Reply from %s: bytes=%d,time=%dms TTL=255\n",
                              ip6addr_ntoa((ip6_addr_t *)&ip6hdr->src), pktsize,
                              os_jiffies_to_msecs(recv_tick - ping_tick));
                    break;
                }
            }
        } else {
            os_printf("Ping %s error!!\n", ipaddr_ntoa(dest_addr));
        }
        os_sleep_ms(1000);
    }
    os_printf("----------------------------------------------------------\n"\
              "Ping statistics for %s:\n"\
              "Packets: Sent = %d, Received = %d, Lost = %d (%d%% loss)\n",
              ipaddr_ntoa(dest_addr), send_times, recv_cnt, (send_times - recv_cnt),
              (send_times - recv_cnt) * 100 / send_times);
__finish:
    lwip_close(sock);
    if (*pkt)
    { os_free(pkt); }
    if (reply)
    { os_free(reply); }
    return ret;
#else
    return -1;
#endif
}

void lwip_ping6(char *ip_domain, int pktsize, unsigned int send_times)
{
#if LWIP_IPV6
    ip_addr_t ipaddr;
    char dest_addr_str[IP6ADDR_STRLEN_MAX];

    if (ip_domain == NULL) {
        os_printf("\n\nInput param error!\n");
        return;
    }
    memset(&ipaddr, 0, sizeof(ip_addr_t));
    ipaddr_aton(ip_domain, &ipaddr);
    os_printf("\n\n%s:Pinging %s[%s] with %d bytes of data:\n", __FUNCTION__,
              ip_domain, ipaddr_ntoa_r(&ipaddr, dest_addr_str, sizeof(dest_addr_str)), pktsize);

    lwip_ipv6_ping(&ipaddr, pktsize, send_times);
#endif
}

void lwip_ping(char *ip_domain, int pktsize, unsigned int send_times)
{
    ip_addr_t ip;
    if (ipaddr_aton(ip_domain, &ip)) {
        if (IP_IS_V6_VAL(ip)) {
            printf("IPv6 ping address!\n");
            lwip_ping6(ip_domain, pktsize, send_times);
        } else if (IP_IS_V4_VAL(ip)) {
            printf("IPv4 ping address!\n");
            lwip_ping4(ip_domain, pktsize, send_times);
        } else {
            printf("Invaild ping address:%s\n", ip_domain);
        }
    } else {
        os_printf("%s:Invaild input ipaddr:%s\n", ip_domain);
        return;
    }
}




