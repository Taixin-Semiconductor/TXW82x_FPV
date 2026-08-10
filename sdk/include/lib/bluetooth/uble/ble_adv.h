#ifndef _BLE_ADV_H_
#define _BLE_ADV_H_
	
#ifdef __cplusplus
	extern "C" {
#endif

// 自定义协议
#define ADV_DISCOVER_TYPE 		(6)			// 广播包类型（可过滤广播包）
#define ADV_MANUFACTURER_ID 	(0x4104)	// 生产厂商ID（可做识别头信息）
#define ADV_DATA_MAX_LEN 		(39) 		// 广播最大长度（header+payload）
#define RX_DATA_MAX_LEN 		(256) 		// 接收数据总长度（id+len+data）

#define ADV_IDENTIFY_MAX_LEN  	(24) 		// 识别头最大长度 
#define ADV_IDENTIFY_SET_LEN    (8) 		// 识别头设置长度(0~23)
   
#define ADV_MAX_SECTION_LEN    	(16)		// 广播发送数据段最大长度

typedef struct ble_pro_str {
	uint32 cur_tsf;
	uint32 next_tsf;
	uint32 cur_interval_tsf;
	uint32 next_interval_tsf;

	uint32 sync_tsf;					 
	uint8	sync_tsf_en;


}TYPE_STRUCT_BLE_CTRL;

struct ble_adv_info {
	union HEADER_INFO {
		struct {
			uint16 pdu_type  : 4,  // bit0:3
				   reserved0 : 1,  // bit4
				   chn_sel   : 1,  // bit5
				   tx_add	  : 1, // bit6
				   rx_add	  : 1, // bit7
				   length	  : 8; // bit8:15
		};
		uint16 header;
	} header_info;

	union PAYLOAD_INFO {
		struct {
			uint8 addr[6];
			uint8 ad_len;
			uint8 ad_type;
			uint16 manufacturer_id;
			#if (ADV_IDENTIFY_SET_LEN > 0 && ADV_IDENTIFY_SET_LEN < ADV_IDENTIFY_MAX_LEN)
			uint8 identify_info[ADV_IDENTIFY_SET_LEN];
			#endif
			uint8 section_num;
			uint8 section_idx;
			uint8 byte_len;
			#if (ADV_IDENTIFY_SET_LEN > 0 && ADV_IDENTIFY_SET_LEN < ADV_IDENTIFY_MAX_LEN) 
			uint8 data[24-ADV_IDENTIFY_SET_LEN]; 
			#else
			uint8 data[24]; 
			#endif
		};
		uint8 payload[37];  
	} payload_info;
	
}__attribute__((packed));


struct ble_adv_process {
	uint8 start_flag;
	uint16 section_num;
	uint16 section_idx;
	uint16 byte_offset_len;
	uint16 rec_overtime; 	// 广播接收超时时间,根据不同包数来定义超时时间
	uint32 cur_tick;
	uint16 len;
	uint8 data[RX_DATA_MAX_LEN];
};

struct ble_rx_ctrl { 
	struct ble_adv_info    adv_info;	// 广播包数据
	struct ble_adv_process adv_pro;		// 数据处理 
};

int32 ble_adv_rx_data(uint8 *data, uint32 len);
int32 ble_adv_tx_data(uint8 *data, int32 len);
int32 ble_adv_ctrl_create(void);
int32 ble_adv_ctrl_destory(void);

#ifdef __cplusplus
}
#endif

#endif


