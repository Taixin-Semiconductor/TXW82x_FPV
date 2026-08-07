#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "hal/sdio_slave.h"

int32 sdio_slave_open(struct sdio_slave *slave, enum sdio_slave_speed speed, uint32 flags)
{
    if (slave) {
        HALDEV_SUSPENDED(slave);
        return ((const struct sdios_hal_ops *)slave->dev.ops)->open(slave, speed, flags);
    }
    return RET_ERR;
}

int32 sdio_slave_close(struct sdio_slave *slave)
{
    if (slave) {
        HALDEV_SUSPENDED(slave);
        return ((const struct sdios_hal_ops *)slave->dev.ops)->close(slave);
    }
    return RET_ERR;
}

int32 sdio_slave_write(struct sdio_slave *slave, uint8 *buff, uint32 len, int8 sync)
{
    if (slave && ((const struct sdios_hal_ops *)slave->dev.ops)->write) {
        HALDEV_SUSPENDED(slave);
        return ((const struct sdios_hal_ops *)slave->dev.ops)->write(slave, buff, len, sync);
    }
    return RET_ERR;
}

int32 sdio_slave_write_scatter(struct sdio_slave *slave, scatter_data *data, uint32 count, int8 sync)
{
    if (slave && ((const struct sdios_hal_ops *)slave->dev.ops)->write_scatter) {
        HALDEV_SUSPENDED(slave);
        return ((const struct sdios_hal_ops *)slave->dev.ops)->write_scatter(slave, data, count, sync);
    }
    return RET_ERR;
}

int32 sdio_slave_read(struct sdio_slave *slave, uint8 *buff, uint32 len, int8 sync)
{
    if (slave && ((const struct sdios_hal_ops *)slave->dev.ops)->read) {
        HALDEV_SUSPENDED(slave);
        return ((const struct sdios_hal_ops *)slave->dev.ops)->read(slave, buff, len, sync);
    }
    return RET_ERR;
}

int32 sdio_slave_ioctl(struct sdio_slave *slave, uint32 cmd, uint32 param)
{
    if (slave && ((const struct sdios_hal_ops *)slave->dev.ops)->ioctl) {
        HALDEV_SUSPENDED(slave);
        return ((const struct sdios_hal_ops *)slave->dev.ops)->ioctl(slave, cmd, param);
    }
    return RET_ERR;
}

int32 sdio_slave_request_irq(struct sdio_slave *slave, sdio_slave_irq_hdl handle, uint32 data)
{
    if (slave && ((const struct sdios_hal_ops *)slave->dev.ops)->request_irq) {
        HALDEV_SUSPENDED(slave);
        return ((const struct sdios_hal_ops *)slave->dev.ops)->request_irq(slave, handle, data);
    }
    return RET_ERR;
}

