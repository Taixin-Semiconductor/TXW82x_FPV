#ifndef __CLASS_CDC_H__
#define __CLASS_CDC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <rtthread.h>

#define SEND_ENCAPSULATED_COMMAND 0x00
#define GET_ENCAPSULATED_RESPONSE 0x01
// CDC PSTN Subclass
#define SET_LINE_CODING 0x20
#define GET_LINE_CODING 0x21
#define SET_CONTROL_LINE_STATE 0x22

#define CDC_RX_BUFSIZE          128
#define CDC_TX_BUFSIZE          1024

/* The baudrate can be defined as*/
#define BAUD_RATE_2400                  2400
#define BAUD_RATE_4800                  4800
#define BAUD_RATE_9600                  9600
#define BAUD_RATE_19200                 19200
#define BAUD_RATE_38400                 38400
#define BAUD_RATE_57600                 57600
#define BAUD_RATE_115200                115200
#define BAUD_RATE_230400                230400
#define BAUD_RATE_460800                460800
#define BAUD_RATE_921600                921600
#define BAUD_RATE_2000000               2000000
#define BAUD_RATE_3000000               3000000
/* Data bits can be defined as*/
#define DATA_BITS_5                     5
#define DATA_BITS_6                     6
#define DATA_BITS_7                     7
#define DATA_BITS_8                     8
#define DATA_BITS_9                     9
/* Stop bits can be defined as */
#define STOP_BITS_1                     0
#define STOP_BITS_2                     1
#define STOP_BITS_3                     2
#define STOP_BITS_4                     3
/* Parity bits can be defined as */
#define PARITY_NONE                     0
#define PARITY_ODD                      1
#define PARITY_EVEN                     2
/* Bit order can be defined as */
#define BIT_ORDER_LSB                   0
#define BIT_ORDER_MSB                   1
/* Mode canbe defined as */
#define NRZ_NORMAL                      0   /* normal mode */
#define NRZ_INVERTED                    1   /* inverted mode */
/* Default size of the receive data buffer */
#define RT_SERIAL_RB_BUFSZ              64

struct ucdc_data
{
    struct uinstance* device;
    upipe_t pipe_in;
    upipe_t pipe_out;
    struct usb_cdc_line_coding* line_coding;

    rt_uint8_t rx_rbp[CDC_RX_BUFSIZE];
    struct rt_ringbuffer rx_ringbuffer;
    rt_uint8_t tx_rbp[CDC_TX_BUFSIZE];
    struct rt_ringbuffer tx_ringbuffer;

    rt_thread_t thread;
    rt_uint32_t thread_state;

};
typedef struct ucdc_data* ucdc_data_t;

struct usb_cdc_line_coding {
    rt_uint32_t dwDTERate;
    rt_uint8_t bCharFormat;
    rt_uint8_t bParityType;
    rt_uint8_t bDataBits;
} __attribute__((packed));

rt_err_t rt_usbh_cdc_send_command(uinst_t device, void* buffer, int nbytes);
rt_err_t rt_usbh_cdc_get_response(uinst_t device, void* buffer, int nbytes);
rt_err_t rt_usbh_cdc_get_line_coding(uinst_t device, int intf, void* buffer);
rt_err_t rt_usbh_cdc_set_line_coding(uinst_t device, int intf, void* buffer);
rt_err_t rt_usbh_cdc_set_control_line_state(uinst_t device, int intf, void * buffer, int len);
void analysis_cdc_line_coding(struct usb_cdc_line_coding * line_coding);
void rt_usbh_cdc_trans_init();
void rt_usbh_cdc_trans_deinit();

#ifdef __cplusplus
}
#endif

#endif