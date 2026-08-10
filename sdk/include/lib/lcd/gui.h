#ifndef _GUI_H_
#define _GUI_H_


typedef struct
{     
	uint32_t dvp_w;
	uint32_t dvp_h;
	uint32_t uvc_w;
	uint32_t uvc_h;
	uint32_t photo_w;
	uint32_t photo_h;
	uint32_t rec_w;
	uint32_t rec_h;	
	uint8_t  sound_en;
	uint8_t  iso_en;
	uint8_t  cycle_rec_en;
	uint8_t  take_photo_num;
	uint8_t  enlarge_lcd;     //10 == double
}gui_msg;






extern gui_msg gui_cfg;



#endif
