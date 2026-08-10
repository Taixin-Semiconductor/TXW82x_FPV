#include "uf_http.h"

static size_t uf_curl_http_header(char *buffer, size_t size, size_t nitems, void *outstream)
{
    long  http_code = 0;
    struct uf_httppriv *priv = NULL;
    struct urlfile *file = (struct urlfile *)outstream;

    if (file && file->priv) {
        priv = (struct uf_httppriv *)file->priv;
        if (uf_strstr(buffer, size * nitems, "Accept-Ranges: bytes")) {
            priv->seek_type = 1;
        }
    }

    curl_easy_getinfo(priv->curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code >= 400) { //response error
        UF_DEBUG("HTTP Response Error Code:%d\n", http_code);
        return 0;
    }

    return size * nitems;
}

static size_t uf_curl_http_read(char *buffer, size_t size, size_t nitems, void *instream)
{
    uint32 left;
    uint32 count = size * nitems;
    struct urlfile *file = (struct urlfile *)instream;
    struct uf_httppriv *priv = (struct uf_httppriv *)file->priv;

    if (priv->send_size == 0xffffffff || priv->uplen == priv->upsize) {
        return 0;
    }

    while (priv->send_size == priv->send_len && file->state != UF_RUN) {
        if (os_event_wait(&priv->send_sig, BIT(0), NULL, OS_EVENT_WMODE_AND | OS_EVENT_WMODE_CLEAR, 100) == RET_OK) {
            break;
        }
    }

    if (file->state != UF_RUN || priv->send_size == 0xffffffff) {
        return 0;
    }

    left = priv->send_size - priv->send_len;
    if (count >= left) {
        count = left;
    }

    os_memcpy(buffer, priv->send_data + priv->send_len, count);
    priv->send_len += count;

    if (priv->send_len == priv->send_size) {
        os_event_set(&priv->send_sig, BIT(1), NULL);
    }

    return count;
}

static size_t uf_curl_http_write(char *buffer, size_t size, size_t nitems, void *outstream)
{
    double content_length;
    size_t count = size * nitems;
    struct urlfile *file = (struct urlfile *)outstream;
    struct uf_httppriv *priv = (struct uf_httppriv *)file->priv;

    if (file == NULL) {
        return 0;
    }

    priv->lifetime = os_jiffies();
    if (file->size == 0) {
        if (file->playlist.count) {
            file->size = -1;
        } else {
            curl_easy_getinfo(priv->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);
            if (content_length == 0) {
                file->size = -1;
            } else {
                file->size = content_length;
            }
        }
    }

    if (!UF_AVMODE(file)) {
        file->playlist.type = -1;
    }

    if (file->playlist.type == 0) {
        uf_playlist_detect(&file->playlist, buffer, count);
    }

    if (file->playlist.type > 0 && file->playlist.count == 0) {
        count = uf_playlist_recieve(&file->playlist, buffer, count, file->size);
        if (count > 0) {
            priv->range_start += count;
        }
        return count;
    }

    //add ICY support
//    if (os_memcmp(buffer, "ICY 200 OK", 10) == 0) {
//        return uf_switch_2_http_icy(file, buffer, size);
//    }

    count = uf_store_data(file, buffer, count);
    if (count > 0 && file->size > 0) {
        priv->range_start += count;
    }
    return count;
}

static int uf_curl_http_progcb(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
    int ret = 0;
    struct urlfile *file = (struct urlfile *)clientp;
    struct uf_httppriv *priv = (struct uf_httppriv *)file->priv;

    if (UF_AVMODE(file) && os_jiffies() - priv->lifetime > 5 * 1000 &&
        file->state == UF_RUN && RB_COUNT(&file->buffer) == 0) {
        UF_DEBUG("no data during 5 sec! retry! [%llu, %llu]\n", file->doffset, file->size);
        return 1;
    }

    ret = (file->state < UF_NONE);
    return ret;
}

static const struct uf_curl_ops http_curl_ops = {
    .rd   = uf_curl_http_read,
    .wr   = uf_curl_http_write,
    .gres = uf_curl_http_progcb,
    .dbg  = uf_curl_debug,
};

static int uf_curl_http_parse_list(struct urlfile *uf)
{
    //struct uf_httppriv *priv = (struct uf_httppriv *)uf->priv;

    if (UF_READ(uf) && uf->playlist.type > 0 && uf->playlist.count == 0) {
        switch (uf->playlist.type) {
            case UF_PLAYLIST_ASX:
                //asx_parser_build_tree(uf);
                break;
            case UF_PLAYLIST_MMSREF:
                //mms_parser_build_tree(uf);
                break;
            case UF_PLAYLIST_M3U:
                //uf_m3u_parser_build_tree(uf);
                break;
            case UF_PLAYLIST_PLS:
                //uf_pls_parser_build_tree(uf);
                break;
            case UF_PLAYLIST_NSC:
                break;
            case UF_PLAYLIST_SMIL:
                //smil_parser_build_tree(uf);
                break;
            case UF_PLAYLIST_XSPF:
                //xspf_parser_build_tree(uf);
                break;
            default:
                break;
        }
    }
    return 0;
}

static int uf_curl_http_next_url(struct urlfile *file)
{
    struct uf_httppriv *priv = (struct uf_httppriv *)file->priv;

    if (UF_READ(file) && file->playindex < file->playlist.count) {
        file->playindex++;
        priv->range_start = 0;
        uf_free(file->url);
        file->url = uf_strdup(uf_playlist_get_url(&file->playlist, file->playindex));
        if (file->url) {
            uf_state(file, UF_ROPEN); //open next url.
            return 1;
        }
    }
    return 0;
}

static int32 uf_curl_http_reopen(struct urlfile *file)
{
    struct uf_httppriv *priv = (struct uf_httppriv *)file->priv;
    if (UF_READ(file) && file->state == UF_RUN && priv->break_cont &&
        (file->size == -1/*livestream*/ || file->doffset < file->size/*断点续传*/)) {
        uf_state(file, UF_ROPEN);
        return 1;
    }
    return 0;
}

static int uf_curl_http_init(struct urlfile *file)
{
    char httprange[32];
    struct uf_httppriv *priv = (struct uf_httppriv *)file->priv;

    ASSERT(!(file->mode & UF_MODE_MONITOR));

    if (priv == NULL) {
        priv = uf_alloc(sizeof(struct uf_httppriv));
        if (priv == NULL) {
            uf_state(file, UF_ERROR);
            return -ENOMEM;
        }

        os_memset(priv, 0, sizeof(struct uf_httppriv));
        priv->curl = curl_easy_init();
        if (priv->curl == NULL) {
            uf_free(priv);
            uf_state(file, UF_ERROR);
            return -ENOMEM;
        }

        priv->break_cont = UF_AVMODE(file);
        priv->headerlist = curl_slist_append(priv->headerlist, "Icy-MetaData: 0");
        priv->lifetime = os_jiffies();
        file->priv = priv;
        os_event_init(&priv->send_sig);
    }

    uf_curl_init(file, priv->curl, &http_curl_ops);
    if (priv->range_start > 0) { //set http range.
        os_memset(httprange, 0, sizeof(httprange));
        os_sprintf(httprange, "%llu-", priv->range_start);
        curl_easy_setopt(priv->curl, CURLOPT_RANGE, httprange);
        UF_DEBUG("http range:%s\n", httprange);
    }

    curl_easy_setopt(priv->curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(priv->curl, CURLOPT_HEADERFUNCTION, uf_curl_http_header);
    curl_easy_setopt(priv->curl, CURLOPT_WRITEHEADER, (void *)file);
    curl_easy_setopt(priv->curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(priv->curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(priv->curl, CURLOPT_TIMEOUT, 0L);
    curl_easy_setopt(priv->curl, CURLOPT_CONNECTTIMEOUT, 10);

    if (UF_AVMODE(file)) {
        curl_easy_setopt(priv->curl, CURLOPT_USERAGENT, "VLC/2.1.5");
    } else {
        curl_easy_setopt(priv->curl, CURLOPT_USERAGENT, "Mozilla/4.0");
    }

    if (priv->headerlist) {
        curl_easy_setopt(priv->curl, CURLOPT_HTTPHEADER, priv->headerlist);
    }

    if (UF_WRITE(file)) {
        if (priv->http_put) {
            curl_easy_setopt(priv->curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(priv->curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)priv->upsize);
        } else {
            curl_easy_setopt(priv->curl, CURLOPT_POST, 1L);
            curl_easy_setopt(priv->curl, CURLOPT_POSTFIELDSIZE, (long)priv->upsize);
        }
    }

    priv->retry_connect = 10;
    uf_state(file, UF_RUN);
    return 0;
}

static int uf_curl_http_release(struct urlfile *file)
{
    struct uf_httppriv *priv = (struct uf_httppriv *)file->priv;
    if (priv) {
        file->priv = NULL;
        curl_easy_cleanup(priv->curl);
        curl_slist_free_all(priv->headerlist);
        os_event_del(&priv->send_sig);
        uf_free(priv);
    }
    return RET_OK;
}

static int uf_curl_http_seek(struct urlfile *file, off_t offset, int whence)
{
    int ret = 0;
    struct uf_httppriv *priv = NULL;

    if (file == NULL) {
        return -1;
    }

    priv = (struct uf_httppriv *)file->priv;
    if (priv == NULL || file->size <= 0) {
        return -1;
    }

    switch (whence) {
        case SEEK_SET:
            if (offset >= 0 && offset < file->size) {
                file->stop = 1;
                UF_LOCK_PROTO(file);
                file->stop = 0;
                file->cur_pos = offset;
                file->doffset = offset;
                priv->range_start = offset;
                uf_state(file, UF_ROPEN);
                rbuffer_reset(&file->buffer);
                UF_UNLOCK_PROTO(file);
            } else if (offset == file->size) {
                file->stop = 1;
                UF_LOCK_PROTO(file);
                file->stop = 0;
                uf_state(file, UF_DONE);
                rbuffer_reset(&file->buffer);
                file->cur_pos = offset;
                file->doffset = offset;
                UF_UNLOCK_PROTO(file);
                UF_DEBUG("seek to end!\n");
            } else {
                ret = -1; /*invalid offset.*/
            }
            break;
        case SEEK_CUR:
            if (offset + file->cur_pos > file->size) {
                ret = -1; /*invalid offset.*/
            } else if (offset + file->cur_pos == file->size) {
                file->stop = 1;
                UF_LOCK_PROTO(file);
                file->stop = 0;
                uf_state(file, UF_DONE);
                rbuffer_reset(&file->buffer);
                file->cur_pos = file->size;
                file->doffset = file->size;
                UF_UNLOCK_PROTO(file);
                UF_DEBUG("seek to end!\n");
            } else { //do real seek acation.
                file->stop = 1;
                UF_LOCK_PROTO(file);
                file->stop = 0;
                priv->range_start = file->cur_pos + offset;
                file->cur_pos = priv->range_start;
                file->doffset = priv->range_start;
                uf_state(file, UF_ROPEN);
                rbuffer_reset(&file->buffer);
                UF_UNLOCK_PROTO(file);
            }
            break;
        case SEEK_END:
            file->stop = 1;
            UF_LOCK_PROTO(file);
            file->stop = 0;
            uf_state(file, UF_DONE);
            rbuffer_reset(&file->buffer);
            file->doffset = file->size;
            file->cur_pos = file->size;
            UF_UNLOCK_PROTO(file);
            UF_DEBUG("seek to end!\n");
            break;
        default:
            ret = -1;
            break;
    }

    priv->lifetime = os_jiffies();
    return ret;
}

static int uf_curl_http_ioctl(struct urlfile *file, uint32 cmd, uint32 param1, uint32 param2)
{
    int ret = 0;
    struct uf_httppriv *priv = NULL;

    if (file == NULL || file->priv == NULL) {
        return -1;
    }

    priv = (struct uf_httppriv *)file->priv;
    switch (cmd) {
        case UF_IOCTL_CMD_GET_TOTALTIME:
            break;
        case UF_IOCTL_CMD_SET_BREAKPOINT_CONTINUALLY:
            priv->break_cont = param1;
            break;
        case UF_IOCTL_CMD_SET_HTTP_HEADER:
            priv->headerlist = curl_slist_append(priv->headerlist, (const char *)param1);
            break;
        case UF_IOCTL_CMD_SET_HTTP_UPSIZE:
            priv->uplen    = 0;
            priv->upsize   = param2;
            priv->http_put = param1;
            break;
        default:
            ret = -ENOTSUPP;
            break;
    }
    return ret;
}

static int32 uf_curl_http_run(struct urlfile *file)
{
    CURLcode ret;
    long http_code = 0;
    struct uf_httppriv *priv = (struct uf_httppriv *)file->priv;

    ret = curl_easy_perform(priv->curl);

    UF_DEBUG("%s: ret:%d (%lld:%lld)\n", __FUNCTION__, ret, file->size, file->doffset);

    curl_easy_getinfo(priv->curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code >= 400) {
        uf_state(file, UF_ERROR); //error occurred
        return -EINVAL;
    }

    switch (ret) {
        case CURLE_OK:
            uf_curl_http_parse_list(file);
            if (!file->stop && uf_curl_http_next_url(file)) {
                break;
            }

            if (!file->stop && uf_curl_http_reopen(file)) {
                break;
            }

            uf_state(file, UF_DONE);
            break;

        case CURLE_WRITE_ERROR:
        case CURLE_GOT_NOTHING:
        case CURLE_ABORTED_BY_CALLBACK:
            uf_state(file, UF_DONE);
            break;

        case CURLE_RECV_ERROR:
        case CURLE_PARTIAL_FILE:
            if (!file->stop && uf_curl_http_reopen(file)) {
                break;
            }
            uf_state(file, UF_ERROR);
            break;

        case CURLE_COULDNT_CONNECT:
        case CURLE_COULDNT_RESOLVE_HOST:
            if (!file->stop && UF_AVMODE(file) && priv->retry_connect-- > 0) { //need reconnect again
                uf_state(file, UF_ROPEN); //connect again.
                break;
            }
            uf_state(file, UF_ERROR);
            break;

        default:
            uf_state(file, UF_ERROR);
            break;
    }
    return RET_OK;
}

static int32 uf_curl_http_send(struct urlfile *file, void *data, size_t size)
{
    uint8 loop = 0;
    struct uf_httppriv *priv = (struct uf_httppriv *)file->priv;

    if (file->state != UF_RUN) {
        return -EINVAL;
    }

    UF_LOCK_WRITE(file);
    priv->send_len  = 0;
    priv->send_size = size;
    priv->send_data = data;

    os_event_clear(&priv->send_sig, BIT(1), NULL);
    os_event_set(&priv->send_sig, BIT(0), NULL);

    while (size != 0xffffffff && loop++ < 100 && file->state == UF_RUN) {
        if (os_event_wait(&priv->send_sig, BIT(1), NULL, OS_EVENT_WMODE_AND | OS_EVENT_WMODE_CLEAR, 100) == RET_OK) {
            break;
        }
    }

    UF_UNLOCK_WRITE(file);
    return priv->send_len;
}

const struct ufprotocol_ops uf_curl_http = {
    .do_init         = uf_curl_http_init,
    .do_release      = uf_curl_http_release,
    .do_seek         = uf_curl_http_seek,
    .do_ioctl        = uf_curl_http_ioctl,
    .do_run          = uf_curl_http_run,
    .do_send         = uf_curl_http_send,
};

