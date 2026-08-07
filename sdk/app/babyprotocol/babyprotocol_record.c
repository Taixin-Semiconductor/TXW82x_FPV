#include "basic_include.h"
#include "stream_define.h"
#include "lib/multimedia/msi.h"
#include "video_app_h264_msi.h"
#include "audio_media_ctrl/audio_code_ctrl.h"
#include "audio_msi/audio_adc.h"
#include "intercom/intercom.h"
#include "babyprotocol_record.h"
#include "app/video_app/file_thumb.h"
#include "recorder/file_process.h"
#include "fs/fatfs/osal_file.h"
#include "app/loop_record_moudle/loop_record_moudle.h"
#include "app/video_app/file_common_api.h"

#ifdef SYS_APP_BBM_CAM

struct msi *mp4_encode_msi2_init(const char *mp4_msi_name, uint8_t srcID, uint8_t filter_type, uint8_t rec_time, 
                                 uint32_t audio_encode, struct file_process *file_process, uint8_t mode);

static struct msi *rec_msi = NULL;
static struct msi *h264_msi = NULL;
static struct msi *aac_msi = NULL;

int32_t client_local_record_init(uint32_t record_time_minutes)
{
    AUENC_INIT auenc_init;
    if(!rec_msi) {
        h264_msi = msi_find(AUTO_H264, 1);
        if(h264_msi) {
            msi_put(h264_msi);
            auenc_init.destroy_self = 0;
            auenc_init.src_msi = get_auadc_msi(AUSYS_AUAD);
            aac_msi = audio_encode_init(AAC_ENC, audio_adc_get_samplerate(AUSYS_AUAD), &auenc_init);
            if(!aac_msi) {   
                h264_msi = NULL;  
                return RET_ERR;              
            }
            struct file_process mp4_file_process = {
                .loop = NULL,
                .rec_path = REC_PATH,
                .ext_name = MP4_EXTENSION_NAME,
                .create_file = rec_create_file,
                .loop_free = rec_loop_free,
                .lock_file = NULL,
            };
            rec_msi = mp4_encode_msi2_init("bbm_record_mp4", FRAMEBUFF_SOURCE_CAMERA0, FSTYPE_H264_VPP_DATA0, 
                                           record_time_minutes, AAC_ENC, &mp4_file_process, 0);
            if(rec_msi) {
                msi_add_output(h264_msi, NULL, "bbm_record_mp4");
                audio_code_add_output(aac_msi, "bbm_record_mp4");
				msi_do_cmd(rec_msi, MSI_CMD_MEDIA_CTRL, MSI_MEDIA_CTRL_RECORD_START, 0);
                return RET_OK;
            }
            else {
                h264_msi = NULL;
                audio_encode_deinit(aac_msi);
                aac_msi = NULL;
                return RET_ERR;
            }
        }
        else {
            return RET_ERR;
        }
    }
    return RET_OK;
}

int32_t client_local_record_deinit(void)
{
    if(rec_msi) {
        if(h264_msi) {
            msi_del_output(h264_msi, NULL, "bbm_record_mp4");
        }
        if(aac_msi) {
            audio_code_del_output(aac_msi, "bbm_record_mp4");
            audio_encode_deinit(aac_msi);
        }
        msi_destroy(rec_msi);
        rec_msi = NULL;
        h264_msi = NULL;
        aac_msi = NULL;
    }
    return RET_OK;    
}

static uint32_t mp4_record = 0;
static void mp4_record_thread(void *d)
{
	uint32_t state = *((uint32_t*)d);
	if(state == 1) {
		client_local_record_init(1);  		
	}
	else if(state == 0) {
		client_local_record_deinit();		
	}
}

int32 atcmd_bbm_client_record(const char *cmd, char *argv[], uint32 argc)
{
	if(argc >= 1) {
		mp4_record = os_atoi(argv[0]);
		os_task_create("mp4_record_thread", mp4_record_thread, &mp4_record, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
	}
    
	return 0;
}

#endif