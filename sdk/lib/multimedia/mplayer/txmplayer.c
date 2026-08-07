#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "txmplayer.h"

#define TXMPLAYER_STREAM_MAX (4)

#define URLFILE(file)          (os_strstr(file, "://") != NULL)

extern void *fopen(const char *filename, const char *mode);
extern void fclose(void *stream);
extern size_t fread(void *ptr, size_t size, size_t nmemb, void *stream);
extern int fseek(void *stream, long int offset, int whence);
extern long ftell(void *stream);
extern int feof(void *stream);

extern void *uf_open(char *url, char *mode, uint32 flags);
extern void uf_close(void *file);
extern size_t uf_read(void *ptr, size_t size, size_t nmemb, void *file);
extern int uf_seek(void *file, long int offset, int whence);
extern int uf_eof(void *file);
extern ulong uf_filesize(void *fp, uint32 size);
extern int32 uf_seektype(void *fp);

struct txmplayer_codec {
    uint8 type;
    struct msi *codec;
};

#define TXMPLAYER_HDRDATA_SIZE (2048)
struct txmplayer_stream {
    ulong  total_size;
    void  *file;
    uint8  pause; //暂停读取数据

    void                             *container_ctx;  //container私有数据
    const txmplayer_container        *container;      //关联的container
    const struct txmplayer_input_ops *ops;

    struct txmplayer_codec          codecs[MEDIA_DATA_MAX]; //各路编码流的decoder
    struct txmplayer_audio_info     audio_info;
    struct txmplayer_video_info     video_info;
    struct txmplayer_picture_info   pic_info;
    uint32                          last_input_time;  //最后一次输入数据的时间
};

struct txmplayer {
    struct msi *msi;
    os_mutex_t  lock;
    uint8   pause: 1, mute: 1, init: 1, rev: 4;
    uint8   volume;
    uint8   stream_max;
    int8    msi_stream;
    void   *task_hdl;
    uint32  containers_cnt;
    const txmplayer_container *containers;
    struct txmplayer_stream streams[TXMPLAYER_STREAM_MAX];

    struct framebuff *fb;
    uint32 fb_off;
} g_txmplayer;

size_t msi_read(void *ptr, size_t size, size_t nmemb, void *stream)
{
    uint32 len = 0;

    if (g_txmplayer.fb == NULL) {
        g_txmplayer.fb = msi_get_fb(g_txmplayer.msi, 0);
        g_txmplayer.fb_off = 0;
    }

    if (g_txmplayer.fb) {
        len = size * nmemb;
        if (len == 0) {
            return 0;
        }

        len = min(len, (g_txmplayer.fb->len - g_txmplayer.fb_off));
        hw_memcpy(ptr, g_txmplayer.fb->data + g_txmplayer.fb_off, len);
        g_txmplayer.fb_off += len;
        if (g_txmplayer.fb_off >= g_txmplayer.fb->len) {
            fb_put(g_txmplayer.fb);
            g_txmplayer.fb = NULL;
        }
    }

    return len;
}
void msi_close(void *stream)
{
}
int msi_seek(void *hdl, long int offset, int whence)
{
    return -ENOTSUP;
}
int msi_eof(void *stream)
{
    return 0;
}
const struct txmplayer_input_ops msi_ops = {
    .close = msi_close,
    .read  = msi_read,
    .seek  = msi_seek,
    .eof   = msi_eof,
};

const struct txmplayer_input_ops file_ops = {
    .close = fclose,
    .read  = fread,
    .seek  = fseek,
    .eof   = feof,
};

const struct txmplayer_input_ops uf_ops = {
    .close = uf_close,
    .read  = uf_read,
    .seek  = uf_seek,
    .eof   = uf_eof,
};

static struct msi *txmplayer_find_decoder(uint16 type)
{
    struct msi *pmsi = NULL;
    const char *name = NULL;

    switch (type) {
        case F_H264:
            name = "h264_dec";
            break;
        default:
            break;
    }

    pmsi = msi_find(name, 0);
    if (pmsi == NULL) {
        SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_UNKNOWN_DECODER, type);
    }
    return pmsi;
}

static const txmplayer_container *txmplayer_find_container(uint32 type)
{
    uint32 i = 0;
    for (i = 0; i < g_txmplayer.containers_cnt; i++) {
        if (g_txmplayer.containers[i].type == type) {
            return &g_txmplayer.containers[i];
        }
    }
    SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_UNKNOWN_CONTAINER, type);
    return NULL;
}

static int32 txmplayer_match_container(struct txmplayer_stream *stream, uint8 *data, uint32 len)
{
    uint32 mtype;

    if (stream->container_ctx == NULL) {
        mtype = txmplayer_detect_mtype(data, len); //检测媒体数据类型
        stream->container = txmplayer_find_container(mtype); //根据类型查找container
        if (stream->container == NULL) {
            return RET_ERR;
        }

        stream->container_ctx = stream->container->open(stream->total_size, data, len);
        if (stream->container_ctx == NULL) {
            SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_CONTAINER_OPEN_ERROR, mtype);
            return RET_ERR;
        }
        SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_CONTAINER_MATCH, mtype);
    }

    return RET_OK;
}

static int32 txmplayer_release_stream(uint8 id)
{
    int8 i = 0;
    struct txmplayer_stream *stream = &g_txmplayer.streams[id];

    for (i = 0; i < MEDIA_DATA_MAX; i++) {
        msi_put(stream->codecs[i].codec);
        stream->codecs[i].codec = NULL;
        stream->codecs[i].type = MEDIA_DATA_MAX;
    }

    if (stream->container_ctx) {
        stream->container->close(stream->container_ctx);
    }

    if (stream->ops) {
        stream->ops->close(stream->file);
    }

    if (stream->file == g_txmplayer.msi) {
        g_txmplayer.msi_stream = -1;
    }

    stream->pause = 0;
    stream->last_input_time = 0;
    stream->container_ctx = NULL;
    stream->container = NULL;
    stream->file = NULL;
    stream->ops = NULL;
    return RET_OK;
}

static int32 txmplayer_request_stream(const struct txmplayer_input_ops *ops, void *file)
{
    uint8 i = 0;
    int8  last = -1;

    for (i = 0; i < g_txmplayer.stream_max; i++) {
        if (g_txmplayer.streams[i].file == NULL) {
            g_txmplayer.streams[i].ops = ops;
            g_txmplayer.streams[i].file = file;
            return i;
        }

        if ((last == -1) || (g_txmplayer.streams[i].last_input_time < g_txmplayer.streams[last].last_input_time)) {
            last = i;
        }
    }

    txmplayer_release_stream(last);
    g_txmplayer.streams[last].ops = ops;
    g_txmplayer.streams[last].file = file;
    return last;
}

static int32 txmplayer_check_filesize(uint8 id)
{
    struct txmplayer_stream *stream = &g_txmplayer.streams[id];
    if (stream->total_size == 0) {
        if (stream->ops == &file_ops) {
            stream->ops->seek(stream->file, 0, SEEK_END);
            stream->total_size = ftell(stream->file);
            stream->ops->seek(stream->file, 0, SEEK_SET);
        } else if ((stream->ops == &uf_ops)) {
            stream->total_size = uf_filesize(stream->file, TXMPLAYER_HDRDATA_SIZE);
            if (stream->total_size == 0) { //等待网络数据
                txm_dbg("waitting for connectting ...\r\n");
                return -EAGAIN;
            }
        } else {
            stream->total_size = -1;
        }
        SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_OPEN_SUCCESS, id);
    }
    return RET_OK;
}

static int32 txmplayer_proc_read_data(void)
{
    int8 i = 0;
    int8 more = 0;
    uint32 len;
    struct txmplayer_stream *stream;
    uint8 *buff = NULL;

    for (i = 0; i < g_txmplayer.stream_max; i++) {
        stream = &g_txmplayer.streams[i];
        if (stream->ops && stream->file != g_txmplayer.msi && !stream->pause) {
            if (txmplayer_check_filesize(i) == -EAGAIN) {
                return 0; //等待网络连接成功
            }

            if (stream->container_ctx == NULL) { //匹配container
                buff = os_malloc(TXMPLAYER_HDRDATA_SIZE); //预读header数据,用于检测媒体类型
                if (buff) {
                    len = stream->ops->read(buff, 1, TXMPLAYER_HDRDATA_SIZE, stream->file);
                    if (len > 0) {
                        if (txmplayer_match_container(stream, buff, len)) {
                            txmplayer_release_stream(i);
                            txm_err("unknown media format!\r\n");
                        }
                    }
                    os_free(buff);
                } else {
                    txm_err("no memory!\r\n");
                }
            }

            //执行demux，解析一笔数据
            if (stream->container_ctx && stream->container->demux(stream->container_ctx, stream->ops, stream->file, stream)) {
                stream->last_input_time = os_seconds();
                more = 1;
            }
        }
    }
    return more;
}

//处理通过MSI组件流程输入的fb
static int32 txmplayer_proc_msi_data()
{
    struct framebuff *fb;
    struct txmplayer_stream *stream;

    if (g_txmplayer.msi_stream != -1 && g_txmplayer.streams[g_txmplayer.msi_stream].container_ctx) {
        stream = &g_txmplayer.streams[g_txmplayer.msi_stream];
        if (!stream->pause) {
            return stream->container->demux(stream->container_ctx, stream->ops, stream->file, stream);
        }
        return 0;
    }

    fb = msi_get_fb(g_txmplayer.msi, 0);
    if (fb) {
        if (!g_txmplayer.msi->enable) {
            fb_put(fb);
            txm_err("TXMplayer stopped!\r\n");
            return 1;
        }

        if (g_txmplayer.msi_stream == -1) {
            g_txmplayer.msi_stream = txmplayer_request_stream(&msi_ops, g_txmplayer.msi);
        }

        stream = &g_txmplayer.streams[g_txmplayer.msi_stream];
        stream->last_input_time = os_seconds();
        stream->total_size = -1;

        MEDIA_DATA_CATEGORY dtype = fb_category(fb);
        if (dtype == MEDIA_DATA_MAX) { //未知类型，识别container，进行解封装
            if (txmplayer_match_container(stream, fb->data, fb->len)) {
                txmplayer_release_stream(g_txmplayer.msi_stream);
                txm_err("unknown media format!\r\n");
            }
            fb_put(fb);
        } else { //已知类型，直接传递给decoder解码
            txmplayer_stream_output(stream, fb);
        }
        return 1;
    }

    return 0;
}

static void txmplayer_thread(void *arg)
{
    uint8 more_data = 0;

    while (1) {
        if (!g_txmplayer.pause) {
            os_mutex_lock(&g_txmplayer.lock, osWaitForever);
            more_data |= txmplayer_proc_msi_data();
            more_data |= txmplayer_proc_read_data();
            os_mutex_unlock(&g_txmplayer.lock);
        }

        if (!more_data) {
            os_sleep_ms(10);
        }
        more_data = 0;
    }
}

//container解封装，构建fb之后，执行此函数输出fb。
//该函数根据编码类型:fb->mtype，查找对应的解码模块，输出fb
int32 txmplayer_stream_output(void *priv, struct framebuff *fb)
{
    struct msi *decoder = NULL;
    struct txmplayer_stream *stream = priv;
    MEDIA_DATA_CATEGORY dtype = fb_category(fb);

    if (fb->msi == NULL) {
        msi_get(g_txmplayer.msi);
        fb->msi = g_txmplayer.msi;
    }

    if (dtype == MEDIA_DATA_MAX) {
        fb_put(fb);
        return -ENOTSUP;
    }

    if (fb->mtype != stream->codecs[dtype].type) { //编码类型发生变化，需要重新选择解码模块
        msi_put(stream->codecs[dtype].codec);
        stream->codecs[dtype].codec = NULL;
        stream->codecs[dtype].type = fb->mtype;
    }

    if (stream->codecs[dtype].codec == NULL) {
        stream->codecs[dtype].codec = txmplayer_find_decoder(fb->mtype);
    }

    decoder = stream->codecs[dtype].codec;
    if (decoder && decoder->enable) {
        if (msi_do_cmd(decoder, MSI_CMD_TRANS_FB, (uint32)fb, 0) == RET_OK) {
            if (decoder->fbQ.init) {
                fbq_enqueue(&decoder->fbQ, fb);
            }
        }

        fb_put(fb);
        return RET_OK;
    } else {
        fb_put(fb);
        return -ENOTSUP;
    }
}

static int32 txmplayer_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int32_t ret = RET_OK;

    switch (cmd_id) {
        case MSI_CMD_TRANS_FB:
            break;
        case MSI_CMD_FREE_FB:
            break;
        case MSI_CMD_POST_DESTROY:
            break;
        default:
            break;
    }
    return ret;
}

int32 txmplayer_init(uint8 stream_max, const txmplayer_container *containers, uint32 container_cnt, uint32 stack_size)
{
    ASSERT(containers && container_cnt);
    if (stream_max == 0) stream_max = 1; 
    if (stack_size == 0) stack_size = 1024;

    os_mutex_init(&g_txmplayer.lock);
    g_txmplayer.stream_max = min(stream_max, TXMPLAYER_STREAM_MAX);
    g_txmplayer.containers = containers;
    g_txmplayer.containers_cnt = container_cnt;
    g_txmplayer.msi = msi_new("txmplayer", 256, NULL);
    g_txmplayer.task_hdl = os_task_create("txmplayer", txmplayer_thread, NULL, OS_TASK_PRIORITY_HIGH, 5, NULL, stack_size);
    ASSERT(g_txmplayer.msi && g_txmplayer.task_hdl);
    g_txmplayer.msi->action = (msi_action)txmplayer_msi_action;
    g_txmplayer.msi->enable = 1;
    g_txmplayer.init = 1;
    return RET_OK;
}

int32 txmplayer_deinit()
{
    int8 i = 0;
    struct framebuff *fb;

    ASSERT(g_txmplayer.init);
    g_txmplayer.init = 0;
    g_txmplayer.msi->enable = 0;

    os_mutex_lock(&g_txmplayer.lock, osWaitForever);
    os_task_destroy(g_txmplayer.task_hdl);
    for (i = 0; i < g_txmplayer.stream_max; i++) {
        txmplayer_release_stream(i);
    }

    if (g_txmplayer.fb) {
        fb_put(g_txmplayer.fb);
        g_txmplayer.fb = NULL;
    }

    fb = msi_get_fb(g_txmplayer.msi, 0);
    while (fb) {
        fb_put(fb);
        fb = msi_get_fb(g_txmplayer.msi, 0);
    }
    os_mutex_unlock(&g_txmplayer.lock);

    msi_destroy(g_txmplayer.msi);

    g_txmplayer.pause = 0;
    g_txmplayer.mute = 0;
    g_txmplayer.volume = 0;
    g_txmplayer.msi_stream = -1;
    g_txmplayer.task_hdl = NULL;
    g_txmplayer.msi = NULL;
    SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_PLAY_CLOSE, 0);
    return RET_OK;
}

int32 txmplayer_open(char *file)
{
    int32 ret = -EIO;
    void *file_hdl;
    const struct txmplayer_input_ops *ops;

    ASSERT(g_txmplayer.init);
    if (URLFILE(file)) {
        ops = &uf_ops;
        file_hdl = uf_open(file, "rav", 0);
    } else {
        ops = &file_ops;
        file_hdl = fopen(file, "r");
    }

    if (file_hdl == NULL) {
        txm_err("open %s fail\r\n", file);
        SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_OPEN_FAIL, 0);
        return RET_ERR;
    }

    os_mutex_lock(&g_txmplayer.lock, osWaitForever);
    if (g_txmplayer.init) {
        ret = txmplayer_request_stream(ops, file_hdl);
    }
    os_mutex_unlock(&g_txmplayer.lock);

    if (ret < 0) {
        ops->close(file_hdl);
    } else {
        SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_PLAY_START, ret);
    }
    return ret;
}

int32 txmplayer_stop(uint8 stream_id)
{
    ASSERT(g_txmplayer.init);
    if (stream_id < g_txmplayer.stream_max) {
        os_mutex_lock(&g_txmplayer.lock, osWaitForever);
        if (g_txmplayer.init) {
            txmplayer_release_stream(stream_id);
            SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_PLAY_STOP, stream_id);
        }
        os_mutex_unlock(&g_txmplayer.lock);
        return RET_OK;
    }
    return -EINVAL;
}

int32 txmplayer_pause(uint8 stream_id, uint8 pause)
{
    ASSERT(g_txmplayer.init);
    if (stream_id < g_txmplayer.stream_max) {
        os_mutex_lock(&g_txmplayer.lock, osWaitForever);
        if (g_txmplayer.init) {
            g_txmplayer.streams[stream_id].pause = pause;

            //TBD ..........

            if(pause){
                SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_PLAY_PAUSE, stream_id);
            }else{
                SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_PLAYING, stream_id);
            }
        }
        os_mutex_unlock(&g_txmplayer.lock);
        return RET_OK;
    }
    return -EINVAL;
}

int32 txmplayer_seek(uint8 stream_id, uint32 new_time)
{
    struct txmplayer_stream *stream;
    ASSERT(g_txmplayer.init);

    if (stream_id < g_txmplayer.stream_max) {
        os_mutex_lock(&g_txmplayer.lock, osWaitForever);
        if (g_txmplayer.init) {
            stream = &g_txmplayer.streams[stream_id];
            if (stream->file) {
                if (stream->total_size == (ulong)(-1)) {
                    os_mutex_unlock(&g_txmplayer.lock);
                    SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_PLAY_SEEK_ERR, stream_id);
                    txm_err("Livestream, Can not seek!\r\n");
                    return -ENOTSUP;
                } else {
                    if (stream->ops == &uf_ops && uf_seektype(stream->file) == 1) { //time seek: 直接执行seek操作
                        stream->ops->seek(stream->file, new_time, SEEK_SET);
                    } else if (stream->container) { //由container执行seek
                        stream->container->seek(stream->container_ctx, new_time);
                    }
                }
            }
        }
        os_mutex_unlock(&g_txmplayer.lock);
        return RET_OK;
    }
    return -EINVAL;
}

int32 txmplayer_set_speed(uint8 stream_id, MEDIA_PLAY_SPEED speed)
{
    ASSERT(g_txmplayer.init);
    os_mutex_lock(&g_txmplayer.lock, osWaitForever);
    //TBD ..........
    os_mutex_unlock(&g_txmplayer.lock);
	return RET_OK;
}

int32 txmplayer_set_volume(uint8 stream_id, uint8 volume)
{
    ASSERT(g_txmplayer.init);
    os_mutex_lock(&g_txmplayer.lock, osWaitForever);
    //TBD ..........
    os_mutex_unlock(&g_txmplayer.lock);
	return RET_OK;
}

