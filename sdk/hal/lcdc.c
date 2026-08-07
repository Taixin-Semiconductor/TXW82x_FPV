#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/lcdc.h"
#include "osal/string.h"
int32 lcdc_init(struct lcdc_device *p_lcdc)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->init) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->init(p_lcdc);
    }
    return RET_ERR;
}

int32 lcdc_deinit(struct lcdc_device *p_lcdc)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->deinit) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->deinit(p_lcdc);
    }
    return RET_ERR;
}

int32 lcdc_open(struct lcdc_device *p_lcdc)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->open) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->open(p_lcdc);
    }
    return RET_ERR;
}

int32 lcdc_close(struct lcdc_device *p_lcdc)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->close) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->close(p_lcdc);
    }
    return RET_ERR;
}

int32 lcdc_request_irq(struct lcdc_device *p_lcdc, uint32 irq_flags, lcdc_irq_hdl irq_hdl, uint32 irq_data)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->request_irq) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->request_irq(p_lcdc, irq_flags, irq_hdl, irq_data);
    }
    return RET_ERR;
}

int32 lcdc_release_irq(struct lcdc_device *p_lcdc, uint32 irq_flags)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->release_irq) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->release_irq(p_lcdc, irq_flags);
    }
    return RET_ERR;
}

int32 lcdc_set_baudrate(struct lcdc_device *p_lcdc, uint32 mclk)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->baudrate) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->baudrate(p_lcdc, mclk);
    }
    return RET_ERR;

}

int32 lcdc_set_color_mode(struct lcdc_device *p_lcdc, enum lcdc_color_mode cm)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_COLOR_MODE, cm, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_bus_width(struct lcdc_device *p_lcdc, enum lcdc_bus_width bw)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_BUS_MODE, bw, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_interface(struct lcdc_device *p_lcdc, enum lcdc_interface_mode im)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_INTERFACE_MODE, im, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_colrarray(struct lcdc_device *p_lcdc, uint8 colrarray)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_COLRARRAY, colrarray, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_p0p1_enable(struct lcdc_device *p_lcdc, uint8 p0_en,uint8 p1_en){
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_ROTATE_P0_P1_EN, p0_en, p1_en);
	}
	return RET_ERR;

}

int32 lcdc_set_lcd_vaild_size(struct lcdc_device *p_lcdc, uint32 w,uint32 h,uint8 pixel_dot_num)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_LCD_VAILD_SIZE, (w<<16)|h, pixel_dot_num);
    }
    return RET_ERR;
}

int32 lcdc_set_lcd_visible_size(struct lcdc_device *p_lcdc, uint32 w,uint32 h,uint8 pixel_dot_num)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_LCD_VISIBLE_SIZE, (w<<16)|h, pixel_dot_num);
	}
	return RET_ERR;
}

int32 lcdc_signal_config(struct lcdc_device *p_lcdc, uint8 vs_en,uint8 hs_en,uint8 de_en,uint8 vs_inv,uint8 hs_inv,uint8 de_inv,uint8 clk_inv)
{
	uint8 arg = 0;
	arg  =  (  vs_en<<(0));
	arg  |= (  hs_en<<(1));
	arg  |= (  de_en<<(2));
	arg  |= ( vs_inv<<(3));
	arg  |= ( hs_inv<<(4));
	arg  |= ( de_inv<<(5));
	arg  |= (clk_inv<<(6));
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_SIGNAL_CONFIG, arg, 0);
	}
	return RET_ERR;
}


int32 lcdc_mcu_signal_config(struct lcdc_device *p_lcdc, uint8 value)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_MCU_SIGNAL_CONFIG, value, 0);
	}
	return RET_ERR;
}

int32 lcdc_set_invalid_line(struct lcdc_device *p_lcdc, uint8 invalid_line)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_INVALID_LINE, invalid_line, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_valid_dot(struct lcdc_device *p_lcdc, uint32 w_start,uint32 h_start)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_VALID_DOT, w_start, h_start);
    }
    return RET_ERR;
}

int32 lcdc_set_hlw_vlw(struct lcdc_device *p_lcdc, uint32 hlw,uint32 vlw)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_HLW_VLW, hlw, vlw);
    }
    return RET_ERR;
}

int32 lcdc_set_bigendian(struct lcdc_device *p_lcdc, uint8 en)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_VIDEO_BIG_ENDIAN, en, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_video_size(struct lcdc_device *p_lcdc, uint32 w,uint32 h)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_VIDEO_SIZE, w, h);
    }
    return RET_ERR;
}

int32 lcdc_set_p0_rotate_y_src_addr(struct lcdc_device *p_lcdc, uint32 yaddr)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_P0_ROTATE_Y_SRC_ADDR, yaddr, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_p0_rotate_u_src_addr(struct lcdc_device *p_lcdc, uint32 uaddr)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_P0_ROTATE_U_SRC_ADDR, uaddr, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_p0_rotate_v_src_addr(struct lcdc_device *p_lcdc, uint32 vaddr)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_P0_ROTATE_V_SRC_ADDR, vaddr, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_p1_rotate_y_src_addr(struct lcdc_device *p_lcdc, uint32 yaddr)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_P1_ROTATE_Y_SRC_ADDR, yaddr, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_p1_rotate_u_src_addr(struct lcdc_device *p_lcdc, uint32 uaddr)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_P1_ROTATE_U_SRC_ADDR, uaddr, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_p1_rotate_v_src_addr(struct lcdc_device *p_lcdc, uint32 vaddr)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_P1_ROTATE_V_SRC_ADDR, vaddr, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_rotate_p0_up(struct lcdc_device *p_lcdc, uint8 en)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_ROTATE_P0_UP, en, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_rotate_p0p1_start_location(struct lcdc_device *p_lcdc, uint32 x0,uint32 y0,uint32 x1,uint32 y1)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_P0P1_ROTATE_START_ADDR, (y0<<16)|x0, (y1<<16)|x1);
	}
	return RET_ERR;
}

int32 lcdc_set_rotate_linebuf_num(struct lcdc_device *p_lcdc, uint32 linebuf)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_ROTATE_LINE_BUF_NUM, linebuf, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_rotate_linebuf_y_addr(struct lcdc_device *p_lcdc, uint32 yaddr)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_ROTATE_LINE_BUF_Y, yaddr, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_rotate_linebuf_u_addr(struct lcdc_device *p_lcdc, uint32 uaddr)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_ROTATE_LINE_BUF_U, uaddr, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_rotate_linebuf_v_addr(struct lcdc_device *p_lcdc, uint32 vaddr)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_ROTATE_LINE_BUF_V, vaddr, 0);
    }
    return RET_ERR;
}


int32 lcdc_set_rotate_mirror(struct lcdc_device *p_lcdc, uint8 mirror,enum lcdc_rotate_mode rotate)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_ROTATE_MIRROR, mirror, rotate);
    }
    return RET_ERR;
} 

int32 lcdc_set_rotate_p0p1_size(struct lcdc_device *p_lcdc, uint32 w0,uint32 h0,uint32 w1,uint32 h1)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_ROTATE_SET_SIZE, (w0<<16)|h0, (w1<<16)|h1);
    }
    return RET_ERR;
}

int32 lcdc_set_video_data_from(struct lcdc_device *p_lcdc, enum lcdc_video_mode vm)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_VIDEO_MODE, vm, 0);
	}
	return RET_ERR;
}

int32 lcdc_set_video_start_location(struct lcdc_device *p_lcdc, uint32 x,uint32 y)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_VIDEO_START_ADDR, (y<<16)|x, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_video_en(struct lcdc_device *p_lcdc, uint8 en)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_VIDEO_EN, en, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_osd_size(struct lcdc_device *p_lcdc, uint32 w,uint32 h)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_OSD_SIZE, (h<<16)|w, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_osd_start_location(struct lcdc_device *p_lcdc, uint32 x,uint32 y)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_OSD_START_ADDR, y, x);
    }
    return RET_ERR;
}

int32 lcdc_set_osd_dma_addr(struct lcdc_device *p_lcdc, uint32 addr)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_OSD_DMA_ADDR, addr, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_osd_lut_addr(struct lcdc_device *p_lcdc, uint32 addr)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_OSD_LUTDMA_ADDR, addr, 0);
	}
	return RET_ERR;
}


int32 lcdc_set_osd_dma_len(struct lcdc_device *p_lcdc, uint32 len)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_OSD_DMA_LEN, len, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_osd_format(struct lcdc_device *p_lcdc, enum lcdc_osd_format osd_format)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_OSD_FORMAT, osd_format, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_osd_alpha(struct lcdc_device *p_lcdc, uint32 alpha)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_OSD_ALPHA, alpha, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_osd_enc_head(struct lcdc_device *p_lcdc, uint32 head, uint32 head_tran)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_OSD_ENC_HEAD, head, head_tran);
    }
    return RET_ERR;
}

int32 lcdc_set_osd_enc_diap(struct lcdc_device *p_lcdc, uint32 diap, uint32 diap_tran)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_OSD_ENC_DIAP, diap, diap_tran);
    }
    return RET_ERR;
}

int32 lcdc_set_osd_enc_cfg(struct lcdc_device *p_lcdc, uint32 head, uint32 diap)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_OSD_ENC_CFG, head, diap);
    }
    return RET_ERR;
}

int32 lcdc_set_osd_en(struct lcdc_device *p_lcdc, uint8 en)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_OSD_EN, en, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_osd_enc_src_addr(struct lcdc_device *p_lcdc, uint32 addr)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_OSD_ENC_SRC_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 lcdc_set_osd_enc_dst_addr(struct lcdc_device *p_lcdc, uint32 addr)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_OSD_ENC_DST_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 lcdc_set_osd_enc_src_len(struct lcdc_device *p_lcdc, uint32 len)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_OSD_ENC_SRC_LEN, len, 0);
	}
	return RET_ERR;
}

int32 lcdc_get_osd_enc_dst_len(struct lcdc_device *p_lcdc)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_GET_OSD_ENC_DST_LEN, 0, 0);
	}
	return RET_ERR;
}

int32 lcdc_osd_enc_start_run(struct lcdc_device *p_lcdc)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_OSD_ENC_START, 0, 0);
    }
    return RET_ERR;
}

int32 lcdc_set_start_run(struct lcdc_device *p_lcdc)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_START_RUN, 0, 0);
    }
    return RET_ERR;
}


int32 lcdc_mcu_write_reg(struct lcdc_device *p_lcdc,uint8 data)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_MCU_CPU_WRITE, data, 0);
    }
    return RET_ERR;
}

int32 lcdc_mcu_write_data(struct lcdc_device *p_lcdc,uint8 data)
{
    if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
        return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_MCU_CPU_WRITE, data, 1);
    }
    return RET_ERR;
}

int32 lcdc_reg_write_data(struct lcdc_device *p_lcdc,uint8 cmdlen,uint8 reglen, uint8 *buf)
{
	uint8_t data_buf[64];
	data_buf[0] = cmdlen;
	data_buf[1] = reglen;
	memcpy(data_buf+2,buf,reglen+cmdlen);
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_REG_CPU_WRITE, (uint32)data_buf, 1);
	}
	return RET_ERR;
}

int32 lcdc_reg_read_data(struct lcdc_device *p_lcdc,uint8 cmdlen,uint8 reglen, uint8 *buf)
{
	//uint8_t data_buf[64];
	//data_buf[0] = cmdlen;
	//data_buf[1] = reglen;
	//memcpy(data_buf+2,buf,reglen+cmdlen);
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_MCU_CPU_READ, (uint32)buf, 1);
	}
	return RET_ERR;
}


int32 lcdc_video_enable_constrast_saturation(struct lcdc_device *p_lcdc, uint8 constrast,uint8 saturation)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_CONSTRAST_SATURATION_EN, constrast, saturation);
	}
	return RET_ERR;
}

int32 lcdc_video_constrast_saturation_val(struct lcdc_device *p_lcdc, uint32 constrast,uint32 saturation)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_CONSTRAST_SATURATION_VAL, constrast, saturation);
	}
	return RET_ERR;
}

int32 lcdc_video_enable_ccm_gamma(struct lcdc_device *p_lcdc, uint8 ccm, uint8 gamma)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_CCM_GAMMA_EN, ccm, gamma);
	}
	return RET_ERR;
}

int32 lcdc_video_enable_auto_ks(struct lcdc_device *p_lcdc, uint8 enable)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_AUTO_KICK_EN, enable,0);
	}
	return RET_ERR;
}

int32 lcdc_set_timeout_info(struct lcdc_device *p_lcdc, uint8 enable,uint8 timeout)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_TIMEOUT_INF, enable,timeout);
	}
	return RET_ERR;
}

int32 lcdc_rot_burst_dw16(struct lcdc_device *p_lcdc, uint8 dw16)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_ROT_PSRAM_BURST_SW, dw16,0);
	}
	return RET_ERR;
}

int32 lcdc_qspi_cmd_dw4(struct lcdc_device *p_lcdc, uint8 dw4)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_QSPI_DW4, dw4,0);
	}
	return RET_ERR;
}

int32 lcdc_cmd_auto_send(struct lcdc_device *p_lcdc, uint32 cmd0,  uint32 cmd1)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_TXCMD_AUTO, cmd0,cmd1);
	}
	return RET_ERR;
}

int32 lcdc_spi_cs_lock_time(struct lcdc_device *p_lcdc, uint8 cs_setup,uint8 cs_hold,uint8 cs_high)
{
	uint32_t cs_time[1];
	uint8_t *p8;
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		p8 = (uint8_t *)cs_time;
		p8[0] = cs_setup;
		p8[1] = cs_hold;
		p8[2] = cs_high;
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_CS_LOCK_TIME, (uint32)cs_time,0);
	}
	return RET_ERR;
}

int32 lcdc_spi_dummy_en(struct lcdc_device *p_lcdc, uint8 en)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_SPI_DUMMY_EN, en,0);
	}
	return RET_ERR;
}

int32 lcdc_dsi_tear_request(struct lcdc_device *p_lcdc, uint8 en)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_DSI_TEAR_REQUEST, en,0);
	}
	return RET_ERR;
}

int32 lcdc_dsi_reg_update(struct lcdc_device *p_lcdc, uint8 en)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_DSI_REG_UPDATE, en,0);
	}
	return RET_ERR;
}

int32 lcdc_dsi_dpi_colorm(struct lcdc_device *p_lcdc, uint8 en)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_DSI_DPI_COLORM, en,0);
	}
	return RET_ERR;
}

int32 lcdc_dsi_dpi_shutdown(struct lcdc_device *p_lcdc, uint8 en)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_DSI_DPI_SHUT_DN, en,0);
	}
	return RET_ERR;
}

int32 lcdc_dsi_select_edpi_or_dpi(struct lcdc_device *p_lcdc, uint8 edpi)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_EDSI_ITF_SELECT, edpi,0);
	}
	return RET_ERR;
}

int32 lcdc_clock_alway_on(struct lcdc_device *p_lcdc, uint8 en)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_CLK_ALWAY_ON, en,0);
	}
	return RET_ERR;
}

int32 lcdc_gamma_rgb_table(struct lcdc_device *p_lcdc, uint32 radr,uint32 gadr,uint32 badr)
{
	uint32 gamma_tbl[3];
	gamma_tbl[0] = radr;
	gamma_tbl[1] = gadr;
	gamma_tbl[2] = badr;
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_GAMMA_RGB_ADR, (uint32)gamma_tbl,0);
	}
	return RET_ERR;
}

int32 lcdc_te_edge_cfg(struct lcdc_device *p_lcdc, uint8 pullup)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_TE_EDGE_CFG, pullup,0);
	}
	return RET_ERR;
}

int32 lcdc_osd_enc_en(struct lcdc_device *p_lcdc, uint8 en)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_OSD_ENC_EN, en,0);
	}
	return RET_ERR;
}

int32 lcdc_osd_trans_en(struct lcdc_device *p_lcdc, uint8 en)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_OSD_TRANS_EN, en,0);
	}
	return RET_ERR;
}

int32 lcdc_dither_linebuf(struct lcdc_device *p_lcdc, uint32 lineaddr)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_DITHER_LINEBUF, lineaddr,0);
	}
	return RET_ERR;
}

int32 lcdc_dither_en(struct lcdc_device *p_lcdc, uint8 en)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SET_DITHER_EN, en,0);
	}
	return RET_ERR;
}

int32 lcdc_clk_gather_select(struct lcdc_device *p_lcdc, uint8 pull)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_CLK_GATHER, pull,0);
	}
	return RET_ERR;
}

int32 lcdc_screen_start(struct lcdc_device *p_lcdc, uint8 en)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SCREEN_SHOT_START, en,0);
	}
	return RET_ERR;
}

int32 lcdc_screen_format(struct lcdc_device *p_lcdc, uint8 format)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SCREEN_SHOT_FORMAT, format,0);
	}
	return RET_ERR;
}

int32 lcdc_screen_yuv_addr(struct lcdc_device *p_lcdc, uint32 yadr,uint32 uadr,uint32 vadr)
{
	uint32 yuv_tbl[3];
	yuv_tbl[0] = yadr;
	yuv_tbl[1] = uadr;
	yuv_tbl[2] = vadr;
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_SCREEN_YUVADR, (uint32)yuv_tbl,0);
	}
	return RET_ERR;
}

int32 lcdc_osd_sram_fifo_enable(struct lcdc_device *p_lcdc, uint8 en)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_OSD_USE_SRAM_FIFO_ENABLE, en,0);
	}
	return RET_ERR;
}

int32 lcdc_osd_sram_fifo_addr(struct lcdc_device *p_lcdc, uint32 start, uint32 end)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_OSD_SRAM_FIFO_ADDR, start,end);
	}
	return RET_ERR;
}

int32 lcdc_osd_spec_color_alpha(struct lcdc_device *p_lcdc, uint8 enable, uint32 color, uint32 alpha)
{
	uint32 color_alpha[2];
	color_alpha[0] = color;
	color_alpha[1] = alpha;	
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_OSD_USE_SPEC_COLOR_ALPHA, enable,(uint32)color_alpha);
	}
	return RET_ERR;
}

int32 lcdc_get_clk_status(struct lcdc_device *p_lcdc)
{
	if (p_lcdc && ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl) {
		return ((const struct lcdc_hal_ops *)p_lcdc->dev.ops)->ioctl(p_lcdc, LCDC_CLK_STATUS_IS_OK, 0,0);
	}
	return RET_ERR;
}


