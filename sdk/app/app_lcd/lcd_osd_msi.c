#include "app_lcd.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
// 这里只是为了给osd创建流,实际内部没有做什么,这个流取到的数据应该是已经压缩过的
// 接收到就可以配置硬件寄存器了

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE av_free
#define STREAM_LIBC_ZALLOC av_zalloc

#define MAX_OSD_SHOW_RX 8

static int lcd_osd_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int ret = RET_OK;
    struct lcd_osd_msi_s *show = (struct lcd_osd_msi_s *)msi->priv;
    switch (cmd_id)
    {

        // 暂时没有考虑释放
        case MSI_CMD_POST_DESTROY:
        {

            STREAM_LIBC_FREE(show);
        }
        break;
        // 接收,判断是否已经压缩了
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;
            if(fb->mtype != F_ERGB)
            {
                ret = RET_ERR;
            }
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
struct msi *lcd_osd_msi(const char *name)
{
    struct msi *msi = msi_new(name, MAX_OSD_SHOW_RX, NULL);
    struct lcd_osd_msi_s *show = (struct lcd_osd_msi_s *)msi->priv;
    struct lcdc_device *lcd_dev;
    if (!show)
    {
        show = (struct lcd_osd_msi_s *)STREAM_LIBC_ZALLOC(sizeof(struct lcd_osd_msi_s));
        lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);
        show->lcd_dev = lcd_dev;
        msi->action = lcd_osd_msi_action;
        show->msi = msi;
        msi->priv = (void *)show;
        msi->enable = 1;
    }

    return msi;
}