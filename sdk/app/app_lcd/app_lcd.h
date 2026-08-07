#ifndef __APP_LCD_H
#define __APP_LCD_H
#include "sys_config.h"
#include "typesdef.h"
#include "stream_frame.h"


#include "basic_include.h"
#include "lib/multimedia/msi.h"

typedef void (*app_lcd_callback)(void *lcd_s);

struct app_lcd_s
{
    struct os_work work;
    struct msi *osd_enc_msi;
    struct msi *lcd_osd_msi;
    struct msi *video_p0_msi;
    struct msi *video_p1_msi;

    struct framebuff * p0_fb;
    struct framebuff * p1_fb;
    struct framebuff * osd_fb;

    app_lcd_callback app_lcd_cb;

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
    uint16_t w, h;
    uint16_t screen_w, screen_h;
	uint16_t video_w, video_h;
    uint16_t x0,y0,x1,y1;
    uint8_t line_buf_num;
    uint8_t rotate;
    uint8_t video_rotate;
    uint8_t hardware_auto_ks : 1, // 由应用层写入,由lcd模块读取
        get_auto_ks : 1,          // 由应用层去读取,由lcd的模块去设置
        hardware_ready : 1, thread_exit : 1;
};
extern struct app_lcd_s lcd_msg_s;

void lcd_arg_setting(uint16_t w, uint16_t h, uint8_t rotate, uint16_t screen_w, uint16_t screen_h,uint16_t video_w, uint16_t video_h, uint8_t video_rotate);
void lcd_driver_init(const char *osd_encode_name, const char *lcd_osd_name, const char *lcd_video_p0, const char *lcd_video_p1);
void wait_lcd_exit();
void lcd_driver_reinit();
void lcd_hardware_init(uint16_t *w, uint16_t *h, uint8_t *rotate, uint16_t *screen_w, uint16_t *screen_h,uint16_t *video_w, uint16_t *video_h, uint8_t *video_rotate);
uint8_t g_read_hardware_auto_ks();
uint8_t g_set_hardware_auto_ks(uint8_t en);

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
    osd_finish_cb finish_cb;
    osd_free_cb free_cb;
    void *user_data;
};
/*************************************************************************************/
#endif
