#include "basic_include.h"
#include "wifi_service.h"
#include "sys_config.h"
#include <string.h>
#include "syscfg.h"
#include "devid.h"
 
#define WIFI_SCAN_DEFAULT_CHANNEL 1

extern struct sys_config sys_cfgs;

static wifi_info_t g_wifi_infos;

void wifi_service_init(void)
{
    memset(&g_wifi_infos, 0, sizeof(g_wifi_infos));
    g_wifi_infos.count = 0;
    g_wifi_infos.scan_done = false;
    if (sys_cfgs.ssid != NULL && sys_status.wifi_status_code == 16)
    {
        g_wifi_infos.connect_status = WIFI_STATUS_CONNECTING;
    }
    else
    {
        g_wifi_infos.connect_status = sys_status.wifi_connected ? WIFI_STATUS_CONNECTED : WIFI_STATUS_IDLE;
    }
}

void wifi_service_scan(void)
{
    g_wifi_infos.scan_done = false;
    g_wifi_infos.count = 0;
    ieee80211_scan(sys_cfgs.wifi_mode, WIFI_SCAN_DEFAULT_CHANNEL, NULL);
}

void wifi_service_set_scan_stauts(bool sta)
{
    g_wifi_infos.scan_done = sta;
}

static void wifi_set_info(struct hgic_bss_info *bss_info, int32_t cnt)
{
    if (bss_info == NULL || cnt == 0)
    {
        memset(g_wifi_infos.bss_info, 0, sizeof(g_wifi_infos.bss_info));
        g_wifi_infos.count = 0;
        return;
    }
    if (cnt > WIFI_ITEM_MAX_COUNT)
    {
        cnt = WIFI_ITEM_MAX_COUNT;
    }

    memset(g_wifi_infos.bss_info, 0, sizeof(g_wifi_infos.bss_info));
    memcpy(g_wifi_infos.bss_info, bss_info, sizeof(struct hgic_bss_info) * cnt);
    g_wifi_infos.count = cnt;
}

void wifi_service_info_update(void)
{
    struct hgic_bss_info bsslistp[WIFI_ITEM_MAX_COUNT];
    int32_t cnt = ieee80211_get_bsslist(bsslistp, WIFI_ITEM_MAX_COUNT, 0);
    wifi_set_info(bsslistp, cnt);
}

uint8_t wifi_service_get_count(void)
{
    return g_wifi_infos.count;
}

bool wifi_service_get_scan_status(void)
{
    return g_wifi_infos.scan_done;
}

static int32_t wifi_set_ssid(const char *ssid)
{
    uint8_t ifidx = (sys_cfgs.wifi_mode == WIFI_MODE_APSTA)
                        ? WIFI_MODE_STA
                        : sys_cfgs.wifi_mode;

    if (!ssid)
        return -1;

    os_memset(sys_cfgs.bssid, 0, 6);

    os_strncpy(sys_cfgs.ssid, ssid, SSID_MAX_LEN);
    sys_cfgs.ssid[SSID_MAX_LEN - 1] = '\0';

    ieee80211_conf_set_bssid(ifidx, NULL);

    if (os_strlen(sys_cfgs.passwd) > 0)
    {
        wpa_passphrase(sys_cfgs.ssid,
                       (char *)sys_cfgs.passwd,
                       sys_cfgs.psk);
    }

    ieee80211_conf_set_ssid(ifidx, sys_cfgs.ssid);
    ieee80211_conf_set_psk(ifidx, sys_cfgs.psk);

    os_printf("set new ssid: %s\r\n", sys_cfgs.ssid);

    return 0;
}

static int32_t wifi_set_password(const char *password)
{
    uint8_t ifidx = (sys_cfgs.wifi_mode == WIFI_MODE_APSTA)
                        ? WIFI_MODE_STA
                        : sys_cfgs.wifi_mode;

    if (!password)
        return -1;

    if (os_strlen(password) < 8)
    {
        os_printf("psk need at least 8 bytes\r\n");
        return -1;
    }

    os_strncpy(sys_cfgs.passwd, password, PASSWD_MAX_LEN);
    sys_cfgs.passwd[PASSWD_MAX_LEN - 1] = '\0';

    if (os_strlen(sys_cfgs.ssid) > 0)
    {
        wpa_passphrase(sys_cfgs.ssid,
                       (char *)sys_cfgs.passwd,
                       sys_cfgs.psk);
    }

    ieee80211_conf_set_psk(ifidx, sys_cfgs.psk);
    ieee80211_conf_set_passwd(ifidx, (char *)sys_cfgs.passwd);

    os_printf("set new key: %s\r\n",password);

    return 0;
}

void wifi_service_connect(const char *ssid, const char *password)
{
    os_printf("Connecting to WiFi SSID: %s, Password: %s\r\n",
              ssid ? ssid : "(null)",
              (password && password[0] != '\0') ? password : "(none)");
    wifi_set_ssid(ssid);
    wifi_set_password(password);
}

int8_t wifi_service_get_connect_status(void)
{
    return g_wifi_infos.connect_status;
}

wifi_info_t *wifi_service_get_wifi_info(void)
{
    return &g_wifi_infos;
}

void wifi_service_set_connect_status(uint8_t sta)
{
    g_wifi_infos.connect_status = sta;
}