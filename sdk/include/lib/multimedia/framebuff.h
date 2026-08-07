#ifndef _TXSEMI_FRAMEBUFF_H_
#define _TXSEMI_FRAMEBUFF_H_
#include "media_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FB_ALLOC(size) os_malloc(size)
#define FB_FREE(fb)    os_free(fb)

enum FRAMEBUFF_MTYPE {
    F_NONE = 0, //无类型
    F_YUV,      //YUV数据
    F_JPG_NODE,      //jpeg
    F_JPG,      //jpeg
    F_JPG_DECODE_MSG,      //jpeg
    F_THUMB_JPG,      //jpeg
    F_H264,      //h264
    F_AUDIO,    //音频
    F_RGB,      //未压缩的rgb
    F_ERGB,     //压缩后的rgb
    F_FILE_T,     //文件类型(一般用于文件保存,仅仅支持普通模式:fb->data+fb->len)
};

//设定framebuff的子类型,用uint16_t去定义吧
enum FRAMEBUFF_STYPE
{
    FSTYPE_NONE = 0,    //无类型
    FSTYPE_NORMAL_THUMB_JPG,
    FSTYPE_OVER_DPI_THUMB_JPG,
    FSTYPE_OVER_DPI_JPG,
    FSTYPE_VIDEO_VPP_DATA0,
    FSTYPE_VIDEO_VPP_DATA1,
    FSTYPE_VIDEO_PRC_DATA,
    FSTYPE_VIDEO_SCALER_DATA,
    FSTYPE_VIDEO_GEN420_DATA,
    FSTYPE_VIDEO_SOFT_DATA,

    
    FSTYPE_YUV_P0,
    FSTYPE_YUV_P1,
    FSTYPE_YUV_P2,
    FSTYPE_YUV_SIM_P0,
    FSTYPE_YUV_SIM_P1,
    FSTYPE_YUV_OTHER,
    FSTYPE_YUV_TAKEPHOTO,
    FSTYPE_JPG_CAMERA0,
    FSTYPE_JPG_CAMERA1,
    FSTYPE_JPG_CAMERA2,
    FSTYPE_JPG_FILE,
    FSTYPE_JPG_GEN420_0,
    FSTYPE_JPG_GEN420_REJPG,
    FSTYPE_USB_CAM0,
    FSTYPE_USB_CAM1,

	FSTYPE_AUDIO_ADC,
    FSTYPE_AUDIO_RAW,
	FSTYPE_AUDIO_AAC_DECODER,
    FSTYPE_AUDIO_ALAW_DECODER,
    FSTYPE_AUDIO_AMR_DECODER,
    FSTYPE_AUDIO_MP3_DECODER,
    FSTYPE_AUDIO_OPUS_DECODER,
    FSTYPE_AUDIO_WAVE_DECODER,
    FSTYPE_AUDIO_USB_MIC,
    FSTYPE_AUDIO_PCM,

    //h264,硬件产生
    FSTYPE_H264_VPP_DATA0,
    FSTYPE_H264_VPP_DATA1,
    FSTYPE_H264_PRC_DATA,
    FSTYPE_H264_SCALER_DATA,
    FSTYPE_H264_GEN420_DATA,
    FSTYPE_H264_SOFT_DATA,

    FSTYPE_SCALE1_DATA,//scale1自行添加类型,这里添加一个demo去实现大分辨率拍照

    //h264软件(比如读取文件)
    FSTYPE_H264_FILE,

    FSYPTE_INVALID = 0x80, //无效的stype,用于特殊值,由应用去特殊使用
    FSTYPE_GEN420_720P,
};

enum FRAMEBUFF_SOURCE{
    FRAMEBUFF_SOURCE_NONE,
    FRAMEBUFF_SOURCE_JPG_GEN420,    //特殊,标记从gen420编码
    FRAMEBUFF_SOURCE_JPG_SCALER,    //特殊,标记从scaler编码


    FRAMEBUFF_SOURCE_FILE,
    FRAMEBUFF_SOURCE_USB,
	FRAMEBUFF_SOURCE_CAMERA0,
    FRAMEBUFF_SOURCE_CAMERA1,
    FRAMEBUFF_SOURCE_CAMERA2,


    
};

struct msi;

/*结构体Size控制在32Byte以内，满足DMA 16 对齐的要求*/
struct framebuff {
    // framebuff片段链表，用于多个framebuff组成一个完整的帧
    struct framebuff *next;

    uint8    *data;         //数据地址: 可以是外部地址，也可以是framebuff自己分配的地址
    uint32    len;          //数据长度
    uint32    time;         //数据时间戳：存差值
    void     *priv;         //私有数据

    uint8     mtype, stype;  //数据类型：main_type + sub_type
    uint8     srcID;         //来源ID : enum FRAMEBUFF_SOURCE
    uint8     datatag;       //数据标签，应用自定义

    atomic8_t users;   //引用计数
    uint8     index;   //fbpool使用    
    uint8     used: 1, pool: 1, clone:1, keyfrm:1, rev: 4; //flags
    struct msi *msi;     //产生此fb的MSI组件
};

/* framebuff 队列 */
struct fbqueue {
    struct os_semaphore sema;
    uint16  reader_max;
    uint16  init: 1, alloc: 1, rev: 14;
    uint32 *readers;
    RBUFFER_DEF_R(rbQ, struct framebuff *);
};

/* framebuff 预分配池
   预分配的framebuff必须处理 MSI_CMD_FREE_FB：
      在MSI_CMD_FREE_FB处理后需要调用 fbpool_put API，而且返回值为 1.
*/
struct fbpool {
    uint8 size, rev1;
    uint16 inited: 1, rev: 15;
    struct framebuff *pool;
};

//创建framebuff
//data != NULL: 表示framebuff关联外部数据，自己不分配数据空间
//data == NULL && size > 0: 表示framebuff需要自己分配数据空间
//data == NULL && size = 0: 表示framebuff不需要数据空间
extern struct framebuff *fb_alloc(uint8 *data, int32 size, uint16 type, struct msi *msi);

//MSI组件接收framebuff后，需要执行fb_get操作，fb计数加1
extern void fb_get(struct framebuff *fb);

//使用新的framebuff引用关联另1个framebuff，组成framebuff链表；fb_old引用计数会加1
extern void fb_ref(struct framebuff *fb_new, struct framebuff *fb_old);

//获取framebuff链表中指定type的数据的第1个节点
extern struct framebuff *fb_find(struct framebuff *fb, uint8 mtype, uint8 stype);

//获取framebuff链表中指定type的数据的总长度
extern uint32 fb_len(struct framebuff *fb,  uint8 mtype, uint8 stype);

//framebuff put操作，引用计数减至0时会释放framebuff资源
extern void fb_put(struct framebuff *fb);

//framebuff 队列初始化，需要指定队列的buffer和大小
extern int32 fbq_init(struct fbqueue *q, uint8 *qbuff, int32 qsize);

//销毁framebuff队列，并释放队列中存放的framebuff
extern int32 fbq_destory(struct fbqueue *q);

//向framebuff队列存入1个framebuff，返回值表示存入是否成功
extern int32 fbq_enqueue(struct fbqueue *q, struct framebuff *fb);

//在framebuff队列中查找或丢弃指定的framebuff
extern int32 fbq_trace(struct fbqueue *q, struct framebuff *fb, int8 discard);

//从framebuff队列中取出1个framebuff
extern struct framebuff *fbq_dequeue(struct fbqueue *q, uint32 tmo_ms);

extern struct framebuff *fbq_dequeue_r(struct fbqueue *q, uint8 reader);

//framebuff预分配池初始化，需要指定预分配池的大小
extern int32 fbpool_init(struct fbpool *pool, uint8 size);

//从预分配池中取出一个framebuff
extern struct framebuff *fbpool_get(struct fbpool *pool, uint16 type, struct msi *msi);

//向预分配池释放一个framebuff
extern int32 fbpool_put(struct fbpool *pool, struct framebuff *fb);

//销毁framebuff预分配池，同时通知各个模块丢弃指定的framebuff
extern int32 fbpool_destroy(struct fbpool *pool);

//framebuff克隆操作，克隆的framebuff会关联原framebuff的大部分信息(但是type会重新修改)
extern struct framebuff *fb_clone(struct framebuff *fb, uint16 type, struct msi *msi);

//根据fb->mtype 识别fb携带的数据为 视频，音频，图片，字幕
extern MEDIA_DATA_CATEGORY fb_category(struct framebuff *fb);

//初始化预分配的framebuff信息，仅在初始化时调用。
//   fbpool   : framebuff 预分配池：struct fbpool* 类型
//   index    : framebuffer的索引，不能超过pool的size
//   buff_addr: framebuffer 预分配的buffer地址
//   buff_size: framebuffer 预分配的buffer大小
//   priv_data: framebuffer 的私有数据
#define FBPOOL_SET_INFO(fbpool, index, buff_addr, buff_size, priv_data) do { \
        (fbpool)->pool[index].data = buff_addr; \
        (fbpool)->pool[index].len  = buff_size; \
        (fbpool)->pool[index].priv = priv_data; \
    }while(0)

#ifdef __cplusplus
}
#endif

#endif

