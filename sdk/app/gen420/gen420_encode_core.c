#include "basic_include.h"
#include "lib/multimedia/msi.h"

#include "dev/gen/hggen420.h"
#include "jpg_concat_msi.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "lib/video/gen/gen420_dev.h"
#include "stream_define.h"

#include "hal/vcodec.h"
#include "app/video_app/gen420_hardware_msi.h"
#include "lib/video/dvp/jpeg/jpg_common.h"
#include "gen420_hardware_msi.h"
#include "user_work/user_work.h"
#include "video_msi.h"
#include "gen420_encode_core.h"

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

#define MAX_GEN420_ENCODE_NUM 8

#ifdef JPG_NODE_COUNT
#define GEN420_JPG_COUNT JPG_NODE_COUNT
#else
#define GEN420_JPG_COUNT 10
#endif

#define GEN420_CHAN_NUM (2)

int32_t get_gen420_core_free_chan(struct gen420_core_s *encode, void *hd)
{
    int32_t              ret = RET_ERR;
    struct gen420_hdl_s *hdl = (struct gen420_hdl_s *) hd;
    for (int i = 0; i < GEN420_CORE_CHAN; i++)
    {
        if (encode->chan[i] == 0)
        {
            encode->chan[i] = (uint32_t) hdl;
            hdl->chan       = i;
            os_printf(KERN_DEBUG "%s chan select:%d\n", __FUNCTION__, i);
            ret = 0;
            break;
        }
    }
    return ret;
}

static int32_t gen420_core_req_done(struct vcodec_encode_req *req, int32_t status)
{
    if (!req)
    {
        return -1;
    }
    // 先hold住fb_out
    if (status == RET_OK)
    {
        fb_get(req->fb_out);
    }
    req->done(req, status);
    msi_delete_fb(NULL, req->fb);
    // 释放fb_out
    if (status == RET_OK)
    {
        fb_put(req->fb_out);
    }

    return 0;
}

static void gen420_core_realse_hdl(struct gen420_core_s *encode)
{
    struct gen420_hdl_s *hdl = encode->current_hdl;
    hdl->req                 = NULL;
    os_event_set(&hdl->evt, ENCODE_CHAN_EMPTY, NULL);
    encode->current_hdl = NULL;
}

static int32_t get_next_encode_chan(struct gen420_core_s *encode)
{
    int32_t              ret = RET_ERR;
    struct gen420_hdl_s *hdl;
    // 寻找通道,然后从对应通道获取数据
    for (int i = 0; i < GEN420_CORE_CHAN; i++)
    {
        hdl                 = (struct gen420_hdl_s *) encode->chan[encode->channel_num];
        encode->channel_num = (encode->channel_num + 1) % GEN420_CORE_CHAN;
        if (hdl)
        {
            // 如果不需要关闭,则使用这个通道尝试解码
            if (!hdl->closed)
            {
                if (hdl->req)
                {
                    encode->current_hdl = hdl;
                    ret                 = RET_OK;
                    break;
                }
                break;
            }
            else
            {
                encode->gc = 1;
            }
        }
    }
    return ret;
}

static int32_t gen420_done(void *arg, struct framebuff *fb)
{
    struct gen420_core_s *encode = (struct gen420_core_s *) arg;
    // 接收的fb记录下来,使用完毕后就需要释放
    fb_get(fb);
    encode->recv_fb = fb;
    encode->done    = 1;
    os_run_work(&encode->work);
    return 0;
}
static int32_t gen420_kick(struct gen420_msg_s *msg)
{
    int32_t               ret    = RET_OK;
    struct gen420_core_s *encode = (struct gen420_core_s *) msg->fn_data;
    // 配置一下jpg的类型
    if (encode->jpg_msi)
    {
        msi_do_cmd(encode->jpg_msi, MSI_CMD_SET_DATATAG, 0, 0);
        msi_do_cmd(encode->jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_SET_GEN420_TYPE, 0);
    }
    // 关闭jpg
    // msi_do_cmd(encode->jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 0);
    msi_do_cmd(encode->jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_NODE_COUNT, GEN420_JPG_COUNT);
    msi_do_cmd(encode->jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_SET_TIME, 0);
    // 修改jpg参数
    msi_do_cmd(encode->jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_MSG, msg->w << 16 | msg->h);
    // 修改数据源头
    msi_do_cmd(encode->jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_FROM, GEN420_DATA);

    uint32_t arg[2];
    arg[0] = (uint32_t) gen420_done;
    arg[1] = (uint32_t) encode;
    msi_do_cmd(encode->jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_SET_EXTERN_ARG, (uint32_t) arg);
    // 重新启动mjpg
    ret = msi_do_cmd(encode->jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 1);
    return ret;
}

static int32_t gen420_free(struct gen420_msg_s *msg)
{
    struct gen420_core_s *encode = (struct gen420_core_s *) msg->fn_data;
    unregister_gen420_queue(GEN420_CHAN_NUM);
    msi_do_cmd(encode->jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 0);
    if (msg->error == RET_OK)
    {
        encode->ready = 1;
    }
    else
    {
        encode->err = 1;
        os_printf(KERN_ERR "encode core err:%d\n", msg->error);
    }
    // msg->error重置
    msg->error = RET_OK;
    os_run_work(&encode->work);

    return 0;
}

static int32_t gen420_err_free(struct gen420_core_s *encode)
{
    encode->err = 1;
    os_run_work(&encode->work);
    return 0;
}

// 这里要注意,如果是正在编码的数据是不能回收的,需要等待完成才能回收
static void encode_chan_gc(struct gen420_core_s *encode)
{
    struct gen420_hdl_s *del;
    struct gen420_hdl_s *now_hdl = (struct gen420_hdl_s *) encode->current_hdl;
    // 循环检查一下其他通道是否有需要closed的通道
    for (int i = 0; i < GEN420_CORE_CHAN; i++)
    {
        del = (struct gen420_hdl_s *) encode->chan[i];
        if (del && (now_hdl != del))
        {
            if (del->closed || del->gc)
            {
                if (del->req)
                {
                    gen420_core_req_done(del->req, -1);
                    del->req = NULL;
                    os_event_set(&del->evt, ENCODE_CHAN_EMPTY, NULL);
                }
            }

            if (del->closed)
            {
                // 如果当前解码通道与closed不一样,则直接删除,否则完成后删除
                if (del != encode->current_hdl)
                {
                    os_printf(KERN_DEBUG "gen420 encode close chan:%d\n", del->chan);
                    encode->chan[i] = 0;
                    os_event_set(&del->evt, ENCODE_CHAN_DESTROY, NULL);
                    // del->fn->encode_free_hdl(encode, del);
                }
            }
        }
    }
}

static int32_t gen420_run(void *fn_data, uint8_t *encode_data, uint16_t w, uint16_t h)
{
    struct gen420_core_s *encode = (struct gen420_core_s *) fn_data;
    struct gen420_hdl_s  *hdl    = (struct gen420_hdl_s *) encode->current_hdl;
    int32_t               ret    = RET_ERR;
    if (!encode->jpg_msi)
    {
        encode->jpg_msi = jpg_concat_msi_init_start(hdl->which_jpg, w, h, NULL, GEN420_DATA, 0);
        if (encode->jpg_msi)
        {
            // 主动停止一下
            msi_do_cmd(encode->jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 0);
        }
    }
    else
    {
        // 不是gen420获取的锁,需要停止先
        if (hdl->belong == 0)
        {
            msi_do_cmd(encode->jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 0);
        }
    }
    if (encode->jpg_msi)
    {
        ret = register_gen420_queue(GEN420_CHAN_NUM, w, h, gen420_kick, gen420_free, (uint32) fn_data);
        encode->last_encode_time = os_jiffies();
        if (ret == 0)
        {
            encode->ready            = 0;
            encode->done             = 0;
            wake_up_gen420_queue(GEN420_CHAN_NUM, encode_data);
        }
    }

    if (ret != RET_OK)
    {
        gen420_err_free(encode);
    }

    return ret;
}

static int32 gen420_core_work(struct os_work *work)
{
    struct gen420_core_s     *encode = (struct gen420_core_s *) work;
    uint8_t                   unlock = 0;
    int32_t                   ret    = RET_OK;
    struct gen420_hdl_s      *hdl;
    struct vcodec_encode_req *req;
    uint8_t                   ready_kick = 1;

    if (encode->gc)
    {
        encode->gc = 0;
        // 循环检查一下其他通道是否有需要closed的通道
        encode_chan_gc(encode);
    }

    // 如果已经ready,则取检查通道,是否有需要编码的
    if (encode->ready || encode->err || encode->done || (os_jiffies() - encode->last_encode_time > 1000))
    {
        hdl = (struct gen420_hdl_s *) encode->current_hdl;
        // 超时了,需要关闭
        if (hdl && (os_jiffies() - encode->last_encode_time > 1000))
        {
            req = hdl->req;
            os_printf(KERN_ERR "ready:%d\terr:%d\tdone:%d\n", encode->ready, encode->err, encode->done);
            os_printf(KERN_ERR "Gen420 encode timeout, close it,encode time:%d\n",encode->last_encode_time);
            gen420_core_req_done(req, -1);
            gen420_core_realse_hdl(encode);
            encode->done  = 1;
            encode->ready = 1;
            encode->err   = 0;
            unlock        = 1;
        }
        else if (encode->err)
        {
            if (hdl)
            {
                req = hdl->req;
                // 无论是完成还是失败,都要释放这张图片了
                gen420_core_req_done(req, -1);
                gen420_core_realse_hdl(encode);
            }
            encode->err   = 0;
            encode->ready = 1;
            unlock        = 1;
        }
        else if (encode->current_hdl && encode->done && encode->ready)
        {
            // 当前有通道完成,则先处理通道的内容

            req = hdl->req;
            if (req)
            {
                hdl->fn->encode_done(encode, hdl);
                gen420_core_req_done(req, 0);
            }
            gen420_core_realse_hdl(encode);
            unlock = 1;
        }

        if (unlock)
        {
            if (hdl)
            {
                hdl->fn->unlock(encode, hdl);
            }
            unlock = 0;
        }

        if (!encode->current_hdl)
        {
            get_next_encode_chan(encode);
            if (!encode->current_hdl)
            {
                goto not_encode;
            }
            goto start_encode;
        }
        // 只有没有真正完成才会跑这里
        else
        {
            goto not_encode;
        }
    }
    else
    {
        goto not_encode;
    }

start_encode:
    ready_kick = 0;
    hdl        = (struct gen420_hdl_s *) encode->current_hdl;
    if (!hdl)
    {
        goto not_encode;
    }

    ret = hdl->fn->pre_encode(encode, hdl);
    if (ret)
    {
        goto not_encode;
    }

    // 获取锁
    if (hdl)
    {
        ret = hdl->fn->lock(encode, hdl);
    }
    if (ret)
    {
        goto not_encode;
    }
    req = hdl->req;
    // 注册到g420,然后去kick
    os_printf(KERN_DEBUG "hdl->w:%d\thdl->h:%d\thdl->fn:%X\n", hdl->w, hdl->h, hdl->fn);
    gen420_run((void *) encode, req->fb->data, hdl->w, hdl->h);
    ready_kick = 1;
not_encode:
    // 没有kick,可能获取不到锁
    if (!ready_kick)
    {
        encode->current_hdl = NULL;
    }
    if (unlock)
    {
        if (hdl)
        {
            hdl->fn->unlock(encode, hdl);
        }
    }
    os_run_work_delay(work, 1);
    return 0;
}

static int32_t gen420_core_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t               ret    = RET_OK;
    struct gen420_core_s *encode = (struct gen420_core_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            os_event_del(&encode->evt);
            STREAM_LIBC_FREE(encode);
        }
        break;

        case MSI_CMD_PRE_DESTROY:
        {
            os_work_cancle2(&encode->work, 1);
            // 正常这里一定不会有hdl,如果有,应该是有问题
            ASSERT(!encode->current_hdl);
        }
        break;
        default:
            break;
    }
    return ret;
}

struct msi *gen420_encode_core(const char *msi_name)
{
    uint8_t               isnew;
    struct msi           *msi = msi_new(msi_name, 0, &isnew);
    struct gen420_core_s *gen420_core;
    if (isnew)
    {
        gen420_core        = (struct gen420_core_s *) STREAM_LIBC_ZALLOC(sizeof(struct gen420_core_s));
        gen420_core->ready = 1;
        gen420_core->done  = 1;
        os_event_init(&gen420_core->evt);
        msi->priv              = (void *) gen420_core;
        msi->fb_limits.counter = MAX_GEN420_ENCODE_NUM;
        msi->action            = gen420_core_action;
        msi->enable            = 1;
        // 启动workqueue
        OS_WORK_INIT(&gen420_core->work, gen420_core_work, 0);
        os_run_work(&gen420_core->work);
    }
    return msi;
}