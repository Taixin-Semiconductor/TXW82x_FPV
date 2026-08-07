#ifndef MBEDTLS_SHA256_ALT_H
#define MBEDTLS_SHA256_ALT_H

#if defined(MBEDTLS_SHA256_ALT)

#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"

#include "mbedtls/private_access.h"
#include "mbedtls/build_info.h"
#include "mbedtls/platform_util.h"
#include "hal/sha.h"



typedef struct mbedtls_sha256_context {
    struct sha_dev *MBEDTLS_PRIVATE(hw);
    struct sha_ctx ctx;
}
mbedtls_sha256_context;


#endif /* MBEDTLS_SHA256_ALT */

#endif /* MBEDTLS_SHA256_ALT_H */