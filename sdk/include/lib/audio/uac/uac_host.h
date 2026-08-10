#ifndef _UAC_HOST_H_
#define _UAC_HOST_H_
#include "basic_include.h"
#include "hal/usb_device.h"

typedef struct{
	struct list_head list;
	uint8 *data_addr;     //当前写入数据的地址
	uint32 data_len;
	uint32 respace_len;
	uint32 offset;
	uint8 sta;
}UAC_MANAGE;

typedef void (*usbh_audio_spk_tx)(void * p_dev, uint8_t ep, uint8_t *data, uint32_t len);

void del_usbmic_frame(UAC_MANAGE *uac_manage);
void del_usbspk_frame(UAC_MANAGE *uac_manage);
UAC_MANAGE *get_usbmic_frame(void);
UAC_MANAGE *get_usbspk_new_frame(uint8 grab);
uint32 get_uac_frame_datalen(UAC_MANAGE *uac_manage);
uint8 *get_uac_frame_data(UAC_MANAGE *uac_manage);
void set_uac_frame_datalen(UAC_MANAGE *uac_manage, uint32 len);
void put_usbspk_frame_to_use(UAC_MANAGE *uac_manage);
void set_uac_frame_sta(UAC_MANAGE *uac_manage, uint8 sta);
void usbspk_room_init(uint32_t empty_buf_len);
void usbmic_room_init(void);
void usbmic_room_del(void);
void usbspk_room_del(void);
void uac_stop(void);
void force_reset_usbmic_frame();
void force_reset_usbspk_frame();
int usbmic_deal(struct usb_device *p_dev, uint8_t* rx_buff);
void usbspk_tx(struct usb_device *p_dev, uint8_t ep, uint32_t len, usbh_audio_spk_tx func);

#endif