#include "lib/net/urlfile/urlfile.h"
#include "curl/curl.h"

#ifndef _UF_UTIL_H__
#define _UF_UTIL_H__

struct uf_curl_ops{
    curl_read_callback rd;
    curl_write_callback wr;
    curl_progress_callback gres;
    curl_debug_callback dbg;
};

char *uf_strstr(char *s1, int len, char *s2);
void uf_strstrip(char *str);
char *uf_playlist_get_line(char *buf, char **line);

int uf_curl_debug(CURL *curl, curl_infotype itype, char *pData, size_t size, void *userdata);
int32 uf_curl_init(struct urlfile *file, CURL *curl, const struct uf_curl_ops *ops);

#endif // _UF_UTIL_H__

