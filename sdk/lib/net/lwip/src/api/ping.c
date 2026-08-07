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

void lwip_ping(char *ip_domain, int pktsize, unsigned int send_times)
{
    int isdomain = 0;
    int seqno = 0;
    int sock = -1;
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

    while (send_cnt++ < send_times || send_times == 0) {
        ICMPH_TYPE_SET(echo, ICMP_ECHO);
        ICMPH_CODE_SET(echo, 0);
        echo->chksum = 0;
        echo->id     = 0xAFAF;
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
                if ((iecho->id == 0xAFAF) && (iecho->seqno == echo->seqno)) {
                    recv_cnt++;
                    os_printf("Reply from %s: bytes=%d time:%dms TTL=255\n",
                              inet_ntoa(from.sin_addr.s_addr), pktsize, recv_tick - ping_tick);
                    break;
                }
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

