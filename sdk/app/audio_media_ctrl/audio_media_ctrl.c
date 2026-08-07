#include "basic_include.h"
#include "csi_kernel.h"
#include "fatfs/osal_file.h"
#include "audio_media_ctrl.h"
#include "audio_msi/audio_adc.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "aac/aac_code.h"
#include "amr/amr_decode.h"
#include "mp3/mp3_decode.h"
#include "wave/wave_code.h"

#define RECORD_DIR "0:/audio"
#define MAX(a, b) ((a) > (b) ? a : b)

struct audio_record_struct {
    char filename[30];
    uint8_t record_format;
    int32_t record_time;  //seconds 
    uint8_t is_running;
    uint32_t sampleRate; 
    struct msi *msi; 
};
static struct audio_record_struct *audio_record_s = NULL;

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
    uint8_t status = AUCODEC_EXIT;
    uint32_t count = 0;
    uint32_t start_time = 0;
    AUENC_INIT auenc_init;

    auenc_init.destroy_self = 0;
    auenc_init.src_msi = get_auadc_msi(AUSYS_AUAD);
    os_printf("start record file:%s\n",audio_record_s->filename);
    if(audio_record_s->record_format == WAV) {
        audio_record_s->msi = wave_encode_init(audio_record_s->filename, audio_record_s->sampleRate, &auenc_init);
    }
    else if(audio_record_s->record_format == AAC) {
        audio_record_s->msi = aac_encode_init(audio_record_s->filename, audio_record_s->sampleRate, 1, &auenc_init);
    }
    if(audio_record_s->msi == NULL) {
        os_printf("audio encode init err!\r\n");
        goto audio_record_thread_end;
    } 
    start_time = os_jiffies();
    while(audio_record_s->is_running && (audio_record_s->record_time < 0 || 
                    (os_jiffies()-start_time)/1000 < audio_record_s->record_time)) {
        status = AUCODEC_END;
        msi_do_cmd(audio_record_s->msi, MSI_CMD_AUCODER, MSI_AUCODER_GET_STATUS, (uint32_t)(&status));
		if(status == AUCODEC_END) {
			os_printf("record err!\r\n");
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
    msi_do_cmd(audio_record_s->msi, MSI_CMD_AUCODER, MSI_AUCODER_DEINIT, 1);
    av_free(audio_record_s);
    audio_record_s = NULL;  
    os_printf("audio record thread end!\r\n");  
}

int32_t audio_file_record_pause(void)
{
    int32_t ret = RET_ERR;
    if(audio_record_s && audio_record_s->is_running) {
        ret = msi_do_cmd(audio_record_s->msi, MSI_CMD_AUCODER, MSI_AUCODER_PAUSE, 0);
    }   
    return ret;
}

int32_t audio_file_record_continue(void)
{
    int32_t ret = RET_ERR;
    if(audio_record_s && audio_record_s->is_running) {
        ret = msi_do_cmd(audio_record_s->msi, MSI_CMD_AUCODER, MSI_AUCODER_CONTINUE, 0);
    }  
    return ret; 
}

int32_t audio_file_record_stop(void)
{
    uint32_t count = 0;
    if(audio_record_s) {
        audio_record_s->is_running = 0;
        while(audio_record_s && ((++count) < 1000))
            os_sleep_ms(1);
        return RET_OK;
    }
    return RET_ERR;
}

int32_t audio_file_record_status(struct msi *msi)
{
    int32_t ret = RET_ERR;
    if(audio_record_s && audio_record_s->is_running) {
        msi_do_cmd(audio_record_s->msi, MSI_CMD_AUCODER, MSI_AUCODER_GET_STATUS, (uint32_t)(&ret));
    }  
    return ret;
}

int32_t audio_file_record_init(char *filename, uint32_t sampleRate, int32_t record_time)
{
    uint8_t *file_extension = NULL;

    if(audio_record_s) {
        os_printf("%s err,already recording!\r\n", __FUNCTION__);
        return RET_ERR;
    }
    else {
        audio_record_s = (struct audio_record_struct *)av_zalloc(sizeof(struct audio_record_struct));
        if(!audio_record_s) {
            os_printf("malloc audio_record_s fail!\r\n");
            return RET_ERR;
        }
    }
    os_memset(audio_record_s->filename, 0, sizeof(audio_record_struct));
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
            return RET_ERR;
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
    os_task_create("audio_file_record_thread", audio_file_record_thread, audio_record_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
    return RET_OK;
}

int32_t audio_file_play_pause(struct msi *msi)
{
    int32_t ret = RET_ERR;
    ret = msi_do_cmd(msi, MSI_CMD_AUCODER, MSI_AUCODER_PAUSE, 0); 
    return ret; 
}

int32_t audio_file_play_continue(struct msi *msi)
{
    int32_t ret = RET_ERR;
    ret = msi_do_cmd(msi, MSI_CMD_AUCODER, MSI_AUCODER_CONTINUE, 0);
    return ret; 
}

int32_t audio_file_play_stop(struct msi *msi)
{
    int32_t ret = RET_ERR;
    ret = msi_do_cmd(msi, MSI_CMD_AUCODER, MSI_AUCODER_DEINIT, 0);
    return ret;
}

int32_t audio_file_play_status(struct msi *msi)
{
    int32_t ret = RET_ERR;
    msi_do_cmd(msi, MSI_CMD_AUCODER, MSI_AUCODER_GET_STATUS, (uint32_t)(&ret));
    return ret;
}

struct msi *audio_file_play_init(char *filename, uint8_t play_mode, AUDEC_INIT *audec_init)
{
    uint8_t *file_extension = NULL;
    uint8_t audio_format = 0;
    struct msi *msi = NULL;

    if(filename) {
        file_extension = (uint8_t*)get_file_extension((char*)filename);
        if((os_strncmp(file_extension, "wav", 3)==0) || (os_strncmp(file_extension, "WAV", 3)==0))
            audio_format = WAV;
        else if((os_strncmp(file_extension, "mp3", 3)==0) || (os_strncmp(file_extension, "MP3", 3)==0))
            audio_format = MP3;
        else if((os_strncmp(file_extension, "amr", 3)==0) || (os_strncmp(file_extension, "AMR", 3)==0))
            audio_format = AMR;
        else if((os_strncmp(file_extension, "aac", 3)==0) || (os_strncmp(file_extension, "AAC", 3)==0))
            audio_format = AAC;
        else {
            return NULL;
        }
    }
	else {
		os_printf("enter audio filename\n");
		return NULL;		
	}
    switch(audio_format) {
        case WAV:msi = wave_decode_init(filename, (play_mode==2), audec_init);break;
        case MP3:msi = mp3_decode_init(filename, (play_mode==2), audec_init);break;
        case AMR:msi = amr_decode_init(filename, (play_mode==2), audec_init);break;
        case AAC:msi = aac_decode_init(filename, (play_mode==2), audec_init);break;
        default:break;
    }	
    return msi;
}
