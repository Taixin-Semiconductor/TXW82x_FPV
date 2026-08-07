#include "basic_include.h"
#include "stream_define.h"
#include "lib/multimedia/msi.h"
#include "video_app_h264_msi.h"
#include "audio_msi/audio_adc.h"
#include "audio_media_ctrl/audio_code_ctrl.h"
#include "intercom/intercom.h"
#include "babyprotocol_playback.h"
#include "babyprotocol_h264.h"

#ifdef SYS_APP_BBM_CAM

extern struct msi *mp4_demux_msi_init(const char *msi_name, const char *filename);
extern void protocol_client_filtertype(uint8_t type);

static struct msi *playback_msi = NULL;
static struct msi *aac_msi = NULL;

int32_t client_remote_playback_init(const char *filename)
{
    struct msi *opus_msi = NULL;
    AUDEC_INIT audec_init;
    if(playback_msi) {
        msi_destroy(playback_msi);
        audio_decode_deinit(aac_msi);
    }
	audec_init.track_type = MEDIA_TRACK;
    audec_init.priority = play_interruptible;
    audec_init.direct_to_dac = 0;
	audec_init.destroy_self = 0;
    audec_init.use_tpc = 0;
	audec_init.src_msi = NULL;
    aac_msi = audio_decode_init(AAC_DEC, 8000, &audec_init);
    if(aac_msi == NULL) {
        return RET_ERR;
    }
	protocol_client_filtertype(FSTYPE_H264_FILE);
	intercom_encode_pause(1, 1);
	opus_msi = msi_find("SR_OPUS_ENCODE", 1);
    if(opus_msi) {
        msi_put(opus_msi);
        audio_code_set_src_msi(opus_msi, aac_msi);
    }
	intercom_encode_pause(0, 0);
	os_sleep_ms(100);
    playback_msi = mp4_demux_msi_init("bbm_playback_mp4", filename);
    if(playback_msi) {
		msi_add_output(playback_msi, NULL, "NET_H264");
        audio_code_set_src_msi(aac_msi, playback_msi);
		intercom_set_stream_type(intercom_live_audio, intercom_playback_audio);
        msi_do_cmd(playback_msi, MSI_CMD_VIDEO_DEMUX_CTRL, MSI_VIDEO_DEMUX_START, 0);
        return RET_OK;
    }
    audio_decode_deinit(aac_msi);
    return RET_ERR;
}

int32_t client_remote_playback_deinit(void)
{
    struct msi *opus_msi = NULL;
    if(playback_msi) {
        msi_destroy(playback_msi);
		os_sleep_ms(100);
        protocol_client_filtertype(FSTYPE_H264_GEN420_DATA);
        audio_decode_deinit(aac_msi);
        intercom_encode_pause(1, 1);
        opus_msi = msi_find("SR_OPUS_ENCODE", 1);
        if(opus_msi) {
            msi_put(opus_msi);
            audio_code_set_src_msi(opus_msi, get_auadc_msi(AUSYS_AUAD));
        }
		intercom_set_stream_type(intercom_live_audio, intercom_live_audio);
        intercom_encode_pause(0, 0); 
    }
    playback_msi = NULL;
    return RET_OK;
}

#endif

#ifdef SYS_APP_BBM_LCD

int32_t server_remote_playback_init(void)
{
    intercom_set_stream_type(intercom_playback_audio,intercom_live_audio);
    intercom_reset_play();	
	return RET_OK;    
}

int32_t server_remote_playback_deinit(void)
{
    intercom_set_stream_type(intercom_live_audio,intercom_live_audio);
    intercom_reset_play();	   
	return RET_OK; 
}

#endif