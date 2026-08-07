#include "typesdef.h"
#include "pdmFilter.h"
/** @brief 去直流函数，使用一阶切比雪夫1型高通滤波器，fpass=20
  * @{
  */
#define FIXED_POINT_NUM         30
//增加IIR缓存数据的小数位数，减少噪声
#define FRAC_FIXED_NUM          12

const int NUM[2][2] = {
    { 1072963942, 0,            },
    { 1073741824, -1073741824   },
};
const int DEN[2][2] = {
    { 1073741824, 0,            },
    { 1073741824, -1072186059,  },
};


int16_t rm_dc_filter(TYPE_FIRST_ORDER_FILTER_TYPE *p_filter, int16_t input)
{
    int x = input << FRAC_FIXED_NUM;
    int y;
    int64_t acc;

    acc = (int64_t)x * (int64_t)NUM[0][0];
    if(acc < 0) {
        x = (acc + (1<<FIXED_POINT_NUM) - 1) >> FIXED_POINT_NUM;
    } else {
        x = acc >> FIXED_POINT_NUM;
    }
    
    acc  = (int64_t)x * (int64_t)NUM[1][0];
    acc += (int64_t)p_filter->x * (int64_t)NUM[1][1];
    acc -= (int64_t)p_filter->y * (int64_t)DEN[1][1];
    if(acc < 0) {
        y = (acc + (1<<FIXED_POINT_NUM) - 1) >> FIXED_POINT_NUM;
    } else {
        y = acc >> FIXED_POINT_NUM;
    }
    
    p_filter->x = x;
    p_filter->y = y;

    y >>= FRAC_FIXED_NUM;
    
    //饱和操作
    if(y > 32767) {
        y = 32767;
    } else if(y < -32768) {
        y = -32768;
    }
    
    return y;
}


/**
  * @}
  */ 

/** @brief 音量控制。注意gain为 bit 定点数
  * @{
  */
int16_t pcm_volum_gain(int16_t input, int32_t gain)
{
    int32_t temp = (input * gain) >> 8;
    if(temp > 32767) {
        return 32767;
    } else if(temp < -32768) {
        return -32768;
    } else {
        return temp;
    }
}