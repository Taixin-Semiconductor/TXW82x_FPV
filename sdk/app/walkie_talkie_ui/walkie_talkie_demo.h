#ifndef _WALKIE_TALKIE_DEMO_H_
#define _WALKIE_TALKIE_DEMO_H_

#define BABY_UI_MAGICSOUND
// #define DISPLAY_DEBUGINFO_ENABLE
//#define AUTO_POWER_OFF_ENABLE

#ifndef SCREEN_ON_TIME_DEFAULT
#define SCREEN_ON_TIME_DEFAULT   -1
#endif

typedef enum _PAGE_NUM_
{
    PAGE_INTERCOM = 0,
    PAGE_POWEROFF,
    PAGE_MAX
}PAGENUM;

typedef struct
{
    uint8_t volumeSet;
    uint8_t sundtype;
    uint8_t view_swtichtype;
}cam_set_t;
extern cam_set_t camSetParam;

typedef struct
{
    uint8_t welcome_times;
    uint8_t specialeffects_index;
    PAGENUM page_cur ;
    uint8_t poweron_nextpage;
    uint8_t volume_anim_times;
    uint8_t pair_out_times;
    uint8_t pair_success;
    uint8_t calling_connect;
    uint8_t calling_status;
    uint8_t key_operate;
    uint8_t screen_on;
    int32_t key_operate_timeout;
    int32_t screen_on_time;
} camera_global_t;
extern camera_global_t camera_gvar;

extern const unsigned char ui_bgLogo[14217];
extern const unsigned char ui_bgLogo_ap[14241];

extern uint32_t USER_KEY_EVENT;

extern lv_obj_t * curPage_obj;
extern lv_obj_t *ui_intercomPage;
extern lv_obj_t *ui_intercomTopBar;
extern lv_obj_t *ui_intercomBatImg;
extern lv_obj_t *ui_intercomBtmBar;
extern lv_obj_t * ui_dialogPanel; 
extern lv_obj_t * ui_dialogContent;
extern lv_obj_t * ui_volPanel;
extern lv_obj_t * ui_lisglabel;
extern lv_obj_t * ui_volumeLevels[10];
extern lv_obj_t * mic_img;
extern lv_obj_t *ui_signalPanel;
extern lv_obj_t *ui_signalLevels[4];
extern lv_obj_t * ui_pairPanel;
extern lv_obj_t *ui_intercomMagicSoundImg;
extern lv_obj_t * ui_pairTeimlabel;
extern lv_obj_t * ui_speedinfo_id_obj;
extern lv_obj_t * ui_speedinfo_num_obj;

extern const lv_img_dsc_t *ui_imgset_iconMagicSound[];
extern const lv_img_dsc_t *ui_imgset_iconBat[];

LV_IMG_DECLARE(iconBat0); 
LV_IMG_DECLARE(iconBat1); 
LV_IMG_DECLARE(iconBat2); 
LV_IMG_DECLARE(iconBat3); 
LV_IMG_DECLARE(iconBat4); 
LV_IMG_DECLARE(iconSoundRobot);
LV_IMG_DECLARE(iconSoundNormal);
LV_IMG_DECLARE(iconSoundChild);
LV_IMG_DECLARE(iconSoundAlien);
LV_IMG_DECLARE(iconFocusP); 
LV_IMG_DECLARE(iconFocus); 
LV_IMG_DECLARE(mkf30); 
LV_FONT_DECLARE(alifangyuan16);
LV_FONT_DECLARE(alifangyuan28);


void lv_page_select(uint8_t page);
void event_handler(lv_event_t * e);
void ui_intercomPage_screen_init();
void ui_event_intercomPage(lv_event_t * e);
void volDisAnimationStart(void);
void render_vol_level(uint8_t vol);
void mic_img_set_visible(bool visible);
uint32_t user_key_filter(uint32_t val);

extern void set_lvgl_get_key_func(void *func);

#endif
