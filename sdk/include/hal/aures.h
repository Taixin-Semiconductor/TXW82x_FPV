#ifndef _AURES_H_
#define _AURES_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef int32 (*aures_irq_hdl)(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2);

struct aures_req {
    void   *chan;
    int16  *in_data;
    uint32 in_nsamples;
    int16  *outbuf;
    uint32 out_nsamples;
};

enum aures_ioctl_cmd {
    AURES_IOCTL_SET_SRC_SAMPLERATE,
    AURES_IOCTL_GET_SRC_SAMPLERATE,
    AURES_IOCTL_SET_DEST_SAMPLERATE,
    AURES_IOCTL_GET_DEST_SAMPLERATE,
    AURES_IOCTL_SET_CHANNELS,
    AURES_IOCTL_GET_CHANNELS,
};

struct aures_device {
    struct dev_obj dev;
};

struct aures_hal_ops {
    struct devobj_ops ops;
    
    void *(*open)(struct aures_device *aures, uint32 src_sample_rate, uint32 dest_sample_rate, uint32 channels);

    int32 (*close)(struct aures_device *aures, void *chan);
    
    int32 (*resample)(struct aures_device *aures, struct aures_req *req);
    
    int32 (*ioctl)(struct aures_device *aures, void *chan, enum aures_ioctl_cmd cmd, uint32 param);

    int32 (*request_irq)(struct aures_device *aures, uint32 irq_id, aures_irq_hdl hdl, void *data);
    int32 (*release_irq)(struct aures_device *aures, uint32 irq_id);
};

void *aures_open(struct aures_device *aures, uint32 src_sample_rate, uint32 dest_sample_rate, uint32 channels);

int32 aures_close(struct aures_device *aures, void *chan);

int32 aures_resample(struct aures_device *aures, struct aures_req *req);

int32 aures_ioctl(struct aures_device *aures, void *chan, enum aures_ioctl_cmd cmd, uint32 param);

#ifdef __cplusplus
}
#endif

#endif