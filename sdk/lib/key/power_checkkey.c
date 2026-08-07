#include "sys_config.h"
#include "typesdef.h"
#include "adkey.h"
#include "keyScan.h"
 
#include "dev/adc/hgadc_v0.h"
#include "hal/gpio.h"


static void key_powerkey_init(key_channel_t *key)
{
    adkey_t *adkey = (adkey_t *)key->priv;
    struct hgadc_v0 *adc = (struct hgadc_v0*)dev_get(HG_ADC0_DEVID);

	adc_open((struct adc_device *)adc);	
	gpio_set_mode(adkey->pin,adkey->pull,adkey->pull_level);
	adc_add_channel((struct adc_device *)adc, adkey->pin);	
	
    adkey->priv = (void*)adc;
	key->enable = 1;

}

static uint8 key_powerkey_scan(key_channel_t *key)
{
    adkey_t *adkey = (adkey_t *)key->priv;
    uint32 vol;
    struct adkey_scan_code *key_scan = (struct adkey_scan_code*)key->key_table;
    adc_get_value((struct adc_device *)adkey->priv, adkey->pin, &vol);
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
    return key_scan->key;
}








/*********************************************************
 *                      powerkey的参数配置
 * 
 *  可以配置类似低电量的检测
************************************************************/
static const struct adkey_scan_code powerkey_table[] = 
{
	{0,     LOWPOWER_CHECK},
    {400,   KEY_NONE},
    {4096,  KEY_NONE},
};

//配置对应的时间,正常来说,电量检测不需要短按事件,只要设置长按以及repeat的事件就行了,repeat就是
//当前电量
static const keys_t powerkey_arg = 
{
    .period_long     = 500,
    .period_repeat   = 1000,
    .period_dither = 80,
};


static adkey_t powerkey= {
  .priv = NULL,
  .pin  = PA_3,
  .pull = GPIO_PULL_NONE,
  .pull_level = GPIO_PULL_LEVEL_100K,
};



//这里逻辑与ADKEY一样,因为都是ad检测
key_channel_t powercheck_key = 
{
  .init       = key_powerkey_init,
  .scan       = key_powerkey_scan,
  .prepare    = NULL,
  .priv       = (void*)&powerkey,
  .key_arg    = &powerkey_arg,//按键的参数,可能不同的类型按键,参数不一样
  .key_table  = &powerkey_table,
};



