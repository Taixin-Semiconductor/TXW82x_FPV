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
#include "hal/scale.h"
#include "hal/isp.h"

#define HARDWARE_JPG_NUM        2
#define DESTROY_MSI_NAME(index) "_destroy_jpg" #index

// jpg中断记录的msg,通过jpg_node预留空间来及记录
struct jpg_isr_msg
{
    uint8_t  stype;
    uint8_t  src_from;
    uint8_t  srcID;
    uint8_t  datatag;
    uint16_t w, h;
    uint32_t time;
    uint32_t len;
};

enum
{
    MSI_JPG_DONE_ERR             = BIT(0), // done的时候报错
    MSI_JPG_BUF_FULL_ERR         = BIT(1), // buf full的时候报错
    MSI_JPG_BUF_ERR              = BIT(2), // 硬件直接报错
    MSI_SCALE1_DATA0_DATA1_CLOSE = BIT(3),
    MSI_JPG_END_FLAG             = BIT(4),
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

// 采用默认jpg方式,如果遇到频繁切换mjpg模式,会变慢
#ifndef FAST_JPG

static int32_t vpp_start_JPEG(uint32_t irq_data)
{
    struct jpg_device *jpg = (struct jpg_device *) irq_data;
    // 如果需要拼接,就要等待镜头2才能启动
    if (video_msg.video_type_cur == ISP_VIDEO_1 || video_msg.camera_mode != CAM_DUAL_SPLICE_SLAVE_MODE)
    {
        jpg_open(jpg);
        return 1;
    }
    else
    {
        return 0;
    }
}

int jpg_quality_pidCtrl(struct jpg_V3_msi_s *jpg_msg, int diff, int p, int i, int d)
{
    int32_t res = p * diff + i * jpg_msg->diff_sum + d * (diff - jpg_msg->diff_prev);
    jpg_msg->diff_sum += diff;
    jpg_msg->diff_prev = diff;
    res                = res >> 16;
    res                = LIMITING(res, 120, -120);
    jpg_msg->diff_sum  = LIMITING(jpg_msg->diff_sum, 2000, -2000);
    //_os_printf("diff_sum:%d\n",diff_sum);
    return res;
}

static uint8_t jpg_msi_quality_tidy(struct jpg_V3_msi_s *jpg_msg, uint32_t cur_len, uint8_t *dqt_index_diff)
{
    uint8_t updata_dqt    = 0;
    int32_t jpg_len_diff  = cur_len - jpg_msg->target_len;
    int32_t res           = jpg_quality_pidCtrl(jpg_msg, jpg_len_diff, QUALITY_CTRL_P, QUALITY_CTRL_I, QUALITY_CTRL_D);
    uint8_t qt_diff       = os_abs(res) % 0x10;
    uint8_t qt_index_diff = os_abs(res) / 0x10;
    *dqt_index_diff       = qt_index_diff;
    if (res > 0)
    {
        if (qt_diff + (jpg_msg->qt) > 0xf)
        {
            if ((jpg_msg->dqtable_index + qt_index_diff + 1 + (qt_diff - (0xf - jpg_msg->qt)) / 8) > DQT_MAX_INDEX)
            {
                *dqt_index_diff = DQT_MAX_INDEX - jpg_msg->dqtable_index;
                jpg_msg->qt     = 0xf;
            }
            else
            {
                jpg_msg->qt     = 0x8 + (qt_diff - (0xf - jpg_msg->qt)) % 8;
                *dqt_index_diff = qt_index_diff + 1 + (qt_diff - (0xf - jpg_msg->qt)) / 8;
            }
        }
        else
        {
            jpg_msg->qt += qt_diff;
        }
        if ((*dqt_index_diff) >= 1 && jpg_msg->dqtable_index < DQT_MAX_INDEX)
        {
            *dqt_index_diff = ((*dqt_index_diff) > (DQT_MAX_INDEX - jpg_msg->dqtable_index)) ? (DQT_MAX_INDEX - jpg_msg->dqtable_index) : (*dqt_index_diff);
            updata_dqt      = 2;
        }
    }
    else if (res < 0)
    {
        if (jpg_msg->qt - qt_diff < 0)
        {
            if ((jpg_msg->dqtable_index - (qt_index_diff + 1 + (qt_diff - jpg_msg->qt) / 8)) < 0)
            {
                *dqt_index_diff = jpg_msg->dqtable_index;
                jpg_msg->qt     = 0;
            }
            else
            {
                jpg_msg->qt     = 0x8 - (qt_diff - jpg_msg->qt) % 8; //(qt_diff-(*qt)) maybe > 0x8
                *dqt_index_diff = qt_index_diff + 1 + (qt_diff - jpg_msg->qt) / 8;
            }
        }
        else
        {
            jpg_msg->qt -= qt_diff;
        }
        if ((*dqt_index_diff) >= 1 && jpg_msg->dqtable_index > 0)
        {
            *dqt_index_diff = ((*dqt_index_diff) > (jpg_msg->dqtable_index)) ? jpg_msg->dqtable_index : (*dqt_index_diff);
            updata_dqt      = 1;
        }
    }
    if (updata_dqt == 0)
    {
        *dqt_index_diff = 0;
    }

    return updata_dqt;
}

static void jpg_msi_DQT_updata(struct jpg_V3_msi_s *jpg_msg, uint8_t upOdown, uint8_t diff)
{
    uint32_t *ptable   = NULL;
    int8_t    pdqt_tab = jpg_msg->dqtable_index;
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
    jpg_msg->dqtable_index = pdqt_tab;

    ptable = (uint32 *) quality_tab[pdqt_tab];
    jpg_updata_dqt(jpg_msg->jpg, ptable);
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

    fb = fbpool_get(&jpg_msg->pool, 0, jpg_msg->msi);
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

    // 如果应用需要关闭,等待关闭流程
    if (!jpg_msg->end_flag)
    {
        jpg_set_ready(jpg_msg->jpg);
    }

    // jpg_open(jpg_msg->jpg);
    send_fb->datatag = jpg_msg->datatag;
    if (os_msgq_put(&jpg_msg->msgq, (uint32_t) send_fb, 0))
    {
        err             = RET_ERR;
        jpg_msg->err_fb = send_fb;
    }
    else
    {
        uint32_t time = 0;
        // send_fb->mtype = F_JPG_NODE;
        if (jpg_msg->src_from == GEN420_DATA)
        {
            // 配置这个是从gen420来的数据,后续要考虑是否要独立配置类型(因为gen420来源数据可能是内存或者其他来源),或者说由其他回调接口去配置吧
            // 这里需要调用msi命令去配置
            send_fb->stype = FSTYPE_VIDEO_GEN420_DATA;
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
            if (video_msg.camera_mode == CAM_DUAL_SPLICE_SLAVE_MODE)
            {
                send_fb->srcID = FRAMEBUFF_SOURCE_CAMERA0;
            }
            else
            {
                send_fb->srcID = FRAMEBUFF_SOURCE_CAMERA0 + video_msg.video_type_cur;
            }
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
        send_fb->mtype = F_JPG_NODE;
        // 利用time的结构体保存图片长度,msi_output_fb后将数据保存到结构体再恢复
        send_fb->time  = time;
        send_fb->len   = jpg_len;
        // 唤醒workqueue去处理fb
        os_run_work(&jpg_msg->work);
    }
    // os_printf("jpglen:%d %d %d\n", jpg_len, jpg_msg->qt, jpg_msg->dqtable_index);
    uint8_t update = jpg_msi_quality_tidy(jpg_msg, jpg_len, &dqt_index_diff);
    // os_printf("quality res:%d %s%d\n", jpg_msg->qt, ((update>1)?"+":"-"),dqt_index_diff);
    if (last_qt != jpg_msg->qt)
    {
        jpg_set_qt(jpg_msg->jpg, jpg_msg->qt);
    }
    if (update)
    {
        jpg_msi_DQT_updata(jpg_msg, update, dqt_index_diff);
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
        _os_printf(KERN_DEBUG "JDE");
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
        _os_printf(KERN_ERR "JO");
        goto jpg_msi_outbuff_full_isr_end;
    }
    struct framebuff *fb = fbpool_get(&jpg_msg->pool, 0, jpg_msg->msi);
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
        if (jpg_msg->end_flag)
        {
            jpg_msg->err |= MSI_JPG_END_FLAG;
            os_event_set(&jpg_msg->evt, MSI_JPG_END_FLAG, NULL);
        }
        _os_printf(KERN_ERR "JU");
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
    if (jpg_msg->end_flag)
    {
        jpg_msg->err |= MSI_JPG_END_FLAG;
        os_event_set(&jpg_msg->evt, MSI_JPG_END_FLAG, NULL);
    }
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
    uint32_t len = (uint32_t) fb->len;
    // 将头的fb还原为节点的长度
    fb->len      = jpg_msg->jpg_node_len;

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

    msi_output_fb(jpg_msg->msi, fb, 0);
    

jpg_msi_work_end:
    // jpg异常,那么去重新启动一下jpg
    // 需要先清除对应资源才行
    if (jpg_err)
    {
        os_printf(KERN_ERR "jpg_err:%X\twhich:%d\n", jpg_err, jpg_msg->which);
        struct framebuff *tmp_fb, *now_fb, *fb;
        if (jpg_msg->err_fb)
        {
            msi_delete_fb(NULL, jpg_msg->err_fb);
            jpg_msg->err_fb = NULL;
        }
        now_fb = tmp_fb = jpg_msg->now_fb;
        if (now_fb)
        {
            msi_delete_fb(NULL, now_fb);
        }

        // 最后一次配置寄存器的fb(没有链接到now_fb)
        if (jpg_msg->last_fb)
        {
            msi_delete_fb(NULL, jpg_msg->last_fb);
        }
        jpg_msg->last_fb = NULL;
        jpg_msg->now_fb  = NULL;

        // 重新配置jpg的地址,启动
        fb = fbpool_get(&jpg_msg->pool, 0, jpg_msg->msi);
        ASSERT(fb);
        jpg_set_addr(jpg_msg->jpg, (uint32) fb->data, fb->len);
        jpg_msg->now_fb      = fb;
        jpg_msg->use_last_fb = fb;

        fb = fbpool_get(&jpg_msg->pool, 0, jpg_msg->msi);
        ASSERT(fb);
        jpg_set_addr(jpg_msg->jpg, (uint32) fb->data, fb->len);
        jpg_msg->last_fb = fb;
        jpg_msg->err     = 0;
        jpg_msg->running = 1;
        
        jpg_set_data_from(jpg_msg->jpg, jpg_msg->src_from);
        jpg_set_ready(jpg_msg->jpg);
        if (jpg_msg->src_from == VPP_DATA0 || jpg_msg->src_from == VPP_DATA1)
        {
            vppdone_func_register(VPP_JPEG0_START + jpg_msg->which, vpp_start_JPEG, (uint32_t) jpg_msg->jpg);
        }
        else
        {
            
            jpg_open(jpg_msg->jpg);
        }

        // jpg_open(jpg_msg->jpg);
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
            // 为了显示哪个jpg被释放
            if (jpg_msg->which)
            {
                msi->name = DESTROY_MSI_NAME(1);
            }
            else
            {
                msi->name = DESTROY_MSI_NAME(0);
            }
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
            vppdone_func_unregister(VPP_JPEG0_START + jpg_msg->which);
            // 先关闭workqueue(防止有报错,将jpg重新启动)
            os_work_cancle2(&jpg_msg->work, 1);
            // 如果编码源是VPP_DATA0或者VPP_DATA1就需要特定时间close
            if (jpg_msg->vpp_close_flag)
            {
                jpg_msg->end_flag = 1;
                os_event_wait(&jpg_msg->evt, MSI_JPG_END_FLAG | MSI_JPG_BUF_ERR | MSI_JPG_DONE_ERR | MSI_JPG_BUF_FULL_ERR, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 1000);
            }

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
            if (now_fb)
            {
                msi_delete_fb(NULL, now_fb);
            }

            // 最后一次配置寄存器的fb(没有链接到now_fb)
            if (jpg_msg->last_fb)
            {
                msi_delete_fb(NULL, jpg_msg->last_fb);
            }
            jpg_msg->last_fb = NULL;
            jpg_msg->now_fb  = NULL;

            if (jpg_msg->err_fb)
            {
                msi_delete_fb(NULL, jpg_msg->err_fb);
                jpg_msg->err_fb = NULL;
            }
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
                else
                {
                    os_printf(KERN_INFO "jpg_msg:%X\tjpg_msg_err:%d\n", jpg_msg, jpg_msg->err);
                    os_printf("jpg_msg->running:%d\n", jpg_msg->running);
                    os_printf("jpg_msg->err:%d\n", jpg_msg->err);
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
                        jpg_init(jpg_msg->jpg, jpg_msg->dqtable_index, jpg_msg->qt);
                        jpg_set_size(jpg_msg->jpg, jpg_msg->h, jpg_msg->w);
                        if (jpg_msg->src_from == VPP_DATA0 || jpg_msg->src_from == VPP_DATA1)
                        {
                            jpg_msg->vpp_close_flag = 1;
                        }

                        jpg_set_data_from(jpg_msg->jpg, jpg_msg->src_from);
                        jpg_set_hw_check(jpg_msg->jpg, 1);

                        // 注册中断
                        jpg_request_irq(jpg_msg->jpg, jpg_msi_outbuff_full_isr, JPG_IRQ_FLAG_JPG_BUF_FULL, msi);
                        jpg_request_irq(jpg_msg->jpg, jpg_msi_buf_err, JPG_IRQ_FLAG_ERROR, msi);
                        jpg_request_irq(jpg_msg->jpg, jpg_msi_done_isr, JPG_IRQ_FLAG_JPG_DONE, msi);

                        
                        jpg_set_vsync_dly(jpg_msg->jpg, 1);
                        jpg_select_oe_using(jpg_msg->jpg, 0, 1);

                        fb = fbpool_get(&jpg_msg->pool, 0, jpg_msg->msi);
                        ASSERT(fb);
                        jpg_set_addr(jpg_msg->jpg, (uint32) fb->data, fb->len);
                        jpg_msg->now_fb      = fb;
                        jpg_msg->use_last_fb = fb;

                        fb = fbpool_get(&jpg_msg->pool, 0, jpg_msg->msi);
                        ASSERT(fb);
                        jpg_set_addr(jpg_msg->jpg, (uint32) fb->data, fb->len);
                        
                        //设置一下scale1是手动还是自动
                        jpg_set_autoscale(jpg_msg->jpg, jpg_msg->scale1_flag);
                        // 记录最后一个配置链表
                        jpg_msg->last_fb = fb;
                        jpg_msg->running = 1;
                        jpg_set_ready(jpg_msg->jpg);
                        if (jpg_msg->src_from == VPP_DATA0 || jpg_msg->src_from == VPP_DATA1)
                        {
                            vppdone_func_register(VPP_JPEG0_START + jpg_msg->which, vpp_start_JPEG, (uint32_t) jpg_msg->jpg);
                        }
                        else
                        {
                            jpg_open(jpg_msg->jpg);
                        }
                        // jpg_open(jpg_msg->jpg);
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
                        os_work_cancle2(&jpg_msg->work, 1);
                        if (jpg_msg->scale1_flag)
                        {
                            jpg_msg->scale1_flag = 0;
                            scale_close(jpg_msg->scale_dev);
                            jpg_close(jpg_msg->jpg);
                        }
                        else if (jpg_msg->vpp_close_flag)
                        {
                            // 如果编码源是VPP_DATA0或者VPP_DATA1就需要特定时间close
                            if (jpg_msg->vpp_close_flag)
                            {
                                jpg_msg->end_flag = 1;
                                os_event_wait(&jpg_msg->evt, MSI_JPG_END_FLAG | MSI_JPG_BUF_ERR | MSI_JPG_DONE_ERR | MSI_JPG_BUF_FULL_ERR, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 100);
                            }
                            jpg_msg->vpp_close_flag = 0;
                            jpg_close(jpg_msg->jpg);
                        }
                        else
                        {
                            jpg_close(jpg_msg->jpg);
                        }
                        jpg_msg->running = 0;
                        jpg_msg->err     = 0;
                        // 清除异常
                        os_event_wait(&jpg_msg->evt, MSI_JPG_DONE_ERR | MSI_JPG_BUF_FULL_ERR | MSI_JPG_BUF_ERR, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
                        // 需要清理一下队列?这个时候队列的数据应该是不需要了以及now_fb的数据也要清除
                        {
                            struct framebuff *fb;
                            int32             err = 0;
                            if (jpg_msg->err_fb)
                            {
                                msi_delete_fb(NULL, jpg_msg->err_fb);
                                jpg_msg->err_fb = NULL;
                            }
                            // 将队列的数据帮忙发送出去
                            while (!err)
                            {
                                fb = (struct framebuff *) os_msgq_get2(&jpg_msg->msgq, 0, &err);
                                if (!err)
                                {
                                    // 中断将长度记录在msi的结构体中
                                    uint32_t len                = (uint32_t) fb->len;
                                    fb->len                     = jpg_msg->jpg_node_len;
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
                                    msi_output_fb(jpg_msg->msi, fb, 0);
                                }
                            }
                        }

                        // 中断的临时资源清除
                        {
                            struct framebuff *tmp_fb, *now_fb;
                            now_fb = tmp_fb = jpg_msg->now_fb;
                            if (now_fb)
                            {
                                msi_delete_fb(NULL, now_fb);
                            }

                            // 最后一次配置寄存器的fb(没有链接到now_fb)
                            if (jpg_msg->last_fb)
                            {
                                msi_delete_fb(NULL, jpg_msg->last_fb);
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

                case MSI_JPEG_SET_LEN:
                {
                    jpg_msg->target_len = arg;
                }
                break;

                case MSI_JPEG_SET_SCALE1_FLAG:
                {
                    jpg_msg->scale1_flag = arg;
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
                sys_dcache_invalid_range((uint32_t *) fb->data, fb->len);
            }
            if (fb->priv)
            {
                STREAM_LIBC_FREE(fb->priv);
                fb->priv = NULL;
            }
            // 防止有异常,这里将长度还原为节点长度
            fb->len = jpg_msg->jpg_node_len;
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
        sys_dcache_invalid_range((uint32_t *) buff, jpg_node_len);
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
        jpg_msg->qt       = 0xf;
        // 这里式分配多少帧,如果分配1帧,就是应用要快速去处理,否则可能来不及
        // 如果帧数据量大,就可能出现最后节点不够的可能
        os_msgq_init(&jpg_msg->msgq, 1);
        os_event_init(&jpg_msg->evt);
        if (msi)
        {
            fbpool_init(&jpg_msg->pool, jpg_node_count, NULL, NULL);
            jpg_msg->msi            = msi;
            // 固定输出,外部尽量不要用这个msi,统一给到RS_JPG_CONCAT这个msi去管理
            jpg_msg->jpg_node_len   = jpg_node_len;
            jpg_msg->jpg_node_count = jpg_node_count;
            jpg_msg->jpg            = (struct jpg_device *) dev_get(which_jpg + HG_JPG0_DEVID);
            jpg_msg->scale_dev      = (struct scale_device *) dev_get(HG_SCALE1_DEVID);
            jpg_close(jpg_msg->jpg);
            // 预分配buf空间到各个fb中
            uint16_t init_count = 0;
            uint8_t *m_buff;
            // 初始化framebuffer节点数量空间?最后由workqueue去从ringbuf去获取一个节点
            while (init_count < jpg_node_count)
            {
                m_buff = (uint8_t *) jpg_msg->jpg_node_buf[init_count];
                FBPOOL_SET_INFO(&jpg_msg->pool, init_count, m_buff, jpg_node_len, NULL);
                init_count++;
            }
            jpg_msg->dqtable_index = DQT_DEF;
            jpg_msg->target_len    = TARGET_JPG_LEN;
            jpg_msg->qt            = 0xf;
            msi->action            = jpg_msi_action;
            msi->enable            = 1;
            // 启动一个workqueue,去将fb发送出去,中断唤醒一次,workqueue执行一次
            OS_WORK_INIT(&jpg_msg->work, jpg_msi_work, 0);
        }
    }

    if (!msi)
    {
        if (jpg_msg)
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
    }
    else
    {
        g_jpg_msi[which_jpg] = msi;
    }
    return msi;
}

void jpg_mem_init(int num)
{
    return;
}

// 减少频繁申请空间,使用mjpg自己的内存池,暂时只是支持了预先分配
#else
int jpg_quality_pidCtrl(struct jpg_V3_msi_s *jpg_msg, int diff, int p, int i, int d)
{
    int32_t res = p * diff + i * jpg_msg->diff_sum + d * (diff - jpg_msg->diff_prev);
    jpg_msg->diff_sum += diff;
    jpg_msg->diff_prev = diff;
    res                = res >> 16;
    res                = LIMITING(res, 120, -120);
    jpg_msg->diff_sum  = LIMITING(jpg_msg->diff_sum, 2000, -2000);
    //_os_printf("diff_sum:%d\n",diff_sum);
    return res;
}

static uint8_t jpg_msi_quality_tidy(struct jpg_V3_msi_s *jpg_msg, uint32_t cur_len, uint8_t *dqt_index_diff)
{
    uint8_t updata_dqt    = 0;
    int32_t jpg_len_diff  = cur_len - jpg_msg->target_len;
    int32_t res           = jpg_quality_pidCtrl(jpg_msg, jpg_len_diff, QUALITY_CTRL_P, QUALITY_CTRL_I, QUALITY_CTRL_D);
    uint8_t qt_diff       = os_abs(res) % 0x10;
    uint8_t qt_index_diff = os_abs(res) / 0x10;
    *dqt_index_diff       = qt_index_diff;
    if (res > 0)
    {
        if (qt_diff + (jpg_msg->qt) > 0xf)
        {
            if ((jpg_msg->dqtable_index + qt_index_diff + 1 + (qt_diff - (0xf - jpg_msg->qt)) / 8) > DQT_MAX_INDEX)
            {
                *dqt_index_diff = DQT_MAX_INDEX - jpg_msg->dqtable_index;
                jpg_msg->qt     = 0xf;
            }
            else
            {
                jpg_msg->qt     = 0x8 + (qt_diff - (0xf - jpg_msg->qt)) % 8;
                *dqt_index_diff = qt_index_diff + 1 + (qt_diff - (0xf - jpg_msg->qt)) / 8;
            }
        }
        else
        {
            jpg_msg->qt += qt_diff;
        }
        if ((*dqt_index_diff) >= 1 && jpg_msg->dqtable_index < DQT_MAX_INDEX)
        {
            *dqt_index_diff = ((*dqt_index_diff) > (DQT_MAX_INDEX - jpg_msg->dqtable_index)) ? (DQT_MAX_INDEX - jpg_msg->dqtable_index) : (*dqt_index_diff);
            updata_dqt      = 2;
        }
    }
    else if (res < 0)
    {
        if (jpg_msg->qt - qt_diff < 0)
        {
            if ((jpg_msg->dqtable_index - (qt_index_diff + 1 + (qt_diff - jpg_msg->qt) / 8)) < 0)
            {
                *dqt_index_diff = jpg_msg->dqtable_index;
                jpg_msg->qt     = 0;
            }
            else
            {
                jpg_msg->qt     = 0x8 - (qt_diff - jpg_msg->qt) % 8; //(qt_diff-(*qt)) maybe > 0x8
                *dqt_index_diff = qt_index_diff + 1 + (qt_diff - jpg_msg->qt) / 8;
            }
        }
        else
        {
            jpg_msg->qt -= qt_diff;
        }
        if ((*dqt_index_diff) >= 1 && jpg_msg->dqtable_index > 0)
        {
            *dqt_index_diff = ((*dqt_index_diff) > (jpg_msg->dqtable_index)) ? jpg_msg->dqtable_index : (*dqt_index_diff);
            updata_dqt      = 1;
        }
    }
    if (updata_dqt == 0)
    {
        *dqt_index_diff = 0;
    }
    return updata_dqt;
}

static void jpg_msi_DQT_updata(struct jpg_V3_msi_s *jpg_msg, uint8_t upOdown, uint8_t diff)
{
    uint32_t *ptable   = NULL;
    int8_t    pdqt_tab = jpg_msg->dqtable_index;
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
    jpg_msg->dqtable_index = pdqt_tab;

    ptable = (uint32 *) quality_tab[pdqt_tab];
    jpg_updata_dqt(jpg_msg->jpg, ptable);
}

static uint32_t jpg_node_free(void *node)
{
    void *tmp_node, *now_node;
    if (node)
    {
        now_node = tmp_node = node;
        while (now_node)
        {
            tmp_node = now_node;
            now_node = pop_node(now_node);
            // 释放tmp_node
            free_jpg_node(tmp_node);
        }
    }
    return 0;
}
static uint32_t jpg_free_res(struct jpg_V3_msi_s *jpg_msg)
{
    jpg_node_free(jpg_msg->now_node);

    if (jpg_msg->last_node)
    {
        free_jpg_node(jpg_msg->last_node);
    }
    jpg_msg->now_node  = NULL;
    jpg_msg->last_node = NULL;
    return 0;
}

static int32 jpg_msi_done_isr(uint32 irq_flag, uint32 irq_data, uint32 param1, uint32 param2)
{
    struct msi          *msi     = (struct msi *) irq_data;
    struct jpg_V3_msi_s *jpg_msg = (struct jpg_V3_msi_s *) msi->priv;

    uint8    dqt_index_diff = 0;
    int8_t   err            = RET_OK;
    void    *send_node      = jpg_msg->now_node;
    uint32_t jpg_len        = param1;
    uint8_t  last_qt        = jpg_msg->qt;
    void    *node;
    uint16_t len;
    // 如果异常,这里关闭jpg,workqueue去清理资源,重新启动mjpg
    if (jpg_msg->err)
    {
        err = RET_ERR;
        goto jpg_msi_done_isr_end;
    }


    node = jpg_msg->last_node;
    // 这里不应该进来,进来后,需要检查是否正常
    if (!node)
    {
        err = RET_ERR;
        goto jpg_msi_done_isr_end;
    }
    len = get_node_len(node);
    jpg_set_addr(jpg_msg->jpg, (uint32) node, len);

    jpg_msg->last_node = node;

    node = get_jpg_node(&len);
    if (!node)
    {
        err = RET_ERR;
        goto jpg_msi_done_isr_end;
    }

    // 这里确认足够空间重新启动jpg,可以将now_fb重新赋值
    jpg_msg->now_node      = jpg_msg->last_node;
    jpg_msg->use_last_node = jpg_msg->last_node;

    // 配置第二次的寄存器
    jpg_set_addr(jpg_msg->jpg, (uint32) node, len);
    jpg_msg->last_node = node;

    // 如果应用需要关闭,等待关闭流程
    if (!jpg_msg->end_flag)
    {
        jpg_set_ready(jpg_msg->jpg);
    }
    
    if (os_msgq_put(&jpg_msg->msgq, (uint32_t) send_node, 0))
    {
        jpg_node_free(send_node);
    }
    else
    {
        // 检查是否有足够保留空间使用
        struct jpg_isr_msg *msg = get_node_rev(send_node, sizeof(struct jpg_isr_msg));
        if (msg)
        {
            uint32_t time = 0;
            if (jpg_msg->src_from == GEN420_DATA)
            {
                // 配置这个是从gen420来的数据,后续要考虑是否要独立配置类型(因为gen420来源数据可能是内存或者其他来源),或者说由其他回调接口去配置吧
                // 这里需要调用msi命令去配置
                msg->stype = jpg_msg->gen420_type;
                msg->srcID = FRAMEBUFF_SOURCE_JPG_GEN420;
                time       = jpg_msg->set_time;
            }
            else if (jpg_msg->src_from == SCALER_DATA)
            {
                msg->stype = jpg_msg->scale1_type ? jpg_msg->scale1_type : (FSTYPE_VIDEO_VPP_DATA0 + jpg_msg->src_from);
                msg->srcID = FRAMEBUFF_SOURCE_JPG_SCALER;
                time       = jpg_msg->set_time;
            }
            else
            {
                msg->srcID = FRAMEBUFF_SOURCE_CAMERA0 + video_msg.video_type_cur;
                msg->stype = FSTYPE_VIDEO_VPP_DATA0 + jpg_msg->src_from;
            }

            if (!time)
            {
                time = os_jiffies();
            }

            // 配置子类型
            msg->datatag = jpg_msg->datatag;
            msg->w       = jpg_msg->w;
            msg->h       = jpg_msg->h;
            // 利用time的结构体保存图片长度,msi_output_fb后将数据保存到结构体再恢复
            msg->time    = time;
            msg->len     = (void *) jpg_len;
        }
        os_run_work(&jpg_msg->work);
    }
#if 1
    // os_printf("jpglen:%d %d %d\ttartget:%d\n", jpg_len, jpg_msg->qt, jpg_msg->dqtable_index,jpg_msg->target_len);

    uint8_t update = jpg_msi_quality_tidy(jpg_msg, jpg_len, &dqt_index_diff);
    // os_printf("quality res:%d %s%d\n", jpg_msg->qt, ((update>1)?"+":"-"),dqt_index_diff);
    if (last_qt != jpg_msg->qt)
    {
        jpg_set_qt(jpg_msg->jpg, jpg_msg->qt);
    }
    if (update)
    {
        jpg_msi_DQT_updata(jpg_msg, update, dqt_index_diff);
    }
#endif

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
    void                *node;
    uint16_t             len;

    if (jpg_msg->err)
    {
        goto jpg_msi_outbuff_full_isr_end;
    }

    node = get_jpg_node(&len);
    if (!node)
    {
        err = RET_ERR;
        goto jpg_msi_outbuff_full_isr_end;
    }
    jpg_set_addr(jpg_msg->jpg, (uint32) node, len);
    ASSERT(jpg_msg->last_node);

    // 放到链表里面(now_fb为head)
    jpg_msg->use_last_node = push_node(jpg_msg->use_last_node, jpg_msg->last_node);
    jpg_msg->last_node     = node;
jpg_msi_outbuff_full_isr_end:
    if (err != RET_OK)
    {
        jpg_msg->err |= MSI_JPG_BUF_FULL_ERR;
        os_event_set(&jpg_msg->evt, MSI_JPG_BUF_FULL_ERR, NULL);
        if (jpg_msg->end_flag)
        {
            jpg_msg->err |= MSI_JPG_END_FLAG;
            os_event_set(&jpg_msg->evt, MSI_JPG_END_FLAG, NULL);
        }
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
    if (jpg_msg->end_flag)
    {
        jpg_msg->err |= MSI_JPG_END_FLAG;
        os_event_set(&jpg_msg->evt, MSI_JPG_END_FLAG, NULL);
    }
    _os_printf(KERN_ERR "JE");
    return 0;
}

static uint32_t jpg_start_set_addr(struct jpg_V3_msi_s *jpg_msg)
{
    uint16_t node_len;
    void    *node;
    node = get_jpg_node(&node_len);
    ASSERT(node);
    jpg_set_addr(jpg_msg->jpg, (uint32) node, node_len);
    jpg_msg->now_node      = node;
    jpg_msg->use_last_node = node;

    node = get_jpg_node(&node_len);
    ASSERT(node);
    jpg_set_addr(jpg_msg->jpg, (uint32) node, node_len);
    // 记录最后一个配置链表
    jpg_msg->last_node = node;
    return 0;
}

static uint8_t jpg_output(struct jpg_V3_msi_s *jpg_msg, void *node)
{
    uint8_t             ret       = 0;
    struct jpg_isr_msg *msg       = (struct jpg_isr_msg *) get_node_rev(node, sizeof(struct jpg_isr_msg));
    struct framebuff   *fb        = NULL;
    struct framebuff   *last_fb   = NULL;
    void               *next_node = node;
    struct framebuff   *output_fb = NULL;

    if (msg)
    {
        uint32_t jpg_len = msg->len;
        uint32_t fb_len;
        ASSERT(next_node);
        while (next_node)
        {
            // 将node节点给到fb
            next_node = pop_node(node);
            fb_len    = jpg_len > get_node_len(node) ? get_node_len(node) : jpg_len;
            fb        = msi_alloc_fb(jpg_msg->msi, NULL, node, fb_len, 0, 0);
            ASSERT(fb);
            fb->mtype = F_JPG_NODE;
            fb->stype = msg->stype;
            if (last_fb)
            {
                last_fb->next = fb;
            }
            else
            {
                output_fb = fb;
            }
            jpg_len -= fb_len;
            last_fb = fb;
            node    = next_node;
        }

        struct jpg_node_s *jpg_priv = (struct jpg_node_s *) STREAM_LIBC_MALLOC(sizeof(struct jpg_node_s));
        if (jpg_priv)
        {
            jpg_priv->w        = msg->w;
            jpg_priv->h        = msg->h;
            jpg_priv->jpg_len  = msg->len;
            output_fb->time    = msg->time;
            output_fb->datatag = msg->datatag;
            output_fb->mtype   = F_JPG_NODE;
            output_fb->priv    = jpg_priv;
            output_fb->srcID   = msg->srcID;
        }
        else
        {
            output_fb->priv = NULL;
        }
        msi_output_fb(jpg_msg->msi, output_fb, 0);
    }
    else
    {
        return 1;
    }
    return ret;
}
static int32 jpg_msi_work(struct os_work *work)
{
    struct jpg_V3_msi_s *jpg_msg = (struct jpg_V3_msi_s *) work;
    void                *node;
    int32                err     = -1;
    uint32               jpg_err = 0;

    os_event_wait(&jpg_msg->evt, MSI_JPG_DONE_ERR | MSI_JPG_BUF_FULL_ERR | MSI_JPG_BUF_ERR, &jpg_err, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
    node = (void *) os_msgq_get2(&jpg_msg->msgq, 0, &err);
    if (err)
    {
        goto jpg_msi_work_end;
    }

    if (!jpg_output(jpg_msg, node))
    {
    }
    // 异常,没有保留信息,只能释放
    else
    {
        jpg_node_free(node);
    }

jpg_msi_work_end:
    // jpg异常,那么去重新启动一下jpg
    // 需要先清除对应资源才行
    if (jpg_err)
    {
        os_printf(KERN_ERR "jpg_err:%X\twhich:%d\n", jpg_err, jpg_msg->which);
        jpg_free_res(jpg_msg);
        jpg_start_set_addr(jpg_msg);
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
            os_msgq_del(&jpg_msg->msgq);
            os_event_del(&jpg_msg->evt);
            msi->name = NULL; // 这里比较特殊,正常不能清空的
            STREAM_LIBC_FREE(jpg_msg);
        }
        break;
        case MSI_CMD_PRE_DESTROY:
        {
            // 先关闭workqueue(防止有报错,将jpg重新启动)
            os_work_cancle2(&jpg_msg->work, 1);

            // 如果编码源是VPP_DATA0或者VPP_DATA1就需要特定时间close
            if (jpg_msg->vpp_close_flag)
            {
                jpg_msg->end_flag = 1;
                os_event_wait(&jpg_msg->evt, MSI_JPG_END_FLAG | MSI_JPG_BUF_ERR | MSI_JPG_DONE_ERR | MSI_JPG_BUF_FULL_ERR, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 100);
            }

            // 关闭jpg
            if (jpg_msg->running)
            {
                jpg_close(jpg_msg->jpg);
                jpg_msg->running = 0;
            }

            g_jpg_msi[jpg_msg->which] = NULL;

            jpg_free_res(jpg_msg);
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

                        // 硬件初始化
                        jpg_init(jpg_msg->jpg, jpg_msg->dqtable_index, jpg_msg->qt);
                        jpg_set_size(jpg_msg->jpg, jpg_msg->h, jpg_msg->w);
                        if (jpg_msg->src_from == VPP_DATA0 || jpg_msg->src_from == VPP_DATA1)
                        {
                            jpg_msg->vpp_close_flag = 1;
                        }
                        jpg_set_data_from(jpg_msg->jpg, jpg_msg->src_from);
                        jpg_set_hw_check(jpg_msg->jpg, 1);

                        // 注册中断
                        jpg_request_irq(jpg_msg->jpg, jpg_msi_outbuff_full_isr, JPG_IRQ_FLAG_JPG_BUF_FULL, msi);
                        jpg_request_irq(jpg_msg->jpg, jpg_msi_buf_err, JPG_IRQ_FLAG_ERROR, msi);
                        jpg_request_irq(jpg_msg->jpg, jpg_msi_done_isr, JPG_IRQ_FLAG_JPG_DONE, msi);

                        jpg_set_ready(jpg_msg->jpg);
                        jpg_set_vsync_dly(jpg_msg->jpg, 1);
                        jpg_select_oe_using(jpg_msg->jpg, 0, 1);

                        jpg_start_set_addr(jpg_msg);
                        os_event_wait(&jpg_msg->evt, MSI_SCALE1_DATA0_DATA1_CLOSE, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
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
                        os_work_cancle2(&jpg_msg->work, 1);
                        if (jpg_msg->scale1_flag)
                        {
                            jpg_msg->scale1_flag = 0;
                            scale_close(jpg_msg->scale_dev);
                            jpg_close(jpg_msg->jpg);
                        }
                        else if (jpg_msg->vpp_close_flag)
                        {
                            // 如果编码源是VPP_DATA0或者VPP_DATA1就需要特定时间close
                            if (jpg_msg->vpp_close_flag)
                            {
                                jpg_msg->end_flag = 1;
                                os_event_wait(&jpg_msg->evt, MSI_JPG_END_FLAG | MSI_JPG_BUF_ERR | MSI_JPG_DONE_ERR | MSI_JPG_BUF_FULL_ERR, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 100);
                            }
                            jpg_msg->vpp_close_flag = 0;
                            jpg_close(jpg_msg->jpg);
                        }
                        else
                        {
                            jpg_close(jpg_msg->jpg);
                        }

                        jpg_msg->running = 0;
                        jpg_msg->err     = 0;
                        // 清除异常
                        os_event_wait(&jpg_msg->evt, MSI_JPG_DONE_ERR | MSI_JPG_BUF_FULL_ERR | MSI_JPG_BUF_ERR, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
                        // 需要清理一下队列?这个时候队列的数据应该是不需要了以及now_fb的数据也要清除
                        {
                            void *node;
                            int32 err = 0;
                            // 将队列的数据帮忙发送出去
                            while (!err)
                            {
                                node = (struct framebuff *) os_msgq_get2(&jpg_msg->msgq, 0, &err);
                                if (!err)
                                {
                                    if (!jpg_output(jpg_msg, node))
                                    {
                                    }
                                    // 异常,没有保留信息,只能释放
                                    else
                                    {
                                        jpg_node_free(node);
                                    }
                                }
                            }
                        }
                        jpg_free_res(jpg_msg);
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

                case MSI_JPEG_SET_LEN:
                {
                    jpg_msg->target_len = arg;
                }
                break;

                case MSI_JPEG_SET_SCALE1_FLAG:
                {
                    jpg_msg->scale1_flag = arg;
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
                free_jpg_node((void *) fb->data);
                fb->data = NULL;
            }
            if (fb->priv)
            {
                STREAM_LIBC_FREE(fb->priv);
                fb->priv = NULL;
            }
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
    struct jpg_V3_msi_s *jpg_msg = (struct jpg_V3_msi_s *) STREAM_LIBC_ZALLOC(sizeof(struct jpg_V3_msi_s));
    struct msi          *msi     = NULL;
    uint8_t              isnew   = 0;
    if (jpg_msg)
    {
        os_sprintf(jpg_msg->msi_name, "H_JPG%d_%08X", which_jpg, (uint32) os_jiffies() & 0xFFFFFFFF);
        msi = msi_new(jpg_msg->msi_name, 0, &isnew);
        ASSERT(msi);
        // 这里一定是新创建
        ASSERT(isnew);
        msi->priv         = (void *) jpg_msg;
        jpg_msg->which    = which_jpg;
        jpg_msg->src_from = src_from;
        jpg_msg->qt       = 0xf;
        // 这里式分配多少帧,如果分配1帧,就是应用要快速去处理,否则可能来不及
        // 如果帧数据量大,就可能出现最后节点不够的可能
        os_msgq_init(&jpg_msg->msgq, 1);
        os_event_init(&jpg_msg->evt);
        if (msi)
        {
            jpg_msg->msi       = msi;
            // 固定输出,外部尽量不要用这个msi,统一给到RS_JPG_CONCAT这个msi去管理
            jpg_msg->jpg       = (struct jpg_device *) dev_get(which_jpg + HG_JPG0_DEVID);
            jpg_msg->scale_dev = (struct scale_device *) dev_get(HG_SCALE1_DEVID);
            jpg_close(jpg_msg->jpg);
            jpg_msg->dqtable_index = DQT_DEF;
            jpg_msg->target_len    = TARGET_JPG_LEN;
            jpg_msg->qt            = 0xf;
            msi->action            = jpg_msi_action;
            msi->enable            = 1;
            // 启动一个workqueue,去将fb发送出去,中断唤醒一次,workqueue执行一次
            OS_WORK_INIT(&jpg_msg->work, jpg_msi_work, 0);
        }
    }

    if (!msi)
    {
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

void jpg_mem_init(int num)
{
    add_jpg_block(num);
    return;
}
#endif