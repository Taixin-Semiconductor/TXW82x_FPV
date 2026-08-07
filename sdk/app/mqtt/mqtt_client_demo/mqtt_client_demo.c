// Copyright (c) 2020 Cesanta Software Limited
// All rights reserved
//
// Example MQTT client. It performs the following steps:
//    1. Connects to the Aliyun IoT MQTT server
//    2. When connected, subscribes to the topic `s_sub_topic`
//    3. Publishes message `hello` to the `s_post_topic` periodically
//
// This example requires TLS support. By default, it is built with mbedTLS,
// therefore make sure mbedTLS is installed. To build with OpenSSL, execute:
//    make clean all CFLAGS="-W -Wall -DMG_ENABLE_OPENSSL=1 -lssl"

// In order to get MQTT URL, login to Aliyun IoT, click on "Settings" on the left
// bar, copy the "Endpoint" URL.
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
#if MQTT_CLIENT
#include "mongoose.h"
#include "mqtt_client_demo.h"

static const char *s_url =
        User_TXW80X_MQTTServer_IP;
//    "mqtts://a1pjwh2bop1ojt-ats.iot.eu-west-1.amazonaws.com";

// To create certificates:
// 1. Click Policies -> Create, fill fields:
//               Name        : Policy1
//               Action      : iot:*
//               Resource ARN: *
//               Effect      : allow
//        then, click "Create"
// 2. Click Manage -> Things -> Create things -> Create single thing -> Next
//      Thing name: t1, no shadow, Next
//      Auto-generate new certificate, Next
//      Select policy Policy1, Create thing
// 3. From the dialog box that appears, download:
//      xxx-certificate.pem.crt as cert.pem to the example directory
//      xxx-private.pem.key as key.pem to the example directory
static const char *s_cert = "cert.pem";
static const char *s_key = "key.pem";

static const char *s_sub_topic = User_MQTT_Subscribe_Topic;
static const char *s_post_topic = User_MQTT_Post_Topic;
static int s_qos = 1;

os_task_t     mqtt_task;

static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    //_os_printf("%s\r\n",__FUNCTION__);
  if (ev == MG_EV_OPEN) {
    // c->is_hexdumping = 1;
  } else if (ev == MG_EV_ERROR) {
    // On error, log error message
    MG_ERROR(("error!!!!%p %s", c->fd, (char *) ev_data));
  } else if (ev == MG_EV_CONNECT) {
    if (mg_url_is_ssl(s_url)) {
      struct mg_tls_opts opts = {.ca = "ca.pem"};
      mg_tls_init(c, &opts);
    }
  } else if (ev == MG_EV_MQTT_OPEN) {
    // MQTT connect is successful
    struct mg_str topic = mg_str(s_sub_topic);
    MG_INFO(("Connected to %s", s_url));
    MG_INFO(("Subscribing to %s", s_sub_topic));
    mg_mqtt_sub(c, topic, s_qos);
    c->data[0] = 'X';  // Set a label that we're logged in
  } else if (ev == MG_EV_MQTT_MSG) {
    // When we receive MQTT message, print it
    struct mg_mqtt_message *mm = (struct mg_mqtt_message *) ev_data;
    MG_INFO(("Received on %.*s : %.*s", (int) mm->topic.len, mm->topic.ptr,
             (int) mm->data.len, mm->data.ptr));
  } else if (ev == MG_EV_POLL && c->data[0] == 'X') {
    static unsigned long prev_second;
    unsigned long now_second = (*(unsigned long *) ev_data) / 1000;
    if (now_second != prev_second) {
      struct mg_str topic = mg_str(s_post_topic), data = mg_str("{\"a\":123}");
      MG_INFO(("Publishing to %s", s_post_topic));
      mg_mqtt_pub(c, topic, data, s_qos, false);
      prev_second = now_second;
    }
  }

  if (ev == MG_EV_ERROR || ev == MG_EV_CLOSE) {
    MG_INFO(("Got event %d, stopping...", ev));
    *(bool *) fn_data = true;  // Signal that we're done
  }
}

int mqtt_demo(void) {
  struct mg_mgr mgr;
  struct mg_mqtt_opts opts = {
        .user = mg_str(User_MQTT_UserName) ,
        .pass = mg_str(User_MQTT_Password),
        .client_id = mg_str(User_MQTT_Client_Id),
        .will_topic = mg_str(s_sub_topic),
        .will_message = mg_str("i'm offline"),
        .will_qos = (uint8_t)s_qos,
        .version = 4,
        .keepalive = 60,
        .will_retain = 0,
        .clean = 1,
        };
  bool done = false;
  mg_log_set(MG_LL_DEBUG); 
  mg_mgr_init(&mgr);                               // Initialise event manager
  MG_INFO(("Connecting to %s", s_url));            // Inform that we're starting
  mg_mqtt_connect(&mgr, s_url, &opts, fn, &done);  // Create client connection
  while (!done) mg_mgr_poll(&mgr, 5000);           // Loop until done
  mg_mgr_free(&mgr);                               // Finished, cleanup
  return 0;
}

static sysevt_hdl_res dhcp_client_get_ip(uint32 event_id, uint32 data, uint32 priv)
{
	static uint8_t is_get_ip = 0; 
	_os_printf("%s:%d!!!!!!!!!!!\n",__FUNCTION__,__LINE__);
	if(!is_get_ip)
	{
		is_get_ip = 1;
		
		OS_TASK_INIT("mqtt", &mqtt_task, mqtt_demo, NULL, OS_TASK_PRIORITY_NORMAL, 8192);
	}
	return 0;
}


int mqtt_test_init(uint32 ip)
{

	_os_printf("%s:%d\tip:%X\n",__FUNCTION__,__LINE__,ip);

	sys_event_take(SYS_EVENT(SYS_EVENT_WIFI, SYSEVT_WIFI_STA_CONNECTTED),dhcp_client_get_ip,0);
	//if(!ip)
	//{
	//	sys_event_take(SYS_EVENT(SYS_EVENT_NETWORK, SYSEVT_LWIP_DHCPC_DONE),dhcp_client_get_ip,0);
	//}
	//else
	//{
	//	OS_TASK_INIT("mqtt", &mqtt_task, mqtt_demo, NULL, OS_TASK_PRIORITY_NORMAL, 8192);
	//}



	return 0;
}

#endif
