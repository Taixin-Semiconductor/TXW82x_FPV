#ifndef __AVI_H
#define __AVI_H
#include "typesdef.h"
#include "stream_frame.h"
typedef struct {          
    uint32_t dwFourCC;     // 必须为'avih'
    uint32_t dwSize;       // 本数据结构的大小，不包括最初的8个字节(dwFourCC和dwSize两个字段)   
    uint32_t dwMicroSecPerFrame;       // 每帧的持续时间(以毫秒ms为单位)，定义avi的显示速率
    uint32_t dwMaxBytesPerSec;         // 最大的数据传输率
    uint32_t dwPaddingGranularity;     // 数据填充的粒度
    uint32_t dwFlages;                 // AVI文件的特殊属性，如是否包含索引块，音视频数据是否交叉存储
    uint32_t dwTotalFrame;             // 文件中的总帧数
    uint32_t dwInitialFrames;          // 说明在开始播放前需要多少桢
    uint32_t dwStreams;                // 文件中包含的数据流个数
    uint32_t dwSuggestedBufferSize;    // 建议使用的缓冲区的大小，
                                    // 通常为存储一桢图像以及同步声音所需要的数据之和
    uint32_t dwWidth;                  // 图像宽
    uint32_t dwHeight;                 // 图像高
    uint32_t dwReserved[4];            // 保留值
} AVIMainHeader;


typedef struct {
    uint32_t fccType;                 // 4字节，表示数据流的种类，vids表示视频数据流，auds表示音频数据流
    uint32_t fccHandler;              // 4字节 ，表示数据流解压缩的驱动程序代号
    uint32_t dwFlags;                  // 数据流属性
    uint16_t wPriority;                 // 此数据流的播放优先级
    uint16_t wLanguage;                 // 音频的语言代号
    uint32_t dwInitalFrames;           // 说明在开始播放前需要多少桢
    uint32_t dwScale;                  // 数据量，视频每桢的大小或者音频的采样大小
    uint32_t dwRate;                   // dwScale/dwRate = 每秒的采样数
    uint32_t dwStart;                  // 数据流开始播放的位置，以dwScale为单位
    uint32_t dwLength;                 // 数据流的数据量，以dwScale为单位
    uint32_t dwSuggestedBufferSize;    // 建议缓冲区的大小
    uint32_t dwQuality;                // 解压缩质量参数，值越大，质量越好
    uint32_t dwSampleSize;             // 音频的采样大小
    uint16_t rcFrame[4];                   // 视频图像所占的矩形
}AVIStreamHeader;

//实际标准后面可能还有信息,这里就不解析了
typedef struct tWAVEFORMATEX {
    uint16_t  wFormatTag;
    uint16_t  nChannels;
    uint32_t nSamplesPerSec;
    uint32_t nAvgBytesPerSec;
    uint16_t  nBlockAlign;
    uint16_t  wBitsPerSample;
    uint16_t  cbSize;
} WAVEFORMATEX;

struct avi_list
{
    uint32_t offset;
    uint32_t size;
};

typedef struct avi_list avi_list_audio_s;
typedef struct avi_list avi_list_photo_s;

struct idx1_s
{
    uint32_t dwChunkId;
    uint32_t dwFlags;
    uint32_t dwOffset;
    uint32_t dwSize;
};


//快速索引表
struct fast_index_list
{
    struct fast_index_list *next;
    struct fast_index_list *prev;
    uint32_t avi_index;         //当前视频index的值,这个记录的是视频的帧数
    uint32_t audio_index;       //当前音频index,这个记录的是音频的长度
    uint32_t offset;            //当前的偏移值,读取的第一个index就是对应avi或者index
};
struct avi_msg_s
{
    AVIMainHeader avi;
    void *fp;
    uint32_t fps;
    uint32_t sample;
    uint32_t movi_addr;
    uint32_t idx1_addr;
    uint32_t idx1_size;
    struct avi_read_msg_s *avi_read_msg;
    struct fast_index_list *fast_index;

    //当前读取视频帧的buf
    uint32_t    video_buf[512/4];       //当前读取视频帧的索引的缓冲区
    uint32_t    video_file_addr;        //当前读取视频帧的地址
    int32_t     video_frame_num;        //视频帧数
    uint32_t    video_not_read_size;    //剩余长度
    int32_t    video_cache_remain;     //缓存剩余长度
    struct idx1_s *video_idx1;

    //当前读取音频帧的buf
    uint32_t    audio_buf[512/4];       //当前读取音频帧的索引的缓冲区
    uint32_t    audio_file_addr;        //当前读取音频帧的地址
    int32_t     audio_frame_num;        //音频的读取偏移长度
    uint32_t    audio_not_read_size;    //剩余长度
    int32_t    audio_cache_remain;     //缓存剩余长度
    struct idx1_s *audio_idx1;
    uint8_t offset;
    uint32_t v_id,a_id;
};

struct chunk_s
{
   uint32_t dwFourCC; 
   uint32_t size;
};







struct avi_read_msg_s
{
    uint32_t addr_offset;
    uint32_t avi_index;
    uint32_t audio_index;
    uint32_t remain_read_size;
    uint32_t buf[512/4];
    
};



void *avi_read_init(const char *filename);
void parse_idx1(char *buf,uint32_t size);
void avi_deinit(struct avi_msg_s *avi_msg);
struct avi_read_msg_s *read_msg_init(struct avi_msg_s *avi_msg);
void gen_fast_index_list(struct avi_msg_s *avi_msg);
void read_msg_deinit(struct avi_msg_s *avi_msg);
void locate_avi_index(struct avi_msg_s *avi_msg,uint32_t index);
void avi_read_next_index(struct avi_msg_s *avi_msg,uint32_t *offset,uint32_t *size,uint32_t *frame_num);
void avi_read_next_audio_index(struct avi_msg_s *avi_msg,uint32_t *offset,uint32_t *size,uint32_t *frame_num);
#endif