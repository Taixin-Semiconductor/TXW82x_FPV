#include "sys_config.h"
#include "basic_include.h"
#include "osal/string.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/heap/av_heap.h"

// 通用用户空间内存初始化

void user_heap_init()
{
    _os_printf("\r\n");
    os_printf("--------------------------------------------------------------------\r\n");
#if defined(MPOOL_ALLOC) && defined(AV_PSRAM_HEAP) && defined(PSRAM_HEAP)
    {
        uint32   flags         = SYSHEAP_FLAGS_MEM_ALIGN_32 | SYSHEAP_FLAGS_MEM_LEAK_TRACE;
        void    *av_psram_buf  = os_malloc_psram(CONFIG_PSRAM_AVHEAP_SIZE);
        uint32_t av_psram_size = CONFIG_PSRAM_AVHEAP_SIZE;
        os_printf("| CPU0 AV_PSRAM_HEAP   : %p ~ %p, Size:%-8d     |\r\n", av_psram_buf, av_psram_buf+av_psram_size, av_psram_size);
        
        if (av_psram_buf)
        {
            av_psram_heap_init((void *) av_psram_buf, av_psram_size, flags);
        }
    }
#endif

#if defined(MPOOL_ALLOC) && defined(AV_HEAP)
    {
        uint32 flags = SYSHEAP_FLAGS_MEM_ALIGN_32;
        void *rev_mem_head = NULL;
        uint32_t sram_size = CONFIG_AVHEAP_SIZE;
        //计算SRAM_HEAP剩余空间
        uint32_t total_mem_remain_size = sysheap_freesize(&sram_heap);
        if(total_mem_remain_size > sram_size + 1024)
        {
            //申请空间,尽量将av_sram空间放到最后
            rev_mem_head = (void*)os_malloc(total_mem_remain_size - sram_size - 1024);
        }
        uint8_t *sram_buf = (uint8_t*)os_malloc(sram_size);
        
        os_printf("| CPU0 AV_HEAP         : %p ~ %p, Size:%-8d     |\r\n", sram_buf, sram_buf+sram_size, sram_size);
        if(sram_buf)
        {
            av_heap_init((void *) sram_buf, sram_size, flags);
        }

        if(rev_mem_head)
        {
            os_free(rev_mem_head);
        }
        
    }
#endif

    os_printf("--------------------------------------------------------------------\r\n");
}
