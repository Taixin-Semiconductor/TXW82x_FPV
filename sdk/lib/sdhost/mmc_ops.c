#include "sys_config.h"
#include "typesdef.h"
#include "devid.h"
#include "list.h"
#include "dev.h"
#include "osal/task.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "lib/sdhost/sdhost.h"
#include "lib/sdhost/mmc.h"
#include "hal/gpio.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "osal/task.h"
#include "osal/sleep.h"
#include "osal/timer.h"
#include "osal/work.h"


#if 1
#define EMMC_PRINTF(fmt, arg...) printf(fmt, ##arg)
#else
#define EMMC_PRINTF(fmt, arg...)
#endif

/*
 * Read extended CSD.
 */
int mmc_get_ext_csd(struct sdh_device *host, uint8_t *ext_csd)
{
    int ret;
    struct rt_mmcsd_cmd cmd;
    struct rt_mmcsd_data data;

    if (GET_BITS(host->resp_csd, 122, 4) < 4) {
        EMMC_PRINTF("can't get ext csd\r\n");
        return 0;
    }
        

    /*
    * As the ext_csd is so large and mostly unused, we don't store the
    * raw block in mmc_card.
    */

    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    memset(&data, 0, sizeof(struct rt_mmcsd_data));

    cmd.cmd_code = SEND_EXT_CSD;
    cmd.arg = 0;

    /* NOTE HACK:  the RESP_SPI_R1 is always correct here, but we
    * rely on callers to never use this with "native" calls for reading
    * CSD or CID.  Native versions of those commands use the R2 type,
    * not R1 plus a data block.
    */
    cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_ADTC;

    host->data.blksize = 512;
    host->data.blks = 1;
    host->data.flags = DATA_DIR_READ;
    host->data.buf = ext_csd;

    /*
    * Some cards require longer data read timeout than indicated in CSD.
    * Address this by setting the read timeout to a "reasonably high"
    * value. For the cards tested, 300ms has proven enough. If necessary,
    * this value can be increased if other problematic cards require this.
    */
    host->data.timeout_ns = 300000000;
    host->data.timeout_clks = 0;

    ret = host->cmd(host, &cmd);
    if(ret) {
        EMMC_PRINTF("%s cmd error\r\n", __FUNCTION__);
        return 1;
    }
	
    ret = host->read(host, ext_csd);

    if (host->complete)
        ret |= host->complete(host);

    if(ret) {
        return RET_ERR;
    }

    return 0;
}

int mmc_parse_ext_csd(struct sdh_device *host, uint8_t *ext_csd)
{
    uint32_t card_capacity = 0;
    if (ext_csd == NULL)
    {
        EMMC_PRINTF("emmc parse ext csd fail, invaild args");
        return RET_ERR;
    }

    // uint8_t device_type = ext_csd[EXT_CSD_CARD_TYPE];
    // if ((host->flags & MMCSD_SUP_HS400) && (device_type & EXT_CSD_CARD_TYPE_HS400))
    // {
    //     card->flags |=  CARD_FLAG_HS400;
    //     card->max_data_rate = 200000000;
    // }
    // else if ((host->flags & MMCSD_SUP_HS200) && (device_type & EXT_CSD_CARD_TYPE_HS200))
    // {
    //     card->flags |=  CARD_FLAG_HS200;
    //     card->max_data_rate = 200000000;
    // }
    // else if ((host->flags & MMCSD_SUP_HIGHSPEED_DDR) && (device_type & EXT_CSD_CARD_TYPE_DDR_52))
    // {
    //     card->flags |=  CARD_FLAG_HIGHSPEED_DDR;
    //     card->hs_max_data_rate = 52000000;
    // }
    // else
    // {
    //     card->flags |=  CARD_FLAG_HIGHSPEED;
    //     card->hs_max_data_rate = 52000000;
    // }

    // host->flags |= CARD_FLAG_HIGHSPEED;
    // card->hs_max_data_rate = 52000000;

    // if (ext_csd[EXT_CSD_STROBE_SUPPORT] != 0)
    // {
    //     card->ext_csd.enhanced_data_strobe = 1;
    // }

    // host->ext_csd.cache_size =
    //     ext_csd[EXT_CSD_CACHE_SIZE + 0] << 0 |
    //     ext_csd[EXT_CSD_CACHE_SIZE + 1] << 8 |
    //     ext_csd[EXT_CSD_CACHE_SIZE + 2] << 16 |
    //     ext_csd[EXT_CSD_CACHE_SIZE + 3] << 24;

    card_capacity = *((uint32_t*)&ext_csd[EXT_CSD_SEC_CNT]);
    card_capacity /= 2; //unit: KB  
    host->card_capacity = card_capacity;
	
    EMMC_PRINTF("emmc card capacity %d KB, card sec count:0x%x \r\n", card_capacity, card_capacity*2);
    EMMC_PRINTF("bus mode: %d\r\n", ext_csd[EXT_CSD_BUS_WIDTH]);
    // switch (ext_csd[EXT_CSD_PART_CONFIG] & 7)
    // {
    // case 0:
    //     EMMC_PRINTF("access default partition\r\n");
    //     break;
    // case 1:
    // case 2:
    //     EMMC_PRINTF("access boot partition-%d\r\n", (ext_csd[EXT_CSD_PART_CONFIG]&7));
    //     break;
    // case 3:
    //     EMMC_PRINTF("access RPMB partition\r\n");
    //     break;
    // case 4:
    // case 5:
    // case 6:
    // case 7:
    //     EMMC_PRINTF("access to gp partition-%d\r\n", (ext_csd[EXT_CSD_PART_CONFIG]&7) - 3);
    //     break;
    // default:
    //     EMMC_PRINTF("access to ?\r\n");
    // }

    return 0;
}

int mmc_switch(struct sdh_device *host, uint8_t set, uint8_t index, uint8_t value)
{
    int err;
    struct rt_mmcsd_cmd cmd = {0};

    cmd.cmd_code = SWITCH;
    cmd.arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
              (index << 16) | (value << 8) | set;
    cmd.flags = RESP_R1B | CMD_AC;

    err = host->cmd(host, &cmd);
    if (err)
        return err;

    return 0;
}

int mmc_compare_ext_csds(struct sdh_device *host, uint8_t *ext_csd, uint32_t bus_width)
{
    uint8_t *bw_ext_csd = NULL;
    int err = 0;

    if (bus_width == MMCSD_BUS_WIDTH_1)
        return 0;

    bw_ext_csd = os_malloc(512);
    if(!bw_ext_csd) {
        EMMC_PRINTF("%s malloc fail\r\n", __FUNCTION__);
        return RET_ERR;
    }

    err = mmc_get_ext_csd(host, bw_ext_csd);

    /* only compare read only fields */
    err = !((ext_csd[EXT_CSD_PARTITION_SUPPORT] == bw_ext_csd[EXT_CSD_PARTITION_SUPPORT]) &&
            (ext_csd[EXT_CSD_ERASED_MEM_CONT] == bw_ext_csd[EXT_CSD_ERASED_MEM_CONT]) &&
            (ext_csd[EXT_CSD_REV] == bw_ext_csd[EXT_CSD_REV]) &&
            (ext_csd[EXT_CSD_STRUCTURE] == bw_ext_csd[EXT_CSD_STRUCTURE]) &&
            (ext_csd[EXT_CSD_CARD_TYPE] == bw_ext_csd[EXT_CSD_CARD_TYPE]) &&
            (ext_csd[EXT_CSD_S_A_TIMEOUT] == bw_ext_csd[EXT_CSD_S_A_TIMEOUT]) &&
            (ext_csd[EXT_CSD_HC_WP_GRP_SIZE] == bw_ext_csd[EXT_CSD_HC_WP_GRP_SIZE]) &&
            (ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT] == bw_ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT]) &&
            (ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] == bw_ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]) &&
            (ext_csd[EXT_CSD_SEC_TRIM_MULT] == bw_ext_csd[EXT_CSD_SEC_TRIM_MULT]) &&
            (ext_csd[EXT_CSD_SEC_ERASE_MULT] == bw_ext_csd[EXT_CSD_SEC_ERASE_MULT]) &&
            (ext_csd[EXT_CSD_SEC_FEATURE_SUPPORT] == bw_ext_csd[EXT_CSD_SEC_FEATURE_SUPPORT]) &&
            (ext_csd[EXT_CSD_TRIM_MULT] == bw_ext_csd[EXT_CSD_TRIM_MULT]) &&
            (ext_csd[EXT_CSD_SEC_CNT + 0] == bw_ext_csd[EXT_CSD_SEC_CNT + 0]) &&
            (ext_csd[EXT_CSD_SEC_CNT + 1] == bw_ext_csd[EXT_CSD_SEC_CNT + 1]) &&
            (ext_csd[EXT_CSD_SEC_CNT + 2] == bw_ext_csd[EXT_CSD_SEC_CNT + 2]) &&
            (ext_csd[EXT_CSD_SEC_CNT + 3] == bw_ext_csd[EXT_CSD_SEC_CNT + 3]) &&
            (ext_csd[EXT_CSD_PWR_CL_52_195] == bw_ext_csd[EXT_CSD_PWR_CL_52_195]) &&
            (ext_csd[EXT_CSD_PWR_CL_26_195] == bw_ext_csd[EXT_CSD_PWR_CL_26_195]) &&
            (ext_csd[EXT_CSD_PWR_CL_52_360] == bw_ext_csd[EXT_CSD_PWR_CL_52_360]) &&
            (ext_csd[EXT_CSD_PWR_CL_26_360] == bw_ext_csd[EXT_CSD_PWR_CL_26_360]) &&
            (ext_csd[EXT_CSD_PWR_CL_200_195] == bw_ext_csd[EXT_CSD_PWR_CL_200_195]) &&
            (ext_csd[EXT_CSD_PWR_CL_200_360] == bw_ext_csd[EXT_CSD_PWR_CL_200_360]) &&
            (ext_csd[EXT_CSD_PWR_CL_DDR_52_195] == bw_ext_csd[EXT_CSD_PWR_CL_DDR_52_195]) &&
            (ext_csd[EXT_CSD_PWR_CL_DDR_52_360] == bw_ext_csd[EXT_CSD_PWR_CL_DDR_52_360]) &&
            (ext_csd[EXT_CSD_PWR_CL_DDR_200_360] == bw_ext_csd[EXT_CSD_PWR_CL_DDR_200_360]));

    if (err)
        err = -RET_ERR;

    os_free(bw_ext_csd);
    return err;
}

int mmc_select_bus_width(struct sdh_device *host, uint8_t *ext_csd)
{

    unsigned bus_width = 0;
    int err = 0;

    if (GET_BITS(host->resp_csd, 122, 4) < 4)
        return 0;

    /*
    * Unlike SD, MMC cards don't have a configuration register to notify
    * supported bus width. So bus test command should be run to identify
    * the supported bus width or compare the EXT_CSD values of current
    * bus width and EXT_CSD values of 1 bit mode read earlier.
    */

    err = mmc_switch(host, EXT_CSD_CMD_SET_NORMAL,
                     EXT_CSD_BUS_WIDTH,
                     EXT_CSD_BUS_WIDTH_4);

    //2就是EXT_CSD_BUS_WIDTH_4
    sd_set_bus_width(host, 2);
    err = mmc_compare_ext_csds(host, ext_csd, bus_width);

    if(err) {
        mmc_switch(host, EXT_CSD_CMD_SET_NORMAL,
                     EXT_CSD_BUS_WIDTH,
                     EXT_CSD_BUS_WIDTH_1);
        //0就是EXT_CSD_BUS_WIDTH_1
        sd_set_bus_width(host, 0);
        err = mmc_compare_ext_csds(host, ext_csd, bus_width);
        if(err) {
            EMMC_PRINTF("%s dangenrous error ocurr\r\n", __FUNCTION__);
        }
    }

	EMMC_PRINTF("bus width: %d\r\n", 1 << host->io_cfg.bus_width);

    return err;
}

int mmc_set_card_addr(struct sdh_device *host, uint32_t rca)
{
  int err;
  struct rt_mmcsd_cmd cmd;
  
  memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
  
  cmd.cmd_code = SET_RELATIVE_ADDR;
  cmd.arg = rca << 16;
  cmd.flags = RESP_R1 | CMD_AC;
  
  err = host->cmd(host, &cmd);
  if (err)
    return err;
  
  return 0;
}

int __send_status(struct sdh_device *host, uint32_t *status, unsigned retries)
{
    int err;
    struct rt_mmcsd_cmd cmd;

    cmd.cmd_code = SEND_STATUS;
    cmd.arg = host->rca << 16;
    cmd.flags = RESP_R1 | CMD_AC;
    err = host->cmd(host, &cmd);

    if (err)
        return err;

    if (status)
        *status = cmd.resp[0];

    return 0;
}

int card_busy_detect(struct sdh_device *host, unsigned int timeout_ms,
                     uint32_t *resp_errs)
{
    int err = 0;
    uint32_t status;
    uint32_t start;
    uint32_t is_to;

    start = os_jiffies();
    do
    {
        is_to = (int)(os_jiffies() - start) > timeout_ms;

        err = __send_status(host, &status, 5);
        if (err)
        {
            EMMC_PRINTF("error %d requesting status", err);
            return err;
        }

        /* Accumulate any response error bits seen */
        if (resp_errs)
            *resp_errs |= status;

        if (is_to)
        {
            EMMC_PRINTF("wait card busy timeout");
            return -RET_ERR;
        }
        /*
         * Some cards mishandle the status bits,
         * so make sure to check both the busy
         * indication and the card state.
         */
    }
    while (!(status & (1 << 8)) ||
            (((status & 0x00001E00) >> 9) == 7));

    return err;
}

int mmcsd_req_blk1(struct sdh_device *host,
                                 uint32_t           sector,
                                 void                 *buf,
                                 uint32_t             blks,
                                 uint8_t            dir)
{
    struct rt_mmcsd_cmd  cmd, stop;
    uint32_t r_cmd, w_cmd;
    int err;

    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    memset(&stop, 0, sizeof(struct rt_mmcsd_cmd));

    cmd.arg = sector;
    if (!(host->flags & CARD_FLAG_SDHC))
    {
        cmd.arg <<= 9;
    }
    cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_ADTC;

    host->data.blksize = SECTOR_SIZE;
    host->data.blks  = blks;


    stop.cmd_code = STOP_TRANSMISSION;
    stop.arg = 0;
    stop.flags = RESP_SPI_R1B | RESP_R1B | CMD_AC;
    r_cmd = READ_MULTIPLE_BLOCK;
    w_cmd = WRITE_MULTIPLE_BLOCK;


    if (host->flags & 0x8000)
    {
        /* last request is WRITE,need check busy */
        card_busy_detect(host, 10000, NULL);
    }

    if (!dir) {
        cmd.cmd_code = r_cmd;
        host->data.flags |= DATA_DIR_READ;
        host->flags &= 0x7fff;
    } else {
        cmd.cmd_code = w_cmd;
        host->data.flags |= DATA_DIR_WRITE;
        host->flags |= 0x8000;
    }
    host->data.buf = buf;
    host->data.blks = blks;

    err = host->cmd(host, &cmd);
    if(err) {
        EMMC_PRINTF("%s start err", __FUNCTION__);
    }

    if(!dir) {
        host->read(host, buf);
    } else {
        host->write(host, buf);
    }

    if(host->complete) {
        host->complete(host);
    }

    os_sleep_ms(1);
    err = host->cmd(host, &stop);
    if(err) {
        EMMC_PRINTF("%s stop err", __FUNCTION__);
    }

    return RET_OK;
}


int mmcsd_req_blk(struct sdh_device *host,
                                 uint32_t           sector,
                                 void                 *buf,
                                 uint32_t             blks,
                                 uint8_t            dir)
{
    struct rt_mmcsd_cmd  cmd1, cmd2;
    uint32_t r_cmd, w_cmd;
    int err = 0;

    memset(&cmd1, 0, sizeof(struct rt_mmcsd_cmd));
    memset(&cmd2, 0, sizeof(struct rt_mmcsd_cmd));

    cmd1.arg = sector;
    if (!(host->flags & CARD_FLAG_SDHC))
    {
        cmd1.arg <<= 9;
    }
    cmd1.flags = RESP_SPI_R1 | RESP_R1 | CMD_ADTC;

    r_cmd = READ_MULTIPLE_BLOCK;
    w_cmd = WRITE_MULTIPLE_BLOCK;

    if (host->flags & 0x8000)
    {
        /* last request is WRITE,need check busy */
        card_busy_detect(host, 10000, NULL);
    }

    if (!dir) {
        cmd1.cmd_code = r_cmd;
        host->sd_opt  = SD_M_R;
        host->data.flags |= DATA_DIR_READ;
        host->flags &= 0x7fff;
    } else {
        cmd1.cmd_code = w_cmd;
        host->sd_opt  = SD_M_W;
        host->data.flags |= DATA_DIR_WRITE;
        host->flags |= 0x8000;
    }

    cmd2.cmd_code = SET_BLOCK_COUNT;
    cmd2.arg = blks;
    cmd2.flags = RESP_SPI_R1 | RESP_R1 | CMD_AC;

	if(host->cmd) {
		err |= host->cmd(host, &cmd2);
		err |= host->cmd(host, &cmd1);
	}

	
    if(err) {
        EMMC_PRINTF("%s start err", __FUNCTION__);
    }

    host->data.blksize = SECTOR_SIZE;
    host->data.buf = buf;
    host->data.blks = blks;

    if(!dir) {
        host->read(host, buf);
    } else {
        host->write(host, buf);
    }

    if(host->complete) {
        host->complete(host);
    }

    return RET_OK;
}

int mmc_cmdq_switch(struct sdh_device *host, bool enable)
{
	uint8_t val = enable ? 1 : 0;
	int err;

#define EXT_CSD_CMDQ_MODE_EN		15	/* R/W */

	err = mmc_switch(host, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_CMDQ_MODE_EN, val);
	if (err)
		printf("cmdq switch fail %d\r\n", enable);

	return err;
}



