/**
  ******************************************************************************
  * @file    application_loader_protocol.h
  * @brief   Application and secondary-loader PSRAM update protocol.
  ******************************************************************************
  */
#ifndef __APPLICATION_LOADER_PROTOCOL_H__
#define __APPLICATION_LOADER_PROTOCOL_H__

#include "typesdef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PSRAM UPDATE TO FLASH */
#define PSRAM_UPDATE_USER_SRAM_BASE        (0x2006BA00UL)
#define PSRAM_UPDATE_USER_SRAM_BYTES       (0x500U)
#define PSRAM_UPDATE_CONTROL_BYTES         (0x100U)
#define PSRAM_UPDATE_CONTROL_ADDR          (PSRAM_UPDATE_USER_SRAM_BASE + \
                                            PSRAM_UPDATE_USER_SRAM_BYTES - \
                                            PSRAM_UPDATE_CONTROL_BYTES)

#define PSRAM_UPDATE_MAGIC                 (0x50535550UL) /* "PSUP" */
#define PSRAM_UPDATE_NODE_MAGIC            (0x50534E44UL) /* "PSND" */
#define PSRAM_UPDATE_PROTOCOL_VERSION      (2U)
#define PSRAM_UPDATE_FLAG_NONE             (0U)

#define PSRAM_UPDATE_FLASH_PAGE_BYTES      (256U)
#define PSRAM_UPDATE_FLASH_SECTOR_BYTES    (4096U)
#define PSRAM_UPDATE_MAX_NODE_COUNT        (4096U)
#define PSRAM_UPDATE_NODE_END              (0U)

struct psram_update_request {
    uint32 magic;
    uint16 version;
    uint16 header_bytes;
    uint32 first_node_addr;
    uint32 image_size;
    uint32 image_crc32;
    uint32 node_count;
    uint32 flags;
    uint32 sequence;
    uint32 header_crc32;
} __attribute__((packed));

struct psram_update_node {
    uint32 magic;
    uint16 version;
    uint16 header_bytes;
    uint32 next_node_addr;
    uint32 data_addr;
    uint32 data_bytes;
    uint32 firmware_offset;
    uint32 header_crc32;
} __attribute__((packed));

#define PSRAM_UPDATE_REQUEST_CRC_BYTES     \
    (sizeof(struct psram_update_request) - sizeof(uint32))
#define PSRAM_UPDATE_NODE_CRC_BYTES        \
    (sizeof(struct psram_update_node) - sizeof(uint32))

typedef char psram_update_request_must_fit_control[
    (sizeof(struct psram_update_request) <= PSRAM_UPDATE_CONTROL_BYTES) ? 1 : -1];
typedef char psram_update_request_size_must_be_36[
    (sizeof(struct psram_update_request) == 36U) ? 1 : -1];
typedef char psram_update_node_size_must_be_28[
    (sizeof(struct psram_update_node) == 28U) ? 1 : -1];

#ifdef __cplusplus
}
#endif

#endif /* __APPLICATION_LOADER_PROTOCOL_H__ */
