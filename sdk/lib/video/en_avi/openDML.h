#ifndef _OPENDML_H_
#define _OPENDML_H_
//include "type.h"
#include "typesdef.h"
#include "osal_file.h"
#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

//最后可改成变量模式
#define FPS 30



/*
#define STRL 0x6c727473
#define STRH 0x68727473
#define STRF 0x66727473
#define VIDS 0x73646976
#define AUDS 0x73647561
#define MJPG 0x47504a4d
#define INDX 0x78646e69
#define DC   0x63643030//00dc
#define WB   0x62773130//01wb
#define VPRP 0x70727076
#define LIST 0x5453494c
#define ODML 0x6c6d646f
#define DMLH 0x686c6d64
#define JUNK 0x4b4e554a

*/
#define AVIF_HASINDEX 			0x10
#define AVIF_MUSTUSEINDEX  		0x20
#define AVIF_ISINTERLEAVED		0x100
#define AVIF_WASCAPTUREFILE	0x10000
#define AVIF_COPYRIGHTED		0x20000
#define AVI_KNOWN_FLAGS 		0x30130
#define AVI_INDEX_OF_INDEXES       0x00
#define AVI_INDEX_OF_CHUNKS        0x01
#define AVI_INDEX_OF_TIMED_CHUNKS  0x02
#define AVI_INDEX_OF_SUB_2FIELD    0x03
#define AVI_INDEX_IS_DATA          0x80
#define FIL_SIZE_4G  			0XF85A2000
#define FIL_SIZE_3G				0XDC758000
#define FIL_SIZE_2G				0X7D000000
#define FIL_SIZE_1G				0X3E800000
#define PCM_SHEET_SIZE          8192
/************** user seting *****************/
//#define SECTOR_SYNC
//#define HEAD_SYNC
#define SYNC_EN 			1
#define FILE_SIZE_MAX_		FIL_SIZE_1G             //size of one file
#define SYNTIME 			10						//how many second sync file
#define FRAME_SIZE  		40*1024				// Probably size every frame(720p)
/*************************************************/
#define SUPERIDXLEN  (FILE_SIZE_MAX_ / (SYNTIME*30*FRAME_SIZE))
#define SupIDXBUFLEN SUPERIDXLEN				//buff len 
#define StdIDXBUFLEN (SYNTIME*30)     
#define _ODML_AVI_HEAD_SIZE__ (((2048 + SUPERIDXLEN*16*2)/1024 +1)*1024)
#define MOVIMAXLEN (1024*1024)*900		//ever AVIX chunk size
#define AVIXMAXNUM  6		

typedef struct
{	
    uint32 win_w;
    uint32 win_h;
    uint32 audiofrq;
    uint32 frame_rate;
    uint32 pcm;
}AVI_INFO;
typedef struct {
	DWORD ckid;
	DWORD dwFlags;
	DWORD dwChunkOffset;
	DWORD dwChunkLength;
} AVIINDEXENTRY;

typedef struct {
  DWORD  fcc;//avih
  DWORD  cb;//size
  DWORD  dwMicroSecPerFrame;
  DWORD  dwMaxBytesPerSec;
  DWORD  dwPaddingGranularity;
  DWORD  dwFlags;
  DWORD  dwTotalFrames;
  DWORD  dwInitialFrames;
  DWORD  dwStreams;
  DWORD  dwSuggestedBufferSize;
  DWORD  dwWidth;
  DWORD  dwHeight;
  DWORD  dwReserved[4];
} AVIMAINHEADER;
#ifndef AVI_STREAM_HEADER
#define AVI_STREAM_HEADER
typedef struct {
DWORD  fcc;//4//strh
DWORD  cb;//4//size
DWORD  fccType;//4//vids|auds
DWORD  fccHandler;//4
DWORD  dwFlags;//4
WORD   wPriority;//2
WORD   wLanguage;//2
DWORD dwInitialFrames;//4
DWORD dwScale;//4
DWORD dwRate; //4/* dwRate / dwScale == samples/second */
DWORD dwStart;//4
DWORD dwLength; //4/* In units above... */
DWORD dwSuggestedBufferSize;//4
DWORD dwQuality;//4
DWORD dwSampleSize;//4
  struct {
    short int left;//2
    short int top;//2
    short int right;//2
    short int bottom;//2
  } rcFrame;

} AVISTREAMHEADER;
#endif
typedef struct{ 
	uint32 rv[34];
}BITMAPAUDIODC;
typedef struct{ 
	uint32 rv[2];
}WAVEDECPR;


typedef struct  {
  DWORD biSize;
  DWORD  biWidth;//4
  DWORD  biHeight;//4
  WORD  biPlanes;
  WORD  biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  DWORD  biXPelsPerMeter;
  DWORD  biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} AVIBITMAPINFOHEADER;


//修改nBlockAlign它的类型，否则ffmpeg识别不了
typedef struct {
  WORD  wFormatTag;
  WORD  nChannels;
  DWORD nSamplesPerSec;
  DWORD nAvgBytesPerSec;
  WORD  nBlockAlign;
  WORD  wBitsPerSample;
  WORD  cbSize;
  
} AVIWAVEFORMATEX;

typedef struct {
	DWORD dwFourCC;
	DWORD dwSize;
} CHUNK;

typedef struct{ 
	DWORD fcc;
	DWORD cb;
	DWORD fcctype;
} LIST;


 struct _avistdindex_chunk   {
uint32 fcc; // Â¡Â¯ix##Â¡Â¯
uint32 cb;
uint16 wLongsPerEntry ; // must be sizeof(aIndex[0])/sizeof(DWORD)
uint8 bIndexSubType  ; // must be 0
uint8 bIndexType  ; // must be AVI_INDEX_OF_CHUNKS
uint32 nEntriesInUse  ; //
uint32 dwChunkId ; // Â¡Â¯##dcÂ¡Â¯ or Â¡Â¯##dbÂ¡Â¯ or Â¡Â¯##wbÂ¡Â¯ etc..

uint32 qwBaseOffset ;  // all dwOffsets in aIndex array are
uint32 qwBaseOffset1 ;  // all dwOffsets in aIndex array are
uint32 dwReserved ;  // all dwOffsets in aIndex array are
// relative to this
//uint32 dwReserved; // must be 0
} __attribute__((packed)) ;
typedef struct _avistdindex_chunk  AVISTDINDEXHEAD;
#ifndef AVI_SUPER_INDEX
#define AVI_SUPER_INDEX
typedef struct _avistdindex_entry {
DWORD dwOffset; // qwBaseOffset + this is absolute file offset
DWORD dwSize; // bit 31 is set if this is NOT a keyframe
} stdindx;
	
	
typedef struct _avisuperindex_entry {
uint64 qwOffset; 	// absolute file offset
DWORD dwSize; 		// size of index chunk at this offset include stdindx head
DWORD dwDuration; 	// time span in stream ticks
} superindx;
#endif	
typedef struct _avisuperindex_chunk {

WORD wLongsPerEntry; // must be 4 (size of each entry in aIndex array)
BYTE bIndexSubType; // must be 0 or AVI_INDEX_2FIELD
BYTE bIndexType; // must be AVI_INDEX_OF_INDEXES
DWORD nEntriesInUse; // number of entries in aIndex array that
// are used
DWORD dwChunkId; // Â¡Â¯##dcÂ¡Â¯ or Â¡Â¯##dbÂ¡Â¯ or Â¡Â¯##wbÂ¡Â¯, etc
DWORD dwReserved[3]; // must be 0
} AVISUPERINDEXHEAD;


typedef struct {
DWORD CompressedBMHeight;
DWORD CompressedBMWidth;
DWORD ValidBMHeight;
DWORD ValidBMWidth;
DWORD ValidBMXOffset;
DWORD ValidBMYOffset;
DWORD VideoXOffsetInT;
DWORD VideoYValidStartLine;
} VIDEO_FIELD_DESC;
typedef struct {

DWORD VideoFormatToken;
DWORD VideoStandard;
DWORD dwVerticalRefreshRate;
DWORD dwHTotalInT;
DWORD dwVTotalInLines;
DWORD dwFrameAspectRatio; 
DWORD dwFrameWidthInPixels;
DWORD dwFrameHeightInLines;
DWORD nbFieldPerFrame;
VIDEO_FIELD_DESC FieldInfo;
} VideoPropHeader;

typedef struct {
DWORD OpenDML_Header;
DWORD size;
DWORD dwTotalFrames; //video total frame + auds frame
DWORD   dwFuture[61];
} ODMLExtendedAVIHeader;


typedef struct {
LIST riff;				//{"RIFF",dwsize,"AVIX "};
LIST movi;			//{"LIST",dwsize,"movi"};
} AVIXMOVILIST;


//		sizeof(LIST)*5+sizeof(AVIMAINHEADER)+sizeof(AVISTREAMHEADER)*2+sizeof(CHUNK)*11+
//sizeof(AVIBITMAPINFOHEADER)+sizeof(AVISUPERINDEXHEAD)*2+sizeof(superindx)*2*SUPERIDXLEN+sizeof(VideoPropHeader)+sizeof(AVIWAVEFORMATEX)+sizeof(ODMLExtendedAVIHeader);

//sizeof(aviheader->strl_a.fcctype)+3*sizeof(CHUNK)+sizeof(AVISTREAMHEADER)+sizeof(AVIWAVEFORMATEX) + 
//sizeof(AVISUPERINDEXHEAD)+SUPERIDXLEN*sizeof(superindx)+367;		
typedef struct {

    LIST riff;				//{"RIFF",dwsize,"AVI "};
    LIST hdrl;			//{"LIST",dwsize,"hdr1"};
    AVIMAINHEADER avih;	//aviheader

    LIST strl_v;						//strl vids
    AVISTREAMHEADER strh_v;			//stream vids
    CHUNK strf_v;					//vids stream info
    AVIBITMAPINFOHEADER bitmapinfo;
    CHUNK sector1_junk;	
    uint8 sector1fill[260];
    //CHUNK strd_v;	
    //BITMAPAUDIODC	bitmapvideodc;
    CHUNK sidx_v;
    AVISUPERINDEXHEAD vsupindxh; 	//superindx head for video
    /**********sector 1*********************/
    superindx vindx[SUPERIDXLEN];			//super indx for video
    CHUNK sector2_junk;
    uint8 sector2fill[512-(SUPERIDXLEN*16)%512-8];
    /**********sector 2*********************/
    CHUNK c_vprp;
    VideoPropHeader vprp;			//Video Properties Header
    CHUNK sector3_junk;
    uint8 sector3fill[428];
    /**********sector 3*********************/
    LIST strl_a;						//strl auds
    AVISTREAMHEADER strh_a;			//stream auds
    CHUNK strf_a;					//auds stream info
    AVIWAVEFORMATEX  wavinfo;	
    CHUNK sector4_junk;
    uint8 sector4fill[368];
    //CHUNK strd_a;	
    //WAVEDECPR	wavedcpr;

    CHUNK sidx_a;
    AVISUPERINDEXHEAD asupindxh;    //superindx head for auds
    /**********sector 4*********************/
    superindx aindx[SUPERIDXLEN];			//super indx for auds
    CHUNK sector5_junk;
    uint8 sector5fill[512-(SUPERIDXLEN*16)%512-8];
    /**********sector 5*********************/
    LIST obml;
    ODMLExtendedAVIHeader strodml;	//Extended AVI Header (dmlh)

    CHUNK junk;	
}  __attribute__((packed)) ODMLAVIFILEHEADER;


#if SYNC_EN
typedef struct {
LIST riff;				//{"RIFF",dwsize,"AVI "};
LIST hdrl;			//{"LIST",dwsize,"hdr1"};
AVIMAINHEADER avih;	//aviheader

LIST strl_v;						//strl vids
AVISTREAMHEADER strh_v;			//stream vids
CHUNK strf_v;					//vids stream info
AVIBITMAPINFOHEADER bitmapinfo;
CHUNK sector1_junk;	
uint8 sector1fill[259];
//CHUNK strd_v;	
//BITMAPAUDIODC	bitmapvideodc;
CHUNK sidx_v;
AVISUPERINDEXHEAD vsupindxh; 	//superindx head for video
}VIDEO_SECTOR;

typedef struct {
LIST strl_a;						//strl auds
AVISTREAMHEADER strh_a;			//stream auds
CHUNK strf_a;					//auds stream info
AVIWAVEFORMATEX  wavinfo;	
CHUNK sector4_junk;
uint8 sector4fill[367];
//CHUNK strd_a;	
//WAVEDECPR	wavedcpr;

CHUNK sidx_a;
AVISUPERINDEXHEAD asupindxh;    //superindx head for auds
/**********sector 4*********************/
}AUDS_SECTOR;

typedef struct {
	superindx vindx[32];
}VIDSUPIND_SECTOR;

typedef struct {
	superindx Aindx[32];
}AUDSUPIND_SECTOR;
typedef struct {
	LIST obml;
	ODMLExtendedAVIHeader strodml;	//Extended AVI Header (dmlh)
	CHUNK junk;	
	uint8 junkfill[_ODML_AVI_HEAD_SIZE__ - (sizeof(ODMLAVIFILEHEADER) + 12)];
	LIST list_movi;
}MOVI_SECTOR;
typedef struct{
uint16				 vsupindx_count;
uint16				 asupindx_count;
uint32 			 vsupidx_sector;
uint32 			 asupidx_sector;
uint32 			 video_sector;
uint32 	 		 auds_sector;
uint32				 movi_sector;
VIDEO_SECTOR  	 video;
AUDS_SECTOR 	 auds;
VIDSUPIND_SECTOR vsupidx;
AUDSUPIND_SECTOR asupidx;
MOVI_SECTOR 	 movi;
}ODMLAVI_SECTOR;
#endif

typedef struct  {
//buff for video and auds's std record the ""indx 

stdindx vstdindx[StdIDXBUFLEN];
stdindx astdindx[StdIDXBUFLEN];
//buff for video and auds's super indx 
superindx vsuperindx[SupIDXBUFLEN];
superindx asuperindx[SupIDXBUFLEN];
//record the "movi" offest that is using
uint64 qwOffset;  
//how many frame in a std indx 
uint32 vidxcount;
uint32 aidxcount;
//how many indx of indx in super indx
uint32 svidxcount;
uint32 saidxcount;
/*
//record how many frame int one stdindx enter arry(not use now)
uint32 vframecount;
uint32 aframecount;
*/
uint64 fileofset_last;//record the offset that end of last movi list(point to new RIFF )
uint64 fileofset;//record the file offset about now
int cur_timestamp; 
int last_timestamp;
int ef_time;    //ever frame time,每一帧的时间,但由于忽略小数,因此存在一定误差
int ef_fps;			//fps,记录帧率,如果需要准确计算时间,则需要通过帧率来计算
uint32 avilisframecnt; //AVI list total frame
uint32 vframecnt; // video frame count total
uint32 aframecnt; // auds frame count total

uint16_t aframeSample;//音频单包的长度,这样不再用宏控制,可以通过初始化的时候修改
int size;	  //size of frame

//use when insert frame 
uint64 inserofset;
uint32 insersize;

//buff for list "movi"  movlist[n] math moviofest[n]
short AVIXnum; //init value is -1 ,when it is 0 that mean have 1 AVIX List
AVIXMOVILIST movlist[AVIXMAXNUM];
uint64 moviofest[AVIXMAXNUM];
//#if SYNC_EN
//ODMLAVI_SECTOR Sync_buff;
uint8_t * sync_buf;
//#endif

} ODMLBUFF;



typedef int (*opendml_write_priv)(void *fp,void *d,int size);

void ODMLbuff_init(ODMLBUFF *buff);
int OMDLvideo_header_write(void *f_write,void *f, AVI_INFO *msg,ODMLAVIFILEHEADER *headerbuf);

void stdindx_updata(void *f,ODMLBUFF *buff);
int ODMLUpdateAVIInfo(void * file,ODMLBUFF *buff,bool Enpcm,void *f_write,ODMLAVIFILEHEADER * headerbuf);

void opendml_write_video(ODMLBUFF *odml_buff,void *fp,void *jpeg,int size);
int opendml_write_video2(ODMLBUFF *odml_buff,void *fp,opendml_write_priv write_priv,int size,void *d);
int insert_frame(ODMLBUFF *buff,void *pavi,float *tim_diff);
int opendml_write_audio(ODMLBUFF *odml_buff,void *fp,opendml_write_priv write_priv,int size,void *d);
bool ODMLadd_indx(bool type,void *f, ODMLBUFF *buff);
#endif /*_OPENDML_H_*/