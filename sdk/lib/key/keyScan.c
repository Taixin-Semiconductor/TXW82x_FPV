#include "keyScan.h"

/*************************************************************************************
 *                             按键扫描判断的timer事件
 *                             最终调用key_scan_init来注册事件
 * 
 * 
**************************************************************************************/
static void g_key_scan(key_channel_t  *ch)
{
    const keys_t *arg = ch->key_arg;
    uint8 key = ch->scan(ch);
    uint8 key_code = KEY_EVENT_MAX;
    uint8 key_event = KEY_EVENT_MAX;

    //分析按键情况
    //按键可能被释放或者说原本就是没有被按下
    if(key == 0)
    {
        //按键没有被按下,无需去管理
        if(ch->last_key == 0)
        {
            goto g_key_scan_end;
        }

        //按键从被按下到释放,反馈事件,有两种可能:短按释放,长按释放,异常情况则是(消抖没有完成就被释放)
        else
        {   

            //异常事件,正常不可能存在,检查程序,可能存在按键抖动情况,不作任何判断
            if(ch->last_event == KEY_EVENT_MAX)
            {
                //printf("%s:%d err,last key:%d,now key:%d\tlast_event:%d\n",__FUNCTION__,__LINE__,ch->last_key,key,ch->last_event );
                //printf("last time:%d\tnow time:%d\n",ch->last_time,os_jiffies());
            }
            else if(ch->last_event  == KEY_EVENT_LDOWN || ch->last_event  == KEY_EVENT_REPEAT)
            {
                ch->last_event  = KEY_EVENT_MAX;
                key_event = KEY_EVENT_LUP;
                key_code = ch->last_key;
            }
            else if(ch->last_event  == KEY_EVENT_DOWN)
            {
                ch->last_event  = KEY_EVENT_MAX;
                key_event = KEY_EVENT_SUP;
                key_code = ch->last_key;
            }
            ch->last_key = key;
        }
    }

    //有多种情况,key与last key不一样,分两种情况,按键抖动(相隔的时间太短),则以后面的key为准,并且时间清0,另一种则是按键检测的时候,错过了up的事件(因为用软定时,有可能错过,这个时候应该发出一个up事件,但也有可能按键接触不良,可以先发出up事件,key当作没有,等待下次的key再去判断)
    else
    {   

        //同样按键,判断时间超出按键抖动范围,如果是,则发出被按下事件
        if(key == ch->last_key)
        {
            //上一次没有事件,则是消抖时间完成,发出按键按下事件
            if(ch->last_event  == KEY_EVENT_MAX)
            {
                if(os_jiffies() - ch->last_time > arg->period_dither) 
                {
                    ch->last_event  = KEY_EVENT_DOWN;
                    key_event = KEY_EVENT_DOWN;
                    key_code = ch->last_key;
                    //无需更新时间,下一次则进入去判断是否长按
                    ch->last_time = os_jiffies();
                    
                }
                //消抖时间没有完成
                else
                {

                }
            }
            //上一次已经有down的事件,则需要继续判断后续的事件(是否为长按或者重复按键)
            else
            {

                //短按,超过长按的时间,发出长按被按下的事件
                if(ch->last_event  == KEY_EVENT_DOWN )
                {
                    if(os_jiffies() - ch->last_time > arg->period_long)
                    {
                        //长按事件被按下
                        ch->last_event  = KEY_EVENT_LDOWN;
                        key_event = KEY_EVENT_LDOWN;
                        key_code = ch->last_key;
                        //用于判断是否是重复按键
                        ch->last_time = os_jiffies();
                    }
                    //不是长按,还在等待时间
                    else
                    {

                    }
                }
                else
                {
                    if(ch->last_event  == KEY_EVENT_LDOWN || ch->last_event  == KEY_EVENT_REPEAT)
                    {
                        if(os_jiffies() - ch->last_time > arg->period_repeat)
                        {
                            ch->last_event  = KEY_EVENT_REPEAT;
                            key_event = KEY_EVENT_REPEAT;
                            key_code = ch->last_key;
                            //用于判断下一次判断
                            ch->last_time = os_jiffies(); 
                        }
                    }
                    else 
                    {
                        os_printf("%s:%d err,last key:%d,now key:%d\tlast_event:%d\n",__FUNCTION__,__LINE__,ch->last_key,key,ch->last_event );
                    }
                }
            }
        }

        //刚被按下,则记录时间,等待抖动结束
        if(!ch->last_key)
        {
            ch->last_time = os_jiffies();
            ch->last_key = key;
        }

        //两次按键不一致,那么就要去识别事件,如果last_event没有事件,代表消抖没有完成,则重新计算时间,如果消抖完成,则需要发出up事件,last_key = 0
        else if(ch->last_key != key)
        {
            //消抖没有完成,更新按键的值
            if(ch->last_event  == KEY_EVENT_MAX)
            {
                ch->last_time = os_jiffies();
                ch->last_key = key;
            }
            //消抖完成,发出up事件,然后last_key设置为0,等待下次按键判断
            else
            {
                if(ch->last_event  == KEY_EVENT_LDOWN || ch->last_event  == KEY_EVENT_REPEAT)
                {
                    ch->last_event  = KEY_EVENT_MAX;
                    key_event = KEY_EVENT_LUP;
                    key_code = ch->last_key;
                }
                else if(ch->last_event  == KEY_EVENT_DOWN)
                {
                    ch->last_event  = KEY_EVENT_MAX;
                    key_event = KEY_EVENT_SUP;
                    key_code = ch->last_key;
                    ch->last_time = os_jiffies();
                }
                ch->last_key = 0;
            }
        }
    }
g_key_scan_end:
    if(key_event != KEY_EVENT_MAX)
    {
       // key_map_t *map = ch->key_table_map;
        //uint32 map_msg = map[key_code][key_event];
        uint32 map_msg = (key_code<<8 | key_event);
        if(map_msg && ch->key_send)
        {  
            ch->key_send(map_msg,ch->extern_value);
        }
    }
}


//timer事件
static void key_auto_timer_cb(void *arg)
{
    key_channel_t  **channel = (key_channel_t  **)arg;
    key_channel_t **ch;
    for (ch = &channel[0]; *ch; ++ch) 
    {
        if(!(*ch)->enable)
        {
            continue;
        }

        if((*ch)->prepare)
        {
            (*ch)->prepare(*ch);
        }
        if((*ch)->scan)
        {
            g_key_scan(*ch);
        }
    }
}




static os_timer_t  key_timer;
//先初始化,然后注册timer,注册按键的映射表,最后启动timer
void key_button_scan_init(key_channel_t  **channel,key_send_msg send)
{
    key_channel_t  **ch = channel;
    for (; *ch; ++ch) 
    {
        (*ch)->last_event  = KEY_EVENT_MAX;
        (*ch)->last_key    = KEY_EVENT_MAX;
        (*ch)->last_time   = os_jiffies();
        (*ch)->key_send = send;
        if((*ch)->init)
        {
            (*ch)->init(*ch);
        }
    }
    os_timer_init(&key_timer, key_auto_timer_cb, OS_TIMER_MODE_PERIODIC, (void*)channel);
    os_timer_start(&key_timer, 5);
}


