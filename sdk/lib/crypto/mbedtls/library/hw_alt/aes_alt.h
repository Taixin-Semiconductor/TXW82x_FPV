#ifndef MBEDTLS_AES_ALT_H
#define MBEDTLS_AES_ALT_H

#if defined(MBEDTLS_AES_ALT)

#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"

#include "mbedtls/private_access.h"
#include "mbedtls/build_info.h"
#include "mbedtls/platform_util.h"
#include "hal/sysaes.h"

typedef struct mbedtls_aes_context {
    struct sysaes_dev *MBEDTLS_PRIVATE(hw);
    struct sysaes_para MBEDTLS_PRIVATE(para);
}
mbedtls_aes_context;

#if defined(MBEDTLS_CIPHER_MODE_XTS)
/**
 * \brief The AES XTS context-type definition.
 */
typedef struct mbedtls_aes_xts_context {
    mbedtls_aes_context MBEDTLS_PRIVATE(crypt); /*!< The AES context to use for AES block
                                                   encryption or decryption. */
    mbedtls_aes_context MBEDTLS_PRIVATE(tweak); /*!< The AES context used for tweak
                                                   computation. */
} mbedtls_aes_xts_context;
#endif /* MBEDTLS_CIPHER_MODE_XTS */

#endif /* MBEDTLS_AES_ALT */

#endif /* MBEDTLS_AES_ALT_H */