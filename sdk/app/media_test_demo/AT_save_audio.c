/********************************************************
    该demo主要是使用AT命令控制录制音频,录制默认格式为aac-lc，
*********************************************************/
#include "basic_include.h"
#include "audio_media_ctrl/audio_media_ctrl.h"

int32 atcmd_save_audio(const char *cmd, char *argv[], uint32 argc)
{
	uint32_t record_time = 0;
	uint32_t record_ctl = 0;
	uint32_t samplerate = 0;
	
	if(argc < 3) {
		os_printf("%s argc err:%d,enter the mode,samplerate and time\n",__FUNCTION__,argc);
        return 0;
	}
	record_ctl = os_atoi(argv[0]);
	if(record_ctl) {
		samplerate = os_atoi(argv[1]);
		record_time = os_atoi(argv[2]);
		audio_file_record_init(NULL,samplerate,record_time);	
	}
	else {
		audio_file_record_stop();
	}
    return 0;
}
