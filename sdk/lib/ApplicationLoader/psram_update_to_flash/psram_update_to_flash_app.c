/**
  ******************************************************************************
  * @file    psram_update_to_flash_app.c
    * @brief   应用侧 PSRAM 固件暂存及二次 Loader 升级请求发布实现。
    * @details 本文件负责固件 CRC32 累计、PSRAM 节点链表管理、上下文校验、
    *          Cache 清理以及升级请求发布。固件数据仅在 PSRAM 中暂存，最终由
    *          二次 Loader 按控制区请求将完整镜像写入 Flash。
    * @warning 本模块发布的节点地址和控制区字段构成跨重启协议，修改布局、写入
    *          顺序或 Cache 处理前必须同步核对二次 Loader 的解析约定。
  ******************************************************************************
  */
#include "basic_include.h"
#include "psram_update_to_flash_app.h"
#include "application_loader_protocol.h"


#ifndef RET_OK
/** @brief 模块内部使用的成功返回值。 */
#define RET_OK                               (0)
#endif

#ifndef RET_ERR
/** @brief 模块内部使用的失败返回值。 */
#define RET_ERR                              (-1)
#endif

/** @brief 从 PSRAM 堆分配内存，保证升级节点及固件数据位于 PSRAM。 */
#define AL_MALLOC                            os_malloc_psram
/** @brief 从 PSRAM 堆分配并清零内存；当前预留但尚未使用。 */
#define AL_ZALLOC                            os_zalloc_psram
/** @brief 将节点分配块归还 PSRAM 堆。 */
#define AL_FREE                              os_free_psram

/**
 * @brief 单个节点的总分配字节数。
 * @note 每块固定由 32 字节描述符存储区和 100 KiB 数据区组成。
 */
#define PSRAM_UPDATE_ALLOC_BYTES             (32U + PSRAM_UPDATE_TO_FLASH_APP_BLOCK_BYTES)
/**
 * @brief 节点分配块内为描述符保留的固定存储字节数。
 * @note 协议节点结构实际为 28 字节，数据区仍从偏移 32 字节开始，以满足
 *       32 字节布局和对齐要求。
 */
#define PSRAM_UPDATE_NODE_STORAGE_BYTES      (32U)
/** @brief CRC32 初值及最终异或值，均为 0xFFFFFFFF。 */
#define PSRAM_UPDATE_CRC32_INIT              (0xFFFFFFFFUL)
/** @brief 反射 CRC32 算法使用的多项式 0xEDB88320。 */
#define PSRAM_UPDATE_CRC32_POLY              (0xEDB88320UL)

#if PSRAM_UPDATE_TO_FLASH_APP_DEBUG
#define LOG_D(fmt, ...)   _os_printf("[ApplicationLoader][PSRAM] " fmt, ##__VA_ARGS__)
#else
#define LOG_D(fmt, ...)
#endif

/**
 * @brief 一次应用侧 PSRAM 固件暂存过程的运行上下文。
 * @note 除初始化入口外，公共操作通过 mutex 串行访问本结构中的链表和进度状态。
 */
struct psram_update_to_flash_app_context {
    os_mutex_t mutex;                    /**< 串行保护暂存状态和节点链表的互斥锁。 */
    struct psram_update_node *first;     /**< 节点链表首节点，发布给 Loader。 */
    struct psram_update_node *tail;      /**< 当前链表尾节点。 */
    uint8 *tail_data;                    /**< 当前尾节点固定 32 字节偏移后的数据区。 */
    uint32 tail_used;                    /**< 当前尾节点数据区已使用字节数。 */
    uint32 image_size;                   /**< begin 声明的完整固件镜像字节数。 */
    uint32 written_bytes;                /**< 本轮已按顺序暂存的累计字节数。 */
    uint32 image_crc32;                  /**< 尚未执行最终异或的镜像 CRC32 累计值。 */
    uint32 node_count;                   /**< 本轮已分配并链接的节点数量。 */
    uint8 initialized;                   /**< 互斥锁和控制区已初始化的标志。 */
    uint8 active;                        /**< 当前是否存在进行中的暂存过程。 */
};

/** @brief 模块唯一的固件暂存运行上下文。 */
static struct psram_update_to_flash_app_context g_psram_update;

/**
 * @brief 在既有 CRC32 状态上继续累计一段数据。
 * @param crc 调用前的 CRC32 累计值；首段应传入 0xFFFFFFFF。
 * @param data 待累计的数据起始地址。
 * @param data_bytes 待累计的数据字节数。
 * @return 尚未执行最终异或的 CRC32 累计值。
 * @note 算法按位使用反射多项式 0xEDB88320，支持分段顺序累计固件数据。
 */
static uint32 psram_update_crc32_accumulate(uint32 crc, const uint8 *data,
                                            uint32 data_bytes)
{
    uint32 byte_index;
    uint32 bit_index;

    for (byte_index = 0; byte_index < data_bytes; byte_index++) {
        crc ^= data[byte_index];
        for (bit_index = 0; bit_index < 8U; bit_index++) {
            if (crc & 1U) {
                crc = (crc >> 1) ^ PSRAM_UPDATE_CRC32_POLY;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

/**
 * @brief 计算一段连续数据的完整 CRC32。
 * @param data 待校验的数据起始地址。
 * @param data_bytes 待校验的数据字节数。
 * @return 使用初值 0xFFFFFFFF、反射多项式 0xEDB88320，并以
 *         0xFFFFFFFF 最终异或所得的 CRC32。
 */
static uint32 psram_update_crc32(const void *data, uint32 data_bytes)
{
    return psram_update_crc32_accumulate(PSRAM_UPDATE_CRC32_INIT,
                                         (const uint8 *)data,
                                         data_bytes) ^ PSRAM_UPDATE_CRC32_INIT;
}

/**
 * @brief 校验一段地址范围是否完整位于允许使用的 PSRAM 区域。
 * @param address 待校验范围的起始地址。
 * @param data_bytes 待校验范围的字节数。
 * @return 范围有效返回 0；地址为空、长度为零、地址加法溢出或范围超出
 *         [PSRAM_BASE, PSRAM_END_ADDR] 时返回 -1。
 */
static int psram_update_range_valid(const void *address, uint32 data_bytes)
{
    uint32 start = (uint32)address;
    uint32 end;

    if ((address == NULL) || (data_bytes == 0U)) {
        return RET_ERR;
    }

    end = start + data_bytes;
    if (end < start) {
        return RET_ERR;
    }

    if ((start < PSRAM_BASE) || (end > PSRAM_END_ADDR)) {
        return RET_ERR;
    }

    return RET_OK;
}

/**
 * @brief 清零应用与二次 Loader 共享的升级请求控制区。
 * @note 在开始、中止、失败回滚或发布新请求前调用，用于移除旧升级请求，
 *       避免 Loader 将残留 magic 解释为有效请求。
 */
static void psram_update_request_clear(void)
{
    volatile uint32 *control = (volatile uint32 *)PSRAM_UPDATE_CONTROL_ADDR;
    uint32 index;

    for (index = 0;
         index < (PSRAM_UPDATE_CONTROL_BYTES / sizeof(uint32));
         index++) {
        control[index] = 0U;
    }
    LOG_D("control cleared: addr=0x%08x, bytes=%u\n",
                         PSRAM_UPDATE_CONTROL_ADDR,
                         PSRAM_UPDATE_CONTROL_BYTES);
}

/**
 * @brief 释放本轮暂存过程分配的全部 PSRAM 节点并复位运行状态。
 * @note 释放过程按协议字段 next_node_addr 遍历，遇到
 *       PSRAM_UPDATE_NODE_END 终止；随后重置链表、进度、CRC、节点计数和
 *       active 标志，但保留已创建的互斥锁及 initialized 状态。
 */
static void psram_update_free_nodes(void)
{
    struct psram_update_node *node = g_psram_update.first;
    uint32 freed_count = 0U;

    while (node != NULL) {
        struct psram_update_node *next;

        if (node->next_node_addr == PSRAM_UPDATE_NODE_END) {
            next = NULL;
        } else {
            next = (struct psram_update_node *)node->next_node_addr;
        }
        LOG_D(
            "free node[%u]: node=0x%08x, data=0x%08x, bytes=%u, next=0x%08x\n",
            freed_count, (uint32)node, node->data_addr, node->data_bytes,
            node->next_node_addr);
        AL_FREE(node);
        node = next;
        freed_count++;
    }

    g_psram_update.first = NULL;
    g_psram_update.tail = NULL;
    g_psram_update.tail_data = NULL;
    g_psram_update.tail_used = 0U;
    g_psram_update.image_size = 0U;
    g_psram_update.written_bytes = 0U;
    g_psram_update.image_crc32 = PSRAM_UPDATE_CRC32_INIT;
    g_psram_update.node_count = 0U;
    g_psram_update.active = 0U;
    if (freed_count != 0U) {
        LOG_D("all nodes freed: count=%u\n", freed_count);
    }
}

/**
 * @brief 为顺序写入分配并链接一个新的 PSRAM 暂存节点。
 * @return 分配、布局及范围检查均成功返回 0，否则返回 -1。
 * @note 节点数不得超过协议上限；分配起始地址必须按 32 字节对齐，且
 *       32 + 100 KiB 的整个分配块必须位于 PSRAM。节点结构虽为 28 字节，
 *       数据区仍固定从分配块偏移 32 字节处开始。
 */
static int psram_update_allocate_node(void)
{
    uint8 *storage;
    struct psram_update_node *node;

    if (g_psram_update.node_count >= PSRAM_UPDATE_MAX_NODE_COUNT) {
        LOG_D("node count limit reached: count=%u\n",
                             g_psram_update.node_count);
        return RET_ERR;
    }

    storage = (uint8 *)AL_MALLOC(PSRAM_UPDATE_ALLOC_BYTES);
    if (storage == NULL) {
        LOG_D("node allocation failed: bytes=%u\n",
                             PSRAM_UPDATE_ALLOC_BYTES);
        return RET_ERR;
    }

    if ((((uint32)storage) & 0x1FU) != 0U) {
        LOG_D("node alignment invalid: addr=0x%08x\n",
                             (uint32)storage);
        AL_FREE(storage);
        return RET_ERR;
    }

    memset(storage, 0, PSRAM_UPDATE_ALLOC_BYTES);
    node = (struct psram_update_node *)storage;
    node->magic = PSRAM_UPDATE_NODE_MAGIC;
    node->version = PSRAM_UPDATE_PROTOCOL_VERSION;
    node->header_bytes = (uint16)sizeof(struct psram_update_node);
    node->next_node_addr = PSRAM_UPDATE_NODE_END;
    node->data_addr = (uint32)(storage + PSRAM_UPDATE_NODE_STORAGE_BYTES);
    node->data_bytes = 0U;
    node->firmware_offset = g_psram_update.written_bytes;
    node->header_crc32 = 0U;

    if (psram_update_range_valid(node, PSRAM_UPDATE_ALLOC_BYTES) != RET_OK) {
        LOG_D("node range invalid: addr=0x%08x, bytes=%u\n",
                             (uint32)node, PSRAM_UPDATE_ALLOC_BYTES);
        AL_FREE(storage);
        return RET_ERR;
    }

    if (g_psram_update.tail != NULL) {
        g_psram_update.tail->next_node_addr = (uint32)node;
    } else {
        g_psram_update.first = node;
    }

    g_psram_update.tail = node;
    g_psram_update.tail_data = storage + PSRAM_UPDATE_NODE_STORAGE_BYTES;
    g_psram_update.tail_used = 0U;
    g_psram_update.node_count++;

    LOG_D(
        "node allocated[%u]: node=0x%08x, data=0x%08x, capacity=%u, offset=%u\n",
        g_psram_update.node_count - 1U, (uint32)node, node->data_addr,
        PSRAM_UPDATE_TO_FLASH_APP_BLOCK_BYTES, node->firmware_offset);

    return RET_OK;
}

/**
 * @brief 在发布请求前校验当前 PSRAM 节点链表及镜像布局的一致性。
 * @return 魔数、协议版本、头长度、连续固件偏移、非零数据长度、节点容量、
 *         非末节点满载要求、数据范围、节点数及总大小全部有效时返回 0，
 *         任一条件不满足时返回 -1。
 * @warning 本函数验证的是将由二次 Loader 消费的完整上下文；校验失败时
 *          调用方不得发布升级请求。
 */
static int psram_update_validate_context(void)
{
    struct psram_update_node *node = g_psram_update.first;
    uint32 expected_offset = 0U;
    uint32 node_count = 0U;

    while (node != NULL) {
        if (psram_update_range_valid(node,
                                     PSRAM_UPDATE_NODE_STORAGE_BYTES) != RET_OK) {
            return RET_ERR;
        }
        if ((node->magic != PSRAM_UPDATE_NODE_MAGIC) ||
            (node->version != PSRAM_UPDATE_PROTOCOL_VERSION) ||
            (node->header_bytes != sizeof(struct psram_update_node)) ||
            (node->firmware_offset != expected_offset) ||
            (node->data_bytes == 0U) ||
            (node->data_bytes > PSRAM_UPDATE_TO_FLASH_APP_BLOCK_BYTES) ||
            ((node->next_node_addr != PSRAM_UPDATE_NODE_END) &&
             (node->data_bytes != PSRAM_UPDATE_TO_FLASH_APP_BLOCK_BYTES)) ||
            (psram_update_range_valid((const void *)node->data_addr,
                                      node->data_bytes) != RET_OK)) {
            return RET_ERR;
        }

        expected_offset += node->data_bytes;
        node_count++;
        if (node_count > PSRAM_UPDATE_MAX_NODE_COUNT) {
            return RET_ERR;
        }

        if (node->next_node_addr == PSRAM_UPDATE_NODE_END) {
            node = NULL;
        } else {
            node = (struct psram_update_node *)node->next_node_addr;
        }
    }

    if ((node_count != g_psram_update.node_count) ||
        (expected_offset != g_psram_update.image_size)) {
        return RET_ERR;
    }

    return RET_OK;
}

/**
 * @brief 最终化所有节点头 CRC，并将节点数据和描述符写回 PSRAM。
 * @note 每个节点先计算头部 CRC32，再分别清理实际数据区和固定 32 字节
 *       描述符存储区的 D-Cache，确保二次 Loader 观察到一致内容。
 */
static void psram_update_finalize_and_clean_nodes(void)
{
    struct psram_update_node *node = g_psram_update.first;

    while (node != NULL) {
        struct psram_update_node *next;

        node->header_crc32 = psram_update_crc32(
            node, PSRAM_UPDATE_NODE_CRC_BYTES);
        LOG_D(
            "finalize node: node=0x%08x, data=0x%08x, bytes=%u, "
            "offset=%u, next=0x%08x, header_crc=0x%08x\n",
            (uint32)node, node->data_addr, node->data_bytes,
            node->firmware_offset, node->next_node_addr,
            node->header_crc32);
        sys_dcache_clean_range((uint32 *)node->data_addr,
                               (int32_t)node->data_bytes);
        sys_dcache_clean_range((uint32 *)node,
                               (int32_t)PSRAM_UPDATE_NODE_STORAGE_BYTES);
        LOG_D(
            "cache clean: data=0x%08x/%u, node=0x%08x/%u\n",
            node->data_addr, node->data_bytes, (uint32)node,
            PSRAM_UPDATE_NODE_STORAGE_BYTES);

        if (node->next_node_addr == PSRAM_UPDATE_NODE_END) {
            next = NULL;
        } else {
            next = (struct psram_update_node *)node->next_node_addr;
        }
        node = next;
    }
}

/**
 * @brief 在共享控制区发布一条完整的 PSRAM 升级请求。
 * @param sequence 本次升级请求的序列号。
 * @note 发布时先清除旧控制区，再写入除 magic 外的全部字段，最后写 magic；
 *       magic 作为有效性提交标志，使 Loader 只观察到完整请求。
 */
static void psram_update_publish_request(uint32 sequence)
{
    struct psram_update_request request;
    volatile struct psram_update_request *control =
        (volatile struct psram_update_request *)PSRAM_UPDATE_CONTROL_ADDR;

    memset(&request, 0, sizeof(request));
    request.magic = PSRAM_UPDATE_MAGIC;
    request.version = PSRAM_UPDATE_PROTOCOL_VERSION;
    request.header_bytes = (uint16)sizeof(request);
    request.first_node_addr = (uint32)g_psram_update.first;
    request.image_size = g_psram_update.image_size;
    request.image_crc32 = g_psram_update.image_crc32 ^ PSRAM_UPDATE_CRC32_INIT;
    request.node_count = g_psram_update.node_count;
    request.flags = PSRAM_UPDATE_FLAG_NONE;
    request.sequence = sequence;
    request.header_crc32 = psram_update_crc32(
        &request, PSRAM_UPDATE_REQUEST_CRC_BYTES);

    LOG_D(
        "publish request: control=0x%08x, first=0x%08x, size=%u, "
        "image_crc=0x%08x, nodes=%u, sequence=%u, header_crc=0x%08x\n",
        PSRAM_UPDATE_CONTROL_ADDR, request.first_node_addr,
        request.image_size, request.image_crc32, request.node_count,
        request.sequence, request.header_crc32);

    psram_update_request_clear();
    control->version = request.version;
    control->header_bytes = request.header_bytes;
    control->first_node_addr = request.first_node_addr;
    control->image_size = request.image_size;
    control->image_crc32 = request.image_crc32;
    control->node_count = request.node_count;
    control->flags = request.flags;
    control->sequence = request.sequence;
    control->header_crc32 = request.header_crc32;
    control->magic = request.magic;
}

/**
 * @brief 初始化应用侧 PSRAM 固件暂存模块。
 * @return 互斥锁创建并完成控制区清理返回 0；互斥锁创建失败返回 -1。
 * @note 本接口可重复调用；已经初始化时直接返回成功。首次成功初始化会将
 *       镜像 CRC 累计值设为 0xFFFFFFFF，并清除可能残留的旧升级请求。
 */
int psram_update_to_flash_app_init(void)
{
    int result = RET_OK;

    if (g_psram_update.initialized != 0U) {
        LOG_D("init skipped: already initialized\n");
        return RET_OK;
    }

    memset(&g_psram_update, 0, sizeof(g_psram_update));
    if (os_mutex_init(&g_psram_update.mutex) != RET_OK) {
        LOG_D("mutex initialization failed\n");
        result = RET_ERR;
    } else {
        g_psram_update.image_crc32 = PSRAM_UPDATE_CRC32_INIT;
        g_psram_update.initialized = 1U;
        psram_update_request_clear();
        LOG_D("initialized: block=%u, allocation=%u\n",
                     PSRAM_UPDATE_TO_FLASH_APP_BLOCK_BYTES,
                     PSRAM_UPDATE_ALLOC_BYTES);
    }

    return result;
}

/**
 * @brief 开始一次新的 APP 固件 PSRAM 暂存过程。
 * @param image_size 本次完整固件镜像的总字节数，必须大于 0。
 * @return 成功建立活动暂存上下文返回 0；自动初始化失败、镜像大小为零或
 *         已存在活动升级时返回 -1。
 * @note 本接口会自动初始化模块，并在成功开始前释放旧节点、清控制区和
 *       重置本轮 CRC 状态。
 */
int psram_update_to_flash_app_begin(uint32 image_size)
{
    int result = RET_OK;

    if ((g_psram_update.initialized == 0U) &&
        (psram_update_to_flash_app_init() != RET_OK)) {
        return RET_ERR;
    }
    if (image_size == 0U) {
        LOG_D("begin rejected: image size is zero\n");
        return RET_ERR;
    }

    os_mutex_lock(&g_psram_update.mutex, osWaitForever);
    if (g_psram_update.active != 0U) {
        LOG_D("begin rejected: update already active\n");
        result = RET_ERR;
    } else {
        psram_update_free_nodes();
        psram_update_request_clear();
        g_psram_update.image_size = image_size;
        g_psram_update.image_crc32 = PSRAM_UPDATE_CRC32_INIT;
        g_psram_update.active = 1U;
        LOG_D("begin: image_size=%u\n", image_size);
    }
    os_mutex_unlock(&g_psram_update.mutex);

    return result;
}

/**
 * @brief 按镜像偏移顺序将一段固件数据追加到 PSRAM 节点链表。
 * @param data 待写入的固件数据起始地址，必须非空。
 * @param data_bytes 本次追加的数据字节数，必须非零。
 * @return 数据全部顺序追加成功返回 0；参数无效、模块未初始化、无活动升级、
 *         累计数据超过声明总大小或节点分配失败时返回 -1。
 * @note 写入过程按 100 KiB 节点容量自动跨节点，并同步累计整镜像 CRC32。
 * @warning 节点分配失败会清除升级请求、释放整条节点链并重置本轮状态，
 *          调用方需要重新 begin 后才能继续。
 */
int psram_update_to_flash_app_write(const uint8 *data, uint32 data_bytes)
{
    uint32 copied_bytes = 0U;
    int result = RET_OK;

    if ((data == NULL) || (data_bytes == 0U) ||
        (g_psram_update.initialized == 0U)) {
        LOG_D(
            "write rejected: data=0x%08x, bytes=%u, initialized=%u\n",
            (uint32)data, data_bytes, g_psram_update.initialized);
        return RET_ERR;
    }

    os_mutex_lock(&g_psram_update.mutex, osWaitForever);
    if ((g_psram_update.active == 0U) ||
        (g_psram_update.written_bytes > g_psram_update.image_size) ||
        (data_bytes > (g_psram_update.image_size -
                       g_psram_update.written_bytes))) {
        LOG_D(
            "write invalid: active=%u, written=%u, input=%u, total=%u\n",
            g_psram_update.active, g_psram_update.written_bytes,
            data_bytes, g_psram_update.image_size);
        result = RET_ERR;
        goto write_exit;
    }

    while (copied_bytes < data_bytes) {
        uint32 free_bytes;
        uint32 copy_bytes;

        if ((g_psram_update.tail == NULL) ||
            (g_psram_update.tail_used ==
             PSRAM_UPDATE_TO_FLASH_APP_BLOCK_BYTES)) {
            if (psram_update_allocate_node() != RET_OK) {
                LOG_D(
                    "write node allocation failed: progress=%u/%u\n",
                    g_psram_update.written_bytes,
                    g_psram_update.image_size);
                result = RET_ERR;
                psram_update_request_clear();
                psram_update_free_nodes();
                break;
            }
        }

        free_bytes = PSRAM_UPDATE_TO_FLASH_APP_BLOCK_BYTES -
                     g_psram_update.tail_used;
        copy_bytes = data_bytes - copied_bytes;
        if (copy_bytes > free_bytes) {
            copy_bytes = free_bytes;
        }

        memcpy(g_psram_update.tail_data + g_psram_update.tail_used,
               data + copied_bytes, copy_bytes);
        g_psram_update.image_crc32 = psram_update_crc32_accumulate(
            g_psram_update.image_crc32, data + copied_bytes, copy_bytes);
        g_psram_update.tail_used += copy_bytes;
        g_psram_update.tail->data_bytes = g_psram_update.tail_used;
        g_psram_update.written_bytes += copy_bytes;
        copied_bytes += copy_bytes;

        if ((g_psram_update.tail_used ==
             PSRAM_UPDATE_TO_FLASH_APP_BLOCK_BYTES) ||
            (g_psram_update.written_bytes == g_psram_update.image_size)) {
            LOG_D(
            "node complete: node=0x%08x, bytes=%u, progress=%u/%u\n",
            (uint32)g_psram_update.tail,
            g_psram_update.tail->data_bytes,
            g_psram_update.written_bytes, g_psram_update.image_size);
        }
    }

write_exit:
    os_mutex_unlock(&g_psram_update.mutex);
    return result;
}

/**
 * @brief 完成本轮暂存并向二次 Loader 发布升级请求。
 * @param sequence 本次升级请求的序列号。
 * @return 数据完整、上下文有效且平台静默成功时返回 0；否则返回 -1。
 * @note 成功路径先最终化节点头 CRC 并清理 D-Cache，再调用平台静默钩子，
 *       随后发布请求并调用 rom_qspi_reboot_trampoline()，通常不会返回。
 * @warning 必须已写满 begin 声明的镜像大小且节点上下文校验成功；本函数
 *          不会在校验或平台静默失败时发布请求。
 */
int psram_update_to_flash_app_finish(uint32 sequence)
{
    int result = RET_OK;

    if (g_psram_update.initialized == 0U) {
        LOG_D("finish rejected: not initialized\n");
        return RET_ERR;
    }

    os_mutex_lock(&g_psram_update.mutex, osWaitForever);
    if ((g_psram_update.active == 0U) ||
        (g_psram_update.written_bytes != g_psram_update.image_size) ||
        (g_psram_update.first == NULL) ||
        (psram_update_validate_context() != RET_OK)) {
        LOG_D(
            "finish validation failed: active=%u, written=%u, total=%u, "
            "first=0x%08x, nodes=%u\n",
            g_psram_update.active, g_psram_update.written_bytes,
            g_psram_update.image_size, (uint32)g_psram_update.first,
            g_psram_update.node_count);
        result = RET_ERR;
        goto finish_exit;
    }

    psram_update_finalize_and_clean_nodes();
    LOG_D(
        "nodes finalized: count=%u, image_crc=0x%08x\n",
        g_psram_update.node_count,
        g_psram_update.image_crc32 ^ PSRAM_UPDATE_CRC32_INIT);
    if (psram_update_to_flash_app_platform_quiesce() != RET_OK) {
        LOG_D("platform quiesce failed\n");
        result = RET_ERR;
        goto finish_exit;
    }

    psram_update_publish_request(sequence);
    g_psram_update.active = 0U;
    os_mutex_unlock(&g_psram_update.mutex);

    LOG_D("finish complete: request published\n");
    _os_printf("PSRAM update request ready: size=%u, nodes=%u, sequence=%u\n",
               g_psram_update.image_size, g_psram_update.node_count, sequence);


    rom_qspi_reboot_trampoline(1);
    return result;

finish_exit:
    os_mutex_unlock(&g_psram_update.mutex);
    return result;
}

/**
 * @brief 中止当前 PSRAM 固件暂存过程。
 * @return 始终返回 0；模块尚未初始化时也视为成功。
 * @note 已初始化时会先清除共享控制区，再释放整条节点链并重置本轮状态。
 */
int psram_update_to_flash_app_abort(void)
{
    if (g_psram_update.initialized == 0U) {
        LOG_D("abort skipped: not initialized\n");
        return RET_OK;
    }

    os_mutex_lock(&g_psram_update.mutex, osWaitForever);
    psram_update_request_clear();
    psram_update_free_nodes();
    os_mutex_unlock(&g_psram_update.mutex);

    LOG_D("abort complete\n");

    return RET_OK;
}

/**
 * @brief 在发布升级请求前执行平台业务静默的弱符号钩子。
 * @return 默认实现返回 0；平台覆盖实现应在静默成功时返回 0，失败时返回 -1。
 * @note 应用可覆盖本弱实现，用于停止网络、媒体、存储、CPU1 业务及其他
 *       PSRAM 使用者；返回失败会阻止升级请求发布。
 */
__attribute__((weak)) int psram_update_to_flash_app_platform_quiesce(void)
{
    return RET_OK;
}
