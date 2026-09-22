#ifndef HOGPD_LIVE_APP_H
#define HOGPD_LIVE_APP_H

#include "hogpd_types.h"
#include "typesdef.h"
#include "lib/common/sysevt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 自定义 main id，避开 SDK SYSEVT_MAINID(1~7)。 */
enum {
    HOGPD_SYSEVT_MAIN = 0x50,
};

/* 人形目标事件(坐标均为scale3输出帧坐标, 主画面800×480, 中心(400,240))。 */
typedef enum {
    /* 实测检测框中心：data = (cy<<16)|cx */
    HOGPD_SYSEVT_HUMAN_DETECTED = SYS_EVENT(HOGPD_SYSEVT_MAIN, 1),
    /* 短暂丢失期间Kalman预测框中心(电机平滑跟随) */
    HOGPD_SYSEVT_HUMAN_PREDICT  = SYS_EVENT(HOGPD_SYSEVT_MAIN, 2),
    /* 可见→不可见下降沿发一次：data = 0 */
    HOGPD_SYSEVT_HUMAN_LOST     = SYS_EVENT(HOGPD_SYSEVT_MAIN, 3),
} hogpd_sysevt_event_t;

// 打包坐标为事件data(按值传递, 异步分发安全)。
static inline uint32 HOGPD_SYSEVT_PACK_XY(int32 x, int32 y)
{
    return ((((uint32)y) & 0xFFFFu) << 16) | (((uint32)x) & 0xFFFFu);
}

// 从事件data解包X坐标。
static inline int32 HOGPD_SYSEVT_UNPACK_X(uint32 data)
{
    return (int32)(data & 0xFFFFu);
}

// 从事件data解包Y坐标。
static inline int32 HOGPD_SYSEVT_UNPACK_Y(uint32 data)
{
    return (int32)((data >> 16) & 0xFFFFu);
}

void hogpd_submit_yuv420p(uint8 *yuv420p, int32 width, int32 height);
// 设置人形事件节流间隔(ms)，0=每帧都发；LOST下降沿不受节流。默认50。
void hogpd_set_event_interval_ms(uint32 interval_ms);

#ifdef __cplusplus
}
#endif

#endif
