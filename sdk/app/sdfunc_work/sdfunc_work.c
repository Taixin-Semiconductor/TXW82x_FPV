#include "osal/sleep.h"
#include "osal/work.h"
#include "osal/irq.h"
#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/fs/fatfs/osal_file.h"
#include "user_work/sd_work.h"

#define SD_FILENAME_LEN (64)
/**********************************************************************************
 * 以下是应用层代码,封装了一个sd写入的接口
 * 写入模式:clone的fb会跳过,只会写入非clone的fb,不对数据做任何处理
 *********************************************************************************/

struct os_work_sdfunc
{
    struct os_work    work;
    struct framebuff *fb;
    char              filename[128];
    uint8_t           ishid;
};

static int32 os_work_func_cb(struct os_work *work)
{
    struct os_work_sdfunc *wk = container_of(work, struct os_work_sdfunc, work);
    struct framebuff      *fb;
    struct framebuff      *tmp_fb;
    void                  *fp;
    uint32_t               write_max_len;
    uint32_t               write_len;
    uint32_t               res;
    fp = osal_fopen_auto((const char *) wk->filename, "w+", wk->ishid);
    fb = wk->fb;
    ASSERT(fb);
    if (fp)
    {
        tmp_fb        = fb;
        write_max_len = fb->len;
        while (tmp_fb && write_max_len)
        {
            if (!tmp_fb->clone)
            {
                write_len = write_max_len > tmp_fb->len ? tmp_fb->len : write_max_len;
                write_max_len -= write_len;
                res = osal_fwrite(tmp_fb->data, write_len, 1, fp);
                if (!res)
                {
                    os_printf(KERN_ERR "write file err:%d\n", get_errno());
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
        os_printf(KERN_ERR "save file err:%d\n", get_errno());
    }
    msi_delete_fb(NULL, fb);

    return 0;
}

int32 sd_fb_write_work(struct framebuff *fb, const char *filename, uint8_t ishid)
{
    struct os_work_sdfunc *work = NULL;
    int                    ret;

    if (filename == NULL || fb == NULL)
    {
        return -EINVAL;
    }
    // 文件名不能超过SD_FILENAME_LEN
    if (strlen(filename) >= SD_FILENAME_LEN)
    {
        return -EINVAL;
    }

    work = os_zalloc(sizeof(struct os_work_sdfunc));
    if (work == NULL)
    {
        os_printf(KERN_ERR "%s:%d\tmalloc work failed\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }
    fb_get(fb);
    work->work.func  = os_work_func_cb;
    work->work.alloc = 1;
    work->work.init  = 1;
    work->fb         = fb;
    os_memcpy(work->filename, filename, strlen(filename));
    work->ishid = ishid;

    ret = os_run_sdwork(&work->work);
    if (ret != RET_OK)
    {
        fb_put(fb);
    }
    return ret;
}