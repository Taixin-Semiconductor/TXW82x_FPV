#ifndef LOWPOWER_MODULE_REGISTRY_H
#define LOWPOWER_MODULE_REGISTRY_H

#include "typesdef.h"

#define LOWPOWER_MODULE_OK          RET_OK
#define LOWPOWER_MODULE_ERR         RET_ERR

typedef int (*lowPower_module_cb)(void *param1, void *param2, void *param3, void *param4);

struct lowPower_module_ops {
    const char *name;
    lowPower_module_cb suspend;
    lowPower_module_cb resume;
    void *priv[4];
};

int lowPower_app_register_module(const struct lowPower_module_ops *ops);
int lowPower_app_unregister_all_modules(void);
int lowPower_app_has_registered_modules(void);
int lowPower_app_suspend_modules(void);
int lowPower_app_resume_modules(void);

#endif