#include "basic_include.h"
#include "lib/heap/sysheap.h"

#ifndef PRINT_LEVEL_DEFAULT
#define PRINT_LEVEL_DEFAULT (6)
#endif

#ifndef PRINT_BUFF_SIZE
#define PRINT_BUFF_SIZE (256)
#endif

#define LOWCASE(c)   (((c)>='A'&&(c) <= 'Z')?(c)+32:(c))

#define _PRINT_DISABLED_        (BIT(0))
#define _PRINT_COLOR_DISABLED_  (BIT(1))
#define _PRINT_TIME_DISABLED_   (BIT(2))
#define _PRINT_TIME_NTP_        (BIT(3))
#define _PRINT_REDIRECT_DUAL_   (BIT(4))

__bobj static osprint_hook __print_hook;
__bobj static void *__print_hook_priv;
__bobj void *console_handle;
__bobj int8 __print_level__;
__bobj uint8 __print_ctrl__;
__bobj char _print_buff_p[PRINT_BUFF_SIZE];

static const char *__print_color__[] = {
    "\e[40;35;7m",   //KERN_EMERG  : Red
    "\e[40;35;7m",   //KERN_ALERT  : Red
    "\e[40;35;7m",   //KERN_CRIT   : Red
    "\e[40;31m",     //KERN_ERR    : Red
    "\e[40;33m",     //KERN_WARNING: Yellow
    "\e[40;32m",     //KERN_NOTICE : Green
    "\e[40;34m",     //KERN_INFO   : Blue
    "\e[40;30m",     //KERN_DEBUG  : Black/Default
    "\e[0m",         //END
};

int printf(const char *format, ...)  __alias(hgprintf);
int puts(const char *s) __alias(hgputs);
int __wrap_printf(const char *format, ...)  __alias(hgprintf);
int __cskyvprintfprintf(const char *format, ...)  __alias(hgprintf);

////////////////////////////////////////////////////////////////////////
int32 hexchr2int(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

int32 hex2char(char *hex_str)
{
    int a, b;
    a = hexchr2int(*hex_str++);
    if (a < 0) {
        return -1;
    }
    b = hexchr2int(*hex_str++);
    if (b < 0) {
        return -1;
    }
    return (a << 4) | b;
}

int32 hex2bin(char *hex_str, uint8 *bin, uint32 len)
{
    uint32 i = 0;
    uint32 str_len = 0;
    int32  a;
    char *ipos = hex_str;
    uint8 *opos = bin;

    if (!hex_str || !bin) {
        return i;
    }

    str_len = os_strlen(hex_str);
    for (i = 0; i < len && (i * 2) + 2 <= str_len; i++) {
        a = hex2char(ipos);
        if (a < 0) {
            return i;
        }
        *opos++ = (uint8)a;
        ipos += 2;
    }
    return i;
}

void str2mac(char *macstr, uint8 *mac)
{
    mac[0] = hex2char(macstr);
    mac[1] = hex2char(macstr + 3);
    mac[2] = hex2char(macstr + 6);
    mac[3] = hex2char(macstr + 9);
    mac[4] = hex2char(macstr + 12);
    mac[5] = hex2char(macstr + 15);
}

uint32 str2ip(char *ipstr)
{
    char *ptr = ipstr;
    uint32 ip = os_atoi(ipstr);
    do {
        if (*ptr == '.') {
            ptr++;
            ip = (ip << 8 | os_atoi(ptr));
        }
        ptr++;
    } while (*ptr);
    return os_htonl(ip);
}

uint64 _os_atoh(char *str, int8 count)
{
    uint64 val = 0;
    int32  s = 0;
    int8   cnt = 1;

    if (os_strlen(str) > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
    }

    while (*str && cnt++ <= count) {
        s = hexchr2int(*str);
        if (s == -1) {
            break;
        }
        val = (val << 4) + s;
        str++;
    }
    return val;
}


uint32 os_atoh(char *str)
{
    return (uint32)_os_atoh(str, 8);
}

uint64 os_atohl(char *str)
{
    return _os_atoh(str, 16);
}

char *os_strdup(const char *s)
{
    size_t len;
    char *d;

    if (s == NULL) {
        return NULL;
    }

    len = strlen(s);
    d = os_malloc(len + 1);
    if (d == NULL) {
        return NULL;
    }
    os_memcpy(d, s, len);
    d[len] = '\0';
    return d;
}

void print_redirect(osprint_hook print, void *priv, uint8 dual_out)
{
    uint32 flag = disable_irq();
    __print_hook      = print;
    __print_hook_priv = priv;
    if(dual_out){
        __print_ctrl__ |= _PRINT_REDIRECT_DUAL_;
    } else {
        __print_ctrl__ &= ~ _PRINT_REDIRECT_DUAL_;
    }
    enable_irq(flag);
}

void print_level(int8 level)
{
    __print_level__ = level;
}

void disable_print(int8_t dis)
{
    if(dis){
        __print_ctrl__ |= _PRINT_DISABLED_;
    }else{
        __print_ctrl__ &= ~ _PRINT_DISABLED_;
    }
}

void disable_print_color(int8_t dis)
{
    if(dis){
        __print_ctrl__ |= _PRINT_COLOR_DISABLED_;
    }else{
        __print_ctrl__ &= ~ _PRINT_COLOR_DISABLED_;
    }
}

void disable_print_time(int8_t dis)
{
    if(dis){
        __print_ctrl__ |= _PRINT_TIME_DISABLED_;
    }else{
        __print_ctrl__ &= ~ _PRINT_TIME_DISABLED_;
    }
}

void print_with_ntp(uint8 en)
{
    if(en){
        __print_ctrl__ |= _PRINT_TIME_NTP_;
    }else{
        __print_ctrl__ &= ~ _PRINT_TIME_NTP_;
    }
}

static void hgprintf_uart(char *str, int32 len)
{
    int32 off = 0;
    if (len == 0) { len = os_strlen(str); }
    while (off < len) {
        uart_putc((struct uart_device *)console_handle, str[off++]);
    }
}

void hgprintf_out(char *str, int32 len, uint8 level)
{
    osprint_hook _print;
    void *_print_priv;
    uint8 color = 0;

    uint32 flag = disable_irq();
    _print_priv = __print_hook_priv;
    _print = __print_hook;
    enable_irq(flag);

    if (__print_level__ && level > __print_level__) {
        return;
    }

    if (level > 7) 
        level = 7;

    if(level < 7 && !(__print_ctrl__ & _PRINT_COLOR_DISABLED_))
        color = level;
    
    if (_print) {
        if (color) _print(_print_priv, (char *)__print_color__[color], 0);
        _print(_print_priv, str, len);
        if (color) _print(_print_priv, (char *)__print_color__[8], 0);
    } 

    if(_print == NULL || (__print_ctrl__ & _PRINT_REDIRECT_DUAL_)){
        if (color) hgprintf_uart((char *)__print_color__[color], 0);
        hgprintf_uart(str, len);
        if (color) hgprintf_uart((char *)__print_color__[8], 0);
    }

    if (color) sys_errlog_save((char *)__print_color__[color], 0, level);
    sys_errlog_save(str, len, level);
    if (color) sys_errlog_save((char *)__print_color__[8], 0, level);    
}

void hgvprintf(const char *fmt, va_list ap)
{
    int32 ret   = 0;
    int32 len   = 0;
    uint8 level = 7;
    uint8 tick  = 0;
    struct timeval tv;
    struct tm *tm_t;

    if (__print_ctrl__ & _PRINT_DISABLED_) {
        return;
    }

    if (fmt[0] == 2) {
        tick = !(__print_ctrl__ & _PRINT_TIME_DISABLED_);
        fmt++;
    }

    if (fmt[0] == 1) {
        level = fmt[1] - '0';
        fmt += 2;
    }

    if (__print_level__ && level > __print_level__) {
        return;
    }

    if (tick) {
        len = sprintf(_print_buff_p, "[%llu]", os_jiffies_to_msecs(os_jiffies()));
        if (__print_ctrl__ & _PRINT_TIME_NTP_) {
            gettimeofday(&tv, NULL);
            if(tv.tv_sec > 1704038400){
                tm_t = localtime((const time_t *)&tv.tv_sec);
                len += sprintf(_print_buff_p + len, "[%02d/%02d %02d:%02d:%02d-%03llu]",
                                 tm_t->tm_mon + 1, tm_t->tm_mday, tm_t->tm_hour,
                                 tm_t->tm_min, tm_t->tm_sec, tv.tv_usec / 1000);
            }
        }
    }

    ret = vsnprintf(_print_buff_p + len, (PRINT_BUFF_SIZE - 2 - len), fmt, ap);
    if (ret < 0 || ret > (PRINT_BUFF_SIZE - 2 - len)) {
        len = PRINT_BUFF_SIZE - 1;
        _print_buff_p[len - 1] = '\n';
        _print_buff_p[len - 2] = '\r';
    } else {
        len += ret;
    }

    if (len > 0) {
        if (_print_buff_p[len - 1] == '\n' && _print_buff_p[len - 2] != '\r') {
            _print_buff_p[len - 1] = '\r';
            _print_buff_p[len++] = '\n';
        }
        _print_buff_p[len] = 0;
        hgprintf_out(_print_buff_p, len, level);
    }
}
void hgprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    hgvprintf(fmt, ap);
    va_end(ap);
}

int hgputs(const char *s)
{
    hgprintf("%s", s);
    return os_strlen(s) + 1;
}

void dump_hex(char *title, uint8 *data, uint32 len, int32 newline)
{
    int i = 0;
    if (data && len) {
        _os_printf("[dump address: %p]\r\n", data);
        if (title) _os_printf("%s", title);
        for (i = 0; i < len; i++) {
            if (i > 0 && newline) {
                if ((i & 0x7) == 0) _os_printf("   ");
                if ((i & 0xf) == 0) _os_printf("\r\n");
            }
            _os_printf("%02x ", data[i] & 0xFF);
        }
        _os_printf("\r\n");
    }
}

void dump_memory(char *title, uint32 *data, uint32 len)
{
    int i = 0;
    if (data && len) {
        if (title) _os_printf("%s", title);
        for (i = 0; i < len; i++) {
            if ((i & 0x7) == 0)  _os_printf("0x%x: ", data+i);
            if ((i & 0x7) == 7) _os_printf("\r\n");
            _os_printf("%x ", data[i]);
        }
        _os_printf("\r\n");
    }
}

void dump_key(char *str, uint8 *key, uint32 len, uint32 sp)
{
    int32 i = 0;
    if (key && len) {
        if (str) _os_printf("%s", str);
        for (i = 0; i < len; i++) {
            if (sp) {
                _os_printf("%02x ", key[i]);
            } else {
                _os_printf("%02x", key[i]);
            }
        }
        _os_printf("\r\n");
    }
}

// str_buf申请的空间需要比key_len多1byte，sprintf会额外在字符串结束添加0
void key_str(uint8 *key, uint32 key_len, char *str_buf)
{
    int32 i = 0;
    for (i = 0; i < key_len; i++) {
        os_sprintf(str_buf + i * 2, "%02x", key[i]);
    }
}

void *_os_memcpy(void *str1, const void *str2, int32 n)
{
#ifdef MEM_TRACE
#ifdef PSRAM_HEAP
    void *heap = IS_PSRAM_ADDR(str1) ? (void *)(&psram_heap) : (void *)(&sram_heap);
#else
    void *heap = &sram_heap;
#endif
    int32 ret = sysheap_of_check(heap, str1, n);
    if (ret == 0) {
        os_printf(KERN_WARNING"%s: memroy %p maybe overflow!!\r\n", __FUNCTION__, str1);
    }
#endif
    return memcpy(str1, str2, n);
}

char *_os_strcpy(char *dest, const char *src)
{
#ifdef MEM_TRACE
#ifdef PSRAM_HEAP
    void *heap = IS_PSRAM_ADDR(dest) ? (void *)(&psram_heap) : (void *)(&sram_heap);
#else
    void *heap = &sram_heap;
#endif
    int32 n   = strlen(src) + 1;  // +'\0'
    int32 ret = sysheap_of_check(heap, dest, n);
    if (ret == 0) {
        os_printf(KERN_WARNING"%s: memroy %p maybe overflow!!\r\n", __FUNCTION__, dest);
    }
#endif
    return strcpy(dest, src);
}

void *_os_memset(void *str, int c, int32 n)
{
#ifdef MEM_TRACE
#ifdef PSRAM_HEAP
    void *heap = IS_PSRAM_ADDR(str) ? (void *)(&psram_heap) : (void *)(&sram_heap);
#else
    void *heap = &sram_heap;
#endif
    int32 ret = sysheap_of_check(heap, str, n);
    if (ret == 0) {
        os_printf(KERN_WARNING"%s: memroy %p maybe overflow!!\r\n", __FUNCTION__, str);
    }
#endif
    return memset(str, c, n);
}

void *_os_memmove(void *str1, const void *str2, size_t n)
{
#ifdef MEM_TRACE
#ifdef PSRAM_HEAP
    void *heap = IS_PSRAM_ADDR(str1) ? (void *)(&psram_heap) : (void *)(&sram_heap);
#else
    void *heap = &sram_heap;
#endif
    int32 ret = sysheap_of_check(heap, str1, n);
    if (ret == 0) {
        os_printf(KERN_WARNING"%s: memroy %p maybe overflow!!\r\n", __FUNCTION__, str1);
    }
#endif
    return memmove(str1, str2, n);
}

char *_os_strncpy(char *dest, const char *src, int32 n)
{
#ifdef MEM_TRACE
#ifdef PSRAM_HEAP
    void *heap = IS_PSRAM_ADDR(dest) ? (void *)(&psram_heap) : (void *)(&sram_heap);
#else
    void *heap = &sram_heap;
#endif
    int32 ret = sysheap_of_check(heap, dest, n);
    if (ret == 0) {
        os_printf(KERN_WARNING"%s: memroy %p maybe overflow!!\r\n", __FUNCTION__, dest);
    }
#endif
    return strncpy(dest, src, n);
}

int _os_sprintf(char *str, const char *format, ...)
{
#ifdef MEM_TRACE
    int ret;
#ifdef PSRAM_HEAP
    void *heap = IS_PSRAM_ADDR(str) ? (void *)(&psram_heap) : (void *)(&sram_heap);
#else
    void *heap = &sram_heap;
#endif
#endif

    int len;
    va_list ap;

    va_start(ap, format);
    len = vsprintf(str, format, ap);
    va_end(ap);

#ifdef MEM_TRACE
    ret = sysheap_of_check(heap, str, len+1);
    if (ret == 0) {
        os_printf(KERN_WARNING"%s: memroy %p maybe overflow!!\r\n", __FUNCTION__, str);
    }
#endif
    return len;
}

int _os_vsnprintf(char *s, size_t n, const char *format, va_list arg)
{
#ifdef MEM_TRACE
#ifdef PSRAM_HEAP
    void *heap = IS_PSRAM_ADDR(s) ? (void *)(&psram_heap) : (void *)(&sram_heap);
#else
    void *heap = &sram_heap;
#endif
#endif

    int len = vsnprintf(s, n, format, arg);

#ifdef MEM_TRACE
    int check_len = (len < n) ? len + 1 : n;
    int ret = sysheap_of_check(heap, s, check_len);
    if (ret == 0) {
        os_printf(KERN_WARNING"%s: memroy %p maybe overflow!!\r\n", __FUNCTION__, s);
    }
#endif
    return len;
}

int _os_snprintf(char *str, size_t size, const char *format, ...)
{
#ifdef MEM_TRACE
    int ret;
#ifdef PSRAM_HEAP
    void *heap = IS_PSRAM_ADDR(str) ? (void *)(&psram_heap) : (void *)(&sram_heap);
#else
    void *heap = &sram_heap;
#endif
#endif

    int len;
    va_list ap;

    va_start(ap, format);
    len = vsnprintf(str, size, format, ap);
    va_end(ap);

#ifdef MEM_TRACE
    int check_len = (len < size) ? len + 1 : size;
    ret = sysheap_of_check(heap, str, check_len);
    if (ret == 0) {
        os_printf(KERN_WARNING"%s: memroy %p maybe overflow!!\r\n", __FUNCTION__, str);
    }
#endif
    return len;
}

int32 os_strtok(char *str, char *separator, char *argv[], int argv_size)
{
    int32 cnt = 0;
    char *ptr = str;

    if (str == NULL || separator == NULL || argv == NULL || argv_size <= 0) {
        return 0;
    }

    argv[cnt++] = ptr;
    ptr = os_strstr(ptr, separator);
    while (ptr && cnt < argv_size) {
        *ptr = 0;
        ptr += os_strlen(separator);
        if (os_strlen(ptr) == 0) {
            break;
        }

        argv[cnt++] = ptr;
        ptr = os_strstr(ptr, separator);
    }
    return cnt;
}

int os_strncaschr(char *s, int n, char c)
{
    int32 i = 0;
    char c1 = 0, c2 = 0;

    c1 = LOWCASE(c);
    for (i = 0; i < n; i++) {
        c2 = LOWCASE(s[i]);
        if (c1 == c2) {
            return 1;
        }
    }
    return 0;
}

