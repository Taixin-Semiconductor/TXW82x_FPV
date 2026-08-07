// 主要是增加debug流,可以保存log到内存,然后发向流,可以作为定时保存log使用
// 这个流就不作保护,所以最好单线程去使用(如果多线程,可以增加锁),可以使用一个
// 全局去保存对应的句柄,但是这里就不全局保存也不使用全局(可以创建多个流,每个
// 独立的线程可以独立调试)
#include "debug_log_msi.h"

#define DEBUG_LOG_STREAM_MALLOC av_psram_malloc
#define DEBUG_LOG_STREAM_FREE   av_psram_free

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

#define DEBUG_LOG_MAX_NUM    10
#define DEBUG_LOG_CACHE_SIZE (1024 * 4) // 4K

#define DBG_LOG_FAIL_TIMES (3)


static int32_t debug_log_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t           ret   = RET_OK;
    struct dbg_log_s *log_s = (struct dbg_log_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            fbpool_destroy(&log_s->tx_pool);
            if (log_s)
            {
                STREAM_LIBC_FREE(log_s);
                msi->priv = NULL;
            }
        }
        break;

        case MSI_CMD_PRE_DESTROY:
        {
            if (log_s)
            {
                os_work_cancle2(&log_s->work, 1);
            }
        }
        break;

        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            if (fb->data)
            {
                DEBUG_LOG_STREAM_FREE(fb->data);
                fb->data = NULL;
            }
            fbpool_put(&log_s->tx_pool, fb);
            // 不需要内核去释放fb
            ret = RET_OK + 1;
        }
        break;
    }
    return ret;
}

static uint32_t sync_debug_buf(struct dbg_log_s *log_s)
{
    uint32_t flags;
    int      ret = 0;

    struct framebuff *fb = NULL;

    flags = disable_irq();
    if (log_s->fb && log_s->offset)
    {
        fb            = log_s->fb;
        fb->len       = log_s->offset;
        log_s->fb     = NULL;
        log_s->offset = 0;
    }
    enable_irq(flags);

    if (fb)
    {
        ret = fb->len;
        msi_output_fb(log_s->s, fb);
    }

    return ret;
}

static int32 debug_log_sync_work(struct os_work *work)
{
    struct dbg_log_s *log_s = (struct dbg_log_s *) work;
    sync_debug_buf(log_s);
    os_run_work_delay(work, 1000);
    return 0;
}

void *dbg_log_msi(const char *host_name)
{
    struct msi       *s = NULL;
    uint8_t           isnew;
    struct dbg_log_s *log_s;
    s = msi_new(host_name, 0, &isnew);

    if (isnew)
    {
        log_s = (struct dbg_log_s *) STREAM_LIBC_ZALLOC(sizeof(struct dbg_log_s));
        ASSERT(log_s);
        s->priv   = (void *) log_s;
        s->action = (msi_action) debug_log_action;
        fbpool_init(&log_s->tx_pool, DEBUG_LOG_MAX_NUM);
        log_s->s      = s;
        log_s->fb     = NULL;
        log_s->offset = 0;
        OS_WORK_INIT(&log_s->work, debug_log_sync_work, 0);
        os_run_work_delay(&log_s->work, 1000);
        msi_add_output(s, NULL, R_DEBUG_STREAM);
        s->enable = 1;
    }

    return s;
}

uint8_t *get_debug_buf(struct dbg_log_s *log_s, uint32_t *write_len, uint32_t len)
{
    uint8_t *malloc_buf;
    uint8_t *buf;
    uint32_t remain_size;
    uint32_t flags;
    uint8_t  disable_irq_en = 0;
    uint32_t times          = 0;

    struct framebuff *fb = NULL;

again:
    flags = disable_irq();
    if (log_s->fb && (DEBUG_LOG_CACHE_SIZE == log_s->offset))
    {
        fb            = log_s->fb;
        fb->len       = log_s->offset;
        log_s->fb     = NULL;
        log_s->offset = 0;
    }
    enable_irq(flags);

    if (fb)
    {
        msi_output_fb(log_s->s, fb);
    }

    if (!log_s->fb)
    {
        malloc_buf = (uint8_t *) DEBUG_LOG_STREAM_MALLOC(DEBUG_LOG_CACHE_SIZE);
        if (malloc_buf)
        {
            flags          = disable_irq();
            disable_irq_en = 1;
            if (!log_s->fb)
            {
                log_s->fb = fbpool_get(&log_s->tx_pool, 0, log_s->s);
                if (log_s->fb)
                {
                    log_s->fb->data = malloc_buf;
                    log_s->offset   = 0;
                }
            }
            else
            {
                DEBUG_LOG_STREAM_FREE(malloc_buf);
            }
        }
    }

    // 如果没有关闭中断,就关闭中断去操?因为log_s->fb有可能需要同步,会被释放
    if (!disable_irq_en)
    {
        flags          = disable_irq();
        disable_irq_en = 1;
    }
    if (log_s->fb)
    {
        buf         = log_s->fb->data + log_s->offset;
        remain_size = DEBUG_LOG_CACHE_SIZE - log_s->offset;
        if (remain_size > len)
        {
            *write_len = len;
        }
        else
        {
            *write_len = remain_size;
        }

        log_s->offset += (*write_len);
    }
    else
    {
        *write_len = 0;
        buf        = NULL;
    }
    if (disable_irq_en)
    {
        enable_irq(flags);
    }
    times++;
    // 如果保存失败,重新去尝试吧
    if (!buf && (times < DBG_LOG_FAIL_TIMES))
    {
        os_sleep_ms(1);
        goto again;
    }

    return buf;
}



void hgprntf_debug_log_msi(void *priv, char *str, int32 len)
{
    uint32_t in_disable_irq(void);
    if (__in_interrupt() || in_disable_irq())
    {
        return;
    }
    uint8_t          *buf;
    struct dbg_log_s *log_s = (struct dbg_log_s *)priv;
    if (len == 0)
    {
        len = os_strlen(str);
    }

    do
    {
        uint32_t cp_len;
        buf = get_debug_buf(log_s, &cp_len, len);
        if (buf)
        {
            os_memcpy(buf, str, cp_len);
            len -= cp_len;
            str += cp_len;
        }
    } while (len && buf);
}