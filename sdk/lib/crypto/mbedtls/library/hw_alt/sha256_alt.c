#include "common.h"

#if defined(MBEDTLS_SHA256_C) || defined(MBEDTLS_SHA224_C)

#if defined(MBEDTLS_SHA256_ALT)

#include <string.h>
#include "sha256_alt.h"
#include "mbedtls/sha256.h"
#include "mbedtls/error.h"
#include "mbedtls/platform.h"


#include "osal/task.h"
#include "osal/mutex.h"


#define SHA256_HW_VERSION 2



void mbedtls_sha256_init(mbedtls_sha256_context *ctx)
{
    static struct sha_dev *sha = NULL;
    if (!sha) {
        sha = (struct sha_dev *)dev_get(HG_SHA_DEVID);
    }
    memset(ctx, 0, sizeof(mbedtls_sha256_context));
    ctx->hw = sha;
}

void mbedtls_sha256_free(mbedtls_sha256_context *ctx)
{
    if (ctx == NULL) {
        return;
    }

    mbedtls_platform_zeroize(ctx, sizeof(mbedtls_sha256_context));
}

void mbedtls_sha256_clone(mbedtls_sha256_context *dst,
                          const mbedtls_sha256_context *src)
{
    *dst = *src;
}

/*
 * SHA-256 context setup
 */
int mbedtls_sha256_starts(mbedtls_sha256_context *ctx, int is224)
{
    sha_ioctl(ctx->hw, SHA_IOCTL_RELOAD_CTX, (uint32_t)&ctx->ctx);
    sha_init(ctx->hw, T_SHA256);
    return 0;
}

/*
 * SHA-256 process buffer
 */
int mbedtls_sha256_update(mbedtls_sha256_context *ctx,
                          const unsigned char *input,
                          size_t ilen)
{
   sha_ioctl(ctx->hw, SHA_IOCTL_RELOAD_CTX, (uint32_t)&ctx->ctx);
    sha_update(ctx->hw, input, ilen);
    return 0;
}

/*
 * SHA-256 final digest
 */
int mbedtls_sha256_finish(mbedtls_sha256_context *ctx,
                          unsigned char *output)
{
    sha_ioctl(ctx->hw, SHA_IOCTL_RELOAD_CTX, (uint32_t)&ctx->ctx);
    sha_final(ctx->hw, output);
    return 0;
}

#endif /* MBEDTLS_SHA256_ALT_H */

#endif /* MBEDTLS_SHA256_C || MBEDTLS_SHA224_C */