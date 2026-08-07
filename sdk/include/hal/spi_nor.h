#ifndef _HAL_SPI_NOR_H_
#define _HAL_SPI_NOR_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "osal/mutex.h"
#include "osal/atomic.h"

/**
  * @brief spi nor mode
  */
enum spi_nor_mode {
    SPI_NOR_NORMAL_SPI_MODE,
    SPI_NOR_DUAL_SPI_MODE,
    SPI_NOR_QUAD_SPI_MODE,
    SPI_NOR_QPI_MODE,
    SPI_NOR_XIP_MODE,
};

struct spi_nor_flash;

enum spi_nor_cmd {
    SPI_NOR_CMD_READ_JEDEC_ID,
    SPI_NOR_CMD_POWER_DOWN,
    SPI_NOR_CMD_POWER_UP,

    SPI_NOR_CMD_COSTUM_ERASE,
    SPI_NOR_CMD_COSTUM_READ,
    SPI_NOR_CMD_COSTUM_WRITE,

    SPI_NOR_CMD_SEC_ERASE,
    SPI_NOR_CMD_SEC_PROGRAM,
    SPI_NOR_CMD_SEC_READ,

    SPI_NOR_CMD_SUSPEND,
    SPI_NOR_CMD_RESUME,
};

enum spi_nor_erase_mode {
    SPI_NOR_ERASE_SECTOR,
    SPI_NOR_ERASE_BLOCK,
    SPI_NOR_ERASE_CHIP,
};

struct spi_nor_bus {
    void (*open)(struct spi_nor_flash *flash);
    void (*close)(struct spi_nor_flash *flash);
    void (*erase)(struct spi_nor_flash *flash, uint32 mode, uint32 addr);
    void (*read)(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 size);
    void (*program)(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 size);
    int32(*ioctl)(struct spi_nor_flash *flash, uint32_t cmd, uint32_t param1, uint32_t param2);
};

struct spi_nor_regval {
    uint8 *buff;
    uint32 size;
};

struct bpreg_cfg {
    uint8_t srx : 2,
            len : 3,
            idx : 3;
    uint8_t val;
    uint8_t mask;
};

struct bp_area {
    uint8_t bpv:4,
            sec:1,
            tb :1,
            rev:2;
    uint32_t area[2];
};

struct bp_info {
    uint32_t area_cnt       : 9,
             disable_cmp    : 1,
             tb_shift       : 5,
             cmp_shift      : 5,
             pre_cmd        : 8,
             rev            : 4;
    struct bp_area *areas;
};

struct spi_nor_flash {
    struct dev_obj dev;
    os_mutex_t     lock;
    atomic_t       ref;
    enum spi_nor_mode mode;
    uint32 size, sector_size, page_size, block_size;
    uint32 bp_partition_size; //块保护分区大小
    uint32 vendor_id, product_id;
    const struct spi_nor_bus *bus;
    struct spi_device *spidev;
    struct {
        uint32 clk, clk_mode, wire_mode, cs;
    } spi_config;
    struct bpreg_cfg bpbk[4];
    struct bp_info *bpi;
    int (*bp_conv) (struct spi_nor_flash *flash, 
    uint32_t addrl, uint32_t addru, struct bpreg_cfg *bp);
};

const struct spi_nor_bus *spi_nor_bus_get(enum spi_nor_mode mode);

int32 spi_nor_open(struct spi_nor_flash *flash);

void spi_nor_close(struct spi_nor_flash *flash);

void spi_nor_read(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 len);

void spi_nor_write(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 len);

void spi_nor_sector_erase(struct spi_nor_flash *flash, uint32 sector_addr);

void spi_nor_block_erase(struct spi_nor_flash *flash, uint32 block_addr);

void spi_nor_chip_erase(struct spi_nor_flash *flash);

void spi_nor_sec_erase(struct spi_nor_flash *flash, uint32 addr);

void spi_nor_sec_program(struct spi_nor_flash *flash, uint32 addr, struct spi_nor_regval *values);

void spi_nor_sec_read(struct spi_nor_flash *flash, uint32 addr, struct spi_nor_regval *values);

void spi_nor_custom_read(struct spi_nor_flash *flash, void *param);

void spi_nor_custom_write(struct spi_nor_flash *flash, void *param);

void spi_nor_custom_erase(struct spi_nor_flash *flash, void *param);

void spi_nor_power_down(struct spi_nor_flash *flash);

void spi_nor_power_up(struct spi_nor_flash *flash);

void spi_nor_read_jedec_id(struct spi_nor_flash *flash, uint8 *buff);

int32 spi_nor_attach(struct spi_nor_flash *flash, uint32 dev_id);


#ifdef __cplusplus
}
#endif
#endif

