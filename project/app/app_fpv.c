#include "sys_config.h"
#include "basic_include.h"
#include "lib/common/atcmd.h"
#include "lib/net/eloop/eloop.h"
#include "lib/video/isp/isp_dev.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "lib/video/mipi_csi/mipi_csi.h"
#include "lib/video/h264/h264_drv.h"
#include "lib/video/vpp/vpp_dev.h"
#include "lib/video/para_in/para_in_dev.h"
#include "lib/multimedia/msi.h"
#include "stream_define.h"
#include "syscfg.h"

#include "app_lcd/app_lcd.h"
#include "lib/net/dhcpd/dhcpd.h"

#include "audio_msi/audio_adc.h"
#include "audio_msi/audio_dac.h"
#include "intercom/intercom.h"
#include "lib/audio/audio_code/audio_code.h"
#include "lib/audio/audio_proc/audio_proc.h"
#include "lib/audio/wsola/wsola_process.h"

#if RTT_USB_EN
#include "rtthread.h"
#endif
#include "lib/heap/av_psram_heap.h"
#include "lib/heap/av_heap.h"
#include "app/spook/spook.h"
#include "app/recorder/recorder_viidure.h"
#include "app/app_iic/app_iic.h"
#include "ota.h"

#include "cjson/cJSON.h"
#include "scale_msi/scale_msi.h"
#include "lib/touch/touch_pad.h"
#include "gen420_hardware_msi.h"
#include "lib/sdhost/sdhost.h"
#include "hg_lv_mem.h"
#include "keyWork.h"
#include "video_app/video_msi.h"

#include "lib/lvgl_rotate_rpc/lvgl_rotate_msi.h"
#include "debug_log_msi.h"
#include "log_save_msi.h"

int32 atcmd_recv(uint8 *data, int32 len);
void user_workqueue_init(uint16 pri,void *stack,uint16 stack_size);

extern uint32 psrampool_start;
extern uint32 psrampool_end;

extern uint32 srampool_start;
extern uint32 srampool_end;

struct fpv_status {
    uint32_t dbg_av_heap:1,
             dbg_av_psram_heap:1,
             dbg_sram_heap:1,
             dbg_psram_heap:1;
};

static struct fpv_status g_fpv_status;
//fpv工程增加AT命令
const void *r_heap[] = 
{
#if defined(MPOOL_ALLOC) && defined(AV_PSRAM_HEAP) && defined(PSRAM_HEAP)
    (void*)&av_psram_heap,
#endif

#if defined(MPOOL_ALLOC) && defined(AV_HEAP) && defined(PSRAM_HEAP)
    (void*)&av_heap,
#endif
    (void*)&sram_heap,
    (void*)&psram_heap,
    NULL,
};

int32 fpv_atcmd_check_heap(const char *cmd, char *argv[], uint32 argc)
{
    char *arg = argv[0];
    int i = 0;
    uint8_t mode;
    if(argc == 1)
    {
        mode = 0;
    }
    else
    {
        mode = os_atoi(argv[1]);
    }
    struct sys_heap *heap = (struct sys_heap *)r_heap[i];
    while(heap)
    {
        if (os_strcasecmp(arg, heap->name) == 0)
        {
            if(mode == 0)
            {
                os_printf(KERN_ALERT"%s heap free size:%d\n",heap->name, sysheap_freesize(heap));
            }
            else
            {
                uint32_t s_buf[256];
                sysheap_status(heap, s_buf, sizeof(s_buf)/sizeof(uint32_t), 0);
            }
            
            break;
        }
        i++;
        heap = (struct sys_heap *)r_heap[i];
    }
    if(!heap)
    {
        os_printf(KERN_ALERT"no found heap %s\n",arg);
    }
    return 0;
}

int32 fpv_atcmd_dbg(const char *cmd, char *argv[], uint32 argc)
{
    char *arg = argv[0];
    if (argc == 2) {
        if (os_strcasecmp(arg, "av_sram") == 0) {
            g_fpv_status.dbg_av_heap = (os_atoi(argv[1]) == 1);
        }
        if (os_strcasecmp(arg, "av_psram") == 0) {
            g_fpv_status.dbg_av_psram_heap = os_atoi(argv[1]);
        }
        if (os_strcasecmp(arg, "sram") == 0) {
            g_fpv_status.dbg_sram_heap = (os_atoi(argv[1]) == 1);
        }
        if (os_strcasecmp(arg, "psram") == 0) {
            g_fpv_status.dbg_psram_heap = (os_atoi(argv[1]) == 1);
        }
        return 0;
    } else {
        return -1;
    }
    return 0;
}


// 用户自定义内存池初始化
static void user_heap_init()
{
#if defined(MPOOL_ALLOC) && defined(AV_PSRAM_HEAP) && defined(PSRAM_HEAP)
    {
        uint32 flags = SYSHEAP_FLAGS_MEM_ALIGN_32;
        os_printf("CONFIG_PSRAM_AVHEAP_START:%X\n", CONFIG_PSRAM_AVHEAP_START);
        os_printf("CONFIG_PSRAM_AVHEAP_SIZE:%X\n", CONFIG_PSRAM_AVHEAP_SIZE);
        av_psram_heap_init((void *)CONFIG_PSRAM_AVHEAP_START, CONFIG_PSRAM_AVHEAP_SIZE, flags);
    }
#endif

#if defined(MPOOL_ALLOC) && defined(AV_HEAP)
    {
        uint32 flags = SYSHEAP_FLAGS_MEM_ALIGN_32;
        os_printf("CONFIG_AVHEAP_START:%X\n", CONFIG_AVHEAP_START);
        os_printf("CONFIG_AVHEAP_SIZE:%X\n", CONFIG_AVHEAP_SIZE);
        av_heap_init((void *)CONFIG_AVHEAP_START, CONFIG_AVHEAP_SIZE, flags);
    }
#endif
}

__weak void user_protocol()
{
    spook_init();
    config_Viidure(80);
}

// 应用程序初始化
__init static void fpv_app_init(void)
{
    uint8_t takephoto_from = 0;
#ifdef PSRAM_HEAP
    cJSON_Hooks hook;
    hook.malloc_fn = _os_malloc_psram;
    hook.free_fn = _os_free_psram;
    cJSON_InitHooks(&hook);
#endif


    eloop_init();
    os_task_create("eloop_run", user_eloop_run, NULL, OS_TASK_PRIORITY_NORMAL + 2, 0, NULL, 2048);
    os_sleep_ms(1);
    ota_Tcp_Server();

    //独立的文件保存msi(独立线程,后续可以所有的fb需要保存都发到这个msi去执行)
	extern struct msi *file_msi_init(const char *msi_name);
    file_msi_init(R_FILE_MSI);
#ifndef FORCE_SCALE_TO_H264
    #if H264_EN == 1
		extern struct msi *auto_h264_msi_init(const char *auto_h264_name, uint8_t src_from0, uint16_t w0, uint16_t h0, uint8_t src_from1, uint16_t w1, uint16_t h1);
        #if SUB_STREAM_EN == 1
            auto_h264_msi_init(AUTO_H264,VPP_DATA0,0,0,GEN420_DATA,640,360);
        #else
            auto_h264_msi_init(AUTO_H264,VPP_DATA0,0,0,~0,0,0);
        #endif
    #endif
#else
    uint8_t get_vpp_scale_w_h(uint16_t *w, uint16_t *h);
    uint16_t scale_w,scale_h;
    int scale_ret = get_vpp_scale_w_h(&scale_w,&scale_h);
    if(!scale_ret)
    {
        os_printf(KERN_INFO"scale_w:%d\tscale_h:%d\n",scale_w,scale_h);
        auto_h264_msi_init(AUTO_H264,SCALER_DATA,scale_w,scale_h,GEN420_DATA,640,360);
    }
    else
    {
        os_printf(KERN_ERR"scale cfg err,scale_ret:%d\n",scale_ret);
    }
    
#endif
    
    //支持拍照和缩略图,暂时默认启动,(紧紧支持录风者模式)
    #if TAKEPHOTO_EN
    // 正常拍照和缩略图
    {
		extern uint8_t get_vpp_w_h(uint16_t *w, uint16_t *h);
		extern void takephoto_with_thumb_init(const char *thumb_msi_name);
		extern void takephoto_with_thumb_over_dpi_init(const char *thumb_msi_name);
		extern void takephoto_720P_with_thumb_init(const char *thumb_msi_name);
        uint16_t camera_w,camera_h;
        get_vpp_w_h(&camera_w,&camera_h);
        //没有识别到摄像头
        if(!camera_w || !camera_h)
        {   

        }
        //如果是720P摄像头,支持原分辨率以及大分辨拍照
        else if(camera_w == 1280 && camera_h == 720)
        {
            takephoto_with_thumb_init(R_THUMB);
            takephoto_with_thumb_over_dpi_init(SR_OVER_DPI_THUMB_JPG);
            takephoto_from = VPP_DATA0;
        }
        //其他摄像头,主要是为了拍照720P的图片,1080P摄像头从VPP_DATA1,不支持大分辨率拍照
        else
        {
            takephoto_with_thumb_init(R_THUMB);
            takephoto_from = VPP_DATA1;
        }
    }
    #endif

    #if JPG_EN == 1 
    auto_jpg_msi_init(AUTO_JPG,JPGID0,takephoto_from);
    #endif

    #if USB_JPG_ADD_WATERMARK
    usb_to_recode_init();
    #endif
    /*
        添加其他应用代码初始化
        ...
    */

    #if ISP_TUNNING_EN
    void isp_tunning_init(uint32 img_w, uint32 img_h);
    isp_tunning_init(1920, 1080);
    #endif
    user_protocol();
}

__weak void user_hardware_config()
{
}

uint8_t vcam_en(void)
{
    uint8_t ret = TRUE;
#if VCAM_EN
    pmu_vcam_dis();
    os_sleep_ms(1);
    pmu_set_vcam_vol(VCAM_VOL_2V80);
    pmu_vcam_oc_set(VCAM_OC_200MA);
    pmu_vcam_lc_en();
    pmu_vcam_oc_int_dis();
    pmu_vcam_discharge_dis();
    pmu_vcam_pg_dis();
#ifdef VCAM_33
    pmu_set_vcam_vol(VCAM_VOL_3V25);
    pmu_vcam_en();
    os_sleep_ms(1);
    pmu_vcam_pg_en();
#else
    pmu_vcam_en();
    os_sleep_ms(1);
#endif

    pmu_vcam_oc_pending_clr();

    pmu_vcam_oc_int_dis();
    pmu_lvd_oe_en();
#endif
    return ret;
}

extern void scale2_mutex_init();
void hardware_init(uint8_t vcam)
{
	
    void eff_stop();
    eff_stop();
    iic_thread_init();
    sensor_info_init();
	scale2_mutex_init();	
#if KEY_MODULE_EN == 1
    keyWork_init(10);
#endif

#if JPG_EN == 1 
	extern int32 jpg_mutex_init();
    jpg_mutex_init();
#endif
    //默认打开gen420的模块
    gen420_hardware_msi_init();

#if SDH_EN && FS_EN
    extern bool fatfs_register();
    sd_open();
    fatfs_register();
    file_ota();
#endif

#if DEBUG_LOG_FILE_SAVE_EN
    //默认写入到SD卡，不支持中断打印保存
    //默认每10秒同步写卡，每60秒新建一个文件保存
    creat_log_save_stream(R_DEBUG_STREAM, 1000, 60*1000);
    struct msi *dbg_msi = dbg_log_msi(S_DEBUG_STREAM);
    if(dbg_msi)
    {
        print_redirect((osprint_hook)hgprntf_debug_log_msi, dbg_msi->priv, 1);
    }
    
#endif

#if DVP_EN
    bool csi_ret = 0;
    bool csi_cfg();
    csi_ret = csi_cfg();
#endif

#if DVP_EN
    bool csi_open();
    if (csi_ret)
        csi_open();
#endif

#if MIPI_CSI_EN
    struct mipi_csi_debug mipi_debug;
    os_memset(&mipi_debug, 0, sizeof(struct mipi_csi_debug));
    mipi_debug.debug_enable = 0;
    mipi_debug.debug_io0    = PD_4;
    mipi_debug.debug_io1    = PD_5;
    mipi_debug.debug_io2    = PD_6;
    mipi_debug.debug_io3    = PD_7;
    mipi_debug.debug_type0  = 6;
    mipi_debug.debug_type1  = 7;
    mipi_debug.debug_type2  = 8;
    mipi_debug.debug_type3  = 9;
    int mipi_csi_hardware_config(uint32_t csi_dev_id, uint8_t init_en, uint8_t csi_data_lane_num, uint8_t dual_en, uint8_t dual_sensor_type, uint8_t mclk,struct mipi_csi_debug *p_debug);
#if DVP_EN
	int ret;
    ret = mipi_csi_hardware_config(HG_MIPI_CSI_DEVID,  1, 1, DUAL_EN, SENSOR_TYPE_SLAVE0,24, &mipi_debug);
    mipi_csi_hardware_config(HG_MIPI1_CSI_DEVID, 1, 1, DUAL_EN, ret?SENSOR_TYPE_SLAVE1:SENSOR_TYPE_SLAVE0,24, &mipi_debug);
#else
    mipi_csi_hardware_config(HG_MIPI_CSI_DEVID, 1, 1, DUAL_EN, SENSOR_TYPE_MASTER, 24, &mipi_debug);
    //mipi_csi_hardware_config(HG_MIPI1_CSI_DEVID, 1, 1, DUAL_EN, SENSOR_TYPE_SLAVE0, 24, &mipi_debug);
#endif
#endif

#if PARA_IN_EN
	para_in_hareware_init();
#endif

#if ISP_EN
    //  debug_config();
    isp_cfg_dev();
#endif
#if VPP_EN
{
	extern void vpp_evt_init();
    extern uint8_t set_vpp_bu1_shrink(uint16_t w, uint16_t shrink_w);
	extern void get_single_mipi(uint32_t csi_dev_id,uint16_t *w,uint16_t *h);
    vpp_evt_init();
    uint16_t w = 0,h = 0;
    get_single_mipi(HG_MIPI_CSI_DEVID,&w,&h);
    os_printf(KERN_INFO"vpp_cfg w:%d h:%d\n",w,h);
    #ifdef SUB_STREAM_WIDTH
    set_vpp_bu1_shrink(w,SUB_STREAM_WIDTH);
    #endif
    vpp_cfg(w, h, VPP_INPUT_FROM);
}

#endif
#if LCD_EN

    uint16_t osd_w, osd_h;
    uint16_t screen_w, screen_h;
	uint16_t video_w, video_h;
    uint8_t rotate, video_rotate;
    lcd_hardware_init(&osd_w, &osd_h, &rotate, &screen_w, &screen_h,&video_w, &video_h, &video_rotate);
    lcd_arg_setting(osd_w, osd_h, rotate, screen_w, screen_h,video_w,video_h, video_rotate);
    lcd_driver_init(R_OSD_ENCODE, R_LCD_OSD, R_VIDEO_P0, R_VIDEO_P1);


#endif

#if TOUCH_PAD_EN
    touch_pad_hareware_init();
#endif

#if DUAL_EN
	void dorg_double_sensor(uint32 src0_w,uint32 src0_h,uint32 src1_w,uint32 src1_h,uint32 src0_raw_num,uint32 src1_raw_num,uint8_t dvp_type,uint8_t csi0_type,uint8_t csi1_type);
    dorg_double_sensor(1280, 720, 1280, 720, RAW8, RAW10,1,2,3);
#endif

#if AUDIO_EN
	reg_auproc_alloc(av_psram_malloc, av_psram_zalloc, av_psram_calloc, av_psram_realloc, av_psram_free);
	reg_wsola_alloc(av_psram_malloc, av_psram_zalloc, av_psram_calloc, av_psram_realloc, av_psram_free);
    reg_aucoder_alloc(av_psram_malloc, av_psram_zalloc, av_psram_calloc, av_psram_realloc, av_psram_free);
    aucode_mutex_init();
    audio_adc_init();
    audio_dac_init();
#endif

#if RTT_USB_EN
#ifdef RT_USING_USB_DEVICE
    extern rt_err_t rt_usbd_core_device_list_init();
    rt_usbd_core_device_list_init();
#endif
#endif

#if (USB11_EN && RTT_USB_EN)
#if USB11_HOST_EN
    extern rt_err_t hg_usb11h_register(rt_uint32_t devid);
    hg_usb11h_register(HG_USB11_HOST_CONTROLLER_DEVID);
#else
    extern void hg_usb11d_class_driver_register();
    extern int hg_usb11d_register(rt_uint32_t devid);
    hg_usb11d_class_driver_register();
    hg_usb11d_register(HG_USB11_DEV_CONTROLLER_DEVID);
#endif
#endif

#if (USB_EN && RTT_USB_EN)
#if (USB_DETECT_EN && !USB_HOST_EN)
    extern void hg_usb_connect_detect_init(void);
    hg_usb_connect_detect_init();
#else
#if USB_HOST_EN
    extern rt_err_t hg_usbh_register(rt_uint32_t devid);
    hg_usbh_register(HG_USB_HOST_CONTROLLER_DEVID);
#else
    extern void hg_usbd_class_driver_register();
    extern int hg_usbd_register(rt_uint32_t devid);
    hg_usbd_class_driver_register();
    hg_usbd_register(HG_USB_DEV_CONTROLLER_DEVID);
#endif
#endif
#endif
#if LCD_EN
    void lcd_demo_thread(void *d);
    void lvgl_init_msi(uint16_t w, uint16_t h, uint8_t rotate);

    #if LVGL_HW_ROTATE_RPC_EN
    struct hg_lv_mem_hooks hook = {
        .malloc  = _os_malloc,
        .realloc = _os_realloc,
        .zalloc  = _os_zalloc,
        .free    = _os_free,
    };
    #else
	struct hg_lv_mem_hooks hook = {
        .malloc  = av_psram_malloc,
        .realloc = av_psram_realloc,
        .zalloc  = av_psram_zalloc,
        .free    = av_psram_free,
    };
    #endif
    hg_lv_mem_register(&hook);
    
    lvgl_init_msi(osd_w, osd_h, rotate);
    // lcd_demo_thread(2);
#endif
    user_hardware_config();
}

static struct os_work fpv_wk;
extern void print_status(uint32_t *s_buf, uint32_t size);
static int32 sys_fpv_loop(struct os_work *work)
{
    uint32_t s_buf[256];
    if(g_fpv_status.dbg_sram_heap)
    {
        sysheap_status(&sram_heap, s_buf, sizeof(s_buf) / 4, 0);
    }
    if(g_fpv_status.dbg_psram_heap)
    {
        sysheap_status(&psram_heap, s_buf, sizeof(s_buf) / 4, 0);
    }
    
#if defined(MPOOL_ALLOC) && defined(AV_PSRAM_HEAP) && defined(PSRAM_HEAP)
    if(g_fpv_status.dbg_av_psram_heap)
    {
        sysheap_status(&av_psram_heap, s_buf, sizeof(s_buf) / 4, 0);
    }
#endif

#if defined(MPOOL_ALLOC) && defined(AV_HEAP)
    if(g_fpv_status.dbg_av_heap)
    {
        sysheap_status(&av_heap, s_buf, sizeof(s_buf) / 4, 0);
    }
#endif

    os_run_work_delay(work, 1000);
    return 0;
}

//部分控制at命令
void fpv_at_dbg()
{
    //是否打开对应sdk的dbg
    atcmd_recv((uint8_t*)"at+print=1,7",0);
    atcmd_recv((uint8_t*)"AT+FPV_DBG=av_psram,0",0);
    atcmd_recv((uint8_t*)"AT+FPV_DBG=av_sram,0",0);
    atcmd_recv((uint8_t*)"AT+FPV_DBG=sram,0",0);
    atcmd_recv((uint8_t*)"AT+FPV_DBG=psram,0",0);
    atcmd_recv((uint8_t*)"AT+SYSDBG=top,0",0);

    #if 0
    //一次性命令
    atcmd_recv("AT+FPV_HEAP=av_psram,0",0);
    atcmd_recv("AT+FPV_HEAP=av_sram,0",0);
    atcmd_recv("AT+FPV_HEAP=sram,0",0);
    atcmd_recv("AT+FPV_HEAP=psram,0",0);
    #endif
    
}


/**********************************************************************
 * print_level设置打印的等级,7是将所有打印都打开(调试的时候可以打开)
 * 例子:对应不同等级参考string.h
 *      os_printf(KERN_DEBUG"ABC"); //等级7
 *      os_printf(KERN_EMERG"ABC"); //等级1
 * disable_print_color 是否关闭打印颜色(特定串口工具)
*******************************************************************/
int sys_app_fpv_init(void)
{
    print_level(7); 
    disable_print_color(1);
    uint8_t vcam;
    user_heap_init();
    vcam = vcam_en();
    pmu_vcam2_ldo_en(1, VCC_LDO_VOL_1V80);
    msi_core_init();
    //初始化fpv应用用的workqueue,注意这个workqueu是应用,尽量不要执行过长时间
    user_workqueue_init(OS_TASK_PRIORITY_HIGH,NULL,2048);
    hardware_init(vcam);
    fpv_app_init();
    fpv_at_dbg();
    OS_WORK_INIT(&fpv_wk, sys_fpv_loop, 0);
    os_run_work_delay(&fpv_wk, 1000);
    return 0;
}
