
#include "basic_include.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/multimedia/msi.h"
#include "user_work/user_work.h"
#include "lib/scale/scale_common.h"
#include "hal/vcodec.h"
#include "multimedia/video.h"
#include "stream_define.h"
#include "decode_mem.h"
#include "decode_common.h"
#include "dev/scale/hgscale.h"

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

void decode_realse_hdl(struct decode_msi_s *decode)
{
    struct common_decode_hdl *hdl = decode->current_hdl;
    hdl->decode_addr              = NULL;
    hdl->req                      = NULL;
    os_event_set(&hdl->evt, DECODE_CHAN_EMPTY, NULL);
    decode->current_hdl = NULL;
}

// 获取空闲的通道
int32_t get_free_chan(struct decode_msi_s *decode, void *hd)
{
    int32_t                   ret = RET_ERR;
    struct common_decode_hdl *hdl = (struct common_decode_hdl *) hd;
    for (int i = 0; i < MAX_DECODE_CHANNEL; i++)
    {
        if (decode->chan[i] == 0)
        {
            decode->chan[i] = (uint32_t) hdl;
            hdl->chan       = i;
            ret             = 0;
            os_printf(KERN_DEBUG "decode channel select:%d\n", i);
            break;
        }
    }
    return ret;
}

// 先hold住fb_out,可以保证流程完成再释放fb_out
int32_t decode_req_done(struct vcodec_decode_req *req, int32_t status)
{
    if (!req)
    {
        return -1;
    }
    // 先hold住fb_out
    fb_get(req->fb_out);
    req->done(req, status);
    msi_delete_fb(NULL, req->fb);
    // 释放fb_out
    fb_put(req->fb_out);
    return 0;
}

static void decode_realse_space(struct decode_msi_s *decode)
{

    if (decode->scaler2buf_y)
    {
        STREAM_LIBC_FREE(decode->scaler2buf_y);
        decode->scaler2buf_y = NULL;
    }

    if (decode->scaler2buf_u)
    {
        STREAM_LIBC_FREE(decode->scaler2buf_u);
        decode->scaler2buf_u = NULL;
    }

    if (decode->scaler2buf_v)
    {
        STREAM_LIBC_FREE(decode->scaler2buf_v);
        decode->scaler2buf_v = NULL;
    }
    decode->now_decode_pw = 0;
}

static int32_t decode_buf_ready(struct decode_msi_s *decode)
{
    struct common_decode_hdl *hdl = (struct common_decode_hdl *) decode->current_hdl;
    int32_t                   err = 0;
    if (hdl->dw > decode->now_decode_pw)
    {
        // 先移除旧buf的空间
        decode_realse_space(decode);
        uint8_t  scale_coeff = 0;
        uint16_t iw          = hdl->w;
        uint16_t ow          = hdl->dw;
        if (iw >= ow)
        {
            scale_coeff = 1;
        }
        else
        {
            scale_coeff = 2;
        }

        // 默认一定申请到,没有做申请失败的处理
        decode->scaler2buf_y = STREAM_LIBC_MALLOC(0x20 + ow + scale_coeff * 20 * DECODE_SRAMBUF_WLEN * 4 + 256);
        decode->scaler2buf_u = STREAM_LIBC_MALLOC(((0x12 + ow / 2 + scale_coeff * 11 * DECODE_SRAMBUF_WLEN * 2 + 128 + 3) / 4) * 4);
        decode->scaler2buf_v = STREAM_LIBC_MALLOC(((0x12 + ow / 2 + scale_coeff * 11 * DECODE_SRAMBUF_WLEN * 2 + 128 + 3) / 4) * 4);
        if (!decode->scaler2buf_y || !decode->scaler2buf_u || !decode->scaler2buf_v)
        {
            os_printf(KERN_ERR "%s:%d err\tpw:%X\tnow_pw:%X\n", __FUNCTION__, __LINE__, hdl->dw, decode->now_decode_pw);
            os_printf(KERN_ERR "fail y:%X\tu:%X\tv:%X\n", decode->scaler2buf_y, decode->scaler2buf_u, decode->scaler2buf_v);
            err = 1;
            // 申请失败,则移除已经申请的空间
            decode_realse_space(decode);
        }
        else
        {
            decode->now_decode_pw = hdl->dw;
        }
    }
    return err;
}

static int32_t get_next_decode_chan(struct decode_msi_s *decode)
{
    int32_t                   ret = RET_ERR;
    struct common_decode_hdl *hdl;
    // 寻找通道,然后从对应通道获取数据
    for (int i = 0; i < MAX_DECODE_CHANNEL; i++)
    {
        hdl                 = (struct common_decode_hdl *) decode->chan[decode->channel_num];
        decode->channel_num = (decode->channel_num + 1) % MAX_DECODE_CHANNEL;
        if (hdl)
        {
            // 如果不需要关闭,则使用这个通道尝试解码
            if (!hdl->closed)
            {
                if (hdl->req)
                {
                    decode->current_hdl = hdl;
                    ret                 = RET_OK;
                    break;
                }
                break;
            }
            else
            {
                decode->gc = 1;
            }
        }
    }
    return ret;
}

static void decode_chan_gc(struct decode_msi_s *decode)
{
    struct common_decode_hdl *del;
    // 循环检查一下其他通道是否有需要closed的通道
    for (int i = 0; i < MAX_DECODE_CHANNEL; i++)
    {
        del = (struct common_decode_hdl *) decode->chan[i];
        if (del)
        {
            if (del->closed || del->gc)
            {
                if (del->req)
                {
                    decode_req_done(del->req, -1);
                    del->req = NULL;
                    os_event_set(&del->evt, DECODE_CHAN_EMPTY, NULL);
                }
            }

            if (del->closed)
            {
                // 如果当前解码通道与closed不一样,则直接删除,否则完成后删除
                if (del != decode->current_hdl)
                {
                    os_printf(KERN_DEBUG "decode close chan:%d\n", del->chan);
                    decode->chan[i] = 0;
                    del->fn->decode_free_hdl(decode, del);
                }
            }
        }
    }
}

static int32 decode_core_work(struct os_work *work)
{
    int                       ret;
    uint32_t                  unlock = 0;
    struct decode_msi_s      *decode = (struct decode_msi_s *) work;
    struct vcodec_decode_req *req;
    struct common_decode_hdl *hdl;
    struct common_decode_hdl *del;
    uint8_t                  *decode_buf;
    uint8_t                   ready_kick = 1;
    // 通道检查,清空通道数据,公用部分
    if (decode->gc)
    {
        decode->gc = 0;
        // 循环检查一下其他通道是否有需要closed的通道
        decode_chan_gc(decode);
    }

    // 检测解码模块是否完成或者超时代表解码失败
    // 能进去,模块需要reset或者已经解码完毕,可以重新去解码,否则只能慢慢等超时或者硬件解码完成
    if (decode->hardware_err || decode->hardware_ready || os_jiffies() - decode->last_decode_time > 1000)
    {
        hdl = decode->current_hdl;
        del = hdl;
        if (decode->current_hdl && (os_jiffies() - decode->last_decode_time > 1000))
        {
            // 解码可能失败了
            os_printf(KERN_ERR "decode failed1:%d\n", decode->hardware_ready);
            os_printf(KERN_ERR "decode->hardware_err:%d\n", decode->hardware_err);

            hdl->sps_flag = 0;

            decode->hardware_ready = 1;
            // 无论是完成还是失败,都要释放这张图片了
            decode_req_done(hdl->req, -1);
            decode_mem_free(hdl->decode_addr);
            decode_realse_hdl(decode);

            // 这里最好将硬件模块停止
            // 解锁
            unlock = 1;
        }
        // 解码异常
        else if (decode->hardware_err)
        {
            if (hdl)
            {
                req = hdl->req;
                // 无论是完成还是失败,都要释放这张图片了
                decode_req_done(req, -1);
                decode_mem_free(hdl->decode_addr);
                decode_realse_hdl(decode);
            }
            _os_printf(KERN_ERR "decode failed2\n");
            decode->hardware_err   = 0;
            decode->hardware_ready = 1;

            // 解锁
            unlock = 1;
        }
        // 解码完成,检查有解码完的数据需要发送
        else if (decode->current_hdl)
        {
            hdl->fn->decode_done(decode, hdl);
            req = hdl->req;
            if (req)
            {
                if (req->fb_out)
                {
                    req->fb_out->len  = hdl->decode_size;
                    req->fb_out->data = hdl->decode_addr;
                }
                decode_req_done(req, 0);
            }
            // 没有req,但是依然解码完成了,需要释放解码的内存空间
            else
            {
                decode_mem_free(hdl->decode_addr);
            }
            _os_printf("&");
            decode_realse_hdl(decode);
            decode->is_register_isr = 0;
            // 解锁
            unlock                  = 1;
        }
        // 没有需要解码的图片
        else
        {
        }
        if (unlock)
        {
            if (hdl)
            {
                hdl->fn->unlock(decode, hdl);
            }
            // 解锁后的回调函数
            scale_mutex_unlock(2, SCALE_DECODE_LOCK);
            unlock         = 0;
            decode->unlock = 0;
        }

        // 这里没有解码的图片,尝试去看看有没有需要解码图片
        if (!decode->current_hdl)
        {
            get_next_decode_chan(decode);
            // 不需要解码
            if (!decode->current_hdl)
            {
                // decode_realse_space(decode);
                goto not_decode;
            }
        }
        else
        {
            goto start_decode;
        }

    // 开始尝试解码
    // 只要解码,就应该有req
    start_decode:
        if(!decode->scale_dev)
        {
            decode->scale_dev = (struct scale_device *) dev_get(HG_SCALE2_DEVID);
        }
        // 已经可以kick了,先清空ready_kick,等待kick启动后在置位
        ready_kick = 0;
        hdl        = decode->current_hdl;
        uint32_t need_size;
        // 运行到这里,应该是一定存在的
        if (!hdl)
        {
            goto not_decode;
        }
        ret = hdl->fn->pre_decode(decode, hdl);
        if (ret)
        {
            goto not_decode;
        }

        need_size = hdl->fn->get_decode_size(decode, hdl);
        // 先确定linebuf是否足够
        ret       = decode_buf_ready(decode);
        if (ret)
        {
            goto not_decode;
        }

        // 获取锁
        if (hdl)
        {
            ret = hdl->fn->lock(decode, hdl);
        }

        if (ret)
        {
            goto not_decode;
        }

        // lock的函数,返回错误则退出
        ret |= scale_mutex_lock(2, SCALE_DECODE_LOCK, NULL);
        if (ret)
        {
            goto not_decode;
        }
        decode->unlock = 1;

        // 计算解码后的size空间
        decode_buf = decode_mem_malloc(decode->mem_info, decode->mem_info_size, need_size, STREAM_MALLOC);
        // 申请失败
        if (!decode_buf)
        {
            // 解锁
            unlock = 1;
        }
        else
        {
            hdl->decode_addr = (uint8_t *) decode_buf;
            hdl->decode_size = need_size;
            // 申请一个fb结构体,ready的回调函数
            hdl->fn->decode_ready(decode, hdl);
            if (!decode->hardware_err)
            {
                // kick的回调函数
                hdl->fn->decode_kick(decode, hdl);
            }

            decode->last_decode_time = os_jiffies();
            ready_kick               = 1;
        }
    }
not_decode:
    // 不需要解码,移除空间
    if (!decode->current_hdl)
    {
        decode_realse_space(decode);
    }
    if (unlock)
    {
        if (hdl)
        {
            hdl->fn->unlock(decode, hdl);
        }
        // 解锁的回调函数
        scale_mutex_unlock(2, SCALE_DECODE_LOCK);
        decode->unlock = 0;
    }
    // 没有kick,可能获取不到锁
    if (!ready_kick)
    {
        decode->current_hdl = NULL;
    }
    decode_mem_check(decode->mem_info, decode->mem_info_size, MAX_TTL, STREAM_FREE);
    // 硬件ready,检测是否超时,超时可以释放一下rom的空间
    if (decode->hardware_ready)
    {
        if (decode->rom && (os_jiffies() - decode->rom_last_use_time))
        {
            STREAM_FREE(decode->rom);
            decode->rom          = NULL;
            decode->rom_max_size = 0;
        }
    }
    os_run_work_delay(work, 1);
    return 0;
}

static int decode_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int                  ret    = RET_OK;
    struct decode_msi_s *decode = (struct decode_msi_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            struct common_decode_hdl *del;
            for (int i = 0; i < MAX_DECODE_CHANNEL; i++)
            {
                del = (struct common_decode_hdl *) decode->chan[i];
                if (del)
                {
                    decode->chan[i] = 0;
                    del->fn->decode_free_hdl(decode, del);
                }
            }

            // uint8_t *ref_mem = hdl->ref_mem;
            if (decode->unlock)
            {
                scale_mutex_unlock(2, SCALE_DECODE_LOCK);
                decode->unlock = 0;
            }
            decode_mem_free_all(decode->mem_info, decode->mem_info_size, STREAM_FREE);

            if (decode->rom)
            {
                STREAM_FREE(decode->rom);
                decode->rom = NULL;
            }
            decode_realse_space(decode);
            os_free(decode);
        }
        break;

        // 停止硬件
        case MSI_CMD_PRE_DESTROY:
        {
            os_work_cancle2(&decode->work, 1);
            // 关闭硬件
            scale_close(decode->scale_dev);
            struct common_decode_hdl *hdl = decode->current_hdl;
            if (hdl)
            {
                hdl->fn->unlock(decode, hdl);
                decode_req_done(hdl->req, -1);
                decode_realse_hdl(decode);
                decode_mem_free(hdl->decode_addr);
                hdl->fn->decode_free_hdl(decode, hdl);
            }
        }
        break;
        default:
            break;
    }

    return ret;
}

struct msi *decode_core_msi(const char *name, uint8_t info_size)
{
    uint8_t              is_new;
    struct msi          *msi    = msi_new(name, 0, &is_new);
    struct decode_msi_s *decode = (struct decode_msi_s *) msi->priv;
    if (is_new)
    {
        if (!info_size)
        {
            info_size = MIN_MEM_INFO;
        }
        decode                = (struct decode_msi_s *) os_zalloc(sizeof(struct decode_msi_s) + sizeof(struct mem_info *) * info_size);
        decode->msi           = msi;
        msi->priv             = (void *) decode;
        msi->action           = decode_msi_action;                 // 限制申请的数量
        decode->mem_info      = (struct mem_info **) (decode + 1); // 放到结构体的后面
        decode->mem_info_size = info_size;

        //decode->scale_dev       = (struct scale_device *) dev_get(HG_SCALE2_DEVID);
        decode->scaler2buf_y    = NULL;
        decode->scaler2buf_u    = NULL;
        decode->scaler2buf_v    = NULL;
        decode->now_decode_pw   = 0;
        decode->hardware_ready  = 1;
        decode->auto_free_space = 1;
        msi->enable             = 1;
        OS_WORK_INIT(&decode->work, decode_core_work, 0);
        os_run_work_delay(&decode->work, 1);
    }
    return msi;
}

// 通用的中断函数,如果有特殊,在各自解码去独立实现,这里是给通用的
int32_t general_scale2_done(uint32 irq_flag, uint32 irq_data, uint32 param1)
{
    return RET_OK;
}

int32_t general_scale2_ov(uint32 irq_flag, uint32 irq_data, uint32 param1)
{
    return RET_OK;
}

int32_t general_decode_err(uint32 irq_flag, uint32 irq_data, uint32 param1, uint32 param2)
{
    struct decode_msi_s *decode = (struct decode_msi_s *) irq_data;
    os_printf("decode err\r\n");
    decode->hardware_err = 1;
    return RET_OK;
}
int32_t general_decode_done(uint32 irq_flag, uint32 irq_data, uint32 param1, uint32 param2)
{
    struct decode_msi_s *decode = (struct decode_msi_s *) irq_data;
    decode->hardware_ready      = 1;
    return RET_OK;
}