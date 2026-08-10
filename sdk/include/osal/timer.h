#ifndef __OS_TIMER_H_
#define __OS_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "osal/time.h"

/**
 * @brief 定时器回调函数类型定义
 * 
 * 当定时器超时或周期到达时，系统将调用此函数。
 * @param arg 用户创建定时器时传入的参数，用于区分不同的定时器实例或传递上下文。
 * @note 
 * - 该函数通常运行在定时器任务/线程上下文中，而非中断上下文（取决于具体实现）。
 * - 应避免在回调中执行长时间阻塞操作，以免影响其他定时器的精度。
 */
typedef void (*os_timer_func_t)(void *arg);

/**
 * @brief 定时器工作模式枚举
 */
enum OS_TIMER_MODE {
    /** 
     * @brief 单次模式 (One-shot)
     * 定时器触发一次后自动停止并进入非活动状态。
     * 若需再次触发，必须重新调用 os_timer_start。
     */
    OS_TIMER_MODE_ONCE,

    /** 
     * @brief 周期模式 (Periodic)
     * 定时器触发后，会自动重置计数并按照设定的间隔再次触发。
     * 除非显式调用 os_timer_stop，否则将一直运行。
     */
    OS_TIMER_MODE_PERIODIC
};

/**
 * @brief 定时器控制块结构体 (os_timer)
 * 
 * 描述一个软件定时器的所有属性和运行时统计信息。
 */
struct os_timer {
    uint32_t magic;           /**< [内部] 魔数，用于校验定时器是否已正确初始化，防止操作非法对象 */
    void    *hdl;             /**< [内部] 底层定时器句柄 (如 RTOS 的 Timer Handle 或链表节点) */
    os_timer_func_t cb;       /**< [只读] 超时回调函数指针 */
    void        *data;        /**< [只读] 传递给回调函数的用户参数 (arg) */
    
    uint8        mode;        /**< [配置] 工作模式: OS_TIMER_MODE_ONCE 或 OS_TIMER_MODE_PERIODIC */
    uint32       interval;    /**< [配置] 定时间隔时间 (单位通常为 Tick 或 ms，视实现而定) */
    
    /**
     * @brief 运行时统计字段
     * 以下字段用于监控定时器的执行情况，可用于性能分析或调试。
     */
    uint32_t     trigger_cnt; /**< [统计] 触发次数计数器。记录自启动以来回调被执行的总次数。 */
    uint32_t     total_time;  /**< [统计] 累计运行时间。记录回调函数执行耗时的总和 (单位视实现而定)。 */
    uint32_t     max_time;    /**< [统计] 最大单次执行时间。记录回调函数执行耗时的峰值，用于检测是否有阻塞风险。 */
};

typedef struct os_timer os_timer_t;

/**
 * @brief 初始化定时器
 * 
 * 配置定时器的基本属性，但不启动计时。
 * 
 * @param timer [Out] 指向定时器控制块的指针
 * @param func  [In]  超时回调函数
 * @param mode  [In]  工作模式 (单次 或 周期)
 * @param arg   [In]  传递给回调函数的用户参数
 * @return int  0: 成功, 非0: 失败
 * 
 * @note:
 * - 设置 magic 值、回调、模式和参数。
 * - 此时定时器处于“未运行”状态。
 */
int os_timer_init(os_timer_t *timer, os_timer_func_t func, enum OS_TIMER_MODE mode, void *arg);

/**
 * @brief 启动/重启定时器
 * 
 * 开始计时或重置现有计时器。
 * 
 * @param timer  [In] 指向定时器控制块的指针
 * @param expires [In] 超时时间 (单位通常为 Tick 或 ms)
 *                  - 对于单次模式：表示多久后触发一次。
 *                  - 对于周期模式：表示第一次触发的时间，后续按 init 时设定的 interval 循环。
 * @return int   0: 成功, 非0: 失败
 * 
 * @note:
 * - 若定时器已在运行，此调用通常会重置计时器（重新开始倒计时）。
 * - 确保 expires > 0。
 */
int os_timer_start(os_timer_t *timer, unsigned long expires);

/**
 * @brief 停止定时器
 * 
 * 暂停定时器，使其不再触发回调。
 * 
 * @param timer [In] 指向定时器控制块的指针
 * @return int  0: 成功, 非0: 失败
 * 
 * @note:
 * - 定时器进入“停止”状态，但配置（回调、模式、间隔）保留。
 * - 可随时通过 os_timer_start 再次启动。
 * - 不会清除统计计数 (trigger_cnt 等)。
 */
int os_timer_stop(os_timer_t *timer);

/**
 * @brief 删除/销毁定时器
 * 
 * 彻底释放定时器资源。
 * 
 * @param timer [In] 指向定时器控制块的指针
 * @return int  0: 成功, 非0: 失败
 * 
 * @note:
 * - 必须先停止定时器。
 * - 释放底层句柄 (hdl)。
 * - 销毁后不可再使用该结构体，除非重新 init。
 */
int os_timer_del(os_timer_t *timer);

/**
 * @brief 获取定时器当前的状态： 有效 或 无效
 * 
 * @param timer [In] 指向定时器控制块的指针
 * @return int  1: 有效(处于活动状态), 0: 无效（非活动状态)
 * 
 */
int os_timer_stat(os_timer_t *timer);

#ifdef __cplusplus
}
#endif
#endif /* __OS_TIMER_H_ */
