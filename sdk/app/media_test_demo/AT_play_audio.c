/***************************************************
    该demo主要是使用AT命令控制播放音频,
	AT命令格式：AT+PLAY_AUDIO=xxx.xxx,mode，
	支持的格式为wav、mp3、amr、aac-lc,
***************************************************/
#include "basic_include.h"
#include "audio_media_ctrl/audio_media_ctrl.h"

int32 atcmd_play_audio(const char *cmd, char *argv[], uint32 argc)
{
	char audio_filePath[20];
	uint8_t play_mode = 0;
	if(argc < 2) {
        os_printf("%s argc err:%d,enter the path and mode\n",__FUNCTION__,argc);
        return 0;
    }

	if(argv[0]) {
		memset(audio_filePath,0,sizeof(audio_filePath));
		memcpy(audio_filePath,argv[0],strlen(argv[0]));
		play_mode = os_atoi(argv[1]);
		os_printf("audio path:%s,mode:%d\n",audio_filePath,play_mode);
		if(play_mode)
			audio_file_play_init(audio_filePath,play_mode);
		else 
			audio_file_play_stop();			
	}
	return 0;
}
