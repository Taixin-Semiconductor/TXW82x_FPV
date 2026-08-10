#include "sys_config.h"
#include "basic_include.h"
#include "hal/timer_device.h"
#include "osal/string.h"

struct _list_func
{
    struct _list_func *next;
    int32_t (*func)(void *arg,uint32_t kick_time);
    void *arg;
    uint32_t kick_time;
    volatile uint8_t finish;
};

struct driver_timer_s
{
    void *timer;
    struct _list_func *list;
    uint8_t init;
};



struct driver_timer_s g_driver_timer;


int32_t driver_timer_list_check(uint32 irq_data, uint32 irq_flag)
{
    struct _list_func *node = g_driver_timer.list;
    //上一个没有finish的节点
    struct _list_func *last_node = NULL;
    int32_t ret;
    while(node)
    {
        ret = node->func(node->arg,node->kick_time);
        if(!ret)
        {
            node->finish = 1;
            //如果首个节点一直finish,代表是第一个节点,需要更新头指针
            if(g_driver_timer.list == node)
            {
                g_driver_timer.list = node->next;
            }
            //如果有上一个节点,就把当前node的节点移除
            else if(last_node)
            {
                last_node->next = node->next;
            }
            //不存在上一个节点,不作处理
            else
            {

            }
        }
        //记录上一个节点
        else
        {
            last_node = node;
        }
        node = node->next;
    }

    if(g_driver_timer.list)
    {
        return 1;
    }
    return 0;
}

void driver_timer_cb(uint32 irq_data, uint32 irq_flag)
{

    int32_t ret = driver_timer_list_check(irq_data, irq_flag);
    //关闭timer
    if(!ret)
    {
        struct driver_timer_s *driver_timer = (struct driver_timer_s*)irq_data;
        timer_device_stop(driver_timer->timer);
    }
}

void driver_timer_init()
{
    g_driver_timer.timer = dev_get(HG_SIMTMR5_DEVID);
    if(g_driver_timer.timer)
    {
        timer_device_open(g_driver_timer.timer,TIMER_TYPE_PERIODIC,0);
        g_driver_timer.list = NULL;
        g_driver_timer.init = 1;
    }
    else
    {
        os_printf("driver_timer_init fail\n");
    }
}

void driver_timer_add(int32_t (*func)(void *arg,uint32_t kick_time), void *arg)
{
    struct _list_func node;
    uint32_t flag;
    if(!g_driver_timer.init)
    {
        driver_timer_init();
    }
    
    node.func = func;
    node.arg = arg;
    node.finish = 0;
    node.kick_time = os_jiffies();
    flag = disable_irq();
    node.next = g_driver_timer.list;
    g_driver_timer.list = &node;
    enable_irq(flag);
    //启动timer(有可能重复启动timer,但不会影响)
    timer_device_start(g_driver_timer.timer,DEFAULT_SYS_CLK/10000,driver_timer_cb,(uint32_t)&g_driver_timer);
    while(!node.finish)
    {
        os_sleep_ms(1);
    }
}