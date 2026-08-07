#ifndef __SCALE3_NORMAL_MSI_H__
#define __SCALE3_NORMAL_MSI_H__
#include "basic_include.h"
struct scale3_normal_cmd_s
{
    uint16_t       w;
    uint16_t       h;
    uint32_t       magic;
    uint32_t       force_type;
    uint8_t        is_thumb;
    struct timeval t;
};

struct msi *scale3_normal_msi2(const char *name, uint8_t force_stype, uint16_t ow, uint16_t oh);
struct msi *scale3_msi_no_lcd(const char *name, uint8_t splice, uint8_t force_stype, uint16_t ow, uint16_t oh);
struct msi *scale3_normal_msi(const char *name, uint16_t ow, uint16_t oh);
#endif