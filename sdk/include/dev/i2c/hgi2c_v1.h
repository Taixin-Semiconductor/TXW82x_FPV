/**
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __HGI2C_V1_H
#define __HGI2C_V1_H

#ifdef __cplusplus
 extern "C" {
#endif

struct hgi2c_v1 {
    struct i2c_device       dev;
    uint32                  hw;
    i2c_irq_hdl             irq_hdl;
    uint32                  irq_data;
    uint32                  irq_num;
    struct os_mutex         i2c_lock;
    struct os_semaphore     i2c_done;
    uint32                  irq_tx_done_en       : 1,
                            irq_rx_done_en       : 1,
                            flag_tx_done         : 1,
                            flag_rx_done         : 1,
                            flag_table_done      : 1,
                            flag_table_config_en : 1,
                            opened               : 1,
                            dsleep               : 1;
#ifdef CONFIG_SLEEP
            struct {
				__IO uint32_t con0;
				__IO uint32_t con1;
				__IO uint32_t timecon;
				__IO uint32_t tdmalen;
				__IO uint32_t rdmalen;
				__IO uint32_t tstadr;
				__IO uint32_t rstadr;
				__IO uint32_t slavesta;
				__IO uint32_t ownadrcon;
				__IO uint32_t timeoutcon;
				__IO uint32_t rxtimeoutcon;
				__IO uint32_t rstadr1;
				__IO uint32_t vsync_tcon;
				__IO uint32_t hsync_tcon;
            }bp_regs;
            uint32               bp_irq_flags;
            i2c_irq_hdl          bp_irq_hdl;
            uint32               bp_irq_data;
            struct os_mutex      bp_suspend_lock;
            struct os_mutex      bp_resume_lock;
#endif

}; 



int32 hgi2c_v1_attach(uint32 dev_id, struct hgi2c_v1 *hgi2c_s);



#ifdef __cplusplus
}
#endif



#endif //__HGI2C_V1_H

/*************************** (C) COPYRIGHT 2018 HUGE-IC ***** END OF FILE *****/
