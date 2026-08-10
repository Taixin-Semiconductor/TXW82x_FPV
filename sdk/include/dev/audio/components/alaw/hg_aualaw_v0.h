#ifndef _HG_AUALAW_V0_H_
#define _HG_AUALAW_V0_H_

#ifdef __cplusplus
extern "C" {
#endif


int32 hg_aualaw_v0_init(void);
int32 hg_aualaw_v0_open(struct aualaw_device *aualaw);
int32 hg_aualaw_v0_close(struct aualaw_device *aualaw);
int32 hg_aualaw_v0_encode(struct aualaw_device *aualaw, void* dat_buf, uint32 bytes, void* result_buf);
int32 hg_aualaw_v0_decode(struct aualaw_device *aualaw, void* dat_buf, uint32 bytes, void* result_buf);
int32 hg_aualaw_v0_ioctl(struct aualaw_device *aualaw, enum aualaw_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
int32 hg_aualaw_v0_request_irq(struct aualaw_device *aualaw, enum aualaw_irq_flag irq_flag, aualaw_irq_hdl irq_hdl, uint32 irq_data);
int32 hg_aualaw_v0_release_irq(struct aualaw_device *aualaw, enum aualaw_irq_flag irq_flag);

#ifdef __cplusplus
}
#endif

#endif /* _HGI2S_V0_H_ */
