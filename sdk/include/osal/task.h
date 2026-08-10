#ifndef _OS_TASK_H_
#define _OS_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 任务入口函数类型定义
 * @param arg 传递给任务的参数指针
 */
typedef void (*os_task_func_t)(void *arg);

/**
 * @brief 任务标志位枚举
 */
enum OS_TASK_FLAGS{
    /** 
     * @brief 低功耗保活运行标志 (BIT 31)
     * 当系统进入低功耗模式时，若任务设置了此标志，该任务仍会被调度运行。
     * 通常用于看门狗、心跳包等关键维持任务。
     */
    OS_TASK_FLAGS_LPRUN = BIT(31), 
};

/**
 * @brief 任务优先级定义
 * 数值越大，优先级越高。
 */
typedef enum  {
    OS_TASK_PRIORITY_IDLE         = 0,    /**< 空闲优先级 (最低) */
    OS_TASK_PRIORITY_LOW          = 0x10, /**< 低优先级 */
    OS_TASK_PRIORITY_BELOW_NORMAL = 0x20, /**< 低于正常优先级 */
    OS_TASK_PRIORITY_NORMAL       = 0x30, /**< 正常优先级 (默认) */
    OS_TASK_PRIORITY_ABOVE_NORMAL = 0x40, /**< 高于正常优先级 */
    OS_TASK_PRIORITY_HIGH         = 0x50, /**< 高优先级 */
    OS_TASK_PRIORITY_REALTIME     = 0x60, /**< 实时优先级 */
    OS_TASK_PRIORITY_ISR          = 0xFF, /**< 中断服务程序虚拟优先级 (最高) */
} OS_TASK_PRIORITY;

/**
 * @brief 任务初始化宏
 * 一站式完成：初始化 -> 设置栈 -> 设置优先级 -> 运行任务
 * 
 * @param name     任务名称字符串
 * @param task     任务控制块变量名 (os_task_t 类型)
 * @param func     任务入口函数
 * @param data     传递给任务的参数 (转换为 uint32)
 * @param prio     任务优先级
 * @param stack    任务栈内存起始地址指针
 * @param stksize  任务栈大小 (字节)
 */
#define OS_TASK_INIT(name, task, func, data, prio, stack, stksize) do { \
        os_task_init((const uint8 *)name, task, (os_task_func_t)func, (uint32)data); \
        os_task_set_stack(task, stack, stksize); \
        os_task_set_priority(task, prio); \
        os_task_run(task);\
    }while(0)

/**
 * @brief 任务初始化宏别名
 * @note 当前定义为自身，可能用于版本兼容或未来扩展。
 */
#define OS_TASK_INIT2 OS_TASK_INIT2

/**
 * @brief 阻塞列表结构体 (用于管理等待队列)
 * @note 若未定义 OS_BLKLIST，则在此定义。
 */
#ifndef OS_BLKLIST
#define OS_BLKLIST
struct os_blklist{
    void          *hdl; /**< 底层阻塞列表句柄 */
};
typedef struct os_blklist os_blklist_t;
#endif

/**
 * @brief 任务控制块结构体
 * 描述任务的核心属性，由内核维护。
 */
typedef struct os_task {
    void          *hdl;            /**< 底层任务句柄 (TCB 指针或 ID) */
    os_task_func_t func;           /**< 任务入口函数指针 */
    const char    *name;           /**< 任务名称 */
    uint32_t       args;           /**< 任务参数 */
    
    /**
     * 位域定义 (共 32 位):
     * - priority (8 bit):  任务优先级 (0-255)
     * - stack_size (20 bit): 栈大小
     * - lprun (1 bit):     低功耗运行标志 (对应 OS_TASK_FLAGS_LPRUN)
     * - rev (3 bit):       保留位
     */
    uint32_t       priority:8, stack_size:20, lprun:1, rev:3;
    
    void          *stack;          /**< 任务栈起始地址指针 */
}os_task_t;

/**
 * @brief 任务运行时信息结构体
 * 用于获取任务的状态、CPU 占用、栈使用等统计信息。
 */
struct os_task_info {
    uint32 id;             /**< 任务 ID */
    const  char *name;     /**< 任务名称 */
    const  char *status;   /**< 任务状态字符串 (如 "Ready", "Suspend") */
    uint32 time;           /**< 运行时间或 CPU 占用计数 */
    uint32 stack;          /**< 栈使用量信息 */
    uint32 prio;           /**< 当前优先级 */
    uint32 arg;            /**< 任务参数 */
};

/**
 * @brief 初始化操作系统内核
 * @note 必须在启动调度器前调用，初始化内存、调度器等核心组件。
 */
void os_kernel_init(void);

/**
 * @brief 启动操作系统内核
 * @note 开始多任务调度，此函数通常不会返回。
 */
void os_kernel_start(void);

/**
 * @brief 设置控制台 UART 设备
 * @param uart UART 设备句柄或基地址
 * @note 用于重定向系统打印输出。
 */
void os_console_uart(void *uart);

/**
 * @brief 标记进入中断服务程序 (ISR)
 * @param irqn 中断号
 * @note 通知内核禁止任务切换，保护临界区。
 */
void OS_INTRPT_ENTER(int irqn);

/**
 * @brief 标记退出中断服务程序 (ISR)
 * @param irqn 中断号
 * @note 通知内核恢复调度，如有高优先级任务就绪则触发切换。
 */
void OS_INTRPT_EXIT(int irqn);

/**
 * @brief 初始化任务控制块
 * @param name 任务名称
 * @param task 任务控制块指针
 * @param func 任务入口函数
 * @param data 任务参数
 * @return int32 状态码 (0: 成功)
 */
int32 os_task_init(const uint8 *name, os_task_t *task, os_task_func_t func, uint32 data);

/**
 * @brief 获取任务优先级
 * @param task 任务控制块指针
 * @return int32 优先级值
 */
int32 os_task_priority(os_task_t *task);

/**
 * @brief 通过句柄获取任务优先级
 * @param hdl 任务句柄
 * @return int32 优先级值
 */
int32 os_task_priority2(void *hdl);

/**
 * @brief 获取任务栈大小
 * @param task 任务控制块指针
 * @return int32 栈大小
 */
int32 os_task_stacksize(os_task_t *task);

/**
 * @brief 通过句柄获取任务栈大小
 * @param hdl 任务句柄
 * @return int32 栈大小
 */
int32 os_task_stacksize2(void *hdl);

/**
 * @brief 设置任务优先级
 * @param task 任务控制块指针
 * @param pri 新优先级
 * @return int32 状态码
 */
int32 os_task_set_priority(os_task_t *task, uint32 pri);

/**
 * @brief 设置任务栈
 * @param task 任务控制块指针
 * @param stack 栈内存起始地址
 * @param stack_size 栈大小
 * @return int32 状态码
 */
int32 os_task_set_stack(os_task_t *task, void *stack, int32 stack_size);

/**
 * @brief 启动/运行任务
 * @param task 任务控制块指针
 * @return int32 状态码
 * @note 将任务状态置为 Ready，加入调度队列。
 */
int32 os_task_run(os_task_t *task);

/**
 * @brief 停止任务
 * @param task 任务控制块指针
 * @return int32 状态码
 * @note 暂停任务执行，从调度队列移除，但不释放资源。
 */
int32 os_task_stop(os_task_t *task);

/**
 * @brief 删除/销毁任务
 * @param task 任务控制块指针
 * @return int32 状态码
 * @note 彻底移除任务，释放内核资源。
 */
int32 os_task_del(os_task_t *task);

/**
 * @brief 获取任务运行时统计信息
 * @param tsk_times 接收信息的数组指针
 * @param count 数组容量
 * @param diff_tick 统计时间窗口 (Tick)
 * @return int32 实际填充的任务数量
 */
int32 os_task_runtime(struct os_task_info *tsk_times, int32 count, uint32 diff_tick);

/**
 * @brief 打印所有任务状态 (调试用)
 * @note 通常输出到控制台，显示任务名、状态、栈水位等。
 */
void os_task_print(void);

/**
 * @brief 获取当前运行任务的句柄
 * @return void* 当前任务句柄
 */
void *os_task_current(void);

/**
 * @brief 获取任务创建时传入的参数
 * @param hdl 任务句柄
 * @return void* 参数值
 */
void *os_task_data(void *hdl);

/**
 * @brief 挂起任务
 * @param task 任务控制块指针
 * @return int32 状态码
 * @note 强制暂停任务，需调用 resume 恢复。
 */
int32 os_task_suspend(os_task_t *task);

/**
 * @brief 恢复被挂起的任务
 * @param task 任务控制块指针
 * @return int32 状态码
 */
int32 os_task_resume(os_task_t *task);

/**
 * @brief 任务异常转储 (调试用)
 * @param hdl 任务句柄
 * @param stack 栈指针位置
 * @note 用于故障分析，打印堆栈内容。
 */
void os_task_dump(void *hdl, void *stack);

/**
 * @brief 获取系统中任务总数
 * @return int32 任务数量
 */
int32 os_task_count(void);

/**
 * @brief 主动让出 CPU
 * @return int32 状态码
 * @note 当前任务放弃剩余时间片，触发调度。
 */
int32 os_task_yield(void);

/* ================= 调度器控制 ================= */

/**
 * @brief 禁用任务调度
 * @return 0.
 * @note 进入临界区，禁止上下文切换。
 */
int32 os_sched_disable(void);

/**
 * @brief 启用任务调度
 * @return 0.
 * @note 退出临界区，恢复上下文切换。
 */
int32 os_sched_enbale(void);

/**
 * @brief 设置低功耗模式
 * @param enable 1: 进入低功耗模式, 0: 退出
 * @note 在低功耗模式下，仅调度标记了 LPRUN 标志的任务。
 */
void os_lpower_mode(uint8 enable);

/**
 * @brief 将句柄转换为任务控制块指针
 * @param hdl 任务句柄
 * @return os_task_t* 任务控制块指针
 * @note 只有使用了 os_task_t结构体 创建的task才能使用这个API进行转换使用。
 *       例如：os_task_t *task = os_task_hdl2tsk(os_task_current());
 */
os_task_t *os_task_hdl2tsk(void *hdl);

/* ================= 动态创建与句柄操作 ================= */

/**
 * @brief 动态创建任务
 * @param name 任务名称
 * @param func 入口函数
 * @param args 参数指针
 * @param prio 优先级
 * @param time 时间片 (若支持)
 * @param stack 栈指针 (若为 NULL 可能由内核分配)
 * @param stack_size 栈大小
 * @return void* 任务句柄，失败返回 NULL
 */
void *os_task_create(const char *name, os_task_func_t func, void *args, uint32 prio, uint32 time, void *stack, uint32 stack_size);

/**
 * @brief 动态销毁任务 (通过句柄)
 * @param hdl 任务句柄
 * @return int32 状态码
 */
int32 os_task_destroy(void *hdl);

/**
 * @brief 通过句柄挂起任务
 * @param hdl 任务句柄
 * @return int32 状态码
 */
int32 os_task_suspend2(void *hdl);

/**
 * @brief 通过句柄恢复任务
 * @param hdl 任务句柄
 * @return int32 状态码
 */
int32 os_task_resume2(void  *hdl);

/* ================= 阻塞列表管理 (内部/高级) ================= */

/**
 * @brief 初始化阻塞列表
 * @param blkobj 阻塞列表对象指针
 * @return int32 状态码
 */
int32 os_blklist_init(os_blklist_t *blkobj);

/**
 * @brief 删除阻塞列表
 * @param blkobj 阻塞列表对象指针
 */
void os_blklist_del(os_blklist_t *blkobj);

/**
 * @brief 将任务挂入阻塞列表 (等待)
 * @param blkobj 阻塞列表对象指针
 * @param task_hdl 要阻塞的任务句柄
 */
void os_blklist_suspend(os_blklist_t *blkobj, void *task_hdl);

/**
 * @brief 唤醒阻塞列表中的任务
 * @param blkobj 阻塞列表对象指针
 */
void os_blklist_resume(os_blklist_t *blkobj);

#ifdef __cplusplus
}
#endif
#endif
