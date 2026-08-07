#ifndef _URL_FILE_H_
#define _URL_FILE_H_
#include "basic_include.h"
#include "lib/common/rbuffer.h"
#include "lib/posix/stdio.h"
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "lwip/inet_chksum.h"
#include "netif/etharp.h"
#include "netif/ethernetif.h"
#include "lwip/ip.h"
#include "lwip/init.h"
#include "lib/net/skmonitor/skmonitor.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UF_DEBUG(f,...)      os_printf("%s:%d:"f, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define UF_ERR(f,...)        os_printf(KERN_ERR"%s:%d:"f, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define UF_LOCK_READ(uf)     os_mutex_lock(&uf->r_lock, osWaitForever)
#define UF_UNLOCK_READ(uf)   os_mutex_unlock(&uf->r_lock)

#define UF_LOCK_WRITE(uf)    os_mutex_lock(&uf->w_lock, osWaitForever)
#define UF_UNLOCK_WRITE(uf)  os_mutex_unlock(&uf->w_lock)

#define UF_LOCK_PROTO(uf)    os_mutex_lock(&uf->lock, osWaitForever)
#define UF_UNLOCK_PROTO(uf)  os_mutex_unlock(&uf->lock)

#define UF_READ(uf)          ((uf)->mode & UF_MODE_RD)
#define UF_WRITE(uf)         ((uf)->mode & UF_MODE_WR)
#define UF_AVMODE(uf)        ((uf)->mode & UF_MODE_AV)

typedef enum {
    UF_CLOSE         = -2, /*need close, this state only can be set by uf_close*/
    UF_ERROR         = -1, /*error occured*/
    UF_NONE          =  0, /*initial state*/
    UF_ROPEN         =  1, /*reopen*/
    UF_OPEN          =  2, /*start open*/
    UF_RUN           =  3, /*data transfer*/
    UF_DONE          =  4  /*data transfer complete, it is idle*/
} UF_STATE;

typedef enum {
    UF_MODE_AV       = BIT(0), //AV Mode
    UF_MODE_RD       = BIT(1), //support Read
    UF_MODE_WR       = BIT(2), //support Write
    UF_MODE_RECV     = BIT(3), //Customer recieve
    UF_MODE_MONITOR  = BIT(4), //Use sock moniotr.
    UF_MODE_AUTO_RUN = BIT(5), //auto run.
} UF_MODE;

typedef enum  {
    UF_PLAYLIST_NONE = 0,
    UF_PLAYLIST_CUST,
    UF_PLAYLIST_HLS,
    UF_PLAYLIST_ASX,
    UF_PLAYLIST_MMSREF,
    UF_PLAYLIST_M3U,
    UF_PLAYLIST_PLS,
    UF_PLAYLIST_NSC,
    UF_PLAYLIST_SMIL,
    UF_PLAYLIST_XSPF,
    UF_PLAYLIST_MAX,
} UF_PLAYLIST_TYPE;

typedef enum {
    UF_IOCTL_CMD_IDLE,
    UF_IOCTL_CMD_SOCK_ERROR,
    UF_IOCTL_CMD_GET_FILESIZE,
    UF_IOCTL_CMD_GET_TOTALTIME,
    UF_IOCTL_CMD_SET_EVTCB,
    UF_IOCTL_CMD_SET_BREAKPOINT_CONTINUINGLY,
    UF_IOCTL_CMD_SET_HTTP_HEADER,
    UF_IOCTL_CMD_SET_HTTP_UPSIZE,
} UF_IOCTL_CMD;

typedef enum {
    UF_EVENT_RECVD_DATA,
    UF_EVENT_TRANS_COMPLETE,
} UF_EVENT;

struct urlfile;

struct ufprotocol_ops {
    int (*do_init)(struct urlfile *file);
    int (*do_release)(struct urlfile *file);
    int (*do_run)(struct urlfile *file);
    int (*do_seek)(struct urlfile *file, off_t offset, int whence);
    int (*do_send)(struct urlfile *file, void *data, size_t size);
    int (*do_ioctl)(struct urlfile *file, uint32 cmd, uint32 param1, uint32 param2);
};

struct ufprotocol {
    const char *name;
    const struct ufprotocol_ops *ops;
};

typedef int32(*uf_evt_cb)(struct urlfile *file, uint32 evet, uint32 param1, uint32 param2);

struct uf_playlist_entry {
    struct list_head list;
    char  *title;
    char  *url;
};

struct uf_playlist {
    uint8 *buff_addr;
    uint32 buff_size;
    uint32 buff_len;
    uint16 type;
    uint16 count;
    struct list_head  list;
};

struct urlfile {
    uint8              mode;      /*url open mode: 0:AVMODE, BIT0:read; BIT1:write; others:customize*/
    int8               state;     //url file state
    uint16             sock;
    char              *url;
    off_t              size;      /*initial value is 0, it need be set after connect success.
                                    if the file size is unkown this value need set -1. such as for a live stream URL*/
    off_t              cur_pos;   //current read position
    off_t              doffset;   //current download offset
    void              *priv;      //private data for ufprotocol
    uf_evt_cb          evt_cb;    //event callback.
    struct os_mutex    lock;
    struct os_mutex    r_lock;
    struct os_mutex    w_lock;
    uint64             open_tick;

    const struct ufprotocol *proto;   //match ufprotocol by URL protocol name
    struct rbuffer      buffer;       //used to store the received data.
    void               *task;
    struct uf_playlist  playlist;
    uint16              playindex;
    uint8               seek_type;
    uint8               rev;
};

//mode: 
//  'v': AV mode, 
//  'r': support read, 
//  'w': support write, 
//  'c': customer receive data, needn't use uf_read.
//       the received data will be output by UF_EVENT_RECVD_DATA. 
//       need set UF_IOCTL_CMD_SET_EVTCB.
//  'm': use socket monitor, needn't create task for this urlfile.
//  'a': auto run
void *uf_open(char *url, char *mode, uint32 flags);
void uf_close(void *file);
size_t uf_read(void *ptr, size_t size, size_t nmemb, void *file);
size_t uf_write(void *ptr, size_t size, size_t nmemb, void *file);
int uf_seek(void *file, off_t offset, int whence);
off_t uf_tell(void *file);
int uf_eof(void *file);
int uf_ioctl(void *file, uint32 cmd, uint32 param1, uint32 param2);
off_t uf_filesize(void *fp, uint32 size);
int32 uf_seektype(void *fp);

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void *uf_alloc(int32 size);
void uf_free(void *ptr);
char *uf_strdup(char *s);
void uf_state(struct urlfile *file, UF_STATE state);
void urlfile_init(const struct ufprotocol *protos, int count);
size_t uf_store_data(struct urlfile *file, char *data, size_t len);
int uf_playlist_recieve(struct uf_playlist *list, char *data, int len, uint32 tot_size);
void uf_playlist_detect(struct uf_playlist *list, void *data, size_t size);
int32 uf_playlist_build(struct uf_playlist *list, char *urls);
int32 uf_playlist_free(struct uf_playlist *list);
int32 uf_playlist_new_url(struct uf_playlist *list, char *title, char *url);
char *uf_playlist_get_url(struct uf_playlist *list, uint16 index);
char *uf_playlist_get_line(char *buf, char **line);

//////////////////////////////////////////////////////////////////
const struct ufprotocol *uf_findproto(char *url);
extern const struct ufprotocol_ops uf_curl_http;

#ifdef __cplusplus
}
#endif
#endif

