#ifndef _HG_AUVAD_V0_H_
#define _HG_AUVAD_V0_H_

#ifdef __cplusplus
extern "C" {
#endif


int32 hg_auvad_v0_init(void);
int32 hg_auvad_v0_open(struct auvad_device *auvad, enum auvad_calc_mode mode);
int32 hg_auvad_v0_close(struct auvad_device *auvad);
int32 hg_auvad_v0_calc(struct auvad_device *auvad, void* dat_buf, uint32 bytes, uint32 calc_type, void* result_buf);
int32 hg_auvad_v0_ioctl(struct auvad_device *auvad, enum auvad_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
int32 hg_auvad_v0_request_irq(struct auvad_device *auvad, enum auvad_irq_flag irq_flag, auvad_irq_hdl irq_hdl, uint32 irq_data);
int32 hg_auvad_v0_release_irq(struct auvad_device *auvad, enum auvad_irq_flag irq_flag);

#ifdef __cplusplus
}
#endif

#endif /* _HG_AUVAD_V0_H_ */
