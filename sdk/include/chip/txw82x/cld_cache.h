/*
 * Copyright (C) 2024 ZHUHAI TAIXIN Co., Ltd. All rights reserved.
 *
 */

/******************************************************************************
 * @file     cld_cahce.h
 * @brief    cldache Access Layer Header File
 * @version  V1.0
 * @date     2024.5.29
 ******************************************************************************/

#ifndef __CLD_CACHE_H_GENERIC
#define __CLD_CACHE_H_GENERIC

#include <stdint.h>
#include <csi_gcc.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 *                  definitions
 ******************************************************************************/
/**
  \ingroup 
  @{
 */

/*   definitions */
typedef struct
{
    __I  uint32_t HWPARAMS;            //0x000
    __I  uint32_t reserve0[3];         //0x004-0x00c
    __IO uint32_t CTRL;                //0x010
    __I  uint32_t NSEC_ACCESS;         //0x014
    __I  uint32_t reserve1[2];         //0x018-0x01c
    __O  uint32_t MAINT_CTRL_ALL;      //0x020  
    __O  uint32_t MAINT_CTRL_LINES;    //0x024
    __I  uint32_t MAINT_STATUS;        //0x028
    __I  uint32_t reserve2[53];        //0x02c-0x0fc
    __I  uint32_t SECIRQSTAT;          //0x100;
    __O  uint32_t SECIRQSCLR;          //0x104;
    __IO uint32_t SECIRQEN;            //0x108;
    __I  uint32_t SECIRQINFO1;         //0x10C;
    __I  uint32_t SECIRQINFO2;         //0x110;
    __I  uint32_t reserve3[11];        //0x114-0x13c
    __I  uint32_t NSECIRQSTAT;         //0x140;
    __O  uint32_t NSECIRQSCLR;         //0x144;
    __IO uint32_t NSECIRQEN;           //0x148;
    __I  uint32_t NSECIRQINFO1;        //0x14C;
    __I  uint32_t NSECIRQINFO2;        //0x150;
    __I  uint32_t reserve4[107];       //0x154-0x2fc
    __I  uint32_t SECHIT;              //0x300;
    __I  uint32_t SECMISS;             //0x304;
    __IO uint32_t SECSTATCTRL;         //0x308;
    __I  uint32_t reserve5[1];         //0x30c
    __I  uint32_t NSECHIT;             //0x310;
    __I  uint32_t NSECMISS;            //0x314;
    __IO uint32_t NSECSTATCTRL;        //0x318;
    __I  uint32_t reserve6[813];       //0x31c-0xFCC
    __I  uint32_t PIDR4;               //0xFD0;
    __I  uint32_t PIDR5;               //0xFD4;
    __I  uint32_t PIDR6;               //0xFD8;
    __I  uint32_t PIDR7;               //0xFDC;
    __I  uint32_t PIDR0;               //0xFE0;
    __I  uint32_t PIDR1;               //0xFE4;
    __I  uint32_t PIDR2;               //0xFE8;
    __I  uint32_t PIDR3;               //0xFEC;
    __I  uint32_t CIDR0;               //0xFF0;
    __I  uint32_t CIDR1;               //0xFF4;
    __I  uint32_t CIDR2;               //0xFF8;
    __I  uint32_t CIDR3;               //0xFFC; 
} DCACHE_CTRL_TypeDef;
#define DCACHE_CTRL_BASE2        ((uint32_t)0x50000000)
#define DCACHE_CTRL             ((DCACHE_CTRL_TypeDef *) DCACHE_CTRL_BASE2)

#define DCACHE_MAINT_NO_SEC       ((uint32_t)0x00000004)


#define DCACHE_STA_ENABLE         ((uint32_t)0x00000001)
#define DCACHE_STA_ONGOING_EN     ((uint32_t)0x00000002)
#define DCACHE_STA_ONGOING_MAINT  ((uint32_t)0x00000004)
#define DCACHE_STA_ONGOING_PWR_MAINT  ((uint32_t)0x00000008)
#define DCACHE_STA_IS_CLEAN       ((uint32_t)0x00000010)
static inline uint32_t cld_cache_get_status(void)
{
    return DCACHE_CTRL->MAINT_STATUS;
}

/* TXW82x S_NS ONLY */
#define CLD_INTR_NEST_MAINT_SUPPORT     0
#if CLD_INTR_NEST_MAINT_SUPPORT
#define CLD_CACHE_MAINT_OPT(opt)     do { uint32 __disable_irq(void); void __enable_irq(void); uint32 flag = __disable_irq(); do { DCACHE_CTRL->SECIRQSCLR = 0x8; opt; } while (DCACHE_CTRL->SECIRQSTAT & 0x8); if(!flag) __enable_irq(); } while(0)
#else
#define CLD_CACHE_MAINT_OPT(opt)     do { DCACHE_CTRL->SECIRQSCLR = 0x8; opt; } while(DCACHE_CTRL->SECIRQSTAT & 0x8)
#endif
#define CLD_CACHE_MAINT_OPT_WAIT()   do { while (cld_cache_is_ongoing_maint()); } while (0)

static inline uint32_t cld_cache_is_maint_done(void)
{
    return (DCACHE_CTRL->SECIRQSTAT & 0x4);
}
static inline void cld_cache_clr_maint_done(void)
{
    DCACHE_CTRL->SECIRQSCLR = 0x4;
}

static inline uint32_t cld_cache_is_enable(void)
{
    return DCACHE_CTRL->MAINT_STATUS & DCACHE_STA_ENABLE;
}

static inline uint32_t cld_cache_is_clean(void)
{
    return DCACHE_CTRL->MAINT_STATUS & DCACHE_STA_IS_CLEAN;
}

static inline uint32_t cld_cache_is_ongoing_enable(void)
{
    return DCACHE_CTRL->MAINT_STATUS & DCACHE_STA_ONGOING_EN;
}

static inline uint32_t cld_cache_is_ongoing_maint(void)
{
    return DCACHE_CTRL->MAINT_STATUS & DCACHE_STA_ONGOING_MAINT;
}

static inline uint32_t cld_cache_is_ongoing_pwr_maint(void)
{
    return DCACHE_CTRL->MAINT_STATUS & DCACHE_STA_ONGOING_PWR_MAINT;
}

static inline void cld_cache_force_write_through(void)
{
    DCACHE_CTRL->CTRL |= 1UL << 1; 
}

static inline void cld_cache_enable(void)
{
    DCACHE_CTRL->CTRL |= 1UL << 0;// | 0x70000; 
    while (cld_cache_is_ongoing_enable());
}

static inline void cld_cache_irq_enable(void)
{
    DCACHE_CTRL->SECIRQEN = 0x90;
}

static inline void cld_cache_invalidate_all(void)
{
    DCACHE_CTRL->MAINT_CTRL_ALL = 0x2;
    while (cld_cache_is_ongoing_maint());
}

static inline void cld_cache_disable(void)
{
    cld_cache_invalidate_all();

    DCACHE_CTRL->CTRL &= ~(1UL << 0);
    while (cld_cache_is_ongoing_enable());
}


static inline void cld_cache_clean_all(void)
{
    DCACHE_CTRL->MAINT_CTRL_ALL = 0x1;
    while (cld_cache_is_ongoing_maint());
}


static inline void cld_cache_clean_line(uint32_t addr)
{
    CLD_CACHE_MAINT_OPT(DCACHE_CTRL->MAINT_CTRL_LINES = ((uint32_t)addr & (0xFFFFFFFF << 5)) | 0x1 | DCACHE_MAINT_NO_SEC;);
    CLD_CACHE_MAINT_OPT_WAIT();
     
}


/**
  \brief   D-Cache Clean by address
  \details Cleans D-Cache for the given address
  \param[in]   addr    address (aligned to 32-byte boundary)
  \param[in]   dsize   size of memory block (in number of bytes)
*/
static inline void cld_cache_clean_range (uint32_t *addr, int32_t dsize)
{
    int32_t op_size = ((uint32_t)addr & 0x1F) + dsize;
    uint32_t op_addr = (uint32_t)addr & (0xFFFFFFFF << 5);
    int32_t linesize = 32;

    op_addr |= 0x1 | DCACHE_MAINT_NO_SEC;
    if (op_size < 32) op_size = 32;

    while (op_size > 0) {
        CLD_CACHE_MAINT_OPT(DCACHE_CTRL->MAINT_CTRL_LINES = op_addr;);
        op_addr += linesize;
        op_size -= linesize;
        CLD_CACHE_MAINT_OPT_WAIT();
    }
}


/**
  \brief   D-Cache Clean and Invalidate by address
  \details Cleans and invalidates D_Cache for the given address
  \param[in]   addr    address (aligned to 32-byte boundary)
  \param[in]   dsize   size of memory block (in number of bytes)
*/
static inline void cld_cache_clean_invalid_range (uint32_t *addr, int32_t dsize)
{
    int32_t op_size = ((uint32_t)addr & 0x1F) + dsize;
    uint32_t op_addr = (uint32_t)addr & (0xFFFFFFFF << 5);
    int32_t linesize = 32;

    op_addr |= 0x3 | DCACHE_MAINT_NO_SEC;
    if (op_size < 32) op_size = 32;

    while (op_size > 0) {
        CLD_CACHE_MAINT_OPT(DCACHE_CTRL->MAINT_CTRL_LINES = op_addr;);
        op_addr += linesize;
        op_size -= linesize;
        CLD_CACHE_MAINT_OPT_WAIT();
    }
}

/**
  \brief   D-Cache Invalidate by address
  \details Cleans and invalidates D_Cache for the given address
  \param[in]   addr    address (aligned to 32-byte boundary)
  \param[in]   dsize   size of memory block (in number of bytes)
*/
static inline void cld_cache_invalid_range (uint32_t *addr, int32_t dsize)
{
    int32_t op_size = ((uint32_t)addr & 0x1F) + dsize;
    uint32_t op_addr = (uint32_t)addr & (0xFFFFFFFF << 5);
    int32_t linesize = 32;

    //!!! It also cannot start invalidate-only maintenance by S_NS
    op_addr |= 0x2;
    if (op_size < 32) op_size = 32;

    while (op_size > 0) {
        CLD_CACHE_MAINT_OPT(DCACHE_CTRL->MAINT_CTRL_LINES = op_addr;);
        op_addr += linesize;
        op_size -= linesize;
        CLD_CACHE_MAINT_OPT_WAIT();
    }
}

static inline uint32_t cld_cache_get_nsec_hits(void)
{
    return DCACHE_CTRL->NSECHIT;
}
static inline uint32_t cld_cache_get_nsec_miss(void)
{
    return DCACHE_CTRL->NSECMISS;
}
static inline void cld_cache_reset_profile(void)
{
    DCACHE_CTRL->NSECSTATCTRL |= 0x2;
//    DCACHE_CTRL->SECSTATCTRL |= 0x2;
}

static inline void cld_cache_enable_profile(void)
{
    DCACHE_CTRL->NSECSTATCTRL |= 0x1;
//    DCACHE_CTRL->SECSTATCTRL |= 0x1;
}

static inline void cld_cache_disable_profile(void)
{
    DCACHE_CTRL->NSECSTATCTRL &= ~ 0x1;
//    DCACHE_CTRL->SECSTATCTRL &= ~ 0x1;
}


#ifdef __cplusplus
}
#endif

#endif /* __CLD_CACHE_H_GENERIC */

