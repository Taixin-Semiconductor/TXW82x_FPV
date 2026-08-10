#ifndef _HGIC_IEEE80211_H_
#define _HGIC_IEEE80211_H_

#include "lib/lmac/hgic.h"
#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_SAE) || defined(CONFIG_OWE)
#define CONFIG_ECC
#define sae_dbg(fmt, ...) //os_printf("%s:%d:: __SAE:"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define sae_dbg(fmt, ...)
#endif

/* IEEE 802.11r 快速漫游相关常量 */
#define MOBILITY_DOMAIN_ID_LEN 2   /**< 移动域标识符长度（字节） */
#define FT_R0KH_ID_MAX_LEN 48      /**< R0密钥持有者标识符最大长度 */
#define FT_R1KH_ID_LEN 6           /**< R1密钥持有者标识符长度（字节） */
#define WPA_PMK_NAME_LEN 16        /**< PMK名称长度（字节） */

/**
 * @brief WPA密钥管理类型（加密模式）的位掩码定义
 * @note 用于指示WiFi连接使用的认证和密钥管理方式
 */
#define WPA_KEY_MGMT_PSK        BIT(1)   /**< WPA-PSK 预共享密钥模式（个人模式） */
#define WPA_KEY_MGMT_NONE       BIT(2)   /**< 无加密（开放网络） */
#define WPA_KEY_MGMT_PSK_SHA256 BIT(8)   /**< 使用SHA256的WPA-PSK（更安全的PSK） */
#define WPA_KEY_MGMT_SAE        BIT(10)  /**< SAE 同时认证对等（WPA3个人版） */
#define WPA_KEY_MGMT_OWE        BIT(22)  /**< OWE 机会无线加密（增强开放网络） */

/**
 * @brief WPA加密算法（数据加密方式）的位掩码定义
 */
#define WPA_CIPHER_NONE         BIT(0)   /**< 无加密 */
#define WPA_CIPHER_TKIP         BIT(3)   /**< TKIP 临时密钥完整性协议（WPA1旧标准） */
#define WPA_CIPHER_CCMP         BIT(4)   /**< CCMP AES加密（WPA2主流标准） */
#define WPA_CIPHER_CCMP_256     BIT(9)   /**< CCMP-256 使用256位密钥的AES加密 */

/**
 * @brief WPA协议版本位掩码
 */
#define WPA_PROTO_WPA           BIT(0)   /**< WPA版本1 */
#define WPA_PROTO_RSN           BIT(1)   /**< WPA2/RSN版本 */

/**
 * @brief WiFi无线频段枚举
 * @note 常见2.4GHz和5GHz，其他为特殊频段
 */
enum IEEE80211_BAND {
    IEEE80211_BAND_2GHZ,   /**< 2.4GHz频段（最常用，穿墙能力强） */
    IEEE80211_BAND_5GHZ,   /**< 5GHz频段（干扰少，速率高） */
    IEEE80211_BAND_60GHZ,  /**< 60GHz频段（超高速短距离） */
    IEEE80211_BAND_S1GHZ,  /**< Sub-1GHz频段（物联网远距离） */

    NUM_IEEE80211_BANDS,   /**< 频段总数 */
};

/**
 * @brief WiFi协议模式（支持的802.11标准版本）
 * @note 决定设备能够使用的速率和特性
 */
enum ieee80211_hwmode {
    IEEE80211_HWMODE_NONE,   /**< 未指定模式 */
    IEEE80211_HWMODE_11B,    /**< 仅支持802.11b（2.4GHz，最高11Mbps） */
    IEEE80211_HWMODE_11G,    /**< 混合802.11b/g（2.4GHz，最高54Mbps） */
    IEEE80211_HWMODE_11N,    /**< 混合802.11b/g/n（2.4GHz，支持MIMO，最高300Mbps以上） */
    IEEE80211_HWMODE_11AH,   /**< 802.11ah（Sub-1GHz，低功耗长距离） */
};

/**
 * @brief 国家/地区代码枚举
 * @note 不同国家WiFi信道和功率法规不同
 */
enum country_code {
    COUNTRY_US,   /**< 美国 */
    COUNTRY_EU,   /**< 欧洲 */
    COUNTRY_JP,   /**< 日本 */
    COUNTRY_CN,   /**< 中国 */
    COUNTRY_KR,   /**< 韩国 */
    COUNTRY_SG,   /**< 新加坡 */
    COUNTRY_AU,   /**< 澳大利亚 */
    COUNTRY_NZ,   /**< 新西兰 */

    NUM_COUNTRIES,
};

/**
 * @brief WiFi协议栈事件枚举
 * @note 所有事件通过 ieee80211_evt_cb 回调函数上报，param1和param2为事件参数
 * @see ieee80211_evt_cb
 * @note 应用程序需要注册回调函数才能收到这些事件通知
 */
enum ieee80211_event {
    IEEE80211_EVENT_PAIR_START = 1,     /**< 开始一键配对，无参数 */
    IEEE80211_EVENT_PAIR_SUCCESS,       /**< 配对成功，param1=对端MAC地址，param2=地址长度6 */
    IEEE80211_EVENT_PAIR_FAIL,          /**< 配对失败，param1=1，param2=2 */
    IEEE80211_EVENT_PAIR_DONE,          /**< 配对完成，param1=对端MAC地址，param2=6 */
    IEEE80211_EVENT_SCAN_START,         /**< 开始扫描WiFi热点 */
    IEEE80211_EVENT_SCAN_DONE,          /**< 扫描完成 */
    IEEE80211_EVENT_CONNECT_START,      /**< 开始连接AP（热点），param1=AP的MAC地址 */
    IEEE80211_EVENT_CONNECTED,          /**< 连接成功，param1=AP或STA的MAC地址（取决于角色） */
    IEEE80211_EVENT_CONNECT_FAIL,       /**< 连接失败，param1=802.11状态码 */
    IEEE80211_EVENT_DISCONNECTED,       /**< 断开连接，param1=对方MAC地址，param2=802.11原因码 */
    IEEE80211_EVENT_RX_FRAME,           /**< 收到原始WiFi帧，param1=帧数据指针，param2=接收状态结构体指针 */
    IEEE80211_EVENT_RSSI,               /**< 信号强度变化，param1=新RSSI值(dBm)，param2=对方MAC地址 */
    IEEE80211_EVENT_STATE_CHANGE,       /**< 内部状态变化（保留） */
    IEEE80211_EVENT_NEW_BSS,            /**< 发现新BSS（热点），param1=BSSID，param2=SSID */
    IEEE80211_EVENT_UPDATE_BSS,         /**< BSS信息更新（保留） */
    IEEE80211_EVENT_PS_START,           /**< 进入省电模式（保留） */
    IEEE80211_EVENT_PS_END,             /**< 退出省电模式（保留） */
    IEEE80211_EVENT_STA_PS_START,       /**< 某个STA（手机等）进入省电模式，param1=STA的MAC地址 */
    IEEE80211_EVENT_STA_PS_END,         /**< STA退出省电模式，param1=STA的MAC地址 */
    IEEE80211_EVENT_INTERFACE_ENABLE,   /**< WiFi接口已启用 */
    IEEE80211_EVENT_INTERFACE_DISABLE,  /**< WiFi接口已禁用 */
    IEEE80211_EVENT_ADD_CUSTOMER_IE,    /**< 需要添加自定义信息元素，用于扩展WiFi帧 */
    IEEE80211_EVENT_MAC_CHANGE,         /**< MAC地址已改变，param1=新MAC地址指针 */
    IEEE80211_EVENT_EVM,                /**< EVM（误差向量幅度，信号质量指标）变化，param1=新EVM值，param2=STA的MAC */
    IEEE80211_EVENT_UNKNOWN_STA,        /**< 未知STA（保留） */
    IEEE80211_EVENT_PRE_AUTH,           /**< STA请求认证前事件，可用于黑名单过滤，param1=STA的MAC */
    IEEE80211_EVENT_PRE_ASSOC,          /**< STA请求关联前事件，可用于黑名单过滤，param1=STA的MAC */
    IEEE80211_EVENT_UNPAIR_SUCCESS,     /**< 解除配对成功，param1=对方STA的MAC地址 */
    IEEE80211_EVENT_TX_BITRATE,         /**< 发送速率变化（保留） */
    IEEE80211_EVENT_EXCEPTION_INFO,     /**< 异常信息（保留） */
    IEEE80211_EVENT_ACS_DONE,           /**< 自动信道选择完成（保留） */
    IEEE80211_EVENT_TX_FRAME,           /**< 协议栈即将发送数据帧前触发，可在此修改或阻止发送 */
    IEEE80211_EVENT_RX_CUSTMGMT,        /**< 收到自定义管理帧（通过ieee80211_tx_custmgmt发送的对端回复） */
    IEEE80211_EVENT_FT_START,           /**< 快速漫游开始（802.11r） */
    IEEE80211_EVENT_WRONG_KEY,          /**< 密码错误 */
    IEEE80211_EVENT_CHANNEL_CHANGE,     /**< 信道发生变化 */
    IEEE80211_EVENT_P2P_NGO_DONE,       /**< P2P协商完成 */
    IEEE80211_EVENT_WSC_DONE,           /**< WSC（WiFi简单配置，即WPS）完成 */
    IEEE80211_EVENT_TX_STATUS,          /**< 发送状态反馈 */
    IEEE80211_EVENT_P2P_FOUND,          /**< 发现P2P设备 */
    IEEE80211_EVENT_AP_CSA_DONE,        /**< AP信道切换完成 */
    IEEE80211_EVENT_SCAN_IDLE,          /**< 扫描空闲 */
    IEEE80211_EVENT_GEN_BEACON,         /**< 生成Beacon帧时触发，可用于修改Beacon内容 */
    IEEE80211_EVENT_BEACON_IDLE,        /**< Beacon空闲 */
    IEEE80211_EVENT_RELAY_UPDATE,       /**< 中继信息更新 */
    IEEE80211_EVENT_RELAY_ERROR,        /**< 中继发生错误 */
    IEEE80211_EVENT_PAIR_NGO,           /**< 配对协商过程事件 */
    IEEE80211_EVENT_ROMAING_FAIL,       /**< 漫游失败 */
    IEEE80211_EVENT_REQ_PAIR_AID,       /**< 请求配对AID（关联ID） */
    IEEE80211_EVENT_SLEEP_EXIT,         /**< 协议栈检测到需要退出低功耗模式，param1=退出原因 */
};

/**
 * @brief TXSDK自定义的断开连接原因码（扩展802.11标准）
 * @note 当连接断开时，可通过 ieee80211_conf_get_reason_code 获取这些自定义原因
 * @see ieee80211_conf_get_reason_code
 */
enum IEEE80211_TXREASON {
    WLAN_REASON_AP_NOT_FOUND = 60000,   /**< 扫描未发现目标AP */
    WLAN_REASON_ASSOC_TIMEOUT,          /**< 关联超时（AP没有响应关联请求） */
    WLAN_REASON_PAIR_START,             /**< 启动配对过程中断开了现有连接 */
    WLAN_REASON_UNPAIR,                 /**< 用户主动解除配对导致断开 */
    WLAN_REASON_RELAY_DISCONNECT,       /**< 上级中继链路断开导致本机断开 */
};

/**
 * @brief WPA/WPA2连接状态机状态枚举
 * @note 用于指示WiFi连接当前所处的阶段，可通过 ieee80211_conf_get_connstate 获取
 * @see ieee80211_conf_get_connstate
 */
enum wpa_states {
    WPA_DISCONNECTED,       /**< 完全断开，未连接到任何网络 */
    WPA_INTERFACE_DISABLED, /**< 接口被禁用（如用户关闭了WiFi） */
    WPA_INACTIVE,           /**< 空闲状态，未开始连接 */
    WPA_SCANNING,           /**< 正在扫描热点 */
    WPA_AUTHENTICATING,     /**< 正在认证（802.11认证阶段） */
    WPA_ASSOCIATING,        /**< 正在关联（请求加入网络） */
    WPA_ASSOCIATED,         /**< 已关联，但尚未完成WPA密钥交换 */
    WPA_4WAY_HANDSHAKE,     /**< 正在进行4次握手（WPA/WPA2密钥协商） */
    WPA_GROUP_HANDSHAKE,    /**< 正在进行组密钥握手 */
    WPA_COMPLETED,          /**< 连接完全成功，可以正常收发数据 */
};

/**
 * @brief 数据包钩子（Hook）处理结果枚举
 * @note 用于决定钩子函数处理完数据包后，协议栈应如何操作
 * @see ieee80211_pkthdl
 */
typedef enum {
    IEEE80211_PKTHDL_CONTINUE = 0, /**< 继续正常处理此数据包 */
    IEEE80211_PKTHDL_CONSUMED = 1, /**< 数据包已被钩子消耗，协议栈应丢弃它（不再继续传递） */
} ieee80211_pkthdl_res;

/**
 * @brief 自定义管理帧的数据结构（用于接收方向）
 * @note 当收到对端通过 ieee80211_tx_custmgmt 发送的自定义管理帧时，通过 IEEE80211_EVENT_RX_CUSTMGMT 事件上报此结构
 * @see IEEE80211_EVENT_RX_CUSTMGMT
 */
struct ieee80211_custmgmt_data {
    uint8 *from;  /**< 发送此帧的设备的MAC地址 */
    uint8 *data;  /**< 自定义管理帧的数据负载指针（不含802.11头部） */
    uint8  len;   /**< 数据长度（字节） */
};

/**
 * @brief 信道切换通知（CSA）参数结构体
 * @note 用于AP主动切换信道时，向连接的STA通告切换信息
 * @see ieee80211_conf_set_csa
 */
struct ieee80211_csa_param {
    uint8 mode;  /**< 切换模式：0=仅通告（STA可选择是否跟随），1=强制切换（STA必须跟随） */
    uint8 chan;  /**< 目标信道号（如1,6,11等） */
    uint8 count; /**< 切换前的Beacon发送次数，表示多少个Beacon后执行切换，通常为1-10 */
};

/**
 * @brief 钩子函数收到的数据包信息结构体
 * @note 在注册的钩子函数中，通过此结构获取数据包内容及相关元数据
 * @see ieee80211_pkthdl, ieee80211_conf_set_datatag
 */
struct ieee80211_hookdata {
    uint8 *data;        /**< 数据包内容指针（完整的802.11帧） */
    uint16 len;         /**< 数据包总长度（字节） */
    uint16 ext: 1,      /**< 扩展标志（保留） */
           recv: 15;    /**< 方向标志：1表示接收方向，0表示发送方向 */
    uint32 hdl;         /**< 数据句柄，用于后续标签设置（ieee80211_conf_set_datatag 需要此句柄） */
};
struct ieee80211_pkthook;
/**
 * @brief 钩子处理函数类型定义
 * @param hook 钩子实例指针
 * @param ifidx 接口索引
 * @param data 数据包信息
 * @return 处理结果，参见 ieee80211_pkthdl_res
 */
typedef ieee80211_pkthdl_res(*ieee80211_pkthdl)(struct ieee80211_pkthook *hook, uint8 ifidx, struct ieee80211_hookdata *data);
/**
 * @brief 钩子常量配置结构体（在注册时提供）
 * @note 用于描述钩子的基本属性和回调函数
 * @see ieee80211_register_pkthook
 */
struct ieee80211_pkthook_const {
    const char *name;      /**< 钩子名称（便于调试识别） */
    void  *priv;           /**< 私有数据指针，在钩子函数中可通过hook->c_data->priv获取 */
    uint16 protocol;       /**< 要过滤的以太网协议类型（如0x0800=IPv4），0表示全部 */
    uint16 mcast: 1,       /**< 是否监听所有的组播数据包 */
           ucast: 1,       /**< 是否监听所有的单播数据包 */
           rev: 14;        /**< 保留位，必须为0 */
    ieee80211_pkthdl tx;   /**< 协议栈发送数据前的钩子函数 */
    ieee80211_pkthdl rx;   /**< 协议栈接收数据后的钩子函数 */
};
/**
 * @brief 钩子实例结构体（注册时填充）
 * @note 应用程序需要定义一个该类型的静态变量，并调用注册函数将其挂载到协议栈
 * @see ieee80211_register_pkthook
 */
struct ieee80211_pkthook {
    struct ieee80211_pkthook *next;     /**< 链表指针（由协议栈管理） */
    uint32 time_max;                     /**< 钩子最大处理时间统计（微秒，调试用） */
    uint32 consume_data;                 /**< 钩子消耗的数据总量统计（字节） */
    const struct ieee80211_pkthook_const *c_data; /**< 指向常量配置的指针 */
};

/**
 * @brief WiFi协议栈事件回调函数类型
 * @param ifidx 发生事件的接口索引（支持多接口时区分）
 * @param evt 事件类型，参见 enum ieee80211_event
 * @param param1 事件参数1，具体含义参见事件枚举注释
 * @param param2 事件参数2，具体含义参见事件枚举注释
 * @return 0表示处理成功，非0可能影响后续行为（取决于事件类型）
 * @see enum ieee80211_event
 * @note 此回调由用户应用程序实现，并通过 ieee80211_event_cb 注册到协议栈。
 *       此回调过程是在协议栈的task中执行，所以在回调过程不能执行耗时的动作，以免影响协议栈的实时性。
 */
typedef int32(*ieee80211_evt_cb)(uint8 ifidx, uint16 evt, uint32 param1, uint32 param2);

/**
 * @brief 协议栈初始化参数结构体
 * @note 调用 ieee80211_init 之前需要填充此结构体
 * @see ieee80211_init
 */
struct ieee80211_initparam {
    uint8            vif_maxcnt;        /**< 最多可以创建多少个虚拟接口（如同时做AP和STA） */
    uint8            bss_maxcnt;        /**< 最多可以缓存多少个BSS（扫描结果）信息 */
    uint8            headroom;          /**< 数据帧头部预留空间，用于添加自定义头部（字节） */
    uint8            tailroom;          /**< 数据帧尾部预留空间（字节） */
    uint16           sta_maxcnt;        /**< 最多允许连接的STA数量（AP模式时生效） */
    uint16           bss_lifetime;      /**< 缓存的BSS信息有效期（秒），超时后自动删除 */
    uint16           stack_size;        /**< 协议栈内部任务堆栈大小（字节），默认2000通常足够 */
    uint16           no_rxtask: 1,      /**< 1=不使用独立的接收任务（将收包处理放在其他上下文中） */
                       ssid_fuzzy_match: 4, /**< SSID模糊匹配时允许的字符差异位数（0=精确匹配） */
                       rev: 11;          /**< 保留位，必须填0 */
    ieee80211_evt_cb evt_cb;            /**< 事件回调函数，所有WiFi事件都会调用此函数通知应用 */
};

/**
 * @brief 主动扫描参数结构体
 * @note 调用 ieee80211_scan 时传递此结构，定制扫描行为
 * @see ieee80211_scan
 */
struct ieee80211_scandata {
    uint32 chan_bitmap;                 /**< 要扫描的信道位图，每一位代表一个信道，如bit1表示信道1 */
    uint8 ssid[SSID_MAX_LEN + 1];       /**< 指定要扫描的SSID名称，字符串以'\0'结尾；若为空字符串则扫描所有热点 */
    uint8 scan_time;                    /**< 在每个信道上停留的时间（单位：ms） */
    uint8 scan_cnt;                     /**< 每个信道重复扫描的次数 */
    uint8 passive_scan: 1,              /**< 是否为被动扫描：1=仅监听不发送探测请求，0=主动发送探测请求 */
          flush: 1,                     /**< 是否清空之前缓存的扫描结果 */
          rev: 6;                       /**< 保留位，必须为0 */
};

/**
 * @brief 已连接的STA的信息结构体
 * @note AP模式时，可通过此结构获取每个连接设备的详细状态
 * @see ieee80211_conf_get_stainfo, ieee80211_conf_get_stalist
 */
struct ieee80211_stainfo {
    uint16 aid;         /**< 关联ID，AP分配给每个STA的唯一标识（1-2007） */
    uint8  addr[6];     /**< STA的MAC地址 */
    int8   rssi;        /**< 接收信号强度指示（dBm），例如-50表示信号很好，-80表示较差 */
    int8   evm;         /**< 误差向量幅度，衡量信号质量（值越小越好，通常小于-30dB为佳） */
    uint8  wake_reason; /**< 最近一次唤醒原因 */
    uint8  ps: 1,       /**< 是否处于省电模式（1表示STA休眠，AP需要为其缓存数据） */
           connected: 1, /**< 是否已完全连接（1表示已完成4次握手） */
           wmm: 1,      /**< 是否支持WMM（WiFi多媒体，即QoS优先级） */
           r: 5;        /**< 保留位 */
    uint32 tx_mcs;      /**< 发送数据使用的MCS索引（调制编码方案，代表速率） */
    uint32 rx_mcs;      /**< 接收数据使用的MCS索引 */
    uint64 tx_bytes;    /**< 累计发送给此STA的总字节数 */
    uint64 rx_bytes;    /**< 累计从此STA接收的总字节数 */
    int8   tx_snr;      /**< 发送方向信噪比（dB） */
    int8   rx_snr;      /**< 接收方向信噪比（dB） */
    uint8  per;         /**< 误包率（百分比），例如5表示约5%的数据包丢失） */
    uint8  r2;          /**< 保留 */
};

/**
 * @brief WMM（WiFi多媒体）EDCA参数，用于设置不同优先级队列的竞争参数
 * @note EDCA（增强分布式信道访问）决定不同业务（语音、视频、背景等）获得发送机会的优先级
 * @see ieee80211_conf_set_wmm_param, ieee80211_conf_get_wmm_param
 */
struct ieee80211_wmm_param {
    uint16 txop;    /**< 传输机会限制（微秒），允许一次占用信道连续发送的最大时间，0表示只发一帧 */
    uint16 cw_min;  /**< 竞争窗口指数最小值（用于随机退避算法，值越小，发送机会越多） */
    uint16 cw_max;  /**< 竞争窗口指数最大值 */
    uint8  aifsn;   /**< 仲裁帧间隙数，等待空闲的时间（数量越少，优先级越高） */
    uint8  acm;     /**< 强制准入控制（暂未使用，预留） */
};

/**
 * @brief 发送数据时可携带的额外控制信息
 * @note 用于高级发送接口，如自定义发送行为
 * @see ieee80211_tx
 */
struct ieee80211_tx_info {
    uint8  band;        /**< 使用哪个频段发送（详见 IEEE80211_BAND） */
    uint8  ifidx;       /**< 通过哪个接口索引发送 */
    uint16 no_ack: 1,   /**< 是否要求对方回复ACK确认：1=不需要（常用于组播或无需可靠传输的场景） */
           rev: 15;     /**< 保留位 */
    uint32 tag;         /**< 自定义标签，可用于数据流识别 */
};

/**
 * @brief 发送统计信息，用于获取数据包发送结果
 * @note 某些发送API会返回此联合体，供应用程序检查是否发送成功
 */
union ieee80211_tx_stats {
    struct {
        uint8 acked: 1,  /**< 是否收到对方的ACK确认（1=已收到） */
              rev1: 7;   /**< 保留 */
        uint8 retry;     /**< 实际重传次数（首次发送不计入） */
        uint8 rev2;      /**< 保留 */
        uint8 rev3;      /**< 保留 */
    } stats;
    uint32 v;            /**< 以32位整数整体访问所有信息 */
};

/**
 * @brief 接收帧时的射频状态信息
 * @note 通过 IEEE80211_EVENT_RX_FRAME 事件上报，可获取信号质量等元数据
 * @see IEEE80211_EVENT_RX_FRAME
 */
struct ieee80211_rx_status {
    uint32 freq;        /**< 接收数据的中心频率（MHz），如2412表示2.412GHz */
    uint8  band;        /**< 频段（详见 IEEE80211_BAND） */
    uint8  ifidx;       /**< 接收此帧的接口索引 */
    int8   rssi;        /**< 信号强度（dBm），数值越大（绝对值越小）信号越好 */
    int8   evm;         /**< 误差向量幅度，信号质量指标（越小越好） */
    uint8  mcs;         /**< 接收此帧时使用的MCS索引 */
    uint8  rx_flags;    /**< 接收标志位组合，见 enum ieee80211_rx_flag */
    uint16 rxlen;       /**< 实际接收到的帧长度（字节） */
};

/**
 * @brief 接收到的完整帧信息（包含帧数据和接收状态）
 * @note 用于某些接收回调的聚合信息
 */
struct ieee80211_rx_frminfo {
    uint8 *frame;                   /**< 指向完整的802.11帧数据的指针 */
    uint32 frame_len;               /**< 帧数据的总长度（字节） */
    struct ieee80211_rx_status *status; /**< 指向接收状态信息的指针 */
};

/**
 * @brief 接收标志位枚举，用于 ieee80211_rx_status.rx_flags 字段
 */
enum ieee80211_rx_flag {
    IEEE80211_RX_FLAG_MORE_DATA = BIT(0), /**< 指示AP还有更多数据要发给STA（用于省电模式） */
    IEEE80211_RX_FLAG_AMSDU = BIT(1),     /**< 此帧是A-MSDU聚合帧，内部包含多个以太网帧 */
};

/**
 * @brief 漫游参数结构体（内部状态跟踪）
 * @note 用于记录漫游过程中的状态信息，通常不需要应用层直接操作
 * @see ieee80211_conf_set_roam_config, ieee80211_wnb_roam_enable
 */
struct ieee80211_roaming_param {
    uint8  start: 1,    /**< 漫游是否已开始 */
           ft_en: 1,    /**< 快速漫游（802.11r）是否启用 */
           request: 2;  /**< 漫游请求状态（内部） */
    uint8  state;       /**< 当前漫游状态机状态 */
    uint8  old_chan;    /**< 原AP所在信道 */
    int8   rssi_avg;    /**< 当前AP的平均RSSI */
    int8   rssi_check;  /**< 用于判断的RSSI阈值 */
    int8   rssi_th;     /**< 漫游触发的RSSI门限（低于此值考虑漫游） */
    uint8  rssi_diff;   /**< 新旧AP信号强度差值门限（新AP需比旧AP强超过此值才漫游） */
    uint8  rssi_int;    /**< 连续检测beacon的次数（连续多次超过门限才触发漫游） */
    uint8  cur_chan;    /**< 当前工作信道 */
    uint16 roaming_tmo; /**< 漫游过程超时时间（ms） */
    int16  rssi_sum;    /**< RSSI累加和（用于计算平均值） */
    uint8  bssid_new[6]; /**< 目标新AP的BSSID */
    uint8  bssid_old[6]; /**< 当前连接的AP的BSSID */
    uint8  ssid[SSID_MAX_LEN + 1]; /**< 目标SSID（与当前AP相同） */
    struct os_timer tmr;            /**< 漫游定时器对象 */
    struct ieee80211_bss_info *newbss; /**< 指向新BSS信息的指针 */
};

/**
 * @brief BSS（基本服务集，即一个热点）的WPA加密信息
 * @note 用于保存或恢复AP的加密配置，加速后续连接（跳过扫描和解析beacon）
 * @see ieee80211_bss_get_wpadata, ieee80211_bss_add_manualAP
 */
struct ieee80211_bss_wpadata {
    uint32 proto;           /**< WPA协议版本，值为WPA_PROTO_WPA或WPA_PROTO_RSN */
    uint32 group_cipher;    /**< 组播加密算法（用于广播和组播数据） */
    uint32 pairwise_cipher; /**< 单播加密算法（用于一对一通信） */
    uint32 key_mgmt;        /**< 密钥管理方式（如WPA_KEY_MGMT_PSK） */
};

/**
 * @brief MLME（MAC层管理实体）管理帧扩展大小配置
 * @note 用于在协议栈预分配的帧缓冲中额外留出空间，以便添加自定义IE（信息元素）
 * @see ieee80211_expand_mlme_size
 */
struct ieee80211_mlme_size {
    uint16 probe_req;           /**< Probe Request帧需要额外增加的字节数 */
    uint16 probe_resp;          /**< Probe Response帧需要额外增加的字节数 */
    uint16 beacon;              /**< Beacon帧需要额外增加的字节数 */
    uint16 vendor_spec_action;  /**< 厂商自定义Action帧需要额外增加的字节数 */
    uint16 assoc_req;           /**< Association Request帧需要额外增加的字节数 */
    uint16 assoc_resp;          /**< Association Response帧需要额外增加的字节数 */
    uint8  auth;                /**< Authentication帧需要额外增加的字节数 */
    uint8  deauth;              /**< Deauthentication帧需要额外增加的字节数 */
    uint8  disassoc;            /**< Disassociation帧需要额外增加的字节数 */
    uint8  addba_resp;          /**< ADDBA Response帧（块确认协商）需要额外增加的字节数 */
    uint16 p2p_neg;             /**< P2P Negotiation帧需要额外增加的字节数 */
};

/**
 * @brief 链路质量评估参数，用于计算WiFi链路的综合得分
 * @note 可用于智能选择最佳AP或评估当前连接质量
 * @see ieee80211_conf_set_linkcost_param
 */
struct ieee80211_linkcost_param {
    /* 各项指标的上限或下限（超出此范围则认为质量极差） */
    int8   rssi_min;        /**< RSSI最低可接受值（dBm），低于此值的链路不考虑 */
    int8   evm_min;         /**< EVM最低可接受值（越大约差，通常为负数） */
    uint8  stas_max;        /**< AP上关联的STA数量上限，超过则认为AP负载过高 */
    uint8  loading_max;     /**< AP信道利用率上限（百分比），超过则认为过载 */
    uint16 txdelay_max;     /**< 最大传输延迟（ms），超过则认为链路慢 */
    uint16 datarate_max;    /**< 最大数据速率（Mbps），用于归一化计算 */

    /* 各项指标在最终得分中的权重（百分比，总和不一定为100，内部会归一化） */
    uint8 rssi_weight;      /**< RSSI权重（越大越重要） */
    uint8 evm_weight;       /**< EVM权重 */
    uint8 txdelay_weight;   /**< 传输延迟权重 */
    uint8 loading_weight;   /**< AP负载权重 */
    uint8 stas_weight;      /**< STA数量权重 */
    uint8 per_weight;       /**< 误包率权重 */
    uint8 datarate_weight;  /**< 数据速率权重 */

    /* 每项指标的质量分级阈值（百分比）：
       例如 rssi_poor=30 表示实际RSSI值低于最差值区间上限30%时评为“差” */
    uint8 rssi_poor, rssi_maiginal, rssi_good;
    uint8 evm_poor, evm_maiginal, evm_good;
    uint8 loading_poor, loading_maiginal, loading_good;
    uint8 stas_poor, stas_maiginal, stas_good;
    uint8 per_poor, per_maiginal, per_good;
    uint8 txdelay_poor, txdelay_maiginal, txdelay_good;
    uint8 datarate_poor, datarate_maiginal, datarate_good;
};

/**
 * @brief RMESH（泰芯私有RMesh网络）设备信息
 * @note 用于记录Mesh网络中的节点设备
 * @see ieee80211_conf_set_rmesh_device, ieee80211_conf_get_rmesh_device
 */
struct ieee80211_rmesh_device {
    uint8 aid;      /**< Mesh内部分配的短地址（关联ID） */
    uint8 addr[6];  /**< 设备MAC地址 */
};

/* ==================== 初始化API ==================== */

/**
 * @brief 根据SSID和明文密码计算PSK（预共享密钥）密文
 * @param ssid [in] WiFi网络名称（字符串）
 * @param passwd [in] 明文密码（8~63字符）
 * @param psk [out] 输出32字节二进制PSK，可保存到flash
 * @return 0成功，-1失败（参数错误或计算失败）
 * @note PSK计算采用PBKDF2算法，比较耗时（可能需要几百毫秒），建议提前计算并保存，避免每次连接都重新计算。
 *       保存的PSK可用于 ieee80211_conf_set_psk 直接设置。
 */
extern int32 wpa_passphrase(uint8 *ssid, char *passwd, uint8 psk[32]);

/**
 * @brief 初始化WiFi协议栈（必须在调用任何其他WiFi API之前执行）
 * @param param [in] 初始化参数，参见 struct ieee80211_initparam
 * @return 0成功，非0失败（返回错误码）
 * @note 此函数会创建协议栈内部任务和数据结构，只能调用一次。
 */
extern int32 ieee80211_init(struct ieee80211_initparam *param);

/**
 * @brief 初始化协议栈的数据转发路由表（用于桥接或中继场景）
 * @param max [in] 路由表可缓存的最大条目数
 * @param lifetime [in] 每个路由表条目的生存时间（秒）
 * @return 0成功，非0失败
 */
extern int32 ieee80211_deliver_init(int32 max, uint32 lifetime);

/**
 * @brief 配置协议栈以支持TXW8301芯片（泰芯半导体WiFi芯片）
 * @param ops [in] 底层LMAC（低层MAC）操作接口句柄
 * @return 0成功，非0失败
 */
extern int32 ieee80211_support_txw830x(void *ops);

/**
 * @brief 配置协议栈以支持TXW80x系列芯片
 * @param ops [in] 底层LMAC操作接口句柄
 * @return 0成功，非0失败
 */
extern int32 ieee80211_support_txw80x(void *ops);

/**
 * @brief 配置协议栈以支持TXW81x系列芯片
 * @param ops [in] 底层LMAC操作接口句柄
 * @return 0成功，非0失败
 */
extern int32 ieee80211_support_txw81x(void *ops);

/* ==================== 接口创建API ==================== */

/**
 * @brief 创建一个标准AP模式的接口（设备作为WiFi热点）
 * @param ifidx [in] 接口索引（0~max-1，由应用分配）
 * @param band [in] WiFi频段，参见 enum IEEE80211_BAND
 * @return 0成功，非0失败（如索引已被占用）
 * @note 创建后需要调用 ieee80211_iface_start 启动。
 */
extern int32 ieee80211_iface_create_ap(uint8 ifidx, uint8 band);

/**
 * @brief 创建一个标准STA模式的接口（设备作为客户端连接热点）
 * @param ifidx [in] 接口索引
 * @param band [in] WiFi频段
 * @return 0成功，非0失败
 */
extern int32 ieee80211_iface_create_sta(uint8 ifidx, uint8 band);

/**
 * @brief 创建一个RMESH模式接口（泰芯私有RMesh网络）
 * @param ifidx [in] 接口索引
 * @param band [in] WiFi频段
 * @return 0成功，非0失败
 */
extern int32 ieee80211_iface_create_rmesh(uint8 ifidx, uint8 band);

/**
 * @brief 创建一个P2P（WiFi直连）模式接口
 * @param ifidx [in] 接口索引
 * @param band [in] WiFi频段
 * @param go [in] 是否为Group Owner（1=是，0=否）
 * @param gc [in] 是否为Group Client（1=是，0=否）
 * @return 0成功，非0失败
 */
extern int32 ieee80211_iface_create_p2pdev(uint8 ifidx, uint8 band, uint8 go, uint8 gc);

/**
 * @brief 创建泰芯私有协议WNBSTA模式接口（类似STA但使用私有扩展）
 * @param ifidx [in] 接口索引
 * @param band [in] WiFi频段
 * @return 0成功，非0失败
 */
extern int32 ieee80211_iface_create_wnbsta(uint8 ifidx, uint8 band);

/**
 * @brief 创建泰芯私有协议WNBAP模式接口（类似AP但使用私有扩展）
 * @param ifidx [in] 接口索引
 * @param band [in] WiFi频段
 * @return 0成功，非0失败
 */
extern int32 ieee80211_iface_create_wnbap(uint8 ifidx, uint8 band);

/**
 * @brief 启动已创建的接口（开始工作）
 * @param ifidx [in] 接口索引
 * @return 0成功，非0失败
 * @note 对于AP模式，启动后会开始发送Beacon；对于STA模式，会开始扫描和连接。
 */
extern int32 ieee80211_iface_start(uint8 ifidx);

/**
 * @brief 停止正在运行的接口（停止收发）
 * @param ifidx [in] 接口索引
 * @return 0成功，非0失败
 */
extern int32 ieee80211_iface_stop(uint8 ifidx);

/**
 * @brief 设置接口进入或退出省电模式（仅STA模式有效）
 * @param ifidx [in] 接口索引
 * @param ps [in] 1=进入省电模式（定期休眠以省电），0=退出省电模式（一直唤醒）
 * @return 0成功，非0失败
 * @note 进入省电模式后，AP会为STA缓存数据，STA会定期醒来接收。
 */
extern int32 ieee80211_iface_sleep(uint8 ifidx, uint8 ps);

/* ==================== 一键配对API ==================== */

/**
 * @brief 初始化一键配对功能（需在配对前调用）
 * @param ifidx [in] 启用配对的接口索引
 * @param pair_magic [in] 配对魔数，只有魔数相同的设备才能互相配对
 * @return 0成功，非0失败
 */
extern int32 ieee80211_pair_enable(uint8 ifidx, uint16 pair_magic);

/**
 * @brief 启动或停止配对操作
 * @param ifidx [in] 接口索引
 * @param pair_magic [in] 取值：
 *                        - 0: 停止配对
 *                        - 1: 使用 ieee80211_pair_enable 设置的魔数启动配对
 *                        - >1: 直接使用此值作为魔数启动配对
 * @return 0成功，非0失败
 * @note 配对过程中协议栈会自动断开当前连接（如果有），然后寻找魔数相同的设备进行配对。
 */
extern int32 ieee80211_pairing(uint8 ifidx, uint16 pair_magic);

/**
 * @brief 解除与指定STA的配对关系
 * @param ifidx [in] 接口索引
 * @param mac [in] 需要解除配对的设备的MAC地址
 * @return 0成功，非0失败
 */
extern int32 ieee80211_unpair(uint8 ifidx, uint8 *mac);

/* ==================== 状态查询API ==================== */

/**
 * @brief 打印WiFi协议栈内部状态信息（用于调试）
 * @param buff [in] 输出缓冲区指针
 * @param size [in] 缓冲区大小
 * @note 信息包含任务状态、内存使用、连接状态等文本信息。
 */
extern void  ieee80211_status(uint8 *buff, uint32 size);

/* ==================== 扫描API ==================== */

/**
 * @brief 执行WiFi扫描（搜索周围热点）
 * @param index [in] 接口索引
 * @param start [in] 1=开始扫描，0=停止扫描（如果正在进行）
 * @param scan_param [in] 扫描参数，参见 struct ieee80211_scandata
 * @return 0成功，非0失败
 * @note 扫描结果通过 IEEE80211_EVENT_NEW_BSS 事件陆续上报，或调用 ieee80211_get_bsslist 获取。
 */
extern int32 ieee80211_scan(uint8 index, uint8 start, struct ieee80211_scandata *scan_param);

/**
 * @brief 获取扫描到的BSS（热点）列表
 * @param bsslist [out] BSS信息列表缓冲区，类型为 struct hgic_bss_info 数组
 * @param list_size [in] 缓冲区能容纳的最大条目数
 * @param ignore_dis [in] 是否忽略已被应用标记为禁用的BSS（1=忽略，0=全部返回）
 * @return 实际写入列表的BSS数量
 */
extern int32 ieee80211_get_bsslist(struct hgic_bss_info *bsslist, int32 list_size, int8 ignore_dis);

/**
 * @brief 清除协议栈缓存的BSS列表（所有扫描结果）
 */
extern void ieee80211_cleanup_bsslist(void);

/* ==================== 数据收发API ==================== */

/**
 * @brief 发送WiFi数据帧（使用分散-聚集方式，避免数据拷贝）
 * @param ifidx [in] 接口索引
 * @param data [in] 分散数据数组（scatter_data 包含每个分片的数据指针和长度）
 * @param count [in] 数据段数量
 * @return 0成功，非0失败
 * @note 适用于将多个不连续的缓冲区一次性发送，提高效率。
 */
extern int32 ieee80211_scatter_tx(uint8 ifidx, scatter_data *data, uint32 count);

/**
 * @brief 发送WiFi数据帧（简单方式，数据连续）
 * @param ifidx [in] 接口索引
 * @param data [in] 数据指针（应为完整的802.11帧或以太网帧，取决于接口类型）
 * @param len [in] 数据长度
 * @return 0成功，非0失败
 */
extern int32 ieee80211_tx(uint8 ifidx, uint8 *data, uint32 len);

/**
 * @brief 向WiFi协议栈输入数据（通常用于将外部收到的数据送入协议栈处理）
 * @param ifidx [in] 接口索引
 * @param data [in] 数据指针（应为以太网帧格式）
 * @param len [in] 数据长度
 * @note 此函数主要用于驱动层将底层收到的原始帧传递给上层协议栈。
 */
extern void ieee80211_input(uint8 ifidx, uint8 *data, uint32 len);

/**
 * @brief 发送一个管理帧（如Beacon、Probe Response等）
 * @param ifidx [in] 接口索引
 * @param mgmt [in] 管理帧数据指针（必须是完整的802.11管理帧格式）
 * @param len [in] 数据长度
 * @return 0成功，非0失败
 */
extern int32 ieee80211_tx_mgmt(uint8 ifidx, uint8 *mgmt, uint32 len);

/**
 * @brief 发送自定义管理帧（协议栈会自动封装802.11头部）
 * @param ifidx [in] 接口索引
 * @param dest [in] 目标MAC地址
 * @param data [in] 管理帧的有效载荷（不需要包含802.11头部）
 * @param len [in] 有效载荷长度
 * @return 0成功，非0失败
 * @note 接收方会收到 IEEE80211_EVENT_RX_CUSTMGMT 事件，并得到 struct ieee80211_custmgmt_data。
 */
extern int32 ieee80211_tx_custmgmt(uint8 ifidx, uint8 *dest, uint8 *data, uint32 len);

/**
 * @brief 发送以太网帧（协议栈会自动添加802.11头部并加密）
 * @param ifidx [in] 接口索引
 * @param dest [in] 目标MAC地址
 * @param proto [in] 以太网协议类型（如0x0800=IPv4）
 * @param data [in] 数据负载（不含以太网头部）
 * @param len [in] 数据长度
 * @return 0成功，非0失败
 */
extern int32 ieee80211_tx_ether(uint8 ifidx, uint8 *dest, uint16 proto, uint8 *data, uint32 len);

/* ==================== 连接管理API ==================== */

/**
 * @brief 断开与指定STA（在AP模式下）或断开与AP（在STA模式下）的连接
 * @param ifidx [in] 接口索引
 * @param addr [in] 要断开的目标MAC地址（如果为NULL或全0，则断开所有？具体看实现）
 * @return 0成功，非0失败
 */
extern int32 ieee80211_disassoc(uint8 ifidx, uint8 *addr);

/**
 * @brief 断开当前接口上所有已连接的STA（仅AP模式）
 * @param ifidx [in] 接口索引
 * @return 0成功，非0失败
 */
extern int32 ieee80211_disassoc_all(uint8 ifidx);

/**
 * @brief 主动执行建立连接过程（仅STA模式）。（通常是在关闭自动连接后需要执行）
 * @param ifidx [in] 接口索引
 * @return 0成功，非0失败
 */
extern int32 ieee80211_start_connect(uint8 ifidx);

/* ==================== Hook机制API ==================== */

/**
 * @brief 注册一个数据包钩子（用于拦截或修改收发的WiFi帧）
 * @param hook [in] 钩子实例指针（需应用静态分配并填充好）
 * @return 0成功，非0失败
 * @note 钩子可在发送前或接收后对帧进行处理，如添加自定义头部、记录日志、甚至丢弃帧。
 */
extern int32 ieee80211_register_pkthook(struct ieee80211_pkthook *hook);

/**
 * @brief 使WiFi协议栈的钩子机制处理外部传入的数据（即模拟从WiFi收到数据）
 * @param ifidx [in] 接口索引
 * @param tx [in] 1表示模拟发送方向，0表示模拟接收方向
 * @param data [in] 数据指针
 * @param len [in] 数据长度
 * @return 0成功，非0失败
 */
extern int32 ieee80211_hook_ext_data(uint8 ifidx, uint8 tx, uint8 *data, uint32 len);

/**
 * @brief 打印所有已注册钩子的状态信息（调试用）
 */
extern void ieee80211_hook_status(void);

/**
 * @brief 设置或获取当前的事件回调函数
 * @param cb [in] 新的回调函数指针，若为NULL则不修改
 * @return 之前的回调函数指针（可用于链式调用）
 * @note 如果多次调用，返回前一次注册的函数，注意不要丢失，否则前一个回调将不再被调用。
 */
extern ieee80211_evt_cb ieee80211_event_cb(ieee80211_evt_cb cb);

/* ==================== 基础配置API ==================== */

/**
 * @brief 设置WiFi工作信道（仅对AP模式有效）
 * @param ifidx [in] 接口索引
 * @param channel [in] 信道号（如1,6,11等）
 * @return 0成功，非0失败
 * @note STA模式下信道由连接的AP决定，此设置无效。
 */
extern int32 ieee80211_conf_set_channel(uint8 ifidx, uint8 channel);

/**
 * @brief 获取当前工作信道
 * @param ifidx [in] 接口索引
 * @return 信道号，失败返回负数
 */
extern int32 ieee80211_conf_get_channel(uint8 ifidx);

/**
 * @brief 设置WiFi网络名称（SSID）
 * @param ifidx [in] 接口索引
 * @param ssid [in] 以'\0'结尾的字符串，最大32字节
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_ssid(uint8 ifidx, uint8 *ssid);

/**
 * @brief 获取当前SSID
 * @param ifidx [in] 接口索引
 * @param ssid [out] 输出缓冲区，至少33字节
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_get_ssid(uint8 ifidx, uint8 *ssid);

/**
 * @brief 设置WiFi加密模式（密钥管理类型）
 * @param ifidx [in] 接口索引
 * @param keymgmt [in] 参见 WPA_KEY_MGMT_* 宏的组合值
 * @return 0成功，非0失败
 * @example 设置WPA2-PSK：ieee80211_conf_set_keymgmt(ifidx, WPA_KEY_MGMT_PSK);
 */
extern int32 ieee80211_conf_set_keymgmt(uint8 ifidx, uint32 keymgmt);

/**
 * @brief 获取当前配置的加密模式
 * @param ifidx [in] 接口索引
 * @return 加密模式位掩码，失败返回负数
 */
extern int32 ieee80211_conf_get_keymgmt(uint8 ifidx);

/**
 * @brief 设置PSK（预共享密钥）二进制密文（通常是通过 wpa_passphrase 计算得到，也可以自定义）
 * @param ifidx [in] 接口索引
 * @param psk [in] 32字节二进制PSK
 * @return 0成功，非0失败
 * @note 设置后，连接时直接使用此PSK，无需再计算。
 */
extern int32 ieee80211_conf_set_psk(uint8 ifidx, uint8 psk[32]);

/**
 * @brief 设置明文密码（用于WPA3 SAE或自动计算PSK）
 * @param ifidx [in] 接口索引
 * @param passwd [in] 明文密码字符串
 * @return 0成功，非0失败
 * @note 如果同时设置了PSK，优先使用PSK。
 */
extern int32 ieee80211_conf_set_passwd(uint8 ifidx, char *passwd);

/**
 * @brief 获取当前设置的PSK（32字节）
 * @param ifidx [in] 接口索引
 * @param psk [out] 输出缓冲区，至少32字节
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_get_psk(uint8 ifidx, uint8 psk[32]);

/**
 * @brief 设置Beacon帧的发送间隔（仅AP模式）
 * @param ifidx [in] 接口索引
 * @param beacon_int [in] 间隔（毫秒），通常为100
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_beacon_int(uint8 ifidx, uint32 beacon_int);

/**
 * @brief 设置DTIM周期（AP省电模式相关）
 * @param ifidx [in] 接口索引
 * @param dtim_int [in] 每多少个Beacon发送一次DTIM指示，默认10
 * @return 0成功，非0失败
 * @note DTIM周期影响休眠STA接收组播/广播数据的频率。
 */
extern int32 ieee80211_conf_set_dtim_int(uint8 ifidx, uint16 dtim_int);

/**
 * @brief 设置要连接的目标AP的BSSID（仅STA模式）
 * @param ifidx [in] 接口索引
 * @param bssid [in] 6字节MAC地址
 * @return 0成功，非0失败
 * @note 通常需要先设置SSID，再设置BSSID，这样会强制只连接这个特定的AP（而不是同SSID的其他AP）。
 */
extern int32 ieee80211_conf_set_bssid(uint8 ifidx, uint8 *bssid);

/**
 * @brief 获取当前连接的AP的BSSID
 * @param ifidx [in] 接口索引
 * @param bssid [out] 输出6字节MAC地址
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_get_bssid(uint8 ifidx, uint8 *bssid);

/* ==================== MAC地址API ==================== */

/**
 * @brief 获取接口当前的MAC地址
 * @param ifidx [in] 接口索引
 * @param mac [out] 输出6字节MAC地址
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_get_mac(uint8 ifidx, uint8 *mac);

/**
 * @brief 设置接口的MAC地址（需要接口未启动时进行）
 * @param ifidx [in] 接口索引
 * @param mac [in] 新的6字节MAC地址
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_mac(uint8 ifidx, uint8 *mac);

/* ==================== 高级配置API ==================== */

/**
 * @brief 设置BSS带宽（20MHz/40MHz等）
 * @param ifidx [in] 接口索引
 * @param bssbw [in] 带宽值，具体含义需查阅芯片手册
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_bssbw(uint8 ifidx, uint8 bssbw);

/**
 * @brief 获取当前BSS带宽
 * @param ifidx [in] 接口索引
 * @return 带宽值，失败返回负数
 */
extern int32 ieee80211_conf_get_bssbw(uint8 ifidx);

/**
 * @brief 设置自定义信道列表（限制扫描或使用的信道）
 * @param ifidx [in] 接口索引
 * @param chan_list [in] 信道号数组（uint16，每个元素是信道号）
 * @param count [in] 数组元素个数
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_chanlist(uint8 ifidx, uint16 *chan_list, uint32 count);

/**
 * @brief 设置AP模式下STA的最大空闲时间（秒），超时后断开
 * @param ifidx [in] 接口索引
 * @param max_idle_period [in] 最大空闲秒数，默认300秒
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_bss_max_idle(uint8 ifidx, uint16 max_idle_period);

/**
 * @brief 获取当前连接状态（STA模式）
 * @param ifidx [in] 接口索引
 * @return enum wpa_states 中的值，失败返回负数
 */
extern int32 ieee80211_conf_get_connstate(uint8 ifidx);

/**
 * @brief 获取最近一次系统唤醒的原因
 * @param ifidx [in] 接口索引
 * @return 唤醒原因代码
 */
extern uint8 ieee80211_conf_get_wkreason(uint8 ifidx);

/**
 * @brief 主动唤醒一个处于省电模式的STA。
 *        如果AP进入休眠，STA也可以通过此接口唤醒AP。
 * @param ifidx [in] 接口索引
 * @param addr [in] STA的MAC地址
 * @param aid [in] STA的关联ID
 * @param reason [in] 唤醒原因值（自定义）
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_wakeup_sta(uint8 ifidx, uint8 *addr, uint16 aid, uint32 reason);

/**
 * @brief 获取当前发射功率（dBm）
 * @param ifidx [in] 接口索引
 * @return 功率值，失败返回负数
 */
extern int32 ieee80211_conf_get_txpower(uint8 ifidx);

/**
 * @brief 设置心跳包间隔（用于保持连接）
 * @param ifidx [in] 接口索引
 * @param heartbeat_int [in] 间隔秒数
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_heartbeat_int(uint8 ifidx, uint32 heartbeat_int);

/**
 * @brief 设置心跳包的内容和重试次数
 * @param ifidx [in] 接口索引
 * @param data [in] 心跳数据指针
 * @param retry [in] 最大重试次数
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_heartbeat_data(uint8 ifidx, void *data, uint8 retry);

/**
 * @brief 设置AP离线检测时间（STA模式）
 * @param ifidx [in] 接口索引
 * @param time [in] 检测时间（单位：Beacon周期数），默认30
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_aplost_time(uint8 ifidx, uint32 time);

/**
 * @brief 开启或关闭自动信道选择（ACS，仅AP模式）
 * @param ifidx [in] 接口索引
 * @param acs [in] 1=开启，0=关闭
 * @param tmo [in] ACS超时时间（秒）
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_acs(uint8 ifidx, uint8 acs, uint8 tmo);

/**
 * @brief 开启或关闭WMM（WiFi多媒体，即QoS功能）
 * @param ifidx [in] 接口索引
 * @param enable [in] 1=开启，0=关闭
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_wmm_enable(uint8 ifidx, uint8 enable);

/**
 * @brief 设置是否使用4地址模式（用于桥接或WDS）
 * @param ifidx [in] 接口索引
 * @param enable [in] 1=开启，0=关闭
 * @return 0成功，非0失败
 * @note 普通WiFi帧只有三个地址，4地址模式可携带源和目的地址，用于中继。
 */
extern int32 ieee80211_conf_set_use4addr(uint8 ifidx, uint8 enable);

/**
 * @brief 获取当前是否启用4地址模式
 * @param ifidx [in] 接口索引
 * @return 1=启用，0=关闭，失败返回负数
 */
extern int32 ieee80211_conf_get_use4addr(uint8 ifidx);

/* ==================== STA管理API ==================== */

/**
 * @brief 获取单个STA的详细信息（AP模式）
 * @param ifidx [in] 接口索引
 * @param aid [in] STA的关联ID，如果非0则按AID查找
 * @param mac [in] STA的MAC地址，如果非NULL则优先按MAC查找
 * @param sta [out] 输出STA信息结构体
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_get_stainfo(uint8 ifidx, uint16 aid, uint8 *mac, struct ieee80211_stainfo *sta);

/**
 * @brief 获取所有已连接STA的信息列表 - ieee80211_stainfo 格式
 * @param ifidx [in] 接口索引
 * @param sta [out] STA信息数组缓冲区
 * @param count [in] 缓冲区最多能存放的条目数
 * @return 实际获取的STA数量，失败返回负数
 */
extern int32 ieee80211_conf_get_stalist(uint8 ifidx, struct ieee80211_stainfo *sta, uint32 count);

/**
 * @brief 获取所有已连接STA的信息列表 - hgic_sta_info 格式
 * @param ifidx [in] 接口索引
 * @param sta [out] STA信息数组缓冲区
 * @param count [in] 缓冲区最多能存放的条目数
 * @return 实际获取的STA数量，失败返回负数
 */
extern int32 ieee80211_get_stalist(uint8 ifidx, struct hgic_sta_info *sta, int32 count);

/**
 * @brief 获取指定STA的信噪比（SNR）
 * @param ifidx [in] 接口索引
 * @param aid [in] STA的AID
 * @param snr [out] 输出信噪比值（dB）
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_get_sta_snr(uint8 ifidx, uint8 aid, int8 *snr);

/**
 * @brief 获取当前连接的STA总数（AP模式）
 * @param ifidx [in] 接口索引
 * @return STA数量，失败返回负数
 */
extern int32 ieee80211_conf_get_stacnt(uint8 ifidx);

/* ==================== AP配置API ==================== */

/**
 * @brief 设置AP是否隐藏SSID（不广播Beacon中的SSID）
 * @param ifidx [in] 接口索引
 * @param hide [in] 1=隐藏，0=广播
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_aphide(uint8 ifidx, uint8 hide);

/**
 * @brief 获取指定信道的背景RSSI（噪声基底）
 * @param ifidx [in] 接口索引
 * @param channel [in] 信道号
 * @param bgr [out] 输出背景RSSI值（dBm）
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_get_bgrssi(uint8 ifidx, uint8 channel, uint8 *bgr);

/**
 * @brief 设置组播（多播）帧的发送速率
 * @param ifidx [in] 接口索引
 * @param txrate [in] 速率值（单位取决于底层）
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_mcast_txrate(uint8 ifidx, uint32 txrate);

/**
 * @brief 设置接口的WiFi协议模式（如802.11b/g/n等）
 * @param ifidx [in] 接口索引
 * @param mode [in] 参见 enum ieee80211_hwmode
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_hwmode(uint8 ifidx, enum ieee80211_hwmode mode);

/**
 * @brief 设置AP为休眠STA缓存的最大数据帧数量
 * @param ifidx [in] 接口索引
 * @param max_cnt [in] 最大缓存帧数
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_psdata_cnt(uint8 ifidx, uint8 max_cnt);

/* ==================== WMM配置API ==================== */

/**
 * @brief 设置WMM EDCA参数（特定访问类别AC）
 * @param ifidx [in] 接口索引
 * @param ac [in] 访问类别：0=背景(AC_BK),1=尽力(AC_BE),2=视频(AC_VI),3=语音(AC_VO)
 * @param param [in] EDCA参数结构体
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_wmm_param(uint8 ifidx, uint8 ac, struct ieee80211_wmm_param *param);

/**
 * @brief 获取WMM EDCA参数
 * @param ifidx [in] 接口索引
 * @param ac [in] 访问类别
 * @param param [out] 输出EDCA参数
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_get_wmm_param(uint8 ifidx, uint8 ac, struct ieee80211_wmm_param *param);

/**
 * @brief 设置组播密钥的更新周期（仅AP模式）
 * @param ifidx [in] 接口索引
 * @param wpa_group_rekey [in] 更新周期（秒），0表示不自动更新
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_wpa_group_rekey(uint8 ifidx, uint32 wpa_group_rekey);

/* ==================== 桥接/转发API ==================== */

/**
 * @brief 初始化STA接口的桥接路由表（用于STABR模式）
 * @param ifidx [in] 接口索引
 * @param max [in] 路由表最大条目数
 * @param lifetime [in] 条目生存时间（秒）
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_stabr_table(uint8 ifidx, uint16 max, int32 lifetime);

/**
 * @brief 获取转发（Deliver）模块的状态信息（文本）
 * @param buff [out] 输出缓冲区
 * @param size [in] 缓冲区大小
 * @return 0成功，非0失败
 */
extern int32 ieee80211_deliver_status(uint8 *buff, uint32 size);

/**
 * @brief 获取STABR（STA桥接）模块的状态信息（文本）
 * @param ifidx [in] 接口索引
 * @param buff [out] 输出缓冲区
 * @param size [in] 缓冲区大小
 * @return 0成功，非0失败
 */
extern int32 ieee80211_stabr_status(uint8 ifidx, uint8 *buff, uint32 size);

/* ==================== 数据标签/过滤API ==================== */

/**
 * @brief 为指定的数据包句柄设置标签值（用于过滤或识别）
 * @param ifidx [in] 接口索引
 * @param hdl [in] 数据句柄（从钩子中的 ieee80211_hookdata.hdl 获得）
 * @param tag [in] 要设置的标签（32位用户自定义值）
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_datatag(uint8 ifidx, uint32 hdl, uint32 tag);

/**
 * @brief 获取当前选择的天线（用于多天线设备）
 * @param ifidx [in] 接口索引
 * @return 天线选择值，失败返回负数
 */
extern int32 ieee80211_conf_get_ant_sel(uint8 ifidx);

/**
 * @brief 设置重复包过滤（开启后，协议栈会丢弃同一帧的重复副本）
 * @param ifidx [in] 接口索引
 * @param enable [in] 1=开启过滤，0=关闭
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_dupfilter(uint8 ifidx, uint8 enable);

/* ==================== 错误码API ==================== */

/**
 * @brief 获取最近一次连接失败的原因码（reason code）
 * @param ifidx [in] 接口索引
 * @return 标准802.11原因码或厂商自定义原因码（见 IEEE80211_TXREASON）
 */
extern int32 ieee80211_conf_get_reason_code(uint8 ifidx);

/**
 * @brief 获取最近一次连接失败的状态码（status code）
 * @param ifidx [in] 接口索引
 * @return 802.11状态码
 */
extern int32 ieee80211_conf_get_status_code(uint8 ifidx);

/* ==================== RTC/时间API ==================== */

/**
 * @brief 获取协议栈的RTC时间值（可能用于同步）
 * @param ifidx [in] 接口索引
 * @return RTC值，失败返回负数
 */
extern int32 ieee80211_conf_get_rtc(uint8 ifidx);

/**
 * @brief 设置协议栈的RTC时间
 * @param ifidx [in] 接口索引
 * @param data [in] 时间数据（格式取决于实现）
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_rtc(uint8 ifidx, uint8 *data);

/**
 * @brief 获取ACS（自动信道选择）的结果
 * @param ifidx [in] 接口索引
 * @param buff [out] 结果缓冲区（至少1字节，内容为最佳信道号）
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_get_acs_result(uint8 ifidx, uint8 *buff);

/* ==================== 射频控制API ==================== */

/**
 * @brief 开启或关闭射频（相当于硬件开关WiFi）
 * @param ifidx [in] 接口索引
 * @param en [in] 1=开启，0=关闭
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_radio_onoff(uint8 ifidx, uint8 en);

/**
 * @brief 设置接口隔离（多接口时各接口之间是否可以互通转发）
 * @param ifidx [in] 接口索引
 * @param isolate [in] 1=隔离，0=不隔离
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_isolate(uint8 ifidx, uint8 isolate);

/* ==================== 802.11r快速漫游API ==================== */

/**
 * @brief 设置802.11r快速漫游参数（用于STA在AP间快速切换）
 * @param ifidx [in] 接口索引
 * @param ft_param [in] 漫游参数结构体指针
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_ft(uint8 ifidx, void *ft_param);

/* ==================== BSS信息API ==================== */

/**
 * @brief 根据信道号计算中心频率（MHz）
 * @param ifidx [in] 接口索引（用于确定频段信息）
 * @param channel [in] 信道号
 * @return 频率值（MHz），失败返回负数
 */
extern int32 ieee80211_chan_center_freq(uint8 ifidx, uint8 channel);

/**
 * @brief 获取指定BSS的详细信息
 * @param bss [out] 输出BSS信息结构体（hgic_bss_info）
 * @param band [in] 频段
 * @param ssid [in] SSID字符串
 * @param bssid [in] BSSID（MAC地址）
 * @return 0成功，非0失败
 */
extern int32 ieee80211_get_bssinfo(struct hgic_bss_info *bss, uint8 band, uint8 *ssid, uint8 *bssid);

/**
 * @brief 手动添加一个AP的信息到协议栈的BSS列表（用于跳过扫描，快速连接）
 * @param ssid [in] SSID
 * @param bssid [in] BSSID
 * @param channel [in] 信道号
 * @param band [in] 频段
 * @param wpa_data [in] 加密信息（通过ieee80211_bss_get_wpadata获取后保存）
 * @return 0成功，非0失败
 */
extern int32 ieee80211_bss_add_manualAP(uint8 *ssid, uint8 *bssid, uint8 channel, uint8 band, struct ieee80211_bss_wpadata *wpa_data);

/**
 * @brief 获取指定AP的WPA加密信息（从扫描结果中提取）
 * @param bssid [in] AP的MAC地址
 * @param band [in] 频段
 * @param wpa_data [out] 输出加密信息结构体
 * @return 0成功，非0失败
 */
extern int32 ieee80211_bss_get_wpadata(uint8 *bssid, uint8 band, struct ieee80211_bss_wpadata *wpa_data);

/* ==================== MLME扩展API ==================== */

/**
 * @brief 扩展MLME管理帧的预分配大小，以便添加自定义信息元素（IE）
 * @param ifidx [in] 接口索引
 * @param size [in] 各帧类型的扩展字节数配置
 * @return 0成功，非0失败
 * @note 必须在接口创建后、启动前调用。
 */
extern int32 ieee80211_expand_mlme_size(uint8 ifidx, struct ieee80211_mlme_size *size);

/**
 * @brief 设置信道切换通告（CSA）参数，通知连接的STA即将切换信道
 * @param ifidx [in] 接口索引
 * @param csa [in] CSA参数
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_csa(uint8 ifidx, struct ieee80211_csa_param *csa);

/**
 * @brief 设置WPA加密算法（加密方式）
 * @param ifidx [in] 接口索引
 * @param wpa_proto [in] 协议版本，WPA_PROTO_WPA 或 WPA_PROTO_RSN
 * @param pairwise_cipher [in] 单播加密算法（如 WPA_CIPHER_CCMP）
 * @param group_cipher [in] 组播加密算法
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_wpa_cipher(uint8 ifidx, uint32 wpa_proto, uint32 pairwise_cipher, uint32 group_cipher);

/* ==================== 连接超时API ==================== */

/**
 * @brief 设置连接过程中的各个阶段超时时间
 * @param ifidx [in] 接口索引
 * @param auth_tmo [in] 认证阶段超时时间（毫秒），默认100ms
 * @param assoc_tmo [in] 关联阶段超时（毫秒），默认100ms
 * @param handshake_tmo [in] 4次握手阶段超时（毫秒），默认2000ms
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_conn_timeout(uint8 ifidx, uint32 auth_tmo, uint32 assoc_tmo, uint32 handshake_tmo);

/* ==================== P2P配置API ==================== */

extern int32 ieee80211_conf_set_p2p_dev_name(uint8 ifidx, char *dev_name);
extern int32 ieee80211_conf_set_p2p_model_name(uint8 ifidx, char *model_name);
extern int32 ieee80211_conf_set_p2p_model_num(uint8 ifidx, char *model_num);
extern int32 ieee80211_conf_set_p2p_vendor_name(uint8 ifidx, char *vendor_name);
extern int32 ieee80211_conf_set_p2p_serial_number(uint8 ifidx, char *serial_number);
extern int32 ieee80211_conf_set_p2p_uuid(uint8 ifidx, char *uuid);
extern int32 ieee80211_conf_set_p2p_methods(uint8 ifidx, char *methods);
extern int32 ieee80211_conf_set_p2p_dev_type(uint8 ifidx, char *dev_type);
extern int32 ieee80211_conf_set_wfd_element(uint8 ifidx, uint8 id, uint8 *data, uint16 len);
extern int32 ieee80211_conf_set_absolute_beacon(uint8 ifidx, uint8 en);

/* ==================== 自动连接API ==================== */

/**
 * @brief 设置STA是否自动连接和自动扫描
 * @details 如果关闭了自动连接和自动扫描，则需要通过执行 ieee80211_start_connect 和 ieee80211_scan API来建立连接和扫描。
 * @param ifidx [in] 接口索引
 * @param auto_assoc [in] 1=自动连接，0=手动
 * @param auto_scan [in] 1=自动扫描，0=不自动扫描
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_auto_assoc(uint8 ifidx, uint8 auto_assoc, uint8 auto_scan);

/* ==================== 中继模式API ==================== */

/**
 * @brief 设置中继模式（Relay）参数
 * @param ifidx [in] 接口索引
 * @param enable [in] 1=开启中继模式，0=关闭
 * @param relay_thrs [in] 中继阈值（信号强度低于此值时触发中继）
 * @param relay_mcast [in] 是否中继组播包（1=是，0=否）
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_relay_mode(uint8 ifidx, uint8 enable, uint8 relay_thrs, uint8 relay_mcast);

/**
 * @brief 获取当前设备在中继网络中的层级（0=根节点）
 * @param ifidx [in] 接口索引
 * @return 层级数，失败返回负数
 */
extern int32 ieee80211_conf_get_relay_level(uint8 ifidx);

/* ==================== 监听间隔API ==================== */

/**
 * @brief 设置STA的监听间隔（Listen Interval）
 * @param ifidx [in] 接口索引
 * @param listen_interval [in] 监听间隔（以Beacon周期为单位），如10表示每10个Beacon醒来一次
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_listen_interval(uint8 ifidx, uint16 listen_interval);

/**
 * @brief 设置最大扫描次数（连续扫描失败多少次后停止）
 * @param ifidx [in] 接口索引
 * @param scan_max [in] 最大次数
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_scan_max(uint8 ifidx, uint16 scan_max);

/* ==================== 信号阈值API ==================== */

/**
 * @brief 设置RSSI和EVM的波动阈值（超过此值时触发相应事件）
 * @param ifidx [in] 接口索引
 * @param rssi_thres [in] RSSI变化阈值（dB），默认6
 * @param evm_thres [in] EVM变化阈值（dB），默认6
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_signal_threshold(uint8 ifidx, uint8 rssi_thres, uint8 evm_thres);

/**
 * @brief 设置安全策略（如是否允许不加密连接等）
 * @param ifidx [in] 接口索引
 * @param security_policy [in] 策略值
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_security_policy(uint8 ifidx, uint8 security_policy);

/**
 * @brief 设置SAE（WPA3）密码加密循环次数（影响计算强度与速度）
 * @param ifidx [in] 接口索引
 * @param loop [in] 循环次数，默认根据配置
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_sae_pwe_loop(uint8 ifidx, uint8 loop);

/**
 * @brief 开启或关闭AP模式下的省电模式（AP侧省电，非STA侧）
 * @param ifidx [in] 接口索引
 * @param enable [in] 1=开启，0=关闭
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_ap_psmode_en(uint8 ifidx, uint8 enable);

/**
 * @brief 设置配对协商模式（是否允许在配对过程中进行角色协商）
 * @param ifidx [in] 接口索引
 * @param enable [in] 1=开启，0=关闭
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_pair_ngo(uint8 ifidx, uint8 enable);

/**
 * @brief 设置是否允许多设备配对（一对多）
 * @param ifidx [in] 接口索引
 * @param enable [in] 1=允许多设备配对，0=仅允许一对一
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_mutl_pair(uint8 ifidx, uint8 enable);

/**
 * @brief 设置链路质量评估参数（用于智能漫游或切换决策）
 * @param ifidx [in] 接口索引
 * @param param [in] 链路质量参数结构体
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_linkcost_param(uint8 ifidx, struct ieee80211_linkcost_param *param);

/**
 * @brief 设置配对过程中使用的信道
 * @param ifidx [in] 接口索引
 * @param chan [in] 信道号
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_pair_channel(uint8 ifidx, uint8 chan);

/* ==================== MCS配置API ==================== */

/**
 * @brief 获取指定STA当前的发送MCS（调制编码方案，代表速率）
 * @param ifidx [in] 接口索引
 * @param sta_addr [in] STA的MAC地址
 * @param aid [in] STA的AID（至少提供一种标识）
 * @return MCS索引，失败返回负数
 */
extern int32 ieee80211_conf_get_tx_mcs(uint8 ifidx, uint8 *sta_addr, uint32 aid);

/**
 * @brief 设置发送MCS（可能用于限速或固定速率）
 * @param ifidx [in] 接口索引
 * @param mcs [in] MCS值
 * @param type [in] 类型（特定于芯片）
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_tx_mcs(uint8 ifidx, uint32 mcs, uint32 type);

/* ==================== RMESH配置API ==================== */

/**
 * @brief 设置RMESH网络允许的最大设备数
 * @param ifidx [in] 接口索引
 * @param devmax [in] 最大设备数
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_rmesh_devmax(uint8 ifidx, uint8 devmax);

/**
 * @brief 设置本设备在RMESH网络中的AID
 * @param ifidx [in] 接口索引
 * @param aid [in] 要设置的AID值
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_rmesh_aid(uint8 ifidx, uint8 aid);

/**
 * @brief 在RMESH网络中注册一个设备（添加邻居）
 * @param ifidx [in] 接口索引
 * @param aid [in] 设备的AID
 * @param mac [in] 设备的MAC地址
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_rmesh_device(uint8 ifidx, uint8 aid, uint8 *mac);

/**
 * @brief 禁用或启用某个BSS（使其在扫描结果中被过滤）
 * @param ifidx [in] 接口索引
 * @param bssid [in] 目标BSSID
 * @param disable [in] 1=禁用（不再考虑连接此BSS），0=启用
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_bss_disable(uint8 ifidx, uint8 *bssid, uint8 disable);

/**
 * @brief 设置RMESH设备是否转发数据（非叶子节点需要转发）
 * @param ifidx [in] 接口索引
 * @param not [in] 1=不转发（只能一跳发送），0=转发（可以多跳发送）
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_rmesh_notfw(uint8 ifidx, uint8 not);

/**
 * @brief 设置RMESH连接所需的最小RSSI（低于此值的链路不考虑）
 * @param ifidx [in] 接口索引
 * @param rssi_min [in] 最小RSSI（dBm）
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_rmesh_rssimin(uint8 ifidx, int8 rssi_min);

/**
 * @brief 获取RMESH网络中已注册的所有设备
 * @param ifidx [in] 接口索引
 * @param dev [out] 设备信息数组缓冲区
 * @param cnt [in] 缓冲区最多可容纳设备数
 * @return 实际获取的设备数量，失败返回负数
 */
extern int32 ieee80211_conf_get_rmesh_device(uint8 ifidx, struct ieee80211_rmesh_device *dev, uint32 cnt);

/* ==================== 漫游API ==================== */

/**
 * @brief 初始化漫游功能（STA模式），使其能够在多个AP间切换
 * @param ifidx [in] 接口索引
 * @return 0成功，非0失败
 */
extern int32 ieee80211_wnb_roam_enable(uint8 ifidx);

/**
 * @brief 开启或关闭漫游功能
 * @param ifidx [in] 接口索引
 * @param roaming [in] 1=开启，0=关闭
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_roaming(uint8 ifidx, uint8 roaming);

/**
 * @brief 配置漫游触发条件
 * @param ifidx [in] 接口索引
 * @param roam_rssi_th [in] 漫游RSSI门限（dBm），低于此值时开始考虑漫游
 * @param roam_rssi_diff [in] 新旧AP的RSSI差值门限（dB），新AP需比旧AP强至少此值才切换
 * @param roam_rssi_int [in] 连续监测的Beacon数量（连续多次满足条件才触发）
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_roam_config(uint8 ifidx, int8 roam_rssi_th, uint8 roam_rssi_diff, uint8 roam_rssi_int);

/* ==================== 组播密钥API ==================== */

/**
 * @brief 设置或清除组播密钥（加密广播/组播数据）
 * @param ifidx [in] 接口索引
 * @param mkey_set [in] 1=设置密钥，0=清除密钥
 * @param key [in] 密钥数据（长度取决于加密算法）
 * @return 0成功，非0失败
 */
extern int32 ieee80211_conf_set_mcast_key(uint8 ifidx, uint8 mkey_set, uint8 *key);

/* ==================== 加密算法支持API ==================== */

extern void ieee80211_crypto_ec_support(void);      /**< 启用椭圆曲线加密支持（用于SAE等） */
extern void ieee80211_crypto_ecdh_support(void);    /**< 启用ECDH椭圆曲线密钥交换 */
extern void ieee80211_crypto_aes_support(void);     /**< 启用AES加密算法 */
extern void ieee80211_crypto_bignum_support(void);  /**< 启用大数运算支持 */
extern void ieee80211_crypto_dh_support(void);      /**< 启用Diffie-Hellman密钥交换 */
extern void ieee80211_crypto_sha_support(void);     /**< 启用SHA哈希算法 */
extern void ieee80211_crypto_hash_support(void);    /**< 启用通用哈希函数 */

#ifdef __cplusplus
}
#endif
#endif
