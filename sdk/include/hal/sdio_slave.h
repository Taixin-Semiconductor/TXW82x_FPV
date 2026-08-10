#ifndef _HAL_SDIO_SLAVE_H_
#define _HAL_SDIO_SLAVE_H_
#ifdef __cplusplus
extern "C" {
#endif

enum sdio_slave_flags {
    SDIO_SLAVE_FLAG_4BIT      = BIT(0),
};

enum sdio_slave_speed {
    SDIO_SLAVE_SPEED_1M,
    SDIO_SLAVE_SPEED_2M,
    SDIO_SLAVE_SPEED_4M,
    SDIO_SLAVE_SPEED_8M,
    SDIO_SLAVE_SPEED_12M,
    SDIO_SLAVE_SPEED_24M,
    SDIO_SLAVE_SPEED_48M,

    SDIO_SLAVE_SPEED_NUM,
};

enum sdio_slave_irqs {
    SDIO_SLAVE_IRQ_RX          = BIT(0),
    SDIO_SLAVE_IRQ_RX_OVERFLOW = BIT(1),
    SDIO_SLAVE_IRQ_RX_CRC_ERR  = BIT(2),
    SDIO_SLAVE_IRQ_TX_DONE     = BIT(3),
    SDIO_SLAVE_IRQ_TX_ERR      = BIT(4),
    SDIO_SLAVE_IRQ_RESET       = BIT(5),
};

enum sdio_slave_ioctl_cmd {
    SDIO_SLAVE_IO_CMD_RESET,
    SDIO_SLAVE_IO_CMD_MNTR_RDYSTAT,
    SDIO_SLAVE_IO_CMD_SET_USER_REG0,
    SDIO_SLAVE_IO_CMD_SET_USER_REG1,
    SDIO_SLAVE_IO_CMD_SET_USER_REG2,

};

typedef void (*sdio_slave_irq_hdl)(uint32 irq, uint32 param1, uint32 param2, uint32 param3);

struct sdio_slave {
    struct dev_obj dev;
};

struct sdios_hal_ops{
    struct devobj_ops ops;
    int32(*open)(struct sdio_slave *slave, enum sdio_slave_speed speed, uint32 flags);
    int32(*close)(struct sdio_slave *slave);
    int32(*ioctl)(struct sdio_slave *slave, int32 cmd, uint32 param);
    int32(*read)(struct sdio_slave *slave, uint8 *buff, uint32 len, int8 sync);
    int32(*write)(struct sdio_slave *slave, uint8 *buff, uint32 len, int8 sync);
    int32(*write_scatter)(struct sdio_slave *slave, scatter_data *data, uint32 count, int8 sync);
    int32(*request_irq)(struct sdio_slave *slave, sdio_slave_irq_hdl irqhdl, uint32 data);
};

int32 sdio_slave_open(struct sdio_slave *slave, enum sdio_slave_speed speed, uint32 flags);
int32 sdio_slave_close(struct sdio_slave *slave);
int32 sdio_slave_write(struct sdio_slave *slave, uint8 *buff, uint32 len, int8 sync);
int32 sdio_slave_write_scatter(struct sdio_slave *slave, scatter_data *data, uint32 count, int8 sync);
int32 sdio_slave_read(struct sdio_slave *slave, uint8 *buff, uint32 len, int8 sync);
int32 sdio_slave_ioctl(struct sdio_slave *slave, uint32 cmd, uint32 param);
int32 sdio_slave_request_irq(struct sdio_slave *slave, sdio_slave_irq_hdl handle, uint32 data);

#ifdef __cplusplus
}
#endif
#endif

