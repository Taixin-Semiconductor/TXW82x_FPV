#ifndef _HAL_CRC_H_
#define _HAL_CRC_H_
#ifdef __cplusplus
extern "C" {
#endif

enum CRC_DEV_FLAGS {
    CRC_DEV_FLAGS_CONTINUE_CALC = BIT(0),
};

enum CRC_REQ_FLAGS {
    CRC_REQ_FLAGS_SKIP_CACHE_SYNC = BIT(0),
};


#define CRC_DEV_MAX_XFER_SIZE       (512UL * 1024UL)


enum CRC_DEV_TYPE {
    CRC_TYPE_CRC5_USB    = 0,
    CRC_TYPE_CRC7_MMC,
    CRC_TYPE_CRC8_MAXIM,
    CRC_TYPE_CRC8,
    CRC_TYPE_CRC16,
    CRC_TYPE_CRC16_CCITT,
    CRC_TYPE_CRC16_MODBUS,
    CRC_TYPE_CRC32_WINRAR,
    CRC_TYPE_TCPIP_CHKSUM,

    CRC_TYPE_MAX,
};

/** CRC request. A single hardware transaction is limited by CRC_DEV_MAX_XFER_SIZE. */
struct crc_dev_req {
    uint16 type : 8,
           flag : 8; /* CRC_REQ_FLAGS */
    uint16 cookie;
    uint8 *data;
    uint32 len;
    uint32 crc_last; //use when CRC_DEV_FLAGS_CONTINUE_CALC
};

struct crc_dev {
    struct dev_obj dev;
};

struct crc_hal_ops{
    struct devobj_ops ops;
    int32(*calc)(struct crc_dev *dev, struct crc_dev_req *req, uint32 *crc_val, uint32 flags);
};

/** 
  * @brief  Hold CRC .
  * @param  dev      : crc_dev use @ref dev_get() function to get the handle.
  * @param  req      : req(type\addr\len) for crc dev. 
  *                      1、req crc_last must set when flags:CRC_DEV_FLAGS_CONTINUE_CALC
  * @param  crc_val  : crc result.
  * @param  flags    : CRC_DEV_FLAGS .
  * @return 
  *         - RET_OK  : Successfully.
  *         - RET_ERR : Failed.
  * @note
  */
static inline int32 crc_dev_calc(struct crc_dev *dev, struct crc_dev_req *req, uint32 *crc_val, uint32 flags)
{
    const struct crc_hal_ops *ops;

    if (!dev || !req || !crc_val || !dev->dev.ops) {
        return RET_ERR;
    }
    ops = (const struct crc_hal_ops *)dev->dev.ops;
    return ops->calc ? ops->calc(dev, req, crc_val, flags) : RET_ERR;
}

/** Calculate CRC and return an explicit status. Large buffers are segmented. */
extern int32 hw_crc_s(enum CRC_DEV_TYPE type, const uint8 *data, uint32 len,
                         uint32 *crc);

/** Legacy value-only interfaces. Prefer hw_crc_s() for new code. */
extern uint32 hw_crc(enum CRC_DEV_TYPE type, uint8 *data, uint32 len);
extern uint32 hw_crc_no_cache(enum CRC_DEV_TYPE type, uint8 *data, uint32 len);

#ifdef __cplusplus
}
#endif

#endif

