#ifndef __OS_IRQ_H_
#define __OS_IRQ_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 中断处理函数类型定义
 * 
 * 当特定中断发生时，系统将调用此函数。
 * @param data 用户注册中断时传入的私有数据指针，用于区分设备实例或传递上下文。
 * @note 
 * - 该函数运行在中断上下文 (Interrupt Context) 中。
 * - **严禁**在回调中执行可能导致阻塞的操作 (如睡眠、等待信号量、延时等)。
 * - 应尽量快速执行，耗时操作建议移交到底半部 (Bottom Half) 或任务中处理。
 */
typedef void (*irq_handle)(void *data);

/**
 * @brief 系统硬件中断控制块结构
 * 
 * 用于描述和管理一个特定的硬件中断源。
 */
struct sys_hwirq {
    void *data;           /**< [用户] 传递给中断处理函数的私有数据参数 */
    irq_handle handle;    /**< [用户] 中断服务程序 (ISR) 入口函数指针 */

#ifdef SYS_IRQ_STAT
    /**
     * @brief 中断统计信息 (仅在定义 SYS_IRQ_STAT 时编译入内)
     * 用于性能分析和中断负载监控。
     */
    uint32 trig_cnt;      /**< [统计] 中断触发总次数 */
    uint16 max;           /**< [统计] 单次中断处理的最大耗时 (单位：CPU 周期或特定计时单位) */
    uint16 min;           /**< [统计] 单次中断处理的最小耗时 */
    uint32 tot_cycle;     /**< [统计] 所有中断处理耗时的总和 (用于计算平均值) */
#endif
};

/**
 * @brief 使能指定中断
 * 
 * 打开硬件中断控制器中指定 IRQ 号的开关，允许该中断触发。
 * @param irq 中断号 (IRQ Number)
 * @note 通常在初始化完成后或临界区结束后调用。
 */
void irq_enable(uint32 irq);

/**
 * @brief 禁用指定中断
 * 
 * 关闭硬件中断控制器中指定 IRQ 号的开关，屏蔽该中断。
 * @param irq 中断号
 * @note 通常在中断处理程序中屏蔽同级/低级中断，或在临界区保护中使用。
 */
void irq_disable(uint32 irq);

/**
 * @brief 判断指定中断是否使能
 * 
 * 检查硬件中断控制器中指定 IRQ 号的中断使能开关
 * @param irq 中断号 (IRQ Number)
 * @return uint32 返回之前的中断使能状态，用于恢复。
 * @note 通常在临界区保护调用。
 */
uint32 irq_is_enable(uint32 irq);


/**
 * @brief 全局禁用中断 (关中断)
 * 
 * 禁用 CPU 的全局中断掩码，阻止所有可屏蔽中断的发生。
 * @return uint32 返回之前的中断状态标志 (PRIMASK 或类似寄存器值)，用于恢复。
 * @note 
 * - 必须与 enable_irq 配对使用。
 * - 关中断时间应尽可能短，以免严重影响系统实时性。
 */
uint32 disable_irq(void);

/**
 * @brief 全局使能中断 (开中断)
 * 
 * 恢复 CPU 的全局中断掩码到指定状态。
 * @param flag 之前由 disable_irq 返回的状态标志。
 */
void enable_irq(uint32 flag);

/**
 * @brief 设置中断优先级
 * 
 * 配置指定中断源的优先级。
 * @param irq 中断号
 * @param prio 优先级数值
 */
void irq_priority(uint32 irq, uint32 prio);

/**
 * @brief 申请/注册中断
 * 
 * 将用户定义的中断处理函数绑定到指定的中断号。
 * @param irq_num 中断号
 * @param handle 中断处理函数指针
 * @param data 传递给处理函数的用户数据
 * @return int32 
 *         - 0: 成功
 *         - 负值: 失败 (如中断号无效、已被占用等)
 * @note 
 * - 注册后默认可能是禁用状态，需调用 irq_enable。
 */
int32 request_irq(uint32 irq_num, irq_handle handle, void *data);

/**
 * @brief 释放/注销中断
 * 
 * 解除指定中断号的处理函数绑定，并通常会自动禁用该中断。
 * @param irq_num 中断号
 * @return int32 
 *         - 0: 成功
 *         - 负值: 失败 (如未注册)
 * @note 在卸载驱动或模块时必须调用，防止野指针调用。
 */
int32 release_irq(uint32 irq_num);

/**
 * @brief 获取系统统计周期内的中断执行时间，并打印各个中断的统计信息
 * 
 */
uint32 sysirq_time(void);

#ifdef __cplusplus
}
#endif
#endif /* __OS_IRQ_H_ */
