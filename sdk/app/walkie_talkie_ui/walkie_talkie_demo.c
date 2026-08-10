#include "basic_include.h"
#include "lvgl/lvgl.h"
#include "lib/multimedia/msi.h"
#include "stream_define.h"
#include "walkie_talkie_demo.h"
#include "keyScan.h"
#include "walkie_talkie_ui_event.h"
#include "lib/multimedia/txmplayer.h"

camera_global_t camera_gvar;
cam_set_t camSetParam;

lv_obj_t *curPage_obj;
lv_obj_t *ui_poweronPage;
lv_obj_t *ui_poweroffPage;
lv_obj_t *ui_intercomPage;
lv_obj_t *ui_intercomTopBar;
lv_obj_t *ui_intercomBtmBar;
lv_obj_t *ui_intercomBatImg;
lv_obj_t *ui_dialogPanel; 
lv_obj_t *ui_dialogContent;
lv_obj_t *ui_volPanel;
lv_obj_t *ui_lisglabel;
lv_obj_t *ui_volumeLevels[10];
lv_obj_t *ui_signalPanel;
lv_obj_t *ui_signalLevels[4];
lv_obj_t *mic_img;
lv_obj_t *ui_intercomMagicSoundImg;
lv_obj_t *ui_pairPanel;
lv_obj_t *ui_pairTeimlabel;
lv_obj_t *ui_speedinfo_id_obj;
lv_obj_t *ui_speedinfo_num_obj;

#ifdef DISPLAY_DEBUGINFO_ENABLE 
char msgid_str[100];
char msgnum_str[100];
#endif
static char pair_str[100];
uint32_t USER_KEY_EVENT = 0;
int32_t calling_tone_stream_id = -1;

const lv_img_dsc_t *ui_imgset_iconBat[5] = {&iconBat0,&iconBat1,&iconBat2,&iconBat3,&iconBat4};
const lv_img_dsc_t *ui_imgset_iconMagicSound[4] = {&iconSoundNormal,&iconSoundAlien,&iconSoundRobot,&iconSoundChild};

/* walkie_talkie_demo key */
uint32_t user_key_filter(uint32_t val)
{
	static uint32_t key =0;
	uint32_t key_ret = 0;
    if(val > 0 && camera_gvar.screen_on)
	{
		if((val & 0xff) == KEY_EVENT_SUP)
		{
   			key = (val >> 8);
			switch(val >> 8)
			{
				case AD_UP:
					key_ret = LV_KEY_PREV;
				break;
				case AD_DOWN:
					key_ret = LV_KEY_NEXT;
				break;
				case AD_LEFT:
					key_ret = LV_KEY_PREV;
				break;
				case AD_RIGHT:
					key_ret = LV_KEY_NEXT;
				break;
				case AD_PRESS:
					key_ret = LV_KEY_ENTER;
				break;
				case AD_SPEACH:
#if USE_CALLING_DEMO
				if(camera_gvar.page_cur == PAGE_INTERCOM)
				{
					key = KEY_CALLING_STOP;
					lv_event_send(curPage_obj, USER_KEY_EVENT, &key);
				}
#else
					mic_img_set_visible(0);
#endif
				break;
				default:
				break;
			}
			lv_event_send(curPage_obj, USER_KEY_EVENT, &key);
		}
		else if((val & 0xff) == KEY_EVENT_DOWN)
		{
			switch(val >> 8)
			{
#if !USE_CALLING_DEMO
				case AD_SPEACH:
					mic_img_set_visible(1);
				break;
#endif
				default:
				break;
			}
		}
        else if((val & 0xff) == KEY_EVENT_LUP)
		{	
			switch(val >> 8)
			{
#if !USE_CALLING_DEMO
				case AD_SPEACH:
					mic_img_set_visible(0);
				break;
#endif
				default:
				break;
			}
			os_printf("lvgl longup keyval:%02X\n",val);
		}
		else if((val & 0xff) == KEY_EVENT_REPEAT)
		{
			switch(val >> 8)
			{
				//lvgl需要持续按键才能识别长按
				case AD_PRESS:
					key_ret = LV_KEY_ENTER;
				break;

				case KEY_POWER:
				if(camera_gvar.page_cur == PAGE_INTERCOM)
				{
					key = KEY_POWEROFF;
					lv_event_send(curPage_obj, USER_KEY_EVENT, &key);
				}
				break;
		
				default:
				break;
			}
		}
        else if((val & 0xff) == KEY_EVENT_LDOWN)
		{
			os_printf("lvgl long keyval:%02X\n",val);
			switch(val >> 8)
			{
				case KEY_POWER:
				if(camera_gvar.page_cur == PAGE_INTERCOM)
				{
					key =KEY_POWEROFF;
					lv_event_send(curPage_obj, USER_KEY_EVENT, &key);
				}
				break;

				case AD_RIGHT:
				if(camera_gvar.page_cur == PAGE_INTERCOM)
				{
					key = KEY_MAGIC_SWITCH;
					lv_event_send(curPage_obj, USER_KEY_EVENT, &key);
				}
				break;

				case AD_DOWN:
				if(camera_gvar.page_cur == PAGE_INTERCOM)
				{
					key = KEY_PAIR;
					lv_event_send(curPage_obj, USER_KEY_EVENT, &key);
				}
				break;
				
				case AD_UP:
				if(camera_gvar.page_cur == PAGE_INTERCOM)
				{
					key = KEY_IPF_SWITCH;
					lv_event_send(curPage_obj, USER_KEY_EVENT, &key);
				}
				break;

#if USE_CALLING_DEMO
				case AD_SPEACH:
				if(camera_gvar.page_cur == PAGE_INTERCOM)
				{
					key = KEY_CALLING_START;
					lv_event_send(curPage_obj, USER_KEY_EVENT, &key);
				}					
				break;
#endif

				default:
				break;
			}
		}
	}
	camera_gvar.key_operate = 1;
	return key_ret;
}

void event_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if((code == LV_EVENT_CLICKED) ||(code == USER_KEY_EVENT))
	{
		LV_LOG_USER("Clicked");

		os_printf(" # Clicked camera_gvar.page_cur=%d ,event_handler=%d USER_KEY_EVENT=%d  \n",camera_gvar.page_cur,code,USER_KEY_EVENT);
		
		if(camera_gvar.page_cur == PAGE_INTERCOM){
			ui_event_intercomPage(e);
		}

	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

void batteryStatusProcess(void)
{
	uint8_t bt_status = walkie_talkie_send_event(WT_EVT_BAT_GET_LEVEL, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL);
	 if(camera_gvar.page_cur == PAGE_INTERCOM)
	 {
		lv_img_set_src(ui_intercomBatImg, ui_imgset_iconBat[bt_status]);
	 }
}

void render_vol_level(uint8_t vol)
{
	if(vol > 0)
	{
		if(vol > 6)
			lv_label_set_text(ui_lisglabel, LV_SYMBOL_VOLUME_MAX);
		else
			lv_label_set_text(ui_lisglabel, LV_SYMBOL_VOLUME_MID);
		lv_obj_set_style_text_color(ui_lisglabel, lv_color_hex(0xFCA702), 0);
	}
	else
	{
		lv_label_set_text(ui_lisglabel, LV_SYMBOL_MUTE);
		lv_obj_set_style_text_color(ui_lisglabel, lv_color_hex(0x6D6C6C), 0);
	}

	for(int i = 0; i < 10; i++)
	{
		if(i < vol)
			lv_obj_set_style_bg_color(ui_volumeLevels[i], lv_color_hex(0xFCA702), LV_PART_MAIN | LV_STATE_DEFAULT );
		else
			lv_obj_set_style_bg_color(ui_volumeLevels[i], lv_color_hex(0x6D6C6C), LV_PART_MAIN | LV_STATE_DEFAULT );
	}
}

void volDisAnimationStart(void)
{
	if((camera_gvar.page_cur == PAGE_INTERCOM))
	{	
		// visiable shot icon
		lv_obj_clear_flag(ui_volPanel, LV_OBJ_FLAG_HIDDEN); 
		os_printf(" ## volDisAnimationStart \n");
		camera_gvar.volume_anim_times = 5;
	}
}

void volDisplayProcess(void)
{
	if((camera_gvar.page_cur == PAGE_INTERCOM))
	{
		if(camera_gvar.volume_anim_times)
		{
			camera_gvar.volume_anim_times --;
			os_printf(" ## volume_anim_times=%d \n",camera_gvar.volume_anim_times);
			if(camera_gvar.volume_anim_times == 0)
			{
				lv_obj_add_flag(ui_volPanel, LV_OBJ_FLAG_HIDDEN);
			}
		}
	}
}

void signalDisplayProcess(void)
{
	int32_t signal_strength = -100;
	int16_t signal_level = 0;
	if(camera_gvar.page_cur == PAGE_INTERCOM)
	{
		signal_strength = walkie_talkie_send_event(WT_EVT_WIFI_SIGNAL_RSSI_GET, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL);

		if(walkie_talkie_send_event(WT_EVT_DISP_NUM_GET, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL))
		{
			if(signal_strength > -60)
			{
				signal_level = 4;
			}
			else if(signal_strength > -70)
			{
				signal_level = 3;
			}
			else if(signal_strength > -80)
			{
				signal_level = 2;
			}
			else if(signal_strength > -90)
			{
				signal_level = 1;
			}
		}
		else
		{
			signal_level = 0;
		}

		if(lv_obj_is_valid(ui_signalPanel))
		{
			for(int i=0 ; i < 4;i++)
			{
				if(i<signal_level)
					lv_obj_set_style_bg_color(ui_signalLevels[i], lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT );
				else
					lv_obj_set_style_bg_color(ui_signalLevels[i], lv_color_hex(0x6D6C6C), LV_PART_MAIN | LV_STATE_DEFAULT );
			}
		}
	}
}

void mic_img_set_visible(bool visible)
{
    if(!lv_obj_is_valid(mic_img)) return;

    if(visible) {
        lv_obj_clear_flag(mic_img, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(mic_img, LV_OBJ_FLAG_HIDDEN);
    }
}

void user_stopPair(void)
{
	os_printf(" ## userPairStop \n");
	walkie_talkie_send_event(WT_EVT_PAIR_MODE_SET, 0, (uint32_t)NULL, (uint32_t)NULL);
}

void user_startPair(void)
{
	os_printf(" ## userPairStart \n");
	walkie_talkie_send_event(WT_EVT_PAIR_MODE_SET, 1, (uint32_t)NULL, (uint32_t)NULL);
	camera_gvar.pair_out_times = 20;
	if (camera_gvar.page_cur == PAGE_INTERCOM) 
	{	
		lv_obj_set_style_bg_color(ui_intercomPage, lv_color_hex(0xb3d9e6), 0);

		sprintf(pair_str,"配对 时间:%02d", camera_gvar.pair_out_times);
		lv_label_set_text(ui_pairTeimlabel,pair_str);	
		
		lv_obj_clear_flag(ui_pairPanel, LV_OBJ_FLAG_HIDDEN );  
	}
}

void pairDisplayProcess(void)
{
	if(camera_gvar.page_cur == PAGE_INTERCOM)
	{
		if(camera_gvar.pair_out_times)
		{
			camera_gvar.pair_out_times --;
			os_printf(" ## pair_out_times=%d \n", camera_gvar.pair_out_times);

			if(walkie_talkie_send_event(WT_EVT_PAIRSUCCESS_GET, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL))	
			{
				lv_label_set_text(ui_pairTeimlabel, "配对 成功 ");	
					
				//  camera_gvar.pair_out_times = 12;

				if(walkie_talkie_send_event(WT_EVT_DISP_NUM_GET, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL) > 0) 
				{
					lv_obj_add_flag( ui_pairPanel, LV_OBJ_FLAG_HIDDEN ); 
					lv_obj_set_style_bg_color(ui_intercomPage, lv_color_hex(0x000000), 0);
					camera_gvar.pair_out_times = 0;
				}
			}
			else
			{
				if(camera_gvar.pair_out_times== 0)
				{
					// if(walkie_talkie_send_event(WT_EVT_PAIRSTATUS_GET, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL))
					user_stopPair();

					lv_label_set_text(ui_pairTeimlabel, "配对 超时");	
			
					lv_obj_add_flag( ui_pairPanel, LV_OBJ_FLAG_HIDDEN ); 
					lv_obj_set_style_bg_color(ui_intercomPage, lv_color_hex(0x000000), 0);
				}	
				else
				{
					sprintf(pair_str, "配对 时间:%02d", camera_gvar.pair_out_times);
					lv_label_set_text(ui_pairTeimlabel, pair_str);	
				}
			}
		}
	}
}

#ifdef DISPLAY_DEBUGINFO_ENABLE
void display_msg(uint32 rx_speed, uint32 tx_speed, uint32 lcd_num, uint32 id)
{

	int8_t channel		 = walkie_talkie_send_event(WT_EVT_WIFI_CHANNEL_GET, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL);
	int8_t freq_offset 	 = walkie_talkie_send_event(WT_EVT_WIFI_FREQ_OFFSET_GET, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL);
	int8_t evm 			 = walkie_talkie_send_event(WT_EVT_WIFI_SIGNAL_EVM_GET, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL);
	int8_t rssi 		 = walkie_talkie_send_event(WT_EVT_WIFI_SIGNAL_RSSI_GET, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL);

	memset(msgid_str, 0, 100);
	sprintf(msgid_str,"num:#ff0088 %02d# ch:#ff0088 %02d# offset:#ff0088 %02d#", lcd_num, channel, freq_offset);
	lv_label_set_text(ui_speedinfo_id_obj, msgid_str);
	
	memset(msgnum_str, 0, 100);
	sprintf(msgnum_str,"mcs:#ff0088 %02x# evm:#ff0088 %d# rssi:#ff0088 %d#", 7, evm, rssi);
	lv_label_set_text(ui_speedinfo_num_obj, msgnum_str);	
}
#endif

void timer_event()
{
	static uint32 timer_count = 0;

	if((timer_count % 5) == 0) // half second
	{
		batteryStatusProcess();
		volDisplayProcess();
		signalDisplayProcess();

		// poweron back homepage
		if(camera_gvar.welcome_times)
		{
			camera_gvar.welcome_times--;
			if (camera_gvar.welcome_times == 0) {
				lv_page_select(camera_gvar.poweron_nextpage);
				walkie_talkie_send_event(WT_EVT_WELCOME_READY_SET, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL);
			}
		}
	}

	if ((timer_count % 20) == 0) { // one second 
#ifdef DISPLAY_DEBUGINFO_ENABLE
		if(camera_gvar.page_cur == PAGE_INTERCOM)
			display_msg(0, 0, walkie_talkie_send_event(WT_EVT_DISP_NUM_GET, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL)/2, 0x12356789);	
#endif
	}

	if ((timer_count % 10) == 0) { // one second 
		pairDisplayProcess();
	}
	
	if(camera_gvar.key_operate == 0) {
		camera_gvar.key_operate_timeout++;
		if(camera_gvar.key_operate_timeout >= camera_gvar.screen_on_time) {
			camera_gvar.key_operate_timeout = camera_gvar.screen_on_time;
			if(camera_gvar.screen_on_time >= 0 && camera_gvar.screen_on) {
				walkie_talkie_send_event(WT_EVT_BACKLIGHT_OFF, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL);
				camera_gvar.screen_on = 0;
			}
		}
	}
	else {
		camera_gvar.key_operate_timeout = 0;
		camera_gvar.key_operate = 0;
		if(camera_gvar.screen_on == 0) {
			walkie_talkie_send_event(WT_EVT_BACKLIGHT_ON, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL);
			camera_gvar.screen_on = 1;
		}
	}

#if USE_CALLING_DEMO
	int32 calling_status = status_none;
	calling_status = walkie_talkie_calling_get();
	switch(calling_status) {
		case wait_connect:
			camera_gvar.key_operate_timeout = 0;
			if(camera_gvar.screen_on == 0) {
				walkie_talkie_send_event(WT_EVT_BACKLIGHT_ON, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL);
				camera_gvar.screen_on = 1;
			}
			break;
		case wait_accept_connect:
			camera_gvar.key_operate_timeout = 0;
			if(camera_gvar.screen_on == 0) {
				walkie_talkie_send_event(WT_EVT_BACKLIGHT_ON, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL);
				camera_gvar.screen_on = 1;
			}
			break;
		case accept_connect:
			camera_gvar.key_operate_timeout = 0;
			if(camera_gvar.screen_on == 0) {
				walkie_talkie_send_event(WT_EVT_BACKLIGHT_ON, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL);
				camera_gvar.screen_on = 1;
			}
			break;
		case connecting:
			camera_gvar.key_operate_timeout = 0;
			if(camera_gvar.screen_on == 0) {
				walkie_talkie_send_event(WT_EVT_BACKLIGHT_ON, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL);
				camera_gvar.screen_on = 1;
			}
			break;
		default:
			break;							
	}
#if CALLING_DEMO_DEBUG
	os_printf("walkie_talkie_calling status:%d\n", calling_status);
#endif
#endif
	timer_count++;
}

void lv_time_set() 
{
	static uint32_t user_data = 10;
	lv_timer_create(timer_event, 100,  &user_data);
}

void lv_page_poweron_menu_config()
{
	static lv_style_t poweonMenuStyle;

	lv_style_reset(&poweonMenuStyle);
	lv_style_init(&poweonMenuStyle);
	lv_style_set_width(&poweonMenuStyle, lv_obj_get_width(lv_scr_act()));
	lv_style_set_height(&poweonMenuStyle, lv_obj_get_height(lv_scr_act()));
	lv_style_set_bg_color(&poweonMenuStyle, lv_color_hex(0x000000));	//0x101018
	
	lv_style_set_shadow_color(&poweonMenuStyle, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&poweonMenuStyle, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&poweonMenuStyle, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_width(&poweonMenuStyle, 0);
	lv_style_set_radius(&poweonMenuStyle,0);
	lv_style_set_pad_all(&poweonMenuStyle, 0);
	lv_style_set_pad_gap(&poweonMenuStyle,0);

	ui_poweronPage = lv_obj_create(lv_scr_act());
	lv_obj_add_style(ui_poweronPage, &poweonMenuStyle, 0);
	lv_obj_clear_flag( ui_poweronPage, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
	curPage_obj = ui_poweronPage;
}

void lv_page_poweroff_menu_config()
{
	static lv_style_t poweoffMenuStyle;
    
	lv_style_reset(&poweoffMenuStyle);
	lv_style_init(&poweoffMenuStyle);
	lv_style_set_width(&poweoffMenuStyle, lv_obj_get_width(lv_scr_act()));
	lv_style_set_height(&poweoffMenuStyle, lv_obj_get_height(lv_scr_act()));
	lv_style_set_bg_color(&poweoffMenuStyle, lv_color_hex(0x000000));	//0x101018

	lv_style_set_shadow_color(&poweoffMenuStyle, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&poweoffMenuStyle, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&poweoffMenuStyle, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_width(&poweoffMenuStyle, 0);
	lv_style_set_radius(&poweoffMenuStyle,0);
	lv_style_set_pad_all(&poweoffMenuStyle, 0);
	lv_style_set_pad_gap(&poweoffMenuStyle,0);

	ui_poweroffPage = lv_obj_create(lv_scr_act());
	lv_obj_add_style(ui_poweroffPage, &poweoffMenuStyle, 0);
	lv_obj_clear_flag( ui_poweroffPage, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	curPage_obj = ui_poweroffPage;

	lv_obj_add_event_cb(curPage_obj, event_handler, LV_EVENT_ALL, NULL);

    os_printf("### lv_page_poweroff_menu_config \n\r");
    
	os_sleep_ms(100);
    mcu_reset();

	while(1);
}

void lv_page_init()
{
	USER_KEY_EVENT = lv_event_register_id();
}

void poweron_welcome(void)
{
	lv_page_poweron_menu_config();

	// 设置默认参数
	camera_gvar.welcome_times 		= 10;
	camera_gvar.poweron_nextpage 	= PAGE_INTERCOM;
	camSetParam.volumeSet 			= 3;
	camSetParam.sundtype 			= 0;
	camera_gvar.screen_on           = 1;
	camera_gvar.screen_on_time      = SCREEN_ON_TIME_DEFAULT;

	// 音量设置
	walkie_talkie_send_event(WT_EVT_VOLUME_SET, (uint32_t)camSetParam.volumeSet, (uint32_t)NULL, (uint32_t)NULL);
    
	// 通知外部模块初始化 MSI
    walkie_talkie_send_event(WT_EVT_MSI_INIT, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL);

	walkie_talkie_send_event(WT_EVT_VPP_IPF_INIT, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL);

	// 设置默认视图切换类型并应用
	camSetParam.view_swtichtype = 3;					
	walkie_talkie_send_event(WT_EVT_VIEW_SWITCH, (uint32_t)camSetParam.view_swtichtype, (uint32_t)NULL, (uint32_t)NULL);

	// 解码 Logo
	#if USE_90_DEGREE_LOGO
    walkie_talkie_send_event(WT_EVT_DECODE_LOGO, (uint32_t)ui_bgLogo, sizeof(ui_bgLogo), (uint32_t)NULL);
	#else
	walkie_talkie_send_event(WT_EVT_DECODE_LOGO, (uint32_t)ui_bgLogo_ap, sizeof(ui_bgLogo_ap), (uint32_t)NULL);
	#endif

	// 播放开机提示音
	walkie_talkie_send_event(WT_EVT_PROMPT_TONE_PLAY, (uint32_t)"FLASH:/didi.mp3", (uint32_t)NULL, (uint32_t)NULL);

    walkie_talkie_send_event(WT_EVT_BACKLIGHT_ON, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL);
}


void lv_page_select(uint8_t page)
{
	camera_gvar.page_cur = page;

	os_printf("camera_gvar.page_cur:%d\r\n",page);

	if (page == PAGE_INTERCOM) 
	{	
		if(curPage_obj)
		{
			lv_obj_del(curPage_obj);
			curPage_obj = NULL;
		}
		ui_intercomPage_screen_init();		
	}
	else if(page == PAGE_POWEROFF)
	{
		if(curPage_obj)
		{
			lv_obj_del(curPage_obj);
			curPage_obj = NULL;
		}
		lv_page_poweroff_menu_config();
	}
}

void walkie_talkie_calling_callback(uint32_t local_status)
{
	static uint32_t last_local_status = status_none;
	if(walkie_talkie_send_event(WT_EVT_WIFI_CONNECT_GET, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL) == 0) {
		walkie_talkie_calling_set(calling_stop);
		return;
	}
	if(last_local_status == local_status) {
		return;
	}
	last_local_status = local_status;
	switch(local_status) {
		case status_none:
		{
            if(calling_tone_stream_id >= 0) {
                txmplayer_close(calling_tone_stream_id);
                calling_tone_stream_id = -1;
            }
			if(camera_gvar.calling_connect == 1) {
				intercom_deinit();
#ifdef SYS_APP_WALKIE_TALKIE
				user_protocol_deinit();
#endif
				camera_gvar.calling_connect = 0;
				walkie_talkie_send_event(WT_EVT_DISP_NUM_SET, 0, (uint32_t)NULL, (uint32_t)NULL);
				#if USE_90_DEGREE_LOGO
				extern const unsigned char ui_bgLogo[14217];
				walkie_talkie_send_event(WT_EVT_JPG_DECODE_RUN, (uint32_t)ui_bgLogo, sizeof(ui_bgLogo), NULL);
				#else
				extern const unsigned char ui_bgLogo_ap[14241];
				walkie_talkie_send_event(WT_EVT_JPG_DECODE_RUN, (uint32_t)ui_bgLogo_ap, sizeof(ui_bgLogo_ap), (uint32_t)NULL);
				#endif
			}
			break;
		}
		case wait_connect:
		{
            if(calling_tone_stream_id >= 0) {
                txmplayer_close(calling_tone_stream_id);
                calling_tone_stream_id = -1;
            }
            txmplayer_init(0, 0, NULL);
            calling_tone_stream_id = txmplayer_open("FLASH:/calling1.mp3", 0, NULL);
			break;
		}
		case wait_accept_connect:
		{
            if(calling_tone_stream_id >= 0) {
                txmplayer_close(calling_tone_stream_id);
                calling_tone_stream_id = -1;
            }
            txmplayer_init(0, 0, NULL);
            calling_tone_stream_id = txmplayer_open("FLASH:/calling2.mp3", 0, NULL);
			break;
		}
		case accept_connect:
		{
            if(calling_tone_stream_id >= 0) {
                txmplayer_close(calling_tone_stream_id);
                calling_tone_stream_id = -1;
            }
			break;
		}
		case connecting:
		{
            if(calling_tone_stream_id >= 0) {
                txmplayer_close(calling_tone_stream_id);
                calling_tone_stream_id = -1;
            }
			if(camera_gvar.calling_connect == 0) {
#ifdef SYS_APP_WALKIE_TALKIE
				user_protocol_reinit();
#endif
				intercom_init();
				camera_gvar.calling_connect = 1;
			}
			break;
		}
		case wait_disconnect:
		{
            if(calling_tone_stream_id >= 0) {
                txmplayer_close(calling_tone_stream_id);
                calling_tone_stream_id = -1;
            }
			if(camera_gvar.calling_connect == 1) {
				intercom_deinit();
#ifdef SYS_APP_WALKIE_TALKIE
				user_protocol_deinit();
#endif
				camera_gvar.calling_connect = 0;
				walkie_talkie_send_event(WT_EVT_DISP_NUM_SET, 0, (uint32_t)NULL, (uint32_t)NULL);
				#if USE_90_DEGREE_LOGO
				extern const unsigned char ui_bgLogo[14217];
				walkie_talkie_send_event(WT_EVT_JPG_DECODE_RUN, (uint32_t)ui_bgLogo, sizeof(ui_bgLogo), NULL);
				#else
				extern const unsigned char ui_bgLogo_ap[14241];
				walkie_talkie_send_event(WT_EVT_JPG_DECODE_RUN, (uint32_t)ui_bgLogo_ap, sizeof(ui_bgLogo_ap), (uint32_t)NULL);
				#endif
			}
			break;
		}
		case disconnecting:
		{
            if(calling_tone_stream_id >= 0) {
                txmplayer_close(calling_tone_stream_id);
                calling_tone_stream_id = -1;
            }
			if(camera_gvar.calling_connect == 1) {
				intercom_deinit();
#ifdef SYS_APP_WALKIE_TALKIE
				user_protocol_deinit();
#endif
				camera_gvar.calling_connect = 0;
				walkie_talkie_send_event(WT_EVT_DISP_NUM_SET, 0, (uint32_t)NULL, (uint32_t)NULL);
				#if USE_90_DEGREE_LOGO
				extern const unsigned char ui_bgLogo[14217];
				walkie_talkie_send_event(WT_EVT_JPG_DECODE_RUN, (uint32_t)ui_bgLogo, sizeof(ui_bgLogo), NULL);
				#else
				extern const unsigned char ui_bgLogo_ap[14241];
				walkie_talkie_send_event(WT_EVT_JPG_DECODE_RUN, (uint32_t)ui_bgLogo_ap, sizeof(ui_bgLogo_ap), (uint32_t)NULL);
				#endif
			}
			break;
		}
		default:
			break;
	}
}

void walkie_talkie_demo(void)
{
#if USE_CALLING_DEMO
	walkie_talkie_calling_init((void*)walkie_talkie_calling_callback);
#endif
	set_lvgl_get_key_func(user_key_filter);
	walkie_talkie_msi_ext_init();
	walkie_talkie_send_event(WT_EVT_BAT_DET_INIT, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL);
	lv_page_init();
	poweron_welcome();
	lv_time_set();
	os_printf("## walkie_talkie_demo\n");
}