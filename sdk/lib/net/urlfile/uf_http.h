#ifndef _UF_HTTP_H__
#define _UF_HTTP_H__
#include "lib/net/urlfile/urlfile.h"
#include "curl/curl.h"
#include "uf_util.h"

struct uf_httppriv {
    uint64   range_start; // for seek/reopen
    uint64   lifetime;

    int8     seek_type;     // 0: time seek, 1: byte seek.
    int8     break_cont: 1, // breakpoint continuingly, 0: disable, 1: enable, default is 1;
             http_put: 1,
             rev: 6;
    int8     retry_connect;  //try to connect if connect fail.
    uint32   upsize, uplen;

    struct os_event send_sig;
    uint8         *send_data;
    int32          send_size;
    int32          send_len;

    CURL          *curl;
    struct   curl_slist *headerlist;
};


#endif//_UF_HTTP_H__

