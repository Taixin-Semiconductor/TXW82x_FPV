#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "stream_define.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "dev/jpg/hgjpg.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/video/vpp/vpp_dev.h"
#include "jpg_concat_msi.h"
#include "user_work/user_work.h"
#if JPG_EN
#define HARDWARE_JPG_NUM 2

volatile int8_t g_dqtable_index = DQT_DEF;

enum
{
    MSI_JPG_DONE_ERR     = BIT(0), // done的时候报错
    MSI_JPG_BUF_FULL_ERR = BIT(1), // buf full的时候报错
    MSI_JPG_BUF_ERR      = BIT(2), // 硬件直接报错
};

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

struct msi *g_jpg_msi[HARDWARE_JPG_NUM] = {NULL, NULL};

int jpg_quality_pidCtrl(int diff, int p, int i, int d)
{
    static int diff_prev = 0;
    static int diff_sum  = 0;
    int        res       = 0;
    res                  = p * diff + i * diff_sum + d * (diff - diff_prev);
    diff_sum += diff;
    diff_prev = diff;
    res       = res >> 16;
    res       = LIMITING(res, 120, -120);
    diff_sum  = LIMITING(diff_sum, 2000, -2000);
    //_os_printf("diff_sum:%d\n",diff_sum);
    return res;
}

static uint8_t jpg_msi_quality_tidy(uint8_t *qt, uint32 len, uint8 *dqt_index_diff)
{
    uint8 updata_dqt = 0;

#if 0
    if (len >= 110 * 1024)
    {
        (*qt)++;
        if (*qt == 0x10)
        {
            *qt = 0x8;
            updata_dqt = 2;
        }
    }
    else if (len <= 55 * 1024)
    {
        (*qt)--;
        if (*qt == 0)
        {
            *qt = 8;
            updata_dqt = 1;
        }
    }
    *dqt_index_diff = 1;
#else
    int   jpg_len_diff  = len - TARGET_JPG_LEN;
    int   res           = jpg_quality_pidCtrl(jpg_len_diff, QUALITY_CTRL_P, QUALITY_CTRL_I, QUALITY_CTRL_D);
    uint8 qt_diff       = os_abs(res) % 0x10;
    uint8 qt_index_diff = os_abs(res) / 0x10;
    *dqt_index_diff     = qt_index_diff;
    if (res > 0)
    {
        if (qt_diff + (*qt) > 0xf)
        {
            if ((g_dqtable_index + qt_index_diff + 1 + (qt_diff - (0xf - (*qt))) / 8) > DQT_MAX_INDEX)
            {
                *dqt_index_diff = DQT_MAX_INDEX - g_dqtable_index;
                (*qt)           = 0xf;
            }
            else
            {
                (*qt)           = 0x8 + (qt_diff - (0xf - (*qt))) % 8;
                *dqt_index_diff = qt_index_diff + 1 + (qt_diff - (0xf - (*qt))) / 8;
            }
        }
        else
        {
            (*qt) += qt_diff;
        }
        if ((*dqt_index_diff) >= 1 && g_dqtable_index < DQT_MAX_INDEX)
        {
            *dqt_index_diff = ((*dqt_index_diff) > (DQT_MAX_INDEX - g_dqtable_index)) ? (DQT_MAX_INDEX - g_dqtable_index) : (*dqt_index_diff);
            updata_dqt      = 2;
        }
    }
    else if (res < 0)
    {
        if ((*qt) - qt_diff < 0)
        {
            if ((g_dqtable_index - (qt_index_diff + 1 + (qt_diff - (*qt)) / 8)) < 0)
            {
                *dqt_index_diff = g_dqtable_index;
                (*qt)           = 0;
            }
            else
            {
                (*qt)           = 0x8 - (qt_diff - (*qt)) % 8; //(qt_diff-(*qt)) maybe > 0x8
                *dqt_index_diff = qt_index_diff + 1 + (qt_diff - (*qt)) / 8;
            }
        }
        else
        {
            (*qt) -= qt_diff;
        }
        if ((*dqt_index_diff) >= 1 && g_dqtable_index > 0)
        {
            *dqt_index_diff = ((*dqt_index_diff) > (g_dqtable_index)) ? g_dqtable_index : (*dqt_index_diff);
            updata_dqt      = 1;
        }
    }
    if (updata_dqt == 0)
    {
        *dqt_index_diff = 0;
    }
#endif
    return updata_dqt;
}

static void jpg_msi_DQT_updata(struct jpg_device *p_jpg, uint8 upOdown, uint8 diff)
{
    uint32     *ptable;
    static int8 pdqt_tab = DQT_DEF;
    if (upOdown == 1)
    {
        pdqt_tab -= diff;
        if (pdqt_tab < 0)
        {
            pdqt_tab = 0;
        }
    }
    else
    {
        pdqt_tab += diff;
        if (pdqt_tab > DQT_MAX_INDEX)
        {
            pdqt_tab = DQT_MAX_INDEX;
        }
    }
    g_dqtable_index = pdqt_tab;

    ptable = (uint32 *) quality_tab[pdqt_tab];
    jpg_updata_dqt(p_jpg, ptable);
}

static int32 jpg_msi_done_isr(uint32 irq_flag, uint32 irq_data, uint32 param1, uint32 param2)
{
    uint8                dqt_index_diff = 0;
    int8_t               err            = RET_OK;
    struct msi          *msi            = (struct msi *) irq_data;
    struct jpg_V3_msi_s *jpg_msg        = (struct jpg_V3_msi_s *) msi->priv;
    // 如果没有异常,send_fb应该通过信号量发出去
    struct framebuff    *send_fb        = jpg_msg->now_fb;
    struct framebuff    *fb;
    uint32_t             jpg_len = param1;
    uint8_t              last_qt = jpg_msg->qt;
    // 如果数据不对,需要处理异常情况(需要将fb->now_fb里面的链表全部放回到pool中)
    // 如果异常,这里关闭jpg,workqueue去清理资源,重新启动mjpg
    if (jpg_msg->err)
    {
        err = RET_ERR;
        goto jpg_msi_done_isr_end;
    }

    jpg_set_ready(jpg_msg->jpg);
    // 最后配置的fb,这个是没有被用的,可以重复利用
    fb = jpg_msg->last_fb;
    // 这里不应该进来,进来后,需要检查是否正常
    if (!fb)
    {
        err = RET_ERR;
        goto jpg_msi_done_isr_end;
    }
    jpg_set_addr(jpg_msg->jpg, (uint32) fb->data, fb->len);

    // 先将fb记录在last_fb
    jpg_msg->last_fb = fb;

    fb = fbpool_get(&jpg_msg->pool, 0, NULL);
    // 空间不够,则通知workqueue检查或者重新启动mjpg
    if (!fb)
    {
        err = RET_ERR;
        goto jpg_msi_done_isr_end;
    }

    // 这里确认足够空间重新启动jpg,可以将now_fb重新赋值
    jpg_msg->now_fb      = jpg_msg->last_fb;
    jpg_msg->use_last_fb = jpg_msg->last_fb;

    // 配置第二次的寄存器
    jpg_set_addr(jpg_msg->jpg, (uint32) fb->data, fb->len);
    jpg_msg->last_fb = fb;

    // jpg_open(jpg_msg->jpg);
    send_fb->datatag = jpg_msg->datatag;
    if (os_msgq_put(&jpg_msg->msgq, (uint32_t) send_fb, 0))
    {
        struct framebuff *tmp_fb = send_fb;
        // 发送失败,应该将fb全部放回pool池
        while (tmp_fb)
        {
            tmp_fb = send_fb->next;
            fbpool_put(&jpg_msg->pool, send_fb);
            send_fb = tmp_fb;
        }
    }
    else
    {
        uint32_t time = 0;
        // send_fb->mtype = F_JPG_NODE;
        if (jpg_msg->src_from == GEN420_DATA)
        {
            // 配置这个是从gen420来的数据,后续要考虑是否要独立配置类型(因为gen420来源数据可能是内存或者其他来源),或者说由其他回调接口去配置吧
            // 这里需要调用msi命令去配置
            send_fb->stype = jpg_msg->gen420_type;
            send_fb->srcID = FRAMEBUFF_SOURCE_JPG_GEN420;
            time           = jpg_msg->set_time;
        }
        else if (jpg_msg->src_from == SCALER_DATA)
        {
            send_fb->stype = jpg_msg->scale1_type ? jpg_msg->scale1_type : (FSTYPE_VIDEO_VPP_DATA0 + jpg_msg->src_from);
            send_fb->srcID = FRAMEBUFF_SOURCE_JPG_SCALER;
            time           = jpg_msg->set_time;
        }
        else
        {
            // send_fb->stype = FSTYPE_JPG_CAMERA0+video_msg.video_type_cur;
            send_fb->srcID = FRAMEBUFF_SOURCE_CAMERA0 + video_msg.video_type_cur;
            send_fb->stype = FSTYPE_VIDEO_VPP_DATA0 + jpg_msg->src_from;
        }

        if (!time)
        {
            time = os_jiffies();
        }

        // 配置子类型
        send_fb->datatag = jpg_msg->datatag;

        // 这里是特殊赋值,为了不每一个send_fb申请空间,所以这里利用指针去保存
        send_fb->priv  = (void *) (jpg_msg->w << 16 | jpg_msg->h);
        // 利用mtype来保存低位的时间戳,在msi_output_fb重新计算时间戳(因为不应该存在65s错过)
        send_fb->mtype = F_JPG_NODE;
        // 利用time的结构体保存图片长度,msi_output_fb后将数据保存到结构体再恢复
        send_fb->time  = time;
        send_fb->msi   = (void *) jpg_len;
        // 唤醒workqueue去处理fb
        os_run_work(&jpg_msg->work);
    }
    // os_printf("jpglen:%d %d %d\n", jpg_len, jpg_msg->qt, g_dqtable_index);
    uint8_t update = jpg_msi_quality_tidy(&jpg_msg->qt, jpg_len, &dqt_index_diff);
    // os_printf("quality res:%d %s%d\n", jpg_msg->qt, ((update>1)?"+":"-"),dqt_index_diff);
    if (last_qt != jpg_msg->qt)
    {
        jpg_set_qt(jpg_msg->jpg, jpg_msg->qt);
    }
    if (update)
    {
        jpg_msi_DQT_updata(jpg_msg->jpg, update, dqt_index_diff);
    }

jpg_msi_done_isr_end:
    if (err == RET_OK)
    {
        _os_printf(KERN_DEBUG "JD");
    }
    else
    {
        jpg_msg->err |= MSI_JPG_DONE_ERR;
        jpg_close(jpg_msg->jpg);
        os_event_set(&jpg_msg->evt, MSI_JPG_DONE_ERR, NULL);
        os_run_work(&jpg_msg->work);
    }
    return 0;
}

static int32 jpg_msi_outbuff_full_isr(uint32 irq_flag, uint32 irq_data, uint32 param1, uint32 param2)
{
    int8_t               err     = RET_OK;
    struct msi          *msi     = (struct msi *) irq_data;
    struct jpg_V3_msi_s *jpg_msg = (struct jpg_V3_msi_s *) msi->priv;

    if (jpg_msg->err)
    {
        goto jpg_msi_outbuff_full_isr_end;
    }
    struct framebuff *fb = fbpool_get(&jpg_msg->pool, 0, NULL);
    if (!fb)
    {
        err = RET_ERR;
        goto jpg_msi_outbuff_full_isr_end;
    }
    jpg_set_addr(jpg_msg->jpg, (uint32) fb->data, fb->len);
    ASSERT(jpg_msg->last_fb);
    // 放到链表里面(now_fb为head)
    jpg_msg->use_last_fb->next = jpg_msg->last_fb;
    // 记录当前的配置的最后fb
    jpg_msg->use_last_fb       = jpg_msg->last_fb;
    // 最后配置jpg地址的fb(尚未添加到链表)
    jpg_msg->last_fb           = fb;
jpg_msi_outbuff_full_isr_end:
    if (err != RET_OK)
    {
        jpg_msg->err |= MSI_JPG_BUF_FULL_ERR;
        os_event_set(&jpg_msg->evt, MSI_JPG_BUF_FULL_ERR, NULL);
    }
    return 0;
}

static int32 jpg_msi_buf_err(uint32 irq_flag, uint32 irq_data, uint32 param1, uint32 param2)
{
    struct msi          *msi     = (struct msi *) irq_data;
    struct jpg_V3_msi_s *jpg_msg = (struct jpg_V3_msi_s *) msi->priv;
    // 关闭jpg
    jpg_close(jpg_msg->jpg);
    // 唤醒workqueue?然后workqueue检查是不是有异常?有异常重新启动jpg?统一由外部线程去重新启动
    os_event_set(&jpg_msg->evt, MSI_JPG_BUF_ERR, NULL);
    os_run_work(&jpg_msg->work);
    jpg_msg->err |= MSI_JPG_BUF_ERR;
    _os_printf(KERN_ERR "JE");
    return 0;
}
static int32 jpg_msi_work(struct os_work *work)
{
    struct jpg_V3_msi_s *jpg_msg = (struct jpg_V3_msi_s *) work;
    struct framebuff    *fb;
    int32                err     = -1;
    uint32               jpg_err = 0;

    os_event_wait(&jpg_msg->evt, MSI_JPG_DONE_ERR | MSI_JPG_BUF_FULL_ERR | MSI_JPG_BUF_ERR, &jpg_err, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
    fb = (struct framebuff *) os_msgq_get2(&jpg_msg->msgq, 0, &err);
    if (err)
    {
        goto jpg_msi_work_end;
    }
    // 测试,删除fb(正常是添加msi,然后get_msi,发送出去)
    struct framebuff *tmp_fb = fb;
    uint32_t          len    = (uint32_t) fb->msi;

    while (tmp_fb)
    {
        tmp_fb->msi = jpg_msg->msi;
        // 这个模块是特殊处理
        msi_get(tmp_fb->msi);
        tmp_fb = tmp_fb->next;
    }

    // 申请结构体,这里是比较特殊的使用
    // w和h保存在priv结构体
    // 时间低位保存在mtype
    // time保存的是图片的size
    // 最后还原到结构体
    struct jpg_node_s *jpg_priv = (struct jpg_node_s *) STREAM_LIBC_MALLOC(sizeof(struct jpg_node_s));
    if (jpg_priv)
    {
        uint32_t w_h      = (uint32_t) fb->priv;
        jpg_priv->w       = (w_h) >> 16;
        jpg_priv->h       = (w_h) & 0xffff;
        jpg_priv->jpg_len = len;
        fb->time          = fb->time;
        fb->mtype         = F_JPG_NODE;
        fb->priv          = jpg_priv;
    }
    else
    {
        fb->priv = NULL;
    }
    // 为了不添加新结构,使用time作为w和h保存,priv用于保存总长度(仅仅用于这个msi)
    msi_output_fb(jpg_msg->msi, fb);

jpg_msi_work_end:
    // jpg异常,那么去重新启动一下jpg
    // 需要先清除对应资源才行
    if (jpg_err)
    {
        os_printf(KERN_ERR "jpg_err:%X\twhich:%d\n", jpg_err, jpg_msg->which);
        struct framebuff *tmp_fb, *now_fb, *fb;
        now_fb = tmp_fb = jpg_msg->now_fb;
        while (now_fb)
        {
            tmp_fb = now_fb;
            now_fb = now_fb->next;
            fbpool_put(&jpg_msg->pool, tmp_fb);
        }

        // 最后一次配置寄存器的fb(没有链接到now_fb)
        if (jpg_msg->last_fb)
        {
            fbpool_put(&jpg_msg->pool, jpg_msg->last_fb);
        }
        jpg_msg->last_fb = NULL;
        jpg_msg->now_fb  = NULL;

        // 重新配置jpg的地址,启动
        fb = fbpool_get(&jpg_msg->pool, 0, NULL);
        ASSERT(fb);
        jpg_set_addr(jpg_msg->jpg, (uint32) fb->data, fb->len);
        jpg_msg->now_fb      = fb;
        jpg_msg->use_last_fb = fb;

        fb = fbpool_get(&jpg_msg->pool, 0, NULL);
        ASSERT(fb);
        jpg_set_addr(jpg_msg->jpg, (uint32) fb->data, fb->len);
        jpg_msg->last_fb = fb;
        jpg_msg->err     = 0;
        jpg_msg->running = 1;
        jpg_set_ready(jpg_msg->jpg);
        jpg_open(jpg_msg->jpg);
    }
    return 0;
}
static int jpg_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int                  ret     = RET_OK;
    struct jpg_V3_msi_s *jpg_msg = (struct jpg_V3_msi_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            fbpool_destroy(&jpg_msg->pool);
            os_msgq_del(&jpg_msg->msgq);
            os_event_del(&jpg_msg->evt);
            msi->name               = NULL; // 这里比较特殊,正常不能清空的
            // 释放节点
            uint32_t jpg_node_count = jpg_msg->jpg_node_count;
            uint32_t m_size         = 0;
            while (m_size < jpg_node_count)
            {
                if (jpg_msg->jpg_node_buf[m_size])
                {
                    ASSERT(jpg_msg->jpg_node_buf[m_size]);
                    STREAM_FREE((uint8_t *) jpg_msg->jpg_node_buf[m_size]);
                }
                m_size++;
            }

            STREAM_LIBC_FREE(jpg_msg);
        }
        break;
        case MSI_CMD_PRE_DESTROY:
        {
            // 先关闭workqueue(防止有报错,将jpg重新启动)
            os_work_cancle2(&jpg_msg->work, 1);
            // 关闭jpg
            if (jpg_msg->running)
            {
                jpg_close(jpg_msg->jpg);
                jpg_msg->running = 0;
            }

            g_jpg_msi[jpg_msg->which] = NULL;

            // 清除资源,理论队列的内容已经没有用了,不需要管理

            // 清理一些临时资源,检查一下now_fb是否有数据?(正常流程是去释放,时间不释放也不影响,应为没有对msi进行引用)
            struct framebuff *tmp_fb, *now_fb;
            now_fb = tmp_fb = jpg_msg->now_fb;
            while (now_fb)
            {
                tmp_fb = now_fb;
                now_fb = now_fb->next;
                fbpool_put(&jpg_msg->pool, tmp_fb);
            }

            // 最后一次配置寄存器的fb(没有链接到now_fb)
            if (jpg_msg->last_fb)
            {
                fbpool_put(&jpg_msg->pool, jpg_msg->last_fb);
            }
            jpg_msg->last_fb = NULL;
            jpg_msg->now_fb  = NULL;
        }
        break;

        case MSI_CMD_GET_RUNNING:
        {
            if (param1)
            {
                uint32_t running = 0;
                if (jpg_msg->running && !jpg_msg->err)
                {
                    running = 1;
                }
                *(uint32_t *) param1 = running;
            }
        }
        break;

        case MSI_CMD_HARDWARE_JPEG:
        {
            uint32_t cmd_self = (uint32_t) param1;
            uint32_t arg      = param2;
            switch (cmd_self)
            {
                // 如果需要实现多个编码size,可能需要保存,然后通过中断自动切换
                case MSI_JPEG_HARDWARE_MSG:
                {
                    uint16_t w, h;
                    w          = arg >> 16;
                    h          = arg & 0xffff;
                    jpg_msg->w = w;
                    jpg_msg->h = h;
                    // jpg_set_size(jpg_msg->jpg, h, w);
                }
                break;

                case MSI_JPEG_HARDWARE_START:
                {
                    if (!jpg_msg->running)
                    {
                        OS_WORK_REINIT(&jpg_msg->work);
                        struct framebuff *fb;

                        // 硬件初始化
                        jpg_msg->qt = 0xf;
                        jpg_set_size(jpg_msg->jpg, jpg_msg->h, jpg_msg->w);
                        jpg_init(jpg_msg->jpg, g_dqtable_index, jpg_msg->qt);

                        jpg_set_data_from(jpg_msg->jpg, jpg_msg->src_from);
                        jpg_set_hw_check(jpg_msg->jpg, 1);

                        // 注册中断
                        jpg_request_irq(jpg_msg->jpg, jpg_msi_outbuff_full_isr, JPG_IRQ_FLAG_JPG_BUF_FULL, msi);
                        jpg_request_irq(jpg_msg->jpg, jpg_msi_buf_err, JPG_IRQ_FLAG_ERROR, msi);
                        jpg_request_irq(jpg_msg->jpg, jpg_msi_done_isr, JPG_IRQ_FLAG_JPG_DONE, msi);

                        jpg_set_ready(jpg_msg->jpg);
                        jpg_set_vsync_dly(jpg_msg->jpg, 1);
                        jpg_select_oe_using(jpg_msg->jpg, 0, 1);

                        fb = fbpool_get(&jpg_msg->pool, 0, NULL);
                        ASSERT(fb);
                        jpg_set_addr(jpg_msg->jpg, (uint32) fb->data, fb->len);
                        jpg_msg->now_fb      = fb;
                        jpg_msg->use_last_fb = fb;

                        fb = fbpool_get(&jpg_msg->pool, 0, NULL);
                        ASSERT(fb);
                        jpg_set_addr(jpg_msg->jpg, (uint32) fb->data, fb->len);
                        // 记录最后一个配置链表
                        jpg_msg->last_fb = fb;
                        jpg_msg->running = 1;
                        jpg_open(jpg_msg->jpg);
                    }
                    else
                    {
                    }
                }
                break;

                case MSI_JPEG_HARDWARE_STOP:
                {
                    if (jpg_msg->running)
                    {
                        jpg_close(jpg_msg->jpg);
                        os_work_cancle2(&jpg_msg->work, 1);
                        jpg_msg->running = 0;
                        jpg_msg->err     = 0;
                        // 清除异常
                        os_event_wait(&jpg_msg->evt, MSI_JPG_DONE_ERR | MSI_JPG_BUF_FULL_ERR | MSI_JPG_BUF_ERR, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
                        // 需要清理一下队列?这个时候队列的数据应该是不需要了以及now_fb的数据也要清除
                        {
                            struct framebuff *fb;
                            int32             err = 0;
                            // 将队列的数据帮忙发送出去
                            while (!err)
                            {
                                fb = (struct framebuff *) os_msgq_get2(&jpg_msg->msgq, 0, &err);
                                if (!err)
                                {
                                    struct framebuff *tmp_fb = fb;
                                    // 中断将长度记录在msi的结构体中
                                    uint32_t          len    = (uint32_t) fb->msi;
                                    while (tmp_fb)
                                    {
                                        tmp_fb->msi = jpg_msg->msi;
                                        // 这个模块是特殊处理
                                        msi_get(tmp_fb->msi);
                                        tmp_fb = tmp_fb->next;
                                    }
                                    // 申请结构体,这里是比较特殊的使用
                                    // w和h保存在priv结构体
                                    // 时间低位保存在mtype
                                    // time保存的是图片的size
                                    // 最后还原到结构体
                                    struct jpg_node_s *jpg_priv = (struct jpg_node_s *) STREAM_LIBC_MALLOC(sizeof(struct jpg_node_s));
                                    if (jpg_priv)
                                    {
                                        uint32_t w_h      = (uint32_t) fb->priv;
                                        jpg_priv->w       = (w_h) >> 16;
                                        jpg_priv->h       = (w_h) & 0xffff;
                                        jpg_priv->jpg_len = len;
                                        fb->time          = fb->time;
                                        fb->mtype         = F_JPG_NODE;
                                        fb->priv          = jpg_priv;
                                    }
                                    else
                                    {
                                        fb->priv = NULL;
                                    }
                                    msi_output_fb(jpg_msg->msi, fb);
                                }
                            }
                        }

                        // 中断的临时资源清除
                        {
                            struct framebuff *tmp_fb, *now_fb;
                            now_fb = tmp_fb = jpg_msg->now_fb;
                            while (now_fb)
                            {
                                tmp_fb = now_fb;
                                now_fb = now_fb->next;
                                fbpool_put(&jpg_msg->pool, tmp_fb);
                            }

                            // 最后一次配置寄存器的fb(没有链接到now_fb)
                            if (jpg_msg->last_fb)
                            {
                                fbpool_put(&jpg_msg->pool, jpg_msg->last_fb);
                            }
                            jpg_msg->last_fb = NULL;
                            jpg_msg->now_fb  = NULL;
                        }
                    }
                    else
                    {
                        os_work_cancle2(&jpg_msg->work, 1);
                    }
                }
                break;

                case MSI_JPEG_HARDWARE_FROM:
                {
                    if (arg >= VPP_DATA0 && arg <= SOFT_DATA)
                    {
                        jpg_msg->src_from = arg;
                        // jpg_set_data_from(jpg_msg->jpg, arg);
                    }
                    else
                    {
                        os_printf(KERN_ERR "jpg set data from err:%d\n", arg);
                    }
                }
                break;

                // 配置gen42的类型
                case MSI_JPEG_HARDWARE_SET_GEN420_TYPE:
                {
                    jpg_msg->gen420_type = arg;
                }
                break;

                case MSI_JPEG_HARDWARE_SET_SCALE1_TYPE:
                {
                    jpg_msg->scale1_type = arg;
                }
                break;

                case MSI_JPEG_SET_TIME:
                {
                    jpg_msg->set_time = arg;
                }
                break;
                default:
                    break;
            }
        }
        break;
        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            if (fb->data)
            {
                sys_dcache_clean_invalid_range((uint32_t *) fb->data, fb->len);
            }
            if (fb->priv)
            {
                STREAM_LIBC_FREE(fb->priv);
                fb->priv = NULL;
            }
            fbpool_put(&jpg_msg->pool, fb);
            // 不需要内核去释放fb
            ret = RET_OK + 1;
        }
        break;

        case MSI_CMD_SET_DATATAG:
        {
            uint8_t datatag  = (uint8_t) param1;
            jpg_msg->datatag = datatag;
        }
        break;
    }
    return ret;
}

// 这个接口尽量不在外部调用,因为硬件只有一个
// 所以由中间流对这个硬件控制
struct msi *hardware_jpg_msi(uint8_t which_jpg, uint8_t src_from, uint16_t jpg_node_len, uint16_t jpg_node_count)
{
    if (which_jpg >= HARDWARE_JPG_NUM)
    {
        return NULL;
    }
    // 硬件只有一个,所以只能启动一次,外部显式调用一次msi_destroy才可以
    if (g_jpg_msi[which_jpg])
    {
        // 先将硬件关闭,然后重新创建吧
        // struct msi *old_msi = g_jpg_msi[which_jpg];
        // msi_destroy(old_msi);
        return NULL;
    }
    struct jpg_V3_msi_s *jpg_msg = (struct jpg_V3_msi_s *) STREAM_LIBC_ZALLOC(sizeof(struct jpg_V3_msi_s) + jpg_node_count * sizeof(uint32_t));
    if (jpg_msg)
    {
        jpg_msg->jpg_node_buf = (uint32_t *) (jpg_msg + 1);
    }

    // 先去分配空间,如果空间不够,则不启动硬件,分配节点不足4个也会退出
    uint32_t m_size = 0;
    while (m_size < jpg_node_count)
    {
        uint8_t *buff = (uint8_t *) STREAM_MALLOC(jpg_node_len);
        sys_dcache_clean_invalid_range((uint32_t *) buff, jpg_node_len);
        if (!buff)
        {
            break;
        }
        jpg_msg->jpg_node_buf[m_size] = (uint32_t) buff;
        m_size++;
    }
    if (m_size != jpg_node_count)
    {
        os_printf(KERN_WARNING "%s:%d jpg[%d]not enought mem,m_size:%d\n", __FUNCTION__, __LINE__, which_jpg, m_size);
    }

    os_printf(KERN_INFO "JPG[%d] jpg_node_count:%d\tm_size:%d\n", which_jpg, jpg_node_count, m_size);
    jpg_node_count    = m_size;
    struct msi *msi   = NULL;
    uint8_t     isnew = 0;
    if (jpg_msg && jpg_node_count >= 4)
    {
        os_sprintf(jpg_msg->msi_name, "H_JPG%d_%08X", which_jpg, (uint32) os_jiffies() & 0xFFFFFFFF);
        msi = msi_new(jpg_msg->msi_name, 0, &isnew);
        ASSERT(msi);
        // 这里一定是新创建
        ASSERT(isnew);
        msi->priv         = (void *) jpg_msg;
        jpg_msg->which    = which_jpg;
        jpg_msg->src_from = src_from;
        // 这里式分配多少帧,如果分配1帧,就是应用要快速去处理,否则可能来不及
        // 如果帧数据量大,就可能出现最后节点不够的可能
        os_msgq_init(&jpg_msg->msgq, 1);
        os_event_init(&jpg_msg->evt);
        if (msi)
        {
            fbpool_init(&jpg_msg->pool, jpg_node_count);
            jpg_msg->msi            = msi;
            msi->action             = jpg_msi_action;
            // 固定输出,外部尽量不要用这个msi,统一给到RS_JPG_CONCAT这个msi去管理
            jpg_msg->jpg_node_len   = jpg_node_len;
            jpg_msg->jpg_node_count = jpg_node_count;
            jpg_msg->jpg            = (struct jpg_device *) dev_get(which_jpg + HG_JPG0_DEVID);
            // 预分配buf空间到各个fb中
            uint16_t init_count     = 0;
            uint8_t *m_buff;
            // 初始化framebuffer节点数量空间?最后由workqueue去从ringbuf去获取一个节点
            while (init_count < jpg_node_count)
            {
                m_buff = (uint8_t *) jpg_msg->jpg_node_buf[init_count];
                FBPOOL_SET_INFO(&jpg_msg->pool, init_count, m_buff, jpg_node_len, NULL);
                init_count++;
            }
            msi->enable = 1;
            // 启动一个workqueue,去将fb发送出去,中断唤醒一次,workqueue执行一次
            OS_WORK_INIT(&jpg_msg->work, jpg_msi_work, 0);
            jpg_close(jpg_msg->jpg);
        }
    }

    if (!msi)
    {
        m_size = 0;
        while (m_size < jpg_node_count)
        {
            if (jpg_msg->jpg_node_buf[m_size])
            {
                ASSERT(jpg_msg->jpg_node_buf[m_size]);
                STREAM_FREE((uint8_t *) jpg_msg->jpg_node_buf[m_size]);
            }
            m_size++;
        }

        if (jpg_msg)
        {
            STREAM_LIBC_FREE(jpg_msg);
        }
    }
    else
    {
        g_jpg_msi[which_jpg] = msi;
    }
    return msi;
}
#else
struct msi *hardware_jpg_msi(uint8_t which_jpg, uint8_t src_from, uint16_t jpg_node_len, uint16_t jpg_node_count)
{
    return NULL;
}
#endif