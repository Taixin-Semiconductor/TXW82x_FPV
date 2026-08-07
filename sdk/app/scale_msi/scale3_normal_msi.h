#ifndef __SCALE3_NORMAL_MSI_H__
#define __SCALE3_NORMAL_MSI_H__
#include "basic_include.h"
struct scale3_normal_cmd_s
{
    uint32_t       w;
    uint32_t       h;
    uint32_t       magic;
    uint32_t       force_type;
    struct timeval t;
};

struct msi *scale3_normal_msi2(const char *name, uint8_t force_stype, uint16_t ow, uint16_t oh);
#endif