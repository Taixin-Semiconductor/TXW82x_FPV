#ifndef __KEYSCAN_H
#define __KEYSCAN_H
#include "basic_include.h"
/**************************************************************************
 * 按键的事件
 * 说明:按键触发可能短时间由多个事件,比如长按:先是有个按键按下->长按按下->长按释放
 * 所以要考虑监听哪个事件,比如短按的,最好监听短按释放,毕竟按键按下事件都会有触发的
***************************************************************************/
enum 
{
  KEY_EVENT_DOWN,   //按键按下
  KEY_EVENT_SUP,    //短按释放
  KEY_EVENT_LDOWN,  //长按按下
  KEY_EVENT_LUP,    //长按释放
  KEY_EVENT_REPEAT, //一直按住按键
  KEY_EVENT_MAX,
};

/***************************************
 * 按键值的定义
***************************************/
enum 
{
  KEY_NONE,
  AD_UP,
  AD_DOWN,
  AD_LEFT,
  AD_RIGHT,
  AD_PRESS,
  AD_A,
  AD_B,
  AD_C,
  LOWPOWER_CHECK,
};



struct key_channel_t ;
struct keys_t ;

typedef struct key_channel_t key_channel_t;
typedef void (*keyinit_func) (struct key_channel_t*);
typedef void (*keyprepare_func) (struct key_channel_t*);
typedef uint8 (*keyscan_func) (struct key_channel_t*);
typedef int32 (*key_send_msg) (uint32 data,uint32_t value);
typedef uint32 key_map_t[KEY_EVENT_MAX];
typedef struct keys_t keys_t;



struct keys_t 
{
  uint32_t        period_long;
  uint32_t        period_repeat;
  uint32_t        period_dither;
};

struct key_channel_t 
{
  keyinit_func    init; //初始化
  keyprepare_func prepare;//scan之前的准备,可以设置为NULL，也可以去设置enable值来作为是否要进行按键检测
  keyscan_func    scan; //获取按键键值 
  const void           *key_table;//按键表格,scan需要用到,如果是adc需要根据表格获取是哪个按键被按下
  const keys_t          *key_arg;


  void           *priv;
  key_send_msg    key_send;
  uint32_t        last_key;
  uint32_t        last_time;
  uint32_t        last_event;
  
  uint32_t        extern_value;
  uint8           enable;
};






void key_button_scan_init(key_channel_t  **channel,key_send_msg send);
#endif