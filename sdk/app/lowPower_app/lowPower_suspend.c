#include "basic_include.h"
#include "lowPower_app.h"

static uint8_t app_suspend_flag = 0;

int lowPower_app_suspend(void)
{
    if (app_suspend_flag) {
        return RET_OK;
    }

    app_suspend_flag = 1;

    os_printf("%s:%d\n",__FUNCTION__,__LINE__);

    if (lowPower_app_has_registered_modules()) {
        lowPower_app_suspend_modules();
    }

    return RET_OK;

}

void lowPower_app_suspend_exit(void)
{
    app_suspend_flag = 0;
}


