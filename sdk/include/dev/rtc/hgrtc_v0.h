#ifndef _HGRTC_V1_H
#define _HGRTC_V1_H
#include "hal/rtc.h"

#ifdef __cplusplus
extern "C" {
#endif




struct hgrtc_v0 {
    struct rtc_device    dev;
    uint32               hw;
    rtc_irq_hdl          irq_hdl;
    uint32               irq_data;
    uint32               irq_num;
    os_mutex_t      os_lock_set_time;
    os_mutex_t      os_lock_get_time;
    struct rtc_time_type time;
    struct {
        //1：闰年；0：平年
        uint8  leap_year;
    }time_ctrl;
    
    uint8                irq_second:1,
                         opened    :1;
};



int32 hgrtc_v0_attach(uint32 dev_id, struct hgrtc_v0 *rtc);


#ifdef __cplusplus
}
#endif
#endif
