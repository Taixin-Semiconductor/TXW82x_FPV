/**
 * @file
 * Ethernet input function - handles INCOMING ethernet level traffic
 * To be used in most low-level netif implementations
 */

/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * Copyright (c) 2003-2004 Leon Woestenberg <leon.woestenberg@axon.tv>
 * Copyright (c) 2003-2004 Axon Digital Design B.V., The Netherlands.
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

#ifndef LWIP_ETHERNETIF_H
#define LWIP_ETHERNETIF_H
#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/netdev.h"
#include "lwipopts.h"
#include "lwip/def.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"

/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 'n'

#define MIN_PACKET_LEN  14
#define MAX_TRANS_UNIT  1500
#define MAX_NETWORK_LAYER   8

/**
 * Helper struct to hold private data used to operate your ethernet interface.
 * Keeping the ethernet address of the MAC in this struct is not necessary
 * as it is already kept in the struct netif.
 * But this is only an example, anyway...
 */
struct ethernetif {
    /* Add whatever per-interface state that is needed here. */
    struct netdev *ndev;
};

err_t lwip_netif_remove_register(struct netdev *ndev);
err_t lwip_netif_remove(struct netdev *ndev);
err_t lwip_netif_add(struct netdev *ndev, char *name, ip_addr_t *ipaddr, ip_addr_t *netmask, ip_addr_t *gw);
err_t lwip_netif_set_default(struct netdev *ndev);
err_t lwip_netif_set_default2(char *name);
void lwip_netdev_input_cb(struct netdev *ndev, uint8 *data, uint32 data_len, void *priv);
err_t lwip_netif_set_dhcp(struct netdev *ndev, uint8 enable);
err_t lwip_netif_set_dhcp2(char *name, uint8 enable);
int32 lwip_netif_set_ip(struct netdev *ndev, ip_addr_t *ipaddr, ip_addr_t *netmask, ip_addr_t *gw);
int32 lwip_netif_set_ip2(char *name, ip_addr_t *ipaddr, ip_addr_t *netmask, ip_addr_t *gw);

ip_addr_t lwip_netif_get_ip(struct netdev *ndev);
ip_addr_t lwip_netif_get_netmask(struct netdev *ndev);
ip_addr_t lwip_netif_get_gw(struct netdev *ndev);
uint8 *lwip_netif_get_mac(struct netdev *ndev);

ip_addr_t lwip_netif_get_ip2(const char *name);
ip_addr_t lwip_netif_get_netmask2(const char *name);
ip_addr_t lwip_netif_get_gw2(const char *name);
uint8 *lwip_netif_get_mac2(const char *name);
void lwip_netif_updown(struct netdev *ndev, uint8 up);
void lwip_netif_updown2(char *ifname, uint8 up);

uint8 *lwip_netif_hwaddr(struct netif* netif);
uint32 lwip_netif_ipaddr4(struct netif* netif);
uint8 *lwip_netif_ipaddr6(struct netif* netif);
err_t lwip_netif_linkoutput(struct netif* netif, struct pbuf *buf);

#endif /* LWIP_ETHERNETIF_H */
