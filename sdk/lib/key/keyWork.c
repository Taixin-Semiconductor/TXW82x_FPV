#include "sys_config.h"
#include "typesdef.h"  
#include "osal/string.h"     
#include "osal/msgqueue.h"
#include "osal/work.h"
#include "osal/string.h"
#include "keyWork.h"
#include "keyScan.h"
#include "adkey.h"
#include "power_checkkey.h"
#include "csi_kernel.h"
/*****************************************
 * 按照不同按键,请注册对应按键进来,参看adkey.c
 * 
 * 
 * 
 * 
 * adkey或者ioKey或者touchkey
 * 添加多个的例子
 * static key_channel_t  *button_channels[] = {&adkey_key,&powercheck_key,NULL};
*****************************************/

static key_channel_t  *button_channels[] = {&adkey_key,NULL};






//添加其他不同类型的按键头文件
static void *g_key_msgq = NULL;
static struct list_head wait_register_list;
static struct list_head register_list;
static uint8_t key_callback_change = 0;




void *add_keycallback(key_callback callback,void *priv)
{
    struct key_callback_list_s *add_list = os_zalloc(sizeof(struct key_callback_list_s));
    if(!add_list)
    {
        return NULL;
    }
    add_list->priv = priv;
    add_list->callback = callback;
    add_list->flag = 0;
    INIT_LIST_HEAD(&add_list->list);

    uint32_t flags;
    os_printf("add_list:%X\n",add_list);
    flags = disable_irq();
    list_add_tail(&add_list->list,  &wait_register_list);
    key_callback_change = 1;
    enable_irq(flags);
    return add_list;
    
}

uint8_t remove_keycallback(struct key_callback_list_s *remove_list)
{
    uint32_t flags;
    flags = disable_irq();
    remove_list->flag = 1;
    key_callback_change = 1;
    enable_irq(flags);
    return 0;
}



//获取按键的workqueue或者任务
static int32 key_read_loop(struct os_work *work)
{
    struct key_msg_s key_msg;
	struct key_callback_list_s *ptr, *next;
    void *msg = g_key_msgq;
    int res;
    uint32_t val = 0;
    uint32_t extern_value = 0;
    //判断一下标志位
    //可能有链表要移除或者新增加callback
    if(key_callback_change)
    {
        uint32_t flags;
        
        flags = disable_irq();
        //先添加,后移除
        list_for_each_entry_safe(ptr, next, &wait_register_list, list) 
        {
            list_del(&ptr->list);
            list_add_tail(&ptr->list,  &register_list);
        }

        list_for_each_entry_safe(ptr, next, &register_list, list) 
        {
            // 对每个节点执行操作
            if(ptr->flag)
            {
                os_printf("del key_callback:%X\n",ptr);
                list_del(&ptr->list);  // 从链表中删除节点
                os_free(ptr);  // 释放节点内存
            }
        }
        key_callback_change = 0;
        enable_irq(flags);
    }
    res = csi_kernel_msgq_get(msg,&key_msg,0);
    if(!res)
    {
        val = key_msg.keycode;
        extern_value = key_msg.extern_value;
        //os_printf("key:%d\tstatus:%d\textern_value:%d\n",val>>8,val&0xff,extern_value);
        list_for_each_entry_safe(ptr, next, &register_list, list) 
        {
            //_os_printf("register ptr:%X\n",ptr);
            if(ptr->callback)
            {
                ptr->callback(ptr,val,extern_value);
            }
        }
    }


    //隔1ms读取一次按键,因为用的是workqueue,不能卡住这个任务,这里的1ms也不是准确的,因为按键,所以可以延时,如果延时太大,要检查
    //workqueue哪个事件卡住导致的
    os_run_work_delay(work, 1);
    return 0;
}





//注册的的消息函数
static int32 key_send_msg_func(uint32_t data,uint32_t extern_value)
{
    struct key_msg_s key_msg = {data,extern_value};
    if(g_key_msgq)
    {
        return csi_kernel_msgq_put(g_key_msgq,&key_msg,0,0);;
    }
    return RET_ERR;
}

//初始化函数
static struct os_work key_wk;




int32 keyWork_init(int listen_max)
{
    //创建key的信号量
    INIT_LIST_HEAD(&wait_register_list);
    INIT_LIST_HEAD(&register_list);
    void *key_msgq = (void*)csi_kernel_msgq_new(listen_max,sizeof(struct key_msg_s));
    if(key_msgq)
    {
        g_key_msgq = key_msgq;
        key_button_scan_init(button_channels,key_send_msg_func);
        //使用workqueue去监听按键?如果没有workqueue就使用创建线程吧
        OS_WORK_INIT(&key_wk, key_read_loop, 0);
        os_run_work_delay(&key_wk, 1);
        return RET_OK;
    }
    return RET_ERR;
}