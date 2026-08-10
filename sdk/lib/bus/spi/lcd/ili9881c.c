#include "lib/lcd/lcd.h"

#if LCD_ILI9881C_EN

#define CMD(x)    {LCD_CMD,x}
#define DAT(x)    {LCD_DAT,x}
#define DLY(x)    {DELAY_MS,x}
#define END		{LCD_TAB_END,LCD_TAB_END}


const uint8_t ili9881c_register_init_tab[][2] = {
	CMD(0XFF),DAT(0X98),DAT(0X81),DAT(0X03),
	CMD(0X01),DAT(0X00),
	CMD(0X02),DAT(0X00),
	CMD(0X03),DAT(0X72),
	CMD(0X04),DAT(0X00),
	CMD(0X05),DAT(0X00),
	CMD(0X06),DAT(0X09),
	CMD(0X07),DAT(0X00),
	CMD(0X08),DAT(0X00),
	CMD(0X09),DAT(0X01),
	CMD(0X0A),DAT(0X00),
	CMD(0X0B),DAT(0X00),
	CMD(0X0C),DAT(0X01),
	CMD(0X0D),DAT(0X00),
	CMD(0X0E),DAT(0X00),
	CMD(0X0F),DAT(0X00),
	CMD(0X10),DAT(0X00),
	CMD(0X11),DAT(0X00),
	CMD(0X12),DAT(0X00),
	CMD(0X13),DAT(0X00),
	CMD(0X14),DAT(0X00),
	CMD(0X15),DAT(0X00),
	CMD(0X16),DAT(0X00),
	CMD(0X17),DAT(0X00),
	CMD(0X18),DAT(0X00),
	CMD(0X19),DAT(0X00),
	CMD(0X1A),DAT(0X00),
	CMD(0X1B),DAT(0X00),
	CMD(0X1C),DAT(0X00),
	CMD(0X1D),DAT(0X00),
	CMD(0X1E),DAT(0X40),
	CMD(0X1F),DAT(0X80),
	CMD(0X20),DAT(0X05),
	CMD(0X21),DAT(0X02),
	CMD(0X22),DAT(0X00),
	CMD(0X23),DAT(0X00),
	CMD(0X24),DAT(0X00),
	CMD(0X25),DAT(0X00),
	CMD(0X26),DAT(0X00),
	CMD(0X27),DAT(0X00),
	CMD(0X28),DAT(0X33),
	CMD(0X29),DAT(0X02),
	CMD(0X2A),DAT(0X00),
	CMD(0X2B),DAT(0X00),
	CMD(0X2C),DAT(0X00),
	CMD(0X2D),DAT(0X00),
	CMD(0X2E),DAT(0X00),
	CMD(0X2F),DAT(0X00),
	CMD(0X30),DAT(0X00),
	CMD(0X31),DAT(0X00),
	CMD(0X32),DAT(0X00),
	CMD(0X33),DAT(0X00),
	CMD(0X34),DAT(0X04),
	CMD(0X35),DAT(0X00),
	CMD(0X36),DAT(0X00),
	CMD(0X37),DAT(0X00),
	CMD(0X38),DAT(0X3c),
	CMD(0X39),DAT(0X00),
	CMD(0X3A),DAT(0X40),
	CMD(0X3B),DAT(0X40),
	CMD(0X3C),DAT(0X00),
	CMD(0X3D),DAT(0X00),
	CMD(0X3E),DAT(0X00),
	CMD(0X3F),DAT(0X00),
	CMD(0X40),DAT(0X00),
	CMD(0X41),DAT(0X00),
	CMD(0X42),DAT(0X00),
	CMD(0X43),DAT(0X00),
	CMD(0X44),DAT(0X00),


	CMD(0X50),DAT(0X01),
	CMD(0X51),DAT(0X23),
	CMD(0X52),DAT(0X45),
	CMD(0X53),DAT(0X67),
	CMD(0X54),DAT(0X89),
	CMD(0X55),DAT(0XAB),
	CMD(0X56),DAT(0X01),
	CMD(0X57),DAT(0X23),
	CMD(0X58),DAT(0X45),
	CMD(0X59),DAT(0X67),
	CMD(0X5A),DAT(0X89),
	CMD(0X5B),DAT(0XAB),
	CMD(0X5C),DAT(0XCD),
	CMD(0X5D),DAT(0XEF),


	CMD(0X5E),DAT(0X11),
	CMD(0X5F),DAT(0X01),
	CMD(0X60),DAT(0X00),
	CMD(0X61),DAT(0X15),
	CMD(0X62),DAT(0X14),
	CMD(0X63),DAT(0X0E),
	CMD(0X64),DAT(0X0F),
	CMD(0X65),DAT(0X0C),
	CMD(0X66),DAT(0X0D),
	CMD(0X67),DAT(0X06),
	CMD(0X68),DAT(0X02),
	CMD(0X69),DAT(0X07),
	CMD(0X6A),DAT(0X02),
	CMD(0X6B),DAT(0X02),
	CMD(0X6C),DAT(0X02),
	CMD(0X6D),DAT(0X02),
	CMD(0X6E),DAT(0X02),
	CMD(0X6F),DAT(0X02),
	CMD(0X70),DAT(0X02),
	CMD(0X71),DAT(0X02),
	CMD(0X72),DAT(0X02),
	CMD(0X73),DAT(0X02),
	CMD(0X74),DAT(0X02),
	CMD(0X75),DAT(0X01),
	CMD(0X76),DAT(0X00),
	CMD(0X77),DAT(0X14),
	CMD(0X78),DAT(0X15),
	CMD(0X79),DAT(0X0E),
	CMD(0X7A),DAT(0X0F),
	CMD(0X7B),DAT(0X0C),
	CMD(0X7C),DAT(0X0D),
	CMD(0X7D),DAT(0X06),
	CMD(0X7E),DAT(0X02),
	CMD(0X7F),DAT(0X07),
	CMD(0X80),DAT(0X02),
	CMD(0X81),DAT(0X02),
	CMD(0X82),DAT(0X02),
	CMD(0X83),DAT(0X02),
	CMD(0X84),DAT(0X02),
	CMD(0X85),DAT(0X02),
	CMD(0X86),DAT(0X02),
	CMD(0X87),DAT(0X02),
	CMD(0X88),DAT(0X02),
	CMD(0X89),DAT(0X02),
	CMD(0X8A),DAT(0X02),

	CMD(0XFF),DAT(0X98),DAT(0X81),DAT(0X04),
	
	//CMD(0X00),DAT(0X00),       //3 lane
	
	CMD(0X6C),DAT(0X15),
	CMD(0X6E),DAT(0X2A),
	CMD(0X6F),DAT(0X35),
	CMD(0X3A),DAT(0X94),
	CMD(0X8D),DAT(0X15),
	CMD(0X87),DAT(0XBA),
	CMD(0X26),DAT(0X76),
	CMD(0XB2),DAT(0XD1),
	CMD(0XB5),DAT(0X06),

	//CMD(0X),DAT(0X1),DAT(0X01	),

	CMD(0XFF),DAT(0X98),DAT(0X81),DAT(0X01),
	CMD(0X22),DAT(0X0A),
	CMD(0X31),DAT(0X00),
	CMD(0X53),DAT(0Xa5),
	CMD(0X55),DAT(0XA5),
	CMD(0X50),DAT(0X9A),
	CMD(0X51),DAT(0X9A),
	CMD(0X60),DAT(0X22),
	CMD(0X61),DAT(0X00),
	CMD(0X62),DAT(0X19),
	CMD(0X63),DAT(0X10),
	CMD(0XA0),DAT(0X08),
	CMD(0XA1),DAT(0X18),
	CMD(0XA2),DAT(0X1f),
	CMD(0XA3),DAT(0X0f),
	CMD(0XA4),DAT(0X14),
	CMD(0XA5),DAT(0X24),
	CMD(0XA6),DAT(0X1B),
	CMD(0XA7),DAT(0X1B),
	CMD(0XA8),DAT(0X5a),
	CMD(0XA9),DAT(0X1c),
	CMD(0XAA),DAT(0X29),
	CMD(0XAB),DAT(0X4e),
	CMD(0XAC),DAT(0X19),
	CMD(0XAD),DAT(0X16),
	CMD(0XAE),DAT(0X4c),
	CMD(0XAF),DAT(0X21),
	CMD(0XB0),DAT(0X2a),
	CMD(0XB1),DAT(0X48),
	CMD(0XB2),DAT(0X62),
	CMD(0XB3),DAT(0X39),
	
	//CMD(0XB6),DAT(0Xa0),
	CMD(0XB7),DAT(0X02),     //03:2 lane  2: 4 lane

	CMD(0XC0),DAT(0X08),
	CMD(0XC1),DAT(0X0d),
	CMD(0XC2),DAT(0X18),
	CMD(0XC3),DAT(0X10),
	CMD(0XC4),DAT(0X0c),
	CMD(0XC5),DAT(0X1d),
	CMD(0XC6),DAT(0X10),
	CMD(0XC7),DAT(0X16),
	CMD(0XC8),DAT(0X61),
	CMD(0XC9),DAT(0X1b),
	CMD(0XCA),DAT(0X27),
	CMD(0XCB),DAT(0X5e),
	CMD(0XCC),DAT(0X1c),
	CMD(0XCD),DAT(0X1b),
	CMD(0XCE),DAT(0X4e),
	CMD(0XCF),DAT(0X22),
	CMD(0XD0),DAT(0X27),
	CMD(0XD1),DAT(0X52),
	CMD(0XD2),DAT(0X61),
	CMD(0XD3),DAT(0X39),
	
	
	CMD(0X35),DAT(0X05),
	CMD(0X11),
	CMD(0XFF),DAT(0X98),DAT(0X81),DAT(0X00),
	
	DLY(120),
	CMD(0X11),	
	DLY(120),
	CMD(0X29),
	DLY(120),  

	//CMD(0XFF),DAT(0X98),DAT(0X81),DAT(0X01),
	//CMD(0XB6),DAT(0XA0),  
	DLY(120), 

	END

};


lcddev_t  lcdstruct = {
    .name = "ili9881c_mipi",
    .lcd_bus_type = LCD_BUS_MIPI,
    .bus_width = LCD_BUS_WIDTH_16,
    .color_mode = LCD_MODE_565,
    //.red_width = LCD_BUS_WIDTH_6,
    //.green_width = LCD_BUS_WIDTH_6,
    //.blue_width = LCD_BUS_WIDTH_6,
    .scan_mode = LCD_ROTATE_90,//rotate 90
    .te_mode = 0xff,//te mode, 0xff:disable
    .colrarray = 0,//0:_RGB_ 1:_RBG_,2:_GBR_,3:_GRB_,4:_BRG_,5:_BGR_
    .lane_num      = 4,
    //f(wr) = source_clk/div/2
    //f(wr) >= screen_w * screen_h * clk_per_pixel * 60
    .pclk = 30000000,
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
    .screen_w = 720,
    .screen_h = 1280,
    .video_x  = 0,
    .video_y  = 0,
    .video_w  = 720,
    .video_h  = 1280,
	.osd_x = 0,
	.osd_y = 0,
	.osd_w = 0, // 0 : value will set to video_w  , use for 4:3 LCD +16:9 sensor show UPDOWN BLACK
	.osd_h = 0, // 0 : value will set to video_h  , use for 4:3 LCD +16:9 sensor show UPDOWN BLACK
	.init_table = ili9881c_register_init_tab,
	.frame_table = NULL,
    .clk_per_pixel = 2,

    .pclk_inv = 1,

    .vlw 			= 2*2,
    .vbp 			= 30*2,
    .vfp 			= 20*2,

    .hlw 			= 33*2,
    .hbp 			= 100*2,
    .hfp 			= 100*2,

	
	.de_inv = 0,
	.hs_inv = 0,
	.vs_inv = 0,


	.de_en  = 1,
	.vs_en	= 1,
	.hs_en	= 1,

    
    .brightness = 1,
    .saturation = 7,
    .contrast   = 7,
    .contra_index = 8,

    .gamma_red = 3,
    .gamma_green=3,
    .gamma_blue=3,

};
#endif

