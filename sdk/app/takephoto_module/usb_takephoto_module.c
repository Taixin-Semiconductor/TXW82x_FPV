#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "stream_define.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "jpg_concat_msi.h"
#include "gen420_hardware_msi.h"
#include "video_msi.h"

extern struct msi *jpg_decode_msi(const char *name);
extern struct msi *jpg_decode_msg_msi(const char *name, uint16_t out_w, uint16_t out_h, uint16_t step_w, uint16_t step_h, uint32_t filter);
extern struct msi *jpg_thumb_msi_init(const char *msi_name, uint16_t filter, uint8_t thumb_stype);
extern struct msi *usb_jpg_thumb_msi_init(const char *msi_name, uint8_t filter, uint8_t thumb_stype);
extern struct msi *usb_thumb_over_dpi_msi_init(const char *msi_name, uint8_t stype, uint32_t magic);

// 过滤类型,因为缩略图的stype一定大于FSYPTE_INVALID
static uint8_t filter(void *f, uint8_t recv_type)
{
    struct framebuff *fb = (struct framebuff *) f;
    uint8_t res = 1;

    if (fb->mtype == F_JPG && fb->stype == recv_type)
    {
        res = 0;
    }

    return res;
}

void usb_takephoto_init(const char *source_msi_name, const char *photo_msi_name, const char *thumb_msi_name)
{
    msi_add_output(NULL, source_msi_name, NULL, photo_msi_name);

    struct msi *usb_jpg_thumb_msi = usb_jpg_thumb_msi_init(photo_msi_name, FRAMEBUFF_SOURCE_USB, FSTYPE_NORMAL_THUMB_JPG);
    if(thumb_msi_name)
    {
        // 给到缩略图模块去解码后编码小缩略图
        msi_add_output(usb_jpg_thumb_msi, NULL, NULL, thumb_msi_name);
    }
    if (usb_jpg_thumb_msi)
    {
        // 文件保存的msi
        msi_add_output(usb_jpg_thumb_msi, NULL, NULL, R_FILE_MSI);
    }
}

/****************************************************************************************************************************
 * 缩略图生成绑定
 * thumb_msi_name->R_THUMB_DECODE_MSG_USB->S_JPG_DECODE->R_GEN420_THUMB_JPG_USB(接收yuv数据,通过gen420去编码,生成图片)
 *    ^                                                         |
 *    |                                                         |(将jpg图片传递会给thumb_msi_name去保存)
 *    |                                                         V
 *     ----------------------------------------------------------
 *
 * R_GEN420_THUMB_JPG:既做接收也做发送,先接收yuv,kick gen420去编码,然后接收生成的jpg的图片,将jpg图片转发给thumb_msi_name去保存
 ***************************************************************************************************************************/

static void usb_takephoto_thumb_init(const char *thumb_msi_name)
{
    uint32_t    magic;
    // 启动解码模块,名称已经固定,参数已经无效
    struct msi *decode_msi = jpg_decode_msi(S_JPG_DECODE);
    if (decode_msi)
    {
        // 给到gen420去重新生成缩略图jpg
        msi_add_output(decode_msi, NULL, NULL, R_GEN420_THUMB_JPG_USB);
    }

    // 生成缩略图的size
    struct msi *decode_msg_msi = jpg_decode_msg_msi(R_THUMB_DECODE_MSG_USB, 320, 180, 320, 180, 0);
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
        msi_add_output(decode_msg_msi, NULL, NULL, decode_msi->name);
    }

    struct msi *thumb_msi = usb_thumb_over_dpi_msi_init(thumb_msi_name, FSTYPE_NORMAL_THUMB_JPG_USB, magic);
    if (thumb_msi)
    {
        // 处理原图,然后给到解码后生成缩略图
        msi_add_output(thumb_msi, NULL, NULL, R_THUMB_DECODE_MSG_USB);

        // 同时会将有一个需要保存的文件
        msi_add_output(thumb_msi, NULL, NULL, R_FILE_MSI);
    }

    // 启动一个专门用yuv->gen420->mjpg的模块,这个模块会接收mjpg图片,并且通过filter函数决定是否转发
    struct msi *gen420_jpg_msi = gen420_jpg_msi_init(R_GEN420_THUMB_JPG_USB, JPGID0, FSTYPE_NORMAL_THUMB_JPG_USB, JPG_LOCK_GEN420_THUBM_ENCODE_USB, GEN420_QUEUE_THUMB_JPEG_USB, NULL, filter);
    if (gen420_jpg_msi && thumb_msi)
    {
        // 设置magic
        msi_do_cmd(gen420_jpg_msi, MSI_CMD_JPG_RECODE, MSI_JPG_RECODE_MAGIC, magic);
        msi_add_output(gen420_jpg_msi, NULL, NULL ,thumb_msi->name);
    }
}

void usb_takephoto_with_thumb_init(const char *source_msi_name, const char *photo_msi_name, const char *thumb_msi_name)
{
    usb_takephoto_init(source_msi_name, photo_msi_name, thumb_msi_name);
    usb_takephoto_thumb_init(thumb_msi_name);
}
