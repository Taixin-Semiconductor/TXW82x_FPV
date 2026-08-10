#include "lib/lcd/lcd.h"

#if LCD_ST7789_SPI_EN

#define CMD(x) {LCD_CMD, x}
#define DAT(x) {LCD_DAT, x}
#define DLY(x) {DELAY_MS, x}
#define END {LCD_TAB_END, LCD_TAB_END}

uint8_t st7789_register_init_tab[][2] = {

	// CTC2.8 + ST7789 init sequence
	CMD(0x11),
	DLY(0x82),
	//-------------------------------display and color format setting-----------------------------//
	CMD(0x21),
	CMD(0x36),
	DAT(0x60),

	CMD(0x3A),
	DAT(0x05), // 16bit/pixel
			   //--------------------------------ST7789S Frame rate setting----------------------------------//
	CMD(0xB2),
	DAT(0x0C),
	DAT(0x0C),
	DAT(0x00),
	DAT(0x33),
	DAT(0x33),

	CMD(0xB7),
	DAT(0x35),
	//---------------------------------ST7789S Power setting--------------------------------------//
	CMD(0xBB),
	DAT(0x1F),

	CMD(0xC0),
	DAT(0x2C),

	CMD(0xC2),
	DAT(0x01),

	CMD(0xC3),
	DAT(0x11),

	CMD(0xC4),
	DAT(0x20),

	CMD(0xC6),
	DAT(0x0F),

	CMD(0xD0),
	DAT(0xA4),
	DAT(0xA1),

	//--------------------------------ST7789S gamma setting---------------------------------------//
	CMD(0xE0),
	DAT(0xD0),
	DAT(0x00),
	DAT(0x14),
	DAT(0x15),
	DAT(0x13),
	DAT(0x2C),
	DAT(0x42),
	DAT(0x43),
	DAT(0x4E),
	DAT(0x09),
	DAT(0x16),
	DAT(0x14),
	DAT(0x18),
	DAT(0x21),

	CMD(0xE1),
	DAT(0xD0),
	DAT(0x00),
	DAT(0x14),
	DAT(0x15),
	DAT(0x13),
	DAT(0x0B),
	DAT(0x43),
	DAT(0x55),
	DAT(0x53),
	DAT(0x0C),
	DAT(0x17),
	DAT(0x14),
	DAT(0x23),
	DAT(0x20),

    // 设置列地址 0~319（宽）
    CMD(0x2A),
    DAT(0x00), DAT(0x00), DAT(0x01), DAT(0x3F),   // 0x013F = 319
    
    // 设置行地址 0~239（高）
    CMD(0x2B),
    DAT(0x00), DAT(0x00), DAT(0x00), DAT(0xEF),   // 0x00EF = 239

	CMD(0x29),
	CMD(0x2C),
	DLY(0x32),
	END};

lcddev_t lcdstruct = {
	.name = "st7789",
	.lcd_bus_type = LCD_BUS_SPI4,
	.bus_width = LCD_BUS_WIDTH_1,
	.color_mode = LCD_MODE_565,
	.osd_scan_mode = LCD_ROTATE_0,
	.scan_mode = LCD_ROTATE_0, // rotate 90
	.te_mode = 0xff,		   // te mode, 0xff:disable
	.colrarray = 0,			   // 0:_RGB_ 1:_RBG_,2:_GBR_,3:_GRB_,4:_BRG_,5:_BGR_
	// f(wr) = source_clk/div/2
	// f(wr) >= screen_w * screen_h * clk_per_pixel * 60
	.pclk = 45000000,
	.even_order = 0,
	.odd_order = 0,
	.lcd_data_mode = (0 << 31) | // data inversion mode
					 (2 << 24) | // data compress mode
					 (1 << 20) | // fifo mode
					 (0 << 17) | // output cycle 2 shift direction
					 (0 << 12) | // output cycle 2 shift bit
					 (0 << 11) | // output cycle 1 shift direction
					 (0 << 6) |	 // output cycle 1 shift bit
					 (0 << 5) |	 // output cycle 0 shift direction
					 (8 << 0),	 // output cycle 0 shift bit
    .even_order = 0,
    .odd_order = 0,
    .screen_w = 320,
    .screen_h = 240,
    .video_x  = 0,
    .video_y  = 0,
    .video_w  = 320,
    .video_h  = 240,
	.osd_x = 0,
	.osd_y = 0,
	.osd_w = 320, // 0 : value will set to video_w  , use for 4:3 LCD +16:9 sensor show UPDOWN BLACK
	.osd_h = 240, // 0 : value will set to video_h  , use for 4:3 LCD +16:9 sensor show UPDOWN BLACK
	.init_table = st7789_register_init_tab,
	.clk_per_pixel = 2,

	.pclk_inv = 1,

	.vlw = 0, // 0,
	.vbp = 0, // 12,
	.vfp = 0, // 12,
	.hlw = 0, // 1,
	.hbp = 0, // 14,
	.hfp = 0, // 13,

	.de_en = 1,
	.vs_en = 1,
	.hs_en = 1,
	.de_inv = 0xff,
	.hs_inv = 0,
	.vs_inv = 0,

	.brightness = 1,
	.saturation = 7,
	.contrast = 7,
	.contra_index = 8,

	.gamma_red = 3,
	.gamma_green = 3,
	.gamma_blue = 3,

};

#endif
