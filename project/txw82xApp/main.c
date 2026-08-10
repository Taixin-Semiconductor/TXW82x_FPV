#include "sys_config.h"
#include "basic_include.h"
#include "hal/adc.h"
#include "lib/rpc/cpurpc.h"
#include "lib/atcmd/libatcmd.h"
#include "lib/common/atcmd.h"
#include "lib/common/timezone.h"
#include "lib/umac/ieee80211.h"
#include "lib/lmac/lmac.h"
#include "lib/bluetooth/uble/ble_demo.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#include "lwip/tcpip.h"
#include "netif/ethernetif.h"

#include "lib/net/dhcpd/dhcpd.h"
#include "lib/net/utils.h"
#include "lib/net/skmonitor/skmonitor.h"
#include "syscfg.h"
#include "demo/user_app.h"
#include "lib/multimedia/msi.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/heap/av_heap.h"
#include "demo/app_device.h"


extern uint32 srampool_start;
extern uint32 srampool_end;
extern uint32 psrampool_start;
extern uint32 psrampool_end;

static struct os_work main_wk;
struct system_status  sys_status;

extern sysevt_hdl_res sys_event_hdl(uint32 event_id, uint32 data, uint32 priv);
extern void wifi_dev_status(uint32 dev_id);
extern void sys_atcmd_init(void);
extern void sys_wifi_test_mode_init(void);
extern void sys_wifi_init(void);
extern void sys_network_init(void);
extern void sys_dhcpd_start(void);
extern void sys_wifi_pair_init(void);
extern void sys_wifi_pair_led_init(void);
extern void sys_ble_init();

/****************************************************
 * @brief 初始化编码解码器,同时是否支持播放器
************************************************** */
static void Codec_init()
{
    uint8_t video_flag = 0;
    int32_t ret = -1;
#ifdef SUPPORT_DECODER_H264
    #include "decode/decode.h"
    extern int h264_dec_msi_init(void);
    ret &= decode_h264_attach(HG_H264_DEC_DEVID, 4);
    h264_dec_msi_init();
#endif

#ifdef SUPPORT_DECODER_JPEG
    #include "multimedia/image/coder/jpg_msi.h"
    #include "decode/decode.h"
    extern int decode_jpg_attach(uint32 dev_id, uint32 max_num);
    ret &= decode_jpg_attach(HG_JPEG_DEC_DEVID, 4);
    jpg_dec_msi_init();
#endif

#ifdef SUPPORT_ENCODER_JPEG
    #include "gen420/gen420_jpg_en.h"
    encode_jpg_attach(HG_JPEG_ENC_DEVID, 4);
#endif
    if (ret == RET_OK)
    {
        video_flag = 1;
    }

    ret = -1;

#if AAC_DEC_CTRL
    extern int aac_dec_msi_init(void);
    ret &= hgacodec_v1_attach(HG_AAC_DEC_DEVID, &aacdec);
    aac_dec_msi_init();
#endif
#if AAC_ENC_CTRL
    extern int aac_enc_msi_init(void);
    ret &= hgacodec_v1_attach(HG_AAC_ENC_DEVID, &aacenc);
    aac_enc_msi_init();
#endif
#if ALAW_DEC_CTRL
    extern int alaw_dec_msi_init(void);
    ret &= hgacodec_v1_attach(HG_ALAW_DEC_DEVID, &alawdec);
    alaw_dec_msi_init();
#endif
#if ALAW_ENC_CTRL
    extern int alaw_enc_msi_init(void);
    ret &= hgacodec_v1_attach(HG_ALAW_ENC_DEVID, &alawenc);
    alaw_enc_msi_init();
#endif
#if AMRNB_DEC_CTRL
    extern int amrnb_dec_msi_init(void);
    ret &= hgacodec_v1_attach(HG_AMRNB_DEC_DEVID, &amrnbdec);
    amrnb_dec_msi_init();
#endif
#if AMRWB_DEC_CTRL
    extern int amrwb_dec_msi_init(void);
    ret &= hgacodec_v1_attach(HG_AMRWB_DEC_DEVID, &amrwbdec);
    amrwb_dec_msi_init();
#endif
#if MP3_DEC_CTRL
    extern int mp3_dec_msi_init(void);
    ret &= hgacodec_v1_attach(HG_MP3_DEC_DEVID, &mp3dec);
    mp3_dec_msi_init();
#endif
#if OPUS_DEC_CTRL
    extern int opus_dec_msi_init(void);
    ret &= hgacodec_v1_attach(HG_OPUS_DEC_DEVID, &opusdec);
    opus_dec_msi_init();
#endif
#if OPUS_ENC_CTRL
    extern int opus_enc_msi_init(void);
    ret &= hgacodec_v1_attach(HG_OPUS_ENC_DEVID, &opusenc);
    opus_enc_msi_init();
#endif
#if ULAW_DEC_CTRL
    extern int ulaw_dec_msi_init(void);
    ret &= hgacodec_v1_attach(HG_ULAW_DEC_DEVID, &ulawdec);
    ulaw_dec_msi_init();
#endif
#if ULAW_ENC_CTRL
    extern int ulaw_enc_msi_init(void);
    ret &= hgacodec_v1_attach(HG_ULAW_ENC_DEVID, &ulawenc);
    ulaw_enc_msi_init();
#endif
#if PCM_DEC_CTRL
    extern int pcm_dec_msi_init(void);
    ret &= hgacodec_v1_attach(HG_PCM_DEC_DEVID, &pcmdec);
    pcm_dec_msi_init();
#endif
    if (ret == RET_OK)
    {
        extern int32 audio_coder_msi_init(uint32 priority, void *stack, uint16 stack_size);
        audio_coder_msi_init(0, 0, 4096);
    }

#ifdef SUPPORT_LCD
    #include "app_lcd/lcd_virtual.h"
    void lcd_virtual_msi_init(void);
    vdd_attach(HG_LCD_VIRTUAL_DEVID);
    lcd_virtual_msi_init();
#endif

#ifdef SUPPORT_TXMPLAYER
    #include "lib/multimedia/txmplayer.h"
    txmplayer_init(0, 0, NULL);
    if(video_flag)
    {
        extern int32 vdec_wkq_init(uint32 priority, void *stack, uint16 stack_size);
        vdec_wkq_init(0, 0, 1024);
    }
#endif
}

static __init void sys_cpurpc_init()
{
    cpu_rpc_init(CPU0_MSGBOX_BASE, CPU_RECV_MAIL_IRQn, CPU_SEND_MAIL_IRQn);
}

__init static void sys_cfg_load(void)
{
    if (syscfg_init("syscfg", &sys_cfgs, sizeof(sys_cfgs)) == RET_OK) {
        return;
    }

    os_printf("use default params.\r\n");
    syscfg_default();
    syscfg_save();
}

static void sys_print_dbgtime(uint32 *buff, uint32 size)
{
#if SYS_APP_SNTP
    time_t utc = time(NULL);
    time_t local;
    timezone_utc_to_local(utc, &local);
    os_printf("system time: %s (%d) sizeof(time_t)=%d\r\n", ctime((const time_t *)&local), utc, sizeof(time_t));
    struct tm t;
    localtime_tz(time(NULL), &t);
    os_printf("local time: %d年%d月%d日 %d时%d分%d秒 \r\n", t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
#endif
}

static void sys_print_dbgcache(uint32 *buff, uint32 size)
{
    if (sys_status.dbg_cache) { //打印Cache命中率
        uint32 csi_miss = sys_csi_cache_static(sysctrl_get_cpu_id(), 1);
        uint32 cld_miss = sys_cld_cache_static(1);
        sys_psram_eff_static((cld_miss > 20) || (csi_miss > 30));
    }
}

static void sys_print_dbgtop(uint32 *buff, uint32 size)
{
    if (sys_status.dbg_top) {  //打印CPU使用率
        cpu_loading_print(sys_status.dbg_top == 2, (struct os_task_info *)buff, size / sizeof(struct os_task_info));
    } else {
        os_printf("cpu loading: %d%%\r\n", os_cpuloading());
    }
}

static void sys_print_dbgheap(uint32 *buff, uint32 size)
{
    if (sys_status.dbg_heap) { //打印Heap使用情况
        sysheap_status(&sram_heap, buff, size / 4, 0);
#ifdef PSRAM_HEAP
        sysheap_status(&psram_heap, buff, size / 4, 0);
#endif
    } else {
        os_printf("sram heap: total=%d, free=%d\r\n", sysheap_totalsize(&sram_heap), sysheap_freesize(&sram_heap));
#ifdef PSRAM_HEAP
        os_printf("psram heap: total=%d, free=%d\r\n", sysheap_totalsize(&psram_heap), sysheap_freesize(&psram_heap));
#endif
    }
}

static void sys_print_dbgumac(uint32 *buff, uint32 size)
{
    if (sys_status.dbg_umac) { //打印WIFI调试信息
        wifi_dev_status(HG_WIFI0_DEVID);
    }
}

static void sys_print_dbgnet(uint32 *buff, uint32 size)
{
    struct netif *nif;

    if (sys_status.dbg_net) {
        os_printf("-----------------------------------------------------\r\n");
        os_printf("Network Info:\r\n");
        nif = netif_find("w0");
        if (nif) {
            os_printf("  w0: (%s) "IPSTR"/"IPSTR"/"IPSTR"\r\n",
                      (sys_cfgs.dhcpc_en && sys_status.dhcpc_done) ? "DHCP" : "Static",
                      IP2STR_N(ip_addr_get_ip4_u32(&nif->ip_addr)),
                      IP2STR_N(ip_addr_get_ip4_u32(&nif->netmask)),
                      IP2STR_N(ip_addr_get_ip4_u32(&nif->gw)));
        }

        nif = netif_find("e0");
        if (nif) {
            os_printf("  e0: (%s) "IPSTR"/"IPSTR"/"IPSTR"\r\n", sys_cfgs.dhcpc_en ? "DHCP" : "Static",
                      IP2STR_N(ip_addr_get_ip4_u32(&nif->ip_addr)),
                      IP2STR_N(ip_addr_get_ip4_u32(&nif->netmask)),
                      IP2STR_N(ip_addr_get_ip4_u32(&nif->gw)));
        }

#if IP_NAT
        os_printf("-----------------------------------------------------\r\n");
        ip4_nat_status();
#endif

        if (sys_cfgs.dhcpd_en) {
            os_printf("-----------------------------------------------------\r\n");
            dhcpd_dump_ippool();
        }

    }
}

static void sys_dbginfo_print(void)
{
    static int8 print_interval = 0;
#if 0
    static uint32 _print_buf[256];
#else
    uint32 _print_buf[256];
    ASSERT(sizeof(_print_buf) < 1600); //使用task堆栈，避免堆栈溢出
#endif

    if (print_interval++ >= 5) { // 5秒打印一次
        sys_print_dbgcache(_print_buf, sizeof(_print_buf));
        sys_print_dbgtop(_print_buf, sizeof(_print_buf));
        sys_print_dbgheap(_print_buf, sizeof(_print_buf));
        sys_print_dbgumac(_print_buf, sizeof(_print_buf));
        sys_print_dbgtime(_print_buf, sizeof(_print_buf));
        sys_print_dbgnet(_print_buf, sizeof(_print_buf));
        print_interval = 0;
    }
}

__init static void sys_heap_info()
{
    /*打印各个heap区间信息*/
    os_printf("------------------------------------------------------------\r\n");
    os_printf("System Heaps Info\r\n");
    os_printf("| CPU0 SRAM  HEAP : %p ~ %p, Size:%-8d |\r\n", SYS_HEAP_START, SYS_HEAP_START + SYS_HEAP_SIZE, SYS_HEAP_SIZE);
    os_printf("| CPU0 PSRAM HEAP : %p ~ %p, Size:%-8d |\r\n", SYS_PSRAM_HEAP_START, SYS_PSRAM_HEAP_START + SYS_PSRAM_HEAP_SIZE, SYS_PSRAM_HEAP_SIZE);
    os_printf("|----------------------------------------------------------|\r\n");
    os_printf("| CPU1 HEAP   : %p ~ %p, Size:%-8d     |\r\n", CoreSetting->heap_addr, CoreSetting->heap_addr + CONFIG_CORE_HEAP_SIZE, CONFIG_CORE_HEAP_SIZE);
    os_printf("| CPU1 RXBUF  : %p ~ %p, Size:%-8d     |\r\n", CoreSetting->rxbuf_addr, CoreSetting->rxbuf_addr + CONFIG_CORE_RXBUF_SIZE, CONFIG_CORE_RXBUF_SIZE);
    os_printf("| CPU1 SKBPOOL: %p ~ %p, Size:%-8d     |\r\n", CoreSetting->skbpool_addr, CoreSetting->skbpool_addr + CONFIG_CORE_SKB_POOL_SIZE, CONFIG_CORE_SKB_POOL_SIZE);
    //os_printf("| CPU0 AVHEAP : %p ~ %p, Size:%-8d     |\r\n", CONFIG_AVHEAP_START, CONFIG_AVHEAP_START+CONFIG_AVHEAP_SIZE, CONFIG_AVHEAP_SIZE);
    os_printf("------------------------------------------------------------\r\n");
}

void av_heap_print()
{
    uint32_t s_buf[256];
    sysheap_status(&av_psram_heap, s_buf, sizeof(s_buf) / 4, 0);
    sysheap_status(&av_heap, s_buf, sizeof(s_buf) / 4, 0);
    sysheap_status(&psram_heap, s_buf, sizeof(s_buf) / 4, 0);
    sysheap_status(&sram_heap, s_buf, sizeof(s_buf) / 4, 0);
}

static int32 sys_main_loop(struct os_work *work)
{
    mcu_watchdog_feed();
    sys_dbginfo_print();
    //av_heap_print();

    /*run again after 1000 ms.*/
    os_run_work_delay(&main_wk, 1000);
    return 0;
}

__init static void sys_vfs_init(void)
{
#if FS_EN
    vfs_init();
#endif
}

__init static void sys_app_init(void)
{
#if SYS_APP_DHCPD && SYS_NETWORK_SUPPORT
    sys_dhcpd_start();
#endif

#if SYS_APP_SNTP && SYS_NETWORK_SUPPORT
    timezone_set_preset(TZ_CST);   // 中国标准时间
    sntp_client_init("ntp.aliyun.com", 2);
#endif

#if SYS_APP_BLENC
    sys_ble_netconfig_init();
#endif

#ifdef IPC_720P_DEMO
    ipc_720p_demo_init();
#endif

#ifdef IPC_1080P_DEMO
    ipc_1080p_demo_init();
#endif

#ifdef AI_DIALOGUE_DEMO
    ai_dialogue_demo_init();
#endif

#ifdef AI_ALARM_CLOCK_DEMO
    ai_alarm_clock_demo_init();
#endif

#ifdef IPC_720P_SLEEP_DEMO
    app_sleep_720p_demo_init();
#endif

#ifdef BATTERY_CAMERA_1080P_DEMO
    app_battery_camera_1080p_demo_init();
#endif

#ifdef LCD_720P_DEMO
    app_lcd_720p_demo_init();
#endif

#ifdef ISP_TUNING_DEMO
    app_isp_tuning_demo_init();
#endif

}

__init static void usr_app_init(void)
{
    /*
       添加用户App代码初始化
    */
}

static int32 watchdog_loop(struct os_work *work)
{
    mcu_watchdog_feed();
    os_run_work_delay(&main_wk, 1000);
    return 0;
}

int main(void)
{
    mcu_watchdog_timeout(0); //打开或关闭MCU看门狗
    sys_cpurpc_init();
    sys_heap_info();
    sys_cfg_load();
    sys_event_init(32);
    sys_atcmd_init();
    msi_core_init();
    Codec_init();

    if (system_is_wifi_test_mode()) { // enter wifi test mode
        sys_wifi_test_mode_init();
        system_reboot_normal_mode();
        OS_WORK_INIT(&main_wk, watchdog_loop, 0);
        os_run_work_delay(&main_wk, 1000);
    } else { // normal mode
        sys_wifi_init();
        sys_ble_init();
        sys_network_init();
        do_global_ctors();
        sys_vfs_init();
        sys_app_init();
        usr_app_init();
        sys_event_take(0xffffffff, sys_event_hdl, 0);
        OS_WORK_INIT(&main_wk, sys_main_loop, 0);
        os_run_work_delay(&main_wk, 1000);
    }
    pmu_watchdog_timeout(16); //打开或关闭PMU看门狗, 放在sys_wifi_init之后！！
    return 0;
}

