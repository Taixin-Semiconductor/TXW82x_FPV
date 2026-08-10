#ifndef _HGIC_SKB_H_
#define _HGIC_SKB_H_

#include "lib/skb/skbuff.h"
#include "lib/skb/skbpool.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SKB_TRACE
#define alloc_rx_skb(s) _alloc_rx_skb(s, __FUNCTION__, __LINE__)
#define alloc_tx_skb(s) _alloc_tx_skb(s, __FUNCTION__, __LINE__)
#define alloc_skb(s)    _alloc_skb(s, __FUNCTION__, __LINE__)
#define skb_copy(s, f)  _skb_copy(s, f, __FUNCTION__, __LINE__)
#define skb_clone(s, f) _skb_clone(s, f, __FUNCTION__, __LINE__)
#else
extern struct sk_buff *alloc_rx_skb(uint32 size);
extern struct sk_buff *alloc_tx_skb(uint32 size);
extern struct sk_buff *alloc_skb(uint32 size);
extern struct sk_buff *skb_copy(const struct sk_buff *skb, int32 flags);
extern struct sk_buff *skb_clone(struct sk_buff *skb, int32 flags);
#endif
extern struct sk_buff *_alloc_rx_skb(uint32 size, char *func, int32 line);
extern struct sk_buff *_alloc_tx_skb(uint32 size, char *func, int32 line);
extern struct sk_buff *_alloc_skb(uint32 size, char *func, int32 line);
extern struct sk_buff *_skb_copy(const struct sk_buff *skb, int32 flags, char *func, int32 line);
extern struct sk_buff *_skb_clone(struct sk_buff *skb, int32 flags, char *func, int32 line);
extern void kfree_skb(struct sk_buff *skb);
extern void copy_skb_header(struct sk_buff *n, const struct sk_buff *old);
extern struct sk_buff *alloc_heapskb(uint32 size);


#ifdef __cplusplus
}
#endif
#endif

