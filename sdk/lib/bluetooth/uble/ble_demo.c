/**
  ******************************************************************************
  * @file    sdk\lib\bluetooth\uble
  * @author  TAIXIN-SEMI Application Team
  * @version V2.0.0
  * @date    03-07-2025
  * @brief   This file contains three BLE configuration network features.
  ******************************************************************************
  * @attention
  *
  * COPYRIGHT 2023 TAIXIN-SEMI
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "lib/bluetooth/uble/ble_demo.h"
#include "lib/umac/ieee80211.h"
#include "syscfg.h"
 
#if BLE_SUPPORT

/**
 * @brief   This function is a response callback function that handles ATT requests.
 *
 * @param   entry
 * @param   read    ATT read request
 * @param   buff    ATT write request
 * @param   size    buffer size
 * @param   offset  read/write offset.
 * @return  int32
 */
static int32 uble_test_hdlval(const struct uble_value_entry *entry, uint8 read, uint8 *buff, int32 size, uint32 offset)
{
    char *ptr1, *ptr2;
    uint8 temp = 1;
    if (read) { /*get value*/

        //buff: used to store response data.
        //size: buff's size.

        os_printf("BLE READ: offset = %d\r\n", offset);
        os_memset(buff, 'U', size);

        //return response len.
        return offset == 0 ? size : 4;

    } else { /*set value*/
        dump_hex("BLE WRITE: ", buff, size, 0);
        //os_printf("BLE WRITE: buf=%s, size = %d\r\n", buff, size);
        if (buff[0] == ':') {

            ptr1 = os_strchr(buff, ',');
            if (ptr1 == NULL) { return UBLE_ATT_ERR_REQ_NOT_SUPPORTED; }
            *ptr1++ = 0;
            os_strcpy(sys_cfgs.ssid, buff + 1);
            os_printf("SET ssid:%s\r\n", buff + 1);

            ptr2 = os_strchr(ptr1, ',');
            if (ptr2 == NULL) { return UBLE_ATT_ERR_REQ_NOT_SUPPORTED; }
            *ptr2++ = 0;
            os_strcpy(sys_cfgs.passwd, ptr1);
            os_printf("SET passwd:%s\r\n", ptr1);
            
            if (ptr2[0] == '0') {
                sys_cfgs.key_mgmt = WPA_KEY_MGMT_NONE;
            } else if (ptr2[0] == '1') {
                sys_cfgs.key_mgmt = WPA_KEY_MGMT_PSK;
            } else if (ptr2[0] == '2') {
                sys_cfgs.key_mgmt = WPA_KEY_MGMT_SAE;
            } else if (ptr2[0] == '3') {
                sys_cfgs.key_mgmt = WPA_KEY_MGMT_OWE;
            } else {
                temp = 0;
            }
            
            if (temp) {
                os_printf("SET keymgmt:%c\r\n", ptr2[0]);
            } else {
                os_printf("Unsupported keymgmt:%c\r\n", ptr2[0]);
            }
            
            syscfg_dump();

            SYSEVT_NEW_BLE_EVT(SYSEVT_BLE_NETWORK_CONFIGURED, 0);
        }
        return 0;
    }
}

/**
 * @brief define BLE value table
 */
static const struct uble_value_entry uble_demo_values[] = {
    {.type = UBLE_VALUE_TYPE_HDL, .value = uble_test_hdlval, .size = 0, .bitoff = 0, .maskbit = 0,},
    {.type = UBLE_VALUE_TYPE_STRING, .value = sys_cfgs.ssid, .size = 21, .bitoff = 0, .maskbit = 0,},
    {.type = UBLE_VALUE_TYPE_STRING, .value = sys_cfgs.passwd, .size = 21, .bitoff = 0, .maskbit = 0,},
};

#if BLE_UUID_128
/*360 service*/
#define BLE_360_SERVICE   {0x06,0x05,0x04,0x03,0x02,0x01,0x03,0x00,0x02,0x00,0x01,0x00,0x01,0x00,0x00,0x36}
#define BLE_360_WR_UUID   {0x06,0x05,0x04,0x03,0x02,0x01,0x03,0x00,0x02,0x00,0x01,0x00,0x02,0x00,0x00,0x36}
#define BLE_360_RD_UUID   {0x06,0x05,0x04,0x03,0x02,0x01,0x03,0x00,0x02,0x00,0x01,0x00,0x03,0x00,0x00,0x36}
#define BLE_360_CCCD_UUID {0xfb,0x34,0x9b,0x5f,0x80,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0x02,0x29,0x00,0x00}
#define BLE_360_properties1   UBLE_GATT_CHARAC_WRITE|UBLE_GATT_CHARAC_WRITE_WITHOUT_RESPONSE
#define BLE_360_properties2   UBLE_GATT_CHARAC_READ|UBLE_GATT_CHARAC_NOTIFY
#endif

#define BLE_TUYA_properties1 UBLE_GATT_CHARAC_WRITE_WITHOUT_RESPONSE|UBLE_GATT_CHARAC_READ

/**
 * @brief ATT service
 */
static const struct uble_gatt_data uble_demo_att_table[] = {
    /* Generic Access service ***************************************************/
    { .att_type=0x2800, .properties=0,                     .att_value=0x1800},
    /* Characteristic: *Device Name *********************************************/
    { .att_type=0x2803, .properties=UBLE_GATT_CHARAC_READ, .att_value=0x2A00},
    { .att_type=0x2A00, .properties=0,                     .att_value=0},
    /* Characteristic: Appearance ***********************************************/
    { .att_type=0x2803, .properties=UBLE_GATT_CHARAC_READ, .att_value=0x2A01},
    { .att_type=0x2A01, .properties=0,                     .att_value=0},

#if BLE_UUID_128
    /* 360 service: 360 service  ******************************************************************/
    { .att_type=0x2800,              .properties=0,                   .att_value_128=BLE_360_SERVICE},
    /* Characteristic: app write ******************************************************************/
    { .att_type=0x2803,              .properties=BLE_360_properties1, .att_value_128=BLE_360_WR_UUID},
    { .att_type_128=BLE_360_WR_UUID, .properties=0,                   .att_value=(uint32)&uble_demo_values[0]},
    /* Characteristic: read, notify ****************************************************************/
    { .att_type=0x2803,              .properties=BLE_360_properties2, .att_value_128= BLE_360_RD_UUID},
    { .att_type_128=BLE_360_RD_UUID, .properties=0,                   .att_value=0},
    { .att_type=0x2902,              .properties=0,                   .att_value=0}, /*Characteristic CCCD*/
#else
    /* Tuya service ************************************************************************/
    { .att_type=0x2800, .properties=0,                    .att_value=0x1910},
    /* Characteristic: app write ***********************************************************/
    { .att_type=0x2803, .properties=BLE_TUYA_properties1, .att_value=0x2b11},
    { .att_type=0x2b11, .properties=0,                    .att_value=(uint32)&uble_demo_values[0]},
    /* Characteristic: notify **************************************************************/
    { .att_type=0x2803, .properties=UBLE_GATT_CHARAC_NOTIFY, .att_value=0x2b10},
    { .att_type=0x2b10, .properties=0,                       .att_value=0},
    { .att_type=0x2902, .properties=0,                       .att_value=0}, /*Characteristic CCCD*/
#endif

    /* Characteristic: ssid (read) **************************************************************/
    { .att_type=0x2803, .properties=UBLE_GATT_CHARAC_READ, .att_value=0x2b12},
    { .att_type=0x2b12, .properties=0,                     .att_value=(uint32)&uble_demo_values[1]},
    /* Characteristic: password (read) **********************************************************/
    { .att_type=0x2803, .properties=UBLE_GATT_CHARAC_READ, .att_value=0x2b13},
    { .att_type=0x2b13, .properties=0,                     .att_value=(uint32)&uble_demo_values[2]},
};

/**
 * @brief   This function is used to process data obtained from mode 1(Broadcast Configuration Network).
 *
 * @param   data    buffer
 * @param   len     buffer size
 */
void ble_adv_parse_param(uint8 *data, uint32 len)
{
    uint8 *ptr = data;
    uint8 temp = 1;
    extern struct sys_config sys_cfgs;
#if 1 //sample code
    uint8 buff[33];
    while (ptr < data + len) {
        switch (ptr[0]) {
            case 1: //SSID
                os_memset(buff, 0, sizeof(buff));
                os_memcpy(buff, ptr + 2, ptr[1]);
                os_printf("SET ssid:%s\r\n", buff);
                os_memcpy(sys_cfgs.ssid, buff, sizeof(buff));
                break;
            case 2: //PassWord
                os_memset(buff, 0, sizeof(buff));
                os_memcpy(buff, ptr + 2, ptr[1]);
                os_printf("SET passwd:%s\r\n", buff);
                os_memcpy(sys_cfgs.passwd, buff, sizeof(buff));
                break;
            case 3: //Keymgmt
                os_memset(buff, 0, sizeof(buff));
                os_memcpy(buff, ptr + 2, ptr[1]);
                if (os_strncasecmp(buff, "NONE", 4) == 0) {
                    sys_cfgs.key_mgmt = WPA_KEY_MGMT_NONE;
                } else if (os_strncasecmp(buff, "WPA-PSK", 8) == 0) {
                    sys_cfgs.key_mgmt = WPA_KEY_MGMT_PSK;
                } else if (os_strncasecmp(buff, "SAE", 3) == 0) {
                    sys_cfgs.key_mgmt = WPA_KEY_MGMT_SAE;
                } else if (os_strncasecmp(buff, "OWE", 3) == 0) {
                    sys_cfgs.key_mgmt = WPA_KEY_MGMT_OWE;
                } else {
                    temp = 0;
                }
                if (temp) {
                    os_printf("SET keymgmt:%s\r\n", buff);
                } else {
                    os_printf("Unsupported keymgmt:%s\r\n", buff);
                }
                break;
            case 4: //auth
                os_printf("AUTH %d\r\n", ptr[2]);
                break;
            default:
                os_printf("Unsupport ID:%d\r\n", ptr[0]);
                break;
        }
        ptr += (ptr[1] + 2);
    }

    syscfg_dump();

    SYSEVT_NEW_BLE_EVT(SYSEVT_BLE_NETWORK_CONFIGURED, 0);
#endif
}

int32 ble_demo_init(void)
{
    uint8 scan_resp[] = {0x04, 0x09, 0x53, 0x53, 0x53, 0x19, 0xFF, 0xD0, 0x07, 0x01, 0x03, 0x00, 0x00, 0x0C, 0x00, 0x88, 0xD1, 0xC4, 0x89, 0x2B, 0x56, 0x7D, 0xE5, 0x65, 0xAC, 0xA1, 0x3F, 0x09, 0x1C, 0x43, 0x92};
    uint8 adv_data[] = {0x02, 0x01, 0x06, 0x03, 0x02, 0x01, 0xA2, 0x14, 0x16, 0x01, 0xA2, 0x01, 0x6B, 0x65, 0x79, 0x79, 0x66, 0x67, 0x35, 0x79, 0x33, 0x34, 0x79, 0x71, 0x78, 0x71, 0x67, 0x64};

    uble_init(uble_demo_att_table, ARRAY_SIZE(uble_demo_att_table), 512, (void *)ble_adv_rx_data);
    bt_hci_set_advdata(adv_data, sizeof(adv_data));
    bt_hci_set_scan_rsp(scan_resp, sizeof(scan_resp));
    bt_hci_set_adv_interval(50000);
    bt_hci_set_adv_en(1);
    bt_hci_set_ll_length(251);
    return RET_OK;
}

/**
 *  @brief  Startup or shutdown mode.
 *  @param  mode
 *      0   close
 *      1   mode1(广播配网)
 *      2   mode2(可扫描配网)
 *      3   mode3(BLE协议配网)
 *  @param  chan    37/38/39
 */
int32 ble_set_mode(uint8 mode, uint8 chan)
{
    switch (mode) {
        case 0:
            ble_adv_ctrl_destory();
            break;
        case 1:
        case 2:
            ble_adv_ctrl_create();
            break;
        case 3:
            break;
        default:
            os_printf(KERN_ERR"Unsupported mode = %d\r\n", mode);
            return -ENOTSUPP;
    }
    return bt_hci_set_mode(mode, chan);
}

int32 ble_set_coexist_en(uint8 coexist, uint8 dec_duty)
{
    return bt_hci_set_coexist_en(coexist, dec_duty);
}

#endif

/*************************** (C) COPYRIGHT 2023 TAIXIN-SEMI ***** END OF FILE *****/

