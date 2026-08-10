#ifndef __OS_STRING_H__
#define __OS_STRING_H__
#include "typesdef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ================= 内核日志级别定义 (Kernel Log Levels) ================= */
/* 这些宏用于在打印日志时嵌入级别标识，类似 Linux Kernel 的 printk 风格 */

#define KERN_SOH        "\001"      /**< ASCII Start Of Header (控制字符 SOH) */
#define KERN_TSOH       "\002"      /**< ASCII Start Of Tick Header (自定义控制字符，可能用于带时间戳的日志) */

#define KERN_EMERG      KERN_SOH"0" /**< 系统不可用 (Emergency) */
#define KERN_ALERT      KERN_SOH"1" /**< 必须立即采取行动 (Alert) */
#define KERN_CRIT       KERN_SOH"2" /**< 严重条件 (Critical) */
#define KERN_ERR        KERN_SOH"3" /**< 错误条件 (Error) */
#define KERN_WARNING    KERN_SOH"4" /**< 警告条件 (Warning) */
#define KERN_NOTICE     KERN_SOH"5" /**< 正常但重要的条件 (Notice) */
#define KERN_INFO       KERN_SOH"6" /**< 信息性消息 (Info) */
#define KERN_DEBUG      KERN_SOH"7" /**< 调试级消息 (Debug) */

/**
 * @brief 设置或获取当前打印级别
 * @param level 日志级别 (1-7)
 * @note 低于此级别的日志会被过滤掉不输出。
 *       默认级别0：表示输出所有的信息
 */
void print_level(int8 level);

/* ================= 硬件加速/底层内存操作 ================= */
/** 
 * @brief 利用 M2M DMA 加速内存拷贝
 */
void hw_memcpy(void *dest, const void *src, uint32 size);

/** 
 * @brief 利用 M2M DMA 加速加速内存填充
 */
void hw_memset(void *dest, uint8 val, uint32 n);

/** 
 * @brief 利用 M2M DMA 加速加速内存拷贝
 */
void hw_memcpy0(void *dest, const void *src, uint32 size);

/** 
 * @brief 非缓存内存拷贝
 * @note 用于操作未映射到 Cache 的内存区域 (如某些 DMA 缓冲区)，避免 Cache 一致性问题。
 */
void hw_memcpy_no_cache(void *dest, const void *src, uint32 size);

/* ============================================================================ 
 *  内部封装的字符串/内存操作，开启 MEM_TRACE时，这些API可以检测数据越界行为
 * ============================================================================ */
/** 
 * @brief 内部内存拷贝实现
 */
void *_os_memcpy(void *str1, const void *str2, int32 n);

/** 
 * @brief 内部字符串拷贝实现
 */
char *_os_strcpy(char *dest, const char *src);

/** 
 * @brief 内部内存填充实现
 */
void *_os_memset(void *str, int c, int32 n);

/** 
 * @brief 内部内存移动实现 (处理重叠区域)
 */
void *_os_memmove(void *str1, const void *str2, size_t n);

/** 
 * @brief 内部字符串拷贝 (限制长度)
 */
char *_os_strncpy(char *dest, const char *src, int32 n);

/** 
 * @brief 内部格式化字符串 (sprintf)
 */
int _os_sprintf(char *str, const char *format, ...);

/** 
 * @brief 内部格式化字符串 (vsnprintf)
 */
int _os_vsnprintf(char *s, size_t n, const char *format, va_list arg);

/** 
 * @brief 内部格式化字符串 (snprintf)
 */
int _os_snprintf(char *str, size_t size, const char *format, ...);

/**
 * @brief 字符串分割 (Tokenizer)
 * @param str 待分割的字符串 (会被修改)
 * @param separator 分隔符字符串
 * @param argv 输出数组，存储分割后的子串指针
 * @param argv_size 输出数组的最大容量
 * @return int32 分割出的参数个数
 * @note 类似 shell 的参数解析功能。
 */
int32 os_strtok(char *str, char *separator, char *argv[], int argv_size);

/**
 * @brief 忽略大小写查找字符 (限制长度)
 */
const char *os_strncasechr(const char *s, char c, int32 n);

/**
 * @brief 忽略大小写查找子串 (限制长度)
 */
const char *os_strncasestr(const char *str1, const char *str2, int32 n);

/**
 * @brief 复制字符串并分配新内存
 * @return char* 新分配的字符串指针，需用户释放
 */
char *os_strdup(const char *s);

/**
 * @brief 十六进制字符串转整数 (32位)
 * @param str 十六进制字符串 (如 "1A2B")
 * @return uint32 转换后的数值
 */
uint32 os_atoh(char *str);

/**
 * @brief 十六进制字符串转长整数 (64位)
 * @param str 十六进制字符串
 * @return uint64 转换后的数值
 */
uint64 os_atohl(char *str);

/* ================= 网络地址宏工具 (MAC & IP) ================= */

/**
 * @brief MAC 地址转字符串格式参数
 * @param a MAC 地址数组 (uint8[6])
 * @note 用法: printf(MACSTR, MAC2STR(mac));
 */
#ifndef MAC2STR
#define MAC2STR(a) (a)[0]&0xff, (a)[1]&0xff, (a)[2]&0xff, (a)[3]&0xff, (a)[4]&0xff, (a)[5]&0xff
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

/**
 * @brief 字符串转 MAC 地址宏
 * @param s 字符串指针
 * @param a MAC 地址数组指针
 */
#ifndef STR2MAC
#define STR2MAC(s, a) str2mac((char *)(s), (uint8 *)(a))
#endif

/**
 * @brief IP 地址转字符串格式参数 (主机字节序 Host Byte Order)
 * @param ip 32位 IP 地址 (如 0xC0A80101 代表 192.168.1.1)
 * @note 假设高位在前 (Big Endian 逻辑展示)，具体取决于 host 序定义。
 */
#ifndef IP2STR_H
#define IP2STR_H(ip) ((ip)>>24)&0xff,((ip)>>16)&0xff,((ip)>>8)&0xff,(ip&0xff)
/**
 * @brief IP 地址转字符串格式参数 (网络字节序 Network Byte Order)
 * @param ip 32位 IP 地址 (网络序)
 */
#define IP2STR_N(ip) (ip&0xff),((ip)>>8)&0xff,((ip)>>16)&0xff,((ip)>>24)&0xff
#define IPSTR "%-03d.%-03d.%-03d.%-03d"
#endif

/**
 * @brief 判断两个 MAC 地址是否相等
 */
#define MAC_EQU(a1,a2)    (os_memcmp((a1), (a2), 6)==0)

/**
 * @brief 判断是否为广播地址 (FF:FF:FF:FF:FF:FF)
 */
#define IS_BCAST_ADDR(a)  (((a)[0]&(a)[1]&(a)[2]&(a)[3]&(a)[4]&(a)[5]) == 0xff)

/**
 * @brief 判断是否为组播地址 (首字节最低位为 1)
 */
#define IS_MCAST_ADDR(a)  ((a)[0]&0x01)

/**
 * @brief 判断是否为零地址 (00:00:00:00:00:00)
 */
#define IS_ZERO_ADDR(a)   (!((a)[0] | (a)[1] | (a)[2] | (a)[3] | (a)[4] | (a)[5]))

#define SSID_MAX_LEN      (32)  /**< SSID 最大长度 */
#define PASSWD_MAX_LEN    (63)  /**< 密码最大长度 */

/**
 * @brief 环形缓冲区回绕位置计算
 * @param p 当前指针位置
 * @param s 缓冲区大小
 * @param i 偏移量
 * @return 计算后的新位置 (处理回绕)
 */
#define NEXT_RPOS(p,s,i)  (((p)+(i)) >= (s) ? ((p)+(i)-(s)) : ((p)+(i)))

/* ================= 打印系统接口 ================= */

/**
 * @brief 带时间戳/标签的打印宏
 * @note 自动添加 KERN_TSOH 前缀，可能触发带时间戳的输出。
 */
#define os_printf(fmt, ...)  hgprintf(KERN_TSOH fmt, ##__VA_ARGS__)

/**
 * @brief 普通打印宏
 * @note 直接调用底层打印，无额外前缀。
 */
#define _os_printf(fmt, ...) hgprintf(fmt, ##__VA_ARGS__)

/**
 * @brief 禁用/启用打印输出
 * @param dis 1: 禁用, 0: 启用
 */
void disable_print(int8 dis);

/**
 * @brief 禁用/启用打印颜色
 * @param dis 1: 禁用颜色码, 0: 启用
 */
void disable_print_color(int8_t dis);

/**
 * @brief 禁用/启用打印时间戳
 * @param dis 1: 禁用时间, 0: 启用
 */
void disable_print_time(int8_t dis);

/**
 * @brief 启用 NTP 时间同步打印
 * @param en 1: 启用, 0: 禁用
 * @note 若启用了 NTP，打印的时间戳将使用网络同步时间而非启动计时。
 */
void print_with_ntp(uint8 en);

/**
 * @brief 底层全局打印函数 (Variadic)
 * @param fmt 格式字符串
 */
void hgprintf(const char *fmt, ...);

/**
 * @brief 底层全局打印函数 (va_list)
 */
void hgvprintf(const char *fmt, va_list ap);

/**
 * @brief 打印输出重定向到缓冲区
 * @param str 输出缓冲区
 * @param len 缓冲区长度
 * @param level 日志级别
 */
void hgprintf_out(char *str, int32 len, uint8 level);

/**
 * @brief 打印钩子函数类型
 * @param priv 用户私有数据
 * @param msg 消息内容
 * @param len 消息长度
 */
typedef void (*osprint_hook)(void *priv, char *msg, int len);

/**
 * @brief 重定向打印输出
 * @param print 钩子函数
 * @param priv 传递给钩子的私有数据
 * @param dual_out 1: 同时输出到原控制台和新钩子, 0: 仅输出到新钩子
 */
void print_redirect(osprint_hook print, void *priv, uint8 dual_out);

/* ================= 数据转换与调试转储 ================= */

/**
 * @brief 十六进制字符转整数 (0-9, A-F -> 0-15)
 */
int32 hexchr2int(char c);

/**
 * @brief 两个十六进制字符转一个字节
 * @param hex 指向两个字符的指针
 * @return int32 转换后的字节值
 */
int32 hex2char(char *hex);

/**
 * @brief 十六进制字符串转二进制缓冲区
 * @param hex 十六进制字符串
 * @param buf 输出缓冲区
 * @param len 缓冲区最大长度
 * @return int32 转换的字节数
 */
int32 hex2bin(char *hex, uint8 *buf, uint32 len);

/**
 * @brief 字符串转 MAC 地址
 */
void str2mac(char *macstr, uint8 *mac);

/**
 * @brief 字符串转 IP 地址 (返回 32 位整数)
 */
uint32 str2ip(char *ipstr);

/**
 * @brief 打印十六进制数据块
 * @param str 标题字符串
 * @param data 数据指针
 * @param len 数据长度
 * @param newline 是否每行换号 (1: 是, 0: 否)
 */
void dump_hex(char *str, uint8 *data, uint32 len, int32 newline);

/**
 * @brief 打印密钥数据
 * @param str 标题
 * @param key 密钥数据
 * @param len 长度
 * @param sp 是否添加空格
 */
void dump_key(char *str, uint8 *key, uint32 len, uint32 sp);

/**
 * @brief 打印内存区域
 * @param title 标题
 * @param addr 地址 (uint32*)
 * @param len 长度 (字数)
 */
void dump_memory(char *title, uint32 *addr, uint32 len);

/**
 * @brief 密钥转字符串
 */
void key_str(uint8 *key, uint32 key_len, char *str_buf);

/**
 * @brief 复制内存块并分配新空间
 * @return void* 新分配的内存指针
 */
void *os_memdup(const void *ptr, uint32 len);

/**
 * @brief 生成随机字节
 * @param data 输出缓冲区
 * @param len 长度
 * @return int32 成功生成的字节数
 */
int32 os_random_bytes(uint8 *data, int32 len);

/* ================= 标准库函数外部声明 ================= */
extern int snprintf(char *str, size_t size, const char *format, ...);
extern int sprintf(char *string, const char *format, ...);
extern int vsprintf(char *str, const char *format, va_list arg_ptr);
extern int vsnprintf(char *str, size_t length, const char *format, va_list arg_ptr);
extern int printf(const char *format, ...);

/* ================= 错误日志 (Error Log)，调试功能，将系统异常信息保存到flash ================= */
#ifdef ERRLOG_ENABLE
/**
 * @brief 保存错误日志到flash
 */
void sys_errlog_save(char *log, int32 len, uint8 level);
/**
 * @brief 刷新错误日志 (写入 Flash 或输出)
 */
void sys_errlog_flush(uint32 p1, uint32 p2, uint32 p3);
/**
 * @brief 初始化错误日志系统
 * @param level 记录级别
 * @param buf_size 缓冲区大小
 */
void sys_errlog_init(int8 level, uint16 buf_size);
/**
 * @brief 输出所有保存的错误日志信息
 */
void sys_errlog_dump(void);
#else
/* 若未启用，定义为空宏以消除编译开销 */
#define sys_errlog_save(log, len, level)
#define sys_errlog_flush(p1, p2, p3)
#define sys_errlog_init(level, buf_size)
#define sys_errlog_dump()
#endif

/* ================= 内存调试记录 (Memory Trace) ================= */
/**
 * @brief 记录内存释放地址和返回地址 (用于检测 Double Free)
 */
void mem_free_rec(void *addr, void *lr);
/**
 * @brief 记录内存分配地址和返回地址 (用于检测 Leak)
 */
void mem_alloc_rec(void *addr, void *lr);
/**
 * @brief 转储所有未释放的内存记录 (内存泄漏检测)
 */
void mem_reclist_dump(void);

/* ================= 内存分配接口 (带/不带追踪) ================= */

/* --- 基础分配函数 (内部实现) --- */
void *_os_malloc(int size);
void _os_free(void *ptr);
void *_os_zalloc(int size);       /**< 分配并清零 */
void *_os_realloc(void *ptr, int size);
void *_os_calloc(size_t nmemb, size_t size);

/* --- 带调试信息的分配函数 (记录文件名/行号) --- */
void *_os_malloc_t(int size, const char *func, int line);
void _os_free_t(void *ptr, const char *func, int line);
void *_os_zalloc_t(int size, const char *func, int line);
void *_os_realloc_t(void *ptr, int size, const char *func, int line);
void *_os_calloc_t(int nmemb, int size, const char *func, int line);

/* --- PSRAM (外部大容量内存) 分配函数 --- */
void *_os_malloc_psram(int size);
void _os_free_psram(void *ptr);
void *_os_zalloc_psram(int size);
void *_os_realloc_psram(void *ptr, int size);
void *_os_calloc_psram(int nmemb, int size);

/* --- PSRAM 带调试信息的分配函数 --- */
void *_os_malloc_psram_t(int size, const char *func, int line);
void _os_free_psram_t(void *ptr, const char *func, int line);
void *_os_zalloc_psram_t(int size, const char *func, int line);
void *_os_realloc_psram_t(void *ptr, int size, const char *func, int line);
void *_os_calloc_psram_t(int nmemb, int size, const char *func, int line);

/* ================= 宏映射与内存追踪开关 ================= */

/* 标准数学/字符串函数映射 */
#define os_abs                   abs
#define os_atoi(c)               atoi((const char *)c)
#define os_atol(c)               atol((const char *)c)
#define os_atoll(c)              atoll((const char *)c)
#define os_atof(c)               atof((const char *)c)
#define os_strcmp(s1,s2)         strcmp((const char *)(s1), (const char *)(s2))
#define os_strstr(s1,s2)         strstr((const char *)(s1), (const char *)(s2))
#define os_strchr(s,c)           strchr((const char *)(s), c)
#define os_strlen(s)             strlen((const char *)(s))
#define os_memcmp(s1,s2,n)       memcmp((const void *)(s1), (const void *)(s2), n)
#define os_strncmp(s1,s2,n)      strncmp((const char *)s1, (const char *)s2, n)
#define os_strrchr(s,c)          strrchr((const char *)(s), c)
#define os_strncasecmp(s1,s2,n)  strncasecmp((const char *)(s1), (const char *)(s2), n)
#define os_strcasecmp(s1,s2)     strcasecmp((const char *)(s1), (const char *)(s2))
#define os_srand(v)              srand(v)
#define os_rand()                rand()
#define os_sscanf                sscanf

/**
 * @brief 内存追踪模式 (MEM_TRACE)
 * 
 * 若定义了 MEM_TRACE:
 * - os_malloc 等宏会自动展开为带 __FUNCTION__ 和 __LINE__ 的版本 (_os_malloc_t)。
 * - os_free 宏会在释放后将指针置为 NULL，防止悬空指针。
 * - 字符串/内存操作函数也会映射到内部 _os_ 版本 (可能包含边界检查)。
 * 
 * 若未定义:
 * - 直接映射到标准库函数或基础 _os_ 函数，无调试开销。
 */
#define os_malloc(s)              _os_malloc(s)
#define os_free(p)                do{ _os_free((void *)p); (p)=NULL;}while(0)
#define os_zalloc(s)              _os_zalloc(s)
#define os_realloc(p,s)           _os_realloc(p,s)
#define os_calloc(p,s)            _os_calloc(p, s)
#define os_malloc_psram(s)        _os_malloc_psram(s)
#define os_free_psram(p)          do{ _os_free_psram((void *)p); (p)=NULL;}while(0)
#define os_zalloc_psram(s)        _os_zalloc_psram(s)
#define os_realloc_psram(p,s)     _os_realloc_psram(p,s)
#define os_calloc_psram(p,s)      _os_calloc_psram(p, s)

#define os_strcpy(d,s)            strcpy((char *)(d), (const char *)(s))
#define os_strncpy(d,s,n)         strncpy((char *)(d), (const char *)(s), n)
#define os_memset(s,c,n)          memset((void *)(s), c, n)
#define os_memcpy(d,s,n)          memcpy((void *)(d), (const void *)(s), n)
#define os_memmove(d,s,n)         memmove((void *)(d), (const void *)(s), n)
#define os_sprintf                sprintf
#define os_vsnprintf              vsnprintf
#define os_snprintf               snprintf

/* ================= 断言系统 (Assert) ================= */

extern uint8 assert_holdup; /**< 标志位：断言失败后是否死循环挂起 (1: 挂起, 0: 继续/重启) */

/**
 * @brief 内部断言处理函数
 * @param __function 函数名
 * @param __line 行号
 * @param __assertion 断言表达式字符串
 * @param lr 返回地址 (用于堆栈回溯)
 */
extern void assert_internal(const char *__function, unsigned int __line, const char *__assertion, void *lr);

/**
 * @brief 断言宏
 * @param f 条件表达式
 * @note 若 f 为假 (0)，则调用 assert_internal 记录错误信息。
 *       行为取决于 assert_holdup 配置 (死机或记录后继续)。
 */
#define ASSERT(f) if(!(f)) {\
                    assert_internal(__ASSERT_FUNC, __LINE__, #f, RETURN_ADDR()); \
                  }

#ifdef __cplusplus
}
#endif
#endif /* __OS_STRING_H__ */
