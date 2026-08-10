#ifndef __OS_EVENT_H
#define __OS_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 事件等待模式枚举 (OS_EVENT_WMODE)
 * 
 * 用于配置 os_event_wait 的行为，支持位掩码组合使用。
 */
enum OS_EVENT_WMODE {
    /**
     * @brief AND 模式 (所有位匹配)
     * 
     * 只有当事件标志寄存器中 **所有** 在 'flags' 参数中指定的位都被置为 1 时，
     * 等待线程才会被唤醒。
     * 
     * 示例: wait(flags=0x03, mode=AND) -> 需要 bit0 和 bit1 同时为 1 才唤醒。
     */
    OS_EVENT_WMODE_AND   = BIT(0),   

    /**
     * @brief OR 模式 (任意位匹配)
     * 
     * 只要事件标志寄存器中 **任意一个** 在 'flags' 参数中指定的位被置为 1，
     * 等待线程就会被唤醒。
     * 
     * 示例: wait(flags=0x03, mode=OR) -> bit0 或 bit1 任意一个为 1 即唤醒。
     * 
     * @note 如果同时设置了 AND 和 OR 标志，具体行为取决于实现，通常 OR 优先级较高或视为非法输入，
     *       建议明确只使用其中一种逻辑模式。
     */
    OS_EVENT_WMODE_OR    = BIT(1),   

    /**
     * @brief 自动清除模式 (Auto-Clear)
     * 
     * 当等待条件满足且线程被唤醒后，系统会自动清除触发该唤醒的事件位。
     * 
     * - 若配合 OR 模式：通常只清除那个导致唤醒的特定位置 0。
     * - 若配合 AND 模式：通常将所有参与匹配的位全部清零。
     * 
     * @note 如果不设置此标志，事件位将保持为 1，后续调用 os_event_wait 可能会立即返回（非阻塞）。
     */
    OS_EVENT_WMODE_CLEAR = BIT(2)    
};

/**
 * @brief 事件标志组结构体 (os_event)
 * 
 * 事件标志组是一种同步机制，允许一个或多个线程等待一个或多个事件位（bit）的发生。
 * 适用于“多对多”的同步场景（多个任务等待多个不同的事件组合）。
 */
struct os_event {
    uint32 magic;       /**< [内部] 魔数，用于校验结构体是否已正确初始化，防止操作未初始化的对象 */
    void  *hdl;         /**< [内部] 底层事件对象句柄 (如 RTOS 的 EventGroup 指针或内部实现结构) */
};

/**
 * @brief 事件标志组类型定义
 */
typedef struct os_event os_event_t;

/**
 * @brief 初始化事件标志组
 * @param evt [Out] 指向事件结构体的指针
 * @return int32 状态码 (0: 成功, 非0: 失败)
 * 
 * @note:
 * - 创建底层事件对象，初始所有事件位通常为 0。
 * - 设置 magic 值。
 * - 必须在首次使用前调用。
 */
int32 os_event_init(os_event_t *evt);

/**
 * @brief 销毁/删除事件标志组
 * @param evt [In] 指向事件结构体的指针
 * @return int32 状态码
 * 
 * @note:
 * - 释放底层资源 (hdl)。
 * - 【警告】：调用前必须确保没有线程正在该事件上等待，否则可能导致未定义行为。
 * - 通常在模块卸载或对象析构时调用。
 */
int32 os_event_del(os_event_t *evt);

/**
 * @brief 设置事件位 (置 1)
 * @param evt    [In] 指向事件结构体的指针
 * @param flags  [In] 要置为 1 的位掩码 (例如：BIT(0) | BIT(2))
 * @param rflags [Out] 可选输出参数。若不为 NULL，返回设置操作完成后的当前事件寄存器总值。
 * @return int32 状态码
 * 
 * @note:
 * - 此操作是原子的。
 * - 设置位后，系统会检查是否有线程在等待这些位。如果有线程的等待条件被满足，
 *   该线程将被唤醒（从 os_event_wait 返回）。
 * - 不会清除任何已有的位 (OR 操作)。
 */
int32 os_event_set(os_event_t *evt, uint32 flags, uint32 *rflags);

/**
 * @brief 清除事件位 (置 0)
 * @param evt    [In] 指向事件结构体的指针
 * @param flags  [In] 要置为 0 的位掩码
 * @param rflags [Out] 可选输出参数。若不为 NULL，返回清除操作完成后的当前事件寄存器总值。
 * @return int32 状态码
 * 
 * @note:
 * - 此操作是原子的。
 * - 仅修改指定位为 0，不影响其他位。
 * - 通常用于手动重置状态，或在处理完事件后清理标志（如果不使用 AUTO_CLEAR 模式）。
 */
int32 os_event_clear(os_event_t *evt, uint32 flags, uint32 *rflags);

/**
 * @brief 获取当前事件状态 (非阻塞)
 * @param evt    [In] 指向事件结构体的指针
 * @param rflags [Out] 输出参数，返回当前事件寄存器的所有位状态。
 * @return int32 状态码
 * 
 * @note:
 * - 立即返回，不阻塞当前线程。
 * - 用于轮询检查事件状态，或者在不希望进入等待状态时读取当前标志位。
 */
int32 os_event_get(os_event_t *evt, uint32 *rflags);

/**
 * @brief 等待事件发生 (阻塞)
 * 
 * @param evt     [In] 指向事件结构体的指针
 * @param flags   [In] 关注的事件位掩码 (要等待哪些位)
 * @param rflags  [Out] 可选输出参数。若唤醒，返回 **实际触发唤醒** 的事件位组合 (或者是当前的寄存器值，视实现而定)。
 * @param mode    [In] 等待模式组合 (见 OS_EVENT_WMODE 枚举)
 *                  - 必须指定 OS_EVENT_WMODE_AND 或 OS_EVENT_WMODE_OR 之一。
 *                  - 可选追加 OS_EVENT_WMODE_CLEAR。
 * @param timeout [In] 超时时间 (毫秒或系统 Tick，视具体实现定义)
 *                  - 0: 立即返回 (非阻塞检查)。
 *                  - >0: 等待指定时间。
 *                  - -1 / OS_WAIT_FOREVER: 永久等待。
 * 
 * @return int32 状态码
 *         - 0: 成功，等待的条件已满足。
 *         - 非0: 超时或发生错误。
 * 
 * @note 【工作流程】:
 * 1. 根据 mode 检查当前 flags 状态：
 *    - AND 模式: 检查 (current & flags) == flags
 *    - OR 模式:  检查 (current & flags) != 0
 * 2. 如果条件已满足：
 *    - 若 mode 包含 CLEAR，则自动清除相应的位。
 *    - 立即返回成功。
 * 3. 如果条件不满足：
 *    - 当前线程进入阻塞状态，挂入等待队列。
 *    - 直到：
 *      a) 其他任务调用 os_event_set 设置了相关位，满足了条件。
 *      b) 超时时间到达。
 * 4. 被唤醒后：
 *    - 若因条件满足而唤醒且 mode 含 CLEAR，则清除位。
 *    - 填充 rflags (如有)。
 *    - 返回。
 * 
 * @par 典型用法示例:
 * // 等待 BIT0 和 BIT1 同时发生，发生后自动清除这两个位
 * uint32 actual_flags;
 * os_event_wait(&evt, (BIT(0)|BIT(1)), &actual_flags, OS_EVENT_WMODE_AND | OS_EVENT_WMODE_CLEAR, OS_WAIT_FOREVER);
 */
int32 os_event_wait(os_event_t *evt, uint32 flags, uint32 *rflags, uint32 mode, int32 timeout);

#ifdef __cplusplus
}
#endif
#endif /* __OS_EVENT_H */
