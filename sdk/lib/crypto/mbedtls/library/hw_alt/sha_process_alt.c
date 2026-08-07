#include "common.h"

#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"

#include <string.h>
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/error.h"
#include "mbedtls/platform.h"

#include "hal/sha.h"


static struct sha_dev *sha = NULL;

#if defined(MBEDTLS_SHA256_C) || defined(MBEDTLS_SHA224_C)

#if defined(MBEDTLS_SHA256_PROCESS_ALT)

int mbedtls_internal_sha256_process(mbedtls_sha256_context *ctx,
                                    const unsigned char data[64])
{
    if (!sha) {
        sha = (struct sha_dev *)dev_get(HG_SHA_DEVID);
    }
    struct sha_req req = {
        .type = T_SHA256,
        .len = 64,
        .state = ctx->state,
        .input = (void *)data,
    };
    
    sha_xform(sha, &req);
    return 0;
}

#if 1
int mbedtls_internal_sha256_process_many(mbedtls_sha256_context *ctx,
                                    uint8_t *input,
                                    uint32_t ilen)
                                    
{
    if (!sha) {
        sha = (struct sha_dev *)dev_get(HG_SHA_DEVID);
    }
    struct sha_req req = {
        .type = T_SHA256,
        .len = ilen & ~(64-1),
        .state = ctx->state,
        .input = (void *)input,
    };
    
    sha_xform(sha, &req);
    return req.len;
}
#endif

#endif //defined(MBEDTLS_SHA256_ALT)
#endif //defined(MBEDTLS_SHA256_C) || defined(MBEDTLS_SHA224_C)


#if defined(MBEDTLS_SHA1_C) 
#if defined(MBEDTLS_SHA1_PROCESS_ALT)
int mbedtls_internal_sha1_process(mbedtls_sha1_context *ctx,
                                    const unsigned char data[64])
{
    if (!sha) {
        sha = (struct sha_dev *)dev_get(HG_SHA_DEVID);
    }
        struct sha_req req = {
        .type = T_SHA1,
        .len = 64,
        .state = ctx->state,
        .input = (void *)data,
    };
    
    sha_xform(sha, &req);
    return 0;
}

#endif //defined(MBEDTLS_SHA1_PROCESS_ALT)
#endif //defined(MBEDTLS_SHA1_C) 

