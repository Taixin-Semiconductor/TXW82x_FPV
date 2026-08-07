/*
 * @Author: jiejie
 * @Github: https://github.com/jiejieTop
 * @Date: 2019-12-11 21:53:07
 * @LastEditTime : 2022-06-15 23:03:30
 * @Description: the code belongs to jiejie, please keep the author information and source code according to the license.
 */
#include "basic_include.h"

#include "mqtt_config.h"
#include "mqtt_log.h"
#include "mqttclient.h"
#include "syscfg.h"

// #include "ca.inc"
#define KAWAII_EMQX_TEST 1
#if KAWAII_EMQX_TEST
// #define TEST_USEING_TLS  

static void topic1_handler(void* client, message_data_t* msg)
{
    (void) client;
    MQTT_LOG_I("-----------------------------------------------------------------------------------");
    MQTT_LOG_I("%s:%d %s()...\ntopic: %s\nmessage:%s", __FILE__, __LINE__, __FUNCTION__, msg->topic_name, (char*)msg->message->payload);
    MQTT_LOG_I("-----------------------------------------------------------------------------------");
}

int kawaii_emqx_test_demo(void)
{
    mqtt_client_t *client = NULL;
    char client_id[32];
    char user_name[32];
    char password[32];
    char buf[100] = { 0 };
    mqtt_message_t msg;

    os_printf("\nwelcome to mqttclient test...\n");
	os_printf("Waiting for network connection...");
    while (!sys_status.wifi_connected || !sys_status.dhcpc_done) {
        _os_printf(".");
        os_sleep(1);
    }
    os_printf("Network connected!\r\n");

    random_string(client_id, 10);
    random_string(user_name, 10);
    random_string(password, 10);

    mqtt_log_init();

    client = mqtt_lease();

#ifdef TEST_USEING_TLS
    mqtt_set_port(client, "8883");
    mqtt_set_ca(client, (char*)test_ca_get());
#else
    mqtt_set_port(client, "1883");
#endif

    mqtt_set_host(client, "broker.emqx.io");
    mqtt_set_client_id(client, client_id);
    mqtt_set_user_name(client, user_name);
    mqtt_set_password(client, password);
    mqtt_set_clean_session(client, 1);

    mqtt_connect(client);
    
    mqtt_subscribe(client, "topic1", QOS0, topic1_handler);
    mqtt_subscribe(client, "topic2", QOS1, NULL);
    mqtt_subscribe(client, "topic3", QOS2, NULL);

    os_memset(&msg, 0, sizeof(msg));
    os_sprintf(buf, "welcome to mqttclient, this is a publish test...");

    os_sleep(2);

    mqtt_list_subscribe_topic(client, NULL);

    msg.payload = (void *) buf;
    
    while(1) {
        os_sprintf(buf, "welcome to mqttclient, this is a publish test, a rand number: %d ...", random_number());

        msg.qos = 0;
        mqtt_publish(client, "topic1", &msg);

        msg.qos = 1;
        mqtt_publish(client, "topic2", &msg);

        msg.qos = 2;
        mqtt_publish(client, "topic3", &msg);
        
        os_sleep(4);
    }
}

struct os_task kawaii_emqx_test_task;
void kawaii_emqx_test(void)
{
    OS_TASK_INIT("KAWAII_EMQX_TEST", &kawaii_emqx_test_task, kawaii_emqx_test_demo, NULL, OS_TASK_PRIORITY_NORMAL, NULL, 1024);
}
#endif
