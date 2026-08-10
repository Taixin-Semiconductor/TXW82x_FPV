/**
  ******************************************************************************
  * @file    psram_update_to_flash_app_ota.c
    * @brief   TCP OTA 服务及固件接收实现。
    * @details 本文件负责创建 TCP 监听 Socket、接入单个 OTA 连接、完成升级
    *          协议收发，并将接收的固件顺序暂存到 PSRAM 后交给二次 Loader。
    * @warning 当前 C 端直接按目标平台本地结构布局接收协议数据；配套 Python
    *          工具使用小端格式 <9I 和 <16si 打包，协议两端必须保持布局一致。
  ******************************************************************************
  */
#include "basic_include.h"
#include "lwip/sockets.h"
#include "lib/net/eloop/eloop.h"
#include "psram_update_to_flash_app_ota.h"
#include "psram_update_to_flash_app.h"

#ifndef RET_OK
/** @brief 模块内部使用的成功返回值。 */
#define RET_OK                              (0)
#endif

#ifndef RET_ERR
/** @brief 模块内部使用的失败返回值。 */
#define RET_ERR                             (-1)
#endif

/** @brief 单次固件正文接收使用的 SRAM 缓冲区字节数。 */
#define PSRAM_UPDATE_TO_FLASH_APP_OTA_RECV_BYTES        (1460U)
/** @brief TCP 监听 Socket 的连接等待队列长度。 */
#define PSRAM_UPDATE_TO_FLASH_APP_OTA_BACKLOG           (3)
/** @brief 单连接 OTA 处理任务的栈字节数。 */
#define PSRAM_UPDATE_TO_FLASH_APP_OTA_STACK_BYTES       (2048U)
/** @brief 设备向客户端发送的升级成功结果字符串。 */
#define PSRAM_UPDATE_TO_FLASH_APP_OTA_RESULT_OK         "Upgrade_ok"
/** @brief 设备向客户端发送的升级失败结果字符串。 */
#define PSRAM_UPDATE_TO_FLASH_APP_OTA_RESULT_ERR        "Upgrade_err"
/** @brief 固件接收进度日志的累计字节间隔，每 100 KiB 记录一次。 */
#define PSRAM_UPDATE_TO_FLASH_APP_OTA_PROGRESS_BYTES    (100U * 1024U)

#if PSRAM_UPDATE_TO_FLASH_APP_OTA_DEBUG
/** @brief 输出 TCP OTA 调试日志。 */
#define LOG_D(fmt, ...) _os_printf("[PSRAM Update][TCP] " fmt, ##__VA_ARGS__)
#else
/** @brief 调试关闭时丢弃 TCP OTA 调试日志。 */
#define LOG_D(fmt, ...)
#endif

/**
 * @brief TCP OTA 请求或响应使用的 36 字节协议头。
 * @note 配套 Python 工具按小端 <9I 打包；当前实现直接接收目标平台本地布局。
 */
struct application_loader_tcp_header {
    uint32 command;       /**< 协议命令；客户端请求为 0xA0，设备 ACK 为 0xA1。 */
    uint32 reserved[8];   /**< 预留字段，发送响应前清零，接收请求时不作解析。 */
};

/**
 * @brief 客户端向设备发送的 20 字节固件信息。
 * @note 配套 Python 工具按小端 <16si 打包；当前实现直接接收目标平台本地布局。
 */
struct application_loader_tcp_firmware_info {
    char version[16];     /**< 最多 16 字节的固件版本字段，不保证以 NUL 结尾。 */
    int32 file_size;      /**< 客户端声明的固件正文字节数，必须大于 0。 */
};

/** @brief TCP OTA 监听 Socket 描述符，负值表示服务尚未启动。 */
static int g_application_loader_server_fd = -1;
/** @brief 监听 Socket 在 eloop 中注册的可读事件句柄。 */
static EVT_HDL g_application_loader_server_event;
/** @brief 单连接 OTA 运行标志，非零表示已有升级任务占用服务。 */
static uint8 g_psram_update_to_flash_app_ota_running;

/**
 * @brief 从已连接 Socket 精确接收指定字节数的数据。
 * @param[in] socket_fd 已建立连接的 TCP Socket 描述符。
 * @param[out] buffer 保存客户端发往设备的数据的缓冲区。
 * @param[in] bytes 必须完整接收的字节数。
 * @retval 0 已接收全部指定字节。
 * @retval -1 recv 返回零或负值，数据未完整接收。
 * @note TCP 是字节流，单次 recv 不保证返回请求的全部字节，因此循环接收
 *       直至满足长度或发生连接关闭、错误。
 */
static int application_loader_tcp_recv_exact(int socket_fd, void *buffer,
                                             uint32 bytes)
{
    uint8 *position = (uint8 *)buffer;
    uint32 received = 0U;

    while (received < bytes) {
        int result = recv(socket_fd, position + received, bytes - received, 0);

        if (result <= 0) {
            LOG_D(
                "recv exact failed: fd=%d, result=%d, received=%u/%u\n",
                socket_fd, result, received, bytes);
            return RET_ERR;
        }
        received += (uint32)result;
    }

    return RET_OK;
}

/**
 * @brief 向已连接 Socket 精确发送指定字节数的数据。
 * @param[in] socket_fd 已建立连接的 TCP Socket 描述符。
 * @param[in] buffer 保存设备发往客户端的数据的缓冲区。
 * @param[in] bytes 必须完整发送的字节数。
 * @retval 0 已发送全部指定字节。
 * @retval -1 send 返回零或负值，数据未完整发送。
 * @note TCP 是字节流，单次 send 不保证发送请求的全部字节，因此循环发送
 *       直至满足长度或发生连接关闭、错误。
 */
static int application_loader_tcp_send_exact(int socket_fd, const void *buffer,
                                             uint32 bytes)
{
    const uint8 *position = (const uint8 *)buffer;
    uint32 sent = 0U;

    while (sent < bytes) {
        int result = send(socket_fd, position + sent, bytes - sent, 0);

        if (result <= 0) {
            LOG_D(
                "send exact failed: fd=%d, result=%d, sent=%u/%u\n",
                socket_fd, result, sent, bytes);
            return RET_ERR;
        }
        sent += (uint32)result;
    }

    return RET_OK;
}

/**
 * @brief 向客户端发送当前 OTA 处理结果字符串。
 * @param[in] socket_fd 已建立连接的 TCP Socket 描述符。
 * @param[in] success 非零发送 Upgrade_ok，零发送 Upgrade_err。
 * @note 字符串通过 sizeof 计入末尾 NUL：Upgrade_ok\0 发送 11 字节，
 *       Upgrade_err\0 发送 12 字节；当前调用方不检查底层发送结果。
 */
static void application_loader_tcp_send_result(int socket_fd, int success)
{
    const char *message;
    uint32 bytes;

    if (success != 0) {
        message = PSRAM_UPDATE_TO_FLASH_APP_OTA_RESULT_OK;
        bytes = sizeof(PSRAM_UPDATE_TO_FLASH_APP_OTA_RESULT_OK);
    } else {
        message = PSRAM_UPDATE_TO_FLASH_APP_OTA_RESULT_ERR;
        bytes = sizeof(PSRAM_UPDATE_TO_FLASH_APP_OTA_RESULT_ERR);
    }
    LOG_D("send result: success=%d, message=%s\n",
                                     success, message);
    application_loader_tcp_send_exact(socket_fd, message, bytes);
}

/**
 * @brief 创建并开始监听 TCP OTA 服务器 Socket。
 * @param[in] port 以主机字节序给出的监听端口。
 * @return 成功时返回非负 Socket 描述符，失败时返回 -1。
 * @note 服务器启用地址复用并绑定 INADDR_ANY；bind 或 listen 失败时关闭
 *       已创建的 Socket。监听 backlog 为 3。
 */
static int application_loader_tcp_create_server(uint16 port)
{
    struct sockaddr_in address;
    int socket_fd;
    int reuse_address = 1;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        LOG_D("socket create failed: result=%d\n",
                         socket_fd);
        return RET_ERR;
    }

    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR,
               &reuse_address, sizeof(reuse_address));
    os_memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if ((bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0) ||
        (listen(socket_fd, PSRAM_UPDATE_TO_FLASH_APP_OTA_BACKLOG) < 0)) {
        LOG_D(
            "bind/listen failed: fd=%d, port=%u\n", socket_fd, port);
        close(socket_fd);
        return RET_ERR;
    }

    LOG_D(
        "server socket ready: fd=%d, port=%u\n", socket_fd, port);
    return socket_fd;
}

/**
 * @brief 处理一个已接入客户端的 TCP OTA 任务。
 * @param[in] argument 转换为整数后使用的已连接 TCP Socket 描述符。
 * @note 任务先从客户端精确接收 36 字节请求头并校验请求命令 0xA0，随后
 *       调用固件接收流程；退出时始终关闭客户端 Socket 并清除运行标志。
 */
static void psram_update_to_flash_app_ota_task(void *argument)
{
    struct application_loader_tcp_header request;
    int tcp_connect_fd = (int)argument;

    LOG_D("OTA task started: fd=%d\n",
                                     tcp_connect_fd);
    os_memset(&request, 0, sizeof(request));
    if (application_loader_tcp_recv_exact(tcp_connect_fd, &request,
                                          sizeof(request)) != RET_OK) {
        LOG_D(
            "request receive failed: fd=%d\n", tcp_connect_fd);
        goto task_exit;
    }

    LOG_D("request: command=0x%08x\n",
                                     request.command);
    if (request.command != PSRAM_UPDATE_TO_FLASH_APP_REQ_NET_UPGRADE) {
        LOG_D(
            "invalid command: actual=0x%08x, expected=0x%08x\n",
            request.command, PSRAM_UPDATE_TO_FLASH_APP_REQ_NET_UPGRADE);
        goto task_exit;
    }

    if (psram_update_to_flash_app_ota_receive(tcp_connect_fd) != RET_OK) {
        LOG_D("OTA receive returned error\n");
    }

task_exit:
    LOG_D("OTA task exit: fd=%d\n",
                                     tcp_connect_fd);
    close(tcp_connect_fd);
    g_psram_update_to_flash_app_ota_running = 0U;
}

/**
 * @brief 处理监听 Socket 的可读事件并接入一个 OTA 客户端。
 * @param[in] event eloop 事件句柄，当前回调不使用。
 * @param[in] data 转换为整数后使用的监听 Socket 描述符。
 * @note 服务一次只允许一个 OTA 任务运行；忙碌时直接关闭新连接。创建任务
 *       失败时会清除运行标志并关闭刚接入的客户端 Socket。
 */
static void application_loader_tcp_accept(EVT_HDL event, void *data)
{
    struct sockaddr_in client_address;
    socklen_t address_bytes = sizeof(client_address);
    int server_fd = (int)data;
    int tcp_connect_fd;
    void *task_handle;

    (void)event;
    tcp_connect_fd = accept(server_fd, (struct sockaddr *)&client_address,
                            &address_bytes);
    if (tcp_connect_fd < 0) {
        LOG_D("accept failed: server_fd=%d\n",
                         server_fd);
        return;
    }

    LOG_D(
        "client connected: fd=%d, ip=%u.%u.%u.%u, port=%u\n",
        tcp_connect_fd,
        (unsigned int)(ntohl(client_address.sin_addr.s_addr) >> 24) & 0xFFU,
        (unsigned int)(ntohl(client_address.sin_addr.s_addr) >> 16) & 0xFFU,
        (unsigned int)(ntohl(client_address.sin_addr.s_addr) >> 8) & 0xFFU,
        (unsigned int)ntohl(client_address.sin_addr.s_addr) & 0xFFU,
        ntohs(client_address.sin_port));

    if (g_psram_update_to_flash_app_ota_running != 0U) {
        LOG_D(
            "reject client: OTA already running\n");
        close(tcp_connect_fd);
        return;
    }

    g_psram_update_to_flash_app_ota_running = 1U;
    task_handle = os_task_create(
        "psram_app_ota", psram_update_to_flash_app_ota_task,
        (void *)tcp_connect_fd, OS_TASK_PRIORITY_NORMAL, 0, NULL,
        PSRAM_UPDATE_TO_FLASH_APP_OTA_STACK_BYTES);
    if (task_handle == NULL) {
        LOG_D("task create failed: fd=%d\n",
                         tcp_connect_fd);
        g_psram_update_to_flash_app_ota_running = 0U;
        close(tcp_connect_fd);
    }
}

/**
 * @brief 启动 TCP OTA 监听服务并注册客户端接入事件。
 * @retval 0 服务已成功启动，或此前已经启动。
 * @retval -1 监听 Socket 创建失败或 eloop 事件注册失败。
 * @pre eloop 必须已经初始化并处于可注册事件的状态。
 * @note 本函数可重复调用；事件注册失败时关闭监听 Socket 并恢复未启动状态。
 */
int psram_update_to_flash_app_ota_server_start(void)
{
    if (g_application_loader_server_fd >= 0) {
        LOG_D(
            "server already started: fd=%d\n", g_application_loader_server_fd);
        return RET_OK;
    }

    g_application_loader_server_fd = application_loader_tcp_create_server(
        PSRAM_UPDATE_TO_FLASH_APP_OTA_PORT);
    if (g_application_loader_server_fd < 0) {
        _os_printf("ApplicationLoader TCP OTA server create failed: port=%u\n",
                   PSRAM_UPDATE_TO_FLASH_APP_OTA_PORT);
        return RET_ERR;
    }

    g_application_loader_server_event = eloop_add_fd(
        g_application_loader_server_fd, EVENT_READ, EVENT_F_ENABLED,
        application_loader_tcp_accept,
        (void *)g_application_loader_server_fd);
    if (g_application_loader_server_event == NULL) {
        LOG_D(
            "eloop add fd failed: fd=%d\n", g_application_loader_server_fd);
        close(g_application_loader_server_fd);
        g_application_loader_server_fd = -1;
        return RET_ERR;
    }

    _os_printf("ApplicationLoader TCP OTA server started: port=%u\n",
               PSRAM_UPDATE_TO_FLASH_APP_OTA_PORT);
    return RET_OK;
}

/**
 * @brief 获取单连接 TCP OTA 任务的运行状态。
 * @retval 0 当前没有 OTA 任务运行。
 * @retval 非0 当前已有 OTA 任务运行。
 */
int psram_update_to_flash_app_ota_get_status(void)
{
    return (int)g_psram_update_to_flash_app_ota_running;
}

/**
 * @brief 通过已连接的 TCP Socket 接收固件并暂存到 PSRAM。
 * @param[in] tcp_connect_fd 已建立连接的 TCP Socket 描述符；本函数不关闭它。
 * @retval 0 PSRAM finish 正常返回成功。
 * @retval -1 ACK 发送失败，固件信息或正文接收失败，固件大小或内存检查
 *         失败，或者 PSRAM begin/write/finish 失败。
 * @details 设备先向客户端发送 36 字节 0xA1 ACK，再从客户端接收 20 字节
 *          固件信息并校验 file_size > 0；随后分配 1460 字节 SRAM 缓冲，
 *          begin 后循环从客户端接收固件正文并写入 PSRAM。正文完整后，当前
 *          实现先尝试向客户端发送 Upgrade_ok，再打印固件信息，以
 *          os_jiffies 生成 sequence，最后调用 finish。
 * @note 正文传输或 PSRAM 写入失败时调用 abort；退出时 result 非 0 会向
 *       客户端尝试发送 Upgrade_err，并释放已分配的 SRAM 缓冲。结果发送
 *       函数不检查底层发送返回值。finish 成功通常触发系统重启而不会按
 *       普通函数路径返回。
 * @warning Upgrade_ok 的发送尝试发生在 finish 之前。如果 finish 随后失败
 *          并返回，当前流程还会尝试发送 Upgrade_err，客户端可能依次收到
 *          两个结果；此处仅记录既有协议时序，不表示前一结果会被撤回。
 */
int psram_update_to_flash_app_ota_receive(int tcp_connect_fd)
{
    struct application_loader_tcp_header response;
    struct application_loader_tcp_firmware_info firmware_info;
    uint8 *receive_buffer = NULL;
    uint32 offset = 0U;
    uint32 next_progress = PSRAM_UPDATE_TO_FLASH_APP_OTA_PROGRESS_BYTES;
    uint32 sequence;
    int result = RET_ERR;

    LOG_D(
        "send ACK: command=0x%08x\n",
        PSRAM_UPDATE_TO_FLASH_APP_ACK_NET_UPGRADE);
    os_memset(&response, 0, sizeof(response));
    response.command = PSRAM_UPDATE_TO_FLASH_APP_ACK_NET_UPGRADE;
    if (application_loader_tcp_send_exact(tcp_connect_fd, &response,
                                          sizeof(response)) != RET_OK) {
        goto receive_exit;
    }

    os_memset(&firmware_info, 0, sizeof(firmware_info));
    if (application_loader_tcp_recv_exact(tcp_connect_fd, &firmware_info,
                                          sizeof(firmware_info)) != RET_OK) {
        LOG_D("firmware info receive failed\n");
        goto receive_exit;
    }
    if (firmware_info.file_size <= 0) {
        LOG_D("invalid firmware size: %d\n",
                         firmware_info.file_size);
        goto receive_exit;
    }

    LOG_D("firmware: version=%.16s, size=%d\n",
                                     firmware_info.version,
                                     firmware_info.file_size);

    receive_buffer =
        (uint8 *)os_malloc(PSRAM_UPDATE_TO_FLASH_APP_OTA_RECV_BYTES);
    if (receive_buffer == NULL) {
        LOG_D(
            "buffer allocation failed: bytes=%u\n",
            PSRAM_UPDATE_TO_FLASH_APP_OTA_RECV_BYTES);
        goto receive_exit;
    }

    LOG_D(
        "buffer allocated: addr=0x%08x, bytes=%u\n",
        (uint32)receive_buffer, PSRAM_UPDATE_TO_FLASH_APP_OTA_RECV_BYTES);

    if (psram_update_to_flash_app_begin(
            (uint32)firmware_info.file_size) != RET_OK) {
        LOG_D("PSRAM begin failed: size=%d\n",
                         firmware_info.file_size);
        goto receive_exit;
    }

    LOG_D("firmware transfer started\n");

    while (offset < (uint32)firmware_info.file_size) {
        uint32 remaining = (uint32)firmware_info.file_size - offset;
        uint32 receive_bytes = PSRAM_UPDATE_TO_FLASH_APP_OTA_RECV_BYTES;
        int received;

        if (receive_bytes > remaining) {
            receive_bytes = remaining;
        }

        received = recv(tcp_connect_fd, receive_buffer, receive_bytes, 0);
        if (received <= 0) {
            LOG_D(
                "firmware recv failed: result=%d, offset=%u/%d\n",
                received, offset, firmware_info.file_size);
            goto receive_abort;
        }
        if (psram_update_to_flash_app_write(receive_buffer,
                                            (uint32)received) != RET_OK) {
            LOG_D(
                "PSRAM write failed: offset=%u, bytes=%d\n",
                offset, received);
            goto receive_abort;
        }
        offset += (uint32)received;
        if ((offset >= next_progress) ||
            (offset == (uint32)firmware_info.file_size)) {
            LOG_D(
                "progress: %u/%d bytes (%u%%)\n",
                offset, firmware_info.file_size,
                (offset * 100U) / (uint32)firmware_info.file_size);
            while (next_progress <= offset) {
                next_progress += PSRAM_UPDATE_TO_FLASH_APP_OTA_PROGRESS_BYTES;
            }
        }
    }

    application_loader_tcp_send_result(tcp_connect_fd, 1);
    _os_printf("ApplicationLoader TCP OTA received: version=%.16s, size=%u\n",
               firmware_info.version, offset);

    sequence = (uint32)os_jiffies();
    LOG_D("finish update: sequence=%u\n", sequence);
    result = psram_update_to_flash_app_finish(sequence);
    LOG_D("finish returned: result=%d\n", result);
    goto receive_exit;

receive_abort:
    LOG_D("abort update: offset=%u/%d\n",
                                     offset, firmware_info.file_size);
    psram_update_to_flash_app_abort();
receive_exit:
    if (result != RET_OK) {
        LOG_D(
            "receive failed: result=%d, offset=%u\n", result, offset);
        application_loader_tcp_send_result(tcp_connect_fd, 0);
    }
    if (receive_buffer != NULL) {
        LOG_D("free buffer: addr=0x%08x\n",
                         (uint32)receive_buffer);
        os_free(receive_buffer);
    }
    return result;
}
