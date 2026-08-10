/****************************************************************************************
 * @file    vcodec.h
 * @brief   视频编解码硬件抽象层（Video Codec HAL）接口定义
 * @details 提供统一的视频编解码驱动接口，屏蔽底层硬件差异，支持同步/异步编解码、
 *          外部/内部输出缓冲区管理、中断回调、动态参数控制等功能
 * @note    1. 输出Buffer由调用者管理或驱动内部管理（通过outbuf是否为NULL判断）
 *          2. 异步编解码通过done回调函数通知处理完成
 ***************************************************************************************/

#ifndef _HAL_VCODEC_H_
#define _HAL_VCODEC_H_

#include "lib/multimedia/framebuff.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef vcodec_irq_hdl
 * @brief  视频编解码中断回调函数原型定义
 * @param  irq       中断号，标识具体中断类型
 * @param  irq_data  中断携带的硬件数据/状态
 * @param  param1    用户自定义参数1，由注册时传入
 * @param  param2    用户自定义参数2，由注册时传入
 * @return 0  中断处理完成
 *         <0 中断处理失败，返回错误码
 */
typedef int32(*vcodec_irq_hdl)(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2);

/**
 * @defgroup VCODEC_DECODE_FLAG 视频解码控制标志位（位掩码）
 * @brief  用于struct vcodec_decode_info.flags，控制解码行为
 * @{
 */
#define VDEC_FLAG_NONE          0x00    /**< 无特殊控制标志，默认解码模式 */
#define VDEC_FLAG_KEYFRAME      0x01    /**< 解码：强制请求输出关键帧，用于Seek/跳转场景 */
#define VDEC_FLAG_DISCARD       0x02    /**< 解码：提示当前帧可丢弃，用于快进/网络拥塞场景 */
#define VDEC_FLAG_EOS           0x08    /**< 通用：码流结束标志，用于刷新驱动内部缓冲区 */
#define VDEC_FLAG_FLUSH         0x10    /**< 解码：清空解码器内部状态机，用于Seek后重置 */
#define VDEC_FLAG_LOW_DELAY     0x20    /**< 解码：低延迟模式，优先输出图像，忽略B帧优化 */
/** @} */

/**
 * @struct vcodec_decode_info
 * @brief  视频解码动态控制参数与输出结果结构体
 * @note   输入参数由调用者配置，输出参数由驱动填充
 */
struct vcodec_decode_info {
    uint32 actual_width;        /**< [输出] 当前帧解码后的实际宽度（可能因裁剪Crop变化） */
    uint32 actual_height;       /**< [输出] 当前帧解码后的实际高度 */
    uint32 frame_duration;      /**< [输出] 当前帧显示时长，单位：微秒(us)，用于音视频同步 */
    uint8 *y_off;               /**< [输出] Y起始地址 */
    uint8 *u_off;               /**< [输出] U起始地址 */
    uint8 *v_off;               /**< [输出] V起始地址 */
    uint32 target_width;        /**< [输入]  请求缩放目标宽度，0表示使用原始尺寸 */
    uint32 target_height;       /**< [输入]  请求缩放目标高度，0表示使用原始尺寸 */
    uint8  color_space;         /**< [输出] 实际解析的色彩空间类型（ITU-R BT.601/BT.709等） */
    uint8  flags;               /**< [输入]  解码控制标志，取值：VDEC_FLAG_* 位掩码组合 */
    uint8  _pad[2];             /**< 结构体字节对齐填充，保证内存布局规整 */
};

/**
 * @struct vcodec_decode_req
 * @brief  视频解码任务请求结构体
 * @note   编解码核心请求载体，支持同步/异步处理、外部/内部缓冲区
 */
struct vcodec_decode_req {
    void *chan;                     /**< 解码器通道句柄 */
    void *priv;                     /**< [用户] 调用者私有上下文指针，回调时原样返回 */
    struct vcodec_decode_info info; /**< [核心] 解码控制参数与结果信息 */
    struct framebuff *fb;           /**< [输入]   压缩视频码流缓冲区（H.264/H.265 Annex B/AVCC格式） */
    struct framebuff *fb_out;       /**< [输出] 解码输出数据缓冲区 */

    /**
     * @brief  异步解码完成回调函数
     * @param  req     指向当前解码请求结构体的指针
     * @param  status  处理状态：0=成功，<0=失败，>0=格式变化等提示信息
     * @note   同步模式下该回调不会被调用
     */
    void (*done)(struct vcodec_decode_req *req, int32 status);
};

/**
 * @struct vcodec_encode_info
 * @brief  视频编码动态控制参数与输出结果结构体
 * @note   输入参数由调用者配置，输出参数由驱动填充
 */
struct vcodec_encode_info {
    int32 target_bitrate;     /**< [输入] 编码目标码率，单位：bps（比特每秒） */
    uint16 target_width;      /**< [输入] 编码目标宽度，0表示使用原始尺寸 */
    uint16 target_height;     /**< [输入] 编码目标高度，0表示使用原始尺寸 */
    int8  quality;            /**< [输入] 编码质量因子，范围：-128 ~ 127，数值越大质量越高 */
    uint8 flags : 4;          /**< [输入] 编码帧控制标志（低4位） */
    uint8 gop_size : 4;       /**< [输入] 关键帧间隔（高4位），0表示保持当前配置不变 */
    uint8 is_keyframe : 1;    /**< [输出] 编码结果：1=生成I帧（关键帧），0=非关键帧 */
    uint8 rev : 7;            /**< 保留位，用于字节对齐，未使用 */
    uint16 _pad;              /**< 结构体字节对齐填充 */
};

/**
 * @struct vcodec_encode_req
 * @brief  视频编码任务请求结构体
 * @note   编码核心请求载体，与解码请求设计保持一致
 */
struct vcodec_encode_req {
    void *chan;                     /**< 编码器通道句柄 */
    void *priv;                     /**< [用户] 调用者私有上下文指针，回调时原样返回 */
    struct vcodec_encode_info info; /**< [核心] 编码控制参数与结果信息 */
    struct framebuff *fb;           /**< [输入] 原始图像数据缓冲区（YUV/RGB格式） */
    struct framebuff *fb_out;       /**< [输出] 编码输出数据缓冲区

    /**
     * @brief  异步编码完成回调函数
     * @param  req     指向当前编码请求结构体的指针
     * @param  status  处理状态：0=成功，<0=失败
     * @note   同步模式下该回调不会被调用
     */
    void (*done)(struct vcodec_encode_req *req, int32 status);
};

/**
 * @enum  vcodec_ioctl_cmd
 * @brief 视频编解码设备控制命令字
 * @note  用于vcodec_ioctl接口，实现设备动态配置与状态查询
 */
enum vcodec_ioctl_cmd {
    VDEC_IOCTL_FLUSH,           /**< 清空解码器内部任务队列与缓存 */
    VDEC_IOCTL_GET_DELAY,       /**< 获取解码延迟，单位：毫秒(ms) */
    VDEC_IOCTL_SET_SPEED,       /**< 设置视频播放倍速，param1：100=1倍速，200=2倍速 */
    VDEC_IOCTL_GET_PTS,         /**< 获取当前帧的PTS（显示时间戳） */
    VDEC_IOCTL_SET_CROP,        /**< 设置图像裁剪区域，param1/param2传入裁剪坐标与尺寸 */
    VDEC_IOCTL_GET_INFO,        /**< 获取视频码流详细信息（txVideoInfo_t类型） */
    VDEC_IOCTL_PAUSE,           /**< 暂停编解码设备，等待下一帧唤醒 */
    VDEC_IOCTL_RESUME,          /**< 恢复暂停的编解码设备 */
    VDEC_IOCTL_CANCLE,          /**< 取消编解码动作 */
    VDEC_IOCTL_RELEASE_FBDATA,  /**< 释放解码输出的fb->data资源 */
};

/**
 * @struct vcodec_device
 * @brief  视频编解码设备对象
 * @note   继承系统通用设备模型，绑定硬件操作接口
 */
struct vcodec_device {
    struct dev_obj dev;               /**< 继承系统通用设备基础对象 */
};

/**
 * @struct vcodec_hal_ops
 * @brief  视频编解码HAL层硬件操作接口表
 * @note   底层驱动开发者必须实现该结构体所有函数指针
 */
struct vcodec_hal_ops {
    struct devobj_ops ops;      /**< 继承设备基础操作集（open/close/read/write等） */

    /**
     * @brief  硬件层：打开视频编解码设备
     * @param  dev   视频设备对象指针
     * @return void *  打开成功返回通道句柄
     *                 打开失败，返回NULL
     */
    void *(*open)(struct vcodec_device *dev);

    /**
     * @brief  硬件层：关闭视频编解码设备
     * @param  dev  视频设备对象指针
     * @param  chan 通道句柄
     * @return 0  关闭成功
     *         <0 关闭失败，返回错误码
     */
    int32(*close)(struct vcodec_device *dev, void *chan);

    /**
     * @brief  硬件层：提交视频解码任务
     * @param  dev 视频设备对象指针
     * @param  req 解码任务请求结构体指针
     * @return 0           同步解码成功（done回调不执行）
     *         -EINPROGRESS 异步解码已提交（通过done回调通知结果）
     *         -EAGAIN     设备忙/队列满，需重试
     *         <0          提交失败，返回错误码
     */
    int32(*decode)(struct vcodec_device *dev, struct vcodec_decode_req *req);

    /**
     * @brief  硬件层：提交视频编码任务
     * @param  dev 视频设备对象指针
     * @param  req 编码任务请求结构体指针
     * @return 0           同步编码成功（done回调不执行）
     *         -EINPROGRESS 异步编码已提交（通过done回调通知结果）
     *         -EAGAIN     设备忙/队列满，需重试
     *         <0          提交失败，返回错误码
     */
    int32(*encode)(struct vcodec_device *dev, struct vcodec_encode_req *req);

    /**
     * @brief  硬件层：设备控制接口
     * @param  dev    视频设备对象指针
     * @param  cmd    控制命令字，取值：vcodec_ioctl_cmd
     * @param  param1 命令参数1
     * @param  param2 命令参数2
     * @return 0  控制成功
     *         <0 控制失败，返回错误码
     */
    int32(*ioctl)(struct vcodec_device *dev, void *chan, enum vcodec_ioctl_cmd cmd, uint32 param);

    /**
     * @brief  硬件层：注册编解码中断回调
     * @param  dev    视频设备对象指针
     * @param  irq_id 中断ID
     * @param  hdl    中断回调函数
     * @param  data   中断回调私有数据
     * @return 0  注册成功
     *         <0 注册失败，返回错误码
     */
    int32(*request_irq)(struct vcodec_device *dev, uint32 irq_id, vcodec_irq_hdl hdl, void *data);

    /**
     * @brief  硬件层：注销编解码中断回调
     * @param  dev    视频设备对象指针
     * @param  irq_id 中断ID
     * @return 0  注销成功
     *         <0 注销失败，返回错误码
     */
    int32(*release_irq)(struct vcodec_device *dev, uint32 irq_id);
};

/**
 * @brief  上层接口：打开视频编解码器通道
 * @param  dev  视频设备对象指针
 * @return 成功：返回编解码通道句柄
 *         失败：返回NULL，可通过get_errno()获取错误码
 */
void *vcodec_open(struct vcodec_device *dev);

/**
 * @brief  上层接口：关闭视频编解码器通道
 * @param  dev  视频设备对象指针
 * @param  chan 编解码通道句柄
 * @return 0  关闭成功
 *         <0 关闭失败，返回错误码
 */
int32 vcodec_close(struct vcodec_device *dev, void *chan);

/**
 * @brief  上层接口：提交视频解码请求
 * @param  dev   视频设备对象指针
 * @param  chan  编解码通道句柄
 * @param  req   解码任务请求
 * @return 0     同步解码成功（done回调不执行）
 *         -EINPROGRESS 异步解码已提交（通过done回调通知结果）
 *         -EAGAIN     设备忙/队列满，需重试
 *         <0          提交失败，返回错误码 
 * @note   线程安全，支持多线程并发调用
 */
int32 vcodec_decode(struct vcodec_device *dev, struct vcodec_decode_req *req);

/**
 * @brief  上层接口：提交视频编码请求
 * @param  dev   视频设备对象指针
 * @param  chan  编解码通道句柄
 * @param  req   编码任务请求
 * @return 0           同步编码成功（done回调不执行）
 *         -EINPROGRESS 异步编码已提交（通过done回调通知结果）
 *         -EAGAIN     设备忙/队列满，需重试
 *         <0          提交失败，返回错误码
 * @note   线程安全，支持多线程并发调用
 */
int32 vcodec_encode(struct vcodec_device *dev, struct vcodec_encode_req *req);

/**
 * @brief  上层接口：执行编解码设备控制命令
 * @param  dev   视频设备对象指针
 * @param  chan  编解码通道句柄
 * @param  cmd   控制命令字
 * @param  param 命令参数
 * @return 0  控制成功
 *         <0 控制失败，返回错误码
 */
int32 vcodec_ioctl(struct vcodec_device *dev, void *chan, enum vcodec_ioctl_cmd cmd, uint32 param);

#ifdef __cplusplus
}
#endif

#endif /* _HAL_VCODEC_H_ */
