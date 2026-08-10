#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/string.h"
#include "osal/mutex.h"
#include "osal/work.h"
#include "osal/semaphore.h"
#include "osal/timer.h"
#include "hal/netdev.h"
#include "hal/dma.h"
#include "hal/netdev.h"
#include "lib/heap/sysheap.h"
#include "lib/skb/skb.h"
#include "lib/skb/skb_list.h"
#include "lib/rpc/cpurpc.h"

__init void *sys_wifi_register(uint32 ifidx)
{
    uint32 args[] = {ifidx};
    int32 ret = CPU_RPC_CALL(sys_wifi_register);
    return (ret < 0 && ret > -MAX_ERRNO) ? NULL : (void *)ret;
}

int32 sys_wifi_recv(void *priv, uint8 *data, uint32 len, uint32 flags)
{
    uint32 args[] = {(uint32)priv, (uint32)data, len, flags};
    return CPU_RPC_CALL(sys_wifi_recv);
}

