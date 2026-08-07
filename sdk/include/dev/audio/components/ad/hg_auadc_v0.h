#ifndef _HG_AUADC_V0_H_
#define _HG_AUADC_V0_H_

#ifdef __cplusplus
extern "C" {
#endif


int32 hg_auadc_v0_init(struct auadc_device *auadc);
int32 hg_auadc_v0_open(struct auadc_device *auadc, enum auadc_sample_rate sample_rate);
int32 hg_auadc_v0_close(struct auadc_device *auadc);
int32 hg_auadc_v0_read(struct auadc_device *auadc, void* buf, uint32 bytes);
int32 hg_auadc_v0_ioctl(struct auadc_device *auadc, enum auadc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
int32 hg_auadc_v0_request_irq(struct auadc_device *auadc, enum auadc_irq_flag irq_flag, auadc_irq_hdl irq_hdl, uint32 irq_data);
int32 hg_auadc_v0_release_irq(struct auadc_device *auadc, enum auadc_irq_flag irq_flag);

#ifdef __cplusplus
}
#endif

#endif /* _HGI2S_V0_H_ */
