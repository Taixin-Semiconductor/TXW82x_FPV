#ifndef LIB_LCD_H
#define LIB_LCD_H
#include "typesdef.h"
#include "dev.h"
#include "hal/lcdc.h"
#include "hal/scale.h"

#define LCD_ROTATE                 1
#define SCALE_DIRECT_TO_LCD        0
#define LCD_FROM_DEC			   0
#define LCD_SET_ROTATE_180         0
#define LCD_THREE_BUF              0
#define LCD_FROM_DMA2D_BUF         1
#define LCD_33_WVGA                1

#if LCD_33_WVGA
#define SCALE_WIDTH		800//800//800//854//848//640
#define SCALE_HIGH		480//480//480//480//480//
#else
#define SCALE_WIDTH		480//400//162//320
#define SCALE_HIGH		320//400//132//240
#endif

#define PHOTO1_W        320
#define PHOTO1_H        240



#define SCALE_CONFIG_W  ((SCALE_WIDTH+3)/4)*4
#define SCALE_PHOTO1_CONFIG_W  ((PHOTO1_W+3)/4)*4        
#define SCALE_CONFIG_H  ((SCALE_HIGH+7)/8)*8

#if LCD_SET_ROTATE_180
#define Y_OFFSET   SCALE_CONFIG_W*(SCALE_HIGH-1)
#define UV_OFFSET  ((SCALE_CONFIG_W/2+3)/4)*4 * (SCALE_HIGH/2-1)

#define DEC_Y_OFFSET      PHOTO1_W*(PHOTO1_H-1)//428*(240-1)
#define DEC_UV_OFFSET     ((SCALE_PHOTO1_CONFIG_W/2+3)/4)*4 * (PHOTO1_H/2-1) //160*119//216*(119)

#else
#define Y_OFFSET   0
#define UV_OFFSET  0

#define DEC_Y_OFFSET      0
#define DEC_UV_OFFSET     0

#endif

extern uint32 p1_w,p1_h;
extern uint32 scale_p1_w;
extern uint32 dec_y_offset,dec_uv_offset;


#define SRAMBUF_WLEN   64




enum init_tab_token_e {
    LCD_CMD,
    LCD_DAT,
    SPI_DAT,
    DELAY_MS,
    LCD_TAB_END  = 0XFFUL,
};

typedef struct
{     
	uint8_t osd_buf_to_lcd;
	uint8_t osd_buf_to_updata;
	uint8_t updata_finish;
	uint8_t lcd_run_new_lcd;
	uint8_t lcd_p0p1_state;    //0:p0p1 disable  1:p0 enable p1 disable  2: p0 disable p1 enable  3: p0p1 enable
	uint8_t lcd_p0p1_cur;
	uint8_t lcd_p1_size_reset;
	uint8_t lcd_osd_mode;
	uint8_t lcd_osd_cur_mode;
}lcd_msg;


typedef struct _lcd_desc_s {
	char name[32];
	uint8 lcd_bus_type;
	uint8 bus_width;
	uint8 bus_cmd;
	uint8 color_mode;
	//uint8 red_width;
	//uint8 green_width;
	//uint8 blue_width;
	uint8 osd_scan_mode;  //osd是否旋转,给到lvgl
	uint8 scan_mode;
	//only mcu-bus use
	//-1,te disable.
	//0,te just interrupt,no kick TX.用户(写APP的人)自己编写TE中断服务函数,自己控制kick tx.
	//1,te interrupt and auto kick TX,中断被内部使用,用户不应该操作TE中断
	uint8 te_mode;

	uint8 lane_num;
	//for mcu data-bus, cs_setup and cs_hold eq 0 default,so freq(wr) = src_clk/div/2.
	//for rgb bus,freq(pclk) = src_clk/div;
	uint32 pclk;

	//the 'lcd_data_mode' eq LCDDMOD
	uint32 lcd_data_mode;
	uint32 lcd_data_mode1;
	uint32 colrarray;
	uint16 screen_w,video_x,video_w;
	uint16 screen_h,video_y,video_h;

	uint16 osd_x,osd_y,osd_w,osd_h;
	
	uint8 (* init_table)[2];
	uint8 * frame_table;

	
	uint8 clk_per_pixel;
	uint8 even_order;
	uint8 odd_order;
	//------rgb-bus----------
	uint8 pclk_inv;
	//unit:line,real vfp=vfp+1(line)
	uint8 vlw,vbp,vfp;
	//unit:pclk
	uint8 hlw;
	//unit:pixel,real hfp=.hfp(pixel) + 1(pclk)
	uint8 hbp,hfp;

	uint8 de_en;
	uint8 hs_en;
	uint8 vs_en;
	uint8 de_inv;//de默认是高电平有效,如果de_inv是1，则将其反转,0xff:disable
	uint8 hs_inv;//hs_inv同de_inv
	uint8 vs_inv;//vs_inv同de_inv
	union 
	{
		uint8_t   value;
		struct
		{
			uint8_t   rd_inv: 1;
			uint8_t   cs_inv: 1;
			uint8_t   dc_inv: 1;
			uint8_t   wr_inv: 1;
			uint8_t   resev: 4;
		}mcu;
	}signal_config;
	
	uint8 spi_cpol; //CPOL = 0,CLK is "Low" when inactive
	uint8 spi_cpha; //CPHA = 0,sample edge is first edge
	uint8 spi_order;//ORDER = 0,MSB
	uint8 spi_bits; //number of data bits
	
	uint32 contrast;
	
	uint32 brightness;
	uint32 saturation;	  
	uint32 contra_index;

	uint32 gamma_red;
	uint32 gamma_green;
	uint32 gamma_blue;
	
	uint32 de_ccm[12];
	uint32 de_saj[5];
}
lcddev_t;




extern lcddev_t  lcdstruct;
void scale_soft_run(uint8_t *softbuf,uint32_t w,uint32 h);
void scale_to_lcd_config_soft(uint8_t *softbuf,uint32_t w,uint32_t h);
void scale_from_vpp_to_jpg(struct scale_device *scale_dev,uint32 yuvbuf_addr,uint32 s_w,uint32 s_h,uint32 d_w,uint32 d_h);
void scale_from_soft_to_jpg(struct scale_device *scale_dev,uint32 yuvbuf_addr,uint32 s_w,uint32 s_h,uint32 d_w,uint32 d_h);
void set_lcd_photo1_config(uint32 w,uint32 h,uint8 rotate_180);
void set_lcd_enlarge_config(uint32_t sw,uint32_t sh,uint32_t ow,uint32_t oh,uint8_t enlarge);
void jpg_dec_scale_del();
void jpg_decode_scale_config();
void lcd_module_run();
void lcd_user_frame(uint32 frame_addr);
void jpg_decode_to_lcd(uint32 photo,uint32 jpg_w,uint32 jpg_h,uint32 video_w,uint32 video_h);
int32 jpg_decode_is_finish();
void mipi_dsi_init(uint32 w,uint32 h,uint32 dclk,uint8 vsa,uint8 vbp,uint8 vfp,uint8 hsa,uint8 hbp,uint8 hfp,uint8 lanenum,uint8 colortype);



#endif
