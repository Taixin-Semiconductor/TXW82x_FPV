#include "llm.h"
#include "curl_setup.h"
#include "curl/curl.h"

/* ---- Base64 Encoding/Decoding Table --- */
const char llm_base64encdec[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* The Base 64 encoding with a URL and filename safe alphabet, RFC 4648
   section 5 */
static const char llm_base64urlencdec[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

static const unsigned char llm_decodetable[] = {
    62, 255, 255, 255, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 255, 255, 255,
    255, 255, 255, 255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 255, 255, 255, 255, 255, 255, 26, 27, 28,
    29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51
};

int32 llm_base64_decode(const char *src,
                        unsigned char **outptr, size_t *outlen)
{
    size_t srclen = 0;
    size_t padding = 0;
    size_t i;
    size_t numQuantums;
    size_t fullQuantums;
    size_t rawlen = 0;
    unsigned char *pos;
    unsigned char *newstr;
    unsigned char lookup[256];

    *outptr = NULL;
    *outlen = 0;
    srclen = os_strlen(src);

    /* Check the length of the input string is valid */
    if (!srclen || srclen % 4) {
        llm_err("Check the length of the input string is valid!\r\n");
        return RET_ERR;
    }

    /* srclen is at least 4 here */
    while (src[srclen - 1 - padding] == '=') {
        /* count padding characters */
        padding++;
        /* A maximum of two = padding characters is allowed */
        if (padding > 2) {
            llm_err("A maximum of two = padding characters is allowed!\r\n");
            return RET_ERR;
        }
    }

    /* Calculate the number of quantums */
    numQuantums = srclen / 4;
    fullQuantums = numQuantums - (padding ? 1 : 0);

    /* Calculate the size of the decoded string */
    rawlen = (numQuantums * 3) - padding;

    /* Allocate our buffer including room for a null-terminator */
    newstr = llm_malloc(rawlen + 1);
    if (!newstr) { 
        llm_err("no memory!\r\n");
        return LLME_NOMEM;
    }

    pos = newstr;

    os_memset(lookup, 0xff, sizeof(lookup));
    os_memcpy(&lookup['+'], llm_decodetable, sizeof(llm_decodetable));

    /* Decode the complete quantums first */
    for (i = 0; i < fullQuantums; i++) {
        unsigned char val;
        unsigned int x = 0;
        int j;

        for (j = 0; j < 4; j++) {
            val = lookup[(unsigned char) * src++];
            if (val == 0xff) /* bad symbol */
            { goto bad; }
            x = (x << 6) | val;
        }
        pos[2] = x & 0xff;
        pos[1] = (x >> 8) & 0xff;
        pos[0] = (x >> 16) & 0xff;
        pos += 3;
    }
    if (padding) {
        /* this means either 8 or 16 bits output */
        unsigned char val;
        unsigned int x = 0;
        int j;
        size_t padc = 0;
        for (j = 0; j < 4; j++) {
            if (*src == '=') {
                x <<= 6;
                src++;
                if (++padc > padding)
                    /* this is a badly placed '=' symbol! */
                { goto bad; }
            } else {
                val = lookup[(unsigned char) * src++];
                if (val == 0xff) /* bad symbol */
                { goto bad; }
                x = (x << 6) | val;
            }
        }
        if (padding == 1)
        { pos[1] = (x >> 8) & 0xff; }
        pos[0] = (x >> 16) & 0xff;
        pos += 3 - padding;
    }

    /* Zero terminate */
    *pos = '\0';

    /* Return the decoded data */
    *outptr = newstr;
    *outlen = rawlen;

    return RET_OK;
bad:
    llm_free(newstr);
    return RET_ERR;
}

static int32 llm_base64_encode_process(const char *table64,
                                       unsigned char padbyte,
                                       const char *inputbuff, size_t insize,
                                       char **outptr, size_t *outlen)
{
    char *output;
    char *base64data;
    const unsigned char *in = (const unsigned char *)inputbuff;

    *outptr = NULL;
    *outlen = 0;

    if (!insize)
    { return RET_OK; }

    /* safety precaution */
    if (insize > LLM_MAX_BASE64_INPUT) { 
        llm_err("too large data!\r\n");
        return RET_ERR;
    }

    base64data = output = llm_malloc((insize + 2) / 3 * 4 + 1);
    if (!output) {
        llm_err("no memory!\r\n");
        return LLME_NOMEM;
    }

    while (insize >= 3) {
        *output++ = table64[ in[0] >> 2 ];
        *output++ = table64[((in[0] & 0x03) << 4) | (in[1] >> 4) ];
        *output++ = table64[((in[1] & 0x0F) << 2) | ((in[2] & 0xC0) >> 6) ];
        *output++ = table64[ in[2] & 0x3F ];
        insize -= 3;
        in += 3;
    }
    if (insize) {
        /* this is only one or two bytes now */
        *output++ = table64[ in[0] >> 2 ];
        if (insize == 1) {
            *output++ = table64[((in[0] & 0x03) << 4) ];
            if (padbyte) {
                *output++ = padbyte;
                *output++ = padbyte;
            }
        } else {
            /* insize == 2 */
            *output++ = table64[((in[0] & 0x03) << 4) | ((in[1] & 0xF0) >> 4) ];
            *output++ = table64[((in[1] & 0x0F) << 2) ];
            if (padbyte)
            { *output++ = padbyte; }
        }
    }

    /* Zero terminate */
    *output = '\0';

    /* Return the pointer to the new data (allocated memory) */
    *outptr = base64data;

    /* Return the length of the new data */
    *outlen = (size_t)(output - base64data);

    return RET_OK;
}

int32 llm_base64_encode(const char *inputbuff, size_t insize,
                        char **outptr, size_t *outlen, uint8 is_url)
{
    int32 result = CURLE_OK;
    if (is_url) {
        result = llm_base64_encode_process(llm_base64urlencdec, 0, inputbuff, insize, outptr, outlen);
    } else {
        result = llm_base64_encode_process(llm_base64encdec, '=',
                                           inputbuff, insize, outptr, outlen);
    }
    return result;
}

static int32 llm_code_transform(CURLcode result)
{
    int32 ret = RET_OK;
    switch (result) {
        case CURLE_ABORTED_BY_CALLBACK:
        case CURLE_OK:
            break;
        case CURLE_AGAIN:
            ret = LLME_AGAIN;
            break;
        case CURLE_OUT_OF_MEMORY:
            ret = LLME_NOMEM;
            break;
        case CURLE_OPERATION_TIMEDOUT:
            ret = LLME_WAIT_TIMEOUT;
            break;
        case CURLE_WRITE_ERROR:
            ret = LLME_INTR;
            break;
        default:
            ret = RET_ERR;
            break;
    }
    return ret;
}

static int llm_curl_debug(CURL *curl, curl_infotype itype, char *pData, size_t size, void *userdata)
{
    if (itype == CURLINFO_TEXT) {
        llm_err("[TEXT]%s\n", pData);
    } else if (itype == CURLINFO_HEADER_IN) {
        llm_err("[HEADER_IN]%s\n", pData);
    }
    /*
    else if (itype == CURLINFO_HEADER_OUT)
    {
        llm_err("[HEADER_OUT]%s\n", pData);
    }
    else if (itype == CURLINFO_DATA_IN)
    {
        llm_err("[DATA_IN]%s\n", pData);
    }
    else if (itype == CURLINFO_DATA_OUT)
    {
        llm_err("[DATA_OUT]%s\n", pData);
    }
    */
    return 0;
}

void *llm_build_header(void *headers, char *req_headers)
{
    if (req_headers == NULL || *req_headers == '\0') {
        llm_err("Input param error!\r\n");
        return NULL;
    }

    struct curl_slist *curl_headers = (struct curl_slist *)headers;

    struct curl_slist *new_headers = curl_slist_append(curl_headers, req_headers);
    if (new_headers == NULL) {
        llm_err("no memory\r\n");
        return NULL;
    }

    return (void *)new_headers;
}

int32 llm_free_header(void *headers)
{
    struct curl_slist *curl_headers = (struct curl_slist *)headers;

    if (curl_headers) {
        curl_slist_free_all(curl_headers);
        return RET_OK;
    }
    return RET_ERR;
}

void *llm_https_connect(void *headers, void *cfg,
                        void *write_callback, void *read_callback, void *xferinfo_callback, void *userdata)
{
    CURL *curl = NULL;

    if (!cfg) {
        llm_err("Missing configuration parameters!\r\n");
        return NULL;
    }

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist *curl_headers = (struct curl_slist *)headers;
        struct llm_trans_param *trans_cfg = (struct llm_trans_param *)cfg;

        if (trans_cfg->debug) {
    		curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, llm_curl_debug);
		    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        }

        if (trans_cfg->blob_data && trans_cfg->blob_len) {
            // 设置 CURLOPT_CAINFO_BLOB
            struct curl_blob blob;
            blob.data = trans_cfg->blob_data;
            blob.len = trans_cfg->blob_len;
            blob.flags = CURL_BLOB_NOCOPY;
            curl_easy_setopt(curl, CURLOPT_CAINFO_BLOB, &blob);
        } else {
            // 不验证证书
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            // 不验证主机名
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }
        curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, trans_cfg->buffersize);           //1K-CURL_MAX_WRITE_SIZE,  default:16K
        curl_easy_setopt(curl, CURLOPT_UPLOAD_BUFFERSIZE, trans_cfg->upload_buffersize);    //CURL_MAX_WRITE_SIZE-2M,   default:64K

        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, trans_cfg->connect_timeout);

        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, trans_cfg->low_speed_limit);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, trans_cfg->low_speed_time);

        if (write_callback) {
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, userdata);
        }
        if (read_callback) {
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
            curl_easy_setopt(curl, CURLOPT_READDATA, userdata);
        }
        if (xferinfo_callback) {
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo_callback);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, userdata);
        }

        if (curl_headers) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
        }
    } else {
        llm_err("curl create fail!\r\n");
    }

    return (void *)curl;
}

int32 llm_https_disconnect(void *handle)
{
    CURL *curl = (CURL *)handle;

    if (curl) {
        curl_easy_cleanup(curl);
        return RET_OK;
    }
    return RET_ERR;
}

int32 llm_https_send(void *handle, llm_http_type type, char *url, char *buff, size_t length)
{
    CURLcode result;
    CURL *curl = (CURL *)handle;

    if (!curl) {
        llm_err("curl is not exist!!\r\n");
        return RET_ERR;
    }
    if (!url) {
        llm_err("url is not exist!!\r\n");
        return RET_ERR;
    }

    curl_easy_setopt(curl, CURLOPT_HTTPGET, 0L);
    curl_easy_setopt(curl, CURLOPT_POST, 0L);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, NULL);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);

    // 设置 url
    curl_easy_setopt(curl, CURLOPT_URL, url);

    switch (type) {
        case LLM_HTTP_GET:
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            break;

        case LLM_HTTP_POST:
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buff);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, length);
            break;

        case LLM_HTTP_PUT:
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(curl, CURLOPT_READDATA, buff);
            curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)length);
            break;

        case LLM_HTTP_DELETE:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            break;

        case LLM_HTTP_PATCH:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buff);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, length);
            break;

        default:
            llm_err("unsupported http type!\r\n");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buff);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, length);
            break;
    }

    result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        llm_err("failed to receive message(%d): %s\r\n", result, curl_easy_strerror(result));
    }
    return llm_code_transform(result);
}

int32 llm_https_upload(void *handle, char *url, char *buff, size_t length,
                        const char *name, const char *filename,
                        void *read_callback, void *seek_callback, void *free_callback, void *userdata)
{
    CURLcode result;
    CURL *curl = (CURL *)handle;
    curl_mime *form     = NULL;
    curl_mimepart *part = NULL;

    if (!curl) {
        llm_err("curl is not exist!!\r\n");
        return RET_ERR;
    }
    if (!url) {
        llm_err("url is not exist!!\r\n");
        return RET_ERR;
    }

    form = curl_mime_init(curl);
    if (!form) {
        llm_err("init mime failed!\n");
        return RET_ERR;
    }

    part = curl_mime_addpart(form);
    if (!part) {
        llm_err("init mime part failed!\n");
        curl_mime_free(form);
        return RET_ERR;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    
    curl_mime_name(part, name);
    curl_mime_filename(part, filename);
    if (buff) {
        curl_mime_data(part, buff, length);
    } else {
        curl_mime_data_cb(part, length, read_callback, seek_callback, free_callback, userdata);
    }
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

    result = curl_easy_perform(curl);

    curl_mime_free(form);

    return llm_code_transform(result);
}

//no use
int32 llm_https_recv(void *handle, char *buff, size_t length, size_t *nread)
{
    CURL *curl = (CURL *)handle;

    if (!curl) {
        llm_err("curl is not exist!!\r\n");
        return RET_ERR;
    }

    return RET_OK;
}

void *llm_websocket_connect(char *url, void *headers, void *cfg)
{
    CURLcode ret = CURLE_OK;
    CURL *curl = NULL;

    if (!url || !cfg) {
        llm_err("Missing configuration parameters!\r\n");
        return NULL;
    }

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist *curl_headers = (struct curl_slist *)headers;
        struct llm_trans_param *trans_cfg = (struct llm_trans_param *)cfg;

        if (trans_cfg->debug) {
    		curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, llm_curl_debug);
		    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        }

        if (trans_cfg->blob_data && trans_cfg->blob_len) {
            // 设置 CURLOPT_CAINFO_BLOB
            struct curl_blob blob;
            blob.data = trans_cfg->blob_data;
            blob.len = trans_cfg->blob_len;
            blob.flags = CURL_BLOB_NOCOPY;
            curl_easy_setopt(curl, CURLOPT_CAINFO_BLOB, &blob);
        } else {
            // 不验证证书
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            // 不验证主机名
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }
        curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, trans_cfg->buffersize);           //1K-CURL_MAX_WRITE_SIZE,  default:16K
        curl_easy_setopt(curl, CURLOPT_UPLOAD_BUFFERSIZE, trans_cfg->upload_buffersize);    //CURL_MAX_WRITE_SIZE-2M,   default:64K
        // 指定只做连接，不处理数据传输（仅用于握手）
        curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L);

        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, trans_cfg->connect_timeout);

        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, trans_cfg->low_speed_limit);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, trans_cfg->low_speed_time);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        if (curl_headers) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
        }
        ret = curl_easy_perform(curl);
        if (ret != CURLE_OK) {
            llm_err("curl_easy_perform(%d) failed: %s\n", ret, curl_easy_strerror(ret));
            //long http_code = 0;
            //curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            //llm_err("HTTP Response Code: %ld\n", http_code);
            curl_easy_cleanup(curl);
            return NULL;
        }
    } else {
        llm_err("curl create fail!\r\n");
    }

    return (void *)curl;
}

int32 llm_websocket_disconnect(void *handle)
{
    CURL *curl = (CURL *)handle;

    if (curl) {
        curl_easy_cleanup(curl);
        return RET_OK;
    }
    return RET_ERR;
}

int32 llm_websocket_send(void *handle, char *buff, size_t length, size_t *sent)
{
    CURLcode result;
    CURL *curl = (CURL *)handle;

    if (!curl) {
        llm_err("curl is not exist!!\r\n");
        return RET_ERR;
    }
    if (!buff || !length) {
        llm_err("Invalid data!\r\n");
        return RET_ERR;
    }

    result = curl_ws_send(curl, (const char *)buff, length, sent, 0, CURLWS_BINARY);
    if (result != CURLE_OK && result != CURLE_AGAIN) {
        llm_err("Failed to send complete message(%d): %s\n", result, curl_easy_strerror(result));
    }
    return llm_code_transform(result);
}

int32 llm_websocket_recv(void *handle, char *buff, size_t length, size_t *nread)
{
    CURLcode result;
    const struct curl_ws_frame *meta;
    CURL *curl = (CURL *)handle;

    if (!curl) {
        llm_err("curl is not exist!!\r\n");
        return RET_ERR;
    }
    if (!buff || !length) {
        llm_err("No space to receive!\r\n");
        return RET_ERR;
    }

    result = curl_ws_recv(curl, buff, length, nread, &meta);
    if (result != CURLE_OK && result != CURLE_AGAIN) {
        llm_err("failed to receive message(%d): %s\r\n", result, curl_easy_strerror(result));
    }
    if (meta) {
        llm_dbg("rx meta: bytesleft = %lld, flags = %d, len = %d(%d), offset = %lld\r\n",
                meta->bytesleft, meta->flags, *nread, meta->len, meta->offset);
    }
    return llm_code_transform(result);
}

int32 llm_websocket_psend(void *handle, char *buff, size_t length, llm_send_meta *smeta)
{
    CURLcode result;
    CURL *curl = (CURL *)handle;

    if (!curl) {
        llm_err("curl is not exist!!\r\n");
        return RET_ERR;
    }
    if (!buff || !length) {
        llm_err("Invalid data!\r\n");
        return RET_ERR;
    }

    result = curl_ws_send(curl, (const char *)buff, length, &smeta->sent, 0, smeta->flags);
    if (result != CURLE_OK && result != CURLE_AGAIN) {
        llm_err("Failed to send complete message(%d): %s\n", result, curl_easy_strerror(result));
    }
    return llm_code_transform(result);
}

int32 llm_websocket_precv(void *handle, char *buff, size_t length, llm_recv_meta *rmeta)
{
    CURLcode result;
    const struct curl_ws_frame *meta;
    CURL *curl = (CURL *)handle;

    if (!curl) {
        llm_err("curl is not exist!!\r\n");
        return RET_ERR;
    }
    if (!buff || !length) {
        llm_err("No space to receive!\r\n");
        return RET_ERR;
    }

    result = curl_ws_recv(curl, buff, length, &rmeta->nread, &meta);
    if (result != CURLE_OK && result != CURLE_AGAIN) {
        llm_err("failed to receive message(%d): %s\r\n", result, curl_easy_strerror(result));
    }
    if (meta) {
        rmeta->flags = meta->flags;
        rmeta->offset = meta->offset;
        rmeta->bytesleft = meta->bytesleft;
        rmeta->len = meta->len;
        llm_dbg("rx meta: bytesleft = %lld, flags = %d, len = %d(%d), offset = %lld\r\n",
                rmeta->bytesleft, rmeta->flags, rmeta->nread, rmeta->len, rmeta->offset);
    }
    return llm_code_transform(result);
}

int32 llm_trans_init(void)
{
//    curl_global_init_mem(CURL_GLOBAL_SSL,
//        llm_malloc, llm_free, llm_realloc, llm_strdup, llm_calloc);
    return RET_OK;
}

int32 llm_trans_deinit(void)
{
//    curl_global_cleanup();
    return RET_OK;
}

