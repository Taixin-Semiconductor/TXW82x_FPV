/**
 * @file rbuffer.h
 * @brief 环形缓冲区 (Ring Buffer) 通用实现头文件
 * 
 * 本模块提供了一套高效、灵活的环形缓冲区实现，适用于嵌入式系统中的数据流处理。
 * 主要特性包括：
 * - 支持静态编译期分配和动态运行时分配两种模式。
 * - 提供宏定义实现的单字节读写操作，零函数调用开销。
 * - 提供中断安全 (Interrupt Safe) 的读写宏，通过关中断实现原子操作。
 * - 支持“强制写入”模式（当缓冲区满时覆盖最旧数据）。
 * - 包含针对硬件模块优化的乒乓缓冲区 (Ping-Pong Buffer) 实现。
 * 
 * @note 环形缓冲区采用“浪费一个单元”策略来区分空和满状态：
 *       - 空：读指针 (rpos) == 写指针 (wpos)
 *       - 满：(写指针 + 1) % 大小 == 读指针
 */

#ifndef __RBUFFER_H__
#define __RBUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ================= 核心辅助宏 ================= */

/**
 * @brief 计算环形缓冲区的下一个位置索引
 * 
 * 该宏处理索引的回绕逻辑。当当前位置加上偏移量超过队列大小时，自动回绕到头部。
 * 
 * @param rb 环形缓冲区结构体指针
 * @param pos 当前指针成员名 (如 rpos 或 wpos)
 * @param i 偏移量 (通常为 1)
 * @return uint32 计算后的新索引位置
 */
#define RB_NPOS(rb, pos, i) ({ \
        uint32 __pos__ = (rb)->pos;\
        ((__pos__+(i)>=(rb)->qsize) ? (__pos__+(i)-(rb)->qsize) : (__pos__+(i))); \
    })

/* ================= 状态检查宏 ================= */

/**
 * @brief 检查缓冲区是否为空
 * 
 * 判断条件：读指针等于写指针。
 * @param rb 环形缓冲区结构体指针
 * @return 非0表示为空，0表示非空
 */
#define RB_EMPTY(rb) ((rb)->wpos == (rb)->rpos)

/**
 * @brief 检查缓冲区是否已满
 * 
 * 判断条件：(写指针 + 1) % 大小 == 读指针。
 * 注意：为了区分空和满，实际可用容量为 qsize - 1。
 * 
 * @param rb 环形缓冲区结构体指针
 * @return 非0表示已满，0表示未满
 */
#define RB_FULL(rb) ({ \
        uint32 _rpos_ = ((rb)->rpos);\
        uint32 _wpos_ = ((rb)->wpos)+1;\
        ((_wpos_>=(rb)->qsize) ? (_wpos_-(rb)->qsize) : (_wpos_)) == (_rpos_);\
    })

/**
 * @brief 获取缓冲区中当前已存入的数据长度 (可读字节数)
 * 
 * 根据读指针和写指针的相对位置计算有效数据量。
 * 
 * @param rb 环形缓冲区结构体指针
 * @return uint32 当前数据长度
 */
#define RB_COUNT(rb) ({ \
        uint32 _rpos_ = ((rb)->rpos);\
        uint32 _wpos_ = ((rb)->wpos);\
        ((_rpos_<=_wpos_)? (_wpos_-_rpos_): ((rb)->qsize-_rpos_+_wpos_));\
    })

/**
 * @brief 获取缓冲区中剩余可用空间长度 (可写字节数)
 * 
 * 计算还能写入多少数据而不发生溢出（不包含保留的那个区分位）。
 * 
 * @param rb 环形缓冲区结构体指针
 * @return uint32 剩余空间长度
 */
#define RB_IDLE(rb) ({ \
        uint32 _rpos_ = ((rb)->rpos);\
        uint32 _wpos_ = ((rb)->wpos);\
        ((_wpos_<_rpos_)? (_rpos_-_wpos_-1): ((rb)->qsize-_wpos_+_rpos_-1));\
    })

/* ================= 静态定义宏 ================= */

/**
 * @brief 静态定义环形缓冲区结构体 (内部包含数组)
 * 
 * 在编译期分配固定大小的缓冲区内存。
 * 实际可用容量为 size，内部数组大小为 size+1 (用于区分空满)。
 * 
 * @param name 结构体变量名
 * @param type 缓冲区元素类型 (如 uint8_t, char)
 * @param size 期望的逻辑容量
 * 
 * @example
 * RBUFFER_DEF(my_buf, uint8_t, 1024); // 定义一个名为 my_buf 的 1024 字节环形缓冲
 */
#define RBUFFER_DEF(name, type, size) \
    struct {\
        uint32  rpos, wpos, qsize;\
        type rbq[(size)+1];\
    }name

/**
 * @brief 静态定义环形缓冲区结构体 (引用外部数组模式)
 * 
 * 结构体中仅包含指向外部内存的指针，需配合 RB_INIT_R 初始化。
 * 
 * @param name 结构体变量名
 * @param type 缓冲区元素类型
 */
#define RBUFFER_DEF_R(name, type) \
    struct {\
        uint32  rpos, wpos, qsize;\
        type *rbq;\
    }name

/* ================= 单字节操作宏 (非中断安全) ================= */

/**
 * @brief 从缓冲区读取一个元素
 * 
 * 如果缓冲区非空，读取头部元素并移动读指针。
 * 
 * @param rb 缓冲区指针
 * @param val 输出变量，用于存储读取的值
 * @return uint8 1: 成功读取; 0: 缓冲区为空
 */
#define RB_GET(rb, val) ({\
        uint8 __ret__ = 0;\
        if(!RB_EMPTY(rb)){\
            val = (rb)->rbq[(rb)->rpos];\
            (rb)->rpos = RB_NPOS((rb), rpos, 1);\
            __ret__ = 1;\
        }\
        __ret__;\
    })

/**
 * @brief 从缓冲区读取一个元素 并 清空队列中的数据
 * 
 * 
 * @param rb  缓冲区指针
 * @param val 输出变量，用于存储读取的值
 * @return uint8 1: 成功读取; 0: 缓冲区为空
 */
#define RB_GET_CLEAN(rb, val) ({\
        uint8 __ret__ = 0;\
        if(!RB_EMPTY(rb)){\
            val = (rb)->rbq[(rb)->rpos];\
            (rb)->rbq[(rb)->rpos] = 0;  \
            (rb)->rpos = RB_NPOS((rb), rpos, 1);\
            __ret__ = 1;\
        }\
        __ret__;\
    })

/**
 * @brief 向缓冲区写入一个元素
 * 
 * 如果缓冲区未满，写入尾部元素并移动写指针。
 * 
 * @param rb 缓冲区指针
 * @param val 要写入的值
 * @return uint8 1: 成功写入; 0: 缓冲区已满
 */
#define RB_SET(rb, val) ({\
        uint8 __ret__ = 0;\
        if(!RB_FULL(rb)){\
            (rb)->rbq[(rb)->wpos] = val;\
            (rb)->wpos = RB_NPOS((rb), wpos, 1);\
            __ret__ = 1;\
        }\
        __ret__;\
    })

/**
 * @brief 强制向缓冲区写入一个元素 (覆盖模式)
 * 
 * 如果缓冲区已满，先移动读指针（丢弃最旧的一个数据），腾出空间后再写入。
 * 适用于允许丢包但要求实时性的场景。
 * 
 * @param rb 缓冲区指针
 * @param val 要写入的值
 * @return uint8 始终返回 1 (表示操作已完成)
 */
#define RB_SET_F(rb, val) ({\
        if(RB_FULL(rb)) (rb)->rpos = RB_NPOS((rb), rpos, 1);\
        (rb)->rbq[(rb)->wpos] = val;\
        (rb)->wpos = RB_NPOS((rb), wpos, 1);\
        1;\
    })

/* ================= 单字节操作宏 (中断安全) ================= */

/**
 * @brief 中断安全版本：从缓冲区读取一个元素
 * 
 * 通过关闭全局中断保证读写操作的原子性，适用于中断服务程序 (ISR) 与主循环之间的通信。
 * 
 * @param rb 缓冲区指针
 * @param val 输出变量
 * @return uint8 1: 成功; 0: 为空
 */
#define RB_INT_GET(rb, val) ({\
        uint8 __ret__ = 0;\
        uint32 flag = disable_irq(); \
        if(!RB_EMPTY(rb)){\
            val = (rb)->rbq[(rb)->rpos];\
            (rb)->rpos = RB_NPOS((rb), rpos, 1);\
            __ret__ = 1;\
        }\
        enable_irq(flag);\
        __ret__;\
    })

/**
 * @brief 中断安全版本：向缓冲区写入一个元素
 * 
 * @param rb 缓冲区指针
 * @param val 要写入的值
 * @return uint8 1: 成功; 0: 已满
 */
#define RB_INT_SET(rb, val) ({\
        uint8 __ret__ = 0;\
        uint32 flag = disable_irq(); \
        if(!RB_FULL(rb)){\
            (rb)->rbq[(rb)->wpos] = val;\
            (rb)->wpos = RB_NPOS((rb), wpos, 1);\
            __ret__ = 1;\
        }\
        enable_irq(flag);\
        __ret__;\
    })

/**
 * @brief 中断安全版本：强制写入一个元素
 * 
 * @param rb 缓冲区指针
 * @param val 要写入的值
 * @return uint8 始终返回 1
 */
#define RB_INT_SET_F(rb, val) ({\
        uint32 flag = disable_irq(); \
        if(RB_FULL(rb)) (rb)->rpos = RB_NPOS((rb), rpos, 1);\
        (rb)->rbq[(rb)->wpos] = val;\
        (rb)->wpos = RB_NPOS((rb), wpos, 1);\
        enable_irq(flag);\
        1;\
    })

/* ================= 初始化与重置宏 ================= */

/**
 * @brief 初始化静态定义的环形缓冲区 (内部数组模式)
 * 
 * @param rb 缓冲区指针
 * @param size 逻辑容量 (实际分配 size+1)
 */
#define RB_INIT(rb, size) do{\
        (rb)->qsize = (size)+1;\
        (rb)->rpos = 0;\
        (rb)->wpos = 0;\
    } while (0)

/**
 * @brief 初始化引用外部数组的环形缓冲区
 * 
 * @param rb 缓冲区指针
 * @param size 缓冲区实际大小 (注意：此处 size 即为 qsize，无需 +1，调用者需确保物理内存足够区分空满)
 * @param buff 外部内存缓冲区指针
 */
#define RB_INIT_R(rb, size, buff) do{\
        (rb)->qsize = (size);\
        (rb)->rpos = 0;\
        (rb)->wpos = 0;\
        (rb)->rbq  = buff;\
    } while (0)

/**
 * @brief 重置缓冲区 (清空数据)
 * 
 * 将读写指针归零。操作期间关闭中断以保证安全。
 * 
 * @param rb 缓冲区指针
 */
#define RB_RESET(rb) do{\
        uint32 flag = disable_irq(); \
        (rb)->rpos = 0;\
        (rb)->wpos = 0;\
        enable_irq(flag);\
    }while(0)


/**
 * @brief 动态环形缓冲区结构体
 * 
 * 用于需要在堆上分配内存的场景。
 */
struct rbuffer {
    uint32 rpos;      /**< 读指针索引 */
    uint32 wpos;      /**< 写指针索引 */
    uint32 qsize;     /**< 队列总大小 (物理分配大小) */
    char  *rbq;       /**< 数据缓冲区指针 */
};

/**
 * @brief 初始化动态环形缓冲区
 * 
 * @param rb 缓冲区结构体指针
 * @param size 期望的逻辑容量
 * @param buff 用户提供的内存缓冲区指针
 * @return int32 0: 成功; 负值: 失败
 */
int32 rbuffer_init(struct rbuffer *rb, uint32 size, void *buff);

/**
 * @brief 向缓冲区写入数据块
 * 
 * @param rb 缓冲区指针
 * @param data 源数据指针
 * @param length 数据长度
 * @return int32 实际写入的字节数
 */
int32 rbuffer_set(struct rbuffer *rb, void *data, uint32 length);

/**
 * @brief 强制向缓冲区写入数据块 (覆盖模式)
 * 
 * 如果空间不足，会覆盖旧数据以保证新数据写入。
 * 
 * @param rb 缓冲区指针
 * @param data 源数据指针
 * @param length 数据长度
 * @return int32 实际写入的字节数
 */
int32 rbuffer_set_force(struct rbuffer *rb, void *data, uint32 length);

/**
 * @brief 从缓冲区读取数据块
 * 
 * @param rb 缓冲区指针
 * @param buff 目标缓冲区指针
 * @param size 请求读取的最大长度
 * @return int32 实际读取的字节数
 */
int32 rbuffer_get(struct rbuffer *rb, void *buff, uint32 size);

/**
 * @brief 销毁动态缓冲区 (释放内存)
 * 
 * @param rb 缓冲区指针
 */
void  rbuffer_destroy(struct rbuffer *rb);

/**
 * @brief 重置动态缓冲区 (清空数据，不释放内存)
 * 
 * @param rb 缓冲区指针
 */
void  rbuffer_reset(struct rbuffer *rb);

/**
 * @brief 使用os_malloc 从heap自动分配缓冲区内存空间
 * 
 * @param rb 缓冲区结构体指针
 * @param size 期望容量
 * @return int32 0: 成功; 负值: 失败
 */
int32 rbuffer_alloc(struct rbuffer *rb, uint32 size);

/**
 * @brief 释放动态分配的内存，与 rbuffer_alloc 成对使用。
 * 
 * @param rb 缓冲区指针
 */
void  rbuffer_free(struct rbuffer *rb);

/**
 * @brief 计算移动后的索引位置
 * 
 * 辅助函数，用于在不改变指针的情况下预计算位置。
 * 
 * @param rb 缓冲区指针
 * @param cur 当前索引
 * @param offset 偏移量
 * @return int32 新的索引位置
 */
int32 rbuffer_move(struct rbuffer *rb, uint32 cur, uint32 offset);


/* ================= 硬件专用环形缓冲区 (HWRB) ================= */

////////////////////////////////////////////////////////////////
/**
 * @defgroup Hardware_Ring_Buffer 硬件模块专用环形缓冲区
 * 
 * 专为硬件 DMA 或 FIFO 设计的环形缓冲区实现。
 * - 默认支持 "单读单写" (1 Reader - 1 Writer) 模式，无锁高效运行。
 * - 若需支持 "多读多写"，可定义 @ref HWRB_IRQ 宏，启用关中断保护机制。
 */

/**
 * @brief 硬件环形缓冲区控制结构
 */
struct hwrbuffer {
    uint32 rpos;          /**< 读指针    */
    uint32 wpos;          /**< 写指针 */
    uint32 qsize;         /**< 总大小 */
    uint32 ipos;          /**< wpos被强制归0前的值 */
    uint32 min_size;      /**< 最小有效buffer长度，小于min_size的buffer区域会被跳过 */
    uint8 *buff;          /**< 底层数据缓冲区指针 */
};

/**
 * @brief 初始化硬件环形缓冲区
 * 
 * @param rb 缓冲区结构体指针
 * @param buff 底层内存缓冲区指针
 * @param size 底层内存总大小
 * @param min_size 最小有效buffer长度
 */
void hwrbuffer_init(struct hwrbuffer *rb, uint8 *buff, uint32 size, uint16 min_size);

/**
 * @brief 更新写指针并获取下一个可写块
 * 
 * 提交当前写入的数据，并准备下一个写入位置。
 * 
 * @param rb 缓冲区指针
 * @param buff [in/out] 输入：当前完成的 buffer 地址 (可选); 输出：下一个可写的 buffer 地址
 * @param size [in/out] 输入：本次提交的长度; 输出：下一个可写块的最大可用长度
 */
void hwrbuffer_update_wpos(struct hwrbuffer *rb, uint8 **buff, uint32 *size);

/**
 * @brief 更新读指针
 * 
 * 确认已读取的数据，释放缓冲区空间。
 * 
 * @param rb 缓冲区指针
 * @param buff [in/out] 已读取数据的 buffer 地址; 输出：下一个可读的 buffer 地址
 * @param size [in/out] 已读取数据的长度; 输出：下一个可读块的最大可用长度
 */
void hwrbuffer_update_rpos(struct hwrbuffer *rb, uint8 **buff, uint32 *size);

#ifdef __cplusplus
}
#endif

#endif /* __RBUFFER_H__ */
