#include "app_lcd.h"

#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE av_free
#define STREAM_LIBC_ZALLOC av_zalloc

#define MAX_LVGL_OSD_TX 2

struct lvgl_osd_msi_s
{
    struct msi *msi;
    struct fbpool tx_pool;
};

static int lvgl_osd_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int ret = RET_OK;
    struct lvgl_osd_msi_s *lvgl_osd = (struct lvgl_osd_msi_s *)msi->priv;
    switch (cmd_id)
    {
        // 暂时没有考虑释放
        case MSI_CMD_POST_DESTROY:
        {
            struct framebuff *fb;
            // 释放资源fb资源文件,priv是独立申请的
            while (1)
            {
                fb = fbpool_get(&lvgl_osd->tx_pool, 0, NULL);
                if (!fb)
                {
                    break;
                }
                // 预分配空间释放
                if (fb->priv)
                {
                    STREAM_LIBC_FREE(fb->priv);
                }
            }
            fbpool_destroy(&lvgl_osd->tx_pool);
            STREAM_LIBC_FREE(lvgl_osd);
        }
        break;
        // 接收,判断是类型是否可以支持压缩
        case MSI_CMD_TRANS_FB:
        {
        }
        break;
        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;

            struct encode_data_s_callback *callback = (struct encode_data_s_callback *)fb->priv;
            if (callback->finish_cb)
            {
                callback->finish_cb(callback->user_data);
            }
            if (callback->free_cb)
            {
                callback->free_cb(callback->user_data, fb->data);
            }
            // 这个data可能是lvgl那边的,暂时不需要释放
            fb->data = NULL;
            callback->finish_cb = NULL;
            callback->free_cb = NULL;
            callback->user_data = NULL;
            // msi_get_fb(NULL,fb);

            fbpool_put(&lvgl_osd->tx_pool, fb);
            ret = RET_OK + 1;
        }
        break;
        default:
            break;
    }
    return ret;
}

struct framebuff *lvgl_osd_msi_get_fb(struct msi *msi)
{
    struct lvgl_osd_msi_s *lvgl_osd = (struct lvgl_osd_msi_s *)msi->priv;
    struct framebuff *fb = fbpool_get(&lvgl_osd->tx_pool, 0, lvgl_osd->msi);
    if(fb)
    {
        fb->mtype = F_RGB;
        fb->stype = LVGL_RGB;
    }

    return fb;
}

// 拷贝osd到内存,然后发送出去,使用workqueue去作为任务吧
struct msi *lvgl_osd_msi(const char *name)
{
    struct msi *msi = msi_new(name, 0, NULL);

    struct lvgl_osd_msi_s *lvgl_osd = (struct lvgl_osd_msi_s *)msi->priv;

    if (!lvgl_osd)
    {
        lvgl_osd = (struct lvgl_osd_msi_s *)STREAM_LIBC_ZALLOC(sizeof(struct lvgl_osd_msi_s));

        lvgl_osd->msi = msi;

        fbpool_init(&lvgl_osd->tx_pool, MAX_LVGL_OSD_TX);
        int init_count = 0;
        void *priv;
        // 初始化framebuffer节点数量空间?最后由workqueue去从ringbuf去获取一个节点
        while (init_count < MAX_LVGL_OSD_TX)
        {
            priv = (void *)STREAM_LIBC_ZALLOC(sizeof(struct encode_data_s_callback));
            FBPOOL_SET_INFO(&lvgl_osd->tx_pool, init_count, NULL, 0, priv);
            init_count++;
        }
        msi->priv = (void *)lvgl_osd;
        msi->action = lvgl_osd_msi_action;
        msi->enable = 1;
        msi_add_output(lvgl_osd->msi, NULL, R_OSD_ENCODE);
    }

    return msi;
}
