#ifndef __SDK_PROJECT_CONFIG_H__
#define __SDK_PROJECT_CONFIG_H__

#define CUSTOMER_ID 1

/*
 * CUSTOMER_ID :
 *
 * 1 82xCore
 *
*/
#if (CUSTOMER_ID == 1)

//#define PSRAM_HEAP			//如果需要psram当作heap,需要打开这个宏
/*============== WPA3宏定义 ==============*/
#define WIFI_MODULE_80211W_EN   1
#define CONFIG_IEEE80211W
#define CONFIG_SAE
#define CONFIG_OWE
#define WPA_CRYPTO_OPS 1

#endif

#endif
