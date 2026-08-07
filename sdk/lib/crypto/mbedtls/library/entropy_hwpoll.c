
#include "mbedtls/entropy.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen)
{
    int i;
    int r;
    char *ptr;

    ((void)data);
    if (NULL == output){
        return -1;
    }

    ptr = (char *)output;
    srand(time(NULL));

    for (i = 0; i < len; i += sizeof(int))
    {
        r = rand();
        (void) memcpy(ptr + i, &r, sizeof(int) > (unsigned int)(len - i)? (unsigned int)(len - i): sizeof(int));
    }

    *olen = len;
    return 0;
}
