#ifndef _HGIC_UMAC_H_
#define _HGIC_UMAC_H_
#include "lib/lmac/hgic.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WPA_KEY_MGMT_PSK  BIT(1)
#define WPA_KEY_MGMT_NONE BIT(2)
#define WPA_KEY_MGMT_SAE  BIT(10)
#define WPA_KEY_MGMT_OWE  BIT(22)

#define UMAC_CHAN_CNT (16)

enum umac_ops_type {
    UMAC_OPS_HOST,
    UMAC_OPS_LOCAL,
};

enum umac_ops_local_type {
    UMAC_OPS_LOCAL_TYPE_LWIP,
    UMAC_OPS_LOCAL_TYPE_GMAC,
};

struct umac_ops {
    int   type;
    void *priv;
    int (*recv_data)(struct umac_ops *ops, unsigned char *data, unsigned int len);
};

struct umac_config {
    unsigned short freq_start, freq_end;
    unsigned short chan_list[UMAC_CHAN_CNT];
    unsigned char  tx_bw, tx_mcs, acs, acs_tm, primary_chan, bgrssi, bss_bw, chan_cnt;
    unsigned char  hg0[1024];
    unsigned char  hg1[1024];
};

extern struct umac_config *sys_get_umaccfg(void);
extern int sys_save_umaccfg(struct umac_config *cfg);

extern int umac_init(void);
extern int umac_config_read(const char *name, char *buff, int size);
extern int umac_config_write(const char *name, char *buff, int size);

extern int32 umac_add_txw80x(void *ops);


extern int wpas_init(void);
extern int wpas_start(char *ifname);
extern int wpas_stop(char *ifname);
extern int wpas_cli(char *ifname, char *cmd, char *reply_buff, int reply_len);

extern int hapd_init(void);
extern int hapd_start(char *ifname);
extern int hapd_stop(char *ifname);
extern int hapd_cli(char *ifname, char *cmd, char *reply_buff, int reply_len);

#ifdef __cplusplus
}
#endif

#endif

