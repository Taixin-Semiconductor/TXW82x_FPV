
#include "basic_include.h"
#include "fatfs/osal_file.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/multimedia/msi.h"
#include "stream_define.h"
#include "video_app/file_common_api.h"
#include "loop_record_moudle/loop_record_moudle.h"
#include "app/ffavimuxer/avimuxer.h"
#include "app/record/mux_file.h"

// 结构体申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC os_malloc
#define STREAM_LIBC_FREE   os_free
#define STREAM_LIBC_ZALLOC os_zalloc

#define AVI_MAX_SINGLE_SIZE (50 * 1024 * 1024) // 单个AVI文件大小
#define AVI_LOOP_REMAIN_CAP (256)              // 循环录像剩余空间控制

#define AVI_DIR       "0:/AVI"
#define AVI_FILE_NAME ".AVI"

enum
{
    MSI_AVI_STOP        = BIT(0),
    MSI_AVI_THREAD_DEAD = BIT(1),
};

struct avi_encode_msi_s
{
    struct msi     *msi;
    struct os_event evt;
    void           *loop; // 循环录像的句柄,定时检查是否足够空间以及是否需要删除文件
    uint16_t        filter_type;
    uint8_t         rec_time;
};

static void *creat_avi_file(void **loop, char *file_name)
{
    void    *fp   = NULL;
    void    *node = NULL;
    char    *dir_path;
    char     file_path[64];
    char     sub_path[32];
    uint32_t sd_cap     = 0;
    int      res        = 0;
    uint8_t  changeflag = 0;
    uint32_t file_size  = 0;

    res = osal_fatfsfree("0:", NULL, &sd_cap);
    os_printf(KERN_INFO "sd_cap: %d\n", sd_cap);
    if (res != 0 || sd_cap == 0)
    {
        goto creat_avi_file_end;
    }

get_node:
    if (sd_cap < AVI_LOOP_REMAIN_CAP)
    {
        if (!*loop)
        {
            *loop = get_file_list(AVI_DIR, AVI_FILE_NAME);
        }

        if (*loop)
        {
            node = get_file_node(*loop);
            if (!node)
            {
                free_file_list(*loop);
                *loop = get_file_list(AVI_DIR, AVI_FILE_NAME);
                if (!*loop)
                {
                    _os_printf("%s %d\r\n", __FUNCTION__, __LINE__);
                    goto creat_avi_file_end;
                }
                node = get_file_node(*loop);
            }
            while (!node)
            {
                dir_path = get_file_dir(*loop);
                res      = osal_unlink_dir(dir_path, 0);
                if (res != FR_OK)
                {
                    _os_printf("unlink dir %s err, res: %d\r\n", dir_path, res);
                    res = osal_unlink_dir(dir_path, 1);
                    if (res != FR_OK)
                    {
                        _os_printf("%s %d, force unlink dir %s err, res: %d\r\n", __FUNCTION__, __LINE__, dir_path, res);
                        goto creat_avi_file_end;
                    }
                    _os_printf("force unlink dir %s\r\n", dir_path);
                }
                _os_printf("unlink dir %s\r\n", dir_path);
                free_file_list(*loop);
                *loop = get_file_list(AVI_DIR, AVI_FILE_NAME);
                if (!*loop)
                {
                    _os_printf("%s %d\r\n", __FUNCTION__, __LINE__);
                    goto creat_avi_file_end;
                }
                node = get_file_node(*loop);
                if (node)
                {
                    break;
                }
            }
            file_size = get_file_size(node);
            if (file_size < AVI_MAX_SINGLE_SIZE)
            {
                char  path[64];
                char *name = get_file_name(node);
                dir_path   = get_file_dir(*loop);
                os_sprintf(path, "%s/%s", dir_path, name);
                res = osal_unlink(path);
                if (res == FR_OK)
                {
                    _os_printf("unlink file %s, filesize: %d\r\n", path, file_size);
                }
                else
                {
                    _os_printf("%s %d\tunlink file %s fail, res: %d\r\n", __FUNCTION__, __LINE__, path, res);
                }
                free_file_node(node);
                node = NULL;
                goto get_node;
            }
            changeflag = 1;
        }
        else
        {
            _os_printf("%s %d\r\n", __FUNCTION__, __LINE__);
            goto creat_avi_file_end;
        }
    }

    if (get_extension_file_name(AVI_DIR, sub_path, file_name, AVI_FILE_NAME))
    {
        _os_printf("%s %d\tget_avi_file_name fail\r\n", __FUNCTION__, __LINE__);
        goto creat_avi_file_end;
    }
    os_sprintf(file_path, "%s/%s", sub_path, file_name);
    os_printf(KERN_INFO "avi file_path: %s\r\n", file_path);

    void *sub_dir = osal_opendir(sub_path);
    if (!sub_dir)
    {
        res = osal_fmkdir(sub_path);
        if (res != FR_OK)
        {
            if (res == FR_DENIED)
            {
                char  path[64];
                char *name = get_file_name(node);
                dir_path   = get_file_dir(*loop);
                os_sprintf(path, "%s/%s", dir_path, name);
                res = osal_unlink(path);
                if (res != FR_OK)
                {
                    _os_printf("%s %d\tunlink file %s fail, res: %d\r\n", __FUNCTION__, __LINE__, path, res);
                    goto creat_avi_file_end;
                }
                _os_printf("unlink file %s\r\n", path);
                free_file_node(node);
                node = NULL;
                res  = osal_fmkdir(sub_path);
                if (res == FR_OK)
                {
                    goto get_node;
                }
                else
                {
                    _os_printf("%s %d\tmkdir %s fail, res: %d\r\n", __FUNCTION__, __LINE__, sub_path, res);
                    goto creat_avi_file_end;
                }
            }
            _os_printf("mkdir %s err, create rec dir\r\n", sub_path);

            void *rec_dir = osal_opendir(AVI_DIR);
            if (!rec_dir)
            {
                res = osal_fmkdir(AVI_DIR);
                if (res != FR_OK)
                {
                    _os_printf("%s %d\tmkdir %s fail, res: %d\r\n", __FUNCTION__, __LINE__, AVI_DIR, res);
                    goto creat_avi_file_end;
                }
                else
                {
                    res = osal_fmkdir(sub_path);
                    if (res != FR_OK)
                    {
                        _os_printf("%s %d\tmkdir %s fail, res: %d\r\n", __FUNCTION__, __LINE__, sub_path, res);
                        goto creat_avi_file_end;
                    }
                }
            }
            else
            {
                osal_closedir(rec_dir);
                _os_printf("%s %d\tmkdir %s fail, res: %d\r\n", __FUNCTION__, __LINE__, sub_path, res);
                goto creat_avi_file_end;
            }
        }
        _os_printf("mkdir %s\r\n", sub_path);
    }
    else
    {
        osal_closedir(sub_dir);
    }

    if (changeflag)
    {
        char  old_filepath[64];
        char *name = get_file_name(node);
        dir_path   = get_file_dir(*loop);
        os_sprintf(old_filepath, "%s/%s", dir_path, name);
        res = osal_rename(old_filepath, file_path);
        if (res != FR_OK)
        {
            _os_printf("rename file %s err, res: %d\r\n", old_filepath, res);
            if (sd_cap < AVI_LOOP_REMAIN_CAP)
            {
                _os_printf("%s %d\tsd_cap: %d\r\n", __FUNCTION__, __LINE__, sd_cap);
                free_file_node(node);
                node = NULL;
                goto get_node;
            }
        }
        else
        {
            _os_printf("rename file %s to %s\r\n", old_filepath, file_path);
        }

#if 0
        char thumb_path[64];
        gen_thumb_path(name, thumb_path, sizeof(thumb_path));
        res = osal_unlink(thumb_path);
        if (res != FR_OK) {
            _os_printf("unlink thumb_path %s err, res: %d\r\n", thumb_path, res);
        } else {
            _os_printf("unlink thumb_path: %s\r\n", thumb_path);
        }
#endif

        free_file_node(node);
        node = NULL;

        char *min_file = get_min_file(*loop);
        if (min_file && (os_strcmp(file_name, min_file) < 0))
        {
            free_file_list(*loop);
            *loop = NULL;
            _os_printf("%s %d\tfree_file_list\r\n", __FUNCTION__, __LINE__);
        }
    }

    fp = osal_fopen(file_path, "wb+");
creat_avi_file_end:
    if (node)
    {
        free_file_node(node);
        node = NULL;
    }
    return fp;
}

static int avi_encode_running(struct msi *msi, uint32_t save_time)
{
    int      ret               = 0;
    uint32_t res               = 0;
    uint32_t AVI_status        = 0;
    uint32_t write_start_time  = 0;
    uint32_t sys_start_time    = os_jiffies();
    uint32_t count_fps         = 0;
    uint32_t audio_fps         = 0;
    uint32_t last_syn_time     = os_jiffies();
    uint32_t already_save_time = 0;
    uint32_t fbtime            = 0;

    struct avi_encode_msi_s *avi_encode = (struct avi_encode_msi_s *) msi->priv;
    struct framebuff        *fb         = NULL;
    uint32_t                 fps        = 25;
    uint32_t                 fps_time   = 1000 / fps;
    char                     filename[64];
    void                    *fp         = creat_avi_file(&avi_encode->loop, filename);
    void                    *mux_file   = NULL;
    file_ops_t               file_ops;
    void                    *ctx        = NULL;

    if (fp)
    {
        mux_file = mux_file_open((F_FILE *) fp, AVI_MAX_SINGLE_SIZE, MUX_FILE_ALIGN_EN);
        if (mux_file)
        {
            mux_file_get_ops(mux_file, &file_ops);
            ctx = avimuxer_init_with_file(fp, &file_ops, AVI_MAX_SINGLE_SIZE, 1280, 720, fps, 0, 1);
        }
    }

    os_printf(KERN_INFO"fp:%X\tctx:%X\n", fp, ctx);
    if (!fp || !mux_file || !ctx)
    {
        ret = 1;
        goto avi_encode_thread_end;
    }
    else
    {
        ret = 0;
    }

    while (fp)
    {
        os_event_wait(&avi_encode->evt, MSI_AVI_STOP, &AVI_status, OS_EVENT_WMODE_OR, 0);
        // 结束写卡
        if (AVI_status & MSI_AVI_STOP)
        {
            ret = 1;
            os_printf("%s:%d", __FUNCTION__, __LINE__);
            goto avi_encode_thread_end;
        }
        fb = msi_get_fb(msi, 0);
        if (fb && fb->mtype == F_JPG)
        {
            if (write_start_time == 0)
            {
                write_start_time = fb->time;
            }
            count_fps++;
            _os_printf(KERN_INFO "O");
            if ((fb->time - write_start_time) / fps_time > count_fps)
            {
                uint32_t insert_num = ((fb->time - write_start_time) / fps_time) - count_fps;
                for (int i = 0; i < insert_num; i++)
                {
                    res |= avimuxer_video(ctx, fb->data, fb->len, 1, 1);
                    count_fps++;
                }
            }
            res |= avimuxer_video(ctx, fb->data, fb->len, 1, 0);
            fbtime = fb->time;

            msi_delete_fb(NULL, fb);
            fb = NULL;
            if (res || fbtime - write_start_time >= save_time)
            {
                goto avi_encode_thread_end;
            }
        }
        // 音频添加
        else if (write_start_time && fb && fb->mtype == F_AUDIO)
        {
            _os_printf(KERN_INFO "A");
            audio_fps++;
            res |= avimuxer_audio(ctx, fb->data, fb->len);
            msi_delete_fb(NULL, fb);
            fb = NULL;
            if (res)
            {
                goto avi_encode_thread_end;
            }
        }
        else if (fb)
        {
            msi_delete_fb(NULL, fb);
            fb = NULL;
        }
        else
        {
            os_sleep_ms(1);
        }

        // 如果系统时间超过了30s依然没有保存完成,就直接退出
        if (os_jiffies() - sys_start_time >= save_time + 30 * 1000)
        {
            goto avi_encode_thread_end;
        }

        if (os_jiffies() - last_syn_time > 1000)
        {
            // avimuxer_sync(ctx);
            last_syn_time = os_jiffies();
            already_save_time++;
            os_printf(KERN_INFO"sync:%d %d %d\n", already_save_time, count_fps,audio_fps);
        }

        os_sleep_ms(1);
    }

avi_encode_thread_end:
    os_printf(KERN_EMERG "%s:%d\tres:%d\n", __FUNCTION__, __LINE__, res);
    if (fb)
    {
        msi_delete_fb(NULL, fb);
    }

    if (ctx)
    {
        avimuxer_exit(ctx);
    }

    if (mux_file)
    {
        mux_file_close(mux_file);
        mux_file = NULL;
    }

    if (fp)
    {
        osal_fclose(fp);
        fp = NULL;
    }

    // os_printf("save time:%d\tv_count:%d\n",(uint32_t)os_jiffies(),v_count);
    os_printf("avi encode end\n");
    return ret;
}

// 测试文件保存,保存一分钟吧,一直录卡,直到关闭msi
static void avi_encode_thread(void *d)
{
    int                      ret        = 0;
    struct msi              *msi        = (struct msi *) d;
    struct avi_encode_msi_s *avi_encode = (struct avi_encode_msi_s *) msi->priv;

    msi_get(msi);

    while (msi)
    {
        msi->enable = 1;
        ret         = avi_encode_running(msi, avi_encode->rec_time * 1000);
        msi->enable = 0;
        os_printf(KERN_EMERG "%s:%d ret:%d\tend\n", __FUNCTION__, __LINE__, ret);
        if (ret)
        {
            break;
        }
    }

    os_printf("%s:%d\tret:%d\n", __FUNCTION__, __LINE__, ret);
    if (avi_encode->loop)
    {
        free_file_list(avi_encode->loop);
        avi_encode->loop = NULL;
    }

    struct framebuff *fb;
    while (1)
    {
        fb = msi_get_fb(msi, 0);
        if (fb)
        {
            msi_delete_fb(NULL, fb);
        }
        else
        {
            break;
        }
    }

    os_event_set(&avi_encode->evt, MSI_AVI_THREAD_DEAD, NULL);
    msi_put(msi);
    os_printf("%s:%d end\n", __FUNCTION__, __LINE__);
}

static int32_t avi_encode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t                  ret        = RET_OK;
    struct avi_encode_msi_s *avi_encode = (struct avi_encode_msi_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
            os_event_wait(&avi_encode->evt, MSI_AVI_THREAD_DEAD, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, -1);
            os_event_del(&avi_encode->evt);
            STREAM_LIBC_FREE(avi_encode);
            break;
        case MSI_CMD_PRE_DESTROY:
            os_event_set(&avi_encode->evt, MSI_AVI_STOP, NULL);
            break;

        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;

            if (fb->mtype == F_JPG && avi_encode->filter_type != (uint16_t) ~0)
            {
                ret = RET_ERR;
                if (avi_encode->filter_type == fb->stype)
                {
                    ret = RET_OK;
                }
            }
        }
        break;

        case MSI_CMD_GET_RUNNING:
        {
            uint32_t rflags = 0;
            os_event_wait(&avi_encode->evt, MSI_AVI_THREAD_DEAD | MSI_AVI_STOP, &rflags, OS_EVENT_WMODE_OR, 0);
            if (param1)
            {
                *(uint32_t *) param1 = (rflags & (MSI_AVI_THREAD_DEAD | MSI_AVI_STOP)) ? 0 : 1;
            }
        }
        break;
    }
    return ret;
}

struct msi *avi_encode_msi_init(const char *avi_msi_name, uint16_t filter_type, uint8_t rec_time)
{
    struct avi_encode_msi_s *avi_encode = NULL;
    struct msi              *msi        = msi_new(avi_msi_name, 64, NULL);
    if (msi && !msi->priv)
    {
        avi_encode = (struct avi_encode_msi_s *) STREAM_LIBC_ZALLOC(sizeof(struct avi_encode_msi_s));
        ASSERT(avi_encode);
        avi_encode->filter_type = filter_type;
        avi_encode->rec_time    = rec_time;
        msi->priv               = avi_encode;
        os_event_init(&avi_encode->evt);
        avi_encode->msi = msi;
        msi->action     = avi_encode_msi_action;
        msi->enable     = 1;
    }
    else
    {
        // 不要重复打开,同一个名称需要等待上一次写入完成后并且关闭后才可以重新打开
        // 因为这里new了一次,所以要destroy一次(new和destroy要配对,实际msi的destroy是在另一个地方)
        if (msi)
        {
            msi_destroy(msi);
            msi = NULL;
            goto avi_encode_msi_init_end;
        }
    }

    void *avi_hdl = os_task_create("avi_encode", avi_encode_thread, msi, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2048);
    os_printf("avi_hdl:%X\n", avi_hdl);
    if (!avi_hdl && avi_encode)
    {
        os_event_set(&avi_encode->evt, MSI_AVI_THREAD_DEAD, NULL);
    }
avi_encode_msi_init_end:
    return msi;
}
