#ifndef _HGIC_DSLEEP_DATA_H_
#define _HGIC_DSLEEP_DATA_H_
#ifdef __cplusplus
extern "C" {
#endif

enum system_sleepdata_id {
    SYSTEM_SLEEPDATA_ID_LMAC,
    SYSTEM_SLEEPDATA_ID_UMAC,
    SYSTEM_SLEEPDATA_ID_PSALIVE,
    SYSTEM_SLEEPDATA_ID_PSCONNECT,
    SYSTEM_SLEEPDATA_ID_WKDATA,
    SYSTEM_SLEEPDATA_ID_USER,
    SYSTEM_SLEEPDATA_ID_MAX,
};

extern void sys_sleepdata_init(void);
extern void *sys_sleepdata_request(uint8 id, uint32 size);
extern void sys_sleepdata_reset(void);
extern uint32 sys_sleepdata_freesize(void);
extern void *sys_sleepdata_get(uint8 id);


#ifdef __cplusplus
}
#endif
#endif
