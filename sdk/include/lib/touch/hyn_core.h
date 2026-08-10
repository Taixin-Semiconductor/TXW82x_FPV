#ifndef __HYN_CORE_H_
#define __HYN_CORE_H_

#include "basic_include.h"

#define HYN_ENTER()                 os_printf("%s %d\n",__func__,__LINE__)
#define HYN_ERROR(fmt, args...)     os_printf(KERN_ERR "[HYN][Error]%s:"fmt"\n",__func__,##args)
#define HYN_INFO(fmt, args...)      os_printf(KERN_INFO "[HYN][Info]%s:"fmt"\n",__func__,##args)

#define U8TO16(x1,x2) ((((x1)&0xFF)<<8)|((x2)&0xFF))
#define U8TO32(x1,x2,x3,x4) ((((x1)&0xFF)<<24)|(((x2)&0xFF)<<16)|(((x3)&0xFF)<<8)|((x4)&0xFF))
#define U16REV(x)  ((((x)<<8)&0xFF00)|(((x)>>8)&0x00FF))

#undef DISABLE
#undef ENABLE
#define DISABLE (0)
#define ENABLE  (1)

enum report_typ{
    REPORT_NONE = 0,
    REPORT_POS = 0x01,
    REPORT_KEY = 0x02,
    REPORT_GES = 0x04,
    REPORT_PROX = 0x08
};

enum work_mode{
    NOMAL_MODE = 0,
    GESTURE_MODE = 1,
    LP_MODE = 2,
    DEEPSLEEP = 3,
    DIFF_MODE = 4,
    RAWDATA_MODE = 5,
    BASELINE_MODE = 6,
    CALIBRATE_MODE = 7,
    FAC_TEST_MODE = 8,
    GLOVE_EXIT = 0x10,
    GLOVE_ENTER = 0x11,
    CHARGE_EXIT = 0x12,
    CHARGE_ENTER = 0x13,
    ENTER_BOOT_MODE = 0xCA,
};

#define MAX_POINTS_REPORT     (10)
struct ts_frame {
    uint8_t rep_num;
    enum report_typ report_need;
    uint8_t key_id;
    uint8_t key_state;
    struct {
        uint8_t pos_id;
        uint8_t event;
        uint16_t pos_x;
        uint16_t pos_y;
        uint16_t pres_z;
    }pos_info[MAX_POINTS_REPORT];
};

struct tp_info {
    uint8_t  fw_sensor_txnum;
    uint8_t  fw_sensor_rxnum;
    uint8_t  fw_key_num;
    uint8_t  reserve;
    uint16_t fw_res_y;
    uint16_t fw_res_x;
    uint32_t fw_boot_time;
    uint32_t fw_project_id;
    uint32_t fw_chip_type;
    uint32_t fw_ver;
    uint32_t ic_fw_checksum;
    uint32_t fw_module_id;
};

struct hyn_plat_data {
    uint32_t x_resolution;
	uint32_t y_resolution;
    int swap_xy;
	int reverse_x;
	int reverse_y;
};

struct hyn_ts_data {
    struct os_work work;
    struct os_event event;
    struct  os_msgqueue msgque;
    int app_iic_num;
    int gpio_irq;
    uint8_t  iic_addr;
    enum work_mode work_mode;
    struct hyn_plat_data plat_data;
    int boot_is_pass;
    struct tp_info hw_info;
    struct ts_frame rp_buf;
    uint8_t gesture_id;
    uint8_t *iic_trx_buff;
};

int hyn_wr_reg(struct hyn_ts_data *ts_data, uint32_t reg_addr, uint32_t reg_len, uint8_t *rbuf, uint32_t rlen);

void hyn_irq_set(struct hyn_ts_data *ts_data, uint8_t value);
void hyn_esdcheck_switch(struct hyn_ts_data *ts_data, uint8_t enable);

uint32_t hyn_sum32(int val, uint32_t* buf,uint16_t len);

#endif