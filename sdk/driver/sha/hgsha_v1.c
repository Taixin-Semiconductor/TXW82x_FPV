/**
 ******************************************************************************
 * @file    hgsha.c
 * @author  HUGE-IC Application Team
 * @version V1.0.0
 * @date
 * @brief   sha256
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2019 HUGE-IC</center></h2>
 *
 *
 *
 ******************************************************************************
 */
#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"

#include "osal/string.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "osal/sleep.h"

#include "dev/sha/hgsha_v1.h"
#include "hgsha_v1_hw.h"
#include "hal/sha.h"

#if 1
#define SHA_PRINTF(fmt, arg...) printf(fmt, ##arg)
#else
#define SHA_PRINTF(fmt, arg...)
#endif

typedef enum {
    SHA1_INIT   = BIT(3) | BIT(1),
    SHA1_UPDATE = BIT(3),
    SHA1_FINISH = BIT(3) | BIT(2),
    SHA2_INIT   = BIT(1),
    SHA2_UPDATE = 0,
    SHA2_FINISH = BIT(2),

    SHA_INTR_EN = BIT(0),
} SHA_HWMODE;


#define SHA_NONBLOCK (0x1000)

/********************************
 *
 * LOW LAYER FUNCTION AREA
 *
 * *****************************/
static inline void ll_sha_get_result_big_endian(struct hgsha_v1_hw *hw, uint8 *buf)
{
    volatile uint32_t *tmp = (uint32_t *) & (hw->SHA_RESULT0);
    uint32_t  tmp_dat = 0;

    for (uint32_t i = 0; i < 8; i++, tmp++) {
        tmp_dat = *tmp;
        *buf++  = (tmp_dat    ) & 0xFF;
        *buf++  = (tmp_dat >> 8) & 0xFF;
        *buf++  = (tmp_dat >> 16) & 0xFF;
        *buf++  = (tmp_dat >> 24) & 0xFF;
    }
}

static inline void ll_sha_get_result_little_endian(struct hgsha_v1_hw *hw, uint8 *buf)
{
    volatile uint32_t *tmp = (uint32_t *) & (hw->SHA_RESULT0);
    uint32_t  tmp_dat = 0;

    for (uint32_t i = 0; i < 8; i++, tmp++) {
        tmp_dat = *tmp;
        *buf++  = (tmp_dat >> 24) & 0xFF;
        *buf++  = (tmp_dat >> 16) & 0xFF;
        *buf++  = (tmp_dat >> 8) & 0xFF;
        *buf++  = (tmp_dat    ) & 0xFF;
    }
}

static void ll_sha_calc(struct hgsha_v1_hw *sha, uint8_t *buf, uint16_t len, SHA_HWMODE mode)
{
    sha->SHA_STADDR  = (uint32)buf;
    sha->SHA_BYTELEN = len;
    sha->SHA_PENDING = 1;
    sha->SHA_CONFIG  = mode;
    sha->SHA_START = 1;
//  while(!sha->SHA_PENDING);
}


/********************************
 *
 * DRIVE LAYER FUNCTION AREA
 *
 * *****************************/

static void hgsha_irq_handle(void *args)
{
    struct hgsha_v1 *sha        = (struct hgsha_v1 *)args;
    struct hgsha_v1_hw *sha_reg = (struct hgsha_v1_hw *)sha->hw;

    sha_reg->SHA_PENDING = 1;

    os_sema_up(&sha->done);   
    if (sha->irq_func) {
        sha->irq_func(sha->irq_data);
    }
}

static int32_t hgsha_v1_request_irq(struct sha_dev *dev, void * irq_handle, void *args)
{
    struct hgsha_v1 *sha = (struct hgsha_v1 *)dev;
    sha->irq_func   = args;
    sha->irq_data    = irq_handle;

    return RET_OK;
}

static int hgsha_v1_release_irq(struct sha_dev *dev)
{
    struct hgsha_v1 *sha    = (struct hgsha_v1 *)dev;
    sha->irq_func       = NULL;

    return RET_OK;
}

static int hgsha_v1_transform(struct sha_dev *dev, struct sha_req *req)
{
    struct hgsha_v1 *sha    = (struct hgsha_v1 *)dev;
    struct hgsha_v1_hw *hw = (struct hgsha_v1_hw *)sha->hw;
    int ret = 0;
    uint8_t *buf = req->input;
    uint32_t blocks = req->len / 64;
    uint32_t stas   = 0;
    SHA_HWMODE mode = 0;
    if (os_mutex_lock(&sha->lock, 4000)) {
        printf("sha get mutex failure\r\n");
        return RET_ERR;
    }
    switch (req->type) {
    case T_SHA1:
        stas = 5 * 4;
        mode = SHA1_UPDATE;
        break;
    case T_SHA256:
        stas = 8 * 4;
        mode = SHA2_UPDATE;
        break;
    default:
        return -1;
    }
    mode |= SHA_INTR_EN;

    memcpy((void*)&hw->SHA_RESULT0, req->state, stas);

    sys_dcache_clean_range_unaligned((void*)req->input, req->len);
    os_sema_eat(&sha->done);
    for (int bs = 512;bs>8;bs/=2) {
        for (;blocks>=bs;) {
            ll_sha_calc(hw, buf, bs * 64, mode);
            buf += bs * 64;
            blocks -= bs;
            ret |= !os_sema_down(&sha->done, 8000);
        }
    }

    if (blocks) {
        uint64_t tick = os_jiffies();
        mode &= ~SHA_INTR_EN;
        ll_sha_calc(hw, buf, blocks * 64, mode);
        while(!hw->SHA_PENDING)
		{
			if (os_jiffies()-tick >= 8000) {
				ret = 1;
				break;
			}			
		}

    }

    memcpy(req->state, (void*)&hw->SHA_RESULT0, stas);
    os_mutex_unlock(&sha->lock);
    return ret;
}

int __sha_init(struct sha_dev *dev, struct sha_ctx * ctx)
{
    if (ctx->type == T_SHA1) {
        ctx->state[0] = 0x67452301;
        ctx->state[1] = 0xEFCDAB89;
        ctx->state[2] = 0x98BADCFE;
        ctx->state[3] = 0x10325476;
        ctx->state[4] = 0xC3D2E1F0;
    } else {
        ctx->state[0] = 0x6a09e667;
        ctx->state[1] = 0xbb67ae85;
        ctx->state[2] = 0x3c6ef372;
        ctx->state[3] = 0xa54ff53a;
        ctx->state[4] = 0x510e527f;
        ctx->state[5] = 0x9b05688c;
        ctx->state[6] = 0x1f83d9ab;
        ctx->state[7] = 0x5be0cd19;
    }
    ctx->dlen = 0;
    ctx->bit_len = 0;
    return 0;
}

int __sha_update(struct sha_dev *sha, struct sha_ctx *ctx, uint8_t *buf, uint32_t len)
{
    struct sha_req req = {
        .state = ctx->state,
        .type = ctx->type,
    };

    if (ctx->dlen) {
        if (ctx->dlen + len >= 64) {
            memcpy(&ctx->buf[ctx->dlen], buf, 64 - ctx->dlen);
            buf += 64 - ctx->dlen;
            len -= 64 - ctx->dlen;
            req.input = ctx->buf;
            req.len   = 64;
            hgsha_v1_transform(sha, &req);
            ctx->bit_len += req.len * 8;
            ctx->dlen = 0;
        } else {
            memcpy(&ctx->buf[ctx->dlen], buf, len);
            ctx->dlen += len;
            len = 0;
        }
    }

    for (int i = 512; i != 0; i /= 2) {
        while (len / 64 >= i) {
            req.input = buf;
            req.len   = i * 64;
            hgsha_v1_transform(sha, &req);
            ctx->bit_len += req.len * 8;
            buf += i * 64;
            len -= i * 64;
        }
    }
    if (len) {
        memcpy(ctx->buf, buf, len);
        ctx->dlen = len;
    }
    return 0;
}

int __sha_final(struct sha_dev *sha, struct sha_ctx *ctx, uint8_t *hash)
{
    struct sha_req req = {
        .state = ctx->state,
        .type = ctx->type,
        .input = ctx->buf,
        .len  = 64,
    };
    // Pad whatever data is left in the buffer.
    int i = ctx->dlen;
    if (ctx->dlen < 56) {
        ctx->buf[i++] = 0x80;
        while (i < 56)
            ctx->buf[i++] = 0x00;
    } else {
        ctx->buf[i++] = 0x80;
        while (i < 64)
            ctx->buf[i++] = 0x00;
        hgsha_v1_transform(sha, &req);
        memset(ctx->buf, 0, 56);
    }

    // Append to the padding the total message's length in bits and transform.
    ctx->bit_len += ctx->dlen * 8;
    ctx->buf[63] = ctx->bit_len;
    ctx->buf[62] = ctx->bit_len >> 8;
    ctx->buf[61] = ctx->bit_len >> 16;
    ctx->buf[60] = ctx->bit_len >> 24;
    ctx->buf[59] = ctx->bit_len >> 32;
    ctx->buf[58] = ctx->bit_len >> 40;
    ctx->buf[57] = ctx->bit_len >> 48;
    ctx->buf[56] = ctx->bit_len >> 56;
    hgsha_v1_transform(sha, &req);

    // Since this implementation uses little endian uint8_t ordering and SHA uses big endian,
    // reverse all the bytes when copying the final state to the output hash.
    for (i = 0; i < 4; ++i) {
        hash[i]      = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 4]  = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 8]  = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
        if (ctx->type == T_SHA256) {
            hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
            hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
            hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
        }
    }
    return 0;
}

static int hgsha_v1_init(struct sha_dev *dev, SHA_TYPE type)
{
    struct hgsha_v1 *sha = (struct hgsha_v1 *)dev;
    sha->ctx.type = type;
    sha->ctx.bit_len = 0;
    sha->ctx.dlen    = 0;
    __sha_init(dev, sha->pctx);
    return 0;
}

static int32 hgsha_v1_update(struct sha_dev *dev, uint8 *input, uint32 len)
{
    struct hgsha_v1 *sha = (struct hgsha_v1 *)dev;
    return __sha_update(dev, sha->pctx, input, len);
}

static int32 hgsha_v1_final(struct sha_dev *dev, uint8 *output)
{
    struct hgsha_v1 *sha = (struct hgsha_v1 *)dev;
    return __sha_final(dev, sha->pctx, output);
}

static int32 hgsha_v1_ioctl(struct sha_dev *dev, uint32 cmd, uint32 param1, uint32 param2)
{
    struct hgsha_v1    *sha        = (struct hgsha_v1 *)dev;

    switch (cmd) {
    case SHA_IOCTL_CMD_START:
        os_mutex_lock(&sha->lock, osWaitForever);
        break;
    case SHA_IOCTL_CMD_END:
        os_mutex_unlock(&sha->lock);
        break;
    case SHA_IOCTL_CMD_RESET:
        break;
    case SHA_IOCTL_GET_CTX:
        *((struct sha_ctx*)param1) = sha->ctx;
        break;
    case SHA_IOCTL_PUT_CTX:
        sha->ctx = *((struct sha_ctx*)param1);
        break;
    case SHA_IOCTL_RELOAD_CTX:
        sha->pctx = (struct sha_ctx*)param1;
        break;
    default:
        return RET_ERR;
    }
    return RET_OK;
}

static const struct sha_hal_ops sha_v1_ops = {
    .init              = hgsha_v1_init,
    .update            = hgsha_v1_update,
    .final             = hgsha_v1_final,
    .xform             = hgsha_v1_transform,
    .ioctl             = hgsha_v1_ioctl,
    .requset_irq       = hgsha_v1_request_irq,
    .release_irq       = hgsha_v1_release_irq,
};

__init int32_t hgsha_v1_attach(uint32_t dev_id, struct hgsha_v1 *sha)
{
    sha->dev.dev.ops = (const struct devobj_ops *)&sha_v1_ops;
    struct hgsha_v1_hw *hw = (struct hgsha_v1_hw *)sha->hw;

    //clear pending
    hw->SHA_PENDING |= 1;
    sha->irq_func = NULL;
    sha->flags = 0;
    sha->pctx  = &sha->ctx;

    os_mutex_init(&sha->lock);
    os_sema_init(&sha->done, 0);

    request_irq(sha->irq_num, hgsha_irq_handle, sha);
    irq_enable(sha->irq_num);

    dev_register(dev_id, (struct dev_obj *)sha);
    return RET_OK;
}


