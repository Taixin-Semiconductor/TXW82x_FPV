#include "lib/touch/touch_pad.h"

static const struct touch_chip_hardware_ops* tp_chip_ops_set[touch_chip_count] =
{
    &cst226se_ops,
    &chsc6x_ops,
};

touch_multipoint_pos_t *touch_pad_get_multipoint_xy(void *dev)
{

    struct touch_pad_dev *p_dev = (struct touch_pad_dev *)dev;

    if (p_dev == NULL || p_dev->hw_ops == NULL ||
        p_dev->hw_ops->get_multipoint_xy == NULL)
    {
        return NULL;
    }

    return p_dev->hw_ops->get_multipoint_xy(p_dev->user_data);
}

uint32_t touch_pad_free_multipoint_xy(void *dev, touch_multipoint_pos_t *data)
{

    struct touch_pad_dev *p_dev = (struct touch_pad_dev *)dev;

    if (p_dev == NULL || p_dev->hw_ops == NULL ||
        p_dev->hw_ops->free_multipoint_xy == NULL)
    {
        return RET_ERR;
    }

    return p_dev->hw_ops->free_multipoint_xy(p_dev->user_data, (void *)data);
}

int32_t touch_pad_hardware_init(uint16_t dev_id, touch_chip_set touch_chip_select)
{

    struct touch_pad_dev *dev;
    struct dev_obj *registered_dev;
    const struct touch_chip_hardware_ops *hw_ops;
    int32_t ret;

    if (dev_id == 0 || touch_chip_select < cst226se_touch_chip ||
        touch_chip_select >= touch_chip_count)
    {
        return -EINVAL;
    }

    registered_dev = dev_get(dev_id);
    if (registered_dev != NULL)
    {
        return -EEXIST;
    }

    hw_ops = tp_chip_ops_set[touch_chip_select];
    if (hw_ops == NULL || hw_ops->touch_chip_init == NULL ||
        hw_ops->touch_chip_deinit == NULL ||
        hw_ops->get_multipoint_xy == NULL ||
        hw_ops->free_multipoint_xy == NULL)
    {
        return -EINVAL;
    }

    dev = (struct touch_pad_dev *)os_zalloc(sizeof(struct touch_pad_dev));
    if (dev == NULL)
    {
        return -ENOMEM;
    }

    dev->hw_ops = hw_ops;
    ret = dev->hw_ops->touch_chip_init(&dev->user_data);
    if (ret != RET_OK)
    {
        os_free(dev);
        return ret;
    }

    ret = dev_register(dev_id, &dev->dev);
    if (ret != RET_OK)
    {
        dev->hw_ops->touch_chip_deinit(dev->user_data);
        os_free(dev);
        return ret;
    }

    return RET_OK;
}

int32_t touch_pad_hardware_deinit(void *dev)
{
    struct touch_pad_dev *p_dev = (struct touch_pad_dev *)dev;
    int32_t ret = RET_OK;
    int32_t unregister_ret;

    if (p_dev == NULL || p_dev->hw_ops == NULL ||
        p_dev->hw_ops->touch_chip_deinit == NULL)
    {
        return -EINVAL;
    }

    ret = p_dev->hw_ops->touch_chip_deinit(p_dev->user_data);
    if (ret != RET_OK)
    {
        return ret;
    }

    unregister_ret = dev_unregister(&p_dev->dev);
    os_free(p_dev);

    return unregister_ret;
}