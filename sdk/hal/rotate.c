#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/rotate.h"


int32 rotate_set_addr(struct rotate_device *p_rot, uint32_t src, uint32_t des)
{
    if (p_rot && ((const struct rotate_hal_ops *)p_rot->dev.ops)->ioctl) {
        return ((const struct rotate_hal_ops *)p_rot->dev.ops)->ioctl(p_rot, ROTATE_IOCTL_CMD_SET_ADDR, src, des);
    }
    return RET_ERR;
}

int32 rotate_set_size(struct rotate_device *p_rot, uint16_t w, uint16_t h)
{
    if (p_rot && ((const struct rotate_hal_ops *)p_rot->dev.ops)->ioctl) {
        return ((const struct rotate_hal_ops *)p_rot->dev.ops)->ioctl(p_rot, ROTATE_IOCTL_CMD_SET_SIZE, w, h);
    }
    return RET_ERR;
}

int32 rotate_run(struct rotate_device *p_rot, uint8_t angle, uint8_t bit_width)
{
    if (p_rot && ((const struct rotate_hal_ops *)p_rot->dev.ops)->ioctl) {
        return ((const struct rotate_hal_ops *)p_rot->dev.ops)->ioctl(p_rot, ROTATE_IOCTL_CMD_SET_ANALE_RUN, angle, bit_width);
    }
    return RET_ERR;
}

int32 rotate_get_sta(struct rotate_device *p_rot){
	if (p_rot && ((const struct rotate_hal_ops *)p_rot->dev.ops)->ioctl) {
		return ((const struct rotate_hal_ops *)p_rot->dev.ops)->ioctl(p_rot, ROTATE_IOCTL_CMD_GET_STA, 0,0);
	}
	return RET_ERR; 
}

