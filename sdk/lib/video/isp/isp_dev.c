#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "lib/video/dvp/cmos_sensor/csi_V2.h"
#include "devid.h"
#include "hal/gpio.h"
#include "hal/isp.h"
#include "hal/i2c.h"
#include "hal/pwm.h"
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
#include "lib/video/vpp/vpp_dev.h"
#include "lib/video/dvp/cmos_sensor/csi.h"

#ifndef ISP_HARDWARE_CLK
#define ISP_HARDWARE_CLK        ISP_MODULE_CLK_320M
#endif

extern _Sensor_YGAMMA y_gamma_tbl[];
// extern uint32 gamma2p4_tbl[];
extern uint32 gamma_table_addr[3] ;
extern uint32 luma_ca_addr[];
extern IRCUT_INFO ircut_info;

void   *isp_task_hdl = NULL;
volatile struct os_event    isp_event;
volatile struct os_msgqueue isp_msg;

void   *set_fps_task_hdl = NULL;
volatile struct os_msgqueue set_fps_msg;
volatile struct os_event    set_fps_event;

static uint32_t fsync_cnt       = 0;
static uint8_t  fsync_pending   = 0;

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


void hgisp_frame_start_handle(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2) {
    //os_printf("s");
    // os_printf("%s %d\r\n",__func__,__LINE__);
    if(video_msg.camera_mode == CAM_DUAL_SPLICE_SLAVE_MODE){
        video_msg.video_type_last = video_msg.video_type_cur;
        video_msg.video_type_cur  = ISP_VIDEO_0 + param1;
    }
}


void hgisp_frame_done_handle(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2){
	//_os_printf("d");
    if(param1 == 0 && video_msg.camera_mode == CAM_DUAL_SPLICE_SLAVE_MODE){
        os_event_set((void *)&set_fps_event, EVENT_ISP_DOEN_OPT, NULL); 
    }
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

capture_irq_hdl fsync_capture_irq_hdl(uint32 irq, uint32 irq_data){
    if (irq == CAPTURE_IRQ_FLAG_CAPTURE) {
		os_event_set((void *)&set_fps_event, EVENT_FYSNC_CNT, NULL); 
    }
	return 0;
}

/**
 * @brief 设置双路MIPI传感器的帧率
 * 
 * @param mode  帧率模式，仅支持2种预设的帧率切换（正常模式、夜视降帧模式）
 * @param fps   目标帧率值，用于配置fsync信号，必须和预设配置表的帧率对应
 * 
 */
void dual_mipi_sensor_set_fps(enum fps_mode mode, float fps)
{
    if(video_msg.camera_mode != CAM_DUAL_SPLICE_SLAVE_MODE) {
        os_printf(KERN_ERR"%s This camera mode is not supported! \r\n", __func__);
        return;
    }
    int32 msg_ret   = 0;
    struct isp_sensor_opt   *sensor_opt = malloc(sizeof(struct isp_sensor_opt));
    if (!sensor_opt) return;   
    sensor_opt->fps_mode = mode;
    sensor_opt->fps_f    = fps;
    // os_printf("%s ===> %d %f \r\n",__func__,sensor_opt->fps_mode,sensor_opt->fps_f);
    msg_ret = os_msgq_put((void *)&set_fps_msg, (uint32)sensor_opt, 0);
    if(msg_ret != RET_OK) {
        free(sensor_opt);
    }
}




int32 set_fps_task(void)
{
    uint32 flag = 0;
    int32 event_ret = 0;
    int32 msg_ret   = 0;

    struct isp_sensor_opt   *opt = NULL;
    
    while (1)
    {
        event_ret = os_event_wait((void *)&set_fps_event, EVENT_ISP_DOEN_OPT | EVENT_FYSNC_CNT , &flag, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 50);
        if (event_ret){
            continue;
        }

        if(flag & EVENT_FYSNC_CNT){
            //gpio_set_val(PA_15, (cnt++)&0x01);
            if(fsync_pending){
                if(fsync_cnt++ > 3){
                    isp_cfg_dev();
                    fsync_pending = 0;
                }
            }
        }
		
		if (flag & EVENT_ISP_DOEN_OPT){
            opt = (struct isp_sensor_opt *)os_msgq_get2((void *)&set_fps_msg, 0, &msg_ret);
            if (msg_ret == 0){
                dual_mipi_csi_reset(opt->fps_mode,opt->fps_f);
                fsync_cnt = 0;
                fsync_pending = 1;
                free(opt);
            } 
		}
    }
}


int32 isp_task(void *data)
{
    uint32 flag = 0;
    int32 event_ret = 0;
    int32 msg_ret   = 0;
    uint8  msg_cnt     = 0;

    struct isp_device       *dev = (struct isp_device *)data;
    struct isp_exposure_opt *cfg = NULL;
    struct isp_sensor_opt   *opt = NULL;
    
    while (1)
    {
        event_ret = os_event_wait((void *)&isp_event, EVENT_ISP_CALC | EVENT_ISP_IMG_OPT | EVENT_ISP_FPS_OPT | EVENT_ISP_DOEN_OPT , &flag, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 50);
        if (event_ret)
        {
            continue;
        }

        if (flag & EVENT_ISP_CALC)
        {
            if (cfg && iic_devid_finish(cfg->devid_id) == 1)
            {
                cfg->data.size = 0;
                cfg = NULL;
            }

            isp_calculate(dev, (void *)&cfg);

            if (cfg)
            {
                wake_up_iic_queue(cfg->devid_id, (void *)&cfg->data, cfg->cmd_len, 2, NULL);
            }   
        }

        if ((flag & EVENT_ISP_IMG_OPT) || (flag & EVENT_ISP_FPS_OPT))
        {
            msg_cnt = os_msgq_cnt((void *)&isp_msg);
            for (int i = 0; i < msg_cnt; i++)
            {
                opt = (struct isp_sensor_opt *)os_msgq_get2((void *)&isp_msg, 100, &msg_ret);
                if (msg_ret == 0){
                    wake_up_iic_queue(opt->devid_id, (uint8_t*)&opt->data, opt->cmd_len, 2, (uint8_t*)NULL);
                    while(iic_devid_finish(opt->devid_id) != 1){
                        os_sleep_ms(1);
                    }
                }
            }
        }
    }
}

uint32_t sensor_read_frame_length(_Sensor_Adpt_ *sensor_cmd, uint8 addr_num , uint8 data_num , uint8_t devid)
{
	if (sensor_cmd == NULL)	return 0;

	uint32_t frame_len 	    = 0;
	uint8_t reg_val[3] 		= {0};
	uint8_t tablebuf[8] 	= {0};

	for(uint8 i = 0; i < sensor_cmd->vts_reg_num; i++)
	{
		uint8_t index = 3;
		uint16_t sensorRegAddr = sensor_cmd->vts_reg[i];
		tablebuf[0] = addr_num;
		tablebuf[1] = data_num;
		// tablebuf[2] = (w_cmd)>>1;
		
		if(tablebuf[0] == SENSOR_REG_WIDTH_16){
			tablebuf[index++] = (sensorRegAddr >> 8) & 0xFF;
			tablebuf[index++] = sensorRegAddr & 0xFF;
		} else if(tablebuf[0] == SENSOR_REG_WIDTH_8){
			tablebuf[index++] = sensorRegAddr & 0xFF;
		}  

		wake_up_iic_queue(devid,tablebuf,0,0,(uint8_t*)NULL);
		while(iic_devid_finish(devid) != 1){
			os_sleep_ms(1);
		}
		reg_val[i] = tablebuf[index];
		// os_printf("=== %x %x %x %x %x %x\r\n",tablebuf[0],tablebuf[1],tablebuf[2],tablebuf[3],tablebuf[4],tablebuf[5]);
	}
	
    if (sensor_cmd->vts_reg_num == 2) {
        frame_len = (reg_val[0] << 8) | reg_val[1];
    } else if (sensor_cmd->vts_reg_num == 3) {
        frame_len = (reg_val[0] << 16) |(reg_val[1] << 8)  | reg_val[2];
    }

	os_printf("frame_len 0x%04x %d reg_val[0/1/2] %02x/%02x/%02x\r\n",frame_len,frame_len,reg_val[0],reg_val[1],reg_val[2]);
	
    if (frame_len == 0 || frame_len == 0xFFFF || frame_len == 0xFFFFFF) {
        return 0;
    }else{
        return frame_len;
	}
}

extern const uint16_t isp_param[];

void isp_cfg_dev(){
	uint8_t ret;
    struct hgisp_sensor_init   *sensor_init   = NULL;
	struct isp_device          *isp_dev       = NULL;
    _Sensor_Adpt_              *p_sensor_cmd  = NULL;
    _Sensor_Ident_             *sensor_ident  = NULL;
    SENSOR_BASIC_INFO          *info          = NULL;
	isp_dev = (struct isp_device *)dev_get(HG_ISP_DEVID);	
    if (isp_dev == NULL)
    {
        os_printf("get isp device err\r\n");
        return;
    }
	os_printf("isp cfg....\r\n");
    sensor_init = (struct hgisp_sensor_init *)isp_sensor_param_load((void *)isp_param, -1);
    if (sensor_init && !list_empty((void *)&sensor_info_head))
    {
        os_event_init((void *)&isp_event);
        os_msgq_init((void *)&isp_msg, 2);

        ret = isp_open(isp_dev, ISP_HARDWARE_CLK, ISP_INPUT_FIFO_FULL);
        if (!ret)
        {
            isp_yuv_range(isp_dev, VIDEO_YUV_RANGE_TYPE);
            isp_set_camera_mode(isp_dev, video_msg.camera_mode);
            isp_sensor_param_config(isp_dev, (uint32)sensor_init);
            isp_y_gamma_init(isp_dev  , (uint32)y_gamma_tbl);
            isp_rgb_gamma_init(isp_dev, (uint32)gamma_table_addr[1]);
            isp_awb_mannul_mode_map(isp_dev, (uint32)isp_awb_mode_map);
            isp_ae_ev_offset_lut(isp_dev, (uint32)ev_offset_lut);
            isp_awb_gain_type(isp_dev, AWB_GAIN_TYPE_AWB, SENSOR_TYPE_MASTER);
            isp_awb_gain_type(isp_dev, AWB_GAIN_TYPE_AWB, SENSOR_TYPE_SLAVE0);
			isp_awb_gain_type(isp_dev, AWB_GAIN_TYPE_AWB, SENSOR_TYPE_SLAVE1);
            list_for_each_entry(info, (struct list_head*)&sensor_info_head, list)
            {
                p_sensor_cmd = ( _Sensor_Adpt_ *)info->sensor_config;
                sensor_ident = ( _Sensor_Ident_ *)info->sensor_opt_cmd;

                ret = isp_sensor_init(isp_dev, info->sensor_type, info->sensor_src, (uint32)p_sensor_cmd);
                if (ret)
                {
                    os_printf(KERN_ERR"config sensor param err!\r\n");
                    isp_close(isp_dev);
                    goto end;
                } else {
                    isp_sensor_iic_devid_init(isp_dev, info->sensor_dev_id, info->sensor_type);
                    isp_sensor_iic_cmd_init(isp_dev, sensor_ident->w_cmd, sensor_ident->addr_num, sensor_ident->data_num, info->sensor_type);
                    uint32_t frame_len = sensor_read_frame_length(p_sensor_cmd,sensor_ident->addr_num,sensor_ident->data_num,info->sensor_dev_id);
                    if(frame_len){
                        isp_sensor_set_frame_len(isp_dev, frame_len , 1, info->sensor_type);
                    }else{
                        os_printf(KERN_WARNING"read sensor frame length err, Use the default frame length \r\n");
                    }
                }
            }
            
            isp_request_irq(isp_dev, ISP_IRQ_FLAG_DMA_DONE , hgisp_frame_start_handle   , 0);
            isp_request_irq(isp_dev, ISP_IRQ_FLAG_FRM_END  , hgisp_frame_done_handle    , 0);
            isp_request_irq(isp_dev, ISP_IRQ_FLAG_DAT_OF   , hgisp_data_overflow_handle , 0);
            isp_request_irq(isp_dev, ISP_IRQ_FLAG_INTF_SLOW, hgisp_frame_slow_handle    , 0);
            isp_request_irq(isp_dev, ISP_IRQ_FLAG_FRM_FAST , hgisp_frame_fast_handle    , 0);
            isp_request_irq(isp_dev, ISP_IRQ_FLAG_MD_DONE  , hgisp_motion_detect_handle , 0);
            isp_task_hdl = os_task_create("isp", (void *)isp_task, isp_dev, OS_TASK_PRIORITY_HIGH+1, 0, NULL, 1024);

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

    if (isp_task_hdl) {
        os_task_destroy(isp_task_hdl);
    }

    isp_close(isp_dev);

    if (isp_event.hdl) {
        os_event_del((void *)&isp_event);
    }

    if (isp_msg.hdl) {
        os_msgq_del((void *)&isp_msg);
    }

}

