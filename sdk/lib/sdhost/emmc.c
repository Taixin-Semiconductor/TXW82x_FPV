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
#include "lib/sdhost/mmc_ops.h"
#include "hal/gpio.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "osal/task.h"
#include "osal/sleep.h"
#include "osal/timer.h"
#include "osal/work.h"

extern void sdhost_io_func_init(uint32 req);
extern uint32 sd_power_up(struct sdh_device *host,uint8 bus_w);
extern void sd_set_clk(struct sdh_device * host,uint32 clk);
#if 1
#define EMMC_PRINTF(fmt, arg...) printf(fmt, ##arg)
#else
#define EMMC_PRINTF(fmt, arg...)
#endif



uint32_t send_op_cond(struct sdh_device *host,
                            uint32_t           ocr,
                            uint32_t          *rocr)
{
    struct rt_mmcsd_cmd cmd;
    int ret;
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    cmd.cmd_code = SEND_OP_COND;
    cmd.arg = ocr;
    cmd.flags = RESP_SPI_R1 | RESP_R3 | CMD_BCR;

    ret = host->cmd(host, &cmd);
    if(ret){
        EMMC_PRINTF("cmd%d err\r\n", cmd.cmd_code);
    }
    EMMC_PRINTF("cmd resp:%x\r\n",cmd.resp[0]);

    if(rocr)
        *rocr = cmd.resp[0];

    return ret;
}




int emmc_init(struct sdh_device * host, uint32 clk)
{
    uint32_t ocr;
    uint8_t  bw  = 1;
	uint8_t *ext_csd = NULL;
    
    if(host->open)
        host->open(host, 1, SD_MODE_TYPE);
    
    host->flags |= MMCSD_BUSWIDTH_4;
	os_printf("host->flags:%x\r\n",host->flags);
    sdhost_io_func_init(host->flags);
    

    if(bw == 4)
        sd_power_up(host, MMCSD_BUSWIDTH_4);
    else
        sd_power_up(host,0);

    sd_set_clk(host, 400*1000);

    send_idle(host);

    send_op_cond(host, 0x40FF8080, &ocr);
    while(!(ocr & 0x80000000)) 
    {
        os_sleep_ms(5);
        send_op_cond(host, 0x40FF8080, &ocr);
    }

    send_all_get_cid(host, host->resp_cid);

    EMMC_PRINTF("cid0: 0x%x  cid1: 0x%x\r\n", host->resp_cid[0], host->resp_cid[1]);

    host->rca = 1;
    mmc_set_card_addr(host, host->rca);

    EMMC_PRINTF("rca: 0x%x\r\n", host->rca);

   //解析csd (to do)
    send_get_csd(host, host->resp_csd);
    host->card_blksize = 1 << GET_BITS(host->resp_csd, 80, 4);
    EMMC_PRINTF("emmc spec verision: 0x%x\r\n", GET_BITS(host->resp_csd, 122, 4));
    EMMC_PRINTF("max clock frequency %d\r\n", GET_BITS(host->resp_csd, 96, 3) * GET_BITS(host->resp_csd, 99, 4));
    EMMC_PRINTF("block len: %d\r\n", host->card_blksize);
    
   //select card
    send_select_card(host);

   //解析ext_csd (to do)
    ext_csd = os_malloc(512);
    if(!ext_csd) {
        EMMC_PRINTF("ext_csd malloc fail\r\n");
    }

    mmc_get_ext_csd(host, ext_csd);

	mmc_parse_ext_csd(host, ext_csd);
   
   //切换bus width
//    mmc_select_bus_width(host, ext_csd);

   //配置驱动能力


   //设置频率 & 采样调谐
#ifndef FPGA_SUPPORT

#endif

    sd_set_clk(host, clk);
    
    host->cardflags |= CARD_FLAG_SDHC;

    os_free(ext_csd);
	EMMC_PRINTF("emmc init done\r\n");
    return 0;
}
