/*
 *  FIPS-180-1 compliant SHA-1 implementation
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
 *  The SHA-1 standard was published by NIST in 1993.
 *
 *  http://www.itl.nist.gov/fipspubs/fip180-1.htm
 */

#include "../common.h"

#if defined(MBEDTLS_SHA1_C)

#include "mbedtls/sha1.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/error.h"

#include <string.h>

#include "mbedtls/platform.h"

#if defined(MBEDTLS_SHA1_ALT)

static struct sha_dev *sha = NULL;
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

int mbedtls_internal_sha1_process_many(mbedtls_sha1_context *ctx,
                                    const uint8_t *input,
                                    uint32_t ilen)
                                    
{
    if (!sha) {
        sha = (struct sha_dev *)dev_get(HG_SHA_DEVID);
    }
    struct sha_req req = {
        .type = T_SHA1,
        .len = ilen & ~(64-1),
        .state = ctx->state,
        .input = (void *)input,
    };
    
    sha_xform(sha, &req);
    return req.len;
}

void mbedtls_sha1_init(mbedtls_sha1_context *ctx)
{
    memset(ctx, 0, sizeof(mbedtls_sha1_context));
}

void mbedtls_sha1_free(mbedtls_sha1_context *ctx)
{
    if (ctx == NULL) {
        return;
    }

    mbedtls_platform_zeroize(ctx, sizeof(mbedtls_sha1_context));
}

void mbedtls_sha1_clone(mbedtls_sha1_context *dst,
                        const mbedtls_sha1_context *src)
{
    *dst = *src;
}

/*
 * SHA-1 context setup
 */
int mbedtls_sha1_starts(mbedtls_sha1_context *ctx)
{
    ctx->total[0] = 0;
    ctx->total[1] = 0;

    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;

    return 0;
}

/*
 * SHA-1 process buffer
 */
int mbedtls_sha1_update(mbedtls_sha1_context *ctx,
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
    fill = 64 - left;

    ctx->total[0] += (uint32_t) ilen;
    ctx->total[0] &= 0xFFFFFFFF;

    if (ctx->total[0] < (uint32_t) ilen) {
        ctx->total[1]++;
    }

    if (left && ilen >= fill) {
        memcpy((void *) (ctx->buffer + left), input, fill);

        if ((ret = mbedtls_internal_sha1_process(ctx, ctx->buffer)) != 0) {
            return ret;
        }

        input += fill;
        ilen  -= fill;
        left = 0;
    }
    
    size_t processed = mbedtls_internal_sha1_process_many(ctx, input, ilen);
    input += processed;
    ilen  -= processed;

    if (ilen > 0) {
        memcpy((void *) (ctx->buffer + left), input, ilen);
    }

    return 0;
}

/*
 * SHA-1 final digest
 */
int mbedtls_sha1_finish(mbedtls_sha1_context *ctx,
                        unsigned char output[20])
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
        memset(ctx->buffer + used, 0, 64 - used);

        if ((ret = mbedtls_internal_sha1_process(ctx, ctx->buffer)) != 0) {
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

    if ((ret = mbedtls_internal_sha1_process(ctx, ctx->buffer)) != 0) {
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

    return 0;
}

#endif /* !MBEDTLS_SHA1_ALT */

#endif /* MBEDTLS_SHA1_C */
