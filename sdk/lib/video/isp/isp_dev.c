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


extern uint32 luma_ca_addr[];
extern IRCUT_INFO ircut_info;

void   *isp_task_hdl = NULL;
volatile struct os_event    isp_event;
volatile struct os_msgqueue isp_msg;

extern volatile struct os_event  set_fps_event;

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
        info->mode_index = opt_cmd;
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

/* 拷贝函数 */
static void isp_param_to_sensor_info(struct hgisp_sensor_init *init,  const _Sensor_ISP_Init *p_table,  uint8 index)
{
    const _Sensor_ISP_Init *p_src = &p_table[index];
    struct hgisp_param_info *p_dst = &init->sensor_info[index].sensor_param;

    /* 逐个成员拷贝，空则关闭对应使能 */
    if (p_src->p_func) {
        os_memcpy(&p_dst->enable_param, p_src->p_func, sizeof(_ISP_FUNC));
    }

    if (p_src->p_awb) {
        os_memcpy(&p_dst->awb_param, p_src->p_awb, sizeof(TYPE_HGISP_CFG_AWB));
    } else {
        p_dst->enable_param.awb_en = 0;
    }

    if (p_src->p_ae) {
        os_memcpy(&p_dst->cfg_ae, p_src->p_ae, sizeof(struct hgisp_cfg_ae));
    } else {
        p_dst->enable_param.ae_en = 0;
    }

    if (p_src->p_ccm) {
        os_memcpy(&p_dst->ccm_param, p_src->p_ccm, sizeof(_Sensor_CCM));
    } else {
        p_dst->enable_param.ccm_en = 0;
    }

    if (p_src->p_blc) {
        os_memcpy(&p_dst->blc_param, p_src->p_blc, sizeof(_Sensor_BLC));
    } else {
        p_dst->enable_param.blc_en = 0;
    }

    if (p_src->p_csc) {
        os_memcpy(&p_dst->csc_param, p_src->p_csc, sizeof(_Sensor_CSC));
    }

    if (p_src->p_gamma_param) {
        os_memcpy(&p_dst->gamma_param, p_src->p_gamma_param, sizeof(_Sensor_GAMMA_BV));
    }

    if (p_src->p_dpc) {
        os_memcpy(&p_dst->dpc_param, p_src->p_dpc, sizeof(_Sensor_DPC));
    } else {
        p_dst->enable_param.dpc_en = 0;
    }

    if (p_src->p_colenh) {
        os_memcpy(&p_dst->ce_param, p_src->p_colenh, sizeof(_Sensor_BV2COLENH));
    } else {
        p_dst->enable_param.ce_en = 0;
    }

    if (p_src->p_sharp) {
        os_memcpy(&p_dst->sharp_param, p_src->p_sharp, sizeof(_Sensor_SHARP));
    } else {
        p_dst->enable_param.sharpen_en = 0;
    }

    if (p_src->p_bv2nr) {
        os_memcpy(p_dst->nr_param.bv2rawnr_map, p_src->p_bv2nr, sizeof(_Sensor_BV2NR) * BV2RAWNR_ARRAY_NUM);
    } else {
        p_dst->enable_param.bnr_en = 0;
    }

    if (p_src->p_yuvnr) {
        os_memcpy(p_dst->nr_param.bv2yuvnr_map, p_src->p_yuvnr, sizeof(_Sensor_YUVNR) * BV2YUVNR_ARRAY_NUM);
    } else {
        p_dst->enable_param.ynr_en = 0;
    }

    if (p_src->p_csupp) {
        os_memcpy(p_dst->nr_param.bv2csupp_map, p_src->p_csupp, sizeof(_Sensor_CSUPP) * BV2CSUPP_ARRAY_NUM);
    } else {
        p_dst->enable_param.csupp_en = 0;
    }

    if (p_src->p_gic) {
        os_memcpy(&p_dst->gic_param, p_src->p_gic, sizeof(_Sensor_GIC));
    } else {
        p_dst->enable_param.gic_en = 0;
    }

    if (p_src->p_lhs) {
        os_memcpy(&p_dst->lhs_param, p_src->p_lhs, sizeof(_Sensor_LHS_MAP));
    } else {
        p_dst->enable_param.adj_hue_en = 0;
    }

    if (p_src->p_wdr) {
        os_memcpy(&p_dst->config_wdr, p_src->p_wdr, sizeof(TYPE_HGISP_CFG_WDR));
    }

    if (p_src->p_ygamma) {
        os_memcpy(&p_dst->y_gamma, p_src->p_ygamma, sizeof(_Sensor_YGAMMA_MAP));
    } else {
        p_dst->enable_param.y_gamma_en = 0;
    }

    if (p_src->p_rgb_gamma) {
        p_dst->rgb_gamma = p_src->p_rgb_gamma->p_rgb_gamma_tbl;
    } else {
        p_dst->enable_param.rgb_gamma_en = 0;
    }

    if (p_src->p_lsc) {
        p_dst->lsc_tbl = p_src->p_lsc->p_lsc_tbl;
    } else {
        p_dst->enable_param.lsc_en = 0;
    }

    if (p_src->p_rgb_gamma->p_rgb_gamma_tbl) {
        p_dst->rgb_gamma = p_src->p_rgb_gamma->p_rgb_gamma_tbl;
    } else {
        p_dst->enable_param.rgb_gamma_en = 0;
    }

    if (p_src->p_lsc->p_lsc_tbl) {
        p_dst->lsc_tbl = p_src->p_lsc->p_lsc_tbl;
    } else {
        p_dst->enable_param.lsc_en = 0;
    }

    // p_dst->malloc = NULL;
    // p_dst->free = NULL;
}

void isp_sensor_basic_init(struct hgisp_sensor_init *init,  const _Sensor_Adpt_ *p_sensor_init,  uint8 index, uint8 sensor_src ,uint8 mode_index){

    struct hgisp_basic_info *p_basic = &init->sensor_info[index].sensor_basic;
    _Sensor_AE *sensor_ae  = &init->sensor_info[index].sensor_param.cfg_ae.sensor_ae;

    p_basic->format_type        = p_sensor_init->sensor_isp_cfg.input_format;
    p_basic->input_type         = sensor_src;
    p_basic->bayer_type         = p_sensor_init->supported_modes[mode_index].bayer_patten;
    p_basic->sensor_size_h      = p_sensor_init->supported_modes[mode_index].width;
    p_basic->sensor_size_v      = p_sensor_init->supported_modes[mode_index].height;
    p_basic->input_size_h       = p_sensor_init->supported_modes[mode_index].width;
    p_basic->input_size_v       = p_sensor_init->supported_modes[mode_index].height;
    p_basic->input_size_startv  = 1;
    p_basic->input_size_starth  = 1;
    p_basic->crop_size_v        = p_basic->input_size_v     ;
    p_basic->crop_size_h        = p_basic->input_size_h     ;
    p_basic->crop_size_startv   = p_basic->input_size_startv;
    p_basic->crop_size_starth   = p_basic->input_size_starth;
    p_basic->sensor_index       = index;
    p_basic->mirror             = p_sensor_init->sensor_isp_cfg.mirror;
    p_basic->reverse            = p_sensor_init->sensor_isp_cfg.reverse;
    p_basic->curr_mirror        = 0;
    p_basic->curr_reverse       = 0;

    sensor_ae->min_analog_gain = p_sensor_init->sensor_isp_cfg.expo_min_gain ?: 256;
    sensor_ae->max_analog_gain =p_sensor_init->sensor_isp_cfg.expo_max_gain ?: 4096;
    sensor_ae->expo_frame_interval = p_sensor_init->sensor_isp_cfg.expo_update_delay ?: 1;
    sensor_ae->min_frame_vb = p_sensor_init->sensor_isp_cfg.min_frame_vb ?: 8;
    // sensor_ae->min_exposure_line = p_sensor_init->sensor_isp_cfg.min_frame_vb ?: 8;
    sensor_ae->row_time_us = 30;
}

static void *isp_param_load(uint16 *buff)
{
    struct hgisp_sensor_init *init = NULL;
    uint8  sensor_index      = 0;
    uint32 data_offset       = 0;
    uint32 param_size        = buff[2] << 16 | buff[1];
    uint32 gamma_size        = 256;
    uint32 lsc_size          = 153*4*4;
    uint32 info_szie         = sizeof(struct hgisp_sensor_info);
    uint32 sensor_param_size = (gamma_size << 1) + lsc_size + info_szie;
    init = (struct hgisp_sensor_init *)os_malloc(sizeof(struct hgisp_sensor_init));
	//os_printf("======== %d %x \r\n",info_szie,info_szie);	
    if (init)
    {
        os_memset(init, 0, sizeof(struct hgisp_sensor_init));
        sensor_index = buff[3];
        if (sensor_index && (param_size >= sensor_index * sensor_param_size))
        {
            data_offset = 4;
            for (int i = 0; i < sensor_index; i++)
            {
                if (buff[data_offset+2] == ISPCFG_MAGIC)
                {
                    os_memcpy((void *)&init->sensor_info[i], (void *)&buff[data_offset], info_szie);
                    init->sensor_info[i].info_src = ISP_INFO_SRC_TYPE_CODE_PARAM;
                    data_offset += (info_szie >> 1);
                    init->sensor_info[i].sensor_param.rgb_gamma = (uint32 *)&buff[data_offset];
                    data_offset += (gamma_size >> 1);
                    init->sensor_info[i].sensor_param.lsc_tbl   = (uint32 *)&buff[data_offset];
                    data_offset += (lsc_size >> 1);
                } else {
                    os_printf("param_data flash : %04x target : %04x err!\r\n", buff[data_offset], ISPCFG_MAGIC);
                    goto  _use_default;
                }
            }
            os_printf("sensor param use flash_param!\r\n");
            goto _end;
        } else {
            os_printf("param_size : %d calc_size : %d\r\n", param_size, sensor_index * sensor_param_size);
            goto  _use_default;
        }
    } else {
        os_printf("%s malloc sram size : %d err!\r\n", sizeof(struct hgisp_sensor_init));
    }
    return (void *)NULL;


_use_default:
    // os_printf("sensor param use default param!\r\n");

    for (int i = 0; i < ISP_SUPPORT_SENSOR_MAX_NUM; i++) {
        init->sensor_info[i].info_src = ISP_INFO_SRC_TYPE_CODE_DOC;
        // isp_param_to_sensor_info(init, p_table, i);
    }

_end:
    return (void *)init;
}

void isp_cfg_dev(){
	uint8_t  ret;
    uint8_t  last_sensor_type = 0;
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
    sensor_init = (struct hgisp_sensor_init *)isp_param_load((void *)isp_param);
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
            isp_awb_mannul_mode_map(isp_dev, (uint32)isp_awb_mode_map);
            isp_ae_ev_offset_lut(isp_dev, (uint32)ev_offset_lut);
            list_for_each_entry(info, (struct list_head*)&sensor_info_head, list)
            {
                // 要按 master,slave0,slave1 依次初始化
               if (info->sensor_type != last_sensor_type) {
                   os_printf(KERN_ERR"sensor type sequence error! \r\n");
                   isp_close(isp_dev);
                   goto end;
               }
               last_sensor_type = info->sensor_type + 1;

                p_sensor_cmd = ( _Sensor_Adpt_ *)info->sensor_config;
                sensor_ident = ( _Sensor_Ident_ *)&p_sensor_cmd->sensor_iic;

                if(sensor_init->sensor_info[info->sensor_type].info_src == ISP_INFO_SRC_TYPE_CODE_DOC)
                {
                    const _Sensor_ISP_Init *isp_iq_param = NULL;
                    if(p_sensor_cmd->sensor_isp_cfg.isp_iq_param != NULL){
                        isp_iq_param = p_sensor_cmd->sensor_isp_cfg.isp_iq_param;
						os_printf(KERN_INFO "use command delivered isp iq param\r\n");
                    }else{
                        isp_iq_param = default_isp_param_init;
						os_printf(KERN_INFO "isp iq param null, load default isp param\r\n", info->sensor_type);
                    }
                    isp_param_to_sensor_info(sensor_init, isp_iq_param, info->sensor_type);
                    isp_sensor_basic_init(sensor_init,p_sensor_cmd,info->sensor_type,info->sensor_src,info->mode_index);
                }

                ret = isp_sensor_init(isp_dev, info->sensor_type,(uint32)p_sensor_cmd,info->mode_index);
                if (ret)
                {
                    os_printf(KERN_ERR"config sensor param err!\r\n");
                    isp_close(isp_dev);
                    goto end;
                } else {
                    isp_awb_gain_type(isp_dev, AWB_GAIN_TYPE_AWB, info->sensor_type);
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

