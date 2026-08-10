#ifndef __APP_LCD_H
#define __APP_LCD_H
#include "sys_config.h"
#include "typesdef.h"
#include "stream_define.h"


#include "basic_include.h"
#include "lib/multimedia/msi.h"

typedef void (*app_lcd_callback)(void *lcd_s);
typedef void (*already_lcd)();
#define LCD_RB_COUNT 4
#define LCD_DELETE_RB_COUNT 8

typedef void (*display_free_cb)(void *display);
struct display_lcd
{
    struct framebuff *fb;
    display_free_cb free;
};


struct app_lcd_s
{
    struct os_work work;
    struct msi *osd_enc_msi;
    struct msi *lcd_osd_msi;
    struct msi *video_p0_msi;
    struct msi *video_p1_msi;
    struct msi *csc_video_p2_msi;

    //创建多个队列,分别是osd、p0、p1以及csc_p2(将收到的fb放到队列,然后中断去获取队列,这里采用ringbuf的形式)
    RBUFFER_DEF(osd_rb, struct display_lcd *, LCD_RB_COUNT);
    RBUFFER_DEF(p0_rb, struct display_lcd *, LCD_RB_COUNT);
    RBUFFER_DEF(p1_rb, struct display_lcd *, LCD_RB_COUNT);
    RBUFFER_DEF(p2_rb, struct display_lcd *, LCD_RB_COUNT);

    RBUFFER_DEF(delete_rb, struct display_lcd *, LCD_DELETE_RB_COUNT);

    struct display_lcd * p0_fb;
    struct display_lcd * p1_fb;
    struct display_lcd * p2_fb;
    struct display_lcd * osd_fb;    

    app_lcd_callback app_lcd_cb;
    already_lcd      ready_cb;

    struct lcdc_device *lcd_dev;
	struct dsi_device  *lcd_dsi_dev;
    // 只有thread_hdl退出后,才可以休眠
    void *thread_hdl;
    void *line_buf;
    void *free_line_buf;
    uint32_t start_time;
    uint32_t end_time;
    uint32_t last_start_time;
	uint16_t refresh_time;
    uint16_t osd_w, osd_h;
    uint16_t screen_w, screen_h;
	uint16_t video_w, video_h;
    uint16_t x0,y0,x1,y1;
    uint8_t line_buf_num;
    uint8_t osd_rotate;
    uint8_t video_rotate;
    uint8_t hardware_auto_ks : 1, // 由应用层写入,由lcd模块读取
            get_auto_ks      : 1, // 由应用层去读取,由lcd的模块去设置
            hardware_ready   : 1, 
            thread_exit      : 1,
            rekick_lcd       : 1,
            alreay_kick      : 2;
};
extern struct app_lcd_s lcd_msg_s;
void lcd_arg_setting(uint16_t w, uint16_t h, uint8_t rotate, uint16_t screen_w, uint16_t screen_h,uint16_t video_w, uint16_t video_h, uint8_t video_rotate);
void lcd_driver_init(const char *osd_encode_name, const char *lcd_osd_name, const char *lcd_video_p0, const char *lcd_video_p1);
void wait_lcd_exit();
void lcd_driver_suspend();
void lcd_driver_resume();
void lcd_hardware_init(void *lcd_cfg);
int32_t lcd_hardware_display(uint8_t which_video, struct framebuff *fb);
uint8_t g_read_hardware_auto_ks();
uint8_t g_set_hardware_auto_ks(uint8_t en);

void get_osd_w_h(uint16_t *w, uint16_t *h, uint8_t *rotate);
void get_video_w_h(uint16_t *video_w, uint16_t *video_h);
void get_screen_w_h(uint16_t *screen_w, uint16_t *screen_h, uint8_t *video_rotate);

/*****************************osd_encode_msi******************************************/
extern struct msi *osd_encode_msi_init(const char *name);
/*************************************************************************************/

/*****************************lcd_osd_msi******************************************/
struct msi *lcd_osd_msi(const char *name);
struct lcd_osd_msi_s
{
    struct lcdc_device *lcd_dev;
    struct msi *msi;
    struct framebuff *cur_show_data_s;
    struct framebuff *last_show_data_s;
    struct framebuff *delete_show_data_s;
};
/*************************************************************************************/

/*****************************lcd_video_msi******************************************/

struct msi *lcd_video_msi_init(const char *name, uint16_t filter);
struct lcd_video_msi_s
{
    struct msi *msi;
    uint16_t filter;
    struct framebuff *cur_fb;
    struct framebuff *last_fb;
    struct framebuff *delete_fb;
};
/*************************************************************************************/

/*****************************lvgl_osd_msi******************************************/

struct msi *lvgl_osd_msi(const char *name);
struct framebuff *lvgl_osd_msi_get_fb(struct msi *msi);

typedef void (*osd_finish_cb)(void *self);
typedef void (*osd_free_cb)(void *self, void *data);
struct encode_data_s_callback
{
    osd_finish_cb               finish_cb;
    osd_free_cb                 free_cb;
    void *                      user_data;
    uint8_t                     rot_flag : 1,
                                res      : 7;
};
/*************************************************************************************/
void lcd_virtual_msi_init(void);
void lcd_ready_callback(already_lcd cb);
#endif
