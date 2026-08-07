#ifndef __SPOOK_CONFGIG_H__
#define __SPOOK_CONFGIG_H__

//#include <board_config.h>

#define FRAME_HEAP				20
#define SPOOK_MAX_FRAME_SIZE	0/*352*288*3*/
#define SPOOK_PORT				554//7070//

//#define JPEG_ENCODER_NAME		"jpeg_dvp"
//#define JPEG_ENCODER2_NAME		"jpeg_usb"

#define FILE_JPEG_ENCODER_NAME	"jpeg_file"

#define FILE_H264_ENCODER_NAME	"h264_file"


#define JPEG_FRAMEINC			1
#define H264_FRAMEINC			1

#define AUDIO_ENCODER_NAME		"audio"
#define FILE_AUDIO_ENCODER_NAME		"audio"


#define EXCHANGER_SLOT_SIZE		16

//#define RTSP_LIVE_PATH			"/webcam" /* /webcam */
//#define RTSP_LIVE_USB_PATH		"/webcam2" /* /webcam */
#define RTSP_WEB_PATH			"/file" /* /webcam */


//#define RTSP_LIVE_TRACK			"jpeg_dvp"
//#define RTSP_LIVE2_TRACK		"jpeg_usb"
#define RTSP_WEBFILE_TRACK		"jpeg_file"


#define RTSP_LIVE_AUDIO_TRACK	  "audio"
#define RTSP_WEBFILE_AUDIO_TRACK  "audio_file"



#define HTTP_PATTH				"/webcam" /* /webcam */
#define HTTP_INPUT_NAME			"jpeg"
#define HTTP_MODE				"stream"


#define JPEG_FIFO_SIZE			3

#define SCAN_DATA_OFFSET		0x253//0X265

#define MAX_DATA_PACKET_SIZE	1440

#define SPOOK_CACHE_BUF_LEN			1600
#define SPOOK_CACHE_BUF_HEAD_LEN     (4)

typedef struct 
{
	const char  *video_encode_name;		//视频编码的名称
	const char  *audio_encode_name;		//音频编码的名称
	const char* path;
}rtp_name;


#define H264_ENCODER_NAME "h264_encoder"
#define JPG_ENCODER_NAME "jpg_encoder"
#define AUDIO_AAC_ENCODER_NAME "audio_aac_encoder"




#endif

