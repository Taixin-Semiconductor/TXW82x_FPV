#include "basic_include.h"
#include "hal/lcdc.h"
#include "hal/spi.h"
#include "hal/dsi.h"

#include "dev/lcdc/hglcdc.h"
#include "lib/lcd/lcd.h"
#include "lib/multimedia/msi.h"
#include "stream_frame.h"
#include "app_lcd.h"
#include "user_work/user_work.h"
#ifdef PIN_FROM_PARAM
#include "pin_param.h"
#endif

#ifdef LCD_EN

#define LCD_ROTATE_LINE 64

#define OSD_EN 1
#define LCD_SCREEN_RECORD_EN	   0        //录屏功能

struct app_lcd_s lcd_msg_s;

void lcd_hardware_init565();
void lcd_hardware_init666();
extern void lcd_register_read_3line(struct spi_device *spi_dev, uint32 code, uint8 *buf, uint32 len);
extern void lcd_table_init(struct spi_device *spi_dev, uint8_t *lcd_table);
extern void lcd_table_init_MCU(struct lcdc_device *lcd_dev, uint8_t (*lcd_table)[2]);
extern void lcd_reg_table_init(struct lcdc_device *lcd_dev,uint8 lcd_bus_type,uint8_t *lcd_table);

// 暂时没有作处理
static void lcd_timeout1(uint32 irq_flag, uint32 irq_data, uint32 param1)
{
    os_printf("%s:%d\n", __FUNCTION__, __LINE__);
    struct app_lcd_s *lcd_s = (struct app_lcd_s *)irq_data;
    struct lcdc_device *lcd_dev = lcd_s->lcd_dev;
    lcdc_set_timeout_info(lcd_dev,0,3);

    os_printf("......................................................................"
              "..................................................lcd_timeout\r\n");
    // 出现timeout,说明可能哪里卡住了,让应用层重新启动
    lcd_s->hardware_ready = 1;
    
}

// 暂时没有作处理
static void lcd_te_isr1(uint32 irq_flag, uint32 irq_data, uint32 param1)
{
    os_printf("%s:%d\n", __FUNCTION__, __LINE__);
    struct app_lcd_s *lcd_s = (struct app_lcd_s *)irq_data;
    struct lcdc_device *lcd_dev;
    lcd_dev = lcd_s->lcd_dev;
    os_printf("--------------------------------------------te------------------"
              "---------------\r\n");
}

static void lcd_squralbuf_done1(uint32 irq_flag, uint32 irq_data, uint32 param1)
{
    struct app_lcd_s *lcd_s = (struct app_lcd_s *)irq_data;
    // 理论这里只是唤醒信号量之类,由线程来决定是否继续显示
    // os_printf("%s:%d\n",__FUNCTION__,__LINE__);
#if LCD_SCREEN_RECORD_EN
    static uint32 lcd_num = 0;
    static uint32 dither = 0;
	if((lcd_num%200) == 0){
		os_printf("---------------------------------------------------------\r\n");
		if(dither == 1){
			lcdc_dither_en(lcd_s->lcd_dev,0);
			lcdc_screen_start(lcd_s->lcd_dev,1);
			dither = 0;
		}else{
			lcdc_dither_en(lcd_s->lcd_dev,1);
			lcdc_screen_start(lcd_s->lcd_dev,1);
			dither = 1;
		}
	}
#endif
    lcd_s->end_time = os_jiffies();
	lcd_s->refresh_time = lcd_s->end_time-lcd_s->start_time;
    if (lcd_s->hardware_auto_ks)
    {
        lcd_s->hardware_ready = 0;
        // 判断是否启动了硬件自动kick
        if (!lcd_s->get_auto_ks)
        {
            // 启动自动kick
            lcd_s->get_auto_ks = 1;
            lcdc_video_enable_auto_ks(lcd_s->lcd_dev, 1);
        }
    }
    else
    {
        if (lcd_s->get_auto_ks)
        {
            lcd_s->get_auto_ks = 0;
            lcdc_video_enable_auto_ks(lcd_s->lcd_dev, 0);
        }
        else
        {
            lcd_s->hardware_ready = 1;
        }
    }

    lcd_s->app_lcd_cb(lcd_s);

#if 1
    uint32_t st0,st1;
	struct dsi_device *dsi_dev;
	dsi_dev = lcd_s->lcd_dsi_dev;//(struct dsi_device *)dev_get(HG_DSI_DEVID); 
	st0 = mipi_dsi_get_sta0(dsi_dev);
	st1 = mipi_dsi_get_sta1(dsi_dev);	
	if((st0 != 0)||(st1 != 0)){
		printf("L(%08x	%08x)\r\n",st0,st1);
		mipi_dsi_reset_module(dsi_dev);
	}
#endif
}


#if LCD_SCREEN_RECORD_EN
static void lcd_screen_finish(uint32 irq_flag, uint32 irq_data, uint32 param1){
	os_printf("%s  %d\r\n",__func__,__LINE__);
}
#endif

uint8_t g_read_hardware_auto_ks()
{
    // os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    return lcd_msg_s.get_auto_ks;
}

uint8_t g_set_hardware_auto_ks(uint8_t en)
{
    uint32_t flags;
    uint32_t timeout = 0;
    if (en)
    {
        lcd_msg_s.hardware_auto_ks = en;
        while (!lcd_msg_s.get_auto_ks && (timeout < 100))
        {
            timeout++;
            os_sleep_ms(1);
        }
    }
    else
    {
        flags = disable_irq();
        lcd_msg_s.hardware_auto_ks = en;
        enable_irq(flags);
    }

    return lcd_msg_s.get_auto_ks;
}

// 返回的是osd旋转和w h
static void get_osd_w_h(uint16_t *w, uint16_t *h, uint8_t *rotate)
{
    *w = lcdstruct.osd_w;
    *h = lcdstruct.osd_h;
    *rotate = lcdstruct.osd_scan_mode;
}

static void get_video_w_h(uint16_t *video_w,uint16_t *video_h){
    *video_w = lcdstruct.video_w;
    *video_h = lcdstruct.video_h;	
}

// 返回屏幕的size和旋转角度
static void get_screen_w_h(uint16_t *screen_w, uint16_t *screen_h, uint8_t *video_rotate)
{
    *screen_w = lcdstruct.screen_w;
    *screen_h = lcdstruct.screen_h;
    *video_rotate = lcdstruct.scan_mode;
}

// 注意,这部分不同屏以及硬件设计不一样,需要修改
void lcd_hardware_init(uint16_t *w, uint16_t *h, uint8_t *rotate, uint16_t *screen_w, uint16_t *screen_h, uint16_t *video_w, uint16_t *video_h, uint8_t *video_rotate)
{
    if (lcdstruct.lcd_bus_type == LCD_BUS_I80)
    {
        if (lcdstruct.color_mode == LCD_MODE_666)
        {
            lcd_hardware_init666();
        }
        else if (lcdstruct.color_mode == LCD_MODE_565)
        {
            lcd_hardware_init565();
        }
        else
        {
            lcd_hardware_init565();
        }
    }
    else if ((lcdstruct.lcd_bus_type == LCD_BUS_RGB)||(lcdstruct.lcd_bus_type == LCD_BUS_SPI4))
    {
        lcd_hardware_init565();
    }
    else if (lcdstruct.lcd_bus_type == LCD_BUS_MIPI)
    {
        lcd_hardware_init565();
    }

    // 返回osd的w和h
    get_osd_w_h(w, h, rotate);
    get_screen_w_h(screen_w, screen_h, video_rotate);
	get_video_w_h(video_w,video_h);
}

// 主要硬件初始化,但由于存在rgb屏,还是会有部分寄存器需要配置
void lcd_hardware_init565()
{
    uint8 pixel_dot_num = 1;
    struct lcdc_device *lcd_dev;
    struct spi_device *spi_dev;
//	uint32_t rbuf[4];
//	uint8_t *pt8;
    lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);
    spi_dev = (struct spi_device *)dev_get(HG_SPI0_DEVID);

    uint8_t rst = MACRO_PIN(PIN_LCD_RESET);

    if (rst != 255)
    {
        gpio_iomap_output(rst, GPIO_IOMAP_OUTPUT);
        gpio_set_val(rst, 0);
        os_sleep_ms(100);
        gpio_set_val(rst, 1);
        os_sleep_ms(100);
    }

    if (lcdstruct.lcd_bus_type == LCD_BUS_RGB)
    {

        uint8 spi_buf[10];
        if (lcdstruct.init_table != NULL)
        {
            spi_open(spi_dev, 1000000, SPI_MASTER_MODE, SPI_WIRE_SINGLE_MODE, SPI_CPOL_1_CPHA_1);
            spi_ioctl(spi_dev, SPI_SET_FRAME_SIZE, 9, 0);
            // lcd spi_cfg
            _os_printf("read lcd id(%x)\r\n", (unsigned int)spi_dev);
            lcd_register_read_3line(spi_dev, 0xda, spi_buf, 4);
            _os_printf("***ID:%02x %02x %02x %02x\r\n", spi_buf[0], spi_buf[1], spi_buf[2], spi_buf[3]);

            lcd_table_init(spi_dev, (uint8_t *)lcdstruct.init_table);
            gpio_set_val(PIN_SPI0_CS, 1);
            gpio_iomap_output(PIN_SPI0_CS, GPIO_IOMAP_OUTPUT);
        }
    }

    // scale_to_lcd_config();
    lcdc_init(lcd_dev);
    if ((lcdstruct.lcd_bus_type == LCD_BUS_I80)||(lcdstruct.lcd_bus_type == LCD_BUS_SPI4))
    {
        lcdc_open(lcd_dev);
    }
    lcdc_set_color_mode(lcd_dev, lcdstruct.color_mode);
    lcdc_set_bus_width(lcd_dev, lcdstruct.bus_width);
    lcdc_set_interface(lcd_dev, lcdstruct.lcd_bus_type);
    lcdc_set_colrarray(lcd_dev, lcdstruct.colrarray);
    pixel_dot_num = lcdstruct.color_mode / lcdstruct.bus_width;
    lcdc_set_lcd_vaild_size(lcd_dev, lcdstruct.screen_w + lcdstruct.hlw + lcdstruct.hbp + lcdstruct.hfp, lcdstruct.screen_h + lcdstruct.vlw + lcdstruct.vbp + lcdstruct.vfp, pixel_dot_num);
    lcdc_set_lcd_visible_size(lcd_dev, lcdstruct.screen_w, lcdstruct.screen_h, pixel_dot_num);
    if (lcdstruct.lcd_bus_type == LCD_BUS_I80)
    {
        lcdc_mcu_signal_config(lcd_dev, lcdstruct.signal_config.value);
    }
    else if ((lcdstruct.lcd_bus_type == LCD_BUS_RGB) || (lcdstruct.lcd_bus_type == LCD_BUS_MIPI) || (lcdstruct.lcd_bus_type == LCD_BUS_QSPI))
    {
        lcdc_signal_config(lcd_dev, lcdstruct.vs_en, lcdstruct.hs_en, lcdstruct.de_en, lcdstruct.vs_inv, lcdstruct.hs_inv, lcdstruct.de_inv, lcdstruct.pclk_inv);
        lcdc_set_invalid_line(lcd_dev, lcdstruct.vlw);
        lcdc_set_valid_dot(lcd_dev, lcdstruct.hlw + lcdstruct.hbp, lcdstruct.vlw + lcdstruct.vbp);
        lcdc_set_hlw_vlw(lcd_dev, lcdstruct.hlw, 0);
    }
    lcdc_set_baudrate(lcd_dev, lcdstruct.pclk);
    lcdc_set_bigendian(lcd_dev, 1);
    if (lcdstruct.lcd_bus_type == LCD_BUS_I80)
    {
        lcd_table_init_MCU(lcd_dev, lcdstruct.init_table);
    }
    else if (lcdstruct.lcd_bus_type == LCD_BUS_MIPI)
    {
        mipi_dsi_init(lcdstruct.screen_w, lcdstruct.screen_h, lcdstruct.pclk, lcdstruct.vlw, lcdstruct.vbp, lcdstruct.vfp, lcdstruct.hlw, lcdstruct.hbp, lcdstruct.hfp, lcdstruct.lane_num, lcdstruct.color_mode);
    }else if (lcdstruct.lcd_bus_type == LCD_BUS_SPI4){
		lcdc_spi_cs_lock_time(lcd_dev,0x20,0x20,0x20);
		lcdc_cmd_auto_send(lcd_dev,0,0);

//		pt8 = (uint8_t *)rbuf;
//		pt8[0] = 1;
//		pt8[1] = 4;
//		pt8[2] = 0x04;
//		lcdc_reg_read_data(lcd_dev,1,4,rbuf);	
//		os_printf("ID:%02x %02x %02x %02x %02x %02x %02x\r\n",pt8[0],pt8[1],pt8[2],pt8[3],pt8[4],pt8[5],pt8[6]);
		lcd_reg_table_init(lcd_dev,lcdstruct.lcd_bus_type,(uint8_t *)(lcdstruct.init_table));
	}
}

void lcd_hardware_init666()
{
    uint8 pixel_dot_num = 1;
    struct lcdc_device *lcd_dev;
    struct spi_device *spi_dev;

    lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);
    spi_dev = (struct spi_device *)dev_get(HG_SPI0_DEVID);

    uint8_t rst = MACRO_PIN(PIN_LCD_RESET);

    if (rst != 255)
    {
        gpio_iomap_output(rst, GPIO_IOMAP_OUTPUT);
        gpio_set_val(rst, 0);
        os_sleep_ms(100);
        gpio_set_val(rst, 1);
        os_sleep_ms(100);
    }
    if (lcdstruct.lcd_bus_type == LCD_BUS_RGB)
    {

        uint8 spi_buf[10];
        if (lcdstruct.init_table != NULL)
        {
            spi_open(spi_dev, 1000000, SPI_MASTER_MODE, SPI_WIRE_SINGLE_MODE, SPI_CPOL_1_CPHA_1);
            spi_ioctl(spi_dev, SPI_SET_FRAME_SIZE, 9, 0);
            // lcd spi_cfg
            _os_printf("read lcd id(%x)\r\n", (unsigned int)spi_dev);
            lcd_register_read_3line(spi_dev, 0xda, spi_buf, 4);
            _os_printf("***ID:%02x %02x %02x %02x\r\n", spi_buf[0], spi_buf[1], spi_buf[2], spi_buf[3]);

            lcd_table_init(spi_dev, (uint8_t *)lcdstruct.init_table);
            gpio_set_val(PIN_SPI0_CS, 1);
            gpio_iomap_output(PIN_SPI0_CS, GPIO_IOMAP_OUTPUT);
        }
    }

    // scale_to_lcd_config();
    lcdc_init(lcd_dev);
    if (lcdstruct.lcd_bus_type == LCD_BUS_I80)
    {
        lcdc_open(lcd_dev);
    }
    lcdc_set_color_mode(lcd_dev, lcdstruct.color_mode);
    lcdc_set_bus_width(lcd_dev, lcdstruct.bus_width);
    lcdc_set_interface(lcd_dev, lcdstruct.lcd_bus_type);
    lcdc_set_colrarray(lcd_dev, lcdstruct.colrarray);
    pixel_dot_num = lcdstruct.color_mode / lcdstruct.bus_width;
    lcdc_set_lcd_vaild_size(lcd_dev, lcdstruct.screen_w + lcdstruct.hlw + lcdstruct.hbp + lcdstruct.hfp, lcdstruct.screen_h + lcdstruct.vlw + lcdstruct.vbp + lcdstruct.vfp, pixel_dot_num);
    lcdc_set_lcd_visible_size(lcd_dev, lcdstruct.screen_w, lcdstruct.screen_h, 3);
    if (lcdstruct.lcd_bus_type == LCD_BUS_I80)
    {
        lcdc_mcu_signal_config(lcd_dev, lcdstruct.signal_config.value);
    }
    else if (lcdstruct.lcd_bus_type == LCD_BUS_RGB)
    {
        lcdc_signal_config(lcd_dev, lcdstruct.vs_en, lcdstruct.hs_en, lcdstruct.de_en, lcdstruct.vs_inv, lcdstruct.hs_inv, lcdstruct.de_inv, lcdstruct.pclk_inv);
        lcdc_set_invalid_line(lcd_dev, lcdstruct.vlw);
        lcdc_set_valid_dot(lcd_dev, lcdstruct.hlw + lcdstruct.hbp, lcdstruct.vlw + lcdstruct.vbp);
        lcdc_set_hlw_vlw(lcd_dev, lcdstruct.hlw, 0);
    }
    lcdc_set_baudrate(lcd_dev, lcdstruct.pclk);
    lcdc_set_bigendian(lcd_dev, 1);
    if (lcdstruct.lcd_bus_type == LCD_BUS_I80)
    {
        lcd_table_init_MCU(lcd_dev, lcdstruct.init_table);
    }

    // gamma表之类没有配置,这个考虑是放在其他地方,这个函数专注硬件的初始化好了

#if 0 
	lcdc_video_enable_gamma(lcd_dev, 1);
	lcdc_video_enable_ccm(lcd_dev, 1);
	lcdc_video_enable_constarast(lcd_dev, 1);
	lcdc_video_set_CCM_COEF0(lcd_dev, 0x00df0100);  
	lcdc_video_set_CCM_COEF1(lcd_dev, 0x0003b3f3);    
	lcdc_video_set_CCM_COEF2(lcd_dev, 0x0df0000d);        
	lcdc_video_set_CCM_COEF3(lcd_dev, 0x00000000); 
	lcdc_video_set_constarast_val(lcd_dev, 0x12);
	lcdc_video_set_gamma_R(lcd_dev, (uint32_t)rgb_gamma_table);
	lcdc_video_set_gamma_G(lcd_dev, (uint32_t)rgb_gamma_table);
	lcdc_video_set_gamma_B(lcd_dev, (uint32_t)rgb_gamma_table);
#endif

    lcdc_set_bus_width(lcd_dev, LCD_BUS_WIDTH_6);
}

// lcd相关参数配置,w、h、rotate
void lcd_arg_setting(uint16_t w, uint16_t h, uint8_t rotate, uint16_t screen_w, uint16_t screen_h, uint16_t video_w, uint16_t video_h,uint8_t video_rotate)
{
    struct app_lcd_s *lcd_s = &lcd_msg_s;
    lcd_s->w = w;
    lcd_s->h = h;
    lcd_s->rotate = rotate;
    lcd_s->screen_w = screen_w;
    lcd_s->screen_h = screen_h;
    lcd_s->video_w = video_w;
    lcd_s->video_h = video_h;	
    lcd_s->video_rotate = video_rotate;
}

// 芯片lcd的驱动初始化
void lcd_driver_init(const char *osd_encode_name, const char *lcd_osd_name, const char *lcd_video_p0, const char *lcd_video_p1)
{
    struct app_lcd_s *lcd_s = &lcd_msg_s;
	struct dsi_device *dsi_dev;
	dsi_dev = (struct dsi_device *)dev_get(HG_DSI_DEVID); 
	dev_put((struct dev_obj *)dsi_dev);
    lcd_s->osd_enc_msi = osd_encode_msi_init(osd_encode_name);
#if 1
    lcd_s->lcd_osd_msi = lcd_osd_msi(lcd_osd_name);
    lcd_s->video_p0_msi = lcd_video_msi_init(lcd_video_p0, FSTYPE_YUV_P0);
    lcd_s->video_p1_msi = lcd_video_msi_init(lcd_video_p1, FSTYPE_YUV_P1);
#endif

    lcd_s->hardware_ready = 1;
    lcd_s->thread_exit = 0;
    lcd_s->hardware_auto_ks = 0;
    lcd_s->get_auto_ks = 0;

    struct lcdc_device *lcd_dev;

    lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);
	dev_put((struct dev_obj *)lcd_dev);
    lcd_s->lcd_dev = lcd_dev;
	lcd_s->lcd_dsi_dev = dsi_dev;

    lcdc_set_video_size(lcd_dev, lcdstruct.video_w, lcdstruct.video_h);

    lcdc_set_rotate_p0_up(lcd_dev, 0);
    lcdc_set_rotate_p0p1_start_location(lcd_dev, 0, 0, 0, 0);

    lcdc_set_rotate_linebuf_num(lcd_dev, LCD_ROTATE_LINE);
    lcd_s->line_buf_num = LCD_ROTATE_LINE;
    lcdc_set_video_data_from(lcd_dev, VIDEO_FROM_MEMORY_ROTATE);

    lcdc_set_video_start_location(lcd_dev, lcdstruct.video_x, lcdstruct.video_y);
    lcdc_set_video_en(lcd_dev, 0);

    // 设置默认值,因为这个一定要配置,并且地址与p1的地址范围要一样(就是全部是psram或者全部是sram,所以这里最好都是设置psram)
    lcdc_set_p0_rotate_y_src_addr(lcd_dev, (uint32)0x28000000);
    lcdc_set_p0_rotate_u_src_addr(lcd_dev, (uint32)0x28000000);
    lcdc_set_p0_rotate_v_src_addr(lcd_dev, (uint32)0x28000000);

    lcdc_set_osd_start_location(lcd_dev, lcdstruct.osd_x, lcdstruct.osd_y);
    lcdc_set_osd_size(lcd_dev, lcdstruct.osd_w, lcdstruct.osd_h);
    lcdc_set_osd_format(lcd_dev, OSD_RGB_565);
    // 仅仅测试新框架
    // lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd565_encode_test);

    lcdc_set_osd_alpha(lcd_dev, 0x100);
    lcdc_set_osd_enc_cfg(lcd_dev, 0xFFFFFF, 0x000000);
    lcdc_osd_spec_color_alpha(lcd_dev, 0, 0xffffff, 0x00);
    lcdc_osd_enc_en(lcd_dev, 1);
    lcdc_set_osd_en(lcd_dev, 0);
    lcdc_osd_trans_en(lcd_dev, 1);

    lcdc_video_enable_auto_ks(lcd_dev, 0);
    lcdc_set_timeout_info(lcd_dev, 1, 3);

    lcdc_request_irq(lcd_dev, LCD_DONE_IRQ, (lcdc_irq_hdl)&lcd_squralbuf_done1, (uint32)lcd_s);
#if LCD_SCREEN_RECORD_EN
    uint8_t *video_psram_screen = (uint8_t *)av_psram_zalloc(SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2);
    os_printf("LCD_SCREEN_RECORD_EN psram addr:%x\n",video_psram_screen);
    lcdc_screen_yuv_addr(lcd_dev,video_psram_screen,video_psram_screen+SCALE_CONFIG_W*SCALE_HIGH,video_psram_screen+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
    lcdc_request_irq(lcd_dev,SCREEN_DONE_IRQ,(lcdc_irq_hdl )&lcd_screen_finish,(uint32)lcd_s);
#endif
    // lcdc_request_irq(lcd_dev, OSD_EN_IRQ, (lcdc_irq_hdl)&lcd_osd_isr1_msi, (uint32)lcd_s);
    lcdc_request_irq(lcd_dev, TIMEOUT_IRQ, (lcdc_irq_hdl)&lcd_timeout1, (uint32)lcd_s);
#if (LCD_TE != 255)
    lcdc_set_te_edge(lcd_dev, 1);
    lcdc_request_irq(lcd_dev, LCD_TE_IRQ, (lcdc_irq_hdl)&lcd_te_isr1, (uint32)lcd_s);
#endif
    lcdc_open(lcd_dev);

    #if 0
    //extern void lcd_thread(void *d);
    //lcd_s->thread_hdl = os_task_create("lcd_thread", lcd_thread2, lcd_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
    #else
    extern int32 lcd_msi_work(struct os_work *work);
    extern void lcd_msi_irq_callback(void *data);
    lcd_s->app_lcd_cb = lcd_msi_irq_callback;
    OS_WORK_INIT(&lcd_s->work, lcd_msi_work, 0);
    os_run_work_delay(&lcd_s->work, 1);
    #endif
}

void wait_lcd_exit()
{
    struct app_lcd_s *lcd_s = &lcd_msg_s;
    lcd_s->thread_exit = 1;
    while (lcd_s->thread_hdl)
    {
        os_sleep_ms(1);
    }
}

// 重复初始化,一般用于意外断电或者说休眠起来后使用
void lcd_driver_reinit()
{
#if 0
    struct app_lcd_s *lcd_s = &lcd_msg_s;
    lcd_s->hardware_ready = 1;
    lcd_s->thread_exit = 0;
    lcd_s->hardware_auto_ks = 0;
    lcd_s->get_auto_ks = 0;
    struct lcdc_device *lcd_dev;

    lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);

    lcd_s->lcd_dev = lcd_dev;

    lcdc_set_video_size(lcd_dev, lcdstruct.video_w, lcdstruct.video_h);

    // VIDEO
    lcdc_set_rotate_p0_up(lcd_dev, 0);
    lcdc_set_rotate_p0p1_start_location(lcd_dev, 0, 0, 0, 0);
    // line_buf后续再分配
    lcdc_set_rotate_linebuf_num(lcd_dev, LCD_ROTATE_LINE);
    lcdc_set_video_data_from(lcd_dev, VIDEO_FROM_MEMORY_ROTATE);
    lcdc_set_video_start_location(lcd_dev, lcdstruct.video_x, lcdstruct.video_y);
    lcdc_set_video_en(lcd_dev, 0);

    // 设置默认值,因为这个一定要配置,并且地址与p1的地址范围要一样(就是全部是psram或者全部是sram,所以这里最好都是设置psram)
    lcdc_set_p0_rotate_y_src_addr(lcd_dev, (uint32)0x38000000);
    lcdc_set_p0_rotate_u_src_addr(lcd_dev, (uint32)0x38000000);
    lcdc_set_p0_rotate_v_src_addr(lcd_dev, (uint32)0x38000000);

    // OSD
    lcdc_set_osd_start_location(lcd_dev, lcdstruct.osd_x, lcdstruct.osd_y);
    lcdc_set_osd_size(lcd_dev, lcdstruct.osd_w, lcdstruct.osd_h);
    lcdc_set_osd_format(lcd_dev, OSD_RGB_565);
    lcdc_set_osd_alpha(lcd_dev, 0x100);
    lcdc_set_osd_enc_head(lcd_dev, 0xFFFFFF, 0xFFFBFF);
    lcdc_set_osd_enc_diap(lcd_dev, 0x000000, 0x000000);
    lcdc_set_osd_en(lcd_dev, 0);

    lcdc_video_enable_auto_ks(lcd_dev, 0);
    lcdc_set_timeout_info(lcd_dev, 1, 3);

    lcdc_request_irq(lcd_dev, LCD_DONE_IRQ, (lcdc_irq_hdl)&lcd_squralbuf_done1, (uint32)lcd_s);

    lcdc_request_irq(lcd_dev, OSD_EN_IRQ, (lcdc_irq_hdl)&lcd_osd_isr1_msi, (uint32)lcd_s);
    lcdc_request_irq(lcd_dev, TIMEOUT_IRQ, (lcdc_irq_hdl)&lcd_timeout1, (uint32)lcd_s);
#if (LCD_TE != 255)
    lcdc_set_te_edge(lcd_dev, 1);
    lcdc_request_irq(lcd_dev, LCD_TE_IRQ, (lcdc_irq_hdl)&lcd_te_isr1, (uint32)lcd_s);
#endif
    lcdc_open(lcd_dev);
    // lcdc_set_start_run(lcd_dev);

    extern void lcd_thread(void *d);
    lcd_s->thread_hdl = os_task_create("lcd_thread", lcd_thread, (void *)lcd_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
#endif
}

#if 0

void lcd_module_run2(){
	uint32_t rbuf[4];
	uint8_t *pt8;
	uint8 pixel_dot_num = 1;
	uint32_t *osd_sram_fifo;
	struct lcdc_device *lcd_dev;	
	struct spi_device *spi_dev;
	struct scale_device *scale_dev;
	struct vpp_device *vpp_dev;
	struct jpg_device *jpg_dev;
	jpg_dev = (struct jpg_device *)dev_get(HG_JPG1_DEVID);		
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);	
	scale_dev = (struct scale_device *)dev_get(HG_SCALE2_DEVID);	
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
	spi_dev = (struct spi_device * )dev_get(HG_SPI0_DEVID);		

	lcdc_set_video_size(lcd_dev,lcdstruct.video_w,lcdstruct.video_h);



	lcdc_set_osd_start_location(lcd_dev,lcdstruct.osd_x,lcdstruct.osd_y);

	lcdc_set_osd_size(lcd_dev,lcdstruct.osd_w,lcdstruct.osd_h);
	lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd565_encode_test);   
	lcdc_set_osd_format(lcd_dev,OSD_RGB_565);

	lcdc_set_osd_alpha(lcd_dev,0x100);
	lcdc_set_osd_enc_cfg(lcd_dev,0xFFFFFF,0x000000);
	lcdc_osd_spec_color_alpha(lcd_dev,0,0xffffff,0x00);
	lcdc_osd_enc_en(lcd_dev,1);
	lcdc_set_osd_en(lcd_dev,1);
	lcdc_osd_trans_en(lcd_dev,1);


	if((lcdstruct.lcd_bus_type == LCD_BUS_RGB)||(lcdstruct.lcd_bus_type == LCD_BUS_MIPI)||(lcdstruct.lcd_bus_type == LCD_BUS_QSPI)){
		lcdc_clock_alway_on(lcd_dev,1);
	}
	lcdc_rot_burst_dw16(lcd_dev,0);
	
	lcdc_video_enable_auto_ks(lcd_dev,0);
	lcdc_set_timeout_info(lcd_dev,1,3);

	lcdc_request_irq(lcd_dev,LCD_DONE_IRQ,(lcdc_irq_hdl )&lcd_squralbuf_done1,(uint32)lcd_dev);	


	lcdc_open(lcd_dev);
	os_sleep_ms(10);
	lcdc_set_start_run(lcd_dev);
	
}
#endif

#endif
