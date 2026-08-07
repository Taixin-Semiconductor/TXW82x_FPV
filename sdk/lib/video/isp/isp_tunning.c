#include "basic_include.h"
#include "hal/isp_tunning.h"
#include "hal/isp.h"
#include "hal/dual_org.h"
#include "lib/multimedia/framebuff.h"
#include "lib/multimedia/msi.h"
#include "lib/video/dvp/jpeg/jpg.h"

#if ISP_TUNNING_EN
struct isp_tunnning_dev *isp_tunning = NULL;
void isp_tunning_response(struct isp_tunnning_dev *p_dev, uint16 ret_val)
{
    uint16 crc_val   = 0;
    os_memset(p_dev->response, 0, 8);
    p_dev->response[0] = p_dev->tunning_head;
    p_dev->response[1] = p_dev->cmd_num;
    p_dev->response[2] = ret_val;    
    crc_val = hw_crc(CRC_TYPE_CRC16_MODBUS, (void *)p_dev->response, 6);
    p_dev->response[3] = crc_val;    
    p_dev->response[5] = 0x0a;    
    p_dev->write_handle(p_dev->device, 0, (void *)p_dev->response, sizeof(p_dev->response));
}

void isp_tunning_message_head(struct isp_tunnning_dev *p_dev, uint32 data_addr, uint32 data_len)
{
    p_dev->message_head[0] = p_dev->tunning_head;
    p_dev->message_head[1] = p_dev->cmd_num;
    p_dev->message_head[2] = (data_len >>  0) & 0xffff;
    p_dev->message_head[3] = (data_len >> 16) & 0xffff;
    p_dev->message_head[4] = hw_crc(CRC_TYPE_CRC16_MODBUS, (void *)data_addr, data_len);
    p_dev->message_head[5] = hw_crc(CRC_TYPE_CRC16_MODBUS, (void *)p_dev->message_head, TUNNING_PACKAGE_INFO_SIZE - 2 - 4);
    p_dev->message_head[6] = 0x00;
    p_dev->message_head[7] = 0x0a;
    p_dev->write_handle(p_dev->device, 0, (void *)p_dev->message_head, TUNNING_PACKAGE_INFO_SIZE);
}

uint32 isp_tunning_get_img(struct isp_tunnning_dev *p_dev)
{
    uint32           ret_val        = TUNNING_ERR_CODE_RIGHT;
    uint32           flen           = 0;
    uint32           offset         = 0;
    uint8            *img_addr      = NULL;
    struct framebuff *get_f         = NULL;
    uint8            get_max        = 10;
    uint8            get_idx        = 0;
	uint8 		     first_packet   = 0;
    while((++get_idx) <= get_max)
    {
__retry:
        get_f = msi_get_fb(p_dev->v_msi, 100);
        if (get_f)
        {
            if (abs(get_f->time - os_jiffies()) > 100)
            {
                msi_delete_fb(NULL, get_f);
                get_f = NULL;
                goto __retry;
            }
            
            flen = get_f->len;
            img_addr = get_f->data;
            p_dev->write_data.addr = os_zalloc(TUNNING_PACKET_SIZE+4);
            if (p_dev->write_data.addr)
            {
				// os_printf("%s %d addr : %08x\r\n", __func__, __LINE__, (uint32)p_dev->write_data.addr);

                isp_tunning_message_head(p_dev, (uint32)img_addr, flen);
				p_dev->write_data.addr[TUNNING_PACKET_SIZE+0] = 0x00;
				p_dev->write_data.addr[TUNNING_PACKET_SIZE+1] = 0x00;
				p_dev->write_data.addr[TUNNING_PACKET_SIZE+2] = 0x0d;
                p_dev->write_data.addr[TUNNING_PACKET_SIZE+3] = 0x0a;
                while (flen)
                {
                    offset = (flen > TUNNING_PACKET_SIZE) ? (TUNNING_PACKET_SIZE) : (flen);
                    hw_memcpy((void *)p_dev->write_data.addr, img_addr, offset);
                    img_addr += offset;
                    flen     -= offset;
					if (offset < TUNNING_PACKET_SIZE)
					{
						p_dev->write_data.addr[offset+0] = 0x00;
						p_dev->write_data.addr[offset+1] = 0x00;
						p_dev->write_data.addr[offset+2] = 0x0d;
						p_dev->write_data.addr[offset+3] = 0x0a;
					}
                    p_dev->write_handle(p_dev->device, 0, (void *)p_dev->write_data.addr, offset+4);
					if (first_packet == 0) {
						os_sleep_ms(10);
						first_packet = 1;
					}else {
						os_sleep_ms(1);
					}
                }
                os_free(p_dev->write_data.addr);
            } else {
                ret_val = TUNNING_ERR_CODE_MALLOC_ERR;
                os_printf("%s %d malloc size : %d err\r\n", __func__, __LINE__,  TUNNING_PACKET_SIZE);
                goto __err;
            }
            break;
        } 
    }
    if ((get_idx > get_max) && (get_f == NULL))
    {
        ret_val = TUNNING_ERR_CODE_GET_PARAM_ERR;
        os_printf("%s %d get frame err\r\n", __func__, __LINE__);
        goto __err;
    }
    goto __end;
__err:
    isp_tunning_response(p_dev, ret_val);
__end:
    if(get_f)
    {
        msi_delete_fb(NULL, get_f);
        p_dev->v_msi->enable = 0;
        while(1)
        {
            get_f = msi_get_fb(p_dev->v_msi, 0);
            if (get_f)
            {
                msi_delete_fb(NULL, get_f);
            } else {
                break;
            }
        }
        get_f = NULL;
    }
    return ret_val;
}

void isp_tunning_get_raw(struct isp_tunnning_dev *p_dev)
{
    uint32           ret_val        = TUNNING_ERR_CODE_RIGHT;
    uint32           flen           = 0;
    uint32           offset         = 0;
	uint8 		     first_packet   = 0;

    p_dev->backup_addr     = dual_save_cfg(p_dev->p_dual, p_dev->cmd_channel, (uint32)&flen);
    p_dev->write_data.size = TUNNING_PACKET_SIZE;
    p_dev->write_data.addr = os_zalloc(TUNNING_PACKET_SIZE+4);
    if (p_dev->write_data.addr)
    {
        // os_printf("%s %d addr : %08x src : %08x flen : %d\r\n", __func__, __LINE__, (uint32)p_dev->write_data.addr, p_dev->backup_addr, flen);
        while(!dual_save_status(p_dev->p_dual));
        sys_dcache_invalid_range((void *)p_dev->backup_addr, flen);
        isp_tunning_message_head(p_dev, p_dev->backup_addr, flen);
		p_dev->write_data.addr[TUNNING_PACKET_SIZE+0] = 0x00;
		p_dev->write_data.addr[TUNNING_PACKET_SIZE+1] = 0x00;
		p_dev->write_data.addr[TUNNING_PACKET_SIZE+2] = 0x0d;
        p_dev->write_data.addr[TUNNING_PACKET_SIZE+3] = 0x0a;
        while (flen)
        {
            // os_printf("%s %d flen : %d\r\n", __func__, __LINE__, flen);
            offset = (flen > TUNNING_PACKET_SIZE) ? (TUNNING_PACKET_SIZE) : (flen);
            hw_memcpy((void *)p_dev->write_data.addr, (void *)p_dev->backup_addr, offset);
            p_dev->backup_addr += offset;
            flen               -= offset;
            if (offset < TUNNING_PACKET_SIZE)
            {
                p_dev->write_data.addr[offset+0] = 0x00;
                p_dev->write_data.addr[offset+1] = 0x00;
                p_dev->write_data.addr[offset+2] = 0x0d;
                p_dev->write_data.addr[offset+3] = 0x0a;
            }
            p_dev->write_handle(p_dev->device, 0, (void *)p_dev->write_data.addr, offset+4);
			if (first_packet == 0) {
				os_sleep_ms(10);
				first_packet = 1;
			}else {
				os_sleep_ms(1);
			}
        }
		dual_recover_cfg(p_dev->p_dual);
        os_free(p_dev->write_data.addr);
    } else {
        os_printf("%s %d get package_size : %d err\r\n", __func__, __LINE__, TUNNING_PACKET_SIZE);
        ret_val = TUNNING_ERR_CODE_MALLOC_ERR;
        goto __err;
    }
    return;
    
__err:
    isp_tunning_response(p_dev, ret_val);
    return;
}

void isp_tunning_dump_data(struct isp_tunnning_dev *p_dev, uint32 data_addr, uint32 data_size)
{
    uint32 ret_val       = 0;
    uint32 flen          = data_size;
    uint32 first_packet  = 0;
	uint32 offset		 = 0;
    uint32 faddr         = data_addr;

    p_dev->write_data.addr = os_malloc(TUNNING_PACKET_SIZE + 4);
    if (p_dev->write_data.addr)
    {
        isp_tunning_message_head(p_dev, faddr, flen);
        p_dev->write_data.addr[TUNNING_PACKET_SIZE+0] = 0x00;
        p_dev->write_data.addr[TUNNING_PACKET_SIZE+1] = 0x00;
        p_dev->write_data.addr[TUNNING_PACKET_SIZE+2] = 0x0d;
        p_dev->write_data.addr[TUNNING_PACKET_SIZE+3] = 0x0a;
        while (flen)
        {
            offset = (flen > TUNNING_PACKET_SIZE) ? (TUNNING_PACKET_SIZE) : (flen);
            hw_memcpy((void *)p_dev->write_data.addr, (void *)faddr, offset);
            faddr += offset;
            flen  -= offset;
            if (offset < TUNNING_PACKET_SIZE)
            {
                p_dev->write_data.addr[offset+0] = 0x00;
                p_dev->write_data.addr[offset+1] = 0x00;
                p_dev->write_data.addr[offset+2] = 0x0d;
                p_dev->write_data.addr[offset+3] = 0x0a;
            }
            p_dev->write_handle(p_dev->device, 0, (void *)p_dev->write_data.addr, offset+4);
            if (first_packet == 0) {
                os_sleep_ms(10);
                first_packet = 1;
            }else {
                os_sleep_ms(1);
            }
        }
        os_free(p_dev->write_data.addr);
    } else {
        os_printf("%s %d malloc write_data err\r\n", __func__, __LINE__);   
        goto __err;
    }

    return;
    
__err:
    isp_tunning_response(p_dev, ret_val);
    return;
}

void isp_tunning_thread(void *dev)
{
    uint32       ret_val       = 0;
    uint8        response_flag = 1;
    scatter_data dump_data;
    struct isp_tunnning_dev *p_dev    = (struct isp_tunnning_dev *)dev;

    os_memset(&dump_data, 0, sizeof(scatter_data));
    while(1)
    {
        response_flag = 1;
        os_sema_down((void *)&p_dev->usb_cmd_sema, -1);
        
        switch (p_dev->cmd_num)
        {
            case ISP_IOCTL_CMD_3DNR_ENABLE:
                isp_3dnr_enable(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_IMG_MIRROR:
                isp_mirror_enable(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_IMG_REVERSE:
                isp_reverse_enable(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_GET_IMG:
                p_dev->v_msi->enable = 1;
                ret_val = isp_tunning_get_img(p_dev);
                response_flag = 0;
                break;

            case ISP_IOCTL_CMD_AWB_MANNUL_MODE_TYPE:
                isp_awb_mannul_mode_type(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AWB_CONTROL_STEP:
                isp_awb_control_step_config(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AWB_CONTROL_THR:
                isp_awb_control_thr_config(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->p_data[2], p_dev->p_data[3], p_dev->p_data[4], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AWB_AUTO_CTRL:
                isp_awb_auto_config(p_dev->p_isp, p_dev->p_data[0], (void *)&p_dev->p_data[1], p_dev->cmd_channel);
				break;
                
            case ISP_IOCTL_CMD_AWB_FINE_CONSTRAINT_CTRL:
                isp_awb_fine_constraint_enable(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
				break;

            case ISP_IOCTL_CMD_AWB_COARSE_CONSTRAINT_CTRL:
                isp_awb_coarse_constraint_enable(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
				break;

            case ISP_IOCTL_CMD_AWB_MEAS_MODE:
                isp_awb_measure_mode_config(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AWB_WP_EXPECT:
                isp_awb_wp_expect_val(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->cmd_channel);
                break;
                       
			case ISP_IOCTL_CMD_AWB_WP_RANGE_CONSTRAINT:
                isp_awb_cbcr_constraint(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->p_data[2], p_dev->cmd_channel);
                break;

			case ISP_IOCTL_CMD_AWB_WP_RANGE_CONSTRAINTF:
                isp_awb_cbcr_constraintf(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->p_data[2], p_dev->cmd_channel);
                break;

			case ISP_IOCTL_CMD_AWB_WP_RANGE_CONSTRAINT_RESTRAIN:
                isp_awb_cbcr_constraint_restrain(p_dev->p_isp, (uint32)p_dev->p_data, p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AWB_RGB_CONSTRAINT:
                isp_awb_rgb_constraint(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->p_data[2], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AWB_WP_NUM_RESTRAIN:
                isp_awb_wp_constraint(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AWB_GAIN_TYPE:
                isp_awb_gain_type(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AWB_GAIN_CONSTRAINT:
                isp_awb_gain_constraint(p_dev->p_isp, (uint32)p_dev->p_data, p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AWB_GAIN_COARSE_CONSTRAINT:
                isp_awb_gain_coarse_constraint(p_dev->p_isp, (uint32)p_dev->p_data, p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AWB_GAIN_FINE_CONSTRAINT:
                isp_awb_gain_fine_constraint(p_dev->p_isp, (uint32)p_dev->p_data, p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AWB_CROP_RANGE:
                ret_val = isp_awb_crop_range(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->p_data[2], p_dev->p_data[3], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_MANUAL_PARAM:
                isp_ae_manul_config(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->p_data[2], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_ROW_TIME_US:
                isp_ae_row_time(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_ANALOG_GAIN:
                isp_ae_analog_gain(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_EXPOSURE_LINE:
                isp_ae_exposure_line(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->p_data[2], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_CONFIG_INTERVAL:
                isp_ae_interval_config(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_DAY_NIGHT_BV:
                isp_ae_day_night_bv(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_SCENE_LUT:
                isp_ae_scene_lut(p_dev->p_isp, (uint32)p_dev->p_data, p_dev->cmd_channel);
                break;
                
            case ISP_IOCTL_CMD_AE_LOWLIGHT_PARAM:
                isp_ae_lowlight_param(p_dev->p_isp, (uint32)p_dev->p_data, p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_FRAME_MAX:
                isp_ae_frame_max(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_LOCK_PARAM:
                isp_ae_lock_param(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_REDUCE_FPS:
                isp_ae_reduce_fps(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_LOWLIGHT_LSB_GAIN_EN:
                isp_ae_lowlight_gain_enable(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_LOWLIGHT_LSB_GAIN_4HI_FPS:
                isp_ae_lowlight_gain_high_gain(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_LOWLIGHT_LSB_GAIN_4LO_FPS:
                isp_ae_lowlight_gain_low_gain(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_HIST_HS_BIN_THR:
                isp_ae_hist_hs_bin_thr(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_LUMA_TARGET:
                isp_ae_luma_target_config(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_LUMA_WEIGHT:
                isp_ae_luma_weight_config(p_dev->p_isp, p_dev->p_data[0], (void *)&p_dev->p_data[1], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_ABL_LUMA_MAX:
                isp_ae_abl_luma_max(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_POS_THR:
                isp_ae_pos_thr(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_HIST_THR:
                isp_ae_hist_thr(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_AOE_EXPO_GAIN:
                isp_ae_aoe_gain(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_ABL_LOCK_THR:
                isp_ae_abl_lock(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->p_data[2], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_BRIGHT_DARK_PIXEL_RATIO:
                isp_ae_brihgt_dark_pixel_ratio(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->p_data[2], p_dev->p_data[3], p_dev->cmd_channel);
                break;
                
            case ISP_IOCTL_CMD_AE_ABL_DARK_BRIGHT_POS_THR:
                isp_ae_abl_dark_bright_pos_ratio(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->p_data[2], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_AOE_DARK_BRIGHT_POS_THR:
                isp_ae_aoe_dark_bright_pos_ratio(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_ABL_GAIN_MODE:
                isp_ae_abl_gain_type(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_ABL_EXPO_LINE_RATIO:
                isp_ae_abl_expo_line_ratio(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_ABL_EXPO_GAIN_THR:
                isp_ae_abl_expo_gain_set(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_ABL_BV_THR:
                isp_ae_abl_bv_thr(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_AOE_BV_THR:
                isp_ae_aoe_bv_thr(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_HIST_PIXEL_THR:
                isp_ae_hist_pixel_thr(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->p_data[2], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_STG_PARAM:
                ret_val = isp_ae_stg_param(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->p_data[2], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_EV_OFFSET_TYPE:
                isp_ae_ev_offset_type(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_AE_CROP_RANGE:
                isp_ae_crop_range(p_dev->p_isp, (uint32)p_dev->p_data, p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_HIST_CROP_RANGE:
                isp_hist_crop_range(p_dev->p_isp, (uint32)p_dev->p_data, p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_CCM_ARRAY:
                isp_ccm_config(p_dev->p_isp, (void *)p_dev->p_data, p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_BLC_PARAM:
                isp_blc_config(p_dev->p_isp, (void *)p_dev->p_data, p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_MD_WINDOW_PARAM:
                isp_md_window_config(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->p_data[2], p_dev->p_data[3], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_MD_DETECT_PARAM:
                isp_md_param_config(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->p_data[2], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_CSC_PARAM:
                isp_csc_config(p_dev->p_isp, (void *)p_dev->p_data, p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_DPC_PARAM:
                isp_dpc_config(p_dev->p_isp, (void *)p_dev->p_data, p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_CE_YUV_RANGE:
                isp_ce_yuv_range(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_CE_LUMA:
                isp_ce_luma_config(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_CE_SATURATION:
                isp_ce_saturation_config(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_CE_CONTRAST:
                isp_ce_contrast_config(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_CE_HUE:
                isp_ce_hue_config(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_CE_OFFSET_PARAM:
                isp_ce_offset_config(p_dev->p_isp, (uint32)p_dev->p_data, p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_CE_SATURATION_BV_PARAM:
                isp_ce_bv_stauration(p_dev->p_isp, (uint32)p_dev->p_data, p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_SHARP_PARAM:
                isp_sharp_param(p_dev->p_isp, (uint32)p_dev->p_data, p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_RAWNR_MAP:
                isp_rawnr_map(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], (uint32)&p_dev->p_data[2], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_YUVNR_MAP:
                isp_yuvnr_map(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], (uint32)&p_dev->p_data[2], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_CSUPP_MAP:
                isp_csupp_map(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], (uint32)&p_dev->p_data[2], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_Y_GAMMA_TUNNING:
                isp_y_gamma_tunning(p_dev->p_isp, (uint32)p_dev->p_data, p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_RGB_GAMMA_TUNNING:
                isp_rgb_gamma_tunning(p_dev->p_isp, (uint32)p_dev->p_data, p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_LSC_TUNNING:
                isp_lsc_tunning(p_dev->p_isp, (uint32)p_dev->p_data, p_dev->cmd_channel);
                break;
                
            case ISP_IOCTL_CMD_GIC_PARAM:
                isp_gic_init(p_dev->p_isp, (uint32)p_dev->p_data, p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_LHS_MAP:
                isp_lhs_map(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], (uint32)&p_dev->p_data[2], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_GET_SENSOR_RAW:
                isp_tunning_get_raw(p_dev);
                response_flag = 0;
                break;

            case ISP_IOCTL_CMD_GET_Y_GAMMA:
            case ISP_IOCTL_CMD_GET_RGB_GAMMA:
            case ISP_IOCTL_CMD_GET_LSC:
            case ISP_IOCTL_CMD_DUMP_SRAM_PARAM:
            case ISP_IOCTL_CMD_DUMP_REG_PARAM:
                ret_val = isp_ioctl(p_dev->p_isp, p_dev->cmd_num, p_dev->cmd_channel, (uint32)&dump_data);
                //os_printf("%s %d dump addr :%08x dump_data : %08x\r\n", __func__, __LINE__, (uint32)dump_data.addr, dump_data.size);
                if (!ret_val)       isp_tunning_dump_data(p_dev, (uint32)dump_data.addr, dump_data.size);
                response_flag = 0;
                break;

            case ISP_IOCTL_CMD_CONFIG_SRAM_PARAM:
                isp_sensor_sram_param_config(p_dev->p_isp, p_dev->cmd_channel, (uint32)p_dev->p_data);
                break;

            case ISP_IOCTL_CMD_FUNC_ENABLE:
                ret_val = isp_sensor_func_enable(p_dev->p_isp, p_dev->cmd_channel, (uint32)p_dev->p_data[0]);
                break;

            case ISP_IOCTL_CMD_WDR_EN:
                isp_wdr_en_config(p_dev->p_isp, p_dev->p_data[0], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_WDR_NOISE_FLOOR:
                isp_wdr_noise_floor_config(p_dev->p_isp, p_dev->p_data[0], p_dev->p_data[1], p_dev->cmd_channel);
                break;

            case ISP_IOCTL_CMD_WDR_TUNNING:
                isp_wdr_tunning(p_dev->p_isp, (uint32)p_dev->p_data, p_dev->cmd_channel);
                break;

            default:
                os_printf("tunning get cmd type : %d err type : %d!\r\n", p_dev->cmd_num, ret_val);
                break;
        }
        if (p_dev->p_data)      os_free(p_dev->p_data);
        if (response_flag)      isp_tunning_response(p_dev, ret_val);
    }
}

void isp_tunning_get_img_msi_create(struct isp_tunnning_dev *p_dev, enum tunning_img_type type, uint32 img_w, uint32 img_h, uint32 src)
{
    if (type)
    {
        struct msi *h264_msi_init_with_mode(uint32_t drv1_from, uint32_t drv1_w, uint32_t drv1_h, uint32_t drv2_from, uint32_t drv2_w, uint32_t drv2_h);
        p_dev->video_msi =  msi_find("auto-h264", 1);//h264_msi_init_with_mode(src, img_w, img_h, 0, 0, 0);
    } else {
		struct msi *jpg_concat_msi_init_start(uint32_t jpgid,uint16_t w, uint16_t h, uint16_t *filter_type,uint8_t src_from,uint8_t run);
        p_dev->video_msi = msi_find("auto-jpg", 1);//jpg_concat_msi_init_start(JPGID0, img_w, img_h, NULL, src,1);
    }

    if (p_dev->video_msi)
    {
        p_dev->v_msi = msi_new(TUNNING_IMG_MSI,1,NULL);
        if (p_dev->v_msi)
        {
            // p_dev->v_msi->enable = 1;
            msi_add_output(p_dev->video_msi, NULL, TUNNING_IMG_MSI);
        } else {
            p_dev->video_msi = NULL;
            p_dev->v_msi     = NULL;
        }
    } else {
        p_dev->video_msi = NULL;
        p_dev->v_msi     = NULL;
    }
}

void isp_tunning_init(uint32 img_w, uint32 img_h)
{
    rt_ssize_t cdc_usb_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size);
    isp_tunning = os_zalloc(sizeof(struct isp_tunnning_dev));
    if (isp_tunning == NULL)
    {
        os_printf("%s %d isp_tunning_dev malloc err");
        return;
    }
    os_sema_init(&isp_tunning->usb_write_sema, 0);
    os_sema_init(&isp_tunning->usb_cmd_sema  , 0);
    isp_tunning->tunning_head = 0xb103;
    isp_tunning->tunning_succ = 0x55aa;
    isp_tunning->tunning_err  = 0x5a5a;
    isp_tunning->p_isp        = (struct isp_device  *)dev_get(HG_ISP_DEVID);
    isp_tunning->p_dual       = (struct dual_device *)dev_get(HG_DUALORG_DEVID);
    isp_tunning->write_handle = cdc_usb_write;
    isp_tunning_get_img_msi_create(isp_tunning, TUNNING_IMG_JPEG, img_w, img_h, 0);
    if ((isp_tunning->p_isp == NULL) || (isp_tunning->p_dual == NULL) || (isp_tunning->video_msi == NULL))
    {
        os_printf("%s %d get device isp:%d dual:%d msi : %d err", __func__, __LINE__, (isp_tunning->p_isp == NULL), (isp_tunning->p_dual == NULL), (isp_tunning->video_msi == NULL));
        os_free(isp_tunning);
        return;
    }
    os_printf("%s %d create succ\r\n", __func__, __LINE__);
    os_task_create("isp_tunning_demo", isp_tunning_thread, (void *)isp_tunning, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
}
#endif