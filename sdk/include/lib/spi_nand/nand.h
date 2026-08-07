#ifndef __NAND_H__
#define __NAND_H__

#include "typesdef.h"
#include "dev.h"
#include "osal/mutex.h"


#define NAND_MAX_CHIPS 5

#define DEV_REG_CNT (4)

#define NAND_MAX_BLOCKS     32768   /**< Max number of Blocks */
#define NAND_MAX_PAGE_SIZE  2048    /**< Max page size of NAND flash */
#define NAND_MAX_OOB_SIZE   64      /**< Max OOB bytes of a NAND flash page */
#define NAND_ECC_SIZE       256     /**< ECC block size */
#define NAND_ECC_BYTES      2       /**< ECC bytes per ECC block */


#define NAND_ERASE_FAIL      0x01
#define NAND_PROGRAM_FAIL    0x02
#define NAND_ECC_ERROR       0X04

struct NandOobFree {
    uint32_t Offset;
    uint32_t Length;
};

/*
 * ECC layout control structure. Exported to user space for
 * diagnosis and to allow creation of raw images
 */
struct NandEccLayout {
    uint32_t EccBytes;
    uint32_t EccPos;
    uint32_t OobAvail;
    struct NandOobFree OobFree;
};

/*
 * This enum contains ECC Mode
 */
typedef enum {
    NAND_ECC_NONE,
    NAND_ECC_SOFT_BCH,
    NAND_ECC_ONDIE  /**< On-Die ECC */
} Nand_EccMode;

struct NandEccCtrl {
    Nand_EccMode Mode;
    uint32_t EccSteps;      /**< Number of ECC steps for the flash page */
    uint32_t EccSize;       /**< ECC size */
    uint32_t EccBytes;      /**< Number of ECC bytes for a block */
    uint32_t EccTotalBytes; /**< Total number of ECC bytes for Page */
    uint32_t Strength;
    struct NandEccLayout *Layout;
    void *Priv;
};

struct NandBuffers {
    uint8_t DataBuf[NAND_MAX_PAGE_SIZE];
    uint8_t EccCode[NAND_MAX_OOB_SIZE]; /**< Buffer for stored ECC */
    uint8_t EccCalc[NAND_MAX_OOB_SIZE]; /**< Buffer for calculated ECC */
    uint32_t Rcache;
};

enum nand_ctl_cmd {
    NAND_OP_RD_SET,
    NAND_OP_WR_SET,
    NAND_FREQ_SET,
    NAND_IS_BAD_BLOCK,
    NAND_READ_OOB,
    NAND_WRITE_OOB,
    NAND_ECC_STATUS,
    NAND_STATUS,
    NAND_N_OF_BLKS,
    NAND_PG2PG_PAGE,
};

enum nand_req_type {
    NAND_READ_PAGE_REQ,
    NAND_READ_PAGE_COL_REQ,
    NAND_WRITE_PAGE_REQ,
    NAND_ERASE_REQ,
    NAND_PG2PG_REQ,
    NAND_STATUS_REQ,
};

struct nand_req {
    uint32 type;
    union {
        struct _pc {
            uint32 page;
            uint32 col;
        } pc;
        struct _p2p {
            uint32 src;
            uint32 dst;
        } p2p;
        uint32 page;
        uint32 blk;
        uint32 ret;
    };
    uint8 *buf;
    uint32 len;
};

struct nand_dev;
struct nand_ops {
    int (*init)(struct nand_dev *nd);
    int (*rdpg)(struct nand_dev *nd, uint32 page, uint8 *buf, uint32 len);
    int (*wrpg)(struct nand_dev *nd, uint32 page, uint8 *buf, uint32 len);
    int (*erase)(struct nand_dev *nd, uint32 blk);
    int (*ctl)(struct nand_dev *nd, uint32 cmd, uint32 param1, uint32 param2);
};

struct nand_dev {
    struct dev_obj dev;
    struct spi_nand *sn;
    struct nand_ops *ops;
    uint8_t *bbt;
    uint32_t rd_ofs;
    uint32_t status;
    int (*pg_rd)(struct nand_dev *nd, uint32 page, uint8 *buf, uint32 len);
    int (*pg_wr)(struct nand_dev *nd, uint32 page, uint8 *buf, uint32 len);
    struct os_mutex mutex;
    uint32_t \
    ofs_shift_block : 8,
                    ofs_shift_page  : 8,
                    ofs_shift_oob   : 8,
                    page_shift_block: 8;
    struct NandEccCtrl EccCtrl;               /**< ECC configuration parameters */
    struct NandBuffers *Buffers;
};

int nand_init(struct nand_dev *nand);
int nand_req(struct nand_dev *nd, struct nand_req *req);

uint8_t bbt_is_bad(struct nand_dev *nand, int block);

void bbt_mark_entry(struct nand_dev *nand, int block);

#endif
