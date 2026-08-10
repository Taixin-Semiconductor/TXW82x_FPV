#ifndef _AUCHANGE_H_
#define _AUCHANGE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef int32 (*auchange_irq_hdl)(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2);

struct auchange_req {
    void   *chan;
    int16  *data;
    uint32 nsamples;
};

enum auchange_ioctl_cmd {
    AUCHANGE_IOCTL_SET_SPEED,
    AUCHANGE_IOCTL_SET_PITCH,
    AUCHANGE_IOCTL_READ_AVAILABLE,
    AUCHANGE_IOCTL_WRITE_AVAILABLE,
    AUCHANGE_IOCTL_FLUSH_STREAM,
    AUCHANGE_IOCTL_CLEAN_STREAM,
};

struct auchange_device {
    struct dev_obj dev;
};

struct auchange_hal_ops {
    struct devobj_ops ops;
    
    void  *(*open)(struct auchange_device *auchange, uint32 sample_rate, uint32 channels, uint32 max_nsamples);

    int32 (*close)(struct auchange_device *auchange, void *chan);
    
    int32 (*write)(struct auchange_device *auchange, struct auchange_req *req);

    int32 (*read)(struct auchange_device *auchange, struct auchange_req *req);

    int32 (*ioctl)(struct auchange_device *auchange, void *chan, enum auchange_ioctl_cmd cmd, uint32 param);

    int32 (*request_irq)(struct auchange_device *auchange, uint32 irq_id, auchange_irq_hdl hdl, void *data);
    int32 (*release_irq)(struct auchange_device *auchange, uint32 irq_id);
};

void  *auchange_open(struct auchange_device *auchange, uint32 sample_rate, uint32 channels, uint32 max_nsamples);

int32 auchange_close(struct auchange_device *auchange, void *chan);

int32 auchange_write(struct auchange_device *auchange, struct auchange_req *req);

int32 auchange_read(struct auchange_device *auchange, struct auchange_req *req);

int32 auchange_ioctl(struct auchange_device *auchange, void *chan, enum auchange_ioctl_cmd cmd, uint32 param);

#ifdef __cplusplus
}
#endif

#endif