#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/vpp.h"

int32 vpp_open(struct vpp_device *p_vpp)
{
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->open) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->open(p_vpp);
    }
    return RET_ERR;
}

int32 vpp_close(struct vpp_device *p_vpp)
{
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->close) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->close(p_vpp);
    }
    return RET_ERR;
}


int32 vpp_dis_uv_mode(struct vpp_device *p_vpp,uint8 enable_buf0,uint8 enable_buf1){
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_DIS_UV_MODE, enable_buf0, enable_buf1);
    }
    return RET_ERR;
}

int32 vpp_set_threshold(struct vpp_device *p_vpp,uint32 dlt,uint32 dht){
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_THRESHOLD, dlt, dht);
    }
    return RET_ERR;
}

int32 vpp_set_water0_rc(struct vpp_device *p_vpp,uint8 enable){
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_RC, enable, 0);
    }
    return RET_ERR;
}

int32 vpp_set_water1_rc(struct vpp_device *p_vpp,uint8 enable){
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK1_RC, enable, 0);
    }
    return RET_ERR;
}

int32 vpp_set_water0_color(struct vpp_device *p_vpp,uint8 y,uint8 u,uint8 v){
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_COLOR, y|(u<<8)|(v<<16), 0);
    }
    return RET_ERR;
}

int32 vpp_set_water1_color(struct vpp_device *p_vpp,uint8 y,uint8 u,uint8 v){
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK1_COLOR, y|(u<<8)|(v<<16), 0);
    }
    return RET_ERR;
}

int32 vpp_set_water0_bitmap(struct vpp_device *p_vpp,uint32 bitmap){
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_BMPADR, bitmap, 0);
    }
    return RET_ERR;
}

int32 vpp_set_water1_bitmap(struct vpp_device *p_vpp,uint32 bitmap){
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK1_BMPADR, bitmap, 0);
    }
    return RET_ERR;
}

int32 vpp_set_water0_locate(struct vpp_device *p_vpp,uint8 x,uint8 y){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_LOCATED, (y<<8)|x, 0);
	}
	return RET_ERR;
}

int32 vpp_set_water1_locate(struct vpp_device *p_vpp,uint8 x,uint8 y){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK1_LOCATED, (y<<8)|x, 0);
	}
	return RET_ERR;
}


int32 vpp_set_water0_contrast(struct vpp_device *p_vpp,uint8 contrast){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_CONTRAST, contrast, 0);
	}
	return RET_ERR;
}

int32 vpp_set_water1_contrast(struct vpp_device *p_vpp,uint8 contrast){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK1_CONTRAST, contrast, 0);
	}
	return RET_ERR;
}


int32 vpp_set_watermark0_charsize_and_num(struct vpp_device *p_vpp,uint8 w,uint8 h,uint8 num){
	
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_CHAR_SIZE_AND_NUM, w|(h<<8), num);
	}
	return RET_ERR;
}


int32 vpp_set_watermark0_idx(struct vpp_device *p_vpp,uint8 index_num,uint8 index){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_CHAR_IDX, index, index_num);
	}
	return RET_ERR;
}


int32 vpp_set_watermark1_size(struct vpp_device *p_vpp,uint8 w,uint8 h){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK1_PHOTO_SIZE, (h<<8)|w, 0);
	}
	return RET_ERR;
}


int32 vpp_set_watermark0_mode(struct vpp_device *p_vpp,uint8 mode){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_MODE, mode, 0);
	}
	return RET_ERR;
}

int32 vpp_set_watermark1_mode(struct vpp_device *p_vpp,uint8 mode){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK1_MODE, mode, 0);
	}
	return RET_ERR;
}

int32 vpp_set_watermark0_enable(struct vpp_device *p_vpp,uint8 enable){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_EN, enable, 0);
	}
	return RET_ERR;
}

int32 vpp_set_watermark1_enable(struct vpp_device *p_vpp,uint8 enable){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK1_EN, enable, 0);
	}
	return RET_ERR;
}

int32 vpp_set_motion_det_enable(struct vpp_device *p_vpp,uint8 enable){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_MOTION_DET_EN, enable, 0);
	}
	return RET_ERR;
}

int32 vpp_set_motion_calbuf(struct vpp_device *p_vpp,uint32 calbuf){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_MOTION_CALBUF, calbuf, 0);
	}
	return RET_ERR;
}

int32 vpp_set_motion_range(struct vpp_device *p_vpp,uint16 x_s,uint16 y_s,uint16 x_e,uint16 y_e){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_MOTION_RANGE, x_s|(y_s<<16), x_e|(y_e<<16));
	}
	return RET_ERR;
}

int32 vpp_set_motion_blk_threshold(struct vpp_device *p_vpp,uint8 threshold){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_MOTION_BLK_THRESHOLD, threshold, 0);
	}
	return RET_ERR;
}

int32 vpp_set_motion_frame_threshold(struct vpp_device *p_vpp,uint32 threshold){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_MOTION_FRAME_THRESHOLD, threshold, 0);
	}
	return RET_ERR;
}

int32 vpp_set_motion_frame_interval(struct vpp_device *p_vpp,uint8 interval){
	if(interval > 0x1f){
		//printf("motion interval error:%d\r\n",interval);
		return RET_ERR;
	}
	
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_MOTION_FRAME_INTERVAL, interval, 0);
	}
	return RET_ERR;
}

int32 vpp_set_motion_all_frame(struct vpp_device *p_vpp,uint32 allframe){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_MOTION_ALL_FRAME_OR_WINDOW, allframe, 0);
	}
	return RET_ERR;
}

int32 vpp_set_ifp_addr(struct vpp_device *p_vpp,uint32 addr){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_IFP_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_ifp_en(struct vpp_device *p_vpp,uint8 enable){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_IFP_EN, enable, 0);
	}
	return RET_ERR;
}

// 1 : raw/rgb888      0: yuv422
int32 vpp_set_mode(struct vpp_device *p_vpp,uint8 mode){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_MODE, mode, 0);
	}
	return RET_ERR;
}

int32 vpp_set_video_size(struct vpp_device *p_vpp,uint32 w,uint32 h){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_SIZE, w, h);
	}
	return RET_ERR;
}

int32 vpp_set_ycbcr(struct vpp_device *p_vpp,uint8 ycbcr){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_YCBCR, ycbcr, 0);
	}
	return RET_ERR;
}


int32 vpp_set_buf0_count(struct vpp_device *p_vpp,uint8 cnt){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF0_CNT, cnt, 0);
	}
	return RET_ERR;
}

int32 vpp_set_buf0_en(struct vpp_device *p_vpp,uint8 enable){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF0_EN, enable, 0);
	}
	return RET_ERR;
}

int32 vpp_set_buf1_count(struct vpp_device *p_vpp,uint8 cnt){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF1_CNT, cnt, 0);
	}
	return RET_ERR;
}

int32 vpp_set_buf1_en(struct vpp_device *p_vpp,uint8 enable){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF1_EN, enable, 0);
	}
	return RET_ERR;
}

int32 vpp_set_buf1_shrink(struct vpp_device *p_vpp,uint8 half){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF1_SHRINK, half, 0);
	}
	return RET_ERR;
}

//0:DVP   1:DVP1   2:MIPI_CSI0   3:MIPI_CSI1   4:IMAGE_ISP    5:GEN422    6:PARA_IN
int32 vpp_set_input_interface(struct vpp_device *p_vpp,uint8 mode){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_INPUT_INTERFACE, mode, 0);
	}
	return RET_ERR;
}

int32 vpp_set_buf0_y_addr(struct vpp_device *p_vpp,uint32 addr){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF0_Y_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_buf0_u_addr(struct vpp_device *p_vpp,uint32 addr){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF0_U_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_buf0_v_addr(struct vpp_device *p_vpp,uint32 addr){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF0_V_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_buf1_y_addr(struct vpp_device *p_vpp,uint32 addr){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF1_Y_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_buf1_u_addr(struct vpp_device *p_vpp,uint32 addr){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF1_U_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_buf1_v_addr(struct vpp_device *p_vpp,uint32 addr)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF1_V_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_itp_y_addr(struct vpp_device *p_vpp,uint32 addr){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_ITP_Y_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_itp_u_addr(struct vpp_device *p_vpp,uint32 addr){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_ITP_U_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_itp_v_addr(struct vpp_device *p_vpp,uint32 addr)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_ITP_V_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_itp_linebuf(struct vpp_device *p_vpp,uint32 linebuf)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_TAKE_PHOTO_LINEBUF, linebuf, 0);
	}
	return RET_ERR;
}

int32 vpp_set_itp_enable(struct vpp_device *p_vpp,uint8 en)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_ITP_EN, en, 0);
	}
	return RET_ERR;
}

int32 vpp_set_itp_auto_close(struct vpp_device *p_vpp,uint8 en)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_ITP_AUTO_CLOSE, en, 0);
	}
	return RET_ERR;
}

int32 vpp_set_isp_config(struct vpp_device *p_vpp)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_ISP_CONFIG, 0, 0);
	}
	return RET_ERR;
}

int32 vpp_get_status(struct vpp_device *p_vpp)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_GET_STA, 0, 0);
	}
	return RET_ERR;
}

int32 vpp_set_ftusb30_enable(struct vpp_device *p_vpp,uint8 en)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_FTUSB3_EN, en, 0);
	}
	return RET_ERR;
}

int32 vpp_set_psram_ycnt(struct vpp_device *p_vpp,uint32 w,uint32 h){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_PSRAM_YCNT, w, h);
	}
	return RET_ERR;
}

int32 vpp_set_psram_uvcnt(struct vpp_device *p_vpp,uint32 w,uint32 h){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_PSRAM_UVCNT, w, h);
	}
	return RET_ERR;
}

int32 vpp_set_psram1_ycnt(struct vpp_device *p_vpp,uint32 w,uint32 h){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_PSRAM1_YCNT, w, h);
	}
	return RET_ERR;
}

int32 vpp_set_psram1_uvcnt(struct vpp_device *p_vpp,uint32 w,uint32 h){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_PSRAM1_UVCNT, w, h);
	}
	return RET_ERR;
}


//vppbuf:   1:vpp buf0   2:vpp buf1
int32 vpp_set_nosram_buf_enable(struct vpp_device *p_vpp,uint8 vppbuf)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_FRAME_SAVE_NO_SRAM, vppbuf, 0);
	}
	return RET_ERR;
}

//vppbuf : 0==>16+2N   1:2N
int32 vpp_set_sram_buf_mode(struct vpp_device *p_vpp,uint8 vppbuf0,uint8 vppbuf1)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SRAMBUFF_CNT_MODE, vppbuf0, vppbuf1);
	}
	return RET_ERR;
}

//scale : 0==>buf0   1:buf1
int32 vpp_set_scale_buf_select(struct vpp_device *p_vpp,uint8 scale1,uint8 scale3)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SCALEBUF_SELECT, scale1, scale3);
	}
	return RET_ERR;
}

//0:vpp buf0  1:vpp buf1
int32 vpp_get_scale1_buf_select(struct vpp_device *p_vpp){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_GET_SCALE1BUF_SELECT, 0, 0);
	}
	return RET_ERR;
}

//0:vpp buf0  1:vpp buf1
int32 vpp_get_scale3_buf_select(struct vpp_device *p_vpp){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_GET_SCALE3BUF_SELECT, 0, 0);
	}
	return RET_ERR;
}

int32 vpp_set_nosram_buf1_enable(struct vpp_device *p_vpp,uint8 en)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_F1_SAVE_NO_SRAM, en, 0);
	}
	return RET_ERR;
}


int32 vpp_get_f0_is_psram(struct vpp_device *p_vpp)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_GET_F0_IS_PSRAM, 0, 0);
	}
	return RET_ERR;
}

int32 vpp_get_f1_is_psram(struct vpp_device *p_vpp)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_GET_F1_IS_PSRAM, 0, 0);
	}
	return RET_ERR;
}

int32 vpp_f0_is_enable(struct vpp_device *p_vpp)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_GET_F0_IS_ENABLE, 0, 0);
	}
	return RET_ERR;
}

int32 vpp_get_f1_mode(struct vpp_device *p_vpp){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_GET_F1_MODE, 0, 0);
	}
	return RET_ERR;
}

int32 vpp_f1_is_enable(struct vpp_device *p_vpp)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_GET_F1_IS_ENABLE, 0, 0);
	}
	return RET_ERR;
}

int32 vpp_get_f1_shrink(struct vpp_device *p_vpp)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_GET_F1_SHRINK, 0, 0);
	}
	return RET_ERR;
}


int32 vpp_set_gen420_buf_sel(struct vpp_device *p_vpp,uint8 buf)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_GEN420_BUF_SEL, buf, 0);
	}
	return RET_ERR;
}


int32 vpp_set_watermark0_auto_rc_threshold(struct vpp_device *p_vpp,uint32 cth,uint32 hth)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_AUTO_RC_THRESHOLD, cth, hth);
	}
	return RET_ERR;
}

int32 vpp_set_watermark0_auto_rc(struct vpp_device *p_vpp,uint32 en,uint8 y,uint8 u,uint8 v)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_AUTO_RC_COLOR, en, y|(u<<8)|(v<<16));
	}
	return RET_ERR;
}

int32 vpp_set_watermark0_auto_rc_sram_adr(struct vpp_device *p_vpp,uint32 adr)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_AUTO_RC_HADR, adr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_watermark_auto_rc_mode(struct vpp_device *p_vpp,uint8 mode)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_AUTO_RC_MODE, mode, 0);
	}
	return RET_ERR;
}


int32 vpp_request_irq(struct vpp_device *p_vpp, uint32 irq_flags, vpp_irq_hdl irq_hdl, uint32 irq_data)
{
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->request_irq) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->request_irq(p_vpp, irq_flags, irq_hdl, irq_data);
    }
    return RET_ERR;
}

int32 vpp_release_irq(struct vpp_device *p_vpp, uint32 irq_flags)
{
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->release_irq) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->release_irq(p_vpp, irq_flags);
    }
    return RET_ERR;
}


