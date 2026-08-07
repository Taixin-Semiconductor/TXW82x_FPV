#ifndef HAL_OSD_ENC_H
#define HAL_OSD_ENC_H
#ifdef __cplusplus
extern "C" {
#endif

typedef int32(*osdenc_irq_hdl)(uint32 irq_flags, uint32 irq_data, uint32 param);

enum osdenc_ioctl_cmd {
    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	OSD_IOCTL_TRAN_IDENT_TRANS,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	OSD_IOCTL_ENC_ADR,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	OSD_IOCTL_SET_ENC_RLEN,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	OSD_IOCTL_SET_ENC_FORMAT,	


    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	OSD_IOCTL_GET_ENC_DLEN,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	OSD_IOCTL_SET_ENC_RUN,	
};

struct osdenc_device {
    struct dev_obj dev;
};

struct osdenc_hal_ops {
	struct devobj_ops ops;
    int32(*open)(struct osdenc_device *osd_dev);
    int32(*close)(struct osdenc_device *osd_dev);
    int32(*ioctl)(struct osdenc_device *osd_dev, enum osdenc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct osdenc_device *osd_dev, uint32 irq_flag, osdenc_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct osdenc_device *osd_dev, uint32 irq_flag);
};

int32 osd_enc_open(struct osdenc_device *p_osd);
int32 osd_enc_close(struct osdenc_device *p_osd);
int32 osd_enc_tran_config(struct osdenc_device *p_osd,uint32 head,uint32 head_tran,uint32 diap,uint32 diap_tran);
int32 osd_enc_addr(struct osdenc_device *p_osd,uint32 src,uint32 des);
int32 osd_enc_src_len(struct osdenc_device *p_osd,uint32 len);
int32 osd_enc_set_format(struct osdenc_device *p_osd,uint8 isrgb565);
int32 osd_enc_run(struct osdenc_device *p_osd);
int32 osd_enc_request_irq(struct osdenc_device *p_osd, uint32 irq_flags, osdenc_irq_hdl irq_hdl, uint32 irq_data);
int32 osd_enc_release_irq(struct osdenc_device *p_osd, uint32 irq_flags);
int32 osd_enc_dlen(struct osdenc_device *p_osd);

#ifdef __cplusplus
}
#endif
#endif
