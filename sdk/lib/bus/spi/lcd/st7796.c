#include "lib/lcd/lcd.h"


#if LCD_ST7796_EN
#define CMD(x)    {LCD_CMD,x}
#define DAT(x)    {LCD_DAT,x}
#define DLY(x)    {DELAY_MS,x}
#define END		{LCD_TAB_END,LCD_TAB_END}

uint8_t st7796_register_init_tab[][2] = {

	//ST7796
	 
	 CMD(0x11),
	 DLY(120),
	 CMD(0xf0),
	 DAT(0xc3),
	
	 CMD(0xf0),
	 DAT(0x96),  
	
	 CMD(0x21),
	
	 CMD(0x36),
	 DAT(0x48),
	
	 CMD(0x3A),
	 DAT(0x66),
	 //DAT(0x55),
	
	 CMD(0xB0),
	 DAT(0x02),
	
	 CMD(0xB4),
	 DAT(0x01),
	
	 CMD(0xB5), 	
	 DAT(0x02),   //vfp[7:0]
	 DAT(0x02),   //vbp[7:0]
	 DAT(0x00),   
	 DAT(0x04),   //hbp[7:0]
	
	 CMD(0xB6), 
	 DAT(0x80), //MCU:80, RGB:20/A0
	 DAT(0x02),
	 DAT(0x3B),
	
	 CMD(0xB7),
	 DAT(0xC6),
	
	 CMD(0xe8),
	 DAT(0x40),
	 DAT(0x8a),
	 DAT(0x00),
	 DAT(0x00),
	 DAT(0x29),
	 DAT(0x19),
	 DAT(0xa5),
	 DAT(0x33),
	
	 CMD(0xc2),
	 DAT(0xa7),
	
	 CMD(0xc5),
	 DAT(0x19),
	
	 CMD(0xe0), //Positive Voltage Gamma Control
	 DAT(0xf0),
	 DAT(0x00),
	 DAT(0x08),
	 DAT(0x0e),
	 DAT(0x0d),
	 DAT(0x1a),
	 DAT(0x37),
	 DAT(0x54),
	 DAT(0x47),
	 DAT(0x2b),
	 DAT(0x16),
	 DAT(0x15),
	 DAT(0x1a),
	 DAT(0x1d),
	
	 CMD(0xe1), //Negative Voltage Gamma Control
	 DAT(0xf0),
	 DAT(0x02),
	 DAT(0x06),
	 DAT(0x0c),
	 DAT(0x0e),
	 DAT(0x29),
	 DAT(0x34),
	 DAT(0x44),
	 DAT(0x47),
	 DAT(0x2b),
	 DAT(0x17),
	 DAT(0x16),
	 DAT(0x19),
	 DAT(0x1d),
	 CMD(0xf0),
	 DAT(0x3c),
	 CMD(0xf0),
	 DAT(0x69),
	
	 DLY(120),
	 CMD(0x29),
	 DLY(100), 
	 CMD(0x2C),

    END
};

lcddev_t  lcdstruct = {
    .name = "st7796",
    .lcd_bus_type = LCD_BUS_I80,
    .bus_width = LCD_BUS_WIDTH_6,
    .color_mode = LCD_MODE_666,
    //.red_width = LCD_BUS_WIDTH_6,
    //.green_width = LCD_BUS_WIDTH_6,
    //.blue_width = LCD_BUS_WIDTH_6,
    .scan_mode = LCD_ROTATE_90,//rotate 90
    .te_mode = 0xff,//te mode, 0xff:disable
    .colrarray = 0,//0:_RGB_ 1:_RBG_,2:_GBR_,3:_GRB_,4:_BRG_,5:_BGR_
    //f(wr) = source_clk/div/2
    //f(wr) >= screen_w * screen_h * clk_per_pixel * 60
    .pclk = 10000000,
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
    .screen_w = 320,
    .screen_h = 480,
    .video_x  = 0,
    .video_y  = 0,
    .video_w  = 320,
    .video_h  = 480,
	.osd_x = 0,
	.osd_y = 0,
	.osd_w = 0, // 0 : value will set to video_w  , use for 4:3 LCD +16:9 sensor show UPDOWN BLACK
	.osd_h = 0, // 0 : value will set to video_h  , use for 4:3 LCD +16:9 sensor show UPDOWN BLACK
	.init_table = st7796_register_init_tab,
	.frame_table = NULL,
    .clk_per_pixel = 2,

    .pclk_inv = 1,
    
    .vlw = 0,// 0,
    .vbp = 0,// 12,
    .vfp = 0,// 12,
    .hlw = 0,// 1,
    .hbp = 0,//14,
    .hfp = 0,//13,

	.de_en  = 1,
	.vs_en	= 0,
	.hs_en	= 0,
    .de_inv = 1,
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
