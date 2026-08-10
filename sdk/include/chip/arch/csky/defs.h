#ifndef _ARCH_CSKY_DEFS_H_
#define _ARCH_CSKY_DEFS_H_
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////////
#define RETURN_ADDR()        __builtin_return_address(0)
#define cpu_dcache_disable() csi_dcache_clean_invalid(); csi_dcache_disable();
#define cpu_dcache_enable()  csi_dcache_enable()

typedef signed char          int8;
typedef signed short         int16;
typedef signed int           int32;
typedef signed long long     int64;

typedef unsigned char        uint8;
typedef unsigned short       uint16;
typedef unsigned int         uint32;
typedef unsigned long long   uint64;
typedef unsigned long        ulong;

#ifndef _SIZE_T_DECLARED
typedef unsigned int    size_t;
#define _SIZE_T_DECLARED
#endif

#ifndef _SSIZE_T_DECLARED
#if defined(__INT_MAX__) && __INT_MAX__ == 2147483647
typedef int ssize_t;
#else
typedef long ssize_t;
#endif
#define _SSIZE_T_DECLARED
#endif
/////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
#endif
