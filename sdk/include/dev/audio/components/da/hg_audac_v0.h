#ifndef _HG_AUDAC_V0_H_
#define _HG_AUDAC_V0_H_

#ifdef __cplusplus
extern "C" {
#endif

int32 hg_audac_v0_init(struct audac_device *audac);
int32 hg_audac_v0_open(struct audac_device *audac, enum audac_sample_rate sample_rate);
int32 hg_audac_v0_close(struct audac_device *audac);
int32 hg_audac_v0_write(struct audac_device *audac, void* buf, uint32 bytes);
int32 hg_audac_v0_ioctl(struct audac_device *audac, enum audac_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
int32 hg_audac_v0_request_irq(struct audac_device *audac, enum audac_irq_flag irq_flag, audac_irq_hdl irq_hdl, uint32 irq_data);
int32 hg_audac_v0_release_irq(struct audac_device *audac, enum audac_irq_flag irq_flag);



#ifdef __cplusplus
}
#endif

#endif /* _HGI2S_V0_H_ */
