// Virtual Display Device (VDD) Hardware Abstraction Layer (HAL)
// 这个头文件定义了虚拟显示设备的抽象接口，用于将帧缓冲区的内容
// 输出到一个非物理的、由软件定义的显示设备上。

#ifndef _HAL_VDD_H_
#define _HAL_VDD_H_

#include <stdint.h> // 引入标准整数类型
#include "lib/multimedia/framebuff.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- 错误码定义 ---
#define VDD_OK            0   // 操作成功
#define VDD_ERR_INVALID  -1   // 无效参数
#define VDD_ERR_IO       -2   // I/O 错误
#define VDD_ERR_NOMEM    -3   // 内存不足
#define VDD_ERR_NOTSUPP  -4   // 功能不支持
#define VDD_ERR_BUSY     -5   // 设备忙

// --- 数据结构定义 ---

/**
 * @brief 虚拟显示设备的几何信息。
 * 用于描述显示设备在虚拟空间中的位置和尺寸。
 */
struct vdd_rect  {
    uint32_t x;           // 虚拟空间中的 X 坐标
    uint32_t y;           // 虚拟空间中的 Y 坐标
    uint32_t z;           // 虚拟空间中的 Z 轴顺序（层级）
    uint32_t width;       // 显示区域的宽度（像素）
    uint32_t height;      // 显示区域的高度（像素）
};

/**
 * @brief 虚拟显示设备的能力描述。
 * 用于向调用者报告设备支持的显示模式等信息。
 */
struct vdd_caps {
    uint32_t max_width;   // 支持的最大宽度
    uint32_t max_height;  // 支持的最大高度
    uint32_t format_count;// 支持的像素格式数量
};

/**
 * @brief VDD 的 ioctl 命令枚举。
 * 定义了可以对虚拟显示设备执行的控制操作。
 */
enum vdd_ioctl_cmd {
    VDD_IOCTL_SET_LOCATION,   ///< 设置设备在虚拟空间中的位置 (vdd_rect )
    VDD_IOCTL_GET_LOCATION,   ///< 获取设备当前位置 (vdd_rect )
    VDD_IOCTL_CLEAR,          ///< 清空屏幕内容
    VDD_IOCTL_GET_CAPS,       ///< 获取设备能力 (vdd_caps)
    VDD_IOCTL_SET_MODE,       ///< 设置显示模式 (例如分辨率)
    VDD_IOCTL_MAX             ///< 命令数量，用于边界检查
};

/**
 * @brief 虚拟显示设备对象。
 * 这是设备的核心句柄，由驱动实现方进行定义和扩展。
 */
struct vdd_device {
    struct dev_obj dev;       // 继承自通用设备对象
    // 驱动实现方可以在此处添加私有数据成员
};

/**
 * @brief VDD 的硬件抽象层操作集。
 * 这个结构体定义了驱动实现方必须实现的一组回调函数。
 */
struct vdd_hal_ops {
    struct devobj_ops ops;    // 通用设备对象操作集

    /**
     * @brief 打开设备。
     * 在设备首次被使用时调用，用于初始化硬件或分配资源。
     * @param dev 指向 vdd_device 结构体的指针。
     * @return 成功返回一个上下文指针，失败返回 NULL。
     */
    void *(*open)(struct vdd_device *dev);

    /**
     * @brief 关闭设备。
     * 在设备不再被使用时调用，用于释放资源。
     * @param dev 指向 vdd_device 结构体的指针。
     * @param vdd open 函数返回的上下文指针。
     * @return 成功返回 VDD_OK，失败返回负的错误码。
     */
    int32_t (*close)(struct vdd_device *dev, void *vdd);

    /**
     * @brief 显示一帧图像。
     * 这是核心的显示函数，将帧缓冲区的内容提交给虚拟显示器。
     * @param dev 指向 vdd_device 结构体的指针。
     * @param fb 指向要显示的帧缓冲区结构体的指针。
     * @return 成功返回 VDD_OK，失败返回负的错误码。
     */
    int32_t (*display)(struct vdd_device *dev, void *vdd, struct framebuff *fb);

    /**
     * @brief 执行设备特定的控制命令。
     * 用于处理 ioctl 请求。
     * @param dev 指向 vdd_device 结构体的指针。
     * @param vdd open 函数返回的上下文指针。
     * @param cmd ioctl 命令。
     * @param param 命令的参数，通常是一个指针或值。
     * @return 成功返回 VDD_OK，失败返回负的错误码。
     */
    int32_t (*ioctl)(struct vdd_device *dev, void *vdd, enum vdd_ioctl_cmd cmd, uint32_t param);
};

/**
 * @brief 打开一个虚拟显示设备。
 * 这是上层应用或子系统调用的入口函数。
 * @param dev 指向要打开的 vdd_device 结构体的指针。
 * @return 成功返回一个设备句柄（上下文指针），失败返回 NULL。
 */
void *vdd_open(struct vdd_device *dev);

/**
 * @brief 关闭一个虚拟显示设备。
 * @param dev 指向 vdd_device 结构体的指针。
 * @param vdd vdd_open 返回的设备句柄。
 * @return 成功返回 VDD_OK，失败返回负的错误码。
 */
int32_t vdd_close(struct vdd_device *dev, void *vdd);

/**
 * @brief 向虚拟显示设备输出一帧图像。
 * @param dev 指向 vdd_device 结构体的指针。
 * @param fb 指向要显示的帧缓冲区结构体的指针。
 * @return 成功返回 VDD_OK，失败返回负的错误码。
 */
int32_t vdd_display(struct vdd_device *dev, void *vdd, struct framebuff *fb);

/**
 * @brief 对虚拟显示设备执行控制命令。
 * @param dev 指向 vdd_device 结构体的指针。
 * @param vdd vdd_open 返回的设备句柄。
 * @param cmd 要执行的 ioctl 命令。
 * @param param 命令的参数。
 * @return 成功返回 VDD_OK，失败返回负的错误码。
 */
int32_t vdd_ioctl(struct vdd_device *dev, void *vdd, enum vdd_ioctl_cmd cmd, uint32_t param);

#ifdef __cplusplus
}
#endif

#endif // _HAL_VDD_H_