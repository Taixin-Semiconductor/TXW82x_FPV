#ifndef _HAL_SYSAES_H_
#define _HAL_SYSAES_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----- AES Control Codes: Mode Parameters: Key length -----*/



enum SYSAES_KEY_LEN {
    AES_KEY_LEN_BIT_128,
    AES_KEY_LEN_BIT_192,
    AES_KEY_LEN_BIT_256,
};


enum SYSAES_MODE {
    AES_MODE_ECB,
    AES_MODE_CBC,
    AES_MODE_CTR,
//    AES_MODE_CFB1,
//    AES_MODE_CFB8,
//    AES_MODE_CFB128,
//    AES_MODE_OFB,
//    AES_MODE_CTR,
};

#define AES_BLOCK_SIZE 16

struct sysaes_para {
    uint8  mode;    /* enum SYSAES_MODE */
    uint8  key_len; /* enum SYSAES_KEY_LEN */
    uint8 *src;
    uint8 *dest;
    uint32 aes_len; /* v1-v2 blocks, v3 bytes */
    uint8  key[32];
    uint8  *iv;     /* AES_BLOCK_SIZE : 16 */
};

struct sysaes_dev {
    struct dev_obj dev;
};

struct aes_hal_ops {
    struct devobj_ops ops;
    int32(*encrypt)(struct sysaes_dev *dev, struct sysaes_para *para);
    int32(*decrypt)(struct sysaes_dev *dev, struct sysaes_para *para);
};

int32 sysaes_encrypt(struct sysaes_dev *dev, struct sysaes_para *para);
int32 sysaes_decrypt(struct sysaes_dev *dev, struct sysaes_para *para);

#ifdef __cplusplus
}
#endif
#endif /* _HAL_SYSAES_H_ */

