
#ifndef _OS_ATOMIC_H_
#define _OS_ATOMIC_H_
#include "typesdef.h"
#include "osal/irq.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8 counter;
} atomic8_t;

typedef struct {
    uint16 counter;
} atomic16_t;

typedef struct {
    uint32 counter;
} atomic_t;

typedef struct {
    uint64 counter;
} atomic64_t;


#define atomic64_read(v)        ((v)->counter)
#define atomic_read(v)          ((v)->counter)

#define atomic_set(v,i)        ({ \
        uint32 __mask__ = disable_irq();\
        ((v)->counter = i); \
        enable_irq(__mask__);\
    })
#define atomic_add(v,i)        ({ \
        uint32 __mask__ = disable_irq();\
        ((v)->counter += i); \
        enable_irq(__mask__);\
    })
#define atomic_sub(v,i)        ({ \
        uint32 __mask__ = disable_irq();\
        if((v)->counter > (i)) { ((v)->counter -= (i)); }\
        else { (v)->counter = 0; }\
        enable_irq(__mask__);\
    })
#define atomic_inc(v)           ({ \
        uint32 __mask__ = disable_irq();\
        ((v)->counter++); \
        enable_irq(__mask__);\
    })
#define atomic_dec(v)           ({ \
        uint32 __mask__ = disable_irq();\
        if((v)->counter > 0){ (v)->counter--; }\
        enable_irq(__mask__);\
    })
#define atomic_inc_return(v)    ({\
        uint32 __mask__ = disable_irq();\
        uint32_t __val__ = (++(v)->counter);\
        enable_irq(__mask__);\
        __val__;\
    })
#define atomic_dec_return(v)    ({\
        uint32 __mask__ = disable_irq();\
        uint32_t __val__ = ((v)->counter > 0) ? (--(v)->counter) : 0;\
        enable_irq(__mask__);\
        __val__;\
    })
#define atomic_dec2_return(v)    ({\
        uint32 __mask__ = disable_irq();\
        uint32_t __val__ = (v)->counter;\
        if((v)->counter) (--(v)->counter);\
        enable_irq(__mask__);\
        __val__;\
    })
#define atomic_dec_and_test(v)    ({\
        uint32 __mask__ = disable_irq();\
        uint32_t __val__ = ((v)->counter > 0) ? ((--(v)->counter) == 0) : 1;\
        enable_irq(__mask__);\
        __val__;\
    })
#define atomic_cmpxchg(v,o,n)    ({\
        uint32 __mask__ = disable_irq();\
        uint32_t __val = (v)->counter;\
        if(__val == o) { (v)->counter = n; }\
        enable_irq(__mask__);\
        __val;\
    })

static inline int atomic_add_unless(atomic_t *v, int a, int u)
{
    int c, old;
    c = atomic_read(v);
    while (c != u && (old = atomic_cmpxchg((v), c, c + a)) != c) {
        c = old;
    }
    return c != u;
}

#ifdef __cplusplus
}
#endif

#endif
