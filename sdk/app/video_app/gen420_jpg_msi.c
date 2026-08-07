#include "basic_include.h"
#include "lib/multimedia/msi.h"

#include "dev/gen/hggen420.h"
#include "jpg_concat_msi.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "lib/video/gen/gen420_dev.h"
#include "stream_define.h"

#include "app/video_app/gen420_hardware_msi.h"
#include "lib/video/dvp/jpeg/jpg_common.h"
#include "gen420_hardware_msi.h"
#include "user_work/user_work.h"
#include "video_msi.h"
/****************************************************************************************************
 * 该文件主要是利用jpg1通过gen420重新编码jpg
 * 主要最终调用以下函数去触发gen420:
    register_gen420_queue
    wake_up_gen420_queue
    unregister_gen420_queue
 ****************************************************************************************************/
// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

#define MAX_GEN420_RX 8

#ifdef JPG_NODE_COUNT
#define GEN420_JPG_COUNT JPG_NODE_COUNT
#else
#define GEN420_JPG_COUNT 10
#endif
struct gen420_msi_s
{
    struct os_work    work;
    struct msi       *msi;
    struct msi       *register_jpg_msi; // 注册的jpg的msi,因为gen420需要与jpg或者其他硬件联动
    struct framebuff *fb;
    gen420_filter_fn  fn;
    uint32_t          magic;
    uint8_t           hardware_ready;
    uint16_t         *filter_type;
    uint8_t          force_type;
    uint16_t          last_w, last_h;
    uint8_t           src_from;
    uint8_t           which_jpg;
    uint8_t           lock_value;
    uint8_t           queue_value;
    uint8_t           output_en;
    uint8_t           recv_type;
    uint8_t           wait_close_jpg;
    uint8_t           stop;
};

static int32_t gen420_kick(struct gen420_msg_s *msg)
{
    int32_t              ret    = RET_OK;
    struct gen420_msi_s *gen420 = (struct gen420_msi_s *) msg->fn_data;
    // 配置一下jpg的类型
    if (gen420->register_jpg_msi)
    {
        msi_do_cmd(gen420->register_jpg_msi, MSI_CMD_SET_DATATAG, gen420->fb->datatag, 0);
        msi_do_cmd(gen420->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_SET_GEN420_TYPE, gen420->force_type ? gen420->force_type : gen420->fb->stype);
    }
    // 关闭jpg
    // msi_do_cmd(gen420->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 0);
    msi_do_cmd(gen420->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_NODE_COUNT, GEN420_JPG_COUNT);
    msi_do_cmd(gen420->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_SET_TIME, gen420->fb->time);
    // 修改jpg参数
    msi_do_cmd(gen420->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_MSG, gen420->last_w << 16 | gen420->last_h);
    // 修改数据源头
    msi_do_cmd(gen420->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_FROM, GEN420_DATA);
    // 重新启动mjpg
    ret = msi_do_cmd(gen420->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 1);
    return ret;
}

static int32_t gen420_free(struct gen420_msg_s *msg)
{
    struct gen420_msi_s *gen420 = (struct gen420_msi_s *) msg->fn_data;
    unregister_gen420_queue(gen420->queue_value);
    // msi_do_cmd(gen420->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 0);
    //  移除
    msi_delete_fb(NULL, gen420->fb);
    gen420->fb = NULL;
    jpg_mutex_unlock(gen420->which_jpg, gen420->lock_value);
    gen420->hardware_ready = 1;
    // 唤醒workqueue的任务
    _os_printf("]");
    os_run_work(&gen420->work);
    return 0;
}

static int32_t gen420_err_free(struct gen420_msi_s *gen420)
{
    // 移除
    msi_delete_fb(NULL, gen420->fb);
    gen420->fb = NULL;
    jpg_mutex_unlock(gen420->which_jpg, gen420->lock_value);
    gen420->hardware_ready = 1;
    os_run_work(&gen420->work);
    return 0;
}

static int32_t gen420_run(void *fn_data, struct framebuff *fb, uint16_t w, uint16_t h)
{
    struct gen420_msi_s *gen420 = (struct gen420_msi_s *) fn_data;
    // msi_do_cmd(gen420->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 0);
    int                  ret    = register_gen420_queue(gen420->queue_value, w, h, gen420_kick, gen420_free, (uint32) fn_data);
    if (ret == 0)
    {
        wake_up_gen420_queue(gen420->queue_value, fb->data);
    }
    else
    {
        gen420_err_free(gen420);
        os_printf(KERN_ERR "gen420_run err:%d\n", ret);
    }
    return ret;
}

static int32 gen420_jpg_work(struct os_work *work)
{
    struct gen420_msi_s *gen420 = (struct gen420_msi_s *) work;
    uint8_t              last_lock_value;
    int32_t              ret;

    // 如果gen420没有启动,那么一定可以启动mjpg或者gen420
    if (gen420->hardware_ready)
    {
        // 尝试看看是否有新的数据需要编码
        if (!gen420->fb)
        {
            gen420->fb = msi_get_fb(gen420->msi, 0);
            if (!gen420->output_en && gen420->fb)
            {
                uint32_t send_count = msi_output_fb(gen420->msi, NULL);
                // 有下一级的流需要接收,那么就传递下去,如果没有,则移除,不需要做后面的动作
                if (!send_count)
                {
                    msi_delete_fb(NULL, gen420->fb);
                    gen420->fb = NULL;
                    if(gen420->stop)
                    {
                        return 0;
                    }
                }
                else
                {
                    gen420->output_en = 1;
                }
            }
        }

        // 有新的数据需要编码,则尝试去编码
        if (gen420->fb)
        {
            gen420->wait_close_jpg = 0;

            // 去获取jpg1的锁
            ret = jpg_mutex_lock(gen420->which_jpg, gen420->lock_value, &last_lock_value);
            // 获取成功,则去编码
            if (ret == 0)
            {
                // os_printf("lock jpg:%d\n",gen420->which_jpg);
                //  获取成功后,初始化mjpg1相关硬件
                if (!gen420->register_jpg_msi)
                {
                    gen420->register_jpg_msi = jpg_concat_msi_init_start(gen420->which_jpg, 320, 180, NULL, gen420->src_from, 0);
                    if (gen420->register_jpg_msi)
                    {
                        // 主动停止一下
                        msi_do_cmd(gen420->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 0);
                        msi_add_output(gen420->register_jpg_msi, NULL, gen420->msi->name);
                    }
                }

                if (gen420->register_jpg_msi)
                {
                    // 使用不一致,就先停再启动
                    if (last_lock_value != gen420->lock_value)
                    {
                        msi_do_cmd(gen420->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 0);
                    }
                    msi_do_cmd(gen420->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_OPEN, GEN420_DATA);
                }
                // 异常,不应该出现,除非其他地方打开了占用该jpg需要检查,应该应用去规避
                else
                {
                    jpg_mutex_unlock_check(gen420->which_jpg, gen420->lock_value);
                    os_printf("gen420->register_jpg_msi init err\r\n");
                    os_run_work_delay(work, 1);
                    return 0;
                }
                struct yuv_arg_s *yuv_msg;
                yuv_msg = (struct yuv_arg_s *) gen420->fb->priv;
                if (yuv_msg)
                {
                    uint32_t p_w, p_h;
                    p_w                    = yuv_msg->out_w;
                    p_h                    = yuv_msg->out_h;
                    gen420->last_w         = p_w;
                    gen420->last_h         = p_h;
                    gen420->hardware_ready = 0;
                    gen420->stop           = 0;
                    gen420_run(gen420, gen420->fb, p_w, p_h);
                }
                // 不应该进入这里,可能fb给错了,下一次移除
                else
                {
                    // 移除
                    msi_delete_fb(NULL, gen420->fb);
                    gen420->fb = NULL;
                }
            }
            else
            {
                os_run_work_delay(work, 1);
                return 0;
            }
        }
        // 没有找到等到新的yuv数据,那么就等100ms去检查是否需要关闭mjpeg
        else
        {
            if (!gen420->wait_close_jpg)
            {
                gen420->wait_close_jpg = 1;
                os_run_work_delay(work, 100);
            }
            else
            {
                ret = jpg_mutex_lock(gen420->which_jpg, gen420->lock_value, &last_lock_value);
                if (ret == 0)
                {
                    if (gen420->lock_value == last_lock_value)
                    {
                        if (gen420->register_jpg_msi)
                        {
                            _os_printf(KERN_INFO "##################\n");
                            gen420->stop = 1;
                            msi_do_cmd(gen420->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 0);
                        }
                    }
                    jpg_mutex_unlock_check(gen420->which_jpg, gen420->lock_value);
                }
            }
        }
    }
    else
    {
        os_run_work_delay(work, 1);
    }
    return 0;
}

static int32_t gen420_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t              ret    = RET_OK;
    struct gen420_msi_s *gen420 = (struct gen420_msi_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            os_work_cancle2(&gen420->work, 1);
            STREAM_LIBC_FREE(gen420);
        }
        break;
        case MSI_CMD_PRE_DESTROY:
        {
            os_work_cancle2(&gen420->work, 1);
            if (gen420->register_jpg_msi)
            {
                msi_destroy(gen420->register_jpg_msi);
                gen420->register_jpg_msi = NULL;
            }
        }
        break;

        case MSI_CMD_JPG_RECODE:
        {
            uint32_t cmd_self = (uint32_t) param1;
            uint32_t arg      = param2;
            switch (cmd_self)
            {
                // 需要注册jpg,中断可能需要用到
                case MSI_JPG_RECODE_MAGIC:
                {
                    gen420->magic = arg;
                }
                break;
                case MSI_JPG_RECODE_FORCE_TYPE:
                {
                    gen420->force_type = (uint16_t) arg;
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
            // 判断是否要转发
            if (gen420->fn && gen420->fn((void *) fb, gen420->recv_type) == 0)
            {
                // 所有图片都转发
                struct framebuff *fb = (struct framebuff *) param1;
                fb_get(fb);
                int send = msi_output_fb(msi, fb);
                if (!send)
                {
                    gen420->output_en = 0;
                }
                ret = RET_OK + 1;
                break;
            }

            if (fb->mtype != F_YUV)
            {
                ret = RET_ERR;
            }
            else
            {
                if (gen420->filter_type)
                {
                    // 轮询类型是否有一致
                    ret            = RET_ERR;
                    uint16_t *each = gen420->filter_type;
                    while (*each)
                    {
                        // 如果一致,返回OK
                        if (*each == fb->stype)
                        {
                            ret = RET_OK;
                            break;
                        }
                        each++;
                    }
                }

                if (ret == RET_OK)
                {
                    struct yuv_arg_s *yuv_msg;
                    yuv_msg = (struct yuv_arg_s *) fb->priv;
                    ret     = RET_ERR;
                    // 如果magic为0,不需要处理
                    if (!gen420->magic)
                    {
                        ret = RET_OK;
                    }
                    // magic不是0,则需要匹配
                    else if (gen420->magic == yuv_msg->magic)
                    {
                        ret = RET_OK;
                    }
                }

                if (ret == RET_OK)
                {
                    os_run_work(&gen420->work);
                }
            }
        }
        break;

        default:
            break;
    }
    return ret;
}

struct msi *gen420_jpg_msi_init(const char *msi_name, uint8_t which_jpg, uint8_t recv_type, uint8_t lock_value, uint8_t queue_value, uint16_t *filter_type, gen420_filter_fn fn)
{
    uint8_t isnew;
    ASSERT(which_jpg == 0 || which_jpg == 1);
    struct msi          *msi    = msi_new(msi_name, MAX_GEN420_RX, &isnew);
    struct gen420_msi_s *gen420 = (struct gen420_msi_s *) msi->priv;
    if (isnew)
    {
        gen420                 = (struct gen420_msi_s *) STREAM_LIBC_ZALLOC(sizeof(struct gen420_msi_s));
        msi->priv              = (void *) gen420;
        msi->action            = gen420_msi_action;
        gen420->msi            = msi;
        gen420->filter_type    = filter_type;
        gen420->hardware_ready = 1;
        gen420->force_type     = 0; // 强转类型
        gen420->which_jpg      = which_jpg;
        gen420->lock_value     = lock_value;
        gen420->queue_value    = queue_value;
        gen420->recv_type      = recv_type;
        gen420->fn             = fn;
        gen420->output_en      = 0;
        msi->enable            = 1;
        // 启动workqueue
        OS_WORK_INIT(&gen420->work, gen420_jpg_work, 0);
        os_run_work(&gen420->work);
    }

    return msi;
}