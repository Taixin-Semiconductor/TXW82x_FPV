/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
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
#ifndef LWIP_HDR_LWIPOPTS_H
#define LWIP_HDR_LWIPOPTS_H
#ifdef __cplusplus
extern "C" {
#endif
extern void hw_memcpy(void *dest, const void *src, unsigned int size);
#ifdef __cplusplus
}
#endif

#include "sys_config.h"
#include "project_config.h"
/*lwipt configs*/

//#define LWIP_PORTING_PRINTF
#ifdef LWIP_PORTING_PRINTF
#define lwip_printf(fmt, ...) printf("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define lwip_printf(fmt, ...) do{}while(0)
#endif

#ifndef LWIP_THREAD_NUM
#define LWIP_THREAD_NUM 1
#endif
#define LWIP_SYS_THREAD_POOL_N LWIP_THREAD_NUM

#ifndef MEM_LIBC_MALLOC
#define MEM_LIBC_MALLOC 1
#endif

#ifndef MEMP_MEM_MALLOC
#define MEMP_MEM_MALLOC 1
#endif

#ifndef LWIP_TRANSPORT_ETHERNET
#define LWIP_TRANSPORT_ETHERNET       1
#endif

#ifndef MEM_SIZE
//define ram_heap size when MEM_LIBC_MALLOC is 0
#define MEM_SIZE                      (8*1024)
#endif

/* We link to special sys_arch.c (for basic non-waiting API layers unit tests) */
#define NO_SYS                      0
#ifndef LWIP_IPV4
#define LWIP_IPV4                   1
#endif

#ifndef LWIP_IPV6
#define LWIP_IPV6                   0
#endif

#if !LWIP_IPV4 && !LWIP_IPV4
#error "Either IPv4 or IPv6 must be enabled."
#endif

#define LWIP_NETIF_API              1
#define LWIP_NETIF_LOOPBACK         1

#define SYS_LIGHTWEIGHT_PROT        1
#define LWIP_NETCONN                !NO_SYS

#ifndef LWIP_RAW
#define LWIP_RAW                    1
#endif

#ifndef TCPIP_MBOX_SIZE
#if IP_NAT
#define TCPIP_MBOX_SIZE             512
#else
#define TCPIP_MBOX_SIZE             64
#endif
#endif//TCPIP_MBOX_SIZE

#ifndef DEFAULT_TCP_RECVMBOX_SIZE
#define DEFAULT_TCP_RECVMBOX_SIZE   10
#endif

#ifndef DEFAULT_UDP_RECVMBOX_SIZE
#define DEFAULT_UDP_RECVMBOX_SIZE   10
#endif

#ifndef DEFAULT_RAW_RECVMBOX_SIZE
#define DEFAULT_RAW_RECVMBOX_SIZE   4
#endif

#ifndef DEFAULT_ACCEPTMBOX_SIZE
#define DEFAULT_ACCEPTMBOX_SIZE     4
#endif

#ifdef LWIP_DEBUG
#ifndef TCPIP_THREAD_STACKSIZE
#define TCPIP_THREAD_STACKSIZE      4096
#endif
#else
#ifndef TCPIP_THREAD_STACKSIZE
#define TCPIP_THREAD_STACKSIZE      (1024+256)  //82X需要额外的任务栈(可能与浮点相关)
#endif
#endif

#ifndef TCPIP_THREAD_PRIO
#define TCPIP_THREAD_PRIO           OS_TASK_PRIORITY_ABOVE_NORMAL
#endif

#ifdef LWIP_DEBUG
#ifndef DEFAULT_THREAD_STACKSIZE
#define DEFAULT_THREAD_STACKSIZE    512*2
#endif
#else
#ifndef DEFAULT_THREAD_STACKSIZE
#define DEFAULT_THREAD_STACKSIZE    512
#endif
#endif

#ifndef MEMP_NUM_SYS_TIMEOUT
#define MEMP_NUM_SYS_TIMEOUT        10
#endif

// 32-bit alignment
#define MEM_ALIGNMENT               4

//#define LWIP_RAM_HEAP_POINTER       lwip_ram_heap

// Number of pool pbufs.
// Each requires 1514 bytes of RAM.
#ifndef PBUF_POOL_SIZE
#if IP_NAT
#define PBUF_POOL_SIZE              32//pbuf pool size,PBUF_POOL
#else
#define PBUF_POOL_SIZE              6//pbuf pool size,PBUF_POOL
#endif
#endif//PBUF_POOL_SIZE

// One tcp_pcb_listen is needed for each TCPServer.
// Each requires 72 bytes of RAM.
#ifndef MEMP_NUM_TCP_PCB_LISTEN
#define MEMP_NUM_TCP_PCB_LISTEN     2
#endif

// One is tcp_pcb needed for each TCPSocket.
// Each requires 196 bytes of RAM.
#ifndef MEMP_NUM_TCP_PCB
#define MEMP_NUM_TCP_PCB            4
#endif
// One udp_pcb is needed for each UDPSocket.
// Each requires 84 bytes of RAM (total rounded to multiple of 512).
#ifndef MEMP_NUM_UDP_PCB
#define MEMP_NUM_UDP_PCB            4
#endif

// Number of non-pool pbufs.
// Each requires 92 bytes of RAM.
#ifndef MEMP_NUM_PBUF
#define MEMP_NUM_PBUF               10 //PBUF_REF/ROM
#endif 

// Each netbuf requires 64 bytes of RAM.
#ifndef MEMP_NUM_NETBUF
#define MEMP_NUM_NETBUF             16
#endif

#ifndef MEMP_NUM_TCP_SEG
#define MEMP_NUM_TCP_SEG            8
#endif

#ifndef TCP_MSS
#define TCP_MSS                     1460
#endif

/* TCP sender buffer space (bytes). */
#ifndef TCP_SND_BUF
#define TCP_SND_BUF                 (4*TCP_MSS)
#endif

#ifndef TCP_SND_QUEUELEN
#define TCP_SND_QUEUELEN            (2 * TCP_SND_BUF/TCP_MSS)
#endif

#ifndef TCP_SNDLOWAT
#define TCP_SNDLOWAT                (TCP_SND_BUF/2)
#endif

#ifndef TCP_SNDQUEUELOWAT
#define TCP_SNDQUEUELOWAT           LWIP_MAX(((TCP_SND_QUEUELEN)/2), 5)
#endif

#ifndef LWIP_WND_SCALE
#if IP_NAT
#define LWIP_WND_SCALE              1
#define TCP_RCV_SCALE               3
#else 
#define LWIP_WND_SCALE              0
#define TCP_RCV_SCALE               0
#endif
#endif  //LWIP_WND_SCALE

#ifndef TCP_WND
#define TCP_WND                     (4*TCP_MSS)
#endif

#ifndef LWIP_TCP_TIMESTAMPS
#define LWIP_TCP_TIMESTAMPS         0
#endif

#ifndef LWIP_TCPIP_CORE_LOCKING
#define LWIP_TCPIP_CORE_LOCKING     0
#endif

/**
 * LWIP_SO_SNDTIMEO==1: Enable send timeout for sockets/netconns and
 * SO_SNDTIMEO processing.
 */
#ifndef LWIP_SO_SNDTIMEO
#define LWIP_SO_SNDTIMEO                1
#endif

/**
 * LWIP_SO_RCVTIMEO==1: Enable receive timeout for sockets/netconns and
 * SO_RCVTIMEO processing.
 */
#ifndef LWIP_SO_RCVTIMEO
#define LWIP_SO_RCVTIMEO                1
#endif

#ifndef LWIP_SO_SNDRCVTIMEO_NONSTANDARD
#define LWIP_SO_SNDRCVTIMEO_NONSTANDARD 1
#endif

/**
 * MEMP_NUM_TCPIP_MSG_API: the number of struct tcpip_msg, which are used
 * for callback/timeout API communication.
 * (only needed if you use tcpip.c)
 */
#ifndef MEMP_NUM_TCPIP_MSG_API 
#define MEMP_NUM_TCPIP_MSG_API          10
#endif

/**
 * MEMP_NUM_TCPIP_MSG_INPKT: the number of struct tcpip_msg, which are used
 * for incoming packets.
 * (only needed if you use tcpip.c)
 */
#ifndef MEMP_NUM_TCPIP_MSG_INPKT
#define MEMP_NUM_TCPIP_MSG_INPKT        16
#endif

#ifndef MEMCPY
#define MEMCPY(dst,src,len)             hw_memcpy(dst,src,len)
#endif

/**
 * SMEMCPY: override this with care! Some compilers (e.g. gcc) can inline a
 * call to memcpy() if the length is known at compile time and is small.
 */
#ifndef SMEMCPY
#define SMEMCPY(dst,src,len)            hw_memcpy(dst,src,len)
#endif

#ifndef MEMP_NUM_REASSDATA
#define MEMP_NUM_REASSDATA          10
#endif

// One netconn is needed for each UDPSocket, TCPSocket or TCPServer.
// Each requires 236 bytes of RAM (total rounded to multiple of 512).
#ifndef MEMP_NUM_NETCONN
#define MEMP_NUM_NETCONN            6
#endif

/**
 * LWIP_SO_RCVBUF==1: Enable SO_RCVBUF processing.
 */
#ifndef TCP_QUEUE_OOSEQ
#define TCP_QUEUE_OOSEQ             1
#endif

#ifndef LWIP_DHCP
#define LWIP_DHCP                   LWIP_IPV4
#endif

#ifndef DHCP_DOES_ARP_CHECK
#define DHCP_DOES_ARP_CHECK 0
#endif

#ifndef LWIP_DNS
#define LWIP_DNS                    1
#endif

#ifndef DNS_MAX_NAME_LENGTH
#define DNS_MAX_NAME_LENGTH 32
#endif

#ifndef DNS_TABLE_SIZE
#define DNS_TABLE_SIZE 1
#endif

#ifndef LWIP_SOCKET
#define LWIP_SOCKET                 1
#endif

#ifndef SO_REUSE
#define SO_REUSE                    1
#endif

#ifndef LWIP_IGMP
#define LWIP_IGMP                   0
#endif

#ifdef TXWSDK_POSIX
#define LWIP_COMPAT_SOCKETS         2
#endif

#ifndef LWIP_TCP_KEEPALIVE
#define LWIP_TCP_KEEPALIVE          1
#endif

// Fragmentation on, as per IPv4 default
#define LWIP_IPV6_FRAG              LWIP_IPV6

// Queuing "disabled", as per IPv4 default (so actually queues 1)
#ifndef LWIP_ND6_QUEUEING
#define LWIP_ND6_QUEUEING           0
#endif

// Debug Options
#define NETIF_DEBUG                 LWIP_DBG_OFF
#define PBUF_DEBUG                  LWIP_DBG_OFF
#define API_LIB_DEBUG               LWIP_DBG_OFF
#define API_MSG_DEBUG               LWIP_DBG_OFF
#define SOCKETS_DEBUG               LWIP_DBG_OFF
#define ICMP_DEBUG                  LWIP_DBG_OFF
#define IGMP_DEBUG                  LWIP_DBG_OFF
#define INET_DEBUG                  LWIP_DBG_OFF
#define IP_DEBUG                    LWIP_DBG_OFF
#define IP_REASS_DEBUG              LWIP_DBG_OFF
#define RAW_DEBUG                   LWIP_DBG_OFF
#define MEM_DEBUG                   LWIP_DBG_OFF
#define MEMP_DEBUG                  LWIP_DBG_OFF
#define SYS_DEBUG                   LWIP_DBG_OFF
#define TIMERS_DEBUG                LWIP_DBG_OFF
#define TCP_DEBUG                   LWIP_DBG_OFF
#define TCP_INPUT_DEBUG             LWIP_DBG_OFF
#define TCP_FR_DEBUG                LWIP_DBG_OFF
#define TCP_RTO_DEBUG               LWIP_DBG_OFF
#define TCP_CWND_DEBUG              LWIP_DBG_OFF
#define TCP_WND_DEBUG               LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG            LWIP_DBG_OFF
#define TCP_RST_DEBUG               LWIP_DBG_OFF
#define TCP_QLEN_DEBUG              LWIP_DBG_OFF
#define UDP_DEBUG                   LWIP_DBG_OFF
#define TCPIP_DEBUG                 LWIP_DBG_OFF
#define SLIP_DEBUG                  LWIP_DBG_OFF
#define DHCP_DEBUG                  LWIP_DBG_OFF
#define AUTOIP_DEBUG                LWIP_DBG_OFF
#define DNS_DEBUG                   LWIP_DBG_OFF
#define IP6_DEBUG                   LWIP_DBG_OFF

#define PPP_DEBUG                   LWIP_DBG_OFF
#define ETHARP_DEBUG                LWIP_DBG_OFF
#define UDP_LPC_EMAC                LWIP_DBG_OFF

#ifdef LWIP_DEBUG
#define MEMP_OVERFLOW_CHECK         1
#define MEMP_SANITY_CHECK           1
#else
#define LWIP_NOASSERT               1
#define LWIP_STATS                  0
#endif

#define LWIP_DBG_TYPES_ON           LWIP_DBG_ON
#define LWIP_DBG_MIN_LEVEL          LWIP_DBG_LEVEL_ALL

#ifndef LWIP_PLATFORM_BYTESWAP
#define LWIP_PLATFORM_BYTESWAP      1
#endif

#ifndef LWIP_RANDOMIZE_INITIAL_LOCAL_PORTS
#define LWIP_RANDOMIZE_INITIAL_LOCAL_PORTS 1
#endif

#ifndef MAC2STR
#define MAC2STR(a) (a)[0]&0xff, (a)[1]&0xff, (a)[2]&0xff, (a)[3]&0xff, (a)[4]&0xff, (a)[5]&0xff
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif
    
#ifndef STR2MAC
#define STR2MAC(s, a) str2mac((const uint8 *)(s), (uint8 *)(a))
#endif


#if LWIP_TRANSPORT_ETHERNET

// Broadcast
#ifndef IP_SOF_BROADCAST
#define IP_SOF_BROADCAST            0
#endif

#ifndef IP_SOF_BROADCAST_RECV
#define IP_SOF_BROADCAST_RECV       0
#endif

#ifndef LWIP_BROADCAST_PING
#define LWIP_BROADCAST_PING         0
#endif

#ifndef LWIP_CHECKSUM_ON_COPY
#define LWIP_CHECKSUM_ON_COPY       0
#endif

#ifndef LWIP_NETIF_HOSTNAME
#define LWIP_NETIF_HOSTNAME         0
#endif

#ifndef LWIP_NETIF_STATUS_CALLBACK
#define LWIP_NETIF_STATUS_CALLBACK  0
#endif

#ifndef LWIP_NETIF_LINK_CALLBACK
#define LWIP_NETIF_LINK_CALLBACK    0
#endif

#elif LWIP_TRANSPORT_PPP

#define TCP_SND_BUF                     (3 * 536)
#define TCP_WND                         (2 * 536)

#define LWIP_ARP 0

#define PPP_SUPPORT 0
#define CHAP_SUPPORT                    1
#define PAP_SUPPORT                     1
#define PPP_THREAD_STACKSIZE            4*192
#define PPP_THREAD_PRIO 0

#define MAXNAMELEN                      64     /* max length of hostname or name for auth */
#define MAXSECRETLEN                    64

#define MEM_OVERFLOW_CHECK              0

#else
#error A transport mechanism (Ethernet or PPP) must be defined
#endif
#endif /* LWIPOPTS_H_ */
