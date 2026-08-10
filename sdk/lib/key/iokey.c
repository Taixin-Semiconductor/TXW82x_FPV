#include "basic_include.h"
#include "iokey.h"
#include "keyScan.h"
#include "hal/gpio.h"
#include "osal/string.h"
#ifdef PIN_FROM_PARAM
#include "pin_param.h"
#endif

#define KEY_VALUE AD_PRESS

static iokey_t iokey = {
        .priv       = NULL,
        .pull       = GPIO_PULL_NONE,
        .pull_level = GPIO_PULL_LEVEL_4_7K,
};

static void key_iokey_init(key_channel_t *key)
{
    iokey_t *iokey = (iokey_t *) key->priv;
    iokey->pin     = MACRO_PIN(PIN_IOKEY);
    if (iokey->pin != 0xff)
    {
        gpio_set_mode(iokey->pin, GPIO_PULL_NONE, iokey->pull_level);
        gpio_set_dir(iokey->pin, GPIO_DIR_INPUT);
        key->enable = 1;
    }
}

static uint8 key_iokey_scan(key_channel_t *key)
{
    iokey_t *iokey = (iokey_t *) key->priv;
    return gpio_get_val(iokey->pin) ? KEY_NONE : KEY_VALUE;
}

static const keys_t iokey_arg = {
        .period_long   = 500,
        .period_repeat = 1000,
        .period_dither = 80,
};

// 外部调用
key_channel_t iokey_key = {
        .init      = key_iokey_init,
        .scan      = key_iokey_scan,
        .prepare   = NULL,
        .priv      = (void *) &iokey,
        .key_arg   = &iokey_arg, // 按键的参数,可能不同的类型按键,参数不一样
        .key_table = NULL,
};
