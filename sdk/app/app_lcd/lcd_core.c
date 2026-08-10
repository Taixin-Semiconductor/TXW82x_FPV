#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lcd_core.h"
#include "user_work/user_work.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "stream_define.h"

/**
 * @brief 获取LCD核心空闲通道
 * @param lcd_core LCD核心控制器
 * @param hdl 通道句柄
 * @return 成功返回0，失败返回-1
 */
int32 get_lcd_core_free_chan(struct lcd_core_s *lcd_core, void *hdl)
{
    int32             ret    = RET_ERR;
    struct lcd_hdl_s *handle = (struct lcd_hdl_s *) hdl;

    for (int i = 0; i < LCD_CORE_CHAN; i++)
    {
        if (lcd_core->chan[i] == 0)
        {
            lcd_core->chan[i] = (uint32_t) hdl;
            handle->chan      = i;
            handle->lcd_core  = lcd_core;
            os_printf(KERN_DEBUG "LCD chan select:%d\n", i);
            ret = RET_OK;
            break;
        }
    }
    return ret;
}

/**
 * @brief 轮询所有通道，统计有数据的通道
 * @param lcd_core LCD核心控制器
 * @param chan_info 通道信息数组
 * @param data_count 有数据的通道数量
 * @return 有数据返回RET_OK，无数据返回RET_ERR
 */
static int32_t lcd_poll_channels(struct lcd_core_s *lcd_core, struct lcd_chan_info *chan_info, int *data_count)
{
    int               count = 0;
    struct lcd_hdl_s *hdl;

    for (int i = 0; i < LCD_CORE_CHAN; i++)
    {
        hdl                   = (struct lcd_hdl_s *) lcd_core->chan[i];
        chan_info[i].hdl      = hdl;
        chan_info[i].has_data = 0;

        if (hdl && !hdl->closed)
        {
            if (hdl->realse)
            {
                if (hdl->wait_fb)
                {
                    msi_delete_fb(NULL, hdl->wait_fb);
                    hdl->wait_fb = NULL;
                    os_event_set(&hdl->evt, LCD_CHAN_EMPTY, NULL);
                }

                if (hdl->v & BIT(0))
                {
                    msi_cmd2(lcd_core->p0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);
                }
                else if (hdl->v & BIT(1))
                {
                    msi_cmd2(lcd_core->p1, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);
                }
                // 关闭显示
                hdl->v = 0;
                lcd_core->v &= (~BIT(i));
            }
            if (hdl->wait_fb)
            {
                chan_info[i].has_data = 1;
                hdl->fb               = hdl->wait_fb;
                hdl->wait_fb          = NULL;
                count++;
                // 确认是否已经显示过
                if (!(lcd_core->v & BIT(i)))
                {
                    lcd_core->v |= BIT(i);
                    if (i == 0)
                    {
                        msi_cmd2(lcd_core->p0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 1);
                    }
                    else if (i == 1)
                    {
                        msi_cmd2(lcd_core->p1, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 1);
                    }
                }
                hdl->v |= BIT(i);
                // 设置通道清空
                os_event_set(&hdl->evt, LCD_CHAN_EMPTY, NULL);
            }
        }
        else if (hdl && hdl->closed)
        {
            lcd_core->gc = 1;
            os_printf(KERN_DEBUG "LCD found closed chan:%d, trigger GC\n", i);
            // 关闭对应的video(暂时只是支持2层video,所以这里简单处理)
            if (hdl->v & BIT(0))
            {
                msi_cmd2(lcd_core->p0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);
            }
            else if (hdl->v & BIT(1))
            {
                msi_cmd2(lcd_core->p1, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);
            }
            hdl->v = 0;
            lcd_core->v &= (~BIT(i));
        }
    }
    *data_count = count;
    return (count > 0) ? RET_OK : RET_ERR;
}

/**
 * @brief 根据有数据的通道数量决定显示策略
 * @param lcd_core LCD核心控制器
 * @param chan_info 通道信息数组
 * @param data_count 有数据的通道数量
 */
static void lcd_display_strategy(struct lcd_core_s *lcd_core, struct lcd_chan_info *chan_info, int data_count)
{
    struct lcd_hdl_s *hdl;

    if (data_count == 0)
    {
        // 无数据，直接返回
        return;
    }
    else if (data_count <= 2)
    {
        // 多个通道有数据，需要混合后显示
        // 前期实现：依次显示每个通道（后期改为混合显示）
        for (int i = 0; i < LCD_CORE_CHAN; i++)
        {
            if (chan_info[i].has_data)
            {
                hdl = chan_info[i].hdl;
                // msi_delete_fb(NULL, hdl->fb);
                // 将数据发送到屏端显示
                if (hdl->fn->display)
                {
                    hdl->fn->display(hdl, i);
                }
                msi_output_fb(hdl->msi, hdl->fb, 0);
            }
        }

        // 后期扩展：调用混合显示函数
        // if (hdl->fn->display_mix) {
        //     hdl->fn->display_mix(lcd_core, chan_info, data_count);
        // }
    }
    // 暂时不支持
    else
    {
        for (int i = 0; i < LCD_CORE_CHAN; i++)
        {
            if (chan_info[i].has_data)
            {
                hdl = chan_info[i].hdl;
                os_printf(KERN_DEBUG "LCD display chan[no support[%d]]:%d (mix mode)\n", hdl->chan,data_count);
                msi_delete_fb(NULL, hdl->fb);
            }
        }
    }
}

/**
 * @brief 垃圾回收函数，处理已关闭的通道
 * @param lcd_core LCD核心控制器
 */
static void lcd_chan_gc(struct lcd_core_s *lcd_core)
{
    struct lcd_hdl_s *del;

    for (int i = 0; i < LCD_CORE_CHAN; i++)
    {
        del = (struct lcd_hdl_s *) lcd_core->chan[i];
        if (del)
        {
            // 处理标记为关闭或GC的通道
            if (del->closed || del->gc)
            {
                if (del->wait_fb)
                {
                    // 释放待显示的数据
                    os_printf(KERN_DEBUG "LCD GC release fb on chan:%d\n", i);
                    fb_put(del->wait_fb);
                    del->wait_fb = NULL;
                    // 发送通道清空事件
                    os_event_set(&del->evt, LCD_CHAN_EMPTY, NULL);
                }
            }

            // 释放已关闭的通道
            if (del->closed)
            {
                // 关闭对应的video(暂时只是支持2层video,所以这里简单处理)
                if (del->v & BIT(0))
                {
                    msi_cmd2(lcd_core->p0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);
                }
                else if (del->v & BIT(1))
                {
                    msi_cmd2(lcd_core->p1, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);
                }
                del->v = 0;
                lcd_core->v &= (~BIT(i));

                os_printf(KERN_DEBUG "LCD GC destroy chan:%d\n", i);
                lcd_core->chan[i] = 0; // 清空槽位
                // 发送通道清空和销毁事件
                os_event_set(&del->evt, LCD_CHAN_EMPTY | LCD_CHAN_DESTROY, NULL);
                if (del->wait_fb)
                {
                    msi_delete_fb(NULL, del->wait_fb);
                    del->wait_fb = NULL;
                }
                // 释放句柄内存
                if (del->fn && del->fn->lcd_free_hdl)
                {
                    del->fn->lcd_free_hdl(lcd_core, del);
                }
            }
        }
    }
}

/**
 * @brief 工作队列主循环
 * @param work 工作队列结构体
 * @return 总是返回0
 */
static int32_t lcd_core_work(struct os_work *work)
{
    struct lcd_core_s   *lcd_core = (struct lcd_core_s *) work;
    struct lcd_chan_info chan_info[LCD_CORE_CHAN];
    int                  data_count = 0;
    // 阶段1: 垃圾回收
    if (lcd_core->gc)
    {
        lcd_core->gc = 0;
        lcd_chan_gc(lcd_core);
    }

    // 阶段2: 轮询所有通道，统计有数据的通道
    lcd_poll_channels(lcd_core, chan_info, &data_count);
    // 阶段3: 根据数量决定显示策略
    if (data_count > 0)
    {
        lcd_display_strategy(lcd_core, chan_info, data_count);
    }
    // 重新调度，实现持续轮询
    os_run_work_delay(work, 1);
    return 0;
}

/**
 * @brief MSI回调函数
 * @param msi MSI组件
 * @param cmd_id 命令ID
 * @param param1 参数1
 * @param param2 参数2
 * @return 成功返回0
 */
static int32_t lcd_core_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t            ret      = RET_OK;
    struct lcd_core_s *lcd_core = (struct lcd_core_s *) msi->priv;

    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {

            // 后销毁处理：释放核心结构体内存
            if (lcd_core)
            {
                msi_put(lcd_core->p0);
                msi_put(lcd_core->p1);
                LCD_LIBC_FREE(lcd_core);
                msi->priv = NULL;
            }
        }
        break;

        case MSI_CMD_PRE_DESTROY:
        {
            msi_cmd2(lcd_core->p0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);
            msi_cmd2(lcd_core->p1, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);
            // 前销毁处理：停止工作队列
            os_work_cancle2(&lcd_core->work, 1);
            // 检查是否还有未释放的通道
            for (int i = 0; i < LCD_CORE_CHAN; i++)
            {
                if (lcd_core->chan[i])
                {
                    struct lcd_hdl_s *hdl = (struct lcd_hdl_s *) lcd_core->chan[i];
                    if (hdl->wait_fb)
                    {
                        msi_delete_fb(NULL, hdl->wait_fb);
                        hdl->wait_fb = NULL;
                        os_event_set(&hdl->evt, LCD_CHAN_EMPTY, NULL);
                    }
                    // 释放句柄内存
                    if (hdl->fn && hdl->fn->lcd_free_hdl)
                    {
                        hdl->fn->lcd_free_hdl(lcd_core, hdl);
                    }
                    lcd_core->chan[i] = 0;
                }
            }
        }
        break;

        default:
            break;
    }
    return ret;
}

/**
 * @brief 初始化LCD核心实例
 * @param msi_name MSI组件名称
 * @return 成功返回LCD核心指针，失败返回NULL
 */
struct msi *lcd_core_init(const char *msi_name)
{
    uint8_t            isnew;
    struct msi        *msi;
    struct lcd_core_s *lcd_core;

    // 创建MSI组件
    msi = msi_new(msi_name, 0, &isnew);
    if (isnew)
    {
        // 分配核心结构体
        lcd_core = (struct lcd_core_s *) LCD_LIBC_ZALLOC(sizeof(struct lcd_core_s));
        if (!lcd_core)
        {
            os_printf(KERN_ERR "LCD error: malloc failed\n");
            msi_destroy(msi);
            return NULL;
        }

        lcd_core->p0  = msi_find(R_VIDEO_P0, 0);
        lcd_core->p1  = msi_find(R_VIDEO_P1, 0);
        // 初始化核心结构体
        lcd_core->msi = msi;
        msi->priv     = (void *) lcd_core;
        msi->action   = lcd_core_action;
        msi->enable   = 1;

        // 启动工作队列
        OS_WORK_INIT(&lcd_core->work, lcd_core_work, 0);
        os_run_work(&lcd_core->work);

        os_printf(KERN_NOTICE "LCD init success\n");
    }

    return msi;
}

/**
 * @brief 销毁LCD核心实例
 * @param lcd_core LCD核心控制器
 */
void lcd_core_destroy(struct msi *vlcd_msi)
{
    if (!vlcd_msi)
    {
        return;
    }

    // 销毁MSI组件（会触发MSI_CMD_PRE_DESTROY和MSI_CMD_POST_DESTROY）
    msi_destroy(vlcd_msi);
    os_printf(KERN_NOTICE "LCD destroy success\n");
}