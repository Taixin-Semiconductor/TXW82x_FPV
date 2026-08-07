#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/dsi.h"


int32 dsi_init(struct dsi_device *p_dsi,uint8 clk_div,uint8_t tx_esc_clk_div,uint8_t auto_clklane,uint8_t lane_num)
{
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->init) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->init(p_dsi,clk_div,tx_esc_clk_div,auto_clklane,lane_num);
    }
    return RET_ERR;
}

int32 dsi_deinit(struct dsi_device *p_dsi)
{
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->deinit) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->deinit(p_dsi);
    }
    return RET_ERR;
}

int32 dsi_open(struct dsi_device *p_dsi)
{
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->open) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->open(p_dsi);
    }
    return RET_ERR;
}

int32 dsi_close(struct dsi_device *p_dsi)
{
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->close) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->close(p_dsi);
    }
    return RET_ERR;
}

int32 mipi_dsi_set_lane_num(struct dsi_device *p_dsi,uint8 lane_num){
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_LANE_NUM, lane_num,0);
    }
    return RET_ERR;
}

int32 mipi_dsi_max_clk_time(struct dsi_device *p_dsi,uint32 max_clk){
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_MAX_CLK_TIM, max_clk,0);
    }
    return RET_ERR;
}

int32 mipi_dsi_remain_stop_state(struct dsi_device *p_dsi,uint8 state){
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_REMAIN_STOP_STATE, state,0);
    }
    return RET_ERR;
}

int32 mipi_dsi_cfg_enable(struct dsi_device *p_dsi,uint8_t htxeotp_en,uint8_t rxeotp_en,uint8_t bta_en,uint8_t ecc_en,uint8_t crc_en,uint8_t ltxeotp_en){
	uint32_t enablebuf[2];
	uint8_t *p8;
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
		p8 = (uint8_t *)enablebuf;
		p8[0] = htxeotp_en;
		p8[1] = rxeotp_en;
		p8[2] = bta_en;
		p8[3] = ecc_en;
		p8[4] = crc_en;
		p8[5] = ltxeotp_en;
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_CFG_ENABLE, (uint32)enablebuf,0);
    }
    return RET_ERR;
}

int32 mipi_dsi_ulps_ctrl(struct dsi_device *p_dsi,uint8_t indatulps,uint8_t outdatulps,uint8_t inclkulps,uint8_t outclkulps){
	uint32_t ulpsbuf[1];
	uint8_t *p8;
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
		p8 = (uint8_t *)ulpsbuf;
		p8[0] = indatulps;
		p8[1] = outdatulps;
		p8[2] = inclkulps;
		p8[3] = outclkulps;
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_ULPS_CTRL, (uint32)ulpsbuf,0);
    }
    return RET_ERR;
}

int32 mipi_dsi_ulps_min_time(struct dsi_device *p_dsi,uint8 time){
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_ULPS_MIN_TIME, time,0);
    }
    return RET_ERR;
}

int32 mipi_dsi_ulps_entry_delay_time(struct dsi_device *p_dsi,uint32 time){
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_ULPS_ENTRY_DELAY_TIME, time,0);
    }
    return RET_ERR;
}

int32 mipi_dsi_ulps_wakeup_time(struct dsi_device *p_dsi,uint16 twakeup_clk,uint16 twakeup_cnt){
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_ULPS_WAKEUP_TIME, twakeup_clk,twakeup_cnt);
    }
    return RET_ERR;
}

int32 mipi_dsi_ulps_mode(struct dsi_device *p_dsi,uint8_t pll_off,uint8_t auto_ulps,uint8_t pre_pll_off_req){
	uint32_t ulpsbuf[1];
	uint8_t *p8;
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
		p8 = (uint8_t *)ulpsbuf;
		p8[0] = pll_off;
		p8[1] = auto_ulps;
		p8[2] = pre_pll_off_req;
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_ULPS_MODE, (uint32)ulpsbuf,0);
    }
    return RET_ERR;
}

int32 mipi_dsi_select_mode(struct dsi_device *p_dsi,uint8 cmd){
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_CMD_VIDEO_SELECT_MODE, cmd,0);
    }
    return RET_ERR;
}

int32 mipi_dsi_gen_vcid_cfg(struct dsi_device *p_dsi,uint8_t vcid,uint8_t vcid_tear_auto,uint8_t vcid_tx_auto){
	uint8_t ulpsbuf[3];
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
		ulpsbuf[0] = vcid;
		ulpsbuf[1] = vcid_tear_auto;
		ulpsbuf[2] = vcid_tx_auto;
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_GEN_VCID_CFG, (uint32)ulpsbuf,0);
    }
    return RET_ERR;
}

int32 mipi_dsi_cmd_mode_cfg(struct dsi_device *p_dsi,uint8_t te_ack,uint8_t ack,uint8_t sw0p_tx,uint8_t sw1p_tx,uint8_t sw2p_tx,uint8_t sr0p_tx,uint8_t sr1p_tx,uint8_t sr2p_tx,
							uint8_t gen_lw_tx,uint8_t dcs_sw0p_tx,uint8_t dcs_sw1p_tx,uint8_t dcs_sr0p_tx,uint8_t dcs_lw_tx,uint8_t max_rd_pkt_size){
	uint32_t cmdbuf[4];
	uint8_t *p8;
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
		p8 = (uint8_t *)cmdbuf;
		p8[0] = te_ack;
		p8[1] = ack;
		p8[2] = sw0p_tx;
		p8[3] = sw1p_tx;
		p8[4] = sw2p_tx;
		p8[5] = sr0p_tx;
		p8[6] = sr1p_tx;
		p8[7] = sr2p_tx;
		p8[8] = gen_lw_tx;
		p8[9] = dcs_sw0p_tx;
		p8[10] = dcs_sw1p_tx;
		p8[11] = dcs_sr0p_tx;
		p8[12] = dcs_lw_tx;
		p8[13] = max_rd_pkt_size;
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_CMD_MODE_CFG, (uint32)cmdbuf,0);
    }
    return RET_ERR;
}
							

int32 mipi_dsi_dpi_vcid(struct dsi_device *p_dsi,uint8 vcid){
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_DPI_VCID, vcid,0);
    }
    return RET_ERR;	
}							

int32 mipi_dsi_color_cfg(struct dsi_device *p_dsi,uint8_t color_mode,uint8_t loosely18_en){
	uint32_t colorbuf[1];
	uint8_t *p8;
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
		p8 = (uint8_t *)colorbuf;
		p8[0] = color_mode;
		p8[1] = loosely18_en;		
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_COLOR_CFG, (uint32)colorbuf,0);
    }
    return RET_ERR;	
}							

int32 mipi_dsi_cfg_pol(struct dsi_device *p_dsi,uint8_t dataen_active,uint8_t vsync_active,uint8_t hsync_active,uint8_t shutd_active,uint8_t colorm_active){
	uint32_t polbuf[2];
	uint8_t *p8;
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
		p8 = (uint8_t *)polbuf;
		p8[0] = dataen_active;
		p8[1] = vsync_active;
		p8[2] = hsync_active;
		p8[3] = shutd_active;
		p8[4] = colorm_active;		
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_CFG_POL, (uint32)polbuf,0);
    }
    return RET_ERR;	
}

int32 mipi_dsi_vid_mode(struct dsi_device *p_dsi,uint8_t vsa_en,uint8_t vbp_en,uint8_t vfp_en,uint8_t vact_en,uint8_t hbp_en,uint8_t hfp_en,uint8_t frame_bta_ack_en,uint8_t cmd_en,uint8_t vid_mode_type){
	uint32_t vidbuf[3];
	uint8_t *p8;
	if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
		p8 = (uint8_t *)vidbuf;
		p8[0] = vsa_en;
		p8[1] = vbp_en;
		p8[2] = vfp_en;
		p8[3] = vact_en;
		p8[4] = hbp_en;		
		p8[5] = hfp_en;
		p8[6] = frame_bta_ack_en;
		p8[7] = cmd_en;
		p8[8] = vid_mode_type;
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_VID_MODE, (uint32)vidbuf,0);
    }
    return RET_ERR;	
}

int32 mipi_dsi_vid_pkt_size(struct dsi_device *p_dsi,uint32 size){
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_VID_PKT_SIZE, size,0);
    }
    return RET_ERR;	
}	

int32 mipi_dsi_vid_chunk_num(struct dsi_device *p_dsi,uint32 chunk_num){
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_VID_CHUNK_NUM, chunk_num,0);
    }
    return RET_ERR;	
}

int32 mipi_dsi_vid_null_size(struct dsi_device *p_dsi,uint32 null_num){
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_VID_NULL_SIZE, null_num,0);
    }
    return RET_ERR;	
}

int32 mipi_dsi_vid_cfg(struct dsi_device *p_dsi,uint32 dpi2laneclkratio,uint32_t vid_vsa,uint32_t vid_vbp,uint32_t vid_vactive,uint32_t w,uint32_t vid_vfp,uint8_t hsa,uint8_t hbp,uint8_t hfp){
	uint32_t vidbuf[9];
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
		vidbuf[0] = dpi2laneclkratio;
		vidbuf[1] = vid_vsa;
		vidbuf[2] = vid_vbp;
		vidbuf[3] = vid_vactive;
		vidbuf[4] = w;		
		vidbuf[5] = vid_vfp;
		vidbuf[6] = hsa;
		vidbuf[7] = hbp;
		vidbuf[8] = hfp;
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_VID_CFG, (uint32)vidbuf,0);
    }
    return RET_ERR;	
}

int32 mipi_dsi_lpcmd_tmr(struct dsi_device *p_dsi,uint16_t invact_lptime,uint16_t outvact_lptime){
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_LPCMD_TMR, invact_lptime,outvact_lptime);
    }
    return RET_ERR;	
}

int32 mipi_dsi_edpi_cfg(struct dsi_device *p_dsi,uint16_t edpi_maxpkt,uint8_t hw_te_on,uint8_t hw_te_gen,uint8_t hw_scan_line,uint16_t line_parameter){
	uint32_t edpibuf[5];
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
		edpibuf[0] = edpi_maxpkt;
		edpibuf[1] = hw_te_on;
		edpibuf[2] = hw_te_gen;
		edpibuf[3] = hw_scan_line;
		edpibuf[4] = line_parameter;		
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_EDPI_CFG, (uint32)edpibuf,0);
    }
    return RET_ERR;	
}

int32 mipi_dsi_phy_rst(struct dsi_device *p_dsi){
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_DPHY_RST, 0,0);
    }
    return RET_ERR;	
}


int32 mipi_dsi_set_pixel_sram_fifo(struct dsi_device *p_dsi,uint32 gen_addr){
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_PIXEL_GENERIC_SRAM_FIFO,gen_addr,0);
    }
    return RET_ERR;	
}

int32 mipi_dsi_set_fifo_depth(struct dsi_device *p_dsi,uint32 depth){
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_FIFO_DEPTH, depth,0);
    }
    return RET_ERR;	
}

int32 mipi_dsi_cmd_pkt_fifo_empty(struct dsi_device *p_dsi){
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_CMD_PKT_FIFO_EMPTY, 0,0);
    }
    return RET_ERR;	
}

int32 mipi_dsi_wait_pkt_fifo_full(struct dsi_device *p_dsi){
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_WAIT_PKT_FIFO_FULL, 0,0);
    }
    return RET_ERR;	
}

int32 mipi_dsi_gen_pld_write_full(struct dsi_device *p_dsi){
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_GEN_PLD_WRITE_FULL, 0,0);
    }
    return RET_ERR;	
}

int32 mipi_dsi_gen_pld_read_empty(struct dsi_device *p_dsi){
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_GEN_PLD_READ_EMPTY, 0,0);
    }
    return RET_ERR;	
}

int32 mipi_dsi_cmd_cfg(struct dsi_device *p_dsi,uint8_t cmdid,uint8_t cmd,uint8_t vcid,uint8_t dat){
	uint32_t cmdbuf[1];
	uint8_t *p8;
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
		p8 = (uint8_t *)cmdbuf;
		p8[0] = dat;
		p8[1] = cmd;
		p8[2] = vcid;
		p8[3] = cmdid;
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_CMD_CFG, (uint32)cmdbuf,0);
    }
    return RET_ERR;	
}

int32 mipi_dsi_set_gen_pld_data(struct dsi_device *p_dsi,uint8_t param0,uint8_t param1,uint8_t param2,uint8_t param3){
	uint32_t databuf[1];
	uint8_t *p8;
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
		p8 = (uint8_t *)databuf;
		p8[0] = param0;
		p8[1] = param1;
		p8[2] = param2;
		p8[3] = param3;
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_SET_GEN_PLD_DATA, (uint32)databuf,0);
    }
    return RET_ERR;	
}

int32 mipi_dsi_get_gen_pld_data(struct dsi_device *p_dsi){
    if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
        return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_GET_GEN_PLD_DATA, 0,0);
    }
    return RET_ERR;	
}


int32 mipi_dsi_get_sta0(struct dsi_device *p_dsi){
	if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
		return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_GET_STA0, 0,0);
	}
	return RET_ERR; 
}

int32 mipi_dsi_get_sta1(struct dsi_device *p_dsi){
	if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
		return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_GET_STA1, 0,0);
	}
	return RET_ERR; 
}


int32 mipi_dsi_set_lane_remap(struct dsi_device *p_dsi,uint8_t clklane,uint8_t lane0,uint8_t lane1,uint8_t lane2,uint8_t lane3,uint8_t clk_pol,uint8_t lane0_pol,uint8_t lane1_pol,uint8_t lane2_pol,uint8_t lane3_pol){
	uint32_t databuf[3];
	uint8_t *p8;
	if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
		p8 = (uint8_t *)databuf;
		p8[0] = clklane;
		p8[1] = lane0;
		p8[2] = lane1;
		p8[3] = lane2;
		p8[4] = lane3;
		p8[5] = clk_pol;
		p8[6] = lane0_pol;
		p8[7] = lane1_pol;
		p8[8] = lane2_pol;
		p8[9] = lane3_pol;
		return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_SET_IO_REMAP, (uint32)databuf,0);
	}
	return RET_ERR; 
}

int32 mipi_dsi_reset_module(struct dsi_device *p_dsi){	
	if (p_dsi && ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl) {
		return ((const struct dsi_hal_ops *)p_dsi->dev.ops)->ioctl(p_dsi, DSI_IOCTL_RESET_MODULE, 0,0);
	}
	return RET_ERR;
}
