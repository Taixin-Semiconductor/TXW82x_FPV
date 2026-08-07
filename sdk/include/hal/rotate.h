#ifndef HAL_ROTATE_H
#define HAL_ROTATE_H

#ifdef __cplusplus
extern "C" {
#endif

enum rotate_ioctl_cmd {
	ROTATE_IOCTL_CMD_SET_ANALE_RUN,

	ROTATE_IOCTL_CMD_SET_SIZE,

	ROTATE_IOCTL_CMD_SET_ADDR,	

	ROTATE_IOCTL_CMD_GET_STA,		
};

enum rotate_angle {
    ROT_90,
	ROT_180,
	ROT_270,
	ROT_90_SQR,
};

enum rotate_bw {
    ROT_16B,
	ROT_32B,
};

struct rotate_device {
    struct dev_obj dev;
};

struct rotate_hal_ops {
	struct devobj_ops ops;
    int32(*ioctl)(struct rotate_device *rotate_dev, enum rotate_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
};



int32 rotate_set_addr(struct rotate_device *p_rot, uint32_t src, uint32_t des);
int32 rotate_set_size(struct rotate_device *p_rot, uint16_t w, uint16_t h);
int32 rotate_run(struct rotate_device *p_rot, uint8_t angle, uint8_t bit_width);
int32 rotate_get_sta(struct rotate_device *p_rot);


#ifdef __cplusplus
}
#endif
#endif
