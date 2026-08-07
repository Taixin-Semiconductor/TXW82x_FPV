#ifndef __PDMFILTER_H
#define __PDMFILTER_H
#include "typesdef.h"
typedef struct {
    int x;
    int y;
} TYPE_FIRST_ORDER_FILTER_TYPE;

int16_t rm_dc_filter(TYPE_FIRST_ORDER_FILTER_TYPE *p_filter, int16_t input);
int16_t pcm_volum_gain(int16_t input, int32_t gain);
#endif