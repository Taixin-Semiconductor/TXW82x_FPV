#ifndef __ARCH_CSKY_STDLIB_H_
#define __ARCH_CSKY_STDLIB_H_
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define os_htons(x)      ((((x) & (uint16)0x00ffUL) << 8) | (((x) & (uint16)0xff00UL) >> 8))
#define os_ntohs(x)      os_htons(x)
#define os_htonl(x)      ((((x) & (uint32)0x000000ffUL) << 24) | \
                           (((x) & (uint32)0x0000ff00UL) <<  8) | \
                           (((x) & (uint32)0x00ff0000UL) >>  8) | \
                           (((x) & (uint32)0xff000000UL) >> 24))
#define os_ntohl(x)      os_htonl(x)

int strncasecmp(const char *s1, const char *s2, size_t n);
int strcasecmp(const char *s1, const char *s2);

#ifdef __cplusplus
}
#endif

#endif


