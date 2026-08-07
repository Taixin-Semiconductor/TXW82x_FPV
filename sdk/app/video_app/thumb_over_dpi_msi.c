// #include "thumb_msi.h"
#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/fs/fatfs/osal_file.h"
#include "stream_define.h"
#include "video_app/file_thumb.h"
#include "user_work/user_work.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "video_msi.h"
#include "video_app/file_common_api.h"

#define USER_WORK     1
#define MAX_THUMB_MSI 16
#define MAX_RECV_MAX  16

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#ifdef MORE_SRAM
#define STREAM_LIBC_MALLOC av_psram_malloc
#define STREAM_LIBC_FREE   av_psram_free
#define STREAM_LIBC_ZALLOC av_psram_zalloc
#else
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc
#endif

enum thumb_thread_enum
{
    THUMB_THREAD_RUNNING = BIT(0),
    THUMB_THREAD_EXIT    = BIT(1),
    THUMB_THREAD_DEADTH  = BIT(2),
};

struct thumb_queue_fb
{
    uint8_t  datatag;  // 对应的stype,按照顺序递增,只有stype匹配才可以
    uint16_t time16;   // 记录时间戳,只有16位,仅仅简单作为超时使用
    uint8_t  path[64]; // 记录缩略图的路径
};

#if USER_WORK == 0
struct thumb_msi_s
{
    os_task_t       task;
    struct msi     *msi;
    struct os_event evt;
    const char     *normal_base_dir; // 设置如果是原图,则有个基础文件夹地址
    uint32_t        magic;
    uint8_t         ramdom_id; // 随机ID,实际最后与stype匹配用
    uint8_t         jpg_type;  // 0:是缩略图    1:原图
    uint8_t         stype;     // 设置自己的专有类型
    RBUFFER_DEF(rb, struct thumb_queue_fb, MAX_THUMB_MSI);
};

static void thumb_msi_thread(void *d)
{
    struct thumb_msi_s   *thumb_msi = (struct thumb_msi_s *) d;
    struct msi           *msi       = thumb_msi->msi;
    struct framebuff     *fb;
    struct thumb_queue_fb t_fb;
    uint8_t               ret;
    uint8_t               path[80];
    int32_t               res;
    uint32_t              rflags;
    msi_get(msi);
    while (1)
    {
        rflags = 0;
        res    = os_event_wait(&thumb_msi->evt, THUMB_THREAD_RUNNING | THUMB_THREAD_EXIT, &rflags, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, -1);
        if (rflags & THUMB_THREAD_EXIT)
        {
            os_event_set(&thumb_msi->evt, THUMB_THREAD_DEADTH, NULL);
            break;
        }

    thumb_msi_thread_get_fb_again:
        fb = msi_get_fb(msi, 0);
        // 获取到图片,判断stype与rb中的stype是否匹配
        if (fb)
        {
        thumb_msi_thread_again:
            ret = RB_INT_GET(&thumb_msi->rb, t_fb);
            if (ret)
            {
                if (fb->datatag == t_fb.datatag)
                {
                    // 生成缩略图
                    if (thumb_msi->jpg_type == 0)
                    {
                        gen_thumb_path((char *) t_fb.path, path, sizeof(path));
                    }
                    else
                    {
                        takephoto_name_add_dir(path, sizeof(path), (char *) t_fb.path, thumb_msi->normal_base_dir);
                    }
                    // 保存地址,转发出去
                    struct framebuff *send_fb = fb_clone(fb, F_FILE_T << 8 | fb->stype, msi);
                    ASSERT(send_fb);
                    struct file_msg_s *file_msg = (struct file_msg_s *) STREAM_LIBC_ZALLOC(sizeof(struct file_msg_s));
                    ASSERT(file_msg);
                    os_memcpy(file_msg->path, path, strlen(path) + 1);
                    if (thumb_msi->jpg_type == 0)
                    {
                        file_msg->ishid = 1;
                    }
                    send_fb->priv = (void *) file_msg;
                    msi_output_fb(msi, send_fb);
                }
                else
                {
                    // 如果fb->stype > t_fb.stype?代表可能已经有漏数据,则忽略,继续寻找洗一张图
                    if (fb->datatag > t_fb.datatag)
                    {
                        goto thumb_msi_thread_again;
                    }
                    else
                    {
                        os_printf("thumb_msi_thread: no match fb->datatag:%X\tt_fb.datatag:%X\n", fb->datatag, t_fb.datatag);
                    }
                }
            }
            else
            {
                os_printf("thumb_msi_thread: no match fb,datatag:%X\tfb:%X\tret:%d\n", fb->datatag, fb, ret);
            }
            msi_delete_fb(NULL, fb);
            // 去缓冲区继续查找是否有图片需要保存
            goto thumb_msi_thread_get_fb_again;
        }
    }
    msi_put(msi);
}

static int32_t thumb_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t             ret       = RET_OK;
    struct thumb_msi_s *thumb_msi = (struct thumb_msi_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            os_event_wait(&thumb_msi->evt, THUMB_THREAD_DEADTH, NULL, OS_EVENT_WMODE_OR, -1);
            os_event_del(&thumb_msi->evt);
            STREAM_LIBC_FREE(thumb_msi);
        }
        break;
        case MSI_CMD_PRE_DESTROY:
        {
            os_event_set(&thumb_msi->evt, THUMB_THREAD_EXIT, NULL);
        }
        break;

        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            // fb由内部管理,仅仅删除私有结构
            if (fb && fb->mtype == F_FILE_T)
            {
                if (fb->priv)
                {
                    STREAM_LIBC_FREE(fb->priv);
                    fb->priv = NULL;
                }
            }
        }
        break;
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            // 通过YUV去生成缩略图
            if (fb->mtype == F_YUV)
            {
                struct takephoto_yuv_arg_s *arg = (struct takephoto_yuv_arg_s *) fb->priv;
                if (!thumb_msi->magic || arg->yuv_arg.magic == thumb_msi->magic)
                {
                    // 记录path以及转发去生成真的缩略图
                    // 这里采用clone方式,将类型修改
                    thumb_msi->ramdom_id = (thumb_msi->ramdom_id + 1);
                    struct thumb_queue_fb t_fb;
                    t_fb.datatag = thumb_msi->ramdom_id;
                    t_fb.time16  = (uint16_t) (os_jiffies() & 0xffff);

                    // 配置参数,转发
                    if (arg && arg->yuv_arg.type == YUV_ARG_TAKEPHOTO)
                    {
                        os_memcpy(t_fb.path, arg->name, strlen(arg->name) + 1);
                        if (RB_INT_SET(&thumb_msi->rb, t_fb))
                        {
                            // 转发出去
                            struct framebuff *c_fb = fb_clone(fb, fb->mtype << 8 | thumb_msi->stype, msi);
                            if (c_fb)
                            {
                                c_fb->datatag = thumb_msi->ramdom_id;
                                msi_output_fb(msi, c_fb);
                            }
                        }
                    }
                    else
                    {
                        msi_delete_fb(NULL, fb);
                    }
                }
                // 不需要接收到队列
                ret = RET_ERR;
            }
            // 通过mjpeg去生成缩略图(需要转发到解码器去实现)
            else if (fb->mtype == F_THUMB_JPG)
            {
                // 记录path以及转发去生成真的缩略图
                // 这里采用clone方式,将类型修改
                thumb_msi->ramdom_id = (thumb_msi->ramdom_id + 1);
                struct thumb_queue_fb t_fb;
                t_fb.datatag = thumb_msi->ramdom_id;
                t_fb.time16  = (uint16_t) (os_jiffies() & 0xffff);

                if (fb->priv)
                {
                    os_memcpy(t_fb.path, fb->priv, strlen(fb->priv) + 1);
                }
                else
                {
                    os_memset(t_fb.path, 0, sizeof(t_fb.path));
                }
                if (RB_INT_SET(&thumb_msi->rb, t_fb))
                {
                    // 转发出去
                    struct framebuff *c_fb = fb_clone(fb, F_JPG << 8 | thumb_msi->stype, msi);
                    if (c_fb)
                    {
                        c_fb->datatag = thumb_msi->ramdom_id;
                        msi_output_fb(msi, c_fb);
                    }
                }
                ret = RET_ERR;
            }
            // 接收stype匹配才去保存图片,datatag作为id
            else if ((fb->mtype == F_JPG || fb->mtype == F_JPG_NODE) && (fb->stype == thumb_msi->stype))
            {
                // 唤醒线程
                os_event_set(&thumb_msi->evt, THUMB_THREAD_RUNNING, NULL);
            }
            else
            {
                ret = RET_ERR;
            }
        }
        break;
    }
    return ret;
}

struct msi *thumb_over_dpi_msi_init(const char *msi_name, uint8_t jpg_type, uint8_t stype, uint32_t magic)
{
    uint8_t             is_new;
    struct msi         *msi       = msi_new(msi_name, MAX_RECV_MAX, &is_new);
    struct thumb_msi_s *thumb_msi = NULL;
    if (is_new)
    {
        thumb_msi                  = (struct thumb_msi_s *) STREAM_LIBC_ZALLOC(sizeof(struct thumb_msi_s));
        msi->priv                  = thumb_msi;
        thumb_msi->msi             = msi;
        thumb_msi->jpg_type        = jpg_type;
        thumb_msi->magic           = magic;
        thumb_msi->stype           = stype;
        thumb_msi->normal_base_dir = IMG_PATH;
        RB_INIT(&thumb_msi->rb, MAX_THUMB_MSI);
        msi->action = thumb_msi_action;
        os_event_init(&thumb_msi->evt);
        msi->enable = 1;
        OS_TASK_INIT("thumb_msi_over_dpi", &thumb_msi->task, thumb_msi_thread, thumb_msi, OS_TASK_PRIORITY_NORMAL, NULL, 1500);
    }

    return msi;
}
#else
struct thumb_msi_s
{
    struct os_work  work;
    struct msi     *msi;
    struct os_event evt;
    const char     *normal_base_dir; // 设置如果是原图,则有个基础文件夹地址
    uint32_t        magic;
    uint8_t         ramdom_id; // 随机ID,实际最后与stype匹配用
    uint8_t         jpg_type;  // 0:是缩略图    1:原图
    uint8_t         stype;     // 设置自己的专有类型
    RBUFFER_DEF(rb, struct thumb_queue_fb, MAX_THUMB_MSI);
};

static int32_t thumb_msi_work(struct os_work *work)
{
    struct thumb_msi_s   *thumb_msi = (struct thumb_msi_s *) work;
    struct msi           *msi       = thumb_msi->msi;
    struct framebuff     *fb;
    struct thumb_queue_fb t_fb;
    uint8_t               ret;
    char               path[80];
    int32_t               res;
    uint32_t              rflags;

    rflags = 0;
    res    = os_event_wait(&thumb_msi->evt, THUMB_THREAD_RUNNING | THUMB_THREAD_EXIT, &rflags, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, -1);
    if (rflags & THUMB_THREAD_EXIT)
    {
        os_event_set(&thumb_msi->evt, THUMB_THREAD_DEADTH, NULL);
        goto thumb_msi_exit;
    }

thumb_msi_thread_get_fb_again:
    fb = msi_get_fb(msi, 0);
    // 获取到图片,判断stype与rb中的stype是否匹配
    if (fb)
    {
    thumb_msi_thread_again:
        ret = RB_INT_GET(&thumb_msi->rb, t_fb);
        if (ret)
        {
            if (fb->datatag == t_fb.datatag)
            {
                // 生成缩略图
                if (thumb_msi->jpg_type == 0)
                {
                    gen_thumb_path((char *) t_fb.path, path, sizeof(path));
                }
                else
                {
                    takephoto_name_add_dir(path, sizeof(path), (char *) t_fb.path, thumb_msi->normal_base_dir);
                }
                // 保存地址,转发出去
                struct framebuff *send_fb = fb_clone(fb, F_FILE_T << 8 | fb->stype, msi);
                ASSERT(send_fb);
                struct file_msg_s *file_msg = (struct file_msg_s *) STREAM_LIBC_ZALLOC(sizeof(struct file_msg_s));
                ASSERT(file_msg);
                os_memcpy(file_msg->path, path, strlen(path) + 1);
                if (thumb_msi->jpg_type == 0)
                {
                    file_msg->ishid = 1;
                }
                send_fb->priv = (void *) file_msg;
                msi_output_fb(msi, send_fb);
            }
            else
            {
                // 如果fb->stype > t_fb.stype?代表可能已经有漏数据,则忽略,继续寻找洗一张图
                if (fb->datatag > t_fb.datatag)
                {
                    goto thumb_msi_thread_again;
                }
                else
                {
                    os_printf("thumb_msi_thread: no match fb->datatag:%X\tt_fb.datatag:%X\n", fb->datatag, t_fb.datatag);
                }
            }
        }
        else
        {
            os_printf("thumb_msi_thread: no match fb,datatag:%X\tfb:%X\tret:%d\n", fb->datatag, fb, ret);
        }
        msi_delete_fb(NULL, fb);
        // 去缓冲区继续查找是否有图片需要保存
        goto thumb_msi_thread_get_fb_again;
    }

thumb_msi_exit:
    return 0;
}

static int32_t thumb_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t             ret       = RET_OK;
    struct thumb_msi_s *thumb_msi = (struct thumb_msi_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            // os_event_wait(&thumb_msi->evt, THUMB_THREAD_DEADTH, NULL, OS_EVENT_WMODE_OR, -1);
            os_event_del(&thumb_msi->evt);
            STREAM_LIBC_FREE(thumb_msi);
        }
        break;
        case MSI_CMD_PRE_DESTROY:
        {
            os_event_set(&thumb_msi->evt, THUMB_THREAD_EXIT, NULL);
            os_work_cancle2(&thumb_msi->work, 1);
        }
        break;

        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            // fb由内部管理,仅仅删除私有结构
            if (fb && fb->mtype == F_FILE_T)
            {
                if (fb->priv)
                {
                    STREAM_LIBC_FREE(fb->priv);
                    fb->priv = NULL;
                }
            }
        }
        break;
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            // 通过YUV去生成缩略图
            if (fb->mtype == F_YUV)
            {
                struct takephoto_yuv_arg_s *arg = (struct takephoto_yuv_arg_s *) fb->priv;
                if (!thumb_msi->magic || arg->yuv_arg.magic == thumb_msi->magic)
                {
                    // 记录path以及转发去生成真的缩略图
                    // 这里采用clone方式,将类型修改
                    thumb_msi->ramdom_id = (thumb_msi->ramdom_id + 1);
                    struct thumb_queue_fb t_fb;
                    t_fb.datatag = thumb_msi->ramdom_id;
                    t_fb.time16  = (uint16_t) (os_jiffies() & 0xffff);

                    // 配置参数,转发
                    if (arg && arg->yuv_arg.type == YUV_ARG_TAKEPHOTO)
                    {
                        os_memcpy(t_fb.path, arg->name, os_strlen(arg->name) + 1);
                        if (RB_INT_SET(&thumb_msi->rb, t_fb))
                        {
                            // 转发出去
                            struct framebuff *c_fb = fb_clone(fb, fb->mtype << 8 | thumb_msi->stype, msi);
                            if (c_fb)
                            {
                                c_fb->datatag = thumb_msi->ramdom_id;
                                msi_output_fb(msi, c_fb);
                            }
                        }
                    }
                    else
                    {
                        msi_delete_fb(NULL, fb);
                    }
                }
                // 不需要接收到队列
                ret = RET_ERR;
            }
            // 通过mjpeg去生成缩略图(需要转发到解码器去实现)
            else if (fb->mtype == F_THUMB_JPG)
            {
                // 记录path以及转发去生成真的缩略图
                // 这里采用clone方式,将类型修改
                thumb_msi->ramdom_id = (thumb_msi->ramdom_id + 1);
                struct thumb_queue_fb t_fb;
                t_fb.datatag = thumb_msi->ramdom_id;
                t_fb.time16  = (uint16_t) (os_jiffies() & 0xffff);

                if (fb->priv)
                {
                    os_memcpy(t_fb.path, fb->priv, strlen(fb->priv) + 1);
                }
                else
                {
                    os_memset(t_fb.path, 0, sizeof(t_fb.path));
                }
                if (RB_INT_SET(&thumb_msi->rb, t_fb))
                {
                    // 转发出去
                    struct framebuff *c_fb = fb_clone(fb, F_JPG << 8 | thumb_msi->stype, msi);
                    if (c_fb)
                    {
                        c_fb->datatag = thumb_msi->ramdom_id;
                        msi_output_fb(msi, c_fb);
                    }
                }
                ret = RET_ERR;
            }
            // 接收stype匹配才去保存图片,datatag作为id
            else if ((fb->mtype == F_JPG || fb->mtype == F_JPG_NODE) && (fb->stype == thumb_msi->stype))
            {
                // 唤醒线程
                // os_event_set(&thumb_msi->evt, THUMB_THREAD_RUNNING, NULL);
            }
            else
            {
                ret = RET_ERR;
            }
        }
        break;
        case MSI_CMD_TRANS_FB_END:
        {
            // 唤醒线程
            os_event_set(&thumb_msi->evt, THUMB_THREAD_RUNNING, NULL);
            os_run_work(&thumb_msi->work);
        }
        break;
    }
    return ret;
}

struct msi *thumb_over_dpi_msi_init(const char *msi_name, uint8_t jpg_type, uint8_t stype, uint32_t magic)
{
    uint8_t             is_new;
    struct msi         *msi       = msi_new(msi_name, MAX_RECV_MAX, &is_new);
    struct thumb_msi_s *thumb_msi = NULL;
    if (is_new)
    {
        thumb_msi                  = (struct thumb_msi_s *) STREAM_LIBC_ZALLOC(sizeof(struct thumb_msi_s));
        msi->priv                  = thumb_msi;
        thumb_msi->msi             = msi;
        thumb_msi->jpg_type        = jpg_type;
        thumb_msi->magic           = magic;
        thumb_msi->stype           = stype;
        thumb_msi->normal_base_dir = IMG_PATH;
        RB_INIT(&thumb_msi->rb, MAX_THUMB_MSI);
        msi->action = thumb_msi_action;
        os_event_init(&thumb_msi->evt);
        msi->enable = 1;
        OS_WORK_INIT(&thumb_msi->work, thumb_msi_work, 0);
        // OS_TASK_INIT("thumb_msi_over_dpi", &thumb_msi->task, thumb_msi_thread, thumb_msi, OS_TASK_PRIORITY_NORMAL, NULL, 1500);
    }

    return msi;
}
#endif