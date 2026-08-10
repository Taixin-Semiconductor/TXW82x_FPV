#ifndef _IOKEY_H
#define _IOKEY_H
#include "keyScan.h"
typedef struct
{
    void *priv;
    uint8 pin;        // io
    uint8 pull;       // 上拉下拉
    uint8 pull_level; // 上下拉电阻
} iokey_t;
extern key_channel_t iokey_key;


#endif