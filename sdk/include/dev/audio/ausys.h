#ifndef _AUSYS_H
#define _AUSYS_H

#ifdef __cplusplus
extern "C" {
#endif


/*----------------------------------
 *       AUSYS DA platform APIs
 *----------------------------------*/

enum ausys_da_platform {
    AUSYS_AUDA,
    AUSYS_IIS0,
    AUSYS_IIS1,
};

enum ausys_da_msg_type {
    //FIFO empty message
    AUSYS_DA_MSG_FIFO_FULL      = BIT(0),

    //FIFO full message
    AUSYS_DA_MSG_FIFO_EMPTY     = BIT(1),

    //DA successfully play once
    AUSYS_DA_MSG_PLAY_DONE      = BIT(2),

    //DA successfully play half
    AUSYS_DA_MSG_PLAY_HALF      = BIT(3),
};

struct ausys_da_msg_content {
    uint32 fifo_next_addr;
    uint32 fifo_next_len;
};

struct ausys_da_msg {
    enum ausys_da_msg_type          type;
    struct ausys_da_msg_content     da_content;
    void                           *user_content;
};


int32 ausys_da_put(void* buf, uint32 bytes);
int32 ausys_da_play(void);
int32 ausys_da_change_sample_rate(uint32 sample_rate);
int32 ausys_da_change_volume(uint32 percent_0to100);
int32 ausys_da_get_cur_volume(uint32 *percent_0to100);
int32 ausys_da_register_msg(enum ausys_da_msg_type msg_type);
int32 ausys_da_unregister_msg(enum ausys_da_msg_type msg_type);
int32 ausys_da_get_msg(struct ausys_da_msg *msg, uint32 tmo_ms);
int32 ausys_da_init(uint32 sample_rate, uint32 sample_bits, uint32 platform, uint32 buf_bytes, uint32 per_buf_bytes, uint32 play_null_bytes);
int32 ausys_da_deinit(void);





/*----------------------------------
 *       AUSYS AD platform APIs
 *----------------------------------*/

enum ausys_ad_platform {
    AUSYS_AUAD,
    AUSYS_PDM,
};

enum ausys_ad_msg_type {
    //AD successfully play once
    AUSYS_AD_MSG_PLAY_DONE      = BIT(0),

    //AD successfully play half
    AUSYS_AD_MSG_PLAY_HALF      = BIT(1),
};

struct ausys_ad_msg_content {
    uint32 fifo_next_addr;
    uint32 fifo_next_len;
    uint32 fifo_cur_addr;
    uint32 fifo_cur_len;
};

struct ausys_ad_msg {
    enum ausys_ad_msg_type          type;
    struct ausys_ad_msg_content     ad_content;
    void                           *user_content;
};



/*!
 * type of user call back handle
 */
typedef void (*type_ausys_ad_user_cb)(void* buf, uint32 bytes, uint32 res);


int32 ausys_ad_record(void);
int32 ausys_ad_register_msg(enum ausys_ad_msg_type msg_type);
int32 ausys_ad_unregister_msg(enum ausys_ad_msg_type msg_type);
int32 ausys_ad_get_msg(struct ausys_ad_msg *msg, uint32 tmo_ms);
int32 ausys_ad_init(uint32 sample_rate, uint32 sample_bits, uint32 platform, uint32 buf_bytes, uint32 per_buf_bytes, type_ausys_ad_user_cb user_cb, uint32 res);
int32 ausys_ad_deinit(void);



#ifdef __cplusplus
}
#endif
#endif
