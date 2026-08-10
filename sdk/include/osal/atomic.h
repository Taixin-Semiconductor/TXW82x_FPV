#ifndef _OS_ATOMIC_H_
#define _OS_ATOMIC_H_

#include "typesdef.h"
#include "osal/irq.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup OS_Atomic 原子操作接口
 * @brief 提供基于中断屏蔽的多平台原子计数器实现。
 * @{
 */

/**
 * @brief 8 位原子计数器结构体
 */
typedef struct {
    volatile uint8 counter; ///< 计数器值
} atomic8_t;

/**
 * @brief 16 位原子计数器结构体
 */
typedef struct {
    volatile uint16 counter; ///< 计数器值
} atomic16_t;

/**
 * @brief 32 位原子计数器结构体 (默认类型)
 */
typedef struct {
    volatile uint32 counter; ///< 计数器值
} atomic_t;

/**
 * @brief 64 位原子计数器结构体
 */
typedef struct {
    volatile uint64 counter; ///< 计数器值
} atomic64_t;

/**
 * @brief 读取原子变量的当前值
 * @param v 指向原子变量的指针。
 * @return 当前计数器的值。
 * @note 由于 counter 是 volatile 且基础类型读取通常是原子的，此操作无需关中断。
 */
#define atomic_read(v)          ((v)->counter)

/**
 * @brief 设置原子变量的初始值
 * @param v 指向原子变量的指针。
 * @param i 要设置的新值。
 * @note 操作通过关闭本地中断来保证原子性。
 */
#define atomic_set(v, i)        ({ \
        uint32 __mask__ = disable_irq(); \
        ((v)->counter = (i)); \
        enable_irq(__mask__); \
    })

/**
 * @brief 原子增加操作
 * @param v 指向原子变量的指针。
 * @param i 增加的数值。
 * @note 操作通过关闭本地中断来保证原子性。
 */
#define atomic_add(v, i)        ({ \
        uint32 __mask__ = disable_irq(); \
        ((v)->counter += (i)); \
        enable_irq(__mask__); \
    })

/**
 * @brief 原子减少操作 (饱和递减)
 * @param v 指向原子变量的指针。
 * @param i 减少的数值。
 * @details 如果当前值大于 i，则减去 i；否则将值置为 0 (防止下溢变为负数)。
 * @note 操作通过关闭本地中断来保证原子性。
 */
#define atomic_sub(v, i)        ({ \
        uint32 __mask__ = disable_irq(); \
        if ((v)->counter > (i)) { ((v)->counter -= (i)); } \
        else { (v)->counter = 0; } \
        enable_irq(__mask__); \
    })

/**
 * @brief 原子减少操作并返回旧值
 * @param v 指向原子变量的指针。
 * @param i 减少的数值。
 * @return uint32 减少操作**之前**的计数器值。
 * @details 行为同 @ref atomic_sub，但返回修改前的值。
 */
#define atomic_sub_return(v, i) ({ \
        uint32 __mask__ = disable_irq(); \
        uint32 __val__ = (v)->counter; \
        if ((v)->counter > (i)) { ((v)->counter -= (i)); } \
        else { (v)->counter = 0; } \
        enable_irq(__mask__); \
        __val__; \
    })

/**
 * @brief 原子减少操作并返回新值
 * @param v 指向原子变量的指针。
 * @param i 减少的数值。
 * @return uint32 减少操作**之后**的计数器值。
 * @details 行为同 @ref atomic_sub，但返回修改后的值。
 */
#define atomic_sub2_return(v, i) ({ \
        uint32 __mask__ = disable_irq(); \
        if ((v)->counter > (i)) { ((v)->counter -= (i)); } \
        else { (v)->counter = 0; } \
        uint32 __val__ = (v)->counter; \
        enable_irq(__mask__); \
        __val__; \
    })

/**
 * @brief 原子自增 1
 * @param v 指向原子变量的指针。
 */
#define atomic_inc(v)           ({ \
        uint32 __mask__ = disable_irq(); \
        ((v)->counter++); \
        enable_irq(__mask__); \
    })

/**
 * @brief 原子自减 1 (饱和递减)
 * @param v 指向原子变量的指针。
 * @details 如果当前值大于 0，则减 1；否则保持为 0。
 */
#define atomic_dec(v)           ({ \
        uint32 __mask__ = disable_irq(); \
        if ((v)->counter > 0) { (v)->counter--; } \
        enable_irq(__mask__); \
    })

/**
 * @brief 原子自增 1 并返回新值
 * @param v 指向原子变量的指针。
 * @return uint32 自增**之后**的值。
 */
#define atomic_inc_return(v)    ({ \
        uint32 __mask__ = disable_irq(); \
        uint32 __val__ = (++(v)->counter); \
        enable_irq(__mask__); \
        __val__; \
    })

/**
 * @brief 原子自增 1 并返回旧值
 * @param v 指向原子变量的指针。
 * @return uint32 自增**之前**的值。
 */
#define atomic_inc2_return(v)   ({ \
        uint32 __mask__ = disable_irq(); \
        uint32 __val__ = (v)->counter++; \
        enable_irq(__mask__); \
        __val__; \
    })

/**
 * @brief 原子自减 1 并返回新值
 * @param v 指向原子变量的指针。
 * @return uint32 自减**之后**的值。
 */
#define atomic_dec_return(v)    ({ \
        uint32 __mask__ = disable_irq(); \
        if ((v)->counter) { (v)->counter--; } \
        uint32 __val__ = (v)->counter; \
        enable_irq(__mask__); \
        __val__; \
    })

/**
 * @brief 原子自减 1 并返回旧值
 * @param v 指向原子变量的指针。
 * @return uint32 自减**之前**的值。
 */
#define atomic_dec2_return(v)   ({ \
        uint32 __mask__ = disable_irq(); \
        uint32 __val__ = (v)->counter; \
        if ((v)->counter) { (v)->counter--; } \
        enable_irq(__mask__); \
        __val__; \
    })

/**
 * @brief 原子自减 1 并测试是否为 0
 * @param v 指向原子变量的指针。
 * @return int 如果减 1 后值为 0，返回 1 (真)；否则返回 0 (假)。
 * @details 常用于引用计数释放场景：当计数归零时执行清理操作。
 * @note 如果初始值为 0，不会减为负数，直接返回 1 (视为已测试通过/需清理)。
 */
#define atomic_dec_and_test(v)  ({ \
        uint32 __mask__ = disable_irq(); \
        uint32 __val__ = ((v)->counter > 0) ? ((--(v)->counter) == 0) : 1; \
        enable_irq(__mask__); \
        __val__; \
    })

/**
 * @brief 比较并交换 (Compare-And-Swap, CAS)
 * @param v 指向原子变量的指针。
 * @param o 期望的旧值 (Old Value)。
 * @param n 新值 (New Value)。
 * @return uint32 操作**之前**的实际值。
 * @details 如果当前值等于 o，则将其更新为 n；否则保持不变。
 *          返回值可用于判断交换是否成功 (返回值 == o 表示成功)。
 */
#define atomic_cmpxchg(v, o, n) ({ \
        uint32 __mask__ = disable_irq(); \
        uint32 __val__ = (v)->counter; \
        if (__val__ == (o)) { (v)->counter = (n); } \
        enable_irq(__mask__); \
        __val__; \
    })

/**
 * @brief 除非达到上限，否则原子增加
 * @details 如果当前值不等于 u (unless)，则增加 a；否则不操作。
 *          使用 CAS 循环实现无锁并发安全。
 * 
 * @param v 指向原子变量的指针。
 * @param a 增加的数值。
 * @param u 上限值 (Unless value)。如果当前值等于此值，则不增加。
 * @return int 如果执行了增加操作返回 1 (真)，如果因达到上限未执行返回 0 (假)。
 */
static inline int atomic_add_unless(atomic_t *v, int a, int u)
{
    int c, old;
    c = atomic_read(v);
    // CAS 循环：当 c != u 且 交换失败 (old != c) 时重试
    while (c != u && (old = atomic_cmpxchg((v), c, c + a)) != c) {
        c = old;
    }
    return c != u;
}

/** @} */ // End of OS_Atomic group

#ifdef __cplusplus
}
#endif

#endif /* _OS_ATOMIC_H_ */
