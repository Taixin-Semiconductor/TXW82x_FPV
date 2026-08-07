#include "basic_include.h"

#include "lib/multimedia/msi.h"

#include "stream_define.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "jpg_concat_msi.h"
#include "user_work/user_work.h"

extern uint8_t get_vpp_w_h(uint16_t *w, uint16_t *h);
extern uint8_t get_vpp1_w_h(uint16_t *w, uint16_t *h);

// 这里是默认值,最好在project_config.h那里配置
#ifndef JPG_NODE_COUNT
#define JPG_NODE_COUNT 10
#endif

// 最大支持处理200K的图片
#define MAX_JPG_SIZE  (200 * 1024)
// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

#define MAX_JPG_CONCAT_RECV (8)
extern struct msi *hardware_jpg_msi(uint8_t which_jpg, uint8_t src_from, uint16_t jpg_node_len, uint16_t jpg_node_count);

int32_t jpg_concat_msg_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t                  ret            = RET_OK;
    struct jpg_concat_msi_s *jpg_concat_msg = (struct jpg_concat_msi_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            os_work_cancle2(&jpg_concat_msg->work, 1);
            fbpool_destroy(&jpg_concat_msg->tx_pool);
            STREAM_LIBC_FREE(jpg_concat_msg);
        }
        break;

        // 需要关闭,则将关联的msi关闭
        case MSI_CMD_PRE_DESTROY:
        {
            os_work_cancle2(&jpg_concat_msg->work, 1);
            if (jpg_concat_msg->jpg_msi)
            {
                msi_destroy(jpg_concat_msg->jpg_msi);
                jpg_concat_msg->jpg_msi = NULL;
            }
        }
        break;

        case MSI_CMD_GET_RUNNING:
        {
            uint32_t running = 0;
            if (jpg_concat_msg->jpg_msi)
            {
                msi_do_cmd(jpg_concat_msg->jpg_msi, MSI_CMD_GET_RUNNING, (uint32_t)&running, 0);
            }
            *(uint32_t *) param1 = running;
        }
        break;

        case MSI_CMD_JPEG_CONCAT:
        {
            uint32_t cmd_self = (uint32_t) param1;
            uint32_t arg      = param2;
            switch (cmd_self)
            {
                case MSI_JPEG_OPEN:
                {
                    // 如果jpg_msi已经启动,则报错,需要应用层检查是否停止
                    if (jpg_concat_msg->jpg_msi)
                    {
                        ret = 1;
                    }
                    else
                    {
                        jpg_concat_msg->from = arg;
                    }
                }
                break;
                // 配置JPEG的参数?(这样就是固定size了,如果希望编码不同size,如何解决呢,注册形式？注册两个分辨率,然后由中断去切换?)
                // 暂时没有实现

                // 发命令配置jpg编码的参数
                case MSI_JPEG_MSG:
                {
                    jpg_concat_msg->w = arg >> 16;
                    jpg_concat_msg->h = arg & 0xffff;
                    // msi_do_cmd(jpg_concat_msg->jpg_msi, MSI_CMD_HARDWARE_JPEG, MSI_JPEG_HARDWARE_MSG, arg);
                }
                break;
                // 启动mjpeg
                case MSI_JPEG_START:
                {
                    if (arg)
                    {
                        if (!jpg_concat_msg->jpg_msi)
                        {
                            jpg_concat_msg->jpg_msi = hardware_jpg_msi(jpg_concat_msg->which_jpg, jpg_concat_msg->from, 16 * 1024, jpg_concat_msg->jpg_node_count);
                            if (jpg_concat_msg->jpg_msi)
                            {
                                msi_add_output(jpg_concat_msg->jpg_msi, NULL, msi->name);
                            }
                        }
                        if (jpg_concat_msg->jpg_msi)
                        {
                            msi_do_cmd(jpg_concat_msg->jpg_msi, MSI_CMD_SET_DATATAG, jpg_concat_msg->datatag, 0);
                            msi_do_cmd(jpg_concat_msg->jpg_msi, MSI_CMD_HARDWARE_JPEG, MSI_JPEG_HARDWARE_FROM, jpg_concat_msg->from);
                            msi_do_cmd(jpg_concat_msg->jpg_msi, MSI_CMD_HARDWARE_JPEG, MSI_JPEG_HARDWARE_SET_GEN420_TYPE, jpg_concat_msg->gen420_type);
                            msi_do_cmd(jpg_concat_msg->jpg_msi, MSI_CMD_HARDWARE_JPEG, MSI_JPEG_HARDWARE_SET_SCALE1_TYPE, jpg_concat_msg->scale1_type);
                            uint16_t jpg_w, jpg_h;
                            if (jpg_concat_msg->from == VPP_DATA0)
                            {
                                get_vpp_w_h(&jpg_w, &jpg_h);
                            }
                            else if (jpg_concat_msg->from == VPP_DATA1)
                            {
                                get_vpp1_w_h(&jpg_w, &jpg_h);
                            }
                            else
                            {
                                jpg_w = jpg_concat_msg->w;
                                jpg_h = jpg_concat_msg->h;
                            }
                            // 其他的来源,则使用配置参数
                            msi_do_cmd(jpg_concat_msg->jpg_msi, MSI_CMD_HARDWARE_JPEG, MSI_JPEG_SET_TIME, jpg_concat_msg->set_time);
                            msi_do_cmd(jpg_concat_msg->jpg_msi, MSI_CMD_HARDWARE_JPEG, MSI_JPEG_HARDWARE_MSG, jpg_w << 16 | jpg_h);
                            msi_do_cmd(jpg_concat_msg->jpg_msi, MSI_CMD_HARDWARE_JPEG, MSI_JPEG_HARDWARE_START, 0);
                        }
                        else
                        {
                            ret = RET_ERR;
                        }
                    }
                    else
                    {

                        if (jpg_concat_msg->jpg_msi)
                        {
                            msi_do_cmd(jpg_concat_msg->jpg_msi, MSI_CMD_HARDWARE_JPEG, MSI_JPEG_HARDWARE_STOP, 0);
                            if (jpg_concat_msg->auto_free)
                            {
                                // 删除jpg_msi,主要为了释放空间
                                msi_destroy(jpg_concat_msg->jpg_msi);
                                jpg_concat_msg->jpg_msi = NULL;
                            }
                        }
                    }
                }
                break;

                // 配置mjpeg的源头
                case MSI_JPEG_FROM:
                {
                    jpg_concat_msg->from = arg;
                }
                break;

                case MSI_SET_GEN420_TYPE:
                {
                    jpg_concat_msg->gen420_type = arg;
                    break;
                }

                case MSI_SET_SCALE1_TYPE:
                {
                    jpg_concat_msg->scale1_type = arg;
                    break;
                }
                case MSI_JPEG_NODE_COUNT:
                {
                    jpg_concat_msg->jpg_node_count = arg;
                    break;
                }

                case MSI_GET_JPEG_MSI:
                {
                    struct msi **jpg_msi = (struct msi **) param2;
                    // 返回jpg_msi,主要为了外部获取(比如gen420,因为硬件之间耦合,所以这里做了中间层去获取,可能是jpg0可能是jpg1)
                    if (jpg_concat_msg->jpg_msi && jpg_msi)
                    {
                        *jpg_msi = jpg_concat_msg->jpg_msi;
                    }
                    else
                    {
                        ret = RET_ERR;
                    }
                }
                break;

                case MSI_SET_TIME:
                {
                    jpg_concat_msg->set_time = arg;
                }
                break;

                default:
                    break;
            }
        }
        break;
        // 接收到数据,唤醒workqueue,去组图,理论有图都要重新组,然后通过output的时候过滤类型,这里也可以先过滤部分类型
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            if (jpg_concat_msg->filter_type)
            {
                ret            = RET_ERR;
                // 轮询类型是否有一致
                uint16_t *each = jpg_concat_msg->filter_type;
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

            // 判断图片大小是否超过一定size,如果是,就不支持拼接(主要是空间问题)
            // 除了类型外,其他都是克隆发出去,其他模块处理要注意
            // data是没有用的,因为是节点的形式
            if (ret == RET_OK)
            {
                struct jpg_node_s *jpg_priv = (struct jpg_node_s *) fb->priv;
                if (jpg_priv)
                {
                    if (jpg_priv->jpg_len > MAX_JPG_SIZE)
                    {
                        struct framebuff *send_fb = fb_clone(fb, fb->mtype << 8 | fb->stype, msi);
                        send_fb->len              = jpg_priv->jpg_len;
                        send_fb->data             = NULL;
                        msi_output_fb(msi, send_fb);
                        ret = RET_OK + 1;
                    }
                }
            }

            if (ret == RET_OK)
            {
                os_run_work(&jpg_concat_msg->work);
            }
        }
        break;
        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            if (fb->mtype == F_JPG)
            {
                if (fb->data)
                {
                    STREAM_FREE(fb->data);
                    fb->data = NULL;
                }
                if (fb->priv)
                {
                    STREAM_LIBC_FREE(fb->priv);
                    fb->priv = NULL;
                }
                fbpool_put(&jpg_concat_msg->tx_pool, fb);
                // 不需要内核去释放fb
                ret = RET_OK + 1;
            }
            // 那么应该是克隆的,由msi内核管理
            else if (fb->clone)
            {
            }
            // 异常情况
            else
            {
                os_printf(KERN_ERR "%s:%d free fb type err:%d\n", __FUNCTION__, __LINE__, fb->mtype);
            }
        }
        break;

        case MSI_CMD_SET_DATATAG:
        {
            jpg_concat_msg->datatag = param1;
        }
        break;
        default:
            break;
    }
    return ret;
}

static int32 jpg_concat_msi_work(struct os_work *work)
{
    struct jpg_concat_msi_s *jpg_concat_msg = (struct jpg_concat_msi_s *) work;
    struct framebuff        *fb;
    struct framebuff        *send_fb;
    uint32_t                 jpg_len;
    uint8_t                 *jpg_psram_space;
    // 接收图片数据看看
    fb = msi_get_fb(jpg_concat_msg->msi, 0);
    if (fb)
    {
        struct jpg_node_s *jpg_priv = (struct jpg_node_s *) fb->priv;
        if (jpg_priv)
        {
            jpg_len         = jpg_priv->jpg_len;
            jpg_psram_space = (uint8_t *) STREAM_MALLOC(jpg_len);
            if (jpg_psram_space)
            {
                send_fb = fbpool_get(&jpg_concat_msg->tx_pool, 0, jpg_concat_msg->msi);
                if (!send_fb)
                {
                    STREAM_FREE(jpg_psram_space);
                }
                else
                {
                    uint32_t          offset     = 0;
                    uint32_t          remain_len = jpg_len;
                    uint32_t          cp_len     = 0;
                    struct framebuff *tmp_fb     = fb;
                    // 开始拷贝数据
                    while (tmp_fb && remain_len)
                    {
                        if (remain_len > tmp_fb->len)
                        {
                            cp_len = tmp_fb->len;
                        }
                        else
                        {
                            cp_len = remain_len;
                        }
                        hw_memcpy(jpg_psram_space + offset, tmp_fb->data, cp_len);
                        offset += cp_len;
                        remain_len -= cp_len;
                        tmp_fb = tmp_fb->next;
                    }
                    sys_dcache_clean_range((uint32_t *) jpg_psram_space, jpg_len);
                    send_fb->data    = jpg_psram_space;
                    send_fb->len     = jpg_len;
                    send_fb->mtype   = F_JPG;
                    send_fb->stype   = fb->stype;
                    send_fb->time    = fb->time;
                    send_fb->datatag = fb->datatag;
                    send_fb->srcID   = fb->srcID;
                    _os_printf("V");

                    struct jpg_node_s *jpg_msg = (struct jpg_node_s *) STREAM_LIBC_MALLOC(sizeof(struct jpg_node_s));
                    if (jpg_msg)
                    {
                        memcpy(jpg_msg, jpg_priv, sizeof(struct jpg_node_s));
                        send_fb->priv = (void *) jpg_msg;
                        msi_output_fb(jpg_concat_msg->msi, send_fb);
                    }
                    // 申请不到内存,则不发送?正常不应该申请不到空间
                    else
                    {
                        os_printf("%s:%d malloc jpg_node_s fail\n", __FUNCTION__, __LINE__);
                        msi_delete_fb(jpg_concat_msg->msi, send_fb);
                    }
                }
            }
        }
        else
        {
            os_printf("%s:%d find jpg msg fail\n", __FUNCTION__, __LINE__);
        }

        // 将数据拷贝,然后发送出去
        msi_delete_fb(NULL, fb);
    }
    return 0;
}

// 这里还需要做一个事情,就是如果用了VPP_DATA0,应该自动识别分辨率,而不是通过外部传进来
// 暂时自动识别还没有做
struct msi *jpg_concat_msi_init_start(uint32_t jpgid, uint16_t w, uint16_t h, uint16_t *filter_type, uint8_t src_from, uint8_t run)
{
    ASSERT((jpgid == 0) || (jpgid == 1));
    const char *msi_name =  NULL;
    uint8_t     which_jpg = jpgid;
    if (jpgid == 0)
    {
        msi_name = RS_JPG_CONCAT;
    }
    else if (jpgid == 1)
    {
        msi_name = RS_JPG_CONCAT1;
    }
    uint8_t     isnew;
    struct msi *msi = msi_new(msi_name, MAX_JPG_CONCAT_RECV, &isnew);
    ASSERT(msi);
    struct jpg_concat_msi_s *jpg_concat_msg = (struct jpg_concat_msi_s *) msi->priv;
    if (msi && !jpg_concat_msg)
    {
        jpg_concat_msg = (struct jpg_concat_msi_s *) STREAM_LIBC_ZALLOC(sizeof(struct jpg_concat_msi_s));
        ASSERT(jpg_concat_msg);
        msi->priv                      = (void *) jpg_concat_msg;
        msi->enable                    = 1;
        jpg_concat_msg->which_jpg      = which_jpg; // 默认使用jpg0
        jpg_concat_msg->msi            = msi;
        jpg_concat_msg->msi->action    = jpg_concat_msg_msi_action;
        jpg_concat_msg->msi->priv      = (void *) jpg_concat_msg;
        jpg_concat_msg->msi->enable    = 1;
        jpg_concat_msg->filter_type    = filter_type;
        jpg_concat_msg->from           = src_from;
        jpg_concat_msg->auto_free      = 1;
        jpg_concat_msg->jpg_node_count = JPG_NODE_COUNT;
        fbpool_init(&jpg_concat_msg->tx_pool, MAX_JPG_CONCAT_RECV);
        jpg_concat_msg->w = w;
        jpg_concat_msg->h = h;
        // 启动workqueue
        OS_WORK_INIT(&jpg_concat_msg->work, jpg_concat_msi_work, 0);
        msi_do_cmd(msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_OPEN, src_from);
        // 关闭jpg
        msi_do_cmd(msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 0);
        // 修改jpg参数
        msi_do_cmd(msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_MSG, w << 16 | h);

        // 启动mjpg
        if (run)
        {
            msi_do_cmd(msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 1);
        }
    }
    return msi;
}
