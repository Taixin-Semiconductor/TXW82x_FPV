#include "log_save_msi.h"

// 接收debug_stream的log保存到sd卡
#define LOG_MALLOC av_psram_malloc
#define LOG_FREE   av_psram_free

#define LOG_SIZE_UINT     (8 * 1024)
#define SAVE_RESERVE_SIZE (10 + 1 + 8 + 1)
#define SAVE_FLAG_FORMAT  "[%08d]:log end\n"

#define LOG_SAVE_PATH "0:log"

static int32 log_save_work(struct os_work *work)
{
    struct log_save_s *log_s = (struct log_save_s *) work;
    struct framebuff  *get_f;
    struct msi        *s    = log_s->s;
    uint32_t           tell = 0;
    uint8_t            filename[64];
    int                ret = 0;
    // 尝试接收数据,看看是否有需要写入到sd卡或者缓冲区buf
    get_f                  = msi_get_fb(s, 0);
    if (get_f)
    {
        uint32_t write_cache_size = get_f->len;
        // 要先判断缓冲区是否足够空间,如果不够,则先将缓冲区写入到sd卡,然后再将数据拷贝到缓冲区
        if (log_s->start_offset + log_s->offset + write_cache_size + SAVE_RESERVE_SIZE > CACHE_SIZE)
        {
            // 将当前缓冲区写入sd卡,然后再将数据拷贝到缓冲区
            if (!log_s->fp)
            {
                struct tm     *time_info;
                struct timeval ptimeval;
                gettimeofday(&ptimeval, NULL);
                time_t time_val = (time_t) ptimeval.tv_sec;

                time_info = gmtime(&time_val);

                uint32_t year = time_info->tm_year + 1900;
                uint32_t mon  = time_info->tm_mon + 1;
                uint32_t day  = time_info->tm_mday;
                uint32_t hour = time_info->tm_hour;
                uint32_t min  = time_info->tm_min;
                uint32_t sec  = time_info->tm_sec;
                uint32_t ms   = os_jiffies() % 1000;

                os_sprintf((char *) filename, "%s/%04d%02d%02d%02d%02d%02d%03d.txt", LOG_SAVE_PATH, year, mon, day, hour, min, sec, ms);
                log_s->fp       = osal_fopen_auto((const char *) filename, "wb", 0);
                log_s->filesize = 0;
                if (log_s->fp)
                {
                    log_s->filesize = LOG_SIZE_UINT;

                    tell = osal_ftell(log_s->fp);
                    // 预分配文件簇
                    osal_fseek(log_s->fp, log_s->filesize);
                    osal_fseek(log_s->fp, tell);
                }

                log_s->lastupdate_file_time = os_jiffies();
                log_s->last_syn_time        = os_jiffies();
            }

            if (log_s->fp)
            {
                if (log_s->fp)
                {
                    tell = osal_ftell(log_s->fp);
                    if (log_s->filesize < tell + log_s->offset)
                    {
                        log_s->filesize += LOG_SIZE_UINT;
                    }
                    // 预分配文件簇
                    osal_fseek(log_s->fp, log_s->filesize);
                    osal_fseek(log_s->fp, tell);
                }
                ret  = osal_fwrite(log_s->save_buf + log_s->start_offset, log_s->offset + SAVE_RESERVE_SIZE, 1, log_s->fp);
                tell = osal_ftell(log_s->fp);
                osal_fseek(log_s->fp, tell - SAVE_RESERVE_SIZE);
                if (!ret)
                {
                    osal_ftruncate(log_s->fp);
                    osal_fclose(log_s->fp);
                    log_s->fp = NULL;
                }
                log_s->start_offset = 0;
                log_s->offset       = 0;
            }

            // 重新将获取到的数据保存到save_buf,可能要考虑没有sd卡的情况?
            // 没有sd卡,直接丢弃?
            // 理论这里offset一定是从0开始
            if (log_s->fp)
            {
                // 判断当前位置,调整start_offset,作为对齐使用
                uint32_t tell       = osal_ftell(log_s->fp);
                log_s->start_offset = 4 - tell % 4;
                hw_memcpy(log_s->save_buf + log_s->start_offset + log_s->offset, get_f->data, write_cache_size);
                os_sprintf((char *) log_s->save_buf + log_s->start_offset + log_s->offset + write_cache_size, SAVE_FLAG_FORMAT, osal_ftell(log_s->fp));
                log_s->offset = write_cache_size;
            }
        }
        // 缓冲区足够,直接拷贝
        else
        {
            // 将当前缓冲区写入sd卡,然后再将数据拷贝到缓冲区
            if (!log_s->fp)
            {
                struct tm     *time_info;
                struct timeval ptimeval;
                gettimeofday(&ptimeval, NULL);
                time_t time_val = (time_t) ptimeval.tv_sec;

                time_info = gmtime(&time_val);

                uint32_t year = time_info->tm_year + 1900;
                uint32_t mon  = time_info->tm_mon + 1;
                uint32_t day  = time_info->tm_mday;
                uint32_t hour = time_info->tm_hour;
                uint32_t min  = time_info->tm_min;
                uint32_t sec  = time_info->tm_sec;
                uint32_t ms   = os_jiffies() % 1000;

                os_sprintf((char *) filename, "%s/%04d%02d%02d%02d%02d%02d%03d.txt", LOG_SAVE_PATH, year, mon, day, hour, min, sec, ms);
                log_s->fp       = osal_fopen_auto((const char *) filename, "wb", 0);
                log_s->filesize = 0;

                log_s->lastupdate_file_time = os_jiffies();
                log_s->last_syn_time        = os_jiffies();
            }

            if (log_s->fp)
            {
                hw_memcpy(log_s->save_buf + log_s->start_offset + log_s->offset, get_f->data, write_cache_size);
                os_sprintf((char *) log_s->save_buf + log_s->start_offset + log_s->offset + write_cache_size, SAVE_FLAG_FORMAT, osal_ftell(log_s->fp) + log_s->offset + write_cache_size);
                log_s->offset += write_cache_size;
            }
        }

        msi_delete_fb(NULL, get_f);
    }

    // 如果存在文件,要判断是否需要更换文件保存log
    if (log_s->fp && (os_jiffies() - log_s->lastupdate_file_time > log_s->update_file_interval))
    {
        if (log_s->offset)
        {
            osal_fwrite(log_s->save_buf + log_s->start_offset, log_s->offset, 1, log_s->fp);
        }
        log_s->last_syn_time        = os_jiffies();
        log_s->lastupdate_file_time = os_jiffies();
        osal_ftruncate(log_s->fp);
        osal_fclose(log_s->fp);
        log_s->fp           = NULL;
        log_s->start_offset = 0;
        log_s->offset       = 0;
    }

    // 先判断是否要切换下一个文件去记录log
    // 去同步一下
    if (os_jiffies() - log_s->last_syn_time > log_s->syn_interval)
    {
        if (log_s->fp)
        {
            if (log_s->offset)
            {
                if (log_s->fp)
                {
                    tell = osal_ftell(log_s->fp);
                    if (log_s->filesize < tell + log_s->offset)
                    {
                        log_s->filesize += LOG_SIZE_UINT;
                    }
                    // 预分配文件簇
                    osal_fseek(log_s->fp, log_s->filesize);
                    osal_fseek(log_s->fp, tell);
                }
                ret = osal_fwrite(log_s->save_buf + log_s->start_offset, log_s->offset + SAVE_RESERVE_SIZE, 1, log_s->fp);
                tell = osal_ftell(log_s->fp);
                osal_fseek(log_s->fp, tell - SAVE_RESERVE_SIZE);
                tell = osal_ftell(log_s->fp);
                if (!ret)
                {
                    osal_ftruncate(log_s->fp);
                    osal_fclose(log_s->fp);
                    log_s->fp = NULL;
                    goto __log_save_work_end;
                }
                log_s->start_offset = 0;
                log_s->offset       = 0;
            }
            osal_fsync(log_s->fp);
        }
        else
        {
            log_s->start_offset = 0;
            log_s->offset       = 0;
        }
        log_s->last_syn_time = os_jiffies();
    }

__log_save_work_end:
    os_run_work_delay(work, 1);
    return 0;
}

static int32_t log_save_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t            ret   = RET_OK;
    struct log_save_s *log_s = (struct log_save_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            if (log_s->fp)
            {
                osal_ftruncate(log_s->fp);
                osal_fclose(log_s->fp);
            }
            if (log_s)
            {
                LOG_FREE(log_s);
                msi->priv = NULL;
            }
        }
        break;

        case MSI_CMD_PRE_DESTROY:
        {
            os_work_cancle2(&log_s->work, 1);
        }
        break;

        case MSI_CMD_FREE_FB:
        {
        }
        break;
    }
    return ret;
}

void *creat_log_save_stream(const char *name, uint32_t syn_interval, uint32_t update_file_interval)
{
    struct log_save_s *log_s = (struct log_save_s *) LOG_MALLOC(sizeof(struct log_save_s));
    struct msi        *s     = NULL;
    if (log_s)
    {
        memset(log_s, 0, sizeof(struct log_save_s));
        log_s->syn_interval         = syn_interval;
        log_s->update_file_interval = update_file_interval;
        s                           = msi_new(name, 10, NULL);
        if (s)
        {
            s->action = log_save_action;
            s->priv   = (void *) log_s;
            s->enable = 1;
            log_s->s  = s;
            OS_WORK_INIT(&log_s->work, log_save_work, 0);
            os_run_work_delay(&log_s->work, 1);
        }
        else
        {
            LOG_FREE(log_s);
            log_s = NULL;
        }
    }
    return s;
}