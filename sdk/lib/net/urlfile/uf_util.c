#include "uf_util.h"

#define WHITES " \n\r\t"

char *uf_strstr(char *s1, int len, char *s2)
{
    char *tmp = s1;
    while (tmp < s1 + len) {
        if (*tmp == *s2 && os_memcmp(tmp, s2, os_strlen(s2)) == 0) {
            return tmp;
        }
        tmp++;
    }
    return NULL;
}

void uf_strstrip(char *str)
{
    char *i;

    if (str == NULL) {
        return;
    }
    for (i = str ; i[0] != '\0' && strchr(WHITES, i[0]) != NULL; i++) {
        /* NOTHING */;
    }
    if (i[0] != '\0') {
        os_memmove(str, i, os_strlen(i) + 1);
        for (i = str + os_strlen(str) - 1 ; strchr(WHITES, i[0]) != NULL; i--) {
            /* NOTHING */;
        }
        i[1] = '\0';
    } else {
        str[0] = '\0';
    }
}

char *uf_strdup(char *s)
{
    char *p = NULL;
    if (s) {
        p = uf_alloc(os_strlen(s) + 1);
        if (p) {
            os_strcpy(p, s);
        }
    }
    return p;
}

int uf_curl_debug(CURL *curl, curl_infotype itype, char *pData, size_t size, void *userdata)
{
    if (itype == CURLINFO_TEXT) {
        //UF_DEBUG("[TEXT]%s\n", pData);
    } else if (itype == CURLINFO_HEADER_IN) {
        //UF_DEBUG("[HEADER_IN]%s\n", pData);
    }
    /*
    else if (itype == CURLINFO_HEADER_OUT)
    {
        UF_DEBUG("[HEADER_OUT]%s\n", pData);
    }
    else if (itype == CURLINFO_DATA_IN)
    {
        UF_DEBUG("[DATA_IN]%s\n", pData);
    }
    else if (itype == CURLINFO_DATA_OUT)
    {
        UF_DEBUG("[DATA_OUT]%s\n", pData);
    }
    */
    return 0;
}


int32 uf_curl_init(struct urlfile *file, CURL *curl, const struct uf_curl_ops *ops)
{
    int ret = CURLE_OK;

    if (file == NULL || curl == NULL) {
        return 1;
    }

    /*set common parameters*/
    curl_easy_reset(curl);
    ret |= curl_easy_setopt(curl, CURLOPT_URL, file->url);
    ret |= curl_easy_setopt(curl, CURLOPT_FILE, file);
    ret |= curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    ret |= curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, ops->gres);
    ret |= curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, (void *)file);
    ret |= curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, ops->dbg);
    ret |= curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    ret |= curl_easy_setopt(curl, CURLOPT_READFUNCTION, ops->rd);
    ret |= curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ops->wr);
    return ret;
}

