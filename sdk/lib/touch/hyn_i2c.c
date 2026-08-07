#include "lib/touch/hyn_core.h"
#include "app_iic/app_iic.h"

int hyn_wr_reg(struct hyn_ts_data *ts_data, uint32_t reg_addr, uint32_t reg_len, uint8_t *rbuf, uint32_t rlen)
{
    uint8_t tmpbuf[80] = {0};
    uint32_t iic_dev = ts_data->app_iic_num;
    int ret = 0;
    tmpbuf[0] = reg_len;
    tmpbuf[1] = rlen;
    tmpbuf[2] = ts_data->iic_addr;
    // os_printf("hyn_wr_reg: ts_data->iic_addr:%x addr=0x%08X, len=%d, rbuf:0x%08x rbufrlen=%d\n",ts_data->iic_addr, reg_addr, reg_len, rbuf, rlen);

    for(uint32_t i = 0; i < reg_len; i++) {
        tmpbuf[3 + i] = (reg_addr >> (8 * (reg_len - 1 - i))) & 0xFF;
        // os_printf("reg_addr[%d]: 0x%02X\n", i, tmpbuf[3 + i]);
    }

    if(rlen > 0 && rbuf != NULL) {
        while(ret != 1){
            ret = wake_up_iic_queue(iic_dev,tmpbuf,0,0,ts_data->iic_trx_buff);
        }
        while(iic_devid_finish(iic_dev) != 1){
            os_sleep_ms(1);
        }
        os_memcpy(rbuf, ts_data->iic_trx_buff, rlen);
    } else {
        while(ret != 1){
            ret = wake_up_iic_queue(iic_dev,tmpbuf,0,1,(uint8_t*)NULL);
        }
        while(iic_devid_finish(iic_dev) != 1){
            os_sleep_ms(1);
        }
    }
    return RET_OK;
}