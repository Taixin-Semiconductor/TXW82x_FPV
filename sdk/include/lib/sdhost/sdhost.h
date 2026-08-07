#ifndef SDHOST_H
#define SDHOST_H

#define SDHC_WARN_PRINTF(fmt, ...) os_printf(KERN_DEBUG""fmt, ##__VA_ARGS__)
#define SDHC_ERR_PRINTF(fmt, ...)  os_printf(KERN_ERR""fmt, ##__VA_ARGS__)

enum{
	MMCSD_NO_ERR      = 0,
    MMCSD_CMD_ERR     = BIT(0),
    MMCSD_DAT_ERR     = BIT(1),
    MMCSD_STP_ERR     = BIT(2),
	MMCSD_INT_VLE     = BIT(3),
    MMCSD_BUF_ERR     = BIT(6),
    MMCSD_SMP_ERR     = BIT(7),
    MMCSD_CMP_TIMEOUT = BIT(8),
    MMCSD_CMP_DATERR  = BIT(9),
}TYPE_ENUM_MMCSD_ERR;

enum{
    MMCSD_SMP_SINGLE  = 0,
    MMCSD_SMP_DELAY_CHAIN,
    MMCSD_SMP_DELAY_HALF_CLOCK,
    MMCSD_SMP_DELAY_ONE_CLOCK, 
}TYPE_ENUM_MMCSD_SAMPLE;

enum {
    MMCSD_CARD_STATUS_IDLE = 0,
    MMCSD_CARD_STATUS_READY,
    MMCSD_CARD_STATUS_IDENT,
    MMCSD_CARD_STATUS_STBY,
    MMCSD_CARD_STATUS_TRAN,
    MMCSD_CARD_STATUS_DATA,
    MMCSD_CARD_STATUS_RCV,
    MMCSD_CARD_STATUS_PRG,
    MMCSD_CARD_STATUS_DIS,
    MMCSD_CARD_STATUS_RESERVED,
}TYPE_ENUM_MMCSD_CARD_STATUS;

typedef enum
{
    LL_SDHC_DLY_NONE,
    LL_SDHC_DLY_CHAIN,
    LL_SDHC_DLY_HALF_SYSCLK,
    LL_SDHC_DLY_ONE_SYSCLK,
} TYPE_LL_SDHC_DELAY_SYSCLK;

typedef enum
{
    LL_SDHC_ALL_SMP_CFG_DIS,
    LL_SDHC_CMD_SMP_CFG_EN,
    LL_SDHC_DAT_SMP_CFG_EN,
    LL_SDHC_ALL_SMP_CFG_EN,
} TYPE_LL_SDHC_SMP_CFG;

typedef enum {
    SDHC_SMP_CMD_SUCC       = BIT(0),
    SDHC_SMP_DAT_SUCC       = BIT(1),
    SDHC_SMP_WRITE_CRC_SUCC = BIT(2),
    SDHC_SMP_ALL_SUCC       = 0x7,
} TYPE_LL_SDHC_SMP_FLAG;

typedef enum
{
    LL_SDHC_IOCTRL_SET_CLOCK,
    LL_SDHC_IOCTRL_SET_BUS_WIDTH,
    LL_SDHC_IOCTRL_SET_POWER_MODE,
    LL_SDHC_IOCTRL_SET_SMP,
    LL_SDHC_IOCTRL_SET_DELAY_TYPE,
    LL_SDHC_IOCTRL_SET_DAT_OF_STOP_CLK,
} TYPE_LL_SDHC_IOCTRL;

typedef enum
{
    LL_SDHC_SPEED_FULL = 0,
    LL_SDHC_SPEED_HIGH,
}TYPE_LL_SDHC_SPEED;

typedef enum
{
    LL_SDHC_LINE_ONE = 0,
    LL_SDHC_LINE_FOUR,
}TYPE_LL_SDHC_WIDTH;

enum {
    SDHC_SINGLE_CODE_SUPPORT_DIS = 0,
    SDHC_SINGLE_CODE_SUPPORT_EN  = 1,
};

enum {
    LL_SDHC_RETRY_DEFAULT = 0,
    LL_SDHC_RETRY_SELECT_POINT,
    LL_SDHC_RETRY_CHECK,
    LL_SDHC_RETRY_SUCC,
    LL_SDHC_RETRY_ERR,
};

struct rt_mmcsd_csd {
	uint8		csd_structure;	/* CSD register version */
	uint8		taac;
	uint8		nsac;
	uint8		tran_speed;	/* max data transfer rate */
	uint16		card_cmd_class;	/* card command classes */
	uint8		rd_blk_len;	/* max read data block length */
	uint8		rd_blk_part;
	uint8		wr_blk_misalign;
	uint8		rd_blk_misalign;
	uint8		dsr_imp;	/* DSR implemented */
	uint8		c_size_mult;	/* CSD 1.0 , device size multiplier */
	uint32		c_size;		/* device size */
	uint8		r2w_factor;
	uint8		wr_blk_len;	/* max wtire data block length */
	uint8		wr_blk_partial;
	uint8		csd_crc;
	
};

struct rt_mmcsd_io_cfg {
    uint32    clock;          /* clock rate */
    uint16    vdd;

/* vdd stores the bit number of the selected voltage range from below. */

    uint8 bus_mode;       /* command output mode */

#define MMCSD_BUSMODE_OPENDRAIN 1
#define MMCSD_BUSMODE_PUSHPULL  2

    uint8 chip_select;        /* SPI chip select */

#define MMCSD_CS_IGNORE     0
#define MMCSD_CS_HIGH       1
#define MMCSD_CS_LOW        2

    uint8 power_mode;     /* power supply mode */

#define MMCSD_POWER_OFF     0
#define MMCSD_POWER_UP      1
#define MMCSD_POWER_ON      2

    uint8 bus_width;      /* data bus width */

#define MMCSD_BUS_WIDTH_1       0
#define MMCSD_BUS_WIDTH_4       2
#define MMCSD_BUS_WIDTH_8       3

    uint8 self_adaption_flag     : 2,
          delay_flag             : 1,
          dat_overflow_stop_flag : 1,
          delay_chain_cnt        : 4;

#define MMCSD_SMP_SUCC      2
#define MMCSD_SMP_EN        1
#define MMCSD_SMP_DIS       0

    uint8 cmd_crc_sample;
    uint8 dat_crc_sample;
    uint8 crc_sample_max;

    TYPE_LL_SDHC_SMP_CFG       smp_type;
    TYPE_LL_SDHC_IOCTRL        ioctl_type;
    TYPE_LL_SDHC_DELAY_SYSCLK  delay_type;
};

struct rt_mmcsd_cmd {
	uint32  cmd_code;
	uint32  arg;
	uint32  resp[4];
	uint32  flags;
#define MMC_RSP_PRESENT	(1 << 0)
#define MMC_RSP_136	(1 << 1)		/* 136 bit response */
#define MMC_RSP_CRC	(1 << 2)		/* expect valid crc */
#define MMC_RSP_BUSY	(1 << 3)		/* card may send busy */
#define MMC_RSP_OPCODE	(1 << 4)		/* response contains opcode */

#define MMC_CMD_MASK	(3 << 5)		/* non-SPI command type */
#define MMC_CMD_AC	(0 << 5)
#define MMC_CMD_ADTC	(1 << 5)
#define MMC_CMD_BC	(2 << 5)
#define MMC_CMD_BCR	(3 << 5)

#define MMC_RSP_SPI_S1	(1 << 7)		/* one status byte */
#define MMC_RSP_SPI_S2	(1 << 8)		/* second byte */
#define MMC_RSP_SPI_B4	(1 << 9)		/* four data bytes */
#define MMC_RSP_SPI_BUSY (1 << 10)		/* card may send busy */

/*
 * These are the native response types, and correspond to valid bit
 * patterns of the above flags.  One additional valid pattern
 * is all zeros, which means we don't expect a response.
 */
#define MMC_RSP_NONE	(0)
#define MMC_RSP_R1	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R1B	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE|MMC_RSP_BUSY)
#define MMC_RSP_R2	(MMC_RSP_PRESENT|MMC_RSP_136/*|MMC_RSP_CRC*/)
#define MMC_RSP_R3	(MMC_RSP_PRESENT)
#define MMC_RSP_R4	(MMC_RSP_PRESENT)
#define MMC_RSP_R5	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R6	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R7	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)

/* Can be used by core to poll after switch to MMC HS mode */
#define MMC_RSP_R1_NO_CRC	(MMC_RSP_PRESENT|MMC_RSP_OPCODE)

#define mmc_resp_type(cmd)	((cmd)->flags & (MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC|MMC_RSP_BUSY|MMC_RSP_OPCODE))

/*
 * These are the SPI response types for MMC, SD, and SDIO cards.
 * Commands return R1, with maybe more info.  Zero is an error type;
 * callers must always provide the appropriate MMC_RSP_SPI_Rx flags.
 */
#define MMC_RSP_SPI_R1	(MMC_RSP_SPI_S1)
#define MMC_RSP_SPI_R1B	(MMC_RSP_SPI_S1|MMC_RSP_SPI_BUSY)
#define MMC_RSP_SPI_R2	(MMC_RSP_SPI_S1|MMC_RSP_SPI_S2)
#define MMC_RSP_SPI_R3	(MMC_RSP_SPI_S1|MMC_RSP_SPI_B4)
#define MMC_RSP_SPI_R4	(MMC_RSP_SPI_S1|MMC_RSP_SPI_B4)
#define MMC_RSP_SPI_R5	(MMC_RSP_SPI_S1|MMC_RSP_SPI_S2)
#define MMC_RSP_SPI_R7	(MMC_RSP_SPI_S1|MMC_RSP_SPI_B4)

#define mmc_spi_resp_type(cmd)	((cmd)->flags & \
		(MMC_RSP_SPI_S1|MMC_RSP_SPI_BUSY|MMC_RSP_SPI_S2|MMC_RSP_SPI_B4))

/*rsponse types 
 *bits:0~4
 */
#define RESP_MASK	(0x1F)
#define RESP_NONE	(MMC_RSP_NONE)
#define RESP_R1		(MMC_RSP_R1)
#define RESP_R1B	(MMC_RSP_R1B)
#define RESP_R2		(MMC_RSP_R2)
#define RESP_R3		(MMC_RSP_R3)
#define RESP_R4		(MMC_RSP_R4)
#define RESP_R6		(MMC_RSP_R6)
#define RESP_R7		(MMC_RSP_R7)
#define RESP_R5		(MMC_RSP_R5)	/*SDIO command response type*/
/*command types 
 *bits:5~6
 */
#define CMD_MASK	(3 << 5)		/* command type */
#define CMD_AC		(0 << 5)
#define CMD_ADTC	(1 << 5)
#define CMD_BC		(2 << 5)
#define CMD_BCR		(3 << 5)

#define resp_type(cmd)	((cmd)->flags & RESP_MASK)

/*spi rsponse types 
 *bits:7~10
 */
#define RESP_SPI_MASK	(0xF << 7)
#define RESP_SPI_R1	(MMC_RSP_SPI_R1)
#define RESP_SPI_R1B	(MMC_RSP_SPI_R1B)
#define RESP_SPI_R2	(MMC_RSP_SPI_R2)
#define RESP_SPI_R3	(MMC_RSP_SPI_R3)
#define RESP_SPI_R4	(MMC_RSP_SPI_R4)
#define RESP_SPI_R5	(MMC_RSP_SPI_R5)
#define RESP_SPI_R7	(MMC_RSP_SPI_R7)

#define spi_resp_type(cmd)	((cmd)->flags & RESP_SPI_MASK)
/*
 * These are the command types.
 */
#define cmd_type(cmd)	((cmd)->flags & CMD_MASK)
	
	int32_t  err;
};


typedef enum
{
    LL_SDHC_IROF,  //Input Rising Ouput Falling for sd ms
    LL_SDHC_IROR,  //Input Rising Ouput Rising for sd Hi-speed
    //E_IFOF,  //Input Falling Ouput Falling for ms Hi-speed
    //E_IFOR
} TYPE_LL_SDHC_IOE;

typedef enum
{
    LL_SDHC_DAT_1BIT,
    LL_SDHC_DAT_4BIT,
} TYPE_LL_SDHC_DAT_WIDTH;

typedef enum
{
	SD_IDLE,
    SD_M_W,  //Input Rising Ouput Falling for sd ms
    SD_M_R,  //Input Rising Ouput Rising for sd Hi-speed
    //E_IFOF,  //Input Falling Ouput Falling for ms Hi-speed
    //E_IFOR
    SD_OFF,
} SDHOST_OPT;


typedef struct __ll_sdhc_cfg {
    uint32                     bus_clk;
    TYPE_LL_SDHC_IOE        ioe;
    TYPE_LL_SDHC_DAT_WIDTH  width;
} TYPE_LL_SDHC_CFG;

struct rt_mmcsd_data {
	uint32  blksize;
	uint32  blks;
	int32  err;
	uint32  flags;
#define DATA_DIR_WRITE	(1 << 0)
#define DATA_DIR_READ	(1 << 1)
#define DATA_STREAM	(1 << 2)

	uint32  timeout_ns;
	uint32  timeout_clks;
	uint8  *buf;
};

struct rt_sd_scr {
	uint8		sd_version;
	uint8		sd_bus_widths;
};


struct sdh_device {
    struct dev_obj dev;
	uint32  freq_min;
    uint32  freq_max;
	uint32  valid_ocr;	/* current valid OCR */
	struct rt_mmcsd_io_cfg io_cfg;
	struct rt_mmcsd_data data;
	struct rt_sd_scr	scr;
#define VDD_165_195     (1 << 7)    /* VDD voltage 1.65 - 1.95 */
#define VDD_20_21       (1 << 8)    /* VDD voltage 2.0 ~ 2.1 */
#define VDD_21_22       (1 << 9)    /* VDD voltage 2.1 ~ 2.2 */
#define VDD_22_23       (1 << 10)   /* VDD voltage 2.2 ~ 2.3 */
#define VDD_23_24       (1 << 11)   /* VDD voltage 2.3 ~ 2.4 */
#define VDD_24_25       (1 << 12)   /* VDD voltage 2.4 ~ 2.5 */
#define VDD_25_26       (1 << 13)   /* VDD voltage 2.5 ~ 2.6 */
#define VDD_26_27       (1 << 14)   /* VDD voltage 2.6 ~ 2.7 */
#define VDD_27_28       (1 << 15)   /* VDD voltage 2.7 ~ 2.8 */
#define VDD_28_29       (1 << 16)   /* VDD voltage 2.8 ~ 2.9 */
#define VDD_29_30       (1 << 17)   /* VDD voltage 2.9 ~ 3.0 */
#define VDD_30_31       (1 << 18)   /* VDD voltage 3.0 ~ 3.1 */
#define VDD_31_32       (1 << 19)   /* VDD voltage 3.1 ~ 3.2 */
#define VDD_32_33       (1 << 20)   /* VDD voltage 3.2 ~ 3.3 */
#define VDD_33_34       (1 << 21)   /* VDD voltage 3.3 ~ 3.4 */
#define VDD_34_35       (1 << 22)   /* VDD voltage 3.4 ~ 3.5 */
#define VDD_35_36       (1 << 23)   /* VDD voltage 3.5 ~ 3.6 */
	uint32  flags; /* define device capabilities */
#define MMCSD_BUSWIDTH_4    (1 << 0)
#define MMCSD_BUSWIDTH_8    (1 << 1)
#define MMCSD_MUTBLKWRITE   (1 << 2)
#define MMCSD_HOST_IS_SPI   (1 << 3)
#define controller_is_spi(host) (host->flags & MMCSD_HOST_IS_SPI)
#define MMCSD_SUP_SDIO_IRQ  (1 << 4)    /* support signal pending SDIO IRQs */
#define MMCSD_SUP_HIGHSPEED (1 << 5)    /* support high speed */
    uint32    max_seg_size;   /* maximum size of one dma segment */
    uint32    max_dma_segs;   /* maximum number of dma segments in one request */
    uint32    max_blk_size;   /* maximum block size */
    uint32    max_blk_count;  /* maximum block count */
	uint32    new_lba;
	struct rt_mmcsd_csd	csd;
	uint32	rca;		/* card addr */
	uint32	resp_cid[4];	/* card CID register */
	uint32	resp_csd[4];	/* card CSD register */
	uint32	resp_scr[2];	/* card SCR register */	
	uint16	tacc_clks;	/* data access time by ns */
	uint32	tacc_ns;	/* data access time by clk cycles */
	uint32	max_data_rate;	/* max data transfer rate */
	uint32	card_capacity;	/* card capacity, unit:KB */
	uint32	card_blksize;	/* card block size */
    uint32  card_max_blk_num; /*card block max number*/
	uint32	erase_size;	/* erase size in sectors */
	uint32  cardflags;
#define CARD_FLAG_HIGHSPEED  (1 << 0)   /* SDIO bus speed 50MHz */
#define CARD_FLAG_SDHC       (1 << 1)   /* SDHC card */
#define CARD_FLAG_SDXC       (1 << 2)   /* SDXC card */

	uint16	card_type;	
	uint8   sd_8clk_open : 1, sd_8clk_default : 1, sd_read_retry_flag : 1, sd_write_retry_flag : 1, sd_cmd_retry_flag : 1, sd_stop : 1, single_support : 1, reserved : 1;
	uint8   sd_opt;
    uint16  spi_mode_baud;
    uint32  init_clk;
    uint8   sd_clk_io;
    uint8   sd_mode_type;
    uint8   sd_cmd_sample;
    uint8   sd_write_sample;
    uint8   sd_read_sample;
    uint8   spi_delay_cycle;
    uint8   write_crc_pending;
    uint8   sd_read_dly_chain;
    uint8   sd_write_dly_chain;
    uint8   *sd_read_sample_value;
    uint8   *sd_write_sample_value;
    uint8   *sd_cmd_sample_value;
    uint8   sd_write_retry;
    uint8   sd_read_retry;
    uint8   sd_write_sample_num;
    uint8   sd_read_sample_num ;
    uint8   sd_cmd_sample_num  ;
    uint32  write_last_time_retry;
    uint32  read_last_time_retry;

    uint16  clk_io_type;
	void    *private_data;


	uint32 *cfg_backup;
    int32  (*open)(struct sdh_device *sdhost,uint8 bus_w, uint8 mode_type);
    int32  (*close)(struct sdh_device *sdhost);
	int32  (*suspend)(struct sdh_device *sdhost);
	int32  (*resume)(struct sdh_device *sdhost);
	int32  (*iocfg)(struct sdh_device *sdhost,struct rt_mmcsd_io_cfg *io_cfg);
	int32  (*cmd)(struct sdh_device *sdhost,struct rt_mmcsd_cmd *cmd);
	int32  (*write)(struct sdh_device *sdhost,uint8* buf);
	int32  (*read)(struct sdh_device *sdhost,uint8* buf);
    int32  (*complete)(struct sdh_device *sdhost);
	struct os_task state_task;
	struct os_semaphore dat_sema;
	struct os_mutex lock;
#ifdef CONFIG_SLEEP
	struct os_mutex bp_suspend_lock;
	struct os_mutex bp_resume_lock;
#endif
};

struct hgsdh {
    struct sdh_device       dev;
    //struct hgsdh_hw         *hw;
	uint32              hw;
    uint32                  irq_data;
    uint32                  irq_num;
    uint32                  opened : 1,
                            dsleep : 1;
};


#define     LL_SDHC_DAT_WIDTH_1BIT          (0x0UL << 2)
#define     LL_SDHC_DAT_WIDTH_4BIT          (0x1UL << 2)
#define     LL_SDHC_CLK_OUT_EN              (0x1UL << 1)
#define     LL_SDHC_EN                      (0x1UL << 0)

#define     LL_SDHC_8CLK_BEFORE_EN          (0x1UL << 31)
#define     LL_SDHC_DAT_CRC_ERR             (0x1UL << 22)
#define     LL_SDHC_RSP_TIMEOUT             (0x1UL << 21)
#define     LL_SDHC_D0_BUSY                 (0x1UL << 20)
#define     LL_SDHC_CRC_STA                 (0x7UL << 16)
#define     LL_SDHC_CMD_INTR_EN             (0x1UL << 15)
#define     LL_SDHC_DAT_INTR_EN             (0x1UL << 14)
#define     LL_SDHC_CMD_DONE                (0x1UL << 13)
#define     LL_SDHC_DAT_DONE                (0x1UL << 12)
#define     LL_SDHC_CMD_DONE_CLR            (0x1UL << 11)
#define     LL_SDHC_DAT_DONE_CLR            (0x1UL << 10)
#define     LL_SDHC_8CLK_AFTER_EN           (0x1UL << 9)
#define     LL_SDHC_ODAT_FAILING_EDGE       (0x0UL << 8)
#define     LL_SDHC_ODAT_RISING_EDGE        (0x1UL << 8)

#define     LL_SDHC_RSP_CRC_ERR             (0x1UL << 23)

#define     LL_SDHC_CMD_RESP_NONE           (0x2UL << 5)    /* no resp, busy wait */
#define     LL_SDHC_CMD_RESP_R1             (0x1UL << 5)    /* 6byte resp */
#define     LL_SDHC_CMD_RESP_R1B            (0x5UL << 5)    /* 6byte resp, busy wait */
#define     LL_SDHC_CMD_RESP_R2             (0x3UL << 5)    /* 17byte resp */
#define     LL_SDHC_CMD_RESP_R3             (0x1UL << 5)    /* 6byte resp */
#define     LL_SDHC_CMD_RESP_R4             (0x1UL << 5)    /* 6byte resp */
#define     LL_SDHC_CMD_RESP_R5             (0x1UL << 5)    /* 6byte resp */
#define     LL_SDHC_CMD_RESP_R6             (0x1UL << 5)    /* 6byte resp */
#define     LL_SDHC_CMD_RESP_R7             (0x1UL << 5)    /* 6byte resp */
#define     LL_SDHC_CMD_RESP_MSK            (0x7UL << 5)

#define     LL_SDHC_DAT_OVERFLOW_STOP_EN    (0X1UL << 10)   
#define     LL_SDHC_DAT_SMP_EN              (0X1UL << 9)
#define     LL_SDHC_CMD_SMP_EN              (0X1UL << 8)
#define     LL_SDHC_DELAY_WHOLE_SYSCLK      (0X1UL << 7)
#define     LL_SDHC_DELAY_HALF_SYSCLK       (0X1UL << 6)
#define     LL_SDHC_DELAY_CHAIN_EN          (0X1UL << 5)
#define     LL_SDHC_DAT_SMP_SET(n)          ((n) << 16)
#define     LL_SDHC_CMD_SMP_SET(n)          ((n))

   /* class 1 */
#define GO_IDLE_STATE         0   /* bc                          */
#define SEND_OP_COND          1   /* bcr  [31:0] OCR         R3  */
#define ALL_SEND_CID          2   /* bcr                     R2  */
#define SET_RELATIVE_ADDR     3   /* ac   [31:16] RCA        R1  */
#define SET_DSR               4   /* bc   [31:16] RCA            */
#define SWITCH                6   /* ac   [31:0] See below   R1b */
#define SELECT_CARD           7   /* ac   [31:16] RCA        R1  */
#define SEND_EXT_CSD          8   /* adtc                    R1  */
#define SEND_CSD              9   /* ac   [31:16] RCA        R2  */
#define SEND_CID             10   /* ac   [31:16] RCA        R2  */
#define READ_DAT_UNTIL_STOP  11   /* adtc [31:0] dadr        R1  */
#define STOP_TRANSMISSION    12   /* ac                      R1b */
#define SEND_STATUS          13   /* ac   [31:16] RCA        R1  */
#define GO_INACTIVE_STATE    15   /* ac   [31:16] RCA            */
#define SPI_READ_OCR         58   /* spi                  spi_R3 */
#define SPI_CRC_ON_OFF       59   /* spi  [0:0] flag      spi_R1 */

  /* class 2 */
#define SET_BLOCKLEN         16   /* ac   [31:0] block len   R1  */
#define READ_SINGLE_BLOCK    17   /* adtc [31:0] data addr   R1  */
#define READ_MULTIPLE_BLOCK  18   /* adtc [31:0] data addr   R1  */

  /* class 3 */
#define WRITE_DAT_UNTIL_STOP 20   /* adtc [31:0] data addr   R1  */

  /* class 4 */
#define SET_BLOCK_COUNT      23   /* adtc [31:0] data addr   R1  */
#define WRITE_BLOCK          24   /* adtc [31:0] data addr   R1  */
#define WRITE_MULTIPLE_BLOCK 25   /* adtc                    R1  */
#define PROGRAM_CID          26   /* adtc                    R1  */
#define PROGRAM_CSD          27   /* adtc                    R1  */

  /* class 6 */
#define SET_WRITE_PROT       28   /* ac   [31:0] data addr   R1b */
#define CLR_WRITE_PROT       29   /* ac   [31:0] data addr   R1b */
#define SEND_WRITE_PROT      30   /* adtc [31:0] wpdata addr R1  */

  /* class 5 */
#define ERASE_GROUP_START    35   /* ac   [31:0] data addr   R1  */
#define ERASE_GROUP_END      36   /* ac   [31:0] data addr   R1  */
#define ERASE                38   /* ac                      R1b */

  /* class 9 */
#define FAST_IO              39   /* ac   <Complex>          R4  */
#define GO_IRQ_STATE         40   /* bcr                     R5  */

  /* class 7 */
#define LOCK_UNLOCK          42   /* adtc                    R1b */

  /* class 8 */
#define APP_CMD              55   /* ac   [31:16] RCA        R1  */
#define GEN_CMD              56   /* adtc [0] RD/WR          R1  */


#define MMC_QUE_TASK_PARAMS      44   /* ac   [20:16] task id    R1  */
#define MMC_QUE_TASK_ADDR        45   /* ac   [31:0] data addr   R1  */
#define MMC_EXECUTE_READ_TASK    46   /* adtc [20:16] task id    R1  */
#define MMC_EXECUTE_WRITE_TASK   47   /* adtc [20:16] task id    R1  */
#define MMC_CMDQ_TASK_MGMT       48   /* ac   [20:16] task id    R1b */
/* SD commands                           type  argument     response */
  /* class 0 */
/* This is basically the same command as for MMC with some quirks. */
#define SD_SEND_RELATIVE_ADDR     3   /* bcr                     R6  */
#define SD_SEND_IF_COND           8   /* bcr  [11:0] See below   R7  */

  /* class 10 */
#define SD_SWITCH                 6   /* adtc [31:0] See below   R1  */

  /* Application commands */
#define SD_APP_SET_BUS_WIDTH      6   /* ac   [1:0] bus width    R1  */
#define SD_APP_SEND_NUM_WR_BLKS  22   /* adtc                    R1  */
#define SD_APP_OP_COND           41   /* bcr  [31:0] OCR         R3  */
#define SD_APP_SEND_SCR          51   /* adtc                    R1  */

#define SCR_SPEC_VER_0		0	/* Implements system specification 1.0 - 1.01 */
#define SCR_SPEC_VER_1		1	/* Implements system specification 1.10 */
#define SCR_SPEC_VER_2		2	/* Implements system specification 2.00 */


/* SDIO commands                                          type  argument     response */
#define SD_IO_SEND_OP_COND          5 /* bcr  [23:0] OCR         R4  */
#define SD_IO_RW_DIRECT            52 /* ac   [31:0] See below   R5  */
#define SD_IO_RW_EXTENDED          53 /* adtc [31:0] See below   R5  */


/* CMD52 arguments */
#define SDIO_ARG_CMD52_READ             (0<<31)
#define SDIO_ARG_CMD52_WRITE            (1u<<31)
#define SDIO_ARG_CMD52_FUNC_SHIFT       28
#define SDIO_ARG_CMD52_FUNC_MASK        0x7
#define SDIO_ARG_CMD52_RAW_FLAG         (1u<<27)
#define SDIO_ARG_CMD52_REG_SHIFT        9
#define SDIO_ARG_CMD52_REG_MASK         0x1ffff
#define SDIO_ARG_CMD52_DATA_SHIFT       0
#define SDIO_ARG_CMD52_DATA_MASK        0xff
#define SDIO_R5_DATA(resp)            ((resp)[0] & 0xff)

/* CMD53 arguments */
#define SDIO_ARG_CMD53_READ             (0<<31)
#define SDIO_ARG_CMD53_WRITE            (1u<<31)
#define SDIO_ARG_CMD53_FUNC_SHIFT       28
#define SDIO_ARG_CMD53_FUNC_MASK        0x7
#define SDIO_ARG_CMD53_BLOCK_MODE       (1u<<27)
#define SDIO_ARG_CMD53_INCREMENT        (1u<<26)
#define SDIO_ARG_CMD53_REG_SHIFT        9
#define SDIO_ARG_CMD53_REG_MASK         0x1ffff
#define SDIO_ARG_CMD53_LENGTH_SHIFT     0
#define SDIO_ARG_CMD53_LENGTH_MASK      0x1ff
#define SDIO_ARG_CMD53_LENGTH_MAX       511

#define CARD_TYPE_MMC                   0 /* MMC card */
#define CARD_TYPE_SD                    1 /* SD card */
#define CARD_TYPE_SDIO                  2 /* SDIO card */
#define CARD_TYPE_SDIO_COMBO            3 /* SD combo (IO+mem) card */




#define     LL_SDHC_DAT_RCV             (0x1UL << 3)
#define     LL_SDHC_DAT_SEND            (0x2UL << 3)
#define     LL_SDHC_DAT_SEND_WAIT_BUSY  (0x3UL << 3)
#define     LL_SDHC_DAT_RW_MSK          (0x3UL << 3)

#define SD_SCR_BUS_WIDTH_1	(1 << 0)
#define SD_SCR_BUS_WIDTH_4	(1 << 2)


#define CARD_BUSY	0x80000000	/* Card Power up status bit */

#define SECTOR_SIZE   512
#define CMD_ISR_EN    0
#define DAT_ISR_EN    1



void hgsdh_attach(uint32 dev_id, struct hgsdh *sdhost);
int sd_multiple_write(struct sdh_device * host,uint32 lba,uint32 len,uint8* buf);
int sd_multiple_read(struct sdh_device * host,uint32 lba,uint32 len,uint8* buf);
uint32 sdhost_init(uint32 clk, uint8 single_support);
uint32 sdhost_suspend(struct sdh_device * host);
uint32 sdhost_resume(struct sdh_device * host);
void stop_card();
void sd_open();
uint32 sdhost_deinit_for_sleep();

int usb_sd_scsi_read(uint32 lba, uint32 count, uint8* buf);
int usb_sd_scsi_write(uint32 lba, uint32 count, uint8* buf);
 


#endif
