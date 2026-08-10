/**
 * @file app_mem.c
 * @brief 应用层大内存模块的内存分配策略控制
 *
 * @section description 描述
 * 本文件集中管理具有大内存需求的模块（如编解码器、协议栈）的内存分配方式。
 * 通过统一修改底层的 __malloc/__free 调用目标，可快速在不同内存池（SRAM/PSRAM）间切换。
 *
 * @section constraints 使用约束
 * - 仅在此文件定义基础的 xx_alloc / xx_free 配对 API。
 * - 若有 realloc/zalloc/calloc 需求，请在其他文件基于本文件的 API 进行封装。
 * - **重要**：修改内存分配策略时，必须同时修改对应的 alloc 和 free 函数，确保内存池一致。
 */

#include "sys_config.h"
#include "basic_include.h"

/**
 * @addtogroup Memory_Allocation 内存分配模块
 * @{
 */

///////////////////////////////////////////////////////////////////////////////////
/**
 * @brief libCurl 协议库内存分配接口
 * 为 libCurl 提供动态内存分配服务。可根据系统负载情况，选择从 SRAM 或 PSRAM 分配。
 */
void *libcurl_mem_alloc(uint32_t size)
{
#ifndef PSRAM_HEAP
    return __malloc((struct sys_heap *)&sram_heap, size, RETURN_ADDR());    //从SRAM内存池分配
#else
    return __malloc((struct sys_heap *)&psram_heap, size, RETURN_ADDR());   //从PSRAM内存池分配
#endif
}
void libcurl_mem_free(void *ptr)
{
#ifndef PSRAM_HEAP
    __free((struct sys_heap *)&sram_heap, ptr, RETURN_ADDR());
#else
    __free((struct sys_heap *)&psram_heap, ptr, RETURN_ADDR());
#endif
}

///////////////////////////////////////////////////////////////////////////////////
/**
 * @brief FrameBuffer 默认内存分配接口
 * 为视频/音频帧缓冲提供内存分配。由于帧缓冲通常较大，建议根据实际带宽需求选择内存池。
 * @note 通常是结合framebuff预分配池fbpool，控制framebuff分配数量，避免单一模块分配大量的framebuff消耗过多的内存。
 */
void *fb_mem_alloc(uint32_t size)
{
#ifndef PSRAM_HEAP
    return __malloc((struct sys_heap *)&sram_heap, size, RETURN_ADDR());    //从SRAM内存池分配
#else
    return __malloc((struct sys_heap *)&psram_heap, size, RETURN_ADDR());   //从PSRAM内存池分配
#endif
}
void fb_mem_free(void *ptr)
{
#ifndef PSRAM_HEAP
    __free((struct sys_heap *)&sram_heap, ptr, RETURN_ADDR());
#else
    __free((struct sys_heap *)&psram_heap, ptr, RETURN_ADDR());
#endif
}

///////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 音视频解码器内存分配接口
 * 为解码器内部状态、参考帧缓冲等提供内存。
 */
void *decoder_mem_alloc(uint32_t size)
{
#ifndef PSRAM_HEAP
    return __malloc((struct sys_heap *)&sram_heap, size, RETURN_ADDR());    //从SRAM内存池分配
#else
    return __malloc((struct sys_heap *)&psram_heap, size, RETURN_ADDR());   //从PSRAM内存池分配
#endif
}
void decoder_mem_free(void *ptr)
{
#ifndef PSRAM_HEAP
    __free((struct sys_heap *)&sram_heap, ptr, RETURN_ADDR());
#else
    __free((struct sys_heap *)&psram_heap, ptr, RETURN_ADDR());
#endif
}

///////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 音视频编码器内存分配接口
 * 为编码器内部状态、率控制缓冲等提供内存。
 */
void *encoder_mem_alloc(uint32_t size)
{
#ifndef PSRAM_HEAP
    return __malloc((struct sys_heap *)&sram_heap, size, RETURN_ADDR());    //从SRAM内存池分配
#else
    return __malloc((struct sys_heap *)&psram_heap, size, RETURN_ADDR());   //从PSRAM内存池分配
#endif
}
void encoder_mem_free(void *ptr)
{
#ifndef PSRAM_HEAP
    __free((struct sys_heap *)&sram_heap, ptr, RETURN_ADDR());
#else
    __free((struct sys_heap *)&psram_heap, ptr, RETURN_ADDR());
#endif
}

///////////////////////////////////////////////////////////////////////////////////
/**
 * @brief mbedTLS 加密库内存分配接口
 * 为 SSL/TLS 握手、加解密缓冲提供内存。
 */
void *mbedtls_mem_alloc(uint32_t size)
{
#ifndef PSRAM_HEAP
    return __malloc((struct sys_heap *)&sram_heap, size, RETURN_ADDR());    //从SRAM内存池分配
#else
    return __malloc((struct sys_heap *)&psram_heap, size, RETURN_ADDR());   //从PSRAM内存池分配
#endif
}
void mbedtls_mem_free(void *ptr)
{
#ifndef PSRAM_HEAP
    __free((struct sys_heap *)&sram_heap, ptr, RETURN_ADDR());
#else
    __free((struct sys_heap *)&psram_heap, ptr, RETURN_ADDR());
#endif
}

///////////////////////////////////////////////////////////////////////////////////
/**
 * @brief URL File (网络下载) 模块内存分配接口
 * 为网络下载协议的缓冲区提供内存。
 */
void *uf_alloc(uint32_t size)
{
#ifndef PSRAM_HEAP
    return __malloc((struct sys_heap *)&sram_heap, size, RETURN_ADDR());     //从SRAM内存池分配
#else
    return __malloc((struct sys_heap *)&psram_heap, size, RETURN_ADDR()); //从PSRAM内存池分配
#endif
}
void uf_free(void *ptr)
{
#ifndef PSRAM_HEAP
    __free((struct sys_heap *)&sram_heap, ptr, RETURN_ADDR());
#else
    __free((struct sys_heap *)&psram_heap, ptr, RETURN_ADDR());
#endif
}

///////////////////////////////////////////////////////////////////////////////////
/**
 * @brief LwIP 协议栈内存分配接口
 * 为 LwIP 网络协议栈提供内存分配。
 * @note 仅在 LwIP 配置启用 `MEM_LIBC_MALLOC` 时会被调用。
 */
void *lwip_mem_alloc(uint32_t size)
{
#ifndef PSRAM_HEAP
    return __malloc((struct sys_heap *)&sram_heap, size, RETURN_ADDR());    //从SRAM内存池分配
#else
    return __malloc((struct sys_heap *)&psram_heap, size, RETURN_ADDR());   //从PSRAM内存池分配
#endif
}
void lwip_mem_free(void *ptr)
{
#ifndef PSRAM_HEAP
    __free((struct sys_heap *)&sram_heap, ptr, RETURN_ADDR());
#else
    __free((struct sys_heap *)&psram_heap, ptr, RETURN_ADDR());
#endif
}

///////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 互问 (Huwen) 语音识别库 内存分配接口
 */
void *huwen_mem_alloc(uint32_t size)
{
    return __malloc((struct sys_heap *)&psram_heap, size, RETURN_ADDR());       //从SRAM内存池分配
    // return __malloc((struct sys_heap *)&psram_heap, size, RETURN_ADDR());    //从PSRAM内存池分配
}
void huwen_mem_free(void *ptr)
{
    __free((struct sys_heap *)&psram_heap, ptr, RETURN_ADDR());
    // __free((struct sys_heap *)&psram_heap, ptr, RETURN_ADDR());
}

///////////////////////////////////////////////////////////////////////////////////
/**
 * @brief LLM 库 内存分配接口
 */
void *llm_mem_alloc(uint32_t size)
{
#ifndef PSRAM_HEAP
    return __malloc((struct sys_heap *)&sram_heap, size, RETURN_ADDR());    //从SRAM内存池分配
#else
    return __malloc((struct sys_heap *)&psram_heap, size, RETURN_ADDR());   //从PSRAM内存池分配
#endif
}
void llm_mem_free(void *ptr)
{
#ifndef PSRAM_HEAP
    __free((struct sys_heap *)&sram_heap, ptr, RETURN_ADDR());
#else
    __free((struct sys_heap *)&psram_heap, ptr, RETURN_ADDR());
#endif
}

///////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 音频算法 库 内存分配接口
 */
void *audio_mem_alloc(uint32_t size, void *priv)
{
    return __malloc((struct sys_heap *)priv, size, RETURN_ADDR());
}
void audio_mem_free(void *ptr, void *priv)
{
    __free((struct sys_heap *)priv, ptr, RETURN_ADDR());
}

void *vfs_alloc(uint32_t size)
{
#ifndef PSRAM_HEAP
    return __malloc((struct sys_heap *)&sram_heap, size, RETURN_ADDR());    //从SRAM内存池分配
#else
    return __malloc((struct sys_heap *)&psram_heap, size, RETURN_ADDR());   //从PSRAM内存池分配
#endif
}

void vfs_free(void *ptr)
{
#ifndef PSRAM_HEAP
    __free((struct sys_heap *)&sram_heap, ptr, RETURN_ADDR());
#else
    __free((struct sys_heap *)&psram_heap, ptr, RETURN_ADDR());
#endif
}
/** @} */ // End of Memory_Allocation group
