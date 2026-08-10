#ifndef _PROJECT_SYSCFG_H_
#define _PROJECT_SYSCFG_H_

enum WIFI_WORK_MODE {
    WIFI_MODE_NONE = 0,
    WIFI_MODE_STA,
    WIFI_MODE_AP,
    WIFI_MODE_APSTA,
};

struct system_status {
    uint32 dbg_heap: 1,
           dbg_top: 2,
           dbg_lmac: 1,
           dbg_umac: 1,
           dbg_irq: 1,
           dbg_cache: 1,
           dbg_net: 1;
};

extern struct system_status sys_status;

#endif

