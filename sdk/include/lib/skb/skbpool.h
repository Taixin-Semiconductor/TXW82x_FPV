#ifndef _HGIC_SKBPOOL_H_
#define _HGIC_SKBPOOL_H_
#ifdef __cplusplus
extern "C" {
#endif

enum SKBPOOL_FLAGS {
    SKBPOOL_FLAGS_MGMTFRM_INHEAP = BIT(0),
    SKBPOOL_FLAGS_LEAK_TRACE     = BIT(1),
    SKBPOOL_FLAGS_OVERFLOW_CHECK = BIT(2),
    SKBPOOL_FLAGS_ALIGN_16       = BIT(3),
    SKBPOOL_FLAGS_ALIGN_32       = BIT(4),
    SKBPOOL_FLAGS_TAIL_ALIGN_4   = BIT(5),
    SKBPOOL_FLAGS_TAIL_ALIGN_16  = BIT(6),
    SKBPOOL_FLAGS_TAIL_ALIGN_32  = BIT(7),
};

extern uint32 skbpool_time(void);
extern int32 skbpool_init(uint32 start_addr, uint32 end_addr, uint32 max, uint8 flags);
extern uint32 skbpool_freesize(int8 tx);
extern uint32 skbpool_tx_used(void);
extern uint32 skbpool_rx_used(void);
extern int32 skbpool_collect_init(void);
extern int32 skbpool_totalsize(void);
extern void skbpool_status(uint32 *status_buf, uint32 size, uint32 mini_size);
extern int32 skbpool_max_usage(uint8 tx_max, uint8 rx_max);
extern int32 skbpool_add_region(uint32 start_addr, uint32 end_addr);


#ifdef __cplusplus
}
#endif
#endif

