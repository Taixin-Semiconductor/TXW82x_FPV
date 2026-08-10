/**
 * @file txsemi_msi.h
 * @brief 媒体流接口 (MSI) 核心定义
 *
 * @copyright (c) TXsemi Inc. All rights reserved.
 * @author TXsemi Multimedia Team
 *
 * @section msi_architecture MSI 架构概述
 * MSI 是一套基于链表结构的媒体流处理框架。它通过将多媒体处理单元（如解码器、渲染器）
 * 封装为 `struct msi` 对象，并利用 `msi_output_fb` 接口在对象间传递 `framebuff` 数据，
 * 实现了灵活的音视频数据流处理。
 *
 * @section core_features 核心特性
 * - **插件化管理**：通过 msi_new/msi_destroy 进行组件生命周期管理。
 * - **链式调用**：通过 msi_cmd/msi_output_cmd 实现命令在组件链路上的广播。
 * - **静态组件**：名称固定，仅存在一个实例化对象。
 * - **动态组件**：名称不固定，存在多个实例化对象。使用msi_find2 API会自动创建实例化对象，使用msi_put可以销毁对象。
 */

#ifndef _TXSEMI_MSI_H_
#define _TXSEMI_MSI_H_
#include "framebuff.h"
#include "msi_names.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup MSI_Config_Constants 配置常量
 * @brief 框架基础配置参数
 * @{
 */
#ifndef MSI_OUTIF_MAX
    #define MSI_OUTIF_MAX 4 ///< 最大输出接口数量限制
#endif
/** @} */

/**
 * @defgroup MSI_Command_Enums 命令枚举
 * @brief 定义了组件间通信及控制的标准命令集
 * @{
 */

/**
 * @brief MSI 核心命令集
 * @details 这些命令用于控制组件的状态机、数据流和属性设置。
 *          可以通过 msi_cmd/msi_cmd2/msi_do_cmd/msi_output_cmd 执行这些命令。
 */
enum MSI_CMDs {
    /* 生命周期管理 (内部) */
    MSI_CMD_PRE_DESTROY,      ///< [内部] 通知组件即将开始销毁流程.
    MSI_CMD_POST_DESTROY,     ///< [内部] MSI对象即将被释放，用于释放私有资源.
    MSI_CMD_NEW_CHANNEL,      ///< [内部] 获取多通道MSI组件的新实例化对象.

    /* 状态控制 */
    MSI_CMD_START,            ///< 启动组件，切换到运行状态 (默认状态).
    MSI_CMD_STOP,             ///< 停止组件，需清除缓存数据并复位状态.
    MSI_CMD_PAUSE,            ///< 暂停组件，保持当前数据不丢失，停止处理.
    MSI_CMD_PLAY_END,         ///< 通知上游组件媒体流已播放完成.

    /* 音频控制 */
    MSI_CMD_SET_VOLUME,       ///< 设置媒体流的数字音量 (支持不同流独立音量). (param1:volume, param2:0)
    MSI_CMD_GET_VOLUME,       ///< 获取媒体流的数字音量. (param1:uint8 *, param2:0)
    MSI_CMD_SET_DAC_VOLUME,   ///< 设置DAC音量 (全局硬件音量). (param1:volume, param2:0)
    MSI_CMD_SET_SPEED,        ///< 设置播放倍速 (param1: 速度值，例如 100 表示 1.0x). (param1:speed, param2:0)
    MSI_CMD_GET_DAC_BUFTIME,  ///< 获取AUDIO DAC缓冲数据的时间上限. (param1:uint32*, param2:0)
    MSI_CMD_SET_BITRATE,

    /* 同步与状态反馈 */
    MSI_CMD_AVSYNC_MASTER,    ///< 设置 AVSync 的主参考组件. (param1:参考组件 - struct msi*, param2:0)
    MSI_CMD_GET_PLAYTIME,     ///< 从AVSync参考组件获取当前播放时间. (param1:play time - uint32 *, param2:0)
    MSI_CMD_EOF,              ///< 数据流已结束. (param1: 1-EOF，0: Not EOF) (param1:[1/0], param2:0)
    MSI_CMD_NEED_MORE_DATA,   ///< 通知上游需要更多数据输入 (用于缓冲控制). (param1:0, param2:0)
    MSI_CMD_DECODE_ERR,       ///< 通知上游数据解码失败，需要停止加载数据.
    MSI_CMD_CLEAR,            ///< 清空数据.

    MSI_CMD_FREE_FB,          ///< 通知组件即将释放某个framebuff. (param1:fb, param2:0)
    MSI_CMD_FREE_FB_END,      ///< 通知组件已完成释放1个framebuff. (param1:0, param2:0)
    MSI_CMD_TRANS_FB,         ///< 通知MSI组件有新的framebuff需要接收. (param1:fb, param2:0)
    MSI_CMD_TRANS_FB_END,     ///< 通知MSI组件新的framebuff已存入接收队列. (param1:fb, param2:0)

    MSI_CMD_SET_MIXER_MODE,   ///< 设置混音模式。(param1:混音模式, param2:压低音量大小)
    MSI_CMD_SET_PLAYER_PARAM, ///< 设置TXMplayer参数。(param1:struct txmplayer_param*, param2:0).

    MSI_CMD_FOLLOW_OUTPUT,    ///< 设置MSI组件输出跟随指定的MSI组件。用于切换音轨时，新的音轨数据跟随旧的音轨数据输出，不能同时输出.
    MSI_CMD_GET_RUNNING,      ///< [反馈] 查询MSI是否正在运行.

    MSI_CMD_SET_VDD_RECT,     ///< 设置VDD(Virtual Display Device)的尺寸信息. (param1: struct vdd_rect *, param2:0)
    MSI_CMD_SET_VDD_ONOFF,    ///< 控制VDD(Virtual Display Device)的开/关. (param1:[1/0], param2:0)
    MSI_CMD_GET_VDD_RECT,     ///< 获取VDD(Virtual Display Device)的尺寸信息. (param1: struct vdd_rect *, param2:0)

    /* 调试与维护 */
    MSI_CMD_DUMP,                 ///< [调试] Dump MSI内部私有状态信息.
    MSI_CMD_GET_AUDIO_PLAYTIME,   ///< [调试] 获取Audio数据的播放时间.
    MSI_CMD_GET_VIDEO_PLAYTIME,   ///< [调试] 获取Video数据的播放时间.

    /* 扩展功能 (图像/视频/编解码) */
    MSI_CMD_SET_DATATAG,
    MSI_CMD_OSD_ENCODE,
    MSI_CMD_SCALE1,
    MSI_CMD_SCALE2,
    MSI_CMD_SCALE3,
    MSI_CMD_TAKEPHOTO_SCALE3,
    MSI_CMD_SCALE3_NORMAL,
    MSI_CMD_JPEG,
    MSI_CMD_DECODE_JPEG_MSG,
    MSI_CMD_JPEG_CONCAT,
    MSI_CMD_HARDWARE_JPEG,
    MSI_CMD_AUTO_JPG,
    MSI_CMD_JPG_RECODE,
    MSI_CMD_JPG_THUMB,
    MSI_CMD_LCD_VIDEO,
    MSI_CMD_DECODE,
    MSI_CMD_PLAYER,
    MSI_CMD_VIDEO_DEMUX_CTRL,
    MSI_CMD_MEDIA_PLAYER,
    MSI_CMD_MEDIA_CTRL,
    MSI_CMD_CSC,
    MSI_CMD_LCD_OSD,
};

/**
 * @defgroup MSI_Data_Structures 数据结构
 * @brief MSI 核心数据类型定义
 * @{
 */

/* 前向声明 */
struct msi;
struct framebuff;

/**
 * @brief MSI 组件命令处理回调函数原型
 * @param msi   组件实例指针
 * @param cmd_id 命令字 ID
 * @param param1 命令参数 1 (参数具体含义和cmd_id相关联)
 * @param param2 命令参数 2 (参数具体含义和cmd_id相关联)
 * @return int   0 表示成功，负数表示错误码
 */
typedef int (*msi_action)(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2);

/**
 * @brief 媒体流组件 (MSI) 核心结构体
 * @details 这是所有多媒体处理模块的基类。通过将不同的处理单元
 *           封装为此结构体，实现统一的数据流管理和命令分发。
 */
struct msi {
    struct msi *next;             ///< 全局组件链表的下一个节点
    const char *name;             ///< 组件唯一标识名称 (字符串常量)
    msi_action action;            ///< 命令处理回调函数
    struct msi *output_list[MSI_OUTIF_MAX]; ///< 输出组件列表 (下游节点，最多 MSI_OUTIF_MAX 个)
    void *priv;                   ///< 私有数据指针，供具体实现存储上下文

    uint16 type;                  ///< 组件支持的媒体数据类型（mtype<<8|stype) (用于自动过滤。默认值是0xffff)
    uint8 enable   : 1;           ///< 使能标志 (1: 启用, 0: 禁用)
    uint8 chan_mgr : 1;           ///< 多通道管理模块标志 (1: 多通道管理组件)
    uint8 list_head: 1;           ///< 标记msi是否被加入到管理链表
    uint8 delete   : 1;           ///< 标记msi是否需要被删除。在MSI_CMD_PRE_DESTROY无法执行销毁动作时设置此标识。
    uint8 rev      : 4;           ///< 保留位
    uint8 alloc_fail;             ///< [调试] 记录alloc fb 失败的次数。受fb_limits影响

    atomic_t   users;             ///< 引用计数器 (外部持有引用时增加)
    atomic16_t inited;            ///< 初始化计数器 (msi_new 增加，msi_destroy 减少)
    atomic16_t fb_limits;         ///< Framebuff 分配数量限制。用于控制msi组件不能消耗过多的内存资源。
    
    struct fbqueue fbQ;           ///< 输入数据队列，缓存上游传来的 framebuff
    
    struct msi *mgr;              ///< 多通道实例化组件的管理组件指针
    malloc_cb_t fb_alloc;         ///< 自定义 Framebuff 分配接口
    mfree_cb_t  fb_free;          ///< 自定义 Framebuff 释放接口
};
/** @} */ // End of MSI_Data_Structures


/**
 * @brief 初始化 MSI 核心库
 * @return int32 0 表示成功，负数表示错误码
 * @note 必须在调用任何其他 MSI 函数前执行。
 */
extern int32 msi_core_init(void);

/**
 * @brief 建立组件间的数据通路 (连接)
 * @details 将源组件 的输出连接到目标组件 (Output)。
 * @param s_msi  [in, optional] 源组件指针。若为 NULL，需提供 s_name。
 * @param s_name [in, optional] 源组件名称。若为 NULL，需提供 s_msi。
 * @param o_msi  [in, optional] 目标组件指针。若为 NULL，需提供 o_name。
 * @param o_name [in, optional] 目标组件名称。若为 NULL，需提供 o_msi。
 * @return int32 0 表示成功，负数表示错误码。
 * @note 参数规则：源和目标都必须至少提供指针或名称中的一个。
 */
extern int32 msi_add_output(struct msi *s_msi, const char *s_name, struct msi *o_msi, const char *o_name);

/**
 * @brief 断开组件间的数据通路 (断开)
 * @details 支持灵活的批量删除操作。
 * @param s_msi  [in, optional] 源组件指针。
 * @param s_name [in, optional] 源组件名称。
 * @param o_msi  [in, optional] 目标组件指针。
 * @param o_name [in, optional] 目标组件名称。
 * @return int32 0 表示成功，负数表示错误码。
 * @note 特殊行为：
 *       - 若 s_msi 和 s_name 均为 NULL：删除所有指向目标 (o_msi/o_name) 的连接。
 *       - 若 o_msi 和 o_name 均为 NULL：删除源 (s_msi/s_name) 的所有输出连接。
 */
extern int32 msi_del_output(struct msi *s_msi, const char *s_name, struct msi *o_msi, const char *o_name);

/**
 * @brief 向指定名称的组件发送命令 (广播)
 * @details 命令会从匹配名称的组件开始，依次向下游传递。
 *          所有组件接收cmd后根据自身的实现决定是否执行cmd。
 * @param name   目标组件名称
 * @param cmd    命令字 (参考 @ref MSI_CMDs)
 * @param param1 参数 1
 * @param param2 参数 2
 */
extern void msi_cmd(const char *name, uint32 cmd, uint32 param1, uint32 param2);

/**
 * @brief 向指定组件对象发送命令 (广播)
 * @details 命令会从指定的组件对象开始，依次向下游传递。
 *          所有组件接收cmd后根据自身的实现决定是否执行cmd。
 * @param msi    目标组件指针
 * @param cmd    命令字
 * @param param1 参数 1
 * @param param2 参数 2
 */
extern void msi_cmd2(struct msi *msi, uint32 cmd, uint32 param1, uint32 param2);

/**
 * @brief 清空组件的输入队列
 * @param msi 目标组件指针
 * @note 会丢弃队列中所有未处理的 framebuff (调用 fb_put)。
 */
extern void msi_clear(struct msi *msi);

/**
 * @brief 创建并注册新组件
 * @param name   组件唯一名称
 * @param qsize  输入队列深度
 * @param isnew  [out, optional] 输出参数。返回 1 表示新创建，0 表示复用已存在组件。
 * @return struct msi* 组件指针，失败返回 NULL。
 * @note 同一个组件可以被多次调用msi_new，但必须与 @ref msi_destroy 成对调用才能真正销毁。
 */
extern struct msi *msi_new(const char *name, uint32 qsize, uint8 *isnew);

/**
 * @brief 销毁并注销组件
 * @param msi 待销毁组件指针
 * @note 会等待引用计数归零后真正释放资源。
 */
extern void msi_destroy(struct msi *msi);

/**
 * @brief 增加组件引用计数
 * @param msi 组件指针
 * @return struct msi* 返回原指针
 * @note 持有指针期间必须调用，防止组件被意外销毁。
 */
extern struct msi *msi_get(struct msi *msi);

/**
 * @brief 减少组件引用计数。
 *        执行 msi_get/msi_find/msi_find2 API后，需要执行msi_put释放对msi对象的引用。
 * @param msi 组件指针
 * @note 引用归零时可能触发销毁流程。
 */
extern void msi_put(struct msi *msi);

/**
 * @brief 按名称查找组件 (已废弃)
 * @param name   组件名称
 * @param inited 查找条件 (1: 仅查找已初始化, 0: 查找所有)
 * @return struct msi* 找到返回指针 (引用计数+1)，否则 NULL。
 * @note 建议使用 msi_find2 API 替代。
 */
extern struct msi *msi_find(const char *name, uint8 inited);

/**
 * @brief 按名称或类型查找组件 (推荐)
 * @details 若查找的MSI为多通道组件，则自动创建一个新的实例化通道MSI对象，执行msi_put可以销毁自动创建的msi对象。
 * @param name   组件名称
 * @param type   组件类型
 * @param inited 查找条件
 * @param arg    自定义参数
 * @return struct msi* 找到返回指针 (引用计数+1)，否则 NULL。
 */
extern struct msi *msi_find2(const char *name, uint16 type, uint8 inited, void *arg);

/**
 * @brief 向所有下游组件广播命令（自己不执行）
 * @param msi    源组件
 * @param cmd    命令字
 * @param param1 参数 1
 * @param param2 参数 2
 * @return int32 执行结果
 */
extern int32 msi_output_cmd(struct msi *msi, uint32 cmd, uint32 param1, uint32 param2);

/**
 * @brief 向所有下游组件推送数据帧 (核心数据流接口)
 * @warning ：当 fb 被下游接收后，该 API 会自动执行 fb_put 操作。源组件失去对 fb 的所有权。
 * @param msi   源组件
 * @param fb    数据帧链表头指针
 * @param care  源组件是否关心未接收情况
 *              - 1: 未接收时不 fb_put，需特殊处理
 *              - 0: 未接收时继续 fb_put
 * @return int32 
 *         - >0: 成功接收
 *         - =0: 无组件接收
 *         - -1: fb 为空导致无法接收
 * @note **必须**传入链表头节点，严禁传入中间节点。
 */
extern int32 msi_output_fb(struct msi *msi, struct framebuff *fb, uint8 care);

/**
 * @brief 安全释放数据帧
 * @details 当组件决定不转发某帧时，必须调用此函数清理，防止内存泄漏。
 * @param msi 当前组件
 * @param fb  待释放帧的头指针
 * @return int32 0 表示成功
 * @note **必须**传入链表头节点。
 */
extern int32 msi_delete_fb(struct msi *msi, struct framebuff *fb);

/**
 * @brief 从MSI的输入队列读取数据帧 (阻塞/超时)
 * @param msi    组件指针
 * @param tmo_ms 超时时间 (ms), 0 表示非阻塞
 * @return struct framebuff* 数据帧指针，超时或无数据返回 NULL
 */
extern struct framebuff *msi_get_fb(struct msi *msi, uint32 tmo_ms);

/**
 * @brief 强制丢弃 Framebuff。
 *        已存入下游组件的fb也会被丢弃。
 * @param msi    组件指针
 * @param fb     待丢弃帧
 * @param owner  丢弃 owner 创建的所有 framebuff。
 *               - 如果fb=NULL,则丢弃全部由owner创建的framebuff。
 * @return int32 执行结果
 */
extern int32 msi_discard_fb(struct msi *msi, struct framebuff *fb, struct msi *owner);

/**
 * @brief 追踪数据帧流向 (调试用)
 * @param msi   组件指针
 * @param fb    待追踪帧
 * @param owner 拥有者
 * @return int32 执行结果
 */
extern int32 msi_trace_fb(struct msi *msi, struct framebuff *fb, struct msi *owner);

/**
 * @brief 双向通知机制
 * @details 发送可向上游或下游传播的通知命令。
 * @param msi    发起组件
 * @param cmd    命令字
 * @param param1 参数 1
 * @param param2 参数 2
 */
extern void msi_notify(struct msi *msi, uint32 cmd, uint32 param1, uint32 param2);

/**
 * @brief 执行组件内部命令 (不广播)
 * @details 仅在本 MSI 组件执行逻辑，**不**向下游广播。
 * @param msi    目标组件
 * @param cmd    命令字
 * @param param1 参数 1
 * @param param2 参数 2
 * @return int32 执行结果
 */
extern int32 msi_do_cmd(struct msi *msi, uint32 cmd, uint32 param1, uint32 param2);

/**
 * @brief 分配 Framebuff
 * @details 受到 msi->fb_limits 限制，当 fb_limits=0 时则不能分配。
 *
 * @param msi             申请framebuff的MSI 组件
 * @param alloc_priv      申请此framebuff的模块私有信息，传递给 malloc_cb_t 和 mfree_cb_t 使用。
 * @param data            framebuff的数据地址。
 *                        - 如果data!=NULL，表示由外部代码提供数据空间，msi_alloc_fb 函数仅分配fb的信息空间。
 * @param data_size       framebuff的数据大小
 * @param codecinfo_size  framebuff的编码信息大小。
 *                        - 如果codecinfo_size!=0，msi_alloc_fb会自动分配编码信息所需的空间，并给fb->codecinfo进行赋值，外部代码可直接使用。
 *                        - 如果codecinfo_size=0，表示由外部代码对fb->codecinfo进行赋值，由外部代码管理fb->codecinfo的内存释放。
 * @param privinfo_size   framebuff的私有信息大小
 *                        - 如果privinfo_size!=0，msi_alloc_fb会自动分配私有信息所需的空间，并给fb->priv进行赋值，外部代码可直接使用。
 *                        - 如果privinfo_size=0，表示由外部代码对fb->priv进行赋值，由外部代码管理fb->priv的内存释放。
 * @return struct framebuff* 成功返回非 NULL，失败返回 NULL
 */
extern struct framebuff *msi_alloc_fb(struct msi *msi, void *alloc_priv, uint8 *data, uint32 data_size, uint32 codecinfo_size, uint32 privinfo_size);

/**
 * @brief [调试] 打印系统所有 MSI 组件状态
 */
void msi_dump(void);

/**
 * @brief 重复使用指定MSI的output组件
 * @param n_msi   新的MSI 组件
 * @param msi     目标MSI。 n_msi 将会使用 msi的 output list.
 * @return int32 执行结果
 */
int32 msi_reuse_output(struct msi *n_msi, struct msi *msi);

/**
 * @brief 向指定的MSI组件输入framebuf。通常是非MSI模块向MSI组件输入数据。
 * @param msi   MSI 组件
 * @param fb    framebuf
 * @return int32 执行结果: 0:成功，-1:失败
 * @note msi_recv_fb不会执行fb_put，需要调用者在该函数返回后执行fb_put。
 */
int32 msi_recv_fb(struct msi *msi, struct framebuff *fb);

/**
 * @brief 销毁MSI组件。
 * @warning ：仅在msi->delete=1的情况下使用此API。
 * @param msi   MSI 组件
 * @return int32 执行结果: 0:成功，-1:失败
 */
void msi_free(struct msi *msi);

/** @} */ // End of MSI_Internal_API

#ifdef __cplusplus
}
#endif

#endif /* _TXSEMI_MSI_H_ */
