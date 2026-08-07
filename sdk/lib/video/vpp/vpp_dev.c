#include "basic_include.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "lib/video/dvp/cmos_sensor/csi_V2.h"
#include "devid.h"
#include "hal/gpio.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "dev/vpp/hgvpp.h"
#include "dev/csi/hgdvp.h"
#include "lib/lcd/lcd.h"
#include "hal/jpeg.h"
#include "lib/video/vpp/vpp_dev.h"
#include "lib/video/gen/gen420_dev.h"
#include "lib/scale/scale_dev.h"
#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include <sys/time.h>
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "hal/osd_enc.h"

#define VPP_MALLOC av_malloc
#define VPP_FREE   av_free
#define VPP_ZALLOC av_zalloc

#define VPP_PSRAM_MALLOC av_psram_malloc
#define VPP_PSRAM_FREE   av_psram_free
#define VPP_PSRAM_ZALLOC av_psram_zalloc


/***************************************
shrink:

//0: 1/2
//1: 1/3
//2: 1/4
//3: 1/6
//4: 2/3
//5: 1/1
***************************************** */
enum SHRINK_ENUM
{
    SHRINK_1_2,
    SHRINK_1_3,
    SHRINK_1_4,
    SHRINK_1_6,
    SHRINK_2_3,
    SHRINK_1_1,
};
struct vpp_cfg_s
{
    uint8_t  vpp_buf0_line_num;
    uint8_t  vpp_buf1_line_num;
    uint8_t  vpp_buf0_mode : 1, vpp_buf1_mode : 1, scale1_from_vpp : 1, scale3_from_vpp : 1, shrink : 3,rev:1;
    uint16_t vpp_w, vpp_h;
    uint16_t vpp_scale_w,vpp_scale_h;
};

// 默认值
struct vpp_cfg_s vpp_msg = {
        .vpp_buf0_line_num = VPP_BUF0_LINEBUF_NUM,
        .vpp_buf1_line_num = VPP_BUF1_LINEBUF_NUM,
        .vpp_buf0_mode     = VPP_BUF0_MODE,
        .vpp_buf1_mode     = VPP_BUF1_MODE,
        .scale1_from_vpp   = SCALE1_FROM_VPPBF,
        .scale3_from_vpp   = SCALE3_FROM_VPPBF,
        .vpp_w             = 1280,
        .vpp_h             = 720,
        .vpp_scale_w       = 0,
        .vpp_scale_h       = 0,
        .shrink            = SHRINK_1_2,
};

enum
{
    VPP_RUNNING  = BIT(0),
    VPP_STOP     = BIT(1), // 硬件是否已经停止
    APP_VPP_STOP = BIT(2), // 应用控制是否停止
};

struct os_event *vpp_evt;
// uint8 motion_detect_buf[9*1024/*((IMAGE_W+31)/32)  * ((IMAGE_H+31)/32) + 3 + 4*((IMAGE_W+31)/32)*/]__attribute__ ((aligned(4)));//加3是为了防止blk数不是word对齐
uint8           *motion_detect_buf;
uint8 *yuvbuf1;
uint8_t *vpp_data1_psram_buf = NULL;

uint8             *yuvbuf;
uint8             *vpp_encode_ipf;
struct video_cfg_t video_msg;

//设置buf1输出的size(需要整除,否则不会修改或者异常)
uint8_t set_vpp_bu1_shrink(uint16_t w, uint16_t shrink_w)
{
    uint8_t ret = 1;
    if(!w)
    {
        return ret;
    }
    if(w*2/3 == shrink_w)
    {
        vpp_msg.shrink = SHRINK_2_3;
        return 0;
    }

    uint8_t shrink_calc = w/shrink_w;
    uint8_t shrink = vpp_msg.shrink;
    //是倍数关系,检查是否有符合
    if(shrink_w * shrink_calc == w)
    {
        ret = 0;
        switch(shrink_calc)
        {
            case 1:
                shrink = SHRINK_1_1;
            break;
            case 2:
                shrink = SHRINK_1_2;
            break;
            case 3:
                shrink = SHRINK_1_3;
            break;
            case 4:
                shrink = SHRINK_1_4;
            break;
            case 6:
                shrink = SHRINK_1_6;
            break;
            default:
                shrink = vpp_msg.shrink;
                ret = 1;
            break;
        }
    }
    vpp_msg.shrink = shrink;
    if(ret)
    {
        os_printf(KERN_ERR"%s err,w:%d\tshrink_w:%d\tshrink:%d\n",__FUNCTION__,w,shrink_w,shrink);
    }
    else
    {
        os_printf(KERN_INFO"%s success,w:%d\tshrink_w:%d\tshrink:%d\n",__FUNCTION__,w,shrink_w,shrink);
    }
    return ret;
}


uint8_t get_vpp_scale_w_h(uint16_t *w, uint16_t *h)
{
    uint8_t ret = 0;
    if (w)
    {
        *w = vpp_msg.vpp_scale_w;
    }

    if (h)
    {
        *h = vpp_msg.vpp_scale_h;
    }
    if(!vpp_msg.vpp_scale_w || !vpp_msg.vpp_scale_h)
    {
        ret = 1; 
    }
    return ret;
}

uint8_t get_vpp_w_h(uint16_t *w, uint16_t *h)
{
    uint8_t ret = 0;
    if (w)
    {
        *w = vpp_msg.vpp_w;
    }

    if (h)
    {
        *h = vpp_msg.vpp_h;
    }
    if(!vpp_msg.vpp_w || !vpp_msg.vpp_h)
    {
        ret = 1; 
    }
    return ret;
}



uint8_t get_vpp1_w_h(uint16_t *w, uint16_t *h)
{
    uint8_t ret = 0;
    switch (vpp_msg.shrink)
    {
        case SHRINK_1_2:
            if (w)
            {
                *w = vpp_msg.vpp_w / 2;
            }
            if (h)
            {
                *h = vpp_msg.vpp_h / 2;
            }
            break;
        case SHRINK_1_3:
            if (w)
            {
                *w = vpp_msg.vpp_w / 3;
            }
            if (h)
            {
                *h = vpp_msg.vpp_h / 3;
            }
            break;
        case SHRINK_1_4:
            if (w)
            {
                *w = vpp_msg.vpp_w / 4;
            }
            if (h)
            {
                *h = vpp_msg.vpp_h / 4;
            }
            break;
        case SHRINK_1_6:
            if (w)
            {
                *w = vpp_msg.vpp_w / 6;
            }
            if (h)
            {
                *h = vpp_msg.vpp_h / 6;
            }
            break;
        case SHRINK_2_3:
            if (w)
            {
                *w = vpp_msg.vpp_w * 2 / 3;
            }
            if (h)
            {
                *h = vpp_msg.vpp_h * 2 / 3;
            }
            break;
        case SHRINK_1_1:
            if (w)
            {
                *w = vpp_msg.vpp_w;
            }
            if (h)
            {
                *h = vpp_msg.vpp_h;
            }
            break;
        default:
            os_printf("%s:%d err,shrink:%d\n", __FUNCTION__, __LINE__, vpp_msg.shrink);
            break;
    }
    os_printf(KERN_INFO"%s:%d\tw:%d\th:%d\n", __FUNCTION__, __LINE__, *w,*h);
    if(VPP_BUF1_EN == 0)
    {
        ret = 1;
    }
    return ret;
}

uint32_t yuv_buf_line(uint8_t which)
{
    ASSERT(which == 0 || which == 1);
    uint8_t mode;
    uint8_t line_num;
    uint8_t malloc_line;
    if (which == 0)
    {
        mode    = vpp_msg.vpp_buf0_mode;
        line_num = vpp_msg.vpp_buf0_line_num;
    }
    else
    {
        mode    = vpp_msg.vpp_buf1_mode;
        line_num = vpp_msg.vpp_buf1_line_num;
    }

    if (mode == VPP_MODE_2N)
    {
        malloc_line = line_num * 2; // 2N
    }
    else
    {
        malloc_line = line_num * 2 + 16; // 16+2N
    }
    return malloc_line;
}

void *get_vpp_buf(uint8_t which)
{
    ASSERT(which == 0 || which == 1);
    if (which == 0)
    {
        return yuvbuf;
    }
    else if (which == 1)
    {
        return yuvbuf1;
    }
    return NULL;
}

void *get_vpp_psram_buf()
{
    return vpp_data1_psram_buf;
}

uint8 photo_lib2[48 * 6] __attribute__((aligned(4))) = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x80, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x0E, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x07, 0x80, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x18, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x3C, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0x00, 0x40, 0x18, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x70, 0x00, 0x01, 0xE0, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x08, 0x00,
        0xE0, 0x08, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x04, 0x00, 0x00, 0x00, 0x01, 0xC0, 0x06, 0x00, 0x00, 0x00, 0x03, 0x82, 0x03, 0x00, 0x00, 0x00, 0x03, 0x83, 0x81, 0x80, 0x00, 0x00, 0x07, 0x03, 0x81,
        0xC0, 0x00, 0x00, 0x0E, 0x03, 0x00, 0xE0, 0x00, 0x00, 0x1C, 0x03, 0x00, 0x70, 0x00, 0x00, 0x3C, 0x03, 0x03, 0x38, 0x00, 0x00, 0x73, 0x83, 0x07, 0x9E, 0x00, 0x00, 0xE1, 0xE3, 0x0F, 0xCF, 0x80,
        0x01, 0xC0, 0xE3, 0x1C, 0x07, 0xF0, 0x03, 0x80, 0x63, 0x60, 0x03, 0xFC, 0x06, 0x00, 0x63, 0x80, 0x00, 0xE0, 0x1C, 0x00, 0x03, 0x00, 0x00, 0x40, 0x30, 0x00, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xC3, 0x3C, 0x00, 0x00, 0x00, 0x07, 0x83, 0x0F, 0x80, 0x00, 0x00, 0x1E, 0x03, 0x03, 0xE0, 0x00, 0x00, 0xFC, 0x03, 0x01, 0xF0, 0x00, 0x00, 0xF0, 0x03, 0x00, 0xF0, 0x00, 0x00, 0x60, 0xFF, 0x00,
        0x70, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x30, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

uint8 ele_lib[13][64] __attribute__((aligned(4))) = {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xF0, 0x0F, 0xF8, 0x1E, 0x3C, 0x3C, 0x3C, 0x3C, 0x1E,
         0x38, 0x1E, 0x78, 0x0E, 0x78, 0x0E, 0x78, 0x0E, 0x78, 0x0E, 0x78, 0x0E, 0x78, 0x0E, 0x78, 0x0E, 0x78, 0x0E, 0x78, 0x0E, 0x38, 0x1E,
         0x3C, 0x1E, 0x3C, 0x1C, 0x1E, 0x3C, 0x1F, 0xF8, 0x07, 0xF0, 0x01, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*"0",0*/
        /* (16 X 32 , 楷体, 加粗 )*/

        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE0, 0x01, 0xE0, 0x03, 0xE0, 0x07, 0xE0, 0x1F, 0xE0,
         0x1D, 0xE0, 0x19, 0xE0, 0x01, 0xE0, 0x01, 0xE0, 0x01, 0xE0, 0x01, 0xE0, 0x01, 0xE0, 0x01, 0xE0, 0x01, 0xE0, 0x01, 0xE0, 0x01, 0xE0,
         0x01, 0xE0, 0x01, 0xE0, 0x01, 0xE0, 0x01, 0xE0, 0x01, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*"1",1*/
        /* (16 X 32 , 楷体, 加粗 )*/

        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x1F, 0xFC, 0x3E, 0x3C, 0x3C, 0x1E, 0x78, 0x1E,
         0x00, 0x1E, 0x00, 0x1E, 0x00, 0x1C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x78, 0x00, 0xF0, 0x00, 0xE0, 0x01, 0xE0, 0x03, 0xC0, 0x07, 0x80,
         0x0F, 0x00, 0x1E, 0x00, 0x3E, 0x00, 0x3F, 0xFE, 0x3F, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*"2",2*/
        /* (16 X 32 , 楷体, 加粗 )*/

        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xF0, 0x0F, 0xFC, 0x1E, 0x3C, 0x3C, 0x1E, 0x38, 0x1E,
         0x18, 0x1E, 0x00, 0x1E, 0x00, 0x1C, 0x00, 0x7C, 0x03, 0xF0, 0x03, 0xF8, 0x00, 0x7C, 0x00, 0x1E, 0x00, 0x1E, 0x00, 0x1E, 0x18, 0x0E,
         0x78, 0x1E, 0x3C, 0x1E, 0x3E, 0x3C, 0x1F, 0xFC, 0x0F, 0xF0, 0x01, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*"3",3*/
        /* (16 X 32 , 楷体, 加粗 )*/

        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x78, 0x00, 0x78, 0x00, 0xF8, 0x01, 0xF8,
         0x01, 0xF8, 0x03, 0xF8, 0x07, 0xB8, 0x0F, 0x38, 0x0E, 0x38, 0x1E, 0x38, 0x3C, 0x38, 0x38, 0x38, 0x78, 0x38, 0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0x00, 0x38, 0x00, 0x38, 0x00, 0x38, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*"4",4*/
        /* (16 X 32 , 楷体, 加粗 )*/

        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xFC, 0x1F, 0xFC, 0x1C, 0x00, 0x1C, 0x00, 0x1C, 0x00,
         0x3C, 0x00, 0x38, 0x00, 0x3F, 0xE0, 0x3F, 0xF8, 0x7C, 0x7C, 0x78, 0x3C, 0x00, 0x1E, 0x00, 0x1E, 0x00, 0x0E, 0x00, 0x0E, 0x30, 0x1E,
         0x70, 0x1E, 0x78, 0x1E, 0x78, 0x7C, 0x3F, 0xF8, 0x1F, 0xF0, 0x07, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*"5",5*/
        /* (16 X 32 , 楷体, 加粗 )*/

        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0xF0, 0x01, 0xE0, 0x03, 0xC0, 0x03, 0xC0,
         0x07, 0x80, 0x0F, 0x00, 0x0F, 0x00, 0x1F, 0xF8, 0x1F, 0xFC, 0x3E, 0x1E, 0x3C, 0x0F, 0x78, 0x0F, 0x78, 0x0F, 0x78, 0x0F, 0x78, 0x0F,
         0x78, 0x0F, 0x3C, 0x0F, 0x3E, 0x1E, 0x1F, 0xFC, 0x0F, 0xF8, 0x01, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*"6",6*/
        /* (16 X 32 , 楷体, 加粗 )*/

        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xFF, 0x7F, 0xFF, 0x00, 0x0F, 0x00, 0x1E, 0x00, 0x1C,
         0x00, 0x3C, 0x00, 0x38, 0x00, 0x78, 0x00, 0x70, 0x00, 0xF0, 0x00, 0xE0, 0x01, 0xE0, 0x01, 0xE0, 0x01, 0xC0, 0x03, 0xC0, 0x03, 0xC0,
         0x03, 0x80, 0x07, 0x80, 0x07, 0x80, 0x07, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*"7",7*/
        /* (16 X 32 , 楷体, 加粗 )*/

        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x1F, 0xF8, 0x3C, 0x3C, 0x38, 0x1E, 0x38, 0x1E,
         0x38, 0x1E, 0x38, 0x1E, 0x3C, 0x3C, 0x3E, 0x7C, 0x0F, 0xF0, 0x1F, 0xF8, 0x3E, 0x7C, 0x7C, 0x1E, 0x78, 0x1E, 0x70, 0x0E, 0x70, 0x0E,
         0x78, 0x0E, 0x78, 0x1E, 0x7C, 0x3E, 0x3F, 0xFC, 0x1F, 0xF8, 0x03, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*"8",8*/
        /* (16 X 32 , 楷体, 加粗 )*/

        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x1F, 0xF8, 0x3E, 0x7C, 0x78, 0x3E, 0x78, 0x1E,
         0x70, 0x1E, 0x70, 0x1E, 0x70, 0x1E, 0x70, 0x1E, 0x78, 0x3C, 0x7C, 0x7C, 0x3F, 0xFC, 0x1F, 0xF8, 0x00, 0xF0, 0x00, 0xF0, 0x01, 0xE0,
         0x01, 0xE0, 0x03, 0xC0, 0x03, 0xC0, 0x07, 0x80, 0x07, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*"9",9*/
        /* (16 X 32 , 楷体, 加粗 )*/

        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x06, 0x00, 0x0E, 0x00, 0x0C, 0x00, 0x1C, 0x00, 0x38,
         0x00, 0x30, 0x00, 0x70, 0x00, 0x60, 0x00, 0xE0, 0x00, 0xC0, 0x01, 0xC0, 0x03, 0x80, 0x03, 0x80, 0x07, 0x00, 0x06, 0x00, 0x0E, 0x00,
         0x0C, 0x00, 0x1C, 0x00, 0x18, 0x00, 0x38, 0x00, 0x70, 0x00, 0x70, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*"/",11*/
        /* (16 X 32 , 楷体, 加粗 )*/

        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*":",12*/
        /* (16 X 32 , 楷体, 加粗 )*/

        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*" ",10*/
                                                                                                                                  /* (16 X 32 , 楷体, 加粗 )*/

};

uint32_t water2_change_ipf(uint16_t srcw, uint16_t srch, uint8_t *bittable, uint16_t bitw, uint16_t bith, uint16_t x, uint16_t y, uint8_t *outbuf)
{
    int      i, j, k;
    int      l_ofset;
    int      r_ofset;
    uint32_t count = 0; // 压缩长度
    int      len;       // 已计算长度
    int      img_w = srcw;
    int      img_h = srch;             // 1280x720图像
    int      bt_w = bitw, bt_h = bith; // 256x32 bit表
    int      pos_x = x;                // 水平居中放置
    int      pos_y = y;                // 垂直居中放置
    uint8_t *ptr;
    uint8_t *optr;
    int      wobuf = 0;
    uint8_t  lb;
    ptr  = (uint8_t *) bittable;
    optr = outbuf;
    len  = pos_y * img_w + pos_x;

    r_ofset = len % 0xfffe;
    l_ofset = len / 0xfffe;
    count   = l_ofset * 6;
    if (optr != NULL)
    {
        for (i = 0; i < l_ofset; i++)
        {
            optr[wobuf++] = 0xff;
            optr[wobuf++] = 0xff;
            optr[wobuf++] = 0xfd;
            optr[wobuf++] = 0xff;
            optr[wobuf++] = 0x00;
            optr[wobuf++] = 0x00;
        }
    }

    for (j = 0; j < bt_h; j++)
    {
        lb = 0;
        for (i = 0; i < bt_w; i++)
        {
            if (ptr[i / 8] & BIT(7 - i % 8))
            { // 1
                if (lb == 0)
                { // 之前有透明度,透明度数据写0
                    if (r_ofset > 2)
                    {
                        count += 6;
                        if (optr != NULL)
                        {
                            optr[wobuf++] = 0xff;
                            optr[wobuf++] = 0xff;
                            optr[wobuf++] = (r_ofset - 1) & 0xff;
                            optr[wobuf++] = ((r_ofset - 1) & 0xff00) >> 8;
                            optr[wobuf++] = 0x00;
                            optr[wobuf++] = 0x00;
                        }
                    }
                    else
                    {
                        count += (r_ofset * 2);
                        if (optr != NULL)
                        {
                            for (k = 0; k < r_ofset; k++)
                            {
                                optr[wobuf++] = 0x00;
                                optr[wobuf++] = 0x00;
                            }
                        }
                    }

                    r_ofset = 1;
                }
                else
                {
                    r_ofset++;
                    if (r_ofset == 0xfffe)
                    {
                        count += 6;
                        if (optr != NULL)
                        {
                            optr[wobuf++] = 0xff;
                            optr[wobuf++] = 0xff;
                            optr[wobuf++] = (r_ofset - 1) & 0xff;
                            optr[wobuf++] = ((r_ofset - 1) & 0xff00) >> 8;
                            optr[wobuf++] = 0xff;
                            optr[wobuf++] = 0xff;
                        }
                        r_ofset = 0;
                    }
                }
                lb = 1;
            }
            else
            { // 0
                if (lb == 1)
                { // 之前无透明度,无透明度数据写0xff,因为是ff,所以都得按格式写
                    count += 6;
                    if (optr != NULL)
                    {
                        optr[wobuf++] = 0xff;
                        optr[wobuf++] = 0xff;
                        optr[wobuf++] = (r_ofset - 1) & 0xff;
                        optr[wobuf++] = ((r_ofset - 1) & 0xff00) >> 8;
                        optr[wobuf++] = 0xff;
                        optr[wobuf++] = 0xff;
                    }
                    r_ofset = 1;
                }
                else
                {
                    r_ofset++;
                    if (r_ofset == 0xfffe)
                    {
                        count += 6;
                        if (optr != NULL)
                        {
                            optr[wobuf++] = 0xff;
                            optr[wobuf++] = 0xff;
                            optr[wobuf++] = (r_ofset - 1) & 0xff;
                            optr[wobuf++] = ((r_ofset - 1) & 0xff00) >> 8;
                            optr[wobuf++] = 0x00;
                            optr[wobuf++] = 0x00;
                        }
                        r_ofset = 0;
                    }
                }
                lb = 0;
            }
        }
        ptr = ptr + bt_w / 8;
        if (lb == 0)
        { // 保持着透明度
            r_ofset += (img_w - bt_w);

            l_ofset = r_ofset / 0xfffe;
            r_ofset = r_ofset % 0xfffe;
            count += (l_ofset * 6);

            if (optr != NULL)
            {
                for (k = 0; k < l_ofset; k++)
                {
                    optr[wobuf++] = 0xff;
                    optr[wobuf++] = 0xff;
                    optr[wobuf++] = 0xfd;
                    optr[wobuf++] = 0xff;
                    optr[wobuf++] = 0x00;
                    optr[wobuf++] = 0x00;
                }
            }

            len += img_w;
        }
        else
        { // 最后一bit不是透明色
            count += 6;
            if (optr != NULL)
            {
                optr[wobuf++] = 0xff;
                optr[wobuf++] = 0xff;
                optr[wobuf++] = (r_ofset - 1) & 0xff;
                optr[wobuf++] = ((r_ofset - 1) & 0xff00) >> 8;
                optr[wobuf++] = 0xff;
                optr[wobuf++] = 0xff;
            }

            r_ofset = (img_w - bt_w);

            len += img_w;
        }
    }

    r_ofset += (img_w * img_h) - len;
    l_ofset = r_ofset / 0xfffe;
    r_ofset = r_ofset % 0xfffe;
    count += (l_ofset * 6);
    count += 6;

    if (optr != NULL)
    {
        for (i = 0; i < l_ofset; i++)
        {
            optr[wobuf++] = 0xff;
            optr[wobuf++] = 0xff;
            optr[wobuf++] = 0xfd;
            optr[wobuf++] = 0xff;
            optr[wobuf++] = 0x00;
            optr[wobuf++] = 0x00;
        }

        optr[wobuf++] = 0xff;
        optr[wobuf++] = 0xff;
        optr[wobuf++] = (r_ofset - 1) & 0xff;
        optr[wobuf++] = ((r_ofset - 1) & 0xff00) >> 8;
        optr[wobuf++] = 0x00;
        optr[wobuf++] = 0x00;
    }

    printf("count:%d\r\n", count);

    return count;
}

void set_time_watermark(struct vpp_device *vpp_dev, uint16 year, uint16 month, uint16 day, uint16 hour, uint16 min, uint16 sec)
{
    vpp_set_watermark0_idx(vpp_dev, 0, year / 1000);
    vpp_set_watermark0_idx(vpp_dev, 1, (year / 100) % 10);
    vpp_set_watermark0_idx(vpp_dev, 2, (year / 10) % 10);
    vpp_set_watermark0_idx(vpp_dev, 3, year % 10);

    vpp_set_watermark0_idx(vpp_dev, 4, 10);

    vpp_set_watermark0_idx(vpp_dev, 5, month / 10);
    vpp_set_watermark0_idx(vpp_dev, 6, month % 10);

    vpp_set_watermark0_idx(vpp_dev, 7, 10);

    vpp_set_watermark0_idx(vpp_dev, 8, day / 10);
    vpp_set_watermark0_idx(vpp_dev, 9, day % 10);

    vpp_set_watermark0_idx(vpp_dev, 10, 12);

    vpp_set_watermark0_idx(vpp_dev, 11, hour / 10);
    vpp_set_watermark0_idx(vpp_dev, 12, hour % 10);
    vpp_set_watermark0_idx(vpp_dev, 13, 11);
    vpp_set_watermark0_idx(vpp_dev, 14, min / 10);
    vpp_set_watermark0_idx(vpp_dev, 15, min % 10);
    vpp_set_watermark0_idx(vpp_dev, 16, 11);
    vpp_set_watermark0_idx(vpp_dev, 17, sec / 10);
    vpp_set_watermark0_idx(vpp_dev, 18, sec % 10);
}

void vpp_reset()
{
    struct vpp_device *vpp_dev;
    vpp_dev = (struct vpp_device *) dev_get(HG_VPP_DEVID);

    vpp_close(vpp_dev);

    os_printf("fv\r\n");

    vpp_open(vpp_dev);
}

void vpp_video_recfg(uint32 dev, uint32_t w, uint32_t h, uint8_t input_from)
{
    struct vpp_device *p_vpp = (struct vpp_device *) dev;

    vpp_set_video_size(p_vpp, w, h);
#if VPP_BUF1_EN
    uint32_t buf1w = 0, buf1h = 0;
    uint8_t  shrink = vpp_msg.shrink; // 0: 1/2
                                      // 1: 1/3
                                      // 2: 1/4
                                      // 3: 1/6
                                      // 4: 2/3
                                      // 5: 1/1

    switch (shrink)
    {
        case 0:
            buf1w = w / 2;
            buf1h = h / 2;
            break;

        case 1:
            buf1w = w / 3;
            buf1h = h / 3;
            break;

        case 2:
            buf1w = w / 4;
            buf1h = h / 4;
            break;

        case 3:
            buf1w = w / 6;
            buf1h = h / 6;
            break;

        case 4:
            buf1w = (w * 2) / 3;
            buf1h = (h * 2) / 3;
            break;

        case 5:
            buf1w = w;
            buf1h = h;
            break;

        default:
            _os_printf("buf1 shrink no this cfg\r\n");
            break;
    }


    // buf1的配置
    {
        uint32_t malloc_line;
        if (vpp_msg.vpp_buf1_mode == VPP_MODE_2N)
        {
            malloc_line = vpp_msg.vpp_buf1_line_num * 2; // 2N
        }
        else
        {
            malloc_line = vpp_msg.vpp_buf1_line_num * 2 + 16; // 16+2N
        }
        vpp_set_buf1_count(p_vpp, vpp_msg.vpp_buf1_line_num);
        vpp_set_buf1_shrink(p_vpp, vpp_msg.shrink);

        vpp_set_buf1_y_addr(p_vpp, (uint32) yuvbuf1);
        vpp_set_buf1_u_addr(p_vpp, (uint32) yuvbuf1 + buf1w * malloc_line);
        vpp_set_buf1_v_addr(p_vpp, (uint32) yuvbuf1 + buf1w * malloc_line + buf1w * (malloc_line / 4));
    }
    vpp_set_buf1_en(p_vpp, 1);

#endif

    // buf0的配置
    {
        uint32_t malloc_line;
        if (vpp_msg.vpp_buf0_mode == VPP_MODE_2N)
        {
            malloc_line = vpp_msg.vpp_buf0_line_num * 2; // 2N
        }
        else
        {
            malloc_line = vpp_msg.vpp_buf0_line_num * 2 + 16; // 16+2N
        }
        vpp_set_buf0_count(p_vpp, vpp_msg.vpp_buf0_line_num);
        vpp_set_buf0_en(p_vpp, 1);

        vpp_set_buf0_y_addr(p_vpp, (uint32) yuvbuf);
        vpp_set_buf0_u_addr(p_vpp, (uint32) yuvbuf + w * malloc_line);
        vpp_set_buf0_v_addr(p_vpp, (uint32) yuvbuf + w * malloc_line + w * (malloc_line / 4));
    }

#if DET_EN
    vpp_set_motion_calbuf(p_vpp, (uint32_t)motion_detect_buf);
    vpp_set_motion_range(p_vpp, 0, 0, w, h);                                       // 检测图像范围,blk大小为32*32个像素点
    vpp_set_motion_blk_threshold(p_vpp, 20);                                       // 检测对应的blk移动的阀值
    vpp_set_motion_frame_threshold(p_vpp, ((w + 31) / 32) * ((h + 31) / 32) / 20); // 检测多少个blk超过阀值，再触发移动检测中断
#endif
    vpp_set_mode(p_vpp, VPP_INPUT_FORMAT);
    vpp_set_input_interface(p_vpp, input_from);
}

uint32            autorc[64] __attribute__((aligned(256)));
volatile uint8    done_num     = 0;
volatile uint8    done_percent = 0; // 0 ~ 100
volatile uint32_t vpp_md_cnt   = 0; // 有移动，则关闭
scale3_kick_fn    scale3_kick_func;

void vpp_hsie_isr(uint32 irq, uint32 dev, uint32 param)
{
}

void vpp_vsie_isr(uint32 irq, uint32 dev, uint32 param)
{
    // uint8_t itk;
    struct vpp_device *p_vpp = (struct vpp_device *) dev;

	struct tm *time_info;
    struct timeval ptimeval;
	gettimeofday(&ptimeval, NULL);
	time_t time_val = (time_t)ptimeval.tv_sec;
    
	time_info = gmtime(&time_val);
	

	uint32_t year	=	time_info->tm_year + 1900;
	uint32_t mon 	=	time_info->tm_mon + 1;
	uint32_t day 	=	time_info->tm_mday;
	uint32_t hour 	= 	time_info->tm_hour;
	uint32_t min 	= 	time_info->tm_min;
	uint32_t sec 	= 	time_info->tm_sec;

    set_time_watermark(p_vpp, year, mon, day, hour, min, sec);
    //_os_printf("V");
    if (video_msg.video_num == 1)
    {
        video_msg.video_type_vpp = ISP_VIDEO_0;
    }
    else if (video_msg.video_num == 2)
    {
        if (video_msg.video_type_cur == ISP_VIDEO_0)
        {
            video_msg.video_type_vpp = ISP_VIDEO_1;
        }
        else
        {
            video_msg.video_type_vpp = ISP_VIDEO_0;
        }
    }
    else if (video_msg.video_num == 3)
    {
        if (video_msg.video_type_cur == ISP_VIDEO_0)
        {
            if (video_msg.video_type_last == ISP_VIDEO_1)
            {
                video_msg.video_type_vpp = ISP_VIDEO_2;
            }
            else
            {
                video_msg.video_type_vpp = ISP_VIDEO_1;
            }
        }
        else
        {
            video_msg.video_type_vpp = ISP_VIDEO_0;
        }
    }

    done_num     = 0;
    done_percent = 0;
}

void vpp_data_done(uint32 irq, uint32 high, uint32 param)
{

    done_num++;
    done_percent = (done_num * 16 * 100) / high;

    // printf("D");
}

uint8_t vpp_video_type_map(uint8_t stype)
{
    uint8_t video_type;
    if (video_msg.video_num == 3)
    {
        if ((video_msg.video_type_vpp + FSTYPE_YUV_P0) == stype)
        { // 如果vpp已经处理当前数据流中，那则返回错误，防止误匹配
            return 0;
        }

        if (video_msg.video_type_cur == ISP_VIDEO_0)
        { // 当前完成的是摄像头0
            if (video_msg.video_type_last == ISP_VIDEO_1)
            {
                video_type = ISP_VIDEO_2; // 即将要处理的是摄像头2
            }
            else
            {
                video_type = ISP_VIDEO_1; // 即将要处理的是摄像头1
            }
        }
        else
        {                             // 当前完成的是摄像头1/2
            video_type = ISP_VIDEO_0; // 即将要处理的是摄像头0
        }
    }
    else if (video_msg.video_num == 2)
    {
        if ((video_msg.video_type_vpp + FSTYPE_YUV_P0) == stype)
        { // 如果vpp已经处理当前数据流中，那则返回错误，防止误匹配
            return 0;
        }

        if (video_msg.video_type_cur == ISP_VIDEO_0)
        {                             // 当前完成的是摄像头0
            video_type = ISP_VIDEO_1; // 即将要处理的是摄像头1
        }
        else
        {
            video_type = ISP_VIDEO_0; // 即将要处理的是摄像头0
        }
    }
    else
    {
        video_type = ISP_VIDEO_0;
    }

    if ((video_type + FSTYPE_YUV_P0) != stype)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void vpp_frame_done(uint32 irq, uint32 dev, uint32 param)
{
    _os_printf(KERN_DEBUG"F");
    if (scale3_kick_func)
    {
        scale3_kick_func();
    }

    if (video_msg.video_num > 1)
    {
        if (video_msg.video_type_cur != ISP_VIDEO_0)
        { // 上次完成是副镜头，这次要处理主镜头
            if (video_msg.dvp_type == 1)
            {
                vpp_video_recfg(dev, video_msg.dvp_iw, video_msg.dvp_ih, VPP_INPUT_FROM);
            }
            else if (video_msg.csi0_type == 1)
            {
                vpp_video_recfg(dev, video_msg.csi0_iw, video_msg.csi0_ih, VPP_INPUT_FROM);
            }
            else if (video_msg.csi1_type == 1)
            {
                vpp_video_recfg(dev, video_msg.csi1_iw, video_msg.csi1_ih, VPP_INPUT_FROM);
            }
        }
        else
        { // 上次完成是主镜头，这次要处理副镜头
            if (video_msg.dvp_type == 2)
            {
                vpp_video_recfg(dev, video_msg.dvp_iw, video_msg.dvp_ih, VPP_INPUT_FROM);
            }
            else if (video_msg.csi0_type == 2)
            {
                vpp_video_recfg(dev, video_msg.csi0_iw, video_msg.csi0_ih, VPP_INPUT_FROM);
            }
            else if (video_msg.csi1_type == 2)
            {
                vpp_video_recfg(dev, video_msg.csi1_iw, video_msg.csi1_ih, VPP_INPUT_FROM);
            }
        }
    }

    // 判断是否需要关闭vpp,如果需要关闭vpp则去自动关闭
    if (vpp_evt)
    {
        int      evt_ret = 0;
        uint32_t flags   = 0;
        evt_ret          = os_event_get(vpp_evt, &flags);
        // 识别到需要应用关闭,那么就要去关闭vpp_dev
        if (!evt_ret && (flags & APP_VPP_STOP))
        {
            os_event_set(vpp_evt, VPP_STOP, NULL);
            vpp_close((struct vpp_device *)dev);
        }
    }
}
volatile uint8 itp_done = 0;
void           vpp_itp_done(uint32 irq, uint32 dev, uint32 param)
{
    os_printf("ITP_finish..\r\n");
    itp_done = 1;
}

void vpp_itp_error(uint32 irq, uint32 dev, uint32 param)
{
    os_printf("ITP_ER..\r\n");
}

void vpp_lib_error(uint32 irq, uint32 dev, uint32 param)
{
    //	dvp_vpp_reset();
    os_printf("LIB_ER..\r\n");
}
extern volatile uint32 outbuff_isr[2];
void                   vpp_ipf_error(uint32 irq, uint32 dev, uint32 param)
{
    //	vpp_reset();
    os_printf("IPF ERR..\r\n");
    //	outbuff_isr[0] = 0xff;
}

void vpp_md_find(uint32 irq, uint32 dev, uint32 param)
{
    //_os_printf("MDHP..");
    //_os_printf(KERN_DEBUG"?");
    vpp_md_cnt++;
}

void vpp_itp_save_only(struct vpp_device *p_vpp, uint16_t w, uint16_t h, uint32_t psram_adr)
{
    vpp_set_itp_y_addr(p_vpp, psram_adr);
    vpp_set_itp_u_addr(p_vpp, psram_adr + w * h);
    vpp_set_itp_v_addr(p_vpp, psram_adr + w * h + w * h / 4);
    vpp_set_itp_auto_close(p_vpp, 0);
    vpp_set_itp_enable(p_vpp, 1);
}

// uint8 yuv_staic_buf[1280*32 + 1280*16]__attribute__ ((aligned(4),section(".usersram2.src")));;
bool vpp_cfg(uint32_t w, uint32_t h, uint8_t input_from)
{
    vpp_msg.vpp_w = w;
    vpp_msg.vpp_h = h;
    if(!w || !h)
    {
        os_printf(KERN_ERR"vpp_cfg err:w=%d,h=%d\r\n",w,h);
        return FALSE;
    }
    uint32_t             len;
    struct vpp_device   *vpp_dev;
    struct scale_device *scale_dev;
    vpp_dev   = (struct vpp_device *) dev_get(HG_VPP_DEVID);
    scale_dev = (struct scale_device *) dev_get(HG_SCALE1_DEVID);
#if IPF_EN
    struct osdenc_device *osdenc_dev;
    osdenc_dev = (struct osdenc_device *) dev_get(HG_OSD_ENC_DEVID);
    osd_enc_open(osdenc_dev);
    osd_enc_tran_config(osdenc_dev, 0xFFFFFF, 0xFFFFFF, 0x000000, 0x000000);
#endif

#if SCEN_EN
    vpp_set_video_size(vpp_dev, w * 2, h * 2);
#else
    os_printf("csi dvp_size_set\r\n");
    vpp_set_video_size(vpp_dev, w, h);
#endif

    vpp_set_ycbcr(vpp_dev, YUV_MODE);

#if (ONLY_Y == 1)
    vpp_dis_uv_mode(vpp_dev, 1, 1);
#endif

    vpp_set_threshold(vpp_dev, 0x00000000, 0x00ffffff);

    vpp_set_sram_buf_mode(vpp_dev, vpp_msg.vpp_buf0_mode, vpp_msg.vpp_buf1_mode);
    vpp_set_scale_buf_select(vpp_dev, vpp_msg.scale1_from_vpp, vpp_msg.scale3_from_vpp);

#if VPP_BUF1_EN
    uint32_t buf1w = 0, buf1h = 0;
    uint8_t  shrink = vpp_msg.shrink; // 0: 1/2
                                      // 1: 1/3
                                      // 2: 1/4
                                      // 3: 1/6
                                      // 4: 2/3
                                      // 5: 1/1

    switch (shrink)
    {
        case 0:
            buf1w = w / 2;
            buf1h = h / 2;
            break;

        case 1:
            buf1w = w / 3;
            buf1h = h / 3;
            break;

        case 2:
            buf1w = w / 4;
            buf1h = h / 4;
            break;

        case 3:
            buf1w = w / 6;
            buf1h = h / 6;
            break;

        case 4:
            buf1w = (w * 2) / 3;
            buf1h = (h * 2) / 3;
            break;

        case 5:
            buf1w = w;
            buf1h = h;
            break;

        default:
            _os_printf("buf1 shrink no this cfg\r\n");
            break;
    }
#if (PSRAM_FRAME_SAVE == 0)
    // buf1的配置
    {
        uint32_t malloc_line;
        if (vpp_msg.vpp_buf1_mode == VPP_MODE_2N)
        {
            malloc_line = vpp_msg.vpp_buf1_line_num * 2; // 2N
        }
        else
        {
            malloc_line = vpp_msg.vpp_buf1_line_num * 2 + 16; // 16+2N
        }

        if (yuvbuf1 == NULL)
        {
            yuvbuf1 = VPP_MALLOC(buf1w * malloc_line + buf1w * (malloc_line / 2));
            if (yuvbuf1 == NULL)
            {
                _os_printf("no room yuvbuf1\r\n");
                return FALSE;
            }
        }
        vpp_set_buf1_y_addr(vpp_dev, (uint32) yuvbuf1);
        vpp_set_buf1_u_addr(vpp_dev, (uint32) yuvbuf1 + buf1w * malloc_line);
        vpp_set_buf1_v_addr(vpp_dev, (uint32) yuvbuf1 + buf1w * malloc_line + buf1w * (malloc_line / 4));
        vpp_set_buf1_count(vpp_dev, vpp_msg.vpp_buf1_line_num);
    }
#else
    uint16_t p_w, p_h;
    p_w = buf1w;
    p_h = buf1h;
    //	vpp_set_sram_buf_mode(vpp_dev,1,1);
    //	vpp_set_scale_buf_select(vpp_dev,1,1);
    vpp_set_nosram_buf_enable(vpp_dev, 2);
    //	vpp_set_nosram_buf1_enable(vpp_dev,1);

    vpp_set_psram_ycnt(vpp_dev, p_w, p_h);
    vpp_set_psram_uvcnt(vpp_dev, p_w, p_h);

    //	vpp_set_psram1_ycnt(vpp_dev,w/2,h/2);
    //	vpp_set_psram1_uvcnt(vpp_dev,w/2,h/2);

    //	vpp_set_buf1_y_addr(vpp_dev,(uint32)psram_all_frame_buf1);
    //	vpp_set_buf1_u_addr(vpp_dev,(uint32)psram_all_frame_buf1+w*h/4);
    //	vpp_set_buf1_v_addr(vpp_dev,(uint32)psram_all_frame_buf1+w*h/4+w*h/16);
    if(vpp_data1_psram_buf)
    {
        VPP_PSRAM_FREE(vpp_data1_psram_buf);
        vpp_data1_psram_buf = NULL;
    }
    vpp_data1_psram_buf = (uint8_t*)VPP_PSRAM_MALLOC(p_w*p_h*3/2);
    ASSERT(vpp_data1_psram_buf);

    vpp_set_buf1_y_addr(vpp_dev, (uint32) vpp_data1_psram_buf);
    vpp_set_buf1_u_addr(vpp_dev, (uint32) vpp_data1_psram_buf + p_w * p_h);
    vpp_set_buf1_v_addr(vpp_dev, (uint32) vpp_data1_psram_buf + p_w * p_h + p_w * p_h / 4);
#endif
    vpp_set_buf1_shrink(vpp_dev, vpp_msg.shrink);
    vpp_set_buf1_en(vpp_dev, 1);
#endif
    // buf0 的配置
    {
        uint32_t malloc_line;
        if (vpp_msg.vpp_buf0_mode == VPP_MODE_2N)
        {
            malloc_line = vpp_msg.vpp_buf0_line_num * 2; // 2N
        }
        else
        {
            malloc_line = vpp_msg.vpp_buf0_line_num * 2 + 16; // 16+2N
        }

        if (yuvbuf == NULL)
        {
            yuvbuf = (uint8_t*)VPP_MALLOC(w * malloc_line + w * (malloc_line / 2));
            if (yuvbuf == NULL)
            {
                _os_printf("no room yuvbuf0\r\n");
                return FALSE;
            }
        }
        vpp_set_buf0_count(vpp_dev, vpp_msg.vpp_buf0_line_num);
        vpp_set_buf0_en(vpp_dev, 1);
#if (ONLY_Y == 1)
        vpp_set_buf0_y_addr(vpp_dev, (uint32) psram_buf);
#else
        vpp_set_buf0_y_addr(vpp_dev, (uint32) yuvbuf);
        vpp_set_buf0_u_addr(vpp_dev, (uint32) yuvbuf + w * malloc_line);
        vpp_set_buf0_v_addr(vpp_dev, (uint32) yuvbuf + w * malloc_line + w * (malloc_line / 4));
#endif
    }

    vpp_set_water0_color(vpp_dev, 0xff, 0x80, 0x80);
    vpp_set_water0_bitmap(vpp_dev, (uint32) ele_lib);
    vpp_set_water0_locate(vpp_dev, 8, 16);
    vpp_set_water0_contrast(vpp_dev, 0);
    vpp_set_watermark0_charsize_and_num(vpp_dev, 16, 32, 19);
    vpp_set_watermark0_mode(vpp_dev, 1);
    set_time_watermark(vpp_dev, 2222, 22, 22, 22, 22, 22);
    vpp_set_water0_rc(vpp_dev, 0);

    vpp_set_watermark0_auto_rc_sram_adr(vpp_dev, (uint32_t) autorc);
    vpp_set_watermark0_auto_rc_threshold(vpp_dev, 16 * 32 * 128, 16 * 32 * 32);
    vpp_set_watermark_auto_rc_mode(vpp_dev, 0); // double sensor
    vpp_set_watermark0_auto_rc(vpp_dev, 1, 0x00, 0x80, 0x80);

#if IPF_EN
    len            = water2_change_ipf(w, h, photo_lib2, 48, 48, 100, 300, NULL);
    vpp_encode_ipf = (uint8_t*)VPP_MALLOC(len);
    water2_change_ipf(w, h, photo_lib2, 48, 48, 100, 300, vpp_encode_ipf);
    vpp_set_ifp_addr(vpp_dev, (uint32_t) vpp_encode_ipf);
#endif

#if 0	
	vpp_set_water1_color(vpp_dev,0xff,0x80,0x80);
	vpp_set_water1_bitmap(vpp_dev,(uint32)photo_lib2);
	vpp_set_water1_locate(vpp_dev,45,30);
	vpp_set_water1_contrast(vpp_dev,0);
//	vpp_set_watermark1_size(vpp_dev,224,48);
	vpp_set_watermark1_size(vpp_dev,48,48);
	vpp_set_watermark1_mode(vpp_dev,0);
	vpp_set_water1_rc(vpp_dev,0);
#endif

    vpp_request_irq(vpp_dev, HSIE_ISR, (vpp_irq_hdl) &vpp_hsie_isr, (uint32) vpp_dev);
    vpp_request_irq(vpp_dev, VSIE_ISR, (vpp_irq_hdl) &vpp_vsie_isr, (uint32) vpp_dev);
    vpp_request_irq(vpp_dev, FRAME_DONE_ISR, (vpp_irq_hdl) &vpp_frame_done, (uint32) vpp_dev);
    vpp_request_irq(vpp_dev, SCIE_ISR, (vpp_irq_hdl) &vpp_data_done, h);
    vpp_request_irq(vpp_dev, LOVIE_ISR, (vpp_irq_hdl) &vpp_lib_error, (uint32) vpp_dev);
    vpp_request_irq(vpp_dev, IPF_OV_ISR, (vpp_irq_hdl) &vpp_ipf_error, (uint32) vpp_dev);
    vpp_request_irq(vpp_dev, MDPIE_ISR, (vpp_irq_hdl) &vpp_md_find, (uint32) vpp_dev);
    vpp_request_irq(vpp_dev, ITP_OV_ISR, (vpp_irq_hdl) &vpp_itp_error, (uint32) vpp_dev);
    vpp_request_irq(vpp_dev, ITP_DONE_ISR, (vpp_irq_hdl) &vpp_itp_done, (uint32) vpp_dev);

#if DET_EN
    motion_detect_buf = (uint8_t*)VPP_MALLOC(((w + 31) / 32) * ((h + 31) / 32) + 4 * ((w + 31) / 32));
    vpp_set_motion_calbuf(vpp_dev, (uint32_t)motion_detect_buf);
    vpp_set_motion_range(vpp_dev, 0, 0, w, h);                                       // 检测图像范围,blk大小为32*32个像素点
    vpp_set_motion_blk_threshold(vpp_dev, 10);                                       // 检测对应的blk移动的阀值
    vpp_set_motion_frame_threshold(vpp_dev, 10); // 检测多少个blk超过阀值，再触发移动检测中断
#endif
    vpp_set_mode(vpp_dev, VPP_INPUT_FORMAT);
    vpp_set_input_interface(vpp_dev, input_from);

    vpp_set_watermark0_enable(vpp_dev, 1);
    //	vpp_set_watermark1_enable(vpp_dev,1);

#if IPF_EN
    vpp_set_ifp_en(vpp_dev, 1);
#endif
#if DET_EN
    vpp_set_motion_det_enable(vpp_dev, 1);
#endif

#if VPP_SCALE_EN
    vpp_msg.vpp_scale_w = VPP_SCALE_WIDTH;
    vpp_msg.vpp_scale_h = VPP_SCALE_HIGH;
    scale_from_vpp(scale_dev, yuvbuf, w, h, VPP_SCALE_WIDTH, VPP_SCALE_HIGH);
#endif

    vpp_open(vpp_dev);
    return TRUE;
}

void vpp_evt_init()
{
    vpp_evt = (struct os_event *) os_malloc(sizeof(struct os_event));
    os_event_init(vpp_evt);
}

void vpp_test(uint32_t line)
{
    uint32_t flags = 0;
    os_event_wait(vpp_evt, APP_VPP_STOP, &flags, OS_EVENT_WMODE_OR, 0);
    os_printf("flags[%d]:%X\n", line, flags);
}

int32_t vpp_evt_wait_open(int enable, int timeout)
{
    int32_t  ret = 0;
    uint32_t flags;
    if (timeout && timeout < 50)
    {
        timeout = 50;
    }

    // 如果没有初始化,默认返回成功
    if (!vpp_evt)
    {
        os_printf("%s:%d\n", __FUNCTION__, __LINE__);
        return 0;
    }
    // 等待关闭vpp
    if (enable == 0)
    {
        os_event_set(vpp_evt, APP_VPP_STOP, NULL);
        ret = os_event_wait(vpp_evt, VPP_STOP, &flags, OS_EVENT_WMODE_OR, timeout);
        if (!ret && (flags & VPP_STOP))
        {
        }
        // 可能是超时了,应该清除APP_VPP_STOP
        // 有概率出现超时后vpp被关闭?这里没有去处理
        else
        {
            // os_event_wait(vpp_evt, APP_VPP_STOP, &flags, OS_EVENT_WMODE_OR|OS_EVENT_WMODE_CLEAR, 0);
            ret = -1;
        }
    }
    // 启动vpp
    else
    {
        struct vpp_device *vpp_dev;
        vpp_dev = (struct vpp_device *) dev_get(HG_VPP_DEVID);
        // 清除stop状态
        os_event_wait(vpp_evt, APP_VPP_STOP | VPP_STOP, &flags, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
        // 如果没有被停止,则不管
        if (flags & VPP_STOP)
        {
            // 将vpp打开
            vpp_open(vpp_dev);
        }
    }

    return ret;
}

// 如果遇到异常或者重新初始化vpp,需要调用一下这个接口
void vpp_evt_reset()
{
    if (vpp_evt)
    {
        os_event_wait(vpp_evt, 0xff, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
    }
}

bool vpp_cfg_release()
{
    struct vpp_device *p_vpp = (struct vpp_device *) dev_get(HG_VPP_DEVID);
    vpp_close(p_vpp);
    if (yuvbuf)
    {
        VPP_FREE(yuvbuf);
        yuvbuf = NULL;
    }
#if VPP_BUF1_EN
    if (yuvbuf1)
    {
        VPP_FREE(yuvbuf1);
        yuvbuf1 = NULL;
    }
#endif

    if(vpp_data1_psram_buf)
    {
        VPP_PSRAM_FREE(vpp_data1_psram_buf);
        vpp_data1_psram_buf = NULL;
    }
    return TRUE;
}
