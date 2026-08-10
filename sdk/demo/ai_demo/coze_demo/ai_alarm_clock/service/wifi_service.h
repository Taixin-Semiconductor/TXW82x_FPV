#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H
 
#include <stdint.h>
#include <stdbool.h>
#include "osal/timer.h"
#include "../../sdk/include/osal/string.h"
#include "../../sdk/include/lib/umac/ieee80211.h"

#define WIFI_ITEM_MAX_COUNT 20

typedef enum
{
    WIFI_STATUS_IDLE = 0,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_FAILED
} wifi_status_t;

typedef struct
{
    bool scan_done;
    uint8_t count;
    int8_t connect_status;
    struct hgic_bss_info bss_info[WIFI_ITEM_MAX_COUNT];
} wifi_info_t;

void wifi_service_init(void);
void wifi_service_scan(void);
void wifi_service_info_update(void);
uint8_t wifi_service_get_count(void);
void wifi_service_set_scan_stauts(bool sta);
bool wifi_service_get_scan_status(void);
void wifi_service_connect(const char *ssid, const char *password);
int8_t wifi_service_get_connect_status(void);
wifi_info_t *wifi_service_get_wifi_info(void);
void wifi_service_set_connect_status(uint8_t sta);

#endif
