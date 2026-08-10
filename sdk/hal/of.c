#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/of.h"


int32 of_init(struct of_device *p_of)
{
    if (p_of && ((const struct of_hal_ops *)p_of->dev.ops)->init) {
        return ((const struct of_hal_ops *)p_of->dev.ops)->init(p_of);
    }
    return RET_ERR;
}

int32 of_deinit(struct of_device *p_of)
{
    if (p_of && ((const struct of_hal_ops *)p_of->dev.ops)->deinit) {
        return ((const struct of_hal_ops *)p_of->dev.ops)->deinit(p_of);
    }
    return RET_ERR;
}

int32 of_set_instr_mode(struct of_device *p_of,enum of_instr_mode instr_format)
{
    if (p_of && ((const struct of_hal_ops *)p_of->dev.ops)->ioctl) {
        return ((const struct of_hal_ops *)p_of->dev.ops)->ioctl(p_of,OF_INSTR_MODE, instr_format,0);
    }
    return RET_ERR;
}

int32 of_set_pixel_num(struct of_device *p_of,uint32 pixel_num)
{
    if (p_of && ((const struct of_hal_ops *)p_of->dev.ops)->ioctl) {
        return ((const struct of_hal_ops *)p_of->dev.ops)->ioctl(p_of,CAL_PIXEL_NUM, pixel_num,0);
    }
    return RET_ERR;
}

int32 of_set_row_num(struct of_device *p_of,uint32 row_num)
{
    if (p_of && ((const struct of_hal_ops *)p_of->dev.ops)->ioctl) {
        return ((const struct of_hal_ops *)p_of->dev.ops)->ioctl(p_of,CAL_ROW_NUM, row_num,0);
    }
    return RET_ERR;
}

int32 of_set_row_size(struct of_device *p_of,uint32 row_size)
{
    if (p_of && ((const struct of_hal_ops *)p_of->dev.ops)->ioctl) {
        return ((const struct of_hal_ops *)p_of->dev.ops)->ioctl(p_of,SET_ROW_SIZE, row_size,0);
    }
    return RET_ERR;
}

int32 of_set_addr(struct of_device *p_of,uint32 addr0,uint32 addr1)
{
    if (p_of && ((const struct of_hal_ops *)p_of->dev.ops)->ioctl) {
        return ((const struct of_hal_ops *)p_of->dev.ops)->ioctl(p_of,SET_OF_ADDR, addr0,addr1);
    }
    return RET_ERR;
}

int32 of_get_acc(struct of_device *p_of)
{
    if (p_of && ((const struct of_hal_ops *)p_of->dev.ops)->ioctl) {
        return ((const struct of_hal_ops *)p_of->dev.ops)->ioctl(p_of,GET_CAL_ACC, 0,0);
    }
    return RET_ERR;
}

