#ifndef _HGIC_SKBUFF_H_
#define _HGIC_SKBUFF_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <osal/atomic.h>
#include <osal/string.h>
#include "list.h"

struct sk_buff {
    struct sk_buff *next, *prev, *clone;
    uint8   cb[12];
    uint8  *tail;
    uint8  *data;
    uint8  *head;
    uint8  *end;

    /* clear below flags when skb free */
    uint16  len;
    uint16  priority:4, acked: 1, cloned: 1, lmaced: 1, pkt_type: 3, tx: 1, src_in: 1, unreachable:1, rev:3;
    uint64  lifetime;
    void   *txinfo;
    void   *sta;
    /////////////////////////////////////
    atomic_t users;
    void (*free)(void *free_priv, struct sk_buff *skb);
    void *free_priv;
};

uint8 *skb_put(struct sk_buff *skb, uint32 len);
uint8 *skb_push(struct sk_buff *skb, uint32 len);
uint8 *skb_pull(struct sk_buff *skb, uint32 len);
void skb_reserve(struct sk_buff *skb, int len);

static inline int skb_tailroom(const struct sk_buff *skb)
{
    return (skb->end - skb->tail);
}

static inline unsigned int skb_headroom(const struct sk_buff *skb)
{
    return skb->data - skb->head;
}

static inline int skb_dataroom(const struct sk_buff *skb)
{
    return (skb->end - skb->data);
}

static inline struct sk_buff *skb_get(struct sk_buff *skb)
{
    atomic_inc(&skb->users);
    return skb;
}

void *skb_put_zero(struct sk_buff *skb, unsigned int len);
void *skb_put_data(struct sk_buff *skb, const void *data, unsigned int len);

#ifdef __cplusplus
}
#endif
#endif

