#ifndef _HGSHA_V0_HW_H_
#define _HGSHA_V0_HW_H_

#include "typesdef.h"

struct hgsha_v1_hw {
    __IO uint32 SHA_START;
    __IO uint32 SHA_BYTELEN;
    __IO uint32 SHA_CONFIG;
    __IO uint32 SHA_PENDING;
    __IO uint32 SHA_STADDR;
         uint32 reserved[3];
    __IO uint32 SHA_RESULT0;
    __IO uint32 SHA_RESULT1;
    __IO uint32 SHA_RESULT2;
    __IO uint32 SHA_RESULT3;
    __IO uint32 SHA_RESULT4;
    __IO uint32 SHA_RESULT5;
    __IO uint32 SHA_RESULT6;
    __IO uint32 SHA_RESULT7;
};

#endif
