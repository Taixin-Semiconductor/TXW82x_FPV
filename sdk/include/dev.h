#ifndef _HUGEIC_DEV_H_
#define _HUGEIC_DEV_H_

#include "osal/atomic.h"           // 引入原子操作支持

#ifdef __cplusplus
extern "C" {                       // 兼容C++编译器
#endif

// 前向声明设备对象结构体
struct dev_obj;

// 设备类型枚举
enum DEV_TYPE{
    DEV_TYPE_NONE,    // 无类型
    DEV_TYPE_SD,      // SD卡设备
    DEV_TYPE_UDISK,   // U盘设备
    DEV_TYPE_FDISK,   // Flash磁盘设备
    DEV_TYPE_UVC,     // USB视频类设备
    DEV_TYPE_SDIO,    // SDIO接口设备
    DEV_TYPE_MIC,     // 麦克风设备
    DEV_TYPE_DAC,     // 数字模拟转换器设备
};

// 设备对象操作函数表
struct devobj_ops{
#ifdef CONFIG_SLEEP               // 如果使能休眠支持
    int32 (*suspend)(struct dev_obj *obj);   // 设备挂起回调
    int32 (*resume)(struct dev_obj *obj);    // 设备恢复回调
#endif
};

#ifndef OS_BLKLIST
#define OS_BLKLIST
// 阻塞列表结构，用于等待队列
struct os_blklist{
    void *hdl;                    // 句柄指针
};
typedef struct os_blklist os_blklist_t;
#endif

// 设备对象结构体
struct dev_obj{
    struct dev_obj *next;         // 链表指针，指向下一个设备
    uint16 dev_id;                // 设备ID
    uint16 dev_type;              // 设备类型，取值见DEV_TYPE枚举
    uint8  busy;                  /* 设备占用状态，由驱动代码设置。
                                      共有8个bit位可用，驱动可根据读/写等操作设置不同的bit位 */
    uint8  suspend: 1,            /* 设备是否进入挂起状态，由dev_suspend API设置 */
           hotplug: 1,            /* 是否为热插拔设备 */
           r1: 6;                 // 保留位，共6bit
    atomic8_t ref;                /* 热插拔设备的引用计数，原子操作 */
    uint8  r2;                    // 保留字节
#ifdef CONFIG_SLEEP
    struct os_blklist blklist;    // 用于挂起时的阻塞列表
#endif
    const struct devobj_ops *ops; // 设备操作函数表指针
    void  *info;                  // 设备信息指针，指向具体设备的私有数据
};

/* 设备遍历回调函数原型
 * 返回值含义：
 *    0 : 继续遍历下一个设备
 *    1 : 结束遍历
 *   -1 : 结束遍历，并且该设备已被外部引用（不再释放）。ref计数会自动加1，不再使用该设备时需要执行dev_put
 */
typedef int32 (*dev_walkcb)(const struct dev_obj *dev, void *arg);

// 遍历指定类型的所有设备，对每个设备调用回调函数cb
// 返回值：0:未匹配，1:成功匹配，-1:成功匹配且被引用
int32 dev_walk(uint16 type, dev_walkcb cb, void *arg);

//热插拔设备资源管理
struct dev_hotplug_info{
    void (*release)(struct dev_hotplug_info *info); //热插拔设备资源释放
    void *priv;
};

// 热插拔设备插入：注册新设备，通知系统
int32 dev_hotplug_in(uint16 dev_id, uint16 dev_type, struct dev_hotplug_info *info);
// 热插拔设备拔出：注销设备，释放资源
int32 dev_hotplug_out(uint16 dev_id);

// 宏：设置或清除设备的busy标志位
#define DEV_BUSY(dev, val, set)      dev_busy((struct dev_obj *)(dev), val, set)
// 宏：检查/设置设备的挂起状态
#define DEV_SUSPENDED(dev, suspend)  dev_suspended((struct dev_obj *)(dev), suspend)
// 宏：如果设备已挂起则直接返回错误（用于驱动函数入口检查）
#define HALDEV_SUSPENDED(dev)        if(dev_suspended((struct dev_obj *)dev, 1)) return RET_ERR

/**
 * 设备核心模块初始化
 * @return 初始化成功返回RET_OK，失败返回RET_ERR
 */
extern int32 dev_init(void);

/**
 * 根据设备ID获取设备对象
 * @param[in] dev_id 设备ID
 * @return 设备存在时返回设备对象指针，否则返回NULL
 */
extern struct dev_obj *dev_get(uint16 dev_id);

// 宏：减少设备的引用计数
#define DEV_PUT(dev) dev_put((struct dev_obj *)(dev))
// 减少设备引用计数，当计数为0且设备为热插拔时可能释放资源
extern void dev_put(struct dev_obj *dev);

/*
 * 设置或清除设备的忙标志位
 * @param dev   设备对象指针
 * @param busy  要设置的bit位掩码
 * @param set   1表示设置，0表示清除
 */
extern void dev_busy(struct dev_obj *dev, uint8 busy, uint8 set);

/**
 * 注册设备到设备管理器
 * @param[in] dev_id 设备ID
 * @param[in] device 要注册的设备对象指针
 * @return 注册成功返回RET_OK，失败返回RET_ERR
 */
extern int32 dev_register(uint16 dev_id, struct dev_obj *device);

/**
 * 从设备管理器中注销设备
 * @param[in] device 要注销的设备对象指针
 * @return 注销成功返回RET_OK，失败返回RET_ERR
 */
extern int32 dev_unregister(struct dev_obj *device);

/* 检查指定的设备是否已经被挂起
 * @param dev      设备对象指针
 * @param suspend  如果设备已被挂起，是否挂起当前任务（阻塞等待）
 * @return 设备已挂起返回非0（或特定值），否则返回0
 */
int32 dev_suspended(struct dev_obj *dev, uint8 suspend);

/**
 * 系统进入休眠时挂起指定类型的所有设备
 * @param[in] type 设备类型（DEV_TYPE_xxx），若为0则挂起所有设备
 * @return 操作成功返回RET_OK，否则返回RET_ERR
 */
extern int32 dev_suspend(uint16 type);

// 挂起单个设备（供内部或钩子调用）
extern int32 dev_suspend_hook(struct dev_obj *dev, uint16 type);

/**
 * 系统唤醒时恢复指定类型的所有设备
 * @param[in] type      设备类型
 * @param[in] wkreason  唤醒原因（如中断源等）
 * @return 操作成功返回RET_OK，否则返回RET_ERR
 */
extern int32 dev_resume(uint16 type, uint32 wkreason);

// 恢复单个设备（供内部或钩子调用）
extern int32 dev_resume_hook(struct dev_obj *dev, uint16 type, uint32 wkreason);

/**
 * 设置设备的引脚功能（复用）
 * @param dev_id 设备ID
 * @param request 请求的引脚功能配置
 * @return 成功返回0，失败返回负错误码
 */
extern int32 pin_func(uint16 dev_id, int32 request);

#ifdef __cplusplus
}
#endif

#endif /* _HUGEIC_DEV_H_ */
