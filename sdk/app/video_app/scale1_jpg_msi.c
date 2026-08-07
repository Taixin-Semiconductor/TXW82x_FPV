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
#include "hal/scale.h"
#include "dev/scale/hgscale.h"
#include "video_msi.h"
#define STATIC_SCALE1_BUF 1
#define SCALE1_JPG_COUNT  20
extern uint8 *yuvbuf;
extern int32_t vpp_evt_wait_open(int enable, int timeout);

enum
{
    MSI_SCALE1_THREAD_DEAD = BIT(0),
    MSI_SCALE1_THREAD_STOP = BIT(1),
    MSI_SCALE1_THREAD_KICK = BIT(2),
};

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

#define SCALE1_RECV_MAX 1
struct scale1_jpg_msi_s
{
    struct msi          *msi;
    struct msi          *register_jpg_msi; // 注册的jpg的msi,因为gen420需要与jpg或者其他硬件联动
    struct scale_device *scale_dev;
    struct framebuff    *fb;
    scale1_filter_fn     fn;
    struct os_event      evt;
    uint32_t             magic;
    uint16_t             last_w, last_h;
    uint16_t             jpg_w, jpg_h;
    uint8_t              src_from;
    uint8_t              stop;
    uint8_t              which_jpg;
    uint8_t              lock_value;
    uint8_t              output_en;
    uint8_t             *scale1_buf;
    uint32_t             scale1_buf_size;
    uint16_t             force_type; // 设置最后输出图片的类型
    uint8_t              recv_type;
};

static int32_t scale1_soft_ov_isr(uint32_t irq_flag, uint32_t irq_data, uint32_t param1)
{
    // struct scale_device *scale_dev = (struct scale_device *)irq_data;
    //_os_printf("ov");
    return 0;
}

static int32_t scale1_done(uint32_t irq_flag, uint32_t irq_data, uint32_t param1)
{
    os_printf("sd$$$$\r\n");
    struct scale1_jpg_msi_s *scale1_jpg = (struct scale1_jpg_msi_s *) irq_data;
    //struct scale_device     *scale_dev  = scale1_jpg->scale_dev;
    // 完成后,再次kick一下线程,检查是否有新的数据
    os_event_set(&scale1_jpg->evt, MSI_SCALE1_THREAD_KICK, NULL);
    return 0;
}

static void scale1_soft_from_psram_to_enc(struct scale1_jpg_msi_s *scale1_jpg, uint8_t *line_buf, int line_num, uint8_t *psram_data, uint32_t w, uint32 h, uint32_t ow, uint32_t oh)
{
    uint16               icount    = 2;
    uint32_t             running   = 0;
    struct scale_device *scale_dev = scale1_jpg->scale_dev;
    os_printf("%s  %d*%d====>%d*%d\r\n", __func__, w, h, ow, oh);
    scale_close(scale_dev);
    scale_set_in_out_size(scale_dev, w, h, ow, oh);
    scale_set_step(scale_dev, w, h, ow, oh);
    scale_set_start_addr(scale_dev, 0, 0);
    scale_set_data_from_vpp(scale_dev, 0);
    scale_set_line_buf_num(scale_dev, line_num * 2); // soft的line buf
    scale_request_irq(scale_dev, FRAME_END, scale1_done, (uint32_t) scale1_jpg);
    // scale_request_irq(scale_dev,INBUF_OV,scale1_soft_ov_isr,(uint32_t)scale1_jpg);
    scale_open(scale_dev);

    scale_set_inbuf_num(scale_dev, 0, 0);
    if (scale_get_inbuf_num(scale_dev) == 0)
    {
        icount = 2;
        scale_set_new_frame(scale_dev, 1);
    }
    else
    {
        return;
    }

    hw_memcpy(line_buf, psram_data, (line_num * icount) * w);
    hw_memcpy(line_buf + (line_num * icount) * w, psram_data + h * w, (line_num * icount) * w / 4);
    hw_memcpy(line_buf + (line_num * icount) * w + (line_num * icount) * w / 4, psram_data + h * w + h * w / 4, (line_num * icount) * w / 4);
    scale_set_in_yaddr(scale_dev, (uint32) line_buf);
    scale_set_in_uaddr(scale_dev, (uint32) line_buf + (line_num * icount) * w);
    scale_set_in_vaddr(scale_dev, (uint32) line_buf + (line_num * icount) * w + (line_num * icount) * w / 4);

    scale_set_inbuf_num(scale_dev, icount * line_num - 1, line_num * 2 - 1 /*31*/); // 0~31

    uint32_t count = 0;
    while (1)
    {
        if (scale_get_heigh_cnt(scale_dev) > ((icount - 1) * line_num))
        {
            if ((icount % 2) == 0)
            {
                // os_printf("get_height_cnt:%d===>up head\r\n",scale_get_heigh_cnt(scale_dev));
                hw_memcpy(line_buf, psram_data + (line_num * icount) * w, line_num * w);
                hw_memcpy(line_buf + line_num * 2 * w, psram_data + h * w + (line_num * icount) * w / 4, line_num * w / 4);
                hw_memcpy(line_buf + line_num * 2 * w + line_num * 2 * w / 4, psram_data + h * w + h * w / 4 + (line_num * icount) * w / 4, line_num * w / 4);
            }
            else
            {
                // os_printf("get_height_cnt:%d===>up tail\r\n",scale_get_heigh_cnt(scale_dev));
                hw_memcpy(line_buf + line_num * w, psram_data + (line_num * icount) * w, line_num * w);
                hw_memcpy(line_buf + line_num * 2 * w + line_num * 2 * w / 8, psram_data + h * w + (line_num * icount) * w / 4, line_num * w / 4);
                hw_memcpy(line_buf + line_num * 2 * w + line_num * 2 * w / 4 + line_num * 2 * w / 8, psram_data + h * w + h * w / 4 + (line_num * icount) * w / 4, line_num * w / 4);
            }
            icount++;
            if (icount == ((h + line_num - 1) / line_num))
            {
                if ((icount % 2) == 1) // 3   start
                {
                    scale_set_inbuf_num(scale_dev, icount * line_num, line_num);
                }
                else
                {
                    scale_set_inbuf_num(scale_dev, icount * line_num, 0);
                }
                // 延时,为了硬件可以去编码(看起来不是绝对安全)
                os_sleep_ms(1);
                break;
            }
            if ((icount % 2) == 1) // 3   start
            {
                scale_set_inbuf_num(scale_dev, icount * line_num, line_num);
            }
            else
            {
                scale_set_inbuf_num(scale_dev, icount * line_num, 0);
            }
        }
        else
        {
            msi_do_cmd(scale1_jpg->register_jpg_msi, MSI_CMD_GET_RUNNING,(uint32_t) &running, 0);
            if (running == 0)
            {
                os_printf("%s:%d\n", __FUNCTION__, __LINE__);
                os_printf("scale_get_heigh_cnt(scale_dev):%d\n", scale_get_heigh_cnt(scale_dev));
                break;
            }
            count++;
            os_sleep_ms(1);
        }

        if (count % 1000 == 0)
        {
            os_printf("%s:%d\n", __FUNCTION__, __LINE__);
        }
    }
}

static void scale1_jpg_work(void *d)
{
    uint8_t last_value   = 0;
    uint8_t already_kick = 0;

    uint32_t    delay_time = -1;
    uint32_t    flags;
    int32_t     ret;
    //struct msi *msi = scale1_jpg_s->msi;
	struct scale1_jpg_msi_s *scale1_jpg_s = (struct scale1_jpg_msi_s *)d;
    while (1)
    {
    scale1_jpg_work_again:

        if (scale1_jpg_s->scale1_buf || already_kick)
        {
            delay_time = 1000;
        }
        else
        {
            delay_time = -1;
        }
        flags = 0;
        ret   = os_event_wait(&scale1_jpg_s->evt, MSI_SCALE1_THREAD_KICK | MSI_SCALE1_THREAD_STOP, &flags, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, delay_time);
        // 如果有对应的line_buf,则设置为超时1000ms,否则不需要超时

        if (flags & MSI_SCALE1_THREAD_STOP)
        {
            goto scale1_jpg_work_end;
        }
        // 如果gen420没有启动,那么一定可以启动mjpg或者gen420
        // 尝试看看是否有新的数据需要编码
        if (!scale1_jpg_s->fb)
        {
            scale1_jpg_s->fb = msi_get_fb(scale1_jpg_s->msi, 0);
        }
        // 有yuv数据,开始去拉伸编码
        if (scale1_jpg_s->fb)
        {
            // 没有接收的目标,直接放弃产生大分辨率图片
            if (msi_output_fb(scale1_jpg_s->msi, NULL) == 0)
            {
                msi_delete_fb(NULL, scale1_jpg_s->fb);
                scale1_jpg_s->fb = NULL;
                goto scale1_jpg_work_again;
            }

            // 去获取jpg的锁
            int32_t ret = jpg_mutex_lock(scale1_jpg_s->which_jpg, scale1_jpg_s->lock_value, NULL);
            // 获取成功,则去编码
            if (ret == 0)
            {
                //  获取成功后,初始化mjpg1相关硬件
                if (!scale1_jpg_s->register_jpg_msi)
                {
                    scale1_jpg_s->register_jpg_msi = jpg_concat_msi_init_start(scale1_jpg_s->which_jpg, scale1_jpg_s->jpg_w, scale1_jpg_s->jpg_h, NULL, scale1_jpg_s->src_from, 0);
                    if (scale1_jpg_s->register_jpg_msi)
                    {
                        // 主动停止一下
                        msi_do_cmd(scale1_jpg_s->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 0);
                        msi_add_output(scale1_jpg_s->register_jpg_msi, NULL, scale1_jpg_s->msi->name);
                    }
                }

                if (scale1_jpg_s->register_jpg_msi)
                {
                    msi_do_cmd(scale1_jpg_s->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_OPEN, SCALER_DATA);
                    msi_do_cmd(scale1_jpg_s->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_NODE_COUNT, SCALE1_JPG_COUNT);
                }

                // 异常,不应该出现,除非其他地方打开了占用该jpg需要检查,应该应用去规避
                else
                {
                    jpg_mutex_unlock(scale1_jpg_s->which_jpg, scale1_jpg_s->lock_value);
                    os_printf("scale1_jpg_s->register_jpg_msi init err\r\n");
                    delay_time = 1;
                    goto scale1_jpg_work_again;
                }

                // 关闭mjpeg
                msi_do_cmd(scale1_jpg_s->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 0);
                msi_do_cmd(scale1_jpg_s->register_jpg_msi, MSI_CMD_SET_DATATAG, scale1_jpg_s->fb->datatag, 0);
                msi_do_cmd(scale1_jpg_s->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_SET_SCALE1_TYPE, scale1_jpg_s->force_type ? scale1_jpg_s->force_type : scale1_jpg_s->fb->stype);
                msi_do_cmd(scale1_jpg_s->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_FROM, scale1_jpg_s->src_from);
                msi_do_cmd(scale1_jpg_s->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_MSG, scale1_jpg_s->jpg_w << 16 | scale1_jpg_s->jpg_h);
                // 重新启动mjpg
                msi_do_cmd(scale1_jpg_s->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 1);

                struct yuv_arg_s *yuv_msg;
                yuv_msg = (struct yuv_arg_s *) scale1_jpg_s->fb->priv;
                if (yuv_msg)
                {
                    uint32_t p_w, p_h;
                    p_w                  = yuv_msg->out_w;
                    p_h                  = yuv_msg->out_h;
                    scale1_jpg_s->last_w = p_w;
                    scale1_jpg_s->last_h = p_h;
                    scale1_jpg_s->stop   = 0;

                    // 行数不够,直接复用yuv的buf
                    if (STATIC_SCALE1_BUF || scale1_jpg_s->jpg_w <= 2 * p_w)
                    {
                        // yuvbuf需要给28行,传入14行
                        if (yuvbuf)
                        {
                            vpp_evt_wait_open(0, 100);

                            uint32_t start_encode = os_jiffies();
                            scale1_soft_from_psram_to_enc(scale1_jpg_s, yuvbuf, 14, scale1_jpg_s->fb->data, p_w, p_h, scale1_jpg_s->jpg_w, scale1_jpg_s->jpg_h);
                            vpp_evt_wait_open(1, 0);
                            os_printf(KERN_INFO "spend1 time:%d\n", (uint32_t) os_jiffies() - start_encode);
                            already_kick = 1;
                            jpg_mutex_unlock(scale1_jpg_s->which_jpg, scale1_jpg_s->lock_value);
                            msi_delete_fb(NULL, scale1_jpg_s->fb);
                            scale1_jpg_s->fb = NULL;
                        }
                        // 空间申请失败,下次尝试申请
                        else
                        {
                            jpg_mutex_unlock(scale1_jpg_s->which_jpg, scale1_jpg_s->lock_value);
                            delay_time = 1;
                            goto scale1_jpg_work_again;
                        }
                    }
                    else
                    {
                        if (!scale1_jpg_s->scale1_buf || scale1_jpg_s->scale1_buf_size < 16 * p_w * 3 / 2)
                        {
                            if (scale1_jpg_s->scale1_buf)
                            {
                                STREAM_LIBC_FREE(scale1_jpg_s->scale1_buf);
                            }
                            scale1_jpg_s->scale1_buf_size = (10 * 2) * p_w * 3 / 2;
                            scale1_jpg_s->scale1_buf      = STREAM_LIBC_MALLOC(scale1_jpg_s->scale1_buf_size);
                            if (!scale1_jpg_s->scale1_buf)
                            {
                                scale1_jpg_s->scale1_buf_size = 0;
                            }
                        }

                        if (scale1_jpg_s->scale1_buf)
                        {
                            uint32_t start_encode = os_jiffies();
                            scale1_soft_from_psram_to_enc(scale1_jpg_s, scale1_jpg_s->scale1_buf, 10, scale1_jpg_s->fb->data, p_w, p_h, scale1_jpg_s->jpg_w, scale1_jpg_s->jpg_h);
                            os_printf("spend2 time:%d\n", (uint32_t) os_jiffies() - start_encode);
                            already_kick = 1;
                            jpg_mutex_unlock(scale1_jpg_s->which_jpg, scale1_jpg_s->lock_value);
                            msi_delete_fb(NULL, scale1_jpg_s->fb);
                            scale1_jpg_s->fb = NULL;
                        }
                        // 空间申请失败,下次尝试申请
                        else
                        {
                            jpg_mutex_unlock(scale1_jpg_s->which_jpg, scale1_jpg_s->lock_value);
                            delay_time = 1;
                            goto scale1_jpg_work_again;
                        }
                    }
                }
                // 不应该进入这里,可能fb给错了,下一次移除
                else
                {
                    // 移除
                    msi_delete_fb(NULL, scale1_jpg_s->fb);
                    scale1_jpg_s->fb = NULL;
                }
            }
            else
            {
                delay_time = 1;
                goto scale1_jpg_work_again;
            }
        }
        else
        {
            // 超时,所以去释放内存,充分利用内存
            if (ret)
            {
                if (scale1_jpg_s->scale1_buf)
                {
                    STREAM_LIBC_FREE(scale1_jpg_s->scale1_buf);
                    scale1_jpg_s->scale1_buf = NULL;
                }
                scale1_jpg_s->scale1_buf_size = 0;

                int32_t close_ret = jpg_mutex_lock(scale1_jpg_s->which_jpg, scale1_jpg_s->lock_value, &last_value);
                // 既然释放了scale1的空间,那么也去停止jpg硬件吧
                if (close_ret == 0)
                {
                    if ((last_value == scale1_jpg_s->lock_value))
                    {
                        if (scale1_jpg_s->register_jpg_msi)
                        {
                            // 主动停止一下
                            msi_do_cmd(scale1_jpg_s->register_jpg_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 0);
                        }
                    }
                    jpg_mutex_unlock(scale1_jpg_s->which_jpg, scale1_jpg_s->lock_value);
                }
                already_kick = 0;
            }
        }
    }
scale1_jpg_work_end:
    if (scale1_jpg_s->fb)
    {
        msi_delete_fb(NULL, scale1_jpg_s->fb);
        scale1_jpg_s->fb = NULL;
    }
    os_event_set(&scale1_jpg_s->evt, MSI_SCALE1_THREAD_DEAD, NULL);
    msi_put(scale1_jpg_s->msi);
    return;
}

static int32_t scale1_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t                  ret          = RET_OK;
    struct scale1_jpg_msi_s *scale1_jpg_s = (struct scale1_jpg_msi_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            os_event_wait(&scale1_jpg_s->evt, MSI_SCALE1_THREAD_DEAD, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, -1);
            if (scale1_jpg_s->register_jpg_msi)
            {
                msi_destroy(scale1_jpg_s->register_jpg_msi);
                scale1_jpg_s->register_jpg_msi = NULL;
            }

            if (scale1_jpg_s->scale1_buf)
            {
                STREAM_LIBC_FREE(scale1_jpg_s->scale1_buf);
            }
            os_event_del(&scale1_jpg_s->evt);
            STREAM_LIBC_FREE(scale1_jpg_s);
        }
        break;
        case MSI_CMD_PRE_DESTROY:
        {
            os_event_set(&scale1_jpg_s->evt, MSI_SCALE1_THREAD_STOP, NULL);
        }
        break;

        // 由于拥有重编码的能力,这里就监听MSI_CMD_JPG_RECODE事件
        case MSI_CMD_JPG_RECODE:
        {
            uint32_t cmd_self = (uint32_t) param1;
            uint32_t arg      = param2;
            switch (cmd_self)
            {
                // 需要注册jpg,中断可能需要用到
                case MSI_JPG_RECODE_MAGIC:
                {
                    scale1_jpg_s->magic = arg;
                }
                break;
                case MSI_JPG_RECODE_FORCE_TYPE:
                {
                    scale1_jpg_s->force_type = (uint16_t) arg;
                }
                break;

                default:
                    break;
            }
        }
        break;

        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb        = (struct framebuff *) param1;
            uint8_t           send_flag = 0;
            // 判断是否要转发
            if (scale1_jpg_s->fn)
            {
                if (scale1_jpg_s->fn((void *) fb, scale1_jpg_s->recv_type) == 0)
                {
                    send_flag = 1;
                }
            }
            // 如果没有过滤,将所有jpg都转发?
            else if ((fb->mtype == F_JPG) || (fb->mtype == F_JPG_NODE))
            {
                send_flag = 1;
            }

            if (send_flag)
            {
                // 所有图片都转发
                struct framebuff *fb = (struct framebuff *) param1;
                fb_get(fb);
                int send = msi_output_fb(msi, fb);
                if (!send)
                {
                    scale1_jpg_s->output_en = 0;
                }
                os_printf("send:%d\n", send);
                ret = RET_OK + 1;
                break;
            }

            if (fb->mtype != F_YUV)
            {
                ret = RET_ERR;
            }
            else
            {
                struct yuv_arg_s *yuv_msg;
                yuv_msg = (struct yuv_arg_s *) fb->priv;
                // 如果magic为0,不需要处理
                if (!scale1_jpg_s->magic)
                {
                    ret = RET_OK;
                }
                // magic不是0,则需要匹配
                else if (scale1_jpg_s->magic == yuv_msg->magic)
                {
                    ret = RET_OK;
                }
                else
                {
                    ret = RET_ERR;
                }
            }
        }
        break;
        case MSI_CMD_TRANS_FB_END:
        {
            os_event_set(&scale1_jpg_s->evt, MSI_SCALE1_THREAD_KICK, NULL);
            break;
        }

        case MSI_CMD_SCALE1:
        {
            uint32_t cmd_self = (uint32_t) param1;
            uint32_t arg      = param2;
            switch (cmd_self)
            {
                case MSI_SCALE1_RESET_DPI:
                {
                    scale1_jpg_s->jpg_w = arg >> 16;
                    scale1_jpg_s->jpg_h = arg & 0xffff;
                }
                break;
                default:
                    break;
            }
        }
        break;

        default:
            break;
    }
    return ret;
}

struct msi *scale1_jpg_msi_init(const char *msi_name, uint8_t which_jpg, uint8_t recv_type, uint8_t lock_value, uint16_t force_type, scale1_filter_fn fn)
{
    uint8_t isnew;
    ASSERT(which_jpg == 0 || which_jpg == 1);
    struct msi              *msi          = msi_new(msi_name, SCALE1_RECV_MAX, &isnew);
    struct scale1_jpg_msi_s *scale1_jpg_s = (struct scale1_jpg_msi_s *) msi->priv;
    if (isnew)
    {
        scale1_jpg_s             = (struct scale1_jpg_msi_s *) STREAM_LIBC_ZALLOC(sizeof(struct scale1_jpg_msi_s));
        msi->priv                = (void *) scale1_jpg_s;
        msi->action              = scale1_msi_action;
        scale1_jpg_s->msi        = msi;
        scale1_jpg_s->force_type = force_type; // 强转类型
        scale1_jpg_s->stop       = 1;
        scale1_jpg_s->which_jpg  = which_jpg;
        scale1_jpg_s->lock_value = lock_value;
        scale1_jpg_s->fn         = fn;
        scale1_jpg_s->output_en  = 0;
        scale1_jpg_s->jpg_w      = 4000;
        scale1_jpg_s->jpg_h      = 2992;
        scale1_jpg_s->src_from   = SCALER_DATA;
        scale1_jpg_s->scale_dev  = (struct scale_device *) dev_get(HG_SCALE1_DEVID);
        scale1_jpg_s->recv_type  = recv_type;
        os_event_init(&scale1_jpg_s->evt);

        // 将大分辨率拍照改成线程(因为消耗时间过长)
        // 这里get一次,线程内部去释放
        msi_get(msi);
        msi->enable = 1;
        os_task_create("scale1_jpg_thread", scale1_jpg_work, scale1_jpg_s, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 1024);
    }

    return msi;
}