#include "basic_include.h"
#include "stream_define.h"
#include "utlist.h"
#include "lib/lcd/lcd.h"
#include "app_lcd.h"
#include "lib/multimedia/msi.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "user_work/user_work.h"

enum LCD_SHOW_MAP
{
    LCD_OSD = BIT(0),
    LCD_P0  = BIT(1),
    LCD_P1  = BIT(2),
    LCD_P2  = BIT(3),
};

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

#define LCD_MSI_DEBUG(fmt, ...) // os_printf(fmt, ##__VA_ARGS__)

/* 中断 kick lcd 模式 */

typedef int32 (*lcdc_set_rotate_y_src_addr)(struct lcdc_device *p_lcdc, uint32 yaddr);
typedef int32 (*lcdc_set_rotate_u_src_addr)(struct lcdc_device *p_lcdc, uint32 uaddr);
typedef int32 (*lcdc_set_rotate_v_src_addr)(struct lcdc_device *p_lcdc, uint32 vaddr);

static void lcd_msi_irq_video_show(struct app_lcd_s *lcd_s, uint8_t which_video, uint8_t *p0_p1_enable, uint32_t *p_w, uint32_t *p_h)
{
    struct msi             *v_msi = NULL;
    struct lcd_video_msi_s *video = NULL;

    struct display_lcd        *last_fb = NULL;
    uint8_t                    ret;
    uint32_t                   temp_w, temp_h;
    uint16_t                  *x                            = NULL;
    uint16_t                  *y                            = NULL;
    lcdc_set_rotate_y_src_addr p_lcdc_set_rotate_y_src_addr = NULL;
    lcdc_set_rotate_u_src_addr p_lcdc_set_rotate_u_src_addr = NULL;
    lcdc_set_rotate_v_src_addr p_lcdc_set_rotate_v_src_addr = NULL;

    /*****************************************************************************************************
     * 要切换fb的条件分别是:
     * 1、检查delete的队列是否满了并且已经有显示过的fb,如果delete队列满了,那不能切换fb,需要保持原样,等待删除队列有空闲位置才能去切换
     * 2、需要检查当前是否有需要显示的,如果没有显示的,可以去从对应的队列获取fb去显示
     *****************************************************************************************************/
    if (which_video == 0)
    {
        video = (struct lcd_video_msi_s *) lcd_s->video_p0_msi->priv;

        v_msi   = lcd_s->video_p0_msi;
        last_fb = lcd_s->p0_fb;
        if (v_msi->enable)
        {
            x                            = &lcd_s->x0;
            y                            = &lcd_s->y0;
            p_lcdc_set_rotate_y_src_addr = lcdc_set_p0_rotate_y_src_addr;
            p_lcdc_set_rotate_u_src_addr = lcdc_set_p0_rotate_u_src_addr;
            p_lcdc_set_rotate_v_src_addr = lcdc_set_p0_rotate_v_src_addr;

            // 如果上一次没有显示,则尝试从队列获取fb去显示
            if (!last_fb)
            {

                RB_GET(&lcd_s->p0_rb, lcd_s->p0_fb);
                last_fb = lcd_s->p0_fb;
            }
            // 曾经显示过,就要检查一下delete rb是否还有空位,有空位才能切换fb
            else if (!RB_FULL(&lcd_s->delete_rb))
            {

                // 检查是否有需要切换的fb
                ret = RB_GET(&lcd_s->p0_rb, lcd_s->p0_fb);
                // 更新切换的fb
                if (ret)
                {
                    // 把需要删除的last fb放到delete rb的队列
                    RB_INT_SET(&lcd_s->delete_rb, last_fb);
                    last_fb = lcd_s->p0_fb;
                }
            }
            // last fb存在和delete rb队列满了,则不需要切换,等待下次切换
            else
            {
            }
        }
        // 使能被关闭,则需要清除显示的fb,那么要检查delete rb的队列是否为空
        else
        {

            if (last_fb && !RB_FULL(&lcd_s->delete_rb))
            {

                RB_INT_SET(&lcd_s->delete_rb, last_fb);
                last_fb      = NULL;
                lcd_s->p0_fb = NULL;
            }
        }
    }
    else if (which_video == 1)
    {
        v_msi   = lcd_s->video_p1_msi;
        video   = (struct lcd_video_msi_s *) lcd_s->video_p1_msi->priv;
        last_fb = lcd_s->p1_fb;
        if (v_msi->enable)
        {
            x                            = &lcd_s->x1;
            y                            = &lcd_s->y1;
            p_lcdc_set_rotate_y_src_addr = lcdc_set_p1_rotate_y_src_addr;
            p_lcdc_set_rotate_u_src_addr = lcdc_set_p1_rotate_u_src_addr;
            p_lcdc_set_rotate_v_src_addr = lcdc_set_p1_rotate_v_src_addr;

            // 如果上一次没有显示,则尝试从队列获取fb去显示
            if (!last_fb)
            {
                RB_GET(&lcd_s->p1_rb, lcd_s->p1_fb);
                last_fb = lcd_s->p1_fb;
            }
            // 曾经显示过,就要检查一下delete rb是否还有空位,有空位才能切换fb
            else if (!RB_FULL(&lcd_s->delete_rb))
            {
                // 检查是否有需要切换的fb
                ret = RB_GET(&lcd_s->p1_rb, lcd_s->p1_fb);
                // 更新切换的fb
                if (ret)
                {
                    // 把需要删除的last fb放到delete rb的队列
                    RB_INT_SET(&lcd_s->delete_rb, last_fb);
                    last_fb = lcd_s->p1_fb;
                }
            }
            // last fb存在和delete rb队列满了,则不需要切换,等待下次切换
            else
            {
            }
        }
        // 使能被关闭,则需要清除显示的fb,那么要检查delete rb的队列是否为空
        else
        {
            if (last_fb && !RB_FULL(&lcd_s->delete_rb))
            {
                RB_INT_SET(&lcd_s->delete_rb, last_fb);
                last_fb      = NULL;
                lcd_s->p1_fb = NULL;
            }
        }
    }
    else if (which_video == 2)
    {
        v_msi   = lcd_s->csc_video_p2_msi;
        video   = (struct lcd_video_msi_s *) lcd_s->csc_video_p2_msi->priv;
        last_fb = lcd_s->p2_fb;
        if (v_msi->enable)
        {
            x                            = &lcd_s->x0;
            y                            = &lcd_s->y0;
            p_lcdc_set_rotate_y_src_addr = lcdc_set_p0_rotate_y_src_addr;
            p_lcdc_set_rotate_u_src_addr = lcdc_set_p0_rotate_u_src_addr;
            p_lcdc_set_rotate_v_src_addr = lcdc_set_p0_rotate_v_src_addr;

            // 如果上一次没有显示,则尝试从队列获取fb去显示
            if (!last_fb)
            {
                RB_GET(&lcd_s->p2_rb, lcd_s->p2_fb);
                last_fb = lcd_s->p2_fb;
            }
            // 曾经显示过,就要检查一下delete rb是否还有空位,有空位才能切换fb
            else if (!RB_FULL(&lcd_s->delete_rb))
            {
                // 检查是否有需要切换的fb
                ret = RB_GET(&lcd_s->p2_rb, lcd_s->p2_fb);
                // 更新切换的fb
                if (ret)
                {
                    // 把需要删除的last fb放到delete rb的队列
                    RB_INT_SET(&lcd_s->delete_rb, last_fb);
                    last_fb = lcd_s->p2_fb;
                }
            }
            // last fb存在和delete rb队列满了,则不需要切换,等待下次切换
            else
            {
            }
        }
        // 使能被关闭,则需要清除显示的fb,那么要检查delete rb的队列是否为空
        else
        {
            if (last_fb && !RB_FULL(&lcd_s->delete_rb))
            {
                RB_INT_SET(&lcd_s->delete_rb, last_fb);
                last_fb      = NULL;
                lcd_s->p2_fb = NULL;
            }
        }
    }

    // 需要将队列的数据清除
    if (!v_msi->enable)
    {
        return;
    }

    struct framebuff *fb = NULL;
    if (last_fb)
    {
        fb = last_fb->fb;
    }

    // 这里再次判断p_fb,保证非空才打开使能，有可能此时外部选择关闭
    if (fb)
    {
        *p0_p1_enable |= BIT(which_video);
        txYuvInfo_t *yuvinfo = (txYuvInfo_t *) fb->codec_info;
        //uint32_t     y_off = 0, uv_off = 0;
        //uint8_t     *p_buf = (uint8_t *) fb->data;

        temp_w = yuvinfo->width;
        temp_h = yuvinfo->height;
        *p_w   = temp_w;
        *p_h   = temp_h;
        *x     = yuvinfo->x;
        *y     = yuvinfo->y;
// 暂时忽略,需要确认才行
#if 0
        if (lcd_s->video_rotate == LCD_ROTATE_180)
        {
            y_off  = temp_w * (temp_h - 1);
            uv_off = ((temp_w / 2 + 3) / 4) * 4 * (temp_h / 2 - 1);
        }
#endif
        p_lcdc_set_rotate_y_src_addr(lcd_s->lcd_dev, (uint32) yuvinfo->y_off);
        p_lcdc_set_rotate_u_src_addr(lcd_s->lcd_dev, (uint32) yuvinfo->u_off);
        p_lcdc_set_rotate_v_src_addr(lcd_s->lcd_dev, (uint32) yuvinfo->v_off);
    }
}

static void lcd_msi_irq_osd_show(struct app_lcd_s *lcd_s, uint8_t *osd_en)
{

    struct msi         *v_msi = lcd_s->lcd_osd_msi;
    int32_t             ret;
    struct display_lcd *last_fb = lcd_s->osd_fb;

    if (v_msi->enable)
    {

        // 如果上一次没有显示,则尝试从队列获取fb去显示
        if (!last_fb)
        {

            RB_GET(&lcd_s->osd_rb, lcd_s->osd_fb);
            last_fb = lcd_s->osd_fb;
        }
        // 曾经显示过,就要检查一下delete rb是否还有空位,有空位才能切换fb
        else if (!RB_FULL(&lcd_s->delete_rb))
        {

            // 检查是否有需要切换的fb
            ret = RB_GET(&lcd_s->osd_rb, lcd_s->osd_fb);
            // 更新切换的fb
            if (ret)
            {

                // 把需要删除的last fb放到delete rb的队列
                RB_INT_SET(&lcd_s->delete_rb, last_fb);
                last_fb = lcd_s->osd_fb;
            }
        }
        // last fb存在和delete rb队列满了,则不需要切换,等待下次切换
        else
        {
        }
    }
    else
    {

        if (last_fb && !RB_FULL(&lcd_s->delete_rb))
        {
            RB_INT_SET(&lcd_s->delete_rb, last_fb);
            last_fb       = NULL;
            lcd_s->osd_fb = NULL;
        }
    }
    // 这里再次判断osd_fb,保证非空才打开使能，有可能此时外部选择关闭

    if (last_fb)
    {
        *osd_en = 1;
        lcdc_set_osd_dma_addr(lcd_s->lcd_dev, (uint32_t) last_fb->fb->data);
    }
}

void lcd_msi_irq_callback(void *data)
{
    struct app_lcd_s *lcd_s        = (struct app_lcd_s *) data;
    uint8_t           p0_p1_enable = 0;
    uint8_t           osd_en       = 0;
    uint32_t          p0_w = 0, p0_h = 0, p1_w = 0, p1_h = 0;

    // 设置 OSD 的 DMA 地址
    lcd_msi_irq_osd_show(lcd_s, &osd_en);

    // 设置视频 P0 的地址
    lcd_msi_irq_video_show(lcd_s, 0, &p0_p1_enable, &p0_w, &p0_h);

    // 设置视频 P1 的地址
    lcd_msi_irq_video_show(lcd_s, 1, &p0_p1_enable, &p1_w, &p1_h);

    // 设置 CSC P2 的地址
    lcd_msi_irq_video_show(lcd_s, 2, &p0_p1_enable, &p0_w, &p0_h);

    if (lcd_s->p2_fb && lcd_s->osd_fb)
    {
        uint32_t osd_fb_time = lcd_s->osd_fb->fb->time;
        uint32_t p2_fb_time  = lcd_s->p2_fb->fb->time;

        int32_t             diff     = osd_fb_time - p2_fb_time;
        struct display_lcd *del_fb   = NULL;
        uint8_t             del_flag = 0;
        // 因为在中断,delete rb有空位,代表可以删除
        if (!RB_FULL(&lcd_s->delete_rb))
        {
            del_flag = 1;
        }
        // 需要移除一个fb,这里需要保证delete rb有空位,没有空位,则报一下错误(某些情况应该是异常显示,需要优化)
        if (diff > 0)
        {
            del_fb = lcd_s->p2_fb;
            p0_p1_enable &= ~BIT(2);
            if (del_flag)
            {
                lcd_s->p2_fb = NULL;
            }
        }
        else if (diff < 0)
        {
            del_fb = lcd_s->osd_fb;
            osd_en = 0;
            if (del_flag)
            {
                lcd_s->osd_fb = NULL;
            }
        }
        else
        {
        }

        // 删除对应的fb
        if (del_flag)
        {
            RB_INT_SET(&lcd_s->delete_rb, del_fb);
        }
    }

    if (osd_en == 0 && p0_p1_enable == 0)
    {
        os_printf("(@@@@@@@@@@@)\n");
    }
    // 打开video
    if (p0_p1_enable)
    {
        // 检查一下line_buf是否已经申请了,申请空间就是w*line_num的空间(w应该是和屏旋转有关)
        uint16_t rotate_w;
        rotate_w = lcd_s->video_w;
        if (lcd_s->line_buf)
        {
            lcdc_set_rotate_linebuf_y_addr(lcd_s->lcd_dev, (uint32) lcd_s->line_buf);
            lcdc_set_rotate_linebuf_u_addr(lcd_s->lcd_dev, (uint32) lcd_s->line_buf + rotate_w * lcd_s->line_buf_num);
            lcdc_set_rotate_linebuf_v_addr(lcd_s->lcd_dev, (uint32) lcd_s->line_buf + rotate_w * lcd_s->line_buf_num + (rotate_w / 2) * lcd_s->line_buf_num);

            LCD_MSI_DEBUG("p0_w:%d p0_h:%d p1_w:%d p1_h:%d\n", p0_w, p0_h, p1_w, p1_h);
            lcdc_set_rotate_p0p1_size(lcd_s->lcd_dev, p0_w, p0_h, p1_w, p1_h);
            lcdc_set_rotate_mirror(lcd_s->lcd_dev, 0, lcd_s->video_rotate);

            if (lcd_s->video_rotate == LCD_ROTATE_180)
            {
                lcdc_set_video_start_location(lcd_s->lcd_dev, 0, 0);
                lcdc_set_rotate_p0p1_start_location(lcd_s->lcd_dev, lcd_s->x0, 0, lcd_s->x1, lcd_s->y1);
            }
            else
            {
                lcdc_set_video_start_location(lcd_s->lcd_dev, 0, 0);
                lcdc_set_rotate_p0p1_start_location(lcd_s->lcd_dev, lcd_s->x0, lcd_s->y0, lcd_s->x1, lcd_s->y1);
            }

            lcdc_set_p0p1_enable(lcd_s->lcd_dev, (p0_p1_enable & BIT(0)) ? 1 : (p0_p1_enable & BIT(2)), p0_p1_enable & BIT(1));

            lcdc_set_video_en(lcd_s->lcd_dev, 1);
        }
        else
        {
            lcdc_set_video_en(lcd_s->lcd_dev, 0);
        }
    }
    else
    {
        if (lcd_s->free_line_buf && lcd_s->line_buf)
        {
            LCD_MSI_DEBUG("-----------force free video line buff:0x%x-----------\n", lcd_s->free_line_buf);
            STREAM_LIBC_FREE(lcd_s->free_line_buf);
            lcd_s->free_line_buf = NULL;
        }
        if (lcd_s->line_buf)
        {
            lcd_s->free_line_buf = lcd_s->line_buf;
            lcd_s->line_buf      = NULL;
        }
        lcdc_set_video_en(lcd_s->lcd_dev, 0);
    }

    lcdc_set_osd_en(lcd_s->lcd_dev, osd_en);

    lcdc_set_timeout_info(lcd_s->lcd_dev, 1, 3);

    lcdc_set_start_run(lcd_s->lcd_dev);

    lcd_s->start_time = os_jiffies();
}

// 检查如果被关闭了,则清空队列,这里有个问题,如果使能被关闭后立刻打开,是不会清空队列的,这个要考虑后续如何去处理
void clear_msi_fb(struct msi *v_msi)
{
    struct framebuff *fb;
    while (v_msi)
    {
        fb = msi_get_fb(v_msi, 0);
        if (fb)
        {
            os_printf("msi name:%s\tfb:%X\n", v_msi->name, fb);
            msi_delete_fb(NULL, fb);
        }
        else
        {
            break;
        }
    }
}

int32 lcd_msi_work(struct os_work *work)
{
    struct app_lcd_s *lcd_s = (struct app_lcd_s *) work;

    struct lcd_video_msi_s *video_p0     = NULL;
    struct lcd_video_msi_s *video_p1     = NULL;
    struct lcd_video_msi_s *csc_video_p2 = NULL;
    struct lcd_osd_msi_s   *osd_show     = NULL;

    struct framebuff   *fb           = NULL;
    struct display_lcd *display      = NULL;
    uint32_t            lcd_show_map = 0;
    uint32_t            temp         = 0;

    static int count = 0;

    osd_show     = (struct lcd_osd_msi_s *) lcd_s->lcd_osd_msi->priv;
    video_p0     = (struct lcd_video_msi_s *) lcd_s->video_p0_msi->priv;
    video_p1     = (struct lcd_video_msi_s *) lcd_s->video_p1_msi->priv;
    csc_video_p2 = (struct lcd_video_msi_s *) lcd_s->csc_video_p2_msi->priv;

    // 如果不需要显示video,则空间释放吧
    if (lcd_s->free_line_buf)
    {
        LCD_MSI_DEBUG("-----------free video line buff:0x%x-----------\n", lcd_s->free_line_buf);

        irq_disable(LCD_IRQn);
        temp                 = (uint32_t) lcd_s->free_line_buf;
        lcd_s->free_line_buf = NULL;
        irq_enable(LCD_IRQn);

        STREAM_LIBC_FREE((void *) temp);
    }

    // 检查一下是否有fb需要删除,从delete rb里面查找
    //
    while (!RB_EMPTY(&lcd_s->delete_rb))
    {
        RB_GET(&lcd_s->delete_rb, display);
        fb = display->fb;
        LCD_MSI_DEBUG("lcd delete fb:0x%x\n", fb);
        display->free(display);
    }

    // 检查是否需要清空队列,一个一个清空(通过使能位来判断,如果使能被关闭,则清空对应的队列)
    // 这里没有考虑正在显示的情况,正在显示的由中断去放到delete rb的队列
    // 将对应rb队列清空(由于ringbuf结构体原因,暂时没有通用函数,先在这里清空)
    if (!lcd_s->lcd_osd_msi->enable)
    {
        while (!RB_EMPTY(&lcd_s->osd_rb))
        {
            // 因为get会在中断使用,所以这里采用安全方式去读取,防止异步问题
            if (RB_INT_GET(&lcd_s->osd_rb, display))
            {
                display->free(display);
            }
        }
    }
    if (!lcd_s->video_p0_msi->enable)
    {
        while (!RB_EMPTY(&lcd_s->p0_rb))
        {
            // 因为get会在中断使用,所以这里采用安全方式去读取,防止异步问题
            if (RB_INT_GET(&lcd_s->p0_rb, display))
            {
                display->free(display);
            }
        }
    }

    if (!lcd_s->video_p1_msi->enable)
    {
        while (!RB_EMPTY(&lcd_s->p1_rb))
        {
            // 因为get会在中断使用,所以这里采用安全方式去读取,防止异步问题
            if (RB_INT_GET(&lcd_s->p1_rb, display))
            {
                display->free(display);
            }
        }
    }
    if (!lcd_s->csc_video_p2_msi->enable)
    {
        while (!RB_EMPTY(&lcd_s->p2_rb))
        {
            // 因为get会在中断使用,所以这里采用安全方式去读取,防止异步问题
            if (RB_INT_GET(&lcd_s->p2_rb, display))
            {
                display->free(display);
            }
        }
    }

    // 检查队列是否为空
    if (lcd_s->osd_fb || !RB_EMPTY(&lcd_s->osd_rb))
    {
        lcd_show_map |= LCD_OSD;
    }

    if (lcd_s->p0_fb || !RB_EMPTY(&lcd_s->p0_rb))
    {
        lcd_show_map |= LCD_P0;
    }

    if (lcd_s->p1_fb || !RB_EMPTY(&lcd_s->p1_rb))
    {
        lcd_show_map |= LCD_P1;
    }

    if (lcd_s->p2_fb || !RB_EMPTY(&lcd_s->p2_rb))
    {
        lcd_show_map |= LCD_P2;
    }

    // 如果p0和p1、p2的队列不为空,则要尝试申请空间
    if (lcd_show_map & (LCD_P0 | LCD_P1 | LCD_P2))
    {
        uint16_t rotate_w;
        rotate_w = lcd_s->video_w;
        // 这里理论要申请到空间,如果申请失败,重新申请一下(否则要就额外处理资源数据)
        if (!lcd_s->line_buf)
        {
            lcd_s->line_buf = (void *) STREAM_LIBC_MALLOC(lcd_s->line_buf_num * rotate_w * 2);
            LCD_MSI_DEBUG("-----------alloc video line buff:0x%x-----------\n", lcd_s->line_buf);
        }
        // 空间申请失败,是尝试等待申请空间还是清空队列?这里就是等待下次申请空间去尝试吧
        if (!lcd_s->line_buf)
        {
        }
    }

    if (lcd_show_map)
    {
        lcd_s->hardware_ready = 0; // 标记fb已准备好，等待刷新
    }
    else
    {
        goto __lcd_msi_work_end;
    }

    if ((!count && !lcd_s->hardware_ready) || lcd_s->rekick_lcd)
    {
        count++;
        lcd_s->rekick_lcd = 0;
        LCD_MSI_DEBUG("=========first kick lcd=========\n");
        lcd_s->app_lcd_cb(lcd_s);
    }

__lcd_msi_work_end:
    os_run_work_delay(&lcd_s->work, 1);

    return 0;
}