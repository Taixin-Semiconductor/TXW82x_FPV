#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "hal/tk.h"

int32 tk_open(struct tk_device *tk)
{
    if (tk)
    {
        return ((const struct tk_hal_ops *)tk->dev.ops)->open(tk);
    }
    
    return RET_ERR;
}

int32 tk_ioctl(struct tk_device *tk, enum touchkey_ioctl_cmd ioctl_cmd, uint32 param)
{
    if (tk && ((const struct tk_hal_ops *)tk->dev.ops)->ioctl)
    {
        return ((const struct tk_hal_ops *)tk->dev.ops)->ioctl(tk, ioctl_cmd, param);
    }

    return RET_ERR;
}

int32 tk_irq_request(struct tk_device *tk, tk_irq_hdl irq_hdl, uint32 irq_flag,  uint32 irq_data)
{
    if (tk && ((const struct tk_hal_ops *)tk->dev.ops)->request_irq)
    {
        return ((const struct tk_hal_ops *)tk->dev.ops)->request_irq(tk, irq_hdl, irq_flag, irq_data);
    }

    return RET_ERR;
}

int32 tk_irq_release(struct tk_device *tk, uint32 irq_flag)
{
    if (tk && ((const struct tk_hal_ops *)tk->dev.ops)->release_irq)
    {
        return ((const struct tk_hal_ops *)tk->dev.ops)->release_irq(tk, irq_flag);
    }
    
    return RET_ERR;
}

int32 tk_close(struct tk_device *tk)
{
    if (tk)
    {
        return ((const struct tk_hal_ops *)tk->dev.ops)->close(tk);
    }
    
    return RET_ERR;
}
