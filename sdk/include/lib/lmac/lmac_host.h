#ifndef _HGIC_LMAC_HOST_H_
#define _HGIC_LMAC_HOST_H_

#ifdef __cplusplus
extern "C" {
#endif

struct lmachost_param {
    uint8 bus_type;
    uint8 task_priority;
    uint16 stack_size;
    struct lmac_ops *lops;
    struct bt_ops   *btops;
    uint32 rxwrite: 1, rev: 31;
    uint32 dma_scatter;
};

struct lmac_host {
    uint16 stop: 1, req_txwnd: 1, bleraw: 1, user_edca: 1, init_report: 1, 
           icmp_mntr: 1, rxwrite: 1, throughput_en: 3, 
           dup_filter:1, sta_mode:1;
    uint16 cookie, last_cookie;
    uint16 aggbuff_len, agg_len;
    uint8  if_monitor;  //interface monitor.
    atomic8_t if_alive; //interface alive.
    uint32 dma_scatter;

    struct mac_bus  *bus;
    struct lmac_ops *ops;
    struct bt_ops   *btops;

    struct skb_list     iftest_list;
    struct skb_list     cmd_list;
    struct skb_list     up2host;
    //struct skb_list     txstatus_list;
    struct skb_list     rx_list;
    os_semaphore_t sema;
    os_task_t      task;
    uint8         *agg_buff;
    struct sk_buff *cmdskb;

    uint16 sn_head[9];
    uint64 sn_bitmap[9];

    uint16 stats_invalid_frm, stats_cookie_err;
    uint16 stats_unsupport_frm, stats_rx_loop;
    uint32 stats_recv_nomem;
    uint32 rx_bytes, tx_bytes;
    uint64 throughput_jiff;
};


void lmac_host_report_init_done(struct lmac_host *lmachost);
int32 lmac_host_init(struct lmachost_param *l_param, struct macbus_param *m_param);
int32 host_event_new(uint32 evt_id, uint8 *data, uint32 data_len);
int32 host_data_send(uint8 *data, uint32 data_len);
void lmac_host_status(void);

#ifdef __cplusplus
}
#endif

#endif
