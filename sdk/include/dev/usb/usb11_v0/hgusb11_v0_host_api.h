#ifndef _HG_USB11_V0_HOST_H
#define _HG_USB11_V0_HOST_H

#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/event.h"
#include "hal/usb_device.h"
#include "dev/usb/usb11_v0/hgusb11_v0_dev_api.h"

#ifdef __cplusplus
    extern "C" {
#endif

#define USB11_CORE_HOST_EP_RX               0
#define USB11_CORE_HOST_EP_TX               1

#define USB11_CORE_HOST_EP0_TIMEOUT      18000

#define USB11_EP_ATTR_CONTROL             0x00
#define USB11_EP_ATTR_ISOC                0x01
#define USB11_EP_ATTR_BULK                0x02
#define USB11_EP_ATTR_INT                 0x03
#define USB11_EP_ATTR_TYPE_MASK           0x03

#define USB11_DIR_OUT                       0
#define USB11_DIR_IN                     0x80

enum hgusb11_host_flags {
    HGUSB11_HOST_FLAGS_OPENED            =    BIT(0),
    HGUSB11_HOST_FLAGS_READY             =    BIT(1),
    HGUSB11_HOST_FLAGS_SUSPEND           =    BIT(2),
    HGUSB11_HOST_FLAGS_DMA_MODE          =    BIT(3),
    HGUSB11_HOST_FLAGS_SOFT_SUBPACKAGE   =    BIT(4),
};

enum hgusb11_host_speed_det {
    HGUSB11_HOST_NONE = 0,
    HGUSB11_HOST_FS,
    HGUSB11_HOST_LS,
};

struct hgusb11_host {
    struct usb_device           dev;
    void                        *usb_hw;
    usbdev_irq_hdl              irq_hdl;
    uint32                      irq_data;
    uint8                       ep_irq_num;
    uint8                       dma_irq_num;
    /*
     * USB EP0的控制块
     */
    struct _usb_ep0_ctrl        ep0_ctrl;
    /* Pointer of common areas */
    void *                      p_comm;
    
    struct os_event             trx_lock;
    struct os_event             trx_done;

    struct usb11_ep_trx_ctrl      ep_trans_ctrl[4];
    struct usb11_ep_trx_ctrl      ep_recevice_ctrl[4];

    //状态位
    uint32                      flags;
};

int32 hgusb11_v0_host_dev_speed_detect();
int32 hgusb11_v0_host_set_address(uint8 addr);
int32 hgusb11_v0_host_reset();
int32 hgusb11_v0_host_is_ep_crc_err(uint8 ep_num);
int32 hgusb11_v0_host_is_rx_stall(uint8 ep_num, uint8 ep_type);
int32 hgusb11_v0_host_is_xact_err(uint8 ep_num, uint8 ep_type);
int32 hgusb11_v0_host_is_nak_timeout(uint8 ep_num, uint8 ep_type);
void hgusb11_v0_host_clear_data_toggle(uint8 ep_num, uint8 ep_type);
void hgusb11_v0_host_reset_ep_rxcsr(uint8 ep_num);
void hgusb11_v0_host_reset_ep_txcsr(uint8 ep_num);
int32 hgusb11_v0_host_ep_get_rx_dma_len(uint8 ep_num);
int32 hgusb11_v0_host_ep0_tx_kick(struct usb_device *p_usb_d, uint8 setup_pkt, uint8 *buf, uint32 len);
int32 hgusb11_v0_host_ep0_rx_kick(struct usb_device *p_usb_d, uint8* buf, uint32 len);
int32 hgusb11_v0_host_ep_init(struct usb_device *p_usb_d, uint8 tx_or_rx, uint8 ep_num, uint8 ep_addr, uint8 ep_type, uint32 max_pktsize);
int32 hgusb11_v0_host_set_ep_interval(uint8 tx_or_rx, uint8 ep_num, uint32 interval);
int32 hgusb11_v0_host_ep_rx_kick(struct usb_device *p_usb_d, uint8 ep_num, uint8 *buf, uint32 len);
int32 hgusb11_v0_host_ep_tx_kick(struct usb_device *p_usb_d, uint8 ep_num, uint8 *buf, uint32 len, uint8 setup_pkt);
int32 hgusb11_v0_host_ep_abort(struct usb_device *p_usb_d, uint8 tx_or_rx, uint8 ep_num);
int32 hgusb11_v0_host_attch(uint32 dev_id, struct hgusb11_host *p_dev);

#ifdef __cplusplus
}
#endif

#endif