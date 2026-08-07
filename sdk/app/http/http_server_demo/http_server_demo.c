// Copyright (c) 2020 Cesanta Software Limited
// All rights reserved
//
// HTTP server example. This server serves both static and dynamic content.
// It opens two ports: plain HTTP on port 8000 and HTTP on port 8443.
// It implements the following endpoints:
//    /api/stats - respond with free-formatted stats on current connections
//    /api/f2/:id - wildcard example, respond with JSON string {"result": "URI"}
//    any other URI serves static files from s_root_dir
//
// To enable SSL/TLS (using self-signed certificates in PEM files),
//    1. make SSL=OPENSSL or make SSL=MBEDTLS
//    2. curl -k https://127.0.0.1:8443
#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/string.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "osal/work.h"
#include "osal/sleep.h"
#include "osal/timer.h"
#include "hal/gpio.h"
#include "lib/common/common.h"
#include "lib/common/sysevt.h"
#include "lib/syscfg/syscfg.h"
#include "lwip/pbuf.h"
#include "netif/ethernetif.h"
#include <string.h>
#if HTTP_SERVER || HTTPS_SERVER
#include "mongoose.h"
#include "http_server_demo.h"
#include "certs.h"
static char s_http_addr[30] = "http://";    // HTTP port
static char s_https_addr[30] = "https://";  // HTTPS port
static const char *s_root_dir = ".";
os_task_t     http_server_task;
// We use the same event handler function for HTTP and HTTPS connections
// fn_data is NULL for plain HTTP, and non-NULL for HTTPS
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_ACCEPT && fn_data != NULL) {
    printf("ev == MG_EV_ACCEPT && fn_data != NULL\r\n");
    struct mg_tls_opts opts = {  
        //.ca = s_ca,//双向验证，一般不用
        .cert = s_ssl_cert, 
        .certkey = s_ssl_key
        };
    mg_tls_init(c, &opts);
  } else if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    MG_INFO(("%.*s %.*s %ld", (int) hm->method.len, hm->method.ptr,
             (int) hm->uri.len, hm->uri.ptr, (long) hm->body.len));
    if (mg_http_match_uri(hm, "/api/stats")) {
      // Print some statistics about currently established connections
      mg_printf(c, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
      mg_http_printf_chunk(c, "ID PROTO TYPE      LOCAL           REMOTE\n");
      for (struct mg_connection *t = c->mgr->conns; t != NULL; t = t->next) {
         (c, "%-3lu %4s %s %I %I\n", t->id,
                             t->is_udp ? "UDP" : "TCP",
                             t->is_listening  ? "LISTENING"
                             : t->is_accepted ? "ACCEPTED "
                                              : "CONNECTED",
                             4, &t->loc.ip, 4, &t->rem.ip);
      }
      mg_http_printf_chunk(c, "");  // Don't forget the last empty chunk
    } else if (mg_http_match_uri(hm, "/api/f2/*")) {
      mg_http_reply(c, 200, "", "{\"result\": \"%.*s\"}\n", (int) hm->uri.len,
                    hm->uri.ptr);
    } else {
      struct mg_http_serve_opts opts = {.root_dir = s_root_dir};
      
      mg_http_serve_dir(c, ev_data, &opts);
    }
  }
  (void) fn_data;
}

int http_server_demo(void) {
  struct mg_mgr mgr;                            // Event manager
  mg_log_set(MG_LL_DEBUG);                      // Set log level
  mg_mgr_init(&mgr);                            // Initialise event manager
#if HTTP_SERVER
  mg_http_listen(&mgr, s_http_addr, fn, NULL);  // Create HTTP listener
#elif HTTPS_SERVER
  mg_http_listen(&mgr, s_https_addr, fn, (void *) 1);  // HTTPS listener
#endif
  for (;;) mg_mgr_poll(&mgr, 1000);                    // Infinite event loop
  mg_mgr_free(&mgr);
  return 0;
}

static sysevt_hdl_res dhcp_client_get_ip(uint32 event_id, uint32 data, uint32 priv)
{
	static uint8_t is_get_ip = 0; 
	printf("%s:%d!!!!!!!!!!!\n",__FUNCTION__,__LINE__);
	if(!is_get_ip)
	{
      ip_addr_t ipaddr;
      char *ip;
		  is_get_ip = 1;
      ipaddr = lwip_netif_get_ip2("w0");
      ip = inet_ntoa(ipaddr);
#if HTTP_SERVER
    strncat(s_http_addr,ip,strlen(ip));
    printf("server_ip: %s\r\n", s_http_addr);
#elif HTTPS_SERVER
    strncat(s_https_addr,ip,strlen(ip));
    printf("server_ip: %s\r\n", s_https_addr);
#endif

		OS_TASK_INIT("http", &http_server_task, http_server_demo, NULL, OS_TASK_PRIORITY_NORMAL, 20*1024);
	}
	return 0;
}


int http_server_init(uint32 ip)
{

	printf("%s:%d\tip:%X\n",__FUNCTION__,__LINE__,ip);

	
	if(!ip)
	{
		sys_event_take(SYS_EVENT(SYS_EVENT_NETWORK, SYSEVT_LWIP_DHCPC_DONE),dhcp_client_get_ip,0);
	}
	else
	{
      ip_addr_t ipaddr;
      char *ip;
      ipaddr = lwip_netif_get_ip2("w0");
      ip = inet_ntoa(ipaddr);
#if HTTP_SERVER
    strncat(s_http_addr,ip,strlen(ip));
    printf("server_ip: %s\r\n", s_http_addr);
#elif HTTPS_SERVER
    strncat(s_https_addr,ip,strlen(ip));
    printf("server_ip: %s\r\n", s_https_addr);
#endif
		OS_TASK_INIT("http", &http_server_task, http_server_demo, NULL, OS_TASK_PRIORITY_NORMAL, 20*1024);
	}



	return 0;
}

#endif