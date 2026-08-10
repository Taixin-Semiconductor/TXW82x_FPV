#include "lib/touch/hyn_core.h"

void hyn_irq_set(struct hyn_ts_data *ts_data, uint8_t value)
{

}

uint32_t hyn_sum32(int val, uint32_t* buf,uint16_t len)
{
	uint32_t sum = val;
	while(len--) sum += *buf++;
	return sum;
}

void hyn_esdcheck_switch(struct hyn_ts_data *ts_data, uint8_t enable)
{

}