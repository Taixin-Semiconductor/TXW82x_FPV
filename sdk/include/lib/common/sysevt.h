#ifndef _SDK_SYSEVT_H_
#define _SDK_SYSEVT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file sysevt.h
 * @brief 系统全局事件分发与管理中心 (System Event Dispatcher)
 * 
 * @details
 * 本模块提供了一套轻量级的发布/订阅 (Pub/Sub) 机制，用于系统各子系统之间的异步通信。
 * 
 * 核心特性：
 * 1. **分层编码**：事件 ID 由 16位主ID (Main ID) 和 16位子ID (Sub ID) 组成，支持多达 65536 种事件类型。
 * 2. **解耦设计**：事件发送者无需知道谁接收了事件，接收者只需注册感兴趣的事件 ID。
 * 3. **多听众支持**：同一个事件 ID 可以注册多个回调函数，按注册顺序依次调用。
 * 4. **灵活处理**：回调函数可决定事件是否被“消费”(Consumed)，从而阻止后续回调执行。
 * 5. **事件参数**：每个事件可以携带一个uint32类型参数，便于反馈更多的信息。
 * 
 * 典型应用场景：
 * - WiFi 连接状态变化通知应用层。
 * - 网络 IP 获取成功通知 NTP 或 MQTT 模块。
 * - 多媒体播放结束通知 UI 层更新状态。
 * - 低功耗唤醒后通知各模块恢复运行。
 */

/**
 * @brief 事件处理返回值枚举
 */
typedef enum {
    SYSEVT_CONTINUE = 0,  ///< 继续传递：事件未被完全处理，继续调用下一个注册的回调函数
    SYSEVT_CONSUMED = 1,  ///< 已消费：事件已被处理完毕，停止调用后续回调函数
} sysevt_hdl_res;

/**
 * @brief 事件回调函数类型定义
 * @param event_id [In] 触发的事件完整 ID (由主 ID 和子 ID 组成)
 * @param data     [In] 事件携带的私有数据 (具体含义见各子事件定义)
 * @param priv     [In] 用户私有数据 (注册时传入的 context)
 * @return 处理结果 (SYSEVT_CONTINUE 或 SYSEVT_CONSUMED)
 */
typedef sysevt_hdl_res (*sysevt_hdl)(uint32 event_id, uint32 data, uint32 priv);

/**
 * @brief 初始化事件系统
 * @param evt_max_cnt [In] 支持的最大并发事件数量
 * @return 0 表示成功，负值表示失败
 * @note 必须在系统启动早期调用，在使用其他接口前完成初始化。
 */
int32 sys_event_init(uint16 evt_max_cnt);

/**
 * @brief 触发/发送一个新事件
 * @param event_id [In] 事件 ID (建议使用 SYS_EVENT 宏构造)
 * @param data     [In] 事件负载数据 (整数或句柄，具体含义取决于事件类型)
 * @return 0 表示成功投递，负值表示失败 (如系统未初始化、资源不足)
 * @details 
 * 调用此函数后，系统会遍历所有注册了该 `event_id` 的回调函数并依次执行。
 */
int32 sys_event_new(uint32 event_id, uint32 data);

/**
 * @brief 注册事件监听器 (订阅事件)
 * @param event_id [In] 要监听的事件 ID. 
 *                     - event_id=0xffffffff        : 订阅所有的事件; 
 *                     - event_id=mainID<<16        : 订阅一类事件
 *                     - event_id=mainID<<16|subID  : 订阅一个事件
 * @param hdl      [In] 回调函数指针
 * @param priv     [In] 传递给回调函数的私有数据 (用户上下文)
 * @return 0 表示成功，负值表示失败
 * @note 同一个事件 ID 可以被多次调用此函数注册不同的回调。
 *       每次订阅会消耗16byte内存资源，建议多个事件复用一次订阅，这样可以节省内存资源消耗。
 */
int32 sys_event_take(uint32 event_id, sysevt_hdl hdl, uint32 priv);

/**
 * @brief 注销事件监听器 (取消订阅)
 * @param event_id [In] 要取消监听的事件 ID
 * @param hdl      [In] 之前注册的回调函数指针
 * @details 
 * 必须确保 hdl 和 event_id 与注册时完全一致才能成功移除。
 */
void sys_event_untake(uint32 event_id, sysevt_hdl hdl);

/**
 * @brief 构造事件 ID 的宏
 * @param main 主事件 ID (高 16 位，代表模块，如 SYS_EVENT_WIFI)
 * @param sub  子事件 ID (低 16 位，代表具体事件，如 SYSEVT_WIFI_CONNECTTED)
 * @return 组合后的 32 位事件 ID
 * @example SYS_EVENT(SYS_EVENT_WIFI, SYSEVT_WIFI_CONNECTTED)
 */
#define SYS_EVENT(main, sub) ((main) << 16 | ((sub) & 0xffff))

/* -------------------------------------------------------------------------- */
/*                         主事件 ID 定义 (Main Event IDs)                     */
/* -------------------------------------------------------------------------- */
/**
 * @brief 系统主模块事件 ID 枚举
 * @details 定义了一级分类，占用事件 ID 的高 16 位。
 */
enum SYSEVT_MAINID {
    SYS_EVENT_NETWORK = 1,  ///< 网络栈事件 (LwIP, DHCP, Link Status)
    SYS_EVENT_WIFI,         ///< WiFi 协议栈事件 (Connect, Scan, Auth)
    SYS_EVENT_LMAC,         ///< LMAC 事件 (TX Status, ACS)
    SYS_EVENT_SYSTEM,       ///< 系统级事件 (Resume, Mount, Task Delete)
    SYS_EVENT_BLE,          ///< 蓝牙低功耗事件 (Connect, MTU, Config)
    SYS_EVENT_LTE,          ///< LTE 蜂窝网络事件 (Connect, Overlap)
    SYS_EVENT_MEDIA,        ///< 多媒体播放事件 (Play, Stop, Error, Demux)
    SYS_EVENT_USB,          ///< USB 总线及设备事件 (Connect, Disconnect)
    SYS_EVENT_ASR,          ///< 语音识别事件 (Wake Up, VAD)
    SYS_EVENT_ENV,          ///< 环境信息事件 (temperature, humidity, location)

    SYSEVT_MAINID_ID,       ///< 边界标记，表示主 ID 的数量
};

/* -------------------------------------------------------------------------- */
/*                         系统模块子事件 (System Sub Events)                  */
/* -------------------------------------------------------------------------- */
enum SYSEVT_SYSTEM_SUBEVT {
    SYSEVT_SYSTEM_RESUME = 1,    ///< 系统从休眠/暂停中恢复
    SYSEVT_SYSTEM_SD_MOUNT,      ///< SD 设备文件系统挂载成功
    SYSEVT_SYSTEM_SD_UNMOUNT,    ///< SD 设备文件系统卸载
    SYSEVT_TASK_DELETE,          ///< 任务删除通知 (用于资源清理)
    SYSEVT_SYSTEM_PLUGIN,        ///< 设备插入. data:[dev_id << 16 | dev_type]
    SYSEVT_SYSTEM_PLUGOUT,       ///< 设备拔出. data:[dev_id << 16 | dev_type]
    SYSEVT_SYSTEM_USB_MOUNT,     ///< USB 设备文件系统挂载成功
    SYSEVT_SYSTEM_USB_UNMOUNT,   ///< USB 设备文件系统卸载

    SYSEVT_SYSTEM_TOTAL_VOLUME,  ///< 总音量
    SYSEVT_SYSTEM_MEDIA_VOLUME,  ///< 媒体音量
    SYSEVT_SYSTEM_CALL_VOLUME,   ///< 通话音量

    SYSEVT_SYSTEM_BRIGHTNESS,    ///< 亮度
    SYSEVT_SYSTEM_BATTERY_LEVEL, ///< 电池电量
};
/** @brief 快捷宏：发送系统事件 */
#define SYSEVT_NEW_SYSTEM_EVT(subevt, data) sys_event_new(SYS_EVENT(SYS_EVENT_SYSTEM, subevt), (uint32)data)

/* -------------------------------------------------------------------------- */
/*                         LMAC 模块子事件 (Low MAC Sub Events)                */
/* -------------------------------------------------------------------------- */
enum SYSEVT_LMAC_SUBEVT {
    SYSEVT_LMAC_ACS_DONE = 1,            ///< 自动信道选择 (ACS) 完成
    SYSEVT_LMAC_TX_STATUS = 2,           ///< 底层发送状态报告 (ACK/Retry)
    SYSEVT_LMAC_APP_HBDATA_DETECT = 3,   ///< 检测到应用层的心跳包 (用于判断保活包是否进入LMAC层)
};
/** @brief 快捷宏：发送 LMAC 事件 */
#define SYSEVT_NEW_LMAC_EVT(subevt, data) sys_event_new(SYS_EVENT(SYS_EVENT_LMAC, subevt), (uint32)data)

/* -------------------------------------------------------------------------- */
/*                         WiFi 模块子事件 (WiFi Sub Events)                   */
/* -------------------------------------------------------------------------- */
enum SYSEVT_WIFI_SUBEVT {
    SYSEVT_WIFI_CONNECT_START = 1,   ///< STA 开始连接 (data: 0)
    SYSEVT_WIFI_CONNECTTED,          ///< STA 连接成功 (data: 关联 ID / AID)
    SYSEVT_WIFI_CONNECT_FAIL,        ///< STA 连接失败 (data: 状态码/错误码)
    SYSEVT_WIFI_DISCONNECT,          ///< 断开连接 (保留未用)
    SYSEVT_WIFI_SCAN_START,          ///< 开始扫描 (data: 0)
    SYSEVT_WIFI_SCAN_DONE,           ///< 扫描完成 (data: 0)
    SYSEVT_WIFI_STA_DISCONNECT,      ///< AP 模式下 STA 断开 (data: STA 的 AID)
    SYSEVT_WIFI_STA_CONNECTTED,      ///< AP 模式下 STA 连接成功 (data: STA 的 AID)
    SYSEVT_WIFI_STA_PS_START,        ///< STA 进入节能模式 (data: STA 的 AID)
    SYSEVT_WIFI_STA_PS_END,          ///< STA 退出节能模式 (data: STA 的 AID)
    SYSEVT_WIFI_PAIR_DONE,           ///< 配对完成 (data: 1=成功，0=失败)
    SYSEVT_WIFI_TX_SUCCESS,          ///< WiFi 发送成功 (data: 用户设定的数据标签 tag)
    SYSEVT_WIFI_TX_FAIL,             ///< WiFi 发送失败 (data: 用户设定的数据标签 tag)
    SYSEVT_WIFI_UNPAIR,              ///< 取消配对 (data: 0)
    SYSEVT_WIFI_WRONG_KEY,           ///< 密码错误 (data: 0)
    SYSEVT_WIFI_P2P_DONE,            ///< P2P WSC 协商完成 (需更新系统配置)
    SYSEVT_WIFI_NO_FREE_STA,         ///< 协议栈STA数量已用完
    SYSEVT_WIFI_INTERFACE_ENABLE,    ///< 接口启用
    SYSEVT_WIFI_INTERFACE_DISABLE,   ///< 接口禁用
    SYSEVT_WIFI_WAKEUP_HOST,         ///< 协议栈检测到需要唤醒主控. data:唤醒原因
};
/** @brief 快捷宏：发送 WiFi 事件 */
#define SYSEVT_NEW_WIFI_EVT(subevt, data) sys_event_new(SYS_EVENT(SYS_EVENT_WIFI, subevt), (uint32)data)

/* -------------------------------------------------------------------------- */
/*                         网络模块子事件 (Network Sub Events)                 */
/* -------------------------------------------------------------------------- */
enum SYSEVT_NETWORK_SUBEVT {
    SYSEVT_GMAC_LINK_UP = 1,      ///< 有线网口 (GMAC) 链路接通
    SYSEVT_GMAC_LINK_DOWN,        ///< 有线网口 (GMAC) 链路断开
    SYSEVT_LWIP_DHCPC_START,      ///< LwIP DHCP 客户端开始请求
    SYSEVT_LWIP_DHCPC_DONE,       ///< LwIP DHCP 客户端获取 IP 完成
    SYSEVT_WIFI_DHCPC_START,      ///< WiFi 接口 DHCP 客户端开始请求
    SYSEVT_WIFI_DHCPC_DONE,       ///< WiFi 接口 DHCP 客户端获取 IP 完成
    SYSEVT_DHCPD_NEW_IP,          ///< DHCP 服务器分配了新 IP 给客户端
    SYSEVT_DHCPD_IPPOOL_FULL,     ///< DHCP 服务器地址池已满
    SYSEVT_NTP_UPDATE,            ///< NTP 时间同步更新完成
};
/** @brief 快捷宏：发送网络事件 */
#define SYSEVT_NEW_NETWORK_EVT(subevt, data) sys_event_new(SYS_EVENT(SYS_EVENT_NETWORK, subevt), (uint32)data)

/* -------------------------------------------------------------------------- */
/*                         BLE 模块子事件 (Bluetooth Sub Events)               */
/* -------------------------------------------------------------------------- */
enum SYSEVT_BLE_SUBEVT {
    SYSEVT_BLE_CONNECTED = 1,         ///< BLE 连接建立
    SYSEVT_BLE_DISCONNECT,            ///< BLE 连接断开
    SYSEVT_BLE_NETWORK_CONFIGURED,    ///< BLE 网络配置完成 (如 Mesh 或 Proxy)
    SYSEVT_BLE_EXCHANGE_MTU,          ///< BLE MTU 交换完成
};
/** @brief 快捷宏：发送 BLE 事件 */
#define SYSEVT_NEW_BLE_EVT(subevt, data) sys_event_new(SYS_EVENT(SYS_EVENT_BLE, subevt), (uint32)data)

/* -------------------------------------------------------------------------- */
/*                         LTE 模块子事件 (LTE Sub Events)                     */
/* -------------------------------------------------------------------------- */
enum SYSEVT_LTE_SUBEVT {
    SYSEVT_LTE_CONNECTED = 1,         ///< LTE 连接建立
    SYSEVT_LTE_DISCONNECT,            ///< LTE 连接断开
    SYSEVT_LTE_NETWORK_CONFIGURED,    ///< LTE 网络配置完成
    SYSEVT_LTE_OVERLAP_WIFI,          ///< LTE 与 WiFi 共存/冲突事件
};
/** @brief 快捷宏：发送 LTE 事件 */
#define SYSEVT_NEW_LTE_EVT(subevt, data) sys_event_new(SYS_EVENT(SYS_EVENT_LTE, subevt), (uint32)data)

/* -------------------------------------------------------------------------- */
/*                         多媒体模块子事件 (Media Sub Events)                 */
/* -------------------------------------------------------------------------- */
enum SYSEVT_MEDIA_SUBEVT {
    // 播放控制状态
    SYSEVT_MEDIA_PLAY_START = 1,      ///< 启动播放. data:[stream_id]
    SYSEVT_MEDIA_PLAY_STOP,           ///< 停止播放. data:[stream_id]
    SYSEVT_MEDIA_PLAYING,             ///< 正在播放中
    SYSEVT_MEDIA_PLAY_PAUSE,          ///< 暂停播放. data:[stream_id]
    SYSEVT_MEDIA_PLAY_SPEED,          ///< 播放倍速改变
    SYSEVT_MEDIA_PLAY_SEEK_ERR,       ///< 跳转 (Seek) 失败. data:[stream_id]
    SYSEVT_MEDIA_PLAY_CLOSE,          ///< 播放器关闭. data:0
    SYSEVT_MEDIA_PLAY_END,            ///< 播放自然结束. data:[stream_id]
    SYSEVT_MEDIA_VOLUME,              ///< 播放音量调整. data:[volume << 8 | stream_id]

    // 数据流状态
    SYSEVT_MEDIA_OPEN_SUCCESS,        ///< 媒体源打开成功. data:[stream_id]
    SYSEVT_MEDIA_OPEN_FAIL,           ///< 媒体源打开失败. data:[stream_id]
    SYSEVT_MEDIA_READ_EOF,            ///< 读取到文件末尾 (End of File)
    SYSEVT_MEDIA_READ_ERROR,          ///< 读取数据出错
    SYSEVT_MEDIA_BUFFERING_DATA,      ///< 正在缓冲数据. data:[buf_leve(百分比)<< 8 | stream_id]

    // 格式与解码状态
    SYSEVT_MEDIA_UNKNOWN_CONTAINER,   ///< 不支持的容器格式 (如 mp4, mkv). data:[container_type<<8 | stream_id]
    SYSEVT_MEDIA_UNKNOWN_DECODER,     ///< 不支持的解码格式 (如 h264, aac). data:[codec_type<<8 | stream_id]
    SYSEVT_MEDIA_DECODER_MATCH,       ///< 解码器匹配成功
    SYSEVT_MEDIA_DECODER_ERROR,       ///< 解码过程中发生异常. data:[codec_type<<8 | stream_id]
    SYSEVT_MEDIA_CONTAINER_MATCH,     ///< 容器解析器匹配成功. data:[container_type<<8 | stream_id]
    SYSEVT_MEDIA_CONTAINER_OPEN_ERROR,///< 容器打开失败. data:[container_type<<8 | stream_id]
    SYSEVT_MEDIA_CONTAINER_DEMUX_ERROR///< 解复用 (Demux) 过程异常
};
/** @brief 快捷宏：发送多媒体事件 */
#define SYSEVT_NEW_MEDIA_EVT(subevt, data) sys_event_new(SYS_EVENT(SYS_EVENT_MEDIA, subevt), (uint32)data)

/* -------------------------------------------------------------------------- */
/*                         USB 模块子事件 (USB Sub Events)                     */
/* -------------------------------------------------------------------------- */
enum SYSEVT_USB_SUBEVT {
    SYSEVT_USB_DEVICE_CONNECT = 1,    ///< USB 设备插入/连接
    SYSEVT_USB_DEVICE_DISCONNECT,     ///< USB 设备拔出/断开
};
/** @brief 快捷宏：发送 USB 事件 */
#define SYSEVT_NEW_USB_EVT(subevt, data) sys_event_new(SYS_EVENT(SYS_EVENT_USB, subevt), (uint32)data)

/* -------------------------------------------------------------------------- */
/*                         ASR 模块子事件 (Voice Recognition Sub Events)       */
/* -------------------------------------------------------------------------- */
enum SYSEVT_ASR_SUBEVT {
    SYSEVT_ASR_WAKEUP = 1,            ///< 检测到语音唤醒词 (Wake Word)
    SYSEVT_ASR_VAD,                   ///< 语音活动检测 (Voice Activity Detection) 状态变化
};
/** @brief 快捷宏：发送 ASR 事件 */
#define SYSEVT_NEW_ASR_EVT(subevt, data) sys_event_new(SYS_EVENT(SYS_EVENT_ASR, subevt), (uint32)data)

/* -------------------------------------------------------------------------- */
/*                         ENV 信息子事件 (Environment Sub Events)       */
/* -------------------------------------------------------------------------- */
enum SYSEVT_ENV_SUBEVT {
    SYSEVT_ENV_TEMPERATURE = 1,       ///< 温度
    SYSEVT_ENV_HUMIDITY,              ///< 湿度
    SYSEVT_ENV_LOCATION,              ///< 位置
    SYSEVT_ENV_WEATHER,               ///< 天气
};
/** @brief 快捷宏：发送 ENV 事件 */
#define SYSEVT_NEW_ENV_EVT(subevt, data) sys_event_new(SYS_EVENT(SYS_EVENT_ENV, subevt), (uint32)data)

#ifdef __cplusplus
}
#endif

#endif /* _SDK_SYSEVT_H_ */
