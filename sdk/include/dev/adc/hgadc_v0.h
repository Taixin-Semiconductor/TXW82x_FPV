#ifndef _HGADC_V0_H
#define _HGADC_V0_H
#include "hal/adc.h"

#ifdef __cplusplus
extern "C" {
#endif

struct hgadc_v0;

typedef struct _adc_channel_data {
    /* pointer of config function */
    int32 (*func)(struct hgadc_v0 *dev, uint32 channel, uint32 enable);

    /* channel */
    uint16 channel;
}adc_channel_data;


typedef struct _adc_channel_node {
    /* data type */
    adc_channel_data data;

    /* for list opreation */
    struct _adc_channel_node *next;
    
    /* channel amount */
    uint8 channel_amount;
}adc_channel_node;



struct hgadc_v0 {
    struct adc_device   dev;
    uint32              hw;
    adc_irq_hdl         irq_hdl;
    uint32              irq_data;
    uint32              irq_num;
    os_mutex_t     adc_lock;
    os_semaphore_t adc_done;
    adc_channel_node    head_node;
    uint32              refer_vddi;
    uint32              refer_vddi_adc_data;
    uint32              refer_tsensor;
    uint32              refer_adda_vref;
    uint32              rf_vddi_en:1,
                        opened    :1,
                        irq_en    :1;
};





int32 hgadc_v0_attach(uint32 dev_id, struct hgadc_v0 *adc);


#ifdef __cplusplus
}
#endif
#endif
