#ifndef _HAL_JPEG_H_
#define _HAL_JPEG_H_
#ifdef __cplusplus
extern "C" {
#endif

enum jpg_device_irq_flag {
    JPG_IRQ_FLAG_JPG_DONE     = BIT(0),
    JPG_IRQ_FLAG_JPG_BUF_FULL = BIT(1),
    JPG_IRQ_FLAG_ERROR        = BIT(2),
    JPG_IRQ_FLAG_PIXEL_DONE   = BIT(3),
    JPG_IRQ_FLAG_TIME_OUT     = BIT(4),
};

enum jpg_ioctl_cmd {
	JPG_IOCTL_CMD_IS_ONLINE,
    JPG_IOCTL_CMD_SET_ADR,
    JPG_IOCTL_CMD_SET_QT,
    JPG_IOCTL_CMD_SET_SIZE,
    JPG_IOCTL_CMD_SET_SRC_FROM,
    JPG_IOCTL_CMD_UPDATE_QT,
	JPG_IOCTL_CMD_DECODE_TAG,
	JPG_IOCTL_CMD_OPEN_DBG,
	JPG_IOCTL_CMD_SOFT_FRAME_START,
	JPG_IOCTL_CMD_SOFT_KICK,
	JPG_IOCTL_CMD_HW_CHK,
	JPG_IOCTL_CMD_IS_IDLE,
	JPG_IOCTL_CMD_BUFF_INIT,
	JPG_IOCTL_CMD_SET_SOFT_Y,
	JPG_IOCTL_CMD_SET_SOFT_UV,
	JPG_IOCTL_CMD_TIMEOUT_CNT,
	JPG_IOCTL_CMD_TIMEOUT_EN,
	JPG_IOCTL_CMD_DECAUTO_RUN_EN,
	JPG_IOCTL_CMD_DEC_LEN_CFG_EN,
	JPG_IOCTL_CMD_DEC_FLUSH_EN,
	JPG_IOCTL_CMD_CODEC_RESET,
	JPG_IOCTL_CMD_SET_DLEN,
	JPG_IOCTL_CMD_VSYNC_DLY,
	JPG_IOCTL_CMD_SET_READY,
	JPG_IOCTL_CMD_SET_OE_SELECT,	
	JPG_IOCTL_CMD_SET_OE,
	JPG_IOCTL_CMD_GET_OE,
};


typedef int32(*jpg_irq_hdl)(uint32 irq_flag, uint32 irq_data, uint32 param1, uint32 param2);

struct jpg_device {
    struct dev_obj dev;
};

struct jpeg_hal_ops{
    struct devobj_ops ops;
    int32(*open)(struct jpg_device *jpg);
    int32(*close)(struct jpg_device *jpg);
	int32(*suspend)(struct jpg_device *jpg);
	int32(*resume)(struct jpg_device *jpg);
    int32(*init)(struct jpg_device *jpg, uint32 table_idx, uint32 qt);
	int32(*decode)(struct jpg_device *jpg,uint32 photo,uint32_t len);
    int32(*ioctl)(struct jpg_device *jpg, enum jpg_ioctl_cmd cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct jpg_device *jpg, jpg_irq_hdl irq_hdl, uint32 irq_flag,  uint32 irq_data);
    int32(*release_irq)(struct jpg_device *jpg, uint32 irq_flag);
};

int32 jpg_open(struct jpg_device *jpg);
int32 jpg_close(struct jpg_device *jpg);
int32 jpg_init(struct jpg_device *jpg, uint32 table_idx, uint32 qt);
int32 jpg_updata_dqt(struct jpg_device *p_jpg, uint32 *dqtbuf);
int32 jpg_suspend(struct jpg_device *jpg);
int32 jpg_resume(struct jpg_device *jpg);
int32 jpg_set_size(struct jpg_device *p_jpg, uint32 h, uint32 w);
int32 jpg_set_addr(struct jpg_device *jpg, uint32 addr, uint32 buflen);
int32 jpg_set_qt(struct jpg_device *jpg, uint32 qt);
int32 jpg_open_debug(struct jpg_device *p_jpg, uint8 enable);
int32 jpg_soft_kick(struct jpg_device *p_jpg, uint8 kick);
int32 jpg_decode_target(struct jpg_device *p_jpg, uint8 to_scaler);
int32 jpg_decode_photo(struct jpg_device *p_jpg, uint32 photo, uint32 len);
int32 jpg_request_irq(struct jpg_device *jpg, jpg_irq_hdl irq_hdl, uint32 irq_flags,  void *irq_data);
int32 jpg_release_irq(struct jpg_device *jpg, uint32 irq_flags);
int32 jpg_set_data_from(struct jpg_device *p_jpg, uint8 src_from);
int32 jpg_set_hw_check(struct jpg_device *p_jpg, uint8 enable);
int32 jpg_is_online(struct jpg_device *p_jpg);
int32 jpg_is_idle(struct jpg_device *p_jpg);
int32 jpg_set_ready(struct jpg_device *p_jpg);
int32 jpg_select_oe_using(struct jpg_device *p_jpg,uint8_t enable,uint8_t oe);
int32 jpg_get_oe_ready(struct jpg_device *p_jpg);
int32 jpg_set_vsync_dly(struct jpg_device *p_jpg,uint8_t enable);
int32 jpg_set_oe_state(struct jpg_device *p_jpg,uint8_t oe);
int32 jpg_dec_len_cfg_enable(struct jpg_device *p_jpg, uint8 enable);
int32 jpg_timeout_cfg_enable(struct jpg_device *p_jpg, uint8 enable);
int32 jpg_timeout_cfg(struct jpg_device *p_jpg, uint32 timeout);
int32 jpg_auto_rekick_cfg(struct jpg_device *p_jpg, uint8 enable);
int32 jpg_try_dec_cfg(struct jpg_device *p_jpg, uint8 enable);

#ifdef __cplusplus
}
#endif
#endif
