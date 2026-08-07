/*******************************************************************************************************
 * 该文件主要是为了给jpeg配置需要解码的信息,然后传递给到硬件解码
 ******************************************************************************************************/
#include "sys_config.h"
#include "typesdef.h"
#include "dev.h"
#include "devid.h"
#include "stream_frame.h"
#include "utlist.h"
#include "osal/task.h"
#include "osal/string.h"
#include "osal/work.h"
#include "osal_file.h"
#include "jpgdef.h"
#include "utlist.h"
#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "stream_define.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "user_work/user_work.h"

// 获取jpeg的w和h,没有做太多容错,所以尽量给的是正确的jpeg,否则可能异常
#define GET_16(p) (((p)[0] << 8) | (p)[1])

static int parse_SOF(uint8_t *d, uint32_t *w, uint32_t *h)
{
    if (d[0] != 8)
    {
        printf("Invalid precision %d in SOF0\n", d[0]);
        return -1;
    }
    *h = GET_16(d + 1);
    *w = GET_16(d + 3);
    return 0;
}

static int parse_jpg(uint8_t *jpg_buf, uint32_t maxsize, uint32_t *w, uint32_t *h)
{
    uint8_t *buf = jpg_buf;
    uint8_t EOI_flag = 0;
    uint32_t i = 0;
    int res = 1;
    int blen = 0;
    for (i = 0; i < maxsize; i += blen + 2)
    {
        if (buf[i] != 0xFF)
        {
            printf("Found %02X at %d, expecting FF\n", buf[i], i);
            goto parse_jpg_end;
        }
        while (buf[i + 1] == 0xFF)
            ++i;
        if (buf[i + 1] == 0xD8)
            blen = 0;
        else
            blen = GET_16(buf + i + 2);

        switch (buf[i + 1])
        {
            case 0xDB: /* Quantization Table */
                break;
            case 0xC0: /* Start of Frame */
                parse_SOF(buf + i + 4, w, h);
                res = 0;
                // printf("w:%d\th:%d\n",*w,*h);
                break;
            case 0xC4: /* Huffman Table */

                break;
            case 0xDD: /* DRI */
                break;
            case 0xDA: /* Start of Scan */
                goto parse_jpg_end;
        }
    }


parse_jpg_end:

    if (!res) {
        for (i = maxsize - 1; i >= maxsize - 16; i--)
        {
            if (buf[i] != 0xFF) {
                // os_printf("EOI Found %02X at %d, expecting FF\n", buf[i], i);
                continue;
            }
    
            if ((i < maxsize - 1) && (buf[i + 1] == 0xD9)) {
                EOI_flag = 1;
                break;
            }
        }
    }

    if ((!res) && (!EOI_flag)) {
        res = 1;
        os_printf("EOI No found FF D9\n");
    }

    return res;
}

int ex_parse_jpg(uint8_t *jpg_buf, uint32_t maxsize, uint32_t *w, uint32_t *h)
{
    return parse_jpg(jpg_buf,maxsize,w,h);
}
// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE av_free
#define STREAM_LIBC_ZALLOC av_zalloc

struct jpg_decode_msg_s
{
    struct os_work work;
    struct msi *msi;
    struct framebuff *rfb;
    uint32_t magic; //会赋值到对应参数,用于后续msi的识别,如果为0,就是没有任何处理
    uint16_t out_w;
    uint16_t out_h;
    uint16_t step_w;
    uint16_t step_h;
    uint16_t x, y; // 解码配置的x和y,如果没有可以不配置
    uint16_t filter;    //过滤解码的类型
    uint16_t force_type; // 强转类型(统一将解码的类型修改,不按照filter,0是无效)
};

static int32 jpg_decode_msg_work(struct os_work *work)
{
    struct jpg_decode_msg_s *decode_msg = (struct jpg_decode_msg_s *)work;
    struct framebuff *rfb;
    struct framebuff *fb;
    int res;
    if (decode_msg->rfb)
    {
        rfb = decode_msg->rfb;
        // 这里可以考虑用自己的内存管理,后续看情况修改接口
        fb = fb_alloc(NULL, sizeof(struct jpg_decode_arg_s), 0, decode_msg->msi);
        // 申请不到就等下次进来再去尝试解码吧
        if (fb)
        {
            fb_ref(fb, rfb);
            fb->datatag = rfb->datatag;
            fb->stype = rfb->stype;
            fb->srcID = rfb->srcID;
            struct jpg_decode_arg_s *msg = (struct jpg_decode_arg_s *)fb->data;
            struct yuv_arg_s *yuv_msg = &msg->yuv_arg;
            memset(msg, 0, sizeof(struct jpg_decode_arg_s));
            yuv_msg->out_w = decode_msg->out_w;
            yuv_msg->out_h = decode_msg->out_h;
            yuv_msg->x = decode_msg->x;
            yuv_msg->y = decode_msg->y;
            yuv_msg->magic = decode_msg->magic;
            yuv_msg->dispcnt = fb->datatag;
            //os_printf("yuv_msg->dispcnt:%d\n",yuv_msg->dispcnt);
            msg->step_w = decode_msg->step_w;
            msg->step_h = decode_msg->step_h;
            
            res = parse_jpg(rfb->data, rfb->len, &msg->decode_w, &msg->decode_h);
            if (!res)
            {
                fb->mtype = F_JPG_DECODE_MSG;
                fb->stype = decode_msg->force_type ?decode_msg->force_type : rfb->stype;
                fb->time = rfb->time;
                msi_output_fb(decode_msg->msi, fb);
                msi_delete_fb(NULL, rfb);
                decode_msg->rfb = NULL;
            }
            else
            {
                msi_delete_fb(NULL, fb);
                msi_delete_fb(NULL, rfb);
                decode_msg->rfb = NULL;                
            }
        }
    }
    else
    {
        decode_msg->rfb = msi_get_fb(decode_msg->msi, 0);
    }
    os_run_work_delay(work, 1);
    return 0;
}

static int32_t decode_msg_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int32_t ret = RET_OK;
    struct jpg_decode_msg_s *decode_msg = (struct jpg_decode_msg_s *)msi->priv;
    switch (cmd_id)
    {

        // 这里msi已经被删除,那么就要考虑tx_pool的资源释放了
        // 能进来这里,就是代表所有fb都已经用完了
        case MSI_CMD_POST_DESTROY:
        {
            os_work_cancle2(&decode_msg->work, 1);
            STREAM_LIBC_FREE(decode_msg);
        }
        break;

        case MSI_CMD_PRE_DESTROY:
        {
            os_work_cancle2(&decode_msg->work, 1);
        }
            break;

        case MSI_CMD_DECODE_JPEG_MSG:
        {
            uint32_t cmd_self = (uint32_t)param1;
            uint32_t arg = param2;
            switch (cmd_self)
            {
                case MSI_JPEG_DECODE_X_Y:
                {
                    decode_msg->x = arg >> 16;
                    decode_msg->y = arg & 0xffff;
                }
                break;
                
                //配置强转类型
                case MSI_JPEG_DECODE_FORCE_TYPE:
                {
                    decode_msg->force_type = arg;
                }
                break;

                case MSI_JPEG_DECODE_MAGIC:
                {
                    decode_msg->magic = arg;
                }
                break;
            }
        }
        break;
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;
            if (fb->mtype != F_JPG)
            {
                ret = RET_OK + 1;
            }

            if(decode_msg->filter && !(decode_msg->filter == fb->stype))
            {
                ret = RET_OK + 1;
            }
        }
        break;
    }

    return ret;
}

struct msi *jpg_decode_msg_msi(const char *name, uint16_t out_w, uint16_t out_h, uint16_t step_w, uint16_t step_h, uint32_t filter)
{
    struct msi *msi = msi_new(name, 8, NULL);
    struct jpg_decode_msg_s *decode_msg = (struct jpg_decode_msg_s *)msi->priv;
    if (!decode_msg)
    {
        decode_msg = (struct jpg_decode_msg_s *)STREAM_LIBC_ZALLOC(sizeof(struct jpg_decode_msg_s));
        msi->priv = (void *)decode_msg;
        msi->action = decode_msg_msi_action;
        decode_msg->msi = msi;
        decode_msg->out_w = out_w;
        decode_msg->out_h = out_h;
        decode_msg->step_w = step_w;
        decode_msg->step_h = step_h;
        decode_msg->filter = filter;
        decode_msg->rfb = NULL;
        msi->enable = 1;
        // 启动workqueue
        OS_WORK_INIT(&decode_msg->work, jpg_decode_msg_work, 0);
        os_run_work_delay(&decode_msg->work, 1);
    }

    return msi;
}
