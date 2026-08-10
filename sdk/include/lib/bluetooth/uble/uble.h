#ifndef _HGIC_UBLE_H_
#define _HGIC_UBLE_H_

#include "basic_include.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file uble.h
 * @brief Host GATT (Generic Attribute Profile) Layer Interface for BLE
 * 
 * @details
 * 本文件定义了 BLE 主机端 (Host) GATT 层的核心数据结构、常量定义及操作接口。
 * 主要用于构建 BLE 服务表 (Service Table)、处理属性协议 (ATT) 交互以及管理特征值 (Characteristic)。
 */

/* -------------------------------------------------------------------------- */
/*                                日志宏定义                                   */
/* -------------------------------------------------------------------------- */
#define uble_dbg(fmt, ...) os_printf(KERN_DEBUG"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define uble_err(fmt, ...) os_printf(KERN_ERR"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

/* -------------------------------------------------------------------------- */
/*                         GATT 特征值属性权限 (Properties)                    */
/* -------------------------------------------------------------------------- */
/** @defgroup GATT_PROPERTIES GATT Characteristic Properties
 *  @{ 
 *  定义特征值支持的访问权限和操作类型，可组合使用 (Bitmask)。
 */
#define UBLE_GATT_CHARAC_BROADCAST                   (0x01) ///< 允许广播该特征值
#define UBLE_GATT_CHARAC_READ                        (0x02) ///< 允许读取该特征值
#define UBLE_GATT_CHARAC_WRITE_WITHOUT_RESPONSE      (0x04) ///< 允许写入无需响应 (Write Command)
#define UBLE_GATT_CHARAC_WRITE                       (0x08) ///< 允许写入需响应 (Write Request)
#define UBLE_GATT_CHARAC_NOTIFY                      (0x10) ///< 允许服务器主动通知 (Notify)
#define UBLE_GATT_CHARAC_INDICATE                    (0x20) ///< 允许服务器主动指示 (Indicate, 需确认)
#define UBLE_GATT_CHARAC_AUTHENTICATED_SIGNED_WRITES (0x40) ///< 允许带签名的写入
#define UBLE_GATT_CHARAC_EXTENDED_PROPERTIES         (0x80) ///< 包含扩展属性描述符
/** @} */

/* -------------------------------------------------------------------------- */
/*                           ATT 协议错误码 (Error Codes)                      */
/* -------------------------------------------------------------------------- */
/** @defgroup ATT_ERRORS ATT Protocol Error Codes
 *  @{
 *  定义 ATT 协议交互中可能返回的标准错误码 (0x01 - 0x11)。
 */
#define UBLE_ATT_ERR_INVALID_HANDLE          0x01 ///< 无效的句柄 (Handle)
#define UBLE_ATT_ERR_READ_NOT_PERMITTED      0x02 ///< 读取不允许
#define UBLE_ATT_ERR_WRITE_NOT_PERMITTED     0x03 ///< 写入不允许
#define UBLE_ATT_ERR_INVALID_PDU             0x04 ///< 无效的 PDU
#define UBLE_ATT_ERR_INSUFFICIENT_AUTHEN     0x05 ///< 认证不足 (Authentication)
#define UBLE_ATT_ERR_REQ_NOT_SUPPORTED       0x06 ///< 请求不支持
#define UBLE_ATT_ERR_INVALID_OFFSET          0x07 ///< 无效的偏移量
#define UBLE_ATT_ERR_INSUFFICIENT_AUTHOR     0x08 ///< 授权不足 (Authorization)
#define UBLE_ATT_ERR_PREPARE_QUEUE_FULL      0x09 ///< 准备队列满
#define UBLE_ATT_ERR_ATTR_NOT_FOUND          0x0a ///< 属性未找到
#define UBLE_ATT_ERR_ATTR_NOT_LONG           0x0b ///< 属性不是长类型
#define UBLE_ATT_ERR_INSUFFICIENT_KEY_SZ     0x0c ///< 加密密钥长度不足
#define UBLE_ATT_ERR_INVALID_ATTR_VALUE_LEN  0x0d ///< 属性值长度无效
#define UBLE_ATT_ERR_UNLIKELY                0x0e ///< 不可能的错误 (通用错误)
#define UBLE_ATT_ERR_INSUFFICIENT_ENC        0x0f ///< 加密不足 (Encryption)
#define UBLE_ATT_ERR_UNSUPPORTED_GROUP       0x10 ///< 不支持的组类型
#define UBLE_ATT_ERR_INSUFFICIENT_RES        0x11 ///< 资源不足
/** @} */

/* -------------------------------------------------------------------------- */
/*                         ATT 操作码 (Opcodes)                                */
/* -------------------------------------------------------------------------- */
/** @defgroup ATT_OPCODES ATT Protocol Opcodes
 *  @{
 *  定义 ATT 协议中各种请求/响应/指示的操作码。
 */
#define UBLE_OPCODE_ATT_ERROR               (0x01) ///< 错误响应
#define UBLE_OPCODE_EXCHANGE_MTU_REQ        (0x02) ///< 交换 MTU 请求
#define UBLE_OPCODE_EXCHANGE_MTU_RSP        (0x03) ///< 交换 MTU 响应
#define UBLE_OPCODE_FIND_INFORMATION_REQ    (0x04) ///< 查找信息请求
#define UBLE_OPCODE_FIND_INFORMATION_RSP    (0x05) ///< 查找信息响应
#define UBLE_OPCODE_FIND_BY_TYPE_VALUE_REQ  (0x06) ///< 按类型值查找请求
#define UBLE_OPCODE_FIND_BY_TYPE_VALUE_RESP (0x07) ///< 按类型值查找响应
#define UBLE_OPCODE_READ_BY_TYPE_REQ        (0x08) ///< 按类型读取请求
#define UBLE_OPCODE_READ_BY_TYPE_RSP        (0x09) ///< 按类型读取响应
#define UBLE_OPCODE_READ_REQ                (0x0A) ///< 读取请求
#define UBLE_OPCODE_READ_RSP                (0x0B) ///< 读取响应
#define UBLE_OPCODE_READ_BLOB_REQ           (0x0C) ///< 读取长值请求 (Blob)
#define UBLE_OPCODE_READ_MULTIPLE_REQ       (0x0e) ///< 读取多个请求
#define UBLE_OPCODE_READ_MULTIPLE_RESP      (0x0f) ///< 读取多个响应
#define UBLE_OPCODE_READ_BY_GROUP_TYPE_REQ  (0x10) ///< 按组类型读取请求
#define UBLE_OPCODE_READ_BY_GROUP_TYPE_RSP  (0x11) ///< 按组类型读取响应
#define UBLE_OPCODE_WRITE_REQ               (0x12) ///< 写入请求
#define UBLE_OPCODE_WRITE_RESP              (0x13) ///< 写入响应
#define UBLE_OPCODE_PREPARE_WRITE_REQ       (0x16) ///< 准备写入请求 (长写)
#define UBLE_OPCODE_EXECUTE_WRITE_REQ       (0x18) ///< 执行写入请求
#define UBLE_OPCODE_EXECUTE_WRITE_RESP      (0x19) ///< 执行写入响应
#define UBLE_OPCODE_HANDLE_VALUE_NOTIFY     (0x1B) ///< 句柄值通知 (无需确认)
#define UBLE_OPCODE_HANDLE_VALUE_INDICATION (0x1D) ///< 句柄值指示 (需确认)
#define UBLE_OPCODE_HANDLE_VALUE_CONFIRM    (0x1E) ///< 句柄值确认
#define UBLE_OPCODE_WRITE_CMD               (0x52) ///< 写入命令 (无需响应)
#define UBLE_OPCODE_SIGNED_WRITE_CMD        (0xD2) ///< 带签名写入命令
/** @} */

/* -------------------------------------------------------------------------- */
/*                         数据类型枚举 (Value Types)                          */
/* -------------------------------------------------------------------------- */
/**
 * @brief GATT 特征值数据类型枚举
 * @details 用于定义特征值在本地存储或回调处理时的数据格式。
 */
enum UBLE_VALUE_TYPE {
    UBLE_VALUE_TYPE_UINT8,      ///< 8位无符号整数
    UBLE_VALUE_TYPE_UINT16,     ///< 16位无符号整数
    UBLE_VALUE_TYPE_UINT32,     ///< 32位无符号整数
    UBLE_VALUE_TYPE_UINT8_BIT,  ///< 8位位图/标志位
    UBLE_VALUE_TYPE_UINT16_BIT, ///< 16位位图/标志位
    UBLE_VALUE_TYPE_UINT32_BIT, ///< 32位位图/标志位
    UBLE_VALUE_TYPE_BYTES,      ///< 字节数组 (Binary Blob)
    UBLE_VALUE_TYPE_STRING,     ///< ASCII/UTF-8 字符串
    UBLE_VALUE_TYPE_HDL,        ///< 句柄类型 (通常用于内部引用)
};

/**
 * @brief 特征值条目结构 (Value Entry)
 * @details 
 * 用于描述特征值的详细属性，特别是当特征值需要动态生成或复杂解析时。
 * 配合 `uble_gatt_valhdl` 回调函数使用。
 */
struct uble_value_entry {
    uint8  type;    ///< 数据类型 (见 enum UBLE_VALUE_TYPE)
    uint8  bitoff;  ///< 位偏移量 (用于位图类型，指定起始位)
    uint16 maskbit; ///< 位掩码 (用于位图类型，指定有效位)
    uint32 size;    ///< 数据总大小 (字节数)
    void  *value;   ///< 数据指针 (指向实际数值、缓冲区或用户自定义结构)
};

/* -------------------------------------------------------------------------- */
/*                         GATT 服务/特征值数据结构                            */
/* -------------------------------------------------------------------------- */

/**
 * @brief GATT 属性数据定义结构
 * @details 
 * 这是构建 GATT 数据库 (Database) 的核心单元。每个实例代表一个属性 (Attribute)，
 * 可以是服务 (Service)、包含声明 (Include) 或特征值声明 (Characteristic)。
 * 
 * 内存布局说明：
 * - 若 `UBLE_UUID_128_SUPPORT` 开启：支持 128-bit UUID，否则仅支持 16-bit UUID。
 * - `att_type`: 属性的 UUID (如 0x2800 表示 Primary Service, 0x2803 表示 Characteristic)。
 * - `properties`: 特征值权限位 (仅对特征值有效，其他属性通常为 0)。
 * - `att_value`: 
 *   - 对于服务/包含：通常是 16-bit UUID 或 128-bit UUID。
 *   - 对于特征值：通常是指向 `uble_value_entry` 的指针索引，或者是直接的值。
 */
#if UBLE_UUID_128_SUPPORT
struct uble_gatt_data {
    union {
        uint16 att_type;       ///< 16-bit UUID (短 UUID)
        uint8  att_type_128[16]; ///< 128-bit UUID (长 UUID)
    };
    uint16 properties;         ///< 特征值属性权限 (Properties)
    union {
        uint32 att_value;      ///< 属性值索引 (指向 uble_value_entry) 或直接值
        uint8  att_value_128[16]; ///< 128-bit 属性值
    };
};
#else
struct uble_gatt_data {
    uint16 att_type;           ///< 16-bit UUID
    uint16 properties;         ///< 特征值属性权限
    uint32 att_value;          ///< 属性值索引 (指向 uble_value_entry) 或直接值
};
#endif

/**
 * @brief GATT 特征值读写回调函数类型
 * @details 
 * 当远程设备发起 Read 或 Write 请求时，栈将调用此函数。
 * @param entry  [In]  对应的特征值条目描述
 * @param read   [In]  操作类型：1=读操作，0=写操作
 * @param buff   [InOut] 
 *               - 读操作：缓冲区用于填入要返回的数据
 *               - 写操作：缓冲区包含接收到的数据
 * @param size   [In]  缓冲区长度或数据长度
 *               - 读操作：缓冲区的长度
 *               - 写操作：收到数据的长度
 * @param offset [In]  读写偏移量 (用于长特征值的分片读写)
 * @return 成功返回实际处理的字节数，失败返回负值 (ATT Error Code)
 */
typedef int32 (*uble_gatt_valhdl)(const struct uble_value_entry *entry, uint8 read, uint8 *buff, int32 size, uint32 offset);

/* -------------------------------------------------------------------------- */
/*                             核心 API 接口                                   */
/* -------------------------------------------------------------------------- */

/**
 * @brief 初始化 BLE GATT 服务表
 * @param att_table     [In] GATT 属性表数组指针 (由 `struct uble_gatt_data` 组成)
 * @param att_table_size [In] 属性表条目数量
 * @param att_mtu       [In] 初始协商的 MTU 大小
 * @param adv_rx        [In] 广播数据接收回调函数
 * @return 0 表示成功，负值表示失败
 * @note 必须在连接建立前调用，用于注册本地服务。
 */
extern int32 uble_init(const struct uble_gatt_data *att_table, uint16 att_table_size, uint16 att_mtu, void *adv_rx);

/**
 * @brief 发送 GATT 通知 (Notify)
 * @param att_hdl [In] 特征值句柄 (必须是具有 Notify 属性的 Characteristic Value 句柄)
 * @param data    [In] 要发送的数据缓冲区
 * @param len     [In] 数据长度
 * @return 0 表示成功，负值表示失败 (如未订阅、链路断开等)
 * @note Notify 不需要客户端确认，速度快但不可靠。
 */
extern int32 uble_gatt_notify(uint16 att_hdl, uint8 *data, int32 len);

/**
 * @brief 发送 GATT 指示 (Indicate)
 * @param att_hdl [In] 特征值句柄 (必须是具有 Indicate 属性的 Characteristic Value 句柄)
 * @param data    [In] 要发送的数据缓冲区
 * @param len     [In] 数据长度
 * @return 0 表示成功，负值表示失败
 * @note Indicate 需要客户端发送 Confirm 确认，可靠但速度稍慢。
 */
extern int32 uble_gatt_indicate(uint16 att_hdl, uint8 *data, int32 len);

/**
 * @brief 通过 UUID 查询属性句柄 (Handle)
 * @param uuid [In] UUID 值
 *               - 若是 16-bit UUID: 传入数值 (如 0x2A19), `size` 必须为 16
 *               - 若是 128-bit UUID: 传入 uint8 数组指针, `size` 必须为 128
 * @param size [In] UUID 位宽 (16 或 128)
 * @return 找到的属性句柄 (uint16)，若未找到返回负值
 * @note 用于通uuid查找对应的句柄，然后执行 uble_gatt_notify 或 uble_gatt_indicate。
 */
extern int32 uble_uuid_2hdl(uint32 uuid, uint8 size);

/* 
 * -----------------------------------------------------------------------------
 * 完整 Bluetooth SIG Assigned Numbers UUID 列表 (Reference Only)
 * -----------------------------------------------------------------------------
 * 
 *    0x1800             Generic Access service
 *    0x1801             Generic Attribute service
 *    0x1802             Immediate Alert service
 *    0x1803             Link Loss service
 *    0x1804             Tx Power service
 *    0x1805             Current Time service
 *    0x1806             Reference Time Update service
 *    0x1807             Next DST Change service
 *    0x1808             Glucose service
 *    0x1809             Health Thermometer service
 *    0x180A             Device Information service
 *    0x180D             Heart Rate service
 *    0x180E             Phone Alert Status service
 *    0x180F             Battery service
 *    0x1810             Blood Pressure service
 *    0x1811             Alert Notification service
 *    0x1812             Human Interface Device service
 *    0x1813             Scan Parameters service
 *    0x1814             Running Speed and Cadence service
 *    0x1815             Automation IO service
 *    0x1816             Cycling Speed and Cadence service
 *    0x1818             Cycling Power service
 *    0x1819             Location and Navigation service
 *    0x181A             Environmental Sensing service
 *    0x181B             Body Composition service
 *    0x181C             User Data service
 *    0x181D             Weight Scale service
 *    0x181E             Bond Management service
 *    0x181F             Continuous Glucose Monitoring service
 *    0x1820             Internet Protocol Support service
 *    0x1821             Indoor Positioning service
 *    0x1822             Pulse Oximeter service
 *    0x1823             HTTP Proxy service
 *    0x1824             Transport Discovery service
 *    0x1825             Object Transfer service
 *    0x1826             Fitness Machine service
 *    0x1827             Mesh Provisioning service
 *    0x1828             Mesh Proxy service
 *    0x1829             Reconnection Configuration service
 *    0x183A             Insulin Delivery service
 *    0x183B             Binary Sensor service
 *    0x183C             Emergency Configuration service
 *    0x183D             Authorization Control service
 *    0x183E             Physical Activity Monitor service
 *    0x1843             Audio Input Control service
 *    0x1844             Volume Control service
 *    0x1845             Volume Offset Control service
 *    0x1846             Coordinated Set Identification service
 *    0x1847             Device Time service
 *    0x1848             Media Control service
 *    0x1849             Generic Media Control service
 *    0x184A             Constant Tone Extension service
 *    0x184B             Telephone Bearer service
 *    0x184C             Generic Telephone Bearer service
 *    0x184D             Microphone Control service
 *    0x184E             Audio Stream Control service
 *    0x184F             Broadcast Audio Scan service
 *    0x1850             Published Audio Capabilities service
 *    0x1851             Basic Audio Announcement service
 *    0x1852             Broadcast Audio Announcement service
 *    0x1853             Common Audio service
 *    0x1854             Hearing Aid service
 *    0x1855             TMAS service

 *    0x2800             Primary Service
 *    0x2801             Secondary Service
 *    0x2802             Include
 *    0x2803             Characteristic

 *    0x2900             Characteristic Extended Properties
 *    0x2901             Characteristic User Description
 *    0x2902             Client Characteristic Configuration
 *    0x2903             Server Characteristic Configuration
 *    0x2904             Characteristic Presentation Format
 *    0x2905             Characteristic Aggregate Format
 *    0x2906             Valid Range
 *    0x2907             External Report Reference
 *    0x2908             Report Reference
 *    0x2909             Number of Digitals
 *    0x290A             Value Trigger Setting
 *    0x290B             Environmental Sensing Configuration
 *    0x290C             Environmental Sensing Measurement
 *    0x290D             Environmental Sensing Trigger Setting
 *    0x290E             Time Trigger Setting
 *    0x290F             Complete BR-EDR Transport Block Data


 *    0x2A00             Device Name
 *    0x2A01             Appearance
 *    0x2A02             Peripheral Privacy Flag
 *    0x2A03             Reconnection Address
 *    0x2A04             Peripheral Preferred Connection Parameters
 *    0x2A05             Service Changed
 *    0x2A06             Alert Level
 *    0x2A07             Tx Power Level
 *    0x2A08             Date Time
 *    0x2A09             Day of Week
 *    0x2A0A             Day Date Time
 *    0x2A0C             Exact Time 256
 *    0x2A0D             DST Offset
 *    0x2A0E             Time Zone
 *    0x2A0F             Local Time Information
 *    0x2A11             Time with DST
 *    0x2A12             Time Accuracy
 *    0x2A13             Time Source
 *    0x2A14             Reference Time Information
 *    0x2A16             Time Update Control Point
 *    0x2A17             Time Update State
 *    0x2A18             Glucose Measurement
 *    0x2A19             Battery Level
 *    0x2A1C             Temperature Measurement
 *    0x2A1D             Temperature Type
 *    0x2A1E             Intermediate Temperature
 *    0x2A21             Measurement Interval
 *    0x2A22             Boot Keyboard Input Report
 *    0x2A23             System ID
 *    0x2A24             Model Number String
 *    0x2A25             Serial Number String
 *    0x2A26             Firmware Revision String
 *    0x2A27             Hardware Revision String
 *    0x2A28             Software Revision String
 *    0x2A29             Manufacturer Name String
 *    0x2A2A             IEEE 11073-20601 Regulatory Certification Data List
 *    0x2A2B             Current Time
 *    0x2A2C             Magnetic Declination
 *    0x2A31             Scan Refresh
 *    0x2A32             Boot Keyboard Output Report
 *    0x2A33             Boot Mouse Input Report
 *    0x2A34             Glucose Measurement Context
 *    0x2A35             Blood Pressure Measurement
 *    0x2A36             Intermediate Cuff Pressure
 *    0x2A37             Heart Rate Measurement
 *    0x2A38             Body Sensor Location
 *    0x2A39             Heart Rate Control Point
 *    0x2A3F             Alert Status
 *    0x2A40             Ringer Control Point
 *    0x2A41             Ringer Setting
 *    0x2A42             Alert Category ID Bit Mask
 *    0x2A43             Alert Category ID
 *    0x2A44             Alert Notification Control Point
 *    0x2A45             Unread Alert Status
 *    0x2A46             New Alert
 *    0x2A47             Supported New Alert Category
 *    0x2A48             Supported Unread Alert Category
 *    0x2A49             Blood Pressure Feature
 *    0x2A4A             HID Information
 *    0x2A4B             Report Map
 *    0x2A4C             HID Control Point
 *    0x2A4D             Report
 *    0x2A4E             Protocol Mode
 *    0x2A4F             Scan Interval Window
 *    0x2A50             PnP ID
 *    0x2A51             Glucose Feature
 *    0x2A52             Record Access Control Point
 *    0x2A53             RSC Measurement
 *    0x2A54             RSC Feature
 *    0x2A55             SC Control Point
 *    0x2A5A             Aggregate
 *    0x2A5B             CSC Measurement
 *    0x2A5C             CSC Feature
 *    0x2A5D             Sensor Location
 *    0x2A5E             PLX Spot-Check Measurement
 *    0x2A5F             PLX Continuous Measurement
 *    0x2A60             PLX Features
 *    0x2A63             Cycling Power Measurement
 *    0x2A64             Cycling Power Vector
 *    0x2A65             Cycling Power Feature
 *    0x2A66             Cycling Power Control Point
 *    0x2A67             Location and Speed
 *    0x2A68             Navigation
 *    0x2A69             Position Quality
 *    0x2A6A             LN Feature
 *    0x2A6B             LN Control Point
 *    0x2A6C             Elevation
 *    0x2A6D             Pressure
 *    0x2A6E             Temperature
 *    0x2A6F             Humidity
 *    0x2A70             True Wind Speed
 *    0x2A71             True Wind Direction
 *    0x2A72             Apparent Wind Speed
 *    0x2A73             Apparent Wind Direction
 *    0x2A74             Gust Factor
 *    0x2A75             Pollen Concentration
 *    0x2A76             UV Index
 *    0x2A77             Irradiance
 *    0x2A78             Rainfall
 *    0x2A79             Wind Chill
 *    0x2A7A             Heat Index
 *    0x2A7B             Dew Point
 *    0x2A7D             Descriptor Value Changed
 *    0x2A7E             Aerobic Heart Rate Lower Limit
 *    0x2A7F             Aerobic Threshold
 *    0x2A80             Age
 *    0x2A81             Anaerobic Heart Rate Lower Limit
 *    0x2A82             Anaerobic Heart Rate Upper Limit
 *    0x2A83             Anaerobic Threshold
 *    0x2A84             Aerobic Heart Rate Upper Limit
 *    0x2A85             Date of Birth
 *    0x2A86             Date of Threshold Assessment
 *    0x2A87             Email Address
 *    0x2A88             Fat Burn Heart Rate Lower Limit
 *    0x2A89             Fat Burn Heart Rate Upper Limit
 *    0x2A8A             First Name
 *    0x2A8B             Five Zone Heart Rate Limits
 *    0x2A8C             Gender
 *    0x2A8D             Heart Rate Max
 *    0x2A8E             Height
 *    0x2A8F             Hip Circumference
 *    0x2A90             Last Name
 *    0x2A91             Maximum Recommended Heart Rate
 *    0x2A92             Resting Heart Rate
 *    0x2A93             Sport Type for Aerobic and Anaerobic Thresholds
 *    0x2A94             Three Zone Heart Rate Limits
 *    0x2A95             Two Zone Heart Rate Limits
 *    0x2A96             VO2 Max
 *    0x2A97             Waist Circumference
 *    0x2A98             Weight
 *    0x2A99             Database Change Increment
 *    0x2A9A             User Index
 *    0x2A9B             Body Composition Feature
 *    0x2A9C             Body Composition Measurement
 *    0x2A9D             Weight Measurement
 *    0x2A9E             Weight Scale Feature
 *    0x2A9F             User Control Point
 *    0x2AA0             Magnetic Flux Density - 2D
 *    0x2AA1             Magnetic Flux Density - 3D
 *    0x2AA2             Language
 *    0x2AA3             Barometric Pressure Trend
 *    0x2AA4             Bond Management Control Point
 *    0x2AA5             Bond Management Feature
 *    0x2AA6             Central Address Resolution
 *    0x2AA7             CGM Measurement
 *    0x2AA8             CGM Feature
 *    0x2AA9             CGM Status
 *    0x2AAA             CGM Session Start Time
 *    0x2AAB             CGM Session Run Time
 *    0x2AAC             CGM Specific Ops Control Point
 *    0x2AAD             Indoor Positioning Configuration
 *    0x2AAE             Latitude
 *    0x2AAF             Longitude
 *    0x2AB0             Local North Coordinate
 *    0x2AB1             Local East Coordinate
 *    0x2AB2             Floor Number
 *    0x2AB3             Altitude
 *    0x2AB4             Uncertainty
 *    0x2AB5             Location Name
 *    0x2AB6             URI
 *    0x2AB7             HTTP Headers
 *    0x2AB8             HTTP Status Code
 *    0x2AB9             HTTP Entity Body
 *    0x2ABA             HTTP Control Point
 *    0x2ABB             HTTPS Security
 *    0x2ABC             TDS Control Point
 *    0x2ABD             OTS Feature
 *    0x2ABE             Object Name
 *    0x2ABF             Object Type
 *    0x2AC0             Object Size
 *    0x2AC1             Object First-Created
 *    0x2AC2             Object Last-Modified
 *    0x2AC3             Object ID
 *    0x2AC4             Object Properties
 *    0x2AC5             Object Action Control Point
 *    0x2AC6             Object List Control Point
 *    0x2AC7             Object List Filter
 *    0x2AC8             Object Changed
 *    0x2AC9             Resolvable Private Address Only
 *    0x2ACC             Fitness Machine Feature
 *    0x2ACD             Treadmill Data
 *    0x2ACE             Cross Trainer Data
 *    0x2ACF             Step Climber Data
 *    0x2AD0             Stair Climber Data
 *    0x2AD1             Rower Data
 *    0x2AD2             Indoor Bike Data
 *    0x2AD3             Training Status
 *    0x2AD4             Supported Speed Range
 *    0x2AD5             Supported Inclination Range
 *    0x2AD6             Supported Resistance Level Range
 *    0x2AD7             Supported Heart Rate Range
 *    0x2AD8             Supported Power Range
 *    0x2AD9             Fitness Machine Control Point
 *    0x2ADA             Fitness Machine Status
 *    0x2ADB             Mesh Provisioning Data In
 *    0x2ADC             Mesh Provisioning Data Out
 *    0x2ADD             Mesh Proxy Data In
 *    0x2ADE             Mesh Proxy Data Out
 *    0x2AE0             Average Current
 *    0x2AE1             Average Voltage
 *    0x2AE2             Boolean
 *    0x2AE3             Chromatic Distance from Planckian
 *    0x2AE4             Chromaticity Coordinates
 *    0x2AE5             Chromaticity in CCT and Duv Values
 *    0x2AE6             Chromaticity Tolerance
 *    0x2AE7             CIE 13.3-1995 Color Rendering Index
 *    0x2AE8             Coefficient
 *    0x2AE9             Correlated Color Temperature
 *    0x2AEA             Count 16
 *    0x2AEB             Count 24
 *    0x2AEC             Country Code
 *    0x2AED             Date UTC
 *    0x2AEE             Electric Current
 *    0x2AEF             Electric Current Range
 *    0x2AF0             Electric Current Specification
 *    0x2AF1             Electric Current Statistics
 *    0x2AF2             Energy
 *    0x2AF3             Energy in a Period of Day
 *    0x2AF4             Event Statistics
 *    0x2AF5             Fixed String 16
 *    0x2AF6             Fixed String 24
 *    0x2AF7             Fixed String 36
 *    0x2AF8             Fixed String 8
 *    0x2AF9             Generic Level
 *    0x2AFA             Global Trade Item Number
 *    0x2AFB             Illuminance
 *    0x2AFC             Luminous Efficacy
 *    0x2AFD             Luminous Energy
 *    0x2AFE             Luminous Exposure
 *    0x2AFF             Luminous Flux
 *    0x2B00             Luminous Flux Range
 *    0x2B01             Luminous Intensity
 *    0x2B02             Mass Flow
 *    0x2B03             Perceived Lightness
 *    0x2B04             Percentage 8
 *    0x2B05             Power
 *    0x2B06             Power Specification
 *    0x2B07             Relative Runtime in a Current Range
 *    0x2B08             Relative Runtime in a Generic Level Range
 *    0x2B09             Relative Value in a Voltage Range
 *    0x2B0A             Relative Value in an Illuminance Range
 *    0x2B0B             Relative Value in a Period of Day
 *    0x2B0C             Relative Value in a Temperature Range
 *    0x2B0D             Temperature 8
 *    0x2B0E             Temperature 8 in a Period of Day
 *    0x2B0F             Temperature 8 Statistics
 *    0x2B10             Temperature Range
 *    0x2B11             Temperature Statistics
 *    0x2B12             Time Decihour 8
 *    0x2B13             Time Exponential 8
 *    0x2B14             Time Hour 24
 *    0x2B15             Time Millisecond 24
 *    0x2B16             Time Second 16
 *    0x2B17             Time Second 8
 *    0x2B18             Voltage
 *    0x2B19             Voltage Specification
 *    0x2B1A             Voltage Statistics
 *    0x2B1B             Volume Flow
 *    0x2B1C             Chromaticity Coordinate
 *    0x2B1D             RC Feature
 *    0x2B1E             RC Settings
 *    0x2B1F             Reconnection Configuration Control Point
 *    0x2B20             IDD Status Changed
 *    0x2B21             IDD Status
 *    0x2B22             IDD Annunciation Status
 *    0x2B23             IDD Features
 *    0x2B24             IDD Status Reader Control Point
 *    0x2B25             IDD Command Control Point
 *    0x2B26             IDD Command Data
 *    0x2B27             IDD Record Access Control Point
 *    0x2B28             IDD History Data
 *    0x2B29             Client Supported Features
 *    0x2B2A             Database Hash
 *    0x2B2B             BSS Control Point
 *    0x2B2C             BSS Response
 *    0x2B2D             Emergency ID
 *    0x2B2E             Emergency Text
 *    0x2B2F             ACS Status
 *    0x2B30             ACS Data In
 *    0x2B31             ACS Data Out Notify
 *    0x2B32             ACS Data Out Indicate
 *    0x2B33             ACS Control Point
 *    0x2B34             Enhanced Blood Pressure Measurement
 *    0x2B35             Enhanced Intermediate Cuff Pressure
 *    0x2B36             Blood Pressure Record
 *    0x2B37             Registered User
 *    0x2B38             BR-EDR Handover Data
 *    0x2B39             Bluetooth SIG Data
 *    0x2B3A             Server Supported Features
 *    0x2B3B             Physical Activity Monitor Features
 *    0x2B3C             General Activity Instantaneous Data
 *    0x2B3D             General Activity Summary Data
 *    0x2B3E             CardioRespiratory Activity Instantaneous Data
 *    0x2B3F             CardioRespiratory Activity Summary Data
 *    0x2B40             Step Counter Activity Summary Data
 *    0x2B41             Sleep Activity Instantaneous Data
 *    0x2B42             Sleep Activity Summary Data
 *    0x2B43             Physical Activity Monitor Control Point
 *    0x2B44             Activity Current Session
 *    0x2B45             Physical Activity Session Descriptor
 *    0x2B46             Preferred Units
 *    0x2B47             High Resolution Height
 *    0x2B48             Middle Name
 *    0x2B49             Stride Length
 *    0x2B4A             Handedness
 *    0x2B4B             Device Wearing Position
 *    0x2B4C             Four Zone Heart Rate Limits
 *    0x2B4D             High Intensity Exercise Threshold
 *    0x2B4E             Activity Goal
 *    0x2B4F             Sedentary Interval Notification
 *    0x2B50             Caloric Intake
 *    0x2B51             TMAP Role
 *    0x2B77             Audio Input State
 *    0x2B78             Gain Settings Attribute
 *    0x2B79             Audio Input Type
 *    0x2B7A             Audio Input Status
 *    0x2B7B             Audio Input Control Point
 *    0x2B7C             Audio Input Description
 *    0x2B7D             Volume State
 *    0x2B7E             Volume Control Point
 *    0x2B7F             Volume Flags
 *    0x2B80             Volume Offset State
 *    0x2B81             Audio Location
 *    0x2B82             Volume Offset Control Point
 *    0x2B83             Audio Output Description
 *    0x2B84             Set Identity Resolving Key
 *    0x2B85             Coordinated Set Size
 *    0x2B86             Set Member Lock
 *    0x2B87             Set Member Rank
 *    0x2B89             Apparent Energy 32
 *    0x2B8A             Apparent Power
 *    0x2B8C             CO2 Concentration
 *    0x2B8D             Cosine of the Angle
 *    0x2B8E             Device Time Feature
 *    0x2B8F             Device Time Parameters
 *    0x2B90             Device Time
 *    0x2B91             Device Time Control Point
 *    0x2B92             Time Change Log Data
 *    0x2B93             Media Player Name
 *    0x2B94             Media Player Icon Object ID
 *    0x2B95             Media Player Icon URL
 *    0x2B96             Track Changed
 *    0x2B97             Track Title
 *    0x2B98             Track Duration
 *    0x2B99             Track Position
 *    0x2B9A             Playback Speed
 *    0x2B9B             Seeking Speed
 *    0x2B9C             Current Track Segments Object ID
 *    0x2B9D             Current Track Object ID
 *    0x2B9E             Next Track Object ID
 *    0x2B9F             Parent Group Object ID
 *    0x2BA0             Current Group Object ID
 *    0x2BA1             Playing Order
 *    0x2BA2             Playing Orders Supported
 *    0x2BA3             Media State
 *    0x2BA4             Media Control Point
 *    0x2BA5             Media Control Point Opcodes Supported
 *    0x2BA6             Search Results Object ID
 *    0x2BA7             Search Control Point
 *    0x2BA8             Energy 32
 *    0x2BA9             Media Player Icon Object Type
 *    0x2BAA             Track Segments Object Type
 *    0x2BAB             Track Object Type
 *    0x2BAC             Group Object Type
 *    0x2BAD             Constant Tone Extension Enable
 *    0x2BAE             Advertising Constant Tone Extension Minimum Length
 *    0x2BAF             Advertising Constant Tone Extension Minimum Transmit Count
 *    0x2BB0             Advertising Constant Tone Extension Transmit Duration
 *    0x2BB1             Advertising Constant Tone Extension Interval
 *    0x2BB2             Advertising Constant Tone Extension PHY
 *    0x2BB3             Bearer Provider Name
 *    0x2BB4             Bearer UCI
 *    0x2BB5             Bearer Technology
 *    0x2BB6             Bearer URI Schemes Supported List
 *    0x2BB7             Bearer Signal Strength
 *    0x2BB8             Bearer Signal Strength Reporting Interval
 *    0x2BB9             Bearer List Current Calls
 *    0x2BBA             Content Control ID
 *    0x2BBB             Status Flags
 *    0x2BBC             Incoming Call Target Bearer URI
 *    0x2BBD             Call State
 *    0x2BBE             Call Control Point
 *    0x2BBF             Call Control Point Optional Opcodes
 *    0x2BC0             Termination Reason
 *    0x2BC1             Incoming Call
 *    0x2BC2             Call Friendly Name
 *    0x2BC3             Mute
 *    0x2BC4             Sink ASE
 *    0x2BC5             Source ASE
 *    0x2BC6             ASE Control Point
 *    0x2BC7             Broadcast Audio Scan Control Point
 *    0x2BC8             Broadcast Receive State
 *    0x2BC9             Sink PAC
 *    0x2BCA             Sink Audio Locations
 *    0x2BCB             Source PAC
 *    0x2BCC             Source Audio Locations
 *    0x2BCD             Available Audio Contexts
 *    0x2BCE             Supported Audio Contexts
 *    0x2BCF             Ammonia Concentration
 *    0x2BD0             Carbon Monoxide Concentration
 *    0x2BD1             Methane Concentration
 *    0x2BD2             Nitrogen Dioxide Concentration
 *    0x2BD3             Non-Methane Volatile Organic Compounds Concentration
 *    0x2BD4             Ozone Concentration
 *    0x2BD5             Particulate Matter - PM1 Concentration
 *    0x2BD6             Particulate Matter - PM2.5 Concentration
 *    0x2BD7             Particulate Matter - PM10 Concentration
 *    0x2BD8             Sulfur Dioxide Concentration
 *    0x2BD9             Sulfur Hexafluoride Concentration
 *    0x2BDA             Hearing Aid Features
 *    0x2BDB             Hearing Aid Preset Control Point
 *    0x2BDC             Active Preset Index
 *    0x2BDE             Fixed String 64
 *    0x2BDF             High Temperature
 *    0x2BE0             High Voltage
 *    0x2BE1             Light Distribution
 *    0x2BE2             Light Output
 *    0x2BE3             Light Source Type
 *    0x2BE4             Noise
 *    0x2BE5             Relative Runtime in a Correlated Color Temperature Range
 *    0x2BE6             Time Second 32
 *    0x2BE7             VOC Concentration
 *    0x2BE8             Voltage Frequency
 */

#ifdef __cplusplus
}
#endif

#endif /* _HGIC_UBLE_H_ */
