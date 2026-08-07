#include "sys_config.h"	
#include "tx_platform.h"
#include "osal/string.h"
#include "osal/task.h"
#include "osal_file.h"
#include "lib/net/eloop/eloop.h"
#include "video_app/video_app.h"
#include "lwip/api.h"
#include "lwip/sockets.h"
#include "lwip/etharp.h"
#include "utlist.h"
#include "jpgdef.h"
#include "lib/lcd/lcd.h"
#include "lib/common/sysevt.h"
#include "syscfg.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "walkie_talkie.h"
#include "stream_define.h"
#include "lib/multimedia/msi.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

#ifdef SYS_APP_WALKIE_TALKIE

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE av_free
#define STREAM_LIBC_ZALLOC av_zalloc

#define MAX_USER_VIDEO_TX 16
extern volatile uint8_t *psram_user_ptr;

walkie_msg walkmsg;

static int net_video_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int ret = RET_OK;
    switch (cmd_id)
    {

        // 暂时没有考虑释放
        case MSI_CMD_POST_DESTROY:
        {
        }
        break;
        // 接收,判断是否已经压缩了
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;
			if(fb->stype != FSTYPE_H264_GEN420_DATA)
			{
				ret = RET_ERR;
			}
        }
        break;
        case MSI_CMD_FREE_FB:
        {
        }
        break;
        default:
            break;
    }
    return ret;
}


in_addr_t send_addr = 0;
uint8_t   apsta_mode = 0;   //1:ap   2:sta
void h264_enc_dec_thread(){
	uint8_t * ptr;
	uint32_t fcnt;
	uint8_t  oldspeed = 0;
	uint16_t w,h;
	uint8_t dropcnt = 0;
	uint8_t runh264 = 1;
	uint8_t make_h264 = 1;
	ptr = psram_user_ptr;
	uint32_t time_debug = 0;
	uint32_t ipaddr;
	int32 isstaconnect;
	os_sleep_ms(100);
	//must open SYS_WIFI_PAIR
//	sys_wifi_pair_start(sys_cfgs.wifi_mode,1);     //run after pair init
	w = 320;
	h = 240;
	while(1){
		if(ieee80211_conf_get_stacnt(WIFI_MODE_STA) == 1){
			os_printf("STA mode connect\r\n");
			apsta_mode = 2;
			break;
		}

		if(ieee80211_conf_get_stacnt(WIFI_MODE_AP) == 1){
			os_printf("AP mode connect\r\n");
			apsta_mode = 1;
			break;
		}

		os_sleep_ms(5);
	}

	
	if(apsta_mode == 2)	       //STA
	{
		while(1){
			ipaddr = lwip_netif_get_ip2("w0");		
			isstaconnect = ieee80211_conf_get_stacnt(WIFI_MODE_STA);
			
			if(isstaconnect == 0){
				os_sleep_ms(5);
			}else{
//				_os_printf("ipaddr:%x  sta:%d\r\n",ipaddr.addr,ieee80211_conf_get_stacnt(WIFI_MODE_STA));
				break;
			}
		}
		send_addr = NET_IP_ADDR_DEFAULT;
		user_protocol2(6001,6000);
		user_protocol3(6003,6002);
	}
	else{						//AP
		while(1){
			ipaddr = lwip_netif_get_ip2("w0");		
			isstaconnect = ieee80211_conf_get_stacnt(WIFI_MODE_AP);
			
			if(isstaconnect == 0){
				os_sleep_ms(5);
			}else{
//				_os_printf("ipaddr:%x  ap:%d\r\n",ipaddr.addr,ieee80211_conf_get_stacnt(WIFI_MODE_AP));
				break;
			}
		}
		send_addr = DHCPD_START_IP_DEFAULT;
		user_protocol2(6003,6002);
		user_protocol3(6001,6000);		
	}						
	
	while(1){
		
		if(ptr != psram_user_ptr){
			ptr = psram_user_ptr;
			fcnt++;

#if 0			
			if(walkmsg.speed == 0){
				runh264 = 1;           //全力跑
			}else if(walkmsg.speed == 1){
				runh264 = fcnt%3;      //三帧丢一帧
				if(runh264 != 0){
					runh264 = 1;
				}
			}else if(walkmsg.speed == 2){
				runh264 = fcnt%2;      //减半
			}else if(walkmsg.speed == 3){
				runh264 = fcnt%4;      //四帧丢三帧
				if(runh264 == 0){
					runh264 = 1;
				}else{
					runh264 = 0;
				}				
			}
#else
#if 0
			if((fcnt%200) == 100){
				_os_printf("set QVGA.....\r\n");
				h264_reflash_new_gop(1,1);
				set_vpp_bu1_shrink(640,320);
				change_gen420dev_w_h(1,320,240);
				h264_recfg_bsp(1,200);
				runh264 = 0;
				dropcnt = 3;   
				w = 320;
				h = 240;
			}else if((fcnt%200) == 199){
				_os_printf("set QQVGA.....\r\n");
				h264_reflash_new_gop(1,1);
				set_vpp_bu1_shrink(640,160);
				change_gen420dev_w_h(1,160,120);
				h264_recfg_bsp(1,50);
				runh264 = 0;
				dropcnt = 3;
				w = 160;
				h = 120;
			}

			if(dropcnt != 0){
				dropcnt--;
			}else{
				runh264 = 1;
			}
#else
			if(oldspeed != walkmsg.speed){
				if(walkmsg.speed == 0){					//24帧qvga
					_os_printf("set QVGA1.....\r\n");
					h264_reflash_new_gop(1,1);
					set_vpp_bu1_shrink(640,320);
					change_gen420dev_w_h(1,320,240);
					h264_recfg_bsp(1,200);	
					h264_recfg_rate(1,25);
					h264_recfg_frm_gop(1,25);
					h264_recfg_ini_qp(1,26);
					if(w == 160){
						dropcnt = 3;
					}					
					make_h264 = 1;
					w = 320;
					h = 240;					
				}else if(walkmsg.speed == 1){          //12帧qvga
					_os_printf("set QVGA2.....\r\n");
					h264_reflash_new_gop(1,1);
					set_vpp_bu1_shrink(640,320);
					change_gen420dev_w_h(1,320,240);
					h264_recfg_bsp(1,150);	
					h264_recfg_rate(1,12);
					h264_recfg_frm_gop(1,12);
					h264_recfg_ini_qp(1,31);
					if(w == 160){
						dropcnt = 3;
					}					
					make_h264 = 2;
					w = 320;
					h = 240;				
				}else if(walkmsg.speed == 2){		  //12帧qqvga
					_os_printf("set QQVGA3.....\r\n");
					h264_reflash_new_gop(1,1);
					set_vpp_bu1_shrink(640,160);
					change_gen420dev_w_h(1,160,120);
					h264_recfg_bsp(1,100);	
					h264_recfg_rate(1,12);
					h264_recfg_frm_gop(1,12);
					h264_recfg_ini_qp(1,31);
					if(w == 320){
						dropcnt = 3;
					}
					make_h264 = 2;
					w = 160;
					h = 120;					
				}else if(walkmsg.speed == 3){		  //6帧qqvga
					_os_printf("set QQVGA4.....\r\n");
					h264_reflash_new_gop(1,1);
					set_vpp_bu1_shrink(640,160);
					change_gen420dev_w_h(1,160,120);
					h264_recfg_bsp(1,50);	
					h264_recfg_rate(1,6);
					h264_recfg_frm_gop(1,6);
					h264_recfg_ini_qp(1,31);
					if(w == 320){
						dropcnt = 3;
					}
					make_h264 = 4;
					w = 160;
					h = 120;					
				}
				oldspeed = walkmsg.speed;
			}
			if(dropcnt){
				dropcnt--;
				runh264 = 0;
			}else{
				if(fcnt%make_h264 == 0){
					runh264 = 1;
				}else{
					runh264 = 0;
				}
			}

#endif			
			
#endif
			if(runh264 == 1){
				put_h264msg_to_queue(0,w,h,ptr,0);
			}
			
			
			
		}else{
			os_sleep_ms(3);
			
			if((os_jiffies() - time_debug) > 2000){
				time_debug = os_jiffies();		
				os_printf("\r\nmcs:%x  tx_data:%dbyte/s rx_data:%dbyte/s speed:%d txsucc:%d rxsucc:%d  txlost:%d  rxlost:%d\r\n",walkmsg.mcs,walkmsg.tx_data/2,walkmsg.rx_data/2,walkmsg.speed,walkmsg.frame_tx_success/2,walkmsg.frame_rx_success/2,walkmsg.frame_tx_lost/2,walkmsg.frame_rx_lost/2);
				walkmsg.tx_data = 0;
				walkmsg.rx_data = 0;
				walkmsg.frame_tx_lost = 0;
				walkmsg.frame_rx_lost = 0;
				walkmsg.frame_tx_success = 0;
				walkmsg.frame_rx_success = 0;
			}
		}

	}
}

void walkie_msg_init(){
	walkmsg.tx_data          = 0;
	walkmsg.rx_data          = 0;
	walkmsg.speed            = 0;
	walkmsg.frame_tx_lost    = 0;
	walkmsg.frame_rx_lost    = 0;
	walkmsg.frame_tx_success = 0;
	walkmsg.frame_rx_success = 0;
}

void user_protocol()
{
	struct msi *scale2 = scale2_msi("scale2", NET_W, NET_H, 320, 240, FSTYPE_YUV_P0, 10);
    if (scale2)
    {
		msi_add_output(scale2, NULL, "sim_video");
		//msi_add_output(scale2, NULL, R_VIDEO_P1);
		//os_printf("%s  %d\r\n",__func__,__LINE__);
    }
	walkie_msg_init();
	scale2_output_size_local_change(0,0,0,0,160,120);
	h264wq_queue_init(NET_W,NET_H);
	os_task_create("h264_enc_dec_thread", h264_enc_dec_thread, NULL, OS_TASK_PRIORITY_HIGH, 0, NULL, 1024);
}

#endif