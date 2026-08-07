#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/prc.h"




int32 prc_set_width(struct prc_device *p_prc, uint32 width)
{
    if (p_prc && ((const struct prc_hal_ops *)p_prc->dev.ops)->ioctl) {
        return ((const struct prc_hal_ops *)p_prc->dev.ops)->ioctl(p_prc,PRC_IOCTL_CMD_SET_WIDTH, width,0);
    }
    return RET_ERR;
}

int32 prc_set_yuv_mode(struct prc_device *p_prc, uint32 mode)
{
    if (p_prc && ((const struct prc_hal_ops *)p_prc->dev.ops)->ioctl) {
        return ((const struct prc_hal_ops *)p_prc->dev.ops)->ioctl(p_prc,PRC_IOCTL_CMD_SET_YUV_MODE, mode,0);
    }
    return RET_ERR;
}

int32 prc_set_yaddr(struct prc_device *p_prc, uint32 addr)
{
    if (p_prc && ((const struct prc_hal_ops *)p_prc->dev.ops)->ioctl) {
        return ((const struct prc_hal_ops *)p_prc->dev.ops)->ioctl(p_prc,PRC_IOCTL_CMD_SET_Y_ADDR, addr,0);
    }
    return RET_ERR;
}

int32 prc_set_uaddr(struct prc_device *p_prc, uint32 addr)
{
    if (p_prc && ((const struct prc_hal_ops *)p_prc->dev.ops)->ioctl) {
        return ((const struct prc_hal_ops *)p_prc->dev.ops)->ioctl(p_prc,PRC_IOCTL_CMD_SET_U_ADDR, addr,0);
    }
    return RET_ERR;
}

int32 prc_set_vaddr(struct prc_device *p_prc, uint32 addr)
{
    if (p_prc && ((const struct prc_hal_ops *)p_prc->dev.ops)->ioctl) {
        return ((const struct prc_hal_ops *)p_prc->dev.ops)->ioctl(p_prc,PRC_IOCTL_CMD_SET_V_ADDR, addr,0);
    }
    return RET_ERR;
}

int32 prc_set_src_addr(struct prc_device *p_prc, uint32 addr)
{
    if (p_prc && ((const struct prc_hal_ops *)p_prc->dev.ops)->ioctl) {
        return ((const struct prc_hal_ops *)p_prc->dev.ops)->ioctl(p_prc,PRC_IOCTL_CMD_SET_SRC_ADDR, addr,0);
    }
    return RET_ERR;
}


int32 prc_first_start(struct prc_device *p_prc)
{
	if (p_prc && ((const struct prc_hal_ops *)p_prc->dev.ops)->ioctl) {
		return ((const struct prc_hal_ops *)p_prc->dev.ops)->ioctl(p_prc,PRC_IOCTL_CMD_FIRST_KICK, 0,0);
	}
	return RET_ERR;
}


int32 prc_run(struct prc_device *p_prc)
{
    if (p_prc && ((const struct prc_hal_ops *)p_prc->dev.ops)->ioctl) {
        return ((const struct prc_hal_ops *)p_prc->dev.ops)->ioctl(p_prc,PRC_IOCTL_CMD_KICK, 0,0);
    }
    return RET_ERR;
}

int32 prc_init(struct prc_device *p_prc)
{
    if (p_prc && ((const struct prc_hal_ops *)p_prc->dev.ops)->init) {
        return ((const struct prc_hal_ops *)p_prc->dev.ops)->init(p_prc);
    }
    return RET_ERR;
}

int32 prc_deinit(struct prc_device *p_prc)
{
    if (p_prc && ((const struct prc_hal_ops *)p_prc->dev.ops)->deinit) {
        return ((const struct prc_hal_ops *)p_prc->dev.ops)->deinit(p_prc);
    }
    return RET_ERR;
}

int32 prc_request_irq(struct prc_device *p_prc, uint32 irq_flags, prc_irq_hdl irq_hdl, uint32 irq_data)
{
    if (p_prc && ((const struct prc_hal_ops *)p_prc->dev.ops)->request_irq) {
        return ((const struct prc_hal_ops *)p_prc->dev.ops)->request_irq(p_prc, irq_flags, irq_hdl, irq_data);
    }
    return RET_ERR;
}

int32 prc_release_irq(struct prc_device *p_prc, uint32 irq_flags)
{
    if (p_prc && ((const struct prc_hal_ops *)p_prc->dev.ops)->release_irq) {
        return ((const struct prc_hal_ops *)p_prc->dev.ops)->release_irq(p_prc, irq_flags);
    }
    return RET_ERR;
}


