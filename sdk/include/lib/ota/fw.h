#ifndef __FW_H__
#define __FW_H__

#ifdef __cplusplus
extern "C" {
#endif

#define fw_ota_dbg(fmt, ...)                //os_printf(fmt, ##__VA_ARGS__)

/* The OTA marker and marker size are only applicable to huge-ic chips. */
#define OTA_FLASH_MARKER                    0x5A69
#define OTA_FLASH_MARKER_SIZE               2
#define OTA_VERIFY_READ_BUF_SIZE            256

/* OTA info */
struct ota_fwinfo {
    struct spi_nor_flash   *flash0;
    uint32                  addr0;
    struct spi_nor_flash   *flash1;
    uint32                  addr1;
};

/* OTA control */
struct ota_ctrl {
    struct ota_fwinfo ota_info;
    struct spi_nor_flash *p_flash;
    uint32 base_addr;
    uint32 local_off;
    uint32 marker;
    uint32 earse_locate;
    uint8  verify_read_buf[OTA_VERIFY_READ_BUF_SIZE];
};

extern int32 libota_write_fw_ah(uint32 tot_len, uint32 off, uint8 *data, uint16 len);
extern int32 libotaV2_write_fw(uint32 tot_len, uint8 is_once_earse, uint8 isSector, uint8 isVerify, uint32 off, uint8 *data, uint16 len);
extern int32 ota_fwinfo_get(struct ota_fwinfo *pinfo);

#ifdef TXW4002ACK803
#define libota_write_fw(tot_len,off,data,len)   libota_write_fw_ah(tot_len,off,data,len)
#else
#define libota_write_fw(tot_len,off,data,len)   libotaV2_write_fw(tot_len,0,0,0,off,data,len)
#endif

#ifdef __cplusplus
}
#endif

#endif

