#ifndef __SPI_MEM_H__
#define __SPI_MEM_H__

#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/spi.h"
#include "osal/mutex.h"

#define NAND_PRINT_EN 1
#if (NAND_PRINT_EN)
#define NAND_PRINT(fmt, args...)                     printf(fmt, ##args)
#else
#define NAND_PRINT(...)
#endif

enum spi_mem_line {
    OP_ADR_DAT_111,
    OP_ADR_DAT_112,
    OP_ADR_DAT_114,
    OP_ADR_DAT_118,
    OP_ADR_DAT_122,
    OP_ADR_DAT_144,
    OP_ADR_DAT_188,
    OP_ADR_DAT_222,
    OP_ADR_DAT_444,
    OP_ADR_DAT_888,
};

enum spi_mem_dir {
    SPI_MEM_OP_NO_DATA,
    SPI_MEM_OP_DATA_IN,
    SPI_MEM_OP_DATA_OUT,
};

#define SPI_MEM_OP_DETAIL(_naddr, _ndummy, _line, _dir) \
{.naddr = _naddr, .ndummy = _ndummy, .line = _line, .dir = _dir}

#define SPI_MEM_OP_BASIC(_cmd, _detail, _addr, _data) \
{.cmd = _cmd, .detail = _detail, .addr = _addr, .buf = _data}

struct spi_mem_op_detail {
    uint16 naddr  : 4,
           ndummy : 4,
           line   : 4, 
           dir    : 4;
};

struct spi_mem_op {
    uint16                       cmd;
    struct spi_mem_op_detail     detail;
    uint32                       addr;
    uint8                        *buf;
    uint32                       len;
};

struct spic {
    struct spi_device *spi;
    unsigned int spi_devid;
    unsigned int freq;
    unsigned int work_mode;
    unsigned int wire_mode;
    unsigned int clk_mode;
    unsigned int ntarget;
    unsigned int cur_cs;
};

struct spi_mem {
    struct spic spic;
    int (*exec)(struct spi_mem *mem, struct spi_mem_op *op);
    int (*target)(struct spi_mem *mem, uint32 cs);
};

int spi_mem_init(struct spi_mem *sm);

#endif
