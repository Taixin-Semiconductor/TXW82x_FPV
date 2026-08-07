#include "app_lcd.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE av_free
#define STREAM_LIBC_ZALLOC av_zalloc

#define MAX_LCD_VIDEO 8

static int lcd_video_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int ret = RET_OK;
    struct lcd_video_msi_s *lcd_video = (struct lcd_video_msi_s *)msi->priv;
    switch (cmd_id)
    {

        // 暂时没有考虑释放
        case MSI_CMD_POST_DESTROY:
        {
            STREAM_LIBC_FREE(lcd_video);
        }
        break;
        // 类型不匹配,就不需要接收
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;
            if((fb->mtype != F_YUV) || (fb->stype != lcd_video->filter))
            {
                ret = RET_ERR;
            }
        }
        break;
        case MSI_CMD_FREE_FB:
        {
        }
        break;

        case MSI_CMD_LCD_VIDEO:
        {
            uint32_t cmd_self = (uint32_t)param1;
            uint32_t arg = param2;
            switch (cmd_self)
            {
                case MSI_VIDEO_ENABLE:
                {
                    msi->enable = arg;
                }
                break;
            }
        }
        break;
        default:
            break;
    }
    return ret;
}

struct msi *lcd_video_msi_init(const char *name, uint16_t filter)
{
    struct msi *msi = msi_new(name, MAX_LCD_VIDEO, NULL);
    struct lcd_video_msi_s *lcd_video = (struct lcd_video_msi_s *)msi->priv;
    if (!lcd_video)
    {
        lcd_video = (struct lcd_video_msi_s *)STREAM_LIBC_ZALLOC(sizeof(struct lcd_video_msi_s));
        lcd_video->msi = msi;
        lcd_video->filter = filter;
        msi->priv = (void *)lcd_video;
        msi->enable = 1;
        msi->action = lcd_video_msi_action;
    }
    return msi;
}
