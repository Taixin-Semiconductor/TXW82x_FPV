#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/osd_enc.h"

int32 osd_enc_open(struct osdenc_device *p_osd)
{
    if (p_osd && ((const struct osdenc_hal_ops *)p_osd->dev.ops)->open) {
        return ((const struct osdenc_hal_ops *)p_osd->dev.ops)->open(p_osd);
    }
    return RET_ERR;
}

int32 osd_enc_close(struct osdenc_device *p_osd)
{
    if (p_osd && ((const struct osdenc_hal_ops *)p_osd->dev.ops)->close) {
        return ((const struct osdenc_hal_ops *)p_osd->dev.ops)->close(p_osd);
    }
    return RET_ERR;
}


int32 osd_enc_request_irq(struct osdenc_device *p_osd, uint32 irq_flags, osdenc_irq_hdl irq_hdl, uint32 irq_data)
{
    if (p_osd && ((const struct osdenc_hal_ops *)p_osd->dev.ops)->request_irq) {
        return ((const struct osdenc_hal_ops *)p_osd->dev.ops)->request_irq(p_osd, irq_flags, irq_hdl, irq_data);
    }
    return RET_ERR;
}

int32 osd_enc_release_irq(struct osdenc_device *p_osd, uint32 irq_flags)
{
    if (p_osd && ((const struct osdenc_hal_ops *)p_osd->dev.ops)->release_irq) {
        return ((const struct osdenc_hal_ops *)p_osd->dev.ops)->release_irq(p_osd, irq_flags);
    }
    return RET_ERR;
}


int32 osd_enc_tran_config(struct osdenc_device *p_osd,uint32 head,uint32 head_tran,uint32 diap,uint32 diap_tran){
	uint32_t encbuf[4];
	uint32_t *p32;
    if (p_osd && ((const struct osdenc_hal_ops *)p_osd->dev.ops)->ioctl) {
		p32 = (uint32_t *)encbuf;
		p32[0] = head;
		p32[1] = head_tran;
		p32[2] = diap;
		p32[3] = diap_tran;
        return ((const struct osdenc_hal_ops *)p_osd->dev.ops)->ioctl(p_osd, OSD_IOCTL_TRAN_IDENT_TRANS, (uint32)encbuf,0);
    }
    return RET_ERR;
}

int32 osd_enc_addr(struct osdenc_device *p_osd,uint32 src,uint32 des){
    if (p_osd && ((const struct osdenc_hal_ops *)p_osd->dev.ops)->ioctl) {
        return ((const struct osdenc_hal_ops *)p_osd->dev.ops)->ioctl(p_osd, OSD_IOCTL_ENC_ADR, src,des);
    }
    return RET_ERR;
}

int32 osd_enc_src_len(struct osdenc_device *p_osd,uint32 len){
    if (p_osd && ((const struct osdenc_hal_ops *)p_osd->dev.ops)->ioctl) {
        return ((const struct osdenc_hal_ops *)p_osd->dev.ops)->ioctl(p_osd, OSD_IOCTL_SET_ENC_RLEN, len,0);
    }
    return RET_ERR;
}

int32 osd_enc_dlen(struct osdenc_device *p_osd){
    if (p_osd && ((const struct osdenc_hal_ops *)p_osd->dev.ops)->ioctl) {
        return ((const struct osdenc_hal_ops *)p_osd->dev.ops)->ioctl(p_osd, OSD_IOCTL_GET_ENC_DLEN, 0,0);
    }
    return RET_ERR;
}

int32 osd_enc_set_format(struct osdenc_device *p_osd,uint8 isrgb565){
    if (p_osd && ((const struct osdenc_hal_ops *)p_osd->dev.ops)->ioctl) {
        return ((const struct osdenc_hal_ops *)p_osd->dev.ops)->ioctl(p_osd, OSD_IOCTL_SET_ENC_FORMAT, isrgb565,0);
    }
    return RET_ERR;
}

int32 osd_enc_run(struct osdenc_device *p_osd){
    if (p_osd && ((const struct osdenc_hal_ops *)p_osd->dev.ops)->ioctl) {
        return ((const struct osdenc_hal_ops *)p_osd->dev.ops)->ioctl(p_osd, OSD_IOCTL_SET_ENC_RUN, 0,0);
    }
    return RET_ERR;
}

