#ifndef USB20_DEV_API_H
#define USB20_DEV_API_H

/// include & define here
#include "sys_config.h"
#include "hal/usb_device.h"
#include "dev/usb/hgusb20_v1_base.h"
#include "dev/usb/hgusb20_v1_dev_ep0.h"
#include "osal/work.h"
#include "osal/event.h"
#include "lib/common/rbuffer.h"

#define USB_DEV_DEBUG_ERR_ENABLE           0
#if USB_DEV_DEBUG_ERR_ENABLE
#define USB_DEV_ERR_PRINTF(fmt, args...)               os_printf(fmt, ##args)
#else
#define USB_DEV_ERR_PRINTF(fmt, args...)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* speed flags only host */
#define HGUSB20_FLAGS_HIGH_SPEED      (0)
#define HGUSB20_FLAGS_FULL_SPEED      (30)

#define USBDMA_RX_FLAG_RXING        BIT(0)
#define USBDMA_RX_FLAG_OVERFLOW     BIT(1)
#define USBDMA_RX_FLAG_FIFOFULL     BIT(2)

#define USBDMA_TX_FLAG_TXING        BIT(0)
#define USBDMA_TX_FLAG_NULLPACKET   BIT(1)
#define USBDMA_TX_FLAG_MODE1        BIT(2)

typedef struct {
    uint8 ep0_pkt;
    uint16 time_out;
    uint8 stall;
} type_usb_host;

typedef struct  {
    union {
      struct {
        uint8 dev_hub_addr : 7,
              hub_mtt_en   : 1;
      };
        uint8 usb_hub_addr;
    } u_usb_hub_addr;
    uint8 dev_hub_port;
    uint8 dev_addr;
    
    uint8 ep_host   : 4,
          ep_host_rsv : 4;
    
#define USB_ENDPOINT_XFERTYPE_MASK  0x03    /* in bmAttributes */
#define USB_ENDPOINT_XFER_CONTROL   0
#define USB_ENDPOINT_XFER_ISOC      1
#define USB_ENDPOINT_XFER_BULK      2
#define USB_ENDPOINT_XFER_INT       3    
#define USB_ENDPOINT_SPEED_DEFAULT              0x00
#define USB_ENDPOINT_SPEED_HIGH                 0x01
#define USB_ENDPOINT_SPEED_FULL                 0x02
#define USB_ENDPOINT_SPEED_LOW                  0x03
    uint8 ep_dev    : 4,
          ep_type   : 2,
          ep_speed  : 2;

    uint16 max_pkt_size;
} hgusb20_pipe_attr ;


typedef struct {
    uint8 offset;
    uint8 rx_len;
    uint16 rx_total;
} type_ep0_ptr;


typedef enum {
	UDISK_DEV = 1,
	UVC_DEV
}USB_DEV_TYPE;


struct ep_trx_ctrl {
    uint32 addr;
    uint32 total_bytes;
    uint32 max_packet_size;
    uint32 cur_addr;
    uint32 cur_counter;
    uint32 last_counter;
    uint32 tx_null_pkt_en;
    uint32 tx_null_pkt_flag;
};


struct hgusb20_dev {
    struct usb_device       dev;
    void                   *usb_hw;
    long                    flags;
    usbdev_irq_hdl          irq_hdl;
    uint8                  *rx_buff;
    uint32                  rx_len;
    uint32                  irq_data;
    
    struct usb_device_cfg  *cfg;
    uint8                  *p_com_desc;              /* usb descriptor */
    uint8                  *p_config_desc;              /* usb descriptor */
    uint16                  vid;                        /* usb device VID */
    uint16                  pid;                        /* usb device PID */

    /* basic */
    struct hgusb20_dev_chk     usb_chk;
    struct hgusb20_dev_ctrl    usb_ctrl;
    struct hgusb20_dev_info    usb_dev;
    type_ep0_ptr               ep0_ptr;
    type_usb_host              usb_host;
    
    volatile uint32          usb_dma_int;  /* dma intr status reg value */

    volatile uint8           usb_rxdma_flag;
    volatile uint8           usb_txdma_flag;

    uint8                    ep_irq_num;
    uint8                    dma_irq_num;
    uint16_t                   usb_dev_flag;
    
    uint8                    usb_ep0_sbuf[8] __attribute__((aligned(4))); 
    /* case : usb continue send 64byte data, 2nd data NAK */
    uint8                    usb_ep0_rxbuf[68 + 68] __attribute__((aligned(4))); 

    uint8                    usb_ep0_sta[8];
    RBUFFER_DEF_R(rb, uint8);
    struct os_work             ep0_work;
    /* trx_lock[00:15] for EP_IN  */
    /* trx_lock[15:31] for EP_OUT */
    struct os_event            trx_lock;
    struct os_event            trx_done;

    struct ep_trx_ctrl      ep_trans_ctrl[7];
    struct ep_trx_ctrl      ep_recevice_ctrl[7];
    
#ifdef CONFIG_SLEEP
    uint32                  sys_regs[2];
#endif
};

/* hgusb20_base */
/** 
  * @brief  read usb register
  * @param  p_dev   : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  ep      : 
  * @param  addr    : register address offset
  * @param  data    : data return 
  * @return         : TRUE or FLASE
  * @note  
  */
bool hgusb20_read_reg(struct hgusb20_dev *p_dev, uint8 ep, uint32 addr, uint32 *data);
/** 
  * @brief  write usb register
  * @param  p_dev   : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  ep      : 
  * @param  addr    : register address offset
  * @param  data    : data return 
  * @return         : TRUE or FLASE
  * @note  
  */
bool hgusb20_write_reg(struct hgusb20_dev *p_dev, uint8 ep, uint32 addr, uint32 data);

void hgusb20_switch_otg(struct hgusb20_dev *p_dev);
void hgusb20_switch_host(struct hgusb20_dev *p_dev);
void hgusb20_switch_stable_host(struct hgusb20_dev *p_dev);
void hgusb20_switch_stable_device(struct hgusb20_dev *p_dev);
void hgusb20_switch_stable_otg(struct hgusb20_dev *p_dev);
void hgusb20_intr_dis(struct hgusb20_dev *p_dev);
uint8_t hgusb20_connected(struct hgusb20_dev * p_dev);

uint32 hgusb20_get_rx_crc_error_cnt(struct hgusb20_dev *p_dev);

/** 
  * @brief  usb ep rx data kick
  * @param  p_dev   : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  ep      : [1,2]
  * @param  addr    : data address 
  * @param  len     : data length 
  * @return         : TRUE or FLASE
  * @note  use for host & device
  */
bool hgusb20_ep_rx_kick(struct hgusb20_dev *p_dev, uint8 ep, uint32 addr, uint32 len);
/** 
  * @brief  usb ep tx data kick
  * @param  p_dev   : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  ep      : [1,2]
  * @param  addr    : data address 
  * @param  len     : data length 
  * @return         : TRUE or FLASE
  * @note  use for host & device
  */
bool hgusb20_ep_tx_kick(struct hgusb20_dev *p_dev, uint8 ep, uint32 addr, uint32 len);
bool hgusb20_ep_rx_abort(struct hgusb20_dev *p_dev, uint8 ep);
bool hgusb20_ep_tx_abort(struct hgusb20_dev *p_dev, uint8 ep);

/** 
  * @brief  usb ep get rx data length from dma 
  * @param  p_dev   : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  ep      : [1,2]
  * @return         : rx data length
  * @note  use for host & device
  */
uint32 hgusb20_ep_get_dma_rx_len(struct hgusb20_dev *p_dev, uint8 ep);
/** 
  * @brief  usb ep get rx data length from sie 
  * @param  p_dev   : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  ep      : [1,2]
  * @return         : rx data length
  * @note  use for host & device
  */
uint32 hgusb20_ep_get_sie_rx_len(struct hgusb20_dev *p_dev, uint8 ep);
/** 
  * @brief  usb ep get tx data length  
  * @param  p_dev   : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  ep      : [1,2]
  * @return         : tx data length (same with len in hgusb20_ep_tx_kick)
  * @note  use for host & device
  */
uint32 hgusb20_ep_get_tx_len(struct hgusb20_dev *p_dev, uint8 ep);


/** 
  * @brief  check if usb device online
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return 
  *         - TRUE    : online.
  *         - FALSE   : not online.
  * @note             : For USB Host
  */
bool hgusb20_is_device_online(struct hgusb20_dev *p_dev);

/** 
  * @brief  check if usb host online
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return 
  *         - TRUE    : online.
  *         - FALSE   : not online.
  * @note             : For USB Device
  */
bool hgusb20_is_host_online(struct hgusb20_dev *p_dev);
int32 hgusb20_is_host_mode(struct hgusb20_dev *p_dev);

/** 
  * @brief  suspend usb device .
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return          : 
  * @note            : For USB Host
  */
void hgusb20_host_suspend(struct hgusb20_dev *p_dev);
/** 
  * @brief  resume usb device .
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return          : 
  * @note            : For USB Host
  */
void hgusb20_host_resume(struct hgusb20_dev *p_dev);

/** 
  * @brief  usb host set address(host address)
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  addr     : 
  * @return 
  * @note   use after usb_host_set_address(device address) cmd
  */
void hgusb20_set_address(struct hgusb20_dev *p_dev, uint8 addr);

/** 
  * @brief  usb host bus reset request
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return 
  * @note   
  */
uint8 hgusb20_host_reset(struct hgusb20_dev *p_dev);

/** 
  * @brief  remote_wakeup usb host .
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return          : 
  * @note            : For USB Device
  */
void hgusb20_dev_remote_wakeup(struct hgusb20_dev *p_dev);


/* hgusb20_dev_api */
/** 
  * @brief  attach usb device
  * @param  p_dev   : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  dev_id  : 
  * @return         : err_code
  * @note  
  */
int32 hgusb20_dev_attach(uint32 dev_id, struct hgusb20_dev *p_dev);

/** 
  * @brief  init usb to device mode .
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return 
  *         - RET_OK  : Successfully.
  *         - RET_ERR : Failed.
  * @note
  */
uint8 hgusb20_dev_init(struct hgusb20_dev *p_dev);

/** 
  * @brief  exit usb to device mode .
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return 
  *         - RET_OK  : Successfully.
  *         - RET_ERR : Failed.
  * @note
  */
void hgusb20_dev_exit(struct hgusb20_dev *p_dev);

/** 
  * @brief  usb device ep0 tx data stage handle: 
  *         addr = p_dev->ep0_ptr; len = p_dev->ep0_len;
  *         1、if buf is not NULL（first packet）, update p_dev->ep0_ptr & p_dev->ep0_len
  *         2、TX one packet 
  *         3、data end if len != ep0_pkt
  *         4、auto called repeat when last tx data done while (p_dev->usb_dev.ep0_state == TRANSER_OVER_STATE)
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  buf      : valid when first packet, set NULL when other packet
  * @param  len      : valid when first packet, set 0 when other packet.
  * @return 
  *         - RET_OK  : Successfully.
  *         - RET_ERR : Failed.
  * @note             : weak function, Users can use a strong statement function replace the weak statement function.
  */
bool hgusb20_dev_ep0_tx(struct hgusb20_dev *p_dev, void *buf, uint16_t len);
/** 
  * @brief  usb device ep0 tx one packet only 
  */
bool hgusb20_dev_ep0_tx_rtt(struct hgusb20_dev *p_dev, void *buf, uint16_t len);
bool hgusb20_dev_ep0_tx_abort(struct hgusb20_dev *p_dev);

/** 
  * @brief  usb ep rx data stage handle
  * @param  p_dev   : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  ep      : [1,2]
  * @param  addr    : data address 
  * @param  len     : data length 
  * @return         : TRUE or FLASE
  * @note  use for host & device
  */
bool hgusb20_dev_ep0_rx(struct hgusb20_dev *p_dev, void *buf, uint16_t len);
/** 
  * @brief  usb device ep0 rx one packet only 
  */
bool hgusb20_dev_ep0_rx_rtt(struct hgusb20_dev *p_dev, void *buf, uint16_t len);
bool hgusb20_dev_ep0_rx_abort(struct hgusb20_dev *p_dev);

/** 
  * @brief  usb device clear_stall_flag
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return 
  * @note
  */
void hgusb20_dev_clear_stall_flag(struct hgusb20_dev *p_dev);
/** 
  * @brief  usb device tx zero-packet & rx packet to usb_ep0_rxbuf
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return 
  * @note
  */
bool hgusb20_dev_ep0_clrrx_pkt0(struct hgusb20_dev *p_dev);

/** 
  * @brief  usb device bus reset
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return 
  * @note
  */
void hgusb20_dev_reset(struct hgusb20_dev *p_dev);

/** 
  * @brief  usb device stall_ep
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  epnum    : epnum, set BIT(7) when tx endpiont.
  * @return 
  * @note
  */
void hgusb20_dev_stall_ep(struct hgusb20_dev *p_dev, uint8 epnum);
/** 
  * @brief  usb device clear stall_ep
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  epnum    : epnum, set BIT(7) when tx endpiont.
  * @return 
  * @note
  */
void hgusb20_dev_clear_ep(struct hgusb20_dev *p_dev, uint8 epnum);
/** 
  * @brief  usb device ep reset all endpiont in ep_cfg
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return 
  * @note
  */
void hgusb20_dev_bot_reset(struct hgusb20_dev *p_dev);
/** 
  * @brief  usb device cmd handle ：get descriptor
  *         1、kick next rx to p_dev->usb_ep0_rxbuf
  *         2、tx data(addr = buf, length = len)
  * @param  p_dev   : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return         : TRUE or FLASE
  * @note
  */
bool hgusb20_dev_ep0_descriptor(struct hgusb20_dev *p_dev, void *buf, uint16_t len);
/** 
  * @brief  usb device standard cmd handle 
  *         1、
  *         2、handle DATA(IN or OUT)
  *         3、STATUS(zero-len packet)  
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return 
  * @note            : weak function, Users can use a strong statement function replace the weak statement function.
  *                    these function is called by func "hgusb20_dev_ep0_standard"
  */

 uint8 hgusb20_dev_cfg_desc_prepare(struct hgusb20_dev *p_dev);
__weak bool hgusb20_dev_ep0_get_descriptor(struct hgusb20_dev *p_dev);
__weak bool hgusb20_dev_ep0_get_configuration(struct hgusb20_dev *p_dev);
__weak bool hgusb20_dev_ep0_set_configuration(struct hgusb20_dev *p_dev);
__weak bool hgusb20_dev_ep0_get_interface(struct hgusb20_dev *p_dev);
__weak bool hgusb20_dev_ep0_set_interface(struct hgusb20_dev *p_dev);
__weak bool hgusb20_dev_ep0_set_address(struct hgusb20_dev *p_dev, uint8_t address);
__weak bool hgusb20_dev_ep0_get_status(struct hgusb20_dev *p_dev);
__weak bool hgusb20_dev_ep0_clr_feature(struct hgusb20_dev *p_dev);
__weak bool hgusb20_dev_ep0_set_feature(struct hgusb20_dev *p_dev);

/** 
  * @brief  usb device ep0 setup handle (standard cmd)
  *         1、
  *         2、handle DATA(IN or OUT)
  *         3、STATUS(zero-len packet)
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return         : TRUE or FLASE
  * @note            : weak function, Users can use a strong statement function replace the weak statement function.
  *                    the function is called by func "hgusb20_dev_ep0_request"
  */
__weak bool hgusb20_dev_ep0_standard(struct hgusb20_dev *p_dev);
//__weak bool hgusb20_dev_ep0_class_reset(struct hgusb20_dev *p_dev);
//__weak bool hgusb20_dev_ep0_class_get_lun(struct hgusb20_dev *p_dev);
//__weak bool hgusb20_dev_ep0_audio_sample_ctrl(struct hgusb20_dev *p_dev);
//__weak bool hgusb20_dev_ep0_audio_mute_ctrl(struct hgusb20_dev *p_dev);
//__weak bool hgusb20_dev_ep0_audio_volume_ctrl(struct hgusb20_dev *p_dev);

/** 
  * @brief  usb device ep0 setup handle (class cmd)
  *         1、
  *         2、handle DATA(IN or OUT)
  *         3、STATUS(zero-len packet)
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return         : TRUE or FLASE
  * @note            : weak function, Users can use a strong statement function replace the weak statement function.
  *                    the function is called by func "hgusb20_dev_ep0_request"
  */
__weak bool hgusb20_dev_ep0_class(struct hgusb20_dev *p_dev);
/** 
  * @brief  usb device ep0 setup handle (vendor cmd)
  *         1、
  *         2、handle DATA(IN or OUT)
  *         3、STATUS(zero-len packet)
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return         : TRUE or FLASE
  * @note            : weak function, Users can use a strong statement function replace the weak statement function.
  *                    the function is called by func "hgusb20_dev_ep0_request"
  */
__weak bool hgusb20_vendor_cmd(struct hgusb20_dev *p_dev);
/** 
  * @brief  usb device ep0 setup handle
  *         1、
  *         2、handle DATA(IN or OUT)
  *         3、STATUS(zero-len packet)
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return         : TRUE or FLASE
  * @note            : weak function, Users can use a strong statement function replace the weak statement function.
  *                    
  */
__weak bool hgusb20_dev_ep0_request(struct hgusb20_dev *p_dev);

/** 
  * @brief  usb device config state indication
  *         1、 p_dev->usb_dev.cur_state = USB_CONFIG_STATE;
  *         2、p_dev->flags |= BIT(HGUSB20_FLAGS_READY);
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return         : 
  * @note            : weak function, Users can use a strong statement function replace the weak statement function.
  *                    
  */
__weak void hgusb20_dev_state_config(struct hgusb20_dev *p_dev);

///** 
//  * @brief  usb device ep0 interrupt handle
//  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
//  * @return 
//  * @note            : weak function, Users can use a strong statement function replace the weak statement function.
//  *                    !! Don't recommend  a strong statement function
//  */
//__weak void hgusb20_dev_ep0_func(struct hgusb20_dev *p_dev);

/** 
  * @brief  usb device endpoint init & deinit
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  ep_addr    : ep->ep_desc->bEndpointAddress
  * @param  max_pkt_size    :ep->ep_desc->wMaxPacketSize
  * @param  attributes    : ep->ep_desc->bmAttributes.
  * @return         : 
  * @note            : weak function, Users can use a strong statement function replace the weak statement function.
  */
void hgusb20_dev_ep_init(struct hgusb20_dev *p_dev, uint8 ep_addr, uint16 max_pkt_size, uint8 attributes);
void hgusb20_dev_ep_deinit(struct hgusb20_dev *p_dev, uint8 ep_addr);
/** 
  * @brief  usb device IN data kick （ tx data ）
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  ep       : ep_num = ep->ep_desc->bEndpointAddress & 0x7F
  * @param  buff     : word aligned
  * @param  len      : 
  * @param  sync     : 1: sync(recommand)  0: async
  * @return          : error code
  * @note            : weak function, Users can use a strong statement function replace the weak statement function.
  */
int32 hgusb20_dev_write(struct usb_device *usb,  uint8 ep, uint8 *buff, uint32 len, uint8 sync);
int32 hgusb20_dev_read(struct usb_device *p_usb_d,  uint8 ep, uint8 *buff, uint32 len, uint8 sync);
int32 hgusb20_dev_close(struct usb_device *p_usb_d);



/* hgusb20_host_api */

/** 
  * @brief  attach usb host
  * @param  p_dev   : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  dev_id  : 
  * @return         : err_code
  * @note  
  */
int32 hgusb20_host_attach(uint32 dev_id, struct hgusb20_dev *p_dev);

/** 
  * @brief  init usb to host mode .
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return 
  *         - RET_OK  : Successfully.
  *         - RET_ERR : Failed.
  * @note
  */
bool hgusb20_host_init(struct hgusb20_dev *p_dev);

/** 
  * @brief  usb host ep0 cmd request
  *         1. SETUP: send setup packet , addr = p_dev->usb_ctrl.cmd, len = 8
  *         2. DATA : addr = p_buf, len = p_dev->usb_ctrl.cmd.length for rx/tx(p_dev->usb_ctrl.cmd.rtype & USB_DIR_IN)
            3. STA  : if DATA tx null packet else rx null packet
  * @param  p_dev   : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  p_buf   : data buffer.
  * @return         : TRUE or FLASE
  * @note   Another choice user can realize function by calling "hgusb20_host_ep0_setup" & "hgusb20_host_ep0_rx" & "hgusb20_host_ep0_tx"
  */
bool usb_host_ep0_request(struct hgusb20_dev *p_dev, uint8 *pBuf);

/** 
  * @brief  usb host ep0 send SETUP packet
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return 
  * @note
  */
 uint32_t hgusb20_host_ep0_setup(struct hgusb20_dev *p_dev);

/** 
  * @brief  usb host ep0 send IN packet & request a data packet
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @return 
  * @note   addr = p_dev->usb_ep0_buf, len = p_dev->ep0_ptr.rx_len
  */
uint32_t hgusb20_host_ep0_rx(struct hgusb20_dev *p_dev, uint8 *pBuf, bool first_pkt);
/** 
  * @brief  usb host ep0 send IN packet & request a data packet
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  len      : 
  * @return 
  * @note   addr = p_dev->usb_ep0_buf, len = p_dev->ep0_ptr.rx_len
  */
uint32_t hgusb20_host_ep0_tx(struct hgusb20_dev *p_dev, uint8 *pBuf, uint8 len);

/** 
  * @brief  usb host endpiont init (TXW81X)
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  ep_host  : ep pipe of host
  * @param  ep_dev   : ep pipe of device
  * @param  ep_type  : USB_ENDPOINT_XFERTYPE_MASK @ref usb_ch9.h
  * @param  max_pkt_size : 
  * @return 
  * @note 
  */
void hgusb20_host_tx_ep_init(struct hgusb20_dev *p_dev, uint8 ep_host, uint8 ep_dev, uint8 ep_type, uint16 max_pkt_size);
void hgusb20_host_rx_ep_init(struct hgusb20_dev *p_dev, uint8 ep_host, uint8 ep_dev, uint8 ep_type, uint16 max_pkt_size);

/** 
  * @brief  usb host pipe init (TXW82X)
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  usb_pipe : ep pipe of host
  * @return 
  * @note 
  */
void hgusb20_host_tx_pipe_combine(struct hgusb20_dev *p_dev, hgusb20_pipe_attr *usb_pipe); 
void hgusb20_host_rx_pipe_combine(struct hgusb20_dev *p_dev, hgusb20_pipe_attr *usb_pipe);

/** 
  * @brief  hgusb20_host_clear_data_toggle
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  ep_num   : [1,2]
  * @param  ep_type  : EndpointDir BIT[7]：IN = 1, OUT = 0
  * @return          : 
  * @note  
  */
void hgusb20_host_clear_data_toggle(struct hgusb20_dev *p_dev, uint8 ep_num, uint8 ep_type);

/** 
  * @brief  hgusb20_host_flush_fifo
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  ep_num   : [1,2]
  * @param  ep_type  : EndpointDir BIT[7]：IN = 1, OUT = 0
  * @return          : 
  * @note  
  */
void hgusb20_host_flush_fifo(struct hgusb20_dev *p_dev, uint8 ep_num, uint8 ep_type);

/** 
  * @brief  usb host get stall status 
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  ep_num   : [0,2]
  * @param  ep_type  : EndpointDir BIT[7]：IN = 1, OUT = 0
  * @return          : TRUE or FLASE
  * @note  
  */
bool hgusb20_host_is_rx_stall(struct hgusb20_dev *p_dev, uint8 ep_num, uint8 ep_type);
/** 
  * @brief  usb host get xact error status (continous no response three times)
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  ep_num   : [0,2]
  * @param  ep_type  : EndpointDir BIT[7]：IN = 1, OUT = 0
  * @return          : TRUE or FLASE
  * @note  
  */
bool hgusb20_host_is_xact_err(struct hgusb20_dev *p_dev, uint8 ep_num, uint8 ep_type);
/** 
  * @brief  usb host get nak timeout status 
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  ep_num   : [0,2]
  * @param  ep_type  : EndpointDir BIT[7]：IN = 1, OUT = 0
  * @return          : TRUE or FLASE
  * @note  
  */
bool hgusb20_host_is_nak_timeout(struct hgusb20_dev *p_dev, uint8 ep_num, uint8 ep_type);
/** 
  * @brief  usb host get rx crc status 
  * @param  p_dev    : hgusb20_dev use @ref dev_get() function to get the handle.
  * @param  ep_num   : [0,2]
  * @return          : TRUE or FLASE
  * @note  
  */
bool hgusb20_host_is_crc_err(struct hgusb20_dev *p_dev, uint8 ep_num);

void hgusb20_host_reset_phy(struct hgusb20_dev * p_dev);

int32 hgusb20_host_set_interval(struct hgusb20_dev *p_dev, uint8_t ep_host, uint8_t mode, uint32_t interval);
void hgusb20_host_reset_ep_rxcsr(struct hgusb20_dev *p_dev, uint8 ep_num);
void hgusb20_host_reset_ep_txcsr(struct hgusb20_dev *p_dev, uint8 ep_num);
void hgusb20_host_hub_ep0_mange(struct hgusb20_dev *p_dev, hgusb20_pipe_attr *usb_pipe);
/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif



#endif /* USB_DEV_API_H */
