#include "lib/lcd/lcd.h"

#if LCD_JD9852_EN

#define CMD(x)    {LCD_CMD,x}
#define DAT(x)    {LCD_DAT,x}
#define DLY(x)    {DELAY_MS,x}
#define END		{LCD_TAB_END,LCD_TAB_END}


const uint8_t jd9852_register_init_tab[][2] = {
	DLY(200),
	CMD(0xDF), //Password
	DAT(0x98),
	DAT(0x51),
	DAT(0xE9),
	
	//---------------- PAGE0 --------------
	CMD(0xDE),		
	DAT(0x00),	
	
	//VGMP,VGSP,VGMN,VGSN 4.2
	CMD(0xB7),		
	DAT(0x1E),
	DAT(0x7D),
	DAT(0x1E),
	DAT(0x2B),
	
	//Set_R_GAMMA
	CMD(0xC8),	
	DAT(0x3F),		
	DAT(0x31), 
	DAT(0x29), 
	DAT(0x28), 
	DAT(0x2C), 
	DAT(0x31), 
	DAT(0x2D), 
	DAT(0x2D), 
	DAT(0x2A), 
	DAT(0x28), 
	DAT(0x23), 
	DAT(0x16), 
	DAT(0x10), 
	DAT(0x0E), 
	DAT(0x08), 
	DAT(0x0E), 
	DAT(0x3F),		
	DAT(0x31), 
	DAT(0x29), 
	DAT(0x28), 
	DAT(0x2C), 
	DAT(0x31), 
	DAT(0x2D), 
	DAT(0x2C), 
	DAT(0x2A), 
	DAT(0x28), 
	DAT(0x23), 
	DAT(0x16), 
	DAT(0x10), 
	DAT(0x0E), 
	DAT(0x08), 
	DAT(0x0E), 
	
	//POW_CTRL
	CMD(0xB9),		
	DAT(0x33),		
	DAT(0x08),
	DAT(0xCC),
	
	//DCDC_SEL 
	CMD(0xBB),		
	DAT(0x46),//VGH 14.75,VGL-9.6	
	DAT(0x7A),
	DAT(0x30),		
	DAT(0xD0), //3	
	DAT(0x7C), //6	
	DAT(0x60),		
	DAT(0x70),//50		
	DAT(0x70),
	
	//VDDD_CTRL 
	CMD(0xBC),		
	DAT(0x38),		
	DAT(0x3C),
	
	//SETSTBA
	CMD(0xC0),
	DAT(0x31),
	DAT(0x20),
	
	//SETPANEL(default)  
	CMD(0xC1),		
	DAT(0x12), 
	
	//SETRGBCYC 
	CMD(0xC3),		
	DAT(0x08),
	DAT(0x00),		
	DAT(0x0A),
	DAT(0x10),		
	DAT(0x08),		
	DAT(0x54),		
	DAT(0x45),		
	DAT(0x71),		
	DAT(0x2C),
	
	//SETRGBCYC(default)  
	CMD(0xC4),		
	DAT(0x00),
	DAT(0xA0),		
	DAT(0x79),
	DAT(0x0E),		
	DAT(0x0A),		
	DAT(0x16),		
	DAT(0x79),		
	DAT(0x0E),		
	DAT(0x0A),
	DAT(0x16),		
	DAT(0x79),		
	DAT(0x0E),		
	DAT(0x0A),		
	DAT(0x16),		
	DAT(0x82),		
	DAT(0x00),		
	DAT(0x03),
	
	
	//SET_GD(default)
	CMD(0xD0),		
	DAT(0x04),
	DAT(0x0C),		
	DAT(0x6B),
	DAT(0x0F),		
	DAT(0x07),		
	DAT(0x03),
	
	//RAMCTRL(default)
	CMD(0xD7),		
	DAT(0x13),
	DAT(0x00),	
	
	//---------------- PAGE2 --------------
	CMD(0xDE),		
	DAT(0x02),	
	DLY(1),
	
	//DCDC_SET
	//SSD_Number(0x06),
	CMD(0xB8),		
	DAT(0x1D),
	DAT(0xA0),
	DAT(0x2F),
	DAT(0x04),
	DAT(0x33),
	
	//SETRGBCYC2
	CMD(0xC1),		
	DAT(0x10),
	DAT(0x66),
	DAT(0x66),
	DAT(0x01),
	
	//---------------- PAGE0 --------------
	CMD(0xDE),		
	DAT(0x00),	
	
	
	// sleep out
	CMD(0x11),		// SLPOUT
	DLY(120),
	//---------------- PAGE2 --------------
	CMD(0xDE),		
	DAT(0x02),	
	DLY(1),
	
	//OSCM
	CMD(0xC5),		
	DAT(0x4E),	
	DAT(0x00),
	DAT(0x00),
	DLY(1),
	
	//SETMIPI_2
	CMD(0xCA),		
	DAT(0x30),	
	DAT(0x20),
	DAT(0xF4),
	DLY(1),
	
	//---------------- PAGE4 --------------
	CMD(0xDE),		
	DAT(0x04),	
	DLY(1),
	
	//SETPHY3
	CMD(0xD3),		
	DAT(0x3C),	
	DLY(100),
	//---------------- PAGE0 --------------
	CMD(0xDE),		
	DAT(0x00),
	DLY(50),  
	
	// display on
	CMD(0x29),		// SLPOUT



	END

};


lcddev_t  lcdstruct = {
    .name = "jd9852_mipi",
    .lcd_bus_type = LCD_BUS_MIPI,
    .bus_width = LCD_BUS_WIDTH_24,
    .color_mode = LCD_MODE_888,
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
    .lane_num      = 1,
    .lcd_data_mode = (0<<31)|//data inversion mode
                     (2<<24)|//data compress mode
                     (1<<20)|//fifo mode
                     (0<<17)|//output cycle 2 shift direction
                     (0<<12)|//output cycle 2 shift bit
                     (0<<11)|//output cycle 1 shift direction
                     (0<<6)| //output cycle 1 shift bit
                     (0<<5)| //output cycle 0 shift direction
                     (8<<0), //output cycle 0 shift bit
    .screen_w = 240,
    .screen_h = 320,
    .video_x  = 0,
    .video_y  = 0,
    .video_w  = 240,
    .video_h  = 320,
	.osd_x = 0,
	.osd_y = 0,
	.osd_w = 240, // 0 : value will set to video_w  , use for 4:3 LCD +16:9 sensor show UPDOWN BLACK
	.osd_h = 320, // 0 : value will set to video_h  , use for 4:3 LCD +16:9 sensor show UPDOWN BLACK
	.init_table = jd9852_register_init_tab,
	.frame_table = NULL,
    .clk_per_pixel = 2,

    .pclk_inv = 1,

    .vlw 			= 2,
    .vbp 			= 2,
    .vfp 			= 10,

    .hlw 			= 2,
    .hbp 			= 8,
    .hfp 			= 12,

	
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

