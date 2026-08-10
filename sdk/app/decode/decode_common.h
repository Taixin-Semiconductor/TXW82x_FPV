#ifndef __DECODE_COMMON_H__
#define __DECODE_COMMON_H__


#define DECODE_SRAMBUF_WLEN   64


// 解码器最大channel数量
#define MAX_DECODE_CHANNEL (4)

#define MIN_MEM_INFO (2)
#define MAX_TTL      (3000)

#define DECODE_COMMON_MEMBERS                                                                                                                                                                          \
    struct os_work       work;                                                                                                                                                                         \
    struct msi          *msi;                                                                                                                                                                          \
    struct mem_info    **mem_info;                                                                                                                                                                     \
    uint32_t             mem_info_size;                                                                                                                                                                \
    uint8_t             *scaler2buf_y;                                                                                                                                                                 \
    uint8_t             *scaler2buf_u;                                                                                                                                                                 \
    uint8_t             *scaler2buf_v;                                                                                                                                                                 \
    struct scale_device *scale_dev;                                                                                                                                                                    \
    uint32_t             last_decode_time;                                                                                                                                                             \
    void                *current_hdl;                                                                                                                                                                  \
    uint32_t             chan[MAX_DECODE_CHANNEL];                                                                                                                                                     \
    uint16_t             now_decode_pw;                                                                                                                                                                \
    uint8_t              channel_num;                                                                                                                                                                  \
    uint8_t              hardware_ready : 1, auto_free_space : 1, hardware_err : 1, is_register_isr : 1, unlock : 1, gc : 1, rev : 2

#define DECODE_COMMON_HDL                                                                                                                                                                              \
    struct msi               *msi;                                                                                                                                                                     \
    struct decode_msi_s      *decode;                                                                                                                                                                  \
    void                     *decode_dev;                                                                                                                                                              \
    uint16_t                  w, h;                                                                                                                                                                    \
    uint16_t                  dw, dh;                                                                                                                                                                  \
    struct vcodec_decode_req *req;                                                                                                                                                                     \
    uint8_t                  *decode_addr;                                                                                                                                                             \
    uint32_t                  decode_size;                                                                                                                                                             \
    struct os_event           evt;                                                                                                                                                                     \
    const struct decode_fn   *fn;                                                                                                                                                                      \
    uint8_t                   chan;                                                                                                                                                                    \
    uint8_t                   sps_flag : 1, closed : 1, gc : 1, I_only : 1, index : 4

struct decode_msi_s
{
    DECODE_COMMON_MEMBERS;
    uint32_t rom_max_size;
    uint32_t rom_last_use_time;
    uint8_t *rom;
};

typedef int32_t (*common_cb_t)(void *de, void *hdl);
struct decode_fn
{
    // free_fn
    mfree_cb_t  free;
    // lock_fn
    common_cb_t lock;

    // unlock_fn
    common_cb_t unlock;

    // get_decode_mem_size_fn
    common_cb_t get_decode_size;

    // decode_ready_fn
    common_cb_t decode_ready;

    // decode_kick_fn
    common_cb_t decode_kick;

    // pre_decode_fn
    common_cb_t pre_decode;

    // decode_done_fn
    common_cb_t decode_done;

    common_cb_t decode_free_hdl;
};

struct common_decode_hdl
{
    DECODE_COMMON_HDL;
};
enum
{
    DECODE_CHAN_EMPTY = BIT(0), // 通道被清空
};

struct vcodec_device_decode
{
    struct vcodec_device dev;
    uint32_t             max_buf_num;
};

int32_t     get_free_chan(struct decode_msi_s *decode, void *hd);
void        decode_realse_hdl(struct decode_msi_s *decode);
int32_t     decode_req_done(struct vcodec_decode_req *req, int32_t status);
struct msi *decode_core_msi(const char *name, uint8_t info_size);
int32_t     general_scale2_done(uint32 irq_flag, uint32 irq_data, uint32 param1);
int32_t     general_scale2_ov(uint32 irq_flag, uint32 irq_data, uint32 param1);
int32_t     general_decode_err(uint32 irq_flag, uint32 irq_data, uint32 param1, uint32 param2);
int32_t     general_decode_done(uint32 irq_flag, uint32 irq_data, uint32 param1, uint32 param2);

#endif