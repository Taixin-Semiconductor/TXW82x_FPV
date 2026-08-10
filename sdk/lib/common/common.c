#include "sys_config.h"
#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "version.h"
#include "osal/task.h"
#include "osal/sleep.h"
#include "osal/string.h"
#include "osal/irq.h"
#include "hal/dma.h"
#include "hal/crc.h"
#include "lib/common/common.h"
#include "lib/heap/sysheap.h"



const uint32 sdk_version   = SDK_VERSION;
const uint32 svn_version   = SVN_VERSION;
const uint32 app_version   = APP_VERSION;

__bobj uint64 cpu_loading_tick;
uint32 m2mdam_time = 0;
__bobj struct dma_device *m2mdma;

extern void cpu_loading_api_time(char *api, uint32 time, uint32 diff_tick);

#define CPU_TIME_API(api) extern uint32 api##_time(void); \
                          _time_ = api##_time(); \
                          cpu_loading_api_time(#api, _time_, diff_tick);

void cpu_loading_api_time(char *api, uint32 time, uint32 diff_tick)
{
    if (time > 0) {
        uint32 count = 100 * os_msecs_to_jiffies(time / 1000);
        os_printf(KERN_ALERT"[%s time: %dms, %d%%]\r\n", api, (time / 1000), (count / diff_tick));
    }
}

void module_version_show(void)
{
    extern uint32 __modver_start;
    extern uint32 __modver_end;
    uint32 *start = (uint32 *)&__modver_start;
    uint32 *end   = (uint32 *)&__modver_end;
    while (start < end) {
        _os_printf("**   lib%s\r\n", (char *)*start++);
    }
    _os_printf("------------------------------------------------------------------\r\n");
}

typedef void (*__ctor_func_)(void);
extern __ctor_func_ __CTOR_LIST__[];
extern __ctor_func_ __DTOR_LIST__[];
void do_global_ctors(void)
{
    ulong i;
    ulong nptrs = (ulong)__CTOR_LIST__[0];
    if (nptrs == (ulong) - 1) {
        for (nptrs = 0; __CTOR_LIST__[nptrs + 1] != 0; nptrs++) ;
    }
    for (i = nptrs; i >= 1; i--) {
        __CTOR_LIST__[i]();
    }
}
void do_global_dtors(void)
{
    ulong i;
    ulong nptrs = (ulong)__DTOR_LIST__[0];

    if (nptrs == (ulong) - 1) {
        for (nptrs = 0; __DTOR_LIST__[nptrs + 1] != 0; nptrs++) ;
    }
    for (i = 1; i <= nptrs; i++) {
        __DTOR_LIST__[i]();
    }
}

void cpu_loading_print(uint8 all, struct os_task_info *tsk_info, uint32 size)
{
    uint32 i = 0;
    uint32 diff_tick = 0;
    uint32 _time_ = 0;
    uint32 count;
    uint64 jiff = os_jiffies();
    uint32 total_time = 0;

    if(tsk_info == NULL) return;
    diff_tick = DIFF_JIFFIES(cpu_loading_tick, jiff);
    cpu_loading_tick = jiff;

    os_printf(KERN_NOTICE"-----------------------------------------------------------------------------\r\n");
    os_printf(KERN_NOTICE"Task Runtime Statistic, interval:%dms\r\n", (uint32)os_jiffies_to_msecs(diff_tick));
    os_printf(KERN_NOTICE"PID  Name                          %%CPU    Stack  Prio            Status\r\n");
    os_printf(KERN_NOTICE"-----------------------------------------------------------------------------\r\n");

    cpu_loading_api_time("sram_heap", sysheap_time(&sram_heap), diff_tick);
#ifdef PSRAM_HEAP
    cpu_loading_api_time("sram_heap", sysheap_time(&psram_heap), diff_tick);
#endif

    CPU_TIME_API(sysirq);
#ifdef SKB_POOL_ENABLE
    CPU_TIME_API(skbpool);
#endif
    //CPU_TIME_API(hw_memcpy);

    os_printf(KERN_NOTICE"-----------------------------------------------------------------------------\r\n");

    count = os_task_runtime(tsk_info, size, diff_tick);
    for (i = 0; i < count; i++) {
        if (tsk_info[i].time > 0 || all) {
            total_time += tsk_info[i].time;
            if(tsk_info[i].time < 10){
                os_printf(KERN_NOTICE"%2d   %-28s  0.%2d%%   %4d   %2d (%08x)    %s\r\n",
                          tsk_info[i].id,
                          tsk_info[i].name ? tsk_info[i].name : "----",
                          tsk_info[i].time,
                          tsk_info[i].stack * 4,
                          tsk_info[i].prio,
                          tsk_info[i].arg,
                          tsk_info[i].status);
            }else{
                os_printf(KERN_NOTICE"%2d   %-28s  %2d%%    %4d   %2d (%08x)    %s\r\n",
                          tsk_info[i].id,
                          tsk_info[i].name ? tsk_info[i].name : "----",
                          tsk_info[i].time / 10,
                          tsk_info[i].stack * 4,
                          tsk_info[i].prio,
                          tsk_info[i].arg,
                          tsk_info[i].status);
            }
        }
    }
    os_printf(KERN_NOTICE"-------------------------- CPU Loading: %-02d%%  --------------------------------\r\n", total_time/10);
}

int strncasecmp(const char *s1, const char *s2, size_t n)
{
    size_t i = 0;

    for (i = 0; i < n && s1[i] && s2[i]; i++) {
        if (s1[i] == s2[i] || s1[i] + 32 == s2[i] || s1[i] - 32 == s2[i]) {
        } else {
            break;
        }
    }
    return (i != n);
}

int strcasecmp(const char *s1, const char *s2)
{
    while (*s1 || *s2) {
        if (*s1 == *s2 || *s1 + 32 == *s2 || *s1 - 32 == *s2) {
            s1++; s2++;
        } else {
            return -1;
        }
    }
    return 0;
}

void hw_memcpy(void *dest, const void *src, uint32 size)
{
    if (dest && src) {
        if (m2mdma && size > 45) {
#ifdef MEM_TRACE
#ifdef PSRAM_HEAP
            void *heap = (IS_PSRAM_ADDR(dest)) ? ((void *)&psram_heap) : ((void *)&sram_heap);
#else
            void *heap = (void *)&sram_heap;
#endif
            int32 ret = sysheap_of_check(heap, dest, size);
            if (ret == 0) {
                os_printf(KERN_WARNING"check addr fail: %x, size:%d \r\n", dest, size);
            }
#endif
            uint64 __t__ = os_useconds();
            dma_memcpy(m2mdma, dest, src, size);
            m2mdam_time += os_useconds() - __t__;
        } else {
            os_memcpy(dest, src, size);
        }
    }
}

void hw_memcpy0(void *dest, const void *src, uint32 size)
{
    if (m2mdma && size > 45) {
#ifdef MEM_TRACE
#ifdef PSRAM_HEAP
        void *heap = (IS_PSRAM_ADDR(dest)) ? ((void *)&psram_heap) : ((void *)&sram_heap);
#else
        void *heap = (void *)&sram_heap;
#endif
        int32 ret = sysheap_of_check(heap, dest, size);
        if (ret == 0) {
            os_printf(KERN_WARNING"check addr fail: %x, size:%d \r\n", dest, size);
        }
#endif
        uint64 __t__ = os_useconds();
        dma_memcpy(m2mdma, dest, src, size);
        m2mdam_time += os_useconds() - __t__;
    } else {
        os_memcpy(dest, src, size);
    }
}

void hw_memcpy_no_cache(void *dest, const void *src, uint32 size)
{
    if (dest && src) {

#ifdef MEM_TRACE
#ifdef PSRAM_HEAP
            void *heap = (IS_PSRAM_ADDR(dest)) ? ((void *)&psram_heap) : ((void *)&sram_heap);
#else
            void *heap = (void *)&sram_heap;
#endif
            int32 ret = sysheap_of_check(heap, dest, size);
            if (ret == 0) {
                os_printf(KERN_WARNING"check addr fail: %x, size:%d \r\n", dest, size);
            }
#endif
        uint64 __t__ = os_useconds();
        dma_memcpy_no_cache(m2mdma, dest, src, size);
        m2mdam_time += os_useconds() - __t__;
    }
}

void hw_memset(void *dest, uint8 val, uint32 n)
{
    if (dest) {
        if (m2mdma && n > 12) {
#ifdef MEM_TRACE
#ifdef PSRAM_HEAP
            void *heap = (IS_PSRAM_ADDR(dest)) ? ((void *)&psram_heap) : ((void *)&sram_heap);
#else
            void *heap = (void *)&sram_heap;
#endif
            int32 ret = sysheap_of_check(heap, dest, n);
            if (ret == 0) {
                os_printf(KERN_WARNING"check addr fail: %x, size:%d \r\n", dest, n);
            }
#endif
            uint64 __t__ = os_useconds();
            dma_memset(m2mdma, dest, val, n);
            m2mdam_time += os_useconds() - __t__;
        } else {
            os_memset(dest, val, n);
        }
    }
}

uint32 hw_memcpy_time(void)
{
    uint32 v = m2mdam_time;
    m2mdam_time = 0;
    return v / 1000;
}

void *os_memdup(const void *ptr, uint32 len)
{
    void *p;
    if (!ptr || len == 0) {
        return NULL;
    }
    p = os_malloc(len);
    if (p) {
        hw_memcpy(p, ptr, len);
    }
    return p;
}


int32 os_random_bytes(uint8 *data, int32 len)
{
    int32 i = 0;
    int32 seed, seed_uuid, rand_val = 0;
    uint8 uuid[6];

    sysctrl_get_chip_uuid((uint8 *)&uuid[0], 6);
    memcpy((void *)&seed_uuid, &uuid[2], 4);
    
#ifdef TXW4002ACK803
    seed = CPU_CYCLE_VALUE() ^ (CPU_CYCLE_VALUE() << 8) ^ (CPU_CYCLE_VALUE() >> 8);
#else
    seed = CPU_CYCLE_VALUE() ^ sysctrl_get_trng() ^ (seed_uuid);
#endif
    for (i = 0; i < len; i++) {
        if (i & 1) {
            rand_val = rand_val >> 8;
        } else {
            seed = (seed * 214013L + 2531011L) >> 16;
            rand_val = seed;
        }
        data[i] = (uint8)(rand_val & 0xFF);
    }
    return 0;
}

static int32 hw_crc_calc_do(enum CRC_DEV_TYPE type, const uint8 *data, uint32 len,
                            uint32 *crc, uint8 req_flags)
{
    struct crc_dev_req req;
    struct crc_dev *crcdev;
    uint32 remain = len;
    uint32 flags = 0;
    int32 ret;

    if (!crc || type < 0 || type >= CRC_TYPE_MAX || (len && !data) ||
        ((uint32)data + len < (uint32)data)) {
        return -EINVAL;
    }
    crcdev = (struct crc_dev *)dev_get(HG_CRC_DEVID);
    if (!crcdev) {
        return -ENODEV;
    }

    os_memset(&req, 0, sizeof(req));
    req.flag = req_flags;
    req.type = type;
    req.data = (uint8 *)data;

    do {
        req.len = (remain > CRC_DEV_MAX_XFER_SIZE) ? CRC_DEV_MAX_XFER_SIZE : remain;
        req.crc_last = (flags & CRC_DEV_FLAGS_CONTINUE_CALC) ? *crc : 0;
        ret = crc_dev_calc(crcdev, &req, crc, flags);
        if (ret != RET_OK) {
            return ret;
        }
        if (req.len) {
            req.data += req.len;
        }
        remain -= req.len;
        flags = CRC_DEV_FLAGS_CONTINUE_CALC;
    } while (remain);
    return RET_OK;
}

static uint32 hw_crc_reflected_sw(const uint8 *data, uint32 len, uint32 init,
                                  uint32 poly, uint32 xor_out, uint8 width)
{
    uint32 crc = init;
    uint32 mask = (width == 32) ? 0xFFFFFFFFUL : ((1UL << width) - 1);

    while (len--) {
        crc ^= *data++;
        for (uint8 i = 0; i < 8; i++) {
            crc = (crc & 1) ? ((crc >> 1) ^ poly) : (crc >> 1);
        }
    }
    return (crc ^ xor_out) & mask;
}

static int32 hw_crc_fallback_sw(enum CRC_DEV_TYPE type, const uint8 *data,
                                uint32 len, uint32 *crc)
{
    uint32 value = 0;

    if (!crc || (len && !data)) {
        return -EINVAL;
    }
    switch (type) {
        case CRC_TYPE_CRC5_USB:
            value = hw_crc_reflected_sw(data, len, 0x1F, 0x14, 0x1F, 5);
            break;
        case CRC_TYPE_CRC7_MMC:
            while (len--) {
                value ^= *data++;
                for (uint8 i = 0; i < 8; i++) {
                    value = (value & 0x80) ? ((value << 1) ^ 0x12) : (value << 1);
                    value &= 0xFF;
                }
            }
            value >>= 1;
            break;
        case CRC_TYPE_CRC8_MAXIM:
            value = hw_crc_reflected_sw(data, len, 0, 0x8C, 0, 8);
            break;
        case CRC_TYPE_CRC8:
            while (len--) {
                value ^= *data++;
                for (uint8 i = 0; i < 8; i++) {
                    value = (value & 0x80) ? ((value << 1) ^ 0x07) : (value << 1);
                    value &= 0xFF;
                }
            }
            break;
        case CRC_TYPE_CRC16:
            value = hw_crc_reflected_sw(data, len, 0, 0xA001, 0, 16);
            break;
        case CRC_TYPE_CRC16_CCITT:
            value = hw_crc_reflected_sw(data, len, 0, 0x8408, 0, 16);
            break;
        case CRC_TYPE_CRC16_MODBUS:
            value = hw_crc_reflected_sw(data, len, 0xFFFF, 0xA001, 0, 16);
            break;
        case CRC_TYPE_CRC32_WINRAR:
            value = hw_crc_reflected_sw(data, len, 0xFFFFFFFF, 0xEDB88320,
                                        0xFFFFFFFF, 32);
            break;
        default:
            return -ENOTSUP;
    }
    *crc = value;
    return RET_OK;
}

int32 hw_crc_s(enum CRC_DEV_TYPE type, const uint8 *data, uint32 len, uint32 *crc)
{
    return hw_crc_calc_do(type, data, len, crc, 0);
}

int32 hw_crc_calc_no_cache(enum CRC_DEV_TYPE type, const uint8 *data, uint32 len,
                           uint32 *crc)
{
    return hw_crc_calc_do(type, data, len, crc, CRC_REQ_FLAGS_SKIP_CACHE_SYNC);
}

uint32 hw_crc(enum CRC_DEV_TYPE type, uint8 *data, uint32 len)
{
    uint32 crc = 0;
    int32 ret = (len < 64) ? RET_ERR : hw_crc_s(type, data, len, &crc);

    if ((ret != RET_OK)) {
         hw_crc_fallback_sw(type, data, len, &crc);
    }
    return crc;
}

uint32 hw_crc_no_cache(enum CRC_DEV_TYPE type, uint8 *data, uint32 len)
{
    uint32 crc = 0;
    int32 ret = hw_crc_calc_no_cache(type, data, len, &crc);

    if ((ret != RET_OK)) {
        hw_crc_fallback_sw(type, data, len, &crc);
    }
    return crc;
}

int ffs(int x)
{
    int r = 1;

    if (!x) {
        return 0;
    }

    if (!(x & 0xffff)) {
        x >>= 16;
        r += 16;
    }
    if (!(x & 0xff)) {
        x >>= 8;
        r += 8;
    }
    if (!(x & 0xf)) {
        x >>= 4;
        r += 4;
    }
    if (!(x & 3)) {
        x >>= 2;
        r += 2;
    }
    if (!(x & 1)) {
        x >>= 1;
        r += 1;
    }
    return r;
}

int fls(int x)
{
    int r = 32;

    if (!x) {
        return 0;
    }

    if (!(x & 0xffff0000u)) {
        x <<= 16;
        r -= 16;
    }
    if (!(x & 0xff000000u)) {
        x <<= 8;
        r -= 8;
    }
    if (!(x & 0xf0000000u)) {
        x <<= 4;
        r -= 4;
    }
    if (!(x & 0xc0000000u)) {
        x <<= 2;
        r -= 2;
    }
    if (!(x & 0x80000000u)) {
        x <<= 1;
        r -= 1;
    }
    return r;
}

uint32 scatter_size(scatter_data *data, uint32 count)
{
    uint32 size = 0;
    uint32 i = 0;
    for (i = 0; i < count; i++) {
        size += data[i].size;
    }
    return size;
}

uint8 *scatter_offset(scatter_data *data, uint32 count, uint32 off)
{
    uint8 i;
    for (i = 0; i < count; i++) {
        if (off < data[i].size) {
            return data[i].addr + off;
        }
        off -= data[i].size;
    }
    return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
//系统崩溃产生异常时会执行 trap_data_dump 和 trap_hdl_run
// trap_data_dump: 崩溃时dump指定的数据，可以通过 trap_data_set 添加多个观察数据
// trap_hdl_run  : 崩溃时执行指定的函数，通过 trap_hdl_set API设置系统崩溃时需要执行的函数。注意：添加的函数不能再次崩溃
/////////////////////////////////////////////////////////////////////////////////////////
enum TRAP_DATA {
    //TRAP_DATA_ID_1,
    TRAP_DATA_MAX,
};
enum TRAP_HDL {
    //TRAP_HDL_ID_1,
    TRAP_HDL_MAX,
};
struct {
    void  *addr;
    uint32 len;
} trap_c_data[TRAP_DATA_MAX];
struct {
    void (*hdl)(void *arg);
    void *arg;
} trap_c_hdl[TRAP_HDL_MAX];
void trap_data_set(int8 id, void *addr, uint32 len)
{
    if (id < TRAP_DATA_MAX) {
        trap_c_data[id].addr = addr;
        trap_c_data[id].len  = len;
    } else {
        os_printf(KERN_ERR"trap_data_set: invalid id %d, max %d\r\n", id, TRAP_DATA_MAX);
    }
}
void trap_hdl_set(int8 id, void (*hdl)(void *), void *arg)
{
    if (id < TRAP_HDL_MAX) {
        trap_c_hdl[id].hdl = hdl;
        trap_c_hdl[id].arg = arg;
    } else {
        os_printf(KERN_ERR"trap_hdl_set: invalid id %d, max %d\r\n", id, TRAP_HDL_MAX);
    }
}
void trap_data_dump(void)
{
    int8 i;
    char name[32];
    for (i = 0; i < TRAP_DATA_MAX; i++) {
        if (trap_c_data[i].addr && trap_c_data[i].len) {
            os_printf(KERN_ERR"---------------------------------------------------------------\r\n");
            os_snprintf(name, 31, "dump data %d:\r\n", i);
            dump_hex(name, trap_c_data[i].addr, trap_c_data[i].len, 1);
        }
    }
}
void trap_hdl_run(void)
{
    int8 i;
    for (i = 0; i < TRAP_HDL_MAX; i++) {
        if (trap_c_hdl[i].hdl) {
            os_printf(KERN_ERR"---------------------------------------------------------------\r\n");
            os_printf(KERN_ERR"trap hdl: %p, arg:%p\r\n", trap_c_hdl[i].hdl, trap_c_hdl[i].arg);
            trap_c_hdl[i].hdl(trap_c_hdl[i].arg);
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////////

extern int *__errno_location(void);

void set_errno(int32 err)
{
    *__errno_location() = err;
}

int32 get_errno(void)
{
    return *__errno_location();
}

