#include <stdio.h>
#include <stdlib.h>
#include <openssl/crypto.h>
#include "cryptlib.h"



void *CRYPTO_realloc_clean(void *str, int old_len, int num, const char *file,
                           int line)
{
    void *ret = NULL;

    if (str == NULL)
        return malloc(num);

    if (num <= 0)
        return NULL;

    /*
     * We don't support shrinking the buffer. Note the memcpy that copies
     * |old_len| bytes to the new buffer, below.
     */
    if (num < old_len)
        return NULL;

	
    ret = malloc(num);
    if (ret) {
        memcpy(ret, str, old_len);
        OPENSSL_cleanse(str, old_len);
        free(str);
    }

    return ret;
}

