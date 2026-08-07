#include "basic_include.h"

#include "lib/multimedia/msi.h"
#include "stream_define.h"
#include "app_lcd/app_lcd.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "app/video_app/file_thumb.h"
#include "video_demo.h"
#include "stream_define.h"
#include "osal_file.h"
struct msi *jpg_concat_msi_init_start(uint32_t jpgid, uint16_t w, uint16_t h, uint16_t *filter_type, uint8_t src_from, uint8_t run);
struct msi *h264_msi_init_with_mode(uint32_t drv1_from, uint16_t drv1_w, uint16_t drv1_h, uint16_t drv2_from, uint16_t drv2_w, uint16_t drv2_h);
struct msi *route_msi(const char *name);
struct msi *video_thumb_msi_init(const char *msi_name, uint16_t filter);
void takephoto_with_thumb_init(const char *thumb_msi_name);
void takephoto_with_thumb_over_dpi_init(const char *thumb_msi_name);
/*****************************************************************************************
 * demo都是简单的接收流程,如果有过滤type等,需要将接收msi实现更多东西才可以
 *****************************************************************************************/
// 从其他文件调用解析mjpeg的函数
extern void ex_parse_jpg(uint8_t *jpg_buf, uint32_t maxsize, uint32_t *w, uint32_t *h);

/**********************************************************
 * 这个demo是使用JPG0从VPP_DATA0编码
 *
 * msi数据流程:
 * "H_JPG_XXX"->RS_JPG_CONCAT->recv_jpg_demo
 *
 * 说明:
 * 如果没有修改,只需要关注recv_jpg_demo,
 * "H_JPG_XXX"->RS_JPG_CONCAT,已经默认绑定,"H_JPG_XXX"是动态
 * 名称,RS_JPG_CONCAT是固定输出jpg图片的msi名称
 * msi_add_output(NULL,RS_JPG_CONCAT,"recv_jpg_demo"),这样
 * 就可以绑定
 *
 **********************************************************/
static void jpg0_encode_demo_from_vpp_data0(void *d)
{
    // 请配置与数据源一致的分辨率
    // 这里采用VPP_DATA0,所以与摄像头采进来的数据w和h一致
    struct msi *jpg0_msi = jpg_concat_msi_init_start(JPGID0, 1280, 720, NULL, VPP_DATA0, 1);
    os_printf(KERN_INFO "jpg0_msi:%X\n", jpg0_msi);
    if (jpg0_msi)
    {
        msi_add_output(jpg0_msi, NULL, "recv_jpg_demo");
    }

    // 创建接收的接收的msi模块
    uint8_t     is_new = 0;
    uint32_t    w, h;
    struct msi *recv_jpg_msi = msi_new("recv_jpg_demo", 8, &is_new);
    os_printf(KERN_INFO "recv_jpg_msi:%X\n", recv_jpg_msi);
    // 需要使能,不使能不会接收
    recv_jpg_msi->enable      = 1;
    struct framebuff *fb      = NULL;
    uint8_t          *jpg_buf = NULL;
    while (recv_jpg_msi)
    {
        fb = msi_get_fb(recv_jpg_msi, 0);
        if (fb)
        {
            jpg_buf = (uint8_t *) fb->data;
            // 解析图片的size,看log知道图片生成是否正常
            ex_parse_jpg(jpg_buf, fb->len, &w, &h);
            // os_printf(KERN_INFO"fb:%X\tdata:%X\tlen:%d\n",fb,fb->data,fb->len);
            // os_printf(KERN_INFO"fb mtype:%X\tstype:%d\n",fb->mtype,fb->stype);
            // 打印前2byte,FF D8证明接收到的是正常图片
            os_printf(KERN_INFO "jpg_buf[0]:%02X\tjpg_buf[1]:%02X\n", jpg_buf[0], jpg_buf[1]);
            // os_printf(KERN_INFO"jpg w:%d\th:%d\n",w,h);
            msi_delete_fb(NULL, fb);
            fb = NULL;
        }
        os_sleep_ms(1);
    }
}

/**********************************************************
 * 这个demo是使用JPG0从VPP_DATA1编码
 *
 * msi数据流程:
 * "H_JPG_XXX"->RS_JPG_CONCAT->recv_jpg_demo
 *
 * 说明:
 * 如果没有修改,只需要关注recv_jpg_demo,
 * "H_JPG_XXX"->RS_JPG_CONCAT,已经默认绑定,"H_JPG_XXX"是动态
 * 名称,RS_JPG_CONCAT是固定输出jpg图片的msi名称
 * msi_add_output(NULL,RS_JPG_CONCAT,"recv_jpg_demo"),这样
 * 就可以绑定
 *
 **********************************************************/
static void jpg0_encode_demo_from_vpp_data1(void *d)
{
    // 请配置与数据源一致的分辨率
    // 这里采用VPP_DATA1,所以与vpp_data1配置的参数相关联
    struct msi *jpg0_msi = jpg_concat_msi_init_start(JPGID1, 640, 360, NULL, VPP_DATA1,1);
    os_printf(KERN_INFO "jpg0_msi:%X\n", jpg0_msi);
    if (jpg0_msi)
    {
        msi_add_output(jpg0_msi, NULL, "recv_jpg_demo"); // 与这个等效msi_add_output(NULL,RS_JPG_CONCAT,"recv_jpg_demo");
    }

    // 创建接收的接收的msi模块
    uint8_t     is_new = 0;
    uint32_t    w, h;
    struct msi *recv_jpg_msi = msi_new("recv_jpg_demo", 8, &is_new);
    os_printf(KERN_INFO "recv_jpg_msi:%X\n", recv_jpg_msi);
    // 需要使能,不使能不会接收
    recv_jpg_msi->enable      = 1;
    struct framebuff *fb      = NULL;
    uint8_t          *jpg_buf = NULL;
    while (recv_jpg_msi)
    {
        fb = msi_get_fb(recv_jpg_msi, 0);
        if (fb)
        {
            jpg_buf = (uint8_t *) fb->data;
            // 解析图片的size,看log知道图片生成是否正常
            ex_parse_jpg(jpg_buf, fb->len, &w, &h);
            os_printf(KERN_INFO "fb:%X\tdata:%X\tlen:%d\n", fb, fb->data, fb->len);
            os_printf(KERN_INFO "fb mtype:%X\tstype:%d\n", fb->mtype, fb->stype);
            // 打印前2byte,FF D8证明接收到的是正常图片
            os_printf(KERN_INFO "jpg_buf[0]:%02X\tjpg_buf[1]:%02X\n", jpg_buf[0], jpg_buf[1]);
            os_printf(KERN_INFO "jpg w:%d\th:%d\n", w, h);
            msi_delete_fb(NULL, fb);
            fb = NULL;
        }
        os_sleep_ms(1);
    }
}

/**********************************************************
 * 这个demo是简单h264接收,这里从VPP_DATA0去编码
 * h264->count这个主要是判断是否有丢帧,范围是2-255,如果丢帧
 * 应用应该是要去等到下一个I帧发送,否则可能花屏
 *
 * msi的数据流流程:
 * S_H264->recv_h264_demo
 **********************************************************/
static void h264_encode_demo_from_vpp_data0(void *d)
{
    // 请配置与数据源一致的分辨率
    // 这里采用VPP_DATA1,所以与vpp_data1配置的参数相关联
    struct msi *h264_msi = h264_msi_init_with_mode(VPP_DATA0, 1280, 720, ~0, 0, 0);
    os_printf(KERN_INFO "h264_msi:%X\n", h264_msi);
    if (h264_msi)
    {
        msi_add_output(h264_msi, NULL, "recv_h264_demo");
    }

    // 创建接收的接收的msi模块
    uint8_t     is_new = 0;
    //uint32_t    w, h;
    struct msi *recv_h264_msi = msi_new("recv_h264_demo", 8, &is_new);
    os_printf(KERN_INFO "recv_h264_msi:%X\n", recv_h264_msi);
    // 需要使能,不使能不会接收
    recv_h264_msi->enable = 1;
    struct framebuff *fb  = NULL;
    while (recv_h264_msi)
    {
        fb = msi_get_fb(recv_h264_msi, 0);
        if (fb)
        {
            struct fb_h264_s *h264 = (struct fb_h264_s *) fb->priv;
            if (h264->type == 1)
            {
                os_printf(KERN_INFO "############h264 I frame\tsize:%d\tcount:%d\r\n", fb->len, h264->count);
            }
            else if (h264->type == 2)
            {
                os_printf(KERN_INFO "h264 P frame\tsize:%d\tcount:%d\r\n", fb->len, h264->count);
            }
            else
            {
                os_printf(KERN_INFO "h264 err frame\r\n");
            }
            msi_delete_fb(NULL, fb);
            fb = NULL;
        }
        os_sleep_ms(1);
    }
}

/**********************************************************
 * 这个demo是简单h264接收,这里从VPP_DATA1去编码,注意:
 * 从VPP_DATA1去编码,VPP_DATA1不能保存到psram
 * h264->count这个主要是判断是否有丢帧,范围是2-255,如果丢帧
 * 应用应该是要去等到下一个I帧发送,否则可能花屏
 *
 * msi的数据流流程:
 * S_H264->recv_h264_demo
 **********************************************************/
static void h264_encode_demo_from_vpp_data1(void *d)
{
    // 请配置与数据源一致的分辨率
    // 这里采用VPP_DATA1,所以与vpp_data1配置的参数相关联
    struct msi *h264_msi = h264_msi_init_with_mode(VPP_DATA1, 640, 360, ~0, 0, 0);
    os_printf(KERN_INFO "h264_msi:%X\n", h264_msi);
    if (h264_msi)
    {
        msi_add_output(h264_msi, NULL, "recv_h264_demo");
    }

    // 创建接收的接收的msi模块
    uint8_t     is_new = 0;
    //uint32_t    w, h;
    struct msi *recv_h264_msi = msi_new("recv_h264_demo", 8, &is_new);
    os_printf(KERN_INFO "recv_h264_msi:%X\n", recv_h264_msi);
    // 需要使能,不使能不会接收
    recv_h264_msi->enable = 1;
    struct framebuff *fb  = NULL;
    while (recv_h264_msi)
    {
        fb = msi_get_fb(recv_h264_msi, 0);
        if (fb)
        {
            struct fb_h264_s *h264 = (struct fb_h264_s *) fb->priv;
            if (h264->type == 1)
            {
                os_printf(KERN_INFO "############h264 I frame\tsize:%d\tcount:%d\r\n", fb->len, h264->count);
            }
            else if (h264->type == 2)
            {
                os_printf(KERN_INFO "h264 P frame\tsize:%d\tcount:%d\r\n", fb->len, h264->count);
            }
            else
            {
                os_printf(KERN_INFO "h264 err frame\r\n");
            }
            msi_delete_fb(NULL, fb);
            fb = NULL;
        }
        os_sleep_ms(1);
    }
}

/**********************************************************
 * 这个demo是简单h264接收,这里从VPP_DATA1去编码,然后写卡10s
 * 注意:
 * 从VPP_DATA1去编码,VPP_DATA1不能保存到psram
 * h264->count这个主要是判断是否有丢帧,范围是2-255,如果丢帧
 * 应用应该是要去等到下一个I帧发送,否则可能花屏
 *
 * msi的数据流流程:
 * S_H264->recv_h264_demo
 **********************************************************/
static void h264_encode_demo_from_vpp_data1_save_to_sd(void *d)
{
    // 请配置与数据源一致的分辨率
    // 这里采用VPP_DATA1,所以与vpp_data1配置的参数相关联
    struct msi *h264_msi = h264_msi_init_with_mode(VPP_DATA1, 640, 360, ~0, 0, 0);
    os_printf(KERN_INFO "h264_msi:%X\n", h264_msi);
    if (h264_msi)
    {
        msi_add_output(h264_msi, NULL, "recv_h264_demo");
    }

    // 创建接收的接收的msi模块
    uint8_t     is_new = 0;
    //uint32_t    w, h;
    struct msi *recv_h264_msi = msi_new("recv_h264_demo", 8, &is_new);
    os_printf(KERN_INFO "recv_h264_msi:%X\n", recv_h264_msi);
    // 需要使能,不使能不会接收
    recv_h264_msi->enable               = 1;
    struct framebuff *fb                = NULL;
    uint32_t          record_start_time = os_jiffies();
    void             *fp                = osal_fopen("1.264", "wb");
    while (recv_h264_msi)
    {
        fb = msi_get_fb(recv_h264_msi, 0);
        if (fb)
        {
            struct fb_h264_s *h264 = (struct fb_h264_s *) fb->priv;
            if (h264->type == 1)
            {
                os_printf(KERN_INFO "h264 I frame\tsize:%d\tcount:%d\r\n", fb->len, h264->count);
            }
            else if (h264->type == 2)
            {
                os_printf(KERN_INFO "h264 P frame\tsize:%d\tcount:%d\r\n", fb->len, h264->count);
            }
            else
            {
                os_printf(KERN_INFO "h264 err frame h264->type:%d\r\n", h264->type);
            }

            if (fp)
            {
                osal_fwrite((void*)fb->data, 1, fb->len, fp);
                if (os_jiffies() - record_start_time > 10000)
                {
                    osal_fclose((F_FILE*)fp);
                    fp = NULL;
                    os_printf(KERN_INFO "H264 save finish\r\n");
                }
            }
            msi_delete_fb(NULL, fb);
            fb = NULL;
        }
        os_sleep_ms(1);
    }
}

static void jpg_demo_from_USB(void *d)
{
    // 请配置与数据源一致的分辨率
    // 这里采用VPP_DATA0,所以与摄像头采进来的数据w和h一致
    struct msi *usb_jpg_route = route_msi(ROUTE_USB);
    os_printf(KERN_INFO "usb_jpg_route:%X\n", usb_jpg_route);
    if (usb_jpg_route)
    {
        msi_add_output(usb_jpg_route, NULL, "recv_jpg_demo");
    }

    // 创建接收的接收的msi模块
    uint8_t     is_new = 0;
    uint32_t    w, h;
    struct msi *recv_jpg_msi = msi_new("recv_jpg_demo", 8, &is_new);
    os_printf(KERN_INFO "recv_jpg_msi:%X\n", recv_jpg_msi);
    // 需要使能,不使能不会接收
    recv_jpg_msi->enable      = 1;
    struct framebuff *fb      = NULL;
    uint8_t          *jpg_buf = NULL;
    while (recv_jpg_msi)
    {
        fb = msi_get_fb(recv_jpg_msi, 0);
        if (fb)
        {
            jpg_buf = (uint8_t *) fb->data;
            // 解析图片的size,看log知道图片生成是否正常
            ex_parse_jpg(jpg_buf, fb->len, &w, &h);
            // os_printf(KERN_INFO"fb:%X\tdata:%X\tlen:%d\n",fb,fb->data,fb->len);
            // os_printf(KERN_INFO"fb mtype:%X\tstype:%d\n",fb->mtype,fb->stype);
            // 打印前2byte,FF D8证明接收到的是正常图片
            os_printf(KERN_INFO "jpg_buf[0]:%02X\tjpg_buf[1]:%02X\n", jpg_buf[0], jpg_buf[1]);
            // os_printf(KERN_INFO"jpg w:%d\th:%d\n",w,h);
            msi_delete_fb(NULL, fb);
            fb = NULL;
        }
        os_sleep_ms(1);
    }
}

/*******************************************************************************************************************************************
 * 缩略图demo,这里是从USB获取一张图(所以这个demo运行前提是usb能正常获取到uvc的图片),然后到解码后生成缩略图并且按照sdk特定方式保存缩略图
 *
 *  ROUTE_USB->video_thumb_name(原图)->R_THUMB
 **************************************************************************************************************************************/
static void usb_thumb_demo(void *d)
{
    extern void video_thumb_init(const char *thumb_msi_name);
    char        filename[64];
    uint32_t    start_time = os_jiffies();
    // 拍照以及缩略图的一些msi初始化
    video_thumb_init(R_THUMB);

    // 从usb获取图片数据
    struct msi *usb_jpg_route = route_msi(ROUTE_USB);

    char       *video_thumb_name = "video_thumb_demo";
    struct msi *video_thumb_msi  = video_thumb_msi_init(video_thumb_name, 0);
    if (video_thumb_msi)
    {
        // 给到解码然后生成缩略图
        msi_add_output(video_thumb_msi, NULL, R_THUMB);
        msi_add_output(usb_jpg_route, NULL, video_thumb_name);
    }

    while (video_thumb_msi)
    {
        if (os_jiffies() - start_time > 1000)
        {
            start_time = os_jiffies();
            // 定时启动缩略图生成
            os_sprintf(filename, "%08d.jpg", (uint32_t) os_jiffies() % 99999999);
            os_printf(KERN_INFO "filename:%s\n", filename);
            msi_do_cmd(video_thumb_msi, MSI_CMD_JPG_THUMB, MSI_JPG_THUMB_TAKEPHOTO, (uint32_t)filename);
        }
        os_sleep_ms(1);
    }
}

/*******************************************************************************************************************************************
 * 缩略图demo,这里是从USB获取一张图(所以这个demo运行前提是usb能正常获取到uvc的图片),然后到解码后生成缩略图并且按照sdk特定方式保存缩略图
 *
 *  AUTO_JPG(实际是mjpeg图片)->video_thumb_name(原图)->R_THUMB
 **************************************************************************************************************************************/
static void VPP_thumb_demo(void *d)
{
    extern void video_thumb_init(const char *thumb_msi_name);
    char        filename[64];
    uint32_t    start_time = os_jiffies();
    // 拍照以及缩略图的一些msi初始化
    video_thumb_init(R_THUMB);

    // 从usb获取图片数据
    char       *video_thumb_name = "video_thumb_demo";
    struct msi *video_thumb_msi  = video_thumb_msi_init(video_thumb_name, 0);
    if (video_thumb_msi)
    {
        // 给到解码然后生成缩略图
        msi_add_output(video_thumb_msi, NULL, R_THUMB);

        
        msi_add_output(NULL, AUTO_JPG, R_JPG_THUMB);
    }

    while (video_thumb_msi)
    {
        if (os_jiffies() - start_time > 1000)
        {
            start_time = os_jiffies();
            // 定时启动缩略图生成
            os_sprintf(filename, "%08d.jpg", (uint32_t) os_jiffies() % 99999999);
            os_printf(KERN_INFO "filename:%s\n", filename);
            msi_do_cmd(video_thumb_msi, MSI_CMD_JPG_THUMB, MSI_JPG_THUMB_TAKEPHOTO, (uint32_t)filename);
        }
        os_sleep_ms(1);
    }
}

static void takephoto_demo(void *d)
{
#define MAX_DPI_NUM 5
    // 拍照的数量
    uint8_t takephoto_num = 1;
    uint8_t photo_value   = 0;
    // 大分辨率和普通分辨率拍照的初始化
    // 正常拍照和缩略图
    takephoto_with_thumb_init(R_THUMB);
    // 大分辨率拍照和缩略图
    takephoto_with_thumb_over_dpi_init(SR_OVER_DPI_THUMB_JPG);
    static const int dpi[][2] = {
            {1280, 720},  // 720P
            {1920, 1080}, // 8M
            {2560, 1440}, // 8M
            {3840, 2160}, // 8M
            {7680, 4320}, // 12M
    };

    while (1)
    {
        // 普通模式
        if (photo_value % MAX_DPI_NUM == 0)
        {
            struct msi *jpg_thumb_msi = msi_find(R_JPG_THUMB,1); // 需要预先创建,否则不会真正拍照
            if (jpg_thumb_msi)
            {
                msi_do_cmd(jpg_thumb_msi, MSI_CMD_JPG_THUMB, MSI_JPG_THUMB_TAKEPHOTO, takephoto_num);
                msi_put(jpg_thumb_msi);
            }
        }
        else
        {
            struct msi *over_dpi_recode_msi = msi_find(R_SCALE1_JPG_RECODE, 0);
            uint32_t    dpi_w_h             = dpi[photo_value % MAX_DPI_NUM][0] << 16 | dpi[photo_value % MAX_DPI_NUM][1];
            if (over_dpi_recode_msi)
            {
                msi_do_cmd(over_dpi_recode_msi, MSI_CMD_SCALE1, MSI_SCALE1_RESET_DPI, dpi_w_h);
                msi_put(over_dpi_recode_msi);
            }

            struct msi *over_dpi_msi = msi_find(S_SCALE3_OVER_DPI, 0);
            if (over_dpi_msi)
            {
                msi_do_cmd(over_dpi_msi, MSI_CMD_TAKEPHOTO_SCALE3, MSI_TAKEPHOTO_SCALE3_KICK, takephoto_num);
                msi_put(over_dpi_msi);
            }
        }
        photo_value++;
        //  间隔1s后切换拍照分辨率
        os_sleep_ms(1000);
    }
}
typedef void (*demo_func)(void *d);
static const demo_func demo[DEMO_MAX] = {
        jpg0_encode_demo_from_vpp_data0,
        jpg0_encode_demo_from_vpp_data1,
        h264_encode_demo_from_vpp_data0,
        h264_encode_demo_from_vpp_data1,
        h264_encode_demo_from_vpp_data1_save_to_sd,
        jpg_demo_from_USB,
        usb_thumb_demo,
        VPP_thumb_demo,
        takephoto_demo,
};

void video_demo_thread(enum DEMO_ENUM which)
{
    if (which >= DEMO_MAX)
    {
        return;
    }
    os_task_create("video_demo", demo[which], NULL, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024); // 320x240
}