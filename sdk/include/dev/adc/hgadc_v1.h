#ifndef _HGADC_V1_H
#define _HGADC_V1_H
#include "hal/adc.h"

#ifdef __cplusplus
extern "C" {
#endif

struct hgadc_v1;

typedef struct _adc_channel_data {
    /* pointer of config function */
    int32 (*func)(struct hgadc_v1 *dev, uint32 channel, uint32 enable);

    /* channel */
    uint16 channel;

    /* sel for hw*/
    uint32 sel;
    uint16 sch_vdd;
    
}adc_channel_data;


typedef struct _adc_channel_node {
    /* data type */
    adc_channel_data data;

    /* for list opreation */
    struct _adc_channel_node *next;
    
    /* channel amount */
    uint8 channel_amount;
}adc_channel_node;



struct hgadc_v1 {
    struct adc_device   dev;
    uint32              hw;
    adc_irq_hdl         irq_hdl;
    uint32              irq_data;
    uint32              irq_num;
    os_mutex_t          adc_lock;
    os_semaphore_t      adc_done;
    adc_channel_node    head_node;
    adc_channel_node   *cur_node;
    uint32              refer_vddi;
    uint32              refer_vddi_adc_data;
    uint32              refer_tsensor;
    uint32              refer_pmu_tsensor;
    uint32              refer_adda_vref;
    uint32              ref_temp;
    int32               adkey_range;
    uint32              rf_vddi_en:1,
                        opened    :1,
                        irq_en    :1;
};

struct hgadc_v1_hw;

void saradc_pri_channel_config(struct hgadc_v1_hw *hw, uint8 pri_chanx_en,uint8 pri_chan_sel,uint8 pri_trg_sel);
void saradc_pri_channel_clr_pending(struct hgadc_v1_hw *hw, uint8 pri_chan_sel);
void saradc_pri_channel_kick(struct hgadc_v1_hw *hw);
uint16 saradc_pri_channel_get_pending(struct hgadc_v1_hw *hw, uint8 pri_chan_sel);
void saradc_pri_channel_disable(struct hgadc_v1_hw *hw, uint8 pri_chanx_en);
uint16 saradc_pri_channel_get_raw_data(struct hgadc_v1_hw *hw, uint8 pri_chan_sel);
void sar_adc_sample_one_channel(uint32 channel, uint32 *raw_data);

int32 hgadc_v1_attach(uint32 dev_id, struct hgadc_v1 *adc);


#ifdef __cplusplus
}
#endif
#endif
