#ifndef _UI_VPP_IPF_RESOURCE_H_
#define _UI_VPP_IPF_RESOURCE_H_

#include "basic_include.h"

typedef struct {
    uint32_t data_size;     /**< Size of the image in bytes*/
    const uint8_t * data;   /**< Pointer to the data of the image*/
} _ipf_img_resource_;

#define UI_VPP_IPF_RES_MAX_NUM      2

extern const _ipf_img_resource_ *ipf_imgSrcTable[];

#endif
