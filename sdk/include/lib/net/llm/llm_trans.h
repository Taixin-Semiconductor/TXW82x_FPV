#ifndef _LLM_TRANS_H_
#define _LLM_TRANS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* flag bits */
#define LLMWS_TEXT       (1<<0)
#define LLMWS_BINARY     (1<<1)
#define LLMWS_CONT       (1<<2)
#define LLMWS_CLOSE      (1<<3)
#define LLMWS_PING       (1<<4)
#define LLMWS_OFFSET     (1<<5)
#define LLMWS_PONG       (1<<6)

typedef enum {
    LLM_HTTP_GET,
    LLM_HTTP_POST,
    LLM_HTTP_PUT,
    LLM_HTTP_DELETE,
    LLM_HTTP_PATCH
} llm_http_type;

typedef struct {
    uint32 flags;       // 数据类型
    uint32 sent;        // 实际发送的数据长度
} llm_send_meta;

typedef struct {
    uint32 flags;       // 数据类型
    uint32 nread;       // 实际接收到的数据长度
    int64  offset;      // 当前数据在这个帧中的偏移量
    int64  bytesleft;   // 剩余待处理的有效负载字节数
    int32  len;         // 当前数据块的长度
} llm_recv_meta;

struct llm_trans_param {
    size_t      buffersize;
    size_t      upload_buffersize;
    long        low_speed_limit;
    long        low_speed_time;
    long        connect_timeout;
    size_t      blob_len;
    void        *blob_data;
    uint8       debug;
};

/* basse64编码可接受的最大输入长度 */
#define LLM_MAX_BASE64_INPUT 16000000

//////////////////////////////////////////////////////////////////////////////////
int32 llm_trans_init(void);
int32 llm_trans_deinit(void);
void *llm_build_header(void *headers, char *req_headers);
int32 llm_free_header(void *headers);
int32 llm_base64_encode(const char *inputbuff, size_t insize,
                        char **outptr, size_t *outlen, uint8 is_url);
int32 llm_base64_decode(const char *src, unsigned char **outptr, size_t *outlen);

/* https API */
void *llm_https_connect(void *headers, void *cfg,
                        void *write_callback, void *read_callback, void *xferinfo_callback, void *userdata);
int32 llm_https_disconnect(void *handle);
int32 llm_https_send(void *handle, llm_http_type type, char *url, char *buff, size_t length);
int32 llm_https_upload(void *handle, char *url, char *buff, size_t length,
                        const char *name, const char *filename,
                        void *read_callback, void *seek_callback, void *free_callback, void *userdata);
int32 llm_https_recv(void *handle, char *buff, size_t length, size_t *nread);   //no use

/* websocket API */
void *llm_websocket_connect(char *url, void *headers, void *cfg);
int32 llm_websocket_disconnect(void *handle);
int32 llm_websocket_send(void *handle, char *buff, size_t length, size_t *sent);
int32 llm_websocket_recv(void *handle, char *buff, size_t length, size_t *nread);
int32 llm_websocket_psend(void *handle, char *buff, size_t length, llm_send_meta *smeta);
int32 llm_websocket_precv(void *handle, char *buff, size_t length, llm_recv_meta *rmeta);

#ifdef __cplusplus
}
#endif

#endif
