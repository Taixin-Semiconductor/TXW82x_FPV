#include "basic_include.h"
#include "lvgl_rotate_rpc.h"
#include "lib/rpc/cpurpc.h"
#include "lib/lvgl_rotate_rpc/lvgl_rotate_msi.h"


int32 lvgl_frame_rotate_rpc_init(uint16_t w, uint16_t h, uint8_t depth, void *rotate_src_tem_buf, void *rotate_dst_tem_buf, void *frame_src_buff, void *frame_dst_buff)
{
    #if LVGL_HW_ROTATE_RPC_EN
    uint32 args[] = {(uint32)w, (uint32)h, (uint32)depth, (uint32)rotate_src_tem_buf, (uint32)rotate_dst_tem_buf, (uint32)frame_src_buff, (uint32)frame_dst_buff};
    return CPU_RPC_CALL(lvgl_frame_rotate_rpc_init);   
    #else
    return 0;
    #endif
}

int32 lvgl_frame_rotate_rpc(int is_270)
{
    #if LVGL_HW_ROTATE_RPC_EN
    uint32 args[] = {(uint32)is_270};
    return CPU_RPC_CALL(lvgl_frame_rotate_rpc);   
    #else
    return 0;
    #endif
}

int32 lvgl_frame_rotate_rpc_sync()
{
    #if LVGL_HW_ROTATE_RPC_EN
    lvgl_rotate_frame_end();
    #endif
    return 0;
}