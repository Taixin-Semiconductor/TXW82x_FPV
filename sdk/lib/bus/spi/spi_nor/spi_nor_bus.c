#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "osal/semaphore.h"
#include "dev.h"
#include "dev/spi/hgspi_xip.h"
#include "hal/spi.h"
#include "hal/spi_nor.h"
#include "osal/sleep.h"
#include "osal/string.h"


#define SPI_NOR_BUS_PRINT(fmt, args...) //os_printf(fmt, ##args)

/**
  * @brief Standard SPI instructions
  */
#define SPI_NOR_STANDARD_WRITE_ENABLE                           0x06
#define SPI_NOR_STANDARD_WRITE_DISABLE                          0x04
#define SPI_NOR_STANDARD_READ_STATUS_REG                        0x05
#define SPI_NOR_STANDARD_WRITE_STATUS_REG                       0x01
#define SPI_NOR_STANDARD_PAGE_PROGRAM                           0x02
#define SPI_NOR_STANDARD_SECTOR_ERASE                           0x20
#define SPI_NOR_STANDARD_BLOCK_ERASE                            0xD8
#define SPI_NOR_STANDARD_CHIP_ERASE                             0xC7//0x60
#define SPI_NOR_STANDARD_ERASE_SUSPEND                          0x75
#define SPI_NOR_STANDARD_ERASE_RESUME                           0x7A
#define SPI_NOR_STANDARD_READ_DATA                              0x03
#define SPI_NOR_STANDARD_FAST_READ                              0x0B
#define SPI_NOR_STANDARD_RELEASE_POWERDOWN_ID                   0xAB
#define SPI_NOR_STANDARD_DEVICE_ID                              0x90
#define SPI_NOR_STANDARD_JEDEC_ID                               0x9F
#define SPI_NOR_STANDARD_ENABLE_QPI                             0x38
#define SPI_NOR_STANDARD_POWER_DOWN                             0xB9
#define SPI_NOR_STANDARD_RELEASE_POWER_DOWN                     0xAB
#define SPI_NOR_STANDARD_ERASE_SECURITY_REG                     0x44
#define SPI_NOR_STANDARD_PROGRAM_SECURITY_REG                   0x42
#define SPI_NOR_STANDARD_READ_SECURITY_REG                      0x48
/**
  * @}
  */

/**
  * @brief Dual SPI instructions
  */
#define SPI_NOR_DUAL_FAST_READ_DUAL_OUTPUT                      0x3B
#define SPI_NOR_DUAL_FAST_READ_DUAL_IO                          0xBB
/**
  * @}
  */

/**
  * @brief Quad SPI instructions
  */
#define SPI_NOR_QUAD_PAGE_PROGRAM                               0x32
#define SPI_NOR_QUAD_FAST_READ_QUAD_OUTPUT                      0x6B
#define SPI_NOR_QUAD_FAST_READ_QUAD_IO                          0xEB
/**
  * @}
  */

/**
  * @brief QPI instructions
  */
#define SPI_NOR_QPI_WRITE_ENABLE                                0x06
#define SPI_NOR_QPI_WRITE_DISABLE                               0x04
#define SPI_NOR_QPI_READ_STATUS_REG                             0x05
#define SPI_NOR_QPI_WRITE_STATUS_REG                            0x01
#define SPI_NOR_QPI_PAGE_PROGRAM                                0x02
#define SPI_NOR_QPI_SECTOR_ERASE                                0x20
#define SPI_NOR_QPI_BLOCK_ERASE                                 0xD8
#define SPI_NOR_QPI_CHIP_ERASE                                  0xC7
#define SPI_NOR_QPI_ERASE_SUSPEND                               0x75
#define SPI_NOR_QPI_ERASE_RESUME                                0x7A
#define SPI_NOR_QPI_FAST_READ                                   0x0B
#define SPI_NOR_QPI_FAST_READ_QUAD_IO                           0xEB
#define SPI_NOR_QPI_JEDEC_ID                                    0x9F
#define SPI_NOR_QPI_DISABLE_QPI                                 0xFF
/**
  * @}
  */

/**
  * @brief SPI NOR STATUS register
  */
#define SPI_NOR_STATUS_BUSY                                     0x01
#define SPI_NOR_STATUS_WEL                                      0x02
#define SPI_NOR_STATUS2_QUAD_ENABLE                             0x200
/**
  * @}
  */

/**
  * @brief Standard SPI instructions
  */
//JECEC ID
void spi_nor_standard_read_jedec_id(struct spi_nor_flash *flash, uint8 *buf)
{
    uint8 instruction = SPI_NOR_STANDARD_JEDEC_ID;

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, &instruction, 1);
    spi_read(flash->spidev, buf, 3);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);
}

//write disable
static void spi_nor_standard_write_disable(struct spi_nor_flash *flash)
{
    uint8 instruction = SPI_NOR_STANDARD_WRITE_DISABLE;

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, &instruction, 1);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);
}

//write enable
static void spi_nor_standard_write_enable(struct spi_nor_flash *flash)
{
    uint8 instruction = SPI_NOR_STANDARD_WRITE_ENABLE;

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, &instruction, 1);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);
}

//read status register
static uint8 spi_nor_standard_read_status_register(struct spi_nor_flash *flash)
{
    uint8 instruction = SPI_NOR_STANDARD_READ_STATUS_REG;
    uint8 result;

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, &instruction, 1);
    spi_read(flash->spidev, &result, 1);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);

    return result;
}

//write status register
static void spi_nor_standard_write_status_register(struct spi_nor_flash *flash, uint16 data)
{
    uint8 write_buf[3];

    spi_nor_standard_write_enable(flash);

    write_buf[0] = SPI_NOR_STANDARD_WRITE_STATUS_REG;
    write_buf[1] = data;
    write_buf[2] = data >> 8;
    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, write_buf, 3);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);

    spi_nor_standard_write_disable(flash);
}

//read data
static void spi_nor_standard_read_data(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 buf_size)
{
    uint8 write_buf[4];

    write_buf[0] = SPI_NOR_STANDARD_READ_DATA;
    write_buf[1] = addr >> 16;
    write_buf[2] = addr >> 8;
    write_buf[3] = addr;

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, write_buf, 4);
    spi_read(flash->spidev, buf, buf_size);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);
}

//fast read
static void spi_nor_standard_fast_read(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 buf_size)
{
    uint8 write_buf[5];

    write_buf[0] = SPI_NOR_STANDARD_FAST_READ;
    write_buf[1] = addr >> 16;
    write_buf[2] = addr >> 8;
    write_buf[3] = addr;
    write_buf[4] = 0x00;    //dummy clock

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, write_buf, 5);
    spi_read(flash->spidev, buf, buf_size);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);
}

//page program
static void spi_nor_standard_page_program(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 buf_size)
{
    uint8 write_buf[4];

    spi_nor_standard_write_enable(flash);

    write_buf[0] = SPI_NOR_STANDARD_PAGE_PROGRAM;
    write_buf[1] = addr >> 16;
    write_buf[2] = addr >> 8;
    write_buf[3] = addr;

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, write_buf, 4);
    spi_write(flash->spidev, buf, buf_size);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);

    //wait page program done
    while (spi_nor_standard_read_status_register(flash) & SPI_NOR_STATUS_BUSY);

    spi_nor_standard_write_disable(flash);
}

//sector erase
static void spi_nor_standard_sector_erase(struct spi_nor_flash *flash, uint32 addr)
{
    uint8 write_buf[4];

    spi_nor_standard_write_enable(flash);

    write_buf[0] = SPI_NOR_STANDARD_SECTOR_ERASE;
    write_buf[1] = addr >> 16;
    write_buf[2] = addr >> 8;
    write_buf[3] = addr;

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, write_buf, 4);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);

    //wait sector erase done
    while (spi_nor_standard_read_status_register(flash) & SPI_NOR_STATUS_BUSY);

    spi_nor_standard_write_disable(flash);
}

//block erase
static void spi_nor_standard_block_erase(struct spi_nor_flash *flash, uint32 addr)
{
    uint8 write_buf[4];

    spi_nor_standard_write_enable(flash);

    write_buf[0] = SPI_NOR_STANDARD_BLOCK_ERASE;
    write_buf[1] = addr >> 16;
    write_buf[2] = addr >> 8;
    write_buf[3] = addr;

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, write_buf, 4);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);

    //wait block erase done
    while (spi_nor_standard_read_status_register(flash) & SPI_NOR_STATUS_BUSY);

    spi_nor_standard_write_disable(flash);
}

//chip erase
static void spi_nor_standard_chip_erase(struct spi_nor_flash *flash)
{
    uint8 instruction = SPI_NOR_STANDARD_CHIP_ERASE;

    spi_nor_standard_write_enable(flash);

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, &instruction, 1);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);

    //wait block erase done
    while (spi_nor_standard_read_status_register(flash) & SPI_NOR_STATUS_BUSY);

    spi_nor_standard_write_disable(flash);
}

//standard erase
static void spi_nor_standard_erase(struct spi_nor_flash *flash, uint32 mode, uint32 addr)
{
    switch (mode) {
    case SPI_NOR_ERASE_SECTOR:
        spi_nor_standard_sector_erase(flash, addr);
        break;
    case SPI_NOR_ERASE_BLOCK:
        spi_nor_standard_block_erase(flash, addr);
        break;
    case SPI_NOR_ERASE_CHIP:
        spi_nor_standard_chip_erase(flash);
        break;
    default:
        break;
    }
}

//enter QPI
static void spi_nor_standard_enter_qpi(struct spi_nor_flash *flash)
{
    uint8 instruction = SPI_NOR_STANDARD_ENABLE_QPI;

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, &instruction, 1);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);
}

//enter power down
static void spi_nor_standard_power_down(struct spi_nor_flash *flash)
{
    uint8 instruction = SPI_NOR_STANDARD_POWER_DOWN;

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, &instruction, 1);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);
}

//release power down
static void spi_nor_standard_release_power_down(struct spi_nor_flash *flash)
{
    uint8 instruction = SPI_NOR_STANDARD_RELEASE_POWER_DOWN;

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, &instruction, 1);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);
}

//erase security registers
static void spi_nor_standard_erase_security_reg(struct spi_nor_flash *flash, uint32 addr)
{
    uint8 write_buf[4];

    spi_nor_standard_write_enable(flash);

    write_buf[0] = SPI_NOR_STANDARD_ERASE_SECURITY_REG;
    write_buf[1] = addr >> 16;
    write_buf[2] = addr >> 8;
    write_buf[3] = addr;

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, write_buf, 4);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);

    //wait block erase done
    while (spi_nor_standard_read_status_register(flash) & SPI_NOR_STATUS_BUSY);

    spi_nor_standard_write_disable(flash);
}

//program security registers
static void spi_nor_standard_program_security_reg(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 buf_size)
{
    uint8 write_buf[4];

    spi_nor_standard_write_enable(flash);

    write_buf[0] = SPI_NOR_STANDARD_PROGRAM_SECURITY_REG;
    write_buf[1] = addr >> 16;
    write_buf[2] = addr >> 8;
    write_buf[3] = addr;

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, write_buf, 4);
    spi_write(flash->spidev, buf, buf_size);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);

    //wait block erase done
    while (spi_nor_standard_read_status_register(flash) & SPI_NOR_STATUS_BUSY);

    spi_nor_standard_write_disable(flash);
}

//read security registers
static void spi_nor_standard_read_security_reg(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 buf_size)
{
    uint8 write_buf[5];

    write_buf[0] = SPI_NOR_STANDARD_READ_SECURITY_REG;
    write_buf[1] = addr >> 16;
    write_buf[2] = addr >> 8;
    write_buf[3] = addr;
    write_buf[4] = 0;   //dummy

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, write_buf, 5);
    spi_read(flash->spidev, buf, buf_size);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);
}

static void spi_nor_standard_open(struct spi_nor_flash *flash)
{
    //release power down
    //spi_nor_standard_release_power_down(flash);
    //wait done
    //os_sleep_us(100);
}

static void spi_nor_standard_close(struct spi_nor_flash *flash)
{
    //release power down
    //spi_nor_standard_release_power_down(flash);
    //wait done
    //os_sleep_us(100);
}

static int32 spi_nor_standard_ioctl(struct spi_nor_flash *flash, uint32_t cmd, uint32_t param1, uint32_t param2)
{
	
    int ret = RET_OK;
	struct spi_nor_regval *reg;
    switch (cmd) {
    case SPI_NOR_CMD_READ_JEDEC_ID:
        spi_nor_standard_read_jedec_id(flash, (void*)param1);
        break;
    case SPI_NOR_CMD_POWER_DOWN:
        spi_nor_standard_power_down(flash);
        break;
    case SPI_NOR_CMD_POWER_UP:
        spi_nor_standard_release_power_down(flash);
        break;
    case SPI_NOR_CMD_COSTUM_ERASE:
    case SPI_NOR_CMD_COSTUM_READ:
    case SPI_NOR_CMD_COSTUM_WRITE:
        ret = RET_ERR;
        break;
    case SPI_NOR_CMD_SEC_ERASE:
        spi_nor_standard_erase_security_reg(flash, param1);
        break;
    case SPI_NOR_CMD_SEC_PROGRAM:
        reg = (struct spi_nor_regval *)param2;
        spi_nor_standard_program_security_reg(flash, param1, reg->buff, reg->size);
        break;
    case SPI_NOR_CMD_SEC_READ:
        reg = (struct spi_nor_regval *)param2;
        spi_nor_standard_read_security_reg(flash, param1, reg->buff, reg->size);
        break;
    default:
        break;
    }
    return ret;
}
/**
  * @}
  */

/**
  * @brief Dual SPI instructions
  */
//fast read dual output
//If flash supports DUAL, this function is also generally supported
static void spi_nor_dual_fast_read_dual_output(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 buf_size)
{
    uint8 write_buf[5];

    write_buf[0] = SPI_NOR_DUAL_FAST_READ_DUAL_OUTPUT;
    write_buf[1] = addr >> 16;
    write_buf[2] = addr >> 8;
    write_buf[3] = addr;
    write_buf[4] = 0x00;    //dummy clock

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);

    //instruction & address & dummy clock should tx in Standard SPI mode
    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);
    spi_write(flash->spidev, write_buf, 5);

    //data rx in DUAL SPI mode
    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_DUAL_MODE, 0);
    spi_read(flash->spidev, buf, buf_size);

    //Return to normal mode
    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);

    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);
}

//fast read dual IO
//warning! You need to confirm that nor flash supports this function.
static void spi_nor_dual_fast_read_dual_io(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 buf_size)
{
    uint8 write_buf[5];

    write_buf[0] = SPI_NOR_DUAL_FAST_READ_DUAL_IO;
    write_buf[1] = addr >> 16;
    write_buf[2] = addr >> 8;
    write_buf[3] = addr;
    write_buf[4] = 0x00;

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);

    //instruction should tx in Standard SPI mode
    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);
    spi_write(flash->spidev, write_buf, 1);

    //address & dummy clock should tx in DUAL SPI mode
    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_DUAL_MODE, 0);
    spi_write(flash->spidev, &write_buf[1], 4);

    //data rx in DUAL SPI mode
    spi_read(flash->spidev, buf, buf_size);

    //Return to normal mode
    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);

    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);
}

static void spi_nor_quad_open(struct spi_nor_flash *flash)
{
    uint16 status;

    //release power down
    spi_nor_standard_release_power_down(flash);
    //wait done
    os_sleep_us(50);

    status = spi_nor_standard_read_status_register(flash);
    status |= SPI_NOR_STATUS2_QUAD_ENABLE;
    spi_nor_standard_write_status_register(flash, status);
}

/**
  * @}
  */

/**
  * @brief Quad SPI instructions
  * @note  warning! You need to confirm that nor flash supports this function.
  */
//fast read quad output
static void spi_nor_quad_fast_read_quad_output(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 buf_size)
{
    uint8 write_buf[5];

    write_buf[0] = SPI_NOR_QUAD_FAST_READ_QUAD_OUTPUT;
    write_buf[1] = addr >> 16;
    write_buf[2] = addr >> 8;
    write_buf[3] = addr;
    write_buf[4] = 0x00;    //dummy clock

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);

    //instruction & address & dummy clock should tx in Standard SPI mode
    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);
    spi_write(flash->spidev, write_buf, 5);

    //data rx in QUAD SPI mode
    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_QUAD_MODE, 0);
    spi_read(flash->spidev, buf, buf_size);

    //Return to normal mode
    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);

    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);
}

//fast read quad IO
static void spi_nor_quad_fast_read_quad_io(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 buf_size)
{
    uint8 write_buf[7];

    write_buf[0] = SPI_NOR_QUAD_FAST_READ_QUAD_IO;
    write_buf[1] = addr >> 16;
    write_buf[2] = addr >> 8;
    write_buf[3] = addr;
    write_buf[4] = 0x00;
    write_buf[5] = 0x00;    //dummy clock
    write_buf[6] = 0x00;    //dummy clock

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);

    //instruction should tx in Standard SPI mode
    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);
    spi_write(flash->spidev, write_buf, 1);

    //address & dummy clock should tx in QUAD SPI mode
    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_QUAD_MODE, 0);
    spi_write(flash->spidev, &write_buf[1], 6);

    //data rx in DUAL SPI mode
    spi_read(flash->spidev, buf, buf_size);

    //Return to normal mode
    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);

    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);
}

//quad input page program
static void spi_nor_quad_page_program(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 buf_size)
{
    uint8 write_buf[4];

    write_buf[0] = SPI_NOR_QUAD_PAGE_PROGRAM;
    write_buf[1] = addr >> 16;
    write_buf[2] = addr >> 8;
    write_buf[3] = addr;

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);

    spi_nor_standard_write_enable(flash);

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, write_buf, 4);
    //data tx in QUAD SPI mode
    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_QUAD_MODE, 0);
    spi_write(flash->spidev, buf, buf_size);
    //Return to normal mode
    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);

    //wait tx done
    while (spi_nor_standard_read_status_register(flash) & SPI_NOR_STATUS_BUSY);
    spi_nor_standard_write_disable(flash);
}


/**
  * @}
  */

/**
  * @brief QPI instructions
  * @note  warning! You need to confirm that nor flash supports this function.
  */
//JECEC ID
static void spi_nor_qpi_read_jedec_id(struct spi_nor_flash *flash, uint8 *buf)
{
    uint8 instruction = SPI_NOR_QPI_JEDEC_ID;

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_QUAD_MODE, 0);

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, &instruction, 1);
    spi_read(flash->spidev, buf, 3);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);
}

//write disable
static void spi_nor_qpi_write_disable(struct spi_nor_flash *flash)
{
    uint8 instruction = SPI_NOR_QPI_WRITE_DISABLE;

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_QUAD_MODE, 0);

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, &instruction, 1);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);
}

//write enable
static void spi_nor_qpi_write_enable(struct spi_nor_flash *flash)
{
    uint8 instruction = SPI_NOR_QPI_WRITE_ENABLE;

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_QUAD_MODE, 0);

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, &instruction, 1);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);
}

//read status register
static uint8 spi_nor_qpi_read_status_register(struct spi_nor_flash *flash)
{
    uint8 instruction = SPI_NOR_QPI_READ_STATUS_REG;
    uint8 result;

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_QUAD_MODE, 0);

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, &instruction, 1);
    spi_read(flash->spidev, &result, 1);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);

    return result;
}

//write status register
static void spi_nor_qpi_write_status_register(struct spi_nor_flash *flash, uint16 data)
{
    uint8 write_buf[3];

    spi_nor_qpi_write_enable(flash);

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_QUAD_MODE, 0);

    write_buf[0] = SPI_NOR_QPI_WRITE_STATUS_REG;
    write_buf[1] = data;
    write_buf[2] = data >> 8;
    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, write_buf, 3);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);

    spi_nor_qpi_write_disable(flash);
}

//fast read quad io
static void spi_nor_qpi_fast_read_quad_io(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 buf_size)
{
    uint8 write_buf[5];

    write_buf[0] = SPI_NOR_QPI_FAST_READ_QUAD_IO;
    write_buf[1] = addr >> 16;
    write_buf[2] = addr >> 8;
    write_buf[3] = addr;
    write_buf[4] = 0x00;

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_QUAD_MODE, 0);

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, write_buf, 5);
    spi_read(flash->spidev, buf, buf_size);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);
}

//page program
static void spi_nor_qpi_page_program(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 buf_size)
{
    uint8 write_buf[4];

    spi_nor_qpi_write_enable(flash);

    write_buf[0] = SPI_NOR_QPI_PAGE_PROGRAM;
    write_buf[1] = addr >> 16;
    write_buf[2] = addr >> 8;
    write_buf[3] = addr;

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_QUAD_MODE, 0);

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, write_buf, 4);
    spi_write(flash->spidev, buf, buf_size);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);

    //wait page program done
    while (spi_nor_qpi_read_status_register(flash) & SPI_NOR_STATUS_BUSY);

    spi_nor_qpi_write_disable(flash);
}

//sector erase
static void spi_nor_qpi_sector_erase(struct spi_nor_flash *flash, uint32 addr)
{
    uint8 write_buf[4];

    spi_nor_qpi_write_enable(flash);

    write_buf[0] = SPI_NOR_QPI_SECTOR_ERASE;
    write_buf[1] = addr >> 16;
    write_buf[2] = addr >> 8;
    write_buf[3] = addr;

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_QUAD_MODE, 0);

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, write_buf, 4);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);

    //wait sector erase done
    while (spi_nor_qpi_read_status_register(flash) & SPI_NOR_STATUS_BUSY);

    spi_nor_qpi_write_disable(flash);
}

//block erase
static void spi_nor_qpi_block_erase(struct spi_nor_flash *flash, uint32 addr)
{
    uint8 write_buf[4];

    spi_nor_qpi_write_enable(flash);

    write_buf[0] = SPI_NOR_QPI_BLOCK_ERASE;
    write_buf[1] = addr >> 16;
    write_buf[2] = addr >> 8;
    write_buf[3] = addr;

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_QUAD_MODE, 0);

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, write_buf, 4);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);

    //wait block erase done
    while (spi_nor_qpi_read_status_register(flash) & SPI_NOR_STATUS_BUSY);

    spi_nor_qpi_write_disable(flash);
}

//chip erase
static void spi_nor_qpi_chip_erase(struct spi_nor_flash *flash)
{
    uint8 instruction = SPI_NOR_QPI_CHIP_ERASE;

    spi_nor_qpi_write_enable(flash);

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_QUAD_MODE, 0);

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, &instruction, 1);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);

    //wait block erase done
    while (spi_nor_qpi_read_status_register(flash) & SPI_NOR_STATUS_BUSY);

    spi_nor_qpi_write_disable(flash);
}

//qpi erase
static void spi_nor_qpi_erase(struct spi_nor_flash *flash, uint32 mode, uint32 addr)
{
    switch (mode) {
    case SPI_NOR_ERASE_SECTOR:
        spi_nor_qpi_sector_erase(flash, addr);
        break;
    case SPI_NOR_ERASE_BLOCK:
        spi_nor_qpi_block_erase(flash, addr);
        break;
    case SPI_NOR_ERASE_CHIP:
        spi_nor_qpi_chip_erase(flash);
        break;
    default:
        break;
    }
}

//disable QPI
static void spi_nor_standard_disable_qpi(struct spi_nor_flash *flash)
{
    uint8 instruction = SPI_NOR_QPI_DISABLE_QPI;

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_QUAD_MODE, 0);

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, &instruction, 1);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);

    spi_ioctl(flash->spidev, SPI_WIRE_MODE_SET, SPI_WIRE_NORMAL_MODE, 0);
}

//enter power down
static void spi_nor_qpi_power_down(struct spi_nor_flash *flash)
{
    uint8 instruction = SPI_NOR_STANDARD_POWER_DOWN;

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, &instruction, 1);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);
}

//release power down
static void spi_nor_qpi_release_power_down(struct spi_nor_flash *flash)
{
    uint8 instruction = SPI_NOR_STANDARD_RELEASE_POWER_DOWN;

    spi_set_cs(flash->spidev, flash->spi_config.cs, 0);
    spi_write(flash->spidev, &instruction, 1);
    spi_set_cs(flash->spidev, flash->spi_config.cs, 1);
}

static void spi_nor_qpi_open(struct spi_nor_flash *flash)
{
    uint16 status;

    //release power down
    spi_nor_qpi_release_power_down(flash);
    //wait done
    os_sleep_us(50);

    status = spi_nor_standard_read_status_register(flash);
    status |= SPI_NOR_STATUS2_QUAD_ENABLE;
    spi_nor_standard_write_status_register(flash, status);
    spi_nor_standard_enter_qpi(flash);
}

static void spi_nor_qpi_close(struct spi_nor_flash *flash)
{
    spi_nor_standard_disable_qpi(flash);
}

static int32 spi_nor_qpi_ioctl(struct spi_nor_flash *flash, uint32_t cmd, uint32_t param1, uint32_t param2)
{
    int ret = RET_OK;
	struct spi_nor_regval *reg;
    switch (cmd) {
    case SPI_NOR_CMD_READ_JEDEC_ID:
        spi_nor_qpi_read_jedec_id(flash, (void*)param1);
        break;
    case SPI_NOR_CMD_POWER_DOWN:
        spi_nor_qpi_power_down(flash);
        break;
    case SPI_NOR_CMD_POWER_UP:
        spi_nor_qpi_release_power_down(flash);
        break;
    case SPI_NOR_CMD_COSTUM_ERASE:
    case SPI_NOR_CMD_COSTUM_READ:
    case SPI_NOR_CMD_COSTUM_WRITE:
        ret = RET_ERR;
        break;
    case SPI_NOR_CMD_SEC_ERASE:
        spi_nor_standard_erase_security_reg(flash, param1);
        break;
    case SPI_NOR_CMD_SEC_PROGRAM:
        reg = (struct spi_nor_regval *)param2;
        spi_nor_standard_program_security_reg(flash, param1, reg->buff, reg->size);
        break;
    case SPI_NOR_CMD_SEC_READ:
        reg = (struct spi_nor_regval *)param2;
        spi_nor_standard_read_security_reg(flash, param1, reg->buff, reg->size);
        break;
    default:
        break;
    }
    return ret;
}

/*  XIP NOR FLASH BP REALIZATION
 *  较为常见的硬件块保护实现方式， BP0-2控制1/64比例的块保护(后续称为bp_size)， BP3选择顶部还是底部块保护, 
 *  CMP 表示保护反转
 *  考虑后续通过产家ID和产品ID来使用不同的块保护策略
 *  目前的软件配套是将传进来的待写入地址区间， 取消对应的块保护配置， 底层在写入/擦除前后会自动清除和重置块保护
 */ 

const uint8_t bps2level[] = {
    0, 1,  2,  4,  
    8, 16, 32, 64,
};

#define IN_REGION(__addrl, __addru, __rstart, __rend) \
    ((__addrl) >= (__rstart) && (__addru) <= (__rend))

#define REGION_INCLU(__rstart, __rend, __addrl, __addru) \
    ((__addrl) >= (__rstart) && (__addru) <= (__rend))

#define REGION_EXCLU(__rstart, __rend, __addrl, __addru) \
    (((__addru) < (__rstart) || (__addrl) > (__rend)))

#define DEF_BP_AREA(_tb, _bpv, _sec, _addrl, _addru) \
    (const struct bp_area){.bpv = _bpv, .tb = _tb, .sec = _sec, .area = {_addrl, _addru}}

#define DEF_BP_AREA_TB_BPV(_tb, _bpv, _addrl, _addru) \
    DEF_BP_AREA(_tb, _bpv, 0, _addrl, _addru)

const struct bp_area bp_area_tbl_2MB[] = {
    DEF_BP_AREA_TB_BPV(0, 0, 0, 0),
    DEF_BP_AREA_TB_BPV(1, 1, 0, 0xffff),
    DEF_BP_AREA_TB_BPV(1, 2, 0, 0x1ffff),
    DEF_BP_AREA_TB_BPV(1, 3, 0, 0x3ffff),
    DEF_BP_AREA_TB_BPV(1, 4, 0, 0x7ffff),
    DEF_BP_AREA_TB_BPV(1, 5, 0, 0xfffff),
    DEF_BP_AREA_TB_BPV(1, 6, 0, 0x1fffff),
    DEF_BP_AREA_TB_BPV(0, 1, 0x1f0000, 0x1fffff),
    DEF_BP_AREA_TB_BPV(0, 2, 0x1e0000, 0x1fffff),
    DEF_BP_AREA_TB_BPV(0, 3, 0x1c0000, 0x1fffff),
    DEF_BP_AREA_TB_BPV(0, 4, 0x180000, 0x1fffff),
    DEF_BP_AREA_TB_BPV(0, 5, 0x100000, 0x1fffff),
};

const struct bp_area bp_area_tbl_general[] = {
    DEF_BP_AREA_TB_BPV(0, 0, 0, 0x1000000),
};

const struct bp_info bp_info = {
    .area_cnt = sizeof(bp_area_tbl_general)/sizeof(bp_area_tbl_general[0]),
    .areas = (struct bp_area *)bp_area_tbl_general,
    .disable_cmp = 1,
    .tb_shift    = 11,
    .cmp_shift   = 14,
    .pre_cmd     = 0x50,
};

int tb_judge(int bpl, int bpu, int rl, int ru)
{
    int midl = (rl + ru)     / 2;
    int midu = (rl + ru + 1) / 2;
    if (IN_REGION(bpl, bpu, rl, midl)) {
        return 1; //底部
    } else if (IN_REGION(bpl, bpu, midu, ru)) {
        return 0; //顶部
    } else {
        return 2; //中间
    }
}

//计算出对应的bp配置， 可能存在兼容性问题
int xip_nor_flash_unbp_convert(struct spi_nor_flash *flash, 
    uint32_t addrl, uint32_t addru, struct bpreg_cfg *bp)
{
    extern uint32_t get_flash_cap();
    int flash_size = get_flash_cap();
    int bp_size = (flash_size / 64 < 64*1024) ?  64*1024 : flash_size / 64;
    int bp_level = flash_size / bp_size;     //可能是64, 32, 16.... 意味着块保护被分成了level个区域
    int bpl = (addrl / bp_size);             //以0为第一索引
    int bpu = (addru / bp_size);
    int bps_max = 0; //bp0-2
    int bpv;

    memset(bp, 0, 4*sizeof(struct bpreg_cfg));

    for (;bps_max<8;bps_max++) {
        if (bps2level[bps_max] == bp_level) {
            break;
        }
    }

    if (bps_max == 8) {
        return RET_ERR; //不支持的FLASH
    }

    //考虑"解保护"区域靠近底部还是顶部
    int fdir, tmp;
    int start = 0, end = bp_level-1; 
    fdir = tmp = tb_judge(bpl, bpu, start, end);
    if (fdir == 2) {
        //解保护区刚好夹在中间
        for (int i = bps_max;i>=0;i--)
        {
            if (bps2level[i] < bpl || i==0) {
                bpv = i;
                break;
            }
        }
        //优先保护底部(TB)
        bpv |= 1<<3;
        bp[0] = (struct bpreg_cfg){
            .srx = 0,
            .len = 1,
            .idx = 0,
            .val = bpv << 2, 
            .mask = 0xf << 2, 
        };
        bp[1] = (struct bpreg_cfg){
            .srx = 1,
            .len = 1,
            .idx = 0,
            .val = 0 << 6, //保护
            .mask = 1 << 6, //保护
        };
    } else {
        bpv = bps_max-1;
        //能否继续缩小范围？当bpv == 1时， 仅有一块区域了
        while(fdir == tmp && bpv > 1) 
        {
            if (fdir == 0) {
                start = (start + end + 1) / 2;
            } else {
                end = end / 2;
            }
            tmp = tb_judge(bpl, bpu, start, end);
            if (fdir == tmp) {
                //方向一致， 可以缩小范围
                bpv--;
            }
        }
        bpv |= fdir << 3;
        bp[0] = (struct bpreg_cfg){
            .srx = 0,
            .len = 1,
            .idx = 0,
            .val = bpv << 2, //部分保护
            .mask = 0xf << 2, //部分保护
        }; 
        bp[1] = (struct bpreg_cfg){
            .srx = 1,
            .len = 1,
            .idx = 0,
            .val = 1 << 6, //保护反转
            .mask = 1 << 6, //保护反转
        };
    }
    SPI_NOR_BUS_PRINT("bp0:[0x%x, 0x%x], bp1:[0x%x, 0x%x]\r\n", bp[0].val>>2, bp[0].mask>>2, bp[1].val, bp[1].mask);

    return 0;
}

//查表获得对应的bp配置， 需要手动制表
int xip_nor_flash_unbp_convert_tbl(struct spi_nor_flash *flash, 
    uint32_t addrl, uint32_t addru, struct bpreg_cfg *bp)
{
    extern uint32_t get_flash_cap();
    int min = get_flash_cap();
    int max = 0;
    int area_idx = 0;
    memset(bp, 0, 4*sizeof(struct bpreg_cfg));
    if (flash->bpi->disable_cmp) {
        //找到一个不包含写入区域的保护区， 并判断这个保护区是否是最大的
        for(int i = 0;i<flash->bpi->area_cnt;i++)
        {
            if ((REGION_EXCLU(flash->bpi->areas[i].area[0], 
            flash->bpi->areas[i].area[1], addrl, addru-1)) && 
            flash->bpi->areas[i].area[1]-flash->bpi->areas[i].area[0] > max) {
                max = flash->bpi->areas[i].area[1]-flash->bpi->areas[i].area[0];
                area_idx = i;
            }
        }
    } else {
        //找到一个包含写入区域的保护区， 并判断这个保护区是否是最小的
        for(int i = 0;i<flash->bpi->area_cnt;i++)
        {
            if ((REGION_INCLU(flash->bpi->areas[i].area[0], 
            flash->bpi->areas[i].area[1], addrl, addru-1)) && 
            flash->bpi->areas[i].area[1]-flash->bpi->areas[i].area[0] < min) {
                min = flash->bpi->areas[i].area[1]-flash->bpi->areas[i].area[0];
                area_idx = i;
            }
        }
    }
	
	SPI_NOR_BUS_PRINT("addr: [0x%x, 0x%x]\r\n", addrl, addru);
	SPI_NOR_BUS_PRINT("i = %d, area: [0x%x, 0x%x]\r\n", area_idx, 
        flash->bpi->areas[area_idx].area[0], 
		flash->bpi->areas[area_idx].area[1]);

    bp[0] = (struct bpreg_cfg){
        .len = 1,
        .val = (flash->bpi->areas[area_idx].bpv) << 2,
        .mask = 0xf << 2, //部分保护
    }; 
    
    if (!flash->bpi->disable_cmp) {
        bp[1] = (struct bpreg_cfg){
            .len = 1,
            .val = 1 << 6, //保护反转
            .mask = 1 << 6, //保护反转
        };
    }

    if (flash->bpi->tb_shift > 8) {
        bp[1].val  |= flash->bpi->areas[area_idx].tb << (flash->bpi->tb_shift%8);   
        bp[1].mask |= 1<<(flash->bpi->tb_shift%8);     
    } else {
        bp[0].val  |= flash->bpi->areas[area_idx].tb << (flash->bpi->tb_shift);   
        bp[0].mask |= 1<<(flash->bpi->tb_shift);     
    }

    SPI_NOR_BUS_PRINT("bp0:[0x%x, 0x%x], bp1:[0x%x, 0x%x]\r\n", bp[0].val>>2, bp[0].mask>>2, bp[1].val, bp[1].mask);
    return 0;
}

int xip_nor_flash_unbp_bakup(struct spi_nor_flash *flash, 
    struct bpreg_cfg *bp, int size) 
{
    #if 1
    uint8_t buf[4];
    struct hgxip_flash_reg_opt_param para[2] = {
        (struct hgxip_flash_reg_opt_param ){
            .cmd = 0x05,
            .pre_cmd = 0xff,
            .len = 1,
            .buf = &buf[0],
        },
        (struct hgxip_flash_reg_opt_param ){
            .cmd = 0x35,
            .pre_cmd = 0xff,
            .len = 1,
            .buf = &buf[2],
        },
    };

    spi_ioctl(flash->spidev, SPI_XIP_REGS_OPT, (uint32)&para, 2);
    bp[0] = (struct bpreg_cfg ){
        .srx = 0,
        .idx = 0,
        .len = 1,
        .val = buf[0],
        .mask = 0xff,
    };

    bp[1] = (struct bpreg_cfg ){
		.srx = 1,
		.idx = 0,
		.len = 1,
		.val = buf[2],
        .mask = 0xff,
    };

    return 0;
    #endif
}

int xip_nor_flash_unbp_update(struct spi_nor_flash *flash, 
    struct bpreg_cfg *bp, int size) 
{
#if 1
    uint8_t buf[4] = {
        (flash->bpbk[0].val & ~bp[0].mask) | bp[0].val,
        (flash->bpbk[1].val & ~bp[1].mask) | bp[1].val,
    };

    SPI_NOR_BUS_PRINT("cur reg: 0x%x, 0x%x\r\n", flash->bpbk[0].val, flash->bpbk[1].val);
    SPI_NOR_BUS_PRINT("reg mask: 0x%x, 0x%x\r\n", bp[0].mask, bp[1].mask);
    SPI_NOR_BUS_PRINT("next reg: 0x%x, 0x%x\r\n", buf[0], buf[1]);

    struct hgxip_flash_reg_opt_param para[3] = 
    {
        (struct hgxip_flash_reg_opt_param ){
            .cmd = 0x01,
            .pre_cmd = flash->bpi->pre_cmd,
            .len = 2,
            .buf = buf,
        },
        (struct hgxip_flash_reg_opt_param ){
            .cmd = 0x01,
            .pre_cmd = flash->bpi->pre_cmd,
            .len = 1,
            .buf = buf,
        },
        (struct hgxip_flash_reg_opt_param ){
            .cmd = 0x31,
            .pre_cmd = flash->bpi->pre_cmd,
            .len = 1,
            .buf = &buf[1],
        },
    };
    
    spi_ioctl(flash->spidev, SPI_XIP_REGS_OPT, (uint32)&para, 3);
    // SPI_NOR_BUS_PRINT("2buf: 0x%x, 0x%x\r\n", buf[0], buf[1]);
#if 1
    struct hgxip_flash_reg_opt_param rd_reg[3] = {
        (struct hgxip_flash_reg_opt_param ){
            .cmd = 0x05,
            .pre_cmd = 0xff,
            .len = 1,
            .buf = &buf[0],
        },
        (struct hgxip_flash_reg_opt_param ){
            .cmd = 0x35,
            .pre_cmd = 0xff,
            .len = 1,
            .buf = &buf[1],
        },
        (struct hgxip_flash_reg_opt_param ){
            .cmd = 0x15,
            .pre_cmd = 0xff,
            .len = 1,
            .buf = &buf[2],
        },
    };
    spi_ioctl(flash->spidev, SPI_XIP_REGS_OPT, (uint32)&rd_reg, 3);
    SPI_NOR_BUS_PRINT("updated, rd: 0x%x, 0x%x, 0x%x, \r\n", 
    buf[0], buf[1], buf[2]);
#endif

#endif
    return 0;
}


//JECEC ID
static void spi_nor_xip_read_jedec_id(struct spi_nor_flash *flash, uint8 *buf)
{
    struct hgxip_flash_reg_opt_param para;
    para.buf = buf;
    para.cmd = SPI_NOR_QPI_JEDEC_ID;
    para.pre_cmd = 0xFF;
    para.len = 3;
    spi_ioctl(flash->spidev, SPI_XIP_REG_OPT, (uint32)&para, 0);
}

//enter power down
static void spi_nor_xip_power_down(struct spi_nor_flash *flash)
{
    struct hgxip_flash_reg_opt_param para;
    para.cmd = SPI_NOR_STANDARD_POWER_DOWN;
    para.pre_cmd = 0xff; 
    para.len = 0;
    spi_ioctl(flash->spidev, SPI_XIP_REG_OPT, (uint32)&para, 0);
}

//release power down
static void spi_nor_xip_release_power_down(struct spi_nor_flash *flash)
{
    struct hgxip_flash_reg_opt_param para;
    para.cmd = SPI_NOR_STANDARD_RELEASE_POWER_DOWN;
    para.pre_cmd = 0xff; 
    para.len = 0;
    spi_ioctl(flash->spidev, SPI_XIP_REG_OPT, (uint32)&para, 0);
}

static void spi_nor_xip_open(struct spi_nor_flash *flash)
{

}

static void spi_nor_xip_close(struct spi_nor_flash *flash)
{

}

static void spi_nor_xip_read_data(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 buf_size)
{
    spi_ioctl(flash->spidev, SPI_XIP_SET_ADR, addr, XIP_READ);
    spi_read(flash->spidev, buf, buf_size);
}

static void spi_nor_xip_page_program(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 buf_size)
{
	struct bpreg_cfg bpup[4];
    if (flash->bp_conv) {
        flash->bp_conv(flash, addr, addr + buf_size, bpup);
        xip_nor_flash_unbp_bakup(flash, flash->bpbk, 2);
        
        xip_nor_flash_unbp_update(flash, bpup, 2);
        spi_ioctl(flash->spidev, SPI_XIP_SET_ADR, addr, XIP_WRITE);
        spi_write(flash->spidev, buf, buf_size);
        xip_nor_flash_unbp_update(flash, flash->bpbk, 2);
    } else {
        spi_ioctl(flash->spidev, SPI_XIP_SET_ADR, addr, XIP_WRITE);
        spi_write(flash->spidev, buf, buf_size);
    }
}

static void spi_nor_xip_sector_erase(struct spi_nor_flash *flash, uint32 addr)
{
    struct bpreg_cfg bpup[4];
    if (flash->bp_conv) {
        flash->bp_conv(flash, addr, addr + 0x1000, bpup);
        xip_nor_flash_unbp_bakup(flash, flash->bpbk, 2);
        xip_nor_flash_unbp_update(flash, bpup, 2);
        spi_ioctl(flash->spidev, SPI_XIP_ERASE_4KB, addr, XIP_ERASE);
        xip_nor_flash_unbp_update(flash, flash->bpbk, 2);    
    } else {
        spi_ioctl(flash->spidev, SPI_XIP_ERASE_4KB, addr, XIP_ERASE);
    }
}

static void spi_nor_xip_block_erase(struct spi_nor_flash *flash, uint32 addr)
{
    struct bpreg_cfg bpup[4];
    if (flash->bp_conv) {
        flash->bp_conv(flash, addr, addr + 0x10000, bpup);
        xip_nor_flash_unbp_bakup(flash, flash->bpbk, 2);
        xip_nor_flash_unbp_update(flash, bpup, 2);
        spi_ioctl(flash->spidev, SPI_XIP_ERASE_64KB, addr, XIP_ERASE);
        xip_nor_flash_unbp_update(flash, flash->bpbk, 2);
    } else {
        spi_ioctl(flash->spidev, SPI_XIP_ERASE_64KB, addr, XIP_ERASE);
    }
}

static void spi_nor_xip_erase(struct spi_nor_flash *flash, uint32 mode, uint32 addr)
{
    switch (mode) {
    case SPI_NOR_ERASE_SECTOR:
        spi_nor_xip_sector_erase(flash, addr);
        break;
    case SPI_NOR_ERASE_BLOCK:
        spi_nor_xip_block_erase(flash, addr);
        break;
    case SPI_NOR_ERASE_CHIP:
        break;
    default:
        break;
    }
}

//erase security registers
static void spi_nor_xip_erase_security_reg(struct spi_nor_flash *flash, uint32 addr)
{
    spi_ioctl(flash->spidev, SPI_XIP_ERASE_SECURITY_REG, addr, 0);
}

//program security registers
static void spi_nor_xip_program_security_reg(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 buf_size)
{
    spi_ioctl(flash->spidev, SPI_XIP_SET_ADR, addr, XIP_SECURITY_WRITE);
    spi_ioctl(flash->spidev, SPI_XIP_PROGRAM_SECURITY_REG, (uint32)buf, buf_size);
}

//read security registers
static void spi_nor_xip_read_security_reg(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 buf_size)
{
    spi_ioctl(flash->spidev, SPI_XIP_SET_ADR, addr, XIP_SECURITY_READ);
    spi_ioctl(flash->spidev, SPI_XIP_READ_SECURITY_REG, (uint32)buf, buf_size);
}

static void spi_nor_xip_custom_read_data(struct spi_nor_flash *flash, uint32_t param)
{
    spi_ioctl(flash->spidev, SPI_XIP_CUSTOM_READ_CMD, param, 0);
}

static void spi_nor_xip_custom_write_data(struct spi_nor_flash *flash, uint32_t param)
{
    spi_ioctl(flash->spidev, SPI_XIP_CUSTOM_WRITE_CMD, param, 0);
}

static void spi_nor_xip_custom_erase(struct spi_nor_flash *flash, uint32_t param)
{
    spi_ioctl(flash->spidev, SPI_XIP_CUSTOM_ERASE_CMD, param, 0);
}

static void spi_nor_xip_reg_opt(struct spi_nor_flash *flash, uint32_t param)
{
    spi_ioctl(flash->spidev, SPI_XIP_REG_OPT, param, 0);
}

static int32 spi_nor_xip_ioctl(struct spi_nor_flash *flash, uint32_t cmd, uint32_t param1, uint32_t param2)
{
	struct spi_nor_regval *reg;
    int ret = RET_OK;
    switch (cmd) {
    case SPI_NOR_CMD_READ_JEDEC_ID:
        spi_nor_xip_read_jedec_id(flash, (uint8_t *)param1);
        break;
    case SPI_NOR_CMD_POWER_DOWN:
        spi_nor_xip_power_down(flash);
        break;
    case SPI_NOR_CMD_POWER_UP:
        spi_nor_xip_release_power_down(flash);
        break;
    case SPI_NOR_CMD_COSTUM_ERASE:
        spi_nor_xip_custom_erase(flash, param1);
        break;
    case SPI_NOR_CMD_COSTUM_READ:
        spi_nor_xip_custom_read_data(flash, param1);
        break;
    case SPI_NOR_CMD_COSTUM_WRITE:
        spi_nor_xip_custom_write_data(flash, param1);
        break;
    case SPI_NOR_CMD_SEC_ERASE:
        spi_nor_xip_erase_security_reg(flash, param1);
        break;
    case SPI_NOR_CMD_SEC_PROGRAM:
        reg = (struct spi_nor_regval *)param2;
        spi_nor_xip_program_security_reg(flash, param1, reg->buff, reg->size);
        break;
    case SPI_NOR_CMD_SEC_READ:
        reg = (struct spi_nor_regval *)param2;
        spi_nor_xip_read_security_reg(flash, param1, reg->buff, reg->size);
        break;
    default:
        break;
    }
    return ret;
}



//SPI NOR function
static const struct spi_nor_bus spinor_bus_list[] = {
    //NORMAL SPI
    {
        spi_nor_standard_open,
        spi_nor_standard_close,
        spi_nor_standard_erase,
        spi_nor_standard_read_data,
        spi_nor_standard_page_program,
        spi_nor_standard_ioctl,
    },
    //DUAL SPI
    {
        spi_nor_standard_open,
        spi_nor_standard_close,
        spi_nor_standard_erase,
        spi_nor_dual_fast_read_dual_output,
        spi_nor_standard_page_program,
        spi_nor_standard_ioctl,
    },
    //QUAD SPI
    {
        spi_nor_standard_open,
        spi_nor_standard_close,
        spi_nor_standard_erase,
        spi_nor_quad_fast_read_quad_output,
        spi_nor_quad_page_program,
        spi_nor_standard_ioctl,
    },
    //QPI
    {
        spi_nor_qpi_open,
        spi_nor_qpi_close,
        spi_nor_qpi_erase,
        spi_nor_qpi_fast_read_quad_io,
        spi_nor_qpi_page_program,
        spi_nor_qpi_ioctl,
    },
    //XIP
    {
        spi_nor_xip_open,
        spi_nor_xip_close,
        spi_nor_xip_erase,
        spi_nor_xip_read_data,
        spi_nor_xip_page_program,
        spi_nor_xip_ioctl,
    },
};

const struct spi_nor_bus   *spi_nor_bus_get(enum spi_nor_mode mode)
{
    ASSERT(mode < ARRAY_SIZE(spinor_bus_list));
    return &spinor_bus_list[mode];
}


