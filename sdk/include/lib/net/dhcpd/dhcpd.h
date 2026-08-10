/**
 * @file dhcpd.h
 * @brief DHCP 服务器模块头文件
 * 
 * 本模块实现了一个轻量级的 DHCP 服务器 (Server)，用于为局域网内的客户端自动分配 IP 地址、
 * 子网掩码、网关及 DNS 服务器信息。
 * 本模块需要依赖 skmonitor 模块而运行。
 * 
 * @par 主要功能:
 * - 管理 IP 地址池 (IP Pool) 的分配与回收。
 * - 支持基于 MAC 地址的租约管理 (Lease Management)。
 * - 提供动态修改 DNS 配置的能力。
 * - 支持查看当前已分配的租约列表。
 */

#ifndef _HUGEIC_DHCPD_H_
#define _HUGEIC_DHCPD_H_

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief DHCP 服务器配置参数结构体
 * 
 * 用于在启动 DHCP 服务器时传递初始化配置。
 * @warning 所有 IP 地址字段 (start_ip, end_ip, netmask, router, dns) 必须使用 **网络字节序 (Network Byte Order)**。
 *          请使用 htonl() 宏进行转换后再赋值。
 */
struct dhcpd_param {
    uint32 lease_time;   /**< 租约过期时间，单位：秒 (seconds)。例如 86400 表示 24 小时 */
    
    uint32 start_ip;     /**< 地址池起始 IP (网络字节序)。例如：192.168.1.100 */
    uint32 end_ip;       /**< 地址池结束 IP (网络字节序)。例如：192.168.1.200 */
    
    uint32 netmask;      /**< 子网掩码 (网络字节序)。例如：255.255.255.0 */
    uint32 router;       /**< 默认网关 IP (网络字节序)。通常指向路由器或本机 IP */
    
    uint32 dns1;         /**< 首选 DNS 服务器 IP (网络字节序) */
    uint32 dns2;         /**< 备选 DNS 服务器 IP (网络字节序)，若不使用可设为 0 */
};

/**
 * @brief DHCP 租约信息结构体
 * 
 * 用于存储和查询当前已分配给客户端的 IP 与 MAC 地址映射关系。
 */
struct dhcpd_leaseinfo {
    uint32 ip;           /**< 已分配的 IP 地址 (网络字节序) */
    uint8 mac[6];        /**< 客户端的 MAC 地址 (物理地址) */
};


/**
 * @brief 启动 DHCP 服务器
 * 
 * 在指定的网络接口上启动 DHCP 服务，并开始响应客户端的 Discover/Request 请求。
 * 
 * @param ifname 网络接口名称字符串 (如 "e0", "w0")
 * @param param 指向配置参数结构体的指针 (@ref dhcpd_param)
 *        如果 param=NULL，则使用上一次执行dhcpd_start时设置的param参数
 * 
 * @return int32 
 *         - 0: 启动成功
 *         - 负值: 启动失败 (如接口不存在、参数错误、端口占用)
 */
int32 dhcpd_start(char *ifname, struct dhcpd_param *param);

/**
 * @brief 停止 DHCP 服务器
 * 
 * 关闭 DHCP 服务，关闭监听端口，清理租约信息，并释放相关资源。
 */
void dhcpd_stop(void);

/**
 * @brief 刷新 DHCP 服务器状态
 * 
 * 强制清理所有的租约。
 */
void dhcpd_flush(void);

/**
 * @brief 获取当前租约列表
 * 
 * 遍历内部租约表，将所有活跃的 IP-MAC 映射复制到用户提供的缓冲区中。
 * 
 * @param leaseslist 输出缓冲区指针，指向 struct dhcpd_leaseinfo 数组
 * @param list_size 缓冲区的最大容量 (元素个数)
 * 
 * @return int32 实际复制的租约条目数量。
 */
int32 dhcpd_get_lease_list(struct dhcpd_leaseinfo *leaseslist, int32 list_size);

/**
 * @brief 根据 MAC 地址查询已分配的 IP
 * 
 * @param mac 指向 6 字节 MAC 地址数组的指针
 * 
 * @return uint32 
 *         - 成功：返回对应的 IP 地址 (网络字节序)
 *         - 失败：返回 0 
 */
uint32 dhcpd_get_lease_ip(uint8 *mac);

/**
 * @brief 标记指定 MAC 的 IP 为非活跃/过期
 * 
 * 强制使指定 MAC 地址对应的租约失效，回收其 IP 地址回地址池。
 * 常用于客户端异常断开或管理员强制踢除设备。
 * 
 * @param mac 指向 6 字节 MAC 地址数组的指针
 */
void dhcpd_ip_inactive(uint8 *mac);

/**
 * @brief 动态更新 DNS 服务器配置
 * 
 * 在不重启 DHCP 服务的情况下，修改下发给客户端的 DNS 服务器地址。
 * 新配置仅对后续的新请求或租约续约生效。
 * 
 * @param dns1 新的首选 DNS IP (网络字节序)
 * @param dns2 新的备选 DNS IP (网络字节序)
 */
void dhcpd_set_dns(uint32 dns1, uint32 dns2);

/**
 * @brief 打印 IP 地址池状态
 * 
 * 用于调试，将当前 IP 池的分配列表输出到控制台/日志。
 */
void dhcpd_dump_ippool(void);

#ifdef __cplusplus
}
#endif

#endif /* _HUGEIC_DHCPD_H_ */
