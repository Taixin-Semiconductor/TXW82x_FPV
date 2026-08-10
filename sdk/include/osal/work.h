#ifndef _OS_TASK_WORK_H_
#define _OS_TASK_WORK_H_

#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "osal/string.h"
#include "osal/time.h"
#include "osal/timer.h"
#include "osal/mutex.h"
#include "osal/task.h"
#include "osal/semaphore.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 前向声明：工作项结构体 */
struct os_work;
struct os_workqueue;

/**
 * @brief 工作项回调函数类型定义
 * 
 * @param work 指向当前执行的工作项结构体指针
 * @return int32 执行结果状态码
 *         - 返回 0: 表示成功。work 结构体依然有效，调用者可继续安全访问该指针。
 *         - 返回 非0: 【关键安全约束】表示 work 结构体已不可访问。
 *           常见场景：回调函数内部执行了自我释放 (free) 或将 ownership 转移。
 *           【警告】系统在收到非0返回值后，严禁再解引用 (dereference) 该 work 指针，
 *           否则会导致 Use-After-Free 错误。
 */
typedef int32 (*os_work_func_t)(struct os_work *work);

/**
 * @brief 通用运行函数回调类型定义
 *        用于不需要访问 work 结构体内部状态的简单任务。
 * @param param1 第一个用户参数
 * @param param2 第二个用户参数
 * @param param3 第三个用户参数
 */
typedef void (*os_run_func_t)(uint32 param1, uint32 param2, uint32 param3);

/**
 * @brief 工作项结构体 (os_work)
 * 
 * ============================================================================
 * 【重要安全约束】
 * 应用程序代码 **严禁直接修改** 此结构体内的任何字段 (包括 list, flags, expired 等)。
 * 所有状态的变更 (初始化、调度、取消、重置) **必须** 通过提供的 API 函数进行操作。
 * 直接修改可能导致：
 *   1. 竞态条件 (Race Conditions)
 *   2. 链表指针损坏 (List Corruption)
 *   3. 状态机不一致 (State Inconsistency)
 *   4. 系统崩溃或死锁
 * ============================================================================
 */
struct os_work {
    struct list_head list;          /**< [内部] 链表节点，用于将 work 挂载到 wkq->works 或 wkq->delay_works */
    struct os_workqueue *wkq;       /**< [内部] 指向所属工作队列的指针，由 schedule 接口自动设置 */
    
    /* 位域定义 (共16bit)，紧凑存储内部状态标志 */
    uint16 running: 1,              /**< [内部] 运行标志：1-正在被线程执行，0-空闲/等待中 */
           alloc : 1,               /**< [内部] 分配标志：
                                     *   1: 动态分配模式。work 由 API (如 os_run_func) 内部创建，
                                     *      执行完成后由系统自动释放。
                                     *   0: 静态分配模式。work 由用户分配 (栈或全局)，生命周期由用户管理。
                                     */
           delay : 1,               /**< [内部] 延迟标志：1-延迟任务 (在 delay_works 链表)，0-即时任务 */
           pri   : 5,               /**< [内部] 优先级：5比特，范围 0-31。数值越大优先级越高 (具体视调度器实现) */
           init  : 1,               /**< [内部] 初始化标志：1-已初始化可调度，0-未初始化/已失效。
                                     *   注意：os_work_cancle2 会将此位清零。
                                     */
           rev   : 7;               /**< [内部] 保留位，填充至 16bit，供未来扩展 */
           
    uint16 schedule;                /**< [内部] 调度计数器 (防丢失机制)
                                     * 行为逻辑：
                                     * 1. 当 work 不在队列中时，调用 schedule 将其加入队列，count = 1。
                                     * 2. 当 work 已在队列中 (running=0, 但在 list 上) 时，再次调用 schedule，
                                     *    不会重复插入链表，而是使 count++。
                                     * 3. work 执行完毕后，若 count > 1，则 count-- 并自动重新触发调度，
                                     *    确保多次调度请求不会被丢弃。
                                     */
    uint64 expired;                 /**< [内部] 过期时间戳 (绝对时间)。
                                     * 仅对 delay=1 的任务有效。系统当前时间 >= expired 时触发执行。
                                     */
    uint32 runtime;                 /**< [内部/统计] 最后一次执行耗时。
                                     * 单位：系统 Tick。用于性能分析和监控模块检测长任务。
                                     */
    
    os_work_func_t func;            /**< [用户/内部] 任务执行函数指针。由 OS_WORK_INIT 设置。 */
};

/**
 * @brief 工作队列结构体 (os_workqueue)
 * 
 * 核心组件：包含一个专用的后台线程 (task)、定时器 (timer) 和两个任务链表。
 * 负责按顺序或优先级执行提交的工作项。
 */
struct os_workqueue {
    char  *name;                    /**< 队列名称，用于日志打印和监控标识 */
    
    /* 位域定义 */
    uint16 priority: 10,            /**< 线程优先级：10比特，范围 0-1023 */
           init   : 1,              /**< [内部] 初始化标志：1-队列已启动，0-未启动 */
           rev    : 5;              /**< [内部] 保留位 */
           
    uint16 stack_size;              /**< 线程栈大小 (字节)。若 init 时传入 NULL stack，则由此决定自动分配的大小 */
    
    os_timer_t  timer;              /**< [内部] 硬件/软定时器句柄。用于监听 delay_works 中最近的一个到期时间 */
    os_task_t   task;               /**< [内部] 后端处理线程句柄。死循环从 works 链表取任务执行 */
    
    struct list_head works;         /**< [内部] 即时任务链表 (优先级+FIFO排序)。存放 ready 的任务 */
    struct list_head delay_works;   /**< [内部] 延迟任务链表。按 expired 时间排序 */
    
    os_semaphore_t sema;            /**< [内部] 同步信号量。
                                     * 触发源：新任务加入 (schedule) 或 延迟任务到期 (timer callback)。
                                     */
    
    void  *run_func;                /**< [内部/调试] 快照：记录当前线程正在执行的 func 指针。
                                     * 用途：当看门狗检测到卡死时，可定位是哪个函数阻塞了队列。
                                     */
    uint64 run_jiff;                /**< [内部/调试] 快照：记录当前 func 开始执行的时刻 (jiffies)。
                                     * 用途：结合当前时间计算耗时，判断是否超时。
                                     */
};

/**
 * @brief 初始化系统 Workqueue 监测模块 (看门狗)
 * @return 状态码 (0: 成功, 其他: 失败)
 * 
 * @note 功能描述：
 *       该模块启动一个监控线程，定期扫描所有注册的工作队列。
 *       检测逻辑：若某队列的 run_jiff 距离当前时间超过阈值，且 run_func 不为空，
 *       则判定该 work/func 长时间占用队列，可能触发报警、或打印堆栈。
 *       用于防止单个坏任务导致整个异步系统停滞。
 */
int32 os_wkqmonitor_init(void);

/**
 * @brief 初始化一个工作队列
 * @param wkq       [In]  工作队列结构体指针 (需由调用者分配内存，如全局变量或 malloc)
 * @param name      [In]  队列名称字符串 (建议具有描述性，如 "net_rx_wq")
 * @param priority  [In]  后端处理线程的优先级
 * @param stack     [In]  线程栈内存指针。若传 NULL，系统将内部自动分配大小为 stack_size 的栈。
 * @param stack_size[In]  线程栈大小 (字节)。若 stack 不为 NULL，此参数用于校验或忽略。
 * @return 状态码 (0: 成功, 其他: 失败)
 * 
 * @note 调用成功后，wkq->init 将被置为 1，后端线程开始运行。
 */
int32 os_workqueue_init(struct os_workqueue *wkq, char *name, uint16 priority, void *stack, uint16 stack_size);

/**
 * @brief 去初始化/销毁工作队列
 * @param wkq [In] 工作队列结构体指针
 * @return 状态码
 * 
 * @note 行为：
 *       1. 停止后端线程。
 *       2. 清空 works 和 delay_works 链表 (未执行的任务将被丢弃或根据实现报错)。
 *       3. 销毁定时器和信号量。
 *       【警告】调用前请确保没有外部线程正在对该队列进行 schedule 操作。
 */
int32 os_workqueue_deinit(struct os_workqueue *wkq);

/**
 * @brief 将工作队列注册/注销到监测模块
 * @param wkq [In] 工作队列指针
 * @param add [In] 操作类型：1-添加监测，0-移除监测
 * @return 状态码
 * 
 * @note 建议在 os_workqueue_init 成功后立即调用 add=1，在 deinit 前调用 add=0。
 */
int32 os_workqueue_monitor(struct os_workqueue *wkq, uint8 add);

/**
 * @brief 调度工作项立即执行
 * @param wkq  [In] 目标工作队列
 * @param work [In] 工作项指针 (必须已通过 OS_WORK_INIT 初始化)
 * 
 * @note 内部逻辑：
 *       - 检查 work->init 标志。
 *       - 若 work 已在队列中 (running=0 但在 list 上)：仅增加 work->schedule 计数，不重复插入。
 *       - 若 work 不在队列中：设置 running=0, schedule=1，插入 wkq->works 链表尾部/优先级位置，
 *         并 post 信号量唤醒线程。
 */
void os_work_schedule(struct os_workqueue *wkq, struct os_work *work);

/**
 * @brief 调度工作项延迟执行
 * @param wkq      [In] 目标工作队列
 * @param work     [In] 工作项指针
 * @param delay_ms [In] 延迟时间 (毫秒)
 * 
 * @note 内部逻辑：
 *       - 计算 expired = current_time + delay_ms。
 *       - 若 work 已在 delay_works 链表中：移除旧节点，更新 expired，重新插入以保持时间序。
 *       - 更新内部定时器以匹配最新的最近到期时间。
 */
void os_work_schedule_delay(struct os_workqueue *wkq, struct os_work *work, uint32 delay_ms);

/**
 * @brief 取消工作项
 * @param work [In] 工作项指针
 * @param sync [In] 同步标志
 *             - 1 (Sync): 如果 work 正在运行 (running=1)，调用线程将阻塞，直到 work 执行完毕返回。
 *                         如果 work 在队列中，直接移除并返回。
 *             - 0 (Async): 尝试从队列中移除。如果 work 正在运行，立即返回，不等待。
 * 
 * @note 取消成功后，work 恢复到可重新调度的状态 (init 保持为 1)。
 */
void os_work_cancle(struct os_work *work, uint8 sync);

/**
 * @brief 取消工作项 (严格模式/销毁模式)
 * @param work [In] 工作项指针
 * @param sync [In] 同步标志 (同 os_work_cancle)
 * 
 * @note 【关键约束】：
 *       此接口不仅取消任务，还会将 work->init 清零，标记为“未初始化”。
 *       【强制要求】：若要再次使用该 work 指针，**必须** 先调用 OS_WORK_REINIT 或 OS_WORK_INIT。
 *       直接再次 schedule 未 reinit 的 work 将被忽略或触发断言。
 *       适用场景：确定该 work 生命周期结束，或防止误复用。
 */
void os_work_cancle2(struct os_work *work, uint8 sync);

/**
 * @brief 便捷接口：在主工作队列 (mainwkq) 中执行一个通用函数
 * @param func   [In] 函数指针
 * @param param1 [In] 参数 1
 * @param param2 [In] 参数 2
 * @param param3 [In] 参数 3
 * @return 状态码
 * 
 * @note 内部实现：系统会自动创建一个临时 alloc=1 的 work 结构体，封装 func 和参数，
 *       提交到 mainwkq。执行完成后自动释放内存。用户无需管理 work 生命周期。
 */
int32 os_run_func(os_run_func_t func, uint32 param1, uint32 param2, uint32 param3);

/**
 * @brief 便捷接口：在主工作队列 (mainwkq) 中延迟执行一个通用函数
 * @param func     [In] 函数指针
 * @param param1   [In] 参数 1
 * @param param2   [In] 参数 2
 * @param delay_ms [In] 延迟毫秒数
 * @return 状态码
 */
int32 os_run_func_delay(os_run_func_t func, uint32 param1, uint32 param2, uint32 delay_ms);

/**
 * @brief 便捷接口：在主工作队列 (mainwkq) 中执行一个已定义的 work
 * @param work [In] 工作项指针 (需用户自行管理生命周期，通常 alloc=0)
 * @return 状态码 (透传 work->func 的返回值)
 * 
 * @note 等同于 os_work_schedule(mainwkq, work)。
 */
int32 os_run_work(struct os_work *work);

/**
 * @brief 便捷接口：在主工作队列 (mainwkq) 中延迟执行一个 work
 * @param work     [In] 工作项指针
 * @param delay_ms [In] 延迟毫秒数
 * @return 状态码
 */
int32 os_run_work_delay(struct os_work *work, uint32 delay_ms);

/**
 * @brief 工作项调度生命周期钩子 (Hook)
 * 
 * @param wkq     [In] 所属工作队列
 * @param work    [In] 工作项指针
 * @param start   [In] 阶段标识
 *                - 1: 即将开始执行 (Before Run)。此时 runtime 参数无效。
 *                - 0: 刚刚执行结束 (After Run)。
 * @param runtime [In] 运行时长 (Tick 数)
 *                - 仅在 start=0 时有效，表示刚才那次执行消耗的时间。
 * 
 * @note 此函数由工作队列线程内部调用。
 *       用户可实现此函数 (若为弱符号) 或在系统配置中开启，用于：
 *       1. 统计每个任务的平均耗时。
 *       2. 调试打印任务调度轨迹。
 *       3. 动态调整优先级。
 */
void os_work_schedule_hook(struct os_workqueue *wkq, struct os_work *work, uint8 start, uint32 runtime);

/**
 * @brief 初始化工作项
 * @param work     [In] 工作项指针
 * @param func     [In] 回调函数指针
 * @param priority [In] 任务优先级 (0-31)
 * @return 状态码
 * 
 * @note 动作：
 *       - 设置 work->func = func
 *       - 设置 work->pri = priority
 *       - 设置 work->init = 1
 *       - 清零 running, schedule, delay 等状态位
 *       使 work 进入“待调度”状态。
 */
int32 OS_WORK_INIT(struct os_work *work, os_work_func_t func, int32 priority);

/**
 * @brief 重新初始化工作项
 * @param work [In] 工作项指针
 * @return 状态码
 * 
 * @note 动作：
 *       - 保留 func 和 priority 不变 (或根据实现重置)。
 *       - 强制设置 work->init = 1。
 *       - 清零 running, schedule 等运行时状态。
 *       
 * 【典型用法】：
 * 在调用 os_work_cancle2(work, ...) 之后，work 变为未初始化状态。
 * 若要复用该 work 对象，必须调用此函数，否则后续 schedule 无效。
 */
int32 OS_WORK_REINIT(struct os_work *work);

#ifdef __cplusplus
}
#endif

#endif /* _OS_TASK_WORK_H_ */
