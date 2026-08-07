#ifndef MBEDTLS_SHA256_ALT_H
#define MBEDTLS_SHA256_ALT_H

#if defined(MBEDTLS_SHA1_ALT)

#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"

#include "mbedtls/private_access.h"
#include "mbedtls/build_info.h"
#include "mbedtls/platform_util.h"
#include "hal/sha.h"

typedef struct mbedtls_sha1_context {
    uint32_t MBEDTLS_PRIVATE(total)[2];          /*!< The number of Bytes processed.  */
    uint32_t MBEDTLS_PRIVATE(state)[5];          /*!< The intermediate digest state.  */
    unsigned char MBEDTLS_PRIVATE(buffer)[64];   /*!< The data block being processed. */
}
mbedtls_sha1_context;


#endif /* MBEDTLS_SHA1_ALT */

#endif /* MBEDTLS_SHA1_ALT_H */