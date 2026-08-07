#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/csc.h"

int32 csc_init(struct csc_device *p_csc)
{
    if (p_csc && ((const struct csc_hal_ops *)p_csc->dev.ops)->init) {
        return ((const struct csc_hal_ops *)p_csc->dev.ops)->init(p_csc);
    }
    return RET_ERR;
}

int32 csc_request_irq(struct csc_device *p_csc, uint32 irq_flags, csc_irq_hdl irq_hdl, uint32 irq_data)
{
    if (p_csc && ((const struct csc_hal_ops *)p_csc->dev.ops)->request_irq) {
        return ((const struct csc_hal_ops *)p_csc->dev.ops)->request_irq(p_csc, irq_flags, irq_hdl, irq_data);
    }
    return RET_ERR;
}

int32 csc_release_irq(struct csc_device *p_csc, uint32 irq_flags)
{
    if (p_csc && ((const struct csc_hal_ops *)p_csc->dev.ops)->release_irq) {
        return ((const struct csc_hal_ops *)p_csc->dev.ops)->release_irq(p_csc, irq_flags);
    }
    return RET_ERR;
}

int32 csc_set_format(struct csc_device *p_csc, uint8_t input_format,uint8_t output_format)
{
    if (p_csc && ((const struct csc_hal_ops *)p_csc->dev.ops)->ioctl) {
        return ((const struct csc_hal_ops *)p_csc->dev.ops)->ioctl(p_csc, CSC_IOCTL_FORMAT_TRAN, input_format, output_format);
    }
    return RET_ERR;
}

int32 csc_set_photo_size(struct csc_device *p_csc, uint32_t w,uint32_t h)
{
    if (p_csc && ((const struct csc_hal_ops *)p_csc->dev.ops)->ioctl) {
        return ((const struct csc_hal_ops *)p_csc->dev.ops)->ioctl(p_csc, CSC_IOCTL_PHOTO_SIZE, w, h);
    }
    return RET_ERR;
}

int32 csc_set_type(struct csc_device *p_csc, uint8_t type)
{
    if (p_csc && ((const struct csc_hal_ops *)p_csc->dev.ops)->ioctl) {
        return ((const struct csc_hal_ops *)p_csc->dev.ops)->ioctl(p_csc, CSC_IOCTL_COEF_TYPE, type, 0);
    }
    return RET_ERR;
}

int32 csc_set_input_addr(struct csc_device *p_csc, uint32_t in0,uint32_t in1,uint32_t in2)
{
	uint32 input_tbl[3];
	input_tbl[0] = in0;
	input_tbl[1] = in1;
	input_tbl[2] = in2;
    if (p_csc && ((const struct csc_hal_ops *)p_csc->dev.ops)->ioctl) {
        return ((const struct csc_hal_ops *)p_csc->dev.ops)->ioctl(p_csc, CSC_IOCTL_INPUT_ADDR, (uint32_t)input_tbl, 0);
    }
    return RET_ERR;
}

int32 csc_set_output_addr(struct csc_device *p_csc, uint32_t out0,uint32_t out1,uint32_t out2)
{
	uint32 output_tbl[3];
	output_tbl[0] = out0;
	output_tbl[1] = out1;
	output_tbl[2] = out2;
    if (p_csc && ((const struct csc_hal_ops *)p_csc->dev.ops)->ioctl) {
        return ((const struct csc_hal_ops *)p_csc->dev.ops)->ioctl(p_csc, CSC_IOCTL_OUTPUT_ADDR, (uint32_t)output_tbl, 0);
    }
    return RET_ERR;
}

int32 csc_start_run(struct csc_device *p_csc)
{
    if (p_csc && ((const struct csc_hal_ops *)p_csc->dev.ops)->ioctl) {
        return ((const struct csc_hal_ops *)p_csc->dev.ops)->ioctl(p_csc, CSC_IOCTL_KICK_RUN, 0, 0);
    }
    return RET_ERR;
}

