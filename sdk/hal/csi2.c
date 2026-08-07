#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/csi2.h"


int32 mipi_csi_open(struct mipi_csi_device *p_mipi_csi){
    if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->open) {
        return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->open(p_mipi_csi);
    }
    return RET_ERR;
}

int32 mipi_csi_close(struct mipi_csi_device *p_mipi_csi){
    if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->close) {
        return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->close(p_mipi_csi);
    }
    return RET_ERR;
}

int32 mipi_csi_init(struct mipi_csi_device *p_mipi_csi,uint8_t lane_num){
    if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->init) {
        return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->init(p_mipi_csi,lane_num);
    }
    return RET_ERR;
}

int32 mipi_csi_deinit(struct mipi_csi_device *p_mipi_csi){
    if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->deinit) {
        return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->deinit(p_mipi_csi);
    }
    return RET_ERR;
}

int32 mipi_csi_set_baudrate(struct mipi_csi_device *p_mipi_csi, uint32 mclk)
{
    if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->baudrate) {
        return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->baudrate(p_mipi_csi, mclk);
    }
    return RET_ERR;

}

int32 mipi_csi_request_irq(struct mipi_csi_device *p_mipi_csi, uint32 irq_flags, mipi_csi_irq_hdl irq_hdl, uint32 irq_data)
{
    if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->request_irq) {
        return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->request_irq(p_mipi_csi, irq_flags, irq_hdl, irq_data);
    }
    return RET_ERR;
}

int32 mipi_csi_release_irq(struct mipi_csi_device *p_mipi_csi, uint32 irq_flags)
{
    if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->release_irq) {
        return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->release_irq(p_mipi_csi, irq_flags);
    }
    return RET_ERR;
}

int32 mipi_csi1_request_irq(struct mipi_csi_device *p_mipi_csi, uint32 irq_flags, mipi_csi_irq_hdl irq_hdl, uint32 irq_data)
{
    if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->request_irq1) {
        return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->request_irq1(p_mipi_csi, irq_flags, irq_hdl, irq_data);
    }
    return RET_ERR;
}

int32 mipi_csi1_release_irq(struct mipi_csi_device *p_mipi_csi, uint32 irq_flags)
{
    if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->release_irq1) {
        return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->release_irq1(p_mipi_csi, irq_flags);
    }
    return RET_ERR;
}

int32 mipi_csi_set_lane_num(struct mipi_csi_device *p_mipi_csi,uint32 lane_num){
	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI_LANE_NUM, lane_num, 0);
	}
	return RET_ERR;
}

int32 mipi_csi_open_virtual_channel(struct mipi_csi_device *p_mipi_csi,uint8 open){
	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI_OPEN_VIRTUAL_CHANNEL, open, 0);
	}
	return RET_ERR;
}

int32 mipi_csi_img_size(struct mipi_csi_device *p_mipi_csi,uint32 w,uint32 h){
	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI_IMG_SIZE, w, h);
	}
	return RET_ERR;
}

int32 mipi_csi_hsync_rec_time(struct mipi_csi_device *p_mipi_csi,uint8 time){
	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI_HSYNC_REC_TIME, time, 0);
	}
	return RET_ERR;
}

int32 mipi_csi_hsync_rec_enable(struct mipi_csi_device *p_mipi_csi,uint8 enable){
	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI_HSYNC_REC_EN, enable, 0);
	}
	return RET_ERR;
}

int32 mipi_csi_crop_enable(struct mipi_csi_device *p_mipi_csi,uint8 enable){
	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI_CROP_EN, enable, 0);
	}
	return RET_ERR;
}

int32 mipi_csi_crop_img_start(struct mipi_csi_device *p_mipi_csi,uint32 x,uint32 y){
	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI_CROP_COORD_START, x, y);
	}
	return RET_ERR;
}

int32 mipi_csi_crop_img_end(struct mipi_csi_device *p_mipi_csi,uint32 x,uint32 y){
	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI_CROP_COORD_END, x-1, y);
	}
	return RET_ERR;
}

int32 mipi_csi_input_format(struct mipi_csi_device *p_mipi_csi,uint8 format){
	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI_FORMAT_INPUT, format, 0);
	}
	return RET_ERR;
}


int32 mipi_csi1_img_size(struct mipi_csi_device *p_mipi_csi,uint32 w,uint32 h){
	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI1_IMG_SIZE, w, h);
	}
	return RET_ERR;
}

int32 mipi_csi1_hsync_rec_time(struct mipi_csi_device *p_mipi_csi,uint8 time){
	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI1_HSYNC_REC_TIME, time, 0);
	}
	return RET_ERR;
}

int32 mipi_csi1_hsync_rec_enable(struct mipi_csi_device *p_mipi_csi,uint8 enable){
	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI1_HSYNC_REC_EN, enable, 0);
	}
	return RET_ERR;
}

int32 mipi_csi1_crop_enable(struct mipi_csi_device *p_mipi_csi,uint8 enable){
	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI1_CROP_EN, enable, 0);
	}
	return RET_ERR;
}

int32 mipi_csi1_crop_img_start(struct mipi_csi_device *p_mipi_csi,uint32 x,uint32 y){
	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI1_CROP_COORD_START, x, y);
	}
	return RET_ERR;
}

int32 mipi_csi1_crop_img_end(struct mipi_csi_device *p_mipi_csi,uint32 x,uint32 y){
	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI1_CROP_COORD_END, x, y);
	}
	return RET_ERR;
}

int32 mipi_csi1_input_format(struct mipi_csi_device *p_mipi_csi,uint8 format){
	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI1_FORMAT_INPUT, format, 0);
	}
	return RET_ERR;
}


int32 mipi_csi_dphy_cfg(struct mipi_csi_device *p_mipi_csi,uint32 c0,uint32 c1,uint32 c2,uint32 c3,uint32 c4,uint32 c5,uint32 c6,uint32 c7){
	uint32_t cfgbuf[8];
	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		cfgbuf[0] = c0;
		cfgbuf[1] = c1;
		cfgbuf[2] = c2;
		cfgbuf[3] = c3;
		cfgbuf[4] = c4;
		cfgbuf[5] = c5;
		cfgbuf[6] = c6;
		cfgbuf[7] = c7;
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI_DPHY_CFG, (uint32)cfgbuf, 0);
	}
	return RET_ERR;
}

int32 mipi_csi_dphy_dbg(struct mipi_csi_device *p_mipi_csi,uint32 d0,uint32 d1,uint32 d2,uint32 d3){
	uint32_t cfgdbg[8];
	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		cfgdbg[0] = d0;
		cfgdbg[1] = d1;
		cfgdbg[2] = d2;
		cfgdbg[3] = d3;
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI_DPHY_DBG, (uint32)cfgdbg, 0);
	}
	return RET_ERR;
}


int32 mipi_csi_ftusb3_open(struct mipi_csi_device *p_mipi_csi, uint8 enable)
{
   	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI_FTUSB3_OPEN, enable, 0);
	} 
	return RET_ERR;
}

int32 mipi_csi_hs_rx_zero_cnt_set(struct mipi_csi_device *p_mipi_csi, uint32 value)
{
   	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI_HS_RX_ZERO_CNT, value, 0);
	} 
	return RET_ERR;
}

int32 mipi_csi_get_match_value(struct mipi_csi_device *p_mipi_csi, uint32 *value)
{
   	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI_GET_MATCH_VALUE, (uint32)value, 0);
	} 
	return RET_ERR;
}

int32 mipi_csi_get_stop_state(struct mipi_csi_device *p_mipi_csi, uint32 *value)
{
   	if (p_mipi_csi && ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl) {
		return ((const struct mipi_csi_hal_ops *)p_mipi_csi->dev.ops)->ioctl(p_mipi_csi, MIPI_CSI_STOP_STATE, (uint32)value, 0);
	} 
	return RET_ERR;
}
