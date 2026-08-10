#ifndef __LOWPOWER_APP_H__
#define __LOWPOWER_APP_H__

#include "typesdef.h"
#include "lowPower_module_registry.h"


int lowpower_app_init(const struct lowPower_module_ops *ops, int module_count);
int lowPower_app_resume_preinit(void);
void lowPower_app_resume_wakeup(void);
int lowPower_app_suspend(void);
void lowPower_app_suspend_exit(void);


#endif
