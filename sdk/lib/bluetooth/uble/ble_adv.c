#include "lib/bluetooth/hci/hci_host.h"
#include "lib/bluetooth/uble/ble_adv.h"

struct ble_rx_ctrl *ble_ctrl;
const uint8 adv_identify_info[ADV_IDENTIFY_SET_LEN] = {0x9f, 0x1a, 0x41, 0x10, 0x7d, 0xfd, 0xf0, 0x73}; // 识别头信息
const uint8 adv_addr[6] = {0x70, 0xaf, 0x1e, 0x7f, 0x0e, 0x7e}; // 广播地址

extern void ble_adv_parse_param(uint8 *data, uint32 len);
static int32 ble_adv_parse_data(uint8 *data, int32 len)
{
	if(len <= ADV_DATA_MAX_LEN) {
		memcpy(&ble_ctrl->adv_info, data, len);
	}else {
		return 0;
	}
	if(ble_ctrl->adv_info.header_info.pdu_type != ADV_DISCOVER_TYPE) {   
		return 0;
	} 
	if(ble_ctrl->adv_info.payload_info.manufacturer_id != ADV_MANUFACTURER_ID) { 
		return 0;
	}
	#if (ADV_IDENTIFY_SET_LEN > 0 && ADV_IDENTIFY_SET_LEN < ADV_IDENTIFY_MAX_LEN)
	if(memcmp(ble_ctrl->adv_info.payload_info.identify_info, adv_identify_info, ADV_IDENTIFY_SET_LEN) != 0) {
		return 0;
	}
	#endif

    //dump_hex("ble_adv_parse_data adv_data:\r\n", data, len, 1);
	//接收超时
	if(os_jiffies_to_msecs(os_jiffies())-ble_ctrl->adv_pro.cur_tick >= ble_ctrl->adv_pro.rec_overtime) {
		if(ble_ctrl->adv_info.payload_info.section_idx == 0) {
			ble_ctrl->adv_pro.cur_tick = os_jiffies_to_msecs(os_jiffies());
			ble_ctrl->adv_pro.section_num = ble_ctrl->adv_info.payload_info.section_num;
			ble_ctrl->adv_pro.rec_overtime = (ble_ctrl->adv_pro.section_num+1)*1000; //ms
			ble_ctrl->adv_pro.section_idx = 0;
			ble_ctrl->adv_pro.byte_offset_len = 0;
			ble_ctrl->adv_pro.len = 0;
			ble_ctrl->adv_pro.start_flag = 1;
		}else {
			ble_ctrl->adv_pro.start_flag = 0;
            os_printf(KERN_ERR"%s: adv recv timeout!\r\n", __func__);
		}
	}
	if((ble_ctrl->adv_pro.start_flag == 1) && (ble_ctrl->adv_info.payload_info.section_num == ble_ctrl->adv_pro.section_num))   {
		if(ble_ctrl->adv_info.payload_info.section_idx == ble_ctrl->adv_pro.section_idx) {
			memcpy(ble_ctrl->adv_pro.data + ble_ctrl->adv_pro.byte_offset_len, ble_ctrl->adv_info.payload_info.data, ble_ctrl->adv_info.payload_info.byte_len);
			ble_ctrl->adv_pro.byte_offset_len += ble_ctrl->adv_info.payload_info.byte_len;
			++ble_ctrl->adv_pro.section_idx;
		}
		if(ble_ctrl->adv_info.payload_info.section_num == ble_ctrl->adv_pro.section_idx) {
			ble_ctrl->adv_pro.len = ble_ctrl->adv_pro.byte_offset_len;
			ble_ctrl->adv_pro.start_flag = 0;
			return 1;
		}
	}
	return 0;
}

static int32 ble_adv_get_data(uint8 **data)
{
	if(ble_ctrl->adv_pro.data == NULL) {
		return 0;
	}else {
		*data = ble_ctrl->adv_pro.data;
		return ble_ctrl->adv_pro.len;
	}
}

int32 ble_adv_rx_data(uint8 *data, uint32 len)
{
	uint8 *ncdata = NULL;
    uint32 data_len = 0;

    //dump_hex("ble adv rx adv_data:\r\n", data, len, 1);
	if (ble_adv_parse_data(data, len)) {
   		data_len = ble_adv_get_data(&ncdata);
   		if (data_len && ncdata) {
       		ble_adv_parse_param(ncdata, data_len);
   		}
	}
    return RET_OK;
}

int32 ble_adv_tx_data(uint8 *data, int32 len)
{
    uint8 *buff = NULL;
	uint8 section_num = 0;
	uint8 section_idx = 0;
	uint16 last_section_len = 0;
	uint16 start_pos = 0;
	uint16 cur_section_len = 0;
	struct ble_adv_info adv_info;

	if((data == NULL) || (len <= 0) || (len > 250 * ADV_MAX_SECTION_LEN)) { //section_num:1byte(255)
		return RET_ERR;
	}
	
	section_num = (len + ADV_MAX_SECTION_LEN - 1) / ADV_MAX_SECTION_LEN;
	last_section_len = len - (section_num - 1) * ADV_MAX_SECTION_LEN;
	adv_info.header_info.pdu_type = ADV_DISCOVER_TYPE;
	adv_info.header_info.tx_add = 1;
	adv_info.header_info.rx_add = 0;
	memcpy(adv_info.payload_info.addr, adv_addr, 6);
	adv_info.payload_info.ad_type = 0xFF;
	adv_info.payload_info.manufacturer_id = ADV_MANUFACTURER_ID;
	memcpy(adv_info.payload_info.identify_info, adv_identify_info, ADV_IDENTIFY_SET_LEN);

	
	while(1) {
		start_pos = section_idx * ADV_MAX_SECTION_LEN;
		if(section_idx < section_num - 1) {
			cur_section_len = ADV_MAX_SECTION_LEN;
		}else {
			cur_section_len = last_section_len;
		}
		adv_info.header_info.length = 6 + 4 + ADV_IDENTIFY_SET_LEN + 3 + cur_section_len; //addr+(1E FF 04 41)+ADV_IDENTIFY_SET_LEN+section_num+section_idx+byte_len+data
		adv_info.payload_info.ad_len = 3 + ADV_IDENTIFY_SET_LEN + 3 + cur_section_len;   //(FF 04 41)+ADV_IDENTIFY_SET_LEN+section_num+section_idx+byte_len+data
		adv_info.payload_info.section_num = section_num;
		adv_info.payload_info.section_idx = section_idx;
		adv_info.payload_info.byte_len = cur_section_len;
		memcpy(adv_info.payload_info.data, data + start_pos, cur_section_len);

        buff = os_malloc(adv_info.header_info.length + 2 + 1);
        if (buff) {
            return RET_ERR;
        }
        os_memcpy(buff+1, &adv_info, adv_info.header_info.length + 2);
        if (bt_hci_tx_adv_data(buff, adv_info.header_info.length + 2)) {
            os_free(buff);
            return RET_ERR;
        }
        os_free(buff);

		section_idx = section_idx + 1;
		if(section_idx >= section_num) {
			break;
		}
	}
	return RET_OK;
}

int32 ble_adv_ctrl_create(void)
{
	if (ble_ctrl == NULL) {
		ble_ctrl = (struct ble_rx_ctrl *)os_zalloc(sizeof(struct ble_rx_ctrl));
	}
	return RET_OK;
}

int32 ble_adv_ctrl_destory(void)
{
	if (ble_ctrl) {
		os_free(ble_ctrl);
	}
	return RET_OK;
}
