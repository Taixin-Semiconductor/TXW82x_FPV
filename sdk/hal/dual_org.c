#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/dual_org.h"

int32 dual_init(struct dual_device *p_dual)
{
    if (p_dual && ((const struct dual_hal_ops *)p_dual->dev.ops)->init) {
        return ((const struct dual_hal_ops *)p_dual->dev.ops)->init(p_dual);
    }
    return RET_ERR;
}

int32 dual_deinit(struct dual_device *p_dual)
{
    if (p_dual && ((const struct dual_hal_ops *)p_dual->dev.ops)->deinit) {
        return ((const struct dual_hal_ops *)p_dual->dev.ops)->deinit(p_dual);
    }
    return RET_ERR;
}

int32 dual_open(struct dual_device *p_dual)
{
    if (p_dual && ((const struct dual_hal_ops *)p_dual->dev.ops)->open) {
        return ((const struct dual_hal_ops *)p_dual->dev.ops)->open(p_dual);
    }
    return RET_ERR;
}

int32 dual_close(struct dual_device *p_dual)
{
    if (p_dual && ((const struct dual_hal_ops *)p_dual->dev.ops)->close) {
        return ((const struct dual_hal_ops *)p_dual->dev.ops)->close(p_dual);
    }
    return RET_ERR;
}

int32 dual_request_irq(struct dual_device *p_dual, uint32 irq_flags, dual_irq_hdl irq_hdl, uint32 irq_data)
{
    if (p_dual && ((const struct dual_hal_ops *)p_dual->dev.ops)->request_irq) {
        return ((const struct dual_hal_ops *)p_dual->dev.ops)->request_irq(p_dual, irq_flags, irq_hdl, irq_data);
    }
    return RET_ERR;
}

int32 dual_release_irq(struct dual_device *p_dual, uint32 irq_flags)
{
    if (p_dual && ((const struct dual_hal_ops *)p_dual->dev.ops)->release_irq) {
        return ((const struct dual_hal_ops *)p_dual->dev.ops)->release_irq(p_dual, irq_flags);
    }
    return RET_ERR;
}


int32 dual_input_type(struct dual_device *p_dual,uint8 src,uint8 type){
    if (p_dual && ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl) {
        return ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl(p_dual, DUAL_IOCTL_SET_INPUT_TYPE, src, type);
    }
    return RET_ERR;
}

//0:WR0/1 + RD0/1
//1:WR0
//2:WR1
//3:RD0
//4:RD1
int32 dual_work_mode(struct dual_device *p_dual,uint8 mode){
    if (p_dual && ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl) {
        return ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl(p_dual, DUAL_IOCTL_SET_WORK_MODE, mode, 0);
    }
    return RET_ERR;
}


int32 dual_input_src_num(struct dual_device *p_dual,uint8 num){
    if (p_dual && ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl) {
        return ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl(p_dual, DUAL_IOCTL_SET_INPUT_NUM, num, 0);
    }
    return RET_ERR;
}

int32 dual_open_hdr(struct dual_device *p_dual,uint8 en){
    if (p_dual && ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl) {
        return ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl(p_dual, DUAL_IOCTL_SET_OPEN_HDR, en, 0);
    }
    return RET_ERR;
}

int32 dual_timer_cfg(struct dual_device *p_dual,uint32_t fs_clk_num,uint32_t hs_timer,uint32_t vs_timer){
	uint32_t  time_buf[3];
	if(hs_timer == 1){
		time_buf[0] = fs_clk_num/4;
		time_buf[1] = 15; //sys_240MHz, 1us/per_15
		time_buf[2] = 256;
	}else{
		time_buf[0] = fs_clk_num/4;
		time_buf[1] = hs_timer*(DEFAULT_SYS_CLK/1000000)/16;
		time_buf[2] = vs_timer*(DEFAULT_SYS_CLK/1000000)/256;
	}

    if (p_dual && ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl) {
        return ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl(p_dual, DUAL_IOCTL_SET_TIMER_CFG, (uint32)time_buf, 0);
    }
    return RET_ERR;
}

int32 dual_size_cfg(struct dual_device *p_dual,uint32_t src0_w,uint32_t src1_w,uint32_t src0_raw_num,uint32_t src1_raw_num){
	uint32_t  cfgbuf[4];
	cfgbuf[0] = src0_w;
	cfgbuf[1] = src1_w;
	cfgbuf[2] = src0_raw_num;
	cfgbuf[3] = src1_raw_num;
    if (p_dual && ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl) {
        return ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl(p_dual, DUAL_IOCTL_SET_SIZE_CFG, (uint32)cfgbuf, 0);
    }
    return RET_ERR;
}


int32 dual_fs_trig(struct dual_device *p_dual,uint32 fs_trig_num){
	if (p_dual && ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl) {
		return ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl(p_dual, DUAL_IOCTL_SET_FS_TRIG, fs_trig_num, 0);
	}
	return RET_ERR;
}

int32 dual_rd_trig(struct dual_device *p_dual,uint32 rd0_trig,uint32 rd1_trig){
	if (p_dual && ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl) {
		return ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl(p_dual, DUAL_IOCTL_SET_RD_TRIG, rd0_trig, rd1_trig);
	}
	return RET_ERR;
}

int32 dual_wr_cnt_cfg(struct dual_device *p_dual,uint32 src0_w,uint32 src0_h,uint32 src1_w,uint32 src1_h,uint32 src0_raw_num,uint32 src1_raw_num){
	uint32_t  cfgbuf[6];
	cfgbuf[0] = src0_w;
	cfgbuf[1] = src0_h;
	cfgbuf[2] = src1_w;
	cfgbuf[3] = src1_h;
	cfgbuf[4] = src0_raw_num;
	cfgbuf[5] = src1_raw_num;

	if (p_dual && ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl) {
		return ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl(p_dual, DUAL_IOCTL_WR_CNT_CFG, (uint32)cfgbuf, 0);
	}
	return RET_ERR;
}

int32 dual_set_addr(struct dual_device *p_dual,uint32 psram_Src0,uint32 psram_Src1){
	if (p_dual && ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl) {
		return ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl(p_dual, DUAL_IOCTL_SET_PSRAM_ADDR, psram_Src0, psram_Src1);
	}
	return RET_ERR;
}

int32 dual_save_cfg(struct dual_device *p_dual, uint32 channel, uint32 img_size)
{
    if (p_dual && ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl) {
		return ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl(p_dual, DUAL_IOCTL_SAVE_CFG, channel, img_size);
	}
	return RET_ERR; 
}

int32 dual_recover_cfg(struct dual_device *p_dual)
{
    if (p_dual && ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl) {
		return ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl(p_dual, DUAL_IOCTL_RECOVER_CFG, 0, 0);
	}
	return RET_ERR; 
}

int32 dual_save_status(struct dual_device *p_dual)
{
    if (p_dual && ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl) {
		return ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl(p_dual, DUAL_IOCTL_SAVE_STA, 0, 0);
	}
	return RET_ERR; 
}

int32 dual_kick_read(struct dual_device *p_dual)
{
    if (p_dual && ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl) {
		return ((const struct dual_hal_ops *)p_dual->dev.ops)->ioctl(p_dual, DUAL_IOCTL_KICK_READ, 0, 0);
	}
	return RET_ERR; 
}

