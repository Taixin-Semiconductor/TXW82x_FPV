#include "basic_include.h"
#include "lvgl_rotate_rpc.h"
#include "hal/dma2d.h"
#include "lib/rpc/cpurpc.h"

typedef struct {
    __IO uint32_t CONF;
    __IO uint32_t AREA_CONF;
    __IO uint32_t ADDR_FROM;
    __IO uint32_t ADDR_TO;
} ROT_TypeDef;
#define ROT                     ((ROT_TypeDef    *)ROT_BASE)

#define LV_FRAME_ROTATE_LANE_NUM 16

struct lvgl_rotate_rpc_priv
{
    uint8_t *rotate_src_tem_buf;
    uint8_t *rotate_dst_tem_buf;

    uint8_t *frame_src_buff;
    uint8_t *frame_dst_buff;

    uint16_t lvgl_width;
    uint16_t lvgl_height;
    uint8_t  lvgl_depth;

    struct dma2d_device *dma2d_dev;

    struct os_event event;
    int is_270;

    int done_flag;

} ;

static struct lvgl_rotate_rpc_priv *lvgl_rot_rpc_priv_p = NULL;

enum lvgl_rotate_evt
{
    LV_ROT_START = BIT(0),
    LV_ROT_END   = BIT(1),
};

static int32 lvgl_frame_rotate_rgb565(int is_270)
{
    int ret = 0;
    //os_printf("%s %d\n",__FUNCTION__,__LINE__);
    uint16_t *p_16 =  (uint16_t *)lvgl_rot_rpc_priv_p->frame_src_buff;
    uint16_t *r_16 =  (uint16_t *)lvgl_rot_rpc_priv_p->frame_dst_buff;

    uint32_t rotate_loop_count = lvgl_rot_rpc_priv_p->lvgl_height / LV_FRAME_ROTATE_LANE_NUM;  //480 / 16 = 30


    for(uint32_t i = 0; i < rotate_loop_count; i++)
    {
        struct dma2d_blkcpy_param blkcpy;

        blkcpy.src_addr = (uint32_t)(p_16+(i*LV_FRAME_ROTATE_LANE_NUM)*lvgl_rot_rpc_priv_p->lvgl_width);
        blkcpy.dst_addr = (uint32_t)lvgl_rot_rpc_priv_p->rotate_src_tem_buf;
        blkcpy.color_mode = DMA2D_COLOR_TYPE_RGB565;
        blkcpy.src_pixel_width = lvgl_rot_rpc_priv_p->lvgl_width;
        blkcpy.dst_pixel_width = lvgl_rot_rpc_priv_p->lvgl_width;
        blkcpy.blk_pixel_width = lvgl_rot_rpc_priv_p->lvgl_width;
        blkcpy.blk_pixel_height = LV_FRAME_ROTATE_LANE_NUM;
        blkcpy.src_pixel_start_height = 0;
        blkcpy.src_pixel_start_width = 0;
        blkcpy.dst_pixel_start_height = 0;
        blkcpy.dst_pixel_start_width =  0;
        dma2d_blkcpy(lvgl_rot_rpc_priv_p->dma2d_dev, &blkcpy);
        ret = dma2d_check_status(lvgl_rot_rpc_priv_p->dma2d_dev);
        if (ret)
        {
            os_printf("DMA2D error during full area copy,ret:%d\n",ret);
        }

        ROT_TypeDef   *rot;
        rot = ROT;
        rot->CONF = 0;
        rot->CONF |= BIT(6);
        rot->AREA_CONF = (LV_FRAME_ROTATE_LANE_NUM << 16)|(lvgl_rot_rpc_priv_p->lvgl_width);
        rot->ADDR_FROM = (int32_t)lvgl_rot_rpc_priv_p->rotate_src_tem_buf;
        rot->ADDR_TO = (int32_t)lvgl_rot_rpc_priv_p->rotate_dst_tem_buf;
        if (is_270) {
            rot->CONF |= (BIT(5) | BIT(0));    //旋转270度
        } else {
            rot->CONF |= BIT(0);               //旋转90度
        }
        while((rot->CONF&BIT(6)) == 0){
            os_sleep_ms(1);
        };

        // 将旋转后的数据拷贝到目标缓冲区
        // 使用 DMA2D 一次性拷贝整个区域
        blkcpy.src_addr = (uint32_t)lvgl_rot_rpc_priv_p->rotate_dst_tem_buf;
        if (is_270) {
            blkcpy.dst_addr = (uint32_t)(r_16 + ((rotate_loop_count - i - 1) * LV_FRAME_ROTATE_LANE_NUM));
        } else {
            blkcpy.dst_addr = (uint32_t)(r_16 + (i * LV_FRAME_ROTATE_LANE_NUM));
        }
        blkcpy.color_mode = DMA2D_COLOR_TYPE_RGB565;
        blkcpy.src_pixel_width = LV_FRAME_ROTATE_LANE_NUM;
        blkcpy.dst_pixel_width = lvgl_rot_rpc_priv_p->lvgl_height;
        blkcpy.blk_pixel_width = LV_FRAME_ROTATE_LANE_NUM;
        blkcpy.blk_pixel_height = lvgl_rot_rpc_priv_p->lvgl_width;
        blkcpy.src_pixel_start_height = 0;
        blkcpy.src_pixel_start_width = 0;
        blkcpy.dst_pixel_start_height = 0;
        blkcpy.dst_pixel_start_width =  0;
        dma2d_blkcpy(lvgl_rot_rpc_priv_p->dma2d_dev, &blkcpy);
        ret = dma2d_check_status(lvgl_rot_rpc_priv_p->dma2d_dev);
        if (ret)
        {
            os_printf("DMA2D error during full area copy,ret:%d\n",ret);
        }
    }

    return 0;
}

static void lvgl_frame_rotate_rpc_task(void *d)
{
    while(1)
    {
        os_event_wait(&lvgl_rot_rpc_priv_p->event, LV_ROT_START, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, -1);
        lvgl_rot_rpc_priv_p->done_flag = 1;
        if(lvgl_rot_rpc_priv_p->lvgl_depth == 2)
        {
            lvgl_frame_rotate_rgb565(lvgl_rot_rpc_priv_p->is_270);
        }
        else
        {
            //other format rotate function
        }
        lvgl_frame_rotate_rpc_sync();
        lvgl_rot_rpc_priv_p->done_flag = 0;
    }
}

static void lvgl_frame_rotate_rpc_task_init(void *d)
{
    os_event_init(&lvgl_rot_rpc_priv_p->event);
    os_task_create("lvgl_frame_rotate_rpc_task", lvgl_frame_rotate_rpc_task, d, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 1024);
}

int32 lvgl_frame_rotate_rpc_init(uint16_t w, uint16_t h, uint8_t depth, void *rotate_src_tem_buf, void *rotate_dst_tem_buf, void *frame_src_buff, void *frame_dst_buff)
{
    lvgl_rot_rpc_priv_p = (struct lvgl_rotate_rpc_priv *)os_zalloc(sizeof(struct lvgl_rotate_rpc_priv));

    if (!lvgl_rot_rpc_priv_p) {
        os_printf("%s %d malloc failed\n",__FUNCTION__,__LINE__);
        return 0;
    }
 
    lvgl_rot_rpc_priv_p->lvgl_width = w;
    lvgl_rot_rpc_priv_p->lvgl_height = h;
    lvgl_rot_rpc_priv_p->lvgl_depth = depth;

    lvgl_rot_rpc_priv_p->rotate_src_tem_buf = rotate_src_tem_buf;
    lvgl_rot_rpc_priv_p->rotate_dst_tem_buf = rotate_dst_tem_buf;

    lvgl_rot_rpc_priv_p->frame_src_buff = frame_src_buff;
    lvgl_rot_rpc_priv_p->frame_dst_buff = frame_dst_buff;

    sys_dcache_invalid_range(frame_src_buff, w*h*depth);
    sys_dcache_invalid_range(frame_dst_buff, w*h*depth);

    lvgl_rot_rpc_priv_p->done_flag = 0;

    lvgl_rot_rpc_priv_p->dma2d_dev = (struct dma2d_device *)dev_get(HG_DMA2D_DEVID);
    lvgl_frame_rotate_rpc_task_init(lvgl_rot_rpc_priv_p);
    return 0;
}

int32 lvgl_frame_rotate_rpc(int is_270)
{
    //os_printf("%s %d\n",__FUNCTION__,__LINE__);
    lvgl_rot_rpc_priv_p->is_270 = is_270;
    os_event_set(&lvgl_rot_rpc_priv_p->event, LV_ROT_START, NULL);

    return 0;
}

int32 lvgl_frame_rotate_rpc_sync()
{
    uint32 args[] = {};
    if(lvgl_rot_rpc_priv_p && lvgl_rot_rpc_priv_p->done_flag == 1) {
        return CPU_RPC_CALL(lvgl_frame_rotate_rpc_sync);
    }
    return 0;
}
