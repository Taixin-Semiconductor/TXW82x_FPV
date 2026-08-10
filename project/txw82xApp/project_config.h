#ifndef __SDK_PROJECT_CONFIG_H__
#define __SDK_PROJECT_CONFIG_H__


#define CUSTOMER_ID 5

/*
 * CUSTOMER_ID :
 *
 * 1 82xApp_AI_VOICE_Demo 
 * 2 82xApp_AI_VISION_Demo
 * 3 82xApp_AI_Alarm_Clock_Demo
 * 4 82xApp_ISP_tunning_Demo
 * 5 82xApp_720P_Demo
 * 6 82xApp_LCD_720P_Demo
 * 7 82xApp_720P_Sleep_Demo
 * 8 82xApp_1080P_Demo
 * 9 82xApp_Battery_Camera_1080P_Demo
*/

#if (CUSTOMER_ID == 1)
#include "coze_demo/ai_dialogue/voice_config.h"
//#include "neteast_demo/ai_dialogue/voice_config.h"

#elif (CUSTOMER_ID == 2)
#include "coze_demo/ai_dialogue/vision_config.h"
//#include "neteast_demo/ai_dialogue/voice_config.h"

#elif (CUSTOMER_ID == 3)
#include "coze_demo/ai_alarm_clock/ai_alarm_clock_config.h"

#elif (CUSTOMER_ID == 4)
#include "isp_tuning_demo/isp_tuning_config.h"

#elif (CUSTOMER_ID == 5)
#include "ipc_720p_config.h"

#elif (CUSTOMER_ID == 6)
#include "lcd_720p_config.h"

#elif (CUSTOMER_ID == 7)
#include "ipc_720p_sleep_config.h"

#elif (CUSTOMER_ID == 8)
#include "ipc_1080p_config.h"

#elif (CUSTOMER_ID == 9)
#include "battery_camera_1080p_config.h"


#endif

#endif
