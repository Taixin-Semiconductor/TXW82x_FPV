/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "basic_include.h"
#include "lv_port_disp.h"
#include <stdbool.h>
#include "app_lcd/app_lcd.h"
#include "lib/heap/av_psram_heap.h"

#define LV_PORT_DISP_LINE   (32)

/*********************
 *      DEFINES
 *********************/
#if 0
#ifndef MY_DISP_HOR_RES
    #warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen width, default value 320 is used for now.
    #define MY_DISP_HOR_RES    320
#endif

#ifndef MY_DISP_VER_RES
    #warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen height, default value 240 is used for now.
    #define MY_DISP_VER_RES    240
#endif
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
static void disp_flush_rotate(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

static void lv_wait_cb(struct _lv_disp_drv_t * disp_drv)
{
    os_sleep_ms(1);
}
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//        const lv_area_t * fill_area, lv_color_t color);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
///__psram_data uint8_t osd_menu565_buf[SCALE_HIGH*SCALE_WIDTH*2] __aligned(4);
uint8_t *osd_menu565_buf;

//__psram_data static lv_color_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES] __aligned(4);            /*A screen sized buffer*/
//__psram_data static lv_color_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES] __aligned(4);            /*Another screen sized buffer*/
//	  static lv_color_t *buf_3_1; 		   /*A screen sized buffer*/
//	  static lv_color_t *buf_3_2; 		   /*Another screen sized buffer*/

void lv_port_disp_init(void *stream,uint16_t w,uint16_t h,uint8_t rotate)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();
    /*-----------------------------
     * Create a buffer for drawing
     *----------------------------*/
    
    /**
     * LVGL requires a buffer where it internally draws the widgets.
     * Later this buffer will passed to your display driver's `flush_cb` to copy its content to your display.
     * The buffer has to be greater than 1 display row
     *
     * There are 3 buffering configurations:
     * 1. Create ONE buffer:
     *      LVGL will draw the display's content here and writes it to your display
     *
     * 2. Create TWO buffer:
     *      LVGL will draw the display's content to a buffer and writes it your display.
     *      You should use DMA to write the buffer's content to the display.
     *      It will enable LVGL to draw the next part of the screen to the other buffer while
     *      the data is being sent form the first buffer. It makes rendering and flushing parallel.
     *
     * 3. Double buffering
     *      Set 2 screens sized buffers and set disp_drv.full_refresh = 1.
     *      This way LVGL will always provide the whole rendered screen in `flush_cb`
     *      and you only need to change the frame buffer's address.
     */

    /* Example for 1) */
    static lv_disp_draw_buf_t draw_buf_dsc_1;


    /* Example for 2) */
//    static lv_disp_draw_buf_t draw_buf_dsc_2;
//    static lv_color_t buf_2_1[MY_DISP_HOR_RES * 10];                        /*A buffer for 10 rows*/
//    static lv_color_t buf_2_2[MY_DISP_HOR_RES * 10];                        /*An other buffer for 10 rows*/
//    lv_disp_draw_buf_init(&draw_buf_dsc_2, buf_2_1, buf_2_2, MY_DISP_HOR_RES * 10);   /*Initialize the display buffer*/

    /* Example for 3) also set disp_drv.full_refresh = 1 below*/
//    static lv_disp_draw_buf_t draw_buf_dsc_3;
//    static lv_color_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*A screen sized buffer*/
//    static lv_color_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*Another screen sized buffer*/
//	  static lv_color_t *buf_3_1; 		   /*A screen sized buffer*/
//	  static lv_color_t *buf_3_2; 		   /*Another screen sized buffer*/

//	buf_3_1 = (uint8*)malloc_psram(MY_DISP_HOR_RES * MY_DISP_VER_RES*2);
//	buf_3_2 = (uint8*)malloc_psram(MY_DISP_HOR_RES * MY_DISP_VER_RES*2);
//  lv_disp_draw_buf_init(&draw_buf_dsc_3, buf_3_1, buf_3_2,
//                          MY_DISP_VER_RES * LV_VER_RES_MAX);   /*Initialize the display buffer*/

    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/

    static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
    lv_color_t *buf_1;
    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/
    //重新配置一下是否旋转,由屏参数配置
    if(rotate)
    {
        disp_drv.sw_rotate   = 1;
    }
    else
    {
        disp_drv.sw_rotate   = 0;
    }
    disp_drv.rotated           = rotate;

    //如果旋转,就使用单buf
    if(disp_drv.sw_rotate)
    {
        buf_1 = (lv_color_t*)lv_malloc(w*LV_PORT_DISP_LINE*sizeof(lv_color_t));
        lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, w * LV_PORT_DISP_LINE);   /*Initialize the display buffer*/
        //旋转,需要中间层
        osd_menu565_buf = (uint8_t *)lv_malloc(w*h*2);
        disp_drv.flush_cb = disp_flush_rotate;
    }
    else
    {
        //非旋转,直接绘制就好了
        disp_drv.direct_mode = 1;
        buf_1 = (lv_color_t*)lv_malloc(w*h*sizeof(lv_color_t));
        osd_menu565_buf = (uint8_t *)buf_1;
        lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, w * h);   /*Initialize the display buffer*/
        disp_drv.flush_cb = disp_flush;
    }

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = w;
    disp_drv.ver_res = h;
    disp_drv.user_data = stream;

    disp_drv.disp_buf_len = w*h*sizeof(lv_color_t);

    /*Used to copy the buffer's content to the display*/
    
    disp_drv.wait_cb = lv_wait_cb;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc_1;

    /*Required for Example 3)*/
    //disp_drv.full_refresh = 1;

    /* Fill a memory array with a color if you have GPU.
     * Note that, in lv_conf.h you can enable GPUs that has built-in support in LVGL.
     * But if you have a different GPU you can use with this callback.*/
    //disp_drv.gpu_fill_cb = gpu_fill;

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    /*You code here*/
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/*Flush the content of the internal buffer the specific area on the display
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_disp_flush_ready()' has to be called when finished.*/
uint8_t disp_updata = 0;
//不需要旋转
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{

	lv_color_t *p_16;
	p_16 = (lv_color_t*)osd_menu565_buf;

    if(disp_drv->draw_buf->flushing_last)
    {
        sys_dcache_clean_range((uint32_t*)p_16, disp_drv->disp_buf_len);
        disp_updata = 1;
    }
    lv_disp_flush_ready(disp_drv);

}
//旋转部分
static void disp_flush_rotate(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{

	lv_color_t *p_16;
	p_16 = (lv_color_t*)osd_menu565_buf;

    if(area->x1 == 0 && area->x2 == disp_drv->hor_res-1)
    {
        hw_memcpy(p_16+area->y1*disp_drv->hor_res,color_p,disp_drv->hor_res*(area->y2-area->y1+1) *sizeof(lv_color_t));
    }
    else
    {
        uint32_t y;
        for(y = area->y1; y <= area->y2; y++) 
        {
            hw_memcpy(p_16+y*disp_drv->hor_res+area->x1,color_p,(area->x2-area->x1+1)*sizeof(lv_color_t));
            color_p += (area->x2-area->x1+1);
        }
    }
    if(disp_drv->draw_buf->flushing_last)
    {
        sys_dcache_clean_range((uint32_t*)p_16, disp_drv->disp_buf_len);
        disp_updata = 1;
    }
    
    lv_disp_flush_ready(disp_drv);

}






//msi的驱动中间层
#include "basic_include.h"
#include "lib/multimedia/msi.h"

//不需要旋转
static void disp_flush_msi(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{

	lv_color_t *p_16;
	p_16 = (lv_color_t*)osd_menu565_buf;
    if(disp_drv->draw_buf->flushing_last)
    {
            struct msi    *msi = (struct msi*)disp_drv->user_data;
            struct framebuff *fb;
            int count = 0;
            disp_flush_again:
            fb = lvgl_osd_msi_get_fb(msi);
			if(fb)
			{
                struct encode_data_s_callback *callback = (struct encode_data_s_callback*)fb->priv;
                callback->user_data = (void*)disp_drv;
                callback->finish_cb = (osd_finish_cb)lv_disp_flush_ready;

				fb->data = (void*)p_16;
                fb->len = disp_drv->disp_buf_len;

                //回写空间
                sys_dcache_clean_range((uint32_t*)fb->data, disp_drv->disp_buf_len); 
                msi_output_fb(msi,fb);
			}
            //如果其他地方处理慢,要考虑丢帧了
            else
            {
                if(++count%1000 == 0)
                {
                    os_printf("%s drop frame\n",__FUNCTION__);
                }
                os_sleep_ms(1);
                goto disp_flush_again;
            }
    }
    else
    {
        lv_disp_flush_ready(disp_drv);
    }


}
//旋转部分
static void disp_flush_rotate_msi(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{

	lv_color_t *p_16;
	p_16 = (lv_color_t*)osd_menu565_buf;
    if(area->x1 == 0 && area->x2 == disp_drv->hor_res-1)
    {
        hw_memcpy(p_16+area->y1*disp_drv->hor_res,color_p,disp_drv->hor_res*(area->y2-area->y1+1) *sizeof(lv_color_t));
    }
    else
    {
        uint32_t y;
        for(y = area->y1; y <= area->y2; y++) 
        {
            hw_memcpy(p_16+y*disp_drv->hor_res+area->x1,color_p,(area->x2-area->x1+1)*sizeof(lv_color_t));
            color_p += (area->x2-area->x1+1);
        }
    }

    if(disp_drv->draw_buf->flushing_last)
    {
            struct msi    *msi = (struct msi*)disp_drv->user_data;
            struct framebuff *fb;
            int count = 0;
            disp_flush_rotate_msi_again:
            fb = lvgl_osd_msi_get_fb(msi);
			if(fb)
			{
                struct encode_data_s_callback *callback = (struct encode_data_s_callback*)fb->priv;
                callback->user_data = (void*)disp_drv;
                callback->finish_cb = (osd_finish_cb)lv_disp_flush_ready;

				fb->data = (void*)p_16;
                fb->len = disp_drv->disp_buf_len;

                //回写空间
                sys_dcache_clean_range((uint32_t*)fb->data, disp_drv->disp_buf_len); 
                msi_output_fb(msi,fb);
                //lv_disp_flush_ready(disp_drv);
			}
            //如果其他地方处理慢,要考虑丢帧了
            else
            {
                if(++count%1000 == 0)
                {
                    os_printf("%s drop frame\n",__FUNCTION__);
                }
                os_sleep_ms(1);
                goto disp_flush_rotate_msi_again;
            }
    }
    else
    {
        lv_disp_flush_ready(disp_drv);
    }
}

#if DMA2D_EN
#include "hal/dma2d.h"

static void disp_flush_rotate_dma2d_msi(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    int ret = 0;
	lv_color_t *p_16;
	p_16 = (lv_color_t*)osd_menu565_buf;

    static struct dma2d_device *dma2d_dev = NULL;
    if(dma2d_dev == NULL){
        dma2d_dev = (struct dma2d_device *)dev_get(HG_DMA2D_DEVID);
    }
    struct dma2d_blkcpy_param blkcpy;

    sys_dcache_clean_range((uint32_t*)color_p, (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1) * sizeof(lv_color_t));

    // 使用 DMA2D 一次性拷贝整个区域
    memset(&blkcpy, 0, sizeof(struct dma2d_blkcpy_param));
    blkcpy.src_addr = (uint32_t)color_p;
    blkcpy.dst_addr = (uint32_t)(p_16 +  area->y1 * disp_drv->hor_res + area->x1);
    blkcpy.color_mode = DMA2D_COLOR_TYPE_RGB565;
    blkcpy.src_pixel_width = area->x2 - area->x1 + 1;
    blkcpy.dst_pixel_width = disp_drv->hor_res;
    blkcpy.blk_pixel_width = area->x2 - area->x1 + 1;
    blkcpy.blk_pixel_height = area->y2 - area->y1 + 1;
    blkcpy.src_pixel_start_height = 0;
    blkcpy.src_pixel_start_width = 0;
    blkcpy.dst_pixel_start_height = 0;
    blkcpy.dst_pixel_start_width =  0;

    dma2d_blkcpy(dma2d_dev, &blkcpy);
    ret = dma2d_check_status(dma2d_dev);
    if (ret)
    {
        os_printf("DMA2D error during full area copy\n");
    }

    if(disp_drv->draw_buf->flushing_last)
    {
            struct msi    *msi = (struct msi*)disp_drv->user_data;
            struct framebuff *fb;
            int count = 0;
            disp_flush_rotate_msi_again:
            fb = lvgl_osd_msi_get_fb(msi);
			if(fb)
			{
                struct encode_data_s_callback *callback = (struct encode_data_s_callback*)fb->priv;
                callback->user_data = (void*)disp_drv;
                callback->finish_cb = (osd_finish_cb)lv_disp_flush_ready;

				fb->data = (void*)p_16;
                fb->len = disp_drv->disp_buf_len;

                //回写空间
                sys_dcache_clean_range((uint32_t*)fb->data, disp_drv->disp_buf_len); 
                msi_output_fb(msi,fb);
			}
            //如果其他地方处理慢,要考虑丢帧了
            else
            {
                if(++count%1000 == 0)
                {
                    os_printf("%s drop frame\n",__FUNCTION__);
                }
                os_sleep_ms(1);
                goto disp_flush_rotate_msi_again;
            }
    }
    else
    {
        lv_disp_flush_ready(disp_drv);
    }
}
#endif

#include "lib/lvgl_rotate_rpc/lvgl_rotate_msi.h"

#if LVGL_HW_ROTATE_RPC_EN
#include "lib/heap/av_psram_heap.h"

static void disp_flush_cpu1_hw_rotate_msi(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{

	lv_color_t *p_16;
	p_16 = (lv_color_t*)osd_menu565_buf;
    /* CPU1 使用 DMA2D、CPU0 使用 M2M */
    if(area->x1 == 0 && area->x2 == disp_drv->hor_res-1)
    {
        hw_memcpy_no_cache(p_16+area->y1*disp_drv->hor_res,color_p,disp_drv->hor_res*(area->y2-area->y1+1) *sizeof(lv_color_t));
    }
    else
    {
        uint32_t y;
        for(y = area->y1; y <= area->y2; y++) 
        {
            hw_memcpy_no_cache(p_16+y*disp_drv->hor_res+area->x1,color_p,(area->x2-area->x1+1)*sizeof(lv_color_t));
            color_p += (area->x2-area->x1+1);
        }
    }

    if(disp_drv->draw_buf->flushing_last)
    {
        lvgl_rotate_get_active_buff(p_16);
    }
    else
    {
        lv_disp_flush_ready(disp_drv);
    }

}
#endif

void lv_port_disp_init_msi(const char *name,uint16_t w,uint16_t h,uint8_t rotate)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();
    static lv_disp_draw_buf_t draw_buf_dsc_1;

    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/

    static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
    os_memset(&disp_drv, 0, sizeof(lv_disp_drv_t));

    lv_color_t *buf_1;
    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/
    //重新配置一下是否旋转,由屏参数配置
    if(rotate)
    {
        disp_drv.sw_rotate   = 1;
    }
    else
    {
        disp_drv.sw_rotate   = 0;
    }
    disp_drv.rotated           = rotate;

    //初始化msi
    disp_drv.user_data = lvgl_osd_msi(name);

    disp_drv.disp_buf_len = w*h*sizeof(lv_color_t);

    #if LVGL_HW_ROTATE_RPC_EN

    disp_drv.sw_rotate   = 0;
    disp_drv.rotated     = 0;

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = h;
    disp_drv.ver_res = w;

    disp_drv.full_refresh = 1;

    buf_1 = (lv_color_t*)lv_malloc(disp_drv.hor_res*LV_PORT_DISP_LINE*sizeof(lv_color_t));

    lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, disp_drv.hor_res * LV_PORT_DISP_LINE);   /*Initialize the display buffer*/

    osd_menu565_buf = (uint8_t *)av_psram_malloc(w*h*2);
	sys_dcache_invalid_range((uint32_t)osd_menu565_buf, w*h*2);

    disp_drv.flush_cb = disp_flush_cpu1_hw_rotate_msi;

    lvgl_rotate_task_init(disp_drv.user_data, lv_disp_flush_ready, (void*)&disp_drv, disp_drv.hor_res, disp_drv.ver_res, LV_COLOR_DEPTH);

    #else
    //如果旋转,就使用单buf
    if(disp_drv.sw_rotate)
    {
        buf_1 = (lv_color_t*)lv_malloc(w*LV_PORT_DISP_LINE*sizeof(lv_color_t));
        lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, w * LV_PORT_DISP_LINE);   /*Initialize the display buffer*/
        //旋转,需要中间层
        osd_menu565_buf = (uint8_t *)av_psram_malloc(w*h*2);
        sys_dcache_invalid_range((uint32_t*)osd_menu565_buf, w*h*2);
        #if DMA2D_EN
        disp_drv.flush_cb = disp_flush_rotate_dma2d_msi;
        #else
        disp_drv.flush_cb = disp_flush_rotate_msi;
        #endif
    }
    else
    {
        //非旋转,直接绘制就好了
        disp_drv.direct_mode = 1;
        buf_1 = (lv_color_t*)lv_malloc(w*h*sizeof(lv_color_t));
		sys_dcache_invalid_range((uint32_t*)buf_1, w*h*sizeof(lv_color_t));
        osd_menu565_buf = (uint8_t *)buf_1;
        lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, w * h);   /*Initialize the display buffer*/
        disp_drv.flush_cb = disp_flush_msi;
    }

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = w;
    disp_drv.ver_res = h;

    #endif

    /*Used to copy the buffer's content to the display*/
    
    disp_drv.wait_cb = lv_wait_cb;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc_1;

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
}


#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
