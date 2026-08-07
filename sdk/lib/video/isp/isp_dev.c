#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "lib/video/dvp/cmos_sensor/csi_V2.h"
#include "devid.h"
#include "hal/gpio.h"
#include "hal/isp.h"
#include "hal/i2c.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "dev/vpp/hgvpp.h"
#include "dev/csi/hgdvp.h"
#include "lib/lcd/lcd.h"
#include "hal/jpeg.h"
#include "hal/gpio.h"
#include "app/app_iic/app_iic.h"
#include "lib/video/isp/isp_dev.h"
#include "lib/video/isp/isp_ircut.h"
#include "osal/event.h"
#include "osal/msgqueue.h"
#include "lib/heap/av_heap.h"

#ifndef ISP_HARDWARE_CLK
#define ISP_HARDWARE_CLK        ISP_MODULE_CLK_320M
#endif

extern uint32 y_gamma_tbl[];
// extern uint32 gamma2p4_tbl[];
extern uint32 gamma_table_addr[3] ;
extern uint32 luma_ca_addr[];
extern IRCUT_INFO ircut_info;
#if ISP_DMA_EN
#define ISP_IRQ_HANDLE_TYPE     ISP_IRQ_FLAG_DMA_DONE
#else
#define ISP_IRQ_HANDLE_TYPE    ISP_IRQ_FLAG_FRM_END
#endif

volatile struct list_head sensor_info_head;
const float ev_offset_lut[] = {0.015625, 0.03125, 0.0625, 0.125, 0.25, 0.5, 1, 2, 4, 8, 16, 64};
const struct isp_awb_mode_param isp_awb_mode_map[] = 
{
    // awb_mode    awb_gain_r    awb_gain_gr   awb_gain_gb    awb_gain_b
    {         0,            0,            256,        256,             0},
    {         1,          480,            256,        256,           430},
    {         2,          530,            256,        256,           380},
    {         3,          551,            256,        256,           360},
};

void sensor_info_init()
{
    INIT_LIST_HEAD((struct list_head *)&sensor_info_head);
}

void sensor_info_add(enum sensor_type type, enum isp_input_dat_src sensor_src, uint32 sensor_config, uint32 iic_id, uint32 opt_cmd)
{
    SENSOR_BASIC_INFO *info = os_malloc(sizeof(SENSOR_BASIC_INFO));
    if (info)
    {
        os_memset(info, 0, sizeof(sizeof(SENSOR_BASIC_INFO)));
        info->sensor_src     = sensor_src;
        info->sensor_type    = type;
        info->sensor_dev_id  = iic_id;
        info->sensor_opt_cmd = opt_cmd;
        info->sensor_config  = sensor_config;
        INIT_LIST_HEAD(&info->list);
	    list_add_tail(&info->list,(struct list_head*)&sensor_info_head); 
    }
}

void sensor_info_destory()
{
    SENSOR_BASIC_INFO *info  = NULL;
    struct list_head  *nhead = NULL;

    while(1)
    {
        if (list_empty((void *)&sensor_info_head))   break;
        nhead = sensor_info_head.next;
        info = list_entry(nhead, SENSOR_BASIC_INFO, list);
        list_del(nhead); 
        os_free(info);
    }
}

void iic_config_handle(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2)
{
    _os_printf("iic");
    struct isp_ae_func_cfg *p_cfg      = (struct isp_ae_func_cfg *)param1;
    if (p_cfg->devid_id != 0 && p_cfg->cmd_len) {	
		wake_up_iic_queue(p_cfg->devid_id,(uint8_t*)&p_cfg->data,p_cfg->cmd_len,2,(uint8_t*)NULL);
        p_cfg->devid_id = 0;
        p_cfg->cmd_len  = 0;
	}   
}

void hgisp_frame_start_handle(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2)
{
    // _os_printf("d");
    // os_printf("%s %d\r\n",__func__,__LINE__);
}

void hgisp_frame_done_handle(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2){
	os_printf("%s %d\r\n",__func__,__LINE__);
}

void hgisp_frame_slow_handle(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2){
	os_printf("%s %d\r\n",__func__,__LINE__);
}

void hgisp_frame_fast_handle(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2){
	os_printf("%s %d\r\n",__func__,__LINE__);
}

void hgisp_motion_detect_handle(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2){
	os_printf("%s %d\r\n",__func__,__LINE__);
}

void hgisp_data_overflow_handle(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2){
	os_printf("%s %d\r\n",__func__,__LINE__);
}

void   *isp_task_hdl;
volatile struct os_event    isp_event;
volatile struct os_msgqueue isp_msg;
int32 isp_task(void *data)
{
    struct isp_device *dev = (struct isp_device *)data;
    struct isp_ae_func_cfg cfg;
    struct isp_sensor_opt  *p_opt = NULL;
    uint32 flag = 0;
    int32 event_ret = 0;
    int32 msg_ret   = 0;
    os_memset(&cfg, 0, sizeof(cfg));
    cfg.data.addr = (uint8 *)av_malloc(ISP_SENSOR_REG_MAX_LEN);
    if (cfg.data.addr == NULL)
    {
        os_printf("isp iic data malloc size %d err\r\n", ISP_SENSOR_REG_MAX_LEN);
        return RET_ERR;
    }
    
    while (1)
    {
        event_ret = os_event_wait((void *)&isp_event, EVENT_ISP_CALC | EVENT_ISP_FPS_OPT | EVENT_ISP_IMG_OPT, &flag, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 100);
        // os_printf("%s %d ret : %d\r\n", __func__, __LINE__, ret);
        if (event_ret)
        {
            continue;
        }

        if (flag & EVENT_ISP_CALC)
        {
            _os_printf("c");
            isp_calculate(dev, &cfg);
            if (cfg.devid_id != 0 && cfg.cmd_len) {	
                wake_up_iic_queue(cfg.devid_id, (uint8_t*)&cfg.data, cfg.cmd_len, 2, (uint8_t*)NULL);
                cfg.devid_id = 0;
                cfg.cmd_len  = 0;
            }   
        }

        if (flag & EVENT_ISP_IMG_OPT)
        {
            p_opt = (struct isp_sensor_opt *)os_msgq_get2((void *)&isp_msg, 100, &msg_ret);
            if (msg_ret == 0)
            {
                while (1)
                {
                    if (iic_devid_finish(p_opt->devid_id))
                    {
                        break;
                    } else {
                        os_sleep_ms(1);
                    }
                    
                }
                wake_up_iic_queue(p_opt->devid_id, (uint8_t*)&p_opt->data, p_opt->cmd_len, 2, (uint8_t*)NULL);
            }
        }

        if (flag & EVENT_ISP_FPS_OPT)
        {
            /* code */
        }
    }
}

extern const uint16_t __used isp_param[];

void isp_cfg_dev(){
	uint8_t ret;
    struct hgisp_sensor_init *sensor_init = NULL;
	struct isp_device *isp_dev;
    SENSOR_BASIC_INFO *info = NULL;
	isp_dev = (struct isp_device *)dev_get(HG_ISP_DEVID);	
    if (isp_dev == NULL)
    {
        os_printf("get isp device err\r\n");
        return;
    }
	os_printf("isp cfg....\r\n");
    sensor_init = (struct hgisp_sensor_init *)isp_sensor_param_load((void *)isp_param);
    if (sensor_init)
    {
        ret = isp_open(isp_dev, ISP_HARDWARE_CLK, ISP_INPUT_FIFO_FULL);
        if (!ret)
        {
            isp_yuv_range(isp_dev, VIDEO_YUV_RANGE_TYPE);
            isp_sensor_param_config(isp_dev, (uint32)sensor_init);
            isp_y_gamma_init(isp_dev  , (uint32)y_gamma_tbl);
            isp_rgb_gamma_init(isp_dev, (uint32)gamma_table_addr[1]);
            // isp_fifo_init(isp_dev, ISP_INPUT_FIFO_FULL);
            isp_awb_mannul_mode_map(isp_dev, (uint32)isp_awb_mode_map);
            isp_ae_ev_offset_lut(isp_dev, (uint32)ev_offset_lut);
            isp_awb_gain_type(isp_dev, AWB_GAIN_TYPE_AWB, SENSOR_TYPE_MASTER);
            isp_awb_gain_type(isp_dev, AWB_GAIN_TYPE_AWB, SENSOR_TYPE_SLAVE0);
			isp_awb_gain_type(isp_dev, AWB_GAIN_TYPE_AWB, SENSOR_TYPE_SLAVE1);
            list_for_each_entry(info, (struct list_head*)&sensor_info_head, list)
            {
                ret = isp_sensor_init(isp_dev, info->sensor_type, info->sensor_src, info->sensor_config);
                if (ret)
                {
                    os_printf(KERN_ERR"config sensor param err!\r\n");
                    isp_close(isp_dev);
                    goto end;
                } else {
                    isp_sensor_iic_devid_init(isp_dev, info->sensor_dev_id, info->sensor_type);
                    isp_sensor_iic_cmd_init(isp_dev, info->sensor_opt_cmd, info->sensor_type);
                }
            }
            ircut_init();
            isp_request_irq(isp_dev, ISP_IRQ_FLAG_DMA_DONE , hgisp_frame_start_handle   , 0);
            // isp_request_irq(isp_dev, ISP_IRQ_FLAG_FRM_END  , hgisp_frame_done_handle    , (uint32)isp_dev);
            isp_request_irq(isp_dev, ISP_IRQ_FLAG_DAT_OF   , hgisp_data_overflow_handle , 0);
            isp_request_irq(isp_dev, ISP_IRQ_FLAG_INTF_SLOW, hgisp_frame_slow_handle    , 0);
            isp_request_irq(isp_dev, ISP_IRQ_FLAG_FRM_FAST , hgisp_frame_fast_handle    , 0);
            isp_request_irq(isp_dev, ISP_IRQ_FLAG_MD_DONE  , hgisp_motion_detect_handle , 0);
            // isp_request_irq(isp_dev, ISP_IRQ_FLAG_IIC_CFG  , iic_config_handle          , 0);
            os_event_init((void *)&isp_event);
            os_msgq_init((void *)&isp_msg,1);
            isp_task_hdl = os_task_create("isp", (void *)isp_task, isp_dev, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2*1024);
        } else {
            os_printf("isp open err");
            goto end;
        }	
    } else {
        os_printf("sensor param init err!\r\n");
    }
end:
    sensor_info_destory();
}

void isp_dev_close()
{
    struct isp_device *isp_dev = (struct isp_device *)dev_get(HG_ISP_DEVID);	
    if (isp_dev == NULL)
    {
        os_printf("get isp device err\r\n");
        return;
    }

    isp_close(isp_dev);
    os_task_destroy(isp_task_hdl);
    os_event_del((void *)&isp_event);
    os_msgq_del((void *)&isp_msg);
}

