#ifndef __NAND_CHIP_H__
#define __NAND_CHIP_H__


#include "typesdef.h"
#include "lib/spi_nand/nand.h"
#include "lib/spi_nand/spi_mem.h"


#define SPI_NAND_CMD_ONLY(__cmd) \
SPI_MEM_OP_BASIC(__cmd, SPI_MEM_OP_DETAIL(0, 0, OP_ADR_DAT_111, SPI_MEM_OP_NO_DATA), 0, 0)

#define SPI_NAND_WITHOUT_DATA30(__cmd, __addr, __line) \
SPI_MEM_OP_BASIC(__cmd, SPI_MEM_OP_DETAIL(3, 0, __line, SPI_MEM_OP_NO_DATA), __addr, 0)

#define SPI_NAND_WITHOUT_DATA20(__cmd, __addr, __line) \
SPI_MEM_OP_BASIC(__cmd, SPI_MEM_OP_DETAIL(2, 0, __line, SPI_MEM_OP_NO_DATA), __addr, 0)

#define SPI_NAND_READ_REG(__addr, __buf) \
SPI_MEM_OP_BASIC(0x0f, \
    SPI_MEM_OP_DETAIL(1, 0, OP_ADR_DAT_111, SPI_MEM_OP_DATA_IN), __addr, __buf)

#define SPI_NAND_WRITE_REG(__addr, __buf) \
SPI_MEM_OP_BASIC(0x1f, \
    SPI_MEM_OP_DETAIL(1, 0, OP_ADR_DAT_111, SPI_MEM_OP_DATA_OUT), __addr, __buf)

#define SPI_NAND_IN_WITHOUT_ADDR(__cmd, __dummy, __buf) \
SPI_MEM_OP_BASIC(__cmd, \
    SPI_MEM_OP_DETAIL(0, __dummy, OP_ADR_DAT_111, SPI_MEM_OP_DATA_IN), 0, __buf)

#define SPI_NAND_READ_CACHE_OP() \
SPI_MEM_OP_BASIC(0x03, \
    SPI_MEM_OP_DETAIL(2, 1, OP_ADR_DAT_111, SPI_MEM_OP_DATA_IN), 0, NULL)

#define SPI_NAND_READ_CACHE_OP114() \
SPI_MEM_OP_BASIC(0x6b, \
    SPI_MEM_OP_DETAIL(2, 1, OP_ADR_DAT_114, SPI_MEM_OP_DATA_IN), 0, NULL)

#define SPI_NAND_WRITE_CACHE_OP() \
SPI_MEM_OP_BASIC(0x02, \
    SPI_MEM_OP_DETAIL(2, 0, OP_ADR_DAT_111, SPI_MEM_OP_DATA_OUT), 0, NULL)

#define SPI_NAND_WRITE_CACHE_OP114() \
SPI_MEM_OP_BASIC(0x32, \
    SPI_MEM_OP_DETAIL(2, 0, OP_ADR_DAT_114, SPI_MEM_OP_DATA_OUT), 0, NULL)


struct spi_nand_info {
    uint8  id[4];
    uint32 blks;
    uint32 pgs_per_blk;
    uint32 pg_size;
    uint32 oob_size;
};

struct spi_nand {
	struct nand_dev nand;
    struct spi_nand_info info;
    struct spi_mem_op *op_rd_cache;
    struct spi_mem_op *op_wr_cache;
	
    /*
        page : 2^11
        block: 2^17
    */
    uint8_t  reg_adr[DEV_REG_CNT];
    uint8_t  reg_ini[DEV_REG_CNT];
    struct spi_mem *ctrl;
};

int sn_cmd_only(struct spi_nand *sn, uint8 cmd);

int sn_wren(struct spi_nand *sn);

int sn_wrdi(struct spi_nand *sn);

int sn_rd_reg(struct spi_nand *sn, uint32 addr, uint8 *buf);

int sn_wr_reg(struct spi_nand *sn, uint32 addr, uint8 reg);

int sn_read_jedid(struct spi_nand *sn, uint8 *id);

void sn_read_cache(struct spi_nand *sn, uint32 ofs, uint8 *buf, uint32 len);

void sn_write_cache(struct spi_nand *sn, uint32 ofs, uint8 *buf, uint32 len);

int sn_dump_pg(struct spi_nand *sn, uint32 pg);

int sn_prog_pg(struct spi_nand *sn, uint32 pg);

int sn_erase(struct spi_nand *sn, uint32 blk);

int sn_init(struct spi_nand *sn);

int spi_nand_attach(struct spi_nand *sn);
#endif
