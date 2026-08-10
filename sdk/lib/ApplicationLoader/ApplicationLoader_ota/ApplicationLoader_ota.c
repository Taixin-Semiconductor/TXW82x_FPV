/**
 ******************************************************************************
 * @file    ApplicationLoader_ota.c
 * @brief   ApplicationLoader TCP 20203 OTA 服务实现。
 * @details 创建单连接 TCP 服务，按 36/20 字节协议接收 Loader 镜像，并将
 *          正文按连续偏移顺序提交给 ApplicationLoader_ota_write_fw()。
 * @note    成功完成后仅回复 Upgrade_ok，不执行重启或启动元数据切换。
 ******************************************************************************
 */
#include "basic_include.h"
#include "lwip/sockets.h"
#include "lib/net/eloop/eloop.h"
#include "lib/ApplicationLoader/ApplicationLoader_ota/ApplicationLoader_ota.h"
#include "lib/ApplicationLoader/ApplicationLoader_ota/ApplicationLoader_ota_api.h"

#define APPLICATION_LOADER_OTA_RECV_BYTES  (1460U)
#define APPLICATION_LOADER_OTA_BACKLOG     (3)
#define APPLICATION_LOADER_OTA_STACK_BYTES (2048U)
#define APPLICATION_LOADER_OTA_RESULT_OK   "Upgrade_ok"
#define APPLICATION_LOADER_OTA_RESULT_ERR  "Upgrade_err"

#if APPLICATION_LOADER_OTA_TCP_DEBUG
#define LOG_D(fmt, ...) \
    os_printf("[ApplicationLoader OTA] " fmt, ##__VA_ARGS__)
#else
#define LOG_D(fmt, ...)
#endif

#define APPLICATION_LOADER_OTA_ERROR_LOG(fmt, ...) \
    os_printf("[ApplicationLoader OTA] " fmt, ##__VA_ARGS__)

/** @brief TCP OTA 请求和 ACK 使用的固定 36 字节协议头。 */
struct application_loader_ota_tcp_header {
    uint32 command;
    uint32 reserved[8];
};

/** @brief TCP OTA 固件描述使用的固定 20 字节协议结构。 */
struct application_loader_ota_firmware_info {
    char version[16];
    int32 file_size;
};

typedef char application_loader_ota_header_size_must_be_36[
    sizeof(struct application_loader_ota_tcp_header) == 36U ? 1 : -1];
typedef char application_loader_ota_info_size_must_be_20[
    sizeof(struct application_loader_ota_firmware_info) == 20U ? 1 : -1];

static int g_application_loader_ota_server_fd = -1;
static EVT_HDL g_application_loader_ota_server_event;
static uint8 g_application_loader_ota_running;

/**
 * @brief 从 TCP 字节流精确接收指定长度。
 * @retval 0 已完整接收。
 * @retval -1 recv 返回零或负数。
 */
static int application_loader_ota_recv_exact(int socket_fd, void *buffer,
                                              uint32 bytes)
{
    uint8 *position = (uint8 *)buffer;
    uint32 received = 0U;

    while (received < bytes) {
        int result = recv(socket_fd, position + received, bytes - received, 0);

        if (result <= 0) {
            return -1;
        }
        received += (uint32)result;
    }
    return 0;
}

/**
 * @brief 向 TCP 字节流精确发送指定长度。
 * @retval 0 已完整发送。
 * @retval -1 send 返回零或负数。
 */
static int application_loader_ota_send_exact(int socket_fd, const void *buffer,
                                              uint32 bytes)
{
    const uint8 *position = (const uint8 *)buffer;
    uint32 sent = 0U;

    while (sent < bytes) {
        int result = send(socket_fd, position + sent, bytes - sent, 0);

        if (result <= 0) {
            return -1;
        }
        sent += (uint32)result;
    }
    return 0;
}

/** @brief 尝试发送失败结果，发送失败不改变原始错误。 */
static void application_loader_ota_send_error(int socket_fd)
{
    (void)application_loader_ota_send_exact(
        socket_fd, APPLICATION_LOADER_OTA_RESULT_ERR,
        (uint32)sizeof(APPLICATION_LOADER_OTA_RESULT_ERR));
}

/** @brief 创建、配置并监听固定端口的 TCP Socket。 */
static int application_loader_ota_create_server(void)
{
    struct sockaddr_in address;
    int socket_fd;
    int reuse_address = 1;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        return -1;
    }
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_address,
                   sizeof(reuse_address)) < 0) {
        close(socket_fd);
        return -1;
    }

    os_memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons((uint16)APPLICATION_LOADER_OTA_PORT);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        close(socket_fd);
        return -1;
    }
    if (listen(socket_fd, APPLICATION_LOADER_OTA_BACKLOG) < 0) {
        close(socket_fd);
        return -1;
    }
    return socket_fd;
}

/** @brief 处理一个已接受连接，退出时关闭连接并清除运行状态。 */
static void application_loader_ota_task(void *argument)
{
    struct application_loader_ota_tcp_header request;
    int tcp_connect_fd = (int)(intptr_t)argument;

    os_memset(&request, 0, sizeof(request));
    if (application_loader_ota_recv_exact(tcp_connect_fd, &request,
                                           sizeof(request)) == 0) {
        if (request.command == APPLICATION_LOADER_OTA_REQ_NET_UPGRADE) {
            (void)ApplicationLoader_ota_receive(tcp_connect_fd);
        }
    }
    close(tcp_connect_fd);
    g_application_loader_ota_running = 0U;
}

/** @brief 接受单连接，配置双向超时并创建 OTA 任务。 */
static void application_loader_ota_accept(EVT_HDL event, void *data)
{
    struct sockaddr_in client_address;
    socklen_t address_bytes = sizeof(client_address);
    int server_fd = (int)(intptr_t)data;
    int tcp_connect_fd;
    int timeout_ms = APPLICATION_LOADER_OTA_SOCKET_TIMEOUT_MS;
    void *task_handle;

    (void)event;
    tcp_connect_fd = accept(server_fd, (struct sockaddr *)&client_address,
                            &address_bytes);
    if (tcp_connect_fd < 0) {
        return;
    }
    if (g_application_loader_ota_running != 0U) {
        close(tcp_connect_fd);
        return;
    }
    if (setsockopt(tcp_connect_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout_ms,
                   sizeof(timeout_ms)) < 0) {
        close(tcp_connect_fd);
        return;
    }
    if (setsockopt(tcp_connect_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout_ms,
                   sizeof(timeout_ms)) < 0) {
        close(tcp_connect_fd);
        return;
    }

    g_application_loader_ota_running = 1U;
    task_handle = os_task_create(
        "loader_ota", application_loader_ota_task,
        (void *)(intptr_t)tcp_connect_fd, OS_TASK_PRIORITY_NORMAL, 0U, NULL,
        APPLICATION_LOADER_OTA_STACK_BYTES);
    if (task_handle == NULL) {
        g_application_loader_ota_running = 0U;
        close(tcp_connect_fd);
    }
}

int ApplicationLoader_ota_server_start(void)
{
    if (g_application_loader_ota_server_fd >= 0) {
        return 0;
    }

    g_application_loader_ota_server_fd =
        application_loader_ota_create_server();
    if (g_application_loader_ota_server_fd < 0) {
        return -1;
    }
    g_application_loader_ota_server_event = eloop_add_fd(
        g_application_loader_ota_server_fd, EVENT_READ, EVENT_F_ENABLED,
        application_loader_ota_accept,
        (void *)(intptr_t)g_application_loader_ota_server_fd);
    if (g_application_loader_ota_server_event == NULL) {
        close(g_application_loader_ota_server_fd);
        g_application_loader_ota_server_fd = -1;
        return -1;
    }

    LOG_D("server started: port=%u\n",
                               APPLICATION_LOADER_OTA_PORT);
    return 0;
}

int ApplicationLoader_ota_get_status(void)
{
    return (int)g_application_loader_ota_running;
}

int ApplicationLoader_ota_receive(int tcp_connect_fd)
{
    struct application_loader_ota_tcp_header response;
    struct application_loader_ota_firmware_info firmware_info;
    uint8 *receive_buffer = NULL;
    uint32 total_bytes;
    uint32 offset = 0U;

    os_memset(&response, 0, sizeof(response));
    response.command = APPLICATION_LOADER_OTA_ACK_NET_UPGRADE;
    if (application_loader_ota_send_exact(tcp_connect_fd, &response,
                                           sizeof(response)) != 0) {
        LOG_D("ACK send failed\n");
        goto receive_failed;
    }

    os_memset(&firmware_info, 0, sizeof(firmware_info));
    if (application_loader_ota_recv_exact(tcp_connect_fd, &firmware_info,
                                           sizeof(firmware_info)) != 0) {
        LOG_D("firmware info recv failed\n");
        goto receive_failed;
    }
    if (firmware_info.file_size <= 0) {
        LOG_D("invalid file size: %d\n",
                                   (int)firmware_info.file_size);
        goto receive_failed;
    }
    total_bytes = (uint32)firmware_info.file_size;
    LOG_D("firmware info: version=%.16s total=%u\n",
                               firmware_info.version,
                               (unsigned int)total_bytes);

    receive_buffer = (uint8 *)os_malloc(APPLICATION_LOADER_OTA_RECV_BYTES);
    if (receive_buffer == NULL) {
        LOG_D("malloc failed: bytes=%u\n",
                                   (unsigned int)
                                       APPLICATION_LOADER_OTA_RECV_BYTES);
        goto receive_failed;
    }

    while (offset < total_bytes) {
        uint32 remaining = total_bytes - offset;
        uint32 requested = APPLICATION_LOADER_OTA_RECV_BYTES;
        int received;
        int32 write_result;

        if (requested > remaining) {
            requested = remaining;
        }
        received = recv(tcp_connect_fd, receive_buffer, requested, 0);
        if (received <= 0) {
            LOG_D(
                "body recv failed: received=%d offset=%u total=%u\n",
                received, (unsigned int)offset, (unsigned int)total_bytes);
            goto receive_failed;
        }
        write_result = ApplicationLoader_ota_write_fw(
            total_bytes, offset, receive_buffer, (uint16)received);
        if (write_result != 0) {
            LOG_D(
                "write failed: error=%d offset=%u len=%u total=%u\n",
                (int)write_result, (unsigned int)offset,
                (unsigned int)received, (unsigned int)total_bytes);
            goto receive_failed;
        }
        offset += (uint32)received;
    }

    if (application_loader_ota_send_exact(
            tcp_connect_fd, APPLICATION_LOADER_OTA_RESULT_OK,
            (uint32)sizeof(APPLICATION_LOADER_OTA_RESULT_OK)) != 0) {
        LOG_D("OK send failed: offset=%u total=%u\n",
                                   (unsigned int)offset,
                                   (unsigned int)total_bytes);
        goto receive_failed;
    }
    os_printf("upgrade successful, please restart the device manually\n");

    os_free(receive_buffer);
    return 0;

receive_failed:
    ApplicationLoader_ota_abort();
    application_loader_ota_send_error(tcp_connect_fd);
    if (receive_buffer != NULL) {
        os_free(receive_buffer);
    }
    return -1;
}
