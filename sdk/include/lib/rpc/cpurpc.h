#ifndef _HGIC_CPURPC_H_
#define _HGIC_CPURPC_H_
#ifdef __cplusplus
extern "C" {
#endif

int32 cpu_rpc_init(uint32 mbox_addr, uint32 rx_irq, uint32 tx_irq);
int32 cpu_rpc2_init(uint32 t_box, uint32 r_box, uint32 ctrl_base, uint32 irq);
int32 cpu_rpc_call(uint32 func_id, uint32 *args, uint32 arg_cnt, uint32 sync);
const void *cpu_rpc_func(uint32 func_id);

int32 cpu_splock_init(uint32 addr, uint32 irq_num);
int32 cpu_splock_lock(CPU_SPLOCK_ID lock_id);
int32 cpu_splock_unlock(CPU_SPLOCK_ID lock_id);

#define CPU_RPC_CALL(f)       cpu_rpc_call(RPC_FUNCID_##f, args, ARRAY_SIZE(args), 1)
#define CPU_RPC_CALL_ASYNC(f) cpu_rpc_call(RPC_FUNCID_##f, args, ARRAY_SIZE(args), 0)
#define RPC_FUNC_DEF(f)       [RPC_FUNCID_##f]=f

#ifdef __cplusplus
}
#endif
#endif
