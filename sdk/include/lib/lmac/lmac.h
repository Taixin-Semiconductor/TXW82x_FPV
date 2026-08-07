#ifndef _HGIC_LMAC_H_
#define _HGIC_LMAC_H_

#ifdef __cplusplus
extern "C" {
#endif

/************************************ for lmac rate ************************************/
#define LMAC_RATE_MCS_MASK                  0xFF
#define LMAC_RATE_MCS_SHIFT                 0
#define LMAC_RATE_NSS_MASK                  0x0F
#define LMAC_RATE_NSS_SHIFT                 8
#define LMAC_RATE_PHY_TYPE_MASK             0x3F
#define LMAC_RATE_PHY_TYPE_SHIFT            12

//PHY TYPE
#define LMAC_PHY_S1G                        0
#define LMAC_PHY_DSSS_CCK                   1
#define LMAC_PHY_NON_HT                     2
#define LMAC_PHY_HT                         3

//flags
#define LMAC_RATE_SHORT_PREAMBLE            (1 << 24)
#define LMAC_RATE_SHORT_GI                  (1 << 24)
#define LMAC_RATE_GREEN_FILED               (1 << 25)

//lmac rate define
#define LMAC_RATE_DEF(phy_type, nss, mcs, flags)            ((((phy_type) & LMAC_RATE_PHY_TYPE_MASK) << LMAC_RATE_PHY_TYPE_SHIFT) |    \
        (((nss) & LMAC_RATE_NSS_MASK) << LMAC_RATE_NSS_SHIFT)                |    \
        (((mcs) & LMAC_RATE_MCS_MASK) << LMAC_RATE_MCS_SHIFT) | (flags))
/***************************************************************************************/

//FEM chip support
#define LMAC_FEM_POWER_5V                       BIT(16)     //FEM uses 5V power supply
#define LMAC_FEM_NONE                           0
#define LMAC_FEM_GSR2401C                       1
#define LMAC_FEM_GSR2701                        2
#define LMAC_FEM_GSR2701_3_3V                   LMAC_FEM_GSR2701
#define LMAC_FEM_GSR2701_5V                     (LMAC_FEM_GSR2701_3_3V | LMAC_FEM_POWER_5V)
#define LMAC_FEM_KCT8227D_1_SWITCH              3
#define LMAC_FEM_KCT8227D_1_SWITCH_3_3V         LMAC_FEM_KCT8227D_1_SWITCH
#define LMAC_FEM_KCT8227D_1_SWITCH_5V           (LMAC_FEM_KCT8227D_1_SWITCH | LMAC_FEM_POWER_5V)
#define LMAC_FEM_KCT8227D_NON_SWITCH            4
#define LMAC_FEM_KCT8227D_NON_SWITCH_3_3V       LMAC_FEM_KCT8227D_NON_SWITCH
#define LMAC_FEM_KCT8227D_NON_SWITCH_5V         (LMAC_FEM_KCT8227D_NON_SWITCH | LMAC_FEM_POWER_5V)

//FREQ offset tracking mode
#define LMAC_FREQ_OFFSET_TRACK_ALWAYS_ON        0
#define LMAC_FREQ_OFFSET_TRACK_ALWAYS_OFF       1
#define LMAC_FREQ_OFFSET_TRACK_ONLY_STA_ON      2

struct lmac_txq_param {
    uint16 txop;
    uint16 cw_min;
    uint16 cw_max;
    uint8  aifs;
    uint8  acm;
};

struct lmac_init_param {
    uint32 rxbuf, rxbuf_size;
    uint32 tdma_buff, tdma_buff_size;
    uint32 uart0_sel : 1;
};

struct lmac_acs_ctl {
    uint32 enable: 1, scan_ms: 8, chn_bitmap: 23;
};

struct lmac_csa_scan_ctl {
    uint32      chn_bitmap;
    uint8       mode;
    uint8       rounds;         //The number of rounds to scan before switching channels.
    uint8       scan_time;      //Scan time per channel, in ms. Allowed range: 10~32.
    uint8       period;         //The number of beacons to pass before performing a channel scan.
    uint8       csa_cnt;        //CSA element count
    uint8       debug_en;       //enable debug print
};

struct lmac_cca_ctl {
    int8    start_th;
    int8    mid_th;
    int8    ed_th;
    /* CCA auto adjustment.
     * When the cca automatic adjustment takes effect, the
     * above three parameters are invalid.
     */
    uint8   auto_en     : 1;
};

struct lmac_wifi_ble_switch_ctl {
	 struct {
        uint8  	chan;       //for ble
        uint32  type;       //for ble
    } ble_priv;
    /* only work for ap */
    struct {
        uint8   switch_time;    //Switch time per beacon, in ms. Allowed range: 0~32.
        uint8   period;         //The number of beacons to pass before switching. 0 means no switching will take place.
        /* User-defined content that will be passed as a parameter to the hdl function pointer. */
        void   *arg;
        void (*hdl)(void *lmac_ops, void *arg);
    } ap_ctl;
};

struct lmac_fallback_type {
    uint8 type;
    uint8 mcs;
    uint8 gi;   //for HT: 0 for LGI, 1 for SGI
    uint8 bw;   //0 for 20M, 1 for 40M
};

struct lmac_fallback_ctl {
    struct lmac_fallback_type original;
    struct lmac_fallback_type fallback;
};

enum LMAC_IOCTL_CMD {
    /*Set CMDs*/
    LMAC_IOCTL_SET_AGGCNT = 0x1,
    LMAC_IOCTL_SET_BSS_BW,
    LMAC_IOCTL_SET_TX_BW,
    LMAC_IOCTL_SET_TX_MCS,
    LMAC_IOCTL_SET_RTS_TH,
    LMAC_IOCTL_SET_TXPOWER,
    LMAC_IOCTL_SET_BSS_MAX_IDLE,
    LMAC_IOCTL_SET_TX_ORDERED,
    LMAC_IOCTL_SET_AUTO_CHAN_SWITCH,
    LMAC_IOCTL_SET_PRI_CHAN,
    LMAC_IOCTL_SET_RADIO_ONOFF,
    LMAC_IOCTL_SET_PS_HEARBEAT,
    LMAC_IOCTL_SET_PS_HEARBEAT_RESP,
    LMAC_IOCTL_SET_PS_WAKEUP_DATA,
    LMAC_IOCTL_SET_PS_HEARBEAT_PERIOD,
    LMAC_IOCTL_SET_SSID,
    LMAC_IOCTL_SET_WAKEUPIO_MODE,
    LMAC_IOCTL_SET_PS_MODE,
    LMAC_IOCTL_SET_SLEEP_APLOST_TIME,
    LMAC_IOCTL_SET_SLEEP_WAKEIUP_IO,
    LMAC_IOCTL_SET_SUPER_PWR,
    LMAC_IOCTL_SET_PA_PWR_CTRL,
    LMAC_IOCTL_SET_STA_ROAMING,
    LMAC_IOCTL_SET_HW_SCAN,
    LMAC_IOCTL_SET_COMPRESSED_SSID,
    LMAC_IOCTL_SET_VDD13,
    LMAC_IOCTL_SET_ACK_TIMEOUT_EXTRA,
    LMAC_IOCTL_SET_TX_CNT_MAX,
    LMAC_IOCTL_SET_SLEEP_GPIOA_RESV,
    LMAC_IOCTL_SET_SLEEP_GPIOB_RESV,
    LMAC_IOCTL_SET_SLEEP_GPIOC_RESV,
    LMAC_IOCTL_SET_ENTER_SLEEP_CB,
    LMAC_IOCTL_SET_KEEP_ALIVE_CB,
    LMAC_IOCTL_SET_WAKE_UP_CB,
    LMAC_IOCTL_SET_PSCONNECT_PERIOD,
    LMAC_IOCTL_SET_MCAST_TX_RATE,
    LMAC_IOCTL_SET_MCAST_TXBW,
    LMAC_IOCTL_SET_MCAST_TXPOWER,
    LMAC_IOCTL_SET_MCAST_DUP_TXCNT,
    LMAC_IOCTL_SET_MCAST_CLEAR_CHN,
    LMAC_IOCTL_SET_ANT_CTRL_PIN,
    LMAC_IOCTL_SET_ANT_DUAL_EN,
    LMAC_IOCTL_SET_ANT_AUTO_EN,
    LMAC_IOCTL_SET_ANT_SEL,
    LMAC_IOCTL_SET_REASSOCIATION,
    LMAC_IOCTL_SET_APSLEEP_RC,
    LMAC_IOCTL_SET_BSSID,
    LMAC_IOCTL_SET_AP_PSMODE_EN,
    LMAC_IOCTL_SET_WKSRC_DETECT,
    LMAC_IOCTL_SET_RTC,
    LMAC_IOCTL_SET_STANDBY,
    LMAC_IOCTL_SET_WAIT_PSMODE,
    LMAC_IOCTL_SET_CCA_FOR_CE,
    LMAC_IOCTL_SET_RXG_OFFEST,
    LMAC_IOCTL_SET_OBSS_EN,
    LMAC_IOCTL_SET_OBSS_TH,
    LMAC_IOCTL_SET_OBSS_PER,
    LMAC_IOCTL_SET_BKN_TMO,
    LMAC_IOCTL_SET_AID,
    LMAC_IOCTL_SET_TXQ_PARAM,
    LMAC_IOCTL_SET_MAC_ADDR,
    LMAC_IOCTL_SET_MCAST_KEY,
    LMAC_IOCTL_DEL_MCAST_KEY,
    LMAC_IOCTL_SET_PROMISC_MODE,
    LMAC_IOCTL_SET_STA_SUPP_RATE,
    LMAC_IOCTL_SET_SUPP_RATE,
    LMAC_IOCTL_SET_MAX_STA_CNT,
    LMAC_IOCTL_SET_MAX_PS_FRAME,
    LMAC_IOCTL_SET_TX_DUTY_CYCLE,
    LMAC_IOCTL_SET_SSID_FILTER,
    LMAC_IOCTL_SET_WPHY_DPD,
    LMAC_IOCTL_SET_PREVENT_PS_MODE,
    LMAC_IOCTL_SET_CSA_SCAN_CFG,
    LMAC_IOCTL_SET_CSA_SCAN_START,
    LMAC_IOCTL_SET_CSA_CHN_SWITCH_NOW,
    LMAC_IOCTL_SET_TX_MODULATION_GAIN,
    LMAC_IOCTL_SET_TX_EDCA_SLOT_TIME,
    LMAC_IOCTL_SET_DBG_LEVEL,
    LMAC_IOCTL_SET_FIX_TX_RATE,
    LMAC_IOCTL_SET_NAV_MAX,
    LMAC_IOCTL_CLR_NAV,
    LMAC_IOCTL_SET_CCA,
    LMAC_IOCTL_SET_FRAG_TH,
    LMAC_IOCTL_SET_BEACON,
    LMAC_IOCTL_SET_RTS_FIX_DURATION,
    LMAC_IOCTL_SET_DSLEEP_BSS_MAX_IDLE,
    LMAC_IOCTL_SET_DSLEEP_WKIO_PIN,
    LMAC_IOCTL_SET_DSLEEP_WKIO_EDGE,
    LMAC_IOCTL_SET_DSLEEP_APLOST_TIME,
    LMAC_IOCTL_SET_DSLEEP_SENS_LEVEL,
    LMAC_IOCTL_SET_DSLEEP_PWM_SEL,
    LMAC_IOCTL_SET_DSLEEP_PWM_MODE,
    LMAC_IOCTL_SET_WIFI_BLE_SWITCH_CFG,
    LMAC_IOCTL_SET_WIFI_SWITCH_BLE,
    LMAC_IOCTL_SET_BLE_SWITCH_WIFI,
    LMAC_IOCTL_SET_RX_AGGCNT,
    LMAC_IOCTL_SET_APEP_PADDING,
    LMAC_IOCTL_SET_RETRY_FALLBACK_CNT,
    LMAC_IOCTL_SET_FALLBACK_MCS,
    LMAC_IOCTL_SET_XOSC,
    LMAC_IOCTL_SET_FREQ_CALI_PERIOD,
    LMAC_IOCTL_SET_MAX_TX_DELAY,
    LMAC_IOCTL_SET_RX_DUP_FILTER,
    LMAC_IOCTL_SET_RX_REORDER,
    LMAC_IOCTL_SET_RF_PWR_LEVEL,
    LMAC_IOCTL_SET_RC_MCS_MASK,
    LMAC_IOCTL_SET_RC_TYPE,
    LMAC_IOCTL_SET_PSRAM_USED,
    LMAC_IOCTL_SET_HW_HT_CAP,
    LMAC_IOCTL_SET_FEM,
    LMAC_SETCFG_SET_SLEEP_ROAMING,
    LMAC_IOCTL_SET_TX_DMA_WAIT_EMPTY,
    LMAC_IOCTL_SET_PWRLIMIT_MAX_LIMIT,
    LMAC_IOCTL_SET_PWRLIMIT_EN,
    LMAC_IOCTL_SET_PWRLIMIT_ACTUAL_PWR,
    LMAC_IOCTL_SET_PWRLIMIT_GAIN,
    LMAC_IOCTL_SET_PWRLIMIT_GAIN_TABLE,
    LMAC_IOCTL_SET_EDCA_MAX,
    LMAC_IOCTL_SET_TEMPERATURE_COMPESATE_TYPE,
    LMAC_IOCTL_SET_RX_DMA_KICK_THRESHOLD,
    LMAC_IOCTL_SET_RF_TX_DIGITAL_GAIN,
    LMAC_IOCTL_SET_STA_CONNECTED,
    LMAC_IOCTL_SET_FREQ_OFFSET_TRACK_MODE,
    LMAC_IOCTL_SET_TEMPERATURE_COMPESATE_EN,
    LMAC_IOCTL_SET_EDCA_MIN,
    LMAC_IOCTL_SET_HEARTBEAT_DATA,
    LMAC_IOCTL_SET_START_ASYNC_ACS,
    LMAC_IOCTL_SET_TEMP_COMPENSATION_TABLE,
    LMAC_IOCTL_SET_PS_NO_FRM_LOSS_EN,

    /*Get CMDs*/
    LMAC_IOCTL_GET_AGGCNT = 0x20000000,
    LMAC_IOCTL_GET_BSS_BW,
    LMAC_IOCTL_GET_TX_BW,
    LMAC_IOCTL_GET_TX_MCS,
    LMAC_IOCTL_GET_RTS_TH,
    LMAC_IOCTL_GET_TXPOWER,
    LMAC_IOCTL_GET_BSS_MAX_IDLE,
    LMAC_IOCTL_GET_TX_ORDERED,
    LMAC_IOCTL_GET_LO_FREQ,
    LMAC_IOCTL_GET_QA_BW,
    LMAC_IOCTL_GET_QA_MCS,
    LMAC_IOCTL_GET_QA_PER,
    LMAC_IOCTL_GET_AID,
    LMAC_IOCTL_GET_PRI_CHAN,
    LMAC_IOCTL_GET_MAC_ADDR,
    LMAC_IOCTL_GET_BSS_ID,
    LMAC_IOCTL_GET_BKN_INTERVAL,
    LMAC_IOCTL_GET_DTIM_PERIOD,
    LMAC_IOCTL_GET_SLEEP_USER_DATA_ADDR,
    LMAC_IOCTL_GET_SLEEP_USER_DATA_LEN,
    LMAC_IOCTL_GET_ANT_SEL,
    LMAC_IOCTL_GET_BSS_LOAD,
    LMAC_IOCTL_GET_RTC,
    LMAC_IOCTL_GET_BKN_TMO,
    LMAC_IOCTL_GET_SNR,
    LMAC_IOCTL_GET_BGR,
    LMAC_IOCTL_GET_PROMISC_MODE,
    LMAC_IOCTL_GET_STA_SUPP_RATE,
    LMAC_IOCTL_GET_SUPP_RATE,
    LMAC_IOCTL_GET_MAX_STA_CNT,
    LMAC_IOCTL_GET_BG_RSSI,
    LMAC_IOCTL_GET_TXQ_PARAM,
    LMAC_IOCTL_GET_STA_TX_MCS,
    LMAC_IOCTL_GET_NAV,
    LMAC_IOCTL_GET_STALIST,
    LMAC_IOCTL_GET_RTCC,
    LMAC_IOCTL_GET_RX_AGGCNT,
    LMAC_IOCTL_GET_ACS_RESULT,
    LMAC_IOCTL_GET_XOSC,
    LMAC_IOCTL_GET_FREQ_OFFSET,
    LMAC_IOCTL_GET_RF_DRV_CHECK,
    LMAC_IOCTL_GET_RC_MCS_MASK,
    LMAC_IOCTL_GET_RC_TYPE,
    LMAC_IOCTL_GET_MODULE_TYPE,
    LMAC_IOCTL_GET_ASYNC_ACS_RET,
    LMAC_IOCTL_GET_STA_PER,
    LMAC_IOCTL_GET_STA_TX_DELAY,   
    LMAC_IOCTL_GET_STA_TX_DATARATE,
};

enum LMAC_RATE {
    /*** for AH ***/
    LMAC_RATE_S1G_1_NSS_MCS0                = LMAC_RATE_DEF(LMAC_PHY_S1G, 1, 0, 0),
    LMAC_RATE_S1G_1_NSS_MCS1                = LMAC_RATE_DEF(LMAC_PHY_S1G, 1, 1, 0),
    LMAC_RATE_S1G_1_NSS_MCS2                = LMAC_RATE_DEF(LMAC_PHY_S1G, 1, 2, 0),
    LMAC_RATE_S1G_1_NSS_MCS3                = LMAC_RATE_DEF(LMAC_PHY_S1G, 1, 3, 0),
    LMAC_RATE_S1G_1_NSS_MCS4                = LMAC_RATE_DEF(LMAC_PHY_S1G, 1, 4, 0),
    LMAC_RATE_S1G_1_NSS_MCS5                = LMAC_RATE_DEF(LMAC_PHY_S1G, 1, 5, 0),
    LMAC_RATE_S1G_1_NSS_MCS6                = LMAC_RATE_DEF(LMAC_PHY_S1G, 1, 6, 0),
    LMAC_RATE_S1G_1_NSS_MCS7                = LMAC_RATE_DEF(LMAC_PHY_S1G, 1, 7, 0),
    LMAC_RATE_S1G_1_NSS_MCS10               = LMAC_RATE_DEF(LMAC_PHY_S1G, 1, 10, 0),

    LMAC_RATE_S1G_1_NSS_MCS0_SGI            = LMAC_RATE_DEF(LMAC_PHY_S1G, 1, 0, LMAC_RATE_SHORT_GI),
    LMAC_RATE_S1G_1_NSS_MCS1_SGI            = LMAC_RATE_DEF(LMAC_PHY_S1G, 1, 1, LMAC_RATE_SHORT_GI),
    LMAC_RATE_S1G_1_NSS_MCS2_SGI            = LMAC_RATE_DEF(LMAC_PHY_S1G, 1, 2, LMAC_RATE_SHORT_GI),
    LMAC_RATE_S1G_1_NSS_MCS3_SGI            = LMAC_RATE_DEF(LMAC_PHY_S1G, 1, 3, LMAC_RATE_SHORT_GI),
    LMAC_RATE_S1G_1_NSS_MCS4_SGI            = LMAC_RATE_DEF(LMAC_PHY_S1G, 1, 4, LMAC_RATE_SHORT_GI),
    LMAC_RATE_S1G_1_NSS_MCS5_SGI            = LMAC_RATE_DEF(LMAC_PHY_S1G, 1, 5, LMAC_RATE_SHORT_GI),
    LMAC_RATE_S1G_1_NSS_MCS6_SGI            = LMAC_RATE_DEF(LMAC_PHY_S1G, 1, 6, LMAC_RATE_SHORT_GI),
    LMAC_RATE_S1G_1_NSS_MCS7_SGI            = LMAC_RATE_DEF(LMAC_PHY_S1G, 1, 7, LMAC_RATE_SHORT_GI),
    LMAC_RATE_S1G_1_NSS_MCS10_SGI           = LMAC_RATE_DEF(LMAC_PHY_S1G, 1, 10, LMAC_RATE_SHORT_GI),

    /*** for BGN ***/
    //DSSS/CCK long preamble
    LMAC_RATE_DSSS_CCK_RATE0                = LMAC_RATE_DEF(LMAC_PHY_DSSS_CCK, 1, 0, 0),        //RATE=1Mbps
    LMAC_RATE_DSSS_CCK_RATE1                = LMAC_RATE_DEF(LMAC_PHY_DSSS_CCK, 1, 1, 0),        //RATE=2Mbps
    LMAC_RATE_DSSS_CCK_RATE2                = LMAC_RATE_DEF(LMAC_PHY_DSSS_CCK, 1, 2, 0),        //RATE=5.5Mbps
    LMAC_RATE_DSSS_CCK_RATE3                = LMAC_RATE_DEF(LMAC_PHY_DSSS_CCK, 1, 3, 0),        //RATE=11Mbps
    //DSSS/CCK short preamble
    LMAC_RATE_DSSS_CCK_RATE0_SHORT          = LMAC_RATE_DEF(LMAC_PHY_DSSS_CCK, 1, 1, LMAC_RATE_SHORT_PREAMBLE),        //RATE=2Mbps
    LMAC_RATE_DSSS_CCK_RATE1_SHORT          = LMAC_RATE_DEF(LMAC_PHY_DSSS_CCK, 1, 2, LMAC_RATE_SHORT_PREAMBLE),        //RATE=5.5Mbps
    LMAC_RATE_DSSS_CCK_RATE2_SHORT          = LMAC_RATE_DEF(LMAC_PHY_DSSS_CCK, 1, 3, LMAC_RATE_SHORT_PREAMBLE),        //RATE=11Mbps
    //NON HT
    LMAC_RATE_NON_HT_RATE0                  = LMAC_RATE_DEF(LMAC_PHY_NON_HT, 1, 0, 0),
    LMAC_RATE_NON_HT_RATE1                  = LMAC_RATE_DEF(LMAC_PHY_NON_HT, 1, 1, 0),
    LMAC_RATE_NON_HT_RATE2                  = LMAC_RATE_DEF(LMAC_PHY_NON_HT, 1, 2, 0),
    LMAC_RATE_NON_HT_RATE3                  = LMAC_RATE_DEF(LMAC_PHY_NON_HT, 1, 3, 0),
    LMAC_RATE_NON_HT_RATE4                  = LMAC_RATE_DEF(LMAC_PHY_NON_HT, 1, 4, 0),
    LMAC_RATE_NON_HT_RATE5                  = LMAC_RATE_DEF(LMAC_PHY_NON_HT, 1, 5, 0),
    LMAC_RATE_NON_HT_RATE6                  = LMAC_RATE_DEF(LMAC_PHY_NON_HT, 1, 6, 0),
    LMAC_RATE_NON_HT_RATE7                  = LMAC_RATE_DEF(LMAC_PHY_NON_HT, 1, 7, 0),
    //HT MF long GI
    LMAC_RATE_HT_MF_1_NSS_MCS0              = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 0, 0),
    LMAC_RATE_HT_MF_1_NSS_MCS1              = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 1, 0),
    LMAC_RATE_HT_MF_1_NSS_MCS2              = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 2, 0),
    LMAC_RATE_HT_MF_1_NSS_MCS3              = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 3, 0),
    LMAC_RATE_HT_MF_1_NSS_MCS4              = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 4, 0),
    LMAC_RATE_HT_MF_1_NSS_MCS5              = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 5, 0),
    LMAC_RATE_HT_MF_1_NSS_MCS6              = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 6, 0),
    LMAC_RATE_HT_MF_1_NSS_MCS7              = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 7, 0),
    //HT MF short GI
    LMAC_RATE_HT_MF_1_NSS_MCS0_SGI          = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 0, LMAC_RATE_SHORT_GI),
    LMAC_RATE_HT_MF_1_NSS_MCS1_SGI          = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 1, LMAC_RATE_SHORT_GI),
    LMAC_RATE_HT_MF_1_NSS_MCS2_SGI          = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 2, LMAC_RATE_SHORT_GI),
    LMAC_RATE_HT_MF_1_NSS_MCS3_SGI          = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 3, LMAC_RATE_SHORT_GI),
    LMAC_RATE_HT_MF_1_NSS_MCS4_SGI          = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 4, LMAC_RATE_SHORT_GI),
    LMAC_RATE_HT_MF_1_NSS_MCS5_SGI          = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 5, LMAC_RATE_SHORT_GI),
    LMAC_RATE_HT_MF_1_NSS_MCS6_SGI          = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 6, LMAC_RATE_SHORT_GI),
    LMAC_RATE_HT_MF_1_NSS_MCS7_SGI          = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 7, LMAC_RATE_SHORT_GI),
    //HT GF
    LMAC_RATE_HT_GF_1_NSS_MCS0              = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 0, LMAC_RATE_GREEN_FILED),
    LMAC_RATE_HT_GF_1_NSS_MCS1              = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 1, LMAC_RATE_GREEN_FILED),
    LMAC_RATE_HT_GF_1_NSS_MCS2              = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 2, LMAC_RATE_GREEN_FILED),
    LMAC_RATE_HT_GF_1_NSS_MCS3              = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 3, LMAC_RATE_GREEN_FILED),
    LMAC_RATE_HT_GF_1_NSS_MCS4              = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 4, LMAC_RATE_GREEN_FILED),
    LMAC_RATE_HT_GF_1_NSS_MCS5              = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 5, LMAC_RATE_GREEN_FILED),
    LMAC_RATE_HT_GF_1_NSS_MCS6              = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 6, LMAC_RATE_GREEN_FILED),
    LMAC_RATE_HT_GF_1_NSS_MCS7              = LMAC_RATE_DEF(LMAC_PHY_HT, 1, 7, LMAC_RATE_GREEN_FILED),
};

enum {
    LMAC_CSA_MODE_MANUAL    = 0,    //Active scanning with automatic frequency switching.
    LMAC_CSA_MODE_SEMI_AUTO = 1,    //Active scanning, but no automatic frequency switching.
    LMAC_CSA_MODE_AUTO      = 2,    //Neither active scanning nor automatic frequency switching.
};


#define lmac_set_aggcnt(ops, aggcnt)                        lmac_ioctl(ops, LMAC_IOCTL_SET_AGGCNT, (uint32)(aggcnt), 0)
#define lmac_get_aggcnt(ops)                                lmac_ioctl(ops, LMAC_IOCTL_GET_AGGCNT, 0, 0)
#define lmac_set_rx_aggcnt(ops, aggcnt)                     lmac_ioctl(ops, LMAC_IOCTL_SET_RX_AGGCNT, (uint32)(aggcnt), 0)
#define lmac_get_rx_aggcnt(ops)                             lmac_ioctl(ops, LMAC_IOCTL_GET_RX_AGGCNT, 0, 0)
#define lmac_set_bss_bw(ops, bss_bw)                        lmac_ioctl(ops, LMAC_IOCTL_SET_BSS_BW, (uint32)(bss_bw), 0)
#define lmac_get_bss_bw(ops)                                lmac_ioctl(ops, LMAC_IOCTL_GET_BSS_BW, 0, 0)
#define lmac_set_tx_bw(ops, tx_bw)                          lmac_ioctl(ops, LMAC_IOCTL_SET_TX_BW, (uint32)(tx_bw), 0)
#define lmac_get_tx_bw(ops)                                 lmac_ioctl(ops, LMAC_IOCTL_GET_TX_BW, 0, 0)
#define lmac_set_tx_mcs(ops, tx_mcs)                        lmac_ioctl(ops, LMAC_IOCTL_SET_TX_MCS, (uint32)(tx_mcs), 0)
#define lmac_get_tx_mcs(ops)                                lmac_ioctl(ops, LMAC_IOCTL_GET_TX_MCS, 0, 0)
#define lmac_set_rts(ops, rts_th)                           lmac_ioctl(ops, LMAC_IOCTL_SET_RTS_TH, (uint32)(rts_th), 0)
#define lmac_get_rts(ops)                                   lmac_ioctl(ops, LMAC_IOCTL_GET_RTS_TH, 0, 0)
#define lmac_set_txpower(ops, txpower)                      lmac_ioctl(ops, LMAC_IOCTL_SET_TXPOWER, (uint32)(txpower), 0)
#define lmac_get_txpower(ops)                               lmac_ioctl(ops, LMAC_IOCTL_GET_TXPOWER, 0, 0)
#define lmac_set_bss_max_idle(ops, max_idle_tu)             lmac_ioctl(ops, LMAC_IOCTL_SET_BSS_MAX_IDLE, (uint32)(max_idle_tu), 0)
#define lmac_get_bss_max_idle(ops)                          lmac_ioctl(ops, LMAC_IOCTL_GET_BSS_MAX_IDLE, 0, 0)
#define lmac_set_strictly_ordered(ops, enable)              lmac_ioctl(ops, LMAC_IOCTL_SET_TX_ORDERED, (uint32)(enable), 0)
#define lmac_get_strictly_ordered(ops)                      lmac_ioctl(ops, LMAC_IOCTL_GET_TX_ORDERED, 0, 0)
#define lmac_get_lo_freq(ops)                               lmac_ioctl(ops, LMAC_IOCTL_GET_LO_FREQ, 0, 0)
#define lmac_set_pri_chan(ops, prichan)                     lmac_ioctl(ops, LMAC_IOCTL_SET_PRI_CHAN, (uint32)(prichan), 0)
#define lmac_get_pri_chan(ops)                              lmac_ioctl(ops, LMAC_IOCTL_GET_PRI_CHAN, 0, 0)
#define lmac_set_aid(ops, ifidx, aid)                       lmac_ioctl(ops, LMAC_IOCTL_SET_AID, (uint32)(ifidx), (uint32)(aid))
#define lmac_get_aid(ops)                                   lmac_ioctl(ops, LMAC_IOCTL_GET_AID, 0, 0)
#define lmac_set_ssid(ops, ssid)                            lmac_ioctl(ops, LMAC_IOCTL_SET_SSID, (uint32)(ssid), 32)
#define lmac_set_wkio_mode(ops, mode)                       lmac_ioctl(ops, LMAC_IOCTL_SET_WAKEUPIO_MODE, (uint32)(mode), 0)
#define lmac_set_ps_mode(ops, mode)                         lmac_ioctl(ops, LMAC_IOCTL_SET_PS_MODE, (uint32)(mode), 0)
#define lmac_set_sleep_aplost_time(ops, time)               lmac_ioctl(ops, LMAC_IOCTL_SET_SLEEP_APLOST_TIME, (uint32)(time), 0)
#define lmac_set_auto_chan_switch(ops, disable)             lmac_ioctl(ops, LMAC_IOCTL_SET_AUTO_CHAN_SWITCH, (uint32)(disable), 0)
#define lmac_set_wakeup_io(ops, io, edge)                   lmac_ioctl(ops, LMAC_IOCTL_SET_SLEEP_WAKEIUP_IO, (uint32)(io), (uint32)(edge))
#define lmac_set_super_pwr(ops, en)                         lmac_ioctl(ops, LMAC_IOCTL_SET_SUPER_PWR, (uint32)(en), 0)
#define lmac_set_pa_pwr_ctrl(ops, en)                       lmac_ioctl(ops, LMAC_IOCTL_SET_PA_PWR_CTRL, (uint32)(en), 0)
#define lmac_set_sta_roaming(ops, addr, en)                 lmac_ioctl(ops, LMAC_IOCTL_SET_STA_ROAMING, (uint32)(addr), (en))
#define lmac_set_hw_scan(ops, period, chan)                 lmac_ioctl(ops, LMAC_IOCTL_SET_HW_SCAN, (uint32)(period), (chan))
#define lmac_set_vdd13(ops, mode)                           lmac_ioctl(ops, LMAC_IOCTL_SET_VDD13, (uint32)(mode), 0)
#define lmac_set_ack_timeout_extra(ops, val)                lmac_ioctl(ops, LMAC_IOCTL_SET_ACK_TIMEOUT_EXTRA, (uint32)(val), 0)
#define lmac_set_retry_cnt(ops, frm_max, rts_max)           lmac_ioctl(ops, LMAC_IOCTL_SET_TX_CNT_MAX, (uint32)(frm_max), (uint32)(rts_max))
#define lmac_set_psconnect_period(ops, period)              lmac_ioctl(ops, LMAC_IOCTL_SET_PSCONNECT_PERIOD, (uint32)(period), 0)
#define lmac_set_mcast_txmcs(ops, rate)                     lmac_ioctl(ops, LMAC_IOCTL_SET_MCAST_TX_RATE, (uint32)(rate), 0)
#define lmac_set_mcast_txbw(ops, txbw)                      lmac_ioctl(ops, LMAC_IOCTL_SET_MCAST_TXBW, (uint32)(txbw), 0)
#define lmac_set_mcast_txpower(ops, txpwr)                  lmac_ioctl(ops, LMAC_IOCTL_SET_MCAST_TXPOWER, (uint32)(txpwr), 0)
#define lmac_set_mcast_dup_txcnt(ops, dupcnt)               lmac_ioctl(ops, LMAC_IOCTL_SET_MCAST_DUP_TXCNT, (uint32)(dupcnt), 0)
#define lmac_set_mcast_clear_chn(ops, en)                   lmac_ioctl(ops, LMAC_IOCTL_SET_MCAST_CLEAR_CHN, (uint32)(en), 0)
#define lmac_set_ant_dual_en(ops, en)                       lmac_ioctl(ops, LMAC_IOCTL_SET_ANT_DUAL_EN, (uint32)(en), 0)
#define lmac_set_ant_ctrl_pin(ops, pin)                     lmac_ioctl(ops, LMAC_IOCTL_SET_ANT_CTRL_PIN, (uint32)(pin),0)
#define lmac_set_ant_auto_en(ops, en)                       lmac_ioctl(ops, LMAC_IOCTL_SET_ANT_AUTO_EN, (uint32)(en), 0)
#define lmac_set_ant_sel(ops, sel)                          lmac_ioctl(ops, LMAC_IOCTL_SET_ANT_SEL, (uint32)(sel), 0)
#define lmac_get_ant_sel(ops)                               lmac_ioctl(ops, LMAC_IOCTL_GET_ANT_SEL, 0, 0)
#define lmac_set_bssid(ops, bssid)                          lmac_ioctl(ops, LMAC_IOCTL_SET_BSSID, (uint32)(bssid), 0)
#define lmac_set_ap_psmode_en(ops, en)                      lmac_ioctl(ops, LMAC_IOCTL_SET_AP_PSMODE_EN, (uint32)(en), 0)
#define lmac_set_wksrc_detect(ops, io, level)               lmac_ioctl(ops, LMAC_IOCTL_SET_WKSRC_DETECT, (uint32)(io), (uint32)(level))
#define lmac_set_rtc(ops, rtc)                              lmac_ioctl(ops, LMAC_IOCTL_SET_RTC, (uint32)(rtc), 0)
#define lmac_get_rtc(ops)                                   lmac_ioctl(ops, LMAC_IOCTL_GET_RTC, 0, 0)
#define lmac_set_bkn_tmo(ops, val)                          lmac_ioctl(ops, LMAC_IOCTL_SET_BKN_TMO, (val), 0)
#define lmac_get_bkn_tmo(ops)                               lmac_ioctl(ops, LMAC_IOCTL_GET_BKN_TMO, 0, 0)
#define lmac_get_sta_snr(ops, aid, snr)                     lmac_ioctl(ops, LMAC_IOCTL_GET_SNR, (aid), (snr))
#define lmac_set_reassociation(ops, aid, addr)              lmac_ioctl(ops, LMAC_IOCTL_SET_REASSOCIATION, (uint32)(aid), (uint32)(addr))
#define lmac_get_bss_load(ops, bss_load)                    lmac_ioctl(ops, LMAC_IOCTL_GET_BSS_LOAD, (uint32)(bss_load), 0)
#define lmac_set_sleep_gpioa_resv(ops, resv)                lmac_ioctl(ops, LMAC_IOCTL_SET_SLEEP_GPIOA_RESV, (uint32)(resv), 0)
#define lmac_set_sleep_gpiob_resv(ops, resv)                lmac_ioctl(ops, LMAC_IOCTL_SET_SLEEP_GPIOB_RESV, (uint32)(resv), 0)
#define lmac_set_sleep_gpioc_resv(ops, resv)                lmac_ioctl(ops, LMAC_IOCTL_SET_SLEEP_GPIOC_RESV, (uint32)(resv), 0)
#define lmac_get_sleep_user_data_addr(ops)                  lmac_ioctl(ops, LMAC_IOCTL_GET_SLEEP_USER_DATA_ADDR, 0, 0)
#define lmac_get_sleep_user_data_len(ops)                   lmac_ioctl(ops, LMAC_IOCTL_GET_SLEEP_USER_DATA_LEN, 0, 0)
#define lmac_set_enter_sleep_cb(ops, func)                  lmac_ioctl(ops, LMAC_IOCTL_SET_ENTER_SLEEP_CB, (uint32)(func), 0)
#define lmac_set_keep_alive_cb(ops, func)                   lmac_ioctl(ops, LMAC_IOCTL_SET_KEEP_ALIVE_CB, (uint32)(func), 0)
#define lmac_set_wake_up_cb(ops, func)                      lmac_ioctl(ops, LMAC_IOCTL_SET_WAKE_UP_CB, (uint32)(func), 0)
#define lmac_radio_onoff(ops, onoff)                        lmac_ioctl(ops, LMAC_IOCTL_SET_RADIO_ONOFF, (uint32)(onoff), 0)
#define lmac_get_qa_bw(ops)                                 lmac_ioctl(ops, LMAC_IOCTL_GET_QA_BW, 0, 0)
#define lmac_get_qa_mcs(ops)                                lmac_ioctl(ops, LMAC_IOCTL_GET_QA_MCS, 0, 0)
#define lmac_get_qa_per(ops)                                lmac_ioctl(ops, LMAC_IOCTL_GET_QA_PER, 0, 0)
#define lmac_set_ps_heartbeat(ops, ip, port)                lmac_ioctl(ops, LMAC_IOCTL_SET_PS_HEARBEAT, (uint32)(ip), (uint32)(port))
#define lmac_set_ps_heartbeat_resp(ops, data, size)         lmac_ioctl(ops, LMAC_IOCTL_SET_PS_HEARBEAT_RESP, (uint32)(data), (size))
#define lmac_set_ps_wakeup_data(ops, data, size)            lmac_ioctl(ops, LMAC_IOCTL_SET_PS_WAKEUP_DATA, (uint32)(data), (size))
#define lmac_set_ps_heartbeat_period(ops, period)           lmac_ioctl(ops, LMAC_IOCTL_SET_PS_HEARBEAT_PERIOD, (uint32)(period), 0)
#define lmac_set_mcast_key(ops, key, key_len)               lmac_ioctl(ops, LMAC_IOCTL_SET_MCAST_KEY, (uint32)(key), (key_len))
#define lmac_set_txq_param(ops, ifidx, txq, param)          lmac_ioctl(ops, LMAC_IOCTL_SET_TXQ_PARAM, (uint32)((ifidx)<<16|(txq)), (uint32)(param))
#define lmac_get_txq_param(ops, ifidx, txq, param)          lmac_ioctl(ops, LMAC_IOCTL_GET_TXQ_PARAM, (uint32)((ifidx)<<16|(txq)), (uint32)(param))
#define lmac_set_mac_addr(ops, ifidx, addr)                 lmac_ioctl(ops, LMAC_IOCTL_SET_MAC_ADDR, (uint32)(ifidx), (uint32)(addr))
#define lmac_set_promisc_mode(ops, enable)                  lmac_ioctl(ops, LMAC_IOCTL_SET_PROMISC_MODE, (enable), 0)
#define lmac_get_promisc_mode(ops)                          lmac_ioctl(ops, LMAC_IOCTL_GET_PROMISC_MODE, 0, 0)
#define lmac_set_sta_supp_rate(ops, addr, supp_rate)        lmac_ioctl(ops, LMAC_IOCTL_SET_STA_SUPP_RATE, (uint32)(addr), (supp_rate))
#define lmac_get_sta_supp_rate(ops, addr)                   lmac_ioctl(ops, LMAC_IOCTL_GET_STA_SUPP_RATE, (uint32 *)(addr), 0)
#define lmac_set_supp_rate(ops, supp_rate)                  lmac_ioctl(ops, LMAC_IOCTL_SET_SUPP_RATE, (supp_rate), 0)
#define lmac_get_supp_rate(ops)                             lmac_ioctl(ops, LMAC_IOCTL_GET_SUPP_RATE, 0, 0)
#define lmac_set_max_sta_cnt(ops, cnt)                      lmac_ioctl(ops, LMAC_IOCTL_SET_MAX_STA_CNT, (cnt), 0)
#define lmac_get_max_sta_cnt(ops)                           lmac_ioctl(ops, LMAC_IOCTL_GET_MAX_STA_CNT, 0, 0)
#define lmac_set_max_ps_frame(ops, cnt)                     lmac_ioctl(ops, LMAC_IOCTL_SET_MAX_PS_FRAME, (cnt), 0)
#define lmac_set_tx_duty_cycle(ops, duty)                   lmac_ioctl(ops, LMAC_IOCTL_SET_TX_DUTY_CYCLE, (duty), 0)
#define lmac_set_ssid_filter(ops, ssid, len)                lmac_ioctl(ops, LMAC_IOCTL_SET_SSID_FILTER, (ssid), (len))
#define lmac_get_bg_rssi(ops, chn, bgr)                     lmac_ioctl(ops, LMAC_IOCTL_GET_BG_RSSI, (chn), (uint32)(bgr))
#define lmac_set_wphy_dpd(ops, param1)                      lmac_ioctl(ops, LMAC_IOCTL_SET_WPHY_DPD, (param1), 0)
#define lmac_set_prevent_ps_mode(ops, enable)               lmac_ioctl(ops, LMAC_IOCTL_SET_PREVENT_PS_MODE, (enable), 0)
#define lmac_set_csa_scan_cfg(ops, cfg)                     lmac_ioctl(ops, LMAC_IOCTL_SET_CSA_SCAN_CFG, (uint32)(cfg), 0)
#define lmac_set_csa_chn_switch_now(ops)                    lmac_ioctl(ops, LMAC_IOCTL_SET_CSA_CHN_SWITCH_NOW, 0, 0)
#define lmac_set_csa_scan_start(ops)                        lmac_ioctl(ops, LMAC_IOCTL_SET_CSA_SCAN_START, 1, 0)
#define lmac_set_csa_scan_stop(ops)                         lmac_ioctl(ops, LMAC_IOCTL_SET_CSA_SCAN_START, 0, 0)
#define lmac_set_tx_modulation_gain(ops, table, size)       lmac_ioctl(ops, LMAC_IOCTL_SET_TX_MODULATION_GAIN, (uint32)(table), (size))
#define lmac_set_tx_edca_slot_time(ops, slot_us)            lmac_ioctl(ops, LMAC_IOCTL_SET_TX_EDCA_SLOT_TIME, (uint32)(slot_us), 0)
#define lmac_get_sta_txmcs(ops, addr)                       lmac_ioctl(ops, LMAC_IOCTL_GET_STA_TX_MCS, (uint32)(addr), 0)
#define lmac_set_dbg_levle(ops, level)                      lmac_ioctl(ops, LMAC_IOCTL_SET_DBG_LEVEL, (uint32)(level), 0)
#define lmac_set_fix_tx_rate(ops, rate)                     lmac_ioctl(ops, LMAC_IOCTL_SET_FIX_TX_RATE, (uint32)(rate), 0)
#define lmac_set_nav_max(ops, max)                          lmac_ioctl(ops, LMAC_IOCTL_SET_NAV_MAX, (uint32)(max), 0)
#define lmac_clr_nav(ops)                                   lmac_ioctl(ops, LMAC_IOCTL_CLR_NAV, 0, 0)
#define lmac_get_nav(ops)                                   lmac_ioctl(ops, LMAC_IOCTL_GET_NAV, 0, 0)
#define lmac_set_cca(ops, p_cca_ctl)                        lmac_ioctl(ops, LMAC_IOCTL_SET_CCA, (uint32)(p_cca_ctl), 0)
#define lmac_set_frag_threshold(ops, frag)                  lmac_ioctl(ops, LMAC_IOCTL_SET_FRAG_TH, (uint32)(frag), 0)
#define lmac_set_beacon_start(ops, start)                   lmac_ioctl(ops, LMAC_IOCTL_SET_BEACON, (uint32)(start), 0)
#define lmac_get_sta_list(ops, stas, count)                 lmac_ioctl(ops, LMAC_IOCTL_GET_STALIST, (uint32)(stas), (count))
#define lmac_set_rts_fix_duration(ops, duration_us)         lmac_ioctl(ops, LMAC_IOCTL_SET_RTS_FIX_DURATION, (int32)(duration_us), 0)
#define lmac_set_dsleep_max_idle(ops, max_idle_sec)         lmac_ioctl(ops, LMAC_IOCTL_SET_DSLEEP_BSS_MAX_IDLE, (uint32)(max_idle_sec), 0)
#define lmac_set_dsleep_wkio_pin(ops, pin_idx, pin_num)     lmac_ioctl(ops, LMAC_IOCTL_SET_DSLEEP_WKIO_PIN, (uint32)(pin_idx), (uint32)(pin_num))
#define lmac_set_dsleep_wkio_edge(ops, pin_idx, neg_edge)   lmac_ioctl(ops, LMAC_IOCTL_SET_DSLEEP_WKIO_EDGE, (uint32)(pin_idx), (uint32)(neg_edge))
#define lmac_set_dsleep_aplost_time(ops, t_sec)             lmac_ioctl(ops, LMAC_IOCTL_SET_DSLEEP_APLOST_TIME, (uint32)(t_sec), 0)
#define lmac_set_dsleep_sens_level(ops, sens_level)         lmac_ioctl(ops, LMAC_IOCTL_SET_DSLEEP_SENS_LEVEL, (uint32)(sens_level), 0)
#define lmac_set_dsleep_pwm_sel(ops, pwm_idx, pin_num)      lmac_ioctl(ops, LMAC_IOCTL_SET_DSLEEP_PWM_SEL, (uint32)(pwm_idx), (pin_num))
#define lmac_set_dsleep_pwm_mode(ops, pwm_idx, mode)        lmac_ioctl(ops, LMAC_IOCTL_SET_DSLEEP_PWM_MODE, (uint32)(pwm_idx), (mode))
#define lmac_set_wifi_ble_switch_cfg(ops, cfg)              lmac_ioctl(ops, LMAC_IOCTL_SET_WIFI_BLE_SWITCH_CFG, (uint32)(cfg), 0)
#define lmac_wifi_switch_ble(ops)                           lmac_ioctl(ops, LMAC_IOCTL_SET_WIFI_SWITCH_BLE, 0, 0)
#define lmac_ble_switch_wifi(ops)                           lmac_ioctl(ops, LMAC_IOCTL_SET_BLE_SWITCH_WIFI, 0, 0)
#define lmac_get_rtcc(ops)                                  lmac_ioctl(ops, LMAC_IOCTL_GET_RTCC, 0, 0)
#define lmac_set_apep_padding(ops, en)                      lmac_ioctl(ops, LMAC_IOCTL_SET_APEP_PADDING, (int32)(en), 0)
#define lmac_set_standby(ops, chn, time)                    lmac_ioctl(ops, LMAC_IOCTL_SET_STANDBY, (chn), (time))
#define lmac_set_wait_psmode(ops, mode)                     lmac_ioctl(ops, LMAC_IOCTL_SET_WAIT_PSMODE, (mode), 0)
#define lmac_set_cca_for_ce(ops, en)                        lmac_ioctl(ops, LMAC_IOCTL_SET_CCA_FOR_CE, (en), 0)
#define lmac_set_rxg_offest(ops, offset)                    lmac_ioctl(ops, LMAC_IOCTL_SET_RXG_OFFEST, (offset), 0)
#define lmac_set_obss_en(ops, en)                           lmac_ioctl(ops, LMAC_IOCTL_SET_OBSS_EN, (en), 0)
#define lmac_set_obss_th(ops, th)                           lmac_ioctl(ops, LMAC_IOCTL_SET_OBSS_TH, (th), 0)
#define lmac_set_obss_per(ops, per)                         lmac_ioctl(ops, LMAC_IOCTL_SET_OBSS_PER, (per), 0)
#define lmac_get_acs_result(ops, result)                    lmac_ioctl(ops, LMAC_IOCTL_GET_ACS_RESULT, (uint32)(result), 0)
#define lmac_set_retry_fallback_cnt(ops, cnt)               lmac_ioctl(ops, LMAC_IOCTL_SET_RETRY_FALLBACK_CNT, (uint32)(cnt), 0)
#define lmac_set_fallback_mcs(ops, p_fb_ctl)                lmac_ioctl(ops, LMAC_IOCTL_SET_FALLBACK_MCS, (uint32)(p_fb_ctl), 0)
#define lmac_get_xosc(ops)                                  lmac_ioctl(ops, LMAC_IOCTL_GET_XOSC, 0, 0)
#define lmac_get_freq_offset(ops, addr)                     lmac_ioctl(ops, LMAC_IOCTL_GET_FREQ_OFFSET, (uint32)(addr), 0)
#define lmac_set_xosc(ops, xosc)                            lmac_ioctl(ops, LMAC_IOCTL_SET_XOSC, (uint32)(xosc), 0)
#define lmac_set_freq_cali_period(ops, period)              lmac_ioctl(ops, LMAC_IOCTL_SET_FREQ_CALI_PERIOD, (uint32)(period), 0)
#define lmac_set_max_tx_delay(ops, time_ms)                 lmac_ioctl(ops, LMAC_IOCTL_SET_MAX_TX_DELAY, (uint32)(time_ms), 0)
#define lmac_set_rx_dup_filter(ops, en)                     lmac_ioctl(ops, LMAC_IOCTL_SET_RX_DUP_FILTER, (uint32)(en), 0)
#define lmac_set_rx_reorder(ops, en, tmo_ms)                lmac_ioctl(ops, LMAC_IOCTL_SET_RX_REORDER, (uint32)(en), (uint32)(tmo_ms))
#define lmac_set_rf_pwr_level(ops, level)                   lmac_ioctl(ops, LMAC_IOCTL_SET_RF_PWR_LEVEL, (uint32)(level), 0)
#define lmac_rf_drv_check(ops, id_ptr)                      lmac_ioctl(ops, LMAC_IOCTL_GET_RF_DRV_CHECK, (uint32)(id_ptr), 0)
#define lmac_rate_ctrl_mcs_mask(ops, mcs_mask)              lmac_ioctl(ops, LMAC_IOCTL_SET_RC_MCS_MASK, (uint32)(mcs_mask), 0)
#define lmac_rate_ctrl_type(ops, type)                      lmac_ioctl(ops, LMAC_IOCTL_SET_RC_TYPE, (uint32)(type), 0)
#define lmac_set_psram_used(ops, tx_used, rx_used)          lmac_ioctl(ops, LMAC_IOCTL_SET_PSRAM_USED, (uint32)((tx_used) | ((rx_used) << 16)), 0)
#define lmac_set_hw_ht_cap(ops, ht_cap)                     lmac_ioctl(ops, LMAC_IOCTL_SET_HW_HT_CAP, (uint32)(ht_cap), 0)
#define lmac_set_fem(ops, fem_chip)                         lmac_ioctl(ops, LMAC_IOCTL_SET_FEM, (uint32)(fem_chip), 0)
#define lmac_get_module_type(ops)                           lmac_ioctl(ops, LMAC_IOCTL_GET_MODULE_TYPE, 0, 0)
#define lmac_set_sleep_roaming(ops, en, rssi)               lmac_ioctl(ops, LMAC_SETCFG_SET_SLEEP_ROAMING, en, rssi)
#define lmac_set_tx_dma_wait_empty(ops, wait_empty)         lmac_ioctl(ops, LMAC_IOCTL_SET_TX_DMA_WAIT_EMPTY, (uint32)(wait_empty), 0)
#define lmac_set_pwrlimit_max_limit(ops, max_mdbm)          lmac_ioctl(ops, LMAC_IOCTL_SET_PWRLIMIT_MAX_LIMIT, (uint32)(max_mdbm), 0)
#define lmac_set_pwrlimit_en(ops, en)                       lmac_ioctl(ops, LMAC_IOCTL_SET_PWRLIMIT_EN, (uint32)(en), 0)
#define lmac_set_pwrlimit_actual_pwr(ops, type, mcs, mdbm)  lmac_ioctl(ops, LMAC_IOCTL_SET_PWRLIMIT_ACTUAL_PWR, (uint32)(((type)<<16)|(mcs)), (uint32)(mdbm))
#define lmac_set_pwrlimit_gain(ops, type, mcs, gain)        lmac_ioctl(ops, LMAC_IOCTL_SET_PWRLIMIT_GAIN, (uint32)(((type)<<16)|(mcs)), (uint32)(gain))
#define lmac_set_pwrlimit_gain_table(ops, gain_table, size) lmac_ioctl(ops, LMAC_IOCTL_SET_PWRLIMIT_GAIN_TABLE, (uint32)(gain_table), (uint32)(size))
#define lmac_set_edca_max(ops, p_txq_param)                 lmac_ioctl(ops, LMAC_IOCTL_SET_EDCA_MAX, (uint32)(p_txq_param), 0)
#define lmac_set_temperature_compesate_type(ops, type)      lmac_ioctl(ops, LMAC_IOCTL_SET_TEMPERATURE_COMPESATE_TYPE, (uint32)(type), 0)
#define lmac_set_rx_dma_kick_threshold(ops, size_th)        lmac_ioctl(ops, LMAC_IOCTL_SET_RX_DMA_KICK_THRESHOLD, (uint32)(size_th), 0)
#define lmac_set_rf_tx_digital_gain(ops, gain)              lmac_ioctl(ops, LMAC_IOCTL_SET_RF_TX_DIGITAL_GAIN, (uint32)(gain), 0)
#define lmac_set_sta_connected(ops, addr, connected)        lmac_ioctl(ops, LMAC_IOCTL_SET_STA_CONNECTED, (uint32)(addr), connected)
#define lmac_set_freq_offset_track_mode(ops, mode)          lmac_ioctl(ops, LMAC_IOCTL_SET_FREQ_OFFSET_TRACK_MODE, (uint32)(mode), 0)
#define lmac_set_temperature_compesate_en(ops, en)          lmac_ioctl(ops, LMAC_IOCTL_SET_TEMPERATURE_COMPESATE_EN, (uint32)(en), 0)
#define lmac_set_edca_min(ops, p_txq_param)                 lmac_ioctl(ops, LMAC_IOCTL_SET_EDCA_MIN, (uint32)(p_txq_param), 0)
#define lmac_set_heartbeat_data(ops, addr, retry)           lmac_ioctl(ops, LMAC_IOCTL_SET_HEARTBEAT_DATA, (uint32)(addr), retry)
#define lmac_start_async_acs(ops, p_ctl)                    lmac_ioctl(ops, LMAC_IOCTL_SET_START_ASYNC_ACS, (uint32)(p_ctl), 0)
#define lmac_get_async_acs_ret(ops)                         lmac_ioctl(ops, LMAC_IOCTL_GET_ASYNC_ACS_RET, 0, 0)
#define lmac_set_temp_compensation_table(ops, table, size)  lmac_ioctl(ops, LMAC_IOCTL_SET_TEMP_COMPENSATION_TABLE, (uint32)(table), size)
#define lmac_set_ps_no_frm_loss(ops, en)                    lmac_ioctl(ops, LMAC_IOCTL_SET_PS_NO_FRM_LOSS_EN, (uint32)(en), 0)
#define lmac_get_per(ops, mac)                              lmac_ioctl(ops, LMAC_IOCTL_GET_STA_PER, (uint32)mac, 0)
#define lmac_get_tx_delay(ops, mac)                         lmac_ioctl(ops, LMAC_IOCTL_GET_STA_TX_DELAY, (uint32)mac, 0)
#define lmac_get_tx_datarate(ops, mac)                      lmac_ioctl(ops, LMAC_IOCTL_GET_STA_TX_DATARATE, (uint32)mac, 0)


int32 lmac_ioctl(void *ops, uint32 cmd, uint32 param1, uint32 param2);
int32 lmac_start_acs(void *lops, struct lmac_acs_ctl *p_ctl, uint32 sync);
int32 lmac_set_chan_list(void *lops, uint16 *chan_list, uint16 count);
void *lmac_ah_init(struct lmac_init_param *param);
void *lmac_bgn_init(struct lmac_init_param *param);
//bgn module
int32 lmac_bgn_module_80211w_init(void *ops);
int32 lmac_bgn_module_csa_init(void *ops);
int32 lmac_bgn_module_multi_mac_init(void *ops);
int32 lmac_bgn_module_pwr_limit_init(void *ops);
void *dsleep_lmac_bgn_init(void);

#ifdef __cplusplus
}
#endif

#endif
