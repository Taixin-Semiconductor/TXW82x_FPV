#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "hal/isp.h"

int32 isp_open(struct isp_device *isp, enum isp_module_clk clk_type, enum isp_input_fifo fifo_type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->open) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->open(isp, clk_type, fifo_type);
    }
    return RET_ERR;
}

int32 isp_close(struct isp_device *isp)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->close) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->close(isp);
    }
    return RET_ERR;
}

int32 isp_ioctl(struct isp_device *isp, uint32 cmd, uint32 param1, uint32 param2)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, cmd, param1, param2);
    }
    return RET_ERR;
}

int32 isp_calculate(struct isp_device *isp, struct isp_ae_func_cfg *p_cfg)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->calculate) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->calculate(isp, p_cfg);
    }
    return RET_ERR;
}

int32 isp_get_status(struct isp_device *isp)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_GET_STA, 0, 0);
    }
    return RET_ERR;   
}

int32 isp_request_irq(struct isp_device *isp, uint32 irq_flag, isp_irq_hdl irqhdl, uint32 irq_data)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->request_irq) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->request_irq(isp, irq_flag, irqhdl, irq_data);
    }
    return RET_ERR;
}

int32 isp_release_irq(struct isp_device *isp, uint32 irq_flag)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->release_irq) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->release_irq(isp, irq_flag);
    }
    return RET_ERR;
}

int32 isp_y_gamma_init(struct isp_device *isp, uint32 y_gamma_addr)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_Y_GAMMA_ADDR, y_gamma_addr, 0);
    }
    return RET_ERR;
}

int32 isp_rgb_gamma_init(struct isp_device *isp, uint32 rgb_gamma_addr)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_RGB_GAMMA_ADDR, rgb_gamma_addr, 0);
    }
    return RET_ERR;
}

int32 isp_clk_divide_init(struct isp_device *isp, uint32 clk_type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_CLK_DIVIDE, clk_type, 0);
    }
    return RET_ERR;
}

int32 isp_fifo_init(struct isp_device *isp, uint32 fifo_type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_FIFO_TYPE, fifo_type, 0);
    }
    return RET_ERR;
}

int32 isp_sensor_init(struct isp_device *isp, enum sensor_type type, enum isp_input_dat_src sensor_src, uint32 sensor_config)
{
    uint32 param[] = {type, sensor_src};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_SENSOR_INIT, sensor_config, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_crop_input_size(struct isp_device *isp, uint32 start_v, uint32 start_h, uint32 end_v, uint32 end_h, enum sensor_type type)
{
    uint32 param[4] = {start_v, start_h, end_v, end_h};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_CROP_INPUT, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_crop_output_size(struct isp_device *isp, uint32 start_v, uint32 start_h, uint32 end_v, uint32 end_h, enum sensor_type type)
{
    uint32 param[4] = {start_v, start_h, end_v, end_h};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_CROP_OUTPUT, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_sensor_iic_devid_init(struct isp_device *isp, uint32 devid_id, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_SENSOR_IIC_DEVID, type, (uint32)devid_id);
    }
    return RET_ERR;
}

int32 isp_sensor_iic_cmd_init(struct isp_device *isp, uint32 cmd_id, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_SENSOR_IIC_OPTCMD, type, (uint32)cmd_id);
    }
    return RET_ERR;
}

int32 isp_sensor_slave_index(struct isp_device *isp)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_SLAVE_INDEX, 0, 0);
    }
    return RET_ERR;
}

int32 isp_black_white_enable(struct isp_device *isp, uint32 enable, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_BLACK_WHITE_MODE, type, enable);
    }
    return RET_ERR;
}

int32 isp_mirror_enable(struct isp_device *isp, uint32 enable, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_IMG_MIRROR, type, enable);
    }
    return RET_ERR;
}

int32 isp_reverse_enable(struct isp_device *isp, uint32 enable, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_IMG_REVERSE, type, enable);
    }
    return RET_ERR;
}

int32 isp_luma_ca_init(struct isp_device *isp, uint32 *addr, uint32 num, enum sensor_type type)
{
    uint32 param[2] = {(uint32)addr, num};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_LUMA_CA_ADDR, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_awb_mannul_mode_map(struct isp_device *isp, uint32 map_addr)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AWB_MANNUL_MODE_MAP, map_addr, 0);
    }  
    return RET_ERR;
}

int32 isp_awb_mannul_mode_type(struct isp_device *isp, enum isp_awb_mode mannul_type, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AWB_MANNUL_MODE_TYPE, type, mannul_type);
    }  
    return RET_ERR;
}

int32 isp_awb_control_step_config(struct isp_device *isp, uint16 coarse, uint16 fine, enum sensor_type type)
{
    uint32 param[] = {coarse, fine};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AWB_CONTROL_STEP, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_awb_control_thr_config(struct isp_device *isp, uint16 coarse_thr, uint16 hi_thr, uint16 lo_thr, uint16 rgb_thr, uint16 yuv_thr, enum sensor_type type)
{
    uint32 param[] = {coarse_thr, hi_thr, lo_thr, rgb_thr, yuv_thr};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AWB_CONTROL_THR, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_awb_wp_expect_val(struct isp_device *isp, uint32 cb_target, uint32 cr_target, enum sensor_type type)
{
    uint32 param[2] = {cb_target, cr_target};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AWB_WP_EXPECT, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_awb_rgb_constraint(struct isp_device *isp, uint32 r_max, uint32 g_max, uint32 b_max, enum sensor_type type)
{
    uint32 param[3] = {r_max, g_max, b_max};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AWB_RGB_CONSTRAINT, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_awb_cbcr_constraint(struct isp_device *isp, uint32 cb_min, uint32 cr_min, uint32 sum_val, enum sensor_type type)
{
    uint32 param[3] = {cb_min, cr_min, sum_val};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AWB_WP_RANGE_CONSTRAINT, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_awb_cbcr_constraintf(struct isp_device *isp, uint32 cb_min, uint32 cr_min, uint32 sum_val, enum sensor_type type)
{
    uint32 param[3] = {cb_min, cr_min, sum_val};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AWB_WP_RANGE_CONSTRAINTF, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_awb_cbcr_constraint_restrain(struct isp_device *isp, uint32 param, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AWB_WP_RANGE_CONSTRAINT_RESTRAIN, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_awb_wp_constraint(struct isp_device *isp, uint32 wp_min, uint32 wp_max, enum sensor_type type)
{
    uint32 param[2] = {wp_min, wp_max};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AWB_WP_NUM_RESTRAIN, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_awb_measure_mode_config(struct isp_device *isp, uint32 mode, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AWB_MEAS_MODE, (uint32)type, mode);
    }
    return RET_ERR;
}

int32 isp_awb_auto_config(struct isp_device *isp, uint32 enable, uint32 *gain_buf, enum sensor_type type)
{
    uint32 param[2] = {enable, (uint32)gain_buf};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AWB_AUTO_CTRL, type,  (uint32)param);
    }
    return RET_ERR;
}

int32 isp_awb_fine_constraint_enable(struct isp_device *isp, uint32 enable, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AWB_FINE_CONSTRAINT_CTRL, type,  enable);
    }
    return RET_ERR;
}

int32 isp_awb_coarse_constraint_enable(struct isp_device *isp, uint32 enable, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AWB_COARSE_CONSTRAINT_CTRL, type,  enable);
    }
    return RET_ERR;
}

int32 isp_awb_gain_type(struct isp_device *isp, uint32 gain_type, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AWB_GAIN_TYPE, type, gain_type);
    }
    return RET_ERR;
}

int32 isp_awb_gain_coarse_constraint(struct isp_device *isp, uint32 addr, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AWB_GAIN_COARSE_CONSTRAINT, type, addr);
    }
    return RET_ERR;
}

int32 isp_awb_gain_fine_constraint(struct isp_device *isp, uint32 addr, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AWB_GAIN_FINE_CONSTRAINT, type, addr);
    }
    return RET_ERR;
}

int32 isp_awb_crop_range(struct isp_device *isp, uint32 start_v, uint32 start_h, uint32 end_v, uint32 end_h, enum sensor_type type)
{
    uint32 param[] = {start_v, start_h, end_v, end_h};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AWB_CROP_RANGE, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_awb_gain_constraint(struct isp_device *isp, uint32 addr, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AWB_GAIN_CONSTRAINT, type,  (uint32)addr);
    }
    return RET_ERR;
}

int32 isp_get_current_bv(struct isp_device *isp, uint32 *bv, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_BV, type, (uint32)bv);
    }
    return RET_ERR;
}


int32 isp_ae_manul_config(struct isp_device *isp, uint32 target, uint32 analog_gain, uint32 expo_line, enum sensor_type type)
{
    uint32 param[] = {target, analog_gain, expo_line};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_MANUAL_PARAM, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_ae_row_time(struct isp_device *isp, uint32 row_us, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_ROW_TIME_US, type, row_us);
    }
    return RET_ERR;
}

int32 isp_ae_analog_gain(struct isp_device *isp, uint32 max_gain, uint32 min_gain, enum sensor_type type)
{
    uint32 param[2] = {max_gain, min_gain};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_ANALOG_GAIN, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_ae_exposure_line(struct isp_device *isp, uint32 max_line, uint32 min_line, uint32 def_line, enum sensor_type type)
{
    uint32 param[3] = {max_line, min_line, def_line};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_EXPOSURE_LINE, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_ae_interval_config(struct isp_device *isp, uint32 cnt, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_CONFIG_INTERVAL, type, cnt);
    }
    return RET_ERR;
}

int32 isp_ae_day_night_bv(struct isp_device *isp, uint32 day_bv, uint32 night_bv, enum sensor_type type)
{
    uint32 param[] = {day_bv, night_bv};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_DAY_NIGHT_BV, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_ae_scene_lut(struct isp_device *isp, uint32 addr, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_SCENE_LUT, type, (uint32)addr);
    }
    return RET_ERR;
}

int32 isp_ae_lowlight_param(struct isp_device *isp, uint32 addr, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_LOWLIGHT_PARAM, type, (uint32)addr);
    }
    return RET_ERR;
}

int32 isp_ae_frame_max(struct isp_device *isp, uint32 frame_max, uint32 vb, enum sensor_type type)
{
    uint32 param[] = {frame_max, vb};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_FRAME_MAX, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_ae_lock_param(struct isp_device *isp, uint32 cnt, uint32 diff, enum sensor_type type)
{
    uint32 param[] = {cnt, diff};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_LOCK_PARAM, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_ae_reduce_fps(struct isp_device *isp, uint32 enable, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_REDUCE_FPS, type, enable);
    }
    return RET_ERR;
}

int32 isp_ae_lowlight_gain_enable(struct isp_device *isp, uint32 enable, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_LOWLIGHT_LSB_GAIN_EN, type, enable);
    }
    return RET_ERR;
}

int32 isp_ae_lowlight_gain_high_gain(struct isp_device *isp, uint32 gain, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_LOWLIGHT_LSB_GAIN_4HI_FPS, type, gain);
    }
    return RET_ERR;
}

int32 isp_ae_lowlight_gain_low_gain(struct isp_device *isp, uint32 gain, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_LOWLIGHT_LSB_GAIN_4LO_FPS, type, gain);
    }
    return RET_ERR;
}

int32 isp_ae_hist_hs_bin_thr(struct isp_device *isp, uint32 hs_bin_thr, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_HIST_HS_BIN_THR, type, hs_bin_thr);
    }
    return RET_ERR;
}

int32 isp_ae_luma_target_config(struct isp_device *isp, uint32 luma_target, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_LUMA_TARGET, type, luma_target);
    }
    return RET_ERR;
}

int32 isp_ae_luma_weight_config(struct isp_device *isp, uint32 luma_weight_sum, uint32 *luma_weight, enum sensor_type type)
{
    uint32 param[2] = {luma_weight_sum, (uint32)luma_weight};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_LUMA_WEIGHT, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_ae_abl_luma_max(struct isp_device *isp, uint32 max, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_ABL_LUMA_MAX, type, max);
    }
    return RET_ERR;
}

int32 isp_ae_pos_thr(struct isp_device *isp, uint32 dark_thr, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_POS_THR, type, dark_thr);
    }
    return RET_ERR;
}

int32 isp_ae_hist_thr(struct isp_device *isp, uint32 val1, uint32 val2, enum sensor_type type)
{
    uint32 param[2] = {val1, val2};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_HIST_THR, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_ae_aoe_gain(struct isp_device *isp, uint32 val1, uint32 val2, enum sensor_type type)
{
    uint32 param[2] = {val1, val2};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_AOE_EXPO_GAIN, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_ae_abl_lock(struct isp_device *isp, uint32 diff_ratio, uint32 dark_pos_thr, uint32 bright_pos_thr, enum sensor_type type)
{
    uint32 param[] = {diff_ratio, dark_pos_thr, bright_pos_thr};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_ABL_LOCK_THR, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_ae_brihgt_dark_pixel_ratio(struct isp_device *isp, uint32 bright_low, uint32 bright_high, uint32 dark_low, uint32 dark_high, enum sensor_type type)
{
    uint32 param[] = {bright_low, bright_high, dark_low, dark_high};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_BRIGHT_DARK_PIXEL_RATIO, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_ae_abl_dark_bright_pos_ratio(struct isp_device *isp, uint32 dark_low, uint32 dark_high, uint32 bright_thr, enum sensor_type type)
{
    uint32 param[] = {dark_low, dark_high, bright_thr};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_ABL_DARK_BRIGHT_POS_THR, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_ae_aoe_dark_bright_pos_ratio(struct isp_device *isp, uint32 dark_thr, uint32 bright_thr, enum sensor_type type)
{
    uint32 param[] = {dark_thr, bright_thr};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_AOE_DARK_BRIGHT_POS_THR, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_ae_abl_gain_type(struct isp_device *isp, uint32 abl_gain_type, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_ABL_GAIN_MODE, type, abl_gain_type);
    }
    return RET_ERR;
}

int32 isp_ae_abl_expo_line_ratio(struct isp_device *isp, uint32 low, uint32 high, enum sensor_type type)
{
    uint32 param[] = {low, high};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_ABL_EXPO_LINE_RATIO, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_ae_abl_expo_gain_set(struct isp_device *isp, uint32 low, uint32 high, enum sensor_type type)
{
    uint32 param[] = {low, high};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_ABL_EXPO_GAIN_THR, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_ae_abl_bv_thr(struct isp_device *isp, uint32 low, uint32 high, enum sensor_type type)
{
    uint32 param[] = {low, high};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_ABL_BV_THR, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_ae_aoe_bv_thr(struct isp_device *isp, uint32 low, uint32 high, enum sensor_type type)
{
    uint32 param[] = {low, high};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_AOE_BV_THR, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_ae_hist_pixel_thr(struct isp_device *isp, uint32 upper, uint32 upper_hs, uint32 lower, enum sensor_type type)
{
    uint32 param[] = {upper, upper_hs, lower};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_HIST_PIXEL_THR, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_ae_stg_param(struct isp_device *isp, uint32 stg_mode, uint32 ratio_slope, uint32 max_offset, enum sensor_type type)
{
    uint32 param[] = {stg_mode, ratio_slope, max_offset};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_STG_PARAM, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_ae_ev_offset_lut(struct isp_device *isp, uint32 lut_addr)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_EV_OFFSET_LUT, lut_addr, 0);
    }
    return RET_ERR;
}

int32 isp_ae_ev_offset_type(struct isp_device *isp, enum isp_ev_offset offset_type, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_EV_OFFSET_TYPE, type, offset_type);
    }
    return RET_ERR;
}

int32 isp_ae_crop_range(struct isp_device *isp, uint32 data, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_AE_CROP_RANGE, type, data);
    }
    return RET_ERR;
}

int32 isp_hist_crop_range(struct isp_device *isp, uint32 data, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_HIST_CROP_RANGE, type, data);
    }
    return RET_ERR;
}

int32 isp_ccm_config(struct isp_device *isp, uint32 *ccm_array, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_CCM_ARRAY, type, (uint32)ccm_array);
    }
    return RET_ERR;
}

int32 isp_blc_config(struct isp_device *isp, uint32 *blc_array, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_BLC_PARAM, type, (uint32)blc_array);
    }
    return RET_ERR;
}

int32 isp_md_window_config(struct isp_device *isp, uint32 win_start_h, uint32 win_start_v, uint32 win_end_h, uint32 win_end_v, enum sensor_type type)
{
    uint32 param[4] = {win_start_h, win_start_v, win_end_h, win_end_v};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_MD_WINDOW_PARAM, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_md_param_config(struct isp_device *isp, uint32 frame_val, uint32 block_val, uint32 frame_cnt, enum sensor_type type)
{
    uint32 param[3] = {frame_val, block_val, frame_cnt};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_MD_DETECT_PARAM, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_csc_config(struct isp_device *isp, uint32 *config_addr, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_CSC_PARAM, type, (uint32)config_addr);
    }
    return RET_ERR;
}

int32 isp_dpc_config(struct isp_device *isp, uint32 *config_addr, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_DPC_PARAM, type, (uint32)config_addr);
    }
    return RET_ERR;
}

int32 isp_ce_yuv_range(struct isp_device *isp, uint32 range, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_CE_YUV_RANGE, type, range);
    }
    return RET_ERR;
}

int32 isp_ce_luma_config(struct isp_device *isp, uint32 luma_val, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_CE_LUMA, type, luma_val);
    }
    return RET_ERR;
}

int32 isp_ce_saturation_config(struct isp_device *isp, uint32 luma_val, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_CE_SATURATION, type, luma_val);
    }
    return RET_ERR;
}

int32 isp_ce_contrast_config(struct isp_device *isp, uint32 contrast, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_CE_CONTRAST, type, contrast);
    }
    return RET_ERR;
}

int32 isp_ce_hue_config(struct isp_device *isp, uint32 hue, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_CE_HUE, type, hue);
    }
    return RET_ERR;
}

int32 isp_ce_offset_config(struct isp_device *isp, uint32 addr, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_CE_OFFSET_PARAM, type, addr);
    }
    return RET_ERR;
}

int32 isp_ce_bv_stauration(struct isp_device *isp, uint32 addr, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_CE_SATURATION_BV_PARAM, type, addr);
    }
    return RET_ERR;
}

int32 isp_sharp_param(struct isp_device *isp, uint32 addr, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_SHARP_PARAM, type, addr);
    }
    return RET_ERR;
}

int32 isp_rawnr_map(struct isp_device *isp, uint32 map_num, uint32 map_start, uint32 map_data_addr, enum sensor_type type)
{
    uint32 param[] = {map_num, map_start, map_data_addr};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_RAWNR_MAP, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_yuvnr_map(struct isp_device *isp, uint32 map_num, uint32 map_start, uint32 map_data_addr, enum sensor_type type)
{
    uint32 param[] = {map_num, map_start, map_data_addr};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_YUVNR_MAP, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_csupp_map(struct isp_device *isp, uint32 map_num, uint32 map_start, uint32 map_data_addr, enum sensor_type type)
{
    uint32 param[] = {map_num, map_start, map_data_addr};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_CSUPP_MAP, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_y_gamma_tunning(struct isp_device *isp, uint32 addr, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_Y_GAMMA_TUNNING, type, addr);
    }
    return RET_ERR;
}

int32 isp_rgb_gamma_tunning(struct isp_device *isp, uint32 addr, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_RGB_GAMMA_TUNNING, type, addr);
    }
    return RET_ERR;
}

int32 isp_lsc_tunning(struct isp_device *isp, uint32 addr, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_LSC_TUNNING, type, addr);
    }
    return RET_ERR;
}

int32 isp_gic_init(struct isp_device *isp, uint32 param_addr, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_GIC_PARAM, type, param_addr);
    }
    return RET_ERR;
}

int32 isp_lhs_map(struct isp_device *isp, uint32 map_num, uint32 map_start, uint32 map_data_addr, enum sensor_type type)
{
    uint32 param[] = {map_num, map_start, map_data_addr};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_LHS_MAP, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_3dnr_enable(struct isp_device *isp, uint32 enable, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_3DNR_ENABLE, type, enable);
    }
    return RET_ERR;
}

int32 isp_sensor_param_config(struct isp_device *isp, uint32 addr)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_INIT_SENSOR_PARAM, (uint32)addr, 0);
    }
    return RET_ERR;
}

int32 isp_sensor_sram_param_config(struct isp_device *isp, uint32 channel, uint32 addr)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_CONFIG_SRAM_PARAM, channel, addr);
    }
    return RET_ERR;
}

int32 isp_sensor_func_enable(struct isp_device *isp, uint32 channel, uint32 data)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_FUNC_ENABLE, channel, data);
    }
    return RET_ERR;
}

int32 isp_wdr_en_config(struct isp_device *isp, uint32 wdr_en, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_WDR_EN, type, wdr_en);
    }
    return RET_ERR;
}

int32 isp_wdr_noise_floor_config(struct isp_device *isp, uint32 cfg_noise_floor, uint32 cfg_noise_floor_out, enum sensor_type type)
{
    uint32 param[2] = {cfg_noise_floor, cfg_noise_floor_out};
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_WDR_NOISE_FLOOR, type, (uint32)param);
    }
    return RET_ERR;
}

int32 isp_wdr_tunning(struct isp_device *isp, uint32 addr, enum sensor_type type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_WDR_TUNNING, type, addr);
    }
    return RET_ERR;
}

int32 isp_yuv_range(struct isp_device *isp, enum isp_yuv_range range_type)
{
    if (isp && ((const struct isp_hal_ops *)isp->dev.ops)->ioctl) {
        return ((const struct isp_hal_ops *)isp->dev.ops)->ioctl(isp, ISP_IOCTL_CMD_YUV_RANGE, range_type, 0);
    }
    return RET_ERR;
}
