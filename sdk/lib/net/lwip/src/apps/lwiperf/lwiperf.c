/**
 * @file
 * lwIP iPerf server implementation
 */

/**
 * @defgroup iperf Iperf server
 * @ingroup apps
 *
 * This is a simple performance measuring client/server to check your bandwidth using
 * iPerf2 on a PC as server/client.
 * It is currently a minimal implementation providing a TCP client/server only.
 *
 * @todo:
 * - implement UDP mode
 * - protect combined sessions handling (via 'related_master_state') against reallocation
 *   (this is a pointer address, currently, so if the same memory is allocated again,
 *    session pairs (tx/rx) can be confused on reallocation)
 */

/*
 * Copyright (c) 2014 Simon Goldschmidt
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
 * Author: Simon Goldschmidt
 */

#include "lwip/apps/lwiperf.h"

#include "lwip/tcp.h"
#include "lwip/sys.h"
#include "lwip/inet.h"
#include "lwip/ip_addr.h"

#include <string.h>

#include "lwip/sockets.h"
#include "lwip/timeouts.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define IPERF_UDP_HEADER_LEN       12              
#define UDP_RECV_BUF_LEN    (8192)
#define time_after(a,b)  ((long)(b) - (long)(a) < 0)

/* Currently, only TCP is implemented */
#if LWIP_TCP && LWIP_CALLBACK_API

/** Specify the idle timeout (in seconds) after that the test fails */
#ifndef LWIPERF_TCP_MAX_IDLE_SEC
#define LWIPERF_TCP_MAX_IDLE_SEC    10U
#endif
#if LWIPERF_TCP_MAX_IDLE_SEC > 255
#error LWIPERF_TCP_MAX_IDLE_SEC must fit into an u8_t
#endif

/** Change this if you don't want to lwiperf to listen to any IP version */
#ifndef LWIPERF_SERVER_IP_TYPE
#define LWIPERF_SERVER_IP_TYPE      IPADDR_TYPE_ANY
#endif

/* File internal memory allocation (struct lwiperf_*): this defaults to
   the heap */
#ifndef LWIPERF_ALLOC
#define LWIPERF_ALLOC(type)         mem_malloc(sizeof(type))
#define LWIPERF_FREE(type, item)    mem_free(item)
#endif

/** If this is 1, check that received data has the correct format */
#ifndef LWIPERF_CHECK_RX_DATA
#define LWIPERF_CHECK_RX_DATA       0
#endif

/** This is the Iperf settings struct sent from the client */
typedef struct _lwiperf_settings {
#define LWIPERF_FLAGS_ANSWER_TEST 0x80000000
#define LWIPERF_FLAGS_ANSWER_NOW  0x00000001
  u32_t flags;
  u32_t num_threads; /* unused for now */
  u32_t remote_port;
  u32_t buffer_len; /* unused for now */
  u32_t win_band; /* TCP window / UDP rate: unused for now */
  u32_t amount; /* pos. value: bytes?; neg. values: time (unit is 10ms: 1/100 second) */
} lwiperf_settings_t;

/** Basic connection handle */
struct _lwiperf_state_base;
typedef struct _lwiperf_state_base lwiperf_state_base_t;
struct _lwiperf_state_base {
  /* linked list */
  lwiperf_state_base_t *next;
  /* 1=tcp, 0=udp */
  u8_t tcp;
  /* 1=server, 0=client */
  u8_t server;
  /* master state used to abort sessions (e.g. listener, main client) */
  lwiperf_state_base_t *related_master_state;
};

/** Connection handle for a TCP iperf session */
typedef struct _lwiperf_state_tcp {
  lwiperf_state_base_t base;
  struct tcp_pcb *server_pcb;
  struct tcp_pcb *conn_pcb;
  u32_t time_started;
  lwiperf_report_fn report_fn;
  void *report_arg;
  u8_t poll_count;
  u8_t next_num;
  /* 1=start server when client is closed */
  u8_t client_tradeoff_mode;
  u32_t bytes_transferred;
  lwiperf_settings_t settings;
  u8_t have_settings_buf;
  u8_t specific_remote;
  ip_addr_t remote_addr;
  unsigned long long xfer_record; //hugeic added
  unsigned long long tick_record; //hugeic added
} lwiperf_state_tcp_t;

/** List of active iperf sessions */
static lwiperf_state_base_t *lwiperf_all_connections;
/** A const buffer to send from: we want to measure sending, not copying! */
static const u8_t lwiperf_txbuf_const[1600] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
};

enum IPERF_UDP_MODE {
    IPERF_UDP_MODE_CLIENT = 0,
    IPERF_UDP_MODE_SERVER
};

struct iperf_udp {
    uint8 mode;
    void *task_hdl;
    char remote_ipaddr[64];
    uint16_t port;
    uint32_t duration_sec;
    uint32_t bandwidth_bps;
    uint32_t packet_len;
};

static err_t lwiperf_tcp_poll(void *arg, struct tcp_pcb *tpcb);
static void lwiperf_tcp_err(void *arg, err_t err);
static err_t lwiperf_start_tcp_server_impl(const ip_addr_t *local_addr, u16_t local_port,
                                           lwiperf_report_fn report_fn, void *report_arg,
                                           lwiperf_state_base_t *related_master_state, lwiperf_state_tcp_t **state);


/** Add an iperf session to the 'active' list */
static void
lwiperf_list_add(lwiperf_state_base_t *item)
{
  item->next = lwiperf_all_connections;
  lwiperf_all_connections = item;
}

/** Remove an iperf session from the 'active' list */
static void
lwiperf_list_remove(lwiperf_state_base_t *item)
{
  lwiperf_state_base_t *prev = NULL;
  lwiperf_state_base_t *iter;
  for (iter = lwiperf_all_connections; iter != NULL; prev = iter, iter = iter->next) {
    if (iter == item) {
      if (prev == NULL) {
        lwiperf_all_connections = iter->next;
      } else {
        prev->next = iter->next;
      }
      /* @debug: ensure this item is listed only once */
      for (iter = iter->next; iter != NULL; iter = iter->next) {
        LWIP_ASSERT("duplicate entry", iter != item);
      }
      break;
    }
  }
}

static lwiperf_state_base_t *
lwiperf_list_find(lwiperf_state_base_t *item)
{
  lwiperf_state_base_t *iter;
  for (iter = lwiperf_all_connections; iter != NULL; iter = iter->next) {
    if (iter == item) {
      return item;
    }
  }
  return NULL;
}

/** Call the report function of an iperf tcp session */
static void
lwip_tcp_conn_report(lwiperf_state_tcp_t *conn, enum lwiperf_report_type report_type)
{
  if ((conn != NULL) && (conn->report_fn != NULL)) {
    u32_t now, duration_ms, bandwidth_kbitpsec;
    now = sys_now();
    duration_ms = now - conn->time_started;
    if (duration_ms == 0) {
      bandwidth_kbitpsec = 0;
    } else {
      bandwidth_kbitpsec = (conn->bytes_transferred / duration_ms) * 8U;
    }
    conn->report_fn(conn->report_arg, report_type,
                    &conn->conn_pcb->local_ip, conn->conn_pcb->local_port,
                    &conn->conn_pcb->remote_ip, conn->conn_pcb->remote_port,
                    conn->bytes_transferred, duration_ms, bandwidth_kbitpsec);
  }
}

/** Close an iperf tcp session */
static void
lwiperf_tcp_close(lwiperf_state_tcp_t *conn, enum lwiperf_report_type report_type)
{
  err_t err;

  lwiperf_list_remove(&conn->base);
  lwip_tcp_conn_report(conn, report_type);
  if (conn->conn_pcb != NULL) {
    tcp_arg(conn->conn_pcb, NULL);
    tcp_poll(conn->conn_pcb, NULL, 0);
    tcp_sent(conn->conn_pcb, NULL);
    tcp_recv(conn->conn_pcb, NULL);
    tcp_err(conn->conn_pcb, NULL);
    err = tcp_close(conn->conn_pcb);
    if (err != ERR_OK) {
      /* don't want to wait for free memory here... */
      tcp_abort(conn->conn_pcb);
    }
  } else if (conn->server_pcb != NULL) {
    /* no conn pcb, this is the listener pcb */
    err = tcp_close(conn->server_pcb);
    LWIP_ASSERT("error", err == ERR_OK);
  }
  LWIPERF_FREE(lwiperf_state_tcp_t, conn);
}

/** Try to send more data on an iperf tcp session */
static err_t
lwiperf_tcp_client_send_more(lwiperf_state_tcp_t *conn)
{
  int send_more;
  err_t err;
  u16_t txlen;
  u16_t txlen_max;
  void *txptr;
  u8_t apiflags;

  LWIP_ASSERT("conn invalid", (conn != NULL) && conn->base.tcp && (conn->base.server == 0));

  do {
    send_more = 0;
    if (conn->settings.amount & PP_HTONL(0x80000000)) {
      /* this session is time-limited */
      u32_t now = sys_now();
      u32_t diff_ms = now - conn->time_started;
      u32_t time = (u32_t) - (s32_t)lwip_htonl(conn->settings.amount);
      u32_t time_ms = time * 10;
      if (diff_ms >= time_ms) {
        /* time specified by the client is over -> close the connection */
        lwiperf_tcp_close(conn, LWIPERF_TCP_DONE_CLIENT);
        return ERR_OK;
      }
    } else {
      /* this session is byte-limited */
      u32_t amount_bytes = lwip_htonl(conn->settings.amount);
      /* @todo: this can send up to 1*MSS more than requested... */
      if (conn->bytes_transferred >= amount_bytes) {
        /* all requested bytes transferred -> close the connection */
        lwiperf_tcp_close(conn, LWIPERF_TCP_DONE_CLIENT);
        return ERR_OK;
      }
    }

    if (conn->bytes_transferred < 24) {
      /* transmit the settings a first time */
      txptr = &((u8_t *)&conn->settings)[conn->bytes_transferred];
      txlen_max = (u16_t)(24 - conn->bytes_transferred);
      apiflags = TCP_WRITE_FLAG_COPY;
    } else if (conn->bytes_transferred < 48) {
      /* transmit the settings a second time */
      txptr = &((u8_t *)&conn->settings)[conn->bytes_transferred - 24];
      txlen_max = (u16_t)(48 - conn->bytes_transferred);
      apiflags = TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE;
      send_more = 1;
    } else {
      /* transmit data */
      /* @todo: every x bytes, transmit the settings again */
      txptr = LWIP_CONST_CAST(void *, &lwiperf_txbuf_const[conn->bytes_transferred % 10]);
      txlen_max = TCP_MSS;
      if (conn->bytes_transferred == 48) { /* @todo: fix this for intermediate settings, too */
        txlen_max = TCP_MSS - 24;
      }
      apiflags = 0; /* no copying needed */
      send_more = 1;
    }
    txlen = txlen_max;
    do {
      err = tcp_write(conn->conn_pcb, txptr, txlen, apiflags);
      if (err ==  ERR_MEM) {
        txlen /= 2;
      }
    } while ((err == ERR_MEM) && (txlen >= (TCP_MSS / 2)));

    if (err == ERR_OK) {
      conn->bytes_transferred += txlen;
    } else {
      send_more = 0;
    }
  } while (send_more);

    if (TIME_AFTER(os_jiffies(), conn->tick_record + os_msecs_to_jiffies(1000))) {
        os_printf("lwiperf test: %dKB/s\r\n", (conn->bytes_transferred - conn->xfer_record) >> 10);
        conn->xfer_record = conn->bytes_transferred;
        conn->tick_record = os_jiffies();
    }

    tcp_output(conn->conn_pcb);
    return ERR_OK;
}

/** TCP sent callback, try to send more data */
static err_t
lwiperf_tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  lwiperf_state_tcp_t *conn = (lwiperf_state_tcp_t *)arg;
  /* @todo: check 'len' (e.g. to time ACK of all data)? for now, we just send more... */
  LWIP_ASSERT("invalid conn", conn->conn_pcb == tpcb);
  LWIP_UNUSED_ARG(tpcb);
  LWIP_UNUSED_ARG(len);

  conn->poll_count = 0;

  return lwiperf_tcp_client_send_more(conn);
}

/** TCP connected callback (active connection), send data now */
static err_t
lwiperf_tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
  lwiperf_state_tcp_t *conn = (lwiperf_state_tcp_t *)arg;
  LWIP_ASSERT("invalid conn", conn->conn_pcb == tpcb);
  LWIP_UNUSED_ARG(tpcb);
  if (err != ERR_OK) {
    lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_REMOTE);
    return ERR_OK;
  }
  conn->poll_count = 0;
  conn->time_started = sys_now();
  return lwiperf_tcp_client_send_more(conn);
}

/** Start TCP connection back to the client (either parallel or after the
 * receive test has finished.
 */
#undef IPSTR
#define IPSTR "%d.%d.%d.%d"

static err_t
lwiperf_tx_start_impl(const ip_addr_t *remote_ip, u16_t remote_port, lwiperf_settings_t *settings, lwiperf_report_fn report_fn,
                      void *report_arg, lwiperf_state_base_t *related_master_state, lwiperf_state_tcp_t **new_conn)
{
  err_t err;
  lwiperf_state_tcp_t *client_conn;
  struct tcp_pcb *newpcb;
  ip_addr_t remote_addr;

  LWIP_ASSERT("remote_ip != NULL", remote_ip != NULL);
  LWIP_ASSERT("remote_ip != NULL", settings != NULL);
  LWIP_ASSERT("new_conn != NULL", new_conn != NULL);
  *new_conn = NULL;

  client_conn = (lwiperf_state_tcp_t *)LWIPERF_ALLOC(lwiperf_state_tcp_t);
  if (client_conn == NULL) {
    return ERR_MEM;
  }
  newpcb = tcp_new_ip_type(IP_GET_TYPE(remote_ip));
  if (newpcb == NULL) {
    LWIPERF_FREE(lwiperf_state_tcp_t, client_conn);
    return ERR_MEM;
  }
  memset(client_conn, 0, sizeof(lwiperf_state_tcp_t));
  client_conn->base.tcp = 1;
  client_conn->base.related_master_state = related_master_state;
  client_conn->conn_pcb = newpcb;
  client_conn->time_started = sys_now(); /* @todo: set this again on 'connected' */
  client_conn->report_fn = report_fn;
  client_conn->report_arg = report_arg;
  client_conn->next_num = 4; /* initial nr is '4' since the header has 24 byte */
  client_conn->bytes_transferred = 0;
  memcpy(&client_conn->settings, settings, sizeof(*settings));
  client_conn->have_settings_buf = 1;

  tcp_arg(newpcb, client_conn);
  tcp_sent(newpcb, lwiperf_tcp_client_sent);
  tcp_poll(newpcb, lwiperf_tcp_poll, 2U);
  tcp_err(newpcb, lwiperf_tcp_err);

  ip_addr_copy(remote_addr, *remote_ip);

  err = tcp_connect(newpcb, &remote_addr, remote_port, lwiperf_tcp_client_connected);
  if (err != ERR_OK) {
    lwiperf_tcp_close(client_conn, LWIPERF_TCP_ABORTED_LOCAL);
    return err;
  }
  lwiperf_list_add(&client_conn->base);
  *new_conn = client_conn;
  return ERR_OK;
}

static err_t
lwiperf_tx_start_passive(lwiperf_state_tcp_t *conn)
{
  err_t ret;
  lwiperf_state_tcp_t *new_conn = NULL;
  u16_t remote_port = (u16_t)lwip_htonl(conn->settings.remote_port);

  ret = lwiperf_tx_start_impl(&conn->conn_pcb->remote_ip, remote_port, &conn->settings, conn->report_fn, conn->report_arg,
    conn->base.related_master_state, &new_conn);
  if (ret == ERR_OK) {
    LWIP_ASSERT("new_conn != NULL", new_conn != NULL);
    new_conn->settings.flags = 0; /* prevent the remote side starting back as client again */
  }
  return ret;
}

/** Receive data on an iperf tcp session */
static err_t
lwiperf_tcp_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  u8_t tmp;
  u16_t tot_len;
  u32_t packet_idx;
  struct pbuf *q;
  lwiperf_state_tcp_t *conn = (lwiperf_state_tcp_t *)arg;

  LWIP_ASSERT("pcb mismatch", conn->conn_pcb == tpcb);
  LWIP_UNUSED_ARG(tpcb);

  if (err != ERR_OK) {
    lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_REMOTE);
    return ERR_OK;
  }
  if (p == NULL) {
    /* connection closed -> test done */
    if (conn->settings.flags & PP_HTONL(LWIPERF_FLAGS_ANSWER_TEST)) {
      if ((conn->settings.flags & PP_HTONL(LWIPERF_FLAGS_ANSWER_NOW)) == 0) {
        /* client requested transmission after end of test */
        lwiperf_tx_start_passive(conn);
      }
    }
    lwiperf_tcp_close(conn, LWIPERF_TCP_DONE_SERVER);
    return ERR_OK;
  }
  tot_len = p->tot_len;

  conn->poll_count = 0;

  if ((!conn->have_settings_buf) || ((conn->bytes_transferred - 24) % (1024 * 128) == 0)) {
    /* wait for 24-byte header */
    if (p->tot_len < sizeof(lwiperf_settings_t)) {
      lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_LOCAL_DATAERROR);
      pbuf_free(p);
      return ERR_OK;
    }
    if (!conn->have_settings_buf) {
      if (pbuf_copy_partial(p, &conn->settings, sizeof(lwiperf_settings_t), 0) != sizeof(lwiperf_settings_t)) {
        lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_LOCAL);
        pbuf_free(p);
        return ERR_OK;
      }
      conn->have_settings_buf = 1;
      if (conn->settings.flags & PP_HTONL(LWIPERF_FLAGS_ANSWER_TEST)) {
        if (conn->settings.flags & PP_HTONL(LWIPERF_FLAGS_ANSWER_NOW)) {
          /* client requested parallel transmission test */
          err_t err2 = lwiperf_tx_start_passive(conn);
          if (err2 != ERR_OK) {
            lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_LOCAL_TXERROR);
            pbuf_free(p);
            return ERR_OK;
          }
        }
      }
    } else {
      if (conn->settings.flags & PP_HTONL(LWIPERF_FLAGS_ANSWER_TEST)) {
        if (pbuf_memcmp(p, 0, &conn->settings, sizeof(lwiperf_settings_t)) != 0) {
          lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_LOCAL_DATAERROR);
          pbuf_free(p);
          return ERR_OK;
        }
      }
    }
    conn->bytes_transferred += sizeof(lwiperf_settings_t);
    if (conn->bytes_transferred <= 24) {
      conn->time_started = sys_now();
      tcp_recved(tpcb, p->tot_len);
      pbuf_free(p);
      return ERR_OK;
    }
    conn->next_num = 4; /* 24 bytes received... */
    tmp = pbuf_remove_header(p, 24);
    LWIP_ASSERT("pbuf_remove_header failed", tmp == 0);
    LWIP_UNUSED_ARG(tmp); /* for LWIP_NOASSERT */
  }

  packet_idx = 0;
  for (q = p; q != NULL; q = q->next) {
#if LWIPERF_CHECK_RX_DATA
    const u8_t *payload = (const u8_t *)q->payload;
    u16_t i;
    for (i = 0; i < q->len; i++) {
      u8_t val = payload[i];
      u8_t num = val - '0';
      if (num == conn->next_num) {
        conn->next_num++;
        if (conn->next_num == 10) {
          conn->next_num = 0;
        }
      } else {
        lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_LOCAL_DATAERROR);
        pbuf_free(p);
        return ERR_OK;
      }
    }
#endif
    packet_idx += q->len;
  }
  LWIP_ASSERT("count mismatch", packet_idx == p->tot_len);
  conn->bytes_transferred += packet_idx;
  tcp_recved(tpcb, tot_len);
  pbuf_free(p);
  return ERR_OK;
}

/** Error callback, iperf tcp session aborted */
static void
lwiperf_tcp_err(void *arg, err_t err)
{
  lwiperf_state_tcp_t *conn = (lwiperf_state_tcp_t *)arg;
  LWIP_UNUSED_ARG(err);

  /* pcb is already deallocated, prevent double-free */
  conn->conn_pcb = NULL;
  conn->server_pcb = NULL;

  lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_REMOTE);
}

/** TCP poll callback, try to send more data */
static err_t
lwiperf_tcp_poll(void *arg, struct tcp_pcb *tpcb)
{
  lwiperf_state_tcp_t *conn = (lwiperf_state_tcp_t *)arg;
  LWIP_ASSERT("pcb mismatch", conn->conn_pcb == tpcb);
  LWIP_UNUSED_ARG(tpcb);
  if (++conn->poll_count >= LWIPERF_TCP_MAX_IDLE_SEC) {
    lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_LOCAL);
    return ERR_OK; /* lwiperf_tcp_close frees conn */
  }

  if (!conn->base.server) {
    lwiperf_tcp_client_send_more(conn);
  }

  return ERR_OK;
}

/** This is called when a new client connects for an iperf tcp session */
static err_t
lwiperf_tcp_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  lwiperf_state_tcp_t *s, *conn;
  if ((err != ERR_OK) || (newpcb == NULL) || (arg == NULL)) {
    return ERR_VAL;
  }

  s = (lwiperf_state_tcp_t *)arg;
  LWIP_ASSERT("invalid session", s->base.server);
  LWIP_ASSERT("invalid listen pcb", s->server_pcb != NULL);
  LWIP_ASSERT("invalid conn pcb", s->conn_pcb == NULL);
  if (s->specific_remote) {
    LWIP_ASSERT("s->base.related_master_state != NULL", s->base.related_master_state != NULL);
    if (!ip_addr_eq(&newpcb->remote_ip, &s->remote_addr)) {
      /* this listener belongs to a client session, and this is not the correct remote */
      return ERR_VAL;
    }
  } else {
    LWIP_ASSERT("s->base.related_master_state == NULL", s->base.related_master_state == NULL);
  }

  conn = (lwiperf_state_tcp_t *)LWIPERF_ALLOC(lwiperf_state_tcp_t);
  if (conn == NULL) {
    return ERR_MEM;
  }
  memset(conn, 0, sizeof(lwiperf_state_tcp_t));
  conn->base.tcp = 1;
  conn->base.server = 1;
  conn->base.related_master_state = &s->base;
  conn->conn_pcb = newpcb;
  conn->time_started = sys_now();
  conn->report_fn = s->report_fn;
  conn->report_arg = s->report_arg;

  /* setup the tcp rx connection */
  tcp_arg(newpcb, conn);
  tcp_recv(newpcb, lwiperf_tcp_recv);
  tcp_poll(newpcb, lwiperf_tcp_poll, 2U);
  tcp_err(conn->conn_pcb, lwiperf_tcp_err);

  if (s->specific_remote) {
    /* this listener belongs to a client, so make the client the master of the newly created connection */
    conn->base.related_master_state = s->base.related_master_state;
    /* if dual mode or (tradeoff mode AND client is done): close the listener */
    if (!s->client_tradeoff_mode || !lwiperf_list_find(s->base.related_master_state)) {
      /* prevent report when closing: this is expected */
      s->report_fn = NULL;
      lwiperf_tcp_close(s, LWIPERF_TCP_ABORTED_LOCAL);
    }
  }
  lwiperf_list_add(&conn->base);
  return ERR_OK;
}

/**
 * @ingroup iperf
 * Start a TCP iperf server on the default TCP port (5001) and listen for
 * incoming connections from iperf clients.
 *
 * @returns a connection handle that can be used to abort the server
 *          by calling @ref lwiperf_abort()
 */
void *
lwiperf_start_tcp_server_default(lwiperf_report_fn report_fn, void *report_arg)
{
  return lwiperf_start_tcp_server(IP_ADDR_ANY, LWIPERF_TCP_PORT_DEFAULT,
                                  report_fn, report_arg);
}

/**
 * @ingroup iperf
 * Start a TCP iperf server on a specific IP address and port and listen for
 * incoming connections from iperf clients.
 *
 * @returns a connection handle that can be used to abort the server
 *          by calling @ref lwiperf_abort()
 */
void *
lwiperf_start_tcp_server(const ip_addr_t *local_addr, u16_t local_port,
                         lwiperf_report_fn report_fn, void *report_arg)
{
  err_t err;
  lwiperf_state_tcp_t *state = NULL;

  err = lwiperf_start_tcp_server_impl(local_addr, local_port, report_fn, report_arg,
    NULL, &state);
  if (err == ERR_OK) {
    return state;
  }
  return NULL;
}

static err_t lwiperf_start_tcp_server_impl(const ip_addr_t *local_addr, u16_t local_port,
                                           lwiperf_report_fn report_fn, void *report_arg,
                                           lwiperf_state_base_t *related_master_state, lwiperf_state_tcp_t **state)
{
  err_t err;
  struct tcp_pcb *pcb;
  lwiperf_state_tcp_t *s;

  LWIP_ASSERT_CORE_LOCKED();

  LWIP_ASSERT("state != NULL", state != NULL);

  if (local_addr == NULL) {
    return ERR_ARG;
  }

  s = (lwiperf_state_tcp_t *)LWIPERF_ALLOC(lwiperf_state_tcp_t);
  if (s == NULL) {
    return ERR_MEM;
  }
  memset(s, 0, sizeof(lwiperf_state_tcp_t));
  s->base.tcp = 1;
  s->base.server = 1;
  s->base.related_master_state = related_master_state;
  s->report_fn = report_fn;
  s->report_arg = report_arg;

  pcb = tcp_new_ip_type(LWIPERF_SERVER_IP_TYPE);
  if (pcb == NULL) {
    return ERR_MEM;
  }
  err = tcp_bind(pcb, local_addr, local_port);
  if (err != ERR_OK) {
    return err;
  }
  s->server_pcb = tcp_listen_with_backlog(pcb, 1);
  if (s->server_pcb == NULL) {
    if (pcb != NULL) {
      tcp_close(pcb);
    }
    LWIPERF_FREE(lwiperf_state_tcp_t, s);
    return ERR_MEM;
  }
  pcb = NULL;

  tcp_arg(s->server_pcb, s);
  tcp_accept(s->server_pcb, lwiperf_tcp_accept);

  lwiperf_list_add(&s->base);
  *state = s;
  return ERR_OK;
}

/**
 * @ingroup iperf
 * Start a TCP iperf client to the default TCP port (5001).
 *
 * @returns a connection handle that can be used to abort the client
 *          by calling @ref lwiperf_abort()
 */
void* lwiperf_start_tcp_client_default(const ip_addr_t* remote_addr,
                               lwiperf_report_fn report_fn, void* report_arg)
{
  return lwiperf_start_tcp_client(remote_addr, LWIPERF_TCP_PORT_DEFAULT, LWIPERF_CLIENT,
                                  report_fn, report_arg,10);
}

/**
 * @ingroup iperf
 * Start a TCP iperf client to a specific IP address and port.
 *
 * @returns a connection handle that can be used to abort the client
 *          by calling @ref lwiperf_abort()
 */
void* lwiperf_start_tcp_client(const ip_addr_t* remote_addr, u16_t remote_port,
  enum lwiperf_client_type type, lwiperf_report_fn report_fn, void* report_arg,u32_t xfer_sec)
{
  err_t ret;
  lwiperf_settings_t settings;
  lwiperf_state_tcp_t *state = NULL;
  u32_t xfer_time = 0;

  memset(&settings, 0, sizeof(settings));
  switch (type) {
  case LWIPERF_CLIENT:
    /* Unidirectional tx only test */
    settings.flags = 0;
    break;
  case LWIPERF_DUAL:
    /* Do a bidirectional test simultaneously */
    settings.flags = htonl(LWIPERF_FLAGS_ANSWER_TEST | LWIPERF_FLAGS_ANSWER_NOW);
    break;
  case LWIPERF_TRADEOFF:
    /* Do a bidirectional test individually */
    settings.flags = htonl(LWIPERF_FLAGS_ANSWER_TEST);
    break;
  default:
    /* invalid argument */
    return NULL;
  }
  settings.num_threads = htonl(1);
  settings.remote_port = htonl(LWIPERF_TCP_PORT_DEFAULT);
  /* TODO: implement passing duration/amount of bytes to transfer */
  xfer_time = xfer_sec * 100;
  settings.amount = htonl((u32_t) - xfer_time);

  ret = lwiperf_tx_start_impl(remote_addr, remote_port, &settings, report_fn, report_arg, NULL, &state);
  if (ret == ERR_OK) {
    LWIP_ASSERT("state != NULL", state != NULL);
    if (type != LWIPERF_CLIENT) {
      /* start corresponding server now */
      lwiperf_state_tcp_t *server = NULL;
      ret = lwiperf_start_tcp_server_impl(&state->conn_pcb->local_ip, LWIPERF_TCP_PORT_DEFAULT,
        report_fn, report_arg, (lwiperf_state_base_t *)state, &server);
      if (ret != ERR_OK) {
        /* starting server failed, abort client */
        lwiperf_abort(state);
        return NULL;
      }
      /* make this server accept one connection only */
      server->specific_remote = 1;
      server->remote_addr = state->conn_pcb->remote_ip;
      if (type == LWIPERF_TRADEOFF) {
        /* tradeoff means that the remote host connects only after the client is done,
           so keep the listen pcb open until the client is done */
        server->client_tradeoff_mode = 1;
      }
    }
    state->xfer_record = 0;
    state->tick_record = 0;
    return state;
  }
  return NULL;
}

/**
 * @ingroup iperf
 * Abort an iperf session (handle returned by lwiperf_start_tcp_server*())
 */
void
lwiperf_abort(void *lwiperf_session)
{
  lwiperf_state_base_t *i, *dealloc, *last = NULL;

  LWIP_ASSERT_CORE_LOCKED();

  for (i = lwiperf_all_connections; i != NULL; ) {
    if ((i == lwiperf_session) || (i->related_master_state == lwiperf_session)) {
      dealloc = i;
      i = i->next;
      if (last != NULL) {
        last->next = i;
      }
      LWIPERF_FREE(lwiperf_state_tcp_t, dealloc); /* @todo: type? */
    } else {
      last = i;
      i = i->next;
    }
  }
}

static void  hgic_lwiperf_report_callback(void *arg, enum lwiperf_report_type report_type,
  const ip_addr_t* local_addr, u16_t local_port, const ip_addr_t* remote_addr, u16_t remote_port,
  u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec)
{
    if (report_type == LWIPERF_TCP_DONE_SERVER) {
        os_printf("TCP Server test done,xfer speed:%dKB/s,use time:%dms,remote IP:"IPSTR"\n",
                  bandwidth_kbitpsec >> 3, ms_duration,
                  ip4_addr1(remote_addr), ip4_addr2(remote_addr),
                  ip4_addr3(remote_addr), ip4_addr4(remote_addr));
        return;
    } else if (report_type == LWIPERF_TCP_DONE_CLIENT) {
        os_printf("TCP Client test done,xfer speed:%dKB/s,use time:%dms,server IP:"IPSTR"\n",
                  bandwidth_kbitpsec >> 3, ms_duration,
                  ip4_addr1(remote_addr), ip4_addr2(remote_addr),
                  ip4_addr3(remote_addr), ip4_addr4(remote_addr));
    } else if (report_type == LWIPERF_TCP_ABORTED_LOCAL_DATAERROR) {
        os_printf("%s:Error,can not connect to peer\n", __FUNCTION__);
    } else if (report_type == LWIPERF_TCP_ABORTED_LOCAL) {
        os_printf("%s:Abort by local!\n", __FUNCTION__);
    } else if (report_type == LWIPERF_TCP_ABORTED_LOCAL_TXERROR) {
        os_printf("%s:Abort by local tx error!\n", __FUNCTION__);
    } else if (report_type == LWIPERF_TCP_ABORTED_REMOTE) {
        os_printf("%s:Error,abort by remote peer!\n", __FUNCTION__);
    } else {
        os_printf("Report type:%d\n", report_type);
    }
}

int sys_lwiperf_tcp_client_start(const char *remote_ipaddr,
                                                unsigned short remote_port,
                                                unsigned int test_sec)
{
    unsigned int ipaddr = 0;
    ip4_addr_t remote_addr;
    unsigned short server_port = 0;
    lwiperf_state_tcp_t *handle = NULL;

    if(remote_ipaddr == NULL) {
        os_printf("%s,%d:Input param error!\n",__FUNCTION__,__LINE__);
        return -EINVAL;
    }

    ipaddr = str2ip((char *)remote_ipaddr);
    ip4_addr_set_u32(&remote_addr, ipaddr);
    server_port = remote_port == 0 ? LWIPERF_TCP_PORT_DEFAULT : remote_port;
    os_printf("%s:connect to IP:"IPSTR",port:%d\n",__FUNCTION__,ip4_addr1(&remote_addr),
        ip4_addr2(&remote_addr),ip4_addr3(&remote_addr),ip4_addr4(&remote_addr),server_port);

    handle = lwiperf_start_tcp_client(&remote_addr, server_port, LWIPERF_CLIENT, 
                        hgic_lwiperf_report_callback,NULL,test_sec < 10 ? 10 : test_sec);
    if(handle == NULL) {
        os_printf("%s,%d:Error!Start tcp client failed!\n",__FUNCTION__,__LINE__);
        return -EINPROGRESS;
    }
    
    return 0;
}
                                                
int sys_lwiperf_tcp_server_start(unsigned short local_port)
{
    unsigned short server_port = 0;    
    lwiperf_state_tcp_t *handle = NULL;
    
    server_port = local_port == 0 ? LWIPERF_TCP_PORT_DEFAULT : local_port;
    handle = lwiperf_start_tcp_server(IP_ADDR_ANY, server_port, 
        hgic_lwiperf_report_callback, NULL);
    if(handle == NULL) {
        os_printf("%s,%d:Error!Start tcp client failed!\n",__FUNCTION__,__LINE__);
        return -EINPROGRESS;
    }
    os_printf("%s,%d:Create TCP server success,port:%d!\n",__FUNCTION__,__LINE__,server_port);        
    return 0;
}

// HUGEIC UDP IPERF2 test code

static uint64_t get_time_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((uint64_t)tv.tv_sec) * 1000000ULL + tv.tv_usec;
}

static uint64_t get_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static void print_stats(uint32_t total_pkt, uint32_t total_bytes, uint32_t lost_pkt, uint32_t duration_ms)
{
    double mbps = (total_bytes * 8.0) / (duration_ms / 1000.0);
    os_printf("  *******************************\n");
    os_printf("  iperf UDP Server finish,result:\n");
    os_printf("  Total packets: %u\n", total_pkt);
    os_printf("  Total bytes: %u\n", total_bytes);
    os_printf("  Estimated lost: %u\n", lost_pkt);
    os_printf("  Lost packet percent:%.2f%%\n",(double)lost_pkt / total_pkt * 100);
    os_printf("  Duration: %.2f s\n", duration_ms / 1000.0);
    os_printf("  Bandwidth: %.2f Mbps\n", mbps/1000000);
    os_printf("  *******************************\n");
}

void lwiperf_udp_client_start(void *arg)
{
    int sock;
    struct sockaddr_in server_addr;
    uint8_t *packet_buf = NULL;
    uint32_t seq_num = 0;
    uint64_t time_start_us, time_now_us, deadline_us;
    uint32_t pkt_payload_len = 0;
    double pkt_interval_us = 0.0;
    double pkt_per_sec     = 0.0;
    uint64_t next_send_time_us = 0;
    uint64_t wtd_feed_time = 0;
    uint32_t seq = 0;
    uint32_t sec = 0;
    uint32_t usec = 0;
    uint64 time = 0;
    int64_t wait_us = 0;
    struct iperf_udp *udp = (struct iperf_udp *)arg;

    if(!udp) {
        os_printf("%s,%d:Input param error!\n",__FUNCTION__,__LINE__);
        return;
    }
    if(udp->remote_ipaddr == NULL) {
        os_printf("%s,%d:Input param error!\n",__FUNCTION__,__LINE__);
        return;
    }
    if (udp->packet_len < IPERF_UDP_HEADER_LEN) {
        os_printf("%s,%d:packet_len too small!\n",__FUNCTION__,__LINE__);
        return;
    }
#if !IP_FRAG
    if(udp->packet_len > TCP_MSS) {
        os_printf("%s,%d:packet_len too big [%d:%d],not support IP_FRAG!\n",
            __FUNCTION__,__LINE__,udp->packet_len,TCP_MSS);
        return;
    }
#endif
    pkt_payload_len = udp->packet_len - IPERF_UDP_HEADER_LEN;

    pkt_per_sec = (double)udp->bandwidth_bps / 8.0 / (double)udp->packet_len;
    pkt_interval_us = 1000000.0 / pkt_per_sec;

    if (pkt_interval_us < 100.0) pkt_interval_us = 100.0; 

    os_printf("Target ip:%s,port:%d,bandwidth:%ubps,total time:%us,packet_len:%u bytes\n",
           udp->remote_ipaddr,udp->port,
           udp->bandwidth_bps, udp->duration_sec, udp->packet_len);

    sock = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        os_printf("socket create fail!\n");
        return;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(udp->port);
    server_addr.sin_addr.s_addr = inet_addr(udp->remote_ipaddr);

    packet_buf = (uint8_t *)os_malloc(udp->packet_len);
    if (!packet_buf) {
        os_printf("malloc failed!\n");
        lwip_close(sock);
        return;
    }
    memset(packet_buf, 0, udp->packet_len);

    time_start_us = get_time_us();
    deadline_us = time_start_us + udp->duration_sec * 1000000ULL;
    next_send_time_us = time_start_us;
    wtd_feed_time  = time_start_us;

    while (1) {
        time_now_us = get_time_us();
        if (time_now_us >= deadline_us) {
            seq = htonl(0x80000000 | seq_num);
            sec = htonl((uint32_t)(time_now_us / 1000000ULL));
            usec = htonl((uint32_t)(time_now_us % 1000000ULL));
            memcpy(packet_buf, &seq, 4);
            memcpy(packet_buf + 4, &sec, 4);
            memcpy(packet_buf + 8, &usec, 4);
            lwip_sendto(sock, packet_buf, udp->packet_len, 0, 
                (struct sockaddr *)&server_addr, sizeof(server_addr));
            break;
        }

        seq = htonl(seq_num);
        sec = htonl((uint32_t)(time_now_us / 1000000ULL));
        usec = htonl((uint32_t)(time_now_us % 1000000ULL));
        memcpy(packet_buf, &seq, 4);
        memcpy(packet_buf + 4, &sec, 4);
        memcpy(packet_buf + 8, &usec, 4);

        lwip_sendto(sock, packet_buf, udp->packet_len, 0, 
            (struct sockaddr *)&server_addr, sizeof(server_addr));
        seq_num++;

        time = get_time_us();
        next_send_time_us += (uint64_t)pkt_interval_us;
        if(next_send_time_us < time) {
            next_send_time_us = time;
        }
        wait_us = next_send_time_us - time;
        if (wait_us < 1000) {
            if(time - wtd_feed_time > 500000) {
                mcu_watchdog_feed();
                wtd_feed_time = time;
            }
        }
        if (wait_us > 1000000 || wait_us < 0) {
            os_printf("%s,%d:Error:wait param error:%lld,next_send_time:%llu\n",
                __FUNCTION__,__LINE__,wait_us,next_send_time_us);
        }
        if (wait_us > 0) {
            os_sleep_us(wait_us); 
        }
    }
    os_free(udp);
    os_free(packet_buf);
    lwip_close(sock);
    os_printf("iperf2 UDP send finished\n");
}

void lwiperf_udp_server_start(void *arg)
{
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int32_t sock;
    int32_t recv_len;
    uint32_t last_seq = 0;
    uint32_t start_time_ms = 0, end_time_ms = 0;
    uint32_t total_pkt = 0, total_bytes = 0, lost_pkt = 0;
    int32_t running = 1;
    uint8_t *buf = os_malloc(UDP_RECV_BUF_LEN);
    uint64_t print_jiff = 0;
    uint32_t diff_jiff = 0;
    uint32_t rx_len = 0;
    uint32_t seq = 0;
    struct iperf_udp *udp = (struct iperf_udp *)arg;
    
    if(!udp) {
        os_printf("%s,%d:Input param error!\n",__FUNCTION__,__LINE__);
        return;
    }
    if (!buf) {
        os_printf("%s,%d:malloc failed!\n",__FUNCTION__,__LINE__);
        return;
    }
    memset(buf, 0, UDP_RECV_BUF_LEN);

    sock = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        printf("socket create fail!\n");
        os_free(buf);
        return;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(udp->port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (lwip_bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        os_printf("%s,%d:bind failed!\n",__FUNCTION__,__LINE__);
        lwip_close(sock);
        os_free(buf);
        return;
    }

    printf("lwIP UDP iperf server started on port %d\n", udp->port);

    while (running) {
        recv_len = lwip_recvfrom(sock, buf, UDP_RECV_BUF_LEN, 0,
                                 (struct sockaddr *)&client_addr, &addr_len);
        if (recv_len <= IPERF_UDP_HEADER_LEN) continue;

        if (total_pkt == 0) {
            os_printf("%s,%d:received start from remote!\n",__FUNCTION__,__LINE__);                        
            start_time_ms = get_time_ms();
            print_jiff = os_jiffies();
        }

        rx_len += recv_len; 
        if (time_after(os_jiffies(), print_jiff + os_msecs_to_jiffies(1000))) {
            diff_jiff  = os_jiffies_to_msecs(os_jiffies() - print_jiff);
            diff_jiff /= 1000;
            if (diff_jiff == 0) {
                diff_jiff = 0xffff;
            }
            os_printf("UDP SERVER: rx:%d KB/s,byte:%d byte\r\n",
                   rx_len / 1024 / diff_jiff,rx_len);
            rx_len = 0;
            print_jiff = os_jiffies();
        }
        
        memcpy(&seq, buf, 4);
        seq = ntohl(seq);

        if (seq & 0x80000000) {
            running = 0;
            seq = seq & 0x7FFFFFFF;            
            os_printf("%s,%d:received end from remote!\n",__FUNCTION__,__LINE__);
        } else {
            if (total_pkt > 0 && seq > last_seq + 1) {
                lost_pkt += (seq - last_seq - 1);
            }            
        }
        last_seq = seq;
        total_pkt++;
        total_bytes += recv_len;
    }

    end_time_ms = get_time_ms();
    print_stats(total_pkt, total_bytes, lost_pkt, end_time_ms - start_time_ms);
    os_free(udp);
    lwip_close(sock);
    os_free(buf);
}


int32_t sys_lwiperf_udp_client_start(const char *remote_ipaddr,
                    uint16_t port,
                    uint32_t duration_sec,
                    uint32_t bandwidth_bps,
                    uint32_t packet_len)
{
    struct iperf_udp *udp = NULL;
    
    udp = os_zalloc(sizeof(struct iperf_udp));
    if(!udp) {
        os_printf("Error,no memory!\n");
        return -EINVAL;
    }

    udp->mode = IPERF_UDP_MODE_CLIENT;
    strcpy(udp->remote_ipaddr,remote_ipaddr);
    udp->port           = port;
    udp->duration_sec   = duration_sec;
    udp->bandwidth_bps  = bandwidth_bps;
    udp->packet_len     = packet_len;

    udp->task_hdl = os_task_create("udp_cli_tsk", lwiperf_udp_client_start, udp, 
        OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
    if(!udp->task_hdl) {
        os_printf("Create udp client task failed!\n");
        return -EFAULT;
    }
    return RET_OK;
}

int32_t sys_lwiperf_udp_server_start(uint16_t port)
{
    struct iperf_udp *udp = NULL;

    udp = os_zalloc(sizeof(struct iperf_udp));
    if(!udp) {
        os_printf("Error,no memory!\n");
        return -EINVAL;
    }

    udp->mode = IPERF_UDP_MODE_SERVER;
    udp->port           = port;

    udp->task_hdl = os_task_create("udp_srv_tsk", lwiperf_udp_server_start, udp, 
        OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
    if(!udp->task_hdl) {
        os_printf("Create udp client task failed!\n");
        return -EFAULT;
    }
    return RET_OK;
}
//hugeic test code

#endif /* LWIP_TCP && LWIP_CALLBACK_API */
