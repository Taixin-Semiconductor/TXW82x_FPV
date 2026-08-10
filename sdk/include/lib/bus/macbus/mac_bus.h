#ifndef _MAC_BUS_H_
#define _MAC_BUS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file mac_bus.h
 * @brief WiFi 协议栈与主控芯片之间的统一总线接口抽象层
 * 
 * @details
 * 本文件定义了 WiFi 协议栈对接主控 (Host) 时的**标准访问规范**。
 * 
 * 核心作用：
 * 1. **屏蔽差异**：通过 `struct mac_bus` 统一接口，屏蔽底层物理传输介质 (UART, SPI, SDIO, USB, GMAC 等) 的实现差异。
 * 2. **职责分离**：
 *    - **mac_bus 组件 (底层驱动)**: 负责实现具体的硬件读写、初始化结构体中的函数指针 (write/ioctl)、维护统计信息。
 *    - **WiFi 协议栈 (上层)**: 负责提供数据接收回调 (recv)、设置私有上下文 (priv)、调用发送接口。
 * 
 * 已支持的接口：
 * - UART / GD_UART (串口)
 * - SPI / SDSPI (串行外设接口)
 * - SDIO / MMC (安全数字输入输出)
 * - USB (通用串行总线)
 * - GMAC/EMAC (以太网媒体访问控制器)
 */

/**
 * @brief 总线类型枚举
 * @details 定义了系统支持的所有物理传输接口类型 ID。
 */
enum mac_bus_type {
    MAC_BUS_TYPE_NONE,      ///< 0: 无效类型
    MAC_BUS_TYPE_SDIO,      ///< 1: SDIO 接口 (高速，支持中断，常用於 WiFi 模组)
    MAC_BUS_TYPE_USB,       ///< 2: USB 接口 (通用性强，支持热插拔)
    MAC_BUS_TYPE_EMAC,      ///< 3: GMAC/EMAC 接口 (以太网 MAC，通常用于板载 PHY 或 PCIe WiFi)
    MAC_BUS_TYPE_UART,      ///< 4: UART 接口 (串口，低速，常用于 IoT 或调试)
    MAC_BUS_TYPE_NDEV,      ///< 5: 网络设备接口 (对接本地 lwIP 虚拟设备，[已废弃])
    MAC_BUS_TYPE_QC,        ///< 6: SDK QC模式专用接口
    MAC_BUS_TYPE_COMBI,     ///< 7: 组合模式接口 (如 SDIO+UART 共存，[已废弃])
    MAC_BUS_TYPE_SPI,       ///< 8: SPI 接口 (高速同步串行，常用于无 OS 或 RTOS 环境)
    MAC_BUS_TYPE_MMC,       ///< 9: MMC 接口 (作为 SD Host 控制器使用时，兼容 SDIO 协议)
    MAC_BUS_TYPE_GDUART,    ///< 10: GD芯片专用 UART 接口扩展
};

/**
 * @brief 总线控制命令枚举 (IO Control Commands)
 * @details 用于通过 ioctl 接口向底层驱动发送特定控制指令。
 */
enum MAC_BUS_IOCTL {
    MACBUS_IOCTL_IS_BUSY = 1,      ///< 查询总线是否忙碌 (返回非 0 表示忙，建议暂停发送)
    MACBUS_IOCTL_REINIT_BUS,       ///< 通知底层驱动执行总线重新初始化 (主要用于 SDIO 异常恢复或复位后重连)
    MACBUS_IOCTL_BOOTDL_FLAG,      ///< [SD Host 专用] 通知驱动：外挂 WiFi 模组正处于 Bootloader 下载阶段 (需调整时序/电压/模式)
    MACBUS_IOCTL_BUFFER_IDLE,      ///< 通知 mac_bus 组件：上层协议栈已处理完数据，有 Buffer 空闲，可恢复接收调度
};

/**
 * @brief 总线初始化参数结构
 * @details 传递特定总线所需的配置参数，用于能力协商或硬件配置。
 */
struct macbus_param {
    uint16 drv_aggsize;  ///< [能力协商] 通知协议栈，主控/驱动是否支持聚合收发 (Aggregation)。
                         ///< 若 >0，表示支持，值为最大聚合包大小；若为 0，表示不支持。
    uint16 sdio_clk;     ///< [硬件配置] SDIO 时钟频率 (kHz)。仅当 type=SDIO/MMC 时有效，用于配置主机控制器时钟。
};

/**
 * @brief MAC 总线抽象层核心结构体
 * @details 
 * 这是一个多态对象，代表一个具体的物理通信链路。
 * **初始化职责说明**：
 * - **mac_bus 组件 (驱动层)**: 负责分配此结构体，设置 `type`, `write`, `write_scatter`, `ioctl` 函数指针，并清零统计计数器。
 * - **WiFi 协议栈 (上层)**: 在 attach 时设置 `priv` 和 `recv` 回调。
 * - **运行时**: `rxerr/txerr` 由 mac_bus 组件在发生错误时自动更新。
 */
struct mac_bus {
    int type;                ///< [属性] 总线类型 (对应 enum mac_bus_type)。[由 mac_bus 组件设置]
    
    void *priv;              ///< [私有] 用户私有数据指针。[由 WiFi 协议栈在 attach 时设置]，通常用于存储协议栈上下文。
    
    /// [统计] 错误计数器。[由 mac_bus 组件负责更新]
    uint32 rxerr;            ///< 接收错误计数 (CRC 错、超时、丢包等)
    uint32 txerr;            ///< 发送错误计数 (繁忙、超时、FIFO 满等)

    /**
     * @brief 同步写入数据 (TX Path)
     * @param bus 总线对象指针
     * @param data 数据缓冲区指针 (连续内存)
     * @param len 数据长度
     * @return 成功返回实际发送字节数，失败返回负值
     * @note 数据流向：WiFi 协议栈 --> 主控硬件。[函数指针由 mac_bus 组件设置]
     */
    int (*write)(struct mac_bus *bus, unsigned char *data, int len);

    /**
     * @brief 散射写入数据 (TX Path - Scatter/Gather)
     * @param bus 总线对象指针
     * @param data 散射数组指针 (多个不连续内存块)
     * @param count 数组元素个数
     * @return 成功返回 0，失败返回负值
     * @note 数据流向：WiFi 协议栈 --> 主控硬件。[函数指针由 mac_bus 组件设置]
     */
    int (*write_scatter)(struct mac_bus *bus, scatter_data *data, int count);

    /**
     * @brief 接收数据回调入口 (RX Path Entry)
     * @param bus 总线对象指针
     * @param data 数据缓冲区指针
     * @param len 数据长度
     * @return 0 表示成功处理，非 0 表示失败
     * @note 数据流向：主控硬件 --> WiFi 协议栈。
     *       当底层驱动收到数据时，调用此函数指针将数据送给协议栈。
     *       [函数指针由 WiFi 协议栈在 attach 时设置]
     */
    int (*recv)(struct mac_bus *bus, unsigned char *data, int len);

    /**
     * @brief 总线控制命令 (IO Control)
     * @param bus 总线对象指针
     * @param cmd 命令 ID (见 enum MAC_BUS_IOCTL)
     * @param param1 命令参数 1
     * @param param2 命令参数 2
     * @return 取决于具体命令，通常 0 表示成功
     * @note [函数指针由 mac_bus 组件设置]
     */
    int (*ioctl)(struct mac_bus *bus, uint32 cmd, uint32 param1, uint32 param2);
};

/**
 * @brief 接收回调函数类型定义 (RX Callback Type)
 * @details 
 * WiFi 协议栈必须实现此类型的函数，并在 `mac_bus_attach` 时传入。
 * 当底层驱动收到数据时，会通过 `struct mac_bus::recv` 调用此函数。
 * @param bus 触发接收的总线对象
 * @param data 数据缓冲区指针
 * @param len 数据长度
 * @return 0 表示成功处理
 */
typedef int (*mac_bus_recv)(struct mac_bus *bus, unsigned char *data, int len);

/**
 * @brief 通用总线挂载接口
 * @param bus_type 总线类型枚举值 (MAC_BUS_TYPE_XXX)
 * @param recv 接收回调函数 ([由 WiFi 协议栈提供])
 * @param priv 私有数据指针 ([由 WiFi 协议栈提供])
 * @param param 总线特定参数 (可选，用于配置时钟或聚合大小)
 * @return 成功返回 mac_bus 对象指针 (内部已初始化 write/ioctl 等指针)，失败返回 NULL
 */
extern struct mac_bus *mac_bus_attach(int bus_type, mac_bus_recv recv, void *priv, struct macbus_param *param);

/////////////////////////////////////////////////////////////////////////////////////////////////////////
extern struct mac_bus *mac_bus_sdio_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_usb_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_uart_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_gmac_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_qa_gmac_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_qc_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_ndev_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_combi_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_sdspi_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_spi_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_mmc_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_gduart_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern void mac_bus_sdio_detach(struct mac_bus *bus);
extern void mac_bus_usb_detach(struct mac_bus *bus);
extern void mac_bus_gmac_detach(struct mac_bus *bus);
extern void mac_bus_spi_detach(struct mac_bus *bus);
/////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
 * @brief 检查总线是否忙碌
 * @return 非 0 表示忙，0 表示空闲
 */
#define MACBUS_IS_BUSY(bus)         (bus)->ioctl(bus, MACBUS_IOCTL_IS_BUSY, 0, 0)

/**
 * @brief 设置/清除 Bootloader 下载标志
 * @param bus 总线对象指针
 * @param bootdl 1: 进入 Bootloader 模式 (下载固件), 0: 正常运行模式
 */
#define MACBUS_BOOTDL(bus, bootdl)  (bus)->ioctl(bus, MACBUS_IOCTL_BOOTDL_FLAG, bootdl, 0)

/**
 * @brief 通知总线层 Buffer 已空闲
 * @param bus 总线对象指针
 * @details 用于流控。当上层协议栈处理完接收数据后调用，告知底层驱动可以恢复中断或继续调度接收。
 */
#define MACBUS_BUFFER_IDLE(bus)     (bus)->ioctl(bus, MACBUS_IOCTL_BUFFER_IDLE, 0, 0)

/**
 * @brief 重新初始化总线
 * @param bus 总线对象指针
 * @details 用于错误恢复场景。重置硬件状态机，重新建立连接。
 */
#define MACBUS_REINIT(bus)          (bus)->ioctl(bus, MACBUS_IOCTL_REINIT_BUS, 0, 0)

#ifdef __cplusplus
}
#endif

#endif /* _MAC_BUS_H_ */
