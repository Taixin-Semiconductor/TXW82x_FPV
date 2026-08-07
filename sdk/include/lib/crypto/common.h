#ifndef COMMON_H
#define COMMON_H

#include "osal/string.h"

typedef uint64_t   u64;
typedef uint32_t   u32;
typedef uint16_t   u16;
typedef uint8_t    u8;

typedef int64_t   s64;
typedef int32_t   s32;
typedef int16_t   s16;
typedef int8_t    s8;

#ifndef __must_check
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define __must_check __attribute__((__warn_unused_result__))
#else
#define __must_check
#endif /* __GNUC__ */
#endif /* __must_check */


#endif /* COMMON_H */
