#ifndef _HGIC_DSLEEP_DATA_H_
#define _HGIC_DSLEEP_DATA_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file dsleepdata.h
 * @brief 深度睡眠 (Deep Sleep) 数据保持区管理接口
 * 
 * @details
 * 本模块用于管理系统进入深度睡眠模式时，需要保留在 SRAM (Retention Memory) 中的关键数据。
 * 
 * 工作机制：
 * 1. **分区管理**：将保留内存划分为不同的逻辑区域 (ID)，对应不同的子系统 (如 WiFi LMAC, UMAC, 用户应用等)。
 * 2. **静态分配**：在系统初始化或睡眠准备阶段，各模块通过 `sys_sleepdata_request` 预先申请固定大小的内存块。
 *    - 注意：深睡期间堆 heap 通常不可用，因此必须在睡前完成分配。
 * 3. **数据保持**：进入深睡后，CPU 大部分断电，但指定的 SRAM 区域保持供电，数据不丢失。
 * 4. **唤醒恢复**：系统唤醒后，各模块通过 `sys_sleepdata_get` 获取之前保存的数据指针，恢复上下文状态。
 * 
 * 典型应用场景：
 * - WiFi 协议栈保存连接状态、计数器、时序信息。
 * - 用户应用保存 GPIO 状态、传感器校准数据或运行标志。
 * - 调试工具记录睡眠时长、唤醒源等信息 (Sleep Log)。
 */

/**
 * @brief 睡眠数据区域 ID 枚举
 * @details 定义了系统中各个需要保留数据的子模块标识。
 */
enum system_sleepdata_id {
    SYSTEM_SLEEPDATA_ID_LMAC,      ///< 0: LMAC 数据区 (WiFi 驱动底层上下文)
    SYSTEM_SLEEPDATA_ID_UMAC,      ///< 1: UMAC 数据区 (WiFi 协议栈上下文)
    SYSTEM_SLEEPDATA_ID_PSALIVE,   ///< 2: 应用保活状态数据
    SYSTEM_SLEEPDATA_ID_PSCONNECT, ///< 3: 节能连接状态数据
    SYSTEM_SLEEPDATA_ID_WKDATA,    ///< 4: 应用唤醒数据
    SYSTEM_SLEEPDATA_ID_USER,      ///< 5: 用户自定义数据区
    SYSTEM_SLEEPDATA_ID_SLEEPLOG,  ///< 6: 睡眠日志区 (用于记录睡眠/唤醒的时间戳和事件)
    SYSTEM_SLEEPDATA_ID_MAX,       ///< 7: ID 最大值 (用于边界检查或数组大小定义)
};

/**
 * @brief 初始化睡眠数据管理模块
 * @details 
 * 在系统启动早期调用，初始化内部内存池结构，清零所有区域状态。
 * 必须在调用 request 或 get 接口之前执行。
 */
extern void sys_sleepdata_init(void);

/**
 * @brief 申请睡眠保留内存块
 * @param id   [In] 数据区域 ID (见 enum system_sleepdata_id)
 * @param size [In] 需要申请的字节数
 * @return 成功返回指向保留内存区域的指针，失败返回 NULL
 * @details 
 * - 此函数通常在系统初始化阶段或进入睡眠前的准备阶段调用。
 * - 分配的内存位于长电SRAM区域，深睡期间数据不会丢失。
 * - 每个 ID 只能申请一次，重复申请 返回相同的地址。
 * - 如果剩余空间不足，返回 NULL。
 */
extern void *sys_sleepdata_request(uint8 id, uint32 size);

/**
 * @brief 重置睡眠数据管理模块
 * @details 
 * 清空所有已分配的区域信息，释放内存池标记。
 * 通常在系统冷启动或发生严重错误需要重新初始化睡眠模块时调用。
 */
extern void sys_sleepdata_reset(void);

/**
 * @brief 查询当前剩余可用的保留内存大小
 * @return 剩余可用字节数
 * @details 用于评估是否还有足够空间申请新的睡眠数据块。
 */
extern uint32 sys_sleepdata_freesize(void);

/**
 * @brief 获取指定 ID 的保留数据指针
 * @param id [In] 数据区域 ID
 * @return 指向该区域数据的指针；若该 ID 未分配或无效，返回 NULL
 * @details 
 * - 此函数通常在系统**唤醒后**调用，用于读取之前保存的上下文。
 * - 返回的指针指向的数据在深睡期间保持不变。
 */
extern void *sys_sleepdata_get(uint8 id);

/**
 * @brief 初始化睡眠日志缓冲区
 * @param size [In] 日志缓冲区大小 (字节)
 * @return 成功返回 0，失败返回负值
 * @details 
 * 为 `SYSTEM_SLEEPDATA_ID_SLEEPLOG` 区域分配空间并初始化日志结构。
 * 需在记录任何日志前调用。
 */
extern int32 dsleeplog_int(uint32 size);

/**
 * @brief 打印睡眠日志内容
 * @details 
 * 将记录的睡眠日志通过串口或其他调试通道输出。
 * 通常在系统唤醒并完成初始化后调用，用于分析睡眠时长、唤醒原因及流程耗时。
 */
extern void dsleeplog_print(void);

/**
 * @brief 向dsleep log内存区域存入一个字节信息
 * @param c [In] 要记录的字符
 * @details 
 * 向环形缓冲区写入一个字节数据。
 * 用于在睡眠流程的关键节点存储打印信息，便于唤醒后可以查看日志信息。
 */
extern void dsleeplog_save(char c);

#ifdef __cplusplus
}
#endif

#endif /* _HGIC_DSLEEP_DATA_H_ */
