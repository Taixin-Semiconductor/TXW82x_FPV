#ifndef __LVGL_ROTATE_MSI_H__
#define __LVGL_ROTATE_MSI_H__

#define LVGL_HW_ROTATE_RPC_EN     0

typedef void (*lvgl_flush_ready_cb)(void *disp_drv);

#if LVGL_HW_ROTATE_RPC_EN
void lvgl_rotate_get_active_buff(void *data);
void lvgl_rotate_frame_end();
uint8_t *lvgl_rotate_task_init(void *d, lvgl_flush_ready_cb cb, void* cb_user_data, uint16_t w, uint16_t h, uint8_t depth);
void lvgl_rotate_task_deinit();
#endif

#endif