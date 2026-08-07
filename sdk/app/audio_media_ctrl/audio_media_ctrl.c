#include "basic_include.h"
#include "csi_kernel.h"
#include "fatfs/osal_file.h"
#include "audio_media_ctrl.h"
#include "audio_msi/audio_adc.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

#define RECORD_DIR "0:/audio"
#define MAX(a, b) ((a) > (b) ? a : b)

struct audio_record_struct {
    uint8_t filename[30];
    uint8_t record_format;
    int32_t record_time;  //seconds 
    uint8_t is_running;
    uint32_t sampleRate;  
};
static struct audio_record_struct *audio_record_s = NULL;
struct audio_play_struct {
    uint8_t filename[30];
    uint8_t audio_format;
    uint8_t is_running;
    uint8_t play_mode;
};
static struct audio_play_struct *audio_play_s = NULL;

static char *get_file_extension(char *filename) {
    char *dot = os_strrchr((const char*)filename, '.');
    if (!dot || dot == filename) {
        return filename;
    }
    return dot + 1;
}

static uint32_t a2i(char *str)
{
    uint32_t ret = 0;
    uint32_t indx = 0;
    char str_buf[32];
    memset(str_buf, 0, 32);
    while (str[indx] != '\0')
    {
        if (str[indx] == '.')
            break;
        str_buf[indx] = str[indx];
        indx++;
    }
    indx = 0;
    while (str_buf[indx] != '\0')
    {
        if (str_buf[indx] >= '0' && str_buf[indx] <= '9')
        {
            ret = ret * 10 + str_buf[indx] - '0';
        }
        indx++;
    }
    return ret;
}

static void creat_audio_filename(char *dir_name)
{
    DIR dir;
    FRESULT ret;
    FILINFO finfo;
    int indx = 0;

    ret = f_opendir(&dir, dir_name);
    if (ret != FR_OK)
    {
        f_mkdir(dir_name);
        f_opendir(&dir, dir_name);
    }
    while (1)
    {
        ret = f_readdir(&dir, &finfo);
        if (ret != FR_OK || finfo.fname[0] == 0)
            break;
        indx = MAX(indx, a2i(finfo.fname));
    }
    f_closedir(&dir);
    indx++;
#if DEFAULT_RECORD_FORMAT == RECORD_AAC
    os_sprintf((char*)(audio_record_s->filename), "%s/a%d.%s", dir_name, indx, "aac");
#else
    os_sprintf((char*)(audio_record_s->filename), "%s/a%d.%s", dir_name, indx, "wav");
#endif
}

static void audio_file_record_thread(void *d)
{
    struct msi *msi = NULL;
    uint32_t count = 0;
    uint32_t start_time = 0;

    os_printf("start record file:%s\n",audio_record_s->filename);
    if(audio_record_s->record_format == WAV) {
        auadc_msi_add_output("R_WAVE_ENCODE");
        msi = wave_encode_init(audio_record_s->filename, audio_record_s->sampleRate);
    }
    else if(audio_record_s->record_format == AAC) {
        auadc_msi_add_output("SR_AAC_ENCODE");
        msi = aac_encode_init(audio_record_s->filename, audio_record_s->sampleRate, 1);
    }
    if(msi == NULL) {
        os_printf("audio encode init err!\r\n");
        goto audio_record_thread_end;
    }   
    start_time = os_jiffies();
    while(audio_record_s->is_running && (audio_record_s->record_time < 0 || 
                    (os_jiffies()-start_time)/1000 < audio_record_s->record_time)) {
		if((audio_record_s->record_format == WAV) && (get_wave_encode_status() == AUDIO_STOP)) {
			os_printf("record wave err!\r\n");
			goto audio_record_thread_end;
		}
		else if((audio_record_s->record_format == AAC) && (get_aac_encode_status() == AUDIO_STOP)) {
			os_printf("record aac err!\r\n");
			goto audio_record_thread_end;
		}
        count++;
        if(count % 1000 == 0)
        {
            os_printf("%s\t\trecord time:%d\r\n",__FUNCTION__,os_jiffies()-start_time);
        }
        os_sleep_ms(1);
    }
audio_record_thread_end:
    if(audio_record_s->record_format == WAV) {
        if(wave_encode_deinit() == RET_OK)
            auadc_msi_del_output("R_WAVE_ENCODE");
    }
    else if(audio_record_s->record_format == AAC) {
        if(aac_encode_deinit(1) == RET_OK)
            auadc_msi_del_output("SR_AAC_ENCODE");
    }
    av_free(audio_record_s);
    audio_record_s = NULL;  
    os_printf("audio record thread end!\r\n");  
}

void audio_file_record_stop(void)
{
    uint32_t count = 0;
    if(audio_record_s) {
        audio_record_s->is_running = 0;
        while(audio_record_s && ((++count) < 1000))
            os_sleep_ms(1);
    }
}

void audio_file_record_init(char *filename, uint32_t sampleRate, int32_t record_time)
{
    uint8_t *file_extension = NULL;

    if(audio_record_s) {
        os_printf("%s err,already recording!\r\n", __FUNCTION__);
        return;
    }
    else {
        audio_record_s = (struct audio_record_struct *)av_zalloc(sizeof(struct audio_record_struct));
        if(!audio_record_s) {
            os_printf("malloc audio_record_s fail!\r\n");
            return;
        }
    }
    os_memset(audio_record_s->filename, 0, sizeof(struct audio_record_struct));
    if(filename) {
        file_extension = (uint8_t*)get_file_extension((char*)filename);
        if((os_strncmp(file_extension, "wav", 3)==0) || (os_strncmp(file_extension, "WAV", 3)==0)) {
            audio_record_s->record_format = WAV;
        }
        else if((os_strncmp(file_extension, "aac", 3)==0) || (os_strncmp(file_extension, "AAC", 3)==0)) {
            audio_record_s->record_format = AAC;
        }
        else {
            os_printf("Unsupported audio record format!\r\n");
			av_free(audio_record_s);
			audio_record_s = NULL;
            return;
        }
        os_memcpy(audio_record_s->filename, filename, os_strlen(filename));
    }
    else {
#if DEFAULT_RECORD_FORMAT == RECORD_AAC
        audio_record_s->record_format = AAC;
        creat_audio_filename(RECORD_DIR);
#else
        audio_record_s->record_format = WAV;
        creat_audio_filename(RECORD_DIR);
#endif
    }
    audio_record_s->sampleRate = sampleRate;
    audio_record_s->record_time = record_time;
    audio_record_s->is_running = 1;
    os_task_create("audio_file_record_thread", audio_file_record_thread, NULL, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
}

static void audio_file_play_thread(void *d)
{
    struct msi *msi = NULL;
    int32_t ret = 0;
    
audio_play_again:	
    if(audio_play_s->audio_format == WAV) {
        msi = wave_decode_init(audio_play_s->filename, 1);
    }
    else if(audio_play_s->audio_format == MP3) {
		msi = mp3_decode_init(audio_play_s->filename, 1);
    }
    else if(audio_play_s->audio_format == AMR) {
		msi = amr_decode_init(audio_play_s->filename, 1);
    }
    else if(audio_play_s->audio_format == AAC) {
		msi = aac_decode_init(audio_play_s->filename, 1);
    }
    if(msi == NULL) {
        os_printf("audio decode init err!\r\n");
        goto audio_play_thread_end;
    }
    while(audio_play_s->is_running) {
        if(audio_play_s->audio_format == WAV) {
            ret = get_wave_decode_status();
        }
        else if(audio_play_s->audio_format == MP3) {
            ret = get_mp3_decode_status();
        }
        else if(audio_play_s->audio_format == AMR) {
            ret = get_amr_decode_status();
        }
        else if(audio_play_s->audio_format == AAC) {
            ret = get_aac_decode_status();
        }
        if(ret == AUDIO_STOP) {
            if(audio_play_s->play_mode == NORMAL_PLAY)
                break;
            else if(audio_play_s->play_mode == LOOP_PLAY)
                goto audio_play_again;
        }
        os_sleep_ms(1);
    }
    if((ret != AUDIO_STOP) && (audio_play_s->audio_format == WAV)) {
        wave_decode_deinit();
    }
    else if((ret != AUDIO_STOP) && (audio_play_s->audio_format == MP3)) {
        mp3_decode_deinit();
    }
    else if((ret != AUDIO_STOP) && (audio_play_s->audio_format == AMR)) {
        amr_decode_deinit();
    }  
    else if((ret != AUDIO_STOP) && (audio_play_s->audio_format == AAC)) {
        aac_decode_deinit();
    }  
audio_play_thread_end:
    av_free(audio_play_s);
    audio_play_s = NULL; 
    os_printf("audio play thread end!\r\n");    
}

void audio_file_play_stop(void)
{
    uint32_t count = 0;
    if(audio_play_s) {
        audio_play_s->is_running = 0;
        while(audio_play_s && ((++count) < 2000))
            os_sleep_ms(1);
    }    
}

void audio_file_play_init(char *filename, uint8_t play_mode)
{
    uint8_t *file_extension = NULL;

    if(audio_play_s) {
        os_printf("%s err,already playing\n", __FUNCTION__);
        return;
    }
    else {
        audio_play_s = (struct audio_play_struct *)av_zalloc(sizeof(struct audio_play_struct));
        if(!audio_play_s) {
            os_printf("malloc audio_play_s fail!\r\n");
            return;
        }
    }
    if(filename) {
        file_extension = (uint8_t*)get_file_extension((char*)filename);
        if((os_strncmp(file_extension, "wav", 3)==0) || (os_strncmp(file_extension, "WAV", 3)==0))
            audio_play_s->audio_format = WAV;
        else if((os_strncmp(file_extension, "mp3", 3)==0) || (os_strncmp(file_extension, "MP3", 3)==0))
            audio_play_s->audio_format = MP3;
        else if((os_strncmp(file_extension, "amr", 3)==0) || (os_strncmp(file_extension, "AMR", 3)==0))
            audio_play_s->audio_format = AMR;
        else if((os_strncmp(file_extension, "aac", 3)==0) || (os_strncmp(file_extension, "AAC", 3)==0))
            audio_play_s->audio_format = AAC;
        else {
            os_printf("Unsupported audio play format!\r\n");
			av_free(audio_play_s);
			audio_play_s = NULL;
            return;
        }
        os_memcpy(audio_play_s->filename, filename, os_strlen(filename));
        audio_play_s->play_mode = play_mode;
    }
	else {
		os_printf("enter audio filename\n");
		av_free(audio_play_s);
		audio_play_s = NULL;
		return;		
	}
    audio_play_s->is_running = 1;
    os_task_create("audio_file_play_thread", audio_file_play_thread, NULL, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
}
