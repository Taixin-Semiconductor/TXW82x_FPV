#ifndef HAL_PRC_H
#define HAL_PRC_H

#ifdef __cplusplus
extern "C" {
#endif
/* User interrupt handle */
typedef int32(*prc_irq_hdl)(uint32 irq_flags, uint32 irq_data, uint32 param);

/**
  * @brief UART ioctl_cmd type
  */
enum prc_ioctl_cmd {
    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    PRC_IOCTL_CMD_SET_WIDTH,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    PRC_IOCTL_CMD_SET_YUV_MODE,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    PRC_IOCTL_CMD_SET_Y_ADDR,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    PRC_IOCTL_CMD_SET_U_ADDR,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    PRC_IOCTL_CMD_SET_V_ADDR,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    PRC_IOCTL_CMD_SET_SRC_ADDR,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    PRC_IOCTL_CMD_FIRST_KICK,	
    
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	PRC_IOCTL_CMD_KICK,

};


struct prc_device {
    struct dev_obj dev;

};

struct prc_hal_ops {
	struct devobj_ops ops;
	int32(*init)(struct prc_device *prc_dev);
	int32(*deinit)(struct prc_device *prc_dev);
    int32(*ioctl)(struct prc_device *prc_dev, enum prc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct prc_device *prc_dev, uint32 irq_flag, prc_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct prc_device *prc_dev, uint32 irq_flag);
};


int32 prc_set_width(struct prc_device *p_prc, uint32 width);
int32 prc_set_yuv_mode(struct prc_device *p_prc, uint32 mode);
int32 prc_set_yaddr(struct prc_device *p_prc, uint32 addr);
int32 prc_set_uaddr(struct prc_device *p_prc, uint32 addr);
int32 prc_set_vaddr(struct prc_device *p_prc, uint32 addr);
int32 prc_set_src_addr(struct prc_device *p_prc, uint32 addr);
int32 prc_first_start(struct prc_device *p_prc);
int32 prc_run(struct prc_device *p_prc);
int32 prc_init(struct prc_device *p_prc);
int32 prc_deinit(struct prc_device *p_prc);
int32 prc_request_irq(struct prc_device *p_prc, uint32 irq_flags, prc_irq_hdl irq_hdl, uint32 irq_data);
int32 prc_release_irq(struct prc_device *p_prc, uint32 irq_flags);




#ifdef __cplusplus
}
#endif
#endif
