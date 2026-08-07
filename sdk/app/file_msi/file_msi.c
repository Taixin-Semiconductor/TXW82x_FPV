#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/fs/fatfs/osal_file.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "stream_define.h"

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

#define FILE_MAX_RECV 64
struct file_msi_s
{
    os_task_t   task;
    struct msi *msi;
};

// 没有实现退出功能
static void file_thread(void *d)
{
    struct file_msi_s *file_s = (struct file_msi_s *) d;
    struct framebuff  *fb;
    struct framebuff  *tmp_fb;
    void              *fp;
    msi_get(file_s->msi);
    struct file_msg_s *file_msg;
    uint32_t write_max_len;
    uint32_t write_len;
    uint32_t res;
    while (1)
    {
        fb = msi_get_fb(file_s->msi, -1);
        if (fb)
        {
            file_msg = (struct file_msg_s *) fb->priv;
            os_printf(KERN_INFO "file_msg->path:%s\tlen:%d\tishid:%d\n", file_msg->path,fb->len,file_msg->ishid);
            // 暂时没有考虑不存在是否需要创建文件夹
            fp = osal_fopen_auto((const char*)file_msg->path, "wb", file_msg->ishid);
            if (fp)
            {
                tmp_fb = fb;
                write_max_len = fb->len;
                while(tmp_fb && write_max_len)
                {
                    if(!tmp_fb->clone)
                    {
                        write_len = write_max_len>tmp_fb->len?tmp_fb->len:write_max_len;
                        write_max_len -= write_len;
                        res = osal_fwrite(tmp_fb->data, write_len, 1, fp);
                        if(!res)
                        {
                            os_printf(KERN_ERR"write file err:%d\n",get_errno());
                            break;
                        }
                    }
                    tmp_fb = tmp_fb->next;
                }
                
                // 这里需要轮询,找到不是clone的模块
                osal_fclose(fp);
            }
            else
            {
                os_printf(KERN_ERR"save file err:%d\n",get_errno());
            }
            msi_delete_fb(NULL, fb);
        }
    }
    msi_put(file_s->msi);
}

int32_t file_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t            ret    = RET_OK;
    struct file_msi_s *file_s = (struct file_msi_s *) msi->priv;
	(void)file_s;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
        }
        break;
        case MSI_CMD_PRE_DESTROY:
        {
        }
        break;

        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            if (fb->mtype != F_FILE_T)
            {
                ret = RET_ERR;
            }
            else
            {
            }
        }
        break;
    }
    return ret;
}

struct msi *file_msi_init(const char *msi_name)
{
    uint8_t            is_new;
    struct file_msi_s *file_s;
    struct msi        *msi = msi_new(msi_name, FILE_MAX_RECV, &is_new);
    if (is_new)
    {
        file_s      = (struct file_msi_s *) STREAM_LIBC_ZALLOC(sizeof(struct file_msi_s));
        msi->priv   = (void *) file_s;
        file_s->msi = msi;
        msi->action = file_msi_action;
        msi->enable = 1;
        OS_TASK_INIT("file_s_task", &file_s->task, file_thread, file_s, OS_TASK_PRIORITY_ABOVE_NORMAL, NULL, 1500);
    }

    return msi;
}