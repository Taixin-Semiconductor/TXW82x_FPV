#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "stream_define.h"
#include "gen420_hardware_msi.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "video_msi.h"
/*******************************************
 * usb图片增加水印重新编码
 *******************************************/
 
extern struct msi *route_msi(const char *name);
extern struct msi *jpg_decode_msg_msi(const char *name, uint16_t out_w, uint16_t out_h, uint16_t step_w, uint16_t step_h, uint32_t filter);
extern struct msi *jpg_decode_msi(const char *name);
extern struct msi *yuv_time_watermark(const char *name, uint8_t filter);

static uint8_t recode_filter(void *f, uint8_t recv_type)
{
    struct framebuff *fb  = (struct framebuff *) f;
    uint8_t           res = 1;

    if ((fb->mtype == F_JPG) && fb->stype == recv_type)
    {
        res = 0;
    }

    return res;
}

void usb_to_recode_init()
{

    uint32_t    magic;
    // ROUTE_USB是usb图片获取来源
    struct msi *route_m = route_msi(ROUTE_USB);
    if (route_m)
    {
        msi_add_output(route_m, NULL, SR_USB_RECODE_DEOCDE);
    }

    // 接收USB的图片数据,然后配置需要解码的参数,最后给到硬件去解码
    struct msi *decode_msg_msi = jpg_decode_msg_msi(SR_USB_RECODE_DEOCDE, 1280, 720, 1280, 720, FSTYPE_USB_CAM0);
    if (decode_msg_msi)
    {
        // 这里配置特定的类型,就是重新编码后会设置类型,这样应用层是可以进行识别是否为重新编码的,这个可以自定义,但不要和其他
        // 已经运行的有冲突,不然可能其他msi接收到重编码的数据
        msi_do_cmd(decode_msg_msi, MSI_CMD_DECODE_JPEG_MSG, MSI_JPEG_DECODE_FORCE_TYPE, FSTYPE_JPG_GEN420_REJPG);
        msi_add_output(decode_msg_msi, NULL, S_JPG_DECODE);
    }

    // 把解码模块打开,接收来自SR_USB_RECODE_DEOCDE的数据,解码后,给到水印的msi去添加水印
    struct msi *decode_msi = jpg_decode_msi(S_JPG_DECODE);
    if (decode_msi)
    {
        msi_add_output(decode_msi, NULL, SR_YUV_WATERMARK);
    }

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

    // 水印添加后,将yuv数据给到R_GEN420_JPG_RECODE重新编码,并且仅仅进行过滤类型
    struct msi *watermark = yuv_time_watermark(SR_YUV_WATERMARK, FSTYPE_JPG_GEN420_REJPG);
    if (watermark)
    {
        msi_add_output(watermark, NULL, R_GEN420_JPG_RECODE);
    }

    // 过滤类型,由于从usb过来,由于decode_msg_msi设置了类型,所以这里需要修改
    static const uint16_t watermark_filter[] = {FSTYPE_JPG_GEN420_REJPG, FSTYPE_NONE};
    struct msi           *gen420_jpg_msi = gen420_jpg_msi_init(R_GEN420_JPG_RECODE, JPGID0, FSTYPE_JPG_GEN420_REJPG, JPG_LOCK_GEN420_RECODE, GEN420_QUEUE_JPEG_RECODE, (uint16_t*)watermark_filter, recode_filter);
    if (gen420_jpg_msi)
    {
        // 如果需要默认添加到某个msi,在这里添加,也可以后续在其他地方通过msi_add_output(NULL, R_GEN420_JPG_RECODE, "XXX");的方式添加
        msi_do_cmd(gen420_jpg_msi, MSI_CMD_JPG_RECODE, MSI_JPG_RECODE_MAGIC, magic);
    }
}