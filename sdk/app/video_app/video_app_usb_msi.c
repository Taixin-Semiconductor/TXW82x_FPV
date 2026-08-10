/*
    在硬件初始化后,这里就是相关video的接口
*/


#include "video_app_usb_msi.h"
#include "stream_define.h"
#include "utlist.h"
#include "usbh_video.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "sysevt_usb/sysevt_usb.h"

#define MAX_UVC_APP                     2

#define UVC_YUV422_MJPEG_FORMAT         0     /* 对于 U、V 量化表单独指向的 Mjpeg 图片，需要软件修改指向同一份量化表 (Mjpeg 硬件解码需要) */

// data申请空间函数
#define STREAM_MALLOC                   av_psram_malloc
#define STREAM_FREE                     av_psram_free
#define STREAM_ZALLOC                   av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC              av_malloc
#define STREAM_LIBC_FREE                av_free
#define STREAM_LIBC_ZALLOC              av_zalloc

#define UVC_PSRAM_MALLOC_SIZE_INIT      (100 * 1024)

#if UVC_YUV422_MJPEG_FORMAT
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

    *(d+14) = 0x1;  /* 对于 U、V 量化表单独指向的 Mjpeg 图片，需要软件修改指向同一份量化表 */

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
            printf("usb Found %02X at %d, expecting FF\n", buf[i], i);
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
#endif

static void msi_get_usb_psram_thread(void *d)
{
    // 这里为了让usb那边初始化好对应的空间
    os_printf("%s:%d\n", __FUNCTION__, __LINE__);
    struct msi *msi = (struct msi *)d;
    struct list_head *get_f;
    UVC_MANAGE *uvc_message = NULL;
    UVC_BLANK *uvc_b;
    struct framebuff *fb = NULL;
    struct uvc_msi_s *uvc = (struct uvc_msi_s *)msi->priv;
    // uint8_t *buf;
    uint32_t irq_time = 0;
    uint8_t *malloc_buf = NULL;
    uint32_t current_photo_size = 0;
    uint32_t current_photo_max_size;
    uint8_t exit_err = 0;
    uint32_t usb_time_out = 0;
    uint32_t usb_dma_irq_count = 0;
    uint32_t ctlnum = 0;
    uint32_t offset = 0;
    uint32_t stop_flag = 0;
    uint32_t flags;
    uint32_t err;
    uint8_t uvc_fps = 0;
    uint32_t last_record_time = 0;

    switch ((uvc->uvc_format >> 8) & 0xFF)
    {
        case F_JPG:
            os_printf("F_JPG DETECT\n");
            current_photo_max_size = 80 * 1024;
            ctlnum = 30;
            break;

        case F_H264:
            os_printf("F_H264 DETECT\n");
            current_photo_max_size = 180 * 1024;
            ctlnum = 200;
            break;

        default:
            current_photo_max_size = 80 * 1024;
            ctlnum = 30;
            break;
    }
    
    system_event_usbh_video_new_event(uvc->func->dev_num, SYSEVT_USB_DEVICE_CONNECT);

    while (1)
    {
    uvc_get_psram_again:
        malloc_buf = NULL;
        flags = disable_irq();
        get_f = uvc->func->get_frame(uvc->priv);
        enable_irq(flags);

        // 如果其他任务通知,可以在这里判断,然后退出
        os_event_wait(&uvc->evt, UVC_TASK_STOP, (uint32 *)&stop_flag, OS_EVENT_WMODE_OR, 0);
        if (stop_flag)
        {
            os_printf("%s %d\n",__FUNCTION__,__LINE__);
            goto uvc_get_psram_err_deal;
        }

        if (usb_time_out > 500)
        {
            if (usb_dma_irq_count == *(uvc->p_usb_dma_irq_time))
            {
                os_printf("%s:%d\n", __FUNCTION__, __LINE__);
                exit_err = 1;
                goto uvc_get_psram_err_deal;
            }
            usb_dma_irq_count = *(uvc->p_usb_dma_irq_time);
            usb_time_out = 0;
        }

        if (!get_f)
        {
            os_sleep_ms(1);
            usb_time_out++;
            goto uvc_get_psram_again;
        }
        usb_time_out = 0;
        err = 0;
        offset = 0;
        
        current_photo_size = 0;
        flags = disable_irq();
        uvc_message = list_entry(get_f, UVC_MANAGE, list);
        uvc->func->set_frame_using(uvc_message);
        enable_irq(flags);

        while ((uvc_message->frame_end != UVC_DEVICE_FRAME_ERROR) && ((list_empty(&uvc_message->list) != TRUE) || (uvc_message->frame_end == UVC_DEIVCE_FRAME_NOT_END)))
        {

            if (stop_flag)
            {
                goto uvc_get_psram_err_deal;
            }
            flags = disable_irq();
            uvc_b = list_entry(uvc_message->list.prev, UVC_BLANK, list);
            enable_irq(flags);
            if (uvc_b && uvc_b->busy == 2)
            {
                // 数据头,所以要申请一帧数据
                if (uvc_b->blank_loop == 0)
                {
                    // 获取是否有节点,有节点的话,才可以填充数据
                    fb = fbpool_get(&uvc->tx_pool, 0, uvc->msi);
                    // os_printf("malloc fb:%X\n",fb);
                }
                // 没有帧,那么只能将bank删除,应该是rtsp处理速度不够

                if (fb)
                {
                    // 会申请一个连续的空间有可能空间不够,后续会重新申请,如果不够,则丢帧
                    if (!malloc_buf)
                    {
                        _os_printf("#");
                        malloc_buf = (uint8_t *)STREAM_MALLOC(uvc->malloc_max_size);
                        if(malloc_buf)
                        {
                            sys_dcache_invalid_range((uint32_t*)malloc_buf, uvc->malloc_max_size);
                        }
                        uvc->malloc_count_near++;
                        // os_printf("uvc->malloc_max_size:%d\tmalloc_buf:%X\n",uvc->malloc_max_size,malloc_buf);
                    }
                    if (malloc_buf)
                    {
                        // 判断一下偏移后是否大于当前最大的size,如果是,则需要重新去申请空间了,这种情况申请大于20%,自动调整,则增加10%
                        if (offset + (uvc_b->blank_len-uvc_b->re_space) > uvc->malloc_max_size)
                        {
                            _os_printf("{}\n");
                            uint32_t last_malloc_size = uvc->malloc_max_size;
                            uvc->malloc_max_size = (uint32_t)((offset + uvc_b->blank_len) << 1);

                            uvc->malloc_time = os_jiffies();
                            uvc->malloc_count_near = 0;
                            uint8_t *m_buf = (void *)STREAM_MALLOC(uvc->malloc_max_size);
                            os_printf("uvc->malloc_max_size:%d\t%X\n", uvc->malloc_max_size, m_buf);
                            if (m_buf)
                            {
                                sys_dcache_invalid_range((uint32_t*)m_buf, uvc->malloc_max_size);
                                hw_memcpy_no_cache(m_buf, malloc_buf, offset);
                                STREAM_FREE(malloc_buf);
                                malloc_buf = m_buf;
                            }
                            else
                            {
                                uvc->malloc_max_size = last_malloc_size;
                                if (malloc_buf)
                                {
                                    STREAM_FREE(malloc_buf);
                                    malloc_buf = NULL;
                                }
                            }
                        }
                    }

                    // 申请不到空间,则要去移除这张图片,并且释放data_s,但是要这张图片接收完成才能进行操作
                    if (!malloc_buf)
                    {
                        err = 1;
                        _os_printf("[");
                        if (fb)
                        {
                            msi_delete_fb(NULL, fb);
                            fb = NULL;
                        }
                    }
                    else
                    {
                        // 申请空间
                        sys_dcache_clean_range((uint32_t*)uvc_b->buf_ptr, (uvc_b->blank_len - uvc_b->re_space));
                        hw_memcpy_no_cache(malloc_buf + offset, uvc_b->buf_ptr, (uvc_b->blank_len - uvc_b->re_space));
                        offset += (uvc_b->blank_len - uvc_b->re_space);
                    }
                }
                else
                {
                    _os_printf("=");
                }
                uvc->func->free_node(uvc->priv, (struct list_head *)uvc_b);
                uvc_b = NULL;
                irq_time = 0;
            }
            else
            {
                // 释放cpu
                os_sleep_ms(1);
                irq_time++;
                if (irq_time > 500)
                {
                    _os_printf("&");
                    os_printf("uvc_b:%X\n", uvc_b);
                    if (uvc_b)
                    {
                        os_printf("busy:%d\n", uvc_b->busy);
                    }

                    os_printf("uvc_message:%X\t%d\n", uvc_message, uvc_message->frame_end);
                    if (usb_dma_irq_count == *(uvc->p_usb_dma_irq_time))
                    {
                        exit_err = 1;
                        os_printf("%s usb maybe disconnect\n", __FUNCTION__);
                        goto uvc_get_psram_err_deal;
                    }

                    usb_dma_irq_count = *(uvc->p_usb_dma_irq_time);
                    irq_time = 0;
                }
            }
        }

        if (err || !malloc_buf)
        {
            os_printf("%s %d\n",__FUNCTION__,__LINE__);
            goto uvc_get_psram_err_deal;
        }

        if (uvc_message->frame_end == UVC_DEVICE_FRAME_ERROR || uvc_message->frame_end == UVC_DEIVCE_FRAME_NOT_END)
        {
            // os_printf("%s %d\n",__FUNCTION__,__LINE__);
            goto uvc_get_psram_err_deal;
        }
        else
        {
            // 设置状态,等于发送frame
            if (fb)
            {
                fb->len = uvc_message->frame_len;
                fb->data = (uint8_t *)malloc_buf;

                current_photo_size = uvc_message->frame_len;

                if (current_photo_max_size < current_photo_size)
                {
                    current_photo_max_size = current_photo_size;
                }
                // os_printf("current_photo_size:%d\tcurrent_photo_max_size:%d\n",current_photo_size,current_photo_max_size);
                // os_printf("msi:%X\tname:%s\tfb:%X\n",msi,msi->name,fb);
                fb->mtype = (uvc->uvc_format >> 8) & 0xFF;
                fb->stype = uvc->uvc_format & 0xFF;
                fb->srcID = FRAMEBUFF_SOURCE_USB;
                fb->time = os_jiffies();
                fb->datatag ++;
                // os_printf("fb mtype:%x stype:%x\n",fb->mtype,fb->stype);

                #if UVC_YUV422_MJPEG_FORMAT
                if (fb->mtype == F_JPG) {
                    int res = 0;
                    uint32_t decode_w = 0;
                    uint32_t decode_h = 0;
                    res = parse_jpg(fb->data, fb->len, &decode_w, &decode_h);
                    if (!res)
                    {
                        sys_dcache_invalid_range((uint32_t *)fb->data, fb->len);
                        msi_output_fb(msi, fb, 0);
                    }
                    else
                    {
                        msi_delete_fb(msi, fb);
                    }
                } else {
                    msi_output_fb(msi, fb, 0);
                }
                #else
                    msi_output_fb(msi, fb, 0);
                #endif

                uvc_fps ++;
                if(os_jiffies() - last_record_time >= 1000)
                {
                    os_printf("[%s] current fps:%d\n", msi->name, uvc_fps);
                    uvc_fps = 0;
                    last_record_time = os_jiffies();
                }
                
                fb = NULL;
                malloc_buf = NULL;
            }
            uvc->func->set_frame_idle(uvc_message);
        }

        if (uvc->malloc_count_near < 2)
        {
            os_printf("malloc_max_size:%d\tcurrent_photo_max_size:%d\n", uvc->malloc_max_size, current_photo_max_size);
        }

        // 这里用算法开始对malloc_max_size进行计算,尽量贴近申请的空间大小,在申请超过30次,并且没有调整过后,才考虑将
        // 申请最大内存减少到合适位置,然后重新统计
        if (uvc->malloc_count_near > ctlnum && current_photo_max_size * 1.1 < uvc->malloc_max_size)
        {
            // 重新调整一下
            uvc->malloc_max_size = current_photo_max_size * 1.1;
            current_photo_max_size = 0;
            uvc->malloc_count_near = 0;
        }
        // 重新统计一下
        else if (uvc->malloc_count_near > ctlnum)
        {
            uvc->malloc_count_near = 0;
            current_photo_max_size = 0;
        }
        // 一直要保证图片有10%的冗余度来容纳下一帧可能变化太大的情况,如果这一次调整了,则malloc_count_near清0
        else if (current_photo_max_size * 1.1 > uvc->malloc_max_size)
        {
            uvc->malloc_max_size = current_photo_max_size * 1.15;
            uvc->malloc_count_near = 0;
        }

        // 重新获取图片
        goto uvc_get_psram_again;

        // 异常处理,可能是空间不够之类
    uvc_get_psram_err_deal:
        _os_printf("!");
        uvc->func->del_frame(uvc->priv, get_f);
        uvc->func->set_frame_idle(uvc_message);
        if (fb)
        {
            msi_delete_fb(NULL, fb);
            fb = NULL;
        }
        if (malloc_buf)
        {
            STREAM_FREE(malloc_buf);
            malloc_buf = NULL;
        }
        // 如果有必要,还要看看是不是usb断开,是不是应该退出线程

        // 如果其他任务通知,可以在这里判断,然后退出
        if (stop_flag || exit_err)
        {
            break;
        }
    }

    system_event_usbh_video_new_event(uvc->func->dev_num, SYSEVT_USB_DEVICE_DISCONNECT);

    // 发送一个evt,通知线程需要退出了
    os_event_set(&uvc->evt, UVC_TASK_EXIT, NULL);
    // 代表任务退出,并且将流也退出
    msi_destroy(msi);
    // 标志一下退出标志
    return;
}

static int32_t uvc_host_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    struct uvc_msi_s *uvc = (struct uvc_msi_s *)msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            if(uvc) {
                fbpool_destroy(&uvc->tx_pool);

                os_event_del(&uvc->evt);

                uvc->func->deinit_cb(uvc->priv);

                STREAM_LIBC_FREE(uvc);

                msi->priv = NULL;
            }
        }
        break;

        case MSI_CMD_PRE_DESTROY:
        {
        }
        break;

        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;
            // os_printf("fb:%X\n",fb);
            // os_printf("uvc2:%X\n",uvc);
            if (fb->data)
            {
                STREAM_FREE(fb->data);
                fb->data = NULL;
            }
            #if 0 // 添加了 fb->free 和 fb->free_priv
            fbpool_put(&uvc->tx_pool, fb);
            // 不需要内核去释放fb
            ret = RET_OK + 1;
            #endif
        }
        break;
    }
    return ret;
}

// usb host枚举成功后会执行的这个函数
/**********************************************************************
 * 由于这个是硬件的msi,所以只是绑定发送到中间流,然后让中间流去进行转发,这样
 * 外部就不需要等到usb插入才能使用了
 ***********************************************************************/
extern struct list_head *uvc_device_user_get_frame(void* device);
extern void uvc_device_user_free_blank_node(void* device, struct list_head *del);
extern void uvc_device_user_del_frame(void* device, void *get_f);
extern void uvc_device_user_set_frame_using(void *uvc_msg_frame);
extern void uvc_device_user_set_frame_idle(void *uvc_msg_frame);
extern void* register_uvc_device(uint8_t dev_num, uint8_t blank_num, uint32_t blank_len, void* p_dev);
extern bool unregister_uvc_device(void* device);

/* 一个UVC镜头对应一个结构体 */
struct usbh_video_priv_func uvc_host_stream_1 =
{
    .get_frame = uvc_device_user_get_frame,
    .free_node = uvc_device_user_free_blank_node,
    .del_frame = uvc_device_user_del_frame,
    .set_frame_using = uvc_device_user_set_frame_using,
    .set_frame_idle = uvc_device_user_set_frame_idle,
    .deinit_cb = unregister_uvc_device,
};

struct usbh_video_priv_func uvc_host_stream_2 =
{
    .get_frame = uvc_device_user_get_frame,
    .free_node = uvc_device_user_free_blank_node,
    .del_frame = uvc_device_user_del_frame,    
    .set_frame_using = uvc_device_user_set_frame_using,
    .set_frame_idle = uvc_device_user_set_frame_idle,
    .deinit_cb = unregister_uvc_device,
};

void usbh_video_enum_finish_init(const char *video_dev_name, uint32_t uvc_format ,struct usbh_video_priv_func* func, int* p_usb_dma_irq_time)
{
    struct msi *msi = NULL;
usb_host_enum_finish_init_start:
    msi = msi_new(video_dev_name, 0, NULL);
    // 申请不成功,有两种可能:1、空间不够  2、正在删除,没有办法,需要等待资源释放
    if (!msi)
    {
        return;
    }
    struct uvc_msi_s *uvc = (struct uvc_msi_s *)msi->priv;
    // 没有创建过,则创建新的
    if (!uvc)
    {
        uvc = (struct uvc_msi_s *)STREAM_LIBC_ZALLOC(sizeof(struct uvc_msi_s));
        msi->priv = (void *)uvc;
        uvc->msi = msi;
        msi->action = (msi_action)uvc_host_action;
        os_event_init(&uvc->evt);
        os_printf("uvc1:%X\n", uvc);
        fbpool_init(&uvc->tx_pool, MAX_UVC_APP, NULL, NULL);
        uvc->uvc_format = uvc_format;
        uvc->malloc_max_size = UVC_PSRAM_MALLOC_SIZE_INIT;
        uvc->malloc_count_near = 0;
        uvc->malloc_time = os_jiffies();
        uvc->func = func;
        uvc->p_usb_dma_irq_time = p_usb_dma_irq_time;

        uvc->priv = register_uvc_device(uvc->func->dev_num, 8, 16*1024, uvc->func->p_dev);

        msi->enable = 1;
        // 默认绑定到一个usb的中间流,外部调用就使用ROUTE_USB这个去接收
        msi_add_output(msi, NULL, NULL, ROUTE_USB);
        // 创建任务
        os_task_create(video_dev_name, msi_get_usb_psram_thread, (void *)msi, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2048);
    }
    // 已经被创建了,需要等待资源释放完成后才能重新创建硬件了?或者检测到线程退出了,就将msi->name设置为NULL
    // 因为这个时候代表上一次线程已经完成了,msi会后面自动释放?
    else
    {
        // 发送一个evt,通知线程需要退出了
        os_event_set(&uvc->evt, UVC_TASK_STOP, NULL);
        os_printf("usbh_video_enum_finish_init:uvc wait task exit\n");
        // 等到线程已经退出
        os_event_wait(&uvc->evt, UVC_TASK_EXIT, NULL, OS_EVENT_WMODE_OR, -1);
        os_printf("usbh_video_enum_finish_init:uvc task exit\n");
        // 这里设置name为NULL,是为了msi_new可以立刻申请,因为这个时候上一次的uvc线程已经退出了,所以资源等待后面慢慢释放,不需要管了
        // 前提没有msi绑定uvc这个msi,所以这个一定要注意
        msi->name = NULL;
        msi_destroy(msi);

        // 重新msi_new,然后重新跑流程,理论一定可以申请成功
        goto usb_host_enum_finish_init_start;
    }    
}
