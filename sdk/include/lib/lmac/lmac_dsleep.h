
// * @file    dsleep.h
// * @author  hushifei
// * @version V1.0.0
// * @date    08/25/2020
// * @brief   This file contains all the dsleep defines.

// Revision History
// (1) V1.0.0  05/16/2022  First Release

#ifndef __BGN_DSLEEP_H__
#define __BGN_DSLEEP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "osal/sleep.h"
#include "osal/timer.h"
#include "osal/msgqueue.h"

/*****************************************************
* SDK相关宏定义设置
******************************************************/
#define CONFIG_SYSTICK_HZ_DSLEEP    1000
#define OS_SYSTICK_HZ_DSLEEP        1000
#define SYS_CACHE_ENABLE_DSLEEP     1
//----------------------------------------------------

#ifndef BIT
#define BIT(nr)			(1UL << (nr))
#endif

#define SBIT(n) (1UL<<(n))
#define CBIT(n) ~(1UL<<(n))

#define TS_WIN_MAX  8000//us
#define RX_IDLE_TMO  10//ms
#define WKDATA_TMO  20//ms
#define BKN_TIMEOUT 6
#define BKN_TIMEOUT_ADD 2
#define BKN_TIMEOUT_MAX 20//(2*BKN_TIMEOUT)

#define BEACON_LOST_LIMIT   3
#define DSLEEP_VAR_WIN      10

#define TS_VEC_MAX 20 

#define RC_WK_INIT_DEFAULT  1200 //measured val 
#define RC_RF_INIT_DEFAULT  331 //measured each time
#define LOST_VAR_MS         2


#define DSLEEP_MAGIC_NUM 0x12344321

#define DEBUG_IO_ENABLE  0
#define LMAC_DBG0 PA_9
#define LMAC_DBG1 PA_8

#define DSLEEP_DBG_EN_PRINT         BIT(0)
#define DSLEEP_DBG_NO_WAKEUP        BIT(1)
#define DSLEEP_DBG_AUTO_SLEEP       BIT(2)
#define DSLEEP_DBG_IO_MASK          (0xff<<8)

#define DSLEEP_core_pd_pnd          BIT(0)
#define DSLEEP_TrimPend             BIT(1)
#define DSLEEP_wk_mclr_rst_pnd      BIT(2)
#define DSLEEP_lp_wkup_pnd          BIT(3)
#define DSLEEP_ALR0F                BIT(4)
#define DSLEEP_CoreTmrPnd           BIT(5)
#define DSLEEP_CoreTmrPnd2          BIT(6)
#define DSLEEP_second_pnding        BIT(7)
#define DSLEEP_LVD_PND              BIT(8)
#define DSLEEP_WATCHDOG_PND         BIT(9)

#ifndef LMAC_WKDATA_SIZE
#define LMAC_WKDATA_SIZE (64)
#endif
#ifndef LMAC_WKDATA_COUNT
#define LMAC_WKDATA_COUNT (1)
#endif
struct lmac_wkdata_param {
    uint8 count;       //must equal to LMAC_WKDATA_COUNT
    uint8 wkdata_size; //must equal to LMAC_WKDATA_SIZE
    uint8 r1, r2;
    struct {
        uint32 sip;        //source IP address. can be set 0.
        uint8  offset;     //wkdata's offset start from ether hdr.
        uint8  wkdata_len; //wkdata's length
        uint8  r1, r2;
        uint8  mask[LMAC_WKDATA_SIZE / 8]; //match mask.
        uint8  wkdata[LMAC_WKDATA_SIZE];   //wkdata used to match.
    } data[LMAC_WKDATA_COUNT];
};

static inline uint32 LMAC_WKDATA_SIP(struct lmac_wkdata_param *wkdata, uint8 idx)
{
    uint8 *ptr = (uint8 *)wkdata + 4 + idx * (8 + wkdata->wkdata_size + wkdata->wkdata_size / 8);
    return (idx < wkdata->count) ? *(uint32 *)ptr : 0;
}
static inline uint8 LMAC_WKDATA_OFFSET(struct lmac_wkdata_param *wkdata, uint8 idx)
{
    uint8 *ptr = (uint8 *)wkdata + 4 + idx * (8 + wkdata->wkdata_size + wkdata->wkdata_size / 8) + 4;
    return (idx < wkdata->count) ? *ptr : 0;
}
static inline uint8 LMAC_WKDATA_LEN(struct lmac_wkdata_param *wkdata, uint8 idx)
{
    uint8 *ptr = (uint8 *)wkdata + 4 + idx * (8 + wkdata->wkdata_size + wkdata->wkdata_size / 8) + 5;
    return (idx < wkdata->count) ? *ptr : 0;
}
static inline uint8 *LMAC_WKDATA_MASK(struct lmac_wkdata_param *wkdata, uint8 idx)
{
    uint8 *ptr = (uint8 *)wkdata + 4 + idx * (8 + wkdata->wkdata_size + wkdata->wkdata_size / 8) + 8;
    return (idx < wkdata->count) ? ptr : NULL;
}
static inline uint8 *LMAC_WKDATA_DATA(struct lmac_wkdata_param *wkdata, uint8 idx)
{
    uint8 *ptr = (uint8 *)wkdata + 4 + idx * (8 + wkdata->wkdata_size + wkdata->wkdata_size / 8) + 8 + wkdata->wkdata_size / 8;
    return (idx < wkdata->count) ? ptr : NULL;
}

#define PSALIVE_HEARTBEAT_SIZE      256
#define PSALIVE_HEARTBEAT_RESP_SIZE 128
#define PSALIVE_WAKEUP_DATA_SIZE    128
#define SLEEP_USER_DATA_LEN         512

struct dsleep_psdata {
    uint32              ip;
    uint16              port, period, hb_tmo, aplost_time, wkdata_off;
    uint8               hbdata_len, hbresp_len, wkdata_len;
    uint8               psmode: 3, ap_err: 1, wkio_mode: 1, paired: 1;
    uint8               conn_try;
    uint8               hb_data[PSALIVE_HEARTBEAT_SIZE];
    uint8               hb_resp[PSALIVE_HEARTBEAT_RESP_SIZE];
    uint8               wk_data[PSALIVE_WAKEUP_DATA_SIZE];
    uint8               arp_reply[64];
    uint8               wk_reason;
    uint8               wk_io: 7, wkio_edge: 1;
    uint8               detect_io: 7, detect_level: 1;
    uint8               pa_ctrl: 1, pa_ctrl_io: 7; //todo
    uint8               vdd13_ctrl: 2, ssid_set: 1, gpiob_en: 1, wkdata_save: 1, io_level_wk: 1, wait_psmode: 2;
    uint8               ssid[32], psk[32], bssid[6];
    uint8               wkdata_mask[16];

    //system_sleep_config parameter
    uint32              user_gpioa_resv;
    uint32              user_gpiob_resv;
    uint32              sleep_us;
    uint32              boot_mode : 4, dsleep_en : 1;

    uint32              dhcpc_ip;
    uint64              rtc_local;
    uint64              last_timestamp_local;
    uint64              rtc_remote;
    uint64              last_timestamp_remote;
};

#define GPIOA_REG_LEN           (0x0080/4 + 1)
#define GPIOB_REG_LEN           (0x0080/4 + 1) 
#define GPIOC_REG_LEN           (0x0080/4 + 1) 
struct dsleep_cfg{
    uint32                      magic;

    uint8                       aplost_time;
    uint8                       wk_reason;
    uint8                       encrypt     : 1;
    uint8                       ap_err      : 1,
                                alg_sel     : 2,
                                txic_en     : 1,
                                txic_wakeup : 1,
                                wdt_close   : 1,
                                clk_switch_dis  : 1,
                                use_old_libflash: 1;
    uint8                       fake_tim_fadeout;
    uint8                       usr_fadeout_en  : 1,
                                usr_fadeout_val : 7;
    uint8                       dtim_cnt;
    uint8                       dtim_period;
    uint8                       dtim_test;  //used for at+ test
    uint8                       debounce;
    uint32                      bkn_int_tu;
    uint32                      dtim_period_us;
    uint8                       tim_valid   : 1,//1
                                bc_valid    : 1,//2
                                psm_enter   : 1,//3
                                auto_dsleep : 1,//4
                                buck_mode   : 1,//5
                                gpiob_hold  : 1,//6
                                dbg_en      : 1,//7
                                fem_used    : 1;//8
    uint8                       ext_dcdc    : 1,
                                assert_hold : 1;
    uint8                       auto_dsleep_tmo;
    uint8                       pwm_pin[4]; //[0-3  valid]
    uint8                       pwm_mode;//2bit
    uint8                       wkio_pin[6];//[0-5  vaild]
    uint8                       wkio_edge;
    int8                        sen_level;//0-2 valid
    uint16                      listen_interval;
    uint16                      max_idle_period;//sec
    uint16                      ap_lost_cnt;
    uint16                      margin_ms;

    uint32                      default_sys_clk;

    uint32                      sleep_type;
    struct system_sleep_param   sleep_args;
    uint32                      sleep_dbg_cfg;
    int16                       rx_lead_margin;
    int16                       rx_tmo_margin;

    //---------user define functions--------------
    int32(*usr_kalive_cb)(void *usr_kalive_priv);
    void * usr_kalive_priv;
    int32(*usr_wkdet_cb)(void *usr_wkdet_priv, uint8 *data, uint32 len);
    void * usr_wkdet_priv;
    struct lmac_wkdata_param    *wkdata;

    uint16                      aid;
    uint8                       mac_addr[6];
    uint8                       bssid[6];
    uint32                      ip_addr;
    uint32                      user_gpioa_resv;
    uint32                      user_gpiob_resv;
    uint32                      user_gpioc_resv;
    uint32                      gpioa_regs[GPIOA_REG_LEN];
    uint32                      gpiob_regs[GPIOB_REG_LEN];
    uint32                      gpioc_regs[GPIOC_REG_LEN];
    uint32                      pmu_regs[0x100/4];
    uint32                      sys_wdt_ms;
    uint32                      lp_wdt_ms;
    uint32                      rtc_wkio;
};

enum DSLEEP_MODE {
    DSLEEP_MODE_NONE,
    DSLEEP_MODE_STA_ALIVE,
    DSLEEP_MODE_AP_ALIVE,
    DSLEEP_MODE_UCASTWK,
    DSLEEP_MODE_APWK,
    DSLEEP_MODE_IOWK_ONLY = 6, //之前用了6，后续如果有新增先添加一个5

    DSLEEP_MODE_NUM,
};

enum DSLEEP_WAIT_MODE {
    DSLEEP_WAIT_MODE_PS_CONNECT,
    DSLEEP_WAIT_MODE_STANDBY,

    DSLEEP_WAIT_MODE_NUM,
};

#if 0
enum DSLEEP_IOCTL_CMD {
    /*Set CMDs*/
    DSLEEP_IOCTL_SET_ENTER_SLEEP_CB = 0x1,
    DSLEEP_IOCTL_SET_KEEP_ALIVE_CB,
    DSLEEP_IOCTL_SET_USR_WKDET_CB,
    DSLEEP_IOCTL_SET_DSLEEP_BSS_MAX_IDLE,
    DSLEEP_IOCTL_SET_DSLEEP_WKIO_PIN,
    DSLEEP_IOCTL_SET_DSLEEP_WKIO_EDGE,
    DSLEEP_IOCTL_SET_DSLEEP_APLOST_TIME,
    DSLEEP_IOCTL_SET_DSLEEP_SENS_LEVEL,
    DSLEEP_IOCTL_SET_DSLEEP_PWM_SEL,
    DSLEEP_IOCTL_SET_DSLEEP_PWM_MODE,
    DSLEEP_IOCTL_SET_IP_ADDR,
    DSLEEP_IOCTL_SET_GPIOA_RESV,
    DSLEEP_IOCTL_SET_GPIOB_RESV,
    DSLEEP_IOCTL_SET_GPIOC_RESV,
    /*Get CMDs*/
    DSLEEP_IOCTL_GET_IP_ADDR = 0x20000000,
};

//ioctrl: set_cfg
#define dsleep_set_enter_sleep_cb(func)             dsleep_ioctl(DSLEEP_IOCTL_SET_ENTER_SLEEP_CB, (uint32)(func), 0)
#define dsleep_set_keep_alive_cb(func)              dsleep_ioctl(DSLEEP_IOCTL_SET_KEEP_ALIVE_CB, (uint32)(func), 0)
#define dsleep_set_usr_wkdet_cb(priv, func)         dsleep_ioctl(DSLEEP_IOCTL_SET_USR_WKDET_CB, (uint32)priv, (uint32)(func))
#define dsleep_set_max_idle(max_idle_sec)           dsleep_ioctl(DSLEEP_IOCTL_SET_DSLEEP_BSS_MAX_IDLE, (uint32)(max_idle_sec), 0)
#define dsleep_set_wkio_pin(pin_idx, pin_num)       dsleep_ioctl(DSLEEP_IOCTL_SET_DSLEEP_WKIO_PIN, (uint32)(pin_idx), (uint32)(pin_num))
#define dsleep_set_wkio_edge(pin_idx, neg_edge)     dsleep_ioctl(DSLEEP_IOCTL_SET_DSLEEP_WKIO_EDGE, (uint32)(pin_idx), (uint32)(neg_edge))
#define dsleep_set_aplost_time(t_sec)               dsleep_ioctl(DSLEEP_IOCTL_SET_DSLEEP_APLOST_TIME, (uint32)(t_sec), 0)
#define dsleep_set_sens_level(sens_level)           dsleep_ioctl(DSLEEP_IOCTL_SET_DSLEEP_SENS_LEVEL, (uint32)(sens_level), 0)
#define dsleep_set_pwm_sel(pwm_idx, pin_num)        dsleep_ioctl(DSLEEP_IOCTL_SET_DSLEEP_PWM_SEL, (uint32)(pwm_idx), (pin_num))
#define dsleep_set_pwm_mode(pwm_idx, mode)          dsleep_ioctl(DSLEEP_IOCTL_SET_DSLEEP_PWM_MODE, (uint32)(pwm_idx), (mode))
#define dsleep_set_ip_addr(ip_addr)                 dsleep_ioctl(DSLEEP_IOCTL_SET_IP_ADDR, (uint32)ip_addr, 0)
#define dsleep_set_user_gioa(user_gioa)             dsleep_ioctl(DSLEEP_IOCTL_SET_GPIOA_RESV, (uint32)user_gioa, 0)
#define dsleep_set_user_giob(user_giob)             dsleep_ioctl(DSLEEP_IOCTL_SET_GPIOB_RESV, (uint32)user_giob, 0)
#define dsleep_set_user_gioc(user_gioc)             dsleep_ioctl(DSLEEP_IOCTL_SET_GPIOC_RESV, (uint32)user_gioc, 0)

//ioctrl: get_cfg
#define dsleep_get_ip_addr()                                 dsleep_ioctl(DSLEEP_IOCTL_GET_IP_ADDR, 0, 0)
#endif

union DSLEEP_LO_CFG
{
    struct {
        uint16  freq;    // unit: MHz
        uint8   vco_band;
        uint8   int_part;
        uint32  fract_part;
    }lo_cfg;
    uint32 word_buf[2];
};

typedef struct {
    uint8 hour  ;  
    uint8 minute;
    uint8 second;
    uint16 tsssv;
    float ssv;
    
    uint8 year  ;
    uint8 month ;
    uint8 day   ;
    uint8 week  ;
} RTC_TIMER_TYPEDEF;

struct dsleep_priv {
    uint32  flags;
    uint32  dsleep_mode         :   1,
            beacon_rxed         :   1,
            no_rf_tx_cali       :   1,
            dsleep_wakeup_flag  :   1,//use to indicate dsleep wakeup to inite driver
            no_sleep_wakeup     :   1;
    
    uint32  wkup_cnt;
    uint32  bkn_lost;
    uint32  bkn_lost_cont: 8,
            stat_wkup_cnt : 8,
            stat_lost_cnt  :8;

    int8    rx_rssi;
    int8    rsvd[3];

    struct lmac_bgn_priv    *lmac_bgn;
    struct hggpio_v4_hw     *gpioa;
    struct hggpio_v4_hw     *gpiob;
    struct hggpio_v4_hw     *gpioc;
    struct hggpio_v4_hw     *gpiod;
    struct hggpio_v4_hw     *gpioe;
    struct hggpio_v4_hw     *gpiof;

    struct hguart_v2_hw     *p_uart;
    struct hguart_v2        *dev_uart;
    struct dev_obj          *dev_xip;
    struct dev_obj          *dev_xspi;
    uint32                  sys_con14_bak;//vdd18
    uint32                  clk_con1_bak;//clk_div

    PMU_TypeDef             pmu_bak;
    union DSLEEP_LO_CFG     freq_table;

    uint32 dsleep_rst;
    
    uint32 pmu_con;
    //uint32 magic_num;
    uint32 sleep_cnt;
    uint32 rx_cnt_tmp;
    uint32 tmrpr_err;
    uint32 reg_tmrpr;
    uint32 reg_tmrtrim;
    uint32 rc_period_us;

    uint32 t_rx_start;

    uint32 rc_ms;
    uint32 rc_cycle_sleep;
    uint32 rc_elapse;
	
	uint16 bkn_len_us;
    uint8  bkn_timestamp;

    uint32 tbtt_pre;
	
    int32   drift_acc;

    //add from other .h
    struct sk_buff                      *null_psm;
    struct sk_buff                      *ps_poll;//todo: del
    struct sk_buff                      *ps_arp;
    uint8                               null_fail_cnt;
    uint16                              null_sn;

    //pending should be clear
    uint16                              skb_null_psm_used   : 1,
                                        skb_null_psm_acked  : 1,
                                        skb_ps_poll_used    : 1,
                                        dsleep_loop_test    : 1,
                                        wkdata_waiting      : 1,
                                        ts_idx              : 8,
                                        ts_valid            : 1,
                                        dsleep_rdy          : 1;
    union{
      uint32                            val;    //todo move all clear pending here
    }todo_rsvd;
    uint64                              last_rx;
    uint64                              last_rx_init;
    uint32                              key_updata_tmo;
    uint32                              dsleep_tmr_cnt;
    struct os_timer                     dsleep_tmr;
    struct os_semaphore                 dsleep_sem;

    //move from dcfg
    uint32                      bkn_trim_rc;
    uint32                      bkn_dtim_rc;//updata if hw trum
    uint32                      bkn_intv_rc : 30,
                                rc_init : 1;
    uint32                      bkn_intv_rc_pre;
    uint32                      rc_pr_delt;
    uint8                       rc_var;
    
    int32                       bkn_dly;
    uint32                      bkto;
    uint16                      bkn_dur;

    uint32                     rc_wk_init;//如果初始化需要设置
    uint32                     rc_rf_init;
    int32                      ts_op_avg;
    int32                      ts_op_min;
    int32                      ts_op_max;
    int32                      ts_op_bkn;
    int32                      ts_win;
    uint64                     timestamp_pre;
    uint32                     ts_vec[TS_VEC_MAX];
    int32                      ts_rem[TS_VEC_MAX]; //must int32
    uint8                      rx_buff[512];
    uint32                      ap_lost_det : 1,
                                wkdata_det  : 1,
                                txnull_fail : 1,
                                wkio_pending: 6;
    //no clear area
    struct  dsleep_cfg          dcfg;
    RTC_TIMER_TYPEDEF           rtc_timer;
    struct os_task              dsleep_task;
    struct os_mutex             null_tx_mutex;
    uint32 wkcon_bak[3];
};

//#define ETH_P_ARP       0x0806      /* Address Resolution packet */
//#define ETH_P_IP        0x0800      /* Internet Protocol packet */
struct dsleep_eth_hdr{
    uint8 dsap;
    uint8 ssap;
    uint8 control_field;
    uint8 oui[3];
    uint8 type[2];//ARP= 0x0806 //IP= 0x0800
}; 

struct dsleep_arp_hdr{
    uint8   hw_type[2];// Ethernet 0x0001
    uint8   proto_type[2];//IPv4= 0x0800
    uint8   hw_size;// MAC_addr: 6
    uint8   porto_size;//IP_size: 4
    uint8   op_code[2];//1: ARP request; 2: ARP Reply; 3: RRAP Request; 4: RRAP Reply
    uint8   sender_mac[6];
    uint8   sender_ip[4];
    uint8   target_mac[6];
    uint8   target_ip[4];
    uint8   resv[18];//dump 18B tenda->Tp yes, TP->tenda no,
};

//IP header: 20Bytes
struct dsleep_ip_hdr{
    uint8  version_hdr_len;//version; header length//0x45:ver4 20B
    uint8  service_filed;//Differentiated Services Filed//0x00
    uint16 total_len;//total length
    
    uint16 id;//identification
    uint16 flags;//Flags: 3bits, Fragment Offset:13 bits
    
    uint8  ttl;//Time To Live
    uint8  protocol;//0x01=ICMP 0x02=IGMP 0x06=TCP6 0x11=UDP  //42B
    uint16 hdr_check_sum;//44B
    
    uint32  src_ip;
    uint32  dst_ip; //52B
};

//tcp header: 20Bytes
struct dsleep_tcp_hdr{
    uint16 src_port;
    uint16 dst_port;//56B
    uint32 seq_num;
    uint32 ack_num;

    uint16  hdr_len : 4,
            rsvd    : 6,
            urg     : 1,
            ack     : 1,
            psh     : 1,
            rst     : 1,
            syn     : 1,
            fin     : 1;
    uint16  win_size;

    uint16  checksum;
    uint16  urgentpointer;
};

//tcp header: 20Bytes
struct dsleep_udp_hdr{
    uint16 src_port;
    uint16 dst_port;//56B
    uint16 length;
    uint16 checksum;
};

extern struct dsleep_priv *bgn_dsleep;
void *bgn_dsleep_init(void *ops);
void dsleep_wakeup(void);
void debug_config(void);
int32 dsleep_bgn_resume_hdl(void *ops, uint32 wake_reason, uint32 param2);
int32 dsleep_cfg_init(uint16 sleep_type);
int32 dsleep_wkdata_check(uint8* data, uint32 len);

int32 dsleep_arp_req_check(uint8 *data, int32 len);
int32 dsleep_arp_data_send(uint8 *target_mac, uint8 *target_ip, uint8 pm, uint16 seq_num);
//void lmac_bgn_send_ps_poll(struct lmac_ops *ops, uint8 pm, uint16 sn);
int32 lmac_bgn_send_null_psm(void *ops, uint8 pm);


/*---------------------------- RTC Private define --------------------------------*/

/***** RTC_WPR(for RTC) Register *****/
#define  RTC_WPR_KEY(n)               ((n) & 0xFF)               // 写保护,依次写入0x15和0x5c, 0x0：未解除, 0x1：已解除
/***** RTC_CTR(for RTC) Register *****/
#define  RTC_CTR_ALR0WKE(n)             (((n) & 0x00000001) << 31) // 
#define  RTC_CTR_WUTWKE(n)             (((n) & 0x00000001) << 30) // 
#define  RTC_CTR_TAMPM(n)             (((n) & 0x00000001) << 29) // 入侵检测引脚复用,0x0：PE12,0x1：TMR1_INC_PIN
#define  RTC_CTR_CALOE(n)             (((n) & 0x00000001) << 28) // 校准输出使能 0x0：禁止，0x1：使能 Note：输出校准脉冲和1Hz
#define  RTC_CTR_BPSR(n)              (((n) & 0x00000001) << 27) // Bypass影子寄存器，0x0：日历值取自影子寄存器，0x1：日历值取自日历计数器
#define  RTC_CTR_SUB1H_EN             (0x00000001 << 26)         // 减少1小时 0x0：无作用, 0x1：减少1小时
#define  RTC_CTR_ADD1H_EN             (0x00000001 << 25)         // 增加1小时 0x0：无作用, 0x1：减少1小时
#define  RTC_CTR_HF(n)                (((n) & 0x00000001) << 24) // 小时格式，0x0:24小时制, 0x1:12小时制
#define  RTC_CTR_TAMPPUEN(n)          (((n) & 0x00000001) << 23) // 入侵检测上拉电阻使能 0x0：禁止，0x1：使能
#define  RTC_CTR_TAMPPCH(n)           (((n) & 0x00000003) << 21) // 入侵检测预充电时间,0x0：1个RCLK周期,0x1：2个RCLK周期,0x2：4个RCLK周期,0x3：8个RCLK周期,Note：每次采样之前激活上拉电阻的时间
#define  RTC_CTR_TAMPFLT(n)           (((n) & 0x00000003) << 19) // 入侵检测滤波 0x0：检测到有效边沿后激活入侵，0x1：连续检测到2次有效电平激活入侵，0x2：连续检测到4次有效电平激活入侵，0x3：连续检测到8次有效电平激活入侵
#define  RTC_CTR_TAMPSF(n)            (((n) & 0x00000007) << 16) // 入侵检测采样频率 RCLK/(256*(2^ TAMPSF))
#define  RTC_CTR_TAMPTRG(n)           (((n) & 0x00000001) << 15) // 入侵检测触发；当TAMPFLT = 0时：0x0：下降沿, 0x1：上升沿。当TAMPFLT ！= 0时：0x0：低电平, 0x1：高电平
#define  RTC_CTR_TAMPTS(n)            (((n) & 0x00000001) << 14) // 入侵检测激活时间戳0x0：禁止, 0x1：使能
#define  RTC_CTR_TSTRG(n)             (((n) & 0x00000001) << 13) // 时间戳触发0x0：上升沿, 0x1：下降沿
#define  RTC_CTR_TAMPIE(n)            (((n) & 0x00000001) << 12) // 入侵检测中断使能 0x0：禁止, 0x1：使能
#define  RTC_CTR_TSIE(n)              (((n) & 0x00000001) << 11) // 时间戳中断使能 0x0：禁止, 0x1：使能
#define  RTC_CTR_ALR0IE(n)            (((n) & 0x00000001) << 9)  // 闹钟0中断使能 0x0：禁止, 0x1：使能
#define  RTC_CTR_WUTIE(n)             (((n) & 0x00000001) << 8)  // 唤醒定时器中断使能 0x0：禁止, 0x1：使能
#define  RTC_CTR_TAMPE(n)             (((n) & 0x00000001) << 7)  // 入侵检测使能 0x0：禁止, 0x1：使能
#define  RTC_CTR_TSE(n)               (((n) & 0x00000001) << 6)  // 时间戳使能 0x0：禁止, 0x1：使能
#define  RTC_CTR_ALR0E(n)             (((n) & 0x00000001) << 4)  // 闹钟0使能 0x0：禁止, 0x1：使能
#define  RTC_CTR_WUTE(n)              (((n) & 0x00000001) << 3)  // 唤醒定时器使能 0x0：禁止,0x1：使能
#define  RTC_CTR_WUCKSEL(n)           (((n) & 0x00000007) << 0)  // 唤醒定时器时钟选择,0x0：RCLK/2,0x1：RCLK/4,0x2：RCLK/8,0x3：RCLK/16,0x4，0x5：ck_spre，定时器为16位,0x6，0x7：ck_spre，定时器为17位
/***** RTC_PDR(for RTC) Register *****/
#define  RTC_PDR_PREDIV_A(n)          (((n) & 0x0000007F) << 16) // 异步分频系数F（ck_apre=F(RCLK)/(PREDIV_A+1)	 
#define  RTC_PDR_PREDIV_S(n)          (((n) & 0x00007FFF) << 0)  // 同步分频系数F（ck_spre=F( ck_apre )/(PREDIV_S+1)
/***** RTC_ISR(for RTC) Register *****/
#define  RTC_ISR_SSSPF(n)             (((n) & 0x0001) << 15)     // 亚秒平移挂起标志 0：亚秒平移寄存器可写， 1：亚秒平移寄存器不可写（硬件校准中）
#define  RTC_ISR_RECALPF(n)           (((n) & 0x0001) << 14)     // 重新校准挂起标志 0：校准寄存器可写，1：校准寄存器不可写（硬件校准中）
#define  RTC_ISR_TAMPF(n)             (((n) & 0x0001) << 13)     // 入侵检测标志 0x0：未触发，0x1：已触发
#define  RTC_ISR_TSOVF(n)             (((n) & 0x0001) << 12)     // 时间戳溢出标志 0x0：未溢出，0x1：已溢出
#define  RTC_ISR_TSF(n)               (((n) & 0x0001) << 11)     // 时间戳标志 0x0：未触发，0x1：已触发
#define  RTC_ISR_ALR0F(n)             (((n) & 0x0001) << 9)      // 闹钟0标志 0x0：未触发，0x1：已触发
#define  RTC_ISR_WUTF(n)              (((n) & 0x0001) << 8)      // 唤醒定时器标志 0x0：未触发，0x1：已触发
#define  RTC_ISR_TAMPWF(n)            (((n) & 0x0001) << 7)      // 入侵检测写标志 0x0：不允许更新入侵检测，0x1：允许更新入侵检测
#define  RTC_ISR_ALR0WF(n)            (((n) & 0x0001) << 5)      // 闹钟0写标志 0x0：不允许更新闹钟0，0x1：允许更新闹钟0
#define  RTC_ISR_WUTWF(n)             (((n) & 0x0001) << 4)      // 唤醒定时器写标志 0x0：不允许更新唤醒定时器，0x1：允许更新唤醒定时器
#define  RTC_ISR_INITM(n)             (((n) & 0x0001) << 3)      // 初始化模式 0x0：自由运行模式，该位由1->0时，计数器重新开始计数，0x1：初始化模式，用于编程RTC_TMR，RTC_DAR和RTC_PDR。当该位置1，计数器停止计数
#define  RTC_ISR_INITMF(n)            (((n) & 0x0001) << 2)      // 初始化模式标志 0x0：不允许更新日历寄存器，0x1：允许更新日历寄存器
#define  RTC_ISR_INITS(n)             (((n) & 0x0001) << 1)      // 初始化状态，当日历年份字段不为0时由硬件置1 0x0：未初始化，0x1：已初始化
#define  RTC_ISR_RSF(n)               (((n) & 0x0001) << 0)      // 影子寄存器同步标志 0x0：未同步，0x1：已同步
/***** RTC_SSR(for RTC) Register *****/
#define  RTC_SSR_SSV(n)               (((n) & 0xFF) << 0)
/***** RTC_TMR(for RTC) Register *****/
#define  RTC_TMR_PM(n)                (((n) & 0x00000001) << 24)
#define  RTC_TMR_HT(n)                (((n) & 0x00000003) << 20) // 小时的十位（BCD格式）
#define  RTC_TMR_HU(n)                (((n) & 0x0000000F) << 16) // 小时的个位（BCD格式）
#define  RTC_TMR_MNT(n)               (((n) & 0x00000007) << 12) // 分钟的十位（BCD格式）
#define  RTC_TMR_MNU(n)               (((n) & 0x0000000F) << 8)
#define  RTC_TMR_ST(n)                (((n) & 0x00000007) << 4)  // 秒的十位（BCD格式）
#define  RTC_TMR_SU(n)                (((n) & 0x0000000F) << 0)
/***** RTC_DAR(for RTC) Register *****/
#define  RTC_DAR_YT(n)                (((n) & 0x0000000F) << 28) // 年份的十位（BCD格式）
#define  RTC_DAR_YU(n)                (((n) & 0x0000000F) << 24) // 年份的个位（BCD格式）
#define  RTC_DAR_MT(n)                (((n) & 0x00000001) << 20) // 月份的十位（BCD格式）
#define  RTC_DAR_MU(n)                (((n) & 0x0000000F) << 16) // 月份的个位（BCD格式）
#define  RTC_DAR_DT(n)                (((n) & 0x00000003) << 12) // 日的十位（BCD格式）
#define  RTC_DAR_DU(n)                (((n) & 0x0000000F) << 8)  // 日的个位（BCD格式）
#define  RTC_DAR_WU(n)                (((n) & 0x00000007) << 0)  // 星期的个位（BCD格式）
/***** RTC_ALR0R(for RTC) Register *****/
#define  RTC_ALR0R_MSK4(n)           (((n) & 0xFFFFFFFF) << 31)  // 星期或日期屏蔽 0x0：不屏蔽，0x1：屏蔽
#define  RTC_ALR0R_WDSEL(n)          (((n) & 0x00000001) << 30)  // 星期或日期选择 0x0：DU[3:0]代表日期，0x1：DU[2:0]代表星期。DT[1:0]和DU[3]忽略
#define  RTC_ALR0R_DT(n)             (((n) & 0x00000003) << 28)  // 日期的十位（BCD格式）
#define  RTC_ALR0R_DU(n)             (((n) & 0x0000000F) << 24)  // 星期或日期的个位（BCD格式）
#define  RTC_ALR0R_MSK3(n)           (((n) & 0xFFFFFFFF) << 23)  // 小时屏蔽 0x0：不屏蔽，0x1：屏蔽
#define  RTC_ALR0R_PM(n)             (((n) & 0x00000001) << 22)  // AM/PM符号 0x0：AM或24小时制，0x1：PM
#define  RTC_ALR0R_HT(n)             (((n) & 0x00000003) << 20)  // 小时的十位（BCD格式）
#define  RTC_ALR0R_HU(n)             (((n) & 0x0000000F) << 16)  // 小时的个位（BCD格式）
#define  RTC_ALR0R_MSK2(n)           (((n) & 0xFFFFFFFF) << 15)  // 分钟屏蔽 0x0：不屏蔽，0x1：屏蔽
#define  RTC_ALR0R_MNT(n)            (((n) & 0x00000007) << 12)  // 分钟的十位（BCD格式）
#define  RTC_ALR0R_MNU(n)            (((n) & 0x0000000F) << 8)   // 分钟的个位（BCD格式）
#define  RTC_ALR0R_MSK1(n)           (((n) & 0xFFFFFFFF) << 7)   // 秒钟屏蔽 0x0：不屏蔽，0x1：屏蔽
#define  RTC_ALR0R_ST(n)             (((n) & 0x00000007) << 4)   // 秒钟的十位（BCD格式）
#define  RTC_ALR0R_SU(n)             (((n) & 0x0000000F) << 0)   // 秒钟的个位（BCD格式）
/***** RTC_WUTR(for RTC) Register *****/
#define  RTC_WUTR_ARV(n)              ((n) & 0xFFFF)             // 自动重载值

/***** RTC_TSSSR(for RTC) Register *****/
#define  RTC_TSSSR_SSV(n)             ((n) & 0xFF)               // 亚秒值

/***** RTC_TSTMR(for RTC) Register *****/
#define  RTC_TSTMR_PM(n)              (((n) & 0x00000001) << 31) // AM/PM符号 0x0：AM或24小时制，0x1：PM
#define  RTC_TSTMR_HT(n)              (((n) & 0x00000003) << 20) // 小时的十位（BCD格式）
#define  RTC_TSTMR_HU(n)              (((n) & 0x0000000F) << 16) // 小时的个位（BCD格式）
#define  RTC_TSTMR_MNT(n)             (((n) & 0x00000007) << 12) // 分钟的十位（BCD格式）
#define  RTC_TSTMR_MNU(n)             (((n) & 0x0000000F) << 8)  // 分钟的个位（BCD格式）
#define  RTC_TSTMR_ST(n)              (((n) & 0x00000001) << 4)  // 秒钟的十位（BCD格式）
#define  RTC_TSTMR_SU(n)              (((n) & 0x00000001) << 0)  // 秒钟的个位（BCD格式）
/***** RTC_TSDAR(for RTC) Register *****/
#define  RTC_TSDAR_YT(n)              (((n) & 0x0000000F) << 28) // 年份的十位（BCD格式）
#define  RTC_TSDAR_YU(n)              (((n) & 0x0000000F) << 24) // 年份的个位（BCD格式）
#define  RTC_TSDAR_MT(n)              (((n) & 0x00000001) << 20) // 月份的十位（BCD格式）
#define  RTC_TSDAR_MU(n)              (((n) & 0x0000000F) << 16) // 月份的个位（BCD格式）
#define  RTC_TSDAR_DT(n)              (((n) & 0x00000003) << 12) // 日的十位（BCD格式）
#define  RTC_TSDAR_DU(n)              (((n) & 0x0000000F) << 8)  // 日的个位（BCD格式）
#define  RTC_TSDAR_WU(n)              (((n) & 0x00000007) << 0)  // 星期的个位（BCD格式）
/***** RTC_CALR(for RTC) Register *****/
#define  RTC_CALR_CALP(n)             (((n) & 0x0001) << 14)     // 正/负校准（校准周期为32秒） 0x0：负校准，0x1：正校准
#define  RTC_CALR_CALP16(n)           (((n) & 0x0001) << 13)     // 每2^11个RCLK屏蔽/增加1个RCLK，校准周期为16s
#define  RTC_CALR_CALP8(n)            (((n) & 0x0001) << 12)     // 每2^10个RCLK屏蔽/增加1个RCLK，校准周期为8s
#define  RTC_CALR_CEV(n)              (((n) & 0x01FF) << 0)      // 每2^9个RCLK屏蔽 /增加1个RCLK，校准期望值
/***** RTC_SSSR(for RTC) Register *****/
#define  RTC_SSSR_ASS(n)              (((n) & 0x0001) << 16)     // 提前/延迟亚秒 0x0：延迟亚秒 0x1：提前亚秒
#define  RTC_SSSR_SSEV(n)             (((n) & 0x00003FFF) << 0)  // 亚秒期望值

/* Private typedef -----------------------------------------------------------*/

// 设置时间格式：12小时制、24小时制
typedef enum 
{
    SET_12_HOURS = 0x1,
    SET_24_HOURS = 0x0,
    SET_AM       = 0x0,
    SET_PM       = 0x1
}TYPE_ENUM_TIMER_FORMAT;

/* 夏令时操作 */
typedef enum 
{
    DST_SUB1H = 0x0,
    DST_ADD1H = 0x1
}TYPE_ENUM_DST;
/* 时间戳触发 */
typedef enum 
{
    TSE_FALL_EDGE_TRIG    = 0x0,
    TSE_RISING_EDGE_TRIG  = 0x1
}TYPE_ENUM_TIME_STAMP_TRIG;

/* 入侵检测引脚复用 */
typedef enum 
{  
    PIN_PA8      = 0x0,
    LP_WKUP_PIN  = 0x1
}TYPE_ENUM_TAMPM;
/* 入侵检测触发 */
typedef enum 
{
    FALLING_EDGE_TRIG    = 0x0,
    RISING_EDGE_TRIG     = 0x1,
    LOW_LEV              = 0x0,
    HIGH_LEV             = 0x1
}TYPE_ENUM_INTR_DET_TRIG;

/* Intrusion detection filtering */
typedef enum 
{
    EDGE_EFFE      = 0x0,
    TWICE_EFFE_LEV = 0x1,
    FOUR_EFFE_LEV  = 0x2,
    EIGHT_EFFE_LEV = 0x3
}TYPE_ENUM_EFFE_LEV;

/* Intrusion detection precharge time */
typedef enum 
{
// 入侵检测预充电时间
    RCLK1_PRE_TIME = 0x0,
    RCLK2_PRE_TIME = 0x1,
    RCLK4_PRE_TIME = 0x2,
    RCLK8_PRE_TIME = 0x3
}TYPE_ENUM_PRECHARGE_TIME;

/* 唤醒定时器时钟选择 */
typedef enum 
{
    RCLK_2PRES    = 0x0, // 0x0：RCLK/2
    RCLK_4PRES    = 0x1, // 0x1：RCLK/4
    RCLK_8PRES    = 0x2, // 0x2：RCLK/8
    RCLK_16PRES   = 0x3, // 0x3：RCLK/16
    CK_SPRE_16BIT = 0x4, // 0x4，0x5：ck_spre，定时器为16位
    CK_SPRE_17BIT = 0x7  // 0x6，0x7：ck_spre，定时器为17位
}TYPE_ENUM_RCLK_PRES;

#define RTC_FLAG_USE_EXTERN_LXOSC32K     BIT(0)
void dsleep_rtc_calendar_init(uint32 time, uint32 date, uint32 flags);
void dsleep_rtc_alarm_cfg(uint32 time, uint8 msk);
void dsleep_rtc_calendar_read(RTC_TIMER_TYPEDEF *rtc_time);
void dsleep_bssid_set(void *ops, uint8 *bssid);
void dsleep_tim_tx_hdl(uint8 *element);
int32 dsleep_crc_valid(void);

//----------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif //__IRTCC_H__

/******************* (C) COPYRIGHT 2017 HUGE-IC *****END OF FILE****/
