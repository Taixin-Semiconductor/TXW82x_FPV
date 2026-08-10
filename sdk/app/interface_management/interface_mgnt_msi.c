#include "basic_include.h"
#include "stream_define.h"
#include "lv_demo_benchmark.h"
#include "keyWork.h"
#include "keyScan.h"
#include "lvgl/lvgl.h"
#include "ui/main_ui.h"
#include "hal/dma2d.h"
#include "lv_port_indev.h"
typedef void (*ui_start_cb_t)(void);

void lv_port_disp_init_msi(const char *name, uint16_t w, uint16_t h, uint8_t rotate);
void lv_port_indev_init(void);

static void *g_lvgl_hdl = NULL;
lv_style_t   g_style;

typedef uint32_t (*lvgl_get_key)(uint32_t key);
static lvgl_get_key g_lvgl_get_key = NULL;

struct lvgl_key_s
{
    lvgl_indev_s indev_user_data;
    struct os_msgqueue lvgl_key_msgq;
};

void set_lvgl_get_key_func(void *func)
{
    g_lvgl_get_key = (lvgl_get_key) func;
}

uint32_t lvgl_push_key(struct key_callback_list_s *callback_list, uint32_t keyvalue, uint32_t extern_value)
{
    struct lvgl_key_s *lvgl_key = (struct lvgl_key_s *) callback_list->priv;
    os_msgq_put(&lvgl_key->lvgl_key_msgq, keyvalue, 0);
    return 0;
}

uint32_t key_get_data(void *user_data)
{
    struct lvgl_key_s *lvgl_key = (struct lvgl_key_s *) user_data;
    uint32_t           key_ret  = 0;

    static uint32_t last_key = 0xff;
    uint32_t        val      = os_msgq_get(&lvgl_key->lvgl_key_msgq, 0);
    // 如果有按键被按下,就进入判断
    // 如果没有按键按下,就判断一下上一次是否为释放按键,如果不是,就执行上一次按键的键值(有可能是长按之类)
    if (g_lvgl_get_key)
    {
        if (val > 0)
        {
            return g_lvgl_get_key(val);
        }
        return key_ret;
    }
    if (val > 0 || !(((last_key & 0xff) == KEY_EVENT_SUP) || ((last_key & 0xff) == KEY_EVENT_LUP)))
    {
        if (!(val > 0))
        {
            val = last_key;
        }
        last_key = val;
        if ((val & 0xff) == KEY_EVENT_SUP)
        {
            switch (val >> 8)
            {
                case AD_UP:
                    key_ret = LV_KEY_PREV;
                    break;
                case AD_DOWN:
                    key_ret = LV_KEY_NEXT;
                    break;
                case AD_LEFT:
                    key_ret = LV_KEY_PREV;
                    break;
                case AD_RIGHT:
                    key_ret = LV_KEY_NEXT;
                    break;
                case AD_PRESS:
                    key_ret = LV_KEY_ENTER;
                    break;
                case AD_A:
                    break;
                case AD_B:
                    break;
                case AD_C:
                    break;

                default:
                    break;
            }
        }
        else if ((val & 0xff) == KEY_EVENT_REPEAT)
        {

            switch (val >> 8)
            {
                // lvgl需要持续按键才能识别长按
                case AD_PRESS:
                    key_ret = LV_KEY_ENTER;
                    break;

                default:
                    break;
            }
        }
    }
    return key_ret;
}

void lvgl_key_init(void *user_data)
{
    struct lvgl_key_s *lvgl_key = (struct lvgl_key_s *) os_zalloc(sizeof(struct lvgl_key_s));
    if (lvgl_key)
    {
        os_msgq_init(&lvgl_key->lvgl_key_msgq, 10);
        add_keycallback(lvgl_push_key, (void *) lvgl_key);
        lvgl_key->indev_user_data.read = key_get_data;
        lv_port_key_indev_init(lvgl_key);
    }
}

void lvgl_touchpad_init(void *user_data)
{
    lv_port_touchpad_indev_init(user_data);
}

// 挂起lvgl的任务,暂时没有考虑到是否支持任何时候挂起该任务,是否会有其他影响
// 默认这里没有影响
void lvgl_task_suspend()
{
    if (g_lvgl_hdl)
    {
#if DMA2D_EN
        struct dma2d_device *dma2d_dev = (struct dma2d_device *) dev_get(HG_DMA2D_DEVID);
        dma2d_mutex_lock(dma2d_dev);
#endif
        os_task_suspend2(g_lvgl_hdl);
#if DMA2D_EN
        dma2d_mutex_unlock(dma2d_dev);
#endif
    }
}

void lvgl_task_resume()
{
    if (g_lvgl_hdl)
    {
        os_task_resume2(g_lvgl_hdl);
    }
}

void lvgl_run_msi(void *d)
{
    uint32 cur_tick;
    cur_tick               = os_jiffies();
    ui_start_cb_t ui_start = (ui_start_cb_t) d;
    if (ui_start)
    {
        ui_start();
    }

    while (1)
    {
        os_sleep_ms(1);
        lv_tick_inc(os_jiffies() - cur_tick);
        cur_tick = os_jiffies();
        lv_timer_handler();
        if (os_jiffies() - cur_tick > 30)
        {
            os_printf("%s:%d\tuse_time:%d\n", __FUNCTION__, __LINE__, os_jiffies() - cur_tick);
        }
    }
}

// lvgl基础初始化,初始化完毕后可以初始化UI
void lvgl_init(uint16_t w, uint16_t h, uint8_t rotate)
{
    lv_init();                                       // lvgl初始化，如果这个没有初始化，那么下面的初始化会崩溃
    lv_port_disp_init_msi(S_LVGL_OSD, w, h, rotate); // 显示器初始化
    // lv_port_indev_init();
    //lvgl_key_init(NULL);
}

void lvgl_run(ui_start_cb_t ui_start)
{
    g_lvgl_hdl = os_task_create("gui_thread", lvgl_run_msi, (void *) ui_start, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 4096);
}