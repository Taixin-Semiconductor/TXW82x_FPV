#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/multimedia/AVContainer.h"
#include "lib/multimedia/video.h"


#define JPEG_MAX_SIZE           (256 * 1024)       // 最大支持 256KB 
#define JPEG_MIN_SIZE           (512)              // 最小 JPEG 大小

typedef struct {
    struct msi                 *owner;              // 上层播放器 MSI
    const struct AVDemuxerOps  *ops;                // 文件操作接口
    void                      *file_handle;         // 文件/网络句柄
    uint8_t                   *jpeg_buf;            // JPEG 数据缓冲区
    uint32_t                   jpeg_size;           // 文件总大小
    uint32_t                   data_offset;         // 预读头数据的长度
    uint32_t                   buf_filled;          // jpeg_buf 中已填充的字节数
    int64_t                    file_size;             // 文件总大小
    uint8_t                    output_done : 1;     // 单帧输出标记
    uint8_t                    eof : 1;
    txVideoInfo_t              v_info;              // 视频/图片元数据（jpg_msi 需要 txVideoInfo_t）
} jpeg_demuxer_t;


static int jpeg_parse_size(const uint8_t *data, uint32_t len, uint16_t *w, uint16_t *h)
{
    for (uint32_t i = 0; i < len - 9; i++) {
        if (data[i] == 0xFF && (data[i+1] & 0xF0) == 0xC0) { 
            *h = (data[i+5] << 8) | data[i+6];
            *w = (data[i+7] << 8) | data[i+8];
            return 0;
        }
    }
    return -1;
}

static void *jpeg_demuxer_init(void *hdl, const struct AVDemuxerOps *ops, 
                                void *hdr, uint32_t len, struct msi *owner)
{
    if (!hdr || len < 2) {
        os_printf("[JPEG_INIT] ERROR: hdr is NULL or too short\n");
        return NULL;
    }
    
    uint8_t *header = (uint8_t*)hdr;
    if (header[0] != 0xFF || header[1] != 0xD8) {
        os_printf("[JPEG_INIT] ERROR: Not JPEG header! Got %02X %02X, expected FF D8\n", 
                  header[0], header[1]);
        return NULL;
    }

    jpeg_demuxer_t *d = decoder_mem_zalloc(sizeof(jpeg_demuxer_t));
    if (!d) {
        os_printf("[JPEG_INIT] ERROR: No memory for demuxer context\n");
        return NULL;
    }

    d->owner = owner;
    d->file_handle = hdl;
    d->ops = ops;
    d->output_done = 0;
    d->eof = 0;
    d->data_offset = 0; 
    d->buf_filled = 0;

    d->file_size = 0;
    if (ops->ioctl) {
        ops->ioctl(owner, AVDEMUXER_GET_FILE_SIZE, (uint32)&d->file_size, 0);
    }
    
    if (d->file_size > 0 && d->file_size < JPEG_MAX_SIZE) {
        d->jpeg_size = d->file_size;
    } else {
        d->jpeg_size = JPEG_MAX_SIZE; 
    }

    d->jpeg_buf = decoder_mem_alloc(d->jpeg_size);
    if (!d->jpeg_buf) {
        os_printf("[JPEG_INIT] ERROR: No memory for jpeg buffer (%d bytes)\n", d->jpeg_size);
        decoder_mem_free(d);
        return NULL;
    }

    uint32_t pre_read_len = (len > d->jpeg_size) ? d->jpeg_size : len;
    memcpy(d->jpeg_buf, hdr, pre_read_len);
    d->data_offset = pre_read_len;
    d->buf_filled = pre_read_len;
    
    os_printf("[JPEG_INIT] Pre-read %d bytes saved, total file size: %d\n", 
              pre_read_len, (int)d->file_size);

    memset(&d->v_info, 0, sizeof(d->v_info));
    d->v_info.codec_id = VIDEO_CODEC_MJPEG;       /* 底层走 MJPEG 解码 */
    jpeg_parse_size(d->jpeg_buf, pre_read_len, &d->v_info.width, &d->v_info.height);
    d->v_info.fps_num  = 1;  /* 静态图片 */
    d->v_info.fps_den  = 1;
    
    if (d->v_info.width && d->v_info.height) {
        os_printf("[JPEG_INIT] Image size: %dx%d\n", d->v_info.width, d->v_info.height);
    }

    return d;
}

static int32_t jpeg_demuxer_release(void *c)
{
    jpeg_demuxer_t *d = (jpeg_demuxer_t *)c;
    if (!d) return 0;
    
    if (d->jpeg_buf) {
        decoder_mem_free(d->jpeg_buf);
        d->jpeg_buf = NULL;
    }
    decoder_mem_free(d);
    return 0;
}

static int32_t jpeg_demuxer_do_seek(void *c, uint32_t time_ms)
{
    return 0;
}

static int32_t jpeg_demuxer_do_demux(void *c)
{
    jpeg_demuxer_t *d = (jpeg_demuxer_t *)c;
    
    if (!d || d->output_done) {
        return 0;
    }

    uint32_t read_pos = d->buf_filled;

    while (read_pos < d->jpeg_size) {
        uint32_t to_read = d->jpeg_size - read_pos;
        if (to_read == 0) break;

        int32_t got = d->ops->read(d->jpeg_buf + read_pos, 1, to_read, d->file_handle);
        if (got <= 0) {
            break;
        }
        read_pos += got;
    }
    d->buf_filled = read_pos;

    if (read_pos < 2 || d->jpeg_buf[0] != 0xFF || d->jpeg_buf[1] != 0xD8) {
        return 0;
    }

    struct framebuff *fb = msi_alloc_fb(d->owner, NULL, d->jpeg_buf, read_pos, 0, 0);

    if (!fb) {
        os_printf("[JPEG_DEMUX] ERROR: Failed to alloc framebuff\n");
        return 0;
    }
    sys_dcache_clean_range((void *)fb->data, fb->len);
    fb->mtype = MEDIA_DATA_PICTURE;
    fb->stype = PICTURE_FORMAT_JPEG;
    fb->time = 0;
    fb->srcID = FRAMEBUFF_SOURCE_FILE;
    //fb->priv = &d->v_info;
    fb->codec_info = &d->v_info;

    sys_dcache_clean_invalid_range_unaligned((void *)d->jpeg_buf, read_pos);
    d->ops->outFB(d->owner, fb);

    d->output_done = 1;
    return 0;
}

static int32_t jpeg_demuxer_ioctl(void *c, uint32_t cmd, uint32_t param1, uint32_t param2)
{
    switch (cmd) {
        default:
            break;
    }
    return 0;
}

const __avdemuxer struct AVDemuxer jpeg_demuxer = {
    .type         = MEDIA_CONTAINER_JPEG,  
    .name         = "jpeg_demuxer",
    .init         = jpeg_demuxer_init,
    .release      = jpeg_demuxer_release,
    .do_seek      = jpeg_demuxer_do_seek,
    .do_demux     = jpeg_demuxer_do_demux,
    .ioctl        = jpeg_demuxer_ioctl,
};