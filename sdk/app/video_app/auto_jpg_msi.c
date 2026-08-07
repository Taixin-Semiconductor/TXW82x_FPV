#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "jpg_concat_msi.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "stream_define.h"
#include "user_work/user_work.h"
#include "lib/video/dvp/jpeg/jpg_common.h"

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

#ifdef JPG_NODE_COUNT
#define AUTO_JPG_COUNT JPG_NODE_COUNT
#else
#define AUTO_JPG_COUNT 10
#endif

#ifndef SCALE_W
#define SCALE_W 1280
#endif

#ifndef SCALE_H
#define SCALE_H 720
#endif

struct auto_jpg_msi_s
{
    struct os_work work;
    struct msi    *msi;
    struct msi    *register_jpg_msi; // 注册的jpg的msi
    uint16_t       w, h;
    uint8_t        src_from;
    uint8_t        stop;
    uint8_t        which_jpg;
};

static int32 auto_jpg_work(struct os_work *work)
{
    uint8_t                last_value;
    int32_t                ret;
    uint8_t                delay_time = 40;
    struct auto_jpg_msi_s *auto_jpg   = (struct auto_jpg_msi_s *) work;
    // 这里尝试发送数据?检查一下是否有接收的msi,如果有接收的msi,则启动mjpeg,如果没有,则将硬件停下来?
    uint32_t               send_count = msi_output_fb(auto_jpg->msi, NULL);
    // 代表有需要接收的数据流,那么就要启动硬件,尝试获取锁,然后启动mjpg,如果上一次锁是mjpg,就不需要重新启动
    if (send_count)
    {
        // 尝试查看是否已经停止过
        ret = jpg_mutex_lock(auto_jpg->which_jpg, JPG_LOCK_ENCODE, &last_value);
        // 获取到锁,并且锁的值不一致,则要重新去启动mjpeg
        if (ret == 0)
        {
            // 如果锁的值不一致,代表该mjpg被其他地方使用过了,需要重新启动
            // 如果锁的值一致,但是stop == 1,代表已经停止过了,也需要重新启动
            if (last_value != JPG_LOCK_ENCODE || auto_jpg->stop == 1)
            {
                // 关闭mjpeg
                msi_do_cmd(auto_jpg->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 0);
                msi_do_cmd(auto_jpg->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_NODE_COUNT, AUTO_JPG_COUNT);
                if (auto_jpg->src_from == SCALER_DATA)
                {
                    msi_do_cmd(auto_jpg->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_SET_SCALE1_TYPE, 0);
                    msi_do_cmd(auto_jpg->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_MSG, auto_jpg->w << 16 | auto_jpg->h);
                }

                // 切回到默认源头
                msi_do_cmd(auto_jpg->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_FROM, auto_jpg->src_from);
                // 重新启动mjpg
                msi_do_cmd(auto_jpg->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 1);
                auto_jpg->stop = 0;
            }
            // 解锁,可以被其他线程打断使用,因为这个是从VPP_DATA0,可以被其他线程打断使用完再恢复，
            // 这样只是丢部分帧,但是可以复用mjpg
            jpg_mutex_unlock(auto_jpg->which_jpg, JPG_LOCK_ENCODE);
        }
        // 没有获取到锁,就延时一点时间后,继续获取
        else
        {
            delay_time = 5;
        }
    }
    else if (send_count == 0)
    {
        if (auto_jpg->stop == 0)
        {
            ret = jpg_mutex_lock(auto_jpg->which_jpg, JPG_LOCK_ENCODE, &last_value);
            if (ret == 0)
            {
                // 如果锁的值一致,那么就要主动停止,因为已经在启动了
                if (last_value == JPG_LOCK_ENCODE)
                {
                    msi_do_cmd(auto_jpg->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 0);
                }

                jpg_mutex_unlock_check(auto_jpg->which_jpg, JPG_LOCK_ENCODE);
            }
            auto_jpg->stop = 1;
        }
    }
    os_run_work_delay(work, delay_time);
    return 0;
}

static int32_t auto_jpg_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t                ret      = RET_OK;
    struct auto_jpg_msi_s *auto_jpg = (struct auto_jpg_msi_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            os_work_cancle2(&auto_jpg->work, 1);
            STREAM_LIBC_FREE(auto_jpg);
        }
        break;
        case MSI_CMD_PRE_DESTROY:
        {
            os_work_cancle2(&auto_jpg->work, 1);
            if (auto_jpg->register_jpg_msi)
            {
                msi_destroy(auto_jpg->register_jpg_msi);
                auto_jpg->register_jpg_msi = NULL;
            }
        }
        break;

        case MSI_CMD_AUTO_JPG:
        {
            uint32_t cmd_self = (uint32_t) param1;
            uint32_t arg      = param2;
            switch (cmd_self)
            {
                // 切换数据源
                case MSI_AUTO_JPG_SWITCH_ENCODE_SRC:
                {
                    auto_jpg->src_from = arg;
                    auto_jpg->stop     = 1;
                }
                break;
                default:
                    break;
            }
        }
        break;

        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            // 如果是mjpeg,帮忙转发到其他数据流
            // 如果需要支持264,也要帮忙转发
            if (fb->mtype == F_JPG)
            {
                // 类型匹配才可以转发
                if (fb->stype == FSTYPE_VIDEO_VPP_DATA0 + auto_jpg->src_from)
                {
                    _os_printf("+");
                    struct framebuff *fb = (struct framebuff *) param1;
                    fb_get(fb);
                    msi_output_fb(msi, fb);
                }
                ret = RET_OK + 1;
                break;
            }
        }
        break;

        default:
            break;
    }
    return ret;
}
extern uint8_t get_vpp_scale_w_h(uint16_t *w, uint16_t *h);
struct msi    *auto_jpg_msi_init(const char *auto_jpg_name, uint8_t which_jpg, uint8_t src_from)
{
    uint8_t     isnew;
    uint16_t    w, h;
    int         ret = 0;
    struct msi *msi = NULL;
    // 绑定jpg的msi,这个size不重要(除非是scale的size)
    if (src_from == SCALER_DATA)
    {
        ret = get_vpp_scale_w_h(&w, &h);
    }
    if (ret)
    {
        os_printf(KERN_ERR "%s src_from err ret:%d\n", __FUNCTION__, ret);
        goto auto_jpg_msi_init_end;
    }
    msi                             = msi_new(auto_jpg_name, 0, &isnew);
    struct auto_jpg_msi_s *auto_jpg = (struct auto_jpg_msi_s *) msi->priv;
    if (isnew)
    {
        auto_jpg                   = (struct auto_jpg_msi_s *) STREAM_LIBC_ZALLOC(sizeof(struct auto_jpg_msi_s));
        msi->priv                  = (void *) auto_jpg;
        msi->action                = auto_jpg_msi_action;
        auto_jpg->msi              = msi;
        auto_jpg->src_from         = src_from;
        auto_jpg->stop             = 1;
        auto_jpg->which_jpg        = which_jpg;
        auto_jpg->w                = w;
        auto_jpg->h                = h;
        // w和h在vpp_data0和vpp_data1的时候是没有作用的
        auto_jpg->register_jpg_msi = jpg_concat_msi_init_start(auto_jpg->which_jpg, w, h, NULL, auto_jpg->src_from, 0);
        if (auto_jpg->register_jpg_msi)
        {
            msi_add_output(auto_jpg->register_jpg_msi, NULL, msi->name);
        }
        else
        {
            msi_destroy(msi);
            msi = NULL;
            goto auto_jpg_msi_init_end;
        }

        msi->enable = 1;
        // 启动workqueue
        OS_WORK_INIT(&auto_jpg->work, auto_jpg_work, 0);
        os_run_work(&auto_jpg->work);
    }

auto_jpg_msi_init_end:
    return msi;
}