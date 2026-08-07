// Copyright (c) 2021 Cesanta Software Limited
// All rights reserved
//
// Example HTTP client. Connect to `s_url`, send request, wait for a response,
// print the response and exit.
// You can change `s_url` from the command line by executing: ./example YOUR_URL
//
// To enable SSL/TLS, make SSL=OPENSSL or make SSL=MBEDTLS

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
#include <string.h>
#if HTTPS_CLIENT || HTTP_CLIENT
#include "mongoose.h"
#include "http_client_demo.h"
#include "certs.h"
// The very first web page in history. You can replace it from command line
#if HTTPS_CLIENT
  static const char *s_url = "https://example.org/";//AP模式下无法透传上网
#elif HTTP_CLIENT
  static const char *s_url = "http://192.168.1.100";
#else
  static const char *s_url = "";
#endif
static const char *s_post_data = NULL;      // POST data
static const uint64_t s_timeout_ms = 1500;  // Connect timeout in milliseconds

static struct mg_str *get_header_value(struct mg_http_message *hm,const char *name);

// Print HTTP response and signal that we're done
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_OPEN) {
    // Connection created. Store connect expiration time in c->data
    *(uint64_t *) c->data = mg_millis() + s_timeout_ms;
  } else if (ev == MG_EV_POLL) {
    if (mg_millis() > *(uint64_t *) c->data &&
        (c->is_connecting || c->is_resolving)) {
      mg_error(c, "Connect timeout");
    }
  } else if (ev == MG_EV_CONNECT) {
    // Connected to server. Extract host name from URL
    struct mg_str host = mg_url_host(s_url);

    // If s_url is https://, tell client connection to use TLS
    if (mg_url_is_ssl(s_url)) {
      struct mg_tls_opts opts = {
        //.ca = s_ca,
        .srvname = host
       };
      mg_tls_init(c, &opts);
    }

    // Send request
    int content_length = s_post_data ? strlen(s_post_data) : 0;
    mg_printf(c,
              "%s %s HTTP/1.0\r\n"
              "Host: %.*s\r\n"
              "Content-Type: octet-stream\r\n"
              "Content-Length: %d\r\n"
              "\r\n",
              s_post_data ? "POST" : "GET", mg_url_uri(s_url), (int) host.len,
              host.ptr, content_length);
    mg_send(c, s_post_data, content_length);
  }  else if (ev == MG_EV_HTTP_MSG) {
    // Response is received. Print it
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
	  get_header_value(hm,"Content-Type");
    MG_INFO(("%.*s", (int) hm->message.len, hm->message.ptr));
    c->is_closing = 1;         // Tell mongoose to close this connection
    *(bool *) fn_data = true;  // Tell event loop to stop
  } else if (ev == MG_EV_ERROR) {
    *(bool *) fn_data = true;  // Error, tell event loop to stop
  }
}

os_task_t     http_client_task;

int http_client_demo(void *arg) {

  struct mg_mgr mgr;              // Event manager
  bool done = false;              // Event handler flips it to true
  mg_log_set(MG_LL_DEBUG);    // Set to 0 to disable debug
  mg_mgr_init(&mgr);              // Initialise event manager
  
  mg_http_connect(&mgr, s_url, fn, &done);  // Create client connection
  while (!done) mg_mgr_poll(&mgr, 50);      // Event manager loops until 'done'
  mg_mgr_free(&mgr);                        // Free resources
  return 0;
}

static sysevt_hdl_res dhcp_client_get_ip(uint32 event_id, uint32 data, uint32 priv)
{
	static uint8_t is_get_ip = 0; 
	printf("%s:%d!!!!!!!!!!!\n",__FUNCTION__,__LINE__);
	if(!is_get_ip)
	{
	  is_get_ip = 1;
		OS_TASK_INIT("http_client_task", &http_client_task, http_client_demo, NULL, OS_TASK_PRIORITY_NORMAL, 20*1024);
	}
	return 0;
}


int http_client_init(uint32 ip)
{

	printf("%s:%d\tip:%s\n",__FUNCTION__,__LINE__,inet_ntoa(ip));

  sys_event_take(SYS_EVENT(SYS_EVENT_WIFI, SYSEVT_WIFI_STA_CONNECTTED),dhcp_client_get_ip,0);
	//if(!ip)
	//{
	//	sys_event_take(SYS_EVENT(SYS_EVENT_NETWORK, SYSEVT_LWIP_DHCPC_DONE),dhcp_client_get_ip,0);
	//}
	//else
	//{
	//	OS_TASK_INIT("http_client_task", &http_client_task, http_client_demo, NULL, OS_TASK_PRIORITY_NORMAL, 10*1024);
	//}



	return 0;
}

static struct mg_str *get_header_value(struct mg_http_message *hm,const char *name) {
  if (name == NULL)
    return NULL;
  struct mg_str *cl;
  if ((cl = mg_http_get_header(hm, name)) != NULL) {
    printf("%s--get %s  %.*s\r\n", __FILE__, name, (int) cl->len, cl->ptr);
    return cl;
  } else {
    printf("%s--can not get %s  %.*s\r\n", __FILE__, name);
    return NULL;
  }
}

#endif


