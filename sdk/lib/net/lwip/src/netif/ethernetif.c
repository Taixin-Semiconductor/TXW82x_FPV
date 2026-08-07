/**
 * @file
 * Ethernet Interface Skeleton
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/*
 * This file is a skeleton for developing Ethernet network interface
 * drivers for lwIP. Add code to the low_level functions and do a
 * search-and-replace for the word "ethernetif" to replace it with
 * something that better describes your network interface.
 */
#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "osal/string.h"
#include "hal/dma.h"
#include "lwipopts.h"
#include "lwip/opt.h"

#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "lwip/etharp.h"
#include "lwip/dhcp.h"
#include "netif/ppp/pppoe.h"

#include "lwip/tcpip.h"
#include "sys_arch.h"
#include "lwip/ip4_addr.h"
#include "lwip/err.h"
#include "netif/ethernetif.h"
#include "lwip/netifapi.h"
#include "lib/net/ethphy/ethtool.h"
#include "hal/netdev.h"


/**
 * Helper struct to hold private data used to operate your ethernet interface.
 * Keeping the ethernet address of the MAC in this struct is not necessary
 * as it is already kept in the struct netif.
 * But this is only an example, anyway...
 */

/* Forward declarations. */
static void ethernetif_input(struct netif *netif, void *data, uint32 data_len);

/**
 * Process a pbuf chain data to gmac transfer buffer address
 * This function need to communicate with gmac layer.
 * @ *p  : Input,input pbuf from lwip to gmac
 * @ *buf: Output,output all data in a pbuf chain to a buffer
 * @ *frag_count ï¼Output,output the counts of pbufs in input pbuf chain
 */
static u16_t
ethernetif_pbuf_convert(struct pbuf *p, u8_t *buf)
{
    struct pbuf *q = NULL;
    u16_t  len     = 0;

    for (q = p; q != NULL; q = q->next) {
        hw_memcpy((u8_t *)&buf[len], (u8_t *)q->payload, q->len);
        len += q->len;
    }

    return len;
}

static void ethernetif_print_hex(char *buf, int len)
{
    int i = 0;

    for (i = 0; i < len; i++) {
        if (i > 0 && i % 16 == 0) { printf("\r\n"); }
        else if (i > 0 && i % 8 == 0) { printf("  "); }
        printf("%02x ", buf[i] & 0xff);
    }
    printf("\r\n\r\n");
}

static u16_t
ethernetif_get_scatter_data(struct pbuf *p, scatter_data *data, uint32 size)
{
    struct pbuf *q = NULL;
    u16_t  count   = 0;

    for (q = p; q != NULL && count < size; q = q->next) {
        data[count].addr = (uint8 *)q->payload;
        data[count].size = q->len;
        count++;
        if (count == size && q->next) {
            printf("scaterr error\r\n");
        }
    }
    return count;
}
void lwip_netdev_input_cb(struct netdev *ndev, uint8 *data, uint32 data_len, void *priv)
{
    struct netif *netif = (struct netif *)priv;
    if (ndev == NULL || data == NULL) {
        lwip_printf("Input para error!\n");
        return;
    }
    if (netif == NULL) {
        lwip_printf("Input para error!netif is NULL\n");
        return;
    }
    //lwip_printf("Recv packet,data len:%d\n",data_len);
    ethernetif_input(netif, (void *)data, data_len);
}

static void
low_level_del(struct netif *netif)
{
    struct ethernetif *ethernetif = netif->state;
    netdev_close(ethernetif->ndev);
    mem_free(netif);
    printf("low level del...................................\r\n");
}

/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void
low_level_init(struct netif *netif)
{
    struct ethernetif *ethernetif = netif->state;
    int32 ret = 0;
    u8_t  emac_mac_addr[6];

    ret = netdev_open(ethernetif->ndev, lwip_netdev_input_cb, NULL, netif);
    if (ret) {
        lwip_printf("Open netdev failed,ret:%d\n", ret);
    }

    os_memset(emac_mac_addr, 0, sizeof(emac_mac_addr));
    /* set MAC hardware address length */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    /* set MAC hardware address */
    netdev_get_macaddr(ethernetif->ndev, emac_mac_addr);
    os_memcpy(netif->hwaddr, emac_mac_addr, 6);

    /* maximum transfer unit */
    netif->mtu = MAX_TRANS_UNIT;

    /* device capabilities */
    /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_ETHERNET;

#ifdef LWIP_IGMP
    netif->flags |= NETIF_FLAG_IGMP;
#endif
#if LWIP_IPV6_MLD
    netif->flags |= NETIF_FLAG_MLD6;
#endif

#if LWIP_IPV6 && LWIP_IPV6_MLD
      /*
       * For hardware/netifs that implement MAC filtering.
       * All-nodes link-local is handled by default, so we must let the hardware know
       * to allow multicast packets in.
       * Should set mld_mac_filter previously. */
      if (netif->mld_mac_filter != NULL) {
        ip6_addr_t ip6_allnodes_ll;
        ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
        netif->mld_mac_filter(netif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
      }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
}


/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become available since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
#ifdef SCATTER_TX_OFF
    static uint8 tx_buf[1600];
#endif
    struct ethernetif  *ethernetif = netif->state;
    struct netdev *dev        = ethernetif->ndev;
    scatter_data   scat_data[MAX_NETWORK_LAYER];
    int scat_cnt = 0;
    s32_t ret = 0;

    if (p->tot_len < MIN_PACKET_LEN) {
        return ERR_OK;
    }

#if ETH_PAD_SIZE
    pbuf_remove_header(p, ETH_PAD_SIZE);
#endif

    if (((struct netdev_hal_ops *)dev->dev.ops)->send_scatter_data != NULL) {
        scat_cnt = ethernetif_get_scatter_data(p, scat_data, MAX_NETWORK_LAYER);
#if LWIP_PCAP
        pcap_scatter(netif, scat_data, scat_cnt);
#endif
        ret = netdev_send_scatter_data(dev, scat_data, scat_cnt);
    } else {
#if LWIP_PCAP
        pcap(netif, p->payload, p->tot_len);
#endif
        if (p->next == NULL) {
            ret = netdev_send_data(dev, p->payload, p->len);
        } else {
#ifdef SCATTER_TX_OFF
            ret = netdev_send_data(dev, (void *)tx_buf, ethernetif_pbuf_convert(p, tx_buf));
#else
            printf("*error, device not support scatter tx*\r\n");
#endif
        }
    }

    if (ret) {
        switch(ret){ //转换成lwip自定义的error
            case -ENOMEM:
                ret = ERR_MEM;
                break;
            default:
                ret = ERR_ARG;
                break;
        }

        //printf("Send to netdev error,ret:%d\n", ret);
        return ret;
    }

    MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
    if (((u8_t *)p->payload)[0] & 1) {
        /* broadcast or multicast packet*/
        MIB2_STATS_NETIF_INC(netif, ifoutnucastpkts);
    } else {
        /* unicast packet */
        MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
    }
    /* increase ifoutdiscards or ifouterrors on error */

#if ETH_PAD_SIZE
    pbuf_add_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    LINK_STATS_INC(link.xmit);
    return ERR_OK;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
 
static struct pbuf *
low_level_input(struct netif *netif, void *data, u32_t data_len)
{
    struct pbuf *p, *q;
    //int i = 10;
    /* Obtain the size of the packet and put it into the "len"
       variable. */
    u32_t len  = data_len;

#if ETH_PAD_SIZE
    len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

    /* We allocate a pbuf chain of pbufs from the pool. */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (p != NULL) {
#if ETH_PAD_SIZE
        pbuf_remove_header(p, ETH_PAD_SIZE); /* drop the padding word */
#endif
        /* We iterate over the pbuf chain until we have read the entire
         * packet into the pbuf. */
        for (q = p; q != NULL; q = q->next) {
            hw_memcpy((u8_t *)q->payload, (u8_t *)data, q->len);
        }

        MIB2_STATS_NETIF_ADD(netif, ifinoctets, p->tot_len);
        if (((u8_t *)p->payload)[0] & 1) {
            /* broadcast or multicast packet*/
            MIB2_STATS_NETIF_INC(netif, ifinnucastpkts);
        } else {
            /* unicast packet*/
            MIB2_STATS_NETIF_INC(netif, ifinucastpkts);
        }
#if ETH_PAD_SIZE
        pbuf_add_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

        LINK_STATS_INC(link.recv);
    } else {
        //drop packet();
        //lwip_printf("pbuf malloc failed! drop packet\n");
        LINK_STATS_INC(link.memerr);
        LINK_STATS_INC(link.drop);
        MIB2_STATS_NETIF_INC(netif, ifindiscards);
    }

    return p;
}


/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
static void
ethernetif_input(struct netif *netif, void *data, uint32 data_len)
{
    struct pbuf    *p;

    /* move received packet into a new pbuf */
    p = low_level_input(netif, data, data_len);
    /* if no packet could be read, silently ignore this */
    if (p != NULL) {
        /* pass all packets to ethernet_input, which decides what packets it supports */
        if (netif->input(p, netif) != ERR_OK) {
            LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
            pbuf_free(p);
            p = NULL;
        }
    }
}

void ethernetif_delete(struct netif *netif)
{
    if (netif != NULL) {
        low_level_del(netif);
    }
}

err_t ethernetif_updown(struct netif *netif, int up)
{
    struct ethernetif *ethernetif = (struct ethernetif *)netif->state;
    if (up) {
        return netdev_open(ethernetif->ndev, lwip_netdev_input_cb, NULL, netif);
    }
	return ERR_ARG;
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t ethernetif_init(struct netif *netif)
{
    if (netif == NULL) {
        return ERR_ARG;
    }

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

    /*
     * Initialize the snmp variables and counters inside the struct netif.
     * The last argument should be replaced with your link speed, in units
     * of bits per second.
     */
    MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

    /* We directly use etharp_output() here to save a function call.
     * You can instead declare your own function an call etharp_output()
     * from it if you have to do some checks before sending (e.g. if link
     * is available...) */
    netif->output = etharp_output;
#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
    netif->linkoutput = low_level_output;
    netif->updown = ethernetif_updown;

    /* initialize the hardware */
    low_level_init(netif);
    return ERR_OK;
}

err_t lwip_netif_remove_register(struct netdev *ndev)
{
#if LWIP_NETIF_REMOVE_CALLBACK
    netif_set_remove_callback((struct netif *)ndev->stack_data, ethernetif_delete);
#endif
    return ERR_OK;
}

err_t lwip_netif_remove(struct netdev *ndev)
{
    struct netif *nif = ndev->stack_data;
    netif_remove(nif);
    ndev->stack_data = NULL;
    return ERR_OK;
}

err_t lwip_netif_add(struct netdev *ndev, char *name, ip_addr_t *ipaddr, ip_addr_t *netmask, ip_addr_t *gw)
{
    struct netif *nif = NULL;
    struct ethernetif *ethernetif = NULL;

    if (ndev == NULL) {
        lwip_printf("Input para error!\n");
        return ERR_ARG;
    }

    nif = mem_malloc(sizeof(struct netif));
    if (nif == NULL) {
        os_printf("lwip_netif_add: out of memory\n");
        return ERR_MEM;
    }

    ethernetif = mem_malloc(sizeof(struct ethernetif));
    if (ethernetif == NULL) {
        mem_free(nif);
        os_printf("ethernetif_init: out of memory\n");
        return ERR_MEM;
    }

    os_memset(nif, 0, sizeof(struct netif));
    os_memset(ethernetif, 0, sizeof(struct ethernetif));

    if (name) {
        nif->name[0] = name[0];
        nif->name[1] = name[1];
    } else {
        nif->name[0] = IFNAME0;
        nif->name[1] = IFNAME1;
    }

    ethernetif->ndev = ndev;
    if (!netif_add(nif, ipaddr, netmask, gw, (void *)ethernetif, ethernetif_init, tcpip_input)) {
        mem_free(nif);
        mem_free(ethernetif);
        os_printf("netif_add error!\r\n");
        return ERR_IF;
    }

    ndev->stack_data = (void *)nif;
    netif_set_link_up(nif);
    netif_set_up(nif);
    return ERR_OK;
}

err_t lwip_netif_set_default(struct netdev *ndev)
{
    netif_set_default((struct netif *)ndev->stack_data);
    return ERR_OK;
}

err_t lwip_netif_set_default2(char *name)
{
    netif_set_default(netif_find(name));
    return ERR_OK;
}

static err_t _lwip_netif_set_dhcp(struct netif *nif, uint8 enable)
{
    err_t ret = 0;

    if (nif == NULL) {
        lwip_printf("Input para error!\n");
        return ERR_ARG;
    }

    if (enable) {
        ret = netifapi_dhcp_start(nif);
        if (ret) {
            lwip_printf("DHCP enable Failed,ret:%d\n", ret);
            return ret;
        }
    } else {
        netifapi_dhcp_stop(nif);
    }
    return 0;
}

static int32 _lwip_netif_set_ip(struct netif *nif, ip_addr_t *ipaddr, ip_addr_t *netmask, ip_addr_t *gw)
{
    if (nif) {
        if (ipaddr) {
            netif_set_ipaddr(nif, (const ip4_addr_t *)ipaddr);
        }
        if (netmask) {
            netif_set_netmask(nif, (const ip4_addr_t *)netmask);
        }
        if (gw) {
            netif_set_gw(nif, (const ip4_addr_t *)gw);
        }
        return RET_OK;
    }
    return -EINVAL;
}

static ip_addr_t _lwip_netif_get_netaddr(struct netif *nif, int8 type)
{
    ip_addr_t addr;
    if (nif) {
        if (type == 0) { return nif->ip_addr; }
        if (type == 1) { return nif->netmask; }
        if (type == 2) { return nif->gw; }
    }
    addr.addr = 0;
    return addr;
}

err_t lwip_netif_set_dhcp(struct netdev *ndev, uint8 enable)
{
    return _lwip_netif_set_dhcp((struct netif *)ndev->stack_data, enable);
}
err_t lwip_netif_set_dhcp2(char *name, uint8 enable)
{
    return _lwip_netif_set_dhcp(netif_find(name), enable);
}

int32 lwip_netif_set_ip(struct netdev *ndev, ip_addr_t *ipaddr, ip_addr_t *netmask, ip_addr_t *gw)
{
    return _lwip_netif_set_ip((struct netif *)ndev->stack_data, ipaddr, netmask, gw);
}
int32 lwip_netif_set_ip2(char *name, ip_addr_t *ipaddr, ip_addr_t *netmask, ip_addr_t *gw)
{
    return _lwip_netif_set_ip(netif_find(name), ipaddr, netmask, gw);
}

ip_addr_t lwip_netif_get_ip(struct netdev *ndev)
{
    return _lwip_netif_get_netaddr((struct netif *)ndev->stack_data, 0);
}
ip_addr_t lwip_netif_get_netmask(struct netdev *ndev)
{
    return _lwip_netif_get_netaddr((struct netif *)ndev->stack_data, 1);
}
ip_addr_t lwip_netif_get_gw(struct netdev *ndev)
{
    return _lwip_netif_get_netaddr((struct netif *)ndev->stack_data, 2);
}
uint8 *lwip_netif_get_mac(struct netdev *ndev)
{
    struct netif *nif = (struct netif *)ndev->stack_data;
    return nif ? nif->hwaddr : NULL;
}

ip_addr_t lwip_netif_get_ip2(const char *name)
{
    return _lwip_netif_get_netaddr(netif_find(name), 0);
}
ip_addr_t lwip_netif_get_netmask2(const char *name)
{
    return _lwip_netif_get_netaddr(netif_find(name), 1);
}
ip_addr_t lwip_netif_get_gw2(const char *name)
{
    return _lwip_netif_get_netaddr(netif_find(name), 2);
}
uint8 *lwip_netif_get_mac2(const char *name)
{
    struct netif *nif =  netif_find(name);
    return nif ? nif->hwaddr : NULL;
}

int32 lwip_netif_hook_inputdata(struct netif *nif, uint8 *data, uint32 len)
{
    struct ethernetif *ethernetif = (struct ethernetif *)nif->state;
    return netdev_hook_inputdata(ethernetif->ndev, (uint32)data, len);
}


void lwip_netif_updown(struct netdev *ndev, uint8 up)
{
    if (ndev && ndev->stack_data) {
        if (up) {
            netif_set_up((struct netif *)ndev->stack_data);
        } else   {
            netif_set_down((struct netif *)ndev->stack_data);
        }
    }
}

void lwip_netif_updown2(char *ifname, uint8 up)
{
    struct netif *nif = netif_find(ifname);
    if(nif){
        if (up) {
            netif_set_up(nif);
        } else   {
            netif_set_down(nif);
        }
    }
}

uint8 *lwip_netif_hwaddr(struct netif* netif)
{
    return netif->hwaddr;
}

uint32 lwip_netif_ipaddr4(struct netif* netif)
{
#if LWIP_IPV4
    return netif->ip_addr.addr;
#else
    return 0;
#endif
}

uint8 *lwip_netif_ipaddr6(struct netif* netif)
{
#if LWIP_IPV6
    return NULL;
#else
    return NULL;
#endif
}

err_t lwip_netif_linkoutput(struct netif* netif, struct pbuf *buf)
{
    return netif->linkoutput(netif, buf);
}

