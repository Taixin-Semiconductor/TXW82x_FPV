#include "framebuff.h"

#ifndef _TXSEMI_MSI_H_
#define _TXSEMI_MSI_H_

#ifdef __cplusplus
extern "C" {
#endif

//最多支持4个输出接口
#ifndef MSI_OUTIF_MAX
#define MSI_OUTIF_MAX 4
#endif

//媒体流组件的命令列表
enum MSI_CMDs {
    MSI_CMD_PRE_DESTROY,     //通知组件MSI接口即将开始销毁
    MSI_CMD_POST_DESTROY,    //通知组件MSI接口即将完成销毁，组件需要释放自己的资源
    MSI_CMD_FREE_FB,         //通知组件即将释放framebuff，组件可以对framebuff进行特殊处理
    MSI_CMD_SET_DATATAG,
    MSI_CMD_GET_RUNNING,
    MSI_CMD_SET_SPEED,       //设置播放倍速
    MSI_CMD_TRANS_FB,
    MSI_CMD_TRANS_FB_END,
	MSI_CMD_OSD_ENCODE,
    MSI_CMD_SCALE1,
    MSI_CMD_SCALE2,
    MSI_CMD_SCALE3,
    MSI_CMD_TAKEPHOTO_SCALE3,
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
    MSI_CMD_AUDAC,
    MSI_CMD_AUTPC,
};
   
enum MSI_AUDIO_CTRL_CMD
{
    // audac使用
    MSI_AUDAC_SET_SAMPLING_RATE,
    MSI_AUDAC_GET_SAMPLING_RATE,
    MSI_AUDAC_SET_FILTER_TYPE,
    MSI_AUDAC_GET_FILTER_TYPE,
    MSI_AUDAC_SET_VOLUME,
    MSI_AUDAC_GET_VOLUME,
    MSI_AUDAC_SET_SPEED,
    MSI_AUDAC_GET_SPEED,
    MSI_AUDAC_SET_PITCH,
    MSI_AUDAC_GET_PITCH,
    MSI_AUDAC_CLEAR_STREAM,
    MSI_AUDAC_END_STREAM,
    MSI_AUDAC_GET_EMPTY,
    MSI_AUDAC_TEST_MODE,

    // 音频变速变调组件使用
    MSI_AUTPC_SET_SPEED,
    MSI_AUTPC_GET_SPEED,
    MSI_AUTPC_SET_PITCH,
    MSI_AUTPC_GET_PITCH,
    MSI_AUTPC_END_STREAM,
};

struct msi;

typedef int (*msi_action)(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2);

//媒体流组件接口
//media stream interface
struct msi {
    struct msi *next;           //单链表
    const char *name;           //组件ID：唯一
    msi_action action;          //组件接口的执行函数
    struct msi *output_list[MSI_OUTIF_MAX]; //输出组件列表
    void *priv;                 //私有数据，用于关联组件的另一半数据
    uint8 enable: 1, deleted: 1, rev: 6;   //状态标识
    atomic_t users;             //组件引用计数
    atomic_t bound;             //组件绑定计数
    atomic_t inited;            //组件init计数，msi_new 和 msi_destroy 需要匹配调用
    struct fbqueue fbQ;         //组件接收framebuff的队列
};

////////////////////////////////////////////////////////////////////////
//以下API 适合应用代码直接使用

//媒流体组件库初始化
extern int32 msi_core_init(void);

//设置该组件的输出组件：为 参数msi 或 参数lname 指定的组件设置输出 组件oname。
extern int32 msi_add_output(struct msi *msi, const char *lname, const char *oname);

//删除该组件的输出组件：删除 参数msi 或 参数lname 指定组件 的输出 组件oname。
extern int32 msi_del_output(struct msi *msi, const char *lname, const char *oname);

//指定从某个组件开始执行cmd
extern void msi_cmd(const char *name, uint32 cmd, uint32 param1, uint32 param2);

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
// 以下 API 只在MSI组件内部代码中使用，不适合应用代码使用。

//创建媒流体组件，创建时自动加入组件列表。和 msi_destroy 成对使用
extern struct msi *msi_new(const char *name, uint32 qsize, uint8 *isnew);

//销毁媒流体组件，销毁时自动从组件列表中删除
extern void msi_destroy(struct msi *msi);

extern void msi_put(struct msi *msi);

extern void msi_get(struct msi *msi);

//根据name查找该组件，计数为加1。需要和 msi_put 成对使用
// inited: 用于表示只查找已经被初始化的模块
extern struct msi *msi_find(const char *name, uint8 inited);

//组件输出cmd: 遍历自己的输出组件列表
extern int32 msi_output_cmd(struct msi *msi, uint32 cmd, uint32 param1, uint32 param2);

//组件输出framebuff: 遍历自己的输出组件列表
//注意：fb参数只能是 framebuff链表的第1个节点，不能是中间节点
extern int32 msi_output_fb(struct msi *msi, struct framebuff *fb);

//组件判断不输出framebuff时，需要删除framebuff
//注意：fb参数只能是 framebuff链表的第1个节点，不能是中间节点
extern int32 msi_delete_fb(struct msi *msi, struct framebuff *fb);

//MSI组件从队列中读取framebuff
extern struct framebuff *msi_get_fb(struct msi *msi, uint32 tmo_ms);

//MSI组件从队列中读取framebuff: 队列支持多个reader，需要指定reader ID
extern struct framebuff *msi_get_fb_r(struct msi *msi, uint32 reader);

//通知通路中的各个模块，丢弃指定的framebuff
extern int32 msi_discard_fb(struct msi *msi, struct framebuff *fb);

//追踪framebuff：查看指定的framebuff当前在被哪些模块处理
extern int32 msi_trace_fb(struct msi *msi, struct framebuff *fb);

//组件notify：可以 向上/向下 执行cmd
extern void msi_notify(struct msi *msi, uint32 cmd, uint32 param1, uint32 param2);

//组件执行自己的action（不会向下输出cmd）
extern int32 msi_do_cmd(struct msi *msi, uint32 cmd, uint32 param1, uint32 param2);

#ifdef __cplusplus
}
#endif

#endif

