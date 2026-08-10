#include "lib/lcd/lcd.h"

#if LCD_ST7735_EN
#define CMD(x)    {LCD_CMD,x}
#define DAT(x)    {LCD_DAT,x}
#define DLY(x)    {DELAY_MS,x}
#define END		{LCD_TAB_END,LCD_TAB_END}

uint8_t st7735_firt_send[][2]={
	CMD( 0x2A),
	DAT( 0x00),
	DAT( 0x00),
	DAT( 0x00),
	//DAT( 0x7F),	
	DAT( 0x83),

	CMD( 0x2B),
	DAT( 0x00),
	DAT( 0x00),
	DAT( 0x00),
	//DAT( 0x9F),	
	DAT( 0xA1),	
//	CMD( 0x2C),
	END
};


uint8_t st7735_register_init_tab[][2] = {
	CMD( 0x11),
	DLY( 120 ),
	CMD( 0xB1),
	DAT( 0x05),
	DAT( 0x3C),
	DAT( 0x3C),

	CMD( 0xB2),
	DAT( 0x05),
	DAT( 0x3C),
	DAT( 0x3C),

	CMD( 0xB3),
	DAT( 0x05),
	DAT( 0x3C),
	DAT( 0x3C),
	DAT( 0x05),
	DAT( 0x3C),
	DAT( 0x3C),

	CMD( 0xB4),
	DAT( 0x03),	

	CMD( 0xC0),
	DAT( 0x28),
	DAT( 0x08),
	DAT( 0x04),

	CMD( 0xC1),
	DAT( 0xC0),	

	CMD( 0xC2),
	DAT( 0x0D),
	DAT( 0x00),

	CMD( 0xC3),
	DAT( 0x8D),
	DAT( 0x2A),

	CMD( 0xC4),
	DAT( 0x8D),
	DAT( 0xEE),

	CMD( 0xC5),
	DAT( 0x1A),	

	CMD( 0x36),
	DAT( 0xC0),

	CMD( 0xE0),
	DAT( 0x04),
	DAT( 0x22),
	DAT( 0x07),
	DAT( 0x0A),
	DAT( 0x2E),
	DAT( 0x30),
	DAT( 0x25),
	DAT( 0x2A),
	DAT( 0x28),
	DAT( 0x26),
	DAT( 0x2E),
	DAT( 0x3A),
	DAT( 0x00),
	DAT( 0x01),
	DAT( 0x03),
	DAT( 0x13),

	CMD( 0xE1),
	DAT( 0x04),
	DAT( 0x16),
	DAT( 0x06),
	DAT( 0x0D),
	DAT( 0x2D),
	DAT( 0x26),
	DAT( 0x23),
	DAT( 0x27),
	DAT( 0x27),
	DAT( 0x25),
	DAT( 0x2D),
	DAT( 0x3B),
	DAT( 0x00),
	DAT( 0x01),
	DAT( 0x04),
	DAT( 0x13),

	CMD( 0x3A),
	DAT( 0x05),	

	CMD( 0x29),

	CMD( 0x2A),
	DAT( 0x00),
	DAT( 0x00),
	DAT( 0x00),
	//DAT( 0x7F),
	DAT( 0x83),

	CMD( 0x2B),
	DAT( 0x00),
	DAT( 0x00),
	DAT( 0x00),
	//DAT( 0x9F),	
	DAT( 0xA1),	
	CMD( 0x2C),
	
    END
};





lcddev_t  lcdstruct = {
    .name = "st7735",
    .lcd_bus_type = LCD_BUS_SPI4,
    .bus_width = LCD_BUS_WIDTH_1,
    .color_mode = LCD_MODE_565,
    .osd_scan_mode = LCD_ROTATE_90,
    .scan_mode = LCD_ROTATE_90,//rotate 90
    .te_mode = 0xff,//te mode, 0xff:disable
    .colrarray = 0,//0:_RGB_ 1:_RBG_,2:_GBR_,3:_GRB_,4:_BRG_,5:_BGR_
    //f(wr) = source_clk/div/2
    //f(wr) >= screen_w * screen_h * clk_per_pixel * 60
    .pclk = 24000000,
    .even_order = 0,
    .odd_order = 0,
    .lcd_data_mode = (0<<31)|//data inversion mode
                     (2<<24)|//data compress mode
                     (1<<20)|//fifo mode
                     (0<<17)|//output cycle 2 shift direction
                     (0<<12)|//output cycle 2 shift bit
                     (0<<11)|//output cycle 1 shift direction
                     (0<<6)| //output cycle 1 shift bit
                     (0<<5)| //output cycle 0 shift direction
                     (8<<0), //output cycle 0 shift bit
    .screen_w = 132,
    .screen_h = 162,
    .video_x  = 0,
    .video_y  = 0,
    .video_w  = 132,
    .video_h  = 162,
	.osd_x = 0,
	.osd_y = 0,
	.osd_w = 132, // 0 : value will set to video_w  , use for 4:3 LCD +16:9 sensor show UPDOWN BLACK
	.osd_h = 162, // 0 : value will set to video_h  , use for 4:3 LCD +16:9 sensor show UPDOWN BLACK
	.init_table  = st7735_register_init_tab,
	.frame_table = st7735_firt_send,
    .clk_per_pixel = 2,

    .pclk_inv = 1,
    
    
    .vlw = 0,// 0,
    .vbp = 0,// 12,
    .vfp = 0,// 12,
    .hlw = 0,// 1,
    .hbp = 0,//14,
    .hfp = 0,//13,


	.de_en  = 1,
	.vs_en	= 1,
	.hs_en	= 1,
    .de_inv = 0xff,
    .hs_inv = 0,
    .vs_inv = 0,
    
    .brightness = 1,
    .saturation = 7,
    .contrast   = 7,
    .contra_index = 8,

    .gamma_red = 3,
    .gamma_green=3,
    .gamma_blue=3,

};
#endif
