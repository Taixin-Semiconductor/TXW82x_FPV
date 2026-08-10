#ifndef _AUPROC_H_
#define _AUPROC_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef int32 (*auproc_irq_hdl)(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2);

struct auproc_req {
    int16  *near_data;
    int16  *far_data;
    int16  *out_data;
    uint32 nsamples;
};

/*预留指令，不代表已支持*/
enum auproc_ioctl_cmd {
    AUPROC_IOCTL_GET_VAD,
    AUPROC_IOCTL_SET_MAX_GAIN,
    AUPROC_IOCTL_SET_TARGET_GAIN,
};

struct auproc_device {
    struct dev_obj dev;
};

struct auproc_hal_ops {
    struct devobj_ops ops;
    
    int32 (*open)(struct auproc_device *auproc, uint32 sample_rate, uint32 channels);

    int32 (*close)(struct auproc_device *auproc);
    
    int32 (*process)(struct auproc_device *auproc, struct auproc_req *req);

    int32 (*ioctl)(struct auproc_device *auproc, enum auproc_ioctl_cmd cmd, uint32 param1, uint32 param2);

    int32 (*request_irq)(struct auproc_device *auproc, uint32 irq_id, auproc_irq_hdl hdl, void *data);
    int32 (*release_irq)(struct auproc_device *auproc, uint32 irq_id);
};

int32 auproc_open(struct auproc_device *auproc, uint32 sample_rate, uint32 channels);

int32 auproc_close(struct auproc_device *auproc);

int32 auproc_process(struct auproc_device *auproc, struct auproc_req *req);

int32 auproc_ioctl(struct auproc_device *auproc, enum auproc_ioctl_cmd cmd, uint32 param1, uint32 param2);

#ifdef __cplusplus
}
#endif

#endif