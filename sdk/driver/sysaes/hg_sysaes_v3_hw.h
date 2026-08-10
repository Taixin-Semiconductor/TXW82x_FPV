#ifndef _HG_SYSAES_V3_HW_H_
#define _HG_SYSAES_V3_HW_H_


#ifdef __cplusplus
extern "C" {
#endif

#define BLOCK_NUM_NUM_MSK                   0x000fffff
#define BLOCK_NUM_NUM_SHIFT                 0
	 
#define AES_CTRL_START_MSK                  0x00000001
#define AES_CTRL_START_SHIFT                0
#define AES_CTRL_EOD_MSK                    0x00000002
#define AES_CTRL_EOD_SHIFT                  1
#define AES_CTRL_MOD_MSK                    0x00000004
#define AES_CTRL_MOD_SHIFT                  2
#define AES_CTRL_IRQ_EN_MSK                 0x00000008
#define AES_CTRL_IRQ_EN_SHIFT               3

#define AES_CTRL_AES_128                    0
#define AES_CTRL_AES_192                    BIT(4)
#define AES_CTRL_AES_256                    BIT(5)
#define AES_CTRL_AES_KEYLEN_MSK             (BIT(4)|BIT(5))
#define AES_CTRL_AES_ECB                    0
#define AES_CTRL_AES_CBC                    BIT(6)
#define AES_CTRL_AES_CTR                    BIT(7)
#define AES_CTRL_AES_MODE_MSK               (BIT(6)|BIT(7))

#define AES_STAT_COMP_PD_MSK                0x00000001
#define AES_STAT_COMP_PD_SHIFT              0

enum hg_sysaes_v3_mode {
    ENCRYPT,
    DECRYPT,
};

enum hg_sysaes_v3_flags {
    HG_SYSAES_FLAGS_SUSPEND,
};

struct hg_sysaes_v3_hw {
    __IO uint32 KEY[8];
    __IO uint32 SADDR;
    __IO uint32 DADDR;
    __IO uint32 BLOCK_NUM;
         uint32 RESERVED0[1];
    __IO uint32 IV[4];
    __IO uint32 AES_CTRL;
    __IO uint32 AES_STAT;
};


#endif /* _HG_SYSAES_V3_H_ */
