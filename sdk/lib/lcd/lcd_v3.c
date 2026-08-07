#include "sys_config.h"
#include "typesdef.h"
#include <csi_kernel.h>
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "dev.h"
#include "devid.h"
#include "hal/gpio.h"
#include "hal/lcdc.h"
#include "hal/spi.h"
#include "hal/dsi.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "dev/vpp/hgvpp.h"
#include "dev/scale/hgscale.h"
#include "dev/jpg/hgjpg.h"
#include "dev/lcdc/hglcdc.h"
#include "osal/semaphore.h"
#include "lib/lcd/lcd.h"
#include "osal/mutex.h"
#include "lib/lcd/gui.h"
#include "lib/scale/scale_dev.h"
#include "osal/sleep.h"
#include "osal/time.h"
uint8_t osd_palette[512]__attribute__ ((aligned(4)));

#if LCD_EN
#define VIDEO_EN    1
#define OSD_EN      0
struct os_mutex m2m1_mutex;

uint8 *video_psram_mem; //__attribute__ ((aligned(4)));//
uint8 *video_psram_mem1;
uint8 *video_psram_mem2;

uint8 *video_dma2d_mem;
uint8 *video_dma2d_mem1;
uint8 *video_dma2d_mem1_cache;



__psram_data uint8 video_psram_screen[SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2] __aligned(4);

__psram_data uint8 video_psram_config_mem[SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2] __aligned(4); //__attribute__ ((aligned(4)));//
__psram_data uint8 video_psram_config_mem1[SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2] __aligned(4);

#if LCD_FROM_DMA2D_BUF
__psram_data uint8 video_psram_dma2d_mem[SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2] __aligned(4); //__attribute__ ((aligned(4)));//
__psram_data uint8 video_psram_dma2d_mem1[320*240+320*240/2] __aligned(4);
__psram_data uint8 video_psram_dma2d_mem1_cache[320*240+320*240/2] __aligned(4);
#endif

__psram_data uint8 video_psram_config_mem2[SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2] __aligned(4);


#if (LCD_FROM_DMA2D_BUF != 0)
__psram_data uint8 video_decode_config_mem[SCALE_PHOTO1_CONFIG_W*PHOTO1_H+SCALE_PHOTO1_CONFIG_W*PHOTO1_H/2] __aligned(4);
__psram_data uint8 video_decode_config_mem1[SCALE_PHOTO1_CONFIG_W*PHOTO1_H+SCALE_PHOTO1_CONFIG_W*PHOTO1_H/2] __aligned(4);
__psram_data uint8 video_decode_config_mem2[SCALE_PHOTO1_CONFIG_W*PHOTO1_H+SCALE_PHOTO1_CONFIG_W*PHOTO1_H/2] __aligned(4);
#endif


uint8 *video_decode_mem;
uint8 *video_decode_mem1;
uint8 *video_decode_mem2;

extern uint8 *scaler2buf;

extern uint8_t osd_encode_buf[110*1024];
extern uint8_t osd_encode_buf1[110*1024];
extern uint8_t osd_palette[512];
volatile uint8 lcdc_decode_enable = 0;

volatile uint32 decode_num = 0;

struct os_mutex decode_mutex;

struct lcdc_device *lcd_dev_gobal;
extern uint8 *yuvbuf;
#define LCD_ROTATE_LINE      64
uint8 lcd_line_buf[SCALE_CONFIG_H*LCD_ROTATE_LINE+(SCALE_CONFIG_H/2)*LCD_ROTATE_LINE*2 ]__attribute__ ((aligned(4)));      //行数与rotate的BURST大小相关,要比burst大
//uint8 lcd_line_buf[SCALE_CONFIG_W*LCD_ROTATE_LINE+(SCALE_CONFIG_W/2)*LCD_ROTATE_LINE*2 ]__attribute__ ((aligned(4)));      //行数与rotate的BURST大小相关,要比burst大
extern Vpp_stream photo_msg;
lcd_msg lcd_info;

unsigned char osd565_encode1[]__attribute__ ((aligned(4)))={
	0xFF,0XFF,0XFF,0XFF,0X1f,0X00,0xFF,0XFF,0XFF,0XFF,0X1f,0X00,0xFF,0XFF,0XFF,0XFF,0X1f,0X00,0xFF,0XFF,0XFF,0XFF,0X1f,0X00,0xFF,0XFF,0XFF,0XFF,0X1f,0X00,
	0xFF,0XFF,0XFF,0XFF,0X1f,0X00,0xFF,0XFF,0XFF,0XFF,0X1f,0X00,0xFF,0XFF,0XFF,0XFF,0X1f,0X00,0xFF,0XFF,0XFF,0XFF,0X1f,0X00,0xFF,0XFF,0XFF,0XFF,0X1f,0X00,
	//0xFF,0XFF,0XFF,0XFF,0X00,0Xf8,0XFF,0XFF,0XFF,0XFF,0X00,0Xf8,0xFF,0XFF,0XFF,0XFF,0X00,0Xf8,0xFF,0XFF,0XFF,0XFF,0X00,0Xf8,0xFF,0XFF,0XFF,0XFF,0X00,0Xf8,
	//0xFF,0XFF,0XFF,0XFF,0Xe0,0X07,0xFF,0XFF,0XFF,0XFF,0Xe0,0X07,0xFF,0XFF,0XFF,0XFF,0Xe0,0X07,0xFF,0XFF,0XFF,0XFF,0Xe0,0X07,0xFF,0XFF,0XFF,0XFF,0Xe0,0X07,
	//0xFF,0XFF,0XFF,0XFF,0X1f,0X00,0xFF,0XFF,0XFF,0XFF,0X1f,0X00,0xFF,0XFF,0XFF,0XFF,0X1f,0X00,0xFF,0XFF,0XFF,0XFF,0X1f,0X00,0xFF,0XFF,0XFF,0XFF,0X1f,0X00,
	0xFF,0XFF,0X1F,0X1F,0X00,0X01,0xFF,0XFF,0X1F,0X1F,0X00,0X00,0xFF,0XFF,0XFF,0X1F,0X00,0X00,0xFF,0XFF,0XFF,0XFF,0X00,0X00,0xFF,0XFF,0XFF,0XFF,0X00,0X00,
	0xFF,0XFF,0X1F,0X1F,0X00,0X00,0xFF,0XFF,0X1F,0X1F,0X00,0X00,0xFF,0XFF,0XFF,0X1F,0X00,0X00,0xFF,0XFF,0XFF,0XFF,0X00,0X00,0xFF,0XFF,0XFF,0XFF,0X00,0X00,
	0xFF,0XFF,0X1F,0X1F,0X00,0X00,0xFF,0XFF,0X1F,0X1F,0X00,0X00,0xFF,0XFF,0XFF,0X1F,0X00,0X00,0xFF,0XFF,0XFF,0XFF,0X00,0X00,0xFF,0XFF,0XFF,0XFF,0X00,0X00,
	0xFF,0XFF,0X1F,0X1F,0X00,0X00,0xFF,0XFF,0X1F,0X1F,0X00,0X00,0xFF,0XFF,0XFF,0X1F,0X00,0X00,0xFF,0XFF,0XFF,0XFF,0X00,0X00,0xFF,0XFF,0XFF,0XFF,0X00,0X00,
};

unsigned char osd_rgb256[2432]__attribute__ ((aligned(4))) = {
	0x00, 0x00, 0x00, 0x80, 0x00, 0x04, 0x00, 0x84, 0x10, 0x00, 0x10, 0x80, 0x10, 0x04, 0x18, 0xC6, 
	0xF8, 0xC6, 0x5E, 0xA6, 0x00, 0x41, 0x00, 0x61, 0x00, 0x81, 0x00, 0xA1, 0x00, 0xC1, 0x00, 0xE1, 
	0x00, 0x02, 0x00, 0x22, 0x00, 0x42, 0x00, 0x62, 0x00, 0x82, 0x00, 0xA2, 0x00, 0xC2, 0x00, 0xE2, 
	0x00, 0x03, 0x00, 0x23, 0x00, 0x43, 0x00, 0x63, 0x00, 0x83, 0x00, 0xA3, 0x00, 0xC3, 0x00, 0xE3, 
	0x00, 0x04, 0x00, 0x24, 0x00, 0x44, 0x00, 0x64, 0x00, 0x84, 0x00, 0xA4, 0x00, 0xC4, 0x00, 0xE4, 
	0x00, 0x05, 0x00, 0x25, 0x00, 0x45, 0x00, 0x65, 0x00, 0x85, 0x00, 0xA5, 0x00, 0xC5, 0x00, 0xE5, 
	0x00, 0x06, 0x00, 0x26, 0x00, 0x46, 0x00, 0x66, 0x00, 0x86, 0x00, 0xA6, 0x00, 0xC6, 0x00, 0xE6, 
	0x00, 0x07, 0x00, 0x27, 0x00, 0x47, 0x00, 0x67, 0x00, 0x87, 0x00, 0xA7, 0x00, 0xC7, 0x00, 0xE7, 
	0x08, 0x00, 0x08, 0x20, 0x08, 0x40, 0x08, 0x60, 0x08, 0x80, 0x08, 0xA0, 0x08, 0xC0, 0x08, 0xE0, 
	0x08, 0x01, 0x08, 0x21, 0x08, 0x41, 0x08, 0x61, 0x08, 0x81, 0x08, 0xA1, 0x08, 0xC1, 0x08, 0xE1, 
	0x08, 0x02, 0x08, 0x22, 0x08, 0x42, 0x08, 0x62, 0x08, 0x82, 0x08, 0xA2, 0x08, 0xC2, 0x08, 0xE2, 
	0x08, 0x03, 0x08, 0x23, 0x08, 0x43, 0x08, 0x63, 0x08, 0x83, 0x08, 0xA3, 0x08, 0xC3, 0x08, 0xE3, 
	0x08, 0x04, 0x08, 0x24, 0x08, 0x44, 0x08, 0x64, 0x08, 0x84, 0x08, 0xA4, 0x08, 0xC4, 0x08, 0xE4, 
	0x08, 0x05, 0x08, 0x25, 0x08, 0x45, 0x08, 0x65, 0x08, 0x85, 0x08, 0xA5, 0x08, 0xC5, 0x08, 0xE5, 
	0x08, 0x06, 0x08, 0x26, 0x08, 0x46, 0x08, 0x66, 0x08, 0x86, 0x08, 0xA6, 0x08, 0xC6, 0x08, 0xE6, 
	0x08, 0x07, 0x08, 0x27, 0x08, 0x47, 0x08, 0x67, 0x08, 0x87, 0x08, 0xA7, 0x08, 0xC7, 0x08, 0xE7, 
	0x10, 0x00, 0x10, 0x20, 0x10, 0x40, 0x10, 0x60, 0x10, 0x80, 0x10, 0xA0, 0x10, 0xC0, 0x10, 0xE0, 
	0x10, 0x01, 0x10, 0x21, 0x10, 0x41, 0x10, 0x61, 0x10, 0x81, 0x10, 0xA1, 0x10, 0xC1, 0x10, 0xE1, 
	0x10, 0x02, 0x10, 0x22, 0x10, 0x42, 0x10, 0x62, 0x10, 0x82, 0x10, 0xA2, 0x10, 0xC2, 0x10, 0xE2, 
	0x10, 0x03, 0x10, 0x23, 0x10, 0x43, 0x10, 0x63, 0x10, 0x83, 0x10, 0xA3, 0x10, 0xC3, 0x10, 0xE3, 
	0x10, 0x04, 0x10, 0x24, 0x10, 0x44, 0x10, 0x64, 0x10, 0x84, 0x10, 0xA4, 0x10, 0xC4, 0x10, 0xE4, 
	0x10, 0x05, 0x10, 0x25, 0x10, 0x45, 0x10, 0x65, 0x10, 0x85, 0x10, 0xA5, 0x10, 0xC5, 0x10, 0xE5, 
	0x10, 0x06, 0x10, 0x26, 0x10, 0x46, 0x10, 0x66, 0x10, 0x86, 0x10, 0xA6, 0x10, 0xC6, 0x10, 0xE6, 
	0x10, 0x07, 0x10, 0x27, 0x10, 0x47, 0x10, 0x67, 0x10, 0x87, 0x10, 0xA7, 0x10, 0xC7, 0x10, 0xE7, 
	0x18, 0x00, 0x18, 0x20, 0x18, 0x40, 0x18, 0x60, 0x18, 0x80, 0x18, 0xA0, 0x18, 0xC0, 0x18, 0xE0, 
	0x18, 0x01, 0x18, 0x21, 0x18, 0x41, 0x18, 0x61, 0x18, 0x81, 0x18, 0xA1, 0x18, 0xC1, 0x18, 0xE1, 
	0x18, 0x02, 0x18, 0x22, 0x18, 0x42, 0x18, 0x62, 0x18, 0x82, 0x18, 0xA2, 0x18, 0xC2, 0x18, 0xE2, 
	0x18, 0x03, 0x18, 0x23, 0x18, 0x43, 0x18, 0x63, 0x18, 0x83, 0x18, 0xA3, 0x18, 0xC3, 0x18, 0xE3, 
	0x18, 0x04, 0x18, 0x24, 0x18, 0x44, 0x18, 0x64, 0x18, 0x84, 0x18, 0xA4, 0x18, 0xC4, 0x18, 0xE4, 
	0x18, 0x05, 0x18, 0x25, 0x18, 0x45, 0x18, 0x65, 0x18, 0x85, 0x18, 0xA5, 0x18, 0xC5, 0x18, 0xE5, 
	0x18, 0x06, 0x18, 0x26, 0x18, 0x46, 0x18, 0x66, 0x18, 0x86, 0x18, 0xA6, 0xDE, 0xFF, 0x14, 0xA5, 
	0x10, 0x84, 0x00, 0xF8, 0xE0, 0x07, 0xE0, 0xFF, 0x1F, 0x00, 0x1F, 0xF8, 0xFF, 0x07, 0xDF, 0xFF, 	
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 
	0x38, 0x38, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 
	0x38, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x38, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 
	0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
	0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 
	0x38, 0x38, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 
	0x38, 0x01, 0x01, 0xFF, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0xFE, 0xFE, 0xFE, 
	0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0xFE, 0xFE, 0xFE, 
	0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0xFE, 0xFE, 0xFE, 
	0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x38, 0x01, 0x01, 0xFF, 0x01, 0xFE, 0xFE, 0xFE, 
	0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
	0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 
	0x38, 0x38, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 
	0x38, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x38, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
	0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 
	0x38, 0x38, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 
	0x38, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 0x38, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFE, 0xFE, 0xFE, 0xFE, 0x01, 0xFF, 0x01, 
	0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
	0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 
	0x38, 0x38, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 
	0x38, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 
	0x01, 0xFF, 0xFF, 0xFF, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 0x38, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x01, 0x38, 0x38, 0x38, 0x38, 0x01, 0xFF, 0x01, 
	0x38, 0x38, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x01, 0x38, 0x38, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
	0x38, 0x38, 0x38, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38
};

extern uint8 *yuvbuf;
extern uint8 *yuvbuf1;


void lcd_register_read_3line(struct spi_device *spi_dev,uint32 code, uint8* buf, uint32 len){
	uint8 spi_buf[4];
	spi_buf[0] = code;
	spi_buf[1] = 0x00;	
	//ll_spi_clear_cs(SPI0);
	spi_set_cs(spi_dev,0,0);
	//ll_spi_buf_tx(SPI0, spi_buf, 2);
	spi_write(spi_dev,spi_buf,2);
//	SPI0->CON0 &= ~LL_SPI_CON0_FRAME_SIZE(0x3F);
//	SPI0->CON0 |= LL_SPI_CON0_FRAME_SIZE(8);
	spi_ioctl(spi_dev,SPI_SET_FRAME_SIZE,8,0);
	//ll_spi_buf_rx(SPI0, buf, len);
	spi_read(spi_dev,buf,len);
	//ll_spi_set_cs(SPI0);		
	spi_set_cs(spi_dev,0,1);
	//SPI0->CON0 &= ~LL_SPI_CON0_FRAME_SIZE(0x3F);
	//SPI0->CON0 |= LL_SPI_CON0_FRAME_SIZE(9);	
	spi_ioctl(spi_dev,SPI_SET_FRAME_SIZE,9,0);
}

void lcd_register_write_3line(struct spi_device *spi_dev,uint32 code, uint8* buf, uint32 len){

	uint8 itk;
	uint8 spi_buf[2*20];
	spi_buf[0] = code;
	spi_buf[1] = 0x00;
	for(itk = 0;itk < len;itk++){
		spi_buf[2*(itk+1)] = buf[itk];
		spi_buf[2*(itk+1)+1] = 0x01;		
	}
	
//	ll_spi_clear_cs(SPI0);
	spi_set_cs(spi_dev,0,0);
	
//	ll_spi_buf_tx(SPI0, spi_buf, 2*len+2);
	spi_write(spi_dev,spi_buf,2*len+2);

//	ll_spi_set_cs(SPI0);
	spi_set_cs(spi_dev,0,1);
}


void lcd_table_init(struct spi_device *spi_dev,uint8_t *lcd_table){
	uint32 itk = 0;
	uint8  last_opt = -1;
	uint8 buf[20];
	uint8 code;
	uint8 delay_ms = 0;
	uint8 param_num = 0;


	while(((lcd_table[itk] == LCD_TAB_END) && (lcd_table[itk+1] == LCD_TAB_END)) == 0){
		switch(lcd_table[itk]){
			case LCD_CMD:
				if(last_opt == LCD_CMD){
					lcd_register_write_3line(spi_dev,code,buf,param_num);
				}else if(last_opt == DELAY_MS){
					os_sleep_ms(delay_ms);
				}
				last_opt = LCD_CMD;
				code = lcd_table[itk+1];
				param_num = 0;
			break;
			case LCD_DAT:
				buf[param_num] = lcd_table[itk+1];
				param_num++;
			break;
			case DELAY_MS:
				if(last_opt == LCD_CMD){
					lcd_register_write_3line(spi_dev,code,buf,param_num);
				}else if(last_opt == DELAY_MS){
					os_sleep_ms(delay_ms);
				}
				last_opt = DELAY_MS;
				delay_ms = lcd_table[itk+1];
			break;
			default:
			break;
		}
		itk += 2; 
	}

	if(last_opt == LCD_CMD){
		lcd_register_write_3line(spi_dev,code,buf,param_num);
	}else if(last_opt == DELAY_MS){
		os_sleep_ms(delay_ms);
	}	
}


void lcd_table_init_MCU(struct lcdc_device *lcd_dev,uint8_t (*lcd_table)[2])
{
  uint8_t (*table)[2];
  int      i; 
  uint32_t dat_cmd;
  uint32_t data;
  table = lcd_table;
  for (i = 0;; i++)
  {
    dat_cmd = table[i][0];
    data    = table[i][1];
    //结束
    switch (dat_cmd)
    {
    case LCD_CMD:
      lcdc_mcu_write_reg(lcd_dev,data);
      break;
    case LCD_DAT:
      lcdc_mcu_write_data(lcd_dev,data);
      break;
    case DELAY_MS:
      os_sleep_ms(data);
      break;

    //结束
    case LCD_TAB_END:
      goto lcd_table_init_end;
      break;
    default:
      goto lcd_table_init_end;
      break;
    }
  }
lcd_table_init_end:
  return;
}

void lcd_reg_table_init(struct lcdc_device *lcd_dev,uint8 lcd_bus_type,uint8_t *lcd_table){
	uint32 itk = 0;
	uint8  last_opt = -1;
	uint8 buf[32];
	//uint8 code;
	uint8 delay_ms = 0;
	uint8 param_num = 0;

	while(((lcd_table[itk] == LCD_TAB_END) && (lcd_table[itk+1] == LCD_TAB_END)) == 0){
		switch(lcd_table[itk]){
			case LCD_CMD:
				if(last_opt == LCD_CMD){
					if(lcd_bus_type == LCD_BUS_QSPI){
						lcdc_reg_write_data(lcd_dev,4,param_num,buf);
					}
					else{
						lcdc_reg_write_data(lcd_dev,1,param_num,buf);
					}
				}else if(last_opt == DELAY_MS){
					os_sleep_ms(delay_ms);
				}
				last_opt = LCD_CMD;
				//code = lcd_table[itk+1];
				if(lcd_bus_type == LCD_BUS_QSPI){
					buf[0] = 0xde;
					buf[1] = 0x00;
					buf[2] = lcd_table[itk+1];
					buf[3] = 0x00;
					param_num = 0;					
				}else{
					buf[0] = lcd_table[itk+1];
					param_num = 0;
				}
			break;
			case LCD_DAT:
				if(lcd_bus_type == LCD_BUS_QSPI){
					buf[param_num+4] = lcd_table[itk+1];
				}else{
					buf[param_num+1] = lcd_table[itk+1];
				}
				param_num++;
			break;
			case DELAY_MS:
				if(last_opt == LCD_CMD){
					if(lcd_bus_type == LCD_BUS_QSPI){
						lcdc_reg_write_data(lcd_dev,4,param_num,buf);
					}
					else{
						lcdc_reg_write_data(lcd_dev,1,param_num,buf);
					}
				}else if(last_opt == DELAY_MS){				
					os_sleep_ms(delay_ms);
				}
				last_opt = DELAY_MS;
				delay_ms = lcd_table[itk+1];
			break;
			default:
			break;
		}
		itk += 2; 
	}

	if(last_opt == LCD_CMD){
		if(lcd_bus_type == LCD_BUS_QSPI){
			lcdc_reg_write_data(lcd_dev,4,param_num,buf);
		}
		else{
			lcdc_reg_write_data(lcd_dev,1,param_num,buf);
		}
	}else if(last_opt == DELAY_MS){
		os_sleep_ms(delay_ms);
	}	
}



volatile uint32 scale3_num = 1;
uint32 scale_time_buf[10] = {0};
uint32 avg_scale_time = 0;
uint32 new_scale_time = 0;
uint8  scale_b = 0;
uint8 lcd_or_scaler_kick;
uint32 lcdc_time_buf[10] = {0};
uint8  lcdc_b = 0;
uint32 avg_lcdc_time = 0;

uint32 p1_w,p1_h;
uint32 scale_p1_w;
uint32 dec_y_offset,dec_uv_offset;

void scale2_ov_isr(uint32 irq_flag,uint32 irq_data,uint32 param1){
	//struct scale_device *scale_dev = (struct scale_device *)irq_data;
	os_printf("sor2");
}

void scale3_ov_isr(uint32 irq_flag,uint32 irq_data,uint32 param1){
	//struct scale_device *scale_dev = (struct scale_device *)irq_data;
	os_printf("sor3");
}

void scale3_error_pending_isr(uint32 irq_flag,uint32 irq_data,uint32 param1){
	//struct scale_device *scale_dev = (struct scale_device *)irq_data;
	os_printf("serp3");
}

void lcd_timeout(uint32 irq_flag,uint32 irq_data,uint32 param1){
	struct lcdc_device *lcd_dev = (struct lcdc_device *)irq_data;
	lcdc_set_timeout_info(lcd_dev,0,3);
	//gpio_set_val(PC_7,pc_7);
	//pc_7 ^= BIT(0);	
	
	//lcdc_close(lcd_dev);
	os_printf("........................................................................................................................lcd_timeout\r\n");
	//lcdc_open(lcd_dev);
	//lcdc_set_start_run(lcd_dev);
}

void lcd_screen_finish(uint32 irq_flag,uint32 irq_data,uint32 param1){
//	struct lcdc_device *lcd_dev = (struct lcdc_device *)irq_data;
	os_printf("%s  %d\r\n",__func__,__LINE__);
}


void lcd_doublebuf_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	static uint32 lcd_num = 0;
	static uint32 get_lcdc_time = 0;
	static uint32 get_mask = 0; 
	uint32 kick_time;	
	uint8 itk = 0;
//	uint32 jtk = 0;
	
	struct lcdc_device *lcd_dev = (struct lcdc_device *)irq_data;

	if(lcd_info.lcd_osd_mode != lcd_info.lcd_osd_cur_mode){
		lcd_info.lcd_osd_cur_mode = lcd_info.lcd_osd_mode;
		if(lcd_info.lcd_osd_mode){
			lcdc_set_osd_start_location(lcd_dev,0,33);
			lcdc_set_osd_size(lcd_dev,256,240);	
			lcdc_set_osd_lut_addr(lcd_dev,(uint32)osd_palette);
			lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd_encode_buf1+512);	
			lcdc_set_osd_format(lcd_dev,OSD_RGB_256);			
		}else{
			lcdc_set_osd_size(lcd_dev,lcdstruct.osd_w,lcdstruct.osd_h);	
			lcdc_set_osd_format(lcd_dev,OSD_RGB_565);
		}			
	}

	if(lcd_info.lcd_run_new_lcd)
	{
		lcd_info.lcd_run_new_lcd = 0;
		lcdc_set_osd_en(lcd_dev,0);
		//osd0_enable(0);
		if(lcd_info.osd_buf_to_lcd == 0){
			//osd0_set_dma_address(osd_encode_buf);
			lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd_encode_buf);
			//os_printf("s0");
			//gpio_set_val(PA_15,0);
		}else{
			//osd0_set_dma_address(osd_encode_buf1);	
			lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd_encode_buf1);
			//os_printf("s1");
			//gpio_set_val(PA_15,1);
		}
		
		//osd0_len(p_lcd->OSD_ENC_DLEN * 4);
		lcdc_set_osd_dma_len(lcd_dev,lcdc_get_osd_enc_dst_len(lcd_dev));
		lcdc_set_osd_en(lcd_dev,1);
	}
	
	if(lcd_info.lcd_p0p1_state != lcd_info.lcd_p0p1_cur){
		lcd_info.lcd_p0p1_cur = lcd_info.lcd_p0p1_state;
		if(0 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,0,0);
		}else if(1 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,1,0);
		}else if(2 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,0,1);
		}else if(3 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,1,1);
		}
	}

	if(lcd_info.lcd_p1_size_reset){
		lcd_info.lcd_p1_size_reset = 0;
		lcdc_set_rotate_p0p1_size(lcd_dev,SCALE_WIDTH,SCALE_HIGH,p1_w,p1_h);
		lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem+dec_y_offset/**/);
		lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+dec_uv_offset/**/);
		lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);
	}
	
#if (SCALE_DIRECT_TO_LCD == 0)
	if(get_lcdc_time == 0){
		lcdc_time_buf[lcdc_b] = 0;
	}else{
		lcdc_time_buf[lcdc_b] = (os_jiffies()-get_lcdc_time);
	}
		
	get_lcdc_time = os_jiffies();
	lcdc_b++;
	if(lcdc_b == 10)
		lcdc_b = 0;

	
	if(get_mask == 0){
		avg_lcdc_time = 0;
		for(itk = 0;itk < 10;itk++){
			if(lcdc_time_buf[itk] != 0){
				avg_lcdc_time += lcdc_time_buf[itk];
			}
			else{
				itk++;
				break;
			}
		}
		
		if(itk == 10){
			avg_lcdc_time = avg_lcdc_time/itk;
			get_mask = 1;
		}
	}	

	if(((get_lcdc_time+avg_lcdc_time) > (new_scale_time + avg_scale_time)) && (avg_scale_time != 0)){
		kick_time = (get_lcdc_time+avg_lcdc_time) - (new_scale_time + avg_scale_time);
		if(kick_time < (avg_lcdc_time*2/10) ){
			lcd_or_scaler_kick  = 1;	
		}else{
			lcd_or_scaler_kick	= 0;
		}
	}else{
		lcd_or_scaler_kick  = 1;
	}

	if(lcd_or_scaler_kick){	
		if((scale3_num%2) == 0){
			lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)video_psram_mem1+Y_OFFSET);
			lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
			lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);	
			if(lcdc_decode_enable){
				lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem+dec_y_offset/**/);
				lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+dec_uv_offset);
				lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);	
			}
		}else{
			lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)video_psram_mem+Y_OFFSET);
			lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
			lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);	
			if(lcdc_decode_enable){
				lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem+dec_y_offset/**/);
				lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+dec_uv_offset);
				lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);
			}			
		}		
	}
	//os_printf("--");
	if(lcd_or_scaler_kick)
		lcdc_set_start_run(lcd_dev);

#else
	lcdc_set_start_run(lcd_dev);
#endif
	lcd_num++;
		
}


void lcd_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	static uint32 lcd_num = 0;	
//	static uint32 dither = 0;
	uint32_t st0,st1;
	struct lcdc_device *lcd_dev = (struct lcdc_device *)irq_data;

	struct dsi_device *dsi_dev;
	dsi_dev = (struct dsi_device *)dev_get(HG_DSI_DEVID); 
#if 0	
	if(lcd_info.lcd_osd_mode != lcd_info.lcd_osd_cur_mode){
		lcd_info.lcd_osd_cur_mode = lcd_info.lcd_osd_mode;
		if(lcd_info.lcd_osd_mode){
			lcdc_set_osd_start_location(lcd_dev,0,33);
			lcdc_set_osd_size(lcd_dev,256,240);	
			lcdc_set_osd_lut_addr(lcd_dev,(uint32)osd_palette);
			lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd_encode_buf1+512);	
			lcdc_set_osd_format(lcd_dev,OSD_RGB_256);			
		}else{
			lcdc_set_osd_size(lcd_dev,lcdstruct.osd_w,lcdstruct.osd_h);	
			lcdc_set_osd_format(lcd_dev,OSD_RGB_565);
		}			
	}
#endif

	if(lcd_info.lcd_run_new_lcd)
	{
		lcd_info.lcd_run_new_lcd = 0;
		if(lcd_info.osd_buf_to_lcd == 0){
			//osd0_set_dma_address(osd_encode_buf);
			lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd_encode_buf);
			//os_printf("s0");
			//gpio_set_val(PA_15,0);
		}else{
			//osd0_set_dma_address(osd_encode_buf1);	
			lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd_encode_buf1);
			//os_printf("s1");
			//gpio_set_val(PA_15,1);
		}
		
		//osd0_len(p_lcd->OSD_ENC_DLEN * 4);
		//lcdc_set_osd_dma_len(lcd_dev,lcdc_get_osd_enc_dst_len(osdenc_dev));
	}	

	lcd_num++;
#if 0	
	if((lcd_num%200) == 0){
		os_printf("---------------------------------------------------------\r\n");
		if(dither == 1){
			lcdc_dither_en(lcd_dev,0);
			lcdc_screen_start(lcd_dev,1);
			dither = 0;
		}else{
			lcdc_dither_en(lcd_dev,1);
			lcdc_screen_start(lcd_dev,1);
			dither = 1;
		}
	}
#endif	
	if(lcd_info.lcd_p0p1_state != lcd_info.lcd_p0p1_cur){
		lcd_info.lcd_p0p1_cur = lcd_info.lcd_p0p1_state;
		if(0 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,0,0);
		}else if(1 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,1,0);
		}else if(2 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,0,1);
		}else if(3 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,1,1);
		}
	}

	if(lcd_info.lcd_p1_size_reset){
		lcd_info.lcd_p1_size_reset = 0;
		lcdc_set_rotate_p0p1_size(lcd_dev,SCALE_WIDTH,SCALE_HIGH,p1_w,p1_h);
		lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem+dec_y_offset/**/);
		lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+dec_uv_offset/**/);
		lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);
	}
#if (SCALE_DIRECT_TO_LCD == 0)
	if((scale3_num%3) == 1){		
		lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)video_psram_mem2+Y_OFFSET);
		lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)video_psram_mem2+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
		lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)video_psram_mem2+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);
	}else if((scale3_num%3) == 2){		
		lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)video_psram_mem+Y_OFFSET);
		lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
		lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);
	}else{		
		lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)video_psram_mem1+Y_OFFSET);
		lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
		lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);
	}
	
	if(lcdc_decode_enable){
		if((decode_num%3) == 0){			
			lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem2+dec_y_offset/**/);
			lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem2+scale_p1_w*p1_h+dec_uv_offset/**/);
			lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem2+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);
		}else if((decode_num%3) == 1){
			lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem+dec_y_offset/**/);
			lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+dec_uv_offset/**/);
			lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);	
		}else{
			lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem1+dec_y_offset/**/);
			lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem1+scale_p1_w*p1_h+dec_uv_offset/**/);
			lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem1+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);	
		}
	}
	lcdc_set_timeout_info(lcd_dev,1,3);
	lcdc_set_start_run(lcd_dev);
#else
	lcdc_set_start_run(lcd_dev);
#endif
	st0 = mipi_dsi_get_sta0(dsi_dev);
	st1 = mipi_dsi_get_sta1(dsi_dev);	
	if((st0 != 0)||(st1 != 0)){
		printf("L(%08x	%08x)\r\n",st0,st1);
		mipi_dsi_reset_module(dsi_dev);
	}



	//os_printf("lcd..done");
	
}

volatile uint32 dma2d_flash = 0;
void lcd_dma2d_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	static uint32 lcd_num = 0;	
//	static uint32 dither = 0;
	uint32_t st0,st1;
	struct lcdc_device *lcd_dev = (struct lcdc_device *)irq_data;

	struct dsi_device *dsi_dev;
	dsi_dev = (struct dsi_device *)dev_get(HG_DSI_DEVID); 
#if 0	
	if(lcd_info.lcd_osd_mode != lcd_info.lcd_osd_cur_mode){
		lcd_info.lcd_osd_cur_mode = lcd_info.lcd_osd_mode;
		if(lcd_info.lcd_osd_mode){
			lcdc_set_osd_start_location(lcd_dev,0,33);
			lcdc_set_osd_size(lcd_dev,256,240);	
			lcdc_set_osd_lut_addr(lcd_dev,(uint32)osd_palette);
			lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd_encode_buf1+512);	
			lcdc_set_osd_format(lcd_dev,OSD_RGB_256);			
		}else{
			lcdc_set_osd_size(lcd_dev,lcdstruct.osd_w,lcdstruct.osd_h);	
			lcdc_set_osd_format(lcd_dev,OSD_RGB_565);
		}			
	}
#endif

	if(lcd_info.lcd_run_new_lcd)
	{
		lcd_info.lcd_run_new_lcd = 0;
		if(lcd_info.osd_buf_to_lcd == 0){
			//osd0_set_dma_address(osd_encode_buf);
			lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd_encode_buf);
			//os_printf("s0");
			//gpio_set_val(PA_15,0);
		}else{
			//osd0_set_dma_address(osd_encode_buf1);	
			lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd_encode_buf1);
			//os_printf("s1");
			//gpio_set_val(PA_15,1);
		}
		
		//osd0_len(p_lcd->OSD_ENC_DLEN * 4);
		//lcdc_set_osd_dma_len(lcd_dev,lcdc_get_osd_enc_dst_len(osdenc_dev));
	}	

	lcd_num++;

	if(lcd_info.lcd_p0p1_state != lcd_info.lcd_p0p1_cur){
		lcd_info.lcd_p0p1_cur = lcd_info.lcd_p0p1_state;
		if(0 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,0,0);
		}else if(1 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,1,0);
		}else if(2 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,0,1);
		}else if(3 == lcd_info.lcd_p0p1_cur){
			lcdc_set_p0p1_enable(lcd_dev,1,1);
		}
	}

	if(lcd_info.lcd_p1_size_reset){
		lcd_info.lcd_p1_size_reset = 0;
		lcdc_set_rotate_p0p1_size(lcd_dev,SCALE_WIDTH,SCALE_HIGH,p1_w,p1_h);
		lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem+dec_y_offset/**/);
		lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+dec_uv_offset/**/);
		lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);
	}


	if((dma2d_flash%3) == 0){		
		lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)video_psram_mem2+Y_OFFSET);
		lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)video_psram_mem2+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
		lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)video_psram_mem2+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);
	}else if((dma2d_flash%3) == 1){		
		lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)video_psram_mem+Y_OFFSET);
		lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
		lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);
	}else{
		lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)video_psram_mem1+Y_OFFSET);
		lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
		lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);
	}
	
	lcdc_set_timeout_info(lcd_dev,1,3);
	lcdc_set_start_run(lcd_dev);

	st0 = mipi_dsi_get_sta0(dsi_dev);
	st1 = mipi_dsi_get_sta1(dsi_dev);	
	if((st0 != 0)||(st1 != 0)){
		printf("L(%08x	%08x)\r\n",st0,st1);
		mipi_dsi_reset_module(dsi_dev);
	}



	//os_printf("lcd..done");
	
}


volatile uint8 scale2_finish = 1;

void scale2_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	struct jpg_device *jpg_dev;
	struct scale_device *scale_dev = (struct scale_device *)irq_data;
	jpg_dev = (struct jpg_device *)dev_get(HG_JPG1_DEVID);
	if((decode_num%3) == 0){
		scale_set_out_yaddr(scale_dev,(uint32)video_decode_mem1);
		scale_set_out_uaddr(scale_dev,(uint32)video_decode_mem1+scale_p1_w*p1_h);
		scale_set_out_vaddr(scale_dev,(uint32)video_decode_mem1+scale_p1_w*p1_h+scale_p1_w*p1_h/4);
	}else if((decode_num%3) == 1){
		scale_set_out_yaddr(scale_dev,(uint32)video_decode_mem2);
		scale_set_out_uaddr(scale_dev,(uint32)video_decode_mem2+scale_p1_w*p1_h);
		scale_set_out_vaddr(scale_dev,(uint32)video_decode_mem2+scale_p1_w*p1_h+scale_p1_w*p1_h/4);
	}else{
		scale_set_out_yaddr(scale_dev,(uint32)video_decode_mem);
		scale_set_out_uaddr(scale_dev,(uint32)video_decode_mem+scale_p1_w*p1_h);
		scale_set_out_vaddr(scale_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4);
	}

	scale2_finish = 1;
	decode_num++;
	printf("scale2 done\r\n");
}

void scale3_doublebuf_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	struct scale_device *scale_dev = (struct scale_device *)irq_data;
	uint8  itk = 0;
	static uint8_t enlarge_state = 10;
	//struct vpp_device *vpp_dev;
	//vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);

	if(gui_cfg.enlarge_lcd != 0){
		if(gui_cfg.enlarge_lcd != enlarge_state){
			enlarge_state = gui_cfg.enlarge_lcd;
			set_lcd_enlarge_config(gui_cfg.dvp_w,gui_cfg.dvp_h,SCALE_WIDTH,SCALE_HIGH,gui_cfg.enlarge_lcd); 			
		}
	}


#if (SCALE_DIRECT_TO_LCD == 0)
	if(new_scale_time == 0){
		scale_time_buf[scale_b] = 0;
	}else{
		scale_time_buf[scale_b] = (os_jiffies()-new_scale_time);
	}
		
	new_scale_time = os_jiffies();
	scale_b++;
	if(scale_b == 10)
		scale_b = 0;

	avg_scale_time = 0;
	for(itk = 0;itk < 10;itk++){
		if(scale_time_buf[itk] != 0){
			avg_scale_time += scale_time_buf[itk];
		}
		else{
			itk++;
			break;
		}
	}
	avg_scale_time = avg_scale_time/itk;
	
	if((scale3_num%2) == 0){
		//scale3_set_output_addr(SCALE3,video_psram_mem1,video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH,video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
		scale_set_out_yaddr(scale_dev,(uint32)video_psram_mem1);
		scale_set_out_uaddr(scale_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH);
		scale_set_out_vaddr(scale_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
	}else{
		//scale3_set_output_addr(SCALE3,video_psram_mem,video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH,video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4); 
		scale_set_out_yaddr(scale_dev,(uint32)video_psram_mem);
		scale_set_out_uaddr(scale_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH);
		scale_set_out_vaddr(scale_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
	}

	if(lcd_or_scaler_kick == 0){
		if((scale3_num%2) == 0){
			lcdc_set_p0_rotate_y_src_addr(lcd_dev_gobal,(uint32)video_psram_mem+Y_OFFSET);
			lcdc_set_p0_rotate_u_src_addr(lcd_dev_gobal,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
			lcdc_set_p0_rotate_v_src_addr(lcd_dev_gobal,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);	
			if(lcdc_decode_enable){
				lcdc_set_p1_rotate_y_src_addr(lcd_dev_gobal,(uint32)video_decode_mem+DEC_Y_OFFSET/**/);
				lcdc_set_p1_rotate_u_src_addr(lcd_dev_gobal,(uint32)video_decode_mem+SCALE_PHOTO1_CONFIG_W*PHOTO1_H+DEC_UV_OFFSET/**/);
				lcdc_set_p1_rotate_v_src_addr(lcd_dev_gobal,(uint32)video_decode_mem+SCALE_PHOTO1_CONFIG_W*PHOTO1_H+SCALE_PHOTO1_CONFIG_W*PHOTO1_H/4+DEC_UV_OFFSET/**/);
			}
		}else{
			lcdc_set_p0_rotate_y_src_addr(lcd_dev_gobal,(uint32)video_psram_mem1+Y_OFFSET);
			lcdc_set_p0_rotate_u_src_addr(lcd_dev_gobal,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
			lcdc_set_p0_rotate_v_src_addr(lcd_dev_gobal,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);
			if(lcdc_decode_enable){
				lcdc_set_p1_rotate_y_src_addr(lcd_dev_gobal,(uint32)video_decode_mem+DEC_Y_OFFSET/**/);
				lcdc_set_p1_rotate_u_src_addr(lcd_dev_gobal,(uint32)video_decode_mem+SCALE_PHOTO1_CONFIG_W*PHOTO1_H+DEC_UV_OFFSET/**/);
				lcdc_set_p1_rotate_v_src_addr(lcd_dev_gobal,(uint32)video_decode_mem+SCALE_PHOTO1_CONFIG_W*PHOTO1_H+SCALE_PHOTO1_CONFIG_W*PHOTO1_H/4+DEC_UV_OFFSET/**/);
			}
		}
		lcdc_set_start_run(lcd_dev_gobal);		
	}
	

#endif
	scale3_num++;

}

volatile uint32 scale_v3_oe = 0;
extern volatile uint8_t scale3_oe_select;
void scale3_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	struct scale_device *scale_dev = (struct scale_device *)irq_data;
	static uint8_t enlarge_state = 10;
	//struct vpp_device *vpp_dev;
	//vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
//	printf("-------------------------------scale3----------------------------------------------------\r\n");
#if 1
	scale_v3_oe++;
	if(scale_v3_oe%2 == scale3_oe_select){
		return;
	}
#endif
	if(gui_cfg.enlarge_lcd != 0){
		if(gui_cfg.enlarge_lcd != enlarge_state){
			enlarge_state = gui_cfg.enlarge_lcd;
			set_lcd_enlarge_config(gui_cfg.dvp_w,gui_cfg.dvp_h,SCALE_WIDTH,SCALE_HIGH,gui_cfg.enlarge_lcd); 			
		}
	}


#if (SCALE_DIRECT_TO_LCD == 0)
	if((scale3_num%3) == 0){	
		//printf("video_psram_mem:%08x\r\n",video_psram_mem);
		scale_set_out_yaddr(scale_dev,(uint32)video_psram_mem);
		scale_set_out_uaddr(scale_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH);
		scale_set_out_vaddr(scale_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
	}else if((scale3_num%3) == 1){
		//printf("video_psram_mem1:%08x\r\n",video_psram_mem1);
		scale_set_out_yaddr(scale_dev,(uint32)video_psram_mem1);
		scale_set_out_uaddr(scale_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH);
		scale_set_out_vaddr(scale_dev,(uint32)video_psram_mem1+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
	}else{
		//printf("video_psram_mem2:%08x\r\n",video_psram_mem2);
		scale_set_out_yaddr(scale_dev,(uint32)video_psram_mem2);
		scale_set_out_uaddr(scale_dev,(uint32)video_psram_mem2+SCALE_CONFIG_W*SCALE_HIGH);
		scale_set_out_vaddr(scale_dev,(uint32)video_psram_mem2+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
	}
#endif
	scale3_num++;
	printf("sd3");
	//if(scale3_num == 3)
	//	scale_close(scale_dev);
}

extern void scale_dma2d_up();
volatile uint8 dma2d_part = 0;
void scale3_dma2d_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	struct scale_device *scale_dev = (struct scale_device *)irq_data;
	
	if((scale3_num%2) == 1){	
		set_lcd_enlarge_config(640,360,SCALE_WIDTH,SCALE_HIGH,10);
		scale_set_out_yaddr(scale_dev,(uint32)video_dma2d_mem);
		scale_set_out_uaddr(scale_dev,(uint32)video_dma2d_mem+SCALE_CONFIG_W*SCALE_HIGH);
		scale_set_out_vaddr(scale_dev,(uint32)video_dma2d_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
		dma2d_part |= BIT(0);    //dma2d 更新小分辨率部分
	}else if((scale3_num%2) == 0){
		set_lcd_enlarge_config(640,360,320,240,10);
		scale_set_out_yaddr(scale_dev,(uint32)video_dma2d_mem1);
		scale_set_out_uaddr(scale_dev,(uint32)video_dma2d_mem1+320*240);
		scale_set_out_vaddr(scale_dev,(uint32)video_dma2d_mem1+320*240+320*240/4);
		dma2d_part |= BIT(1);    //dma2d 更新大分辨率部分
	}	
	
	scale3_num++;
	printf("Sdm%d",scale3_num);
	scale_dma2d_up();
}

void jpg_decode_done(uint32 irq_flag,uint32 irq_data,uint32 param1,uint32 param2){

}

void jpg_decode_err(uint32 irq_flag,uint32 irq_data,uint32 param1,uint32 param2){

}

void set_lcd_enlarge_config(uint32_t sw,uint32_t sh,uint32_t ow,uint32_t oh,uint8_t enlarge){
	uint32_t x_offset,y_offset;
	uint32_t w_start,h_start;
	//float  w_enlarge,h_enlarge;
	struct scale_device *scale_dev;
	scale_dev = (struct scale_device *)dev_get(HG_SCALE3_DEVID);	
	scale_set_in_out_size(scale_dev,sw,sh,ow,oh);
	scale_set_step(scale_dev,sw,sh,ow*enlarge/10,oh*enlarge/10);
	w_start = ow*enlarge/10;				//实际放大后的大小
	h_start = oh*enlarge/10;	
	x_offset = (w_start - ow)/2;
	y_offset = (h_start - oh)/2;
	scale_set_start_addr(scale_dev,x_offset,y_offset);

}



void set_lcd_photo1_config(uint32 w,uint32 h,uint8 rotate_180){
	extern uint32 p1_w,p1_h;
	extern uint32 scale_p1_w;
	extern uint32 dec_y_offset,dec_uv_offset;
	uint32 flags;
	flags = disable_irq();
	lcd_info.lcd_p1_size_reset = 1;
	p1_w = w;
	p1_h = h;
	scale_p1_w    = ((p1_w+3)/4)*4;
	if(rotate_180){
		dec_y_offset  = p1_w*(p1_h-1);
		dec_uv_offset = ((scale_p1_w/2+3)/4)*4 * (p1_h/2-1);
	}else{
		dec_y_offset  = 0;
		dec_uv_offset = 0;
	}
	enable_irq(flags);
}


void scale_soft_run(uint8_t *softbuf,uint32_t w,uint32 h){
	struct scale_device *scale_dev;
	uint16 icount = 2;	
	scale_dev = (struct scale_device *)dev_get(HG_SCALE3_DEVID);
	//icount	= h/16;
	scale_set_inbuf_num(scale_dev,0,0);
	if(scale_get_inbuf_num(scale_dev) == 0){
		icount = 2;
		//os_printf("new frame...\r\n");
		scale_set_new_frame(scale_dev,1);
	}else{
		return;
	}
	hw_memcpy(yuvbuf,softbuf,(16*icount)*w);
	hw_memcpy(yuvbuf+(16*icount)*w,softbuf+h*w,(16*icount)*w/4);
	hw_memcpy(yuvbuf+(16*icount)*w+(16*icount)*w/4,softbuf+h*w+h*w/4,(16*icount)*w/4);
	
	scale_set_in_yaddr(scale_dev,(uint32)yuvbuf);
	scale_set_in_uaddr(scale_dev,(uint32)yuvbuf+(16*icount)*w);
	scale_set_in_vaddr(scale_dev,(uint32)yuvbuf+(16*icount)*w+(16*icount)*w/4);
	
	scale_set_inbuf_num(scale_dev,icount*16-1,0);    //0~31
	
	while(1){
		//if(scale_get_inbuf_num(scale_dev) == (scale_get_heigh_cnt(scale_dev)+2))
		if(scale_get_heigh_cnt(scale_dev) > ((icount-1)*16) )
		{			
			if((icount%2) == 0){
				//os_printf("get_height_cnt:%d===>up head\r\n",scale_get_heigh_cnt(scale_dev));
				hw_memcpy(yuvbuf,                  softbuf+(16*icount)*w,                            16*w);
				hw_memcpy(yuvbuf+32*w,             softbuf+h*w+(16*icount)*w/4,                    16*w/4);
				hw_memcpy(yuvbuf+32*w+32*w/4,      softbuf+h*w+h*w/4+(16*icount)*w/4,              16*w/4);	
				
			}else{
				//os_printf("get_height_cnt:%d===>up tail\r\n",scale_get_heigh_cnt(scale_dev));
				hw_memcpy(yuvbuf+16*w,                  softbuf+(16*icount)*w,                       16*w);
				hw_memcpy(yuvbuf+32*w+32*w/8,           softbuf+h*w+(16*icount)*w/4,               16*w/4);
				hw_memcpy(yuvbuf+32*w+32*w/4+32*w/8,    softbuf+h*w+h*w/4+(16*icount)*w/4,         16*w/4);	
			}
			icount++;
			if(icount == (h/16)){	
				scale_set_inbuf_num(scale_dev,icount*16,16);
				//os_printf("end frame...\r\n");
				//scale_set_end_frame(scale_dev,1);
				break;
			}
			if((icount%2) == 1)     //3   start
				scale_set_inbuf_num(scale_dev,icount*16,16);
			else
				scale_set_inbuf_num(scale_dev,icount*16,0);
		}
		//os_sleep_ms(1);
	}

	
}

void scale_to_lcd_config_soft(uint8_t *softbuf,uint32_t w,uint32_t h){
	struct scale_device *scale_dev;
	scale_dev = (struct scale_device *)dev_get(HG_SCALE3_DEVID);
	scale_close(scale_dev);
	scale_set_in_out_size(scale_dev,w,h,SCALE_WIDTH,SCALE_HIGH);
	scale_set_step(scale_dev,w,h,SCALE_WIDTH,SCALE_HIGH);
	scale_set_start_addr(scale_dev,0,0);
	scale_set_dma_to_memory(scale_dev,1);
	scale_set_data_from_vpp(scale_dev,0);  
	scale_set_line_buf_num(scale_dev,32);       //soft的line buf

	scale_set_in_yaddr(scale_dev,(uint32)softbuf);
	scale_set_in_uaddr(scale_dev,(uint32)softbuf+w*h);
	scale_set_in_vaddr(scale_dev,(uint32)softbuf+w*h+w*h/4);	

	scale_set_out_yaddr(scale_dev,(uint32)video_psram_mem);
	scale_set_out_uaddr(scale_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH);
	scale_set_out_vaddr(scale_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);

	scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale3_done,(uint32)scale_dev);	
	scale_request_irq(scale_dev,INBUF_OV,(scale_irq_hdl )&scale3_ov_isr,(uint32)scale_dev);
	scale_open(scale_dev);	
}



void jpg_decode_scale_config(){
	struct jpg_device *jpg_dev;
	struct scale_device *scale_dev;
	if(decode_mutex.magic != 0xa8b4c2d5){
//		os_mutex_init(&decode_mutex);
	}

	scale_dev = (struct scale_device *)dev_get(HG_SCALE2_DEVID);
	jpg_dev = (struct jpg_device *)dev_get(HG_JPG1_DEVID);
	scale_set_out_yaddr(scale_dev,(uint32)video_decode_mem);
	scale_set_out_uaddr(scale_dev,(uint32)video_decode_mem+scale_p1_w*p1_h);
	scale_set_out_vaddr(scale_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4);
	scaler2buf = malloc(32+p1_w*2+32*SRAMBUF_WLEN*4);
	if(scaler2buf == NULL){
		return;
	}	
	scale_set_line_buf_addr(scale_dev,(uint32)scaler2buf);
	scale_set_srambuf_wlen(scale_dev,SRAMBUF_WLEN);
	scale_set_start_addr(scale_dev,0,0);
	scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale2_done,(uint32)scale_dev);	
	scale_request_irq(scale_dev,INBUF_OV,(scale_irq_hdl )&scale2_ov_isr,(uint32)scale_dev);
	jpg_request_irq(jpg_dev,(jpg_irq_hdl )&jpg_decode_done,JPG_IRQ_FLAG_JPG_DONE,(void *)jpg_dev);
	jpg_request_irq(jpg_dev,(jpg_irq_hdl )&jpg_decode_err,JPG_IRQ_FLAG_ERROR,(void *)jpg_dev);	
	jpg_decode_target(jpg_dev,1);
	decode_num = 0;
	scale_open(scale_dev);
}

void jpg_dec_scale_del(){
	free(scaler2buf);
}

volatile uint8_t jpg_auto_dec = 0;
void jpg_done_dec_isr(){
	jpg_auto_dec = 1;
	os_printf("%s %d\r\n",__func__,__LINE__);
}

void jpg_dec_error_isr(){
	printf("%s %d\r\n",__func__,__LINE__);
}

void jpg_dec_timeout_isr(uint32 irq_flag,uint32 irq_data,uint32 param1,uint32 param2){
	printf("%s %d\r\n",__func__,__LINE__);
}


void jpg_decode_to_lcd(uint32 photo,uint32 jpg_w,uint32 jpg_h,uint32 video_w,uint32 video_h){
	struct jpg_device *jpg_dev;
	struct scale_device *scale_dev;
	scale_dev = (struct scale_device *)dev_get(HG_SCALE2_DEVID);	
	jpg_dev = (struct jpg_device *)dev_get(HG_JPG1_DEVID);	
	scale_close(scale_dev);
	scale_set_in_out_size(scale_dev,jpg_w,jpg_h,video_w,video_h);
	scale_set_step(scale_dev,jpg_w,jpg_h,video_w,video_h);	
	scale_open(scale_dev);	
	jpg_request_irq(jpg_dev,(jpg_irq_hdl )&jpg_dec_timeout_isr,JPG_IRQ_FLAG_TIME_OUT,jpg_dev);
	jpg_request_irq(jpg_dev,(jpg_irq_hdl )&jpg_dec_error_isr,JPG_IRQ_FLAG_ERROR,jpg_dev);
	jpg_request_irq(jpg_dev,(jpg_irq_hdl )&jpg_done_dec_isr,JPG_IRQ_FLAG_JPG_DONE,jpg_dev);
	jpg_decode_target(jpg_dev,1);
	jpg_decode_photo(jpg_dev,photo,0);
}

int32 jpg_decode_is_finish(){
	struct jpg_device *jpg_dev;
	jpg_dev = (struct jpg_device *)dev_get(HG_JPG1_DEVID);	

	return jpg_is_idle(jpg_dev);
}

void lcd_user_frame(uint32 frame_addr){
	video_decode_mem  = (uint8 *)frame_addr;
	video_decode_mem1 = (uint8 *)frame_addr;
	video_decode_mem2 = (uint8 *)frame_addr;
}

void jpg_user_demo_thread();

uint8_t *dither_linebuf;
static void get_osd_w_h(uint16_t *w,uint16_t *h,uint8_t *rotate)
{
	*w = lcdstruct.osd_w;
	*h = lcdstruct.osd_h; 
	*rotate = lcdstruct.osd_scan_mode;
}
void lcd_module_run(uint16_t *w,uint16_t *h,uint8_t *rotate){
	get_osd_w_h(w,h,rotate);
	uint32_t rbuf[4];
	uint8_t *pt8;
	uint8 pixel_dot_num = 1;
//	uint32_t *osd_sram_fifo;
	struct lcdc_device *lcd_dev;	
	struct spi_device *spi_dev;
	struct scale_device *scale_dev;
//	k_task_handle_t jpg_task_handle;
	struct vpp_device *vpp_dev;
	struct jpg_device *jpg_dev;
	jpg_dev = (struct jpg_device *)dev_get(HG_JPG1_DEVID);		
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);	
	scale_dev = (struct scale_device *)dev_get(HG_SCALE2_DEVID);	
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
	spi_dev = (struct spi_device * )dev_get(HG_SPI0_DEVID);	
	os_mutex_init(&m2m1_mutex);	
#if LCD_FROM_DEC && (LCD_FROM_DMA2D_BUF == 0)
	lcdc_decode_enable = 1;
#endif
	lcd_dev_gobal = lcd_dev;
	lcdc_init(lcd_dev);
	video_psram_mem = video_psram_config_mem;
	video_psram_mem1 = video_psram_config_mem1;
	video_psram_mem2 = video_psram_config_mem2;

	printf("run---video_psram_mem2:%x\r\n",(uint32_t)video_psram_mem2);
	scale_to_lcd_config(640,360);
	
	//csi_kernel_task_new((k_task_entry_t)jpg_user_demo_thread, "jpg_thread", NULL, 25, 0, NULL, 4096, &jpg_task_handle);
	//os_sleep_ms(10);

	
	if(PIN_LCD_RESET != 255){
		gpio_iomap_output(PIN_LCD_RESET,GPIO_IOMAP_OUTPUT); 
		gpio_set_val(PIN_LCD_RESET,0);
		os_sleep_ms(100);
		gpio_set_val(PIN_LCD_RESET,1);
		os_sleep_ms(100);

	}

	
	//mcu屏提前open,需要发送命令
	if(lcdstruct.lcd_bus_type == LCD_BUS_I80)
	{
		lcdc_open(lcd_dev);
		lcdc_clk_gather_select(lcd_dev,1);
	}
	else if(lcdstruct.lcd_bus_type == LCD_BUS_RGB)
	{
		
		uint8 spi_buf[10];
		if(lcdstruct.init_table != NULL){
			spi_open(spi_dev, 1000000, SPI_MASTER_MODE, SPI_WIRE_SINGLE_MODE, SPI_CPOL_1_CPHA_1);
			spi_ioctl(spi_dev,SPI_SET_FRAME_SIZE,9,0);
			//lcd spi_cfg
			_os_printf("read lcd id(%x)\r\n",(unsigned int)spi_dev);
			lcd_register_read_3line(spi_dev,0xda,spi_buf,4);
			_os_printf("***ID:%02x %02x %02x %02x\r\n", spi_buf[0], spi_buf[1], spi_buf[2], spi_buf[3]);
		}
		lcd_table_init(spi_dev,(uint8_t *)lcdstruct.init_table);
		lcdc_init(lcd_dev);
	}


	
	lcdc_set_color_mode(lcd_dev,lcdstruct.color_mode);
	lcdc_set_bus_width(lcd_dev,lcdstruct.bus_width);
	lcdc_set_interface(lcd_dev,lcdstruct.lcd_bus_type);
	lcdc_set_colrarray(lcd_dev,lcdstruct.colrarray);
	pixel_dot_num = lcdstruct.color_mode/lcdstruct.bus_width;
	lcdc_set_lcd_vaild_size(lcd_dev,lcdstruct.screen_w+lcdstruct.hlw+lcdstruct.hbp+lcdstruct.hfp,lcdstruct.screen_h+lcdstruct.vlw+lcdstruct.vbp+lcdstruct.vfp,pixel_dot_num);
	lcdc_set_lcd_visible_size(lcd_dev,lcdstruct.screen_w,lcdstruct.screen_h,pixel_dot_num);

	if(lcdstruct.lcd_bus_type == LCD_BUS_I80)
	{
		lcdc_mcu_signal_config(lcd_dev,lcdstruct.signal_config.value);
	}
	else if((lcdstruct.lcd_bus_type == LCD_BUS_RGB) || (lcdstruct.lcd_bus_type == LCD_BUS_MIPI) || (lcdstruct.lcd_bus_type == LCD_BUS_QSPI))
	{
		lcdc_signal_config(lcd_dev,lcdstruct.vs_en,lcdstruct.hs_en,lcdstruct.de_en,lcdstruct.vs_inv,lcdstruct.hs_inv,lcdstruct.de_inv,lcdstruct.pclk_inv);
		lcdc_set_invalid_line(lcd_dev,lcdstruct.vlw);
		lcdc_set_valid_dot(lcd_dev,lcdstruct.hlw+lcdstruct.hbp,lcdstruct.vlw+lcdstruct.vbp);
		lcdc_set_hlw_vlw(lcd_dev,lcdstruct.hlw,0);
	}


	if(lcdstruct.lcd_bus_type == LCD_BUS_I80)
	{
		lcdc_set_color_mode(lcd_dev,16);
		lcdc_set_bus_width(lcd_dev,8);
		lcdc_set_interface(lcd_dev,lcdstruct.lcd_bus_type);	
		lcdc_signal_config(lcd_dev,lcdstruct.vs_en,lcdstruct.hs_en,lcdstruct.de_en,lcdstruct.vs_inv,lcdstruct.hs_inv,lcdstruct.de_inv,lcdstruct.pclk_inv);
		lcdc_set_baudrate(lcd_dev,lcdstruct.pclk);
		pt8 = (uint8_t *)rbuf;
//		pt8[0] = 1;
//		pt8[1] = 4;
//		pt8[2] = 0xda;
//		lcdc_reg_read_data(lcd_dev,1,4,rbuf);
		lcd_reg_table_init(lcd_dev,lcdstruct.lcd_bus_type,(uint8_t *)(lcdstruct.init_table));	
		lcdc_set_color_mode(lcd_dev,lcdstruct.color_mode);
		lcdc_set_bus_width(lcd_dev,lcdstruct.bus_width);
		lcdc_set_interface(lcd_dev,lcdstruct.lcd_bus_type);			
	}
	else if(lcdstruct.lcd_bus_type == LCD_BUS_RGB)
	{
		//lcd_table_init(spi_dev,(uint8_t *)lcdstruct.init_table);
	}else if(lcdstruct.lcd_bus_type == LCD_BUS_MIPI){
		lcdc_dsi_select_edpi_or_dpi(lcd_dev,0);
	}else if(lcdstruct.lcd_bus_type == LCD_BUS_QSPI){
		lcdc_set_baudrate(lcd_dev,4000000);
		lcdc_clk_gather_select(lcd_dev,1);
		lcdc_cmd_auto_send(lcd_dev,0xde006100,0xde006000);
		lcdc_spi_cs_lock_time(lcd_dev,0xe0,0xe0,0xe0);
		lcdc_qspi_cmd_dw4(lcd_dev,0);
		lcdc_open(lcd_dev);

		lcd_reg_table_init(lcd_dev,lcdstruct.lcd_bus_type,(uint8_t *)(lcdstruct.init_table));
	}else if(lcdstruct.lcd_bus_type == LCD_BUS_SPI4){
		//lcd_spi_cs_action(0x20,0x20,0x20);
		//lcd_spi_cmd_set(0,0);
		//lcd_control_en();
		lcdc_spi_cs_lock_time(lcd_dev,0x20,0x20,0x20);
		lcdc_cmd_auto_send(lcd_dev,0,0);
		lcdc_open(lcd_dev);
		lcd_reg_table_init(lcd_dev,lcdstruct.lcd_bus_type,(uint8_t *)(lcdstruct.init_table));
	}

	lcdc_set_baudrate(lcd_dev,lcdstruct.pclk);
	lcdc_set_bigendian(lcd_dev,1);

	if(lcdstruct.lcd_bus_type == LCD_BUS_MIPI){
		mipi_dsi_init(lcdstruct.screen_w,lcdstruct.screen_h,lcdstruct.pclk,lcdstruct.vlw,lcdstruct.vbp,lcdstruct.vfp,lcdstruct.hlw,lcdstruct.hbp,lcdstruct.hfp,lcdstruct.lane_num,lcdstruct.color_mode);
	}

#if VIDEO_EN
#if SCALE_DIRECT_TO_LCD
	lcdc_set_video_size(lcd_dev,lcdstruct.video_w,lcdstruct.video_h);
	lcdc_set_video_data_from(lcd_dev,VIDEO_FROM_SCALE);
#elif SCALE2_DIRECT_TO_LCD
	lcdc_set_video_size(lcd_dev,lcdstruct.video_w,lcdstruct.video_h);
	lcdc_set_video_data_from(lcd_dev,VIDEO_FROM_SCALE2_DEC);	
#else
	lcdc_set_video_size(lcd_dev,lcdstruct.video_w,lcdstruct.video_h);
#if LCD_SET_ROTATE_180	
	lcdc_set_rotate_mirror(lcd_dev,0,LCD_ROTATE_180);
#if LCD_33_WVGA
	set_lcd_photo1_config(848,480,1);
#else	
	set_lcd_photo1_config(320,240,1);
#endif
#else
	lcdc_set_rotate_mirror(lcd_dev,0,lcdstruct.scan_mode);
#if LCD_33_WVGA
	set_lcd_photo1_config(848,480,0);
#else	
	set_lcd_photo1_config(320,240,0);
#endif	
#endif
	lcdc_set_rotate_p0_up(lcd_dev,0);
	lcdc_set_rotate_p0p1_start_location(lcd_dev,0,0,0,0);
	lcdc_set_rotate_linebuf_num(lcd_dev,LCD_ROTATE_LINE);
	if((lcdstruct.scan_mode == LCD_ROTATE_90) || (lcdstruct.scan_mode == LCD_ROTATE_270)){
		lcdc_set_rotate_linebuf_y_addr(lcd_dev,(uint32)lcd_line_buf);
		lcdc_set_rotate_linebuf_u_addr(lcd_dev,(uint32)lcd_line_buf+SCALE_CONFIG_H*LCD_ROTATE_LINE);
		lcdc_set_rotate_linebuf_v_addr(lcd_dev,(uint32)lcd_line_buf+SCALE_CONFIG_H*LCD_ROTATE_LINE+(SCALE_CONFIG_H/2)*LCD_ROTATE_LINE);
	}else{
		lcdc_set_rotate_linebuf_y_addr(lcd_dev,(uint32)lcd_line_buf);
		lcdc_set_rotate_linebuf_u_addr(lcd_dev,(uint32)lcd_line_buf+SCALE_CONFIG_W*LCD_ROTATE_LINE);
		lcdc_set_rotate_linebuf_v_addr(lcd_dev,(uint32)lcd_line_buf+SCALE_CONFIG_W*LCD_ROTATE_LINE+(SCALE_CONFIG_W/2)*LCD_ROTATE_LINE);	
	}

#if (LCD_FROM_DMA2D_BUF != 0)	
	video_decode_mem  = video_decode_config_mem;
	video_decode_mem1 = video_decode_config_mem1;
	video_decode_mem2 = video_decode_config_mem2;
#endif
	
	lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)video_psram_mem+Y_OFFSET);
	lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+UV_OFFSET);
	lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4+UV_OFFSET);

	if(lcdc_decode_enable){
		lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)video_decode_mem+dec_y_offset/**/);
		lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+dec_uv_offset/**/);
		lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4+dec_uv_offset/**/);	
	}	

#if LCD_FROM_DEC		
	lcdc_set_rotate_p0p1_size(lcd_dev,SCALE_WIDTH,SCALE_HIGH,p1_w,p1_h);
	//jpg_decode_scale_config();	
#else
	lcdc_set_rotate_p0p1_size(lcd_dev,SCALE_WIDTH,SCALE_HIGH,SCALE_WIDTH,SCALE_HIGH);
#endif	
	lcdc_set_video_data_from(lcd_dev,VIDEO_FROM_MEMORY_ROTATE);
#endif

	lcdc_set_video_start_location(lcd_dev,lcdstruct.video_x,lcdstruct.video_y);
	
	lcdc_set_p0p1_enable(lcd_dev,1,lcdc_decode_enable);
	if(lcdc_decode_enable){
		lcd_info.lcd_p0p1_state = 2;
		lcd_info.lcd_p0p1_cur   = 2;
	}
	else{
		lcd_info.lcd_p0p1_state = 0;
		lcd_info.lcd_p0p1_cur   = 0;
	}
	
	lcdc_set_video_en(lcd_dev,1);
#endif

#if OSD_EN
	lcd_info.lcd_osd_mode       = 0;
	lcd_info.lcd_osd_cur_mode	= 0;
	
	lcdc_osd_sram_fifo_enable(lcd_dev,1);
	osd_sram_fifo = os_malloc(512);
	lcdc_osd_sram_fifo_addr(lcd_dev,osd_sram_fifo,osd_sram_fifo+512/4);

	lcdc_set_osd_start_location(lcd_dev,lcdstruct.osd_x,lcdstruct.osd_y);
#if 1	
	lcdc_set_osd_size(lcd_dev,lcdstruct.osd_w,lcdstruct.osd_h);
	lcdc_set_osd_dma_addr(lcd_dev,(uint32)osd565_encode1);   
	lcdc_set_osd_format(lcd_dev,OSD_RGB_565);
#else	
	lcdc_set_osd_size(lcd_dev,24,80);	
	lcdc_set_osd_lut_addr(lcd_dev,osd_rgb256);
	lcdc_set_osd_dma_addr(lcd_dev,osd_rgb256+512);	//no enc
	lcdc_set_osd_format(lcd_dev,OSD_RGB_256);
#endif	
	lcdc_set_osd_alpha(lcd_dev,0x100);
	lcdc_set_osd_enc_cfg(lcd_dev,0xFFFFFF,0x000000);
	lcdc_osd_spec_color_alpha(lcd_dev,0,0xffffff,0x00);
	lcdc_osd_enc_en(lcd_dev,1);
	lcdc_set_osd_en(lcd_dev,1);
	lcdc_osd_trans_en(lcd_dev,1);
#endif

	if((lcdstruct.lcd_bus_type == LCD_BUS_RGB)||(lcdstruct.lcd_bus_type == LCD_BUS_MIPI)||(lcdstruct.lcd_bus_type == LCD_BUS_QSPI)){
		lcdc_clock_alway_on(lcd_dev,1);
	}
	lcdc_rot_burst_dw16(lcd_dev,0);
	
	lcdc_video_enable_auto_ks(lcd_dev,0);
	lcdc_set_timeout_info(lcd_dev,1,3);
#if LCD_THREE_BUF
	lcdc_request_irq(lcd_dev,LCD_DONE_IRQ,(lcdc_irq_hdl )&lcd_done,(uint32)lcd_dev);	
#elif LCD_FROM_DMA2D_BUF
	lcdc_request_irq(lcd_dev,LCD_DONE_IRQ,(lcdc_irq_hdl )&lcd_dma2d_done,(uint32)lcd_dev);
#else
	lcdc_request_irq(lcd_dev,LCD_DONE_IRQ,(lcdc_irq_hdl )&lcd_doublebuf_done,(uint32)lcd_dev);	
#endif
	lcdc_request_irq(lcd_dev,SCREEN_DONE_IRQ,(lcdc_irq_hdl )&lcd_screen_finish,(uint32)lcd_dev);
	lcdc_request_irq(lcd_dev,TIMEOUT_IRQ,(lcdc_irq_hdl )&lcd_timeout,(uint32)lcd_dev);
	lcdc_screen_yuv_addr(lcd_dev,(uint32)video_psram_screen,(uint32)(video_psram_screen+SCALE_CONFIG_W*SCALE_HIGH),(uint32)(video_psram_screen+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4));
	dither_linebuf = malloc(lcdstruct.screen_w*3);
	lcdc_dither_linebuf(lcd_dev,(uint32)dither_linebuf);
	lcdc_open(lcd_dev);
	os_sleep_ms(10);
	lcdc_set_start_run(lcd_dev);
	
#if 0
	set_lcd_photo1_config(800,480,0);
	scale_from_jpeg_config(scale_dev,0,800,480,800,480,10);
	memset(video_decode_config_mem,0x55,133527);
	hw_memcpy(video_psram_config_mem2,jpg_dec,133527);
	jpg_try_dec_cfg(jpg_dev,1);
	jpg_timeout_cfg_enable(jpg_dev,1);
	jpg_timeout_cfg(jpg_dev,(50*DEFAULT_SYS_CLK)/1000);
	jpg_decode_to_lcd(video_psram_config_mem2,800,480,800,480);
#endif	
}


#endif
