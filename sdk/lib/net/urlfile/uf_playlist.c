#include "lib/net/urlfile/urlfile.h"
#include "uf_util.h"

static uint8 _uf_is_asx_playlist(char *data, int len)
{
    int i = 0;
    char *p = data;

    for (i = 0; i < len; i++) {
        if (*p == '<') {
            break;
        }
        p++;
    }

    if (i == len) {
        return 0;
    }

    p++;
    i++;

    for (; i < len; i++) {
        if (*p != ' ') {
            break;
        }
        p++;
    }

    if (((*p == 'A') || (*p == 'a')) && ((*(p + 1) == 'S') || (*(p + 1) == 's')) &&
        ((*(p + 2) == 'X') || (*(p + 2) == 'x'))) {
        return 1;
    }

    return 0;
}

static uint8 _uf_is_mms_playlist(char *data, int len)
{
    if (uf_strstr(data, len, "[Reference]") && uf_strstr(data, len, "asf")) {
        return 1;
    }
    return 0;
}

static uint8 _uf_is_xspf_playlist(char *data, int len)
{
    if (uf_strstr(data, len, "xspf")) {
        return 1;
    }
    return 0;
}

static uint8 _uf_is_pls_playlist(char *data, int len)
{
    if (uf_strstr(data, len, "[playlist]")) {
        return 1;
    }
    return 0;
}

static uint8 _uf_is_smil_playlist(char *data, int len)
{
    char *ptr = NULL;

    ptr = uf_strstr(data, len, "<?xml");
    if (ptr == NULL) { ptr = uf_strstr(data, len, "<!DOCTYPE smil"); }

    if (ptr) {
        if (uf_strstr(data, len, "<smil") ||
            uf_strstr(data, len, "<?wpl") ||
            uf_strstr(data, len, "(smil-document")) {
            return 1;
        }
    }
    return 0;
}

static uint8 _uf_is_nsc_playlist(char *data, int len)
{
    if (uf_strstr(data, len, "[Address]")) {
        return 1;
    }
    return 0;
}

static uint8 _uf_is_m3u_playlist(char *data, int len)
{
    if (os_strncmp(data, "#EXTM3U", 7)) {
        return 0;
    }

    if (uf_strstr(data, len, "#EXT-X-STREAM-INF:") ||
        uf_strstr(data, len, "#EXT-X-TARGETDURATION:") ||
        uf_strstr(data, len, "#EXT-X-MEDIA-SEQUENCE:")) {
        return 0;
    }

    return 1;
}

static uint8 _uf_is_hls_playlist(char *data, int len)
{
    /* Require #EXTM3U at the start, and either one of the ones below somewhere for a proper match. */
    if (os_strncmp(data, "#EXTM3U", 7)) {
        return 0;
    }
    if (uf_strstr(data, len, "#EXT-X-STREAM-INF:") ||
        uf_strstr(data, len, "#EXT-X-TARGETDURATION:") ||
        uf_strstr(data, len, "#EXT-X-MEDIA-SEQUENCE:")) {
        return 1;
    }

    return 0;
}

int uf_playlist_recieve(struct uf_playlist *list, char *data, int len, uint32 tot_size)
{
    if (data == NULL || len == 0 || list == NULL) {
        return 0;
    }

    if (list->buff_addr == NULL) {
        list->buff_size = tot_size + 1;
        list->buff_addr = uf_alloc(list->buff_size);
        if (list->buff_addr == NULL) {
            UF_ERR("no memory, tot_size:%d\r\n", tot_size);
            return -1;
        }
    }

    ASSERT(list->buff_len + len < list->buff_size);
    os_memcpy(list->buff_addr + list->buff_len, data, len);
    list->buff_len += len;
    list->buff_addr[list->buff_len] = 0;
    return len;
}

void uf_playlist_detect(struct uf_playlist *list, void *data, size_t size)
{
    if (_uf_is_hls_playlist(data, size)) {
        list->type = UF_PLAYLIST_HLS;
    } else if (_uf_is_m3u_playlist(data, size)) {
        list->type = UF_PLAYLIST_M3U;
    } else if (_uf_is_pls_playlist(data, size)) {
        list->type = UF_PLAYLIST_PLS;
    } else if (_uf_is_nsc_playlist(data, size)) {
        list->type = UF_PLAYLIST_NSC;
    } else if (_uf_is_smil_playlist(data, size)) {
        list->type = UF_PLAYLIST_SMIL;
    } else if (_uf_is_asx_playlist(data, size)) {
        list->type = UF_PLAYLIST_ASX;
    } else if (_uf_is_xspf_playlist(data, size)) {
        list->type = UF_PLAYLIST_XSPF;
    } else if (_uf_is_mms_playlist(data, size)) {
        list->type = UF_PLAYLIST_MMSREF;
    } else {
        list->type = -1;
    }
}

int32 uf_playlist_build(struct uf_playlist *list, char *urls)
{
    char *ptr = urls;
    char *url = NULL;

    list->type = UF_PLAYLIST_CUST;
    while (ptr) {
        ptr = uf_playlist_get_line(ptr, &url);
        if (uf_playlist_new_url(list, NULL, url)) {
            return RET_ERR;
        }
    }
    return RET_OK;
}

int32 uf_playlist_free(struct uf_playlist *list)
{
    struct uf_playlist_entry *pos, *n;

    list_for_each_entry_safe(pos, n, &list->list, list) {
        uf_free(pos->title);
        uf_free(pos->url);
        uf_free(pos);
    }

    list->count = 0;
    list->buff_size = 0;
    uf_free(list->buff_addr);
    list->buff_addr = NULL;
    return RET_OK;
}

int32 uf_playlist_new_url(struct uf_playlist *list, char *title, char *url)
{
    struct uf_playlist_entry *entry = uf_alloc(sizeof(struct uf_playlist_entry));
    if (entry) {
        os_memset(entry, 0, sizeof(struct uf_playlist_entry));
        entry->url = url;
        entry->title = uf_strdup(title);
        list_add_tail(&entry->list, &list->list);
        list->count++;
        return RET_OK;
    }
    UF_ERR("no memory\r\n");
    return -ENOMEM;
}

char *uf_playlist_get_url(struct uf_playlist *list, uint16 index)
{
    uint16 i = 0;
    struct uf_playlist_entry *pos;

    if (index > list->count) {
        return NULL;
    }

    list_for_each_entry(pos, &list->list, list) {
        if (++i == index) {
            UF_DEBUG("get url %d [%s]\r\n", index, pos->url);
            return pos->url;
        }
    }
    return NULL;
}

char *uf_playlist_get_line(char *buf, char **line)
{
    int32 len = 0;
    char *ptr = NULL;
    char *url = NULL;

    *line = NULL;
    if (buf == NULL || buf[0] == '\0') {
        return NULL;
    }

    ptr = buf;
    while (*buf) {
        if(*buf == '\r') { buf++; }
        else if(*buf == '\n') { buf++; break; } 
        else if(*buf == ';')  { buf++; break; } 
        else { buf++; len++; }
    }

    if(len == 0) return NULL;

    url = uf_alloc(len + 1);
    if(url){
        *line = url;
        os_memcpy(url, ptr, len); url[len] = 0;
    }

    return *buf ? buf : NULL;
}

