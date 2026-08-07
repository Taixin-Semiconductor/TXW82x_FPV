#ifndef _HAL_DMA_H_
#define _HAL_DMA_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * enum dma_status - DMA transaction status
 * @DMA_SUCCESS: transaction completed successfully
 * @DMA_IN_PROGRESS: transaction not yet processed
 * @DMA_PAUSED: transaction is paused
 * @DMA_ERROR: transaction failed
 */
enum dma_status {
    DMA_SUCCESS,
    DMA_IN_PROGRESS,
    DMA_PAUSED,
    DMA_ERROR,
};

enum dma_xfer_direct {
    /*! xfer directiont: NONE
     */
    DMA_XFER_DIR_NONE,
    
    /*! xfer directiont: memory to memory
     */
    DMA_XFER_DIR_M2M,
    
    /*! xfer directiont: memory to device
     */
    DMA_XFER_DIR_M2D,
    
    /*! xfer directiont: device to memory
     */
    DMA_XFER_DIR_D2M,
    
    /*! xfer directiont: device to device
     */
    DMA_XFER_DIR_D2D,
};

enum dma_xfer_mode {
    /*! xfer mode: source address increase
     */
    DMA_XFER_MODE_INCREASE,

    /*! xfer mode: source address recycling
     */
    DMA_XFER_MODE_RECYCLE,

    /*! xfer mode: source address block copy
     */
    DMA_XFER_MODE_BLKCPY,
};
    

/**
 * enum dma_slave_buswidth - defines bus with of the DMA slave
 * device, source or target buses
 */
enum dma_slave_buswidth {
    DMA_SLAVE_BUSWIDTH_1_BYTE       = 0,
    DMA_SLAVE_BUSWIDTH_2_BYTES      = 1,
    DMA_SLAVE_BUSWIDTH_4_BYTES      = 2,
    DMA_SLAVE_BUSWIDTH_8_BYTES      = 3,
    DMA_SLAVE_BUSWIDTH_UNDEFINED    = 4,
};

enum dma_irq_type {
    DMA_IRQ_TYPE_DONE,
    DMA_IRQ_TYPE_ERROR,
};

enum dma_endian_type{
    DMA_ENDIAN_TYPE_NO_OVERTURN,
    DMA_ENDIAN_TYPE_16BIT_OVERTURN,
    DMA_ENDIAN_TYPE_32BIT_OVERTURN,
};

enum dma_ioctl_cmd{
    DMA_IOCTL_CMD_ENDIAN,
    DMA_IOCTL_CMD_CHECK_DMA1_STATUS,
    DMA_IOCTL_CMD_DMA1_LOCK,
    DMA_IOCTL_CMD_DMA1_UNLOCK,
};

struct dma_device;

typedef void (*dma_irq_hdl)(struct dma_device *p_dma, uint32 ch, enum dma_irq_type irq, uint32 data);

struct dma_xfer_data {
    uint32 dest;
    uint32 src;
    uint32 dir           : 3,   //enum dma_xfer_direct
           endian        : 2,
           dst_id        : 8,
           src_id        : 8,
           src_addr_mode : 2,
           dst_addr_mode : 2,
           reserved      : 7;
    uint32 element_per_width;
    uint32 element_num;
    uint32 irq_data;
    dma_irq_hdl irq_hdl;
};

struct dma_device {
    struct dev_obj dev;
};

struct dma_hal_ops{
    struct devobj_ops ops;
    int32(*pause)(struct dma_device *dma, uint32 ch);
    int32(*resume)(struct dma_device *dma, uint32 ch);
    int32(*xfer)(struct dma_device *dma, struct dma_xfer_data *data);
    int32(*only_hw_xfer)(struct dma_device *dma, struct dma_xfer_data *data);
    int32(*get_status)(struct dma_device *dma, uint32 ch);
    int32(*stop)(struct dma_device *dma, uint32 ch);
    int32(*ioctl)(struct dma_device *dma, uint32 cmd, int32 param1, int32 param2);
};

int32 dma_pause(struct dma_device *dma, uint32 ch);

int32 dma_resume(struct dma_device *dma, uint32 ch);

void  dma_memcpy(struct dma_device *dma, void *dst, const void *src, uint32 n);

void  dma_memcpy_no_cache(struct dma_device *dma, void *dst, const void *src, uint32 n);
 
void  dma_memset(struct dma_device *dma, void *dst, uint8 c, uint32 n);

void  dma_memset_word(struct dma_device *dma, void *dst, uint32 c, uint32 n);

void  dma_memcpy_endian(struct dma_device *dma, void *dst, const void *src, uint32 n, enum dma_endian_type type);

int32 dma_ioctl(struct dma_device *dma, uint32 cmd, int32 param1, int32 param2);

int32 dma_xfer(struct dma_device *dma, struct dma_xfer_data *data);

int32 dma_status(struct dma_device *dma, uint32 ch);

int32 dma_stop(struct dma_device *dma, uint32 ch);


#ifdef __cplusplus
}
#endif

#endif
