/**
  ******************************************************************************
  * @file    User/xxx.c
  * @author  HUGE-IC Application Team
  * @version V1.0.0
  * @date    01-08-2023
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2018 HUGE-IC</center></h2>
  *
  *
  *
  ******************************************************************************
  */
#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "dev.h"
#include "hal/sha.h"

int32 sha_ioctl(struct sha_dev *dev, uint32 cmd, uint32 para)
{
    if (dev && ((const struct sha_hal_ops *)dev->dev.ops)->ioctl) {
        return ((const struct sha_hal_ops *)dev->dev.ops)->ioctl(dev, cmd, para, 0);
    }
    return RET_ERR;
}

int32 sha_init(struct sha_dev *dev, SHA_TYPE type)
{
    if (dev && ((const struct sha_hal_ops *)dev->dev.ops)->init) {
        return ((const struct sha_hal_ops *)dev->dev.ops)->init(dev, type);
    }
    return RET_ERR;
}

int32 sha_update(struct sha_dev *dev, uint8 *input, uint32 len)
{
    if (dev && ((const struct sha_hal_ops *)dev->dev.ops)->update) {
        return ((const struct sha_hal_ops *)dev->dev.ops)->update(dev, input, len);
    }
    return RET_ERR;
}

int32 sha_final(struct sha_dev *dev, uint8 *output)
{
    uint32 ret = 0;
    if (dev && ((const struct sha_hal_ops *)dev->dev.ops)->final) {
        ret = ((const struct sha_hal_ops *)dev->dev.ops)->final(dev, output);
    }
    return ret;
}

int32 sha_finup(struct sha_dev *dev, uint8 *input, uint32 len, uint8 *output)
{
    uint32 ret = 0;
    if (dev && ((const struct sha_hal_ops *)dev->dev.ops)->update) {
        ret = ((const struct sha_hal_ops *)dev->dev.ops)->update(dev, input, len);
    }
    if (dev && ((const struct sha_hal_ops *)dev->dev.ops)->final) {
        ret = ((const struct sha_hal_ops *)dev->dev.ops)->final(dev, output);
    }
    return ret;
}

int32 sha_xform(struct sha_dev *dev, struct sha_req *req)
{
    uint32 ret = 0;
    if (dev && ((const struct sha_hal_ops *)dev->dev.ops)->xform) {
        ret = ((const struct sha_hal_ops *)dev->dev.ops)->xform(dev, req);
    }
    return ret;
}

int32 sha_requset_irq(struct sha_dev *dev, void *irq_handle, void *args)
{
    if (dev && ((const struct sha_hal_ops *)dev->dev.ops)->requset_irq) {
        return ((const struct sha_hal_ops *)dev->dev.ops)->requset_irq(dev, irq_handle, args);
    }
    return RET_ERR;
}

int32 sha_release_irq(struct sha_dev *dev)
{
    if (dev && ((const struct sha_hal_ops *)dev->dev.ops)->release_irq) {
        return ((const struct sha_hal_ops *)dev->dev.ops)->release_irq(dev);
    }
    return RET_ERR;
}

