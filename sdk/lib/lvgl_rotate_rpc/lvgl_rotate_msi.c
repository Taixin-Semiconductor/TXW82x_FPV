#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/lvgl_rotate_rpc/lvgl_rotate_msi.h"
#include "lib/lvgl_rotate_rpc/lvgl_rotate_rpc.h"
#include "app_lcd/app_lcd.h"

#if LVGL_HW_ROTATE_RPC_EN

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC _os_malloc
#define STREAM_LIBC_FREE _os_free
#define STREAM_LIBC_ZALLOC _os_zalloc

#define LV_FRAME_ROTATE_LANE_NUM 16

struct lvgl_rotate_task_priv
{
    uint8_t *rotate_src_tem_buf;
    uint8_t *rotate_dst_tem_buf;

    uint8_t* current_buff;
    uint8_t* current_send_enc_buff;

    uint8_t* copy_buffer;

    struct os_event event;

    struct msi* lvgl_osd_msi;

    uint16_t lvgl_width;
    uint16_t lvgl_height;
    uint8_t  lvgl_depth;

    lvgl_flush_ready_cb cb;
    void *cb_user_data;
} lvgl_rot_priv ;


enum lvgl_rotate_evt
{
    LV_ROT_END    = BIT(0),
    LV_ROT_START  = BIT(1),
    LV_ROT_DELETE = BIT(2),
};


void lvgl_rotate_get_active_buff(void *data)
{
    lvgl_rot_priv.copy_buffer = data;

    os_event_set(&lvgl_rot_priv.event, LV_ROT_START, NULL);
}

void lvgl_rotate_frame_end()
{
    //os_printf("%s %d\n",__FUNCTION__,__LINE__);
    os_event_set(&lvgl_rot_priv.event, LV_ROT_END, NULL);
}

static void lvgl_rotate_task(void *d)
{
    uint32_t evt = 0;
    while(1)
    {
        int ret = 0;
        struct framebuff *fb = NULL;
        struct msi* msi = lvgl_rot_priv.lvgl_osd_msi;
        os_event_wait(&lvgl_rot_priv.event, LV_ROT_START | LV_ROT_DELETE, &evt, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, -1);

        if (evt & LV_ROT_DELETE) {
            break;
        }

__lvgl_osd_msi_get_fb_again:
        fb = lvgl_osd_msi_get_fb(msi);
        if(fb)
        {
            uint32_t cur_tick = os_jiffies();
            struct encode_data_s_callback *callback = (struct encode_data_s_callback*)fb->priv;
            uint16_t* p_16 = lvgl_rot_priv.current_buff;
            uint16_t* r_16 = lvgl_rot_priv.current_send_enc_buff;

            hw_memcpy_no_cache(p_16, lvgl_rot_priv.copy_buffer, lvgl_rot_priv.lvgl_width*lvgl_rot_priv.lvgl_height*lvgl_rot_priv.lvgl_depth);

            lvgl_rot_priv.cb(lvgl_rot_priv.cb_user_data);

            lvgl_frame_rotate_rpc(1);

            os_event_wait(&lvgl_rot_priv.event, LV_ROT_END, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, -1);

            fb->data = (void*)r_16;
            fb->len = lvgl_rot_priv.lvgl_width * lvgl_rot_priv.lvgl_height * lvgl_rot_priv.lvgl_depth;
            
            msi_output_fb(msi,fb);
        }
        else
        {
            os_sleep_ms(1);
            goto __lvgl_osd_msi_get_fb_again;
        }

    }
}


uint8_t * lvgl_rotate_task_init(void *d, lvgl_flush_ready_cb cb, void* cb_user_data, uint16_t w, uint16_t h, uint8_t depth)
{
    os_memset(&lvgl_rot_priv, 0, sizeof(struct lvgl_rotate_task_priv));

    if(d == NULL || cb == NULL || w == NULL || h == NULL) {
        goto __err_exit;
    }

    lvgl_rot_priv.lvgl_width = w;
    lvgl_rot_priv.lvgl_height = h;
    lvgl_rot_priv.lvgl_depth = depth / 8;

    lvgl_rot_priv.rotate_dst_tem_buf = STREAM_LIBC_ZALLOC(lvgl_rot_priv.lvgl_width*LV_FRAME_ROTATE_LANE_NUM*lvgl_rot_priv.lvgl_depth);
    if (!lvgl_rot_priv.rotate_dst_tem_buf) {
        goto __err_exit;
    }

    lvgl_rot_priv.rotate_src_tem_buf = STREAM_LIBC_ZALLOC(lvgl_rot_priv.lvgl_width*LV_FRAME_ROTATE_LANE_NUM*lvgl_rot_priv.lvgl_depth);
    if (!lvgl_rot_priv.rotate_src_tem_buf) {
        goto __err_exit;
    }

    lvgl_rot_priv.current_buff = STREAM_MALLOC(lvgl_rot_priv.lvgl_width*lvgl_rot_priv.lvgl_height*lvgl_rot_priv.lvgl_depth);
    if (!lvgl_rot_priv.current_buff) {
        goto __err_exit;
    }
    
    lvgl_rot_priv.current_send_enc_buff = STREAM_MALLOC(lvgl_rot_priv.lvgl_width*lvgl_rot_priv.lvgl_height*lvgl_rot_priv.lvgl_depth);
    if (!lvgl_rot_priv.current_send_enc_buff) {
        goto __err_exit;
    }

    lvgl_rot_priv.lvgl_osd_msi = (struct msi*)d;

    lvgl_rot_priv.cb = cb;
    lvgl_rot_priv.cb_user_data = cb_user_data;

    os_event_init(&lvgl_rot_priv.event);

    lvgl_frame_rotate_rpc_init(lvgl_rot_priv.lvgl_width, lvgl_rot_priv.lvgl_height, lvgl_rot_priv.lvgl_depth, lvgl_rot_priv.rotate_src_tem_buf, lvgl_rot_priv.rotate_dst_tem_buf, lvgl_rot_priv.current_buff, lvgl_rot_priv.current_send_enc_buff);

    int ret = os_task_create("lvgl_rotate_task", lvgl_rotate_task, NULL, OS_TASK_PRIORITY_ABOVE_NORMAL, NULL, NULL, 1024);
    if (!ret) {
        goto __err_exit;
    }

    return lvgl_rot_priv.current_buff;

__err_exit:
    if (lvgl_rot_priv.rotate_src_tem_buf) {
        STREAM_LIBC_FREE(lvgl_rot_priv.rotate_src_tem_buf);
        lvgl_rot_priv.rotate_src_tem_buf = NULL;
    }

    if (lvgl_rot_priv.rotate_dst_tem_buf) {
        STREAM_LIBC_FREE(lvgl_rot_priv.rotate_dst_tem_buf);
        lvgl_rot_priv.rotate_dst_tem_buf = NULL;
    }

    if (lvgl_rot_priv.current_buff) {
        STREAM_FREE(lvgl_rot_priv.current_buff);
        lvgl_rot_priv.current_buff = NULL;
    }
 
    if (lvgl_rot_priv.current_send_enc_buff) {
        STREAM_FREE(lvgl_rot_priv.current_send_enc_buff);
        lvgl_rot_priv.current_send_enc_buff = NULL;
    }

    return 0;
}

void lvgl_rotate_task_deinit()
{
    os_event_set(&lvgl_rot_priv.event, LV_ROT_DELETE, NULL);

    os_sleep_ms(100);

    if (lvgl_rot_priv.rotate_src_tem_buf) {
        STREAM_LIBC_FREE(lvgl_rot_priv.rotate_src_tem_buf);
        lvgl_rot_priv.rotate_src_tem_buf = NULL;
    }

    if (lvgl_rot_priv.rotate_dst_tem_buf) {
        STREAM_LIBC_FREE(lvgl_rot_priv.rotate_dst_tem_buf);
        lvgl_rot_priv.rotate_dst_tem_buf = NULL;
    }

    if (lvgl_rot_priv.current_buff) {
        STREAM_FREE(lvgl_rot_priv.current_buff);
        lvgl_rot_priv.current_buff = NULL;
    }
 
    if (lvgl_rot_priv.current_send_enc_buff) {
        STREAM_FREE(lvgl_rot_priv.current_send_enc_buff);
        lvgl_rot_priv.current_send_enc_buff = NULL;
    }
}

#endif