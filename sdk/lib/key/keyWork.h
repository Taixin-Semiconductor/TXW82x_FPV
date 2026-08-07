#ifndef __KEYWORK_H
#define __KEYWORK_H
#include "sys_config.h"
#include "typesdef.h"  
#include "list.h"
struct key_callback_list_s;


//keyvalue:按键的键值和状态
//extern_value:额外参数,根据不同按键传输数据不一样,比如adkey传的是adc的值,如果是powerkey,可以传输对应的电量值
typedef uint32_t (*key_callback)(struct key_callback_list_s *key_callback_list,uint32_t keyvalue,uint32_t extern_value);

struct key_callback_list_s
{
    struct list_head list;
    key_callback callback;
    void *priv;
    uint8_t flag;   //0正常   1要删除
};

struct key_msg_s
{
    uint32_t keycode;
    uint32_t extern_value;
};

void *add_keycallback(key_callback callback,void *priv);
uint8_t remove_keycallback(struct key_callback_list_s *remove_list);
int32 keyWork_init(int listen_max);
#endif