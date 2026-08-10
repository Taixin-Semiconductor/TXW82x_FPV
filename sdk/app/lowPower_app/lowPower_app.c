#include "basic_include.h"
#ifdef CONFIG_SLEEP
#include "lib/lmac/lmac_dsleep.h"
#include "lib/common/dsleepdata.h"
#endif
#include "lib/net/eloop/eloop.h"
#include "lowPower_app.h"


static void sleep_test(uint32_t time)
{
    uint16_t sleep_type     = SYSTEM_SLEEP_TYPE_SRAM_ONLY;

    struct system_sleep_param args;
    os_memset(&args, 0, sizeof(args));
    args.sleep_ms           = time;
    //args.wkup_io_sel[0]   = PA_13;    //LP Module PA_13
    args.wkup_io_edge       = 0x00;     //0: Rising; 1:Falling Edge
    args.wkup_io_en         = 0x00;     //IO0 En

    os_printf("%s:%d\tenter sleep\n", __FUNCTION__, __LINE__);

    system_sleep(sleep_type, &args);
}

static void enter_lowpower_timer(void *ei, void *d)
{
    uint32_t time = (uint32_t)d;
    os_printf("%s:%d\ttime:%d\n", __FUNCTION__, __LINE__, time);

    sleep_test(time);
}

static int32 app_sleepcb(uint16_t type, struct sys_sleepcb_param *args, void *priv)
{
    uint8_t action   = args->action;
    uint8_t step     = args->step;
    //uint8_t reason = args->resume.wkreason;
    switch(action)
    {
        case SYS_SLEEPCB_ACTION_SUSPEND:
            if(step == SYS_SLEEPCB_APP)
            {
                lowPower_app_suspend();
                lowPower_app_resume_preinit();
            }
            break;
        case SYS_SLEEPCB_ACTION_RESUME:
            if(step == SYS_SLEEPCB_APP)
            {
                //if(reason == DSLEEP_WK_REASON_WK_DATA)
                {
                    lowPower_app_resume_wakeup();
                }
                
            }
            break;
    }
    return RET_OK;
}

static int32 user_check_data(void *usr_wkdet_priv, uint8 *data, uint32 len)
{
    if(len >= 10)
    {
        if(data[9] == 0x06)
        {
            return 1;
        }
    }
    return 0;
}

//应用层低功耗初始化
int lowpower_app_init(const struct lowPower_module_ops *ops, int module_count)
{
    lowPower_app_unregister_all_modules();
    for (int i = 0; i < module_count; ++i) {
        if (lowPower_app_register_module(&ops[i]) != LOWPOWER_MODULE_OK) {
            os_printf("%s:%d register %s failed\n",
                      __FUNCTION__, __LINE__,
                      ops[i].name);
            lowPower_app_unregister_all_modules();
            return RET_ERR;
        }
    }

    int32 ret = sys_register_sleepcb(app_sleepcb, NULL);

    if (ret != RET_OK) {
        os_printf("%s:%d\tregister sleep callback failed:%d\n", __FUNCTION__, __LINE__, ret);
        lowPower_app_unregister_all_modules();
        return RET_ERR;
    }

    dsleep_set_usr_wkdet_cb(NULL, user_check_data);

    os_printf("%s:%d\t\n", __FUNCTION__, __LINE__);

    //进入休眠,休眠500ms起来
    //sleep_test(2000);

    //定时休眠
    // eloop_add_timer(5000, EVENT_F_ENABLED, enter_lowpower_timer, (void*)5000);

    return RET_OK;
}
