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
#if HTTPFILE_SERVER
#include "mongoose.h"
#include "httpfile_server_demo.h"
static char s_http_addr[30] = "http://";    // HTTP port
os_task_t     httpfile_server_task;
// HTTP request handler function. It implements the following endpoints:
//   /upload - Saves the next file chunk
//   all other URI - serves web_root/ directory
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  struct mg_http_message *hm = (struct mg_http_message *) ev_data;
  if (ev == MG_EV_HTTP_CHUNK && mg_http_match_uri(hm, "/upload")) {
    //MG_INFO(("Got chunk len %lu , Got query len %lu\r\n", (unsigned long) hm->chunk.len,(unsigned long) hm->query.len));
    //MG_INFO(("Query string: [%.*s]\r\n", (int) hm->query.len, hm->query.ptr));
    mg_file_write(&mg_fs_fat, HTTP_DIR"/e17.txt", hm->chunk.ptr, hm->chunk.len);
     //MG_INFO(("Chunk data: [%.*s]\r\n", (int) hm->chunk.len, hm->chunk.ptr));
    mg_http_delete_chunk(c, hm);
    if (hm->chunk.len == 0) {
      MG_INFO(("Last chunk received, sending response"));
      mg_http_reply(c, 200, "", "ok (chunked)\n");
    }
  } else if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_serve_opts opts = { .fs = &mg_fs_fat, .root_dir = HTTP_DIR"/web_root"};
    mg_http_serve_dir(c, hm, &opts);
  }
}


int httpfile_server_demo(void) {
  struct mg_mgr mgr;                            // Event manager
  int ret;
  mg_log_set(MG_LL_NONE);                      // Set log level
  mg_mgr_init(&mgr);                            // Initialise event manager
  ret = f_mkdir(HTTP_DIR);
  if(ret != 0 && ret != 8) {
    printf("%s set failed\r\n",__FUNCTION__);
    return -1;
  }
  mg_http_listen(&mgr, s_http_addr, fn, NULL);  // Create HTTP listener
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
    strncat(s_http_addr,ip,strlen(ip));
    printf("server_ip: %s\r\n", s_http_addr);

		OS_TASK_INIT("http", &httpfile_server_task, httpfile_server_demo, NULL, OS_TASK_PRIORITY_NORMAL, 30*1024);
	}
	return 0;
}


int httpfile_server_init(uint32 ip)
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
      strncat(s_http_addr,ip,strlen(ip));
      printf("server_ip: %s\r\n", s_http_addr);
		  OS_TASK_INIT("http", &httpfile_server_task, httpfile_server_demo, NULL, OS_TASK_PRIORITY_NORMAL, 30*1024);
	}



	return 0;
}

#endif