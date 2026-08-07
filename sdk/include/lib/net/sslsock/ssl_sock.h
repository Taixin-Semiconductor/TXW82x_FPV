#ifndef __SSL_SOCKET_ADAPTER_H__
#define __SSL_SOCKET_ADAPTER_H__

typedef struct {
    int8         is_server;
    int8         verify_cert;            // 是否验证客户端证书（0=不验证，1=验证）
    int8         debug, r2;
    const char  *cert;                   // 服务器证书数据（PEM/DER 自动识别）
    const char  *key;                    // 服务器私钥数据（PEM/DER 自动识别）
    const char  *ca_cert;                // CA证书数据（仅verify_cert=1时需传）
    uint32       max_session_cache;      // 会话缓存最大条目（默认100）
    uint32       session_timeout_sec;    // 会话超时时间（秒，默认30）
} ssl_sock_init_param;

typedef struct {
    /**
     * @brief 初始化SSL全局上下文
     * @param param 初始化参数
     * @return void * 全局上下文指针
     */
    void *(*global_init)(const ssl_sock_init_param *param);

    /**
     * @brief 全局资源销毁
     * @param global_ctx 全局上下文指针
     */
    void (*global_destroy)(void *global_ctx);

    /**
     * @brief 创建连接上下文
     * @param global_ctx 全局上下文指针
     * @return void * 连接上下文指针
     */
    void *(*conn_create)(void *global_ctx);

    /**
     * @brief 连接上下文销毁
     * @param conn_ctx 连接上下文指针
     */
    void (*conn_destroy)(void *conn_ctx);

    /**
     * @brief TLS握手
     * @param conn_ctx 连接上下文指针
     * @param sockfd 套接字文件描述符
     * @param step 是否由外部代码控制握手过程
     * @return int 0成功，-1失败
     */
    int (*handshake)(void *conn_ctx, int sockfd, int step);

    /**
     * @brief 加密发送
     * @param conn_ctx 连接上下文指针
     * @param data 数据指针
     * @param len 数据长度
     * @return int32 发送字节数
     */
    int32(*send)(void *conn_ctx, const uint8 *data, uint32 len);

    /**
     * @brief 加密接收
     * @param conn_ctx 连接上下文指针
     * @param buf 接收缓冲区指针
     * @param buf_len 接收缓冲区长度
     * @return int32 接收字节数
     */
    int32(*recv)(void *conn_ctx, uint8 *buf, uint32 buf_len);

    /**
     * @brief 优雅关闭
     * @param conn_ctx 连接上下文指针
     */
    void (*shutdown)(void *conn_ctx);

    /**
     * @brief 获取错误信息
     * @param conn_ctx 连接上下文指针
     * @return const char* 错误信息指针
     */
    const char *(*get_error)(void *conn_ctx);
} sslsock_ops;

extern const sslsock_ops sslsock_mbedtls_ops;

#endif // __UHTTPD_SSL_ADAPTER_H__

