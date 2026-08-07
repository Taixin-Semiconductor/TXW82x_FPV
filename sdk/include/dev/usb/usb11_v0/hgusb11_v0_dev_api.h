#ifndef _HG_USB11_V0_DEV_H
#define _HG_USB11_V0_DEV_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "hal/usb_device.h"

enum hgusb11_device_flags {
    HGUSB11_DEVICE_FLAGS_OPENED            =    BIT(0),
    HGUSB11_DEVICE_FLAGS_READY             =    BIT(1),
    HGUSB11_DEVICE_FLAGS_SUSPEND           =    BIT(2),
    HGUSB11_DEVICE_FLAGS_DMA_MODE          =    BIT(3),
    HGUSB11_DEVICE_FLAGS_SOFT_SUBPACKAGE   =    BIT(4),
};

/* USB通信控制块：USB Device Requests */
struct _usb_device_requests {
    uint8   bmrequesttype;
    uint8   brequest;
    uint16  wvalue;
    uint16  windex;
    uint16  wlength;
};


/* USB EP0控制块：状态机 */
enum _usb_ep0_state {
    USB_EP0_STATE_SETUP,
    USB_EP0_STATE_OUT,
    USB_EP0_STATE_IN,
    USB_EP0_STATE_LAST_IN,
    USB_EP0_STATE_LAST_OUT,
    USB_EP0_STATE_STATUS_IN,
    USB_EP0_STATE_STATUS_OUT,
    USB_EP0_STATE_STALL,
};


/* USB EP0控制块 */
struct _usb_ep0_ctrl {

    enum _usb_ep0_state     ep0_state;
    
    //本次要发送包的地址、总长度
    uint8                  *p_ep0_ptk2send;
    uint32                  ep0_ptk2send_size;

    //偏移量和计数
    uint32                  ep0_ptk_counter;
    uint32                  ep0_buf_offset;
   
    //单次发送包的最大长度
    uint32                  ep0_oneptk_max_size;

    uint8 ep0_buf[68]   __attribute__((aligned(4)));
};

struct usb11_ep_trx_ctrl {
    uint32 addr;
    uint32 total_bytes;
    uint32 max_packet_size;
    uint32 cur_addr;
    uint32 cur_counter;
    uint32 last_counter;
};

union __comm_state{
    uint32 comm;
    struct {
        uint32 dev_open         : 1,
               dev_host_mode    : 1,
               dev_device_mode  : 1,
               reserve          : 29;
    } comm_bits;
};
typedef union __comm_state   hgusb11_dev_v0_comm;

struct hgusb11_dev {
    struct usb_device           dev;
    void                        *usb_hw;
    usbdev_irq_hdl              irq_hdl;
    uint32                      irq_data;
    uint8                       ep_irq_num;
    uint8                       dma_irq_num;

    /* common areas */
    union __comm_state          comm_dat;
    /* Pointer of common areas */
    void                        *p_comm; 

    /*
     * USB EP0的控制块
     */
    struct _usb_ep0_ctrl        ep0_ctrl;

    struct os_event             trx_lock;
    struct os_event             trx_done;

    struct usb11_ep_trx_ctrl      ep_trans_ctrl[4];
    struct usb11_ep_trx_ctrl      ep_recevice_ctrl[4];

    //状态位
    uint32                      flags;
};


int32 hgusb11_v0_dev_ep_set_stall(struct usb_device *p_usb_d, uint8 address);
int32 hgusb11_v0_dev_ep_clear_stall(struct usb_device *p_usb_d, uint8 address);
int32 hgusb11_v0_dev_set_address(struct usb_device *p_usb_d, uint8 address);
int32 hgusb11_v0_dev_set_config(struct usb_device *p_usb_d, uint8 address);
int32 hgusb11_v0_dev_ep_enable(struct usb_device *p_usb_d, uint8 ep, uint16 max_pkt_size, uint8 attributes);
int32 hgusb11_v0_dev_ep_disable(struct usb_device *p_usb_d, uint8 ep);
int32 hgusb11_v0_dev_ep_read(struct usb_device *p_usb_d, uint8 address, void *buffer);
int32 hgusb11_v0_dev_ep_read_prepare(struct usb_device *p_usb_d, uint8 address, void *buffer, uint32 size);
int32 hgusb11_v0_dev_ep_write(struct usb_device *p_usb_d, uint8 address, void *buffer, uint32 size);
int32 hgusb11_v0_dev_ep0_send_status(struct usb_device *p_usb_d);
int32 hgusb11_v0_dev_attach(uint32 dev_id, struct hgusb11_dev *p_dev);
int32 hgusb11_v0_dev_close(struct usb_device *p_usb_d);









#ifdef __cplusplus
}
#endif

#endif /* _HG_USB11_V0_DEV_H */

