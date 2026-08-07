/**********************************************************************************************************
 * 虚拟的P0和P1,使用dma2d去实现p0和p1的叠层,最终目的是为了让lcd不旋转(省sram),直接预先旋转叠层给到video显示
 * ********************************************************************************************************/
#include "basic_include.h"

#include "lib/multimedia/msi.h"
#include "osal/work.h"
#include "stream_define.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"


extern void yuv_blk_cpy(uint8 *des, uint8* src,uint32 des_w,uint32 des_h, uint32 src_w, uint32_t src_h,uint32 x,uint32 y);

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE av_free
#define STREAM_LIBC_ZALLOC av_zalloc

#define MAX_RX 8

struct sim_video_msi_s
{
    struct os_work work;
    struct msi *msi;
    struct fbpool tx_pool;
    struct framebuff *p0;
    struct framebuff *p1;
    uint16_t w, h;
    uint16_t *filter;   //数组去进行过滤,0为结束
    // 设置哪个在顶层,这个模块只有P0和P1
    uint8_t top;
};

static int32 sim_video_work(struct os_work *work)
{
    struct sim_video_msi_s *sim_video = (struct sim_video_msi_s *)work;
    struct framebuff *fb;
    struct framebuff *output_fb;
    uint32_t *buf;
    fb = msi_get_fb(sim_video->msi, 0);
    // 接收到一个fb,就需要判断类型(p0,p1),然后申请空间去进行叠层
    if (fb)
    {
        if (fb->stype == FSTYPE_YUV_P0)
        {
            if (sim_video->p0)
            {
                msi_delete_fb(NULL, sim_video->p0);
            }
            sim_video->p0 = fb;
        }
        else if (fb->stype  == FSTYPE_YUV_P1)
        {
            if (sim_video->p1)
            {
                msi_delete_fb(NULL, sim_video->p1);
            }
            sim_video->p1 = fb;
        }
        else
        {
            msi_delete_fb(NULL, fb);
            fb = NULL;
            goto sim_video_work_end;
        }

        // 不需要叠层,帮忙转发就好了,但是需要保留p0或者p1
        // 因为output会发送完毕后会执行删除,所以要保留,不能被删除
        if (fb && !(sim_video->p0 && sim_video->p1))
        {
            // 克隆一个fb发出去,但是因为这个sim_video是给到P0显示,所以需要将stype转换成FSTYPE_YUV_P0
            output_fb = fb_clone(fb, fb->mtype<<8|FSTYPE_YUV_P0, sim_video->msi);
            if (output_fb)
            {
                msi_output_fb(sim_video->msi, output_fb);
            }

            goto sim_video_work_end;
        }
        //os_printf("sim video p0:%X\tp1:%X\n",sim_video->p0,sim_video->p1);
        // 开始申请空间
        buf = (uint32_t *)STREAM_MALLOC(sim_video->w * sim_video->h * 3 / 2);
        if (!buf)
        {
            goto sim_video_work_end;
        }

        output_fb = fbpool_get(&sim_video->tx_pool, 0, sim_video->msi);
        if (!output_fb)
        {
            STREAM_FREE(buf);
            goto sim_video_work_end;
        }
        output_fb->data = (uint8_t*)buf;
        output_fb->len = sim_video->w * sim_video->h * 3 / 2;

        // 叠层
        {
            uint16_t p0_w= 0, p0_h = 0, p1_w = 0, p1_h = 0;
            struct yuv_arg_s *yuv_msg_p0 = NULL;
            struct yuv_arg_s *yuv_msg_p1 = NULL;
            if (sim_video->p0)
            {
                yuv_msg_p0 = (struct yuv_arg_s *)sim_video->p0->priv;
                p0_w = yuv_msg_p0->out_w;
                p0_h = yuv_msg_p0->out_h;
            }
            if (sim_video->p1)
            {
                yuv_msg_p1 = (struct yuv_arg_s *)sim_video->p1->priv;
                p1_w = yuv_msg_p1->out_w;
                p1_h = yuv_msg_p1->out_h;
            }

            // 判断一下p0_w和p1_w是否和当前的w,h匹配,不匹配,可能底部需要配置颜色才可以
            uint16_t p_w = p0_w > p1_w ? p0_w : p1_w;
            uint16_t p_h = p0_h > p1_h ? p0_h : p1_h;
            // 如果没有匹配,设置颜色
            if (!(p_w >= sim_video->w && p_h >= sim_video->h))
            {
                hw_memset((uint8_t*)buf, 0, sim_video->w * sim_video->h);
                hw_memset((uint8_t*)buf+sim_video->w * sim_video->h,0x80,sim_video->w * sim_video->h / 2);
                sys_dcache_clean_range(buf, sim_video->w * sim_video->h * 3 / 2);
                sys_dcache_clean_invalid_range(buf, sim_video->w * sim_video->h * 3 / 2);
            }


            //os_printf("yuv_msg_p1->x:%d\ty:%d\n",yuv_msg_p1->x,yuv_msg_p1->y);
            //os_printf("yuv_msg_p0->x:%d\ty:%d\n",yuv_msg_p0->x,yuv_msg_p0->y);
            if (sim_video->top == 0)
            {

                if (yuv_msg_p1)
                {
                    yuv_blk_cpy((uint8_t*)buf, sim_video->p1->data, sim_video->w, sim_video->h, p1_w, p1_h, yuv_msg_p1->x, yuv_msg_p1->y);
                }
                if (yuv_msg_p0)
                {
                    yuv_blk_cpy((uint8_t*)buf, sim_video->p0->data, sim_video->w, sim_video->h, p0_w, p0_h, yuv_msg_p0->x, yuv_msg_p0->y);
                }
            }
            else
            {
                if (yuv_msg_p0)
                {
                    yuv_blk_cpy((uint8_t*)buf, sim_video->p0->data, sim_video->w, sim_video->h, p0_w, p0_h, yuv_msg_p0->x, yuv_msg_p0->y);
                }

                if (yuv_msg_p1)
                {
                    yuv_blk_cpy((uint8_t*)buf, sim_video->p1->data, sim_video->w, sim_video->h, p1_w, p1_h, yuv_msg_p1->x, yuv_msg_p1->y);
                }
            }
            output_fb->mtype = F_YUV;
            output_fb->stype = FSTYPE_YUV_P0;
            msi_output_fb(sim_video->msi, output_fb);
        }
    }
sim_video_work_end:
    os_run_work_delay(work, 1);
    return 0;
}
static int sim_video_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    struct sim_video_msi_s *sim_video = (struct sim_video_msi_s *)msi->priv;
    int ret = RET_OK;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            struct framebuff *fb;
            // 释放资源fb资源文件,priv是独立申请的
            while (1)
            {
                fb = fbpool_get(&sim_video->tx_pool, 0, NULL);
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
            fbpool_destroy(&sim_video->tx_pool);
            STREAM_LIBC_FREE(sim_video);
        }
        break;
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;
            
            if (fb->mtype != F_YUV)
            {
                //os_printf("not recv fb:%X\tcount:%d\n",fb,fb->users.counter);
                ret = RET_ERR;
            }
            else
            {
                if(sim_video->filter)
                {
                    //轮询类型是否有一致
                    ret = RET_ERR;
                    uint16_t *each  = sim_video->filter;
                    while(*each)
                    {
                        //如果一致,返回OK
                        if(*each == fb->stype)
                        {
                            ret = RET_OK;
                            break;
                        }
                        each++;
                    }
                }
            }
            
        }
        break;

        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;
            // 如果不是克隆的,就自己释放
            if (!fb->clone)
            {
                if (fb->data)
                {
                    STREAM_FREE(fb->data);
                    fb->data = NULL;
                }
                fbpool_put(&sim_video->tx_pool, fb);
                // 不需要内核去释放fb
                ret = RET_OK + 1;
            }
        }
        break;
        case MSI_CMD_PRE_DESTROY:
        {
            os_work_cancle2(&sim_video->work, 1);
            if (sim_video->p0)
            {
                msi_delete_fb(NULL, sim_video->p0);
                sim_video->p0 = NULL;
            }
            if (sim_video->p1)
            {
                msi_delete_fb(NULL, sim_video->p1);
                sim_video->p1 = NULL;
            }
        }
        break;
        default:
        {
        }
        break;
    }

    return ret;
}

struct msi *sim_video_msi(const char *name, uint16_t w, uint16_t h, uint8_t top)
{
    static const uint16_t sim_filter[3] = {FSTYPE_YUV_P0,FSTYPE_YUV_P1,FSTYPE_NONE};
    // 设置1个接收,只是为了可以output_fb给到这个msi
    struct msi *msi = msi_new(name, MAX_RX, NULL);
    if (msi)
    {
        struct sim_video_msi_s *sim_video = (struct sim_video_msi_s *)msi->priv;
        if (!sim_video)
        {
            sim_video = (struct sim_video_msi_s *)STREAM_LIBC_ZALLOC(sizeof(struct sim_video_msi_s));
        }
        sim_video->p0 = NULL;
        sim_video->p1 = NULL;
        sim_video->w = w;
        sim_video->h = h;
        sim_video->msi = msi;
        sim_video->filter = (uint16_t*)sim_filter;
        sim_video->top = top;
        msi->priv = (void *)sim_video;
        msi->action = sim_video_msi_action;

        // 预分配结构体空间
        uint32_t init_count = 0;
        void *priv;
        fbpool_init(&sim_video->tx_pool, MAX_RX);
        while (init_count < MAX_RX)
        {
            priv = (void *)STREAM_LIBC_ZALLOC(sizeof(struct yuv_arg_s));
            struct yuv_arg_s *yuv_msg = (struct yuv_arg_s *)priv;
            yuv_msg->out_w = w;
            yuv_msg->out_h = h;
            yuv_msg->y_size = w * h;
            yuv_msg->y_off = 0;
            yuv_msg->uv_off = 0;
            yuv_msg->del = NULL;
            FBPOOL_SET_INFO(&sim_video->tx_pool, init_count, NULL, 0, priv);
            init_count++;
        }

        msi->enable = 1;

        //msi_add_output(sim_video->msi, NULL, R_VIDEO_P0);

        OS_WORK_INIT(&sim_video->work, sim_video_work, 0);
        os_run_work_delay(&sim_video->work, 1);
    }
    return msi;
}
