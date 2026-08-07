#ifndef _HG_AUDRC_V0_H_
#define _HG_AUDRC_V0_H_

#ifdef __cplusplus
extern "C" {
#endif


int32 hg_audrc_v0_init(void);
int32 hg_audrc_v0_open(struct audrc_device *audrc);
int32 hg_audrc_v0_close(struct audrc_device *audrc);
int32 hg_audrc_v0_run(struct audrc_device *audrc, void *p_content, uint32 bytes);
int32 hg_audrc_v0_ioctl(struct audrc_device *audrc, enum audrc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);

#ifdef __cplusplus
}
#endif

#endif /* _HG_AUDRC_V0_H_ */
