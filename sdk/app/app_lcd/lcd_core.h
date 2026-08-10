#ifndef __LCD_CORE_H
#define __LCD_CORE_H

#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "osal/work.h"
#include "hal/vdd.h"

// 数据申请空间函数 (PSRAM)
#define LCD_STREAM_MALLOC av_psram_malloc
#define LCD_STREAM_FREE   av_psram_free
#define LCD_STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数 (SRAM)
#define LCD_LIBC_MALLOC av_malloc
#define LCD_LIBC_FREE   av_free
#define LCD_LIBC_ZALLOC av_zalloc

// 虚拟屏配置
#define LCD_CORE_CHAN (2) // 通道数量

// 事件定义
enum
{
    LCD_CHAN_EMPTY   = BIT(0), // 通道被清空
    LCD_CHAN_DESTROY = BIT(1), // 通道被销毁
};

// 前向声明
struct lcd_core_s;
struct lcd_hdl_s;

// 函数表回调类型
typedef int32_t (*lcd_common_cb_t)(void *lcd, void *hdl);
typedef int32_t (*lcd_display_cb_t)(void *hdl, uint8_t which_video);

// LCD函数表结构体
struct lcd_fn
{
    // 释放资源
    // mfree_cb_t      free;
    // 显示单个通道数据
    lcd_display_cb_t display;
    // 混合多个通道数据后显示 (后期扩展)
    lcd_common_cb_t  display_mix;
    // 释放句柄
    lcd_common_cb_t  lcd_free_hdl;
};

// 通道信息结构体（用于轮询统计）
struct lcd_chan_info
{
    struct lcd_hdl_s *hdl;      // 通道句柄
    uint8_t           has_data; // 是否有数据
};

// LCD核心控制器结构体
typedef struct lcd_core_s
{
    struct os_work work; // 工作队列
    struct msi    *msi;  // 自身MSI组件
    struct msi    *p0;
    struct msi    *p1;
    uint32_t       chan[LCD_CORE_CHAN];    // 通道数组
    uint8_t        channel_num;            // 当前轮询索引
    uint8_t        gc : 1, v : 2, rev : 5; // gc标志
} lcd_core;

// LCD通道句柄结构体
struct lcd_hdl_s
{
    struct msi               *msi;      // 所属MSI
    struct lcd_core_s        *lcd_core; // 指向核心
    struct framebuff         *fb;       // 已显示的数据帧
    struct framebuff         *wait_fb;  // 待显示的数据帧,通道需要将这个数据拿走才能告诉通道已经空闲
    const struct lcd_fn      *fn;       // 函数表
    struct vcodec_decode_req *req;
    struct os_event           evt;  // 事件
    uint16_t                  w, h; // 显示尺寸
    uint8_t                   chan; // 通道索引
    uint8_t                   closed : 1, gc : 1, realse : 1, v : 2, rev : 3;
};

// 公共API函数声明
int32       get_lcd_core_free_chan(struct lcd_core_s *lcd_core, void *hdl);
struct msi *lcd_core_init(const char *msi_name);
void        lcd_core_destroy(struct msi *vlcd_msi);

#endif // __LCD_CORE_H