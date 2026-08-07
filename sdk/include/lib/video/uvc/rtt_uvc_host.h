#ifndef RTT_UVC_HOST_H
#define RTT_UVC_HOST_H

#include "basic_include.h"
#include "hal/usb_device.h"

#include "usbh_video.h"
#include "dev/usb/uvc_host.h"

#define UVC_DEFAULT_FRAME_NUM   2

#define UVC_DEFAULT_BLANK_NUM   8
#define UVC_DEFAULT_BLANK_LEN   16 * 1024

#define MAX_DEVICES     4       // 最大支持设备数

enum uvc_device_frame_state {
    UVC_DEVICE_FRAME_STATE_IDLE = 0,
    UVC_DEVICE_FRAME_STATE_BUSY,
    UVC_DEVICE_FRAME_STATE_AVAILABLE,
    UVC_DEVICE_FRAME_STATE_USING,
};

enum uvc_device_frame_end {
    UVC_DEIVCE_FRAME_NOT_END = 0,
    UVC_DEIVCE_FRAME_END,
    UVC_DEVICE_FRAME_ERROR,
};

enum uvc_device_register_state {
	UVC_DEVICE_REGISTER_IDLE = 0,
	UVC_DEVICE_REGISTER_AVAILABLE,
	UVC_DEVICE_REGISTER_BUSY,
};

enum uvc_device_blank_state {
	UVC_DEVICE_BLANK_STATE_IDLE = 0,
	UVC_DEVICE_BLANK_STATE_BUSY,
	UVC_DEVICE_BLANK_STATE_AVAILABLE,
};

// UVC设备实例结构体
typedef struct {
    uint8_t  dev_num;           // 关联的设备号
    uint8_t  is_register;
    uint8_t  blank_num;
    uint32_t  blank_len;
    struct list_head free_uvc_tab;  //空闲节点链表
    uint32_t frame_counter;
    uint32_t filter_count;
    uint8_t last_fid;
	uint32_t expected_fid;
    uint8_t initial_fid_detected;
    UVC_BLANK *blank_buffer;
    UVC_MANAGE uvc_msg_frame[UVC_DEFAULT_FRAME_NUM];
    UVC_MANAGE *current_frame;
	UVC_BLANK *current_blank;
    struct usb_device *p_dev;
} UVC_Device;

bool uvc_device_pool_init();
void uvc_device_pool_deinit();
void* register_uvc_device(uint8_t dev_num, uint8_t blank_num, uint32_t blank_len, void* p_dev);
bool unregister_uvc_device(void* device);

struct list_head *uvc_device_user_get_frame(void* device);
void uvc_device_user_free_blank_node(void* device, struct list_head *del);
void uvc_device_user_del_frame(void* device, void *get_f);

void process_uvc_payload(uint8_t dev_num, uint8_t ep_type, uint8_t video_format, uint8_t *payload, uint32_t payload_len, bool drop);

#endif