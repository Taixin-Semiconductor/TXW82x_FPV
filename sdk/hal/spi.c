#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "hal/spi.h"

int32 spi_open(struct spi_device *spi, uint32 clk_freq, uint32 work_mode, uint32 wire_mode, uint32 clk_mode)
{
    if (spi && ((const struct spi_hal_ops *)spi->dev.ops)->open) {
        HALDEV_SUSPENDED(spi);
        return ((const struct spi_hal_ops *)spi->dev.ops)->open(spi, clk_freq, work_mode, wire_mode, clk_mode);
    }
    return RET_ERR;
}

int32 spi_close(struct spi_device *spi)
{
    if (spi && ((const struct spi_hal_ops *)spi->dev.ops)->close) {
        HALDEV_SUSPENDED(spi);
        return ((const struct spi_hal_ops *)spi->dev.ops)->close(spi);
    }
    return RET_ERR;
}

int32 spi_ioctl(struct spi_device *spi, uint32 cmd, uint32 param1, uint32 param2)
{
    if (spi && ((const struct spi_hal_ops *)spi->dev.ops)->ioctl) {
        HALDEV_SUSPENDED(spi);
        return ((const struct spi_hal_ops *)spi->dev.ops)->ioctl(spi, cmd, param1, param2);
    }
    return RET_ERR;
}

int32 spi_read(struct spi_device *spi, void *buf, uint32 size)
{
    if (spi && ((const struct spi_hal_ops *)spi->dev.ops)->read) {
        HALDEV_SUSPENDED(spi);
        return ((const struct spi_hal_ops *)spi->dev.ops)->read(spi, buf, size);
    }
    return RET_ERR;
}

int32 spi_write(struct spi_device *spi, const void *buf, uint32 size)
{
    if (spi && ((const struct spi_hal_ops *)spi->dev.ops)->write) {
        HALDEV_SUSPENDED(spi);
        return ((const struct spi_hal_ops *)spi->dev.ops)->write(spi, buf, size);
    }
    return RET_ERR;
}

int32 spi_write_scatter(struct spi_device *spi, const scatter_data *data, uint32 count)
{
    if (spi && ((const struct spi_hal_ops *)spi->dev.ops)->write_scatter) {
        HALDEV_SUSPENDED(spi);
        return ((const struct spi_hal_ops *)spi->dev.ops)->write_scatter(spi, data, count);
    }
    return RET_ERR;
}

inline int32 spi_set_cs(struct spi_device *spi, uint32 cs, uint32 value)
{
    if (spi && ((const struct spi_hal_ops *)spi->dev.ops)->ioctl) {
        HALDEV_SUSPENDED(spi);
        return ((const struct spi_hal_ops *)spi->dev.ops)->ioctl(spi, SPI_SET_CS, cs, value);
    }
    return RET_ERR;
}

int32 spi_request_irq(struct spi_device *spi, uint32 irq_flag, spi_irq_hdl irqhdl, uint32 irq_data)
{
    if (spi && ((const struct spi_hal_ops *)spi->dev.ops)->request_irq) {
        HALDEV_SUSPENDED(spi);
        return ((const struct spi_hal_ops *)spi->dev.ops)->request_irq(spi, irq_flag, irqhdl, irq_data);
    }
    return RET_ERR;
}

int32 spi_release_irq(struct spi_device *spi, uint32 irq_flag)
{
    if (spi && ((const struct spi_hal_ops *)spi->dev.ops)->release_irq) {
        HALDEV_SUSPENDED(spi);
        return ((const struct spi_hal_ops *)spi->dev.ops)->release_irq(spi, irq_flag);
    }
    return RET_ERR;
}


