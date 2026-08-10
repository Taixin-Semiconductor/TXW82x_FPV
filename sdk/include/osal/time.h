#ifndef __OS_TIME_H_
#define __OS_TIME_H_
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 时间单位常量定义
 */

#define OS_MS_PERIOD_TICK            (1000/OS_SYSTICK_HZ)

/** @brief 每秒的微秒数 (1,000,000) */
#define MICROSECONDS_PER_SECOND    ( 1000000LL )

/** @brief 每秒的纳秒数 (1,000,000,000) */
#define NANOSECONDS_PER_SECOND     ( 1000000000LL )

/** 
 * @brief 每个系统滴答 (Tick) 对应的纳秒数
 * @note 计算公式：1秒的纳秒数 / 系统滴答频率 (OS_SYSTICK_HZ)。
 *       例如：若 Hz=1000，则每 Tick 为 1,000,000 纳秒 (1ms)。
 */
#define NANOSECONDS_PER_TICK       ( NANOSECONDS_PER_SECOND / OS_SYSTICK_HZ )

/**
 * @brief 获取系统启动后经过的 Tick 数 (Jiffies)
 * @return uint64 自系统启动以来的 Tick 计数值。
 * @note 这是操作系统最基础的时间计量单位，随系统滴答中断递增。
 */
uint64 os_jiffies(void);

/**
 * @brief 获取系统启动后经过的秒数
 * @return uint32 自系统启动以来的秒数。
 * @note 精度较低，适用于粗略计时。
 */
uint32 os_seconds(void);

/**
 * @brief 获取系统启动后经过的毫秒数
 * @return uint64 自系统启动以来的毫秒数。
 * @note 常用单位，适用于大多数延时和超时计算。
 */
uint64 os_mseconds(void);

/**
 * @brief 获取系统启动后经过的微秒数
 * @return uint64 自系统启动以来的微秒数。
 * @note 精度较高，但受限于系统定时器分辨率，低 Hz 系统下末位可能不准确。
 */
uint64 os_useconds(void);

/**
 * @brief 获取 CPU 负载率
 * @return uint32 CPU 负载百分比 (0-100)。
 * @note 基于空闲任务 (Idle Task) 的运行时间计算得出。
 *       值越高表示 CPU 越忙，值越低表示系统越空闲。
 */
uint32 os_cpuloading(void);

/**
 * @brief 获取指定时钟的时间 (POSIX 兼容接口)
 * @param clk_id 时钟标识符 (如 CLOCK_REALTIME, CLOCK_MONOTONIC 等，视实现而定)
 * @param tp     输出参数，指向 struct timespec 结构体
 * @return int   0: 成功, -1: 失败 (并设置 errno)
 * @note 用于获取墙钟时间或单调时间。
 */
int clock_gettime(uint32 clk_id, struct timespec *tp);

/**
 * @brief 获取系统当前时间
 * @param tm 输出参数，指向 struct timespec 结构体
 * @note 填充当前的秒 (tv_sec) 和纳秒 (tv_nsec)。
 */
void os_systime(struct timespec *tm);

/**
 * @brief 计算两个时间点之间的差值 (转换为 Tick 数)
 * @param abstime 绝对结束时间
 * @param curtime 当前开始时间
 * @param result  输出参数，存储差值对应的 Tick 数 (uint64)
 * @return int32  0: 成功, 非0: 失败 (如结果为负)
 * @note 常用于计算超时剩余的 Tick 数：result = (abstime - curtime)。
 */
int32 timespec_detal_ticks(const struct timespec *abstime, const struct timespec *curtime, uint64 *result);

/**
 * @brief 将 timespec 时间转换为 Tick 数
 * @param time   输入的时间结构体
 * @param result 输出参数，存储对应的 Tick 数
 * @return int32 状态码
 * @note 将秒和纳秒总和转换为系统 Tick 整数。
 */
int32 timespec_to_ticks(const struct timespec *time, uint64 *result);

/**
 * @brief 将纳秒值转换为 timespec 结构体
 * @param llSource 输入的纳秒值 (int64)
 * @param time     输出的 timespec 结构体指针
 * @note 自动处理进位，将纳秒拆分为 tv_sec 和 tv_nsec。
 */
void nanosec_to_timespec(int64 llSource, struct timespec *time);

/**
 * @brief 两个 timespec 时间相加
 * @param x      加数 1
 * @param y      加数 2
 * @param result 输出结果 (x + y)
 * @return int32 状态码
 * @note 处理纳秒进位到秒的逻辑。
 */
int32 timespec_add(const struct timespec *x, const struct timespec *y, struct timespec *result);

/**
 * @brief timespec 时间加上指定的纳秒数
 * @param x           基础时间
 * @param llNanoseconds 要增加的纳秒数 (可为负数以实现减法)
 * @param result      输出结果
 * @return int32      状态码
 * @note 方便地给某个时间点增加一段延时。
 */
int32 timespec_add_nanosec(const struct timespec *x, int64 llNanoseconds, struct timespec *result);

/**
 * @brief 两个 timespec 时间相减
 * @param x      被减数
 * @param y      减数
 * @param result 输出结果 (x - y)
 * @return int32 状态码
 * @note 处理借位逻辑，若 x < y，结果可能为负 (取决于具体实现如何处理 tv_sec)。
 */
int32 timespec_sub(const struct timespec *x, const struct timespec *y, struct timespec *result);

/**
 * @brief 比较两个 timespec 时间
 * @param x 时间 1
 * @param y 时间 2
 * @return int32 
 *         - >0: x > y
 *         - 0 : x == y
 *         - <0: x < y
 */
int32 timespec_cmp(const struct timespec *x, const struct timespec *y);

/**
 * @brief 验证 timespec 时间是否合法
 * @param time 待验证的时间结构体
 * @return int32 
 *         - 0: 合法 (tv_nsec 在 0-999999999 之间)
 *         - 非0: 非法
 */
int32 timespec_validate(const struct timespec *time);

/**
 * @brief 将 Tick 数转换为毫秒数
 * @param jiff 输入的 Tick 数
 * @return uint64 对应的毫秒数
 * @note 公式：jiff * (1000 / OS_SYSTICK_HZ)
 */
uint64 os_jiffies_to_msecs(uint64 jiff);

/**
 * @brief 将毫秒数转换为 Tick 数
 * @param msec 输入的毫秒数
 * @return uint64 对应的 Tick 数 (向上或向下取整视实现而定)
 * @note 公式：msec * (OS_SYSTICK_HZ / 1000)
 * @warning 若毫秒数不能被 Tick 周期整除，可能存在精度丢失。
 */
uint64 os_msecs_to_jiffies(uint64 msec);

int gettimeofday2(struct timeval *ptimeval, uint64 msec);

#ifdef __cplusplus
}
#endif

#endif /* __OS_TIME_H_ */
