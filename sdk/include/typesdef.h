#ifndef _HUGEIC_TYPES_H_
#define _HUGEIC_TYPES_H_
#include "sys_config.h"
#include "errno.h"
#include "version.h"

#ifdef ARCH_CSKY
#include "chip/arch/csky/defs.h"
#include "chip/arch/csky/byteshift.h"
#include "chip/arch/csky/string.h"
#include "chip/arch/csky/time.h"
#endif

#ifndef __IO
#define __IO   volatile
#endif

#ifndef __I
#define __I    volatile const
#endif

#ifndef __O
#define __O    volatile
#endif

#ifndef RET_OK
#define RET_OK    (0)
#endif

#ifndef RET_ERR
#define RET_ERR   (-1)
#endif

#ifndef TRUE
#define TRUE      (1)
#endif

#ifndef FALSE
#define FALSE     (0)
#endif

#ifndef ALIGN
#define ALIGN(s,a)           (((s)+(a)-1) & ~((a)-1))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr)      (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef STRUCT_PACKED
#define STRUCT_PACKED        __attribute__ ((__packed__))
#endif

#ifndef __always_inline
#define __always_inline      inline
#endif

#ifndef ASSERT_HOLDUP
#define ASSERT_HOLDUP        (0)
#endif

#ifndef ATCMD_ARGS_COUNT
#define ATCMD_ARGS_COUNT     (16)
#endif

#ifndef ATCMD_PRINT_BUF_SIZE
#define ATCMD_PRINT_BUF_SIZE (256)
#endif

#ifndef osWaitForever
#define osWaitForever        (-1)
#endif

#ifndef NULL
#define NULL                 ((void *)0)
#endif

#ifndef __inline
#define __inline            inline
#endif

#ifndef __weak
#define __weak              __attribute__((weak))
#endif

#ifndef __alias
#define __alias(f)          __attribute__((alias(#f)))
#endif

#ifndef __at_section
#define __at_section(sec)   __attribute__((section(sec)))
#endif

#ifndef __used
#define __used              __attribute__((used))
#endif

#ifndef __aligned
#define __aligned(n)        __attribute__((aligned(n)))
#endif

#ifndef __packed
#define __packed            __attribute__((packed))
#endif

#ifndef __init
#define __init            //__at_section("INIT.TXT")
#endif

#ifndef __initdata
#define __initdata        //__at_section("INIT.DAT")
#endif

#ifndef __initconst
#define __initconst       //__at_section("INIT.RO")
#endif

#ifndef __rom
#define __rom             //__at_section(".rom.text")
#endif

#ifndef __romro
#define __romro           //__at_section(".rom.ro")
#endif

#ifndef __ram
#define __ram             __at_section(".ram.text")
#endif

#ifndef __bobj
#define __bobj            //__at_section(".bobj")
#endif

#ifndef __dsleeptext
#define __dsleeptext      __at_section(".dsleep.text")
#endif

#ifndef __dsleepdata
#define __dsleepdata      __at_section(".dsleep.data")
#endif

#ifndef __psram_data
#define __psram_data      //__at_section(".psram.data")
#endif

#ifndef __noinit
#define __noinit          //__at_section(".no_init")
#endif

#ifndef BIT
#define BIT(a)             (1UL << (a))
#endif

#ifndef offsetof
#define offsetof(type, member)   ((long) &((type *) 0)->member)
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({      \
        void *__mptr = (void *)(ptr);                   \
        (type *)( (char *)__mptr - offsetof(type,member) );})
#endif

#ifndef min
#define min(a, b) ((a)<(b)?(a):(b))
#endif

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#include "tx_platform.h"

typedef struct {
    uint8  *addr;
    uint32  size;
} scatter_data;

#ifdef CONFIG_SLEEP
#define __SYS_INIT
#else
#define __SYS_INIT __init
#endif

#endif

