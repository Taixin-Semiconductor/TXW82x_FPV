/***********************************************************************
 * 这个文件仅仅是中转数据,主要为了一些硬件模块是插入才有数据,所以外部希望
 * 绑定的时候,可能硬件没有准备好,所以创建一个中转msi,硬件准备好就发送到
 * 中转msi,然后中转msi帮忙转发出去,外部调用就是用中转msi帮定就可以了,
 * 这样硬件准备好会自动通过中转msi去转发
 ***********************************************************************/
#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE av_free
#define STREAM_LIBC_ZALLOC av_zalloc

static int32_t route_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    switch (cmd_id)
    {
        // 帮忙转发,但是永远不需要放在队列中
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;
            fb_get(fb);
            msi_output_fb(msi, fb);
            ret = RET_OK + 1;
        }
        break;
        default:
        {
        }
        break;
    }

    return ret;
}
// 中转(路由)msi,不需要接收,仅仅发送就可以了
struct msi *route_msi(const char *name)
{
    // 设置1个接收,只是为了可以output_fb给到这个msi
    struct msi *msi = msi_new(name, 0, NULL);
    if (msi)
    {
        msi->action = route_msi_action;
        msi->enable = 1;
    }
    return msi;
}
