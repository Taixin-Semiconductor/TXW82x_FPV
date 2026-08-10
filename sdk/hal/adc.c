#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "hal/adc.h"

int32 adc_open(struct adc_device *adc)
{
    if (adc) {
        return ((const struct adc_hal_ops *)adc->dev.ops)->open(adc);
    }
    return RET_ERR;
}

int32 adc_close(struct adc_device *adc)
{
    if (adc) {
        return ((const struct adc_hal_ops *)adc->dev.ops)->close(adc);
    }
    return RET_ERR;
}

int32 adc_add_channel(struct adc_device *adc, uint32 channel)
{
    if (adc && ((const struct adc_hal_ops *)adc->dev.ops)->add_channel) {
        return ((const struct adc_hal_ops *)adc->dev.ops)->add_channel(adc, channel);
    }
    return RET_ERR;
}

int32 adc_delete_channel(struct adc_device *adc, uint32 channel)
{
    if (adc && ((const struct adc_hal_ops *)adc->dev.ops)->delete_channel) {
        return ((const struct adc_hal_ops *)adc->dev.ops)->delete_channel(adc, channel);
    }
    return RET_ERR;
}

int32 adc_get_value(struct adc_device *adc, uint32 channel, uint32 *raw_data)
{
    if (adc && ((const struct adc_hal_ops *)adc->dev.ops)->get_value) {
        return ((const struct adc_hal_ops *)adc->dev.ops)->get_value(adc, channel, raw_data);
    }
    return RET_ERR;
}

int32 adc_ioctl(struct adc_device *adc, enum adc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2)
{
    if (adc && ((const struct adc_hal_ops *)adc->dev.ops)->ioctl) {
        return ((const struct adc_hal_ops *)adc->dev.ops)->ioctl(adc, ioctl_cmd, param1, param2);
    }
    return RET_ERR;
}

int32 adc_request_irq(struct adc_device *adc, enum adc_irq_flag irq_flag, adc_irq_hdl irq_hdl, uint32 irq_data)
{
    if (adc && ((const struct adc_hal_ops *)adc->dev.ops)->request_irq) {
        return ((const struct adc_hal_ops *)adc->dev.ops)->request_irq(adc, irq_flag, irq_hdl, irq_data);
    }
    return RET_ERR;
}

int32 adc_release_irq(struct adc_device *adc, enum adc_irq_flag irq_flag)
{
    if (adc && ((const struct adc_hal_ops *)adc->dev.ops)->release_irq) {
        return ((const struct adc_hal_ops *)adc->dev.ops)->release_irq(adc, irq_flag);
    }
    return RET_ERR;
}

int32 adc_get_vref(struct adc_device *adc, float *vref)
{
    uint32 __vref = 0;

    if (!vref || !adc) {
        return RET_ERR;
    }

    adc_ioctl(adc, ADC_IOCTL_GET_VREF, (uint32)&__vref, 0);

    *vref = (float)(__vref>>8);

    return RET_OK;
}


