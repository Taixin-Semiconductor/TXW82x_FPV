#ifndef _HAL_SHA_H_
#define _HAL_SHA_H_
#ifdef __cplusplus
extern "C" {
#endif

struct sha_dev {
    struct dev_obj dev;
};

enum sha_ioctl_cmd {
    SHA_IOCTL_CMD_START,
    SHA_IOCTL_CMD_END,
    SHA_IOCTL_CMD_RESET,
    SHA_IOCTL_GET_CTX,
    SHA_IOCTL_PUT_CTX,
	SHA_IOCTL_RELOAD_CTX,
};

typedef enum _SHA_TYPE {
    T_SHA1,
    T_SHA256,
} SHA_TYPE;

struct sha_ctx {
    SHA_TYPE type;
    uint32_t state[8];
    uint8_t  buf[64];
    uint32_t dlen;
    unsigned long long bit_len;
};

struct sha_req {
    SHA_TYPE type;
    uint32_t *state;
    uint8_t  *input;
    uint32_t len;
};

struct sha_hal_ops {
    struct devobj_ops ops;
    int32 (*init)(struct sha_dev *dev, SHA_TYPE type);
    int32 (*update)(struct sha_dev *dev, uint8 *input, uint32 len);
    int32 (*final)(struct sha_dev *dev, uint8 *output);
    int32 (*xform)(struct sha_dev *dev, struct sha_req *req);
    int32 (*ioctl)(struct sha_dev *dev, uint32 cmd, uint32 param1, uint32 param2);
    int32 (*requset_irq)(struct sha_dev *dev, void *irq_handle, void *args);
    int32 (*release_irq)(struct sha_dev *dev);
};

int32 sha_ioctl(struct sha_dev *dev, uint32 cmd, uint32 para);
int32 sha_init(struct sha_dev *dev, SHA_TYPE type);
int32 sha_update(struct sha_dev *dev, uint8 *input, uint32 len);
int32 sha_final(struct sha_dev *dev, uint8 *output);
int32 sha_xform(struct sha_dev *dev, struct sha_req *req);
int32 sha_requset_irq(struct sha_dev *dev, void *irq_handle, void *args);
int32 sha_release_irq(struct sha_dev *dev);

#ifdef __cplusplus
}
#endif

#endif

