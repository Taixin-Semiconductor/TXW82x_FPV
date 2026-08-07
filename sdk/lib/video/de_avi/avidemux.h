#ifndef __AVIDEMUX_H
#define __AVIDEMUX_H
typedef unsigned long DWORD;
//typedef unsigned int uint32_t;
//typedef unsigned int uint32_t;
typedef unsigned short int WORD;
typedef unsigned char BYTE;
//typedef unsigned char uint8_t;


struct avi_msg
{
  uint32_t audio_sample_rate;
  uint32_t video_sample_rate;
};

struct _avisuperindex_entry_cache {
  DWORD dwOffsetl;
  DWORD dwOffseth;
  //超级索引表的数量
  DWORD dwSize;
  //编号起始地址
  DWORD offset; // frames
};

#ifndef AVI_STREAM_HEADER
#define AVI_STREAM_HEADER

typedef struct _avistreamheader {
  DWORD fcc;
  DWORD cb;
  DWORD fccType;
  DWORD fccHandler;
  DWORD dwFlags;
  WORD  wPriority;
  WORD  wLanguage;
  DWORD dwInitialFrames;
  DWORD dwScale;
  DWORD dwRate;
  DWORD dwStart;
  DWORD dwLength;
  DWORD dwSuggestedBufferSize;
  DWORD dwQuality;
  DWORD dwSampleSize;
  struct {
    short int left;
    short int top;
    short int right;
    short int bottom;
  } rcFrame;
} AVISTREAMHEADER;
#endif
struct _aviindex_chunk {
  DWORD fcc;
  DWORD cb;
  WORD  wLongsPerEntry;
  BYTE  bIndexSubType;
  BYTE  bIndexType;
  DWORD nEntriesInUse;
  DWORD dwChunkId;
  DWORD dwBaseOffsetl;
  DWORD dwBaseOffseth;
  DWORD dwReserved;
};

#ifndef AVI_SUPER_INDEX
#define AVI_SUPER_INDEX
struct _avisuperindex_entry {
  DWORD dwOffsetl;
  DWORD dwOffseth;
  DWORD dwSize;
  DWORD dwDuration; // frames
};

struct _avistdindex_entry {
  DWORD dwOffset;
  DWORD dwSize;
};
#endif
struct avistrinfo {
  struct aviinfo *avi;
  uint32_t type;
  DWORD dwScale;        // 來自strh的dwScale，但乘以1000以折算成毫秒
  DWORD dwRate;         // 來自strh的dwRate
  DWORD dwTotalFrames;  // 來自strh的dwLength
  DWORD dwIndexNum;     // 來自indx的dwEntriesInUse,超级索引数量
  DWORD dwIndexBase;    // 指向indx的aIndex[]在文件中的位置
  DWORD dwDuration;     // 要求所有超級索引都指向相同長度的標準索引，這樣可以加速索引計算
  DWORD dwEntryNum;     // 緩存的std_index中的nEntriesInUse,某个超级索引指向的标准索引数量
  DWORD dwEntryBase;    // 緩存的std_index中的dwBaseOffsetl

  uint32_t cur;         // 當前播放點。視頻幀號、或音頻包號
  uint32_t super_idx;   // 當前緩存的標準索引，對應超級索引的哪一項

  DWORD cached_indx_base; // 當前緩存的超級索引扇區地址
  DWORD cached_ixnn_base; // 當前緩存的標準索引扇區地址

  struct _aviindex_chunk chunk;
  struct _avisuperindex_entry_cache *superindex_cache;
  struct _avistdindex_entry    *stindex_cache;
};

struct aviinfo {
  void             *fp;         // 文件句柄
  uint32_t          start_time; // 第一幀播放的起始時間
  uint32_t          strmsk;     // 有效流的掩碼
  uint8_t			speed;
  struct avistrinfo str[2];     // 兩個流
};


struct avifp
{
	void *fp;
	void *thread_handle;
	struct aviinfo *aviinfo_point;
	int running;
};

void *avidemux_parse(void *fp,void *aviinfo_point);
uint32_t avidemux_read_begin(struct avistrinfo *str, uint32_t *base, uint32_t *size);
void  avi_read_thread(void *d);
uint32_t avidemux_read (struct avistrinfo *str, uint8_t *buf, uint32_t len);
void *avidemux_stream (struct aviinfo *avi, uint32_t idx);
uint32_t avidemux_set_cur (struct avistrinfo *str,uint32_t cur);
uint32_t get_avidemux_parse_stream(char *path,struct avi_msg *msg);
void free_aviinfo_point(struct aviinfo *info);
uint32_t read_avi_offset(struct avistrinfo *str,uint32_t *base,uint32_t *size);
uint32_t read_avi_data(struct avistrinfo *str,char *buf,uint32_t size);
uint32_t get_avi_cur(struct avistrinfo *info);







#endif
