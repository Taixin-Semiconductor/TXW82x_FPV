#ifndef MQTT_TEST_H
#define MQTT_TEST_H

#define USE_MQTT_EMQX  	1	 //MEQX本地服务器，可以自由创建修改
#define USE_MQTT_Aliyun 0   
#define CMD_TIMEOUT_MS  30000
#define BUF_SIZE 256

#if USE_MQTT_EMQX
#define User_MQTT_Client_Id    "1234"//"gkfxeunUMuu.TestDevice|securemode=2,signmethod=hmacsha256,timestamp=1671767379823|"
#define User_MQTT_UserName     "Test"//"TestDevice&gkfxeunUMuu"	
#define User_MQTT_Password			"12345"//"a5ed28169bbf68b523cd0425b37d519f690b8cbae6a61d2fc818fd11c65fa208"
#define User_TXW80X_MQTTServer_IP     "mqtt://192.168.1.100"//"iot-06z00hveu8ylc8u.mqtt.iothub.aliyuncs.com"
#define User_TXW80X_MQTTServer_PORT   1883

#define User_MQTT_Subscribe_Topic  "Topic1234"//"/sys/gkfxeunUMuu/TestDevice/thing/event/property/post_reply"
#define User_MQTT_Post_Topic  "Topic1234"//"/sys/gkfxeunUMuu/TestDevice/thing/event/property/post"

#elif USE_MQTT_Aliyun
#define User_MQTT_Client_Id    "gkfxeunUMuu.TestDevice|securemode=2,signmethod=hmacsha256,timestamp=1671767379823|"
#define User_MQTT_UserName     "TestDevice&gkfxeunUMuu"	
#define User_MQTT_Password			"a5ed28169bbf68b523cd0425b37d519f690b8cbae6a61d2fc818fd11c65fa208"
#define User_TXW80X_MQTTServer_IP     "mqtt://iot-06z00hveu8ylc8u.mqtt.iothub.aliyuncs.com:1883"
#define User_TXW80X_MQTTServer_PORT   1883

#define User_MQTT_Subscribe_Topic  "/gkfxeunUMuu/TestDevice/user/comc"//"/sys/gkfxeunUMuu/TestDevice/thing/event/property/post_reply"
#define User_MQTT_Post_Topic  "/gkfxeunUMuu/TestDevice/user/comc"//"/sys/gkfxeunUMuu/TestDevice/thing/event/property/post"

#endif
int mqtt_test_init(uint32 ip);
#endif