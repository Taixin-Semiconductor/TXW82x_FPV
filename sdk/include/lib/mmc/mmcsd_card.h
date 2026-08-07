/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2011-07-25     weety     first version
 */

#ifndef __MMCSD_CARD_H__
#define __MMCSD_CARD_H__

#include "lib/mmc/mmcsd_host.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SD_SCR_BUS_WIDTH_1  (1 << 0)
#define SD_SCR_BUS_WIDTH_4  (1 << 2)

struct mmcsd_cid {
    uint8  mid;       /* ManufacturerID */
    uint8  prv;       /* Product Revision */
    uint16 oid;       /* OEM/Application ID */
    uint32 psn;       /* Product Serial Number */
    uint8  pnm[5];    /* Product Name */
    uint8  reserved1;/* reserved */
    uint16 mdt;       /* Manufacturing Date */
    uint8  crc;       /* CID CRC */
    uint8  reserved2;/* not used, always 1 */
};

struct mmcsd_csd {
    uint8      csd_structure;  /* CSD register version */
    uint8      taac;
    uint8      nsac;
    uint8      tran_speed; /* max data transfer rate */
    uint16     card_cmd_class; /* card command classes */
    uint8      rd_blk_len; /* max read data block length */
    uint8      rd_blk_part;
    uint8      wr_blk_misalign;
    uint8      rd_blk_misalign;
    uint8      dsr_imp;    /* DSR implemented */
    uint8      c_size_mult;    /* CSD 1.0 , device size multiplier */
    uint32     c_size;     /* device size */
    uint8      r2w_factor;
    uint8      wr_blk_len; /* max wtire data block length */
    uint8      wr_blk_partial;
    uint8      csd_crc;

};

struct sd_scr {
    uint8      sd_version;
    uint8      sd_bus_widths;
};

struct sdio_cccr {
    uint8      sdio_version;
    uint8      sd_version;
    uint8      direct_cmd: 1,     /*  Card Supports Direct Commands during data transfer
                                                   only SD mode, not used for SPI mode */
               multi_block: 1,   /*  Card Supports Multi-Block */
               read_wait: 1,      /*  Card Supports Read Wait
                                       only SD mode, not used for SPI mode */
               suspend_resume: 1, /*  Card supports Suspend/Resume
                                       only SD mode, not used for SPI mode */
               s4mi: 1,            /* generate interrupts during a 4-bit
                                      multi-block data transfer */
               e4mi: 1,            /*  Enable the multi-block IRQ during
                                       4-bit transfer for the SDIO card */
               low_speed: 1,     /*  Card  is  a  Low-Speed  card */
               low_speed_4: 1;   /*  4-bit support for Low-Speed cards */

    uint8      bus_width: 1,    /* Support SDIO bus width, 1:4bit, 0:1bit */
               cd_disable: 1,    /*  Connect[0]/Disconnect[1] the 10K-90K ohm pull-up
                                     resistor on CD/DAT[3] (pin 1) of the card */
               power_ctrl: 1,   /* Support Master Power Control */
               high_speed: 1;   /* Support High-Speed  */


};

struct sdio_cis {
    uint16     manufacturer;
    uint16     product;
    uint16     func0_blk_size;
    uint32     max_tran_speed;
};

/*
 * SDIO function CIS tuple (unknown to the core)
 */
struct sdio_function_tuple {
    struct sdio_function_tuple *next;
    uint8 code;
    uint8 size;
    uint8 *data;
};

struct sdio_function;
typedef void (*sdio_irq_handler_t)(struct sdio_function *func);

/*
 * SDIO function devices
 */
struct sdio_function {
    struct mmcsd_card     *card;      /* the card this device belongs to */
    sdio_irq_handler_t     irq_handler;   /* IRQ callback */
    uint8      num;        /* function number */

    uint8      func_code;   /*  Standard SDIO Function interface code  */
    uint16     manufacturer;       /* manufacturer id */
    uint16     product;        /* product id */

    uint32     max_blk_size;   /* maximum block size */
    uint32     cur_blk_size;   /* current block size */

    uint64     enable_timeout_val; /* max enable timeout in msec */

    struct sdio_function_tuple *tuples;

    void            *priv;
};

#define SDIO_MAX_FUNCTIONS      2

struct mmcsd_card {
    struct list_head list;
    struct mmcsd_host *host;
    uint32 rca;        /* card addr */
    uint32 resp_cid[4];    /* card CID register */
    uint32 resp_csd[4];    /* card CSD register */
    uint32 resp_scr[2];    /* card SCR register */

    uint16 tacc_clks;  /* data access time by ns */
    uint32 tacc_ns;    /* data access time by clk cycles */
    uint32 max_data_rate;  /* max data transfer rate */
    uint32 card_capacity;  /* card capacity, unit:KB */
    uint32 card_blksize;   /* card block size */
    uint32 erase_size; /* erase size in sectors */
    uint16 card_type;
#define CARD_TYPE_MMC                   0 /* MMC card */
#define CARD_TYPE_SD                    1 /* SD card */
#define CARD_TYPE_SDIO                  2 /* SDIO card */
#define CARD_TYPE_SDIO_COMBO            3 /* SD combo (IO+mem) card */

    uint16 flags;
#define CARD_FLAG_HIGHSPEED  (1 << 0)   /* SDIO bus speed 50MHz */
#define CARD_FLAG_SDHC       (1 << 1)   /* SDHC card */
#define CARD_FLAG_SDXC       (1 << 2)   /* SDXC card */

    struct sd_scr    scr;
    struct mmcsd_csd csd;
    uint32     hs_max_data_rate;  /* max data transfer rate in high speed mode */

    uint8      sdio_function_num;  /* totol number of SDIO functions */
    struct sdio_cccr    cccr;  /* common card info */
    struct sdio_cis     cis;  /* common tuple info */
    struct sdio_function *sdio_function[SDIO_MAX_FUNCTIONS + 1]; /* SDIO functions (devices) */
    struct list_head blk_devices;  /* for block device list */
};

#ifdef __cplusplus
}
#endif

#endif
