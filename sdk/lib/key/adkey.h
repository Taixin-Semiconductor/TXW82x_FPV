#ifndef _ADKEY_H
#define _ADKEY_H
#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "hal/gpio.h"
#include "osal/timer.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "keyScan.h"



struct adkey_t;
struct keys_t;
typedef struct adkey_t adkey_t;



struct adkey_t 
{
  void *priv;
  uint32 pin;   //io
  uint8  pull;  //上拉下拉
  uint8  pull_level;    //上下拉电阻
};



struct adkey_scan_code
{
    int adc;
    uint8 key;
};

key_channel_t adkey_key;
#endif