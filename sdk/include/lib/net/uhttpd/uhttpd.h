#ifndef _TX_UHTTPD_H_
#define _TX_UHTTPD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "lib/net/sslsock/ssl_sock.h"

#define uhttpd_dbg(fmt, ...) //os_printf(KERN_ERR fmt, ##__VA_ARGS__)
#define uhttpd_err(fmt, ...) os_printf(KERN_ERR fmt, ##__VA_ARGS__)

#define UHTTPD_RXBUF_SIZE (1536)

enum UHTTPD_VALUE_TYPE {
    UHTTPD_VALUE_TYPE_UINT32,
    UHTTPD_VALUE_TYPE_UINT8,
    UHTTPD_VALUE_TYPE_UINT16,
    UHTTPD_VALUE_TYPE_STRING,
    UHTTPD_VALUE_TYPE_UINT32_BIT,
    UHTTPD_VALUE_TYPE_UINT8_BIT,
    UHTTPD_VALUE_TYPE_UINT16_BIT,
    UHTTPD_VALUE_TYPE_IPV4,
    UHTTPD_VALUE_TYPE_MACV4,
    UHTTPD_VALUE_TYPE_CMD,
    UHTTPD_VALUE_TYPE_SARRAY,
    UHTTPD_VALUE_TYPE_RET_INT8,
    UHTTPD_VALUE_TYPE_RET_UINT8,
    UHTTPD_VALUE_TYPE_RET_INT16,
    UHTTPD_VALUE_TYPE_RET_UINT16,
    UHTTPD_VALUE_TYPE_RET_INT32,
    UHTTPD_VALUE_TYPE_RET_UINT32,
    UHTTPD_VALUE_TYPE_RET_INT64,
    UHTTPD_VALUE_TYPE_RET_UINT64,
    UHTTPD_VALUE_TYPE_RET_STRING,
};
enum UHTTPD_VALUE_FLAGS {
    UHTTPD_VALUE_FLAGS_READONLY = BIT(0),
};

typedef void(*uhttpd_val_cmdhdl)(int32 sock, char *val, char *resp, int32 size, uint32 *resp_len);

struct uhttpd_value_entry {
    char  *name;
    void  *value;
    uint32 type: 5, maxlen: 8, bitoff: 5, maskbit: 5, flags: 2, rev: 7;
};

struct uhttpd_html {
    const char *name;
    const char *html;
    uint32 size;
};

typedef void (*uhttpd_action_hdl)(int32 sock, uint32 client_ip, char *buff, uint32 size);
struct uhttpd_action {
    char *name;
    uhttpd_action_hdl hdl;
};

int32 uhttpd_start(char *ifname, uint32 port, const sslsock_ops *ssl, const ssl_sock_init_param *param);
void uhttpd_stop(void);

////////////////////////////////////////////////////////////////////////////
int32 uhttpd_send_resp(int32 sock, uint32 status, uint8 gzip, char *values, int vlen, uint32 flash);
int32 uhttpd_resp_status(int32 sock, char *resp, uint8 status);
void uhttpd_proc_post(int32 sock, uint32 client_ip, char *buf, uint32 len);
void uhttpd_proc_get(int32 sock, uint32 client_ip, char *buf, uint32 len);
int32 uhttpd_get_flash_data(uint32 addr, uint32 len, uint8 *buff);
const struct uhttpd_html *uhttpd_find_html(char *url);
const struct uhttpd_action *uhttpd_action_find(int32 sock, uint32 client_ip, char *buf, uint32 len);
const struct uhttpd_value_entry *uhttpd_find_value(char *name);
void uhttpd_proc_auth_action(int32 sock, uint32 client_ip, char *buf, uint32 len);
void uhttpd_proc_post_action(int32 sock, uint32 client_ip, char *buf, uint32 len);
void uhttpd_proc_options(int32 sock, uint32 client_ip, char *buff, uint32 len);
void uhttpd_proc_firmware_upg(int32 sock, uint32 client_ip, char *buf, uint32 size);

#ifdef __cplusplus
}
#endif
#endif

