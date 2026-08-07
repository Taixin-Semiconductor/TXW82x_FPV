
#include "basic_include.h"

#include "fatfs/osal_file.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/multimedia/msi.h"
#include "osal/string.h"
#include "stream_define.h"
#include "loop_record_moudle/loop_record_moudle.h"

void *avimuxer_init2(void *fp,  uint32_t max_size, int w, int h, int frate, int gop, int h265, int sampnum);
uint32_t avimuxer_video2(void *ctx, unsigned char *buf, int len, int key, unsigned pts,uint8_t insert);
uint32_t avimuxer_audio2(void *ctx, unsigned char *buf, int len, int key, unsigned pts);
void avimuxer_sync(void *ctx);
void avimuxer_exit2(void *ctx);

// 结构体申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC os_malloc
#define STREAM_LIBC_FREE   os_free
#define STREAM_LIBC_ZALLOC os_zalloc

#define AVI_LOOP_REMAIN_CAP (1024) // 循环录像剩余空间控制
enum
{
    MSI_AVI_STOP        = BIT(0),
    MSI_AVI_THREAD_DEAD = BIT(1),
};

#define AVI_DIR       "0:/AVI"
#define AVI_FILE_NAME "avi"

struct avi_encode_msi_s
{
    struct msi     *msi;
    struct os_event evt;
    void           *loop; // 循环录像的句柄,定时检查是否足够空间以及是否需要删除文件
    uint16_t        filter_type;
    uint8_t         rec_time;
};

static void *creat_avi_file(char *dir_name)
{

    void    *fp;
    char     filepath[64];
    uint32_t indx = os_jiffies() % 99999999;
    void    *dir  = osal_opendir(dir_name);
    if (!dir)
    {
        osal_fmkdir(dir_name);
    }
    else
    {
        osal_closedir(dir);
    }

    sprintf(filepath, "%s/%08d.%s", dir_name, indx, AVI_FILE_NAME);
    printf("filepath:%s\r\n", filepath);
    fp = osal_fopen(filepath, "wb+");
    return fp;
}

#if 1

static int avi_encode_running(struct msi *msi, uint32_t save_time)
{
    int      ret               = 1;
    uint32_t res               = 0;
    uint32_t AVI_status        = 0;
    uint32_t write_start_time  = 0;
    uint32_t sys_start_time    = os_jiffies();
    uint32_t count_fps         = 0;
    uint32_t last_syn_time     = os_jiffies();
    uint32_t already_save_time = 0;
    uint32_t fbtime            = 0;

    struct avi_encode_msi_s *avi_encode = (struct avi_encode_msi_s *) msi->priv;
    struct framebuff        *fb         = NULL;
    uint32_t                 fps        = 25;
    uint32_t                 fps_time   = 1000 / fps;
    void                    *fp         = creat_avi_file(AVI_DIR);
    void                    *ctx        = avimuxer_init2(fp, 50 * 1024 * 1024, 1280, 720, fps, 0, 0, 0);

    os_printf("fp:%X\tctx:%X\n", fp, ctx);
    if (fp && ctx)
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
            _os_printf("O");
            if ((fb->time - write_start_time) / fps_time > count_fps)
            {
                uint32_t insert_num = ((fb->time - write_start_time) / fps_time) - count_fps;
                // os_printf("insert_num:%d\n", insert_num);
                for (int i = 0; i < insert_num; i++)
                {
                    res |= avimuxer_video2(ctx, fb->data, fb->len, 1, 40, 1);
                    count_fps++;
                }
            }
            res |= avimuxer_video2(ctx, fb->data, fb->len, 1, 40, 0);
            fbtime = fb->time;
            // res |= avimuxer_video2(ctx, test_data, sizeof(data3), 1, 40);
            // os_printf("res:%d\n",res);

            msi_delete_fb(NULL, fb);
            fb = NULL;
            if (res || fbtime - write_start_time >= save_time)
            {
                goto avi_encode_thread_end;
            }
        }
        //音频添加
        else if(fb && fb->mtype == F_AUDIO)
        {
            res |= avimuxer_audio2(ctx, fb->data, fb->len, 0, 0);
            msi_delete_fb(NULL, fb);
            fb = NULL;
            if(res)
            {
                goto avi_encode_thread_end;
            }
        }
        else if(fb)
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
            avimuxer_sync(ctx);
            last_syn_time = os_jiffies();
            already_save_time++;
            os_printf("sync:%d %d\n", already_save_time,count_fps);
        }

        os_sleep_ms(1);
    }

avi_encode_thread_end:
    os_printf("%s:%d\tres:%d\n", __FUNCTION__, __LINE__, res);
    if (res)
    {
        ret = 1;
    }
    if (fb)
    {
        msi_delete_fb(NULL, fb);
    }

    if (ctx)
    {
        avimuxer_exit2(ctx);
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
#endif
// 测试文件保存,保存一分钟吧,一直录卡,直到关闭msi
static void avi_encode_thread(void *d)
{
    int                      ret        = 0;
    struct msi              *msi        = (struct msi *) d;
    struct avi_encode_msi_s *avi_encode = (struct avi_encode_msi_s *) msi->priv;
    uint32_t                 sd_cap;
    void                    *node;
    int                      res;
    msi_get(msi);

    // 先去轮询文件夹,记录文件
    avi_encode->loop = get_file_list(AVI_DIR "/", "." AVI_FILE_NAME);

    while (msi)
    {
        osal_fatfsfree("0:", NULL, &sd_cap);
        os_printf("recount sd_cap:%d\n", sd_cap);
        // 小于1G,则删除一个文件,直到空间足够
        while (sd_cap < AVI_LOOP_REMAIN_CAP)
        {
            char path[64];
            node = get_file_node(avi_encode->loop);
            // 没有文件,但是空间不够,退出,可能是sd卡本身空间不够了
            if (!node)
            {
                // 重新记录文件,如果还是没有文件,代表文件夹没有mp4文件,无法录制了
                avi_encode->loop = get_file_list(AVI_DIR "/", "." AVI_FILE_NAME);

                node = get_file_node(avi_encode->loop);
                if (!node)
                {
                    goto avi_encode_thread_end;
                }
            }
            char *name = get_file_name(node);
            os_printf("del filename:%s\n", name);
            os_sprintf(path, AVI_DIR "/%s", name);
            free_file_node(node);
            res |= osal_unlink((const char*)path);
            os_printf("del res:%d\n", res);
            res |= osal_fatfsfree("0:", NULL, &sd_cap);
            // 可能文件系统异常
            if (res)
            {
                goto avi_encode_thread_end;
            }
            os_printf("del after sd_cap:%d\n", sd_cap);
        }
        msi->enable = 1;
        ret         = avi_encode_running(msi, avi_encode->rec_time * 1000);
        msi->enable = 0;
        os_printf("%s:%d end\n", __FUNCTION__, __LINE__);
        if (ret)
        {
            break;
        }
    }
avi_encode_thread_end:
    os_printf("%s:%d\tret:%d\n", __FUNCTION__, __LINE__, ret);
    free_file_list(avi_encode->loop);
#if 1
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
#endif

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
            // os_printf("mtype:%d\tret:%d\n",fb->mtype,ret);
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