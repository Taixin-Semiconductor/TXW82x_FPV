#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/txmplayer.h"

#define txm_dbg(fmt, ...)      os_printf("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define txm_err(fmt, ...)      os_printf(KERN_ERR"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define txm_warn(fmt, ...)     os_printf(KERN_WARNING"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#if URLFILE_ENABLE
#define URLFILE(file)         (os_strstr(file, "://") != NULL)
#endif

#define TXMPLAYER_STREAM_MAX   (8)     //支持8个通路
#define TXMPLAYER_HDRDATA_SIZE (1024)  //媒体识别时预读取的数据长度
#define TXMPLAYER_UNKOWN_MAX   (8)

#define TXMPLAYER_PLAYING(s)   ((s) == (TXMPLAYER_STATE_BUFFERING) || \
                                (s) == (TXMPLAYER_STATE_PLAYING) || \
                                (s) == (TXMPLAYER_STATE_SEEKING))

extern void *fopen(const char *filename, const char *mode);
extern void fclose(void *stream);
extern size_t fread(void *ptr, size_t size, size_t nmemb, void *stream);
extern int fseek(void *stream, off_t offset, int whence);
extern off_t ftell(void *stream);
extern int feof(void *stream);

extern void *uf_open(char *url, char *mode, uint32 flags);
extern void uf_close(void *file);
extern size_t uf_read(void *ptr, size_t size, size_t nmemb, void *file);
extern int uf_seek(void *file, off_t offset, int whence);
extern int uf_eof(void *file);
extern off_t uf_filesize(void *fp, uint32 size);
extern uint32 uf_datasize(void *fp, uint8 *buffer_level, uint8 *dof);
extern int32 uf_seektype(void *fp);

struct txmplayer_codec {
    uint8 stype, unknow;
    struct msi *codec;
};

struct txmplayer_stream {
    struct msi *msi;
    struct msi *avsync;   //AVSync的参考组件
    struct msi *vdd;      //虚拟屏显示通道： video/picture 共用一个vdd通道
    struct msi *mixer;    //混音器通道
    void  *file;  //数据源句柄：file句柄 或 urlfile句柄
    uint8  id;
    uint8  state;
    uint8  eof: 1, fst_data: 1, unknow_container: 1, preview: 1, rev: 4;
    uint8  avsync_type;   //AVSync类型: enum TXMPLAYER_AVSYNC
    uint8  buflevel;
    uint32 total_duration;
    int64  total_size;
    uint32 last_input_time;
    uint32 seeking_time;

    struct vdd_rect vddrect;

    struct txmplayer_param param;  //每个播放通路的参数
    struct framebuff *dec_fb;      //输出给decoder失败时，挂起的fb

    //MSI通道专用
    struct framebuff *rd_fb;
    uint32            rdfb_off;
    atomic_t          msi_totlen;

    void                           *container_obj;  //container实例化对象
    const struct AVDemuxer         *container;      //关联的container
    const struct AVDemuxerOps      *ops;

    struct txmplayer_codec          codecs[TXMPLAYER_CODEC_COUNT];     //各路编码流的decoder
};

struct txmplayer {
    os_mutex_t  lock;
    atomic_t    init;
    uint8       cur_id;
    struct msi *msi;
    void       *task_hdl;
    struct txmplayer_param   global_param;  //播放器全局参数
    struct txmplayer_stream *streams[TXMPLAYER_STREAM_MAX];
} g_txmplayer;

static const char *txmp_names[TXMPLAYER_STREAM_MAX] = {
    "txmp#1", "txmp#2", "txmp#3", "txmp#4", "txmp#5", "txmp#6", "txmp#7", "txmp#8"
};

//播放器默认参数
static const struct txmplayer_param txmplayer_param_def = {
    .fbQ_size       = 16,   // framebuff 队列大小
    .volume         = 100,
    .netbuf_size    = 256 * 1024, // 网络缓冲区
};

static int32 txmplayer_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2);
static int32 txmplayer_demuxer_ioctl(struct msi *owner, uint32 cmd, uint32 param1, uint32 param2);
static int32 txpmlayer_smoothness_first(struct txmplayer_stream *stream);

static void txmplayer_update_param(struct txmplayer_param *param, struct txmplayer_param *new_p)
{
    if (new_p == NULL) { return; }
    param->volume = new_p->volume;
    if (new_p->fbQ_size) { param->fbQ_size = new_p->fbQ_size; }
    if (new_p->mix_mode) { param->mix_mode = new_p->mix_mode; }
    if (new_p->mix_volume) { param->mix_volume = new_p->mix_volume; }
    if (new_p->netbuf_size) { param->netbuf_size = new_p->netbuf_size; }
    if (new_p->smoothness_first) { param->smoothness_first = new_p->smoothness_first; }
    if (new_p->decoder[0]) { param->decoder[0] = new_p->decoder[0]; }
    if (new_p->decoder[1]) { param->decoder[1] = new_p->decoder[1]; }
    if (new_p->decoder[2]) { param->decoder[2] = new_p->decoder[2]; }
    if (new_p->decoder[3]) { param->decoder[3] = new_p->decoder[3]; }
}

static const char *txmplayer_status_text(uint8 stream_id)
{
    if (stream_id < TXMPLAYER_STREAM_MAX) {
        struct txmplayer_stream *stream = g_txmplayer.streams[stream_id];
        switch (stream->state) {
            case TXMPLAYER_STATE_BUFFERING:
                return  "BUFFERING";
            case TXMPLAYER_STATE_PAUSE:
                return  "PAUSE";
            case TXMPLAYER_STATE_PLAY_END:
                return  "PLAY_END";
            case TXMPLAYER_STATE_PLAYING:
                if (stream->unknow_container) { return "PLAYING - unknow_demuxer"; }
                return  "PLAYING";
            case TXMPLAYER_STATE_DECODE_ERR:
                return  "DECODE_ERROR";
            case TXMPLAYER_STATE_OPEN_FAIL:
                return  "OPEN_FAIL";
            case TXMPLAYER_STATE_SEEKING:
                return  "SEEKING";
            default:
                break;
        }
    }
    return "CLOSED";
}

static void txmplayer_set_avsync(struct txmplayer_stream *stream)
{
    switch (stream->avsync_type) {
        case TXMPLAYER_AVSYNC_AUDIO:
            if (stream->codecs[MEDIA_DATA_AUDIO].codec) {
                stream->avsync = stream->codecs[MEDIA_DATA_AUDIO].codec;
                msi_do_cmd(stream->codecs[MEDIA_DATA_VIDEO].codec, MSI_CMD_AVSYNC_MASTER, (uint32)stream->codecs[MEDIA_DATA_AUDIO].codec, 0);
                msi_do_cmd(stream->codecs[MEDIA_DATA_SUBTITLE].codec, MSI_CMD_AVSYNC_MASTER, (uint32)stream->codecs[MEDIA_DATA_AUDIO].codec, 0);
            }
            break;
        case TXMPLAYER_AVSYNC_VIDEO:
            if (stream->codecs[MEDIA_DATA_VIDEO].codec) {
                stream->avsync = stream->codecs[MEDIA_DATA_VIDEO].codec;
                msi_do_cmd(stream->codecs[MEDIA_DATA_AUDIO].codec, MSI_CMD_AVSYNC_MASTER, (uint32)stream->codecs[MEDIA_DATA_VIDEO].codec, 0);
                msi_do_cmd(stream->codecs[MEDIA_DATA_SUBTITLE].codec, MSI_CMD_AVSYNC_MASTER, (uint32)stream->codecs[MEDIA_DATA_VIDEO].codec, 0);
            }
            break;
        case TXMPLAYER_AVSYNC_CLOCK:
            stream->avsync = NULL;
            msi_do_cmd(stream->codecs[MEDIA_DATA_VIDEO].codec, MSI_CMD_AVSYNC_MASTER, 0, 0);
            msi_do_cmd(stream->codecs[MEDIA_DATA_AUDIO].codec, MSI_CMD_AVSYNC_MASTER, 0, 0);
            msi_do_cmd(stream->codecs[MEDIA_DATA_SUBTITLE].codec, MSI_CMD_AVSYNC_MASTER, 0, 0);
            break;
    }
}

//设置解码器的输出
static void txmplayer_set_decoder_output(struct txmplayer_stream *stream, uint8 mtype, struct msi *decoder, struct msi *old_codec)
{
    if (mtype == MEDIA_DATA_AUDIO) {
        if (old_codec) { //切换音轨
            msi_do_cmd(decoder, MSI_CMD_FOLLOW_OUTPUT, (uint32)old_codec, 0);
        } else {
            if (stream->mixer == NULL) {
                stream->mixer = msi_find2(MIXER_MSI, 0, 0, 0);
            }
            msi_add_output(decoder, NULL, stream->mixer, NULL);
        }
    } else if (mtype == MEDIA_DATA_VIDEO || mtype == MEDIA_DATA_PICTURE) {
        // video/图片输出到 VDD(虚拟屏)
        if (stream->vdd == NULL) {
            stream->vdd = msi_find2(VDD_MSI, 0, 0, 0);
        }
        if (stream->vdd) {
            msi_do_cmd(stream->vdd, MSI_CMD_SET_VDD_RECT, (uint32)&stream->vddrect, 0);
            msi_do_cmd(stream->vdd, MSI_CMD_SET_VDD_ONOFF, 1, 0);
            msi_add_output(decoder, NULL, stream->vdd, NULL);
        } else {
            msi_add_output(decoder, NULL, NULL, "VIDEO_P0");
            msi_cmd2(decoder, MSI_CMD_LCD_VIDEO, 0, 1);
            txm_err("find vdd fail!\r\n");
        }
    } else if (mtype == MEDIA_DATA_SUBTITLE) {
        txm_err("subtitle no dec output!\r\n");
    }
}

static int32 txmplayer_find_decoder(struct txmplayer_stream *stream, struct framebuff *fb, struct msi *old_codec)
{
    struct msi *msi = NULL;
    uint16_t codec_type = (fb->mtype << 8 | fb->stype);

    if (stream->param.decoder[fb->mtype]) {
        msi = msi_find2(stream->param.decoder[fb->mtype], 0, 1, fb->codec_info);
    }

    if (msi == NULL) {
        msi = msi_find2(NULL, codec_type, 1, fb->codec_info);
    }

    if (msi == NULL) {
        stream->codecs[fb->mtype].unknow++;
        if (stream->codecs[fb->mtype].unknow == TXMPLAYER_UNKOWN_MAX) {
            txm_err("can not find decoder for type:%x\r\n", codec_type);
            SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_UNKNOWN_DECODER, codec_type << 8 | stream->id);
        }
        return RET_ERR;
    } else {
        stream->codecs[fb->mtype].codec  = msi;
        stream->codecs[fb->mtype].unknow = 0;
        stream->codecs[fb->mtype].stype  = fb->stype;
        msi_add_output(stream->msi, NULL, msi, NULL);
        txmplayer_set_decoder_output(stream, fb->mtype, msi, old_codec);
        msi_cmd2(msi, MSI_CMD_EOF, 0, 0);
        msi_cmd2(msi, MSI_CMD_START, 0, 0);
        msi_cmd2(msi, MSI_CMD_SET_VOLUME, stream->param.volume, 0);
        msi_cmd2(msi, MSI_CMD_SET_MIXER_MODE, stream->param.mix_mode, stream->param.mix_volume);
        txmplayer_set_avsync(stream);
        return RET_OK;
    }
}

static void txmplayer_out_fb(struct msi *owner, struct framebuff *fb)
{
    struct txmplayer_stream *stream = (struct txmplayer_stream *)owner->priv;
    uint8 mtype = fb->mtype;
    struct msi *old_codec = NULL;

    stream->dec_fb = NULL;
    stream->last_input_time = os_seconds();
    if (stream->state == TXMPLAYER_STATE_CLOSE) {
        txm_err("drop data\r\n");
        fb_put(fb);
        return;
    }

    // 预览模式，丢弃非视频帧
    if (stream->preview && mtype != MEDIA_DATA_VIDEO) {
        txm_dbg("preview! drop data\r\n");
        fb_put(fb);
        return;
    }

    if (stream->container && stream->container_obj) {
        uint32_t latest_duration = 0; //demuxer可能在动态修正total_duration
        stream->container->ioctl(stream->container_obj, AVDEMUXER_GET_TOTAL_DURATION, (uint32)&latest_duration, 0);
        if (latest_duration > 0) {
            stream->total_duration = latest_duration;
        }
    }

    if (stream->fst_data == 0) {
        stream->fst_data = 1;
        txm_warn("demux first data! time:%d, duration：%d\r\n", fb->time / 1000, stream->total_duration / 1000);
        if (!stream->preview && txpmlayer_smoothness_first(stream)) { //平顺优先：先缓存再播放
            stream->dec_fb = fb;
            return;
        }
    }

    if(fb->mtype == MEDIA_DATA_AUDIO && stream->param.volume == 0){
        fb_put(fb);
        return;
    }

    if (fb->mtype == MEDIA_DATA_UNKNOWN || mtype > (TXMPLAYER_CODEC_COUNT - 1)) {
        fb_put(fb);
        txm_err("not support media type! mtype=%d, stype=%d\r\n", mtype, fb->stype);
        return;
    }

    //编码类型发生变化，需要重新选择解码模块
    if (fb->stype != stream->codecs[mtype].stype && stream->codecs[mtype].codec) {
        old_codec = stream->codecs[mtype].codec;
        msi_del_output(stream->msi, NULL, old_codec, NULL);
        stream->codecs[mtype].codec = NULL;
        stream->codecs[mtype].unknow = 0;
    }

    if (stream->codecs[mtype].codec == NULL) {
        if (stream->codecs[fb->mtype].unknow >= TXMPLAYER_UNKOWN_MAX) {
            fb_put(fb);
            msi_put(old_codec);
            return;
        }
        if (txmplayer_find_decoder(stream, fb, old_codec)) {
            fb_put(fb);
            msi_put(old_codec);
            return;
        }
    }

    msi_put(old_codec);
    fb->last = stream->eof;
    if (fb->msi == NULL) {
        fb->msi = msi_get(stream->msi);
    }

    if (msi_output_fb(stream->msi, fb, 1) == 0) {
        stream->dec_fb = fb; //fb未被接收，可能接收队列已满: 挂起fb
    }
}

static size_t msi_read(void *ptr, size_t size, size_t nmemb, void *file)
{
    uint32 len = 0;
    uint32 rd_size = 0;
    uint32 rd_tot = 0;
    struct txmplayer_stream *stream = (struct txmplayer_stream *)file;

    len = size * nmemb;
    while (rd_tot < len) {
        if (stream->rd_fb == NULL) {
            stream->rd_fb = msi_get_fb(stream->msi, 0);
            stream->rdfb_off = 0;
            stream->eof = 0;
        }

        if (stream->rd_fb == NULL) {
            return rd_tot;
        }

        rd_size = len - rd_tot;
        rd_size = min(rd_size, (stream->rd_fb->len - stream->rdfb_off));
        if (ptr) {
            os_memcpy(ptr + rd_tot, stream->rd_fb->data + stream->rdfb_off, rd_size);
        }
        stream->rdfb_off += rd_size;
        if (stream->rdfb_off >= stream->rd_fb->len) {
            if (stream->rd_fb->last) {
                stream->eof = 1;
            }
            fb_put(stream->rd_fb);
            stream->rd_fb = NULL;
        }

        rd_tot += rd_size;
        atomic_sub(&stream->msi_totlen, rd_size);
    }

    return rd_tot;
}
static void msi_close(void *file)
{
}
static int msi_seek(void *file, off_t offset, int whence)
{
    return -ENOTSUP;
}
static int msi_eof(void *file)
{
    struct txmplayer_stream *stream = (struct txmplayer_stream *)file;
    return stream->eof;
}
static int msi_avail(void *file)
{
    struct txmplayer_stream *stream = (struct txmplayer_stream *)file;
    return atomic_read(&stream->msi_totlen);
}
static int uf_avail(void *file)
{
    return uf_datasize(file, NULL, NULL);
}
static int file_avail(void *file)
{
    return 0xfffffff;
}
static size_t file_read(void *ptr, size_t size, size_t nmemb, void *hdl)
{
    if (ptr) {
        return fread(ptr, size, nmemb, hdl);
    } else {
        //txm_dbg("file offset %d, skip %d bytes\r\n", ftell(hdl), size*nmemb);
        fseek(hdl, size * nmemb, SEEK_CUR);
        return size * nmemb;
    }
}

static const struct AVDemuxerOps msi_ops = {
    .close = msi_close,
    .read  = msi_read,
    .seek  = msi_seek,
    .eof   = msi_eof,
    .outFB = txmplayer_out_fb,
    .avail = msi_avail,
    .ioctl = txmplayer_demuxer_ioctl,
};
static const struct AVDemuxerOps file_ops = {
    .close = fclose,
    .read  = file_read,
    .seek  = fseek,
    .eof   = feof,
    .avail = file_avail,
    .outFB = txmplayer_out_fb,
    .ioctl = txmplayer_demuxer_ioctl,
};
static const struct AVDemuxerOps uf_ops = {
#ifdef URLFILE
    .close = uf_close,
    .read  = uf_read,
    .seek  = uf_seek,
    .eof   = uf_eof,
    .avail = uf_avail,
    .outFB = txmplayer_out_fb,
    .ioctl = txmplayer_demuxer_ioctl,
#endif
};

static int32 txpmlayer_smoothness_first(struct txmplayer_stream *stream)
{
    if (stream->ops == &uf_ops && stream->param.smoothness_first) { //平顺优先：先缓存再播放
        uint8 buflevel;
        uf_datasize(stream->file, &buflevel, NULL);
        if (buflevel < 90) {
            txm_warn("smoothness first, buffering enough data!\r\n");
            return 1;
        }
    }
    return 0;
}

static int32 txmplayer_demuxer_ioctl(struct msi *owner, uint32 cmd, uint32 param1, uint32 param2)
{
    int32 ret = RET_OK;
    struct txmplayer_stream *stream = (struct txmplayer_stream *)owner->priv;
    switch (cmd) {
        case AVDEMUXER_GET_FILE_SIZE:
            if (param1) { *(int64 *)param1 = stream->total_size; }
            break;
        case AVDEMUXER_GET_STREAM_TYPE:
            if (stream->ops == &file_ops) {
                ret = AVDEMUXER_STREAM_FILE;
            } else {
                ret = stream->total_size > 0 ? AVDEMUXER_STREAM_URLFILE : AVDEMUXER_STREAM_LIVE_STREAM;
            }
            break;
        case AVDEMUXER_GET_BUF_SIZE:
            //*(uint32 *)param1 = stream->param.netbuf_size;
            break;
    }
    return ret;
}

static int32 txmplayer_match_container(struct txmplayer_stream *stream, uint8 *data, uint32 len)
{
    uint32 container_type;

    if (stream->container_obj == NULL) {
        container_type = detect_container_type(data, len);  //检测媒体数据类型
        stream->container = AVDemuxer_Get(container_type);  //根据类型查找container
        if (stream->container == NULL) {
            if (!stream->unknow_container) {
                txm_err("Unsupport container type %d !\r\n", container_type);
                stream->unknow_container = 1;
                SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_UNKNOWN_CONTAINER,  container_type << 8 | stream->id);
            }
            return RET_ERR;
        }

        stream->container_obj = stream->container->init(stream->file, stream->ops, data, len, stream->msi);
        if (stream->container_obj == NULL) {
            SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_CONTAINER_OPEN_ERROR, container_type << 8 | stream->id);
            return RET_ERR;
        }

        stream->unknow_container = 0;
        txm_warn("match container %s !\r\n", stream->container->name);
        SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_CONTAINER_MATCH, container_type << 8 | stream->id);
    }

    return RET_OK;
}
static void txmplayer_stop_clear(struct txmplayer_stream *stream)
{
    fb_put(stream->rd_fb);
    fb_put(stream->dec_fb);
    stream->rd_fb  = NULL;
    stream->dec_fb = NULL;
    msi_clear(stream->msi);
    msi_discard_fb(stream->msi, NULL, stream->msi);
}
static void txmplayer_stop_play(struct txmplayer_stream *stream)
{
    if (stream->container_obj) {
        stream->container->release(stream->container_obj);
        stream->container_obj = NULL;
    }

    msi_cmd2(stream->msi, MSI_CMD_LCD_VIDEO, 0, 0);
    msi_output_cmd(stream->vdd, MSI_CMD_SET_VDD_ONOFF, 0, 0);
    txmplayer_stop_clear(stream);
    stream->state = TXMPLAYER_STATE_CLOSE;
}

static int32 txmplayer_release_stream(struct txmplayer_stream *stream)
{
    if (stream->ops) {
        stream->ops->close(stream->file);
        stream->ops = NULL;
    }

    for (int i = 0; i < TXMPLAYER_CODEC_COUNT; i++) {
        stream->codecs[i].unknow = 0;
        stream->codecs[i].stype = 0xff;
        msi_put(stream->codecs[i].codec);
        stream->codecs[i].codec = NULL;
    }

    msi_put(stream->vdd);
    stream->vdd = NULL;

    msi_put(stream->mixer);
    stream->mixer = NULL;

    g_txmplayer.streams[stream->id] = NULL;
    os_free(stream);
    return RET_OK;
}

static int32 txmplayer_open_stream(struct txmplayer_stream *stream, void *file)
{
    void *file_hdl;
    const struct AVDemuxerOps *ops;

#ifdef URLFILE
    if (URLFILE(file)) {
        uint32 netbuf_size = stream->param.netbuf_size;
        netbuf_size = (netbuf_size / 1024);
        if (netbuf_size < 8) { netbuf_size = 8; }
        uint32 flags = netbuf_size;
        ops = &uf_ops;
        file_hdl = uf_open(file, "rav", flags);
    } else {
        ops = &file_ops;
        file_hdl = fopen(file, "r");
    }
#else
    ops = &file_ops;
    file_hdl = fopen(file, "r");
#endif

    if (file_hdl == NULL) {
        txm_err("open %s fail\r\n", file);
        SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_OPEN_FAIL, stream->id);
        return RET_ERR;
    }

    SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_PLAY_START, stream->id);
    txm_warn("txmplayer open %s, stream %d\r\n", file, stream->id);

    stream->ops  = ops;
    stream->file = file_hdl;
    stream->last_input_time = os_seconds();
    return RET_OK;
}

static struct txmplayer_stream *txmplayer_request_stream()
{
    uint8 id = TXMPLAYER_STREAM_MAX;
    struct msi *msi;
    struct txmplayer_stream *stream;
    uint8 i = g_txmplayer.cur_id;

    do{
        if (g_txmplayer.streams[i] == NULL) {
            id = i;
            break;
        }
        i++;
        if(i >= TXMPLAYER_STREAM_MAX) i = 0;
    }while(i != g_txmplayer.cur_id);

    if (id >= TXMPLAYER_STREAM_MAX) {
        txm_err("txmplayer: busy! no more free channel\r\n");
        return NULL;
    }

    stream = os_zalloc(sizeof(struct txmplayer_stream));
    if (stream == NULL) {
        txm_err("no memory\r\n");
        return NULL;
    }

    os_memcpy(&stream->param, &g_txmplayer.global_param, sizeof(struct txmplayer_param));

    msi = msi_new(txmp_names[id], stream->param.fbQ_size, NULL);
    if (msi == NULL) {
        txm_err("no mem, msi new fail!\r\n");
        return NULL;
    }

    stream->state = TXMPLAYER_STATE_PLAYING;
    stream->msi   = msi;
    stream->id    = id;
    stream->last_input_time = os_seconds();

    //framebuff数量限制，以及分配方式
    msi->fb_alloc = (malloc_cb_t)decoder_mem_alloc;
    msi->fb_free  = (mfree_cb_t)decoder_mem_free;
    msi->fb_limits.counter = stream->param.fbQ_size;

    msi->priv   = stream;
    msi->action = (msi_action)txmplayer_msi_action;
    msi->mgr    = g_txmplayer.msi;
    msi->enable = 1;
    g_txmplayer.streams[id] = stream;
    g_txmplayer.cur_id = id;

    txm_warn("txmplayer: new channel %s\r\n", txmp_names[id]);
    return stream;
}

static int32 txmplayer_check_filesize(struct txmplayer_stream *stream)
{
    if (stream->total_size == 0) {
        if (stream->ops == &file_ops) {
            stream->ops->seek(stream->file, 0, SEEK_END);
            stream->total_size = ftell(stream->file);
            stream->ops->seek(stream->file, 0, SEEK_SET);
            if (stream->total_size == 0) { stream->eof = 1; }
        } else if ((stream->ops == &uf_ops)) { //网络流，需要等待网络连接成功
            stream->total_size = uf_filesize(stream->file, TXMPLAYER_HDRDATA_SIZE);
            if (stream->total_size == 0) { //等待网络数据
                //txm_dbg("waitting for connectting ...\r\n");
                if (os_seconds() - stream->last_input_time > 10) { //超过10秒未接收到数据
                    SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_OPEN_FAIL, stream->id);
                    stream->state = TXMPLAYER_STATE_OPEN_FAIL;
                }
                return -EAGAIN;
            }
        } else {
            stream->total_size = -1;
        }
        txm_warn("open success, total_size:%d\r\n", stream->total_size);
        SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_OPEN_SUCCESS, stream->id);
    }
    return RET_OK;
}

//处理通过MSI组件流程输入的fb
static int32 txmplayer_proc_msi_data(struct txmplayer_stream *stream)
{
    if (stream->container_obj) {
        return stream->container->do_demux(stream->container_obj);
    } else {
        struct framebuff *fb = msi_get_fb(stream->msi, 0);
        if (fb) {
            stream->total_size = -1;

            if (fb->mtype == MEDIA_DATA_UNKNOWN) { //未知类型，识别container，进行解封装
                if (txmplayer_match_container(stream, fb->data, fb->len)) {
                    txm_err("unknown media format!\r\n");
                }
                fb_put(fb);
            } else { //已知类型，直接传递给decoder解码
                txmplayer_out_fb(stream->msi, fb);
            }

            return fbq_count(&stream->msi->fbQ);
        }
    }
    return 0;
}
static int32 txmplayer_get_buffer_level(struct txmplayer_stream *stream)
{
    uint8 uf_eod = 0;
    uint8 uf_level = 0;
    uf_datasize(stream->file, &uf_level, &uf_eod);
    return uf_eod ? 100 : uf_level;
}

static int32 txmplayer_check_buffer_level(struct txmplayer_stream *stream)
{
    if (stream->state == TXMPLAYER_STATE_BUFFERING && stream->ops == &uf_ops) {
        uint8 buf_level = txmplayer_get_buffer_level(stream);
        if (stream->buflevel != buf_level) {
            stream->buflevel = buf_level; //提示缓冲比例
            SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_BUFFERING_DATA, (buf_level << 8 | stream->id));
        }
        if (buf_level >= 95) { //缓冲区已满，继续播放
            msi_cmd2(stream->msi, MSI_CMD_START, 0, 0);
            if (stream->container_obj && stream->container->ioctl) {
                stream->container->ioctl(stream->container_obj, AVDEMUXER_SET_BUFFERING, 0, 0);
            }
            txm_dbg("start! buf_level: %d\r\n", buf_level);
        }
    }
    return 0;
}

static int32 txmplayer_proc_stream_data(void)
{
    int8 i = 0;
    int8 more = 0;
    uint32 len;
    struct txmplayer_stream *stream;
    uint8 *buff = NULL;

    for (i = 0; i < TXMPLAYER_STREAM_MAX; i++) {
        stream = g_txmplayer.streams[i];
        if (stream == NULL || stream->ops == NULL || !TXMPLAYER_PLAYING(stream->state)) {
            continue;
        }

        if (stream->unknow_container && stream->ops != &msi_ops) {
            continue;
        }

        if (stream->dec_fb) {
            txmplayer_out_fb(stream->msi, stream->dec_fb);
            if (stream->dec_fb) {
                continue;
            }
        }

        if (stream->eof) {
            continue;
        }

        if (stream->preview && stream->fst_data) {
            continue;
        }

        if (stream->ops == &msi_ops) {
            int32 ret = txmplayer_proc_msi_data(stream);
            if (ret > 0 || ret == -EAGAIN) {
                more |= 1;
            }
        } else {
            if (txmplayer_check_filesize(stream) == -EAGAIN) {
                continue; //等待网络连接成功
            }

            txmplayer_check_buffer_level(stream);
            if (stream->container_obj == NULL) { //匹配container
                buff = os_malloc(TXMPLAYER_HDRDATA_SIZE); //预读header数据,用于检测媒体类型
                if (buff) {
                    len = stream->ops->read(buff, 1, TXMPLAYER_HDRDATA_SIZE, stream->file);
                    if (len > 0) {
                        if (txmplayer_match_container(stream, buff, len)) {
                            txm_err("unknown media format!\r\n");
                        }
                    }
                    os_free(buff);
                } else {
                    txm_err("no memory!\r\n");
                }
            }

            if (stream->container_obj) {
                int32 ret = stream->container->do_demux(stream->container_obj);
                if (ret > 0 || ret == -EAGAIN) {
                    more |= 1;
                } else if (ret == 0 && !stream->eof && stream->ops->eof(stream->file)) {
                    stream->eof = 1;
                    msi_output_cmd(stream->msi, MSI_CMD_EOF, 1, 0);
                    if (stream->state == TXMPLAYER_STATE_SEEKING) {
                        msi_do_cmd(stream->msi, MSI_CMD_PLAY_END, 0, 0);
                    }
                    txm_warn("%s: EOF\r\n", stream->msi->name);
                }
            }
        }
    }
    return more;
}

static void txmplayer_thread(void *arg)
{
    uint8 more_data = 0;

    while (1) {
        os_mutex_lock(&g_txmplayer.lock, osWaitForever);
        more_data = txmplayer_proc_stream_data();
        os_mutex_unlock(&g_txmplayer.lock);
        if (!more_data) {
            os_sleep_ms(5);
        }
    }
}

static struct msi *txmplayer_new_chan()
{
    os_mutex_lock(&g_txmplayer.lock, osWaitForever);
    struct txmplayer_stream *stream = txmplayer_request_stream();
    if (stream) {
        stream->ops  = &msi_ops;
        stream->file = stream;
    }
    os_mutex_unlock(&g_txmplayer.lock);
    return stream ? stream->msi : NULL;
}

static int32 txmplayer_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int32 ret = RET_OK;
    struct framebuff *fb;
    struct txmplayer_stream *stream;

    if (msi->chan_mgr || msi == g_txmplayer.msi) {
        if (cmd_id == MSI_CMD_NEW_CHANNEL) {
            *(struct msi **)param2 = txmplayer_new_chan();
        }
        return 0;
    }

    stream = (struct txmplayer_stream *)msi->priv;
    switch (cmd_id) {
        case MSI_CMD_START:
            stream->eof = 0;
            stream->state = TXMPLAYER_STATE_PLAYING;
            msi_output_cmd(stream->vdd, MSI_CMD_SET_VDD_ONOFF, 1, 0);
            SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_PLAY_START, stream->id);
            break;
        case MSI_CMD_STOP:
            txmplayer_stop_play(stream);
            SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_PLAY_STOP, stream->id);
            break;
        case MSI_CMD_CLEAR:
            txmplayer_stop_clear(stream);
            break;
        case MSI_CMD_PAUSE:
            stream->state = TXMPLAYER_STATE_PAUSE;
            SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_PLAY_PAUSE, stream->id);
            break;

        case MSI_CMD_NEED_MORE_DATA:
            if (stream->ops == &uf_ops && !stream->eof && stream->state != TXMPLAYER_STATE_BUFFERING) {
                uint8 buf_level = txmplayer_get_buffer_level(stream);
                if (buf_level < 10) {
                    txm_warn("%s buffering data ...\r\n", txmp_names[stream->id]);
                    stream->state = TXMPLAYER_STATE_BUFFERING;
                    if (stream->container_obj && stream->container->ioctl) {
                        stream->container->ioctl(stream->container_obj, AVDEMUXER_SET_BUFFERING, 1, 0);
                    }
                    SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_BUFFERING_DATA, stream->id); //提示开始缓冲数据
                }
            }
            break;

        case MSI_CMD_GET_PLAYTIME:
            msi_cmd2(stream->avsync, MSI_CMD_GET_PLAYTIME, param1, 0);
            break;

        case MSI_CMD_SET_PLAYER_PARAM:
            if (param1) {
                os_memcpy(&stream->param, (void *)param1, sizeof(struct txmplayer_param));
            }
            break;

        case MSI_CMD_PLAY_END:
            stream->state = TXMPLAYER_STATE_PLAY_END;
            msi_output_cmd(stream->vdd, MSI_CMD_SET_VDD_ONOFF, 0, 0);
            txm_warn("%s PLAY END\r\n", txmp_names[stream->id]);
            SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_PLAY_END, stream->id); //提示播放完成
            break;

        case MSI_CMD_POST_DESTROY:
            txm_warn("relase stream %d [%s]\r\n", stream->id, stream->msi->name);
            txmplayer_release_stream(stream);
            break;
        case MSI_CMD_DECODE_ERR:
            stream->state = TXMPLAYER_STATE_DECODE_ERR;
            SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_DECODER_ERROR, param1 << 8 | stream->id);
            break;
        case MSI_CMD_SET_MIXER_MODE:
            stream->param.mix_mode   = param1;
            stream->param.mix_volume = param2;
            break;

        case MSI_CMD_TRANS_FB:
            if (stream->ops == &msi_ops && stream->eof) {
                stream->eof  = 0;
                msi_output_cmd(stream->msi, MSI_CMD_EOF, 0, 0);
            }
            atomic_add(&stream->msi_totlen, ((struct framebuff *)param1)->len);
            break;

        case MSI_CMD_FREE_FB:
            fb = (struct framebuff *)param1;
            if (atomic_read(&stream->msi->fb_limits) == stream->param.fbQ_size && stream->eof) {
                msi_output_cmd(stream->msi, MSI_CMD_EOF, 1, 0);
                txm_warn("%s: EOF\r\n", stream->msi->name);
            }
            if(stream->preview && fb->mtype == MEDIA_DATA_VIDEO){
                //预留模式仅解码一笔数据，释放demuxer资源
                if (stream->container_obj) {
                    stream->container->release(stream->container_obj);
                    stream->container_obj = NULL;
                }
                stream->state = TXMPLAYER_STATE_PLAY_END;
                SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_PLAY_END, stream->id); //提示播放完成
                txm_warn("preview done!\r\n");
            }
            
            if(stream->state == TXMPLAYER_STATE_SEEKING){
                stream->state = TXMPLAYER_STATE_PLAYING;
            }
            break;

        case MSI_CMD_SET_VDD_RECT:
            os_memcpy(&stream->vddrect, (uint8 *)param1, sizeof(struct vdd_rect));
            break;
        default:
            break;
    }
    return ret;
}

int32 txmplayer_init(uint32 stack_size, uint32 task_pri, struct txmplayer_param *param)
{
    if (atomic_inc2_return(&g_txmplayer.init)) {
        txm_warn("TXMplayer init count %d\r\n", atomic_read(&g_txmplayer.init));
        return RET_OK;
    }

    g_txmplayer.msi = msi_new(TXMPLAYER_MSI, 0, NULL);
    if (g_txmplayer.msi == NULL) {
        txm_err("no memory!\r\n");
        return -ENOMEM;
    }

    if (task_pri == 0)   { task_pri = OS_TASK_PRIORITY_NORMAL; }
    if (stack_size == 0) { stack_size = 2048; }

    os_mutex_init(&g_txmplayer.lock);
    os_memcpy(&g_txmplayer.global_param, &txmplayer_param_def, sizeof(struct txmplayer_param));
    txmplayer_update_param(&g_txmplayer.global_param, param);
    g_txmplayer.task_hdl = os_task_create(TXMPLAYER_MSI, txmplayer_thread, NULL, task_pri, 5, NULL, stack_size);

    g_txmplayer.msi->priv   = &g_txmplayer;
    g_txmplayer.msi->action = (msi_action)txmplayer_msi_action;
    g_txmplayer.msi->chan_mgr = 1;
    g_txmplayer.msi->enable = 1;
    return RET_OK;
}

int32 txmplayer_deinit(void)
{
    int8 i = 0;
    struct txmplayer_stream *stream;

    if (atomic_dec2_return(&g_txmplayer.init) != 1) {
        txm_warn("TXMplayer init count %d\r\n", atomic_read(&g_txmplayer.init));
        return RET_OK;
    }

    msi_destroy(g_txmplayer.msi);

    os_mutex_lock(&g_txmplayer.lock, osWaitForever);
    os_task_destroy(g_txmplayer.task_hdl);
    for (i = 0; i < TXMPLAYER_STREAM_MAX; i++) {
        stream = g_txmplayer.streams[i];
        if (stream) {
            stream->msi->enable = 0;
            txmplayer_stop_play(stream);
            msi_clear(stream->msi);
            if (stream->ops != &msi_ops) {
                msi_destroy(stream->msi);
            }
        }
    }
    os_mutex_del(&g_txmplayer.lock);
    g_txmplayer.task_hdl = NULL;
    SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_PLAY_CLOSE, 0);
    return RET_OK;
}

int32 txmplayer_open(char *file,      uint8 preview, struct txmplayer_param *param)
{
    if (atomic_read(&g_txmplayer.init) == 0) {
        txm_err("need call txmplayer_init first!\r\n");
        return -EIO;
    }

    os_mutex_lock(&g_txmplayer.lock, osWaitForever);
    struct txmplayer_stream *stream = txmplayer_request_stream();
    if (stream) {
        stream->preview = preview;
        stream->vdd = param ? msi_get(param->vdd) : NULL;
        msi_do_cmd(stream->vdd, MSI_CMD_GET_VDD_RECT, (uint32)(&stream->vddrect), 0);
        txmplayer_update_param(&stream->param, param);
        txmplayer_open_stream(stream, file);
        if(param->volume == 0) txm_warn("TXMplayer volume is 0!\r\n");
    }
    os_mutex_unlock(&g_txmplayer.lock);

    return stream ? stream->id : -1;
}

int32 txmplayer_close(int32 stream_id)
{
    struct txmplayer_stream *stream;

    if (atomic_read(&g_txmplayer.init) == 0) {
        txm_err("need call txmplayer_init first!\r\n");
        return -EIO;
    }

    if (stream_id >= 0 && stream_id < TXMPLAYER_STREAM_MAX) {
        os_mutex_lock(&g_txmplayer.lock, osWaitForever);
        stream = g_txmplayer.streams[stream_id];
        if (stream && stream->ops != &msi_ops) {
            msi_cmd2(stream->msi, MSI_CMD_STOP, 0, 0);
            msi_clear(stream->msi);
            msi_destroy(stream->msi);
            txm_warn("close %s\r\n", txmp_names[stream_id]);
        }
        os_mutex_unlock(&g_txmplayer.lock);
        return RET_OK;
    }
    return -EINVAL;
}

int32 txmplayer_pause(int32 stream_id, uint8 pause)
{
    struct txmplayer_stream *stream;

    if (atomic_read(&g_txmplayer.init) == 0) {
        txm_err("need call txmplayer_init first!\r\n");
        return -EIO;
    }

    if (stream_id >= 0 && stream_id < TXMPLAYER_STREAM_MAX) {
        os_mutex_lock(&g_txmplayer.lock, osWaitForever);
        stream = g_txmplayer.streams[stream_id];
        if (stream) {
            if (pause == 1 && TXMPLAYER_PLAYING(stream->state)) {
                msi_cmd2(stream->msi, MSI_CMD_PAUSE, 0, 0);
                stream->state = TXMPLAYER_STATE_PAUSE;
            }
            if (pause == 0 && (stream->state == TXMPLAYER_STATE_PAUSE)) {
                msi_cmd2(stream->msi, MSI_CMD_START, 0, 0);
                stream->state = TXMPLAYER_STATE_PLAYING;
            }
        }
        os_mutex_unlock(&g_txmplayer.lock);
        return RET_OK;
    }
    return -EINVAL;
}

int32 txmplayer_seek(int32 stream_id, uint32 new_time)
{
    struct txmplayer_stream *stream;

    if (atomic_read(&g_txmplayer.init) == 0) {
        txm_err("need call txmplayer_init first!\r\n");
        return -EIO;
    }

    if (stream_id >= 0 && stream_id < TXMPLAYER_STREAM_MAX) {
        os_mutex_lock(&g_txmplayer.lock, osWaitForever);
        stream = g_txmplayer.streams[stream_id];
        if (stream) {
            if (stream->total_size == (ulong)(-1)) {
                os_mutex_unlock(&g_txmplayer.lock);
                SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_PLAY_SEEK_ERR, stream_id);
                txm_err("Livestream, Can not seek!\r\n");
                return -ENOTSUP;
            } else {
                if (stream->ops == &uf_ops && uf_seektype(stream->file) == 1) { //time seek: 直接执行seek操作
                    if (stream->ops->seek(stream->file, new_time, SEEK_SET) == RET_OK) {
                        stream->seeking_time = new_time;
                        fb_put(stream->dec_fb);
                        stream->dec_fb = NULL;
                        msi_discard_fb(stream->msi, NULL, stream->msi);
                        msi_cmd2(stream->msi, MSI_CMD_CLEAR, 0, 0);
                        msi_cmd2(stream->msi, MSI_CMD_EOF, 0, 0);
                        msi_cmd2(stream->msi, MSI_CMD_START, 0, 0);
                        stream->state = TXMPLAYER_STATE_SEEKING;
                    }
                } else if (stream->container && stream->container_obj) { //由container执行seek
                    if (stream->container->do_seek(stream->container_obj, new_time) == RET_OK) {
                        stream->seeking_time = new_time;
                        fb_put(stream->dec_fb);
                        stream->dec_fb = NULL;
                        msi_discard_fb(stream->msi, NULL, stream->msi);
                        msi_cmd2(stream->msi, MSI_CMD_CLEAR, 0, 0);
                        msi_cmd2(stream->msi, MSI_CMD_EOF, 0, 0);
                        msi_cmd2(stream->msi, MSI_CMD_START, 0, 0);
                        stream->state = TXMPLAYER_STATE_SEEKING;
                    }
                }
            }
        }
        os_mutex_unlock(&g_txmplayer.lock);
        return RET_OK;
    }
    return -EINVAL;
}

int32 txmplayer_set_speed(int32 stream_id, int32 speed)
{
    struct txmplayer_stream *stream;

    if (atomic_read(&g_txmplayer.init) == 0) {
        txm_err("need call txmplayer_init first!\r\n");
        return -EIO;
    }

    if (stream_id >= 0 && stream_id < TXMPLAYER_STREAM_MAX) {
        os_mutex_lock(&g_txmplayer.lock, osWaitForever);
        stream = g_txmplayer.streams[stream_id];
        if (stream) {
            if (stream->container_obj && stream->container->ioctl) {
                stream->container->ioctl(stream->container_obj, AVDEMUXER_SET_PLAY_SPEED, speed, 0);
            }
            msi_output_cmd(stream->msi, MSI_CMD_SET_SPEED, speed, 0);
        }
        os_mutex_unlock(&g_txmplayer.lock);
    }
    return RET_OK;
}

int32 txmplayer_set_volume(int32 stream_id, uint8 volume)
{
    struct txmplayer_stream *stream;

    if (atomic_read(&g_txmplayer.init) == 0) {
        txm_err("need call txmplayer_init first!\r\n");
        return -EIO;
    }

    if (stream_id >= 0 && stream_id < TXMPLAYER_STREAM_MAX) {
        os_mutex_lock(&g_txmplayer.lock, osWaitForever);
        stream = g_txmplayer.streams[stream_id];
        if (stream) {
            stream->param.volume = volume;
            msi_output_cmd(stream->msi, MSI_CMD_SET_VOLUME, volume, 0);
            SYSEVT_NEW_MEDIA_EVT(SYSEVT_MEDIA_VOLUME, volume << 8 | stream_id);
        }
        os_mutex_unlock(&g_txmplayer.lock);
    }
    return RET_OK;
}

//切换 音频/字幕 track
int32 txmplayer_set_track(int32 stream_id, uint32 track_id)
{
    int32 ret = RET_OK;
    struct txmplayer_stream *stream;

    if (atomic_read(&g_txmplayer.init) == 0) {
        txm_err("need call txmplayer_init first!\r\n");
        return -EIO;
    }

    if (stream_id >= 0 && stream_id < TXMPLAYER_STREAM_MAX) {
        os_mutex_lock(&g_txmplayer.lock, osWaitForever);
        stream = g_txmplayer.streams[stream_id];
        if (stream && stream->container_obj && stream->container->ioctl) {
            ret = stream->container->ioctl(stream->container_obj, AVDEMUXER_SET_TRACK, track_id, 0);
        }
        os_mutex_unlock(&g_txmplayer.lock);
    }
    return ret;
}

int32 txmplayer_msi_cmd(int32 stream_id, uint32 cmd, uint32 param1, uint32 param2)
{
    struct txmplayer_stream *stream;

    if (atomic_read(&g_txmplayer.init) == 0) {
        txm_err("need call txmplayer_init first!\r\n");
        return -EIO;
    }

    if (stream_id >= 0 && stream_id < TXMPLAYER_STREAM_MAX) {
        os_mutex_lock(&g_txmplayer.lock, osWaitForever);
        stream = g_txmplayer.streams[stream_id];
        if (stream) {
            msi_cmd2(stream->msi, cmd, param1, param2);
        }
        os_mutex_unlock(&g_txmplayer.lock);
        return RET_OK;
    }
    return -EINVAL;
}

int32 txmplayer_playtime(int32 stream_id, uint32 *tot_time)
{
    uint32 plytime = 0;
    struct txmplayer_stream *stream;

    if (atomic_read(&g_txmplayer.init) == 0) {
        txm_err("need call txmplayer_init first!\r\n");
        return 0;
    }

    if (stream_id >= 0 && stream_id < TXMPLAYER_STREAM_MAX) {
        os_mutex_lock(&g_txmplayer.lock, osWaitForever);
        stream = g_txmplayer.streams[stream_id];
        if (stream) {
            if (stream->state == TXMPLAYER_STATE_PLAY_END) {
                plytime = stream->total_duration;
            } else if(stream->state == TXMPLAYER_STATE_SEEKING){
                plytime = stream->seeking_time;
            } else {
                msi_cmd2(stream->msi, MSI_CMD_GET_PLAYTIME, (uint32)&plytime, 0);
            }
            if (tot_time) { *tot_time = stream->total_duration; }
        }
        os_mutex_unlock(&g_txmplayer.lock);
        return plytime;
    }
    return 0;
}

int32 txmplayer_state(int32 stream_id)
{
    uint32 ret = 0;
    struct txmplayer_stream *stream;

    if (atomic_read(&g_txmplayer.init) == 0) {
        txm_err("need call txmplayer_init first!\r\n");
        return TXMPLAYER_STATE_CLOSE;
    }

    if (stream_id >= 0 && stream_id < TXMPLAYER_STREAM_MAX) {
        os_mutex_lock(&g_txmplayer.lock, osWaitForever);
        stream = g_txmplayer.streams[stream_id];
        if (stream) { ret = stream->state; }
        os_mutex_unlock(&g_txmplayer.lock);
        return ret;
    }
    return TXMPLAYER_STATE_CLOSE;
}

int32 txmplayer_reopen(int32 stream_id, char *file)
{
    uint32 ret = 0;
    struct txmplayer_stream *stream;

    if (atomic_read(&g_txmplayer.init) == 0) {
        txm_err("need call txmplayer_init first!\r\n");
        return RET_ERR;
    }

    if (stream_id >= 0 && stream_id < TXMPLAYER_STREAM_MAX) {
        os_mutex_lock(&g_txmplayer.lock, osWaitForever);
        stream = g_txmplayer.streams[stream_id];
        if (stream) {
            msi_cmd2(stream->msi, MSI_CMD_STOP, 0, 0);
            if (stream->ops) {
                stream->ops->close(stream->file);
                stream->ops = NULL;
            }

            for (int i = 0; i < TXMPLAYER_CODEC_COUNT; i++) {
                stream->codecs[i].unknow = 0;
                stream->codecs[i].stype = 0xff;
                msi_put(stream->codecs[i].codec);
                stream->codecs[i].codec = NULL;
            }

            stream->total_duration = 0;
            stream->total_size = 0;
            stream->eof   = 0;
            stream->fst_data = 0;
            stream->buflevel = 0;
            stream->unknow_container = 0;
            stream->state = TXMPLAYER_STATE_PLAYING;
            ret = txmplayer_open_stream(stream, file);
            msi_output_cmd(stream->vdd, MSI_CMD_SET_VDD_ONOFF, 1, 0);
        }
        os_mutex_unlock(&g_txmplayer.lock);
        return ret;
    }
    return RET_ERR;
}


void txmplayer_status(void)
{
    uint8 i = 0;
    uint32 play_time = 0;
    uint32 tot_time = 0;
    uint32 aplay_time = 0;
    uint32 vplay_time = 0;
    uint32 dsize = 0;
    uint8  buflevel = 0;
    uint8  eod = 0;

    if (atomic_read(&g_txmplayer.init) == 0) {
        txm_err("need call txmplayer_init first!\r\n");
        return;
    }

    txm_warn("------------------------------------------\r\n");
    txm_warn("TXMplayer Status:\r\n");

    os_mutex_lock(&g_txmplayer.lock, osWaitForever);
    for (i = 0; i < TXMPLAYER_STREAM_MAX; i++) {
        if (g_txmplayer.streams[i] == NULL) {
            continue;
        }

        play_time = tot_time = eod = 0;
        play_time = txmplayer_playtime(i, &tot_time);
        msi_cmd2(g_txmplayer.streams[i]->msi, MSI_CMD_GET_AUDIO_PLAYTIME, (uint32)&aplay_time, 0);
        msi_cmd2(g_txmplayer.streams[i]->msi, MSI_CMD_GET_VIDEO_PLAYTIME, (uint32)&vplay_time, 0);
        if (g_txmplayer.streams[i]->ops == &uf_ops) {
            dsize = uf_datasize(g_txmplayer.streams[i]->file, &buflevel, &eod);
        }

        txm_warn("  [%s]: %s, time:%ds/%ds|%d|%d|, data:%d/%d%%/%d/%d [%s]\r\n", txmp_names[i],
                 txmplayer_status_text(i), play_time / 1000, tot_time / 1000, aplay_time, vplay_time,
                 dsize, buflevel, eod,
                 g_txmplayer.streams[i]->ops->eof(g_txmplayer.streams[i]->file),
                 g_txmplayer.streams[i]->container ? g_txmplayer.streams[i]->container->name : "RAW");
    }
    os_mutex_unlock(&g_txmplayer.lock);
    txm_warn("------------------------------------------\r\n");
}

int32 txmplayer_test_atcmd(const char *cmd, char *argv[], uint32 argc)
{
    uint8 cmdid;
    uint8 streamid;

    os_printf("TXMplayer测试说明t:[%d]\r\n", sizeof(off_t));
    os_printf("  Usage:\r\n");
    os_printf("    at+txmplayer=0,0          #关闭播放器通道. [参数0:cmd ID；参数1:通道号]\r\n");
    os_printf("    at+txmplayer=1,xxx.mp3,0  #启动播放器.     [参数0:cmd ID；参数1:文件或连接, 参数2：预览模式]\r\n");
    os_printf("    at+txmplayer=2,1,1/0      #暂停/恢复播放.  [参数0:cmd ID, 参数1:通道号]\r\n");
    os_printf("    at+txmplayer=3,1,3000     #跳转到指定时间. [参数0:cmd ID, 参数1:通道号, 参数2:跳转的时间，单位ms]\r\n");
    os_printf("    at+txmplayer=4,1,30       #设置音量.       [参数0:cmd ID, 参数1:通道号, 参数2:音量（0~100)]\r\n");
    os_printf("    at+txmplayer=5,1,2        #设置播放倍速.   [参数0:cmd ID, 参数1:通道号, 参数2:倍速]\r\n");
    os_printf("    at+txmplayer=6,1,2        #切换音轨.       [参数0:cmd ID, 参数1:通道号, 参数2:音轨号]\r\n");
    os_printf("    at+txmplayer=7            #打印播放器状态. [参数0:cmd ID]\r\n");
    os_printf("    at+txmplayer=8,1,1,30     #设置混音模式.   [参数0:cmd ID, 参数1:通道号, 参数2: 混音模式, 参数3:压低音量]\r\n");
    os_printf("    at+txmplayer=9,0,xxx.mp3  #重新打开指定的文件. [参数0:cmd ID；参数1:通道号, 参数2:文件或连接]\r\n");
    os_printf("    at+txmplayer=10,0,x,y,w,h #设置播放器窗口.     [参数0:cmd ID；参数1:通道号, 参数2:窗口信息(x,y,w,h)]\r\n");

    cmdid    = os_atoi(argv[0]);
    streamid = argc > 1 ? os_atoi(argv[1]) : 0;
    os_printf("cmd:%d, streamid:%d\r\n", cmdid, streamid);
    switch (cmdid) {
        case 0: //#close txmplayer.
            os_printf("txmplayer close stream %d\r\n", streamid);
            txmplayer_close(streamid);
            break;
        case 1: //#open txmplayer.
            txmplayer_init(0, 0, NULL);
            streamid = txmplayer_open(argv[1], argc > 2 ? os_atoi(argv[2]) : 0, NULL);
            os_printf("txmplayer open %s, streamid=%d\r\n", argv[1], streamid);
            break;
        case 2: //#pause/resume txmplayer.
            txmplayer_pause(streamid, os_atoi(argv[2]));
            break;
        case 3: //#seek play to 3s.
            txmplayer_seek(streamid, os_atoi(argv[2]));
            break;
        case 4: //#seek volume.
            txmplayer_set_volume(streamid, os_atoi(argv[2]));
            break;
        case 5: //#set play speed.
            txmplayer_set_speed(streamid, os_atoi(argv[2]));
            break;
        case 6: //#change audio track.
            txmplayer_set_track(streamid, os_atoi(argv[2]));
            break;
        case 7: //#dump play status.
            txmplayer_status();
            break;
        case 8: //set audio mix mode.
            txmplayer_msi_cmd(streamid, MSI_CMD_SET_MIXER_MODE, os_atoi(argv[2]), os_atoi(argv[3]));
            break;
        case 9:
            txmplayer_reopen(streamid, argv[2]);
            break;
        case 10: //set VDD: 10,0,x,y,w,h
            if (atomic_read(&g_txmplayer.init) && streamid >= 0 && streamid < TXMPLAYER_STREAM_MAX) {
                os_mutex_lock(&g_txmplayer.lock, osWaitForever);
                struct txmplayer_stream *stream = g_txmplayer.streams[streamid];
                if (stream) {
                    struct vdd_rect vdd;
                    vdd.x = os_atoi(argv[2]);
                    vdd.y = os_atoi(argv[3]);
                    vdd.width = os_atoi(argv[4]);
                    vdd.height = os_atoi(argv[5]);
                    msi_cmd2(stream->msi, MSI_CMD_SET_VDD_RECT, (uint32)&vdd, 0);
                    txm_warn("%s change VDD: [x:%d, y:%d, w:%d, h:%d]\r\n", stream->msi->name, vdd.x, vdd.y, vdd.width, vdd.height);
                }
                os_mutex_unlock(&g_txmplayer.lock);
            }
            break;
        default:
            break;
    }
    return 0;
}

