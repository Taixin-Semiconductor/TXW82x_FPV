#ifndef _HGTOUCHKEY_V2_H
#define _HGTOUCHKEY_V2_H
#include "hal/tk.h"
#include "osal/work.h"
//#include "../driver/tk/hg_touchkey_v2_hw.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef union _u_16 {
    uint16 w;
    struct {
        uint8 h;
        uint8 l;
    } b;
} TYPE_U16;

typedef union _s_16 {
    int16 w;
    struct {
        uint8 h;
        uint8 l;
    } b;
} TYPE_S16;

typedef struct _DW {
    TYPE_S16 MSW;
    TYPE_U16 LSW;
} TYPE_DW;

typedef union _u_32 {
    uint32 w;
    struct {
        uint8 h_32;
        uint8 h_24;
        uint8 l_16;
        uint8 l_8;
    } b;
} TYPE_u32;

typedef union _u_32_bit {
    uint32 w;
    struct {
        uint16 count;
        uint16 scan_done_pend : 1,
               data_normal_flag : 1,
               baseline_int_flag :1,
               cal_finsh_flag : 1,
               adjust_done_peng : 1,
               reserved : 11;
    } b;
} TYPE_U32_BIT;

typedef enum {
    TK_INIT = 0,
    ADJUST_DONE,
    DATA_INIT_DONE,
    SCAN_DONE,
    RE_ADJUST_DONE,
} TYPE_ENUM_STA;

typedef struct {
    int16 b[3];               // 分子多项式的系数Numerator
    int16 a[2];               // 分母多项式的系数Denominator
    int16 fixed_point_num;    // 移位bit数
} TYPE_BIQUAD_FILTER;

struct hgtk_algor{
    uint8                    __tk_long_key_flag;
    uint8                    __tk_ch_inc_flag;
    uint8                    __tk_mult_confirm_cnt;
    uint8                    __tk_noise_confirm_cnt;
    uint8                    __tk_next;
    uint8                    __tk_cm_cnt;
    uint8                    __tk_muli_key_cnt;
    uint8                    __tk_mult_noise_cnt;
    int16                    __tk_data_temp;
    uint16                   __tk_ms_cnt;
    uint32                   __tk_ch_en;
    uint32                   __tk_key_debug_flag;
    uint32                   __tk_key_flag;
    uint32                   __tk_key_flag_temp;
    uint32                   __tk_key_data;

    TYPE_DW                  MUL_RES_1 ; // 4byte
    TYPE_DW                  MUL_RES_2 ;
    TYPE_DW                  MUL_RES_3 ;
    TYPE_u32                 UNION_ADDR;
    TYPE_BIQUAD_FILTER       iir_param;
    TYPE_ENUM_STA            __tk_run_sta;
    TYPE_U32_BIT             __tk_flag;
};


struct hgtouchkey_v2 {
    struct tk_device     dev;
    uint32               hw;
    uint32               alg;
    tk_irq_hdl         	 irq_hdl;
    uint32               irq_data;
    uint32               irq_num;
    uint32               opened:1, dsleep:1, printf_flag:1;
    uint32               tk_config;
    struct os_work       tk_kick_work;
    struct os_work       tk_scan_work;
    struct hgtk_algor    tk_algori;

#ifdef CONFIG_SLEEP
    uint32               *bp_regs;
    uint32               bp_irq_flags;
    os_mutex_t      bp_suspend_lock;
    os_mutex_t      bp_resume_lock;
#endif
};

int32 hgtouchkey_v2_attach(uint32 dev_id, struct hgtouchkey_v2 *touchkey);

#ifdef __cplusplus
}
#endif
#endif
