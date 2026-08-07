#include "video/uvc/rtt_uvc_host.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"



#ifdef PSRAM_HEAP
#define UVC_BLANK_MALLOC    av_psram_zalloc
#define UVC_BLANK_FREE      av_psram_free
#else
#define UVC_BLANK_MALLOC    av_zalloc
#define UVC_BLANK_FREE      av_free
#endif

static UVC_Device *uvc_devices[MAX_DEVICES] = {NULL}; // 设备池
static int registered_devices = 0;          // 已注册设备数

static void uvc_device_wait_frame(UVC_Device* uvc_device);

/* ------------------------------- 设备缓存节点接口 ------------------------------- */
static void uvc_device_blank_node_init(UVC_Device* uvc_device)
{
    os_printf("%s %d\n", __FUNCTION__, __LINE__);
    if (uvc_device->blank_num == 0) {
        uvc_device->blank_num = UVC_DEFAULT_BLANK_NUM;
    }
    if (uvc_device->blank_len == 0) {
        uvc_device->blank_len = UVC_DEFAULT_BLANK_LEN;
    }

    uvc_device->blank_buffer = UVC_BLANK_MALLOC(sizeof(UVC_BLANK) * uvc_device->blank_num);

    os_printf("blank_buffer : %x\n", uvc_device->blank_buffer);

    if (!uvc_device->blank_buffer) {
        os_printf("blank node head malloc failed\n");
        return;
    }

    INIT_LIST_HEAD(&uvc_device->free_uvc_tab);

    UVC_BLANK *blank = uvc_device->blank_buffer;

    for (int i = 0; i < uvc_device->blank_num; i++) {
        
        // os_printf("blank[%d] : %x\n", i, blank);
        INIT_LIST_HEAD(&blank->list);

        blank->blank_len = uvc_device->blank_len;
        blank->blank_loop = 0;
        blank->busy = UVC_DEVICE_BLANK_STATE_IDLE;
        blank->frame_counter = 0;
        blank->re_space = 0;
        blank->buf_ptr = UVC_BLANK_MALLOC(uvc_device->blank_len);
        if (!blank->buf_ptr) {
            os_printf("blank node[%d] buf malloc failed\n", i);
            return;
        }
        list_add_tail(&blank->list, &uvc_device->free_uvc_tab);
        os_printf("blank node[%d] :%x  ptr:%x\n", i, blank, blank->buf_ptr);
        blank++;
    }
}

static void uvc_device_blank_node_deinit(UVC_Device* uvc_device)
{
    if (!uvc_device->blank_buffer) {
        return;
    }

    uvc_device_wait_frame(uvc_device);

    UVC_BLANK *blank = uvc_device->blank_buffer;
    os_printf("%s %d\n", __FUNCTION__, __LINE__);
    for (int i = 0; i < uvc_device->blank_num; i++) {
        if (blank->buf_ptr) {
            UVC_BLANK_FREE(blank->buf_ptr);
            os_printf("free blank node[%d] buf:%x\n", i, blank->buf_ptr);
            blank->buf_ptr = NULL;
        }
        blank++;
    }
    if(uvc_device->blank_buffer) {
        UVC_BLANK_FREE(uvc_device->blank_buffer);
        os_printf("free blank_buffer:%x\n", uvc_device->blank_buffer);
        uvc_device->blank_buffer = NULL;
    }
}

static struct list_head *uvc_device_get_blank_node(struct list_head *head, struct list_head *del)
{
    // os_printf("%s %d head:%x del:%x\n", __FUNCTION__, __LINE__,head,del);
    if(list_empty(del)) {
        return NULL;
    }
    list_move(del->prev, head);
    return head->next;
}

static bool uvc_device_free_get_blank_node_list(struct list_head *head, struct list_head *free) {
    if (list_empty(head)) {
        return 0;
    }

    list_splice_init(head, free);
    return 1;
}

/* ------------------------------- 设备缓存帧接口 ------------------------------- */
static void uvc_device_frame_init(UVC_Device* uvc_device)
{
    os_printf("%s %d\n", __FUNCTION__, __LINE__);
    for (int i = 0; i < UVC_DEFAULT_FRAME_NUM; i++) {
        INIT_LIST_HEAD(&uvc_device->uvc_msg_frame[i].list);
        uvc_device->uvc_msg_frame[i].state = UVC_DEVICE_FRAME_STATE_IDLE;
        uvc_device->uvc_msg_frame[i].sta = 0;
    }
}

static struct list_head *uvc_device_get_new_frame(UVC_Device* uvc_device, int grab)
{
    
    uint8_t frame_num = 0;
    for (frame_num = 0; frame_num < UVC_DEFAULT_FRAME_NUM; frame_num++) {
        if (uvc_device->uvc_msg_frame[frame_num].state == UVC_DEVICE_FRAME_STATE_IDLE) {
            uvc_device->uvc_msg_frame[frame_num].state = UVC_DEVICE_FRAME_STATE_BUSY;
            uvc_device->uvc_msg_frame[frame_num].frame_end = UVC_DEIVCE_FRAME_NOT_END;
            uvc_device->frame_counter++;
            return &uvc_device->uvc_msg_frame[frame_num].list;
        }
    }

    //是否开启抢占模式,开启抢占后，肯定有frame返回，上一帧没使用的frame肯定usable为2
    if (grab) {
        for (frame_num = 0; frame_num < UVC_DEFAULT_FRAME_NUM; frame_num++) {
            if (uvc_device->uvc_msg_frame[frame_num].state == UVC_DEVICE_FRAME_STATE_AVAILABLE) {
                uvc_device->uvc_msg_frame[frame_num].state = UVC_DEVICE_FRAME_STATE_BUSY;
                uvc_device->uvc_msg_frame[frame_num].frame_end = UVC_DEIVCE_FRAME_NOT_END;
                uvc_device_free_get_blank_node_list(&uvc_device->uvc_msg_frame[frame_num].list, &uvc_device->free_uvc_tab);
                uvc_device->frame_counter++;
                return &uvc_device->uvc_msg_frame[frame_num].list;
            }
        }
    }
    return NULL;
}

static void uvc_device_end_frame(UVC_Device* uvc_device, UVC_MANAGE *frame)
{
    if (uvc_device && frame) {
        if (frame->state != UVC_DEVICE_FRAME_STATE_USING) {
            frame->state = UVC_DEVICE_FRAME_STATE_AVAILABLE;
        }
        frame->frame_end = UVC_DEIVCE_FRAME_END;
        uvc_device->current_frame = NULL;
    }
}

static void uvc_device_free_err_frame(UVC_Device* uvc_device, UVC_MANAGE *frame)
{
    if (uvc_device && frame) {
        // 重置帧状态
        if (frame->state != UVC_DEVICE_FRAME_STATE_USING) {
            uvc_device_free_get_blank_node_list(&frame->list, &uvc_device->free_uvc_tab);
            frame->state = UVC_DEVICE_FRAME_STATE_IDLE;
        }
        frame->frame_end = UVC_DEVICE_FRAME_ERROR;
        uvc_device->current_frame = NULL;
    }    
}

static void uvc_device_wait_frame(UVC_Device* uvc_device)
{
    uint32_t timeout = 0;
    os_printf("%s %d\n", __FUNCTION__, __LINE__);
    if (uvc_device){
        for(int j = 0; j < UVC_DEFAULT_FRAME_NUM; j++) {
            if (uvc_device->uvc_msg_frame[j].state == UVC_DEVICE_FRAME_STATE_USING) {
                os_sleep_ms(1);
                timeout++;
                if (timeout > 500) {
                    timeout = 0;
                    os_printf("%s %d\n", __FUNCTION__, __LINE__);
                    //continue;
                }
            }
        }
    }
}

/* ------------------------------- 设备池管理接口 ------------------------------- */
bool uvc_device_pool_init() {
    if (uvc_devices[0]) {
        os_printf("uvc device pool already init\n");
        return true;
    }

    int32 flags = disable_irq();
    for(int i = 0; i < MAX_DEVICES; i++) {
        uvc_devices[i] = (UVC_Device*)os_zalloc(sizeof(UVC_Device));
        if (!uvc_devices[i]) {
            os_printf("uvc device pool malloc failed\n");
            enable_irq(flags);
            return false;
        }
        uvc_devices[i]->is_register = UVC_DEVICE_REGISTER_IDLE;
    }
    enable_irq(flags);
    return true;
}

void uvc_device_pool_deinit() {

    for (int i = 0; i < MAX_DEVICES; i++) {
        unregister_uvc_device(uvc_devices[i]);
    }

    int32 flags = disable_irq();
    for(int i = 0; i < MAX_DEVICES; i++) {
        if (uvc_devices[i]) {
            os_free(uvc_devices[i]);
            uvc_devices[i] = NULL;
        }
    }
    enable_irq(flags);
}

// 注册新设备（实际应用中需结合设备枚举过程）
void* register_uvc_device(uint8_t dev_num, uint8_t blank_num, uint32_t blank_len, void* p_dev) {
    if (registered_devices >= MAX_DEVICES) return false;
    
    UVC_Device *dev = NULL;
    int32 flags = disable_irq();
    for (int i = 0; i < MAX_DEVICES; i++) {
        os_printf("uvc_devices[%d] = %x is_register:%d\n", i, uvc_devices[i],uvc_devices[i]->is_register);
        if (uvc_devices[i]->is_register == UVC_DEVICE_REGISTER_IDLE) {
            dev = uvc_devices[i];
            registered_devices++;
            os_printf("%s %d\n", __FUNCTION__, __LINE__);
            break;
        }
    }
    enable_irq(flags);
    os_printf("%s %d\n", __FUNCTION__, __LINE__);
    if (dev == NULL) {
        return NULL;
    }
    os_printf("%s %d\n", __FUNCTION__, __LINE__);


    dev->blank_num = blank_num;
    dev->blank_len = blank_len;
    dev->frame_counter = 0;
    dev->dev_num = dev_num + 1;
    dev->last_fid = 0xFF;
    dev->is_register = UVC_DEVICE_REGISTER_AVAILABLE;
    dev->current_frame = NULL;
    dev->filter_count = 10;
    dev->expected_fid = 0xFF;
    dev->initial_fid_detected = 0;
    dev->p_dev = (struct usb_device *)p_dev;
    uvc_device_frame_init(dev);
    uvc_device_blank_node_init(dev);

    return dev;
}

bool unregister_uvc_device(void* device) {

    UVC_Device* dev = (UVC_Device*)device;

    os_printf("%s %d dev:0x%x dev->dev_num:%d\n", __FUNCTION__, __LINE__,dev,dev->dev_num);

    if (!dev) {
        return false;
    }

    dev->dev_num = 0;
    dev->is_register = UVC_DEVICE_REGISTER_BUSY;
    uvc_device_blank_node_deinit(dev);
    dev->is_register = UVC_DEVICE_REGISTER_IDLE;
    registered_devices--;

    return true;
}

// 根据设备号查找设备（返回NULL表示未找到）
static UVC_Device* find_uvc_device(uint8_t dev_num) {
    UVC_Device* ret = NULL;

    for (int i = 0; i < MAX_DEVICES; i++) {
        if ((uvc_devices[i]->dev_num == (dev_num + 1)) && (uvc_devices[i]->is_register == UVC_DEVICE_REGISTER_AVAILABLE)) {
            ret = uvc_devices[i];
            break;
        }
    }

    return ret;
}


/* ------------------------------- 上层应用调用接口 ------------------------------- */
struct list_head *uvc_device_user_get_frame(void* device)
{
    uint8_t frame_num = 0;
    struct list_head* list = NULL;
    UVC_Device* uvc_device = (UVC_Device*)device;

    if (!uvc_device || uvc_device->is_register == UVC_DEVICE_REGISTER_IDLE || uvc_device->is_register == UVC_DEVICE_REGISTER_BUSY) {
        return NULL;
    }

    // os_printf("%s %d\n", __FUNCTION__, __LINE__);

    irq_disable(usb_device_ioctl(uvc_device->p_dev, USB_HOST_GET_DMA_IRQ_VECTOR, (uint32)NULL, (uint32)NULL));

    for (frame_num = 0; frame_num < UVC_DEFAULT_FRAME_NUM; frame_num++) {
        if (uvc_device->uvc_msg_frame[frame_num].state == UVC_DEVICE_FRAME_STATE_AVAILABLE) {
            uvc_device->uvc_msg_frame[frame_num].state = UVC_DEVICE_FRAME_STATE_BUSY;
            list = &uvc_device->uvc_msg_frame[frame_num].list;
            goto __exit;
        }
    }    

#ifdef PSRAM_HEAP
    for (frame_num = 0; frame_num < UVC_DEFAULT_FRAME_NUM; frame_num++) {
        if (uvc_device->uvc_msg_frame[frame_num].state == UVC_DEVICE_FRAME_STATE_BUSY) {
            uvc_device->uvc_msg_frame[frame_num].state = UVC_DEVICE_FRAME_STATE_BUSY;
            list = &uvc_device->uvc_msg_frame[frame_num].list;
            goto __exit;
        }
    }      
#endif

__exit:
    irq_enable(usb_device_ioctl(uvc_device->p_dev, USB_HOST_GET_DMA_IRQ_VECTOR, (uint32)NULL, (uint32)NULL));

    return list;
}

void uvc_device_user_set_frame_using(void *uvc_msg_frame)
{
    UVC_MANAGE *frame = (UVC_MANAGE*)uvc_msg_frame;
    frame->state = UVC_DEVICE_FRAME_STATE_USING;
}

void uvc_device_user_set_frame_idle(void *uvc_msg_frame)
{
    UVC_MANAGE *frame = (UVC_MANAGE*)uvc_msg_frame;
    frame->state = UVC_DEVICE_FRAME_STATE_IDLE;    
    frame->frame_len = 0;
    frame->frame_end = 0;
}

int uvc_device_user_get_frame_blank_node_num(void* device, struct list_head *head)
{
    int count = 0;
    UVC_Device* uvc_device = (UVC_Device*)device;
	struct list_head *first = (struct list_head *)head;

    if (!uvc_device || uvc_device->is_register == UVC_DEVICE_REGISTER_BUSY || uvc_device->is_register == UVC_DEVICE_REGISTER_BUSY) {
        return count;
    }

    irq_disable(usb_device_ioctl(uvc_device->p_dev, USB_HOST_GET_DMA_IRQ_VECTOR, (uint32)NULL, (uint32)NULL));
	while(head->next != first)
	{
		head = head->next;
		count++;
	}
    irq_enable(usb_device_ioctl(uvc_device->p_dev, USB_HOST_GET_DMA_IRQ_VECTOR, (uint32)NULL, (uint32)NULL)); 

	return count;				
}

void uvc_device_user_free_blank_node(void* device, struct list_head *del)
{
    UVC_Device* uvc_device = (UVC_Device*)device;

    if (!uvc_device || uvc_device->is_register == UVC_DEVICE_REGISTER_BUSY || uvc_device->is_register == UVC_DEVICE_REGISTER_BUSY) {
        return ;
    }

    // os_printf("%s %d\n", __FUNCTION__, __LINE__);

    irq_disable(usb_device_ioctl(uvc_device->p_dev, USB_HOST_GET_DMA_IRQ_VECTOR, (uint32)NULL, (uint32)NULL));

    list_move(del, &uvc_device->free_uvc_tab);

    irq_enable(usb_device_ioctl(uvc_device->p_dev, USB_HOST_GET_DMA_IRQ_VECTOR, (uint32)NULL, (uint32)NULL)); 
}

void uvc_device_user_del_frame(void* device, void *get_f)
{
    UVC_Device* uvc_device = (UVC_Device*)device;

    if (!uvc_device || uvc_device->is_register == UVC_DEVICE_REGISTER_BUSY || uvc_device->is_register == UVC_DEVICE_REGISTER_BUSY) {
        return;
    }

    // os_printf("%s %d\n", __FUNCTION__, __LINE__);

    irq_disable(usb_device_ioctl(uvc_device->p_dev, USB_HOST_GET_DMA_IRQ_VECTOR, (uint32)NULL, (uint32)NULL));

    uvc_device_free_get_blank_node_list(get_f, &uvc_device->free_uvc_tab);

    irq_enable(usb_device_ioctl(uvc_device->p_dev, USB_HOST_GET_DMA_IRQ_VECTOR, (uint32)NULL, (uint32)NULL)); 
}

/* ------------------------------- UVC数据包解析状态机 ------------------------------- */
#define UVC_DEVICE_CHECK_HEADER_EOH
#define UVC_DEVICE_BULK_CHECK_MJPEG_HEADER

void process_uvc_payload(uint8_t dev_num, uint8_t ep_type, uint8_t video_format, uint8_t *payload, uint32_t payload_len, bool drop) {
    UVC_Device* dev = find_uvc_device(dev_num);
    if (!dev || !payload || payload_len == 0) {
        return;
    }

    // 强制丢帧处理
    if (drop) {
        //释放当前正在处理的帧（如果存在）
        UVC_MANAGE *frame = dev->current_frame;
        _os_printf("Drop frame %x\n", frame);
        if (frame) {
            // 重置帧状态
            uvc_device_free_err_frame(dev, frame);
            dev->current_frame = NULL;
        }
        goto _exit; // 直接返回，不处理后续数据
    }

    if(dev->filter_count > 0) {
        dev->filter_count--;
        goto _exit; // 直接返回，不处理后续数据
    }

    struct video_payload_header *header = NULL;
    uint8_t header_len = 0;
    uint8_t *data_ptr = NULL;
    uint32_t data_len = 0;
    uint8_t cur_fid = 0;
    bool valid_header = false;

    // 包头有效性验证
    if (payload_len > payload[0]
        && (payload[0] == 0x0C || (payload[0] == 0x02 && !(payload[1] & 0x0C)) || (payload[0] == 0x06 && !(payload[1] & 0x08)))
        && !(payload[1] & 0x30)
#ifdef UVC_DEVICE_CHECK_HEADER_EOH
        && (payload[0] == 0x02 ? payload[1] & 0x80 : 1)
#endif
#ifdef UVC_DEVICE_BULK_CHECK_MJPEG_HEADER
        && ((((ep_type & USB_EP_ATTR_TYPE_MASK) == USB_EP_ATTR_BULK) && (video_format == USBH_VIDEO_FORMAT_MJPEG)) ? (payload_len >= payload[0] + 2 && payload[payload[0]] == 0xFF && payload[payload[0] + 1] == 0xD8) : 1)
#endif
    ) 
    {
        //os_printf("UVC: Valid header detected\n");
        //os_printf("UVC: Payload [0]:%x [1]:%x [payload[0]]:%x [payload[0] + 1]:%x\n", payload[0], payload[1], payload[payload[0]], payload[payload[0] + 1]);
        valid_header = true;

        switch(dev->initial_fid_detected)
        {
            case 0:
            {
                header = (struct video_payload_header *)payload;
                dev->expected_fid = header->headerInfoUnion.headerInfoBitmap.FID;
                // os_printf("UVC: Initial FID detected: %d\n", dev->expected_fid);
                dev->initial_fid_detected = 1;
                return;
            } 
                break;
            case 1:
            {
                header = (struct video_payload_header *)payload;
                cur_fid = header->headerInfoUnion.headerInfoBitmap.FID;
                // os_printf("UVC: Current FID detected: %d\n", cur_fid);
                if(cur_fid != dev->expected_fid){
                    dev->initial_fid_detected = 2;
                }else{
                    return;
                }
            }
                break;
            case 2:
            {

            }
                break;
        }
    }else{
       //os_printf("e UVC: Payload [0]:%x [1]:%x [payload[0]]:%x [payload[0] + 1]:%x\n", payload[0], payload[1], payload[payload[0]], payload[payload[0] + 1]);
    };
    
    //初始化变量
    if (valid_header == true) {
        header = (struct video_payload_header *)payload;
        cur_fid = header->headerInfoUnion.headerInfoBitmap.FID;
        header_len = header->bHeaderLength; 
        data_ptr = payload + header_len;
        data_len = payload_len - header_len;      
        // os_printf("UVC: Header length: %d, Data: %x Data length: %d\n", header_len, data_ptr, data_len); 
    } else {
        if(payload_len == payload[0]
            && (payload[0] == 0x0C || (payload[0] == 0x02 && !(payload[1] & 0x0C)) || (payload[0] == 0x06 && !(payload[1] & 0x08)))) {
                cur_fid = dev->last_fid; 
                data_len = 0;
                valid_header = true;
                header = (struct video_payload_header *)payload;
                header_len = header->bHeaderLength; 
                //os_printf("valid_header true\n");
        }else{
            cur_fid = dev->last_fid; 
            data_ptr = payload;
            data_len = payload_len; 
            // os_printf("UVC: Header length: %d, Data: %x Data length: %d\n", header_len, data_ptr, data_len);
        }
    };

    // 获取当前帧管理结构
    UVC_MANAGE *frame = dev->current_frame;
    //os_printf("UVC: Current frame: %x\n", frame);

    // 检查是否为帧起始
    if (valid_header == true) {

        // 处理错误标志
        if (header->headerInfoUnion.headerInfoBitmap.ERR) {
            uvc_device_free_err_frame(dev, frame); // 异常结束
            dev->current_blank = NULL;
            dev->current_frame = NULL;
            goto _exit;
        }

        // FID变化处理（仅当有有效包头时）
        if ((frame ? (dev->last_fid != cur_fid) : 0)) {
            if(((ep_type & USB_EP_ATTR_TYPE_MASK) == USB_EP_ATTR_ISOC)) 
            {
                os_printf("[UVC] FID changed %d->%d payload[0]:0x%x payload[1]:0x%x\n", dev->last_fid, cur_fid, payload[0], payload[1]);
                uvc_device_free_err_frame(dev, frame);//结束
                dev->current_blank = NULL;
                dev->current_frame = NULL;
                goto _exit;
            }
        }

        if(frame == NULL && data_len > 0) {
            // 获取新帧
            struct list_head *new_frame_list = uvc_device_get_new_frame(dev, 1);
            if (!new_frame_list) {
                // 抢占模式获取可用帧
                _os_printf("_D1_");
                goto _exit; // 无法获取帧，丢弃数据
            }    

            frame = list_entry(new_frame_list, UVC_MANAGE, list);
            // os_printf("uvc_device_get_new_frame frame:%x\n", frame);

            if (&dev->uvc_msg_frame[0] != frame && &dev->uvc_msg_frame[1] != frame) {
                os_printf("ERR UVC: Frame %x\n", frame);
                while(1);
            } else {

            }

            dev->current_frame = frame;
            dev->current_blank = NULL;
            dev->last_fid = cur_fid;
            
            // 初始化帧信息
            frame->frame_counter = dev->frame_counter;;
            frame->frame_len = 0;
            frame->data_len = 0;
            frame->cpbuff = NULL;
        } else {

        }

    } else {

    }

    //由于存在有效包头的判断，假如一直没有识别到有效包头，则不会获取到帧，不执行拷贝动作
    if (frame) {
        // 处理数据填充
        uint32_t _data_len = data_len;
        while (_data_len > 0) {
            UVC_BLANK *blank = NULL;
            uint8_t blank_loop = 0;

            if(dev->current_blank) {
                blank = dev->current_blank;
                if (blank->re_space == 0) {
                    // 当前节点已满，强制分配新节点
                    blank->busy = UVC_DEVICE_BLANK_STATE_AVAILABLE;
                    struct list_head *new_node = uvc_device_get_blank_node(&frame->list, &dev->free_uvc_tab);
                    if (!new_node) {
                        _os_printf("_D2_");
                        uvc_device_free_err_frame(dev, frame);
                        dev->current_blank = NULL;
                        dev->current_frame = NULL;
                        // os_printf("Error: Failed to allocate new blank node!\n");
                        goto _exit;
                    }
                    blank_loop = blank->blank_loop;
                    blank = list_entry(new_node, UVC_BLANK, list);
                    dev->current_blank = blank;
                    blank->busy = UVC_DEVICE_BLANK_STATE_BUSY;
                    blank->blank_loop = blank_loop + 1;
                    blank->frame_counter = frame->frame_counter;
                    blank->re_space = blank->blank_len;
                    frame->cpbuff = blank->buf_ptr; // 重置写入指针到新节点
                    // os_printf("new blank:%x %x blank->blank_loop:%d\r\n", blank, blank->buf_ptr, blank->blank_loop);
                }
            } else {
                struct list_head *blank_node = uvc_device_get_blank_node(&frame->list, &dev->free_uvc_tab);
                if (!blank_node) {
                    _os_printf("_D3_");
                    uvc_device_free_err_frame(dev, frame);// 异常结束
                    dev->current_blank = NULL;
                    dev->current_frame = NULL;
                    // os_printf("Error: No free blank nodes available!\n");
                    goto _exit;
                }
                blank = list_entry(blank_node, UVC_BLANK, list);
                dev->current_blank = blank;
                blank->busy = UVC_DEVICE_BLANK_STATE_BUSY;
                blank->blank_loop = blank_loop;
                blank->frame_counter = frame->frame_counter;
                blank->re_space = blank->blank_len;
                frame->cpbuff = blank->buf_ptr; // 初始化写入指针
                // os_printf("blank:%x %x blank->blank_loop:%d\r\n", blank, blank->buf_ptr, blank->blank_loop);
            }

            if(frame->cpbuff == NULL) {
                os_printf("Error: cpbuff is NULL!\n");
                while(1);
            }

            if(blank) {
                // 计算实际可拷贝长度
                uint32_t copy_len = MIN(_data_len, blank->re_space);
                if (copy_len > 0) {
                    // os_printf("copy_len:%d %x %x\r\n", copy_len, data_ptr, frame->cpbuff);
                    os_memcpy(frame->cpbuff, data_ptr, copy_len);
                    data_ptr += copy_len;
                    _data_len -= copy_len;
                    frame->cpbuff += copy_len;  // 更新当前写入位置
                    frame->frame_len += copy_len;
                    blank->re_space -= copy_len;
                    // 标记节点为可用（若已填满）
                    blank->busy = (blank->re_space > 0) ? UVC_DEVICE_BLANK_STATE_BUSY : UVC_DEVICE_BLANK_STATE_AVAILABLE;
                } else {
                    // 数据长度为0或异常情况，退出循环
                    break;
                }
            }else{
                os_printf("Error: blank is NULL!\n");
                break;
            }
        }

    } else {
        //未能获取到frame
    }
#ifdef UVC_DEVICE_BULK_CHECK_MJPEG_HEADER    
    if(((ep_type & USB_EP_ATTR_TYPE_MASK) == USB_EP_ATTR_BULK) && (video_format == USBH_VIDEO_FORMAT_MJPEG)) 
    {
        if ((data_len >= 2) && (payload[data_len - 2] == 0xFF && payload[data_len - 1] == 0xD9) && dev->current_frame)
        {
            // os_printf("[UVC] MJPEG Frame end\n");
            uvc_device_end_frame(dev, frame);
            if (dev->current_blank) {
                dev->current_blank->busy = UVC_DEVICE_BLANK_STATE_AVAILABLE;
            }
            dev->current_blank = NULL; // 重置当前节点
            dev->current_frame = NULL; // 重置当前帧
        }
    }
    else
#endif
    {
        {
            // 检查帧结束标志
            if (valid_header == true && header->headerInfoUnion.headerInfoBitmap.EOI && dev->current_frame) {
                // os_printf("[UVC] Frame end\n");
                uvc_device_end_frame(dev, frame);
                if (dev->current_blank) {
                    dev->current_blank->busy = UVC_DEVICE_BLANK_STATE_AVAILABLE;
                }
                dev->current_blank = NULL; // 重置当前节点
                dev->current_frame = NULL; // 重置当前帧
            }
        }
    }

_exit:
    return;
}

