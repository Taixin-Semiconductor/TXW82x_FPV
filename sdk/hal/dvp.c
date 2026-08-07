#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/dvp.h"

int32 dvp_open(struct dvp_device *p_dvp)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->open) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->open(p_dvp);
    }
    return RET_ERR;
}

int32 dvp_close(struct dvp_device *p_dvp)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->close) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->close(p_dvp);
    }
    return RET_ERR;
}


int32 dvp_init(struct dvp_device *p_dvp)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->init) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->init(p_dvp);
    }
    return RET_ERR;
}

int32 dvp_deinit(struct dvp_device *p_dvp)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->deinit) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->deinit(p_dvp);
    }
    return RET_ERR;
}

int32 dvp_set_baudrate(struct dvp_device *p_dvp, uint32 mclk)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->baudrate) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->baudrate(p_dvp, mclk);
    }
    return RET_ERR;

}

int32 dvp_set_size(struct dvp_device *p_dvp, uint32 x_s, uint32 y_s, uint32 x_e, uint32 y_e)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl) {
        uint32 size_array[4] = {x_s, y_s, x_e, y_e};
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl(p_dvp, DVP_IOCTL_CMD_SET_SIZE, (uint32)size_array, 0);
    }
    return RET_ERR;
}

int32 dvp_set_addr1(struct dvp_device *p_dvp, uint32 yuv_addr)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl(p_dvp, DVP_IOCTL_CMD_SET_ADR_1, yuv_addr, 0);
    }
    return RET_ERR;
}

int32 dvp_set_addr2(struct dvp_device *p_dvp, uint32 yuv_addr)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl(p_dvp, DVP_IOCTL_CMD_SET_ADR_2, yuv_addr, 0);
    }
    return RET_ERR;
}


int32 dvp_set_rgb2yuv(struct dvp_device *p_dvp, uint8 en)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl(p_dvp, DVP_IOCTL_CMD_RGB_2_YUV, en, 0);
    }
    return RET_ERR;

}

int32 dvp_set_exchange_d5_d6(struct dvp_device *p_dvp, uint8 en)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl(p_dvp, DVP_IOCTL_CMD_EX_D5_D6, en, 0);
    }
    return RET_ERR;

}

int32 dvp_set_vsync_rst_offline(struct dvp_device *p_dvp,uint8 enable,uint32 timeout)
{
	if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl) {
		return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl(p_dvp, DVP_IOCTL_CMD_VSYNC_RST_TM, timeout, enable);
	}
	return RET_ERR;

}


int32 dvp_set_format(struct dvp_device *p_dvp, uint8 format)      ///*
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl(p_dvp, DVP_IOCTL_CMD_SET_FORMAT, format, 0);
    }
    return RET_ERR;
}

int32 dvp_set_half_size(struct dvp_device *p_dvp, uint8 en)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl(p_dvp, DVP_IOCTL_CMD_SET_SCEN, en, 0);
    }
    return RET_ERR;
}

int32 dvp_set_vsync_polarity(struct dvp_device *p_dvp, uint8 high_valid)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl(p_dvp, DVP_IOCTL_CMD_SET_VSYNC_VAILD, high_valid, 0);
    }
    return RET_ERR;
}

int32 dvp_set_hsync_polarity(struct dvp_device *p_dvp, uint8 high_valid)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl(p_dvp, DVP_IOCTL_CMD_SET_HSYNC_VAILD, high_valid, 0);
    }
    return RET_ERR;
}

int32 dvp_set_once_sampling(struct dvp_device *p_dvp, uint8 en)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl(p_dvp, DVP_IOCTL_CMD_SET_ONE_SAMPLE, en, 0);
    }
    return RET_ERR;
}

int32 dvp_debounce_enable(struct dvp_device *p_dvp, uint8 en, uint8 pixel)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl(p_dvp, DVP_IOCTL_CMD_SET_DEBOUNCE, en, pixel);
    }
    return RET_ERR;
}

int32 dvp_set_ycbcr(struct dvp_device *p_dvp, uint8 mode)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl(p_dvp, DVP_IOCTL_CMD_SET_YCBCR_MODE, mode, 0);
    }
    return RET_ERR;
}

int32 dvp_unload_uv(struct dvp_device *p_dvp, uint8 en)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl(p_dvp, DVP_IOCTL_CMD_DIS_UV_MODE, en, 0);
    }
    return RET_ERR;
}

int32 dvp_frame_load_precent(struct dvp_device *p_dvp, uint8 precent)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl(p_dvp, DVP_IOCTL_CMD_SET_FRAME_RATE, precent, 0);
    }
    return RET_ERR;
}

int32 dvp_get_status(struct dvp_device *p_dvp)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl(p_dvp, DVP_IOCTL_CMD_GET_STA, 0, 0);
    }
    return RET_ERR;
}

int32 dvp_low_high_threshold(struct dvp_device *p_dvp, bool low_high)
{
    uint32 dlt, dht;
    if (low_high == 0) {
        dlt = 0x00000030;
        dht = 0x00ffffA0;
    } else {
        dht = 0x00ffffff;
        dlt = 0;
    }
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl(p_dvp, DVP_IOCTL_CMD_SET_THRESHOLD, dlt, dht);
    }
    return RET_ERR;
}

int32 dvp_jpeg_mode_set_len(struct dvp_device *p_dvp, uint32 len)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl(p_dvp, DVP_IOCTL_CMD_SET_JPEG_LEN, len, 0);
    }
    return RET_ERR;

}


int32 dvp_10BITS_map(struct dvp_device *p_dvp, uint8 mode)
{
	if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl) {
		return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl(p_dvp, DVP_IOCTL_CMD_10BITS_MAP, mode, 0);
	}
	return RET_ERR;
}


int32 dvp_8BITS_map(struct dvp_device *p_dvp, uint8 mode)
{
	if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl) {
		return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->ioctl(p_dvp, DVP_IOCTL_CMD_8BITS_MAP, mode, 0);
	}
	return RET_ERR;
}

int32 dvp_request_irq(struct dvp_device *p_dvp, uint32 irq_flags, dvp_irq_hdl irq_hdl, uint32 irq_data)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->request_irq) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->request_irq(p_dvp, irq_flags, irq_hdl, irq_data);
    }
    return RET_ERR;
}

int32 dvp_release_irq(struct dvp_device *p_dvp, uint32 irq_flags)
{
    if (p_dvp && ((const struct dvp_hal_ops *)p_dvp->dev.ops)->release_irq) {
        return ((const struct dvp_hal_ops *)p_dvp->dev.ops)->release_irq(p_dvp, irq_flags);
    }
    return RET_ERR;
}

