#include "basic_include.h"
#include "dev/adc/hgadc_v0.h"
#ifdef PIN_FROM_PARAM
#include "pin_param.h"
#endif
#include "lib/lmac/lmac.h"

struct bat_det_priv {
    struct os_work work;
    struct hgadc_v0 *adc_dev;
    uint32_t bat_det_io;
    double vol;
    uint32_t _update_fme_adc_val;
    uint8_t level;
    uint32_t adc_raw;
};

struct bat_det_priv *g_bat_det_priv_data = NULL;

static int32 bat_detect_work(struct os_work *work)
{
	#if WIFI_FEM_CHIP
    void * ops = NULL;
	#endif	

    struct bat_det_priv *bat_det_priv_data = (struct bat_det_priv *)work;

    uint32_t adc_value = 0;

    adc_get_value((struct adc_device *)bat_det_priv_data->adc_dev, bat_det_priv_data->bat_det_io, &adc_value);

    bat_det_priv_data->adc_raw = adc_value;

    bat_det_priv_data->vol = (double)adc_value * 3 / 2048 * 2;

    bat_det_priv_data->_update_fme_adc_val = adc_value * 3 * 2 / 5;
    if (bat_det_priv_data->_update_fme_adc_val > 2047) {
        bat_det_priv_data->_update_fme_adc_val = 2047;
    }

	os_printf("---- battery adc:%d vol:%.2fV _update_fme_adc_val:%.2fV adc_val:%d----\n", adc_value, (double)adc_value * 3 / 2048 * 2, (double)bat_det_priv_data->_update_fme_adc_val * 5 / 2048, bat_det_priv_data->_update_fme_adc_val);

    // bat_det_priv_data->vol = 4;
    #if WIFI_FEM_CHIP
    lmac_update_fem_voltage(ops, (uint32_t)(bat_det_priv_data->vol * (1 << 10)));
    #endif

    os_run_work_delay(&bat_det_priv_data->work, 5000);

    return 0;
}

int bat_get_level()
{
    /* 如果未初始化，返回0 */
    if (!g_bat_det_priv_data) {
        return 0;
    }

    /* 使用 ADC 阈值表
       阈值单位为 ADC 读数，避免浮点运算。加入整型 hysteresis 防跳变。 */
    static uint8_t first_count = 1;
    static const uint16_t thresholds_adc[] = {1125, 1194, 1262, 1364, 1432};
    const int max_level = sizeof(thresholds_adc) / sizeof(thresholds_adc[0]);
    const uint16_t hysteresis_adc = 10; /* ADC 单位去抖 */

    uint32_t adc = g_bat_det_priv_data->adc_raw;

    /* 计算候选等级 */
    int cand = 0;
    while (cand < max_level && adc > thresholds_adc[cand]) {
        cand++;
    }
    if (cand > (max_level - 1)) {
        cand = max_level - 1;
    }

    if (first_count) {
        first_count = 0;
        g_bat_det_priv_data->level = (uint8_t)cand;
        return cand;
    }

    int prev = (int)g_bat_det_priv_data->level;

    if (cand == prev) {
        return (uint8_t)prev;
    }

    if (cand > prev) {
        /* 上升：需要超过阈值 + hysteresis 才更新 */
        if (adc >= (uint32_t)thresholds_adc[cand] + hysteresis_adc) {
            g_bat_det_priv_data->level = (uint8_t)cand;
            return (uint8_t)cand;
        } else {
            return (uint8_t)prev;
        }
    } else { /* cand < prev */
        /* 下降：需要低于阈值 - hysteresis 才更新 */
        if (adc <= (uint32_t)thresholds_adc[cand] - hysteresis_adc) {
            g_bat_det_priv_data->level = (uint8_t)cand;
            return (uint8_t)cand;
        } else {
            return (uint8_t)prev;
        }
    }
}

void bat_ad_init()
{
    struct bat_det_priv *bat_det_priv_data = (struct bat_det_priv*)os_zalloc(sizeof(struct bat_det_priv));

    if (!bat_det_priv_data) {
        os_printf("bat ad init err!!!!!!!!!!!!!\n");
        return ;
    }

    g_bat_det_priv_data = bat_det_priv_data;

    bat_det_priv_data->adc_dev = (struct hgadc_v0*)dev_get(HG_ADC0_DEVID);
    bat_det_priv_data->bat_det_io = MACRO_PIN(BAT_ADC_IO);

	adc_open((struct adc_device *)bat_det_priv_data->adc_dev);	

	gpio_set_mode(bat_det_priv_data->bat_det_io, GPIO_PULL_NONE, GPIO_PULL_LEVEL_NONE);

	adc_add_channel((struct adc_device *)bat_det_priv_data->adc_dev, bat_det_priv_data->bat_det_io);	

    OS_WORK_INIT(&bat_det_priv_data->work, bat_detect_work, 0);
    os_run_work_delay(&bat_det_priv_data->work, 1000);
}