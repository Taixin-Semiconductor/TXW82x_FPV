/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2011-07-25     weety     first version
 */

#ifndef __CORE_H__
#define __CORE_H__

#include "lib/mmc/mmcsd_host.h"
#include "lib/mmc/mmcsd_card.h"
#include "lib/mmc/mmcsd_cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef RT_MMCSD_DBG
#define mmcsd_dbg(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#else
#define mmcsd_dbg(fmt, ...)
#endif

struct mmcsd_data {
    uint32  blksize;
    uint32  blks;
    uint32 *buf;
    int32   err;
    uint32  flags;
#define DATA_DIR_WRITE  (1 << 0)
#define DATA_DIR_READ   (1 << 1)
#define DATA_STREAM (1 << 2)

    unsigned int        bytes_xfered;

    struct mmcsd_cmd *stop;      /* stop command */
    struct mmcsd_req *mrq;       /* associated request */

    uint32  timeout_ns;
    uint32  timeout_clks;
};

struct mmcsd_cmd {
    uint32  cmd_code;
    uint32  arg;
    uint32  resp[4];
    uint32  flags;
    /*rsponse types
     *bits:0~3
     */
#define RESP_MASK   (0xF)
#define RESP_NONE   (0)
#define RESP_R1     (1 << 0)
#define RESP_R1B    (2 << 0)
#define RESP_R2     (3 << 0)
#define RESP_R3     (4 << 0)
#define RESP_R4     (5 << 0)
#define RESP_R6     (6 << 0)
#define RESP_R7     (7 << 0)
#define RESP_R5     (8 << 0)    /*SDIO command response type*/
    /*command types
     *bits:4~5
     */
#define CMD_MASK    (3 << 4)        /* command type */
#define CMD_AC      (0 << 4)
#define CMD_ADTC    (1 << 4)
#define CMD_BC      (2 << 4)
#define CMD_BCR     (3 << 4)

#define resp_type(cmd)  ((cmd)->flags & RESP_MASK)

    /*spi rsponse types
     *bits:6~8
     */
#define RESP_SPI_MASK   (0x7 << 6)
#define RESP_SPI_R1 (1 << 6)
#define RESP_SPI_R1B    (2 << 6)
#define RESP_SPI_R2 (3 << 6)
#define RESP_SPI_R3 (4 << 6)
#define RESP_SPI_R4 (5 << 6)
#define RESP_SPI_R5 (6 << 6)
#define RESP_SPI_R7 (7 << 6)

#define spi_resp_type(cmd)  ((cmd)->flags & RESP_SPI_MASK)
    /*
     * These are the command types.
     */
#define cmd_type(cmd)   ((cmd)->flags & CMD_MASK)

    int32   retries;    /* max number of retries */
    int32   err;

    struct mmcsd_data *data;
    struct mmcsd_req *mrq;       /* associated request */
};

struct mmcsd_req {
    struct mmcsd_data  *data;
    struct mmcsd_cmd   *cmd;
    struct mmcsd_cmd   *stop;
};

/*the following is response bit*/
#define R1_OUT_OF_RANGE     (1 << 31)   /* er, c */
#define R1_ADDRESS_ERROR    (1 << 30)   /* erx, c */
#define R1_BLOCK_LEN_ERROR  (1 << 29)   /* er, c */
#define R1_ERASE_SEQ_ERROR      (1 << 28)   /* er, c */
#define R1_ERASE_PARAM      (1 << 27)   /* ex, c */
#define R1_WP_VIOLATION     (1 << 26)   /* erx, c */
#define R1_CARD_IS_LOCKED   (1 << 25)   /* sx, a */
#define R1_LOCK_UNLOCK_FAILED   (1 << 24)   /* erx, c */
#define R1_COM_CRC_ERROR    (1 << 23)   /* er, b */
#define R1_ILLEGAL_COMMAND  (1 << 22)   /* er, b */
#define R1_CARD_ECC_FAILED  (1 << 21)   /* ex, c */
#define R1_CC_ERROR     (1 << 20)   /* erx, c */
#define R1_ERROR        (1 << 19)   /* erx, c */
#define R1_UNDERRUN     (1 << 18)   /* ex, c */
#define R1_OVERRUN      (1 << 17)   /* ex, c */
#define R1_CID_CSD_OVERWRITE    (1 << 16)   /* erx, c, CID/CSD overwrite */
#define R1_WP_ERASE_SKIP    (1 << 15)   /* sx, c */
#define R1_CARD_ECC_DISABLED    (1 << 14)   /* sx, a */
#define R1_ERASE_RESET      (1 << 13)   /* sr, c */
#define R1_STATUS(x)            (x & 0xFFFFE000)
#define R1_CURRENT_STATE(x) ((x & 0x00001E00) >> 9) /* sx, b (4 bits) */
#define R1_READY_FOR_DATA   (1 << 8)    /* sx, a */
#define R1_APP_CMD      (1 << 5)    /* sr, c */


#define R1_SPI_IDLE     (1 << 0)
#define R1_SPI_ERASE_RESET  (1 << 1)
#define R1_SPI_ILLEGAL_COMMAND  (1 << 2)
#define R1_SPI_COM_CRC      (1 << 3)
#define R1_SPI_ERASE_SEQ    (1 << 4)
#define R1_SPI_ADDRESS      (1 << 5)
#define R1_SPI_PARAMETER    (1 << 6)
/* R1 bit 7 is always zero */
#define R2_SPI_CARD_LOCKED  (1 << 8)
#define R2_SPI_WP_ERASE_SKIP    (1 << 9)    /* or lock/unlock fail */
#define R2_SPI_LOCK_UNLOCK_FAIL R2_SPI_WP_ERASE_SKIP
#define R2_SPI_ERROR        (1 << 10)
#define R2_SPI_CC_ERROR     (1 << 11)
#define R2_SPI_CARD_ECC_ERROR   (1 << 12)
#define R2_SPI_WP_VIOLATION (1 << 13)
#define R2_SPI_ERASE_PARAM  (1 << 14)
#define R2_SPI_OUT_OF_RANGE (1 << 15)   /* or CSD overwrite */
#define R2_SPI_CSD_OVERWRITE    R2_SPI_OUT_OF_RANGE

#define CARD_BUSY   0x80000000  /* Card Power up status bit */

/* R5 response bits */
#define R5_COM_CRC_ERROR    (1 << 15)
#define R5_ILLEGAL_COMMAND  (1 << 14)
#define R5_ERROR            (1 << 11)
#define R5_FUNCTION_NUMBER  (1 << 9)
#define R5_OUT_OF_RANGE     (1 << 8)
#define R5_STATUS(x)        (x & 0xCB00)
#define R5_IO_CURRENT_STATE(x)  ((x & 0x3000) >> 12)

#define MMCSD_HOST_PLUGED       0
#define MMCSD_HOST_UNPLUGED     1

int mmcsd_wait_cd_changed(uint32 timeout);
void mmcsd_host_lock(struct mmcsd_host *host);
void mmcsd_host_unlock(struct mmcsd_host *host);
void mmcsd_req_complete(struct mmcsd_host *host);
void mmcsd_send_request(struct mmcsd_host *host, struct mmcsd_req *req);
int32 mmcsd_send_cmd(struct mmcsd_host *host, struct mmcsd_cmd *cmd, int retries);
int32 mmcsd_go_idle(struct mmcsd_host *host);
int32 mmcsd_spi_read_ocr(struct mmcsd_host *host, uint32 high_capacity, uint32 *ocr);
int32 mmcsd_all_get_cid(struct mmcsd_host *host, uint32 *cid);
int32 mmcsd_get_cid(struct mmcsd_host *host, uint32 *cid);
int32 mmcsd_get_csd(struct mmcsd_card *card, uint32 *csd);
int32 mmcsd_select_card(struct mmcsd_card *card);
int32 mmcsd_deselect_cards(struct mmcsd_card *host);
int32 mmcsd_spi_use_crc(struct mmcsd_host *host, uint32 use_crc);
void mmcsd_set_chip_select(struct mmcsd_host *host, uint32 mode);
void mmcsd_set_clock(struct mmcsd_host *host, uint32 clk);
void mmcsd_set_bus_mode(struct mmcsd_host *host, uint32 mode);
void mmcsd_set_bus_width(struct mmcsd_host *host, uint32 width);
void mmcsd_set_data_timeout(struct mmcsd_data *data, const struct mmcsd_card *card);
uint32 mmcsd_select_voltage(struct mmcsd_host *host, uint32 ocr);
void mmcsd_power_up(struct mmcsd_host *host);
void mmcsd_power_off(struct mmcsd_host *host);
void mmcsd_change(struct mmcsd_host *host);
struct mmcsd_host *mmcsd_alloc_host(void);
void mmcsd_free_host(struct mmcsd_host *host);
int mmcsd_core_init(void);

int32 mmcsd_blk_probe(struct mmcsd_card *card);
void mmcsd_blk_remove(struct mmcsd_card *card);


#ifdef __cplusplus
}
#endif

#endif

