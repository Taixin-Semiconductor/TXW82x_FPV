#include "basic_include.h"
#include "stream_define.h"
#include "lv_demo_benchmark.h"
#include "keyWork.h"
#include "keyScan.h"
#include "lvgl/lvgl.h"
#include "ui/main_ui.h"
void lv_port_disp_init_msi(const char *name,uint16_t w,uint16_t h,uint8_t rotate);
void lv_port_indev_init(void);
lv_style_t g_style;

#if KEY_MODULE_EN == 1
typedef uint32_t (*lvgl_get_key)(uint32_t key);
static struct os_msgqueue lvgl_key_msgq;
static lvgl_get_key g_lvgl_get_key = NULL;
#endif

void set_lvgl_get_key_func(void *func)
{
	#if KEY_MODULE_EN == 1
	g_lvgl_get_key = (lvgl_get_key)func;
	#endif
}

uint32_t key_get_data()
{
	uint32_t key_ret = 0;
	#if KEY_MODULE_EN == 1
	static uint32_t last_key = 0xff;
	uint32_t val = os_msgq_get(&lvgl_key_msgq,0);
	//如果有按键被按下,就进入判断
	//如果没有按键按下,就判断一下上一次是否为释放按键,如果不是,就执行上一次按键的键值(有可能是长按之类)
	if(g_lvgl_get_key)
	{
		if(val > 0)
		{
			return g_lvgl_get_key(val);
		}
		return key_ret;
	}
	if(val > 0 || !(((last_key&0xff) ==   KEY_EVENT_SUP) || ((last_key&0xff) ==   KEY_EVENT_LUP)))
	{
		if(!(val>0))
		{
			val = last_key;
		}
		last_key = val;
		if((val&0xff) ==   KEY_EVENT_SUP)
		{
				switch(val>>8)
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
					case AD_A:
					break;
					case AD_B:
					break;
					case AD_C:
					break;

					default:
					break;
				}
		}
		else if((val&0xff) ==   KEY_EVENT_REPEAT)
		{
			
				switch(val>>8)
				{
					//lvgl需要持续按键才能识别长按
					case AD_PRESS:
						key_ret = LV_KEY_ENTER;
					break;

					default:
					break;

				}
		}
	}
	
	#endif
	return key_ret;
}

#if KEY_MODULE_EN == 1
uint32_t lvgl_push_key(struct key_callback_list_s *callback_list,uint32_t keyvalue,uint32_t extern_value)
{
	os_msgq_put(&lvgl_key_msgq,keyvalue,0);
	return 0;
}
#endif

void lvgl_run_msi(void *d){
	uint32 cur_tick;
	cur_tick = os_jiffies();
	while(1){
		os_sleep_ms(1);
		lv_tick_inc(os_jiffies()-cur_tick);
		cur_tick = os_jiffies();
        lv_timer_handler();
		if(os_jiffies()-cur_tick > 30)
		{
			os_printf("%s:%d\tuse_time:%d\n",__FUNCTION__,__LINE__,os_jiffies()-cur_tick);
		}
	}
}

void lvgl_init_msi(uint16_t w,uint16_t h,uint8_t rotate)
{
#if KEY_MODULE_EN == 1
	memset(&lvgl_key_msgq,0,sizeof(lvgl_key_msgq));
	os_msgq_init(&lvgl_key_msgq,10);
	add_keycallback(lvgl_push_key,NULL);
#endif
	os_printf("w:%d h:%d rotate:%d\n",w,h,rotate);
	lv_init();                  // lvgl初始化，如果这个没有初始化，那么下面的初始化会崩溃
    lv_port_disp_init_msi(S_LVGL_OSD,w,h,rotate);        // 显示器初始化
    lv_port_indev_init();

	lv_style_reset(&g_style);
	lv_style_init(&g_style);
	lv_style_set_bg_color(&g_style, lv_color_make(0x00, 0x00, 0x00));	
	lv_style_set_shadow_color(&g_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&g_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&g_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_radius(&g_style, 0); 

	// lv_demo_benchmark(1);
	main_ui(NULL);

	os_task_create("gui_thread", lvgl_run_msi, NULL, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 4096);

}


