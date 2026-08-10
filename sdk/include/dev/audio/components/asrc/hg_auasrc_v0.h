#ifndef _HG_AUASRC_V0_H_
#define _HG_AUASRC_V0_H_

#ifdef __cplusplus
extern "C" {
#endif


int32 hg_auasrc_v0_init(void);
int32 hg_auasrc_v0_open(struct auasrc_device *auasrc);
int32 hg_auasrc_v0_close(struct auasrc_device *auasrc);
int32 hg_auasrc_v0_run(struct auasrc_device *auasrc, uint32 hz_from, uint32 hz_to);
int32 hg_auasrc_v0_ioctl(struct auasrc_device *auasrc, enum auasrc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
void  hg_auasrc_v0_adjust_sample_ofs(void);


#ifdef __cplusplus
}
#endif

#endif /* _HG_AUASRC_V0_H_ */
