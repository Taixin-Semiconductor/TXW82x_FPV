#ifndef _HAL_TK_H_
#define _HAL_TK_H_
#include "typesdef.h"
#ifdef __cplusplus
extern "C" {
#endif

enum touchkey_ioctl_cmd
{
	TK_IOCTL_CMD_NONE = 0,
};

enum touchkey_irq_flag
{
    TK_IRQ_FLAG_GET_KEY_VALUE = BIT(0),
};

enum touchkey_key_num
{
    TK_KEY_TYPE_LONG    = BIT(26),
    TK_KEY_TYPE_NONE    = BIT(27),
};

/* User interrupt handle */
typedef int32 (*tk_irq_hdl)(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2, uint32 param3);

struct hgtk_param
{
    unsigned char            __tk_index      ;
    unsigned char            __tk_cs_en      ;
    unsigned char            __tk_use_num    ;
	unsigned char		     __tk_long_key	 ;
    unsigned char            __tk_adjust_en  ;
    unsigned char            param [14]      ;
    unsigned char            config[17]      ;
    volatile unsigned char   __time_base_cnt ; 
    volatile unsigned char   __tk_ch_cnt_0[6];
    volatile unsigned char   __tk_ch_cnt_1[6];
    volatile unsigned char   __tk_ch_cnt_2[6];
    volatile unsigned char   __tk_ch_cnt_3[6];
    volatile unsigned char   __tk_ch_cnt_4[6];
    volatile unsigned char   __tk_ch_cnt_5[6];
    volatile unsigned char   __tk_ch_cnt_6[6];
    volatile unsigned char   __tk_ch_cnt_7[6];
    volatile unsigned char   __tk_ch_index[6];
   
    unsigned short           __tk_adjust_time      ;
    unsigned short           __tk_adjust_diff_valu ;
    unsigned short           __tk_valid_time       ;
    unsigned short           __tk_long_key_time    ;
    unsigned short           __tk_noise_value      ;
    volatile unsigned short  __tk_i_set[6]         ;
    volatile unsigned short  __tk_ch_fth[6]        ;
    volatile unsigned short  __tk_ch_data_0[6]     ;
    volatile unsigned short  __tk_ch_data_1[6]     ;
    volatile unsigned short  __tk_ch_data_2[6]     ;
    volatile unsigned short  __tk_ch_data_3[6]     ;
    volatile unsigned short  __tk_ch_data_4[6]     ;
    volatile unsigned short  __tk_ch_data_5[6]     ;
    volatile unsigned short  __tk_ch_data_6[6]     ;
    volatile unsigned short  __tk_ch_data_7[6]     ;
    volatile unsigned short  __tk_ch_data_8[6]     ;
    unsigned long            __tk_adjust_line      ;
};

struct tk_device {
    struct dev_obj dev;
};

struct tk_hal_ops {
    struct devobj_ops ops;
    int32(*open)       (struct tk_device *tk);
    int32(*close)      (struct tk_device *tk);
    int32(*ioctl)      (struct tk_device *tk, enum touchkey_ioctl_cmd ioctl_cmd, uint32 param);
    int32(*request_irq)(struct tk_device *tk, tk_irq_hdl irq_hdl, uint32 irq_flag,  uint32 irq_data);
    int32(*release_irq)(struct tk_device *tk, enum touchkey_irq_flag irq_flag);
};

int32 tk_open(struct tk_device *tk);
int32 tk_ioctl(struct tk_device *tk, enum touchkey_ioctl_cmd ioctl_cmd, uint32 param);
int32 tk_irq_request(struct tk_device *tk, tk_irq_hdl irq_hdl, uint32 irq_flag,  uint32 irq_data);
int32 tk_irq_release(struct tk_device *tk, uint32 irq_flag);
int32 tk_close(struct tk_device *tk);
#ifdef __cplusplus
}
#endif
#endif

