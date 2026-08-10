#include "basic_include.h"
#include "hal/netdev.h"
#include "lib/net/utils.h"

__weak void sys_wakeup_host(void)
{

}

/*
  wkq : 当前运行work的workqueue
  work: 当前被执行的work
  start: [1: work即将开始运行 ]  
         [0：work运行结束 ]
  runtime： work的运行时间
*/
__weak void os_work_schedule_hook(struct os_workqueue *wkq, struct os_work *work, uint8 start, uint32 runtime)
{

}

/*************** Some Weak Functions ***************************/
///////////////////////////////////////////////////////////////////////////////////
__weak int32 sys_register_sleepcb(sys_sleepcb cb, void *priv)
{
    return RET_OK;
}

__weak int32 wifi_proc_drvcmd_cust(uint16 cmd_id, uint8 *data, uint32 len, void *hdr)
{
    /*
       cmd_id: driver cmd id (from host driver).
       data  : cmd data.
       len   : cmd data length.
       hdr   : cmd info, just used to call host_cmd_resp API.

       1. parse cmd data, do some thing if you need.

       2. send cmd response to host:
          // has no additional response data.
          host_cmd_resp(cmd_return_value, NULL, 0, hdr);

          // has some additional response data.
          host_cmd_resp(cmd_return_value, reponse_data, response_data_length, hdr);

       3. return RET_OK if the driver cmd has been processed, else return -ENOTSUPP.

       you can process any driver cmd in this function.
    */
    return -ENOTSUPP;
}

__weak int32 sswitch_dev_rule_check(SSWITCH_RULE_CHECK type, uint8 *data, uint32 len, uint32 port)
{
#if 0
    /*
    *              ——————————————————————
    *              | local system(lwip) |
    *              ——————————————————————
    *               ↑                  ↓
    *       <local_input>        <local_output>
    *             ↑                       |
    *       <port_input> ——> <forward>    |
    *             ↑                ↓      ↓
    * ————> [port dev]            [port dev] ————>
    *
    */

    scatter_data *sdata;

    switch (type) {
        case SSWITCH_RULE_CHECK_PORT_INPUT:  //从某个端口接收数据
            os_printf("Port_%d input data ["MACSTR"<->"MACSTR", proto:%x], len=%d\r\n",
                      (port & 0xff), MAC2STR(data), MAC2STR(data + 6), get_unaligned_be16(data + 12), len);
            break;
        case SSWITCH_RULE_CHECK_LOCAL_INPUT: //接收的数据被输入到本地
            os_printf("Local input data ["MACSTR"<->"MACSTR", proto:%x] len=%d, source port:%d\r\n",
                      MAC2STR(data), MAC2STR(data + 6), get_unaligned_be16(data + 12), len, (port & 0xff));
            break;
        case SSWITCH_RULE_CHECK_FORWARD:    //接收的数据数据被转发
            os_printf("Forward input data ["MACSTR"<->"MACSTR", proto:%, len=%d] Port_%d -> Port_%d\r\n",
                      MAC2STR(data), MAC2STR(data + 6), get_unaligned_be16(data + 12), len,
                      (port & 0xff)/*source port dev*/, (port >> 8) & 0xff /*dest port dev*/);
            break;
        case SSWITCH_RULE_CHECK_LOCAL_OUTPUT: //本地输出数据
            os_printf("Local output data ["MACSTR"<->"MACSTR", proto:%x, len=%d] To Port_%d\r\n",
                      MAC2STR(data), MAC2STR(data + 6), get_unaligned_be16(data + 12), len, (port >> 8) & 0xff);
            break;
        case SSWITCH_RULE_CHECK_LOCAL_OUTPUT_SCATTER: //本地输出数据(scatter类型)
            sdata = (scatter_data *)data;
            os_printf("Local output scatter data ["MACSTR"<->"MACSTR", proto:%x, len=%d] To Port_%d\r\n",
                      MAC2STR(sdata[0].addr), MAC2STR(sdata[0].addr + 6),
                      get_unaligned_be16(sdata[0].addr + 12), scatter_size(sdata, len), (port >> 8) & 0xff);
            break;
    }
#endif
    return 1; //1:pass, 0:drop
}


