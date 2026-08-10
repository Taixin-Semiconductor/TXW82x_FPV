#ifndef _ISP_TUNING_H_
#define _ISP_TUNING_H_

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

typedef struct {
    //AE statistical information
    struct {
        float bv;      
        float exposure_value; 
        uint32_t  exposure_line; 
        uint16_t  analog_gain;
        uint16_t  final_luma_target; 
        uint16_t  ae_luma_avg;    
    } ae_info;
    //AWB statistical information
    struct {
        uint16_t    delta_cb, delta_cr;     
        uint16_t    color_temp;
        uint32_t    awb_back_wp_cnt; 
        uint32_t    awb_front_wp_cnt;
        uint16_t    awb_rgb_mean[3];       
        uint16_t    back_smooth_mean[3];       
        uint16_t    front_smooth_mean[3]; 
        uint16_t    r_gain;
        uint16_t    b_gain;
    } awb_info;

} ISP_AE_AWB_INFO;

typedef struct {
    uint8_t     rw_mode;
    uint8_t     reg_length;
    uint16_t    reg_adrr;
    uint8_t     reg_data;
} TUNING_SENSOR_IIC;


struct isp_tunnning_dev {
    struct os_semaphore     usb_write_sema;
    struct os_semaphore     usb_cmd_sema;
    scatter_data            write_data;
    struct isp_device       *p_isp;
    struct dual_device      *p_dual;
    struct vpp_device       *p_vpp;
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


void isp_tunning_init(uint32 img_w, uint32 img_h);
#ifdef __cplusplus
}
#endif

#endif
