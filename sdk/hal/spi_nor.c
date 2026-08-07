#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "osal/semaphore.h"
#include "dev.h"
#include "dev/spi/hgspi_xip.h"
#include "osal/mutex.h"
#include "osal/string.h"
#include "hal/spi.h"
#include "hal/spi_nor.h"

int32 spi_nor_open(struct spi_nor_flash *flash)
{
    int32 ret = RET_OK;

    if (flash == NULL) {
        return -ENODEV;
    }
    ASSERT(flash->bus);
    if (atomic_read(&flash->ref) == 0) {
        ret = spi_open(flash->spidev, flash->spi_config.clk, SPI_MASTER_MODE,
                       flash->spi_config.wire_mode, flash->spi_config.clk_mode);
        ASSERT(!ret);
        if (flash->bus->open) {
            flash->bus->open(flash);
        }
    }
    atomic_inc(&flash->ref);
    return ret;
}

void spi_nor_close(struct spi_nor_flash *flash)
{
    if (flash) {
    ASSERT(flash->bus);

    if (atomic_dec_and_test(&flash->ref)) {
        if (flash->bus->close) {
            flash->bus->close(flash);
        }
        spi_set_cs(flash->spidev, flash->spi_config.cs, 1);
        spi_close(flash->spidev);
    }
}
}

void spi_nor_read(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 len)
{
    if (flash) {
    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    flash->bus->read(flash, addr, buf, len);
    os_mutex_unlock(&flash->lock);
}
}

void spi_nor_write(struct spi_nor_flash *flash, uint32 addr, uint8 *buf, uint32 len)
{
    uint32 remain_size;

    if (flash == NULL) {
        return;
    }

    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    /* Flash is written according to page */
    remain_size = flash->page_size - (addr % flash->page_size);
    if (remain_size && (len > remain_size)) {
        flash->bus->program(flash, addr, buf, remain_size);
        addr += remain_size;
        buf  += remain_size;
        len  -= remain_size;
    }
    while (len > flash->page_size) {
        flash->bus->program(flash, addr, buf, flash->page_size);
        addr += flash->page_size;
        buf  += flash->page_size;
        len  -= flash->page_size;
    }

    flash->bus->program(flash, addr, buf, len);
    os_mutex_unlock(&flash->lock);
}

void spi_nor_sector_erase(struct spi_nor_flash *flash, uint32 sector_addr)
{
    if (flash) {
    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    flash->bus->erase(flash, SPI_NOR_ERASE_SECTOR, sector_addr);
    os_mutex_unlock(&flash->lock);
}
}

void spi_nor_block_erase(struct spi_nor_flash *flash, uint32 block_addr)
{
    if (flash) {
    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    flash->bus->erase(flash, SPI_NOR_ERASE_BLOCK, block_addr);
    os_mutex_unlock(&flash->lock);
}
}

void spi_nor_chip_erase(struct spi_nor_flash *flash)
{
    if (flash) {
    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    flash->bus->erase(flash, SPI_NOR_ERASE_CHIP, 0);
    os_mutex_unlock(&flash->lock);
  
}
}

void spi_nor_sec_erase(struct spi_nor_flash *flash, uint32 addr)
{
    if (flash) {
    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    flash->bus->ioctl(flash, SPI_NOR_CMD_SEC_ERASE, addr, 0);
    os_mutex_unlock(&flash->lock);
}
}

void spi_nor_sec_program(struct spi_nor_flash *flash, uint32 addr, struct spi_nor_regval *values)
{
    if (flash) {
    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    flash->bus->ioctl(flash, SPI_NOR_CMD_SEC_PROGRAM, addr, (uint32)values);
    os_mutex_unlock(&flash->lock);
}
}

void spi_nor_sec_read(struct spi_nor_flash *flash, uint32 addr, struct spi_nor_regval *values)
{
    if (flash) {
    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    flash->bus->ioctl(flash, SPI_NOR_CMD_SEC_READ, addr, (uint32)values);
    os_mutex_unlock(&flash->lock);
}
}

void spi_nor_custom_read(struct spi_nor_flash *flash, void *param)
{
    if (flash) {
    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    flash->bus->ioctl(flash, SPI_NOR_CMD_COSTUM_READ, (uint32)(param), 0);
    os_mutex_unlock(&flash->lock);
}
}

void spi_nor_custom_write(struct spi_nor_flash *flash, void *param)
{
    if (flash) {
    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    flash->bus->ioctl(flash, SPI_NOR_CMD_COSTUM_WRITE, (uint32)(param), 0);
    os_mutex_unlock(&flash->lock);
}
}

void spi_nor_custom_erase(struct spi_nor_flash *flash, void *param)
{
    if (flash) {
    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    flash->bus->ioctl(flash, SPI_NOR_CMD_COSTUM_ERASE, (uint32)(param), 0);
    os_mutex_unlock(&flash->lock);
}
}

void spi_nor_power_down(struct spi_nor_flash *flash)
{
    if (flash) {
    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    flash->bus->ioctl(flash, SPI_NOR_CMD_POWER_DOWN, 0, 0);
    os_mutex_unlock(&flash->lock);
}
}

void spi_nor_power_up(struct spi_nor_flash *flash)
{
    if (flash) {
    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    flash->bus->ioctl(flash, SPI_NOR_CMD_POWER_UP, 0, 0);
    os_mutex_unlock(&flash->lock);
}
}

void spi_nor_read_jedec_id(struct spi_nor_flash *flash, uint8 *buff)
{
    if (flash) {
    ASSERT(flash->bus);
    os_mutex_lock(&flash->lock, osWaitForever);
    flash->bus->ioctl(flash, SPI_NOR_CMD_READ_JEDEC_ID, (uint32)buff, 0);
    os_mutex_unlock(&flash->lock);
}
}

__init int32 spi_nor_attach(struct spi_nor_flash *flash, uint32 dev_id)
{
    uint8 id[3];
    extern struct bp_info bp_info;
    extern int xip_nor_flash_unbp_convert(struct spi_nor_flash *flash, 
        uint32_t addrl, uint32_t addru, struct bpreg_cfg *bp);    
    extern int xip_nor_flash_unbp_convert_tbl(struct spi_nor_flash *flash, 
        uint32_t addrl, uint32_t addru, struct bpreg_cfg *bp);

    ASSERT(flash->size && flash->sector_size);
    os_mutex_init(&flash->lock);
    flash->bus = spi_nor_bus_get(flash->mode);
    flash->bpi = &bp_info;
    flash->bp_conv = xip_nor_flash_unbp_convert;

    for (int i = 0;i<bp_info.area_cnt;i++)
    {
        os_printf("{0x%x, [0x%x, 0x%x]}\r\n", bp_info.areas[i].bpv | (bp_info.areas[i].tb<<3),
            bp_info.areas[i].area[0], bp_info.areas[i].area[1]);
    }
    ASSERT(flash->bus);

    spi_nor_open(flash);
    flash->bus->ioctl(flash, SPI_NOR_CMD_READ_JEDEC_ID, (uint32_t)id, 0);
    spi_nor_close(flash);

    if ((id[0] | id[1] | id[2]) == 0 || ((id[0] & id[1] & id[2]) == 0xff)) {
        return -EIO;
    }

    flash->vendor_id  = id[0];
    flash->product_id = (id[1] << 8) | id[2];
    return dev_register(dev_id, (struct dev_obj *)flash);
}

