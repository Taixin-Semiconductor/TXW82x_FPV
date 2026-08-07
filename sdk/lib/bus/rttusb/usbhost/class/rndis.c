#include <rtthread.h>
#include <include/rttusb_host.h>
#include "rndis.h"
#include "cdc.h"
#include "hal/usb_device.h"
#include "lib/usb/usb_device_rndis.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#include "lwip/tcpip.h"
#include "netif/ethernetif.h"

#if defined(RT_USBH_WIRELESS) && defined(RT_USBH_WIRELESS_RNDIS)

static rt_err_t rt_rndis_msg_send_recv(struct usb_rndis *rndis, rt_uint8_t *send_buf, rt_uint32_t send_len,
            rt_uint8_t *recv_buf, rt_uint32_t recv_size, rt_uint32_t *recv_len)
{
    uinst_t device = rndis->device;
    int ret = 0;
    rt_uint32_t rndis_avial[2] = {0};
    
    ret = rt_usbh_cdc_send_command(device, send_buf, send_len);
    if (ret == send_len) {
        /* waite for the interrupt ep */
        ret = rt_usb_hcd_pipe_xfer(device->hcd, rndis->pipe_int, rndis_avial, sizeof(rndis_avial), USB_TIMEOUT_BASIC);
        if (ret == 8 && rndis_avial[0] == 1 && rndis_avial[1] == 0) {
            ret = rt_usbh_cdc_get_response(device, recv_buf, recv_size);
            if (ret > 0) {
                *recv_len = ret;
                return RET_OK;
            }
        }
        ret = RET_ERR;
    }
    return ret;
}

static rt_err_t rt_rndis_msg_init(struct usb_rndis *rndis)
{
    USBD_CDC_RNDIS_MsgTypeDef *msg = (USBD_CDC_RNDIS_MsgTypeDef *)rndis->msg_buffer;
    rt_uint32_t recv_len = 0;
    int ret = 0;

    // 清理buffer空间
    os_memset(rndis->msg_buffer, 0, sizeof(USBD_CDC_RNDIS_InitMsgTypeDef));
    msg->Init.MsgType = CDC_RNDIS_INITIALIZE_MSG_ID;
    msg->Init.MsgLength = sizeof(USBD_CDC_RNDIS_InitMsgTypeDef);
    msg->Init.ReqId = ++rndis->req_id;
    msg->Init.MajorVersion = CDC_RNDIS_VERSION_MAJOR;
    msg->Init.MinorVersion = CDC_RNDIS_VERSION_MINOR;
    msg->Init.MaxTransferSize = 2048;
    ret = rt_rndis_msg_send_recv(rndis, rndis->msg_buffer, msg->Init.MsgLength,
                    rndis->msg_buffer, 128, &recv_len);
    if (ret == RET_OK && recv_len == sizeof(USBD_CDC_RNDIS_InitCpltMsgTypeDef) &&
            msg->InitCplt.MsgType == CDC_RNDIS_INITIALIZE_CMPLT_ID &&
            msg->InitCplt.ReqId == rndis->req_id &&
            msg->InitCplt.Status == CDC_RNDIS_STATUS_SUCCESS) {
        return RET_OK;
    }
    return RET_ERR;
}

static rt_err_t rt_rndis_msg_keepalive(struct usb_rndis *rndis)
{
    USBD_CDC_RNDIS_MsgTypeDef *msg = (USBD_CDC_RNDIS_MsgTypeDef *)rndis->msg_buffer;
    rt_uint32_t recv_len = 0;
    int ret = 0;

    os_memset(rndis->msg_buffer, 0, sizeof(USBD_CDC_RNDIS_KpAliveMsgTypeDef));
    msg->KpAlive.MsgType = CDC_RNDIS_KEEPALIVE_MSG_ID;
    msg->KpAlive.MsgLength = sizeof(USBD_CDC_RNDIS_KpAliveMsgTypeDef);
    msg->KpAlive.ReqId = ++rndis->req_id;
    ret = rt_rndis_msg_send_recv(rndis, rndis->msg_buffer, msg->KpAlive.MsgLength,
                    rndis->msg_buffer, 128, &recv_len);
    if (ret == RET_OK && recv_len == sizeof(USBD_CDC_RNDIS_KpAliveCpltMsgTypeDef) &&
            msg->KpAliveCplt.MsgType == CDC_RNDIS_KEEPALIVE_CMPLT_ID &&
            msg->KpAliveCplt.ReqId == rndis->req_id &&
            msg->KpAliveCplt.Status == CDC_RNDIS_STATUS_SUCCESS) {
        return RET_OK;
    }
    return RET_ERR;
}

rt_err_t rt_rndis_msg_query(struct usb_rndis *rndis, rt_uint32_t oid, rt_uint8_t *buff, rt_uint32_t *len)
{
    USBD_CDC_RNDIS_MsgTypeDef *msg = (USBD_CDC_RNDIS_MsgTypeDef *)rndis->msg_buffer;
    rt_uint32_t recv_len = 0;
    int ret = 0;

    os_memset(rndis->msg_buffer, 0, sizeof(USBD_CDC_RNDIS_QueryMsgTypeDef));
    msg->Query.MsgType = CDC_RNDIS_QUERY_MSG_ID;
    msg->Query.MsgLength = sizeof(USBD_CDC_RNDIS_QueryMsgTypeDef);
    msg->Query.RequestId = ++rndis->req_id;
    msg->Query.Oid = oid;
    msg->Query.InfoBufLength = 0;
    msg->Query.InfoBufOffset = 20;
    msg->Query.DeviceVcHandle = 0;
    ret = rt_rndis_msg_send_recv(rndis, rndis->msg_buffer, msg->Query.MsgLength,
                    rndis->msg_buffer, 128, &recv_len);
    if (ret == RET_OK && recv_len >= sizeof(USBD_CDC_RNDIS_QueryCpltMsgTypeDef) &&
            msg->QueryCplt.MsgType == CDC_RNDIS_QUERY_CMPLT_ID &&
            msg->QueryCplt.ReqId == rndis->req_id &&
            msg->QueryCplt.Status == CDC_RNDIS_STATUS_SUCCESS) {
        os_memcpy(buff, msg->QueryCplt.InfoBuf, msg->QueryCplt.InfoBufLength);
        *len = msg->QueryCplt.InfoBufLength;
        return RET_OK;
    }
    return RET_ERR;
}

static void rt_usbh_rndis_keepalive_timer(void *args)
{
    struct usb_rndis *rndis = args;

    // os_printf("keep alive\r\n");
    rt_rndis_msg_keepalive(rndis);
    os_timer_start(&rndis->keepalive_timer, 5000);
}

static rt_err_t rt_usbh_rndis_open(struct netdev *ndev, netdev_input_cb input_cb, netdev_event_cb evt_cb, void *priv)
{
    rt_uint32_t flags;
    struct usb_rndis *rndis = container_of(ndev, struct usb_rndis, ndev);
    
    flags = disable_irq();
    rndis->input_cb   = input_cb;
    rndis->input_priv = priv;
    enable_irq(flags);

    return RET_OK;
}

// 注意：send_data直接将data指针前移获取预留的空间
// 必须确认调用方是skb申请有预留足够空间，否则存在风险
// 由于USB DMA需要4字节对齐，skb->data经过处理后，IP包头不是4字节对齐，不能前移处理
static rt_err_t rt_usbh_rndis_send_data(struct netdev *ndev, rt_uint8_t *p_data, rt_uint32_t size)
{
    struct usb_rndis *rndis = container_of(ndev, struct usb_rndis, ndev);
    uinst_t device = rndis->device;
    USBD_CDC_RNDIS_PacketMsgTypeDef *packet = NULL;

    if (p_data == NULL || size == 0) {
        return -EINVAL;
    }
    if (!rndis->ready) {
        // usb没准备好就不发包
        os_printf("rndis send no ready\r\n");
        return -EIO;
    }
    packet = (USBD_CDC_RNDIS_PacketMsgTypeDef *)os_malloc(size + sizeof(USBD_CDC_RNDIS_PacketMsgTypeDef));
    if (packet == NULL) {
        return -ENOMEM;
    }
    os_memset(packet, 0, sizeof(USBD_CDC_RNDIS_PacketMsgTypeDef));
    packet->MsgType = CDC_RNDIS_PACKET_MSG_ID;
    packet->MsgLength = size + 44;
    packet->DataOffset = 36; // 明明是44，但是抓包看全是36，不知道为什么
    packet->DataLength = size;
    hw_memcpy(packet + 1, p_data, size);
    rt_usb_hcd_pipe_xfer(device->hcd, rndis->pipe_out, packet, packet->MsgLength, 0);
    os_free(packet);
    return RET_OK;
}

// 注意：USB驱动大部分不支持scatter发送，因此需要额外拷贝
// 调用方多数来自于LWIP，数据量可能小一点
static rt_err_t rt_usbh_rndis_send_scatter_data(struct netdev *ndev, scatter_data *data, rt_uint32_t count)
{
    struct usb_rndis *rndis = container_of(ndev, struct usb_rndis, ndev);
    uinst_t device = rndis->device;
    USBD_CDC_RNDIS_PacketMsgTypeDef *packet = NULL;
    rt_uint8_t *p_data;
    rt_uint32_t size = 0;
    rt_uint32_t offset = 0;
    rt_uint32_t i;

    if (data == NULL || count == 0) {
        return -EINVAL;
    }
    if (!rndis->ready) {
        // usb没准备好就不发包
        os_printf("rndis scatter send no ready\r\n");
        return -EIO;
    }
    for (i = 0; i < count; ++i) size += data[i].size;
    packet = (USBD_CDC_RNDIS_PacketMsgTypeDef *)os_malloc(size + sizeof(USBD_CDC_RNDIS_PacketMsgTypeDef));
    if (packet == NULL) {
        return -ENOMEM;
    }
    os_memset(packet, 0, sizeof(USBD_CDC_RNDIS_PacketMsgTypeDef));
    packet->MsgType = CDC_RNDIS_PACKET_MSG_ID;
    packet->MsgLength = size + 44;
    packet->DataOffset = 36; // 明明是44，但是抓包看全是36，不知道为什么
    packet->DataLength = size;
    // 拷贝scatter中数据
    p_data = (rt_uint8_t *)(packet + 1);
    for (i = 0; i < count; ++i) {
        hw_memcpy(p_data + offset, data[i].addr, data[i].size);
        offset += data[i].size;
    }
    rt_usb_hcd_pipe_xfer(device->hcd, rndis->pipe_out, packet, packet->MsgLength, 0);
    os_free(packet);
    return RET_OK;
}

void rt_usbh_rndis_network_init(struct usb_rndis *rndis)
{
    struct netdev *ndev = &rndis->ndev;
    if (ndev) {
        // tcpip_init(NULL, NULL); // 外面wifi应该初始化过了
        lwip_netif_add(ndev, "l0", NULL, NULL, NULL);
        // 利用事件驱动
        os_printf("add l0 interface!\r\n");
    }
}

static const struct netdev_hal_ops rndis_ops = {
    .open      = rt_usbh_rndis_open,
    .close     = NULL,
    .ioctl     = NULL,
    .send_data = rt_usbh_rndis_send_data,
    .send_scatter_data = rt_usbh_rndis_send_scatter_data,
};

rt_err_t rt_usbh_rndis_attach(struct usb_rndis *rndis)
{
    if (rndis == NULL) {
        return -EIO;
    }
    // 绑定处理函数
    rndis->ndev.dev.ops = (const struct devobj_ops *)&rndis_ops;
    // 顺便注册devid
    return dev_register(HG_LTE_RNDIS_DEVID, (struct dev_obj *)rndis);
}

rt_err_t rt_usbh_rndis_attach2(void)
{
    struct usb_rndis *rndis = (struct usb_rndis *)os_zalloc(sizeof(struct usb_rndis));
    
    ASSERT(rndis);
    rndis->ndev.dev.ops = (const struct devobj_ops *)&rndis_ops;
    return dev_register(HG_LTE_RNDIS_DEVID, (struct dev_obj *)rndis);
}

void rndis_data_recv(void *context)
{
    struct usb_rndis *rndis = context;
    uinst_t device = rndis->device;
    USBD_CDC_RNDIS_PacketMsgTypeDef *packet = NULL;
    rt_uint32_t flags;
    int recv_size;
    int offset;
    int shift;
    netdev_input_cb input_cb;
    void *input_priv;
    rt_uint32_t target_copyLength = 0; 

    while (1) {
        if (!rndis->ready) {
            os_sleep_ms(10); // 需要有点延迟给线程，不然占住不释放
            continue;
        }
        offset = 0;
        recv_size = rt_usb_hcd_pipe_xfer(device->hcd, rndis->pipe_in, rndis->data_buffer, 2048, 0);
        while (recv_size > 0) {
            shift = offset % 4;
            if (shift != 0) {
                os_memmove(rndis->data_buffer + offset - shift , rndis->data_buffer + offset, recv_size);
                offset -= shift;
            }
            /*上一个usb包余下的新rndis包数据*/
            if (rndis->ts_saveLength) {
                /*暂存的rndis包里有存储到该包的包头信息*/
                if (rndis->ts_saveLength < sizeof(USBD_CDC_RNDIS_PacketMsgTypeDef)) {
                    /*先拷贝rndis包头大小的长度，确保能读取到该rndis包的长度信息*/
                    os_memcpy(rndis->ts_buffer + rndis->ts_saveLength,
                        rndis->data_buffer + offset,
                        MIN(recv_size, sizeof(USBD_CDC_RNDIS_PacketMsgTypeDef)));
                    rndis->ts_saveLength += MIN(recv_size, sizeof(USBD_CDC_RNDIS_PacketMsgTypeDef));
                    offset += MIN(recv_size, sizeof(USBD_CDC_RNDIS_PacketMsgTypeDef));
                    recv_size -= MIN(recv_size, sizeof(USBD_CDC_RNDIS_PacketMsgTypeDef));
                }
                packet = (USBD_CDC_RNDIS_PacketMsgTypeDef *)(rndis->ts_buffer);
            } else {
                packet = (USBD_CDC_RNDIS_PacketMsgTypeDef *)(rndis->data_buffer + offset);
                /*没有余留的rndis包，包类型也不对，则丢掉该包，可能丢掉一整个聚合的包，正常不应该出现这种情况*/
                if ((recv_size >= sizeof(uint32_t)) && (packet->MsgType != CDC_RNDIS_PACKET_MSG_ID)) {
                    os_printf("WRONG RNDIS ID!\r\n");
                    break;
                }
                /*包头对齐且长度大于一个完整包，则不用拷贝到暂存buf，直接传进lwip*/
                if ((recv_size >= sizeof(USBD_CDC_RNDIS_PacketMsgTypeDef)) && (recv_size >= packet->MsgLength)) {
                    flags = disable_irq();
                    input_cb   = rndis->input_cb;
                    input_priv = rndis->input_priv;
                    enable_irq(flags);
                    if (input_cb) {
                        input_cb(&rndis->ndev,
                            (rt_uint8_t *)packet + packet->DataOffset + 8,
                            packet->DataLength,
                            input_priv);
                    }
                    offset += packet->MsgLength;
                    recv_size -= packet->MsgLength;
                    continue;                   
                }
            }
            if (recv_size > (2 * sizeof(uint32_t))) {
                target_copyLength = (packet->MsgLength - rndis->ts_saveLength);
            } else {
                target_copyLength = recv_size;
            }
            if (recv_size) {
                os_memcpy(rndis->ts_buffer + rndis->ts_saveLength,
                    rndis->data_buffer + offset,
                    MIN(recv_size, target_copyLength));
            }
            offset += MIN(recv_size, target_copyLength);
            rndis->ts_saveLength += MIN(recv_size, target_copyLength);
            recv_size -= MIN(recv_size, target_copyLength);
            packet = (USBD_CDC_RNDIS_PacketMsgTypeDef *)(rndis->ts_buffer);

            if ((packet->MsgType == CDC_RNDIS_PACKET_MSG_ID) && (rndis->ts_saveLength >= packet->MsgLength)) {
                flags = disable_irq();
                input_cb   = rndis->input_cb;
                input_priv = rndis->input_priv;
                enable_irq(flags);
                if (input_cb) {
                    input_cb(&rndis->ndev,
                        (rt_uint8_t *)packet + packet->DataOffset + 8,
                        packet->DataLength,
                        input_priv);
                }
                rndis->ts_saveLength -= packet->MsgLength;
            }
        }
        if (recv_size < 0) {
            // 返回负数大概是usb断线了，不再循环读数，准备释放线程了
            rndis->ready = 0;
        }
    }
}

rt_err_t rt_usbh_rndis_run(struct usb_rndis *rndis)
{
    if (rndis == NULL) {
        return RET_ERR;
    }
    os_printf("rndis\r\n");
#ifndef STATIC_RNDIS_NETDEV
    rt_usbh_rndis_attach(rndis);
    rt_usbh_rndis_network_init(rndis);
#endif

    rt_rndis_msg_init(rndis);
    rndis->ready = 1;

    os_timer_init(&rndis->keepalive_timer, rt_usbh_rndis_keepalive_timer,
                    OS_TIMER_MODE_ONCE, rndis);
    os_timer_start(&rndis->keepalive_timer, 5000);
    rt_thread_init(&rndis->recv_task, "rndis_recv", rndis_data_recv, rndis,
                    NULL, 512, OS_TASK_PRIORITY_HIGH-1, 20);
    rt_thread_startup(&rndis->recv_task);

    return RET_OK;
}

rt_err_t rt_usbh_rndis_stop(struct usb_rndis *rndis)
{
    rt_uint32_t flags;

    if (rndis == NULL) {
        return RET_ERR;
    }
    
    rndis->ready = 0;
    flags = disable_irq();
    rndis->input_cb   = NULL;
    rndis->input_priv = NULL;
    enable_irq(flags);
#ifndef STATIC_RNDIS_NETDEV
    dev_unregister((struct dev_obj *)rndis);
    lwip_netif_remove(&rndis->ndev);
#endif
    os_timer_stop(&rndis->keepalive_timer);
    os_timer_del(&rndis->keepalive_timer);
    rt_thread_detach(&rndis->recv_task);
    os_free(rndis->msg_buffer);
    os_free(rndis->data_buffer);
    os_free(rndis->ts_buffer);
#ifndef STATIC_RNDIS_NETDEV
    os_free(rndis);
#endif
    
    return RET_OK;
}

#endif