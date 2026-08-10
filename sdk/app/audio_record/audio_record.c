#include "basic_include.h"
#include "fatfs/osal_file.h"
#include "audio_msi/audio_adc.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/multimedia/audio.h"
#include "audio_record.h"

#define RECORD_DIR "0:/audio"
#define MAX(a, b) ((a) > (b) ? a : b)

const unsigned char wav_header[] = {  
    'R', 'I', 'F', 'F',      // "RIFF" 标志  
    0, 0, 0, 0,              // 文件长度  
    'W', 'A', 'V', 'E',      // "WAVE" 标志  
    'f', 'm', 't', ' ',      // "fmt" 标志  
    16, 0, 0, 0,             // 过渡字节（不定）  
    0x01, 0x00,              // 格式类别  
    0x01, 0x00,              // 声道数      
    0, 0, 0, 0,              // 采样率  
    0, 0, 0, 0,              // 位速  
    0x01, 0x00,              // 一个采样多声道数据块大小  
    0x10, 0x00,              // 一个采样占的 bit 数  
    'd', 'a', 't', 'a',      // 数据标记符＂data ＂  
    0, 0, 0, 0               // 语音数据的长度，比文件长度小42一般。这个是计算音频播放时长的关键参数~  
};  

typedef struct _riff_chunk {
	uint8_t  ChunkID[4];
	uint32_t ChunkSize;
	uint8_t  Format[4];
} TYPE_RIFF_CHUNK;
typedef struct _fmt_chunk {
	uint8_t  FmtID[4];
	uint32_t FmtSize;
	uint16_t FmtTag;
	uint16_t FmtChannels;
	uint32_t SampleRate;
	uint32_t ByteRate;
	uint16_t BlockAlign;
	uint16_t BitsPerSample;
} TYPE_FMT_CHUNK;
typedef struct _data_chunk {
	uint8_t  DataID[4];
	uint32_t DataSize;
} TYPE_DATA_CHUNK;
typedef struct _wave_head {
	TYPE_RIFF_CHUNK  riff_chunk;
	TYPE_FMT_CHUNK   fmt_chunk;
	TYPE_DATA_CHUNK  data_chunk;
} TYPE_WAVE_HEAD;

struct audio_record_struct {
    char filename[32];
    uint8_t record_format;
    uint8_t is_running;
    uint16_t sampleRate; 
    int32_t record_time;  //seconds 
    uint32_t data_size;
    struct msi *msi; 
    TYPE_WAVE_HEAD *wave_head;
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
    os_memset(str_buf, 0, 32);
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
    void *fp = NULL;
    uint32_t count = 0;
    uint32_t start_time = 0;
    struct msi *audio_msi = NULL;
    struct framebuff *frame_buf = NULL;
    txAudioInfo_t codec_info;
	codec_info.sample_rate = audio_record_s->sampleRate;
	codec_info.channels = 1;

    fp = osal_fopen(audio_record_s->filename, "wb+");
    if(fp == NULL) {
        goto audio_record_thread_end;
    }
    os_printf("start record file:%s\n",audio_record_s->filename);
    audio_record_s->msi = msi_new("audio_recorder", 8, NULL);
    if(audio_record_s->msi == NULL) {
        goto audio_record_thread_end;
    } 
    audio_record_s->msi->enable = 1;
    if(audio_record_s->record_format == WAV) {
        audio_record_s->wave_head = (TYPE_WAVE_HEAD*)os_zalloc_psram(sizeof(TYPE_WAVE_HEAD));
        if(audio_record_s->wave_head == NULL) {
            goto audio_record_thread_end;
        }
        auadc_msi_add_output(AUSYS_AUAD, audio_record_s->msi->name);
        osal_fseek(fp, sizeof(TYPE_WAVE_HEAD));
        os_memcpy(audio_record_s->wave_head, wav_header, sizeof(TYPE_WAVE_HEAD));
    }
    else if(audio_record_s->record_format == AAC) {
        codec_info.frame_size = 1024;
        audio_msi = aenc_get_msi(AUDIO_CODEC_AAC, "adc", &codec_info);
        if(audio_msi == NULL) {
            goto audio_record_thread_end;
        }
        auadc_msi_add_output(AUSYS_AUAD, audio_msi->name);
        msi_add_output(audio_msi, NULL, audio_record_s->msi, NULL);
        msi_do_cmd(audio_msi, MSI_CMD_START, 0, 0);
    }

    start_time = os_jiffies();
    while(audio_record_s->is_running && (audio_record_s->record_time < 0 || 
                    (os_jiffies()-start_time)/1000 < audio_record_s->record_time)) {
        frame_buf = msi_get_fb(audio_record_s->msi, 0);
        if(frame_buf) {
            audio_record_s->data_size += frame_buf->len;
            osal_fwrite(frame_buf->data, 1, frame_buf->len, fp);
            msi_delete_fb(audio_record_s->msi, frame_buf);
        }
        else {
            count++;
            if(count % 100 == 0) {
                os_printf("%s\t\trecord time:%d\r\n",__FUNCTION__,os_jiffies()-start_time);
            }
            os_sleep_ms(10);
        }
    }
audio_record_thread_end:
    if(audio_record_s->record_format == WAV) {
        if(fp) {
            audio_record_s->wave_head->riff_chunk.ChunkSize = audio_record_s->data_size + sizeof(TYPE_WAVE_HEAD) - 8;
            audio_record_s->wave_head->fmt_chunk.SampleRate = audio_record_s->sampleRate;
            audio_record_s->wave_head->fmt_chunk.ByteRate = audio_record_s->sampleRate*2;
            audio_record_s->wave_head->data_chunk.DataSize = audio_record_s->data_size;
			osal_fseek(fp, 0);
			osal_fwrite(audio_record_s->wave_head, 1, sizeof(TYPE_WAVE_HEAD), fp);
        }
        if(audio_record_s->wave_head) {
            os_free_psram(audio_record_s->wave_head);
        }
        auadc_msi_del_output(AUSYS_AUAD, audio_record_s->msi->name);
    }
    else {
        if(audio_msi) {
            msi_del_output(audio_msi, NULL, audio_record_s->msi, NULL);
            msi_do_cmd(audio_msi, MSI_CMD_STOP, 0, 0);
            msi_put(audio_msi);
        }  
    }
    if(fp) {
        osal_fclose(fp);
    }
    if(audio_record_s) {
        if(audio_record_s->msi) {
            msi_destroy(audio_record_s->msi);
        }
        os_free_psram(audio_record_s);
        audio_record_s = NULL; 
    }
    os_printf("audio record thread end!\r\n");  
}

int32_t audio_file_record_pause(void)
{
    int32_t ret = RET_ERR;
    if(audio_record_s && audio_record_s->is_running) {
        ret = msi_do_cmd(audio_record_s->msi, MSI_CMD_PAUSE, 0, 0);
    }   
    return ret;
}

int32_t audio_file_record_continue(void)
{
    int32_t ret = RET_ERR;
    if(audio_record_s && audio_record_s->is_running) {
        ret = msi_do_cmd(audio_record_s->msi, MSI_CMD_START, 0, 0);
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

int32_t audio_file_record_init(char *filename, uint32_t sampleRate, int32_t record_time)
{
    uint8_t *file_extension = NULL;

    if(audio_record_s) {
        os_printf("%s err,already recording!\r\n", __FUNCTION__);
        return RET_ERR;
    }
    else {
        audio_record_s = (struct audio_record_struct *)os_zalloc_psram(sizeof(struct audio_record_struct));
        if(!audio_record_s) {
            os_printf("malloc audio_record_s fail!\r\n");
            return RET_ERR;
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

int32 atcmd_record_audio(const char *cmd, char *argv[], uint32 argc)
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