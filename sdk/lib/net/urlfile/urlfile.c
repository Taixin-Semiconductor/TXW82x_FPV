#include "lib/net/urlfile/urlfile.h"

static const struct ufprotocol *g_ufprotocols;
static int   g_ufprotocols_cnt = 0;

static uint16 _uf_mode(char *mode)
{
    char *p = mode;
    uint16 umode = 0;

    while (*p) {
        switch (*p) {
            case 'v':
                umode |= UF_MODE_AV;
                break;
            case 'r':
                umode |= UF_MODE_RD;
                break;
            case 'w':
                umode |= UF_MODE_WR;
                break;
            case 'c':
                umode |= UF_MODE_RECV;
                break;
            case 'm':
                umode |= UF_MODE_MONITOR;
                break;
            case 'a':
                umode |= UF_MODE_AUTO_RUN;
                break;
            default:
                break;
        }
        p++;
    }
    return umode;
}

static void _uf_release(struct urlfile *file)
{
    if (file->mode & UF_MODE_MONITOR) {
        sock_monitor_del(file->sock);
    }
    if (file->proto) {
        file->proto->ops->do_release(file);
    }
    os_mutex_del(&file->r_lock);
    os_mutex_del(&file->w_lock);
    os_mutex_del(&file->lock);
    rbuffer_destroy(&file->buffer);
    uf_free(file->buffer.rbq);
    uf_free(file->url);
    uf_free(file);
}

static void _uf_task(void *arg)
{
    struct urlfile *file = (struct urlfile *)arg;

    if (file->mode & UF_MODE_AUTO_RUN) {
        uf_state(file, UF_OPEN);
    }

    while (1) {
        if (file->state == UF_CLOSE) {
            break;
        } else if (file->state <= UF_NONE || file->state == UF_DONE) {
            os_sleep_ms(10);
            continue;
        } else { /*以下的状态需要执行lock*/
            UF_LOCK_PROTO(file);
            switch (file->state) {
                case UF_ROPEN:
                    uf_state(file, UF_OPEN);
                    break;
                case UF_OPEN:
                    file->open_tick = os_jiffies();
                    file->proto->ops->do_init(file);
                    break;
                case UF_RUN:
                    file->proto->ops->do_run(file);
                    break;
                default:
                    ASSERT(0);
                    break;
            }
            UF_UNLOCK_PROTO(file);
        }
    }

    UF_DEBUG("uf release ... \n");
    _uf_release(file);
}

static void _uf_skmonitor_cb(uint16 sock, skmonitor_flags flags, uint32 priv)
{
    struct urlfile *file = (struct urlfile *)priv;

    if (file->state == UF_OPEN || file->state == UF_ROPEN) {
        uf_state(file, UF_RUN);
    }

    if (file->state == UF_CLOSE) {
        UF_DEBUG("uf release ... \n");
        _uf_release(file);
    } else if (file->state == UF_RUN) {
        if (flags & SOCK_MONITOR_READ) {
            file->proto->ops->do_run(file);
        }
        if (flags & SOCK_MONITOR_ERROR) {
            sock_monitor_del(file->sock);
            file->proto->ops->do_ioctl(file, UF_IOCTL_CMD_SOCK_ERROR, 0, 0);
        }
    } else {
        UF_ERR("uf %s invalid state %d\r\n", file->url, file->state);
    }
}

static void _uf_skmonitor_init(uint32 param1, uint32 param2, uint32 param3)
{
    struct urlfile *file = (struct urlfile *)param1;

    if (file->proto->ops->do_init(file) == RET_OK) {
        uf_state(file, UF_OPEN);
        fcntl(file->sock, F_SETFL, O_NONBLOCK);
        sock_monitor_add(file->sock, SOCK_MONITOR_READ | SOCK_MONITOR_ERROR, _uf_skmonitor_cb, (uint32)file);
    } else {
        UF_ERR("uf %s init fail\r\n", file->url);
    }
}

static int32 _uf_url_list(char *url)
{
    char *ptr = os_strstr(url, "\r\n");
    if(ptr == NULL) ptr = os_strchr(url, ';');

    if (ptr) {
        if (os_strstr(ptr + 2, "://")) {
            return 1;
        }
    }
    return 0;
}

size_t uf_store_data(struct urlfile *file, char *data, size_t len)
{
    int32 ret = 0;
    size_t wlen = 0;

    if (file == NULL || data == NULL) {
        return wlen;
    }

    if (file->open_tick > 0) {
        UF_DEBUG("url:%s, first data time:%llums. %d bytes\n", file->url, os_jiffies() - file->open_tick, len);
        file->open_tick = 0;
    }

    if (file->state == UF_OPEN) {
        uf_state(file, UF_RUN);
    }

    while (!file->stop && wlen < len) {
        ret = file->evt_cb ? file->evt_cb(file, UF_EVENT_RECVD_DATA, (uint32)(data + wlen), len - wlen) : -ENOTSUP;
        if (ret == -ENOTSUP) {
            ret = rbuffer_set(&file->buffer, data + wlen, len - wlen);
        }

        if (ret > 0) {
            wlen += ret;
            file->doffset += ret;
        } else if (!file->stop) {
            os_sleep_ms(5);
        }
    }

    return wlen;
}

void uf_state(struct urlfile *file, UF_STATE state)
{
    uint32 flag = disable_irq();
    switch (state) {
        case UF_ROPEN:
            if (file->state == UF_OPEN || file->state == UF_RUN ||
                file->state == UF_DONE || file->state == UF_ERROR) {
                file->state = state;
            }
            break;
        case UF_OPEN:
            if (file->state == UF_NONE || file->state == UF_ROPEN) {
                file->state = state;
            }
            break;
        case UF_RUN:
            if (file->state == UF_OPEN) {
                file->state = state;
            }
            break;
        case UF_CLOSE:
            file->state = state;
            break;
        case UF_ERROR:
            if (file->state == UF_OPEN || file->state == UF_RUN) {
                file->state = state;
            }
            break;
        case UF_DONE:
            if (file->state == UF_RUN) {
                file->state = state;
            }
            break;
        default:
            break;
    }
    enable_irq(flag);
}

int32 uf_reopen(struct urlfile *file)
{
    if (file->mode & UF_MODE_MONITOR) {
        sock_monitor_del(file->sock);
        uf_state(file, UF_ROPEN);
        return os_run_func(_uf_skmonitor_init, (uint32)file, 0, 0);
    }
    return -ENOTSUP;
}

int32 uf_run(struct urlfile *file)
{
    if (file->state == UF_NONE) {
        if ((file->mode & UF_MODE_MONITOR) && !(file->mode & UF_MODE_AUTO_RUN)) {
            os_run_func(_uf_skmonitor_init, (uint32)file, 0, 0);
        } else {
            uf_state(file, UF_OPEN);
        }
    }
    return RET_OK;
}

void *uf_open(char *url, char *mode, uint32 flags)
{
    struct urlfile *file  = NULL;
    const struct ufprotocol *proto = NULL;
    uint32 bufsize = (flags & 0xffff) * 1024;
    uint32 stacksz = ((flags >> 16) & 0xff) * 1024;
    uint16 umode   = _uf_mode(mode);

    if (url == NULL) {
        UF_ERR("invalid url [%s]\n", url);
        return NULL;
    }

    proto = uf_findproto(url);
    if (proto == NULL) {
        UF_ERR("not support %s\r\n", url);
        return NULL;
    }

    file = uf_alloc(sizeof(struct urlfile));
    if (file == NULL) {
        UF_ERR("malloc fail [%s]\n", url);
        return NULL;
    }

    os_memset(file, 0, sizeof(struct urlfile));
    if (stacksz == 0) {
        stacksz = 8192;
    }

    if (bufsize == 0) {
        bufsize = 4096;
    }

    INIT_LIST_HEAD(&file->playlist.list);
    if (_uf_url_list(url)) {
        uf_playlist_build(&file->playlist, url);
        file->url = uf_strdup(uf_playlist_get_url(&file->playlist, 1));
        file->playindex = 1;
        UF_DEBUG("open url list, count %d\r\n", file->playlist.count);
    } else {
        file->url = uf_strdup(url);
        UF_DEBUG("open url! [%s]\n", file->url);
    }

    if (file->url == NULL) {
        UF_ERR("no mem\r\n");
        _uf_release(file);
        return NULL;
    }

    if ((umode & UF_MODE_RD) && !(umode & UF_MODE_RECV)) {
        uint8 *rxbuf = uf_alloc(bufsize);
        if (rxbuf == NULL) {
            UF_ERR("malloc fail, bufsize=%d\n", bufsize);
            _uf_release(file);
            return NULL;
        }
        rbuffer_init(&file->buffer, bufsize, rxbuf);
    }

    os_mutex_init(&file->lock);
    os_mutex_init(&file->r_lock);
    os_mutex_init(&file->w_lock);

    file->mode  = umode;
    file->proto = proto;
    if ((umode & UF_MODE_MONITOR) && (umode & UF_MODE_AUTO_RUN)) {
        if (os_run_func(_uf_skmonitor_init, (uint32)file, 0, 0)) {
            _uf_release(file);
            UF_ERR("no mem\r\n");
            return NULL;
        }
    } else {
        file->task = os_task_create(file->proto->name, _uf_task, file, OS_TASK_PRIORITY_NORMAL, 0, NULL, stacksz);
        if (file->task == NULL) {
            _uf_release(file);
            UF_ERR("create task fail, stacksz=%d\n", stacksz);
            return NULL;
        }
    }

    return file;
}

void uf_close(void *fp)
{
    struct urlfile *file = (struct urlfile *)fp;
    if (file) {
        UF_DEBUG("uf_close (%s)\n", file->url);
        if (file->mode & UF_MODE_MONITOR) {
            file->stop = 1;
            if (file->state > UF_NONE) {
                uf_state(file, UF_CLOSE);
                closesocket(file->sock); //触发skmonitor error
            } else {
                _uf_release(file);
            }
        } else {
            uf_state(file, UF_CLOSE);
            file->stop = 1;
        }
    }
}

size_t uf_read(void *ptr, size_t size, size_t nmemb, void *fp)
{
    size_t ret = 0;
    struct urlfile *file = (struct urlfile *)fp;

    if (file == NULL) {
        return 0;
    }

    UF_LOCK_READ(file);
    if (!(file->mode & UF_MODE_RECV)) {
        ret = rbuffer_get(&file->buffer, ptr, size * nmemb);
    }
    file->cur_pos += ret;
    UF_UNLOCK_READ(file);
    return ret;
}

size_t uf_write(void *ptr, size_t size, size_t nmemb, void *fp)
{
    struct urlfile *file = (struct urlfile *)fp;
    if ((file->mode & UF_MODE_WR) && (file->proto->ops->do_send)) {
        return file->proto->ops->do_send(file, ptr, size * nmemb);
    } else {
        return -ENOTSUP;
    }
}

int uf_seek(void *fp, off_t offset, int whence)
{
    int ret = -1;
    off_t npos;
    struct urlfile *file = (struct urlfile *)fp;

    if (file == NULL || file->state <= UF_NONE) {
        return -1;
    }

    UF_LOCK_READ(file);
    npos = file->cur_pos + offset;
    if (whence == SEEK_CUR && npos >= file->cur_pos && npos <= file->doffset) {
        //新的offset尚在buffer内，直接移动读指针
        file->buffer.rpos = rbuffer_move(&file->buffer, file->buffer.rpos, offset);
        ret = RET_OK;
    } else {
        if (file->proto && file->proto->ops->do_seek) {
            ret = file->proto->ops->do_seek(file, offset, whence);
        }
    }
    UF_UNLOCK_READ(file);
    UF_DEBUG("uf_seek [%lld:%d] ret:%d\n", file->doffset, whence, ret);
    return ret;
}

off_t uf_tell(void *fp)
{
    struct urlfile *file = (struct urlfile *)fp;
    if (file == NULL) {
        return -1;
    }
    return file->cur_pos;
}

int uf_eof(void *fp)
{
    int eof = 1;
    struct urlfile *file = (struct urlfile *)fp;

    if (file == NULL) {
        return eof;
    }

    eof = (file->size > 0 && file->cur_pos >= file->size) ||
          (file->state == UF_DONE  && file->cur_pos >= file->doffset) ||
          (file->state == UF_ERROR && file->cur_pos >= file->doffset);

    if (eof) {
        UF_DEBUG("file:%s EOF (%lld - %lld)\r\n", file->url, file->size, file->doffset);
    }
    return eof;
}

int uf_ioctl(void *fp, uint32 cmd, uint32 param1, uint32 param2)
{
    int ret = -1;
    struct urlfile *file = (struct urlfile *)fp;

    if (file == NULL || file->state <= UF_NONE) {
        return ret;
    }

    if (file->proto && file->proto->ops->do_ioctl) {
        ret = file->proto->ops->do_ioctl(file, cmd, param1, param2);
    }

    if (ret == -ENOTSUPP && cmd == UF_IOCTL_CMD_SET_EVTCB) {
        file->evt_cb = (uf_evt_cb)param1;
        ret = 0;
    }

    return ret;
}

off_t uf_filesize(void *fp, uint32 size)
{
    struct urlfile *file = (struct urlfile *)fp;
    return file->size && RB_COUNT(&file->buffer) >= size ? file->size : 0;
}

uint32 uf_datasize(void *fp, uint8 *buffer_level, uint8 *eod)
{
    off_t count = 0;
    struct urlfile *file = (struct urlfile *)fp;
    if (file->buffer.qsize) {
        count = RB_COUNT(&file->buffer);
        if (buffer_level) {
            *buffer_level = (count * 100) / file->buffer.qsize;
        }
        if (eod) {
            *eod = (file->size > 0 && file->doffset >= file->size);
        }
    }
    return count;
}

int32 uf_seektype(void *fp)
{
    struct urlfile *file = (struct urlfile *)fp;
    return file->seek_type;
}

const struct ufprotocol *uf_findproto(char *url)
{
    int i = 0;

    if (url == NULL) {
        return NULL;
    }

    for (i = 0; i < g_ufprotocols_cnt; i++) {
        if (os_strncasecmp(g_ufprotocols[i].name, url, os_strlen(g_ufprotocols[i].name)) == 0) {
            UF_DEBUG("find %s protocol for %s !\r\n", g_ufprotocols[i].name, url);
            return &g_ufprotocols[i];
        }
    }
    return NULL;
}

void urlfile_init(const struct ufprotocol *protos, int count)
{
    int32 i = 0;
    UF_DEBUG("ufprotol list: [%d]\r\n", count);
    for (i = 0; i < count; i++) {
        UF_DEBUG("    %s\r\n", protos[i].name);
    }
    g_ufprotocols     = protos;
    g_ufprotocols_cnt = count;
}

