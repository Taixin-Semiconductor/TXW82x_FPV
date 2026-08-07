#include "basic_include.h"
#include "audio_code_ctrl.h"

const char *const AUCODE_MSI_NAME[] = {
    "SR_AAC_ENCODE",
    "SR_AAC_DECODE",
    "SR_ALAW_ENCODE",
    "SR_ALAW_DECODE",
    "SR_AMR_DECODE",
    "SR_MP3_DECODE",
	"SR_OPUS_ENCODE",
	"SR_OPUS_DECODE",
};

const char *audio_code_msi_name(uint32_t coder)
{
    const char *msi_name = NULL;
    switch(coder) {
        case AAC_ENC:
            msi_name = AUCODE_MSI_NAME[0];
            break;
        case AAC_DEC:
            msi_name = AUCODE_MSI_NAME[1];
            break;
        case ALAW_ENC:
            msi_name = AUCODE_MSI_NAME[2];
            break;
        case ALAW_DEC:
            msi_name = AUCODE_MSI_NAME[3];
            break;
        case AMRNB_DEC:
        case AMRWB_DEC:
            msi_name = AUCODE_MSI_NAME[4];
            break;
        case MP3_DEC:
            msi_name = AUCODE_MSI_NAME[5];
            break;
        case OPUS_ENC:
            msi_name = AUCODE_MSI_NAME[6];
            break;
        case OPUS_DEC:
            msi_name = AUCODE_MSI_NAME[7];
            break;
        default:
            break;
    }   
    return msi_name; 
}

struct msi *audio_encode_init(uint32_t coder, uint32_t samplerate)
{
    struct msi *msi = NULL;
    switch(coder) {
        case AAC_ENC:
            msi = aac_encode_init(NULL, samplerate, 0);
            break;
        case ALAW_ENC:
            msi = alaw_encode_init(samplerate);
            break;
        case OPUS_ENC:
            msi = opus_encode_init(samplerate);
            break;
        default:
            break;
    }
    return msi;
}

void audio_encode_set_bitrate(uint32_t coder, uint32_t bitrate)
{
    switch(coder) {
        case OPUS_ENC:
            opus_encode_set_bitrate(bitrate);
            break;
        default:
            break;
    }  
}

struct msi *audio_decode_init(uint32_t coder, uint32_t samplerate, uint8_t direct_to_dac)
{
    struct msi *msi = NULL;
    switch(coder) {
        case AAC_DEC:
            msi = aac_decode_init(NULL, direct_to_dac);
            break;
        case ALAW_DEC:
            msi = alaw_decode_init(direct_to_dac);
            break;
        case AMRNB_DEC:
        case AMRWB_DEC:
            msi = amr_decode_init(NULL, direct_to_dac);
            break;
        case MP3_DEC:
            msi = mp3_decode_init(NULL, direct_to_dac);
            break;
        case OPUS_DEC:
            msi = opus_decode_init(samplerate, direct_to_dac);
            break;
        default:
            break;
    }
    return msi;
}

int32_t audio_encode_deinit(uint32_t coder)
{
    int32_t ret = RET_ERR;

    switch(coder) {
        case AAC_ENC:
            ret = aac_encode_deinit(0);
            break;
        case ALAW_ENC:
            ret = alaw_encode_deinit();
            break;
        case OPUS_ENC:
            ret = opus_encode_deinit();
            break;
        default:
            break;
    }  
    return ret;  
}

int32_t audio_decode_deinit(uint32_t coder)
{
    int32_t ret = RET_ERR;

    switch(coder) {
        case AAC_DEC:
            ret = aac_decode_deinit();
            break;
        case ALAW_DEC:
            ret = alaw_decode_deinit();
            break;
        case AMRNB_DEC:
        case AMRWB_DEC:
            ret = amr_decode_deinit();
            break;
        case MP3_DEC:
            ret = mp3_decode_deinit();
            break;
        case OPUS_DEC:
            ret = opus_decode_deinit();
            break;
        default:
            break;
    }   
    return ret;
}

void audio_code_continue(uint32_t coder)
{
    switch(coder) {
        case AAC_ENC:
            aac_encode_continue();
            break;
        case ALAW_ENC:
            alaw_encode_continue();
            break;
        case OPUS_ENC:
            opus_encode_continue();
            break;
        case AAC_DEC:
            aac_decode_continue();
            break;
        case ALAW_DEC:
            alaw_decode_continue();
            break;
        case AMRNB_DEC:
        case AMRWB_DEC:
            amr_decode_continue();
            break;
        case MP3_DEC:
            mp3_decode_continue();
            break;
        case OPUS_DEC:
            opus_decode_continue();
            break;
        default:
            break;
    }        
}

void audio_code_pause(uint32_t coder)
{
    switch(coder) {
        case AAC_ENC:
            aac_encode_pause();
            break;
        case ALAW_ENC:
            alaw_encode_pause();
            break;
        case OPUS_ENC:
            opus_encode_pause();
            break;
        case AAC_DEC:
            aac_decode_pause();
            break;
        case ALAW_DEC:
            alaw_decode_pause();
            break;
        case AMRNB_DEC:
        case AMRWB_DEC:
            amr_decode_pause();
            break;
        case MP3_DEC:
            mp3_decode_pause();
            break;
        case OPUS_DEC:
            opus_decode_pause();
            break;
        default:
            break;
    }            
}

void audio_code_clear(uint32_t coder)
{
    switch(coder) {
        case AAC_ENC:
            aac_encode_clear();
            break;
        case ALAW_ENC:
            alaw_encode_clear();
            break;
        case OPUS_ENC:
            opus_encode_clear();
            break;
        case AAC_DEC:
            aac_decode_clear();
            break;
        case ALAW_DEC:
            alaw_decode_clear();
            break;
        case AMRNB_DEC:
        case AMRWB_DEC:
            amr_decode_clear();
            break;
        case MP3_DEC:
            mp3_decode_clear();
            break;
        case OPUS_DEC:
            opus_decode_clear();
            break;
        default:
            break;
    }            
}

int32_t audio_code_add_output(uint32_t coder, const char *msi_name)
{
    int32_t ret = RET_ERR;
    switch(coder) {
        case AAC_ENC:
            ret = aac_encode_add_output(msi_name);
            break;
        case ALAW_ENC:
            ret = alaw_encode_add_output(msi_name);
            break;
        case OPUS_ENC:
            ret = opus_encode_add_output(msi_name);
            break;
        case AAC_DEC:
            ret = aac_decode_add_output(msi_name);
            break;
        case ALAW_DEC:
            ret = alaw_decode_add_output(msi_name);
            break;
        case AMRNB_DEC:
        case AMRWB_DEC:
            ret = amr_decode_add_output(msi_name);
            break;
        case MP3_DEC:
            ret = mp3_decode_add_output(msi_name);
            break;
        case OPUS_DEC:
            ret = opus_decode_add_output(msi_name);
            break;
        default:
            break;
    }   
    return ret;         
}

int32_t audio_code_del_output(uint32_t coder, const char *msi_name)
{
    int32_t ret = RET_ERR;
    switch(coder) {
        case AAC_ENC:
            ret = aac_encode_del_output(msi_name);
            break;
        case ALAW_ENC:
            ret = alaw_encode_del_output(msi_name);
            break;
        case OPUS_ENC:
            ret = opus_encode_del_output(msi_name);
            break;
        case AAC_DEC:
            ret = aac_decode_del_output(msi_name);
            break;
        case ALAW_DEC:
            ret = alaw_decode_del_output(msi_name);
            break;
        case AMRNB_DEC:
        case AMRWB_DEC:
            ret = amr_decode_del_output(msi_name);
            break;
        case MP3_DEC:
            ret = mp3_decode_del_output(msi_name);
            break;
        case OPUS_DEC:
            ret = opus_decode_del_output(msi_name);
            break;
        default:
            break;
    }   
    return ret;         
}

int32_t get_audio_code_status(uint32_t coder)
{
    int32_t ret = RET_ERR;
    switch(coder) {
        case AAC_ENC:
            ret = (int32_t)get_aac_encode_status();
            break;
        case ALAW_ENC:
            ret = (int32_t)get_alaw_encode_status();
            break;
        case OPUS_ENC:
            ret = (int32_t)get_opus_encode_status();
            break;
        case AAC_DEC:
            ret = (int32_t)get_aac_decode_status();
            break;
        case ALAW_DEC:
            ret = (int32_t)get_alaw_decode_status();
            break;
        case AMRNB_DEC:
		case AMRWB_DEC:
            ret = (int32_t)get_amr_decode_status();
            break;
        case MP3_DEC:
            ret = (int32_t)get_mp3_decode_status();
            break;
        case OPUS_DEC:
            ret = (int32_t)get_opus_decode_status();
            break;
        default:
            break;
    }   
    return ret;             
}
