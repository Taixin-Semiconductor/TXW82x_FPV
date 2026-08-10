#ifndef __GEN420_ENCODE_CORE_H
#define __GEN420_ENCODE_CORE_H
#define GEN420_CORE_CHAN (8)

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

typedef int32_t (*common_cb_t)(void *de, void *hdl);
struct encode_fn
{
    // free_fn
    mfree_cb_t  free;
    // lock_fn
    common_cb_t lock;

    // unlock_fn
    common_cb_t unlock;

    // get_encode_mem_size_fn
    common_cb_t get_encode_size;

    // encode_ready_fn
    common_cb_t encode_ready;

    // encode_kick_fn
    common_cb_t encode_kick;

    // pre_encode_fn
    common_cb_t pre_encode;

    // encode_done_fn
    common_cb_t encode_done;

    common_cb_t encode_free_hdl;
};

typedef struct gen420_core_s
{
    struct os_work    work;
    struct msi       *msi;
    struct msi       *jpg_msi; // 注册的jpg的msi,因为gen420需要与jpg或者其他硬件联动
    void             *current_hdl;
    struct framebuff *recv_fb;
    struct os_event   evt;
    uint32_t          last_encode_time;
    uint32_t          chan[GEN420_CORE_CHAN];
    uint8_t           channel_num;
    uint8_t           ready : 1, done : 1, err : 1, gc : 1, rev : 4;
} gen420_core;

struct gen420_hdl_s
{
    struct msi               *msi;
    struct gen420_core_s     *encode;
    struct vcodec_encode_req *req;
    const struct encode_fn   *fn;
    uint16_t                  w, h;
    uint16_t                  s_w, s_h;
    struct os_event           evt;
    uint8_t                   chan;
    uint8_t                   closed : 1, gc : 1, which_jpg : 1, belong : 1, rev : 4;
};

struct vcodec_device_encode
{
    struct vcodec_device dev;
    uint32_t             max_buf_num;
};

enum
{
    ENCODE_CHAN_EMPTY   = BIT(0), // 通道被清空
    ENCODE_CHAN_DESTROY = BIT(1), // 通道被销毁
};

int32_t     get_gen420_core_free_chan(struct gen420_core_s *encode, void *hd);
struct msi *gen420_encode_core(const char *msi_name);

#endif