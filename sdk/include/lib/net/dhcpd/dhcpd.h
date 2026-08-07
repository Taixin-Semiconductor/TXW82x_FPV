#ifndef _HUGEIC_DHCPD_H_
#define _HUGEIC_DHCPD_H_
#ifdef __cplusplus
extern "C" {
#endif

struct dhcpd_param {
    uint32 lease_time; /*unit: seconds*/
    uint32 start_ip, end_ip; /*network byteorder*/
    uint32 netmask; /*network byteorder*/
    uint32 router;  /*network byteorder*/
    uint32 dns1;    /*network byteorder*/
    uint32 dns2;    /*network byteorder*/
};

struct dhcpd_leaseinfo {
    uint32 ip;
    uint8 mac[6];
};

int32 dhcpd_get_lease_list(struct dhcpd_leaseinfo *leaseslist, int32 list_size);
uint32 dhcpd_get_lease_ip(uint8 *mac);
void dhcpd_ip_inactive(uint8 *mac);
int32 dhcpd_start(char *ifname, struct dhcpd_param *param);
void dhcpd_stop(void);
void dhcpd_flush(void);
void dhcpd_set_dns(uint32 dns1, uint32 dns2);
void dhcpd_dump_ippool(void);

#ifdef __cplusplus
}
#endif
#endif
