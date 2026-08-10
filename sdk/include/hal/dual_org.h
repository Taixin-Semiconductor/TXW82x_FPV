#ifndef HAL_DUAL_H
#define HAL_DUAL_H
#ifdef __cplusplus
extern "C" {
#endif

/* User interrupt handle */
typedef int32(*dual_irq_hdl)(uint32 irq_flags, uint32 irq_data, uint32 param);

enum dual_ioctl_cmd {
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DUAL_IOCTL_SET_INPUT_TYPE,	
	
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DUAL_IOCTL_SET_WORK_MODE,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DUAL_IOCTL_SET_INPUT_NUM,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DUAL_IOCTL_SET_OPEN_HDR,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DUAL_IOCTL_SET_TIMER_CFG,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DUAL_IOCTL_SET_SIZE_CFG,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DUAL_IOCTL_SET_FS_TRIG,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DUAL_IOCTL_SET_RD_TRIG,
	
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DUAL_IOCTL_WR_CNT_CFG,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DUAL_IOCTL_SET_PSRAM_ADDR,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DUAL_IOCTL_SAVE_CFG,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DUAL_IOCTL_RECOVER_CFG,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DUAL_IOCTL_SAVE_STA,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	DUAL_IOCTL_KICK_READ,
};




struct dual_device {
    struct dev_obj dev;
};

struct dual_hal_ops {
	struct devobj_ops ops;
	int32(*init)(struct dual_device *dual_dev);
	int32(*deinit)(struct dual_device *dual_dev);
    int32(*open)(struct dual_device *dual_dev);
    int32(*close)(struct dual_device *dual_dev);
    int32(*ioctl)(struct dual_device *dual_dev, enum dual_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct dual_device *dual_dev, uint32 irq_flag, dual_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct dual_device *dual_dev, uint32 irq_flag);
};

int32 dual_init(struct dual_device *p_dual);
int32 dual_deinit(struct dual_device *p_dual);
int32 dual_open(struct dual_device *p_dual);
int32 dual_close(struct dual_device *p_dual);
int32 dual_request_irq(struct dual_device *p_dual, uint32 irq_flags, dual_irq_hdl irq_hdl, uint32 irq_data);
int32 dual_release_irq(struct dual_device *p_dual, uint32 irq_flags);
int32 dual_input_type(struct dual_device *p_dual,uint8 src,uint8 type);
int32 dual_work_mode(struct dual_device *p_dual,uint8 mode);
int32 dual_input_src_num(struct dual_device *p_dual,uint8 num);
int32 dual_open_hdr(struct dual_device *p_dual,uint8 en);
int32 dual_timer_cfg(struct dual_device *p_dual,uint32_t fs_clk_num,uint32_t hs_timer,uint32_t vs_timer);
int32 dual_size_cfg(struct dual_device *p_dual,uint32_t src0_w,uint32_t src1_w,uint32_t src0_raw_num,uint32_t src1_raw_num);
int32 dual_fs_trig(struct dual_device *p_dual,uint32 fs_trig_num);
int32 dual_rd_trig(struct dual_device *p_dual,uint32 rd0_trig,uint32 rd1_trig);
int32 dual_wr_cnt_cfg(struct dual_device *p_dual,uint32 src0_w,uint32 src0_h,uint32 src1_w,uint32 src1_h,uint32 src0_raw_num,uint32 src1_raw_num);
int32 dual_set_addr(struct dual_device *p_dual,uint32 psram_Src0, uint32 psram_Src1);
int32 dual_save_cfg(struct dual_device *p_dual, uint32 channel, uint32 img_size);
int32 dual_recover_cfg(struct dual_device *p_dual);
int32 dual_save_addr_cfg(struct dual_device *p_dual);
int32 dual_save_status(struct dual_device *p_dual);
int32 dual_kick_read(struct dual_device *p_dual);

#ifdef __cplusplus
}
#endif
#endif
