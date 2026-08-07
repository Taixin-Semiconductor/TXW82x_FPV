#ifndef _ISP_PARAM_H_
#define _ISP_PARAM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "osal/string.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "rtthread.h"

enum tunning_img_type {
    TUNNING_IMG_JPEG,
    TUNNING_IMG_H264,
};

enum {
    TUNNING_ERR_CODE_RIGHT = 0,
    TUNNING_ERR_CODE_GET_PARAM_ERR,
    TUNNING_ERR_CODE_CFG_PARAM_ERR,
    TUNNING_ERR_CODE_MALLOC_ERR,
};


#define TUNNING_PACKAGE_INFO_SIZE       (2 + 2 + 4 + 2 + 2 + 4)     // tunning_head + tunning_code + data_size + data_crc + package_crc + line_break
#define TUNNING_PACKET_SIZE             (2*1024)                    // package_data + package_crc(uint16) + package_line_break(uint16)

#define TUNNING_IMG_MSI    "tunning_video_msi"

struct isp_tunnning_dev {
    struct os_semaphore     usb_write_sema;
    struct os_semaphore     usb_cmd_sema;
    scatter_data            write_data;
    struct isp_device       *p_isp;
    struct dual_device      *p_dual;
    struct msi              *video_msi;
    struct msi              *v_msi;
    rt_device_t             device;
    rt_ssize_t              (*write_handle)(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size);
    uint32                  *p_data;
    uint16                  cmd_head;
    uint16                  cmd_num;
    uint16                  cmd_channel;
    uint16                  cmd_size;
    uint16                  tunning_head;
    uint16                  tunning_succ;
    uint16                  tunning_err;
    uint32                  backup_addr;
    uint16                  response[6];
    uint16                  message_head[8];
};
void isp_tunning_init();
#ifdef __cplusplus
}
#endif

#endif
