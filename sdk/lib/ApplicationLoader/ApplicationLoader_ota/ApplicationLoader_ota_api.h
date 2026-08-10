#ifndef APPLICATION_LOADER_OTA_API_H
#define APPLICATION_LOADER_OTA_API_H

#include "basic_include.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APPLICATION_LOADER_OTA_INVALID_ADDR       0xFFFFFFFFUL
#define APPLICATION_LOADER_OTA_MARKER             0x5A69U
#define APPLICATION_LOADER_OTA_MARKER_BYTES       2U
#define APPLICATION_LOADER_OTA_VERIFY_BYTES       256U

#ifndef APPLICATION_LOADER_OTA_ERASE_BY_SECTOR
#define APPLICATION_LOADER_OTA_ERASE_BY_SECTOR    0
#endif

struct spi_nor_flash;

struct ApplicationLoader_ota_fwinfo {
    struct spi_nor_flash *flash0;
    uint32 addr0;
    uint32 size0;
    struct spi_nor_flash *flash1;
    uint32 addr1;
    uint32 size1;
};

enum ApplicationLoader_ota_error {
    APPLICATION_LOADER_OTA_OK = 0,
    APPLICATION_LOADER_OTA_ERR_PARAM = -1,
    APPLICATION_LOADER_OTA_ERR_BUSY = -2,
    APPLICATION_LOADER_OTA_ERR_METADATA = -3,
    APPLICATION_LOADER_OTA_ERR_CURRENT_LOADER = -4,
    APPLICATION_LOADER_OTA_ERR_TARGET_ADDR = -5,
    APPLICATION_LOADER_OTA_ERR_FLASH = -6,
    APPLICATION_LOADER_OTA_ERR_RANGE = -7,
    APPLICATION_LOADER_OTA_ERR_OVERLAP = -8,
    APPLICATION_LOADER_OTA_ERR_HEADER = -9,
    APPLICATION_LOADER_OTA_ERR_VERSION = -10,
    APPLICATION_LOADER_OTA_ERR_ORDER = -11,
    APPLICATION_LOADER_OTA_ERR_ERASE_VERIFY = -12,
    APPLICATION_LOADER_OTA_ERR_WRITE_VERIFY = -13,
    APPLICATION_LOADER_OTA_ERR_COMMIT = -14,
    APPLICATION_LOADER_OTA_ERR_NO_SESSION = -15,
    APPLICATION_LOADER_OTA_ERR_COMMIT_UNCERTAIN = -16,
    APPLICATION_LOADER_OTA_ERR_CONFIG = -17,
    APPLICATION_LOADER_OTA_ERR_ADDR_MISMATCH = -18
};

int32 ApplicationLoader_ota_fwinfo_get(
    struct ApplicationLoader_ota_fwinfo *pinfo);
int32 ApplicationLoader_ota_write_fw(uint32 total_len, uint32 offset,
                                     uint8 *data, uint16 len);
void ApplicationLoader_ota_abort(void);
int ApplicationLoader_ota_is_running(void);

#ifdef __cplusplus
}
#endif

#endif
