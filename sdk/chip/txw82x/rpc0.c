#include "basic_include.h"
#include "lib/common/rbuffer.h"
#include "lib/common/sysevt.h"
#include "lib/rpc/cpurpc.h"
#include "lib/umac/ieee80211.h"
#include "lib/lvgl_rotate_rpc/lvgl_rotate_rpc.h"

extern void *sys_wifi_register(uint32 priv);
extern int32 sys_wifi_recv(void *priv, uint8 *data, uint32 len, uint32 flags);
extern int32 sys_wifi_event_cb(uint8 ifidx, uint16 evt, uint32 param1, uint32 param2);
extern int32 atcmd_recv(uint8 *data, int32 len);
extern int32 hci_host_recv(uint32 data, uint32 len);

static const void *rpc_funcs[CPU0_RPC_FUNCID_NUM] = {
    NULL,
    RPC_FUNC_DEF(sys_wifi_register),
    RPC_FUNC_DEF(sys_wifi_recv),
    RPC_FUNC_DEF(sys_event_new),
    RPC_FUNC_DEF(sys_wifi_event_cb),
    RPC_FUNC_DEF(atcmd_recv),
    
    //BT
    RPC_FUNC_DEF(hci_host_recv),

    //lvgl
    RPC_FUNC_DEF(lvgl_frame_rotate_rpc_sync),
};

const void *cpu_rpc_func(uint32 func_id)
{
    if (func_id < ARRAY_SIZE(rpc_funcs)) {
        return rpc_funcs[func_id];
    }
    return NULL;
}

int32 cpu1_run_func(void *func, uint32 p1, uint32 p2, uint32 p3)
{
    uint32 args[] = {(uint32)func, p1, p2, p3};
    return CPU_RPC_CALL(cpu1_run_func);
}

int32 lmac_ioctl(void * lops, uint32 cmd, uint32 param1, uint32 param2)
{
    uint32 args[] = {(uint32)lops, cmd, param1, param2};
    return CPU_RPC_CALL(lmac_ioctl_rpc);
}

int32 cpu1_atcmd_recv(char *data, uint32 len)
{
    uint32 args[] = {(uint32)data, len};
    return CPU_RPC_CALL(cpu1_atcmd_recv);
}

