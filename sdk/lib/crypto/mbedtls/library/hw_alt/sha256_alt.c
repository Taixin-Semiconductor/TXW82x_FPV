/*
 *  FIPS-180-2 compliant SHA-256 implementation
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*
 *  The SHA-256 Secure Hash Standard was published by NIST in 2002.
 *
 *  http://csrc.nist.gov/publications/fips/fips180-2/fips180-2.pdf
 */

#include "../common.h"

#if defined(MBEDTLS_SHA256_C)

#include "mbedtls/sha256.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/error.h"

#include <string.h>

#include "mbedtls/platform.h"


#if defined(MBEDTLS_SHA256_ALT)

#define SHA256_BLOCK_SIZE 64
static struct sha_dev *sha = NULL;
void mbedtls_sha256_init(mbedtls_sha256_context *ctx)
{
    memset(ctx, 0, sizeof(mbedtls_sha256_context));
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
#if defined(MBEDTLS_SHA224_C) && defined(MBEDTLS_SHA256_C)
    if (is224 != 0 && is224 != 1) {
        return MBEDTLS_ERR_SHA256_BAD_INPUT_DATA;
    }
#elif defined(MBEDTLS_SHA256_C)
    if (is224 != 0) {
        return MBEDTLS_ERR_SHA256_BAD_INPUT_DATA;
    }
#else /* defined MBEDTLS_SHA224_C only */
    if (is224 == 0) {
        return MBEDTLS_ERR_SHA256_BAD_INPUT_DATA;
    }
#endif

    ctx->total[0] = 0;
    ctx->total[1] = 0;

    if (is224 == 0) {
#if defined(MBEDTLS_SHA256_C)
        ctx->state[0] = 0x6A09E667;
        ctx->state[1] = 0xBB67AE85;
        ctx->state[2] = 0x3C6EF372;
        ctx->state[3] = 0xA54FF53A;
        ctx->state[4] = 0x510E527F;
        ctx->state[5] = 0x9B05688C;
        ctx->state[6] = 0x1F83D9AB;
        ctx->state[7] = 0x5BE0CD19;
#endif
    } else {
#if defined(MBEDTLS_SHA224_C)
        ctx->state[0] = 0xC1059ED8;
        ctx->state[1] = 0x367CD507;
        ctx->state[2] = 0x3070DD17;
        ctx->state[3] = 0xF70E5939;
        ctx->state[4] = 0xFFC00B31;
        ctx->state[5] = 0x68581511;
        ctx->state[6] = 0x64F98FA7;
        ctx->state[7] = 0xBEFA4FA4;
#endif
    }

#if defined(MBEDTLS_SHA224_C)
    ctx->is224 = is224;
#endif

    return 0;
}

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

int mbedtls_internal_sha256_process_many(mbedtls_sha256_context *ctx,
                                    const unsigned char *input,
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

/*
 * SHA-256 process buffer
 */
int mbedtls_sha256_update(mbedtls_sha256_context *ctx,
                          const unsigned char *input,
                          size_t ilen)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t fill;
    uint32_t left;

    if (ilen == 0) {
        return 0;
    }

    left = ctx->total[0] & 0x3F;
    fill = SHA256_BLOCK_SIZE - left;

    ctx->total[0] += (uint32_t) ilen;
    ctx->total[0] &= 0xFFFFFFFF;

    if (ctx->total[0] < (uint32_t) ilen) {
        ctx->total[1]++;
    }

    if (left && ilen >= fill) {
        memcpy((void *) (ctx->buffer + left), input, fill);

        if ((ret = mbedtls_internal_sha256_process(ctx, ctx->buffer)) != 0) {
            return ret;
        }

        input += fill;
        ilen  -= fill;
        left = 0;
    }

    while (ilen >= SHA256_BLOCK_SIZE) {
        size_t processed =
            mbedtls_internal_sha256_process_many(ctx, input, ilen);
        if (processed < SHA256_BLOCK_SIZE) {
            return MBEDTLS_ERR_ERROR_GENERIC_ERROR;
        }

        input += processed;
        ilen  -= processed;
    }

    if (ilen > 0) {
        memcpy((void *) (ctx->buffer + left), input, ilen);
    }

    return 0;
}

/*
 * SHA-256 final digest
 */
int mbedtls_sha256_finish(mbedtls_sha256_context *ctx,
                          unsigned char *output)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    uint32_t used;
    uint32_t high, low;

    /*
     * Add padding: 0x80 then 0x00 until 8 bytes remain for the length
     */
    used = ctx->total[0] & 0x3F;

    ctx->buffer[used++] = 0x80;

    if (used <= 56) {
        /* Enough room for padding + length in current block */
        memset(ctx->buffer + used, 0, 56 - used);
    } else {
        /* We'll need an extra block */
        memset(ctx->buffer + used, 0, SHA256_BLOCK_SIZE - used);

        if ((ret = mbedtls_internal_sha256_process(ctx, ctx->buffer)) != 0) {
            return ret;
        }

        memset(ctx->buffer, 0, 56);
    }

    /*
     * Add message length
     */
    high = (ctx->total[0] >> 29)
           | (ctx->total[1] <<  3);
    low  = (ctx->total[0] <<  3);

    MBEDTLS_PUT_UINT32_BE(high, ctx->buffer, 56);
    MBEDTLS_PUT_UINT32_BE(low,  ctx->buffer, 60);

    if ((ret = mbedtls_internal_sha256_process(ctx, ctx->buffer)) != 0) {
        return ret;
    }

    /*
     * Output final state
     */
    MBEDTLS_PUT_UINT32_BE(ctx->state[0], output,  0);
    MBEDTLS_PUT_UINT32_BE(ctx->state[1], output,  4);
    MBEDTLS_PUT_UINT32_BE(ctx->state[2], output,  8);
    MBEDTLS_PUT_UINT32_BE(ctx->state[3], output, 12);
    MBEDTLS_PUT_UINT32_BE(ctx->state[4], output, 16);
    MBEDTLS_PUT_UINT32_BE(ctx->state[5], output, 20);
    MBEDTLS_PUT_UINT32_BE(ctx->state[6], output, 24);

    int truncated = 0;
#if defined(MBEDTLS_SHA224_C)
    truncated = ctx->is224;
#endif
    if (!truncated) {
        MBEDTLS_PUT_UINT32_BE(ctx->state[7], output, 28);
    }

    return 0;
}

#endif /* !MBEDTLS_SHA256_ALT */

#endif /* MBEDTLS_SHA256_C */
