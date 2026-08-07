#ifndef _HGSDK_NETUTILS_H_
#define _HGSDK_NETUTILS_H_

#include "dev.h"
#include "hal/netdev.h"

#ifdef __cplusplus
extern "C" {
#endif

struct udpdata_info {
    uint8 *dest, *src;
    uint32 dest_ip, dest_port;
    uint32 src_ip, src_port;
    uint8 *data;
    uint32 len;
};

struct icmp_info {
    uint8 *dest, *src;
    uint32 dest_ip;
    uint32 src_ip;
    uint8 *data;
    uint32 len;
    uint8  type;
    uint16 sn;
};

uint16 net_checksum(uint16 initcksum, uint8 *data, int32 len);
uint16 net_pseudo_checksum(uint8 proto, uint8 *data, uint16 len, uint32 *srcaddr, uint32 *destaddr);
uint16 net6_pseudo_checksum(uint8 *srcip, uint8 *destip, uint16 nexthdr, uint8 *payload, uint16 plen);
uint16 net_setup_udpdata(struct udpdata_info *data, uint8 *udp);
int32 net_setup_icmp(struct icmp_info *data, uint8 *icmp);
void wnb_ota_init(void);
void net_atcmd_init(void);
void netlog_init(uint16 port);
int32 sntp_client_init(char *ntp_server, uint16 update_interval);
void sntp_client_fresh(void);
void sntp_set_server(char *ntp_server);
void sntp_set_interval(uint16 interval);
void sntp_client_disable(int8 disable);
void sntp_set_sport(uint16 server_port);
int32 dns_redirect_init(void);
int32 dns_redirect_add(const char *domain, const char *ifname);
void icmp_pkt_monitor(const char *prefix, uint8 *data, uint32 len);
void icmp_pkt_monitor_scatter(const char *prefix, scatter_data *data, uint32 count);
int32 icmp_monitor_init(void);

uint32 net_get_dhcp_transID(uint8 *ether_data, uint8 **client_mac);
uint8 net_get_dhcp_msgtype(uint8 *opt, uint32 len);
uint8 net_get_dhcpv6_msgtype(uint8 *opt, uint32 len);
uint8 *net_get_dhcp_option(uint8 *opt, uint32 len, uint8 opcode, uint32 *olen);
const char *dhcpv6_msgtype_str(uint8 type);
const char *dhcp_msgtype_str(uint8 type);

int32 wifi_mcastfw_enable(uint8 enable, uint8 TOTAL_1s, uint8 EACH_1s, uint8 dhcp_only);
void  wifi_mcastfw_clear(void);

void auto_wds_init(void);
void auto_wds_update(uint8 ifidx, uint32 valid_ip);

static inline int32 ipv4_is_mcast_addr(uint32 ip)
{
    return (ip > 0xE0000000 && ip <= 0xEFFFFFFF) || (ip&0xff) == 0xff;
}
static inline int32 ipv6_is_mcast_addr(uint8       *ip)
{
   return ip[0] == 0xff;
}

typedef enum {
    SSWITCH_RULE_CHECK_PORT_INPUT,
    SSWITCH_RULE_CHECK_LOCAL_INPUT,
    SSWITCH_RULE_CHECK_FORWARD,
    SSWITCH_RULE_CHECK_LOCAL_OUTPUT,
    SSWITCH_RULE_CHECK_LOCAL_OUTPUT_SCATTER,
} SSWITCH_RULE_CHECK;
typedef enum {
    SSWITCH_FLAGS_STATIC_ONLY   = BIT(0), // Only use static MAC tables, do not dynamically update MAC tables
    SSWITCH_FLAGS_FORWARD_UCAST = BIT(1), // Allow forwarding of unicast packets with unknown destinations
    SSWITCH_FLAGS_FORWARD_MCAST = BIT(2), // Allow forwarding of multicast packets
} SSWITCH_FLAGS;
void sswitch_dev_status(void);
void sswitch_dev_mactbl_dump(void);
void sswitch_dev_mactbl_delete(uint8 *addr);
void sswitch_dev_mactbl_update(uint8 sport, uint8 *src_mac, uint8 is_static);
int32 sswitch_dev_port_enable(uint8 port_id, uint8 enable);
int32 sswitch_dev_port_config(uint8 port_id, struct netdev *ndev, uint8 mac);
int32 sswitch_dev_rule_check(SSWITCH_RULE_CHECK type, uint8 *data, uint32 len, uint32 port);
int32 sswitch_init(uint8 port_cnt, uint32 mactbl_max, SSWITCH_FLAGS flags);

#ifdef __cplusplus
}
#endif
#endif
