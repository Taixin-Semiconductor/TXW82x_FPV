#ifndef __RBUFFER_H__
#define __RBUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

#define RB_NPOS(rb, pos, i) ({ \
        uint32 __pos__ = (rb)->pos;\
        ((__pos__+(i)>=(rb)->qsize) ? (__pos__+(i)-(rb)->qsize) : (__pos__+(i))); \
    })

/* rpos == wpos 表示 rbuffer 为空 */
#define RB_EMPTY(rb) ((rb)->wpos == (rb)->rpos)

/* wpos+1 == rpos 表示 rbuffer 已满 */
#define RB_FULL(rb) ({ \
        uint32 _rpos_ = ((rb)->rpos);\
        uint32 _wpos_ = ((rb)->wpos)+1;\
        ((_wpos_>=(rb)->qsize) ? (_wpos_-(rb)->qsize) : (_wpos_)) == (_rpos_);\
    })

/* rbuffer 中未被读取的数据长度 */
#define RB_COUNT(rb) ({ \
        uint32 _rpos_ = ((rb)->rpos);\
        uint32 _wpos_ = ((rb)->wpos);\
        ((_rpos_<=_wpos_)? (_wpos_-_rpos_): ((rb)->qsize-_rpos_+_wpos_));\
    })

/* rbuffer中剩余空间长度 */
#define RB_IDLE(rb) ({ \
        uint32 _rpos_ = ((rb)->rpos);\
        uint32 _wpos_ = ((rb)->wpos);\
        ((_wpos_<_rpos_)? (_rpos_-_wpos_-1): ((rb)->qsize-_wpos_+_rpos_-1));\
    })

/*ringbuffer define*/
#define RBUFFER_DEF(name, type, size) \
    struct {\
        uint32  rpos, wpos, qsize;\
        type rbq[(size)+1];\
    }name

/*ringbuffer define (refference)*/
#define RBUFFER_DEF_R(name, type) \
    struct {\
        uint32  rpos, wpos, qsize;\
        type *rbq;\
    }name

/*get a value from ringbuffer*/
#define RB_GET(rb, val) ({\
        uint8 __ret__ = 0;\
        if(!RB_EMPTY(rb)){\
            val = (rb)->rbq[(rb)->rpos];\
            (rb)->rpos = RB_NPOS((rb), rpos, 1);\
            __ret__ = 1;\
        }\
        __ret__;\
    })

/*set a value into ringbuffer*/
#define RB_SET(rb, val) ({\
        uint8 __ret__ = 0;\
        if(!RB_FULL(rb)){\
            (rb)->rbq[(rb)->wpos] = val;\
            (rb)->wpos = RB_NPOS((rb), wpos, 1);\
            __ret__ = 1;\
        }\
        __ret__;\
    })

#define RB_SET_F(rb, val) ({\
        if(RB_FULL(rb)) (rb)->rpos = RB_NPOS((rb), rpos, 1);\
        (rb)->rbq[(rb)->wpos] = val;\
        (rb)->wpos = RB_NPOS((rb), wpos, 1);\
        1;\
    })

/*get a value from ringbuffer in interrupt*/
#define RB_INT_GET(rb, val) ({\
        uint8 __ret__ = 0;\
        uint32 flag = disable_irq(); \
        if(!RB_EMPTY(rb)){\
            val = (rb)->rbq[(rb)->rpos];\
            (rb)->rpos = RB_NPOS((rb), rpos, 1);\
            __ret__ = 1;\
        }\
        enable_irq(flag);\
        __ret__;\
    })

/*set a value into ringbuffer in interrupt*/
#define RB_INT_SET(rb, val) ({\
        uint8 __ret__ = 0;\
        uint32 flag = disable_irq(); \
        if(!RB_FULL(rb)){\
            (rb)->rbq[(rb)->wpos] = val;\
            (rb)->wpos = RB_NPOS((rb), wpos, 1);\
            __ret__ = 1;\
        }\
        enable_irq(flag);\
        __ret__;\
    })

#define RB_INT_SET_F(rb, val) ({\
        uint32 flag = disable_irq(); \
        if(RB_FULL(rb)) (rb)->rpos = RB_NPOS((rb), rpos, 1);\
        (rb)->rbq[(rb)->wpos] = val;\
        (rb)->wpos = RB_NPOS((rb), wpos, 1);\
        enable_irq(flag);\
        1;\
    })

/*ringbuffer init*/
#define RB_INIT(rb, size) do{\
        (rb)->qsize = (size)+1;\
        (rb)->rpos = 0;\
        (rb)->wpos = 0;\
    } while (0)

/*ringbuffer init (referrence mode)*/
#define RB_INIT_R(rb, size, buff) do{\
        (rb)->qsize = (size);\
        (rb)->rpos = 0;\
        (rb)->wpos = 0;\
        (rb)->rbq  = buff;\
    } while (0)

#define RB_RESET(rb) do{\
        uint32 flag = disable_irq(); \
        (rb)->rpos = 0;\
        (rb)->wpos = 0;\
        enable_irq(flag);\
    }while(0)

#define RB_INIT_ALLOC(rb, size) rbuffer_alloc((struct rbuffer *)(rb), size)
#define RB_FREE(rb)             rbuffer_free((struct rbuffer *)(rb))

struct rbuffer {
    uint32 rpos, wpos, qsize, bpos;
    char *rbq;
};
int32 rbuffer_init(struct rbuffer *rb, uint32 size, void *buff);
int32 rbuffer_set(struct rbuffer *rb, void *data, uint32 length);
int32 rbuffer_set_force(struct rbuffer *rb, void *data, uint32 length);
int32 rbuffer_get(struct rbuffer *rb, void *buff, uint32 size);
void  rbuffer_destroy(struct rbuffer *rb);
void  rbuffer_reset(struct rbuffer *rb);
int32 rbuffer_alloc(struct rbuffer *rb, uint32 size);
void  rbuffer_free(struct rbuffer *rb);

int32 rbuffer_set_block(struct rbuffer *rb, void *data, uint32 length, uint8 block);
int32 rbuffer_get_block(struct rbuffer *rb, void *buff, uint32 size, uint8 *block);

////////////////////////////////////////////////////////////////
// 硬件模块可使用的ringbuffer, 默认仅支持 "1读-1写" 模式
// 如果要支持 "多读-多写" 模式，需要打开 HWRB_IRQ 宏定义: 关中断后访问ringbuffer
// 可支持 2~N 个小buffer乒乓使用，max_size 决定了每个乒乓小buffer的大小
//#define HWRB_IRQ
struct hwrbuffer {
    uint32 rpos, wpos, qsize, ipos;
    uint16 min_size, max_size;
    uint8 *buff;
};

//初始化 ringbuffer:
//  max_size: 可写数据的buffer最大长度，大于此长度，则切分buffer使用
//  min_size: 可写数据的buffer最小长度，小于此长度，则忽略调过此buffer，使用下一块buffer
void hwrbuffer_init(struct hwrbuffer *rb, uint8 *buff, uint32 size, uint16 max_size, uint16 min_size);

//更新写地址 并 获取下一个可写的buffer地址 和 长度
//buff: 需要更新的buffer地址，并输出 下一个可写的buffer地址
//size: 需要更新的buffer长度，并输出 下一个可写的buffer长度
void hwrbuffer_update_wpos(struct hwrbuffer *rb, uint8 **buff, uint32 *size);

//更新读地址
//buff: 更新已读取数据的buffer地址
//size: 更新已读取数据的buffer长度
void hwrbuffer_update_rpos(struct hwrbuffer *rb, uint8 *buff, uint32 size);

#ifdef __cplusplus
}
#endif


#endif
