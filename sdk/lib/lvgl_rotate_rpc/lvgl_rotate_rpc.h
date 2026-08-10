#ifndef __LVGL_ROTATE_RPC_H__
#define __LVGL_ROTATE_RPC_H__

extern int32 lvgl_frame_rotate_rpc_init(uint16_t w, uint16_t h, uint8_t depth, void *rotate_src_tem_buf, void *rotate_dst_tem_buf, void *frame_src_buff, void *frame_dst_buff);
extern int32 lvgl_frame_rotate_rpc(int is_270);
extern int32 lvgl_frame_rotate_rpc_sync();

#endif