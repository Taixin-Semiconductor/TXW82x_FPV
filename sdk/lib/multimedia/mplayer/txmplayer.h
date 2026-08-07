#ifndef __TX_MPLAYER_H__
#define __TX_MPLAYER_H__

#include "basic_include.h"
#include "lib/multimedia/framebuff.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif


//编码信息
struct txmplayer_audio_info {
    //TBD...
} ;

struct txmplayer_video_info {
    //TBD...
} ;

struct  txmplayer_picture_info{
    //TBD...
} ;

struct txmplayer_input_ops {
    void (*close)(void *hdl);
    size_t (*read)(void *ptr, size_t size, size_t nmemb, void *hdl);
    int (*seek)(void *hdl, long int offset, int whence);
    int (*eof)(void *hdl);
};

//数据源可能是 文件 或 网络流
//如果是文件，则可以随意执行seek操作，读取数据进行解析
//如果是网络流，执行seek可能失败，container需要应对seek的失败情况 或 不能seek 的情况
//open API的参数size=-1 表示不支持seek操作
typedef struct {
    uint32 type;
    const char *name;

    //初始化cotainer，hdr是 为了识别媒体类型而读取的数据
    //为了避免seek操作，container需要解析并保存 hdr 数据
    void *(*open)(ulong size, void *hdr, uint32 len);

    //关闭container，释放资源
    int32(*close)(void *t);
    
    //由外部调用，seek到指定的时间位置
    int32(*seek)(void *t, uint32 time); 

    //不能阻塞式执行：执行一次只完成一笔数据的解析
    int32(*demux)(void *t, const struct txmplayer_input_ops *ops, void *hdl, void *stream); 
} txmplayer_container;

#define txm_dbg(fmt, ...)   //os_printf("%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define txm_err(fmt, ...)   os_printf(KERN_ERR"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define txm_warn(fmt, ...)  os_printf(KERN_WARNING"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

int32 txmplayer_init(uint8 stream_max, const txmplayer_container *containers, uint32 container_cnt, uint32 stack_size);
int32 txmplayer_open(char *file);
int32 txmplayer_stop(uint8 stream_id);
int32 txmplayer_pause(uint8 stream_id, uint8 pause);
int32 txmplayer_seek(uint8 stream_id, uint32 new_time);
int32 txmplayer_set_speed(uint8 stream_id, MEDIA_PLAY_SPEED speed);
int32 txmplayer_set_volume(uint8 stream_id, uint8 volume);
int32 txmplayer_stream_output(void *stream, struct framebuff *fb);
int32 txmplayer_detect_mtype(const uint8_t *data, uint32_t len);

#endif

