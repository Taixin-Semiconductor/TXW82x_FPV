#ifndef _JPG_DECODE_H_
#define _JPG_DECODE_H_ 
#include "basic_include.h"
#include "lib/multimedia/msi.h"
struct msi *new_jpg_decode_msi(const char *name, uint16_t dw, uint16_t dh);
#endif