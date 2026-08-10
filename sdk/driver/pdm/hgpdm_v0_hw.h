#ifndef _HGPDM_V0_HW_H
#define _HGPDM_V0_HW_H


#ifdef __cplusplus
    extern "C" {
#endif

    /***** CON *****/
#define LL_PDM_CON_LRSW(n)                      ((n) << 4)
#define LL_PDM_CON_DECIM(n)                     ((n) << 3)
#define LL_PDM_CON_WORKMODE(n)                  ((n) << 1)
#define LL_PDM_CON_ENABLE(n)                    ((n) << 0)
    
    /***** DMACON *****/
#define LL_PDM_DMACON_OV_IE_EN(n)               ((n) << 1)
#define LL_PDM_DMACON_HF_IE_EN(n)               ((n) << 0)
    
    /***** STA *****/
#define LL_PDM_STA_OV_PENDING(n)                ((n) << 1)
#define LL_PDM_STA_HF_PENDING(n)                ((n) << 0)
    
    
struct hgpdm_v0_hw {
    __IO uint32 CON;
    __IO uint32 DMACON;
    __IO uint32 DMASTADR;
    __IO uint32 RESERVE;
    __IO uint32 DMALEN;
    __IO uint32 DMACNT;
    __IO uint32 STA;
};


#ifdef __cplusplus
}
#endif

#endif /* _HGPDM_V0_HW_H */