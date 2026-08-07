#include "sys_config.h"
#include "tx_platform.h"
#include "list.h"
#include "typesdef.h"
#include "osal/string.h"
#include "stream_frame.h"
#include "utlist.h"
#include "osal/work.h"
#include "basic_include.h"
static int unbind_stream(stream *s);



typedef void *(*stream_malloc)(int size);
typedef void (*stream_free)(void *ptr);

stream *g_s = NULL;
stream *d_s = NULL;
uint8_t stream_not_gc = 0;

static void *extern_malloc(int size)
{
    return os_malloc(size);
}

static void extern_free(void *ptr)
{
    if (ptr) {
        os_free(ptr);
    }
}


stream_malloc self_malloc = extern_malloc;
stream_free  self_free = extern_free;


static int default_free_data_func(struct data_structure *d)
{
    int res = 0;
    uint32_t flags;
    int ref;
	flags = disable_irq();
    ref = --d->ref;
	enable_irq(flags);

    if(ref < 0)
    {
        os_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    }
    //释放空间
    if(ref == 0)
    {
        //先调用注册的删除函数,后通知流
        if(d->ops && d->ops->free)
        {
            d->ops->free(d);
        }
        res = d->s->func(d->s,d,STREAM_DATA_FREE);
        //返回0,代表不需要删除data,如果返回>0,代表data实体已经无效
        if(res > 0)
        {
            d->data = NULL;
        }
        flags = disable_irq();
        DL_DELETE(d->s->src_d_list_u,d);
        d->prev = NULL;
        d->next = NULL;
        DL_APPEND(d->s->src_d_list_f, d);
        enable_irq(flags);
        d->s->func(d->s,d,STREAM_DATA_FREE_END);
    }
	return 0;
}

static int default_stream_priv_func(stream *s,void *priv,int opcode)
{
    return 0;
}

static void *creat_stream(const char *name)
{
    stream *s = (stream*)self_malloc(sizeof(stream));
    if(!s)
    {
        goto creat_stream_end;
    }
    memset(s,0,sizeof(stream));

    s->name = name;
    s->ref = 0; //代表有人创建了,所以一定是有人在使用
    s->used = STREAM_AVAILABLE;
    s->d_list_f = NULL;
    s->d_list_u = NULL;
    s->ref++;
    
    //往链表头插入,保护
    DL_PREPEND(g_s, s);
    creat_stream_end:
    return s;
}



//查找是否存在对应的流名称
static void *find_stream(const char *name)
{
    stream *s = NULL;
    stream *ret_s = NULL;
    uint32_t flags;
    //保护轮询,并且这个阶段不能进行gc
    stream *each_s = g_s;
    flags = disable_irq();
    stream_not_gc = 1;
    DL_FOREACH(each_s,s) 
    {
        if(!strcmp(s->name,name))
        {
            s->open_ref++;
            s->ref++;
            ret_s = s;
            goto find_stream_end;
        }
    }
    find_stream_end:
    //轮询完毕,可以进行gc操作(gc的时候要全局保护,先对链表进行移动,最后单独进行回收)
    stream_not_gc = 0;
    enable_irq(flags);
    return ret_s;
}









//开放的接口

//对流的data空间进行分配,暂定buf是连续的,后续可以增加buf不连续的接口(避免申请一大片的空间)
void stream_data_mem_dis(stream *s,uint8_t *buf_head,int uint_len,int count)
{
    struct data_structure *elt;
    DL_FOREACH(s->src_d_list_f,elt)
    {
        if(count == 0)
        {
            break;
        }
        elt->data = buf_head;
        buf_head  += uint_len;
        count--;
    }
}

//对data进行分配空间,用回调函数操作,count是分配的最大值
void stream_data_dis_mem(stream *s,int count)
{
    struct data_structure *elt;
    int data_count = 0;
    DL_FOREACH(s->src_d_list_f,elt)
    {
        if(count == 0)
        {
            break;
        }
        elt->priv = (void*)data_count++;
        s->func(s,elt,STREAM_DATA_DIS);
        count--;
    }
}



//对data进行分配空间(自定义),用回调函数操作
void stream_data_dis_mem_custom(stream *s)
{
    struct data_structure *elt;
    int data_count = 0;
    DL_FOREACH(s->src_d_list_f,elt)
    {
        elt->priv = (void*)data_count++;
        s->func(s,elt,STREAM_DATA_DIS);
    }
}


//强行引用加1,特殊作用,适合一次性占用多次,并且不需要经过find_stream的流程,前提流是可用的
int open_stream_again(stream *s)
{
    uint32_t flags;
    int res = 0;
    flags = disable_irq();
    if(s->used == STREAM_ISUED)
    {
        s->open_ref++;
         s->ref++;
    }
    else
    {
        res = 1;
    }
    enable_irq(flags);
    return 0;
}

//打开一个可用的流,如果没有,则创建一个,如果已经存在,则使用原来的,则参数除了name,其他无效
void *open_stream_available(const char *name,int data_count,int recv_count,stream_priv_func func,void *priv)
{
    uint32_t flags;
    stream *s = NULL;
    //检查是否已经创建过了
    s = find_stream(name);
    if(s)
    {
        //判断一下状态,如果是在使用,则代表被创建过,直接返回
        if(s->used == STREAM_ISUED)
        {
            goto open_stream_available_end;
        }
        else
        {
            flags = disable_irq();
            s->open_ref--;
            s->ref--;
            enable_irq(flags);
        }
    }

    s = (stream *)open_stream(name,data_count,recv_count,func,priv);
open_stream_available_end:
    return s;
}

//打开对应的流,如果需要生产data,则data_count>0,如果只是接收,recv_count>0,根据需求来填写
void *open_stream(const char *name,int data_count,int recv_count,stream_priv_func func,void *priv)
{
    stream *s = NULL;
    void *src_d_list_head = NULL;
    void *d_list_head = NULL;
    uint32_t flags;


    if(func)
    {
        func(NULL,priv,STREAM_OPEN_ENTER);
    }


    //先申请对应的空间
    if(data_count > 0)
    {
        src_d_list_head = (void*)self_malloc(data_count*sizeof(struct data_structure));
        if(src_d_list_head)
        {
            memset(src_d_list_head,0,data_count*sizeof(struct data_structure));
        }
    }
    

    if(recv_count > 0)
    {
        d_list_head = (void*)self_malloc(recv_count*sizeof(struct data_list));
        if(d_list_head)
        {
            memset(d_list_head,0,recv_count*sizeof(struct data_list));
        }
    }
    

    if((data_count>0 && !src_d_list_head) || (recv_count > 0 && !d_list_head))
    {
        if(src_d_list_head)
        {
            self_free(src_d_list_head);
        }

        if(d_list_head)
        {
            self_free(d_list_head);
        }
        src_d_list_head = NULL;
        d_list_head = NULL;
        goto open_stream_end;
    }


        
    //检查是否已经创建过了
    s = find_stream(name);
    if(s)
    {
        //stream不是可用或者不是重复可用,则返回空,代表名称有了,可能没有及时回收,需要等待及时回收才可以创建同一个名字的流
        if(s->used != STREAM_AVAILABLE_AGAIN && s->used != STREAM_AVAILABLE)
        {
            flags = disable_irq();
            s->open_ref--;
            s->ref--;
            enable_irq(flags);
            s = NULL;
            goto open_stream_end;
        }


        flags = disable_irq();
        //没有被使用,则初始化对应数据
        s->used = STREAM_ISUED;
        enable_irq(flags);
    }
    //没有找到对应的流,则创建
    else
    {
        //这是第一次创建,不需要锁
        s = creat_stream(name);
        if(s)
        {
            s->open_ref++;
            s->used = STREAM_ISUED;
        } 
    }


    //创建对应data_list_f数量,相当于初始化
    struct data_structure *tmp_data_s = (struct data_structure *)src_d_list_head;
    for(int i=0;i<data_count;i++)
    {
        DL_APPEND(s->src_d_list_f, tmp_data_s);
        
        tmp_data_s->free = default_free_data_func;
        tmp_data_s->s = s;
        tmp_data_s++;
    }

    struct data_list *tmp_data_list = (struct data_list *)d_list_head;
    for(int i=0;i<recv_count;i++)
    {
        DL_APPEND(s->d_list_f, tmp_data_list);
        tmp_data_list++;
    }



    open_stream_end:
    
    if(s)
    {
        s->src_d_list_count = data_count;
        s->src_d_list_head = src_d_list_head;
        s->d_list_head = d_list_head;
        if(func)
        {
            s->func = func;
            func(s,priv,STREAM_OPEN_EXIT);
        }
        else
        {
            s->func = default_stream_priv_func;
        }
    }
    else
    {
        //如果没有流,看看是否申请了空间
        if(src_d_list_head)
        {
            self_free(src_d_list_head);
        }

        if(d_list_head)
        {
            self_free(src_d_list_head);
        }

        if(func)
        {
            func(s,priv,STREAM_OPEN_FAIL);
        }
    }
    return s;
}

int close_stream(stream *s)
{
    if(!s)
    {
        return 1;
    }
    uint32_t flags;
    uint32_t ref;
    flags = disable_irq();
    ref = --s->open_ref;
    enable_irq(flags);
    //如果流还有被打开过,则需要等待另一个占用被释放
    if(ref)
    {
        flags = disable_irq();
        s->ref--;   //理论这里不可能等于0
        enable_irq(flags);
        return 2;
    }

    struct data_list *elt,*tmp;
    s->enable = 0;  //关闭使能
    s->used = STREAM_UNAVAILABLE;    //代表不可使用
    s->func(s,NULL,STREAM_CLOSE_ENTER);
    
    //释放一下自己接收到的数据链表内容,然后它就没有接收的数据了
    DL_FOREACH_SAFE(s->d_list_u,elt,tmp) {
      //从已有链表释放
      DL_DELETE(s->d_list_u,elt);

      //进行elt->data删除
      free_data(elt->data);

      //放回到空闲链表
      elt->prev = NULL;
      elt->next = NULL;
      flags = disable_irq();
      DL_APPEND(s->d_list_f,elt);
      enable_irq(flags);
    }
    
    s->func(s,NULL,STREAM_CLOSE_EXIT);
    s->ref--;
    s->used = STREAM_WAIT_GC_MARKER;   //等待gc标记处理

    //统计src_d_list_f是否与count一致(如果不一致,可能存在悬空态,但未实际发送到流,正常不应该出现这个情况),实际是判断src_d_list_u是否还有被使用,如果有,代表还不能释放

    //剩下交给gc去处理而不是在这里处理


#if 0
    //还有人在引用或者data的数据还在被使用,则不能删除
    if(s->ref || s->src_d_list_u)
    {
        //不能被删除,则由gc去定时去检查处理,一直等到处理完成才进行删除操作
    }
    else
    {
        //释放链表
        if(s->src_d_list_head)
        {
            self_free(s->src_d_list_head);
        }
        
        if(s->d_list_head)
        {
            self_free(s->d_list_head);
        }
        //used设置为0,则可以重新创建,因为对应的空间已经被删除完毕
        s->used = 0;

        //最后可以等到gc自己去删除,而不是主动删除,gc删除可能会延时一段时间,为了统一删除而不会怕有太多异步的可能性
    }
#endif
	return 0;
}




//内部使用解锁绑定
static int unbind_stream(stream *s)
{
    if(!s)
    {
        return 1;
    }
    uint32_t flags;
    os_printf("%s name:%s\tref:%d\n",__FUNCTION__,s->name,s->ref);
    flags = disable_irq();
    s->ref--;   //理论这里不可能等于0
    enable_irq(flags);
    

	return 0;
}



//s为数据产生的源头,然后绑定对应的name
int streamSrc_bind_streamDest(stream *s,const char *name)
{
    int res = -1;
    stream *dest = NULL;
    struct stream_list *s_list;
    dest = find_stream(name);
    if(dest)
    {
        //保证是可用的
        if(dest->used >= 0)
        {
            s_list = (struct stream_list *)self_malloc(sizeof(struct stream_list));
            if(s_list)
            {
                memset(s_list,0,sizeof(struct stream_list));
                s_list->s = dest;
                DL_APPEND(s->s_list, s_list);
                res = 0;
            }
            else
            {
                dest->ref--;
            }
        }
        else
        {
            dest->ref--;
        }
        dest->open_ref--;
    }
    //如果name不存在,则创建一个空的stream s
    else
    {
        dest = creat_stream(name);
        if(dest)
        {
            s_list = (struct stream_list *)self_malloc(sizeof(struct stream_list));
            if(s_list)
            {
                memset(s_list,0,sizeof(struct stream_list));
                s_list->s = dest;
                DL_APPEND(s->s_list, s_list);
                res = 0;
            }
        }
    }
    return res;
}


//解绑定
int streamSrc_unbind_streamDest(stream *s,const char *name)
{
    int res = -1;
    struct stream_list *elt,*tmp;
    stream *dest = NULL;
    dest = find_stream(name);
    if(dest)
    {
        DL_FOREACH_SAFE(s->s_list,elt,tmp) 
        {
            if(dest == elt->s)
            {
                DL_DELETE(s->s_list,elt);
                dest->ref--;
                self_free(elt);
                res = 0;
                break;
            }
        }

        uint32_t flags;
        flags = disable_irq();
        //find_stream的时候ref增加了,所以要--
        dest->ref--;
        dest->open_ref--;
        enable_irq(flags);

    }
    else
    {
    }

    return res;
}


//向stream中发送data数据,会轮询s_list链表发送
int send_data_to_stream(struct data_structure *data)
{
    struct stream_list *elt;
    struct data_list *d_list,*tmp;
    int count = 0;  //计算发送到多少个数据流
    uint32_t flags;
    data->ref++;
    flags = disable_irq();
    //放到已经使用的链表中
    DL_APPEND(data->s->src_d_list_u, data);
    enable_irq(flags);

    data->s->func(data->s,data,STREAM_SEND_DATA_START);
    DL_FOREACH(data->s->s_list,elt) 
    {

        if(elt->s->enable && elt->s->used == STREAM_ISUED)
        {
            //先判断一下该数据是否要发送到流,如果不需要,则不需要进行往下操作
            if(elt->s->func(elt->s,data,STREAM_FILTER_DATA))
            {
                continue;
            }

            d_list = NULL;
            //获取elt->s中可以使用的数据链表,如果没有则不发送过去,代表对方空间不够
            flags = disable_irq();
            DL_FOREACH_SAFE(elt->s->d_list_f,d_list,tmp)
            {
                DL_DELETE(elt->s->d_list_f,d_list);
                break;
            }
            enable_irq(flags);

            

            if(d_list)
            {
                data->s->func(data->s,data,STREAM_SEND_TO_DEST);
                count++;
                data->ref++;
                d_list->data = data;



                //链表的next与prev清空
                d_list->next = NULL;
                d_list->prev = NULL;



                //发送到可用的链表
                flags = disable_irq();
                DL_APPEND(elt->s->d_list_u, d_list);
                enable_irq(flags);
                //调用对应唤醒的接口
                elt->s->func(elt->s,data,STREAM_RECV_DATA_FINISH);
            }

        }
    }
    //告诉流,发送数据完成
    data->s->func(data->s,data,STREAM_SEND_DATA_FINISH);
    //进行删除,应该是调用回调函数
    if(data->free)
    {
        data->free(data);
    }
    return count;
}



//强行进行data删除,可能已经不再需要
int force_del_data(struct data_structure *data)
{
    if(!data)
    {
        return 0;
    }
    uint32_t flags;
    data->ref++;

    //放到已经使用的链表中
    flags = disable_irq();
    DL_APPEND(data->s->src_d_list_u, data);
    enable_irq(flags);


    //进行删除,应该是调用回调函数
    if(data->free)
    {
        data->free(data);
    }
    return 0;
}




//获取data的空间链表,用于生成数据
struct data_structure  *get_src_data_f(stream *s)
{
    struct data_structure  *data,*tmp;
    uint32_t flags;
    data = NULL;
    DL_FOREACH_SAFE(s->src_d_list_f,data,tmp)
    {
        flags = disable_irq();
        DL_DELETE(s->src_d_list_f,data);
        enable_irq(flags);
        data->prev = NULL;
        data->next = NULL;
        data->type = 0;
        break;
    }
    return data;
}

//接收data的链表,用于从其他流接收到数据
struct data_structure  *recv_real_data(stream *s)
{

    if(!s)
    {
        return NULL;
    }
    uint32_t flags;
    struct data_list  *data,*tmp;
    struct data_structure *real_data = NULL;
    data = NULL;
 
    DL_FOREACH_SAFE(s->d_list_u,data,tmp)
    {
        //从used中移除
        flags = disable_irq();
        DL_DELETE(s->d_list_u,data);
        enable_irq(flags);
        data->prev = NULL;
        data->next = NULL;

        if(!s->func(s,data->data,STREAM_RECV_FILTER_DATA))
        {
            break;
        }
        //代表该数据包需要被过滤
        else
        {
            flags = disable_irq();
            DL_APPEND(s->d_list_f, data);
            enable_irq(flags);
            //删除对应的data
            free_data(data->data);
            data = NULL;
        }
        
    }

    if(data)
    {

        real_data = data->data;

        //放回到d_list_f
        flags = disable_irq();
        DL_APPEND(s->d_list_f, data);
        enable_irq(flags);

    }
    return real_data;
}




int free_data(void *data_struct)
{
    struct data_structure *data = (struct data_structure*)data_struct;
    if(data && data->free)
    {
        return data->free(data);
    }
    return 0;
}




//使能stream
int enable_stream(stream *s,int enable)
{
    if(!s)
    {
        return 1;
    }
    s->enable = enable;
    return 0;
}

//如果有注册获取get_data函数,则由get_data去获取,否则直接返回data->data的指针
void *get_stream_real_data(struct data_structure* data)
{
    if(data &&  data->ops && data->ops->get_data)
    {
        return data->ops->get_data(data);
    }
    else
    {
        return data->data;
    }
}

uint32_t get_stream_real_data_len(struct data_structure* data)
{
    if(data && data->ops && data->ops->get_data_len)
    {
        return data->ops->get_data_len(data);
    }
    else
    {
        return 0;
    }
}


uint32_t get_stream_data_timestamp(struct data_structure* data)
{
    if(data && data->ops && data->ops->get_data_time)
    {
        return data->ops->get_data_time(data);
    }
    else
    {
        return data->timestamp;
    }
}




uint32_t set_stream_real_data_len(struct data_structure* data,uint32_t len)
{
    if(data && data->ops && data->ops->set_data_len)
    {
        return data->ops->set_data_len(data,len);
    }
    else
    {
        return 0;
    }
}


uint32_t set_stream_data_time(struct data_structure* data,uint32_t timestamp)
{
    if(data && data->ops && data->ops->set_data_time)
    {
        return data->ops->set_data_time(data,timestamp);
    }
    else
    {
        data->timestamp = timestamp; //默认
        return 0;
    }
}


uint32_t stream_data_custom_cmd_func(struct data_structure* data,int opcode,void *priv)
{
    if(data && data->ops && data->ops->custom_func)
    {
        return data->ops->custom_func(data,opcode,priv);
    }
    else
    {
        return 0;
    }
}

uint32_t register_stream_self_cmd_func(stream *s,stream_self_cmd func)
{
    s->self_cmd_func = func;
    return 0;
}


uint32_t stream_self_cmd_func(stream *s,int opcode,uint32_t arg)
{
    if(s && s->self_cmd_func)
    {
        return s->self_cmd_func(s,opcode,arg);
    }
    //没有对应的函数
    return 1;
}



void broadcast_cmd_to_destStream(stream *s,uint32_t cmd)
{
    struct stream_list *elt;
    DL_FOREACH(s->s_list,elt) 
    {

        if(elt->s->enable && elt->s->used == STREAM_ISUED)
        {
            elt->s->func(elt->s,(void*)cmd,STREAM_SEND_CMD);
        }
    } 
}


int stream_gc_marker_timer(uint32_t t)
{
    stream *s;
    stream *each_s = g_s;
    //对链表进行轮询,检查是否要删除,如果需要删除,则进行标记,在另一个workqueue进行回收,与这个是同一个线程(这样不用考虑一些)
    DL_FOREACH(each_s,s) 
    {
        if(s->used == STREAM_WAIT_GC_CLEAN)
        {
            continue;
        }

        //如果被标记或者ref为0,则会自动启动gc处理
        if(s->used == STREAM_WAIT_GC_MARKER || s->ref == 0)
        {

            //删除自己bind的stream

            if(s->s_list)
            {
                struct stream_list *elt,*tmp;
                DL_FOREACH_SAFE(s->s_list,elt,tmp) 
                {
                    DL_DELETE(s->s_list,elt);
                    //close_stream(elt->s);
                    unbind_stream(elt->s);
                    //elt->s->ref--;
                    self_free(elt);
                }
                s->s_list = NULL;
            }

            //要删除,但是看一下对应的资源是否释放完成
            //统计src_d_list_f是否与count一致(如果不一致,可能存在悬空态,
            //但未实际发送到流,正常不应该出现这个情况),实际是判断src_d_list_u是否还有被使用,如果有,代表还不能释放

            if(!s->src_d_list_u)
            {

                //轮询src_d_list_f,查看是否有需要释放空间的可能
                struct data_structure *data_tmp;
                DL_FOREACH(s->src_d_list_f,data_tmp)
                {
                    s->func(s,data_tmp,STREAM_DATA_DESTORY);
                }
                //可以进行资源释放
                if(s->src_d_list_head)
                {
                    self_free(s->src_d_list_head);
                    s->src_d_list_head = NULL;
                }

                if(s->d_list_head)
                {
                    self_free(s->d_list_head);
                    s->d_list_head = NULL;
                }
                s->src_d_list_f = NULL;
                s->src_d_list_u = NULL;
                s->d_list_f = NULL;
                s->d_list_u = NULL;
                //状态修改
                //代表资源相关资源被释放过,后续可以重复利用
                s->used = STREAM_AVAILABLE_AGAIN;
                //如果引用为0,则代表这个流不需要继续存在,可以被删除
                if(!s->ref)
                {
                    s->used = STREAM_WAIT_GC_CLEAN;   //交给下一个阶段的gc进行链表移位,删除
                    //如果这里将name设置为NULL,则可以在被清除之前也可以创建数据,但是有一个问题就是,尽量不要用类似全局,因为可能会推迟释放资源
                    os_printf("%s wait gc name:%s\t%X\n",__FUNCTION__,s->name,s);
                    s->name = NULL;
                }
            }
        }
    }
    return t;
}

//返回下一次回收的时间,如果没有回收成功,则会10ms后重新进行回收
int stream_gc_clean_timer(uint32_t t)
{
    static uint32_t gc_timer = 100;

    //不能进行回收,等待下一个轮回
    if(stream_not_gc)
    {
        gc_timer = 10;
        goto stream_gc_clean_timer_end;
    }
    uint32_t flags;
    stream *el,*tmp;
    //可以进行回收,要进行全局保护
    flags = disable_irq();
    DL_FOREACH_SAFE(g_s,el,tmp)
    {
        //只是进行链表移位,最后才进行实际删除
        if(el->used == STREAM_WAIT_GC_CLEAN)
        {
            DL_DELETE(g_s,el);
            el->next = NULL;
            el->prev = NULL;
            DL_PREPEND(d_s, el);
        }
    }
    enable_irq(flags);

    //链表进行回收
    DL_FOREACH_SAFE(d_s,el,tmp)
    {
        //name已经被清除,在marker阶段被设置为NULL,所以name暂时不要用申请的空间,用静态(在flash)
        DL_DELETE(d_s,el);
        //进行资源的全部回收
        //回调删除命令
        if(el->func)
        {
            el->func(el,NULL,STREAM_DEL); 
        }
        self_free(el);
    }
    gc_timer = t;

stream_gc_clean_timer_end:
    return gc_timer;
}


static struct os_work gc_marker;
static struct os_work gc_clean;


static int32 stream_gc_marker_timer_work(struct os_work *work)
{
    uint32_t t;
    t = stream_gc_marker_timer(5);
    os_run_work_delay(&gc_marker, t);
	return 0;
}

static int32 stream_gc_clean_timer_work(struct os_work *work)
{
    uint32_t t;
    t = stream_gc_clean_timer(5);
    os_run_work_delay(&gc_clean, t);
	return 0;
}


void stream_work_queue_start()
{
    OS_WORK_INIT(&gc_marker, stream_gc_marker_timer_work, 0);
    os_run_work_delay(&gc_marker, 1000);


    OS_WORK_INIT(&gc_clean, stream_gc_clean_timer_work, 0);
    os_run_work_delay(&gc_clean, 1000);
}
