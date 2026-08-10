#ifndef __SCALE3_NORMAL_MSI_H__
#define __SCALE3_NORMAL_MSI_H__
#include "basic_include.h"
struct scale3_normal_cmd_s
{
    uint16_t       w;
    uint16_t       h;
    uint16_t       x;
    uint16_t       y;
    uint32_t       magic;
    uint32_t       force_type;
    uint8_t        is_thumb;
    struct timeval t;
};

struct scale3_cfg
{
    uint16_t ow;
    uint16_t oh;
    uint8_t  force_stype;
    uint8_t  splice_en : 1, start : 1, rev : 6;
};

struct msi *scale3_normal_msi(const char *name, struct scale3_cfg *cfg);
#endif