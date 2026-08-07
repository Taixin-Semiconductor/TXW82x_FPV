#include "basic_include.h"
#include "keyScan.h"
#include "../lvgl.h"
#include "walkie_talkie_demo.h"
#include "ui_vpp_ipf_resource.h"
#include "magic_voice.h"
#include "lib/multimedia/msi.h"
#include "hal/scale.h"
#include "walkie_talkie_ui_event.h"

void user_startPair(void);

lv_style_t intercomPageStyle;
#ifdef DISPLAY_DEBUGINFO_ENABLE
lv_style_t debugstyle;	
#endif

void ui_event_intercomPage(lv_event_t * e){
	uint32_t* key_val = (uint32_t*)e->param;
	lv_event_code_t code = lv_event_get_code(e);

	if(code == USER_KEY_EVENT)
	{
		switch(*key_val)
		{
			case AD_A:    //魔音
#ifdef BABY_UI_MAGICSOUND
			case KEY_MAGIC_SWITCH:
			{
				camSetParam.sundtype++;
				if (camSetParam.sundtype > 3) {
					camSetParam.sundtype = 0;
				}

				printf("##===sundtype=%d \n",camSetParam.sundtype);
				magic_voice_set_type(camSetParam.sundtype);
				
				lv_img_set_src(ui_intercomMagicSoundImg, ui_imgset_iconMagicSound[camSetParam.sundtype]);
			}
			break;
#endif

			case AD_LEFT: 
			case AD_RIGHT:
				camSetParam.view_swtichtype++;
				if(camSetParam.view_swtichtype >3)
					camSetParam.view_swtichtype = 0;	

				printf("##view_switch=%d \r\n",camSetParam.view_swtichtype);

				walkie_talkie_send_event(WT_EVT_VIEW_SWITCH, (uint32_t)camSetParam.view_swtichtype, (uint32_t)NULL, (uint32_t)NULL);

				os_sleep_ms(100);
#if USE_CALLING_DEMO
				if(camera_gvar.calling_connect == 0)
#else
				if(0 == walkie_talkie_send_event(WT_EVT_DISP_NUM_GET, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL))
#endif
				{
					#if USE_90_DEGREE_LOGO
					walkie_talkie_send_event(WT_EVT_JPG_DECODE_RUN, (uint32_t)ui_bgLogo, sizeof(ui_bgLogo), (uint32_t)NULL);
					#else
					walkie_talkie_send_event(WT_EVT_JPG_DECODE_RUN, (uint32_t)ui_bgLogo_ap, sizeof(ui_bgLogo_ap), (uint32_t)NULL);
					#endif
				}
			break;

            case KEY_PAIR:
                if(0 == walkie_talkie_send_event(WT_EVT_PAIRSTATUS_GET, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL))
                	user_startPair();           
            break;

			case KEY_POWEROFF:
				os_printf("## KEY_POWEROFF=\n");
				lv_page_select(PAGE_POWEROFF);
			break;
            
			case AD_DOWN:
				os_printf("## KEY_VOL_DOWN=\n");

				if(camSetParam.volumeSet)
				{
					camSetParam.volumeSet--;
				}
				render_vol_level(camSetParam.volumeSet);

				walkie_talkie_send_event(WT_EVT_VOLUME_SET, (uint32_t)camSetParam.volumeSet, (uint32_t)NULL, (uint32_t)NULL);
				volDisAnimationStart();
				os_printf("## cur_volumeSet=%d \n",camSetParam.volumeSet);
			break;

			case AD_UP:
				os_printf("## KEY_VOL_UP=\n");

				if(camSetParam.volumeSet<10)
				{
					camSetParam.volumeSet ++;
				}
				render_vol_level(camSetParam.volumeSet);

				walkie_talkie_send_event(WT_EVT_VOLUME_SET, (uint32_t)camSetParam.volumeSet, (uint32_t)NULL, (uint32_t)NULL);
				volDisAnimationStart();
				os_printf("## cur_volumeSet=%d \n",camSetParam.volumeSet);
			break;

	        case KEY_IPF_SWITCH:
				os_printf("## KEY_IPF_SWITCH=\n");
				walkie_talkie_send_event(WT_EVT_VPP_IPF_CTRL, (uint32_t)&camera_gvar.specialeffects_index, (uint32_t)NULL, (uint32_t)NULL);
			break;

			case KEY_CALLING_START:
				if(walkie_talkie_send_event(WT_EVT_WIFI_CONNECT_GET, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL)) {
					camera_gvar.calling_status = calling_start;
					walkie_talkie_send_event(WT_EVT_CALLING_SET, (uint32_t)camera_gvar.calling_status, (uint32_t)NULL, (uint32_t)NULL);
				}
			break;

			case KEY_CALLING_STOP:
				camera_gvar.calling_status = calling_stop;
				walkie_talkie_send_event(WT_EVT_CALLING_SET, (uint32_t)camera_gvar.calling_status, (uint32_t)NULL, (uint32_t)NULL);
			break;

			default:
			break;
		}
	}
}


void ui_intercomPage_screen_init()
{
	os_printf("%s %d\n", __FUNCTION__, __LINE__);

	lv_style_reset(&intercomPageStyle);
	lv_style_init(&intercomPageStyle);
	lv_style_set_width(&intercomPageStyle, lv_obj_get_width(lv_scr_act()));
	lv_style_set_height(&intercomPageStyle, lv_obj_get_height(lv_scr_act()));
	lv_style_set_bg_color(&intercomPageStyle, lv_color_hex(0x000000));	
	lv_style_set_shadow_color(&intercomPageStyle, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_width(&intercomPageStyle, 0);
	lv_style_set_radius(&intercomPageStyle, 0);
	lv_style_set_pad_all(&intercomPageStyle, 0);
	lv_style_set_pad_gap(&intercomPageStyle, 0);

	ui_intercomPage = lv_obj_create(lv_scr_act());
	lv_obj_clear_flag(ui_intercomPage, LV_OBJ_FLAG_SCROLLABLE); 
	curPage_obj = ui_intercomPage;
	lv_obj_add_style(ui_intercomPage, &intercomPageStyle, 0);
	lv_obj_set_style_bg_opa(ui_intercomPage, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_add_event_cb(curPage_obj, event_handler, LV_EVENT_ALL, NULL);

#ifdef DISPLAY_DEBUGINFO_ENABLE
	ui_speedinfo_id_obj  = lv_label_create(ui_intercomPage);
	ui_speedinfo_num_obj = lv_label_create(ui_intercomPage);

	lv_style_init(&debugstyle);
	lv_style_set_text_font(&debugstyle, &lv_font_montserrat_18);
	lv_obj_add_style(ui_speedinfo_id_obj, &debugstyle, LV_STATE_DEFAULT);
	lv_obj_align(ui_speedinfo_id_obj, LV_ALIGN_BOTTOM_LEFT,0,0);
	lv_label_set_recolor(ui_speedinfo_id_obj, 1);

	lv_obj_add_style(ui_speedinfo_num_obj, &debugstyle, LV_STATE_DEFAULT);
	lv_obj_align(ui_speedinfo_num_obj, LV_ALIGN_BOTTOM_LEFT, 0, -30);
	lv_label_set_recolor(ui_speedinfo_num_obj, 1);	
#endif

	ui_intercomTopBar = lv_obj_create(ui_intercomPage);
	lv_obj_set_width( ui_intercomTopBar, lv_pct(100));
	lv_obj_set_height( ui_intercomTopBar, lv_pct(12));
	lv_obj_set_align( ui_intercomTopBar, LV_ALIGN_TOP_MID);
	lv_obj_clear_flag( ui_intercomTopBar, LV_OBJ_FLAG_SCROLLABLE);   
	lv_obj_set_style_radius(ui_intercomTopBar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_intercomTopBar, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_bg_opa(ui_intercomTopBar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui_intercomTopBar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui_intercomTopBar, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui_intercomTopBar, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_intercomTopBar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui_intercomTopBar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

	ui_intercomBatImg = lv_img_create(ui_intercomTopBar);
	uint8_t bt_status = walkie_talkie_send_event(WT_EVT_BAT_GET_LEVEL, (uint32_t)NULL, (uint32_t)NULL, (uint32_t)NULL);
	lv_img_set_src(ui_intercomBatImg, ui_imgset_iconBat[bt_status]);
	lv_obj_set_width(ui_intercomBatImg, LV_SIZE_CONTENT); 
	lv_obj_set_height(ui_intercomBatImg, LV_SIZE_CONTENT); 
	lv_obj_set_x(ui_intercomBatImg, 38);
	lv_obj_set_y(ui_intercomBatImg, 0);
	lv_obj_set_align(ui_intercomBatImg, LV_ALIGN_LEFT_MID);
	lv_obj_add_flag(ui_intercomBatImg, LV_OBJ_FLAG_ADV_HITTEST);  
	lv_obj_clear_flag(ui_intercomBatImg, LV_OBJ_FLAG_SCROLLABLE);    

	ui_signalPanel = lv_obj_create(ui_intercomTopBar);
	lv_obj_set_width(ui_signalPanel, 32);
	lv_obj_set_height(ui_signalPanel, 22);
	lv_obj_set_align(ui_signalPanel, LV_ALIGN_LEFT_MID);
	lv_obj_set_flex_flow(ui_signalPanel, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(ui_signalPanel, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
	lv_obj_clear_flag(ui_signalPanel, LV_OBJ_FLAG_SCROLLABLE);    
	lv_obj_set_style_radius(ui_signalPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_signalPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui_signalPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui_signalPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui_signalPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui_signalPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_signalPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui_signalPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_column(ui_signalPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_row(ui_signalPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

	for(uint8_t i = 0; i < 4; i++)
	{
		ui_signalLevels[i]=lv_obj_create(ui_signalPanel);
		lv_obj_set_width(ui_signalLevels[i], 4);
		lv_obj_set_height(ui_signalLevels[i], (20-3*(4-i)));
		lv_obj_set_align(ui_signalLevels[i], LV_ALIGN_CENTER );
		lv_obj_clear_flag(ui_signalLevels[i], LV_OBJ_FLAG_SCROLLABLE);  
		lv_obj_set_style_radius(ui_signalLevels[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
		lv_obj_set_style_bg_color(ui_signalLevels[i], lv_color_hex(0x6D6C6C), LV_PART_MAIN | LV_STATE_DEFAULT);
		lv_obj_set_style_bg_opa(ui_signalLevels[i], 255, LV_PART_MAIN | LV_STATE_DEFAULT);
		lv_obj_set_style_border_width(ui_signalLevels[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	}

    ui_intercomMagicSoundImg = lv_img_create(ui_intercomTopBar);
	lv_img_set_src(ui_intercomMagicSoundImg, ui_imgset_iconMagicSound[0]);
	lv_obj_set_width(ui_intercomMagicSoundImg, LV_SIZE_CONTENT); 
	lv_obj_set_height(ui_intercomMagicSoundImg, LV_SIZE_CONTENT);  
	lv_obj_set_align(ui_intercomMagicSoundImg, LV_ALIGN_RIGHT_MID);
	lv_obj_add_flag(ui_intercomMagicSoundImg, LV_OBJ_FLAG_ADV_HITTEST);  
	lv_obj_clear_flag(ui_intercomMagicSoundImg, LV_OBJ_FLAG_SCROLLABLE);   

	ui_intercomBtmBar = lv_obj_create(ui_intercomPage);
	lv_obj_set_width(ui_intercomBtmBar, lv_pct(100));
	lv_obj_set_height(ui_intercomBtmBar, lv_pct(12));
	lv_obj_set_align(ui_intercomBtmBar, LV_ALIGN_BOTTOM_MID);
	lv_obj_clear_flag(ui_intercomBtmBar, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(ui_intercomBtmBar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_intercomBtmBar, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_bg_opa(ui_intercomBtmBar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui_intercomBtmBar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui_intercomBtmBar, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui_intercomBtmBar, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_intercomBtmBar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui_intercomBtmBar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

	ui_pairPanel = lv_obj_create(ui_intercomPage);
	lv_obj_set_width(ui_pairPanel, 180);
	lv_obj_set_height(ui_pairPanel, 120);
	lv_obj_set_align(ui_pairPanel, LV_ALIGN_CENTER);
	lv_obj_set_flex_flow(ui_pairPanel, LV_FLEX_FLOW_COLUMN_WRAP);
	lv_obj_set_flex_align(ui_pairPanel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_clear_flag(ui_pairPanel, LV_OBJ_FLAG_SCROLLABLE); 
	lv_obj_set_style_bg_color(ui_pairPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui_pairPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui_pairPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui_pairPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui_pairPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_pairPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui_pairPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_row(ui_pairPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_column(ui_pairPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_add_flag(ui_pairPanel, LV_OBJ_FLAG_HIDDEN);

	ui_pairTeimlabel = lv_label_create(ui_pairPanel);
	lv_obj_set_style_text_font(ui_pairTeimlabel, &alifangyuan28, 0);
	lv_label_set_recolor(ui_pairTeimlabel, 1);
	lv_obj_set_style_text_color(ui_pairTeimlabel, lv_color_hex(0xFFFFFF), 0);
	
	ui_dialogPanel = lv_obj_create(ui_intercomPage);
	lv_obj_set_width(ui_dialogPanel, 160);
	lv_obj_set_height(ui_dialogPanel, 120);
	lv_obj_set_align(ui_dialogPanel, LV_ALIGN_CENTER);
	lv_obj_clear_flag(ui_dialogPanel, LV_OBJ_FLAG_SCROLLABLE);  
	lv_obj_set_style_bg_color(ui_dialogPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui_dialogPanel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui_dialogPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui_dialogPanel, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui_dialogPanel, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_dialogPanel, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui_dialogPanel, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_row(ui_dialogPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_column(ui_dialogPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_add_flag(ui_dialogPanel, LV_OBJ_FLAG_HIDDEN ); 
	lv_obj_set_style_text_font(ui_dialogPanel, &alifangyuan16, 0);


	lv_obj_t * ui_dialogTitle = lv_label_create(ui_dialogPanel);
	lv_obj_set_width(ui_dialogTitle, LV_SIZE_CONTENT); 
	lv_obj_set_height(ui_dialogTitle, LV_SIZE_CONTENT);
	lv_obj_set_align(ui_dialogTitle, LV_ALIGN_TOP_LEFT);
	lv_obj_set_style_text_color(ui_dialogTitle, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_text_opa(ui_dialogTitle, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_label_set_text(ui_dialogTitle, "提醒:");

	ui_dialogContent = lv_label_create(ui_dialogPanel);
	lv_obj_set_width(ui_dialogContent, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_dialogContent, LV_SIZE_CONTENT);
	lv_obj_set_align(ui_dialogContent, LV_ALIGN_CENTER);
	lv_obj_set_style_text_color(ui_dialogContent, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_text_opa(ui_dialogContent, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_label_set_recolor(ui_dialogContent, 1);
	lv_label_set_text(ui_dialogContent, "退出会#ff0088 断开连接#");

    lv_obj_t *btn_cancel = lv_btn_create(ui_dialogPanel);
    lv_obj_set_size(btn_cancel, 36, 26);  
	lv_obj_set_align(btn_cancel, LV_ALIGN_BOTTOM_LEFT);

	lv_obj_t *label_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(label_cancel, "取消");
	lv_obj_set_align(label_cancel, LV_ALIGN_CENTER);

    lv_obj_t *btn_confirm = lv_btn_create(ui_dialogPanel);
    lv_obj_set_size(btn_confirm, 36, 26);  
	lv_obj_set_align(btn_confirm, LV_ALIGN_BOTTOM_RIGHT);
	
    lv_obj_t *label_confirm = lv_label_create(btn_confirm);
    lv_label_set_text(label_confirm, "继续");
	lv_obj_set_align(label_confirm, LV_ALIGN_CENTER);


	// 创建麦克风图片对象
    mic_img = lv_img_create(ui_intercomTopBar);
	lv_img_set_src(mic_img, &mkf30);
	lv_obj_set_width(mic_img, LV_SIZE_CONTENT);
	lv_obj_set_height(mic_img, LV_SIZE_CONTENT); 
	lv_obj_set_align(mic_img, LV_ALIGN_CENTER );
	lv_obj_add_flag(mic_img, LV_OBJ_FLAG_ADV_HITTEST); 
	lv_obj_clear_flag(mic_img, LV_OBJ_FLAG_SCROLLABLE); 
    lv_obj_add_flag(mic_img, LV_OBJ_FLAG_HIDDEN);

	ui_volPanel = lv_obj_create(ui_intercomPage);
	lv_obj_set_width(ui_volPanel, 160);
	lv_obj_set_height(ui_volPanel, 120);
	lv_obj_set_align(ui_volPanel, LV_ALIGN_CENTER);
	lv_obj_set_flex_flow(ui_volPanel, LV_FLEX_FLOW_COLUMN_WRAP);
	lv_obj_set_flex_align(ui_volPanel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_clear_flag(ui_volPanel, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(ui_volPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui_volPanel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui_volPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui_volPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui_volPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_volPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui_volPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_row(ui_volPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_column(ui_volPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_add_flag(ui_volPanel, LV_OBJ_FLAG_HIDDEN);

	ui_lisglabel = lv_label_create(ui_volPanel);
	lv_obj_set_style_text_font(ui_lisglabel, &lv_font_montserrat_28, 0);
	lv_obj_set_style_text_color(ui_lisglabel, lv_color_hex(0xFCA702), 0);
	lv_label_set_text(ui_lisglabel, LV_SYMBOL_VOLUME_MAX);

	lv_obj_t * ui_list = lv_obj_create(ui_volPanel);
	lv_obj_set_width(ui_list, 180);
	lv_obj_set_height(ui_list, 40);
	lv_obj_set_align(ui_list, LV_ALIGN_CENTER);
	lv_obj_set_flex_flow(ui_list,LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(ui_list, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_clear_flag(ui_list, LV_OBJ_FLAG_SCROLLABLE); 
	lv_obj_set_style_radius(ui_list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_list, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui_list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui_list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

	for(uint8_t i=0; i<10; i++)
	{
		ui_volumeLevels[i] = lv_obj_create(ui_list);
		lv_obj_set_width(ui_volumeLevels[i], 6);
		lv_obj_set_height(ui_volumeLevels[i], 16);
		lv_obj_set_align(ui_volumeLevels[i], LV_ALIGN_CENTER);
		lv_obj_clear_flag(ui_volumeLevels[i], LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_set_style_radius(ui_volumeLevels[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
		lv_obj_set_style_bg_color(ui_volumeLevels[i], lv_color_hex(0x6D6C6C), LV_PART_MAIN | LV_STATE_DEFAULT);
		lv_obj_set_style_bg_opa(ui_volumeLevels[i], 255, LV_PART_MAIN | LV_STATE_DEFAULT);
		lv_obj_set_style_border_width(ui_volumeLevels[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	}

	if(camSetParam.volumeSet > 10)
		camSetParam.volumeSet = 10;

	render_vol_level(camSetParam.volumeSet);

    camSetParam.view_swtichtype = 0;					

	walkie_talkie_send_event(WT_EVT_VIEW_SWITCH, (uint32_t)camSetParam.view_swtichtype, (uint32_t)NULL, (uint32_t)NULL);
	#if USE_90_DEGREE_LOGO
	walkie_talkie_send_event(WT_EVT_JPG_DECODE_RUN, (uint32_t)ui_bgLogo, sizeof(ui_bgLogo), (uint32_t)NULL);
	#else
	walkie_talkie_send_event(WT_EVT_JPG_DECODE_RUN, (uint32_t)ui_bgLogo_ap, sizeof(ui_bgLogo_ap), (uint32_t)NULL);
	#endif
}
