/************************************************************************
 * 这个是拍照以及缩略图模块的初始化demo
 * 会涉及其他的msi调用
 ***********************************************************************/
#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "stream_define.h"
#include "osal/string.h"
#include "dev/vpp/hgvpp.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "lib/video/vpp/vpp_dev.h"
#include "dev/scale/hgscale.h"
#include "jpg_concat_msi.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/scale/scale_dev.h"
#include "gen420_hardware_msi.h"
#include "hal/jpeg.h"
#include "video_msi.h"
struct msi *scale3_normal_msi(const char *name, uint16_t ow, uint16_t oh);
extern struct msi *jpg_decode_msi(const char *name);
extern struct msi *jpg_decode_msg_msi(const char *name, uint16_t out_w, uint16_t out_h, uint16_t step_w, uint16_t step_h, uint32_t filter);
extern struct msi *jpg_thumb_msi_init(const char *msi_name, uint16_t filter, uint8_t thumb_stype);
extern void        scale_from_soft_to_jpg(struct scale_device *scale_dev, uint32 yuvbuf_addr, uint32 s_w, uint32 s_h, uint32 d_w, uint32 d_h);
// 过滤类型,因为缩略图的stype一定大于FSYPTE_INVALID
static uint8_t     filter(void *f, uint8_t recv_type)
{
    struct framebuff *fb  = (struct framebuff *) f;
    uint8_t           res = 1;

    if (fb->mtype == F_JPG && fb->stype == recv_type)
    {
        res = 0;
    }

    return res;
}



static void takephoto_photo_init(const char *thumb_msi_name)
{
    // 将AUTO_JPG绑定缩略图
    msi_add_output(NULL, AUTO_JPG, R_JPG_THUMB); // 拍照缩略图


    // 启动接收线程,这个主要是拍照用,获取到照片后,如果有缩略图需要,则会传输到下一个数据流
    struct msi *jpg_thumb_msi = jpg_thumb_msi_init(R_JPG_THUMB, FRAMEBUFF_SOURCE_CAMERA0, FSTYPE_NORMAL_THUMB_JPG);
    if (jpg_thumb_msi && thumb_msi_name)
    {
        // 给到缩略图模块去解码后编码小缩略图
        msi_add_output(jpg_thumb_msi, NULL, thumb_msi_name);
        // 文件保存的msi
        msi_add_output(jpg_thumb_msi, NULL, R_FILE_MSI);
    }
}



static uint8_t filter_720P(void *f, uint8_t recv_type)
{
    struct framebuff *fb  = (struct framebuff *) f;
    uint8_t           res = 1;

    if (fb->mtype == F_JPG && fb->stype == recv_type)
    {
        res = 0;
    }

    return res;
}

static void takephoto_photo_720P_init(const char *thumb_msi_name)
{
    // 将AUTO_JPG绑定缩略图
    //msi_add_output(NULL, AUTO_JPG, R_JPG_THUMB); // 拍照缩略图

    struct msi *scale3_msi = scale3_normal_msi(S_SCALE3_720P, 1280, 720);
    struct msi *gen420_jpg_msi = gen420_jpg_msi_init(SR_GEN420_720P_JPG, JPGID0, FSTYPE_GEN420_720P, JPG_LOCK_NORMAL_ENCODE, GEN420_QUEUE_JPEG, NULL, filter_720P);
    
    if(scale3_msi && gen420_jpg_msi)
    {
        msi_do_cmd(gen420_jpg_msi,MSI_CMD_JPG_RECODE,MSI_JPG_RECODE_FORCE_TYPE,FSTYPE_GEN420_720P);
        msi_add_output(scale3_msi,NULL,gen420_jpg_msi->name);
        msi_add_output(gen420_jpg_msi, NULL, R_JPG_THUMB);
    }
    // 启动接收线程,这个主要是拍照用,获取到照片后,如果有缩略图需要,则会传输到下一个数据流
    struct msi *jpg_thumb_msi = jpg_thumb_msi_init(R_JPG_THUMB, FRAMEBUFF_SOURCE_JPG_GEN420, FSTYPE_NORMAL_THUMB_JPG);
    if (jpg_thumb_msi && thumb_msi_name)
    {
        // 给到缩略图模块去解码后编码小缩略图
        msi_add_output(jpg_thumb_msi, NULL, thumb_msi_name);
        // 文件保存的msi
        msi_add_output(jpg_thumb_msi, NULL, R_FILE_MSI);
    }
}

/****************************************************************************************************************************
 * 缩略图生成绑定
 * thumb_msi_name->R_THUMB_DECODE_MSG->S_JPG_DECODE->R_GEN420_THUMB_JPG(接收yuv数据,通过gen420去编码,生成图片)
 *    ^                                                         |
 *    |                                                         |(将jpg图片传递会给thumb_msi_name去保存)
 *    |                                                         V
 *     ----------------------------------------------------------
 *
 * R_GEN420_THUMB_JPG:既做接收也做发送,先接收yuv,kick gen420去编码,然后接收生成的jpg的图片,将jpg图片转发给thumb_msi_name去保存
 ***************************************************************************************************************************/

static void takephoto_thumb_init(const char *thumb_msi_name)
{
    uint32_t    magic;
    // 启动解码模块,名称已经固定,参数已经无效
    struct msi *decode_msi = jpg_decode_msi(S_JPG_DECODE);
    if (decode_msi)
    {
        // 给到gen420去重新生成缩略图jpg
        msi_add_output(decode_msi, NULL, R_GEN420_THUMB_JPG);
    }

    // 生成缩略图的size
    struct msi *decode_msg_msi = jpg_decode_msg_msi(R_THUMB_DECODE_MSG, 320, 180, 320, 180, 0);
    // 生成一个随机magic
    do
    {
        magic = os_jiffies();
        magic ^= (uint32_t) decode_msg_msi;
    } while (!magic);

    if (decode_msg_msi && decode_msi)
    {
        // 设置magic
        msi_do_cmd(decode_msg_msi, MSI_CMD_DECODE_JPEG_MSG, MSI_JPEG_DECODE_MAGIC, magic);
        msi_add_output(decode_msg_msi, NULL, decode_msi->name);
    }

    struct msi *thumb_msi = thumb_over_dpi_msi_init(thumb_msi_name, 0, FSTYPE_NORMAL_THUMB_JPG, magic);
    if (thumb_msi)
    {
        // 处理原图,然后给到解码后生成缩略图
        msi_add_output(thumb_msi, NULL, R_THUMB_DECODE_MSG);

        // 同时会将有一个需要保存的文件
        msi_add_output(thumb_msi, NULL, R_FILE_MSI);
    }

    // 启动一个专门用yuv->gen420->mjpg的模块,这个模块会接收mjpg图片,并且通过filter函数决定是否转发
    struct msi *gen420_jpg_msi = gen420_jpg_msi_init(R_GEN420_THUMB_JPG, JPGID0, FSTYPE_NORMAL_THUMB_JPG,JPG_LOCK_GEN420_THUBM_ENCODE, GEN420_QUEUE_THUMB_JPEG, NULL, filter);
    if (gen420_jpg_msi && thumb_msi)
    {
        // 设置magic
        msi_do_cmd(gen420_jpg_msi, MSI_CMD_JPG_RECODE, MSI_JPG_RECODE_MAGIC, magic);
        msi_add_output(gen420_jpg_msi, NULL, thumb_msi->name);
    }
}

// 只能调用一次,因为内部很多msi的名称都是固定的,如果需要复用,需要将内部实现调整
void takephoto_with_thumb_init(const char *thumb_msi_name)
{
    takephoto_photo_init(thumb_msi_name);
    takephoto_thumb_init(thumb_msi_name);
}

void takephoto_720P_with_thumb_init(const char *thumb_msi_name)
{
    takephoto_photo_720P_init(thumb_msi_name);
    takephoto_thumb_init(thumb_msi_name);
}

#define MAX_USER_VIDEO_TX 16
static int net_video_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int ret = RET_OK;
    switch (cmd_id)
    {

        // 暂时没有考虑释放
        case MSI_CMD_POST_DESTROY:
        {
        }
        break;
        // 接收,判断是否已经压缩了
        case MSI_CMD_TRANS_FB:
        {
            // struct framebuff *fb = (struct framebuff *)param1;
            // if(fb->stype != FSTYPE_H264_GEN420_DATA)
            //{
            //     ret = RET_ERR;
            // }
        }
        break;
        case MSI_CMD_FREE_FB:
        {
        }
        break;
        default:
            break;
    }
    return ret;
}

int32_t thumb_gen420_kick()
{
    return 0;
}

#define THUMB_W 128
#define THUMB_H 96

extern uint8         *yuvbuf;
extern volatile uint8 scale_take_photo;
uint8                 take_photo_psram_ybuf_src[1280 * 720 + 1280 * 720 / 2] __attribute__((aligned(4), section(".psram.src")));
uint8                *take_photo_thumb_yuv;
extern volatile uint8 itp_done;
extern volatile uint8 thumb_done;
extern uint8         *yuvbuf1;
void                  take_photo_with_big_size(void *d)
{
    uint32_t                 framelen = 0;
    uint8_t                  framenum = 0;
    uint8_t                  thumb; // = 1;
    uint32_t                 retval   = 0;
    uint32_t                 time_out = 0;
    uint32_t                 w; // = 4000;
    uint32_t                 h; // = 2992;
    uint16_t                 iw, ih;
    uint16_t                 stype;
    uint32                  *pixel_itp;
    struct jpg_concat_msi_s *jpg_concat_msg;
    struct jpg_V3_msi_s     *jpg_msg;
    struct framebuff        *jpg_fb = NULL;
    struct dma_device       *dma1_dev;
    struct vpp_device       *vpp_dev;
    struct scale_device     *scale_dev;
    struct scale_device     *scale2_dev;
    struct scale_device     *scale3_dev;
    struct msi              *jpg_concat_msi;
    struct msi              *smsi;
    struct framebuff        *fb;
    struct yuv_arg_s        *yuv_msg = NULL;

    struct msi *msi = msi_new("BIG_JPG", MAX_USER_VIDEO_TX, NULL);
    pixel_itp       = (uint32 *) d;
    w               = pixel_itp[0] & 0xffff;
    h               = (pixel_itp[0] >> 16) & 0xffff;
    thumb           = pixel_itp[1] & 0xff;
    smsi            = (struct msi *) pixel_itp[2];
    stype           = pixel_itp[3];
    vpp_dev         = (struct vpp_device *) dev_get(HG_VPP_DEVID);
    scale_dev       = (struct scale_device *) dev_get(HG_SCALE1_DEVID);
    scale2_dev      = (struct scale_device *) dev_get(HG_SCALE2_DEVID);
    scale3_dev      = (struct scale_device *) dev_get(HG_SCALE3_DEVID);
    dma1_dev        = (struct dma_device *) dev_get(HG_M2MDMA_DEVID);
    msi->action     = net_video_msi_action;
    msi->enable     = 1;

    take_photo_thumb_yuv = av_psram_malloc(THUMB_W * THUMB_H + THUMB_W * THUMB_H / 2);
    msi_add_output(0, RS_JPG_CONCAT, "BIG_JPG");
    jpg_concat_msi = jpg_concat_msi_init_start(JPGID0, w, h, 0, GEN420_DATA, 1);
    jpg_concat_msg = (struct jpg_concat_msi_s *) jpg_concat_msi->priv;
    jpg_msg        = (struct jpg_V3_msi_s *) jpg_concat_msg->jpg_msi->priv;
    if (video_msg.video_num == 1)
    {
        stype = FSTYPE_YUV_P0;
    }

photo_action:
    if (thumb == 1)
    {
        msi_do_cmd(jpg_concat_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 0);
        msi_do_cmd(jpg_concat_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_FROM, GEN420_DATA);
        msi_do_cmd(jpg_concat_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_MSG, THUMB_W << 16 | THUMB_H);
        msi_do_cmd(jpg_concat_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 1);

        if (scale_get_is_open(scale3_dev))
        { // 判断scaler3是否在启动，启动的话，因为scale3是跟vpp绑定，直接从scale那边抽出数据进行缩略图处理
            smsi->enable = 1;
        retget:
            fb = msi_get_fb(smsi, 0);
            if (fb)
            {
                yuv_msg = (struct yuv_arg_s *) fb->priv;
                _os_printf("w:%d * %d\r\n", yuv_msg->out_w, yuv_msg->out_h);
                if (fb->stype != stype)
                {
                    msi_delete_fb(NULL, fb);
                    goto retget;
                }
            }
            else
            {
                os_sleep_ms(1);
                goto retget;
            }
            iw = yuv_msg->out_w;
            ih = yuv_msg->out_h;

            scale2_all_frame(scale2_dev, FRAME_YUV420P, iw, ih, THUMB_W, THUMB_H, (uint32) fb->data, (uint32) take_photo_thumb_yuv);
            while (thumb_done == 0)
            {
                os_sleep_ms(2);
            }
            msi_delete_fb(NULL, fb);
            smsi->enable = 0;
            fb           = msi_get_fb(smsi, 0);
            while (fb)
            {
                msi_delete_fb(NULL, fb);
                fb = msi_get_fb(smsi, 0);
            }
        }
        else
        {
            if (video_msg.dvp_type == (stype - FSTYPE_YUV_P0))
            {
                iw = video_msg.dvp_iw;
                ih = video_msg.dvp_ih;
            }
            else if (video_msg.csi0_type == (stype - FSTYPE_YUV_P0))
            {
                iw = video_msg.csi0_iw;
                ih = video_msg.csi0_ih;
            }
            else if (video_msg.csi1_type == (stype - FSTYPE_YUV_P0))
            {
                iw = video_msg.csi1_iw;
                ih = video_msg.csi1_ih;
            }
            else
            {
                iw = 0;
                ih = 0;
            }

            while (1)
            {
                if (vpp_video_type_map(stype) == 1)
                { // 查看即将处理的摄像头是不是需要用到的摄像头
                    break;
                }
                else
                {
                    os_sleep_ms(1);
                }
            }

            thumb_done = 0;
            if (vpp_get_scale3_buf_select(vpp_dev) == 0)
            {
                scale3_to_memory_for_thumb(iw, ih, THUMB_W, THUMB_H, (uint32) take_photo_thumb_yuv, (uint32) yuvbuf, VPP_BUF0_LINEBUF_NUM);
            }
            else
            {
                scale3_to_memory_for_thumb(iw, ih, THUMB_W, THUMB_H, (uint32) take_photo_thumb_yuv, (uint32) yuvbuf1, VPP_BUF1_LINEBUF_NUM);
            }
            while (thumb_done == 0)
            {
                os_sleep_ms(2);
            }
            scale_close(scale3_dev);
        }
        _os_printf("takephoto_yuv:%x  %x\r\n", take_photo_psram_ybuf_src, take_photo_thumb_yuv);
        wake_up_gen420_queue(3, take_photo_thumb_yuv);
    }
    else
    {

        if ((video_msg.dvp_type - 1) == (stype - FSTYPE_YUV_P0))
        {
            iw = video_msg.dvp_iw;
            ih = video_msg.dvp_ih;
        }
        else if ((video_msg.csi0_type - 1) == (stype - FSTYPE_YUV_P0))
        {
            iw = video_msg.csi0_iw;
            ih = video_msg.csi0_ih;
        }
        else if ((video_msg.csi1_type - 1) == (stype - FSTYPE_YUV_P0))
        {
            iw = video_msg.csi1_iw;
            ih = video_msg.csi1_ih;
        }
        else
        {
            iw = 0;
            ih = 0;
        }

        msi_do_cmd(jpg_concat_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 0);
        msi_do_cmd(jpg_concat_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_FROM, SCALER_DATA);
        msi_do_cmd(jpg_concat_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_MSG, w << 16 | h);
        msi_do_cmd(jpg_concat_msi, MSI_CMD_JPEG_CONCAT, MSI_JPEG_START, 1);

        if (video_msg.video_num > 1)
        {
            dma_ioctl(dma1_dev, DMA_IOCTL_CMD_DMA1_LOCK, 0, 0);
            while (retval != 1)
            {
                os_sleep_ms(100);
                retval = dma_ioctl(dma1_dev, DMA_IOCTL_CMD_CHECK_DMA1_STATUS, 0, 0);
            }

            while (1)
            {
                if (vpp_video_type_map(stype) == 1)
                { // 查看即将处理的摄像头是不是需要用到的摄像头
                    break;
                }
                else
                {
                    os_sleep_ms(1);
                }
            }

            itp_done = 0;
            vpp_itp_save_only(vpp_dev, iw, ih, (uint32) take_photo_psram_ybuf_src);
            while (itp_done == 0)
            {
                os_sleep_ms(2);
            }
            dma_ioctl(dma1_dev, DMA_IOCTL_CMD_DMA1_UNLOCK, 0, 0);
            _os_printf("%s %d\r\n", __func__, __LINE__);
            vpp_close(vpp_dev);
            scale_soft_from_psram_to_enc(scale_dev, take_photo_psram_ybuf_src, iw, ih, w, h);
            vpp_open(vpp_dev);
            _os_printf("%s %d\r\n", __func__, __LINE__);
        }
        else if (w > 1920)
        { // 多镜头直接跑这里，cpu1会卡死，需另外找原因
            dma_ioctl(dma1_dev, DMA_IOCTL_CMD_DMA1_LOCK, 0, 0);
            while (retval != 1)
            {
                os_sleep_ms(100);
                retval = dma_ioctl(dma1_dev, DMA_IOCTL_CMD_CHECK_DMA1_STATUS, 0, 0);
            }

            while (1)
            {
                if (vpp_video_type_map(stype) == 1)
                { // 查看即将处理的摄像头是不是需要用到的摄像头
                    break;
                }
                else
                {
                    os_sleep_ms(1);
                }
            }

            itp_done = 0;
            scale_from_soft_to_jpg(scale_dev, (uint32) take_photo_psram_ybuf_src, iw, ih, w, h);
            while (itp_done == 0)
            {
                os_sleep_ms(2);
            }
            scale_take_photo = 0;
            while (scale_take_photo == 0)
            {
                os_sleep_ms(2);
                time_out++;
                if (time_out == 500)
                {
                    break; // timeout
                }
            }

            dma_ioctl(dma1_dev, DMA_IOCTL_CMD_DMA1_UNLOCK, 0, 0);
            vpp_open(vpp_dev);
        }
        else
        {
            while (1)
            {
                if (vpp_video_type_map(stype) == 1)
                { // 查看即将处理的摄像头是不是需要用到的摄像头
                    break;
                }
                else
                {
                    os_sleep_ms(1);
                }
            }
            scale_from_vpp(scale_dev, (uint32) yuvbuf, iw, ih, w, h);
        }
    }

    while (1)
    {
        jpg_fb = msi_get_fb(msi, 0);
        if (jpg_fb)
        {
            framenum++;
            framelen = jpg_fb->len;
            os_printf("T(%x  %d  jpg:%d*%d)", jpg_fb->data, framelen, jpg_msg->w, jpg_msg->h);
            msi_delete_fb(NULL, jpg_fb);
            if (thumb == 0)
            {
                thumb = 1;
                goto photo_action;
            }

            msi->enable = 0;
            jpg_fb      = msi_get_fb(msi, 0);
            while (jpg_fb)
            {
                msi_delete_fb(NULL, jpg_fb);
                fb = msi_get_fb(msi, 0);
            }
            av_psram_free(take_photo_thumb_yuv);
            return;
        }
        else
        {
            os_sleep_ms(3);
        }
    }
}

#define MAX_THUMB_TX 4
struct scale3_thumb_msi_s
{
    struct os_work     work;
    struct msi        *msi;
    struct os_msgqueue msgq;
    struct fbpool      tx_pool;
};

#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

static int32_t thumb_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t                    ret   = RET_OK;
    struct scale3_thumb_msi_s *thumb = (struct scale3_thumb_msi_s *) msi->priv;

    switch (cmd_id)
    {

        // 这里msi已经被删除,那么就要考虑tx_pool的资源释放了
        // 能进来这里,就是代表所有fb都已经用完了
        case MSI_CMD_POST_DESTROY:
        {

            struct framebuff *fb;
            // 释放资源fb资源文件
            while (1)
            {
                fb = fbpool_get(&thumb->tx_pool, 0, NULL);
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
            fbpool_destroy(&thumb->tx_pool);
            STREAM_LIBC_FREE(thumb);
        }
        break;

        // 停止硬件,移除没有必要的资源(但是fb的资源不能现在删除,这个时候fb可能外部还在调用)
        case MSI_CMD_PRE_DESTROY:
        {
            os_work_cancle2(&thumb->work, 1);
        }
        break;
        // 接收,判断是类型是否可以支持压缩
        case MSI_CMD_TRANS_FB:
        {
        }
        break;
        // 预先分配的,默认不需要释放fb->data,除非是MSI_CMD_DESTROY后,就要释放
        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;

            fbpool_put(&thumb->tx_pool, fb);
            // 不需要内核去释放fb
            ret = RET_OK + 1;
        }
        break;

        default:
            break;
    }
    return ret;
}

// 参数分别是vpp的图像iw和ih,要scale的ow和oh,如果和屏有关,可以传入屏幕的宽高
struct msi *thumb_msi(const char *name)
{
    struct msi                *msi   = msi_new(name, 2, NULL);
    struct scale3_thumb_msi_s *thumb = (struct scale3_thumb_msi_s *) msi->priv;
    if (!thumb)
    {
        thumb       = (void *) STREAM_LIBC_ZALLOC(sizeof(struct scale3_thumb_msi_s));
        msi->action = thumb_msi_action;
        msi->priv   = (void *) thumb;
        thumb->msi  = msi;

        fbpool_init(&thumb->tx_pool, MAX_THUMB_TX);
        uint8_t init_count = 0;
        // 初始化fb的一些默认信息
        while (init_count < MAX_THUMB_TX)
        {
            // 预分配framebuff的空间给到scale
            // 先配置参数信息
            // 由于创建流的时候,参数已经分配好了,所以这里可以确定参数
            FBPOOL_SET_INFO(&thumb->tx_pool, init_count, NULL, 0, NULL);
            init_count++;
        }
        msi->enable = 0;
    }
    os_printf("%s:%d\tmsi:%X\n", __FUNCTION__, __LINE__, msi);
    return msi;
}

extern uint32_t takephoto_arg[4];
void            photo_thread_init(uint16_t w, uint16_t h, uint8_t only_thumb, struct msi *msi, uint16_t stype)
{
    takephoto_arg[0] = (w & 0xffff) | ((h << 16) & 0xffff0000);
    takephoto_arg[1] = only_thumb;
    takephoto_arg[2] = (uint32) msi;
    takephoto_arg[3] = (uint32) stype;
    os_task_create("photo_run", take_photo_with_big_size, (void *) &takephoto_arg, OS_TASK_PRIORITY_BELOW_NORMAL, 0, NULL, 2048);
}

struct msi *photo_take_workrun()
{
    struct msi *msi;
    msi = thumb_msi("scale3_thumb");
    msi_add_output(NULL, S_PREVIEW_SCALE3, "scale3_thumb");
    register_gen420_queue(3, THUMB_W, THUMB_H, thumb_gen420_kick, NULL, (uint32) NULL);
    return msi;
}
