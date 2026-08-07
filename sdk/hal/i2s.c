#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "hal/i2s.h"

int32 i2s_open(struct i2s_device *i2s, enum i2s_mode mode, enum i2s_sample_freq frequency, enum i2s_sample_bits bits)
{
    if (i2s) {
        return ((const struct i2s_hal_ops *)i2s->dev.ops)->open(i2s, mode, frequency, bits);
    }
    return RET_ERR;
}

int32 i2s_close(struct i2s_device *i2s)
{
    if (i2s) {
        return ((const struct i2s_hal_ops *)i2s->dev.ops)->close(i2s);
    }
    return RET_ERR;
}

int32 i2s_write(struct i2s_device *i2s, const void *buf, uint32 len)
{
    if (i2s && ((const struct i2s_hal_ops *)i2s->dev.ops)->write) {
        return ((const struct i2s_hal_ops *)i2s->dev.ops)->write(i2s, buf, len);
    }
    return RET_ERR;
}

int32 i2s_write_scatter(struct i2s_device *i2s, const scatter_data *data, uint32 count)
{
    if (i2s && ((const struct i2s_hal_ops *)i2s->dev.ops)->write_scatter) {
        return ((const struct i2s_hal_ops *)i2s->dev.ops)->write_scatter(i2s, data, count);
    }
    return RET_ERR;
}

int32 i2s_read(struct i2s_device *i2s, void *buf, uint32 len)
{
    if (i2s && ((const struct i2s_hal_ops *)i2s->dev.ops)->read) {
        return ((const struct i2s_hal_ops *)i2s->dev.ops)->read(i2s, buf, len);
    }
    return RET_ERR;
}

int32 i2s_ioctl(struct i2s_device *i2s, uint32 cmd, uint32 param)
{
    if (i2s && ((const struct i2s_hal_ops *)i2s->dev.ops)->ioctl) {
        return ((const struct i2s_hal_ops *)i2s->dev.ops)->ioctl(i2s, cmd, param);
    }
    return RET_ERR;
}

int32 i2s_request_irq(struct i2s_device *i2s, uint32 irq_flag, i2s_irq_hdl irqhdl, uint32 irq_data)
{
    if (i2s && ((const struct i2s_hal_ops *)i2s->dev.ops)->request_irq) {
        return ((const struct i2s_hal_ops *)i2s->dev.ops)->request_irq(i2s, irq_flag, irqhdl, irq_data);
    }
    return RET_ERR;
}

int32 i2s_release_irq(struct i2s_device *i2s, uint32 irq_flag)
{
    if (i2s && ((const struct i2s_hal_ops *)i2s->dev.ops)->release_irq) {
        return ((const struct i2s_hal_ops *)i2s->dev.ops)->release_irq(i2s, irq_flag);
    }
    return RET_ERR;
}

