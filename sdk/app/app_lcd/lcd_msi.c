#include "basic_include.h"
#include "stream_frame.h"
#include "utlist.h"
#include "lib/lcd/lcd.h"
#include "app_lcd.h"
#include "lib/multimedia/msi.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "user_work/user_work.h"

//data申请空间函数
#define STREAM_MALLOC     av_psram_malloc
#define STREAM_FREE       av_psram_free
#define STREAM_ZALLOC     av_psram_zalloc

//结构体申请空间函数
#define STREAM_LIBC_MALLOC     av_malloc
#define STREAM_LIBC_FREE       av_free
#define STREAM_LIBC_ZALLOC     av_zalloc

#define LCD_MSI_DEBUG(fmt, ...)    //os_printf(fmt, ##__VA_ARGS__)



uint8_t video_show(struct app_lcd_s *lcd_s, uint8_t which_video, uint32_t *s_p_w, uint32_t *s_p_h,int16_t x_mov)
{
    uint32_t p_w = 0, p_h = 0;
    uint32_t y_off = 0, uv_off = 0;
    struct lcdc_device *lcd_dev = lcd_s->lcd_dev;
    struct yuv_arg_s *yuv_msg;
    struct framebuff *fb;
    uint8_t res = 0;
    struct lcd_video_msi_s *video;
    struct msi *video_msi = NULL;
    if (which_video == 0)
    {
        video_msi = lcd_s->video_p0_msi;
    }
    else if (which_video == 1)
    {
        video_msi = lcd_s->video_p1_msi;
    }
    if (!video_msi)
    {
        return res;
    }
    video = (struct lcd_video_msi_s *)video_msi->priv;
    fb = msi_get_fb(video->msi, 0);
    if (fb)
    {
        if (video->last_fb)
        {
            msi_delete_fb(NULL, video->last_fb);
        }
        video->last_fb = fb;
    }
    else
    {
        fb = video->last_fb;
    }

    // 如果已经关闭了使能,就要释放一下之前的资源了
    if (!video_msi->enable)
    {
        if (video->last_fb)
        {
            msi_delete_fb(NULL, video->last_fb);
        }
        video->last_fb = NULL;
        fb = NULL;
    }

#if 0
    //判断一下显示时间,如果过时的,就不显示了
    if(fb)
    {
        uint32_t show_time =stream_self_cmd_func(p0_s,GET_LVGL_YUV_UPDATE_TIME,(uint32_t)0);
        if(show_time > get_stream_data_timestamp(video_p0_data_s))
        {
            free_data(video_p0_data_s);
            yuv_p0->last_data_s = NULL;
            goto lcd_thread_get_video_p0_again;
        }
    }
#endif
    if (fb)
    {
        yuv_msg = (struct yuv_arg_s *)fb->priv;
        if (yuv_msg)
        {
            uint8_t *p_buf = (uint8_t *)fb->data;
            p_w = yuv_msg->out_w;
            p_h = yuv_msg->out_h;

            if (lcd_s->video_rotate == LCD_ROTATE_180)
            {
                y_off = p_w * (p_h - 1);
                uv_off = ((p_w / 2 + 3) / 4) * 4 * (p_h / 2 - 1);
            }
            if (which_video == 0)
            {
                lcd_s->x0 = yuv_msg->x;
                lcd_s->y0 = yuv_msg->y;
                lcdc_set_p0_rotate_y_src_addr(lcd_dev, (uint32)p_buf + y_off + x_mov);
                lcdc_set_p0_rotate_u_src_addr(lcd_dev, (uint32)p_buf + yuv_msg->y_size + uv_off + x_mov/2);
                lcdc_set_p0_rotate_v_src_addr(lcd_dev, (uint32)p_buf + yuv_msg->y_size + yuv_msg->y_size / 4 + uv_off + x_mov/2);
            }
            else if (which_video == 1)
            {
                lcd_s->x1 = yuv_msg->x;
                lcd_s->y1 = yuv_msg->y;
                lcdc_set_p1_rotate_y_src_addr(lcd_dev, (uint32)p_buf + y_off);
                lcdc_set_p1_rotate_u_src_addr(lcd_dev, (uint32)p_buf + yuv_msg->y_size + uv_off);
                lcdc_set_p1_rotate_v_src_addr(lcd_dev, (uint32)p_buf + yuv_msg->y_size + yuv_msg->y_size / 4 + uv_off);
            }

            *s_p_w = p_w;
            *s_p_h = p_h;
            res = 1;
        }
        //无效
        else
        {
            video->last_fb = NULL;
            msi_delete_fb(NULL, fb);
        }
    }
    return res;
}

uint8_t osd_show(struct app_lcd_s *lcd_s)
{
    uint8_t res = 0;
    struct msi *lcd_osd_msi = (struct msi *)lcd_s->lcd_osd_msi;
    struct lcd_osd_msi_s *osd_show = (struct lcd_osd_msi_s *)lcd_osd_msi->priv;
    struct framebuff *fb;
    struct lcdc_device *lcd_dev = lcd_s->lcd_dev;
    fb = msi_get_fb(osd_show->msi, 0);
    if (fb)
    {
        //LCD_MSI_DEBUG("@@@@@@@@@@@@fb:%X\n",fb);
        if (osd_show->last_show_data_s)
        {
            msi_delete_fb(NULL, osd_show->last_show_data_s);
        }

        osd_show->last_show_data_s = fb;
    }
    else
    {
        fb = osd_show->last_show_data_s;
    }

    if (fb)
    {
        lcdc_set_osd_dma_addr(lcd_dev, (uint32_t)fb->data);
        // lcdc_set_osd_en(lcd_dev,1);
        res = 1;
    }

    return res;
}

// 如果说workqueue确定没有一些耗时的任务,这个也是可以放到workqueue去处理,节省线程的内存
void lcd_thread(void *d)
{
    struct app_lcd_s *lcd_s = (struct app_lcd_s *)d;
    struct lcdc_device *lcd_dev = lcd_s->lcd_dev;

	uint16_t x_mov= 0,y_mov = 0;
    uint32_t p0_w = 0, p0_h = 0, p1_w = 0, p1_h = 0;
    // 是否要启动显示,只要有osd或者video显示,都要启动硬件模块
    // 如果是mcu屏,内部可以增加其他逻辑,判断显示是否有变化决定是否刷屏
    // 如果是rgb屏,无论内容是否发生变化,都要刷新,否则会有问题
    uint8_t flag = 0;
    uint8_t p0_p1_enable = 0;
    uint8_t osd_en;
    while (!lcd_s->thread_exit)
    {

        // ready
        p0_p1_enable = 0;
        osd_en = 0;
		
        if (lcd_s->hardware_ready)
        {

            // Video P0显示检查
            if (video_show(lcd_s, 0, &p0_w, &p0_h,x_mov))
            {
                p0_p1_enable |= BIT(0);
                flag++;
            }


            // Video P1显示检查
            if (video_show(lcd_s, 1, &p1_w, &p1_h,0))
            {
                p0_p1_enable |= BIT(1);
                flag++;
            }

            // osd显示
            if (osd_show(lcd_s))
            {
                osd_en = 1;
                flag++;
            }
            lcdc_set_osd_en(lcd_dev, osd_en);

            // 打开video
            if (p0_p1_enable)
            {
            	
                // 检查一下line_buf是否已经申请了,申请空间就是w*line_num的空间(w应该是和屏旋转有关)
                if (!lcd_s->line_buf)
                {
                    uint16_t rotate_w;
                    rotate_w = lcd_s->screen_w;
                    lcd_s->line_buf = (void *)STREAM_LIBC_MALLOC(lcd_s->line_buf_num * rotate_w * 2);
                    // 这里理论要申请到空间,如果申请失败,重新申请一下(否则要就额外处理资源数据)
                    while (!lcd_s->line_buf)
                    {
                        os_sleep_ms(1);
                        lcd_s->line_buf = (void *)STREAM_LIBC_MALLOC(lcd_s->line_buf_num * rotate_w * 2);
                    }
                    lcdc_set_rotate_linebuf_y_addr(lcd_dev, (uint32)lcd_s->line_buf);
                    lcdc_set_rotate_linebuf_u_addr(lcd_dev, (uint32)lcd_s->line_buf + rotate_w * lcd_s->line_buf_num);
                    lcdc_set_rotate_linebuf_v_addr(lcd_dev, (uint32)lcd_s->line_buf + rotate_w * lcd_s->line_buf_num + (rotate_w / 2) * lcd_s->line_buf_num);
                }
                lcdc_set_rotate_p0p1_size(lcd_dev, p0_w, p0_h, p1_w, p1_h);
                lcdc_set_rotate_mirror(lcd_dev, 0, lcd_s->video_rotate);

				if(lcd_s->video_rotate == LCD_ROTATE_180){
					lcdc_set_video_start_location(lcd_dev,0,0); 
					lcdc_set_rotate_p0p1_start_location(lcd_dev,lcd_s->x0,y_mov,lcd_s->x1,lcd_s->y1);
				}else{
					lcdc_set_video_start_location(lcd_dev,0,y_mov); 
					lcdc_set_rotate_p0p1_start_location(lcd_dev,lcd_s->x0,lcd_s->y0,lcd_s->x1,lcd_s->y1);
				}
				lcdc_set_p0p1_enable(lcd_dev, p0_p1_enable & BIT(0), p0_p1_enable & BIT(1));

                lcdc_set_video_en(lcd_dev, 1);
                flag++;
            }
            else
            {
                // 如果不需要显示video,则空间释放吧
                if (lcd_s->line_buf)
                {
                    STREAM_LIBC_FREE(lcd_s->line_buf);
                    lcd_s->line_buf = NULL;
                }
                lcdc_set_video_en(lcd_dev, 0);
            }
        }
        // lcd模块已经启动,hardware_ready清0
        if (flag)
        {
            lcd_s->hardware_ready = 0;
            // LCD_MSI_DEBUG("last lcd stream spend time:%d\n",lcd_s->end_time-lcd_s->start_time);
            lcd_s->start_time = os_jiffies();
            lcdc_set_start_run(lcd_dev);
            flag = 0;
        }
        else
        {
            // 不需要显示,则休眠1ms重新检查
            // 这里暂时没有用信号量唤醒,主要考虑到osd和video都有可能需要显示,要考虑如何监听osd和video的数据才可以用信号量
            // 这里用sleep,会占用一点点cpu
            os_sleep_ms(1);
        }
    }

    // 相当于退出lcd的线程
    lcd_s->thread_exit = 0;
    lcd_s->thread_hdl = NULL;
    return;
}






/* 中断 kick lcd 模式 */

typedef int32 (*lcdc_set_rotate_y_src_addr)(struct lcdc_device *p_lcdc, uint32 yaddr);
typedef int32 (*lcdc_set_rotate_u_src_addr)(struct lcdc_device *p_lcdc, uint32 uaddr);
typedef int32 (*lcdc_set_rotate_v_src_addr)(struct lcdc_device *p_lcdc, uint32 vaddr);

static void lcd_msi_irq_video_show(struct app_lcd_s *lcd_s, uint8_t which_video, uint8_t *p0_p1_enable, uint32_t *p_w, uint32_t* p_h)
{
    struct lcd_video_msi_s * video = NULL;
    struct framebuff ** p_fb = NULL;
    uint32_t temp_w, temp_h;
    uint16_t *x = NULL;
    uint16_t *y = NULL;
    lcdc_set_rotate_y_src_addr p_lcdc_set_rotate_y_src_addr = NULL;
    lcdc_set_rotate_u_src_addr p_lcdc_set_rotate_u_src_addr = NULL;
    lcdc_set_rotate_v_src_addr p_lcdc_set_rotate_v_src_addr = NULL;

    if (which_video == 0)
    {
        video = (struct lcd_video_msi_s *)lcd_s->video_p0_msi->priv;
        p_fb = &lcd_s->p0_fb;
        x = &lcd_s->x0;
        y = &lcd_s->y0;
        p_lcdc_set_rotate_y_src_addr = lcdc_set_p0_rotate_y_src_addr;
        p_lcdc_set_rotate_u_src_addr = lcdc_set_p0_rotate_u_src_addr;
        p_lcdc_set_rotate_v_src_addr = lcdc_set_p0_rotate_v_src_addr;
    }
    else if (which_video == 1)
    {
        video = (struct lcd_video_msi_s *)lcd_s->video_p1_msi->priv;
        p_fb = &lcd_s->p1_fb;
        x = &lcd_s->x1;
        y = &lcd_s->y1;
        p_lcdc_set_rotate_y_src_addr = lcdc_set_p1_rotate_y_src_addr;
        p_lcdc_set_rotate_u_src_addr = lcdc_set_p1_rotate_u_src_addr;
        p_lcdc_set_rotate_v_src_addr = lcdc_set_p1_rotate_v_src_addr;
    }

    struct framebuff *fb = *p_fb;
    
    if (fb != video->cur_fb)
    {
        if (video->delete_fb) {
            LCD_MSI_DEBUG("-----------force delete video p%d fb:0x%x-----------\n", which_video, fb);
            msi_delete_fb(NULL, fb);
        } else {
            video->delete_fb = fb;
        }
        *p_fb = video->cur_fb;
        fb = video->cur_fb;
        LCD_MSI_DEBUG("exchange video p%d fb:0x%x\n", which_video, fb);
    }
    //这里再次判断p_fb,保证非空才打开使能，有可能此时外部选择关闭
    if (fb) {

        *p0_p1_enable |= BIT(which_video);

        struct yuv_arg_s * yuv_msg = (struct yuv_arg_s *)fb->priv;
        uint32_t y_off = 0, uv_off = 0;
        uint8_t *p_buf = (uint8_t *)fb->data;

        temp_w = yuv_msg->out_w;
        temp_h = yuv_msg->out_h;
        *p_w = temp_w;
        *p_h = temp_h;
        *x = yuv_msg->x;
        *y = yuv_msg->y;

        if (lcd_s->video_rotate == LCD_ROTATE_180)
        {
            y_off = temp_w * (temp_h - 1);
            uv_off = ((temp_w / 2 + 3) / 4) * 4 * (temp_h / 2 - 1);
        }
        LCD_MSI_DEBUG("p%d yuv_msg->y_size:%d \n", which_video, yuv_msg->y_size);
        p_lcdc_set_rotate_y_src_addr(lcd_s->lcd_dev, (uint32)p_buf + y_off);
        p_lcdc_set_rotate_u_src_addr(lcd_s->lcd_dev, (uint32)p_buf + yuv_msg->y_size + uv_off);
        p_lcdc_set_rotate_v_src_addr(lcd_s->lcd_dev, (uint32)p_buf + yuv_msg->y_size + yuv_msg->y_size / 4 + uv_off);
    }
}

static void lcd_msi_irq_osd_show(struct app_lcd_s *lcd_s, uint8_t *osd_en)
{
    struct lcd_osd_msi_s *osd_show = NULL;
    osd_show = (struct lcd_osd_msi_s *)lcd_s->lcd_osd_msi->priv;

    struct framebuff *osd_fb = lcd_s->osd_fb;

    // 设置 OSD 的 DMA 地址
    if (osd_fb != osd_show->cur_show_data_s)
    {
        if (osd_show->delete_show_data_s) {
            LCD_MSI_DEBUG("-----------force delete osd fb:0x%x-----------\n",osd_fb);
            msi_delete_fb(NULL, osd_fb);
        } else {
            osd_show->delete_show_data_s = osd_fb;
        }
        lcd_s->osd_fb = osd_show->cur_show_data_s;
        osd_fb = osd_show->cur_show_data_s;
        LCD_MSI_DEBUG("exchange osd fb:0x%x\n",osd_fb);
    }
    //这里再次判断osd_fb,保证非空才打开使能，有可能此时外部选择关闭
    if (osd_fb) {
        *osd_en = 1;
        lcdc_set_osd_dma_addr(lcd_s->lcd_dev, (uint32_t)osd_fb->data);
    }

}


void lcd_msi_irq_callback(void *data)
{
    struct app_lcd_s *lcd_s = (struct app_lcd_s *)data;
    uint8_t p0_p1_enable = 0;
    uint8_t osd_en = 0;
    uint32_t p0_w = 0, p0_h = 0, p1_w = 0, p1_h = 0;

    // 设置 OSD 的 DMA 地址
    lcd_msi_irq_osd_show(lcd_s, &osd_en);

    // 设置视频 P0 的地址
    lcd_msi_irq_video_show(lcd_s, 0, &p0_p1_enable, &p0_w, &p0_h);

    // 设置视频 P1 的地址
    lcd_msi_irq_video_show(lcd_s, 1, &p0_p1_enable, &p1_w, &p1_h);

    // 打开video
    if (p0_p1_enable)
    {
        // 检查一下line_buf是否已经申请了,申请空间就是w*line_num的空间(w应该是和屏旋转有关)
        uint16_t rotate_w;
        rotate_w = lcd_s->screen_w;

        if (lcd_s->line_buf) {
            lcdc_set_rotate_linebuf_y_addr(lcd_s->lcd_dev, (uint32)lcd_s->line_buf);
            lcdc_set_rotate_linebuf_u_addr(lcd_s->lcd_dev, (uint32)lcd_s->line_buf + rotate_w * lcd_s->line_buf_num);
            lcdc_set_rotate_linebuf_v_addr(lcd_s->lcd_dev, (uint32)lcd_s->line_buf + rotate_w * lcd_s->line_buf_num + (rotate_w / 2) * lcd_s->line_buf_num);

            LCD_MSI_DEBUG("p0_w:%d p0_h:%d p1_w:%d p1_h:%d\n",p0_w, p0_h, p1_w, p1_h);
            lcdc_set_rotate_p0p1_size(lcd_s->lcd_dev, p0_w, p0_h, p1_w, p1_h);
            lcdc_set_rotate_mirror(lcd_s->lcd_dev, 0, lcd_s->video_rotate);

            lcdc_set_p0p1_enable(lcd_s->lcd_dev, p0_p1_enable & BIT(0), p0_p1_enable & BIT(1));

            lcdc_set_video_en(lcd_s->lcd_dev, 1);
        } else {
            lcdc_set_video_en(lcd_s->lcd_dev, 0);
        }
    }
    else
    {
        if (lcd_s->free_line_buf && lcd_s->line_buf) {
            LCD_MSI_DEBUG("-----------force free video line buff:0x%x-----------\n", lcd_s->free_line_buf);
            STREAM_LIBC_FREE(lcd_s->free_line_buf);
            lcd_s->free_line_buf = NULL;
        }
        if (lcd_s->line_buf) {
            lcd_s->free_line_buf = lcd_s->line_buf;
            lcd_s->line_buf = NULL;
        }
        lcdc_set_video_en(lcd_s->lcd_dev, 0);
    }

    lcdc_set_osd_en(lcd_s->lcd_dev, osd_en);

    lcdc_set_timeout_info(lcd_s->lcd_dev, 1, 3);

    lcdc_set_start_run(lcd_s->lcd_dev);

    lcd_s->start_time = os_jiffies();
}


int32 lcd_msi_work(struct os_work *work)
{
    struct app_lcd_s *lcd_s = (struct app_lcd_s *)work;
    struct framebuff *p0_fb = NULL;
    struct framebuff *p1_fb = NULL;
    struct framebuff *osd_fb = NULL;
    uint8_t p0_p1_enable = 0;
    struct lcd_video_msi_s * video_p0 = NULL;
    struct lcd_video_msi_s * video_p1 = NULL;
    struct lcd_osd_msi_s *osd_show = NULL;
    uint32_t temp = 0;

    static int count = 0;

    if (lcd_s->hardware_ready)
    {
        osd_show = (struct lcd_osd_msi_s *)lcd_s->lcd_osd_msi->priv;
        video_p0 = (struct lcd_video_msi_s *)lcd_s->video_p0_msi->priv;
        video_p1 = (struct lcd_video_msi_s *)lcd_s->video_p1_msi->priv;
        //_os_printf("$");

        video_p0->last_fb = NULL;
        video_p1->last_fb = NULL;
        osd_show->last_show_data_s = NULL;

        if (video_p0->delete_fb) {
            LCD_MSI_DEBUG("delete video p0:0x%x\n",video_p0->delete_fb);

            irq_disable(LCD_IRQn);
            temp = (uint32_t)video_p0->delete_fb;
            video_p0->delete_fb = NULL;
            irq_enable(LCD_IRQn);

            msi_delete_fb(NULL, (struct framebuff *)temp);
        }

        if (video_p1->delete_fb) {
            LCD_MSI_DEBUG("delete video p1:0x%x\n",video_p0->delete_fb);

            irq_disable(LCD_IRQn);
            temp = (uint32_t)video_p1->delete_fb;
            video_p1->delete_fb = NULL;
            irq_enable(LCD_IRQn);

            msi_delete_fb(NULL, (struct framebuff *)temp);
        }

        if (osd_show->delete_show_data_s) {
            LCD_MSI_DEBUG("delete osd show:0x%x\n",osd_show->delete_show_data_s);

            irq_disable(LCD_IRQn);
            temp = (uint32_t)osd_show->delete_show_data_s;
            osd_show->delete_show_data_s = NULL;
            irq_enable(LCD_IRQn);

            msi_delete_fb(NULL, (struct framebuff *)temp);
        }

        // 如果不需要显示video,则空间释放吧
        if (lcd_s->free_line_buf)
        {
            LCD_MSI_DEBUG("-----------free video line buff:0x%x-----------\n", lcd_s->free_line_buf);

            irq_disable(LCD_IRQn);
            temp = (uint32_t)lcd_s->free_line_buf;
            lcd_s->free_line_buf = NULL;
            irq_enable(LCD_IRQn);

            STREAM_LIBC_FREE((void*)temp);
        }

        // 获取视频 P0 的 framebuff
        p0_fb = msi_get_fb(lcd_s->video_p0_msi, 0);
        if (p0_fb)
        {
            LCD_MSI_DEBUG("----get video p0 fb:0x%x----\n",p0_fb);
            p0_p1_enable |= BIT(0);
            video_p0->last_fb = p0_fb;
        }
        else
        {
            if (video_p0->cur_fb) {
                p0_p1_enable |= BIT(0);
                p0_fb = video_p0->cur_fb;
                video_p0->last_fb = NULL;
            }
        }
        if (!lcd_s->video_p0_msi->enable) {
            LCD_MSI_DEBUG("----lcd_s video_p0 msi disable----\n");
            p0_p1_enable &= ~BIT(0);
            msi_delete_fb(NULL, video_p0->last_fb);
            while (1) {
                video_p0->last_fb = msi_get_fb(lcd_s->video_p0_msi, 0);
                if (video_p0->last_fb) {
                    msi_delete_fb(NULL, video_p0->last_fb);
                } else {
                    break;
                }
            }
            video_p0->last_fb = NULL;
            p0_fb = NULL;
        }


        // 获取视频 P1 的 framebuff
        p1_fb = msi_get_fb(lcd_s->video_p1_msi, 0);
        if (p1_fb)
        {
            LCD_MSI_DEBUG("----get video p1 fb:0x%x----\n",p1_fb);
            p0_p1_enable |= BIT(1);
            video_p1->last_fb = p1_fb;
        }
        else
        {
            if (video_p1->cur_fb) {
                p0_p1_enable |= BIT(1);
                p1_fb = video_p1->cur_fb;
                video_p1->last_fb = NULL;
            }
        }
        if (!lcd_s->video_p1_msi->enable) {
            p0_p1_enable &= ~BIT(1);
            msi_delete_fb(NULL, video_p1->last_fb);
            while (1) {
                video_p1->last_fb = msi_get_fb(lcd_s->video_p1_msi, 0);
                if (video_p1->last_fb) {
                    msi_delete_fb(NULL, video_p1->last_fb);
                } else {
                    break;
                }
            }
            video_p1->last_fb = NULL;
            p1_fb = NULL;
        }

        // 获取 OSD 的 framebuff
        osd_fb = msi_get_fb(lcd_s->lcd_osd_msi, 0);
        if (osd_fb)
        {
            LCD_MSI_DEBUG("----get osd show fb:0x%x----\n",osd_fb);
            osd_show->last_show_data_s = osd_fb;
        }
        else
        {
            if (osd_show->cur_show_data_s) {
                osd_fb = osd_show->cur_show_data_s;
                osd_show->last_show_data_s = NULL;
            }
        }
        if (!lcd_s->lcd_osd_msi->enable) {
            msi_delete_fb(NULL, osd_show->last_show_data_s);
            osd_show->last_show_data_s = NULL;
            osd_fb = NULL;
        }

        if (p0_p1_enable)
        {
            uint16_t rotate_w;
            rotate_w = lcd_s->screen_w;
            // 这里理论要申请到空间,如果申请失败,重新申请一下(否则要就额外处理资源数据)
            if (!lcd_s->line_buf) {
                lcd_s->line_buf = (void *)STREAM_LIBC_MALLOC(lcd_s->line_buf_num * rotate_w * 2);
                LCD_MSI_DEBUG("-----------alloc video line buff:0x%x-----------\n", lcd_s->line_buf);
            }
            if (!lcd_s->line_buf) {
                // 异常处理,申请不到空间只能丢弃获取到的fb,不显示video层
                _os_printf("-----------alloc video line buff failed-----------\n");
                if (video_p0->last_fb) {
                    msi_delete_fb(NULL, video_p0->last_fb);
                    video_p0->last_fb = NULL;
                    p0_fb = NULL;
                }

                if (video_p1->last_fb) {
                    msi_delete_fb(NULL, video_p1->last_fb);
                    video_p1->last_fb = NULL;
                    p1_fb = NULL;
                }
            }
        }

        irq_disable(LCD_IRQn);
        video_p0->cur_fb = p0_fb;
        video_p1->cur_fb = p1_fb;
        osd_show->cur_show_data_s = osd_fb;
        irq_enable(LCD_IRQn);

        if(osd_fb || p0_fb || p1_fb) {
            lcd_s->hardware_ready = 0; // 标记fb已准备好，等待刷新
        }else{
            goto __lcd_msi_work_end;
        }

        if(!count && !lcd_s->hardware_ready){
            count++;
            LCD_MSI_DEBUG("=========first kick lcd=========\n");
            lcd_s->app_lcd_cb(lcd_s);
        }
    }
    else
    {
       // _os_printf("@");
        // 如果没有准备好，休眠 1ms
    }
    
__lcd_msi_work_end:
    os_run_work_delay(&lcd_s->work, 1);

    return 0;
}