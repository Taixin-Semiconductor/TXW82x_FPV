#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "hal/rtc.h"


int32 rtc_open(struct rtc_device *rtc, struct rtc_time_type *rtc_time) {

    if(rtc && ((const struct rtc_hal_ops *)rtc->dev.ops)->open) {
        return ((const struct rtc_hal_ops *)rtc->dev.ops)->open(rtc, rtc_time);
    }

    return RET_ERR;
}

int32 rtc_close(struct rtc_device *rtc) {

    if(rtc && ((const struct rtc_hal_ops *)rtc->dev.ops)->close) {
        return ((const struct rtc_hal_ops *)rtc->dev.ops)->close(rtc);
    }

    return RET_ERR;
}

int32 rtc_set_time(struct rtc_device *rtc, struct rtc_time_type *rtc_time) {

    if(rtc && ((const struct rtc_hal_ops *)rtc->dev.ops)->set_time) {
        return ((const struct rtc_hal_ops *)rtc->dev.ops)->set_time(rtc, rtc_time);
    }

    return RET_ERR;
}

int32 rtc_get_time(struct rtc_device *rtc, struct rtc_time_type *rtc_time) {

    if(rtc && ((const struct rtc_hal_ops *)rtc->dev.ops)->get_time) {
        return ((const struct rtc_hal_ops *)rtc->dev.ops)->get_time(rtc, rtc_time);
    }

    return RET_ERR;
}

int32 rtc_ioctl(struct rtc_device *rtc, uint32 cmd, uint32 param1, uint32 param2) {

    if(rtc && ((const struct rtc_hal_ops *)rtc->dev.ops)->ioctl) {
        return ((const struct rtc_hal_ops *)rtc->dev.ops)->ioctl(rtc, cmd, param1, param2);
    }

    return RET_ERR;
}

int32 rtc_request_irq(struct rtc_device *rtc, uint32 irq_flag, rtc_irq_hdl irq_hdl, uint32 irq_data) {

    if(rtc && ((const struct rtc_hal_ops *)rtc->dev.ops)->request_irq) {
        return ((const struct rtc_hal_ops *)rtc->dev.ops)->request_irq(rtc, irq_flag, irq_hdl, irq_data);
    }

    return RET_ERR;
}

int32 rtc_release_irq(struct rtc_device *rtc, uint32 irq_flag) {

    if(rtc && ((const struct rtc_hal_ops *)rtc->dev.ops)->release_irq) {
        return ((const struct rtc_hal_ops *)rtc->dev.ops)->release_irq(rtc, irq_flag);
    }

    return RET_ERR;
}


