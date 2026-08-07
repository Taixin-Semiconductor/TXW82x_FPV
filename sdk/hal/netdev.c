#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "dev.h"
#include "osal/task.h"
#include "hal/netdev.h"

int32 netdev_open(struct netdev *ndev, netdev_input_cb input_cb, netdev_event_cb evt_cb, void *priv)
{
    if (ndev && ((const struct netdev_hal_ops *)ndev->dev.ops)->open) {
        HALDEV_SUSPENDED(ndev);
        return ((const struct netdev_hal_ops *)ndev->dev.ops)->open(ndev, input_cb, evt_cb, priv);
    }
    return -EINVAL;
}

int32 netdev_close(struct netdev *ndev)
{
    if (ndev && ((const struct netdev_hal_ops *)ndev->dev.ops)->close) {
        HALDEV_SUSPENDED(ndev);
        return ((const struct netdev_hal_ops *)ndev->dev.ops)->close(ndev);
    }
    return -EINVAL;
}

int32 netdev_send_data(struct netdev *ndev, uint8 *p_data, uint32 size)
{
    if (ndev && ((const struct netdev_hal_ops *)ndev->dev.ops)->send_data) {
        HALDEV_SUSPENDED(ndev);
        return ((const struct netdev_hal_ops *)ndev->dev.ops)->send_data(ndev, p_data, size);
    }
    return -ENOTSUPP;
}

int32 netdev_send_scatter_data(struct netdev *ndev, scatter_data *p_data, uint32 count)
{
    if (ndev && ((const struct netdev_hal_ops *)ndev->dev.ops)->send_scatter_data) {
        HALDEV_SUSPENDED(ndev);
        return ((const struct netdev_hal_ops *)ndev->dev.ops)->send_scatter_data(ndev, p_data, count);
    }
    return -ENOTSUPP;
}


int32 netdev_ioctl(struct netdev *ndev, uint32 cmd, uint32 param1, uint32 param2)
{
    if (ndev && ((const struct netdev_hal_ops *)ndev->dev.ops)->ioctl) {
        HALDEV_SUSPENDED(ndev);
        return ((const struct netdev_hal_ops *)ndev->dev.ops)->ioctl(ndev, cmd, param1, param2);
    }
    return -EINVAL;
}

int32 netdev_mdio_write(struct netdev *ndev, uint16 phy_addr, uint16 reg_addr, uint16 data)
{
    if (ndev && ((const struct netdev_hal_ops *)ndev->dev.ops)->mdio_write) {
        HALDEV_SUSPENDED(ndev);
        return ((const struct netdev_hal_ops *)ndev->dev.ops)->mdio_write(ndev, phy_addr, reg_addr, data);
    }
    return 0;
}

int32 netdev_mdio_read(struct netdev *ndev, uint16 phy_addr, uint16 reg_addr)
{
    if (ndev && ((const struct netdev_hal_ops *)ndev->dev.ops)->mdio_read) {
        HALDEV_SUSPENDED(ndev);
        return ((const struct netdev_hal_ops *)ndev->dev.ops)->mdio_read(ndev, phy_addr, reg_addr);
    }
    return 0;
}

