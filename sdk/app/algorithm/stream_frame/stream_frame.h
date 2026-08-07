#ifndef __STREAM_FRAME_H
#define __STREAM_FRAME_H
#include "typesdef.h"
#include "stream_define.h"
//定义流的一些函数操作的回调函数,用于不同状态的时候分别通知流
enum
{
    STREAM_OPEN_ENTER,  //准备打开流
    STREAM_OPEN_EXIT,   //打开流完成退出
    STREAM_OPEN_FAIL,   //打开失败

    STREAM_CLOSE_ENTER,  //准备关闭流
    STREAM_CLOSE_EXIT,   //关闭流完成退出
    STREAM_CLOSE_FAIL,   //打开失败

    STREAM_DATA_DIS,    //自定义内存分配(open的时候会调用)
    STREAM_DATA_DESTORY, //自定义内存释放(close的时候调用) 
    STREAM_FILTER_DATA, //在发送的时候,过滤数据包
    STREAM_RECV_FILTER_DATA,    //接收的时候过滤不需要播放的数据包
    STREAM_SEND_DATA_START,   //准备发送数据
    STREAM_SEND_DATA_FINISH,   //发送数据包完成
    STREAM_RECV_DATA_FINISH,
    STREAM_SEND_TO_DEST,
    STREAM_GET_DATA_PRIV,

    STREAM_DATA_FREE,
    STREAM_DATA_FREE_END,

    STREAM_SEND_CMD,    //向流发送命令,可以执行一些特殊操作(如果流不想想响应,则不去响应就行了)
    STREAM_DEL,



    STREAM_DEFAULT_FINISH = 0x80,//定义流默认值最大,0x81-0xff都是用户流自定义
};



enum
{
    STREAM_AVAILABLE,   //流可用
    STREAM_ISUED,       //被使用
    STREAM_UNAVAILABLE,
    STREAM_WAIT_GC_MARKER,
    STREAM_WAIT_GC_CLEAN,
    STREAM_AVAILABLE_AGAIN, //流可重复利用,这个时候可能ref不为0,但是其他资源被释放
};



struct data_structure;
struct data_list;
struct stream_list;
typedef struct stream_s stream;


typedef int (*d_free_func)(struct data_structure *d);
typedef int (*stream_priv_func)(struct stream_s *s,void *priv,int opcode);







typedef void *(*stream_get_data)(void *data);
typedef uint32_t (*stream_get_data_time)(void *data);
typedef uint32_t (*stream_get_data_len)(void *data);
typedef void (*stream_data_free)(void *data);
typedef uint32 (*stream_data_custom_func)(void *data,int opcode,void *priv);
typedef uint32 (*stream_self_cmd)(stream *s,int opcode,uint32_t arg);



typedef uint32_t (*stream_set_data_len)(void *data,uint32_t len);
typedef uint32_t (*stream_set_data_timestamp)(void *data,uint32_t timestamp);

typedef struct 
{
	int 			        type;
    //公用函数
    stream_get_data         get_data;  //获取data实体(不一定是数组可能是一个结构体,看流的定义)
	stream_get_data_len		get_data_len;    //获取data的长度
    stream_set_data_len     set_data_len;
	stream_data_free        free;       //释放data
	stream_get_data_time    get_data_time;   //获取data的time
    stream_set_data_timestamp     set_data_time;

    //用户扩展的函数(可以从data获取到到stream,一般给终端调用)
    stream_data_custom_func custom_func;  


}stream_ops_func;


//数据的结构体
struct data_structure
{
    void *data; //数据的空间,自己申请赋值,可以是一个结构体也可以是一个数组,流自己决定,只是接收方要明确知道该数据的格式(提前协商)
    struct data_structure *next;
    struct data_structure *prev;
    struct stream_s *s;
    //注册函数,通过参数去获取一系列参数
    stream_ops_func *ops;
    void *priv;//私有的结构体
    int type;   //类型,理论每一个stream的数据类型都不是一样的
    d_free_func free; //释放函数
    int ref;    //引用数量
    uint32_t timestamp;
    
};

struct data_list
{
    struct data_structure *data;//数据
    struct data_list *next;     //指针指向下一个
    struct data_list *prev;     //指针指向下一个
    
};


struct stream_list
{
    struct stream_list *next;
    struct stream_list *prev;
    struct stream_s *s;
    int enable; //独立一个开关,可以随时停止发送
};
struct stream_s
{
    struct stream_list *s_list;
    struct stream_s *next;
    struct stream_s *prev;
    void *priv;                 //流自己的私有结构体
    stream_priv_func    func;    //流自己的操作函数,可以自定义操作
    const char *name;           //stream的名称
    int used;                   //是否被使用
    int enable;                 //是否使能
    int ref;                    //引用次数,为0,代表没有人在使用
    int open_ref;               //open成功后,会+1,close一次会-1,知道变成0后,才能实际关闭

    //给stream发送命令(要保证stream没有被提前释放)
    stream_self_cmd         self_cmd_func;  
    //d_free_func free;           //释放data的函数,流自己专有的,释放的时候,先调用这个,再调用默认的


    int src_d_list_count;          //d_list的总数量,open的时候初始化好,直到删除才能被修改
    void *src_d_list_head;      //记录申请的空间头,删除的时候直接free掉
    void *d_list_head;          //记录申请的空间头,删除的时候直接free掉

    //自己data的链表结构体
    struct data_structure *src_d_list_f;   //当是data产生源头的时候,这里可以调用的链表数量
    struct data_structure *src_d_list_u;   //如果被使用,则放在这个链表中,释放的时候是释放到src_d_list_f;
    

    //接收的data链表结构体
    struct data_list *d_list_f;//可用数据链表
    struct data_list *d_list_u;//已有数据链表
};

void *open_stream(const char *name,int data_count,int recv_count,stream_priv_func func,void *priv);
int close_stream(struct stream_s *s);
int streamSrc_bind_streamDest(struct stream_s *s,const char *name);
int streamSrc_unbind_streamDest(struct stream_s *s,const char *name);
int send_data_to_stream(struct data_structure *data);
int enable_stream(struct stream_s *s,int enable);
struct data_structure  *get_src_data_f(struct stream_s *s);
struct data_structure  *recv_real_data(struct stream_s *s);
void *get_real_data(void *data_struct);
int free_data(void *data_struct);
void stream_data_mem_dis(struct stream_s *s,uint8_t *buf_head,int uint_len,int count);
int force_del_data(struct data_structure *data);
void stream_data_dis_mem(struct stream_s *s,int count);
void *get_stream_real_data(struct data_structure* data);
uint32_t get_stream_real_data_len(struct data_structure* data);
void send_stream_cmd(struct stream_s *s,void *data);
uint32_t set_stream_real_data_len(struct data_structure* data,uint32_t len);
void stream_work_queue_start();
uint32_t stream_data_custom_cmd_func(struct data_structure* data,int opcode,void *priv);
uint32_t set_stream_data_time(struct data_structure* data,uint32_t timestamp);
uint32_t get_stream_data_timestamp(struct data_structure* data);
void *open_stream_available(const char *name,int data_count,int recv_count,stream_priv_func func,void *priv);
uint32_t register_stream_self_cmd_func(stream *s,stream_self_cmd func);

uint32_t stream_self_cmd_func(stream *s,int opcode,uint32_t arg);
int open_stream_again(stream *s);
void broadcast_cmd_to_destStream(stream *s,uint32_t cmd);
void stream_data_dis_mem_custom(stream *s);
#endif