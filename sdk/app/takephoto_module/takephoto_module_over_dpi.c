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
struct msi *scale3_msi_pic_thumb(const char *name, uint32_t normal_magic, uint32_t thumb_magic);
extern struct msi *jpg_decode_msi(const char *name);
extern struct msi *jpg_decode_msg_msi(const char *name, uint16_t out_w, uint16_t out_h, uint16_t step_w, uint16_t step_h, uint32_t filter);
extern struct msi *jpg_thumb_msi_init(const char *msi_name, uint16_t filter, uint8_t thumb_stype);
void               scale_from_soft_to_jpg(struct scale_device *scale_dev, uint32 yuvbuf_addr, uint32 s_w, uint32 s_h, uint32 d_w, uint32 d_h);

// 过滤类型,因为缩略图的stype一定大于FSYPTE_INVALID
static uint8_t filter(void *f, uint8_t recv_type)
{
    struct framebuff *fb  = (struct framebuff *) f;
    uint8_t           res = 1;

    if ((fb->mtype == F_JPG || fb->mtype == F_JPG_NODE) && fb->stype == recv_type)
    {
        res = 0;
    }

    return res;
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

// 拍照大分辨率的的缩略图初始化
struct msi *gen_over_dpi_thumb_init(const char *thumb_msi_name, uint32_t *magic)
{
    uint32_t    thumb_magic = 0;
    struct msi *gen420_jpg_msi;
    gen420_jpg_msi = gen420_jpg_msi_init(R_GEN420_THUMB_JPG_OVER_DPI, JPGID0, FSTYPE_OVER_DPI_THUMB_JPG, JPG_LOCK_GEN420_THUBM_ENCODE_OVER_DPI, GEN420_QUEUE_THUMB_JPEG_OVER_DPI, NULL, filter);
    do
    {
        thumb_magic ^= os_jiffies();
        thumb_magic ^= (uint32_t) gen420_jpg_msi;
    } while (!thumb_magic);

    struct msi *thumb_msi = thumb_over_dpi_msi_init(thumb_msi_name, 0, FSTYPE_OVER_DPI_THUMB_JPG, thumb_magic);

    if (gen420_jpg_msi && thumb_msi)
    {
        msi_do_cmd(gen420_jpg_msi, MSI_CMD_JPG_RECODE, MSI_JPG_RECODE_MAGIC, thumb_magic);
        // gen420编码完成后,需要将mjpg给到thumb_msi去保存(通过类型判断)
        msi_add_output(gen420_jpg_msi, NULL, thumb_msi->name);

        // thumb_over_dpi将yuv给到gen420去编码(通过类型判断)
        msi_add_output(thumb_msi, NULL, gen420_jpg_msi->name);

        // 将缩略图给到写卡线程
        msi_add_output(thumb_msi, NULL, R_FILE_MSI);
    }

    if (magic)
    {
        *magic = thumb_magic;
    }
    return thumb_msi;
}

// 拍照大分辨率的图片初始化
struct msi *gen_over_dpi_init(const char *thumb_msi_name, uint32_t *magic)
{
    // normal_magic与thumb_magic正常不能一样,这里没有专门去判断
    uint32_t    normal_magic   = 0;
    struct msi *scale1_jpg_msi = scale1_jpg_msi_init(R_SCALE1_JPG_RECODE, JPGID1, FSTYPE_OVER_DPI_JPG,JPG_LOCK_SCALE1_ENCODE, 0, filter);
    do
    {
        normal_magic ^= os_jiffies();
        normal_magic ^= (uint32_t) scale1_jpg_msi;
    } while (!normal_magic);

    struct msi *normal_jpg_msi = thumb_over_dpi_msi_init(SR_OVER_DPI_JPG, 1, FSTYPE_OVER_DPI_JPG, normal_magic);

    if (scale1_jpg_msi && normal_jpg_msi)
    {
        msi_do_cmd(scale1_jpg_msi, MSI_CMD_JPG_RECODE, MSI_JPG_RECODE_MAGIC, normal_magic);
        // gen420编码完成后,需要将mjpg给到thumb_msi去保存(通过类型判断)
        msi_add_output(scale1_jpg_msi, NULL, normal_jpg_msi->name);

        // thumb_over_dpi将yuv给到gen420去编码(通过类型判断)
        msi_add_output(normal_jpg_msi, NULL, scale1_jpg_msi->name);

        // 将缩略图给到写卡线程
        msi_add_output(normal_jpg_msi, NULL, R_FILE_MSI);
    }

    if (magic)
    {
        *magic = normal_magic;
    }
    return normal_jpg_msi;
}

void gen_thumb_normal_yuv(struct msi *normal_jpg_msi, struct msi *thumb_msi, uint32_t normal_magic, uint32_t thumb_magic)
{
    struct msi *scale3_msi = scale3_msi_pic_thumb(S_SCALE3_OVER_DPI, normal_magic, thumb_magic);
    if (scale3_msi)
    {
        if (normal_jpg_msi)
        {
            msi_add_output(scale3_msi, NULL, normal_jpg_msi->name);
        }

        if (thumb_msi)
        {
            msi_add_output(scale3_msi, NULL, thumb_msi->name);
        }
    }
}

// 只能调用一次,因为内部很多msi的名称都是固定的,如果需要复用,需要将内部实现调整
void takephoto_with_thumb_over_dpi_init(const char *thumb_msi_name)
{
    uint32_t    thumb_magic = 0, normal_magic = 0;
    struct msi *thumb_msi      = NULL;
    struct msi *normal_jpg_msi = NULL;

    // 缩略图
    thumb_msi      = gen_over_dpi_thumb_init(thumb_msi_name, &thumb_magic);
    // 大分辨照片
    normal_jpg_msi = gen_over_dpi_init(thumb_msi_name, &normal_magic);

    // 生成对应照片与缩略图的yuv
    gen_thumb_normal_yuv(normal_jpg_msi, thumb_msi, normal_magic, thumb_magic);
}
