#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "osal/string.h"
#include "stream_define.h"
#include "lib/fs/fatfs/osal_file.h"
#include "user_work/user_work.h"
#include "file_thumb.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

/*******************************************************************
 * 这个模块主要是 USB 拍照用，同时会尝试生成缩略图(转发到下一个msi去生成)
 *******************************************************************/

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

#define USB_JPG_THUMB_RECV_MAX (8)
struct usb_jpg_thumb_s
{
    struct os_work work;
    struct msi    *msi;
    struct msi    *gen420_msi;
    char           filepath[64];
    char          *dirpath;
    uint8_t        filter;
    uint8_t        thumb_stype;
    uint8_t        takephoto_photo;
};

uint8_t takephoto_name(const char *img_dir, char *file_path, int filepath_size);
// 正常应该是这里去切换成拍照模式
static int32 usb_jpg_thumb_work(struct os_work *work)
{
    struct usb_jpg_thumb_s *usb_jpg_thumb = (struct usb_jpg_thumb_s *) work;
    struct framebuff   *fb;
    struct framebuff   *c_fb;

// 执行完一次后,继续去获取图片,如果没有就等待下次信号量唤醒
jpg_thumb_work_again:
    // 尝试获取一下原图
    fb = msi_get_fb(usb_jpg_thumb->msi, 0);
    if (fb)
    {
        // 判断一下是否为原图
        if (usb_jpg_thumb->takephoto_photo)
        {
            usb_jpg_thumb->takephoto_photo--;
            if (takephoto_name((const char *)usb_jpg_thumb->dirpath, usb_jpg_thumb->filepath, sizeof(usb_jpg_thumb->filepath)))
            {
                _os_printf("%s %d\tget file path err\r\n", __FUNCTION__, __LINE__);
                msi_delete_fb(NULL, fb);
                fb = NULL;
                return 1;
            }
            // 设置缩略图的stype
            c_fb       = fb_clone(fb, F_THUMB_JPG << 8 | usb_jpg_thumb->thumb_stype, usb_jpg_thumb->msi, NULL);
            // 设置来源是正常缩略图
            c_fb->priv = (void *) usb_jpg_thumb->filepath; // 记录路径
            // 转发,生成缩略图
            msi_output_fb(usb_jpg_thumb->msi, c_fb, 0);
            // 转发到给到文件去保存(独立线程)
            struct framebuff *send_fb = fb_clone(fb, F_FILE_T << 8 | fb->stype, usb_jpg_thumb->msi, NULL);
            ASSERT(send_fb);
            struct file_msg_s *file_msg = (struct file_msg_s *) STREAM_LIBC_MALLOC(sizeof(struct file_msg_s));
            ASSERT(file_msg);
            os_memcpy(file_msg->path, usb_jpg_thumb->filepath, strlen(usb_jpg_thumb->filepath) + 1);
            file_msg->ishid = 0;
            send_fb->priv   = (void *) file_msg;
            msi_output_fb(usb_jpg_thumb->msi, send_fb, 0);
        }

        if (!usb_jpg_thumb->takephoto_photo)
        {
            usb_jpg_thumb->msi->enable = 0;
        }
        msi_delete_fb(NULL, fb);
        fb = NULL;
        goto jpg_thumb_work_again;
    }
    return 0;
}

static int32_t usb_jpg_thumb_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t             ret       = RET_OK;
    struct usb_jpg_thumb_s *usb_jpg_thumb = (struct usb_jpg_thumb_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            STREAM_LIBC_FREE(usb_jpg_thumb);
        }
        break;
        case MSI_CMD_PRE_DESTROY:
        {
            os_work_cancle2(&usb_jpg_thumb->work, 1);
        }
        break;
        case MSI_CMD_JPG_THUMB:
        {
            uint32_t cmd_self = (uint32_t) param1;
            uint32_t arg      = param2;
            switch (cmd_self)
            {
                // 启动拍照,一定要拍照完毕才能退出?如果需要强行退出,需要设置另外接口,这里默认一定能拍照成功
                case MSI_JPG_THUMB_TAKEPHOTO:
                {
                    usb_jpg_thumb->takephoto_photo += arg;
                    usb_jpg_thumb->msi->enable = 1;
                }
                break;
                case MSI_JPG_THUMB_TAKEPHOTO_SETPATH:
                {
                    usb_jpg_thumb->dirpath = (char *) arg;
                }
                break;
            }
        }
        break;
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            if (fb->mtype != F_JPG)
            {
                ret = RET_ERR;
            }
            else
            {
                // 检查是否来自需要的数据源头
                if (fb->srcID != usb_jpg_thumb->filter)
                {
                    ret = RET_ERR;
                }
            }
        }
        break;
        case MSI_CMD_TRANS_FB_END:
        {
            os_run_work(&usb_jpg_thumb->work);
        }
        break;
        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            // fb由内部管理,仅仅删除私有结构
            if (fb && fb->mtype == F_FILE_T)
            {
                if (fb->priv)
                {
                    STREAM_LIBC_FREE(fb->priv);
                    fb->priv = NULL;
                }
            }
        }
        break;
    }
    return ret;
}

struct msi *usb_jpg_thumb_msi_init(const char *msi_name, uint8_t filter, uint8_t thumb_stype)
{
    uint8_t             is_new;
    struct msi         *msi       = msi_new(msi_name, USB_JPG_THUMB_RECV_MAX, &is_new);
    struct usb_jpg_thumb_s *usb_jpg_thumb = NULL;
    if (is_new)
    {
        usb_jpg_thumb               = (struct usb_jpg_thumb_s *) STREAM_LIBC_ZALLOC(sizeof(struct usb_jpg_thumb_s));
        msi->priv                   = usb_jpg_thumb;
        usb_jpg_thumb->msi          = msi;
        usb_jpg_thumb->filter       = filter;
        usb_jpg_thumb->thumb_stype  = thumb_stype;
        usb_jpg_thumb->dirpath      = IMG_PATH;
        msi->action                 = usb_jpg_thumb_msi_action;
        msi->enable                 = 0;

        msi_get(usb_jpg_thumb->msi);
        OS_WORK_INIT(&usb_jpg_thumb->work, usb_jpg_thumb_work, 0);
    }

    return msi;
}


