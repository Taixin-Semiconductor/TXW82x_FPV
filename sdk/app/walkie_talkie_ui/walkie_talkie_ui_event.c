#include "walkie_talkie_ui_event.h"
#include "stream_define.h"
#include "lib/multimedia/msi.h"
#include "hal/scale.h"
#include "hal/vpp.h"
#include "syscfg.h"
#include "battery_det.h"
#include "ui_vpp_ipf_ctrl.h"
#include "ui_vpp_ipf_resource.h"
#include "scale_msi.h"
#include "lib/umac/ieee80211.h"
#include "lib/common/atcmd.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/net/dhcpd/dhcpd.h"
#include "scale_msi/scale_msi.h"
#include "lib/multimedia/txmplayer.h"
#ifdef PIN_FROM_PARAM
#include "pin_param.h"
#endif


void sys_wifi_pair_start(uint8 ifidx, uint16 magic);
void sys_dhcpd_start();
#define _DEBUG(fmt, ...)    //os_printf(fmt, ##__VA_ARGS__)

// View Switch
#define L_W  320
#define L_H  240

#define S_W  128
#define S_H  96

#define T_OFFSET 8

// wifi pair  interface
struct {
    uint8_t pair_sucess;
    uint8_t pair_open;
    uint8_t wifi_mode;
    uint8_t bssid[6];
} walkie_talkie_wifi_status;

extern struct system_status sys_status;
extern void jpg_decode_run(uint32_t addr, uint32_t id);
extern struct msi *sim_video_more_msi(char *name, int w, int h, uint16_t *filter);

#define WALKIE_TALKIE_JPG_DECODE_CACHE_MAX_SIZE     (15*1024)

static uint8_t *walkie_talkie_jpg_decode_cache = NULL;
static uint8_t *walkie_talkie_jpg_decode_last_addr = NULL;
static volatile uint8_t user_dispnum = 0;      // 当前显示的画面数量
static int32_t mplayer_stream_id = -1; // 提示音播放组件句柄
static volatile uint8_t welcome_ready = 0;

// 回调函数指针
static wt_msi_callback_t g_msi_cb = NULL;

// 注册回调接口
void walkie_talkie_register_msi_callback(wt_msi_callback_t cb)
{
    g_msi_cb = cb;
}

// 发送事件接口
int walkie_talkie_send_event(wt_msi_event_t event, uint32_t param1, uint32_t param2, uint32_t param3)
{
    if (g_msi_cb) {
        return g_msi_cb(event, param1, param2, param3);  // 返回回调函数返回值
    }
    return -1; // 没有注册回调函数
}


// GPIO 背光回调
static void lcd_backlight_on(uint32 param1, uint32 parma2, uint32 param3)
{
    gpio_set_mode(MACRO_PIN(LCD_BACKLIGHT_IO), GPIO_PULL_NONE, GPIO_PULL_LEVEL_NONE);   //PA_3
    gpio_set_dir(MACRO_PIN(LCD_BACKLIGHT_IO), GPIO_DIR_OUTPUT);
    gpio_set_val(MACRO_PIN(LCD_BACKLIGHT_IO), 1);
}

static void lcd_backlight_off(uint32 param1, uint32 parma2, uint32 param3)
{
    gpio_set_mode(MACRO_PIN(LCD_BACKLIGHT_IO), GPIO_PULL_NONE, GPIO_PULL_LEVEL_NONE);   //PA_3
    gpio_set_dir(MACRO_PIN(LCD_BACKLIGHT_IO), GPIO_DIR_OUTPUT);
    gpio_set_val(MACRO_PIN(LCD_BACKLIGHT_IO), 0);
}

// MSI 事件处理回调函数
static int walkie_talkie_ui_event_handler(wt_msi_event_t event, uint32_t param1, uint32_t param2, uint32_t param3)
{
    switch (event)
    {
    case WT_EVT_MSI_INIT:       // 初始化 MSI 组件
        {
            _DEBUG("## WT Event: MSI Init Start \n");
            struct vpp_device   *vpp_dev;
            vpp_dev = (struct vpp_device *) dev_get(HG_VPP_DEVID);
            vpp_set_ifp_en(vpp_dev, 0);
            vpp_set_watermark0_enable(vpp_dev, 0);

            static const uint16_t filter[]  = {FSTYPE_YUV_P0, FSTYPE_YUV_P1, FSTYPE_NONE};
            
            // 可以显示更多的图(通过filter去设置,每一张图都会合并,多摄像头可以参考)
            struct msi  *sim_video = sim_video_more_msi("sim_video", 320, 240, (uint16_t *) filter);
            if (sim_video) {
                msi_add_output(sim_video, NULL, NULL, R_VIDEO_P0);    // sim_video输出到R_VIDEO_P0的msi
            }

            // 将scale3的数据发送到sim_video,然后sim_video去dam2d去重新组成一张图给到lcd显示
            struct msi *scale3 = scale3_msi(S_PREVIEW_SCALE3);
            if (scale3) {
                // scale3输出到sim_video的msi     即scaler3--->sim--->r_video_p0
                msi_add_output(scale3, NULL, NULL, "sim_video");
            }

            struct msi *scale2 = scale2_msi("scale2", 320, 240, 320, 240, FSTYPE_YUV_P0, 10);
            if (scale2) {
                msi_add_output(scale2, NULL, NULL, "sim_video");
				msi_cmd("scale2", MSI_CMD_SCALE2, MSI_SCALE2_SET_FILTER_TYPE, MJPEG_DEC);
			}
        }
        break;

    case WT_EVT_DECODE_LOGO:    // 解码开机 Logo
        {
            uint8_t *ui_logo = (uint8_t*)param1;
            uint32_t size = param2;
            _DEBUG("## WT Event: Decode Logo Start \n");

            if (walkie_talkie_jpg_decode_cache == NULL) {
                walkie_talkie_jpg_decode_cache = (uint8_t *)av_psram_malloc(WALKIE_TALKIE_JPG_DECODE_CACHE_MAX_SIZE);
            }
            if (!walkie_talkie_jpg_decode_cache) return -1;

            if (size > WALKIE_TALKIE_JPG_DECODE_CACHE_MAX_SIZE) {
                return -1;
            }

            if (walkie_talkie_jpg_decode_last_addr != ui_logo) {
                walkie_talkie_jpg_decode_last_addr = ui_logo;
                os_memcpy(walkie_talkie_jpg_decode_cache, ui_logo, size);    // cpu copy
                sys_dcache_clean_range((uint32_t*)walkie_talkie_jpg_decode_cache, size); 
            }
//            scale2_recfg_lock(1);
            scale2_recfg_input_size(320, 240, 0);
            int _ret = scale2_cfg_run(MJPEG_DEC, 0);
            if(_ret == 0) {
                jpg_decode_run((uint32)walkie_talkie_jpg_decode_cache, 0);
            }
//            scale2_recfg_lock(0);
        }
        break;

    case WT_EVT_JPG_DECODE_RUN:
        {
            uint8_t *ui_jpg = (uint8_t*)param1;
            uint32_t size = param2;
            _DEBUG("## WT Event: Jpg Decode Start \n");
            if (walkie_talkie_jpg_decode_cache == NULL) {
                walkie_talkie_jpg_decode_cache = (uint8_t *)av_psram_malloc(WALKIE_TALKIE_JPG_DECODE_CACHE_MAX_SIZE);
            }
            if (!walkie_talkie_jpg_decode_cache) return -1;

            if (size > WALKIE_TALKIE_JPG_DECODE_CACHE_MAX_SIZE) {
                return -1;
            }

            if (walkie_talkie_jpg_decode_last_addr != ui_jpg) {
                walkie_talkie_jpg_decode_last_addr = ui_jpg;
                os_memcpy(walkie_talkie_jpg_decode_cache, ui_jpg, size);    // cpu copy
                sys_dcache_clean_range((uint32_t*)walkie_talkie_jpg_decode_cache, size); 
            }
            os_sleep_ms(10);
//            scale2_recfg_lock(1);
            scale2_recfg_input_size(320, 240, 0);
            for(uint32_t i=0; i<3; i++) {
                int _ret = scale2_cfg_run(MJPEG_DEC, 0);
                if(_ret == 0) {
                    jpg_decode_run((uint32)walkie_talkie_jpg_decode_cache, 0);
                }
            }
//            scale2_recfg_lock(0);
        }
        break;

    case WT_EVT_BACKLIGHT_ON:   // 开启背光
        {
            _DEBUG("## WT Event: Backlight Init \n");
            os_run_func_delay(lcd_backlight_on, (uint32_t)NULL, (uint32_t)NULL, 200);
        }
        break;

    case WT_EVT_BACKLIGHT_OFF:   // 关闭背光
        {
            os_run_func_delay(lcd_backlight_off, (uint32_t)NULL, (uint32_t)NULL, 200);
        }
        break;

    case WT_EVT_VOLUME_SET:   // 音量设置
        {
            // 参数转换回 uint8_t 音量值
            uint8_t vol_val = (uint8_t)param1;
            
            _DEBUG("## WT Event: Volume Change to %d \n", vol_val);

            // uint8_t backVol=0;
            const uint32 dacgain_table[]=
            {  // 0~100
                0,
                10,
                20,
                30,
                40,
                50,
                60,
                70,
                80,
                90,
                100
            };

            if(vol_val>10)
                vol_val =10;

            // if(backVol!=vol_val)
            // {
            //     backVol = vol_val;
                msi_cmd("audio_mixer",MSI_CMD_SET_VOLUME,dacgain_table[vol_val],0);
            // }
        }
        break;

    case WT_EVT_VIEW_SWITCH:    // 视图切换
        {
            _DEBUG("## WT Event: View Switch \n");

            uint8_t p0p1_flag = (uint8_t)param1;

            switch (p0p1_flag)
            {
                case 0 :  //P1_bg  P0_front
                    os_sleep_ms(10);	
                    scale2_output_size_local_change(0,0,0,0,L_W,L_H);
                    #if USE_90_DEGREE_LOGO
                    scale3_output_size_local_change(0,0,T_OFFSET,L_H-S_H-T_OFFSET,S_W,S_H);		
                    #else
                    scale3_output_size_local_change(0,0,L_W-S_W-6,6,S_W,S_H);		
                    #endif
                break;

                case 1 :  //P1_front  P0_back  
                    os_sleep_ms(10);	
                    #if USE_90_DEGREE_LOGO
                    scale2_output_size_local_change(0,0,T_OFFSET,L_H-S_H-T_OFFSET,S_W,S_H);		
                    #else
                    scale2_output_size_local_change(0,0,L_W-S_W-6,6,S_W,S_H);
                    #endif
                    scale3_output_size_local_change(0,0,0,0,L_W,L_H);
                break;

                case 2 :  //P0_only
                    os_sleep_ms(10);
                    #if USE_90_DEGREE_LOGO
                    scale2_output_size_local_change(0,0,T_OFFSET,L_H-S_H-T_OFFSET,S_W,S_H);	
                    #else
                    scale2_output_size_local_change(0,0,L_W-S_W-6,6,S_W,S_H);
                    #endif
                    scale3_output_size_local_change(0,1,0,0,L_W,L_H);
                break;
                
                case 3 :  //P1_only
                    os_sleep_ms(10);
                    scale2_output_size_local_change(0,1,0,0,L_W,L_H);
                    #if USE_90_DEGREE_LOGO
                    scale3_output_size_local_change(0,0,T_OFFSET,L_H-S_H-T_OFFSET,S_W,S_H);	
                    #else
                    scale3_output_size_local_change(0,0,L_W-S_W-6,6,S_W,S_H);		
                    #endif
                break;
            
                default:
                    break;
            }
        }
        break;
    
    case WT_EVT_PROMPT_TONE_PLAY:  // 提示音播放
        {
            _DEBUG("## WT Event: Prompt Tone Play \n");
            char *filepath = (char *)param1;

            //delayUnmute(100);

            if(mplayer_stream_id >= 0) {
                txmplayer_close(mplayer_stream_id);
                mplayer_stream_id = -1;
            }
            txmplayer_init(0, 0, NULL);
            mplayer_stream_id = txmplayer_open(filepath, 0, NULL);
        }
        break;

    case WT_EVT_PAIR_MODE_SET:   // 配对模式设置
        {
            _DEBUG("## WT Event: Pair Mode Set \n");
            uint8_t enable = (uint8_t)param1;

            if(enable)
            {
                walkie_talkie_wifi_status.pair_open = 1;
                sys_status.pair_role = 0;
                walkie_talkie_wifi_status.wifi_mode = sys_cfgs.wifi_mode;
#if !USE_CALLING_DEMO
                intercom_deinit();
#endif
                if (walkie_talkie_wifi_status.wifi_mode == WIFI_MODE_AP) {
                    os_memcpy(walkie_talkie_wifi_status.bssid, sys_cfgs.bssid, 6);
                    os_memset(sys_cfgs.bssid, 0, 6);
                    ieee80211_disassoc_all(WIFI_MODE_AP);
                    dhcpd_stop();
                    sys_cfgs.dhcpc_en = 1;
                    sys_cfgs.wifi_mode = WIFI_MODE_STA;
                    ieee80211_iface_stop(WIFI_MODE_AP); //stop AP
                    wificfg_flush(WIFI_MODE_STA);
                    ieee80211_iface_start(WIFI_MODE_STA); //switch to STA.
                }
                ieee80211_conf_set_pair_ngo(sys_cfgs.wifi_mode, 1);
                ieee80211_conf_set_mutl_pair(sys_cfgs.wifi_mode, 0);
                sys_wifi_pair_start(sys_cfgs.wifi_mode, 1);
            }
            else
            {
                walkie_talkie_wifi_status.pair_open = 0;
                ieee80211_conf_set_pair_ngo(sys_cfgs.wifi_mode, 0);
                ieee80211_conf_set_mutl_pair(sys_cfgs.wifi_mode, 0);
                sys_wifi_pair_start(sys_cfgs.wifi_mode, 0);
                if (walkie_talkie_wifi_status.wifi_mode == WIFI_MODE_AP) {
					os_memcpy(sys_cfgs.bssid, walkie_talkie_wifi_status.bssid, 6);
                    sys_cfgs.wifi_mode = WIFI_MODE_AP;
                    ieee80211_iface_stop(WIFI_MODE_STA); //stop AP
                    wificfg_flush(WIFI_MODE_AP);
                    ieee80211_iface_start(WIFI_MODE_AP); //switch to STA.
                    sys_dhcpd_start();
                }
#if !USE_CALLING_DEMO
                intercom_init();
#endif
            }
        }
        break;

    case WT_EVT_PAIRSTATUS_GET:  // 获取配对状态
        {
            _DEBUG("## WT Event: Get Pair Status \n");
            return walkie_talkie_wifi_status.pair_open;
        }
        break;

    case WT_EVT_PAIRSUCCESS_GET:  // 获取配对成功状态
        {
            _DEBUG("## WT Event: Get Pair Success Status \n");
            return walkie_talkie_wifi_status.pair_sucess;
        }
        break;

    case WT_EVT_DISP_NUM_SET:    // 设置当前显示的画面数量
        {
            _DEBUG("## WT Event: Set Display Number \n");
            uint8_t dispnum = (uint8_t)param1;
            user_dispnum = dispnum;
        }

        break;

    case WT_EVT_DISP_NUM_GET:    // 获取当前显示的画面数量
        {
            _DEBUG("## WT Event: Get Display Number \n");
            if (user_dispnum != sys_status.wifi_connected) {
                user_dispnum = sys_status.wifi_connected;
            }
            return user_dispnum;
        }
        break;
    
    case WT_EVT_BAT_DET_INIT:    // 电池检测初始化
        {
            _DEBUG("## WT Event: Battery Detection Init \n");
            bat_ad_init();
        }
        break;

    case WT_EVT_BAT_GET_LEVEL:
        {
            int level = bat_get_level();
            _DEBUG("## WT Event: Battery get level:%d\n", level);
            return level;
        } 
        break;


    case WT_EVT_WIFI_CONNECT_GET:        // 获取 WiFi 连接状态
        {
            _DEBUG("## WT Event: Get WiFi Connect Status \n");
            return sys_status.wifi_connected;
        }
        break;

    case WT_EVT_WIFI_SIGNAL_RSSI_GET:    // 获取 WiFi 信号强度（RSSI）
        {
            _DEBUG("## WT Event: Get WiFi Signal RSSI \n");
            return (int8)sys_status.rssi;
        }
        break;

    case WT_EVT_WIFI_SIGNAL_EVM_GET:     // 获取 WiFi 信号质量（EVM）
        {
            _DEBUG("## WT Event: Get WiFi Signal EVM \n");
            return (int8)sys_status.evm;
        }
        break;

    case WT_EVT_WIFI_CHANNEL_GET:        // 获取 WiFi 信道
        {
            _DEBUG("## WT Event: Get WiFi Channel \n");
            return (int8)sys_status.channel;
        }
        break;

    case WT_EVT_WIFI_FREQ_OFFSET_GET:     // 获取 WiFi 频偏
        {
            _DEBUG("## WT Event: Get WiFi Frequency Offset \n");
            return 11;
        }
        break;

    case WT_EVT_VPP_IPF_INIT:
        {
            ui_vpp_ipf_ctrl_init();
        }   
        break;

    case WT_EVT_VPP_IPF_CTRL:
        {
            uint32_t *val = (uint32_t*)param1;
            if((*val) < UI_VPP_IPF_RES_MAX_NUM)
            {
                os_sleep_ms(10);
                ui_vpp_ipf_update_resource((void*)ipf_imgSrcTable[(*val)]);
            }

            if((*val) < UI_VPP_IPF_RES_MAX_NUM)
            {
                ++(*val);
            }
            else
            {
                (*val) = 0;
                ui_vpp_ipf_update_resource(NULL);
            }
        }
        break;

    case WT_EVT_WELCOME_READY_GET:
    {
        return welcome_ready;
    }
        break;

    case WT_EVT_WELCOME_READY_SET:
    {
        welcome_ready = 1;
        msi_cmd("scale2", MSI_CMD_SCALE2, MSI_SCALE2_SET_FILTER_TYPE, ~0);
    }
        break;    

    case WT_EVT_CALLING_SET:
    {
        walkie_talkie_calling_set(param1);
    }
    break;

    case WT_EVT_CALLING_GET:
    {
        *((int32*)param1) = walkie_talkie_calling_get();
    }
    break;
    default:
        return -1;
    }
    return 0;
}


// 模块初始化注册回调
void walkie_talkie_msi_ext_init(void)
{
    walkie_talkie_register_msi_callback(walkie_talkie_ui_event_handler);
}

void sys_event_hdl_walkie_talkie(uint32 event_id, uint32 data, uint32 priv)
{
    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_WIFI, SYSEVT_WIFI_PAIR_DONE):
        {
            if(sys_cfgs.wifi_mode == WIFI_MODE_AP) {
                syscfg_save();
            }
#if !USE_CALLING_DEMO
#ifdef SYS_APP_WALKIE_TALKIE
            user_protocol_deinit();
            user_protocol_reinit();
#endif
            intercom_init();
#endif
        }
        break;
    }
}

int32 sys_wifi_event_hdl_walkietalkie(uint8 ifidx, uint16 evt, uint32 param1, uint32 param2)
{
    int32_t ret = 0;
    switch (evt) {
        case IEEE80211_EVENT_PAIR_START:
            walkie_talkie_wifi_status.pair_sucess = 0;
            break;
        case IEEE80211_EVENT_PRE_AUTH:
			if(memcmp(sys_cfgs.bssid, (uint8 *)param1, 6) != 0) {
				return 1;
			}
            break;
        case IEEE80211_EVENT_PRE_ASSOC:
			if(memcmp(sys_cfgs.bssid, (uint8 *)param1, 6) != 0) {
				return 1;
			}
            break;
        case IEEE80211_EVENT_PAIR_SUCCESS:
            ieee80211_pairing(sys_cfgs.wifi_mode, 0);
            break;
        case IEEE80211_EVENT_PAIR_DONE:
            walkie_talkie_wifi_status.pair_sucess = 1;
            if(walkie_talkie_wifi_status.pair_open == 1) {
                walkie_talkie_wifi_status.pair_open = 0;
            }
            if((int32)param2 == 1 && sys_cfgs.wifi_mode == WIFI_MODE_STA) {
                os_memcpy(sys_cfgs.bssid, (uint8_t*)param1, 6);
            }
            break;
#if !USE_CALLING_DEMO
        case IEEE80211_EVENT_CONNECTED:
            walkie_talkie_send_event(WT_EVT_DISP_NUM_SET, 1, (uint32_t)NULL, (uint32_t)NULL);
            break;
        case IEEE80211_EVENT_DISCONNECTED:
            walkie_talkie_send_event(WT_EVT_DISP_NUM_SET, 0, (uint32_t)NULL, (uint32_t)NULL);
            #if USE_90_DEGREE_LOGO
			extern const unsigned char ui_bgLogo[14217];
            walkie_talkie_send_event(WT_EVT_JPG_DECODE_RUN, (uint32_t)ui_bgLogo, sizeof(ui_bgLogo), NULL);
            #else
			extern const unsigned char ui_bgLogo_ap[14241];
            walkie_talkie_send_event(WT_EVT_JPG_DECODE_RUN, (uint32_t)ui_bgLogo_ap, sizeof(ui_bgLogo_ap), (uint32_t)NULL);
            #endif
            break;
#endif
    }
    return ret;
}
