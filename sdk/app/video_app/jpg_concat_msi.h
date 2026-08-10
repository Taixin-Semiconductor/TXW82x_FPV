#ifndef __JPG_CONCAT_MSI_H
#define __JPG_CONCAT_MSI_H
#include "basic_include.h"

#include "lib/multimedia/msi.h"
#include "lib/video/dvp/jpeg/jpg.h"
typedef int32_t (*extern_fn)(void *arg,struct framebuff *fb);
#ifndef FAST_JPG
struct jpg_V3_msi_s
{
    struct os_work       work;
    char                 msi_name[16];
    struct msi          *msi;
    struct jpg_device   *jpg;
    struct scale_device *scale_dev;
    struct os_msgqueue   msgq;
    struct os_event      evt;
    struct fbpool        pool;
    struct framebuff    *now_fb;      // 当前中断使用的fb的头(需要释放)
    struct framebuff    *use_last_fb; // 最后配置的fb(不需要释放,这个是记录使用)
    struct framebuff    *last_fb;     // 最后配置fb的地址,没有放到链表的,(需要释放)
    struct framebuff    *err_fb;
    uint32_t            *jpg_node_buf; // 节点buf数组空间,不再一次申请(一次申请会导致可能申请不到,这样申请则有可能导致碎片化)
    uint32_t             err;
    uint32_t             set_time;
    uint16_t             w, h;
    uint16_t             jpg_node_len;
    uint8_t              jpg_node_count;
    uint8_t              qt;
    uint8_t            which : 1, running : 1, src_from : 3, scale1_flag : 1, vpp_close_flag : 1, end_flag : 1;
    uint8_t              datatag;
    uint8_t              gen420_type; // 如果是gen420的编码,这里需要配置一下类型,因为gen420来源很多地方,也因为是手动kick的,所以这里可以gen420配置了类型再kick,done的时候配置对应类型
    uint8_t              scale1_type;
    int32_t              dqtable_index;
    int32_t              diff_prev;
    int32_t              diff_sum;
    int32_t              target_len;
};
#else
// 不考虑不带psram的情况
struct jpg_V3_msi_s
{
    struct os_work       work;
    char                 msi_name[16];
    struct msi          *msi;
    struct jpg_device   *jpg;
    struct scale_device *scale_dev;
    struct os_msgqueue   msgq;
    struct os_event      evt;

    void *now_node;
    void *use_last_node;
    void *last_node;

    uint32_t err;
    uint32_t set_time;
    uint16_t w, h;
    uint16_t jpg_node_len;
    uint8_t  jpg_node_count;
    uint8_t  qt;
    uint8_t  which : 1, running : 1, src_from : 3, scale1_flag : 1, vpp_close_flag : 1, end_flag : 1;
    uint8_t  datatag;
    uint8_t  gen420_type; // 如果是gen420的编码,这里需要配置一下类型,因为gen420来源很多地方,也因为是手动kick的,所以这里可以gen420配置了类型再kick,done的时候配置对应类型
    uint8_t  scale1_type;

    uint32_t count;
    int32_t  dqtable_index;
    int32_t  diff_prev;
    int32_t  diff_sum;
    int32_t  target_len;
};
#endif
struct jpg_concat_msi_s
{
    struct os_work work;
    struct msi    *msi;
    struct msi    *jpg_msi; // 硬件jpg0的msi
    struct fbpool  tx_pool;
    uint16_t      *filter_type;
    uint32_t       set_time;
    uint16_t       w, h;
    extern_fn      fn;
    void          *extern_arg;
    uint8_t        jpg_node_count;                                                                           // 支持修改,后续可以代码根据不同情况申请空间
    uint8_t        jpg_running : 1, which_jpg : 1, auto_free : 1, from : 3, scale1_flag : 1, force_node : 1; // which_jpg  0:jpg0  1:jpg1   auto_free: 是否自动停止
    uint8_t        datatag;
    uint8_t        gen420_type;
    uint8_t        scale1_type;
};
struct msi *jpg_concat_msi_init_start(uint32_t jpgid, uint16_t w, uint16_t h, uint16_t *filter_type, uint8_t src_from, uint8_t run);
#endif