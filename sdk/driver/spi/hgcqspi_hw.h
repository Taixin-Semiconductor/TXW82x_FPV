#ifndef _HGCQSPI_HW_H_
#define _HGCQSPI_HW_H_


#ifdef __cplusplus
extern "C" {
#endif
    
/** @brief SPI register structure
  * @{
  */
struct hgcqspi_hw {
    __IO  uint32 CONFIG;        //0x00
    __IO  uint32 DEV_RDINSTR_CONFIG;    //0x04
    __IO  uint32 DEV_WRINSTR_CONFIG;    //0x08
    __IO  uint32 DEV_DELAY;        //0x0c
    __IO  uint32 READ_DATA_CAPTURE;   //0x10
    __IO  uint32 DEV_SIZE_CONFIG;    //0x14
    __IO  uint32 SRAM_PARTITION;      //0x18
    __IO  uint32 IND_AHB_TRIGGER;     //0x1c
    __IO  uint32 PERIPH_CFG;          //0x20
    __IO  uint32 REMAP_ADD;       //0x24
    __IO  uint32 MODE_BIT;           //0x28
    __IO  uint32 SRAM_FILL_LEVEL;     //0x2c
    __IO  uint32 TX_THRESH;       //0x30
    __IO  uint32 RX_THRESH;       //0x34
    __IO  uint32 WRITE_COMP_CTRL;       //0x38
    __IO  uint32 MAX_NO_OF_POLLS;       //0x3c
    __IO  uint32 INT_STATUS;       //0x40
    __IO  uint32 INT_MASK;           //0x44
          uint32 RESERVED0[2];    
    __IO  uint32 WRITE_PROT_L;       //0x50
    __IO  uint32 WRITE_PROT_U;       //0x54
    __IO  uint32 WRITE_PROT_CTRL;       //0x58
          uint32 RESERVED1[1];    
    __IO  uint32 IND_RD_XFER_CTRL;    //0x60
    __IO  uint32 IND_RD_XFER_WMARK;   //0x64
    __IO  uint32 IND_RD_XFER_START_ADD;       //0x68
    __IO  uint32 IND_RD_XFER_NUM_BYTES;       //0x6c
    __IO  uint32 IND_WR_XFER_CTRL;        //0x70
    __IO  uint32 IND_WR_XFER_WMARK;       //0x74
    __IO  uint32 IND_WR_XFER_START_ADD;       //0x78
    __IO  uint32 IND_WR_XFER_NUM_BYTES;       //0x7c
    __IO  uint32 IND_AHB_TRIGGER_RANGE;       //0x80
          uint32 RESERVED2[2];    
    __IO  uint32 FLASH_CMD_CTRL_MEM;        //0x8C
    __IO  uint32 FLASH_CMD_CTRL;        //0x90
    __IO  uint32 FLASH_CMD_ADD;        //0x94
          uint32 RESERVED3[2];    
    __IO  uint32 FLASH_CMD_RDATA_L;        //0xa0
    __IO  uint32 FLASH_CMD_RDATA_U;        //0xa4
    __IO  uint32 FLASH_CMD_WDATA_L;        //0xa8
    __IO  uint32 FLASH_CMD_WDATA_U;        //0xac
    __IO  uint32 FLASH_STATUS;           //0xb0
    __IO  uint32 PHY_CONFIG;             //0xb4
    __IO  uint32 PHY_DLL_MASTER_CONFIG;    //0xb8
    __IO  uint32 DLL_OBSERVABLE_L;         //0xbc
    __IO  uint32 DLL_OBSERVABLE_U;           //0xc0
          uint32 RESERVED4[5];    
    __IO  uint32 NEW_ADD_CONFIG0;         //d8
    __IO  uint32 NEW_ADD_CONFIG1;         //dc
    __IO  uint32 OPCODE_EXT_L;           //E0
    __IO  uint32 OPCODE_EXT_U;           //E4          
    __IO  uint32 READ_DATA_CAPTURE_EXT;    //0xE8 
    __IO  uint32 WRITE_PROT_CTRL_EXT;    //0xEC
    __IO  uint32 DEV_RDINSTR_CONFIG_CS1;    //0xF0
    __IO  uint32 DEV_WRINSTR_CONFIG_CS1;    //0xF4
    __IO  uint32 MONITOR;  //0xF8, for OSPI
    __IO  uint32 MODULE_ID;           //0xfc
};


/** @addtogroup SPI MODULE REGISTER 
  * @{
  */

#define CQSPI_NAME          "cadence-qspi"
#define CQSPI_MAX_CHIPSELECT        1//16
#define CQSPI_AHB_TRIG_ADDR         (0x18000000)

/* Operation timeout value */
#define CQSPI_TIMEOUT_MS                500
#define CQSPI_READ_TIMEOUT_MS           10

/* Instruction type */
#define CQSPI_INST_TYPE_SINGLE          0
#define CQSPI_INST_TYPE_DUAL            1
#define CQSPI_INST_TYPE_QUAD            2

#define CQSPI_DUMMY_CLKS_PER_BYTE       8
#define CQSPI_DUMMY_BYTES_MAX           4
#define CQSPI_DUMMY_CLKS_MAX            31

#define CQSPI_STIG_DATA_LEN_MAX         8

/* Register map */
#define CQSPI_REG_CONFIG                0x00
#define CQSPI_REG_CONFIG_ENABLE_MASK        BIT(0)
#define CQSPI_REG_CONFIG_DECODE_MASK        BIT(9)
#define CQSPI_REG_CONFIG_CHIPSELECT_LSB     10
#define CQSPI_REG_CONFIG_DMA_MASK       BIT(15)
#define CQSPI_REG_CONFIG_BAUD_LSB       19
#define CQSPI_REG_CONFIG_IDLE_LSB       31
#define CQSPI_REG_CONFIG_CHIPSELECT_MASK    0xF
#define CQSPI_REG_CONFIG_BAUD_MASK      0xF

#define CQSPI_REG_RD_INSTR          0x04
#define CQSPI_REG_RD_INSTR_OPCODE_LSB       0
#define CQSPI_REG_RD_INSTR_TYPE_INSTR_LSB   8
#define CQSPI_REG_RD_INSTR_TYPE_ADDR_LSB    12
#define CQSPI_REG_RD_INSTR_TYPE_DATA_LSB    16
#define CQSPI_REG_RD_INSTR_MODE_EN_LSB      20
#define CQSPI_REG_RD_INSTR_DUMMY_LSB        24
#define CQSPI_REG_RD_INSTR_TYPE_INSTR_MASK  0x3
#define CQSPI_REG_RD_INSTR_TYPE_ADDR_MASK   0x3
#define CQSPI_REG_RD_INSTR_TYPE_DATA_MASK   0x3
#define CQSPI_REG_RD_INSTR_DUMMY_MASK       0x1F

#define CQSPI_REG_WR_INSTR          0x08
#define CQSPI_REG_WR_INSTR_OPCODE_LSB       0
#define CQSPI_REG_WR_INSTR_TYPE_ADDR_LSB    12
#define CQSPI_REG_WR_INSTR_TYPE_DATA_LSB    16

#define CQSPI_REG_DELAY             0x0C
#define CQSPI_REG_DELAY_TSLCH_LSB       0
#define CQSPI_REG_DELAY_TCHSH_LSB       8
#define CQSPI_REG_DELAY_TSD2D_LSB       16
#define CQSPI_REG_DELAY_TSHSL_LSB       24
#define CQSPI_REG_DELAY_TSLCH_MASK      0xFF
#define CQSPI_REG_DELAY_TCHSH_MASK      0xFF
#define CQSPI_REG_DELAY_TSD2D_MASK      0xFF
#define CQSPI_REG_DELAY_TSHSL_MASK      0xFF

#define CQSPI_REG_READCAPTURE           0x10
#define CQSPI_REG_READCAPTURE_BYPASS_LSB    0
#define CQSPI_REG_READCAPTURE_DELAY_LSB     1
#define CQSPI_REG_READCAPTURE_DELAY_MASK    0xF

#define CQSPI_REG_SIZE              0x14
#define CQSPI_REG_SIZE_ADDRESS_LSB      0
#define CQSPI_REG_SIZE_PAGE_LSB         4
#define CQSPI_REG_SIZE_BLOCK_LSB        16
#define CQSPI_REG_SIZE_ADDRESS_MASK     0xF
#define CQSPI_REG_SIZE_PAGE_MASK        0xFFF
#define CQSPI_REG_SIZE_BLOCK_MASK       0x3F

#define CQSPI_REG_SRAMPARTITION         0x18
#define CQSPI_REG_INDIRECTTRIGGER       0x1C

#define CQSPI_REG_DMA               0x20
#define CQSPI_REG_DMA_SINGLE_LSB        0
#define CQSPI_REG_DMA_BURST_LSB         8
#define CQSPI_REG_DMA_SINGLE_MASK       0xFF
#define CQSPI_REG_DMA_BURST_MASK        0xFF

#define CQSPI_REG_REMAP             0x24
#define CQSPI_REG_MODE_BIT          0x28

#define CQSPI_REG_SDRAMLEVEL            0x2C
#define CQSPI_REG_SDRAMLEVEL_RD_LSB     0
#define CQSPI_REG_SDRAMLEVEL_WR_LSB     16
#define CQSPI_REG_SDRAMLEVEL_RD_MASK        0xFFFF
#define CQSPI_REG_SDRAMLEVEL_WR_MASK        0xFFFF

#define CQSPI_REG_IRQSTATUS         0x40
#define CQSPI_REG_IRQMASK           0x44

#define CQSPI_REG_INDIRECTRD            0x60
#define CQSPI_REG_INDIRECTRD_START_MASK     BIT(0)
#define CQSPI_REG_INDIRECTRD_CANCEL_MASK    BIT(1)
#define CQSPI_REG_INDIRECTRD_DONE_MASK      BIT(5)

#define CQSPI_REG_INDIRECTRDWATERMARK       0x64
#define CQSPI_REG_INDIRECTRDSTARTADDR       0x68
#define CQSPI_REG_INDIRECTRDBYTES       0x6C

#define CQSPI_REG_CMDCTRL           0x90
#define CQSPI_REG_CMDCTRL_EXECUTE_MASK      BIT(0)
#define CQSPI_REG_CMDCTRL_INPROGRESS_MASK   BIT(1)
#define CQSPI_REG_CMDCTRL_WR_BYTES_LSB      12
#define CQSPI_REG_CMDCTRL_WR_EN_LSB     15
#define CQSPI_REG_CMDCTRL_ADD_BYTES_LSB     16
#define CQSPI_REG_CMDCTRL_ADDR_EN_LSB       19
#define CQSPI_REG_CMDCTRL_RD_BYTES_LSB      20
#define CQSPI_REG_CMDCTRL_RD_EN_LSB     23
#define CQSPI_REG_CMDCTRL_OPCODE_LSB        24
#define CQSPI_REG_CMDCTRL_WR_BYTES_MASK     0x7
#define CQSPI_REG_CMDCTRL_ADD_BYTES_MASK    0x3
#define CQSPI_REG_CMDCTRL_RD_BYTES_MASK     0x7

#define CQSPI_REG_INDIRECTWR            0x70
#define CQSPI_REG_INDIRECTWR_START_MASK     BIT(0)
#define CQSPI_REG_INDIRECTWR_CANCEL_MASK    BIT(1)
#define CQSPI_REG_INDIRECTWR_DONE_MASK      BIT(5)

#define CQSPI_REG_INDIRECTWRWATERMARK       0x74
#define CQSPI_REG_INDIRECTWRSTARTADDR       0x78
#define CQSPI_REG_INDIRECTWRBYTES       0x7C

#define CQSPI_REG_CMDADDRESS            0x94
#define CQSPI_REG_CMDREADDATALOWER      0xA0
#define CQSPI_REG_CMDREADDATAUPPER      0xA4
#define CQSPI_REG_CMDWRITEDATALOWER     0xA8
#define CQSPI_REG_CMDWRITEDATAUPPER     0xAC

/* Interrupt status bits */
#define CQSPI_REG_IRQ_MODE_ERR          BIT(0)
#define CQSPI_REG_IRQ_UNDERFLOW         BIT(1)
#define CQSPI_REG_IRQ_IND_COMP          BIT(2)
#define CQSPI_REG_IRQ_IND_RD_REJECT     BIT(3)
#define CQSPI_REG_IRQ_WR_PROTECTED_ERR      BIT(4)
#define CQSPI_REG_IRQ_ILLEGAL_AHB_ERR       BIT(5)
#define CQSPI_REG_IRQ_WATERMARK         BIT(6)
#define CQSPI_REG_IRQ_IND_SRAM_FULL     BIT(12)

#define CQSPI_IRQ_MASK_RD       (CQSPI_REG_IRQ_WATERMARK    | \
                                 CQSPI_REG_IRQ_IND_SRAM_FULL    | \
                                 CQSPI_REG_IRQ_IND_COMP)

#define CQSPI_IRQ_MASK_WR       (CQSPI_REG_IRQ_IND_COMP     | \
                                 CQSPI_REG_IRQ_WATERMARK    | \
                                 CQSPI_REG_IRQ_UNDERFLOW)

#define CQSPI_IRQ_STATUS_MASK       0x1FFFF


/*
 * Note on opcode nomenclature: some opcodes have a format like
 * SPINOR_OP_FUNCTION{4,}_x_y_z. The numbers x, y, and z stand for the number
 * of I/O lines used for the opcode, address, and data (respectively). The
 * FUNCTION has an optional suffix of '4', to represent an opcode which
 * requires a 4-byte (32-bit) address.
 */

/* Flash opcodes. */
#define SPINOR_OP_WREN      0x06    /* Write enable */
#define SPINOR_OP_RDSR      0x05    /* Read status register */
#define SPINOR_OP_WRSR      0x01    /* Write status register 1 byte */
#define SPINOR_OP_READ      0x03    /* Read data bytes (low frequency) */
#define SPINOR_OP_READ_FAST 0x0b    /* Read data bytes (high frequency) */
#define SPINOR_OP_READ_1_1_2    0x3b    /* Read data bytes (Dual SPI) */
#define SPINOR_OP_READ_1_1_4    0x6b    /* Read data bytes (Quad SPI) */
#define SPINOR_OP_PP        0x02    /* Page program (up to 256 bytes) */
#define SPINOR_OP_BE_4K     0x20    /* Erase 4KiB block */
#define SPINOR_OP_BE_4K_PMC 0xd7    /* Erase 4KiB block on PMC chips */
#define SPINOR_OP_BE_32K    0x52    /* Erase 32KiB block */
#define SPINOR_OP_CHIP_ERASE    0xc7    /* Erase whole flash chip */
#define SPINOR_OP_SE        0xd8    /* Sector erase (usually 64KiB) */
#define SPINOR_OP_RDID      0x9f    /* Read JEDEC ID */
#define SPINOR_OP_RDCR      0x35    /* Read configuration register */
#define SPINOR_OP_RDFSR     0x70    /* Read flag status register */

/* 4-byte address opcodes - used on Spansion and some Macronix flashes. */
#define SPINOR_OP_READ4     0x13    /* Read data bytes (low frequency) */
#define SPINOR_OP_READ4_FAST    0x0c    /* Read data bytes (high frequency) */
#define SPINOR_OP_READ4_1_1_2   0x3c    /* Read data bytes (Dual SPI) */
#define SPINOR_OP_READ4_1_1_4   0x6c    /* Read data bytes (Quad SPI) */
#define SPINOR_OP_PP_4B     0x12    /* Page program (up to 256 bytes) */
#define SPINOR_OP_SE_4B     0xdc    /* Sector erase (usually 64KiB) */

/* Used for SST flashes only. */
#define SPINOR_OP_BP        0x02    /* Byte program */
#define SPINOR_OP_WRDI      0x04    /* Write disable */
#define SPINOR_OP_AAI_WP    0xad    /* Auto address increment word program */

/* Used for Macronix and Winbond flashes. */
#define SPINOR_OP_EN4B      0xb7    /* Enter 4-byte mode */
#define SPINOR_OP_EX4B      0xe9    /* Exit 4-byte mode */

/* Used for Spansion flashes only. */
#define SPINOR_OP_BRWR      0x17    /* Bank register write */

/* Used for Micron flashes only. */
#define SPINOR_OP_RD_EVCR      0x65    /* Read EVCR register */
#define SPINOR_OP_WD_EVCR      0x61    /* Write EVCR register */

/* Status Register bits. */
#define SR_WIP          BIT(0)  /* Write in progress */
#define SR_WEL          BIT(1)  /* Write enable latch */
/* meaning of other SR_* bits may differ between vendors */
#define SR_BP0          BIT(2)  /* Block protect 0 */
#define SR_BP1          BIT(3)  /* Block protect 1 */
#define SR_BP2          BIT(4)  /* Block protect 2 */
#define SR_TB           BIT(5)  /* Top/Bottom protect */
#define SR_SRWD         BIT(7)  /* SR write protect */

#define SR_QUAD_EN_MX       BIT(6)  /* Macronix Quad I/O */

/* Enhanced Volatile Configuration Register bits */
#define EVCR_QUAD_EN_MICRON BIT(7)  /* Micron Quad I/O */

/* Flag Status Register bits */
#define FSR_READY       BIT(7)

/* Configuration Register bits. */
#define CR_QUAD_EN_SPAN     BIT(1)  /* Spansion Quad I/O */

enum qspi_dev_size {
    QSPI_DEV_SIZE_8MB,
    QSPI_DEV_SIZE_16MB,
    QSPI_DEV_SIZE_32MB,
    QSPI_DEV_SIZE_4MB,
};

/**
 * @brief   ll_qspi_enable
 * @param   p_qspi  :QSPI register structure pointer
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_enable(struct hgcqspi_hw *p_qspi)
{
    p_qspi->CONFIG |= CQSPI_REG_CONFIG_ENABLE_MASK;
}


/**
 * @brief   ll_qspi_disable
 * @param   p_qspi  :QSPI register structure pointer
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_disable(struct hgcqspi_hw *p_qspi)
{
    p_qspi->CONFIG &= ~CQSPI_REG_CONFIG_ENABLE_MASK;
}


/**
 * @brief   ll_qspi_cs_sel
 * @param   p_qspi  :QSPI register structure pointer
 * @param   cs      :cs select [0:3]
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_cs_sel(struct hgcqspi_hw *p_qspi, uint8 cs)
{
    p_qspi->CONFIG = (p_qspi->CONFIG & ~(CQSPI_REG_CONFIG_CHIPSELECT_MASK << CQSPI_REG_CONFIG_CHIPSELECT_LSB)) |
                        ((~(BIT(cs)) & CQSPI_REG_CONFIG_CHIPSELECT_MASK)  << CQSPI_REG_CONFIG_CHIPSELECT_LSB);
}

/**
 * @brief   
 * @param   p_qspi  :QSPI register structure pointer
 * @param   
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_ahb_decoder_enable(struct hgcqspi_hw *p_qspi)
{
    p_qspi->CONFIG |= BIT(23);
}

/**
 * @brief   
 * @param   p_qspi  :QSPI register structure pointer
 * @param   
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_set_device_size(struct hgcqspi_hw *p_qspi, uint8 dev_index, enum qspi_dev_size dev_size)
{
    uint32 dev_size_reg = p_qspi->DEV_SIZE_CONFIG;

    dev_size_reg &= ~((BIT(22)|BIT(21)) << (dev_index*2));
    dev_size_reg |= (dev_size << (21 + dev_index*2));
    
    p_qspi->DEV_SIZE_CONFIG = dev_size_reg;
}


/**
 * @brief   
 * @param   p_qspi  :QSPI register structure pointer
 * @param   
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_set_page_size(struct hgcqspi_hw *p_qspi, uint32 page_size)
{
    uint32 dev_size_reg = p_qspi->DEV_SIZE_CONFIG;

    dev_size_reg &= 0xFFFF000F;
    dev_size_reg |= (page_size & 0x0FFF) << 4;
    
    p_qspi->DEV_SIZE_CONFIG = dev_size_reg;
}

/**
 * @brief   
 * @param   p_qspi  : QSPI register structure pointer
 * @param   size    : blocksiez = power(2, size)
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_set_block_size(struct hgcqspi_hw *p_qspi, uint32 size)
{
    uint32 dev_size_reg = p_qspi->DEV_SIZE_CONFIG;

    dev_size_reg &= 0xFFE0FFFF;
    dev_size_reg |= (size & 0x01F) << 16;
    
    p_qspi->DEV_SIZE_CONFIG = dev_size_reg;
}

/**
 * @brief   
 * @param   p_qspi  :QSPI register structure pointer
 * @param   
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_ahb_decoder_disable(struct hgcqspi_hw *p_qspi)
{
    p_qspi->CONFIG &= ~(BIT(23));
}

/**
 * @brief   ll_qspi_set_clk
 * @param   p_qspi  :QSPI register structure pointer
 * @param   clk_hz  :
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_set_clk(struct hgcqspi_hw *p_qspi, uint32 clk_hz)
{
    uint32 div;
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

    /* Recalculate the baudrate divisor based on QSPI specification. */
#if 1//FPGA
    div = DIV_ROUND_UP(96000000, 2 * clk_hz) - 1;
#else
    /* fill this : sys_clock_cfg.syspll_clk */
    div = DIV_ROUND_UP(384000000, 2 * clk_hz) - 1;
#endif
    /* avoid qspi div2 bug */
    if (0 == div) div = 1;
    
    p_qspi->CONFIG = (p_qspi->CONFIG & ~(CQSPI_REG_CONFIG_BAUD_MASK << CQSPI_REG_CONFIG_BAUD_LSB)) |
                        ((div & CQSPI_REG_CONFIG_BAUD_MASK) << CQSPI_REG_CONFIG_BAUD_LSB);
}

/**
 * @brief   ll_qspi_is_idle
 * @param   p_qspi  :QSPI register structure pointer
 * @param           :
 * @retval  none
 */
__STATIC_INLINE bool ll_qspi_is_idle(struct hgcqspi_hw *p_qspi)
{
    return (p_qspi->CONFIG & BIT(31)) ? TRUE : FALSE;
}


/**
 * @brief   ll_qspi_is_cmd_done
 * @param   p_qspi  :QSPI register structure pointer
 * @param           :
 * @retval  none
 */
__STATIC_INLINE bool ll_qspi_is_cmd_done(struct hgcqspi_hw *p_qspi)
{
    return (p_qspi->FLASH_CMD_CTRL & BIT(1)) ? FALSE : TRUE;
}


/**
 * @brief   
 * @param   p_qspi  :QSPI register structure pointer
 * @param           :
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_wel_enable(struct hgcqspi_hw *p_qspi)
{
    p_qspi->DEV_WRINSTR_CONFIG &= ~(BIT(8));
}

/**
 * @brief   
 * @param   p_qspi  :QSPI register structure pointer
 * @param           :
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_wel_disable(struct hgcqspi_hw *p_qspi)
{
    p_qspi->DEV_WRINSTR_CONFIG |= BIT(8);
}

/**
 * @brief   
 * @param   p_qspi  :QSPI register structure pointer
 * @param           :
 * @retval  none
 */
__STATIC_INLINE bool ll_qspi_is_wel_enable(struct hgcqspi_hw *p_qspi)
{
    return (p_qspi->DEV_WRINSTR_CONFIG & BIT(8)) ? FALSE : TRUE;
}

/**
 * @brief   
 * @param   p_qspi  :QSPI register structure pointer
 * @param           :
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_poll_enable(struct hgcqspi_hw *p_qspi)
{
    p_qspi->WRITE_COMP_CTRL &= ~(BIT(14));
}

/**
 * @brief   
 * @param   p_qspi  :QSPI register structure pointer
 * @param           :
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_poll_disable(struct hgcqspi_hw *p_qspi)
{
    p_qspi->WRITE_COMP_CTRL |= BIT(14);
}


/**
 * @brief   
 * @param   p_qspi  :QSPI register structure pointer
 * @param           :
 * @retval  none
 */
__STATIC_INLINE bool ll_qspi_is_poll_enable(struct hgcqspi_hw *p_qspi)
{
    return (p_qspi->WRITE_COMP_CTRL & BIT(14)) ? FALSE : TRUE;
}

/**
 * @brief   
 * @param   p_qspi  :QSPI register structure pointer
 * @param   width   : 0/1/2 for data width 1/2/4
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_set_rd_attr(struct hgcqspi_hw *p_qspi, 
                                                uint8 cmd, 
                                                uint8 cmd_width,
                                                uint8 addr_width,
                                                uint8 dat_width,
                                                uint8 dummys,
                                                uint8 ddr_en)
{
    p_qspi->DEV_RDINSTR_CONFIG = ((dummys & 0x1F) << 24)    |
                                 ((dat_width & 0x3) << 16)  |
                                 ((addr_width & 0x3) << 12) |
                                 ((ddr_en & 0x1) << 10)     |
                                 ((cmd_width & 0x3) << 8)   |
                                 cmd;
}

/**
 * @brief   
 * @param   p_qspi  :QSPI register structure pointer
 * @param   width   : 0/1/2 for data width 1/2/4
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_set_wr_attr(struct hgcqspi_hw *p_qspi, 
                                                uint8 cmd, 
                                                uint8 cmd_width,
                                                uint8 addr_width,
                                                uint8 dat_width,
                                                uint8 dummys,
                                                uint8 wel_disable)
{
    p_qspi->DEV_WRINSTR_CONFIG = ((dummys & 0x1F) << 24)    |
                                 ((dat_width & 0x3) << 16)  |
                                 ((addr_width & 0x3) << 12) |
                                 ((wel_disable & 0x1) << 8) |
                                 cmd;
}


/**
 * @brief   
 * @param   p_qspi  : QSPI register structure pointer
 * @param   width   : 
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_set_clk_mode(struct hgcqspi_hw *p_qspi,        uint8 clock_mode)
{
    p_qspi->CONFIG = (p_qspi->CONFIG & ~(BIT(1)|BIT(2))) | ((clock_mode & 0x3) << 1);
}

/**
 * @brief   
 * @param   p_qspi  : QSPI register structure pointer
 * @param   delay_ref_clk   :  the delay_ref_clk less than divor is better 
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_set_sample_dalay(struct hgcqspi_hw *p_qspi,        uint8 delay_ref_clk)
{
    delay_ref_clk &= 0xF;
    p_qspi->DEV_DELAY = (p_qspi->DEV_DELAY & ~(0xFUL<<1)) | ((delay_ref_clk) << 1);
}

/**
 * @brief   
 * @param   p_qspi  : QSPI register structure pointer
 * @param   delay_ref_clk   :  the delay_ref_clk less than divor is better 
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_remap_enable(struct hgcqspi_hw *p_qspi,        uint32 offset)
{
    while(!ll_qspi_is_idle(p_qspi));
    p_qspi->REMAP_ADD = offset;
    p_qspi->CONFIG |= (BIT(16));
    __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
    while(!ll_qspi_is_idle(p_qspi));
    //csi_dcache_invalid();
}

/**
 * @brief   
 * @param   p_qspi  : QSPI register structure pointer
 * @param   delay_ref_clk   :  the delay_ref_clk less than divor is better 
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_remap_disable(struct hgcqspi_hw *p_qspi)
{
    while(!ll_qspi_is_idle(p_qspi));
    p_qspi->CONFIG &= ~(BIT(16));
    __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
    while(!ll_qspi_is_idle(p_qspi));
    //csi_dcache_invalid();
}

/**
 * @brief   
 * @param   p_qspi  : QSPI register structure pointer
 * @param   delay_ref_clk   :  the delay_ref_clk less than divor is better 
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_dac_enable(struct hgcqspi_hw *p_qspi)
{
    p_qspi->CONFIG |= (BIT(7));
}

/**
 * @brief   
 * @param   p_qspi  : QSPI register structure pointer
 * @param   delay_ref_clk   :  the delay_ref_clk less than divor is better 
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_dac_disable(struct hgcqspi_hw *p_qspi)
{
    p_qspi->CONFIG &= ~(BIT(7));
}

/**
 * @brief   
 * @param   p_qspi  : QSPI register structure pointer
 * @param   delay_ref_clk   :  the delay_ref_clk less than divor is better 
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_cs_fix_enable(struct hgcqspi_hw *p_qspi)
{
    p_qspi->WRITE_PROT_CTRL |= (BIT(3));
}

/**
 * @brief   
 * @param   p_qspi  : QSPI register structure pointer
 * @param   delay_ref_clk   :  the delay_ref_clk less than divor is better 
 * @retval  none
 */
__STATIC_INLINE void ll_qspi_cs_fix_disable(struct hgcqspi_hw *p_qspi)
{
    p_qspi->WRITE_PROT_CTRL &= ~(BIT(3));
}

__STATIC_INLINE uint32 ll_qspi_is_xip_mode(struct hgcqspi_hw *p_qspi)
{
    return (p_qspi->CONFIG & (BIT(17)|BIT(18))) ? 1:0;
}

__STATIC_INLINE uint32 ll_qspi_is_dtr_mode(struct hgcqspi_hw *p_qspi)
{
    return (p_qspi->DEV_RDINSTR_CONFIG & BIT(10)) ? 1:0;
}


/**
 * @brief   hg_qspi_set_encrypt_disable_range
 * @param   p_qspi  : QSPI register structure pointer
 * @param   index   : rang 0/1 
 * @param   st_addr_1k : 
 * @param   end_addr_1k : 
 * @retval  none
 */
__STATIC_INLINE void hg_qspi_set_encrypt_disable_range(struct hgcqspi_hw *p_qspi, uint8 index, uint32 st_addr_1k, uint32 end_addr_1k)
{
    uint32 reg = (st_addr_1k & 0x7FFF) | ((end_addr_1k & 0x7FFF) << 16);
    SYSCTRL_REG_OPT(
        if (index) {
            SYSCTRL->QSPI_ENCDEC_CON1 = reg;
        } else {
            SYSCTRL->QSPI_ENCDEC_CON0 = (SYSCTRL->QSPI_ENCDEC_CON0 & BIT(31)) | reg;
        }
    );  
}


enum hgcqspi_flags {
    hgcqspi_flags_suspend,
};



#ifdef __cplusplus
}
#endif

#endif /* _HGCQSPI_H_ */

