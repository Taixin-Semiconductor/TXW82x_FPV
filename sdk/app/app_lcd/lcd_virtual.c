#include "lcd_core.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "stream_define.h"
#include "app_lcd/app_lcd.h"
// 数据申请空间函数 (PSRAM)
#define LCD_STREAM_MALLOC av_psram_malloc
#define LCD_STREAM_FREE   av_psram_free
#define LCD_STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数 (SRAM)
#define LCD_LIBC_MALLOC av_malloc
#define LCD_LIBC_FREE   av_free
#define LCD_LIBC_ZALLOC av_zalloc

/**
 * @brief 释放通道句柄
 * @param lcd LCD核心控制器
 * @param hdl 通道句柄
 * @return 总是返回0
 */
static int32_t lcd_free_hdl(void *lcd, void *hdl)
{
    struct lcd_hdl_s *handle = (struct lcd_hdl_s *) hdl;

    if (handle)
    {
        // 释放事件
        os_event_del(&handle->evt);
        // 释放句柄内存
        LCD_LIBC_FREE(handle);
    }
    return 0;
}
static int32_t lcd_virtual_handle_display(void *hdl, uint8_t which_video)
{
    struct lcd_hdl_s *handle = (struct lcd_hdl_s *) hdl;
    lcd_hardware_display(which_video, handle->fb);
    return 0;
}

// LCD函数表实例
static const struct lcd_fn lcd_virtual_fn = {
        .display      = lcd_virtual_handle_display,
        .lcd_free_hdl = lcd_free_hdl,
};

/**
 * @brief 显示单个通道数据
 * @param lcd LCD核心控制器
 * @param hdl 通道句柄
 * @return 成功返回0
 */
int32_t lcd_display(struct vdd_device *dev, void *chan, struct framebuff *fb)
{
    int32_t            ret      = RET_ERR;
    struct lcd_hdl_s  *hdl      = (struct lcd_hdl_s *) chan;
    struct msi        *m        = hdl->msi;
    struct lcd_core_s *lcd_core = (struct lcd_core_s *) m->priv;

    ret = os_event_wait(&hdl->evt, LCD_CHAN_EMPTY, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
    if (!ret)
    {
        hdl->realse = 0;
        fb_get(fb);
        hdl->wait_fb = fb;
        os_run_work(&lcd_core->work);
    }
    return RET_OK;
}

/**
 * @brief 注册虚拟屏通道
 * @param msi MSI组件
 * @return 成功返回通道句柄，失败返回0
 */
static int32_t lcd_register_chan(struct msi *m)
{
    struct lcd_core_s *lcd_core = (struct lcd_core_s *) m->priv;
    int32_t            ret      = RET_ERR;
    struct lcd_hdl_s  *hdl      = NULL;

    // 分配通道句柄
    hdl = (struct lcd_hdl_s *) LCD_LIBC_ZALLOC(sizeof(struct lcd_hdl_s));
    if (hdl)
    {
        // 初始化句柄
        hdl->msi = m;
        hdl->fn  = &lcd_virtual_fn;
        hdl->fb  = NULL;
        hdl->w   = 0;
        hdl->h   = 0;

        // 初始化事件
        os_event_init(&hdl->evt);
        os_event_set(&hdl->evt, LCD_CHAN_EMPTY, NULL);

        // 在中断保护下分配通道
        uint32_t flag = disable_irq();
        ret           = get_lcd_core_free_chan(lcd_core, hdl);
        enable_irq(flag);

        // 失败则释放句柄
        if (ret != RET_OK)
        {
            hdl->fn->lcd_free_hdl(NULL, hdl);
            hdl = NULL;
        }
    }
    return (int32_t) hdl;
}

/**
 * @brief 打开虚拟屏设备
 * @param msi_name MSI组件名称
 * @return 成功返回通道句柄，失败返回NULL
 */
void *lcd_virtual_open(struct vdd_device *dev)
{
    // 初始化LCD核心（如果尚未初始化）
    struct msi *vlcd_msi = lcd_core_init(LCD_VIRTUAL_CORE_NAME);
    if (!vlcd_msi)
    {
        os_printf(KERN_ERR "LCD error: core init failed\n");
        return NULL;
    }

    // 注册通道
    struct lcd_hdl_s *hdl = (struct lcd_hdl_s *) lcd_register_chan(vlcd_msi);
    if (!hdl)
    {
        os_printf(KERN_ERR "LCD error: register chan failed\n");
        // 注意：这里不销毁lcd_core，因为可能有其他通道在使用
        return NULL;
    }

    return (void *) hdl;
}

/**
 * @brief 关闭虚拟屏设备
 * @param hdl 通道句柄
 * @return 成功返回0
 */
int32 lcd_virtual_close(struct vdd_device *dev, void *chan)
{
    struct lcd_hdl_s  *hdl      = (struct lcd_hdl_s *) chan;
    struct lcd_core_s *lcd_core = NULL;
    struct msi        *m        = hdl->msi;

    if (!hdl)
    {
        return RET_ERR;
    }

    lcd_core = hdl->lcd_core;
    if (!lcd_core)
    {
        return RET_ERR;
    }
    hdl->realse = 1;
    // 设置通道关闭状态
    hdl->closed  = 1;
    // 通知workqueue去释放通道
    lcd_core->gc = 1;
    os_run_work(&lcd_core->work);

    // 等待通道被清理
    // os_event_wait(&hdl->evt, LCD_CHAN_EMPTY | LCD_CHAN_DESTROY, NULL, OS_EVENT_WMODE_AND, -1);
    msi_destroy(m);

    return RET_OK;
}

/**
 * @brief 控制命令接口
 * @param hdl 通道句柄
 * @param cmd 控制命令
 * @param param 命令参数
 * @return 成功返回0
 */
int32 lcd_virtual_ioctl(struct vdd_device *dev, void *chan, uint32_t cmd, uint32_t param)
{
    struct lcd_hdl_s  *hdl      = (struct lcd_hdl_s *) chan;
    struct lcd_core_s *lcd_core = NULL;

    if (!hdl)
    {
        return RET_ERR;
    }

    lcd_core = hdl->lcd_core;
    if (!lcd_core)
    {
        return RET_ERR;
    }

    switch (cmd)
    {
        case 0: // 命令定义待定
            // 处理特定命令
            hdl->realse = 1;
            break;

        default:
            os_printf(KERN_ERR "LCD unknown cmd: 0x%x\n", cmd);
            break;
    }

    return RET_OK;
}

static const struct vdd_hal_ops vdd_hal_ops = {
        .open    = lcd_virtual_open,
        .close   = lcd_virtual_close,
        .display = lcd_display,
        .ioctl   = lcd_virtual_ioctl,
};

int32 vdd_attach(uint32 dev_id)
{
    struct vdd_device *dev = (struct vdd_device *) os_zalloc(sizeof(struct vdd_device));
    if (!dev)
    {
        return -ENOMEM;
    }
    dev->dev.ops = (const struct devobj_ops *) &vdd_hal_ops;
    return dev_register(dev_id, (struct dev_obj *) dev);
}