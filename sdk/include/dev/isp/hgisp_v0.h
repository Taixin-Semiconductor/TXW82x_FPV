#ifndef _HGISP_V0_H_
#define _HGISP_V0_H_
#include "hal/isp.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#ifdef __cplusplus
extern "C" {
#endif

enum isp_data_type {
    ISP_DAT_GAMMA_R   = 0,
    ISP_DAT_GAMMA_G,
    ISP_DAT_GAMMA_B,
    ISP_DAT_GAMMA_Y,
    ISP_DAT_HIST_R    = BIT(2),
    ISP_DAT_HIST_G,
    ISP_DAT_HIST_B,
    ISP_DAT_HIST_Y,
    ISP_DAT_LSC       = BIT(3),
    ISP_DAT_GTMO      = BIT(4),    
    ISP_DAT_HDR_HIST0 = BIT(5),
    ISP_DAT_HDR_HIST1 = BIT(6),
    ISP_DAT_INFRA     = BIT(7),
};

enum isp_data_size{
    ISP_DAT_SINGLE_GAMMA_SIZE = 0x100,
    ISP_DAT_HIST_SIZE      = 0x400,
    ISP_DAT_GAMMA_SIZE     = 0x400,
    ISP_DAT_LSC_SIZE       = 0x990,
    // ISP_DAT_GTMO_SIZE      = 0x304,
    // ISP_DAT_HDR_HIST0_SIZE = 0x2E0,
    // ISP_DAT_HDR_HIST1_SIZE = 0x2E0,
    ISP_DAT_GTMO_SIZE      = 0x000,
    ISP_DAT_HDR_HIST0_SIZE = 0x000,
    ISP_DAT_HDR_HIST1_SIZE = 0x000,
}; 

enum{
    LL_ISP_ENABLE_FUNC_GAMMA     = BIT(0),
    LL_ISP_ENABLE_FUNC_HIST      = BIT(1),
    LL_ISP_ENABLE_FUNC_LSC       = BIT(2),
    LL_ISP_ENABLE_FUNC_GTMO      = BIT(3),    
    LL_ISP_ENABLE_FUNC_HDR_HIST0 = BIT(4),
    LL_ISP_ENABLE_FUNC_HDR_HIST1 = BIT(5),
};

enum {
    ISP_LUMA_CA_MODE_0,
    ISP_LUMA_CA_MODE_IRON,
    ISP_LUMA_CA_MODE_GLOWBOW,
    ISP_LUMA_CA_MODE_RAINBOW,
};

/** @brief ISP operating handle structure
  * @{
  */
struct hgisp_v0 {
    struct isp_device   dev         ;
    uint32              hw          ;
    uint32              data_hw     ;
    isp_irq_hdl         irq_hdl     ;
    uint32              irq_data    ;
    uint32              slave_index : 16,
                        curr_index  : 16 ;
    uint32              irq_num     : 16,
                        opened      : 1,
                        dsleep      : 1,
                        dma_en      : 1,
                        reserved    : 12;
    uint16              i2c_devid[4];
    uint16              i2c_opt[4];
    uint32              module;
};

int32 hgisp_v0_attach(uint32 dev_id, struct hgisp_v0 *isp);
void image_isp_status();
#ifdef __cplusplus
}
#endif

#endif /* _HGISP_V0_H_ */
