#include "sys_config.h"
#include "typesdef.h"
#include "adkey.h"
#include "keyScan.h"
 
#include "dev/adc/hgadc_v0.h"
#include "hal/gpio.h"
#ifdef PIN_FROM_PARAM
#include "pin_param.h"
#endif

static adkey_t adkey= {
  .priv = NULL,
  .pull = GPIO_PULL_NONE,
  .pull_level = GPIO_PULL_LEVEL_4_7K,
};

static void key_adkey_init(key_channel_t *key)
{
    adkey_t *adkey = (adkey_t *)key->priv;
    struct hgadc_v0 *adc = (struct hgadc_v0*)dev_get(HG_ADC0_DEVID);
	adkey->pin = MACRO_PIN(PIN_ADKEY1);
	adc_open((struct adc_device *)adc);	
	gpio_set_mode(adkey->pin,adkey->pull,adkey->pull_level);
	adc_add_channel((struct adc_device *)adc, adkey->pin);	
	
    adkey->priv = (void*)adc;
	key->enable = 1;

}

static uint8 key_adkey_scan(key_channel_t *key)
{
    adkey_t *adkey = (adkey_t *)key->priv;
    uint32 vol;
    struct adkey_scan_code *key_scan = (struct adkey_scan_code*)key->key_table;
    adc_get_value((struct adc_device *)adkey->priv, adkey->pin, &vol);
	// printf("vol:%d\n",vol);
    //记录当前adc的值,用与发送到应用层,至于应用层是否需要,由应用层去管理
    key->extern_value = vol;
    for(;;)
    {
        if(vol>=key_scan->adc)
        {
            key_scan++;
        }
        else
        {
            key_scan--;
            break;
        }
    }
    // printf("key_scan->key:%d\tvol:%d\n",key_scan->key,vol);
    return key_scan->key;
}








/*********************************************************
 *                      adkey的参数配置
 * 
    default: 2117
    up:910
    down:462
    left:6
    right:1368
    press:1826

默认开发板先检查每一个按键的值,然后大概每一个ad-100填到下表
************************************************************/
static const struct adkey_scan_code adkey_table[] = 
{
	{0,     AD_LEFT},
	{350,   AD_DOWN},
	{800,   AD_UP},
	{1200,  AD_RIGHT},
    {1700,  AD_PRESS},
	{2000,  KEY_NONE},
	{3200,  KEY_NONE},
    {3800,  KEY_NONE},
    {4000,  KEY_NONE},
    {4096,  KEY_NONE},
};

static const keys_t adkey_arg = 
{
    .period_long     = 500,
    .period_repeat   = 1000,
    .period_dither = 80,
};

//外部调用
key_channel_t adkey_key = 
{
  .init       = key_adkey_init,
  .scan       = key_adkey_scan,
  .prepare    = NULL,
  .priv       = (void*)&adkey,
  .key_arg    = &adkey_arg,//按键的参数,可能不同的类型按键,参数不一样
  .key_table  = &adkey_table,
};



