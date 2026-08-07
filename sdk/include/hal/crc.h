#ifndef _HAL_CRC_H_
#define _HAL_CRC_H_
#ifdef __cplusplus
extern "C" {
#endif

enum CRC_DEV_FLAGS {
    CRC_DEV_FLAGS_CONTINUE_CALC = BIT(0),
};

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

/**
 * CRC len 512KB for TXW81x； 64KB for TXW80x
 */
struct crc_dev_req {
    uint16 type : 8,
           flag : 8;
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
    const struct crc_hal_ops *ops = (const struct crc_hal_ops *)dev->dev.ops;
    return (dev && ops->calc) ? ops->calc(dev, req, crc_val, flags) : RET_ERR;
}

extern uint32 hw_crc(enum CRC_DEV_TYPE type, uint8 *data, uint32 len);
extern uint32 hw_crc_no_cache(enum CRC_DEV_TYPE type, uint8 *data, uint32 len);

#ifdef __cplusplus
}
#endif

#endif

