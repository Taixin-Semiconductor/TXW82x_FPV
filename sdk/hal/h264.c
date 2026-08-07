#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/h264.h"

int32 h264_init(struct h264_device *p_h264,enum h264_module_clk clk_type)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->init) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->init(p_h264,clk_type);
    }
    return RET_ERR;
}

int32 h264_deinit(struct h264_device *p_h264)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->deinit) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->deinit(p_h264);
    }
    return RET_ERR;
}

int32 h264_open(struct h264_device *p_h264)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->open) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->open(p_h264);
    }
    return RET_ERR;
}

int32 h264_close(struct h264_device *p_h264)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->close) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->close(p_h264);
    }
    return RET_ERR;
}

int32 h264_is_running(struct h264_device *p_h264)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_IS_RUNNING, 0, 0);
    }
    return RET_ERR;
}

int32 h264_decode_start(struct h264_device *p_h264)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->decode) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->decode(p_h264);
    }
    return RET_ERR;
}

int32 h264_request_irq(struct h264_device *p_h264, uint32 irq_flags, h264_irq_hdl irq_hdl, uint32 irq_data)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->request_irq) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->request_irq(p_h264, irq_flags, irq_hdl, irq_data);
    }
    return RET_ERR;
}

int32 h264_release_irq(struct h264_device *p_h264, uint32 irq_flags)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->release_irq) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->release_irq(p_h264, irq_flags);
    }
    return RET_ERR;
}

int32 h264_set_src_from(struct h264_device *p_h264, uint8_t src_from)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_SRC_FROM, src_from, 0);
    }
    return RET_ERR;
}

int32 h264_set_hw_open(struct h264_device *p_h264, uint8_t enable)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_HW_OPEN, enable, 0);
    }
    return RET_ERR;
}

int32 h264_set_sw_buf_ready(struct h264_device *p_h264)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_SW_BUF_READY, 0, 0);
    }
    return RET_ERR;
}

int32 h264_set_sw_frame_ready(struct h264_device *p_h264)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_SW_FRAME_START, 0, 0);
    }
    return RET_ERR;
}

int32 h264_set_sw_ready(struct h264_device *p_h264)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_SW_READY, 0, 0);
    }
    return RET_ERR;
}

int32 h264_set_vpp_vsync_delay(struct h264_device *p_h264, uint8_t enable)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_VPP_VSYNC_DEALY_EN, enable, 0);
    }
    return RET_ERR;
}

int32 h264_set_wrap_img_size(struct h264_device *p_h264, uint16_t w,uint16_t h)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_WRAP_IMG_SIZE, w, h);
    }
    return RET_ERR;
}

int32 h264_set_phy_img_size(struct h264_device *p_h264, uint16_t w,uint16_t h)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_PHY_IMG_SIZE, w, h);
    }
    return RET_ERR;
}

int32 h264_set_frame_base_cfg(struct h264_device *p_h264, uint8_t frm_type,uint16_t enc_mode,uint32_t enc_bs_buf_size,uint32_t timeout_en)
{
	uint32_t basebuf[4];
	uint32_t *p32;
	if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
		p32 = basebuf;
		p32[0] = frm_type;
		p32[1] = enc_mode;
		p32[2] = enc_bs_buf_size;
		p32[3] = timeout_en;
		return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_FRAME_BASE_CFG, (uint32)basebuf, 0);
	}
	return RET_ERR;
}

int32 h264_set_frame_qp(struct h264_device *p_h264, uint32_t frame_qp)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_FRAME_QP, frame_qp, 0);
    }
    return RET_ERR;
}

int32 h264_set_timeout_limit(struct h264_device *p_h264, uint32_t time)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_TIMEOUT_LIMIT, time, 0);
    }
    return RET_ERR;
}

int32 h264_set_len_limit(struct h264_device *p_h264, uint32_t len)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_LEN_LIMIT, len, 0);
    }
    return RET_ERR;
}

int32 h264_set_ref_lu_addr(struct h264_device *p_h264, uint32_t saddr, uint32_t eaddr)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_REF_LU_ADDR, saddr, eaddr);
    }
    return RET_ERR;
}

int32 h264_set_ref_chro_addr(struct h264_device *p_h264, uint32_t saddr, uint32_t eaddr)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_REF_CHRO_ADDR, saddr, eaddr);
    }
    return RET_ERR;
}

int32 h264_set_frame_num(struct h264_device *p_h264, uint32_t num)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_SET_FRM_NUM, num, 0);
    }
    return RET_ERR;
}

int32 h264_set_enc_cnt(struct h264_device *p_h264, uint32_t cnt)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_SET_ENC_CNT, cnt, 0);
    }
    return RET_ERR;
}

int32 h264_set_enc_frame_id(struct h264_device *p_h264, uint32_t id)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_SET_ENC_FRAME_ID, id, 0);
    }
    return RET_ERR;
}

int32 h264_set_buf_addr(struct h264_device *p_h264, uint32_t addr)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_SET_DMA_BS_ADDR, addr, 0);
    }
    return RET_ERR;
}

int32 h264_set_frame_mb_num(struct h264_device *p_h264, uint32_t num)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_FRAME_MB_NUM, num, 0);
    }
    return RET_ERR;
}

int32 h264_set_ref_wb_addr(struct h264_device *p_h264, uint32_t lpf_lu_base, uint32_t lpf_ch_base)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_REF_WB_ADDR, lpf_lu_base, lpf_ch_base);
    }
    return RET_ERR;
}

int32 h264_set_ref_rb_addr(struct h264_device *p_h264, uint32_t last_lpf_lu_base, uint32_t last_lpf_ch_base)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_REF_RB_ADDR, last_lpf_lu_base, last_lpf_ch_base);
    }
    return RET_ERR;
}

int32 h264_set_frame_rate_ctl(struct h264_device *p_h264, uint32_t rc_grp,uint32_t rc_effort,uint32_t rc_en)
{
	uint32_t basebuf[3];
	uint32_t *p32;
	if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
		p32 = basebuf;
		p32[0] = rc_grp;
		p32[1] = rc_effort;
		p32[2] = rc_en;
		return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_SET_RATE_CTL, (uint32)basebuf, 0);
	}
	return RET_ERR;
}

int32 h264_get_rate_ctl(struct h264_device *p_h264)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_GET_RATE_CTL, 0, 0);
    }
    return RET_ERR;
}

int32 h264_set_rate_ctl_byte(struct h264_device *p_h264,uint32_t byte)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_RATE_CTL_BYTE, byte, 0);
    }
    return RET_ERR;
}

int32 h264_set_rate_ctl_rdcost(struct h264_device *p_h264,uint32_t rdcost)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_RATE_CTL_RDCOST, rdcost, 0);
    }
    return RET_ERR;
}

int32 h264_set_flt_3dnr(struct h264_device *p_h264,uint32_t dnr)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_SET_FLT_3DNR, dnr, 0);
    }
    return RET_ERR;
}

int32 h264_get_flt_3dnr(struct h264_device *p_h264)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_GET_FLT_3DNR, 0, 0);
    }
    return RET_ERR;
}

int32 h264_set_y_crop(struct h264_device *p_h264,uint32_t enable,uint32_t linenum)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_Y_CROP, enable, linenum);
    }
    return RET_ERR;
}

int32 h264_set_mb_pipe(struct h264_device *p_h264,uint32_t pipe)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_MB_PIPE, pipe, 0);
    }
    return RET_ERR;
}

int32 h264_set_flt_3dnr_lut(struct h264_device *p_h264,uint32_t num,uint32_t param)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_FLT_3DNR_LUT, num, param);
    }
    return RET_ERR;
}

int32 h264_set_flt_min_lut(struct h264_device *p_h264,uint32_t num,uint32_t param)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_FLT_MIN_LUT, num, param);
    }
    return RET_ERR;
}

int32 h264_set_flt_max_lut(struct h264_device *p_h264,uint32_t num,uint32_t param)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_FLT_MAX_LUT, num, param);
    }
    return RET_ERR;
}


int32 h264_set_flt_3dnr_sf(struct h264_device *p_h264,uint32_t param)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_FLT_3DNR_SF, param, 0);
    }
    return RET_ERR;
}

int32 h264_set_flt_3dnr_con(struct h264_device *p_h264,uint32_t param)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_FLT_3DNR_CON, param, 0);
    }
    return RET_ERR;
}

int32 h264_set_flt_3dnr_con1(struct h264_device *p_h264,uint32_t param)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_FLT_3DNR_CON1, param, 0);
    }
    return RET_ERR;
}

int32 h264_get_bandwidth_wr(struct h264_device *p_h264)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_GET_BANDWIDTH_WR, 0, 0);
    }
    return RET_ERR;
}

int32 h264_get_bandwidth_rd(struct h264_device *p_h264)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_GET_BANDWIDTH_RD, 0, 0);
    }
    return RET_ERR;
}

int32 h264_get_cyc_done_num(struct h264_device *p_h264)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_GET_DBG_CYC_DONE_NUM, 0, 0);
    }
    return RET_ERR;
}

int32 h264_get_frame_len(struct h264_device *p_h264)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_GET_FRM_LEN, 0, 0);
    }
    return RET_ERR;
}

int32 h264_get_dec_del(struct h264_device *p_h264)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_GET_DEC_DEL, 0, 0);
    }
    return RET_ERR;
}

int32 h264_set_sta(struct h264_device *p_h264,uint32_t sta)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_SET_STA, sta, 0);
    }
    return RET_ERR;
}

int32 h264_get_sta(struct h264_device *p_h264)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_GET_STA, 0, 0);
    }
    return RET_ERR;
}

int32 h264_get_enc_frame_rdcost(struct h264_device *p_h264)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_GET_ENC_FRM_RDCOST, 0, 0);
    }
    return RET_ERR;
}

int32 h264_set_enc_frame_rdcost(struct h264_device *p_h264)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_DEC_CLR_ENC_FUNCS, 0, 0);
    }
    return RET_ERR;
}

int32 h264_set_dec_headb_num(struct h264_device *p_h264,uint32_t dec_num)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_SET_DEC_HEADB_NUM, dec_num, 0);
    }
    return RET_ERR;
}

int32 h264_set_oe_select(struct h264_device *p_h264,uint8_t enable,uint8_t oe)
{
	if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
		return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_SET_OE_SELECT, enable, oe);
	}
	return RET_ERR;
}


int32 h264_get_mbl_calc(struct h264_device *p_h264)
{
	if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
		return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_GET_MBL_CALC, 0, 0);
	}
	return RET_ERR;
}

int32 h264_get_mbl_calc_max(struct h264_device *p_h264)
{
	if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
		return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_GET_MBL_CALC_MAX, 0, 0);
	}
	return RET_ERR;
}


int32 h264_is_err(struct h264_device *p_h264)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_IS_ERR, 0, 0);
    }
    return RET_ERR;
}

int32 h264_set_err(struct h264_device *p_h264,uint32_t err)
{
    if (p_h264 && ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl) {
        return ((const struct h264_hal_ops *)p_h264->dev.ops)->ioctl(p_h264, H264_IOCTL_SET_ERR, err, 0);
    }
    return RET_ERR;
}
