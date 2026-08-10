#include "basic_include.h"
#include "video_app_csc_msi.h"
#include "stream_define.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/multimedia/msi.h"
#include "user_work/user_work.h"

// data申请空间函数
#define STREAM_MALLOC                   av_psram_malloc
#define STREAM_FREE                     av_psram_free
#define STREAM_ZALLOC                   av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC              av_malloc
#define STREAM_LIBC_FREE                av_free
#define STREAM_LIBC_ZALLOC              av_zalloc

#define CSC_MSI_RX_NUM                  2
#define CSC_MSI_TX_NUM                  2

struct video_app_csc_priv
{
    struct os_work          work;
    struct csc_device*      csc_dev;
    struct msi*             msi;
    struct fbpool           tx_pool;
    uint8_t                 hardware_ready : 1,
                            wait_irq_done  : 1,
                            rev : 6;
    uint32_t                input_format;
    uint32_t                output_format;
    uint32_t                width;
    uint32_t                height;
    struct framebuff*       current_rx_fb;
    struct framebuff*       current_tx_fb;
};
typedef struct video_app_csc_priv* video_app_csc_priv_t;

static int32_t video_app_csc_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    video_app_csc_priv_t csc_priv = (video_app_csc_priv_t)msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            if(csc_priv) {
                struct framebuff *fb = NULL;
                while (1)
                {
                    fb = fbpool_get(&csc_priv->tx_pool, 0, NULL);
                    if (!fb) {
                        break;
                    }
                    if (fb->data) {
                        STREAM_FREE(fb->data);
                        fb->data = NULL;
                    }
                    if (fb->priv) {
                        STREAM_LIBC_FREE(fb->priv);
                        fb->priv = NULL;
                    }
                }

                fbpool_destroy(&csc_priv->tx_pool);
                STREAM_LIBC_FREE(csc_priv);
                msi->priv = NULL;
            }
        }
        break;

        case MSI_CMD_PRE_DESTROY:
        {
            os_work_cancle2(&csc_priv->work, 1);
        }
        break;

        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;
            if (fb->srcID != FRAMEBUFF_SOURCE_CSC) {
                ret = RET_OK + 1;
            }
        }
        break;

        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;
            fbpool_put(&csc_priv->tx_pool, fb);
            // 不需要内核去释放fb
            ret = RET_OK + 1;
        }
        break;

        case MSI_CMD_CSC:
        {
            uint32_t cmd_self = (uint32_t)param1;
            uint32_t arg = param2;
            switch (cmd_self)
            {
                case MSI_CSC_MSI_ENABLE:
                {
                    csc_priv->msi->enable = arg;
                }
                break;
            }
        }
    }
    return ret;
}

static void csc_irq_done(uint32 irq_flag, uint32 irq_data, uint32 param1)
{
    video_app_csc_priv_t csc_priv = (video_app_csc_priv_t)irq_data;
    csc_priv->hardware_ready = 1;
}

static int32 video_app_csc_msi_work(struct os_work *work)
{
    video_app_csc_priv_t csc_priv           = (video_app_csc_priv_t)work;

    if (csc_priv->hardware_ready) {

        if (csc_priv->wait_irq_done) {
            csc_priv->current_tx_fb->time = os_jiffies();
            msi_output_fb(csc_priv->msi, csc_priv->current_tx_fb, 0);
            msi_delete_fb(csc_priv->msi, csc_priv->current_rx_fb);
            csc_priv->current_tx_fb         = NULL;
            csc_priv->current_rx_fb         = NULL;
            csc_priv->wait_irq_done         = 0;
        }

        if (!csc_priv->current_rx_fb) {
            csc_priv->current_rx_fb         = msi_get_fb(csc_priv->msi, 0);
            if (!csc_priv->current_rx_fb) {
                goto __video_app_csc_msi_work_end;
            }
        }
    
        if (csc_priv->current_rx_fb) {
    
            if (!csc_priv->current_tx_fb) {
                csc_priv->current_tx_fb     = fbpool_get(&csc_priv->tx_pool, 0, csc_priv->msi);
                if (!csc_priv->current_tx_fb) {
                    goto __video_app_csc_msi_work_end;
                }
            }
    
            uint8_t *csc_input_addr         = csc_priv->current_rx_fb->data;
            uint8_t *csc_output_addr        = csc_priv->current_tx_fb->data;

            csc_priv->hardware_ready        = 0;
            csc_priv->wait_irq_done         = 1;
            csc_priv->current_tx_fb->mtype  = F_YUV;
            csc_priv->current_tx_fb->stype  = FSTYPE_YUV_P0;
            csc_priv->current_tx_fb->srcID  = FRAMEBUFF_SOURCE_CSC;

            struct yuv_arg_s * yuv_msg      = csc_priv->current_tx_fb->priv;
            
            yuv_msg->y_size                 = csc_priv->width*csc_priv->height;
            yuv_msg->uv_off                 = 0;
            yuv_msg->y_off                  = 0;
            yuv_msg->x                      = 0;
            yuv_msg->y                      = 0;
            yuv_msg->out_w                  = csc_priv->width;
            yuv_msg->out_h                  = csc_priv->height;
            
            csc_init(csc_priv->csc_dev);
            csc_set_type(csc_priv->csc_dev, 2);                                                     /* RGB -> YUV */
            csc_set_photo_size(csc_priv->csc_dev, csc_priv->width, csc_priv->height);
            csc_set_format(csc_priv->csc_dev, csc_priv->input_format, csc_priv->output_format);     /* RGB565 -> YUV420P */
            csc_set_input_addr(csc_priv->csc_dev, (uint32_t)csc_input_addr, 0, 0);
            csc_set_output_addr(csc_priv->csc_dev,                                                  \
                                (uint32_t)(csc_output_addr),                                          \
                                (uint32_t)(csc_output_addr + csc_priv->width * csc_priv->height),   \
                                (uint32_t)(csc_output_addr + csc_priv->width * csc_priv->height + csc_priv->width * csc_priv->height / 4));
            csc_start_run(csc_priv->csc_dev);
        }
    } else {
        /* 转换未完成，直接退出 */
    }

__video_app_csc_msi_work_end:
    os_run_work_delay(&csc_priv->work, 1);
    return 0;
}

/* 目前 CSC 数据流仅用在 LVGL 由 RGB565 -> YUV420P -> VIDEO P0 的通路上 */
void video_app_csc_msi_init(const char* csc_msi_name, uint32_t input_format, uint32_t output_format, uint32_t width, uint32_t height)
{
    struct msi *msi = NULL;

video_app_csc_msi_init_start:
    msi = msi_new(csc_msi_name, CSC_MSI_RX_NUM, NULL);
    if (!msi) {
        return ;
    }

    video_app_csc_priv_t csc_priv = (video_app_csc_priv_t)msi->priv;

    if (!csc_priv) 
    {
        csc_priv                    = (video_app_csc_priv_t)STREAM_LIBC_ZALLOC(sizeof(struct video_app_csc_priv));
        msi->priv                   = (void *)csc_priv;
        msi->action                 = (msi_action)video_app_csc_msi_action;

        csc_priv->csc_dev           = (struct csc_device *)dev_get(HG_CSC_DEVID);
        csc_priv->msi               = msi;
        csc_priv->input_format      = input_format;
        csc_priv->output_format     = output_format;
        csc_priv->width             = width;
        csc_priv->height            = height;
        csc_priv->hardware_ready    = 1;
        fbpool_init(&csc_priv->tx_pool, CSC_MSI_TX_NUM, NULL, NULL);

        for(int i = 0; i < CSC_MSI_TX_NUM; i++) {
            /* 预分配 CSC 输出的 YUV420P 格式的空间 */
            uint8_t *csc_output_addr = (uint8_t*)STREAM_MALLOC(csc_priv->width*csc_priv->height*3/2);
            if (!csc_output_addr) {
                return ;
            }
            sys_dcache_invalid_range((uint32_t*)csc_output_addr, csc_priv->width*csc_priv->height*3/2);

            struct yuv_arg_s * yuv_msg = (struct yuv_arg_s *)STREAM_LIBC_ZALLOC(sizeof(struct yuv_arg_s));
            if (!yuv_msg) {
                return ;
            }

            FBPOOL_SET_INFO(&csc_priv->tx_pool, i, csc_output_addr, csc_priv->width*csc_priv->height*3/2, yuv_msg);
        }

        csc_request_irq(csc_priv->csc_dev, CSC_DONE_IRQ, (csc_irq_hdl )&csc_irq_done, (uint32)csc_priv);

        OS_WORK_INIT(&csc_priv->work, video_app_csc_msi_work, 0);
        os_run_work_delay(&csc_priv->work, 1);

        msi->enable                 = 1;
    }
    else 
    {
        msi->name = NULL;
        msi_destroy(msi);
        os_sleep_ms(1);
        goto video_app_csc_msi_init_start;
    }

}