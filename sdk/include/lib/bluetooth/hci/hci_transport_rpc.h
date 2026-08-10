#ifndef HCI_TRANSPORT_RPC_H
#define HCI_TRANSPORT_RPC_H

#include "basic_include.h"

#if defined __cplusplus
extern "C" {
#endif

int32 hci_controller_recv(uint32 data, uint32 len);
int32 hci_host_recv(uint32 data, uint32 len);
/* API_END */

#if defined __cplusplus
}
#endif

#endif // HCI_TRANSPORT_RPC_H
