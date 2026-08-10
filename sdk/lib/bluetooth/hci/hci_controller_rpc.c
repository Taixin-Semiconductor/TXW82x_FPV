#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "osal/string.h"
#include "osal/mutex.h"
#include "osal/work.h"
#include "lib/common/rbuffer.h"
#include "lib/rpc/cpurpc.h"

int32 hci_controller_recv(uint32 data, uint32 len)
{
    uint32 args[] = {data, len};
    return CPU_RPC_CALL(hci_controller_recv);
}

