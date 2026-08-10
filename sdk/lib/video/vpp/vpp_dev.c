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
#include "lib/multimedia/msi.h"
#include "lib/common/timezone.h"
#include "basic_include.h"
#include <sys/time.h>
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "hal/osd_enc.h"

uint32 motion_det_pot_check(uint8_t *old_y, uint8_t *new_y, uint8_t *copy, uint16 w, uint16 h, uint8_t blk_thd, uint8_t md_blk_num);
void   md_set_pot_x_y(ISP_VIDEO_E sensor_id, uint16 x, uint16 y);

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
    uint16_t vpp_buf0_mode : 1, vpp_buf1_mode : 1, scale1_from_vpp : 1, scale3_from_vpp : 1, shrink : 3, double_psram_for_buf1 : 1, vpp_buf0_en : 1, vpp_buf1_en : 1, vpp_buf1_in_psram : 1, mdt : 1,
            rev : 4;
    uint16_t vpp_w, vpp_h;
    uint16_t vpp_scale_w, vpp_scale_h;
};

// 默认值
#ifdef SYS_APP_WALKIE_TALKIE
struct vpp_cfg_s vpp_msg = {
        .vpp_buf0_line_num     = VPP_BUF0_LINEBUF_NUM,
        .vpp_buf1_line_num     = VPP_BUF1_LINEBUF_NUM,
        .vpp_buf0_mode         = VPP_BUF0_MODE,
        .vpp_buf1_mode         = VPP_BUF1_MODE,
        .scale1_from_vpp       = SCALE1_FROM_VPPBF,
        .scale3_from_vpp       = SCALE3_FROM_VPPBF,
        .vpp_w                 = 640,
        .vpp_h                 = 480,
        .vpp_scale_w           = 0,
        .vpp_scale_h           = 0,
        .shrink                = SHRINK_1_2,
        .double_psram_for_buf1 = 0,
        .mdt                   = 0,
};
#else
struct vpp_cfg_s vpp_msg = {
        .vpp_buf0_line_num     = VPP_BUF0_LINEBUF_NUM,
        .vpp_buf1_line_num     = VPP_BUF1_LINEBUF_NUM,
        .vpp_buf0_mode         = VPP_BUF0_MODE,
        .vpp_buf1_mode         = VPP_BUF1_MODE,
        .scale1_from_vpp       = SCALE1_FROM_VPPBF,
        .scale3_from_vpp       = SCALE3_FROM_VPPBF,
        .vpp_w                 = 1280,
        .vpp_h                 = 720,
        .vpp_scale_w           = 0,
        .vpp_scale_h           = 0,
        .shrink                = SHRINK_1_2,
        .double_psram_for_buf1 = 0,
        .mdt                   = 0,
};
#endif

// uint8 motion_detect_buf[9*1024/*((IMAGE_W+31)/32)  * ((IMAGE_H+31)/32) + 3 + 4*((IMAGE_W+31)/32)*/]__attribute__ ((aligned(4)));//加3是为了防止blk数不是word对齐

uint8 *motion_detect_buf         = NULL;
uint8 *mdet_oldframe_sensor1_buf = NULL;
uint8 *mdet_oldframe_sensor2_buf = NULL;

uint8 *mdet_result_sensor1 = NULL;
uint8 *mdet_result_sensor2 = NULL;

uint16             motion_blk_threshold    = 0;
uint16             motion_blknum_threshold = 0;
static uint8      *yuvbuf1                 = NULL;
uint8_t           *vpp_data1_psram_buf     = NULL;
uint8_t           *vpp_data2_psram_buf     = NULL;
volatile uint8_t  *psram_ptr               = NULL;
volatile uint8_t  *psram_user_ptr          = NULL;
volatile uint32_t  photo_complex           = 0;
static uint8      *yuvbuf                  = NULL;
uint8             *vpp_encode_ipf          = NULL;
struct video_cfg_t video_msg;
func_done_fn       vpp_deal_dev_func_table[VPP_FUNC_DONE_NUM];
volatile uint32    vpp_deal_dev_func_arg_table[VPP_FUNC_DONE_NUM];

// 返回0,代表成功
// buf0和buf1的参数,一定要先配置buf0才能配置buf1
int32_t vpp_set_buf_msg(uint8_t which_buf, uint8_t en, uint8_t in_psram, uint16_t w, uint16_t h)
{
    int32_t ret = 0;
    uint8_t shrink_calc;
    uint8_t shrink;
    if (which_buf == 0)
    {
        vpp_msg.vpp_buf0_en = en;
        vpp_msg.vpp_w       = w;
        vpp_msg.vpp_h       = h;
    }
    else if (which_buf == 1)
    {

        if (en)
        {
            if (vpp_msg.vpp_w * 2 / 3 == w)
            {
                shrink = SHRINK_2_3;
            }
            else
            {
                shrink_calc = vpp_msg.vpp_w / w;
                shrink      = vpp_msg.shrink;

                // 是倍数关系,检查是否有符合
                if (w * shrink_calc == vpp_msg.vpp_w)
                {
                    ret = 0;
                    switch (shrink_calc)
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
                            ret    = 1;
                            break;
                    }
                }
            }
            vpp_msg.shrink = shrink;
            if (!ret)
            {
                vpp_msg.vpp_buf1_en       = en;
                vpp_msg.vpp_buf1_in_psram = in_psram;
            }
        }
        else
        {
            vpp_msg.vpp_buf1_en       = en;
            vpp_msg.vpp_buf1_in_psram = in_psram;
        }
    }
    return ret;
}

void vpp_set_double_psram_for_buf1(uint8_t en)
{
    vpp_msg.double_psram_for_buf1 = en;
}

int32 vppdone_func_register(uint8_t id, func_done_fn func, uint32 arg)
{
    struct vpp_device *vpp_dev = (struct vpp_device *) dev_get(HG_VPP_DEVID);
    int32_t            closed  = vpp_is_closed(vpp_dev);
    if (closed)
    {
        os_printf(KERN_DEBUG "vpp is closed, func:%p, arg:%X\tclosed:%d\n", func, arg, closed);
        func(arg);
    }
    else
    {
        uint32_t flags                  = disable_irq();
        vpp_deal_dev_func_table[id]     = func;
        vpp_deal_dev_func_arg_table[id] = arg;
        enable_irq(flags);
    }

    return 0;
}

int32 vppdone_func_unregister(uint8_t id)
{
    uint32_t flags                  = disable_irq();
    vpp_deal_dev_func_table[id]     = NULL;
    vpp_deal_dev_func_arg_table[id] = 0;
    enable_irq(flags);
    return 0;
}

// 设置buf1输出的size(需要整除,否则不会修改或者异常)
uint8_t set_vpp_bu1_shrink(uint16_t w, uint16_t shrink_w)
{
    uint8_t ret = 1;
    if (!w)
    {
        return ret;
    }
    if (w * 2 / 3 == shrink_w)
    {
        vpp_msg.shrink = SHRINK_2_3;
        return 0;
    }

    uint8_t shrink_calc = w / shrink_w;
    uint8_t shrink      = vpp_msg.shrink;
    // 是倍数关系,检查是否有符合
    if (shrink_w * shrink_calc == w)
    {
        ret = 0;
        switch (shrink_calc)
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
                ret    = 1;
                break;
        }
    }
    vpp_msg.shrink = shrink;
    if (ret)
    {
        os_printf(KERN_ERR "%s err,w:%d\tshrink_w:%d\tshrink:%d\n", __FUNCTION__, w, shrink_w, shrink);
    }
    else
    {
        os_printf(KERN_INFO "%s success,w:%d\tshrink_w:%d\tshrink:%d\n", __FUNCTION__, w, shrink_w, shrink);
    }
    return ret;
}

uint8_t get_vpp_scale_w_h(uint16_t *w, uint16_t *h)
{
    uint8_t ret = RET_OK;
    if (w)
    {
        *w = vpp_msg.vpp_scale_w;
    }

    if (h)
    {
        *h = vpp_msg.vpp_scale_h;
    }
    if (!vpp_msg.vpp_scale_w || !vpp_msg.vpp_scale_h)
    {
        ret = RET_ERR;
    }
    return ret;
}

uint8_t get_vpp_w_h(uint16_t *w, uint16_t *h)
{
    uint8_t ret = RET_OK;
    if (w)
    {
        *w = vpp_msg.vpp_w;
    }

    if (h)
    {
        if (video_msg.camera_mode == CAM_DUAL_SPLICE_SLAVE_MODE)
        {
            *h = vpp_msg.vpp_h * 2;
        }
        else
        {
            *h = vpp_msg.vpp_h;
        }
    }
    if (!vpp_msg.vpp_w || !vpp_msg.vpp_h)
    {
        ret = RET_ERR;
    }
    return ret;
}

uint8_t get_vpp1_w_h(uint16_t *w, uint16_t *h)
{
    uint8_t ret = RET_OK;
    if (vpp_msg.vpp_buf1_en == 0)
    {
        ret = RET_ERR;
        if (w)
        {
            *w = 0;
        }
        if (h)
        {
            *h = 0;
        }
        return ret;
    }
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
            ret = RET_ERR;
            break;
    }

    if (video_msg.camera_mode == CAM_DUAL_SPLICE_SLAVE_MODE)
    {
        *h = (*h) * 2;
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
        mode     = vpp_msg.vpp_buf0_mode;
        line_num = vpp_msg.vpp_buf0_line_num;
    }
    else
    {
        mode     = vpp_msg.vpp_buf1_mode;
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

static void vpp_video_recfg(uint32 dev, uint32_t w, uint32_t h, uint8_t input_from)
{
    struct vpp_device *p_vpp = (struct vpp_device *) dev;

    vpp_set_video_size(p_vpp, w, h);
    vpp_msg.vpp_w = w;
    vpp_msg.vpp_h = h;
    if (vpp_msg.vpp_buf1_en)
    {
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
            vpp_set_buf1_v_addr(p_vpp, (uint32) yuvbuf1 + buf1w * malloc_line + buf1w * malloc_line / 4);
        }
        vpp_set_buf1_en(p_vpp, 1);
    }

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

        // vpp_set_buf0_y_addr(p_vpp, (uint32) yuvbuf);
        // vpp_set_buf0_u_addr(p_vpp, (uint32) yuvbuf + w * malloc_line);
        // vpp_set_buf0_v_addr(p_vpp, (uint32) yuvbuf + w * malloc_line + w * malloc_line / 4);
    }

#if DET_EN
    vpp_set_motion_calbuf(p_vpp, (uint32_t) motion_detect_buf);
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
    // struct vpp_device *p_vpp = (struct vpp_device *) dev;
    //_os_printf("V");

    if (vpp_deal_dev_func_table[VPP_IFP_EN_CTRL])
    {
        vpp_deal_dev_func_table[VPP_IFP_EN_CTRL](vpp_deal_dev_func_arg_table[VPP_IFP_EN_CTRL]);
    }

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

void vpp_set_time(struct vpp_device *p_vpp, uint32_t time_val)
{
    struct tm *time_info;
    time_info     = gmtime((const time_t *) &time_val);
    uint32_t year = time_info->tm_year + 1900;
    uint32_t mon  = time_info->tm_mon + 1;
    uint32_t day  = time_info->tm_mday;
    uint32_t hour = time_info->tm_hour;
    uint32_t min  = time_info->tm_min;
    uint32_t sec  = time_info->tm_sec;
    set_time_watermark(p_vpp, year, mon, day, hour, min, sec);
}

uint32_t compute_block_laplacian_mean(uint8_t *block_mean_y, int num_vertical_blocks, int num_horizontal_blocks)
{
    int laplacian_sum        = 0;
    int num_laplacian_values = (num_vertical_blocks > 2 ? num_vertical_blocks - 2 : 0) * (num_horizontal_blocks > 2 ? num_horizontal_blocks - 2 : 0);

    //	_os_printf("v:%d h:%d\r\n",num_vertical_blocks,num_horizontal_blocks);
    for (int block_row = 1; block_row < num_vertical_blocks - 1; block_row++)
    {
        for (int block_col = 1; block_col < num_horizontal_blocks - 1; block_col++)
        {
            int laplacian_value = 4 * block_mean_y[block_row * num_horizontal_blocks + block_col] - block_mean_y[(block_row - 1) * num_horizontal_blocks + block_col] -
                                  block_mean_y[(block_row + 1) * num_horizontal_blocks + block_col] - block_mean_y[block_row * num_horizontal_blocks + (block_col - 1)] -
                                  block_mean_y[block_row * num_horizontal_blocks + (block_col + 1)];

            laplacian_sum += (laplacian_value < 0 ? -laplacian_value : laplacian_value);
        }
    }

    if (num_laplacian_values == 0)
    {
        return 0;
    }
    else
    {
        return (laplacian_sum + num_laplacian_values / 2) / num_laplacian_values;
    }
}

void vpp_frame_done(uint32 irq, uint32 dev, uint32 param)
{
    static uint32_t    md_isr_cnt = 0;
    //	static uint32_t  done_num=0;
    //	static uint32_t  detnum=0;
    //  uint32_t defcal;
    uint8_t            itk        = 0;
    int32_t            ret        = 0;
    uint16_t           w          = 0;
    uint16_t           h          = 0;
    uint16_t           detw       = 0;
    uint16_t           deth       = 0;
    uint32_t           loc        = 0;
    uint16_t           buf1w      = 0;
    uint16_t           buf1h      = 0;
    uint8_t           *ptr_cache  = NULL;
    struct vpp_device *p_vpp      = (struct vpp_device *) dev;

    static time_t  last_time = 0;
    struct timeval ptimeval;
    time_t         time;
    gettimeofday(&ptimeval, NULL);
    timezone_utc_to_local(ptimeval.tv_sec, &time);

    // 单目
    if (video_msg.video_num == 1)
    {
        if ((motion_detect_buf != NULL) && (mdet_oldframe_sensor1_buf != NULL))
        { // det enable
            if (vpp_md_cnt != md_isr_cnt)
            {
                detw = (vpp_msg.vpp_w + 31) / 32;
                deth = (vpp_msg.vpp_h + 31) / 32;
                loc  = motion_det_pot_check(mdet_oldframe_sensor1_buf, motion_detect_buf + 4 * ((vpp_msg.vpp_w + 31) / 32), mdet_result_sensor1, detw, deth, 10, 20);
                if (loc != 0xffffffff)
                {
                    md_set_pot_x_y(ISP_VIDEO_0, (loc % detw) * 32, (loc / detw) * 32);
                }
            }
            memcpy(mdet_oldframe_sensor1_buf, motion_detect_buf + 4 * ((vpp_msg.vpp_w + 31) / 32), ((vpp_msg.vpp_w + 31) / 32) * ((vpp_msg.vpp_h + 31) / 32));
        }
    }
    // 双目
    else if (video_msg.video_num == 2)
    {
        if ((motion_detect_buf != NULL) && (mdet_oldframe_sensor1_buf != NULL) && (mdet_oldframe_sensor2_buf != NULL))
        {
            // sensor 0
            if (video_msg.video_type_cur == ISP_VIDEO_0)
            {
                if (vpp_md_cnt != md_isr_cnt)
                {
                    detw = (vpp_msg.vpp_w + 31) / 32;
                    deth = (vpp_msg.vpp_h + 31) / 32;
                    loc  = motion_det_pot_check(mdet_oldframe_sensor1_buf, motion_detect_buf + 4 * ((vpp_msg.vpp_w + 31) / 32), mdet_result_sensor1, detw, deth, 30, 20);

                    if (loc != 0xffffffff)
                    {
                        md_set_pot_x_y(ISP_VIDEO_0, (loc % detw) * 32, (loc / detw) * 32);
                    }
                }
                memcpy(mdet_oldframe_sensor1_buf, motion_detect_buf + 4 * ((vpp_msg.vpp_w + 31) / 32), ((vpp_msg.vpp_w + 31) / 32) * ((vpp_msg.vpp_h + 31) / 32));
            }
            // sensor 1
            else if (video_msg.video_type_cur == ISP_VIDEO_1)
            {
                if (vpp_md_cnt != md_isr_cnt)
                {
                    detw = (vpp_msg.vpp_w + 31) / 32;
                    deth = (vpp_msg.vpp_h + 31) / 32;
                    loc  = motion_det_pot_check(mdet_oldframe_sensor2_buf, motion_detect_buf + 4 * ((vpp_msg.vpp_w + 31) / 32), mdet_result_sensor2, detw, deth, 30, 20);

                    if (loc != 0xffffffff)
                    {
                        md_set_pot_x_y(ISP_VIDEO_1, (loc % detw) * 32, (loc / detw) * 32);
                    }
                }
                memcpy(mdet_oldframe_sensor2_buf, motion_detect_buf + 4 * ((vpp_msg.vpp_w + 31) / 32), ((vpp_msg.vpp_w + 31) / 32) * ((vpp_msg.vpp_h + 31) / 32));
            }
        }
    }

    if (last_time != time)
    {
        vpp_set_time(p_vpp, time);
        last_time = time;
    }

    if (video_msg.video_num > 1 && video_msg.camera_mode == CAM_DUAL_MASTER_SLAVE_MODE)
    {
        if (video_msg.video_type_cur != ISP_VIDEO_0)
        { // 这帧是副镜头，配置下帧主镜头
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
        { // 这帧是主镜头，配置下帧副镜头
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

    for (itk = 0; itk < VPP_FUNC_DONE_NUM; itk++)
    {
        if (vpp_deal_dev_func_table[itk])
        {
            ret = vpp_deal_dev_func_table[itk](vpp_deal_dev_func_arg_table[itk]);
            if (ret)
            {
                vpp_deal_dev_func_table[itk]     = NULL;
                vpp_deal_dev_func_arg_table[itk] = 0;
            }
        }
    }

    if (scale3_kick_func)
    {
        scale3_kick_func();
    }
    if (vpp_msg.double_psram_for_buf1)
    {
        uint8_t shrink = vpp_msg.shrink; // 0: 1/2
                                         // 1: 1/3
                                         // 2: 1/4
                                         // 3: 1/6
                                         // 4: 2/3
                                         // 5: 1/1

        w = vpp_msg.vpp_w;
        h = vpp_msg.vpp_h;
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

        ptr_cache      = (uint8_t *) psram_user_ptr;
        psram_user_ptr = psram_ptr;
        psram_ptr      = ptr_cache;

        if (psram_ptr)
        {

            vpp_set_buf1_shrink(p_vpp, shrink);
            vpp_set_psram_ycnt(p_vpp, buf1w, buf1h);
            vpp_set_psram_uvcnt(p_vpp, buf1w, buf1h);

            if (video_msg.camera_mode == CAM_SINGLE_MASTER_MODE)
            {
                vpp_set_buf1_y_addr(p_vpp, (uint32) psram_ptr);
                vpp_set_buf1_u_addr(p_vpp, (uint32) psram_ptr + buf1w * buf1h);
                vpp_set_buf1_v_addr(p_vpp, (uint32) psram_ptr + buf1w * buf1h + buf1w * buf1h / 4);
            }
            else if (video_msg.camera_mode == CAM_DUAL_SPLICE_SLAVE_MODE)
            {
                if (psram_ptr == vpp_data1_psram_buf)
                {
                    vpp_set_buf1_y_addr(p_vpp, (uint32) psram_ptr);
                    vpp_set_buf1_u_addr(p_vpp, (uint32) psram_ptr + buf1w * buf1h * 2);
                    vpp_set_buf1_v_addr(p_vpp, (uint32) psram_ptr + buf1w * buf1h * 2 + buf1w * (buf1h * 2) / 4);
                }
                else
                {
                    vpp_set_buf1_y_addr(p_vpp, (uint32) psram_ptr);
                    vpp_set_buf1_u_addr(p_vpp, (uint32) psram_ptr + buf1w * buf1h + buf1w * buf1h / 4);
                    vpp_set_buf1_v_addr(p_vpp, (uint32) psram_ptr + buf1w * buf1h + buf1w * buf1h / 4 + buf1w * (buf1h * 2) / 4);
                }
            }
        }
    }

    md_isr_cnt = vpp_md_cnt;

    if (motion_detect_buf != NULL)
    {
        // 720P --->260us
        photo_complex = compute_block_laplacian_mean(motion_detect_buf + 4 * ((vpp_msg.vpp_w + 31) / 32), (vpp_msg.vpp_h + 31) / 32, (vpp_msg.vpp_w + 31) / 32);
    }
    //_os_printf(KERN_DEBUG "F(%d)", photo_complex);
    _os_printf(KERN_DEBUG "F");
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

void vpp_ipf_error(uint32 irq, uint32 dev, uint32 param)
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
bool vpp_cfg(uint8_t input_from)
{

    uint16_t w, h;

    uint32_t line_iw;
    w       = vpp_msg.vpp_w;
    h       = vpp_msg.vpp_h;
    line_iw = w;
    if (w == 0 || h == 0)
    {
        return FALSE;
    }
    if (video_msg.camera_mode == CAM_DUAL_SPLICE_SLAVE_MODE)
    {
        // 如果需要拼接,则设置double_psram_for_buf1
        vpp_msg.double_psram_for_buf1 = 1;
    }
    else if (video_msg.camera_mode == CAM_DUAL_MASTER_SLAVE_MODE)
    {
        // 多目经过dual_org模块, 分辨率不同时, 取宽度更大的一路，计算line buf所需内存空间
        line_iw = VPP_MAX3(video_msg.dvp_iw, video_msg.csi0_iw, video_msg.csi1_iw);
    }

#if IPF_EN
    uint32_t len;
#endif
    struct vpp_device *vpp_dev;
    vpp_dev = (struct vpp_device *) dev_get(HG_VPP_DEVID);

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
            yuvbuf = (uint8_t *) VPP_MALLOC(line_iw * malloc_line + line_iw * malloc_line / 2);
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
        vpp_set_buf0_u_addr(vpp_dev, (uint32) yuvbuf + line_iw * malloc_line);
        vpp_set_buf0_v_addr(vpp_dev, (uint32) yuvbuf + line_iw * malloc_line + line_iw * malloc_line / 4);
#endif
    }

    if (vpp_msg.vpp_buf1_en)
    {
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
        if (!vpp_msg.vpp_buf1_in_psram)
        {
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
                    yuvbuf1 = VPP_MALLOC(buf1w * malloc_line + buf1w * malloc_line / 2);
                    if (yuvbuf1 == NULL)
                    {
                        _os_printf("no room yuvbuf1\r\n");
                        return FALSE;
                    }
                }
                vpp_set_buf1_y_addr(vpp_dev, (uint32) yuvbuf1);
                vpp_set_buf1_u_addr(vpp_dev, (uint32) yuvbuf1 + buf1w * malloc_line);
                vpp_set_buf1_v_addr(vpp_dev, (uint32) yuvbuf1 + buf1w * malloc_line + buf1w * malloc_line / 4);
                vpp_set_buf1_count(vpp_dev, vpp_msg.vpp_buf1_line_num);
            }
        }
        else
        {

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
            if (vpp_data1_psram_buf)
            {
                VPP_PSRAM_FREE(vpp_data1_psram_buf);
                vpp_data1_psram_buf = NULL;
            }

            if (video_msg.camera_mode == CAM_SINGLE_MASTER_MODE)
            {

                vpp_data1_psram_buf = (uint8_t *) VPP_PSRAM_MALLOC(p_w * p_h * 3 / 2);
                ASSERT(vpp_data1_psram_buf);
                if (vpp_msg.double_psram_for_buf1)
                {
                    vpp_data2_psram_buf = (uint8_t *) VPP_PSRAM_MALLOC(p_w * p_h * 3 / 2);
                    ASSERT(vpp_data2_psram_buf);
                    psram_ptr      = vpp_data1_psram_buf;
                    psram_user_ptr = vpp_data2_psram_buf;
                }

                vpp_set_buf1_y_addr(vpp_dev, (uint32) vpp_data1_psram_buf);
                vpp_set_buf1_u_addr(vpp_dev, (uint32) vpp_data1_psram_buf + p_w * p_h);
                vpp_set_buf1_v_addr(vpp_dev, (uint32) vpp_data1_psram_buf + p_w * p_h + p_w * p_h / 4);
            }
            else if (video_msg.camera_mode == CAM_DUAL_SPLICE_SLAVE_MODE)
            {
                vpp_data1_psram_buf = (uint8_t *) VPP_PSRAM_MALLOC(p_w * (p_h * 2) * 3 / 2);
                _os_printf("vpp_data1_psram_buf:%08x\r\n", vpp_data1_psram_buf);
                ASSERT(vpp_data1_psram_buf);

                vpp_data2_psram_buf = vpp_data1_psram_buf + p_w * p_h;
                psram_ptr           = vpp_data1_psram_buf;
                psram_user_ptr      = vpp_data2_psram_buf;
                vpp_set_buf1_y_addr(vpp_dev, (uint32) vpp_data1_psram_buf);
                vpp_set_buf1_u_addr(vpp_dev, (uint32) vpp_data1_psram_buf + p_w * p_h * 2);
                vpp_set_buf1_v_addr(vpp_dev, (uint32) vpp_data1_psram_buf + p_w * p_h * 2 + (p_w * p_h * 2) / 4);
            }
        }

        vpp_set_buf1_shrink(vpp_dev, vpp_msg.shrink);
        vpp_set_buf1_en(vpp_dev, 1);
    }

    vpp_set_water0_color(vpp_dev, 0xff, 0x80, 0x80);
    vpp_set_water0_bitmap(vpp_dev, (uint32) ele_lib);
    vpp_set_water0_locate(vpp_dev, 8, 16);
    vpp_set_water0_contrast(vpp_dev, 0);
    vpp_set_watermark0_charsize_and_num(vpp_dev, 16, 32, 19);
    vpp_set_watermark0_mode(vpp_dev, 1);

    struct timeval ptimeval;
    time_t         time_val;
    gettimeofday(&ptimeval, NULL);
    timezone_utc_to_local(ptimeval.tv_sec, &time_val);
    vpp_set_time(vpp_dev, time_val);

    vpp_set_water0_rc(vpp_dev, 0);
    vpp_set_watermark0_auto_rc_sram_adr(vpp_dev, (uint32_t) autorc);
    vpp_set_watermark0_auto_rc_threshold(vpp_dev, 16 * 32 * 128, 16 * 32 * 32);
    vpp_set_watermark_auto_rc_mode(vpp_dev, video_msg.video_num > 1 ? 1 : 0); // double sensor
    vpp_set_watermark0_auto_rc(vpp_dev, 1, 0x00, 0x80, 0x80);

#if IPF_EN
    len            = water2_change_ipf(w, h, photo_lib2, 48, 48, 100, 300, NULL);
    vpp_encode_ipf = (uint8_t *) VPP_MALLOC(len);
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

    // vpp_request_irq(vpp_dev, HSIE_ISR, (vpp_irq_hdl) &vpp_hsie_isr, (uint32) vpp_dev);
    vpp_request_irq(vpp_dev, VSIE_ISR, (vpp_irq_hdl) &vpp_vsie_isr, (uint32) vpp_dev);
    vpp_request_irq(vpp_dev, FRAME_DONE_ISR, (vpp_irq_hdl) &vpp_frame_done, (uint32) vpp_dev);
    vpp_request_irq(vpp_dev, SCIE_ISR, (vpp_irq_hdl) &vpp_data_done, h);
    vpp_request_irq(vpp_dev, LOVIE_ISR, (vpp_irq_hdl) &vpp_lib_error, (uint32) vpp_dev);
    vpp_request_irq(vpp_dev, IPF_OV_ISR, (vpp_irq_hdl) &vpp_ipf_error, (uint32) vpp_dev);
    vpp_request_irq(vpp_dev, MDPIE_ISR, (vpp_irq_hdl) &vpp_md_find, (uint32) vpp_dev);
    vpp_request_irq(vpp_dev, ITP_OV_ISR, (vpp_irq_hdl) &vpp_itp_error, (uint32) vpp_dev);
    vpp_request_irq(vpp_dev, ITP_DONE_ISR, (vpp_irq_hdl) &vpp_itp_done, (uint32) vpp_dev);

#if DET_EN
    motion_detect_buf = (uint8_t *) VPP_MALLOC(((w + 31) / 32) * ((h + 31) / 32) + 4 * ((w + 31) / 32));
    if (motion_detect_buf == NULL)
    {
        goto det_module_end;
    }
    memset(motion_detect_buf, 0, ((w + 31) / 32) * ((h + 31) / 32) + 4 * ((w + 31) / 32));

    vpp_set_motion_calbuf(vpp_dev, (uint32_t) motion_detect_buf);
    vpp_set_motion_range(vpp_dev, 0, 0, w, h);   // 检测图像范围,blk大小为32*32个像素点
    vpp_set_motion_blk_threshold(vpp_dev, 10);   // 检测对应的blk移动的阀值
    vpp_set_motion_frame_threshold(vpp_dev, 10); // 检测多少个blk超过阀值，再触发移动检测中断

    if (vpp_msg.mdt)
    {
        // 申请空间,支持返回移动的区域坐标
        mdet_result_sensor1       = (uint8_t *) VPP_MALLOC(((w + 31) / 32) * ((h + 31) / 32));
        mdet_oldframe_sensor1_buf = (uint8_t *) VPP_MALLOC(((w + 31) / 32) * ((h + 31) / 32));
        if (mdet_oldframe_sensor1_buf == NULL)
        {
            goto det_module_end;
        }
        memset(mdet_oldframe_sensor1_buf, 0, ((w + 31) / 32) * ((h + 31) / 32));

        if (video_msg.video_num == 2)
        {
            mdet_result_sensor2       = (uint8_t *) VPP_MALLOC(((w + 31) / 32) * ((h + 31) / 32));
            mdet_oldframe_sensor2_buf = (uint8_t *) VPP_MALLOC(((w + 31) / 32) * ((h + 31) / 32));
            if (mdet_oldframe_sensor2_buf == NULL)
            {
                goto det_module_end;
            }
            memset(mdet_oldframe_sensor2_buf, 0, ((w + 31) / 32) * ((h + 31) / 32));
        }
    }

    vpp_set_motion_det_enable(vpp_dev, 1);
det_module_end:
#endif
    vpp_set_mode(vpp_dev, VPP_INPUT_FORMAT);
    vpp_set_input_interface(vpp_dev, input_from);

    vpp_set_watermark0_enable(vpp_dev, 1);
    //	vpp_set_watermark1_enable(vpp_dev,1);

#if IPF_EN
    vpp_set_ifp_en(vpp_dev, 1);
#endif

    vpp_msg.vpp_scale_w = VPP_SCALE_WIDTH;
    vpp_msg.vpp_scale_h = VPP_SCALE_HIGH;

    vpp_open(vpp_dev);
    return TRUE;
}

void set_vpp_scale_w_h(uint8_t en, uint16_t w, uint16_t h)
{
    struct scale_device *scale_dev;
    uint8_t             *vpp_buf = get_vpp_buf(0);
    if (vpp_buf)
    {
        vpp_msg.vpp_scale_w = w;
        vpp_msg.vpp_scale_h = h;
        if (en)
        {
            scale_dev = (struct scale_device *) dev_get(HG_SCALE1_DEVID);
            scale_from_vpp(scale_dev, (uint32_t) vpp_buf, vpp_msg.vpp_w, vpp_msg.vpp_h, w, h);
        }
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

    if (yuvbuf1)
    {
        VPP_FREE(yuvbuf1);
        yuvbuf1 = NULL;
    }

    if (vpp_encode_ipf)
    {
        VPP_FREE(vpp_encode_ipf);
        vpp_encode_ipf = NULL;
    }

    if (motion_detect_buf)
    {
        VPP_FREE(motion_detect_buf);
        motion_detect_buf = NULL;
    }

    if (vpp_data1_psram_buf)
    {
        VPP_PSRAM_FREE(vpp_data1_psram_buf);
        vpp_data1_psram_buf = NULL;
    }

    if (vpp_data2_psram_buf)
    {
        VPP_PSRAM_FREE(vpp_data2_psram_buf);
        vpp_data2_psram_buf = NULL;
    }
    return TRUE;
}

int8_t vpp_dev_open()
{
    struct vpp_device *vpp_dev;
    vpp_dev = (struct vpp_device *) dev_get(HG_VPP_DEVID);
    vpp_open(vpp_dev);
    return 0;
}

/**************************************************检测移动侦测接口***************************************************** */
struct mdt_coord_msg mdet_msg[4] = {0};
uint32               mdxy[2]     = {0};

uint32 motion_det_pot_check(uint8_t *old_y, uint8_t *new_y, uint8_t *copy, uint16 w, uint16 h, uint8_t blk_thd, uint8_t md_blk_num)
{
    uint16   i, j;
    uint16   mw, mh;
    uint16   bx, by;
    uint32   local_base = 0;
    uint16   i1, j1;
    uint8_t  mloop;
    uint8_t  potd;
    uint8_t  mask;
    uint16_t mx0, mx1;
    uint16_t my0, my1;
    uint16_t mapdt_loc[4][6];
    uint16_t mapdt_num;
    uint8_t  result_cnt[4];
    uint32_t result_loc[4];
    uint16   mvblk = 0;
    uint8_t  x0m, x1m, y0m, y1m;

    for (j = 0; j < h; j++)
    {
        for (i = 0; i < w; i++)
        {
            if (abs(old_y[i + j * w] - new_y[i + j * w]) >= blk_thd)
            {
                old_y[i + j * w] = 255;
                mvblk++;
            }
            else
            {
                old_y[i + j * w] = 0;
            }
        }
    }

    if (mvblk < md_blk_num)
    {
        return 0xffffffff;
    }
    memcpy(copy, old_y, w * h);

    memset(mapdt_loc, 0xff, 4 * 6 * 2);
    memset(result_cnt, 0, 4);
    memset(result_loc, 0, 4 * 4);
    mapdt_num = 0;
    for (j1 = 0; j1 < h - 1; j1++)
    {
        for (i1 = 0; i1 < w - 1; i1++)
        {
            potd = 0;
            for (j = 0; j < 2; j++)
            {
                for (i = 0; i < 2; i++)
                {
                    if (old_y[i1 + j1 * w + i + j * w] == 255)
                    {
                        potd++;
                        if (potd == 4)
                        {
                            mask = 1;
                            for (mloop = 0; mloop < 4; mloop++)
                            {
                                if (mapdt_loc[mloop][0] != 0xffff)
                                { // 有座标
                                    mx0 = mapdt_loc[mloop][2];
                                    mx1 = mapdt_loc[mloop][3];
                                    my0 = mapdt_loc[mloop][4];
                                    my1 = mapdt_loc[mloop][5];

                                    x0m = (mx0 > 3) ? (mx0 - 3) : 0;
                                    y0m = (my0 > 3) ? (my0 - 3) : 0;
                                    x1m = (mx1 + 3 > w) ? (w - 1) : (mx1 + 3);
                                    y1m = (my1 + 3 > h) ? (h - 1) : (my1 + 3);

                                    if (((i1 + i) >= x0m) && ((i1 + i) <= x1m) && ((j1 + j) >= y0m) && ((j1 + j) <= y1m))
                                    {
                                        mask = 0;
                                    }
                                }
                            }

                            if (mask == 1)
                            {
                                for (mloop = 0; mloop < 4; mloop++)
                                {
                                    if (mapdt_loc[mloop][0] == 0xffff)
                                    {
                                        mapdt_loc[mloop][0] = i1 + i;
                                        mapdt_loc[mloop][1] = j1 + j;

                                        mx0 = (mapdt_loc[mloop][0] > 3) ? (mapdt_loc[mloop][0] - 3) : 0;
                                        my0 = (mapdt_loc[mloop][1] > 3) ? (mapdt_loc[mloop][1] - 3) : 0;
                                        mx1 = (mapdt_loc[mloop][0] + 3 > w) ? (w - 1) : (mapdt_loc[mloop][0] + 3);
                                        my1 = (mapdt_loc[mloop][1] + 3 > h) ? (h - 1) : (mapdt_loc[mloop][1] + 3);

                                        mapdt_loc[mloop][2] = mx0;
                                        mapdt_loc[mloop][3] = mx1;
                                        mapdt_loc[mloop][4] = my0;
                                        mapdt_loc[mloop][5] = my1;

                                        break;
                                    }
                                }
                                if (mapdt_loc[3][0] != 0xffff)
                                {
                                    goto mdt_end;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

mdt_end:
    for (mloop = 0; mloop < 4; mloop++)
    {
        if (mapdt_loc[mloop][0] != 0xffff)
        {
            bx   = mapdt_loc[mloop][2];
            by   = mapdt_loc[mloop][4];
            mw   = mapdt_loc[mloop][3] - mapdt_loc[mloop][2];
            mh   = mapdt_loc[mloop][5] - mapdt_loc[mloop][4];
            potd = 0;
            for (j = 0; j < mh; j++)
            {
                for (i = 0; i < mw; i++)
                {
                    if (old_y[bx + by * w + i + j * w] == 0xff)
                    {
                        potd++;
                    }
                }
            }
            result_cnt[mloop] = potd;
            // printf("(%d  %d===>%d  %d pot:%d)\r\n",bx,by,mw,mh,potd);
            if (potd < 7)
            {
                result_loc[mloop] = (mapdt_loc[mloop][0] - 1) + (mapdt_loc[mloop][1] - 1) * w;
            }
            else
            {
                result_loc[mloop] = mapdt_loc[mloop][0] + mapdt_loc[mloop][1] * w;
            }
        }
    }

    for (i = 0; i < 4; i++)
    {
        if (mapdt_loc[i][0] != 0xffff)
        {
            mdet_msg[i].blkmv_cnt = result_cnt[i];
            mdet_msg[i].x0        = mapdt_loc[i][2];
            mdet_msg[i].x1        = mapdt_loc[i][3];
            mdet_msg[i].y0        = mapdt_loc[i][4];
            mdet_msg[i].y1        = mapdt_loc[i][5];
            mdet_msg[i].x         = mapdt_loc[i][0];
            mdet_msg[i].y         = mapdt_loc[i][1];
        }
        else
        {
            mdet_msg[i].blkmv_cnt = 0;
        }
    }

    if (result_loc[0] != 0)
    {
        for (i = 0; i < 4; i++)
        {
            if (local_base < result_loc[i])
            {
                local_base = result_loc[i];
            }
        }
        return local_base;
    }
    else
    {
        return 0xffffffff;
    }
}

void md_set_pot_x_y(ISP_VIDEO_E sensor_id, uint16 x, uint16 y)
{
    mdxy[sensor_id] = ((y & 0xffff) << 16 | (x & 0xffff));
}

// 获取绝对位置
uint32 md_get_pot_x_y(ISP_VIDEO_E sensor_id)
{
    return mdxy[sensor_id];
}

// 获取相对位置，sensor_id区分0/1摄像头
uint32 md_get_relative_pot_x_y(ISP_VIDEO_E sensor_id, uint16_t r_w, uint16_t r_h, uint16_t *gx, uint16_t *gy)
{
    uint32   pos = mdxy[sensor_id];
    uint16_t x   = (r_w * (pos & 0xffff) / vpp_msg.vpp_w) & 0xffff;
    uint16_t y   = (r_h * (pos >> 16) / vpp_msg.vpp_h) & 0xffff;
    if (gx)
    {
        *gx = x;
    }
    if (gy)
    {
        *gy = y;
    }
    return y << 16 | x;
}

// 包围框缩放函数
uint32 md_get_relative_pot_x0_y0_x1_y1(uint8_t num, uint16_t r_w, uint16_t r_h, uint16_t *gx0, uint16_t *gy0, uint16_t *gx1, uint16_t *gy1)
{
    uint16_t x0 = (r_w * (mdet_msg[num].x0 * 32) / vpp_msg.vpp_w) & 0xffff;
    uint16_t y0 = (r_h * (mdet_msg[num].y0 * 32) / vpp_msg.vpp_h) & 0xffff;
    uint16_t x1 = (r_w * (mdet_msg[num].x1 * 32) / vpp_msg.vpp_w) & 0xffff;
    uint16_t y1 = (r_h * (mdet_msg[num].y1 * 32) / vpp_msg.vpp_h) & 0xffff;
    if (mdet_msg[num].blkmv_cnt == 0)
    {
        if (gx0)
        {
            *gx0 = 0;
        }
        if (gy0)
        {
            *gy0 = 0;
        }
        if (gx1)
        {
            *gx1 = 1;
        }
        if (gy1)
        {
            *gy1 = 1;
        }
        return 1;
    }
    if (gx0)
    {
        *gx0 = x0;
    }
    if (gy0)
    {
        *gy0 = y0;
    }
    if (gx1)
    {
        *gx1 = x1;
    }
    if (gy1)
    {
        *gy1 = y1;
    }
    return 0;
}

void md_get_table_size(ISP_VIDEO_E sensor_id, uint16_t *w_out, uint16_t *h_out)
{
    uint16_t det_w = (vpp_msg.vpp_w + 31) / 32;
    uint16_t det_h = (vpp_msg.vpp_h + 31) / 32;

    if (w_out)
    {
        *w_out = det_w;
    }
    if (h_out)
    {
        *h_out = det_h;
    }
}

// 图像划分32*32的亮度表，获取二值化后的亮度表（0=静止块，255=运动块）
int8_t md_get_binary_table(ISP_VIDEO_E sensor_id, uint8_t *bin_buf)
{
    uint16_t det_w = (vpp_msg.vpp_w + 31) / 32;
    uint16_t det_h = (vpp_msg.vpp_h + 31) / 32;

    uint8_t *src_bin = NULL;

    if (sensor_id == ISP_VIDEO_0)
    {
        if (mdet_result_sensor1 == NULL)
        {
            return -1;
        }
        src_bin = mdet_result_sensor1;
    }
    else if (sensor_id == ISP_VIDEO_1)
    {
        if (mdet_result_sensor2 == NULL)
        {
            return -1;
        }
        src_bin = mdet_result_sensor2;
    }
    memcpy(bin_buf, src_bin, det_w * det_h);

    return 0;
}