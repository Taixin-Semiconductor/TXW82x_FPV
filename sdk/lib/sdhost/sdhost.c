#include "sys_config.h"
#include "typesdef.h"
#include "devid.h"
#include "list.h"
#include "dev.h"
#include "osal/task.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "lib/sdhost/sdhost.h"
#include "hal/gpio.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "osal/task.h"
#include "osal/sleep.h"
#include "osal/timer.h"
#include "osal/work.h"
#ifdef PIN_FROM_PARAM
#include "pin_param.h"
#endif


extern uint8_t get_fat_isready();
struct sdh_device *sdh_test;
struct os_semaphore sem;

#define SD_SAMPLE_VALUE_PRINT       (1)

#ifdef PSRAM_HEAP
#define SDHC_HEAP_MALLOC  os_malloc_psram
#define SDHC_HEAP_FREE    os_free_psram
#else
#define SDHC_HEAP_MALLOC  os_malloc
#define SDHC_HEAP_FREE    os_free
#endif

volatile uint32 sdhc_curr_write_byte = 0;
volatile uint32 sdhc_curr_read_byte  = 0;
volatile uint32 sdhc_last_write_byte = 0;
volatile uint32 sdhc_last_read_byte  = 0;

#define be32_to_cpu(x) ((uint32)(              \
    (((uint32)(x) & (uint32)0x000000ffUL) << 24) |        \
    (((uint32)(x) & (uint32)0x0000ff00UL) <<  8) |        \
    (((uint32)(x) & (uint32)0x00ff0000UL) >>  8) |        \
    (((uint32)(x) & (uint32)0xff000000UL) >> 24)))

static uint32 __rt_fls(uint32 val)
{
    uint32  bit = 32;

    if (!val)
        return 0;
    if (!(val & 0xffff0000u)) 
    {
        val <<= 16;
        bit -= 16;
    }
    if (!(val & 0xff000000u)) 
    {
        val <<= 8;
        bit -= 8;
    }
    if (!(val & 0xf0000000u)) 
    {
        val <<= 4;
        bit -= 4;
    }
    if (!(val & 0xc0000000u)) 
    {
        val <<= 2;
        bit -= 2;
    }
    if (!(val & 0x80000000u)) 
    {
        bit -= 1;
    }

    return bit;
}




static const uint32 tran_unit[] =
{
    10000, 100000, 1000000, 10000000,
    0,     0,      0,       0
};

static const uint8 tran_value[] =
{
    0,  10, 12, 13, 15, 20, 25, 30,
    35, 40, 45, 50, 55, 60, 70, 80,
};

static const uint32 tacc_uint[] =
{
    1, 10, 100, 1000, 10000, 100000, 1000000, 10000000,
};

static const uint8 tacc_value[] =
{
    0,  10, 12, 13, 15, 20, 25, 30,
    35, 40, 45, 50, 55, 60, 70, 80,
};

uint32 GET_BITS(uint32 *resp,
                               uint32  start,
                               uint32  size)
{                               
        const int32_t __size = size;
        const uint32 __mask = (__size < 32 ? 1 << __size : 0) - 1; 
        const int32_t __off = 3 - ((start) / 32);
        const int32_t __shft = (start) & 31;
        uint32 __res;

        __res = resp[__off] >> __shft;
        if (__size + __shft > 32)
            __res |= resp[__off-1] << ((32 - __shft) % 32);

        return __res & __mask;
}


void sdhost_io_func_init(uint32 req){
    if(req == 1)
        pin_func(HG_SDIOHOST_DEVID,4);
    else
        pin_func(HG_SDIOHOST_DEVID,1);
}

static int32_t sd_parse_scr(struct sdh_device *host)
{
    struct rt_sd_scr *scr = &host->scr;
    uint32 resp[4];

    resp[3] = host->resp_scr[1];
    resp[2] = host->resp_scr[0];
    scr->sd_version = GET_BITS(resp, 56, 4);
    scr->sd_bus_widths = GET_BITS(resp, 48, 4);

    SDHC_WARN_PRINTF("sd_version : %d \t %d\r\n", scr->sd_version, scr->sd_bus_widths);
    return 0;
}
#if (defined (TXW81X) || defined (TXW82X))
static int32_t sd_switch(struct sdh_device *host)
{
    int32_t ret;
    struct rt_mmcsd_cmd cmd;
    uint8 *buf = os_malloc(64);
    if (!buf)
    {
        SDHC_ERR_PRINTF("mallo err!\r\n");
        return 1;
    }

    host->data.blksize = 64;
    host->data.blks    = 1;
    host->data.err     = 0;
    if (host->read)
    {
        ret = host->read(host, buf);
        if (ret)
            return 1;
    }
    
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    cmd.cmd_code = SD_SWITCH;
    cmd.arg = 0x00FFFFF1;
    cmd.flags = RESP_R1 | CMD_ADTC;
    if (host->cmd)
    {
        ret = host->cmd(host, &cmd);
        if (ret)
            return 1;
    }

    if (host->complete)
    {
        ret = host->complete(host);
        if (ret)
            return 1;
    }
        
    if (buf[13] & 0x02)
        host->max_data_rate = 50*1000*1000;

	#if 0
	for(int itk = 0;itk <64;itk++){	
		if(itk%32 == 0)
			SDHC_WARN_PRINTF("\r\n");

		SDHC_WARN_PRINTF("%02x ",buf[itk]);
	}
	#endif

    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    host->data.blksize = 64;
    host->data.blks    = 1;
    host->data.err     = 0;
    if (host->read)
    {
        ret = host->read(host, buf);
        if (ret)
            return 1;
    }
    cmd.cmd_code = SD_SWITCH;
    cmd.arg = 0x80FFFFF1;
    cmd.flags = RESP_R1 | CMD_ADTC;
    if (host->cmd)
    {
        ret = host->cmd(host, &cmd);
        if (ret)
            return 1;
    }

    if (host->complete)
    {
        ret = host->complete(host);
        if (ret)
            return 1;
    }

	#if 0
	for(int itk = 0;itk <64;itk++){	
		if(itk%32 == 0)
			SDHC_WARN_PRINTF("\r\n");
		
		SDHC_WARN_PRINTF("%02x ",buf[itk]);
	}
	SDHC_WARN_PRINTF("\r\n");
	#endif

    if ((buf[16] & 0xF) != 1) 
    {
        SDHC_ERR_PRINTF("switching card to high speed failed!");
        return 1;
    }
	SDHC_WARN_PRINTF("switch finish\r\n");
    host->cardflags |= CARD_FLAG_HIGHSPEED;
    os_free(buf);
    return 0;
}
#endif

unsigned int sd_dwCap;
static int32_t sd_parse_csd(struct sdh_device *host)
{
    struct rt_mmcsd_csd *csd = &host->csd;
    uint32 *resp = host->resp_csd;

    csd->csd_structure = GET_BITS(resp, 126, 2);

    switch (csd->csd_structure)
    {
    case 0:
		host->cardflags &= ~CARD_FLAG_SDHC;
        csd->taac = GET_BITS(resp, 112, 8);
        csd->nsac = GET_BITS(resp, 104, 8);
        csd->tran_speed = GET_BITS(resp, 96, 8);
        csd->card_cmd_class = GET_BITS(resp, 84, 12);
        csd->rd_blk_len = GET_BITS(resp, 80, 4);
        csd->rd_blk_part = GET_BITS(resp, 79, 1);
        csd->wr_blk_misalign = GET_BITS(resp, 78, 1);
        csd->rd_blk_misalign = GET_BITS(resp, 77, 1);
        csd->dsr_imp = GET_BITS(resp, 76, 1);
        csd->c_size = GET_BITS(resp, 62, 12);
        csd->c_size_mult = GET_BITS(resp, 47, 3);
        csd->r2w_factor = GET_BITS(resp, 26, 3);
        csd->wr_blk_len = GET_BITS(resp, 22, 4);
        csd->wr_blk_partial = GET_BITS(resp, 21, 1);
        csd->csd_crc = GET_BITS(resp, 1, 7);

        host->card_blksize = 1 << csd->rd_blk_len;
        host->card_capacity = (csd->c_size + 1) << (csd->c_size_mult + 2);
        host->card_capacity *= host->card_blksize;
        host->card_capacity >>= 10; /* unit:KB */
        host->tacc_clks = csd->nsac * 100;
        host->tacc_ns = (tacc_uint[csd->taac&0x07] * tacc_value[(csd->taac&0x78)>>3] + 9) / 10;
        host->max_data_rate = tran_unit[csd->tran_speed&0x07] * tran_value[(csd->tran_speed&0x78)>>3];

    #if 0
        val = GET_BITS(resp, 115, 4);
        unit = GET_BITS(resp, 112, 3);
        csd->tacc_ns     = (tacc_uint[unit] * tacc_value[val] + 9) / 10;
        csd->tacc_clks   = GET_BITS(resp, 104, 8) * 100;

        val = GET_BITS(resp, 99, 4);
        unit = GET_BITS(resp, 96, 3);
        csd->max_data_rate    = tran_unit[unit] * tran_value[val];
        csd->ccc      = GET_BITS(resp, 84, 12);

        unit = GET_BITS(resp, 47, 3);
        val = GET_BITS(resp, 62, 12);
        csd->device_size      = (1 + val) << (unit + 2);

        csd->read_bl_len = GET_BITS(resp, 80, 4);
        csd->write_bl_len = GET_BITS(resp, 22, 4);
        csd->r2w_factor = GET_BITS(resp, 26, 3);
    #endif
        break;
    case 1:
        host->cardflags |= CARD_FLAG_SDHC;

        /*This field is fixed to 0Eh, which indicates 1 ms. 
          The host should not use TAAC, NSAC, and R2W_FACTOR
          to calculate timeout and should uses fixed timeout
          values for read and write operations*/
        csd->taac = GET_BITS(resp, 112, 8);
        csd->nsac = GET_BITS(resp, 104, 8);
        csd->tran_speed = GET_BITS(resp, 96, 8);
        csd->card_cmd_class = GET_BITS(resp, 84, 12);
        csd->rd_blk_len = GET_BITS(resp, 80, 4);
        csd->rd_blk_part = GET_BITS(resp, 79, 1);
        csd->wr_blk_misalign = GET_BITS(resp, 78, 1);
        csd->rd_blk_misalign = GET_BITS(resp, 77, 1);
        csd->dsr_imp = GET_BITS(resp, 76, 1);
        csd->c_size = GET_BITS(resp, 48, 22);

        csd->r2w_factor = GET_BITS(resp, 26, 3);
        csd->wr_blk_len = GET_BITS(resp, 22, 4);
        csd->wr_blk_partial = GET_BITS(resp, 21, 1);
        csd->csd_crc = GET_BITS(resp, 1, 7);

        host->card_blksize = 512;
        host->card_capacity = (csd->c_size + 1) * 512;  /* unit:KB */
        host->tacc_clks = 0;
        host->tacc_ns = 0;
        host->max_data_rate = tran_unit[csd->tran_speed&0x07] * tran_value[(csd->tran_speed&0x78)>>3];

    #if 0
        csd->tacc_ns     = 0;
        csd->tacc_clks   = 0;

        val = GET_BITS(resp, 99, 4);
        unit = GET_BITS(resp, 96, 3);
        csd->max_data_rate    = tran_unit[unit] * tran_value[val];
        csd->ccc      = GET_BITS(resp, 84, 12);

        val = GET_BITS(resp, 48, 22);
        csd->device_size     = (1 + val) << 10;

        csd->read_bl_len = 9;
        csd->write_bl_len = 9;
        /* host should not use this factor and should use 250ms for write timeout */
        csd->r2w_factor = 2;
    #endif
        break;
    default:
        SDHC_ERR_PRINTF("unrecognised CSD structure version %d!", csd->csd_structure);

        return -EINVAL;
    }
    host->card_max_blk_num = host->card_capacity << 1;
    SDHC_WARN_PRINTF("SD card capacity %d KB.\r\n", host->card_capacity);
    SDHC_WARN_PRINTF("SD card max block num %d\r\n", host->card_max_blk_num);
    sd_dwCap = host->card_capacity;

    return 0;
}

uint32 select_voltage(struct sdh_device *host, uint32 ocr)
{
    int bit;
    //extern int ffs32_lsb(uint32 value);

    ocr &= host->valid_ocr;

    bit = 15;//ffs32_lsb(ocr);
    if (bit) 
    {
        bit -= 1;

        ocr &= 3 << bit;

        host->io_cfg.vdd = bit;
        //mmcsd_set_iocfg(host);
        if(host->iocfg)
            host->iocfg(host,&host->io_cfg);
    } 
    else 
    {
        SDHC_ERR_PRINTF("host doesn't support card's voltages!");
        ocr = 0;
    }

    return ocr;	
}

uint32 sd_power_up(struct sdh_device *host,uint8 bus_w)
{
    int bit = __rt_fls(host->valid_ocr) - 1;
    
     host->io_cfg.vdd = bit;
     if (controller_is_spi(host))
     {
         host->io_cfg.chip_select = MMCSD_CS_HIGH;
         host->io_cfg.bus_mode = MMCSD_BUSMODE_PUSHPULL;
     } 
     else
     {
         host->io_cfg.chip_select = MMCSD_CS_IGNORE;
         host->io_cfg.bus_mode = MMCSD_BUSMODE_OPENDRAIN;
     }
     host->io_cfg.power_mode = MMCSD_POWER_UP;
     if(bus_w == MMCSD_BUSWIDTH_4)
         host->io_cfg.bus_width = MMCSD_BUS_WIDTH_4;
     else
         host->io_cfg.bus_width = MMCSD_BUS_WIDTH_1;
     
     host->io_cfg.clock = 400000;
     
     if(host->iocfg)
          host->iocfg(host,&host->io_cfg);

    
     /*
      * This delay should be sufficient to allow the power supply
      * to reach the minimum voltage.
      */
     os_sleep_ms(10);
    
     host->io_cfg.clock = host->freq_min;
     host->io_cfg.power_mode = MMCSD_POWER_ON;
     if(host->iocfg)
          host->iocfg(host,&host->io_cfg);

    
     /*
      * This delay must be at least 74 clock sizes, or 1 ms, or the
      * time required to reach a stable voltage.
      */
     os_sleep_ms(10);
     return 0;
}

void sd_set_clk(struct sdh_device * host,uint32 clk)
{
    host->io_cfg.clock 		 = clk;
    host->io_cfg.ioctl_type = LL_SDHC_IOCTRL_SET_CLOCK;
    host->iocfg(host,&host->io_cfg);
}

void sd_set_bus_width(struct sdh_device * host,uint32 width)
{
    host->io_cfg.bus_width   = width;
    host->io_cfg.ioctl_type = LL_SDHC_IOCTRL_SET_BUS_WIDTH;
    host->iocfg(host,&host->io_cfg);
}

#if (defined (TXW81X) || defined (TXW82X))
void sd_set_sample(struct sdh_device *host, TYPE_LL_SDHC_SMP_CFG type, uint8 cmd_cmp, uint8 dat_cmp)
{
    host->io_cfg.smp_type       = type;
    host->io_cfg.cmd_crc_sample = cmd_cmp;
    host->io_cfg.dat_crc_sample = dat_cmp;
    host->io_cfg.ioctl_type 	= LL_SDHC_IOCTRL_SET_SMP;
    host->iocfg(host, &host->io_cfg);
}

void sd_delay_config(struct sdh_device *host, TYPE_LL_SDHC_DELAY_SYSCLK dly_cfg, uint8 chain)
{
    if (dly_cfg == LL_SDHC_DLY_NONE)
    {
        host->io_cfg.delay_flag      = 0;
    }else{
        host->io_cfg.delay_flag      = 1;
        host->io_cfg.delay_type      = ((chain == 0) && (dly_cfg == LL_SDHC_DLY_CHAIN)) ? (LL_SDHC_DLY_NONE) : (dly_cfg);
        host->io_cfg.delay_chain_cnt = chain;
    }
    host->io_cfg.ioctl_type 	 = LL_SDHC_IOCTRL_SET_DELAY_TYPE;
    host->iocfg(host, &host->io_cfg);
}

void sd_dat_of_stop_clk_cfg(struct sdh_device *host, uint8 flag)
{
    host->io_cfg.dat_overflow_stop_flag = flag;
    host->io_cfg.ioctl_type 		    = LL_SDHC_IOCTRL_SET_DAT_OF_STOP_CLK;
    host->iocfg(host, &host->io_cfg);
}
#endif

uint32 send_idle(struct sdh_device * host)
{
    uint32 ret;
    struct rt_mmcsd_cmd cmd;
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    cmd.cmd_code = GO_IDLE_STATE;
    cmd.arg = 0;
    cmd.flags = RESP_SPI_R1 | RESP_NONE | CMD_BC;

    if(host->cmd)
        ret = host->cmd(host,&cmd);
    else{
        SDHC_ERR_PRINTF("no cmd action register\r\n");
        return 0;
    }
    return ret;
}

uint32 send_all_get_cid(struct sdh_device * host,uint32 *cid)
{
    uint32 ret;
    struct rt_mmcsd_cmd cmd;

    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    cmd.cmd_code = ALL_SEND_CID;
    cmd.arg = 0;
    cmd.flags = RESP_R2 | CMD_BCR;
    ret = host->cmd(host,&cmd);

    if(ret==0)
        memcpy(cid, cmd.resp, sizeof(uint32) * 4);
    
    return ret;
}

uint32 send_get_card_addr(struct sdh_device * host,uint32 *rca)
{
    uint32 ret;
    struct rt_mmcsd_cmd cmd;
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    cmd.cmd_code = SD_SEND_RELATIVE_ADDR;
    cmd.arg = 0;
    cmd.flags = RESP_R6 | CMD_BCR;	
    ret = host->cmd(host,&cmd);

    *rca = cmd.resp[0] >> 16;
    return 0;
}

uint32 send_card_status(struct sdh_device * host){
    struct rt_mmcsd_cmd cmd;
    int ret = 0;
    uint32  status = 0;
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    cmd.cmd_code = SEND_STATUS;

    cmd.arg = host->rca << 16;
    cmd.flags = RESP_R1 | CMD_AC;


    if(host->cmd)
       ret = host->cmd(host,&cmd);

    status = (cmd.resp[0] >> 9) & 0xf;
    if (status != MMCSD_CARD_STATUS_TRAN)
    {
        // SDHC_WARN_PRINTF("card status : %d\r\n", status);
        return RET_ERR;
    }
    return ret;	
}

uint32 send_select_card(struct sdh_device * host)
{
    struct rt_mmcsd_cmd cmd;
    int ret = 0;
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    cmd.cmd_code = SELECT_CARD;

    if (host->rca) 
    {
        cmd.arg = host->rca << 16;
        cmd.flags = RESP_R1 | CMD_AC;
    } 
    else 
    {
        cmd.arg = 0;
        cmd.flags = RESP_NONE | CMD_AC;
    }
    
    if(host->cmd)
       ret = host->cmd(host,&cmd);

    return ret;
}

uint32 send_if_cond(struct sdh_device * host,uint32 ocr)
{
    struct rt_mmcsd_cmd cmd;
    int ret = 0;
    uint8 pattern;
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    cmd.cmd_code = SD_SEND_IF_COND;
    cmd.arg = ((ocr & 0xFF8000) != 0) << 8 | 0xAA;
    cmd.flags = RESP_SPI_R7 | RESP_R7 | CMD_BCR;

    if(host->cmd)
       ret = host->cmd(host,&cmd);

    //if (controller_is_spi(host))
    //   pattern = cmd.resp[1] & 0xFF;
    //else
    pattern = cmd.resp[0] & 0xFF;

    if (pattern != 0xAA)
       return -EINVAL;

    return ret;

}

uint32 send_get_csd(struct sdh_device * host,uint32 *csd)
{
    int ret;	
    struct rt_mmcsd_cmd cmd;
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    
    cmd.cmd_code = SEND_CSD;
    cmd.arg = host->rca << 16;
    cmd.flags = RESP_R2 | CMD_AC;
    ret = host->cmd(host,&cmd);
    
    memcpy(csd, cmd.resp, sizeof(uint32) * 4);
    return ret;
}


uint32 send_app_cmd(struct sdh_device *host,uint32 rca)
{
    struct rt_mmcsd_cmd cmd = {0};
    int ret = 0;

    cmd.cmd_code = APP_CMD;
    if(rca){
        cmd.arg = rca << 16;
        cmd.flags = RESP_R1 | CMD_AC;		
    }
    else
    {
        cmd.arg = 0;
        cmd.flags = RESP_R1 | CMD_BCR;		
    }
    if(host->cmd)
       ret = host->cmd(host,&cmd);
    return ret;
}

uint32 sd_app_set_bus_width(struct sdh_device *host,int32_t width)
{
    int ret = 0;
    struct rt_mmcsd_cmd cmd;

    send_app_cmd(host,host->rca);


    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    cmd.cmd_code = SD_APP_SET_BUS_WIDTH;
    cmd.flags = RESP_R1 | CMD_AC;

    switch (width) 
    {
    case MMCSD_BUS_WIDTH_1:
        cmd.arg = MMCSD_BUS_WIDTH_1;
        break;
    case MMCSD_BUS_WIDTH_4:
        cmd.arg = MMCSD_BUS_WIDTH_4;
        break;
    default:
        return -EINVAL;
    }	

    if(host->cmd)
       ret = host->cmd(host,&cmd);

    return ret;
}

uint32 send_get_scr(struct sdh_device *host,uint32* scr)
{
    struct rt_mmcsd_cmd cmd;
    int ret;

    if(host->flags & MMCSD_BUSWIDTH_4)
        sd_set_bus_width(host, MMCSD_BUS_WIDTH_1);	

    host->data.blksize = 8;
    host->data.blks    = 1;
    host->data.err     = 0;
    if(host->read)
        ret = host->read(host,(uint8*)scr);

    send_app_cmd(host,host->rca);
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    cmd.cmd_code = SD_APP_SEND_SCR;
    cmd.arg = 0;
    cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_ADTC;
    if(host->cmd)
       ret = host->cmd(host,&cmd);

    if (host->complete)
    {
        ret = host->complete(host);
        if (ret)
            return 0;
    }
    
    if(host->data.err != 0)
        return 0;

    scr[0] = be32_to_cpu(scr[0]);
    scr[1] = be32_to_cpu(scr[1]);

    SDHC_WARN_PRINTF("scr:%x %x\r\n",scr[0],scr[1]);
    return 1;
}


uint32 sd_tran_stop(struct sdh_device * host)
{
    int ret = 0;	
    struct rt_mmcsd_cmd  cmd;

    if (host->cmd == NULL)  return RET_ERR;
    if (!host->sd_stop)     return RET_OK;

    // os_printf("%s %d addr : %d\r\n", __func__, __LINE__, __builtin_return_address(0));
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    cmd.cmd_code = STOP_TRANSMISSION;
    cmd.arg = 0;
    cmd.flags = RESP_SPI_R1B | RESP_R1B | CMD_AC;

    for (int i = 0; i < 10; i++)
    {
        ret  = host->cmd(host, &cmd);
        if (!ret)
        {
            for (int j = 0; j < 10; j++)
            {
                ret = send_card_status(host);
                if (!ret) 
                {
                    host->sd_opt  = SD_IDLE;
                    host->sd_stop = 0;
                    goto _succ;
                }
            }
        }
    }
    ret = RET_ERR;
    host->sd_opt = SD_OFF;

_succ:
    return ret;
}

//给外部接口专门用的停止命令,现在暂时是给文件系统
uint32 fatfs_sd_tran_stop(struct sdh_device * host)
{
    int ret;
    os_mutex_lock(&host->lock,osWaitForever);
    ret = sd_tran_stop(host);
    os_mutex_unlock(&host->lock);
    return ret;
    
}

int sd_multiple_write(struct sdh_device * host,uint32 lba,uint32 len,uint8 *buf)
{
    int ret;
    int send_cmd = 1;
    uint32 curr_lba   = lba;
    uint32 backup_lba = host->new_lba;
    uint32 block_num  = len/SECTOR_SIZE;
    struct rt_mmcsd_cmd  cmd;	
    uint8  retry_cnt  = 0;
    uint8  curr_index = 0;
    uint8  sel_index  = 0;
    uint8  *kick_buf  = buf;
    os_mutex_lock(&host->lock,osWaitForever);
   
__retry:
    if(((curr_lba != host->new_lba)||(host->sd_opt != SD_M_W))&& host->sd_stop)
    {
        ret = sd_tran_stop(host);	
        if (ret)    goto __err;
        host->new_lba = curr_lba;
        send_cmd = 1;
    } else if ((host->sd_opt == SD_IDLE) || (!host->sd_stop)){
        send_cmd = 1;
        if (curr_lba != host->new_lba)
        {
            host->new_lba = curr_lba;
        }
    } else {
        send_cmd = 0;
    }

    if ((host->new_lba + block_num) > host->card_max_blk_num)
    {
        SDHC_ERR_PRINTF("%s operation lba %d size %d max : %d err\r\n", __func__, host->new_lba, block_num, host->card_max_blk_num);
        host->new_lba = backup_lba;
        ret = RET_ERR;
        goto __err;
    }

    if (send_cmd)  host->sd_stop = (host->single_support) ? (block_num > 1) : (1);//;
    host->new_lba = host->new_lba + block_num;
    host->sd_opt  = SD_M_W;
    ///////////////////////////////////////////////////////
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    cmd.cmd_code = (host->single_support) ? ((block_num > 1) ? (WRITE_MULTIPLE_BLOCK) : (WRITE_BLOCK)) : (WRITE_MULTIPLE_BLOCK);

    cmd.arg = curr_lba;
    if (!(host->cardflags & CARD_FLAG_SDHC)) 
    {
        cmd.arg <<= 9;
    }	
    cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_ADTC;
    if((host->cmd) && send_cmd){	
        ret = host->cmd(host,&cmd);  
        if(ret){
			sd_tran_stop(host);
            ret = -1;
            goto __err;
        }
    }

#if defined(TXW82X) || defined(TXW81X)
    if (host->io_cfg.self_adaption_flag != MMCSD_SMP_DIS)
    {
        sd_delay_config(host, LL_SDHC_DLY_CHAIN, host->sd_write_dly_chain);
        sd_set_sample(host, LL_SDHC_DAT_SMP_CFG_EN, 0, host->sd_write_sample);
    }
#endif
    
    ///////////////////////////////////////////////////////
    host->data.blksize = SECTOR_SIZE;
    host->data.blks    = block_num;
    host->data.err     = 0;	
    if((!retry_cnt) && ((uint32)kick_buf >= PSRAM_BASE) ){
		sys_dcache_clean_range_unaligned((void *)kick_buf, len); 
	}

    if(host->write)
        ret = host->write(host,kick_buf);

    if (host->complete)
    {
        ret = host->complete(host);
#if defined(TXW82X)
        if ((host->sd_write_retry_flag == 0) && (ret == MMCSD_CMP_DATERR) && (host->io_cfg.self_adaption_flag != MMCSD_SMP_EN))
        {
            if (host->write_last_time_retry && ((os_jiffies() - host->write_last_time_retry ) < 1000))
            {
                host->sd_write_retry         = LL_SDHC_RETRY_SELECT_POINT;
                host->write_last_time_retry  = 0;
            }

            switch (host->sd_write_retry)
            {
                case LL_SDHC_RETRY_DEFAULT:
                    host->sd_write_retry = LL_SDHC_RETRY_SELECT_POINT;
                    curr_lba  = host->new_lba - host->data.blks;
                    block_num = host->data.blks;
                    kick_buf  = kick_buf + (block_num - host->data.blks) * SECTOR_SIZE;
                    if (host->write_last_time_retry  == 0)      host->write_last_time_retry  = os_jiffies();
                    break;

                case LL_SDHC_RETRY_SELECT_POINT:
                    for (curr_index = 0; curr_index < host->sd_write_sample_num; curr_index++)
                    {
                        if ((host->sd_write_sample_value[curr_index] != 0xff) && (host->sd_write_sample_value[curr_index] == host->sd_write_sample))
                        {
                            sel_index = (curr_index + host->sd_write_sample_num - 1) % host->sd_write_sample_num;
                            host->sd_write_sample_value[curr_index] = 0xff;
                            break;
                        }
                    }
                    if ((curr_index < host->sd_write_sample_num) && ((host->sd_write_sample_value[sel_index] != 0xff)))
                    {
                        host->sd_write_sample = host->sd_write_sample_value[sel_index];
                        host->sd_write_retry  = LL_SDHC_RETRY_CHECK;
                        SDHC_WARN_PRINTF("%s %d sd_write select new point : %d\r\n", __func__, __LINE__, host->sd_write_sample);
                    } else {
                        host->sd_write_retry = LL_SDHC_RETRY_ERR;
                        host->sd_write_retry_flag = 1;
                        SDHC_ERR_PRINTF("%s %d sd_write select all point!\r\n", __func__, __LINE__);
                    }
                    break;

                case LL_SDHC_RETRY_CHECK:
                    host->sd_write_retry = (host->sd_write_retry_flag) ? (LL_SDHC_RETRY_ERR) : (LL_SDHC_RETRY_SELECT_POINT);
                    break;

                case LL_SDHC_RETRY_SUCC :
                case LL_SDHC_RETRY_ERR  : 
                    if (!host->sd_write_retry_flag)     host->sd_write_retry = LL_SDHC_RETRY_SELECT_POINT;
                    break;
            }

            if (host->sd_write_retry < LL_SDHC_RETRY_SUCC)
            {
                retry_cnt++;
                goto __retry;
            }   
        } else if((ret == MMCSD_NO_ERR) && (host->io_cfg.self_adaption_flag != MMCSD_SMP_EN)) {
            if (host->sd_write_retry <= LL_SDHC_RETRY_CHECK)
            {
                host->sd_write_retry = (host->sd_write_retry == LL_SDHC_RETRY_CHECK) ? (LL_SDHC_RETRY_SUCC) : (LL_SDHC_RETRY_DEFAULT);
            }
        } 
#endif
    }

    if (ret && host->sd_stop) {
        sd_tran_stop(host);
    }  
__err:
    os_mutex_unlock(&host->lock);
    return ret;
}

int sd_multiple_read(struct sdh_device * host,uint32 lba, uint32 len, uint8* buf)
{
    struct rt_mmcsd_cmd  cmd;	
    int    ret         = 0;
    int    send_cmd    = 1;
    uint32 curr_lba    = lba;
    uint32 backup_lba  = host->new_lba;
    uint32 block_num   = len/SECTOR_SIZE;
    uint8  retry_cnt   = 0;
    uint8  curr_index  = 0;
    uint8  sel_index   = 0;
    uint8  *kick_buf    = buf;
    uint8  data_rev = 0;
	uint8  *s = NULL;

    os_mutex_lock(&host->lock,osWaitForever);

__retry:
    if(((curr_lba != host->new_lba)||(host->sd_opt != SD_M_R)) && host->sd_stop)
    {
        ret = sd_tran_stop(host);	
        if (ret)    goto __err;
        host->new_lba = curr_lba;
        send_cmd = 1;
    } else if ((host->sd_opt == SD_IDLE) || (!host->sd_stop)){
        send_cmd = 1;
        if (curr_lba != host->new_lba)  host->new_lba = curr_lba;
    } else {
        send_cmd = 0;
    }

    if ((host->new_lba + block_num) > host->card_max_blk_num)
    {
        SDHC_ERR_PRINTF(KERN_ERR"%s operation lba %d size %d max : %d err\r\n", __func__, host->new_lba, block_num, host->card_max_blk_num);
        host->new_lba = backup_lba;
        ret = RET_ERR;
        goto __err;
    }

    if (send_cmd)  host->sd_stop = (host->single_support) ? (block_num > 1) : (1);
    host->new_lba = host->new_lba + block_num;
    host->sd_opt  = SD_M_R;

    ///////////////////////////////////////////////////////
    host->data.blksize = SECTOR_SIZE;
    host->data.blks    = block_num;
    host->data.err     = 0;	

#if defined(TXW82X) || defined(TXW81X)
    if (host->io_cfg.self_adaption_flag != MMCSD_SMP_DIS)
    {
        sd_delay_config(host, LL_SDHC_DLY_CHAIN, host->sd_read_dly_chain);
        sd_set_sample(host, LL_SDHC_DAT_SMP_CFG_EN, 0, host->sd_read_sample);
    } 
#endif

    if(!retry_cnt && ((uint32)kick_buf >= PSRAM_BASE) ) {
		data_rev = (uint32)kick_buf % SYSCTRL_GET_DCACHE_LINE_SIZE;
        s = (uint8*)((uint32)kick_buf - data_rev);
        data_rev += ((uint32)kick_buf + len) % SYSCTRL_GET_DCACHE_LINE_SIZE;
		sys_dcache_invalid_range((void *)s, len + data_rev); 
	}


    if(host->read != NULL){
		ret = host->read(host,kick_buf);
	}

    ///////////////////////////////////////////////////////
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    cmd.cmd_code = (host->single_support) ? ((block_num > 1) ? (READ_MULTIPLE_BLOCK) : (READ_SINGLE_BLOCK)) : (READ_MULTIPLE_BLOCK);
    cmd.arg = curr_lba;
    if (!(host->cardflags & CARD_FLAG_SDHC)) 
    {
        cmd.arg <<= 9;
    }	
    cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_ADTC;
    if((host->cmd) && send_cmd){
        ret = host->cmd(host,&cmd);
        if(ret){
			sd_tran_stop(host);
            ret = MMCSD_CMD_ERR;
            goto __err;
        }
    }

    if (host->complete != NULL){
		ret = host->complete(host);
#if defined(TXW82X)
        if ((host->sd_read_retry_flag == 0) && (ret == MMCSD_CMP_DATERR) && (host->io_cfg.self_adaption_flag != MMCSD_SMP_EN))
        {
            if (host->read_last_time_retry && ((os_jiffies() - host->read_last_time_retry ) < 1000))
            {
                host->sd_read_retry         = LL_SDHC_RETRY_SELECT_POINT;
                host->read_last_time_retry  = 0;
            }
            
            switch (host->sd_read_retry)
            {
                case LL_SDHC_RETRY_DEFAULT:
                    host->sd_read_retry = LL_SDHC_RETRY_SELECT_POINT;
                    curr_lba  = host->new_lba - host->data.blks;
                    block_num = host->data.blks;
                    kick_buf  = kick_buf + (block_num - host->data.blks) * SECTOR_SIZE;
                    if (host->read_last_time_retry  == 0)   host->read_last_time_retry  = os_jiffies();
                    break;

                case LL_SDHC_RETRY_SELECT_POINT:
                    for (curr_index = 0; curr_index < host->sd_read_sample_num; curr_index++)
                    {
                        if ((host->sd_read_sample_value[curr_index] != 0xff) && (host->sd_read_sample_value[curr_index] == host->sd_read_sample))
                        {
                            host->sd_read_sample_value[curr_index]  = 0xff;
                            sel_index = (curr_index + host->sd_read_sample_num - 1) % host->sd_read_sample_num;
                            break;
                        }
                    }
                    if ((curr_index < host->sd_read_sample_num) && (host->sd_read_sample_value[sel_index] != 0xff))
                    {
                        host->sd_read_sample = host->sd_read_sample_value[sel_index];
                        host->sd_read_retry  = LL_SDHC_RETRY_CHECK;
                        SDHC_WARN_PRINTF("%s %d sd_read select new point : %d\r\n", __func__, __LINE__, host->sd_read_sample);
                    } else {
                        host->sd_read_retry = LL_SDHC_RETRY_ERR;
                        host->sd_read_retry_flag = 1;
                        SDHC_ERR_PRINTF("%s %d sd_read select all point!\r\n", __func__, __LINE__);
                    }
                    break;

                case LL_SDHC_RETRY_CHECK:
                    host->sd_read_retry = (host->sd_read_retry_flag) ? (LL_SDHC_RETRY_ERR) : (LL_SDHC_RETRY_SELECT_POINT);
                    break;
                    
                case LL_SDHC_RETRY_SUCC:
                case LL_SDHC_RETRY_ERR : 
                    if (!host->sd_read_retry_flag)     host->sd_read_retry = LL_SDHC_RETRY_SELECT_POINT;
                    break;
            }

            if (host->sd_read_retry < LL_SDHC_RETRY_SUCC)
            {
                retry_cnt++;
                goto __retry;
            }   
        } else if((ret == MMCSD_NO_ERR) && (host->io_cfg.self_adaption_flag != MMCSD_SMP_EN)) {
            if (host->sd_read_retry <= LL_SDHC_RETRY_CHECK)
            {
                host->sd_read_retry = (host->sd_read_retry == LL_SDHC_RETRY_CHECK) ? (LL_SDHC_RETRY_SUCC) : (LL_SDHC_RETRY_DEFAULT);
            }
        }
#endif
        // SDHC_WARN_PRINTF("%s %d status : %d flag : %d\r\n", __func__, __LINE__, host->sd_read_retry, host->sd_read_retry_flag);
	} 

__err:
    os_mutex_unlock(&host->lock);
    return ret;
}


uint32 send_app_op_cond(struct sdh_device *host,
                                uint32           ocr,
                                uint32          *rocr)
{
    struct rt_mmcsd_cmd cmd;
    uint32 i;
    int ret;
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    cmd.cmd_code = SD_APP_OP_COND;
    cmd.arg = ocr;
    cmd.flags = RESP_SPI_R1 | RESP_R3 | CMD_BCR;

    for(i = 100;i;i--){
        ret = send_app_cmd(host,0);
        if(ret){
            SDHC_ERR_PRINTF("cmd err\r\n");
            break;
        }
        
//        memset(cmd->resp, 0, sizeof(cmd->resp));		
        ret = host->cmd(host,&cmd);
        if(ret){
            SDHC_ERR_PRINTF("cmd2 err\r\n");
            break;
        }
        // SDHC_WARN_PRINTF("cmd resp:%x\r\n",cmd.resp[0]);
        if (cmd.resp[0] & CARD_BUSY){
            SDHC_WARN_PRINTF("card busy ok\r\n");
            break;
        }
        os_sleep_ms(10);		
    }

    if(rocr)
        *rocr = cmd.resp[0];

    if(!(cmd.resp[0] & CARD_BUSY))
    {
        SDHC_ERR_PRINTF("card no busy\r\n");
        return RET_ERR;
    } 

    return ret;
}

void stop_card(){	
    sd_tran_stop(sdh_test);
}

uint8 get_sd_status(void) {
	//SDHC_WARN_PRINTF("%s:%d\r\n",__FUNCTION__,sdh_test->sd_opt);
	return sdh_test->sd_opt;
}
uint8 get_sd_status2(void) {
	return sdh_test->sd_opt == SD_OFF;
}

int sd_scsi_read2(uint32 lba,uint8* buf) {
	return sd_multiple_read(sdh_test,lba,SECTOR_SIZE ,buf);
}

int sd_scsi_write2(uint32 lba,uint8* buf) {
	return sd_multiple_write(sdh_test,lba,SECTOR_SIZE ,buf);
}

int usb_sd_scsi_read(uint32 lba, uint32 count, uint8* buf) {
    return sd_multiple_read(sdh_test,lba,SECTOR_SIZE*count,buf);
}

int usb_sd_scsi_write(uint32 lba, uint32 count, uint8* buf) {
    return sd_multiple_write(sdh_test,lba,SECTOR_SIZE*count,buf);
}


//返回sd卡的扇区大小,现在是固定的
uint32_t get_sd_sector_size()
{
    return 512;
}

uint32 get_sd_cap() 
{
	return sd_dwCap * 2;//Blocks
}

#if (defined (TXW81X) || defined (TXW82X))
uint16 sd_spi_mode_baud_cfg(uint32 clk)
{
   return (((peripheral_clock_get(HG_APB0_PT_SPI0) / 2  + clk  - 1) / clk) - 1);
}
void sd_sample_print(uint8 *buf, uint8 cnt, char *str, uint8 print_en)
{
    if (print_en)
    {
        SDHC_WARN_PRINTF("%s\r\n", str);
        for (int z = 0; z < cnt; z++)
        {
            _os_printf("%d, ", buf[z]);
        }
        _os_printf("\r\n");
    }
}

uint32 sd_cmd_sample_retry(struct sdh_device *host, uint8 sample_max)
{
    uint32 result    = RET_OK;
    uint8  buf_index = 0;
    uint32 ret       = 0;
    uint32 last_ret  = MMCSD_INT_VLE;
    for (int i = 0; i < sample_max; i++)
    {
        (host->sd_mode_type) ? (host->spi_delay_cycle = i) : (sd_set_sample(host, LL_SDHC_CMD_SMP_CFG_EN, i, 0));//
        ret = send_card_status(host);
        if (!ret)
        {
            host->sd_cmd_sample_value[buf_index++] = i;
        } else if (!last_ret && (last_ret != ret) && buf_index) {
            break;
        }
        last_ret = ret;
    }

    if (buf_index)
    {
        sd_sample_print(host->sd_cmd_sample_value, buf_index, "cmd", SD_SAMPLE_VALUE_PRINT);
        host->sd_cmd_sample_num = buf_index;
        host->sd_cmd_sample     = host->sd_cmd_sample_value[(buf_index)>>1];
        SDHC_WARN_PRINTF("%s %d sd_cmd_sample : %d\r\n", __func__, __LINE__, host->sd_cmd_sample);
        (host->sd_mode_type) ? (host->spi_delay_cycle = host->sd_cmd_sample) : (sd_set_sample(host, LL_SDHC_CMD_SMP_CFG_EN, host->sd_cmd_sample, 0));
    } else {
        SDHC_ERR_PRINTF("sd sample point num : %d err\r\n", buf_index);
        result = RET_ERR;
    }
    return result;
}

uint32 sd_read_dat_sample_retry(struct sdh_device *host, uint8 sample_max, uint8 *p_read, uint8 *p_write)
{
    uint32 result      = RET_OK;
    uint8  buf_index   = 0;
    uint8  sel_index   = 0;
    uint32 ret         = 0;
    uint32 last_ret    = MMCSD_INT_VLE;
    uint32 write_block = 0;

    for (int i = 0; i < sample_max; i++)
    {
        host->sd_read_sample = (i + sample_max - 2) % sample_max;
        ret = sd_multiple_read(host, 0, SECTOR_SIZE, p_read);
        if (ret == MMCSD_NO_ERR)
        {
            host->sd_read_sample_value[buf_index++] = host->sd_read_sample;
        } else if (ret == MMCSD_CMD_ERR) {
            buf_index = 0;
            result = RET_ERR;
            break;
        } else if (!last_ret && (last_ret != ret) && buf_index) {
            break;
        }
        last_ret = ret;
    }

    if (buf_index)
    {
        write_block              = host->card_max_blk_num - sample_max - 1;
        host->sd_read_sample_num = buf_index;
        sel_index                = (buf_index - 1) >> 1;
        host->sd_read_sample     = (host->sd_read_sample_value[sel_index] == 0) ? (host->sd_read_sample_value[(sel_index + buf_index - 1) % buf_index]) : (host->sd_read_sample_value[sel_index]); 
        ret = sd_multiple_read(host, write_block, SECTOR_SIZE*sample_max, (void *)p_write);
        if (!ret)
        {
            sd_sample_print(host->sd_read_sample_value, buf_index, "read", SD_SAMPLE_VALUE_PRINT);
            SDHC_WARN_PRINTF("%s %d sd_read_sample : %d\r\n", __func__, __LINE__, host->sd_read_sample);
        } else {
            result = RET_ERR;
        }
    }  else {
        result = RET_ERR;
    }
    return result;
}

uint32 sd_write_dat_sample_retry(struct sdh_device *host, uint8 sample_max, uint8 *p_write)
{
    uint32 result      = RET_OK;
    uint8  buf_index   = 0;
    uint32 ret         = 0;
    uint32 last_ret    = MMCSD_INT_VLE;
    uint32 write_block = host->card_max_blk_num - sample_max - 1;

    for (int i = 0; i < sample_max; i++)
    {
        host->sd_write_sample = i;
        ret = sd_multiple_write(host, write_block+i, SECTOR_SIZE, (void *)&p_write[SECTOR_SIZE*i]);
        if (!ret)
        {
            host->sd_write_sample_value[buf_index++] = i;
        } else if (ret == MMCSD_CMD_ERR) {
            buf_index = 0;
            break;
        } else if (!last_ret && (last_ret != ret) && buf_index) {
            break;
        } 
        last_ret = ret;
    }

    if (buf_index)
    {
        sd_sample_print(host->sd_write_sample_value, buf_index, "write\r\n", SD_SAMPLE_VALUE_PRINT);
        host->sd_write_sample     = host->sd_write_sample_value[(buf_index - 1)>>1];
        host->sd_write_sample_num = buf_index;
    } else {
        result = RET_ERR;
    }
    return result;
}

uint32 sd_sample_point_cfg(struct sdh_device *host, uint32 clk)
{
    uint32        ret_val                    = RET_OK;
    uint32        ret                        = 0;
    uint8         flag                       = 0; 
    uint8         sample_max                 = 0;//host->io_cfg.crc_sample_max;
    uint8         *test_data                 = NULL;//os_malloc(SECTOR_SIZE);
    uint8         *write_data                = NULL;//os_malloc(SECTOR_SIZE * sample_max);
    uint32        src_clk                    = host->io_cfg.clock;

    if ((host->card_type != CARD_TYPE_SD))
    {
        ret_val = RET_ERR;
        goto __err;
    }

    sd_set_clk(host, clk);
    sample_max   = host->io_cfg.crc_sample_max;
#if defined(TXW82X)
    host->spi_mode_baud = sd_spi_mode_baud_cfg(clk);
    if(!host->sd_read_sample_value )    host->sd_read_sample_value  = SDHC_HEAP_MALLOC(sample_max);
    if(!host->sd_write_sample_value)    host->sd_write_sample_value = SDHC_HEAP_MALLOC(sample_max);
    if(!host->sd_cmd_sample_value  )    host->sd_cmd_sample_value   = SDHC_HEAP_MALLOC(sample_max);
    os_printf("read : %08x write : %08x cmd : %08x\r\n", (uint32)host->sd_read_sample_value , (uint32)host->sd_write_sample_value, (uint32)host->sd_cmd_sample_value );
#endif
    test_data    = os_malloc(SECTOR_SIZE);
    write_data   = os_malloc(SECTOR_SIZE * sample_max);

    if ((test_data == NULL) || (write_data == NULL)|| (!host->sd_read_sample_value) || (!host->sd_write_sample_value) || (!host->sd_cmd_sample_value))
    {
        os_printf("test_data : %08x write_data : %08x num : %d read : %08x write : %08x cmd : %08x err\r\n", (uint32)test_data, (uint32)write_data, sample_max, (uint32)host->sd_read_sample_value, (uint32)host->sd_write_sample_value, (uint32)host->sd_cmd_sample_value);
        ret_val = RET_ERR;
        sd_set_clk(host, src_clk);
#if defined(TXW82X)
        host->spi_mode_baud = sd_spi_mode_baud_cfg(src_clk);
#endif
        goto __err;
    }
    host->sd_write_dly_chain = 0x3;
    host->sd_read_dly_chain  = 0x0;
    host->io_cfg.self_adaption_flag = MMCSD_SMP_EN;
    sd_set_sample(host, LL_SDHC_ALL_SMP_CFG_DIS, 0, 0);

    os_memset(host->sd_read_sample_value , 0xff, sample_max);
    os_memset(host->sd_write_sample_value, 0xff, sample_max);
    os_memset(host->sd_cmd_sample_value  , 0xff, sample_max);

    ret = sd_cmd_sample_retry(host, sample_max);
    if (ret)
    {
        goto __adjust; 
    } else {
        flag |= SDHC_SMP_CMD_SUCC;
    }
    
    
    while (1)
    {
        ret = sd_read_dat_sample_retry(host, sample_max, test_data, write_data);
        if (ret)
        {
            host->sd_read_dly_chain += 2;
            if (host->sd_read_dly_chain > 7)    goto __adjust; 
        } else {
            flag |= SDHC_SMP_DAT_SUCC;
            break;
        }
    }

    while(1)
    {
        ret = sd_write_dat_sample_retry(host, sample_max, write_data);
        if (ret)
        {
            host->sd_write_dly_chain += 2;
            if (host->sd_write_dly_chain > 7)    goto __adjust; 
        } else {
            flag |= SDHC_SMP_WRITE_CRC_SUCC;
            break;
        }
    }

__adjust:
    if(flag != SDHC_SMP_ALL_SUCC)
    {
		sd_set_sample(host, LL_SDHC_ALL_SMP_CFG_DIS, 0, 0);
        SDHC_WARN_PRINTF("******** write : %d read : %d cmd : %d read_chain : %d write_chain : %d***********\r\n", host->sd_write_sample, host->sd_read_sample, host->sd_cmd_sample, host->sd_read_dly_chain, host->sd_write_dly_chain);  
        ret_val = RET_ERR;
#if defined(TXW82X)
        if(host->sd_read_sample_value )  SDHC_HEAP_FREE(host->sd_read_sample_value );
        if(host->sd_write_sample_value)  SDHC_HEAP_FREE(host->sd_write_sample_value);
        if(host->sd_cmd_sample_value  )  SDHC_HEAP_FREE(host->sd_cmd_sample_value  );
#endif
    } else {
        SDHC_WARN_PRINTF("******** write : %d read : %d cmd : %d read_chain : %d write_chain : %d***********\r\n", host->sd_write_sample, host->sd_read_sample, host->sd_cmd_sample, host->sd_read_dly_chain, host->sd_write_dly_chain);  
    }

__err:
    // sd_tran_stop(host);
    if (test_data ) os_free(test_data );
    if (write_data) os_free(write_data);
    return ret_val;
}
#endif

uint32 sd_init(struct sdh_device * host, uint32 clk, uint8 single_support)
{
    uint32 ret;
    uint32 resp[4];
    uint32 ocr;
    uint8  bw  = 1;
    SDHC_WARN_PRINTF("open_width:%d\r\n",bw);


    if(host->open)
        host->open(host,bw, SD_MODE_TYPE);

#if defined (TXW82X)
    host->sd_clk_io  = MACRO_PIN(PIN_SDH_CLK);
#endif
    host->single_support = single_support;

    sdhost_io_func_init(host->flags&MMCSD_BUSWIDTH_4);
    SDHC_WARN_PRINTF("host->flags:%x\r\n",host->flags);
    if(bw == 4)
        sd_power_up(host,MMCSD_BUSWIDTH_4);
    else
        sd_power_up(host,0);
    
    void __delay_asm(uint32 n);
    ret = send_idle(host);
    if(ret)
    {
        SDHC_ERR_PRINTF("idle cmd err\r\n");
        return RET_ERR;
    }
	delay_us(100);
    ret = send_if_cond(host,host->valid_ocr);
    if(ret)
    {
        SDHC_ERR_PRINTF("SEND_IF_COND cmd err\r\n");
        return RET_ERR;
    }

    ret = send_app_op_cond(host,0x40ff8000,&ocr);
    if(ret){
        SDHC_ERR_PRINTF("init card err\r\n");
        return RET_ERR;
    }
    SDHC_WARN_PRINTF("ocr:%x\r\n",ocr);
    ocr = select_voltage(host,ocr);
    SDHC_WARN_PRINTF("cur_ocr:%x\r\n",ocr);

    if (!ocr)
    {
        SDHC_ERR_PRINTF("cal ocr error\r\n");
        return RET_ERR;
    }	

    send_idle(host);
    delay_us(100);
    ret = send_if_cond(host,ocr);
    if(ret==0)
        ocr |= 1 << 30;

    ret = send_app_op_cond(host,ocr,NULL);
    if(ret){
        SDHC_ERR_PRINTF("init card app_op_cond err\r\n");
        return RET_ERR;
    }	

    send_all_get_cid(host,resp);
    host->card_type = CARD_TYPE_SD;
    memcpy(host->resp_cid,resp,sizeof(host->resp_cid));
    send_get_card_addr(host,&host->rca);
    send_get_csd(host,host->resp_csd);
    sd_parse_csd(host);
    send_select_card(host);
    ret = send_get_scr(host,host->resp_scr);
    if (ret == 0)
    {
        SDHC_ERR_PRINTF("get src err\r\n");
        return RET_ERR;
    }
    
    sd_parse_scr(host);
	
	/*switch bus width*/
	if ((host->flags & MMCSD_BUSWIDTH_4) &&
		(host->scr.sd_bus_widths & SD_SCR_BUS_WIDTH_4)) 
	{
		ret = sd_app_set_bus_width(host, MMCSD_BUS_WIDTH_4);
		if (ret){
			SDHC_ERR_PRINTF("set bus width 4 err\r\n");
			return RET_ERR;
		}
		sd_set_bus_width(host, MMCSD_BUS_WIDTH_4);	
	}
    host->sd_opt = SD_IDLE;
#if (defined (TXW81X) || defined (TXW82X))
    if ((host->flags & MMCSD_SUP_HIGHSPEED) &&
        (!host->io_cfg.self_adaption_flag)  &&
        (host->scr.sd_version)              &&
        (clk > 25*1000*1000))
    {		
        ret = sd_switch(host);
        if (ret)
        {
            sd_set_clk(host, 24*1000*1000);
        }else{
            SDHC_WARN_PRINTF("********** test SD start  ********\r\n");
            ret = sd_sample_point_cfg(host, clk);
            SDHC_WARN_PRINTF("********** test SD finish ********\r\n");
            if (ret)
            {
                SDHC_ERR_PRINTF("set highspeed sampling point err\r\n");
                host->io_cfg.self_adaption_flag = MMCSD_SMP_DIS;
                sd_set_clk(host, 24*1000*1000);
            }
            host->io_cfg.self_adaption_flag = MMCSD_SMP_SUCC;
        }	
    }else{
        sd_set_clk(host, 24*1000*1000);
    }
#else
    sd_set_clk(host, clk);
#endif
    ((struct hgsdh *)host)->opened = 1;
#if defined(TXW82X)
    host->spi_mode_baud =sd_spi_mode_baud_cfg(host->io_cfg.clock);
#endif
    return RET_OK;
}





extern bool fatfs_register();
extern void fatfs_unregister();
void sd_open()
{
    sdh_test = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);
#if SDH_I2C2_REUSE
    os_sema_init(&sem,1);
#endif
}

void sdhost_test()
{
    SDHC_WARN_PRINTF("enter sdhost test\r\n");
    sdh_test = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);
//sdhost_io_func_init();f

    sd_init(sdh_test, 24*1000*1000, 0);

}

uint32 sdhost_suspend(struct sdh_device * host){
    uint32 ret = -1;
    if(host->suspend){
        ret = host->suspend(host);
    }
    return ret;
}

uint32 sdhost_resume(struct sdh_device * host){
    uint32 ret = -1;
    if(host->resume){
        ret = host->resume(host);
    }
    return ret;

}
struct sdhost_hdl
{
    struct os_work wk;
    struct sdh_device *host;
    uint32_t lba;
    uint8_t opt:3,isregister:1,count:2;
};

static struct sdhost_hdl sdhost_wk = {
	.wk.running=0
};



int32 sdh_loop(struct os_work *work)
{
    struct sdhost_hdl *hdl = (struct sdhost_hdl*)work;
    struct sdh_device *host = hdl->host;
    uint32 sleep_time = 500;
    uint32 ret;
    if(SD_OFF == host->sd_opt || !hdl->isregister)
    {
        SDHC_ERR_PRINTF("sdh no online2\r\n");
        if(get_fat_isready())
        {
            fatfs_unregister();
            hdl->isregister = 0;
        }

        //判断状态,是否重新挂在文件系统
        fatfs_register();
        if(get_fat_isready())
        {
            hdl->isregister = 1;
        }
    } else {
        ret = os_mutex_lock(&host->lock,0);
        if(ret)
        {
            sleep_time = 1;
            //获取锁失败
            goto sdh_loop_end;
        }

        if (host->sd_stop && (SD_IDLE != host->sd_opt))
        {
            if((hdl->opt != host->sd_opt)||(hdl->lba != host->new_lba))
            {
                hdl->opt = host->sd_opt;
                hdl->lba = host->new_lba;
                hdl->count = 0;
            } else {
                hdl->count++;
            }
        } else {
            ret = send_card_status(host);
            if(ret != 0)
            {
                host->sd_opt = SD_OFF;
                hdl->count = 0;
            }
        }

        if(hdl->count >= 2)
        {
            sd_tran_stop(host);
        }
        
        os_mutex_unlock(&host->lock);
    }
sdh_loop_end:
    os_run_work_delay(work, sleep_time);
	return 0;
}



uint32 sdhost_deinit_for_sleep()
{
    uint32 err = 0;
    struct sdh_device *sdh = NULL;
    sdh = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);
    os_work_cancle2(&sdhost_wk.wk,1);
    return err;
}


uint32 sdhost_init(uint32 clk, uint8 single_support)
{
    uint32 err = 1;
    struct sdh_device *sdh = NULL;
    sdh = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);

#if SDH_I2C2_REUSE
    os_sema_down(&sem,osWaitForever);
#endif

    if(sdh)
    {
        err = sd_init(sdh, clk, single_support);
        if(err)
            sdh->sd_opt = SD_OFF;

        if(sdhost_wk.wk.init == 0  && sdhost_wk.wk.running == 0)
        {
            sdhost_wk.host = sdh;
            sdhost_wk.isregister = 1;
            OS_WORK_INIT(&sdhost_wk.wk, sdh_loop, 0);
            os_run_work_delay(&sdhost_wk.wk, 500);
        }
    }

#if SDH_I2C2_REUSE
    os_sema_up(&sem);
#endif

    return err;
}


#if SDH_I2C2_REUSE
/**
 * @brief 该函数用于SDH和I2C2端口复用的切换
 * 
 * @param sdh_stop_en 停止SDH、使用I2C2，则置1，否则置0
 * @return uint32 
 */
uint32 sdhost_i2c2_exchange(int sdh_stop_en)
{
    if(sdh_stop_en) {
        os_sema_down(&sem,osWaitForever);
        os_work_cancle2(&sdhost_wk.wk,1);
        pin_func(HG_I2C2_DEVID,1);
    } else {
        os_sema_up(&sem);
        OS_WORK_INIT(&sdhost_wk.wk, sdh_loop, 0);
        os_run_work_delay(&sdhost_wk.wk, 500);
    }
}

#endif

uint32 get_sdhost_status(struct sdh_device *host)
{
    if(host->sd_opt == SD_OFF)
    {
        return 1;
    }
    return 0;
}


