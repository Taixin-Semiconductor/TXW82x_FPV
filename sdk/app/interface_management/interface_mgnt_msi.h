#ifndef __INTERFACE_MGNT_MSI_H__
#define __INTERFACE_MGNT_MSI_H__
typedef void (*ui_start_cb_t)(void);
void lvgl_run(ui_start_cb_t ui_start);
void lvgl_init(uint16_t w, uint16_t h, uint8_t rotate);
void lvgl_key_init(void *user_data);
void lvgl_touchpad_init(void *user_data);
#endif