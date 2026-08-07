#include "stream_frame.h"
#include "osal/work.h"
#include "basic_include.h"

#include "lib/multimedia/msi.h"

#include "hal/osd_enc.h"
#include "dev/osd_enc/hgosd_enc.h"
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

#define MAX_OSD_ENCODE_RX 8
#define MAX_OSD_ENCODE_TX 8

struct osd_encode_msi_s
{
    struct os_work work;
    struct osdenc_device *osd_enc_dev;
    struct msi *msi;
    uint8_t *osd_tmp_buf;
    uint32_t osd_tmp_buf_size;

    struct fbpool tx_pool;

    struct framebuff *parent_data_s;

    // osd压缩后的数据长度
    uint32_t data_len;
    uint32_t start_time;
    // 解码模块是否可用
    uint8_t hardware_ready;
};

static int32 osd_enc_isr_msi(uint32 irq_flag, uint32 irq_data, uint32 param1)
{
    // struct lcdc_device *p_lcd = (struct lcdc_device *)irq_data;
    struct osd_encode_msi_s *osd_enc = (struct osd_encode_msi_s *)irq_data;
    struct osdenc_device *osd_enc_dev;
    // stream *s = (stream*)lcd_s->encode_osd_s;
    struct msi *osd_enc_msi = (struct msi *)osd_enc->msi;
    osd_enc_dev = osd_enc->osd_enc_dev;
    _os_printf("=");
    msi_do_cmd(osd_enc_msi, MSI_CMD_OSD_ENCODE, MSI_OSD_HARDWARE_ENCODE_SET_LEN, osd_enc_dlen(osd_enc_dev));
    msi_do_cmd(osd_enc_msi, MSI_CMD_OSD_ENCODE, MSI_OSD_HARDWARE_REAY_CMD, 1);
    return 0;
}

static int32 osd_encode_work(struct os_work *work)
{
    struct osd_encode_msi_s *osd_encode = (struct osd_encode_msi_s *)work;
    struct framebuff *data_s = NULL;
    // static int osd_encode_count = 0;
    static uint32_t osd_send_times = 0;
    // 先检测一下解码模块是否可用
    if (osd_encode->hardware_ready)
    {
        // 检测一下是否有data_s需要先发送出去
        if (osd_encode->parent_data_s)
        {
            // 获取data_s,申请空间,发送数据
            data_s = fbpool_get(&osd_encode->tx_pool, 0, osd_encode->msi);
            if (data_s)
            {
                data_s->mtype = F_ERGB; //设置为已经压缩的RGB
                data_s->stype = osd_encode->parent_data_s->stype;   //子类保持与接收一致
                msi_delete_fb(NULL, osd_encode->parent_data_s);
                osd_encode->parent_data_s = NULL;

                // 为data_s申请空间
                data_s->data =  (void *)STREAM_MALLOC(osd_encode->data_len);
                if (data_s->data)
                {
                    // 先清除cache
                    sys_dcache_clean_invalid_range((uint32_t *)data_s->data, osd_encode->data_len);
                    // 拷贝数据
                    hw_memcpy_no_cache(data_s->data, (const void *)msi_do_cmd(osd_encode->msi, MSI_CMD_OSD_ENCODE, MSI_OSD_HARDWARE_ENCODE_BUF, 0), (uint32_t)osd_encode->data_len);
                    //sys_dcache_clean_range((uint32_t *)data_s->data, osd_encode->data_len);
                    // 数据类型保持一致
                    data_s->len = osd_encode->data_len;
                    // os_printf("osd encode after len:%d\n",data_s->len);
                    // os_printf("encode spend time:%d\n",(uint32_t)os_jiffies()-osd_encode->start_time);
                    msi_output_fb(osd_encode->msi, data_s);
                    //os_printf("success:%d\n",success);
                    osd_send_times++;
                }
                // 没有发送成功,则释放data_s先吧,下一次再发送
                else
                {

                    msi_delete_fb(osd_encode->msi, data_s);
                    _os_printf("**********************");
                }

                data_s = NULL;
            }
        }

        // 重新检测一下,parent_data_s是否还存在,不存在就去看看是否有需要压缩的osd
        // 会进入中断
        if (!osd_encode->parent_data_s)
        {
            osd_encode->parent_data_s = msi_get_fb(osd_encode->msi, 0);
            if (osd_encode->parent_data_s)
            {
                // 判断一下osd_tmp_buf_size是否足够,如果足够就不需要重新申请空间,否则重新申请一下空间
                if (osd_encode->osd_tmp_buf_size < osd_encode->parent_data_s->len)
                {
                    // 释放原来空间
                    if (osd_encode->osd_tmp_buf)
                    {
                        STREAM_FREE(osd_encode->osd_tmp_buf);
                    }

                    // 重新申请一下空间
                    osd_encode->osd_tmp_buf = (uint8_t *)STREAM_MALLOC(osd_encode->parent_data_s->len);
                    // 申请失败,不压缩了,丢弃
                    if (!osd_encode->osd_tmp_buf)
                    {
                        osd_encode->osd_tmp_buf_size = 0;
                        msi_delete_fb(NULL, osd_encode->parent_data_s);
                        osd_encode->parent_data_s = NULL;
                        goto osd_encode_work_end;
                    }
                    osd_encode->osd_tmp_buf_size = osd_encode->parent_data_s->len;
                    // 先清除cache
                    sys_dcache_clean_invalid_range((uint32_t *)osd_encode->osd_tmp_buf, osd_encode->osd_tmp_buf_size);
                }

                // 硬件模块开始被使用,标志置一下
                msi_do_cmd(osd_encode->msi, MSI_CMD_OSD_ENCODE, MSI_OSD_HARDWARE_REAY_CMD, 0);
                // stream_self_cmd_func(osd_encode->s,OSD_HARDWARE_REAY_CMD,0);
                // 去压缩
                osd_enc_addr(osd_encode->osd_enc_dev, (uint32)osd_encode->parent_data_s->data, (uint32)msi_do_cmd(osd_encode->msi, MSI_CMD_OSD_ENCODE, MSI_OSD_HARDWARE_ENCODE_BUF, 0));
                osd_enc_src_len(osd_encode->osd_enc_dev, osd_encode->osd_tmp_buf_size);
                // gpio_set_val(PA_15,1);
                osd_encode->start_time = os_jiffies();
                osd_enc_run(osd_encode->osd_enc_dev);
            }
        }
    }
osd_encode_work_end:
    os_run_work_delay(work, 1);
    return 0;
}

static int32 osd_encode_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int32_t ret = RET_OK;
    struct osd_encode_msi_s *osd_encode = (struct osd_encode_msi_s *)msi->priv;
    uint32_t arg = param2;
    switch (cmd_id)
    {

        case MSI_CMD_PRE_DESTROY:
        {
            os_work_cancle2(&osd_encode->work, 1);
            // lcdc_osd_enc_start_run(osd_encode->osd_enc_dev, 0);

            // 释放资源空间
            if (osd_encode->osd_tmp_buf)
            {
                STREAM_FREE(osd_encode->osd_tmp_buf);
                osd_encode->osd_tmp_buf_size = 0;
                osd_encode->osd_tmp_buf = NULL;
            }

            // 将缓冲区的fb释放
            struct framebuff *fb;
            while (1)
            {
                fb = msi_get_fb(osd_encode->msi, 0);
                if (fb)
                {
                    msi_delete_fb(NULL, fb);
                }
                else
                {
                    break;
                }
            }

            if (osd_encode->parent_data_s)
            {
                msi_delete_fb(NULL, osd_encode->parent_data_s);
                osd_encode->parent_data_s = NULL;
            }

            // 由于tx_pool的fb没有特殊空间,直接释放就好了
            fbpool_destroy(&osd_encode->tx_pool);
            break;
        }
        // 暂时没有考虑释放
        case MSI_CMD_POST_DESTROY:
        {
            STREAM_LIBC_FREE(osd_encode);
        }
        break;
        // 接收,判断是类型是否可以支持压缩,暂时默认全部接收
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;
            if(fb->mtype != F_RGB)
            {
                ret = RET_ERR;
            }
        }
        break;
        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;
            if (fb->data)
            {
                STREAM_FREE(fb->data);
                fb->data = NULL;
            }
            fbpool_put(&osd_encode->tx_pool, fb);
            ret = RET_OK + 1;
        }
        break;

        case MSI_CMD_OSD_ENCODE:
        {
            uint32_t cmd_self = (uint32_t)param1;
            switch (cmd_self)
            {
                case MSI_OSD_HARDWARE_REAY_CMD:
                    osd_encode->hardware_ready = arg;
                    break;

                case MSI_OSD_HARDWARE_ENCODE_SET_LEN:
                    osd_encode->data_len = arg;
                    break;

                case MSI_OSD_HARDWARE_ENCODE_GET_LEN:
                    ret = osd_encode->data_len;
                    break;

                case MSI_OSD_HARDWARE_ENCODE_BUF:
                    ret = (uint32_t)osd_encode->osd_tmp_buf;
                    break;

                case MSI_OSD_HARDWARE_DEV:
                    ret = (uint32_t)osd_encode->osd_enc_dev;
                    break;

                case MSI_OSD_HARDWARE_STREAM_RESET:
                {
                    // 硬件模块可用
                    osd_encode->hardware_ready = 1;
                    OS_WORK_REINIT(&osd_encode->work);
                    os_run_work(&osd_encode->work);
                }
                break;

                case MSI_OSD_HARDWARE_STREAM_STOP:
                    // 停止硬件模块
                    // lcdc_osd_enc_start_run(osd_encode->osd_enc_dev, 0);
                    // workqueue停止
                    os_work_cancle2(&osd_encode->work, 1);
                    break;
            }
        }
        break;

        default:
            break;
    }
    return ret;
}

// osd压缩是一个模块,所以在这里统一管理自己的模块,所有需要压缩的数据都到这里,然后再发出去
// 然后类型可以跟着父流一致
// 要预先申请一个空间给到osd压缩,最后再读取寄存器看看要拷贝多少压缩的数据
// 所以空间是:w*h*2+压缩的size,压缩的size根据不同ui去压缩的,一般不会超过原来的size

struct msi *osd_encode_msi_init(const char *name)
{
    struct msi *msi = msi_new(name, MAX_OSD_ENCODE_RX, NULL);      //R_OSD_ENCODE
    struct osd_encode_msi_s *osd_encode = (struct osd_encode_msi_s *)msi->priv;
    if (!osd_encode)
    {
        osd_encode = (struct osd_encode_msi_s *)STREAM_LIBC_ZALLOC(sizeof(struct osd_encode_msi_s));
        msi->priv = (void *)osd_encode;
        osd_encode->msi = msi;
        fbpool_init(&osd_encode->tx_pool, MAX_OSD_ENCODE_TX);
        osd_encode->osd_enc_dev = (struct osdenc_device *)dev_get(HG_OSD_ENC_DEVID);
        osd_encode->hardware_ready = 1;
        osd_encode->osd_tmp_buf_size = 0;
        osd_encode->osd_tmp_buf = NULL;
        msi->action = osd_encode_msi_action;
        msi->enable = 1;
        msi_add_output(osd_encode->msi, NULL, R_LCD_OSD);

        osd_enc_open(osd_encode->osd_enc_dev);
        osd_enc_tran_config(osd_encode->osd_enc_dev, 0xFFFFFF, 0xFFFFFF, 0x000000, 0x000000);
        osd_enc_set_format(osd_encode->osd_enc_dev, 1);
        osd_enc_request_irq(osd_encode->osd_enc_dev, ENC_DONE_IRQ, osd_enc_isr_msi, (uint32)osd_encode);
        OS_WORK_INIT(&osd_encode->work, osd_encode_work, 0);
        os_run_work_delay(&osd_encode->work, 1);
    }

    return msi;
}
