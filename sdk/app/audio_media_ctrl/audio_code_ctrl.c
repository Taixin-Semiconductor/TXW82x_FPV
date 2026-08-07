#include "basic_include.h"
#include "audio_code_ctrl.h"
#include "aac_code.h"
#include "alaw_code.h"
#include "amr_decode.h"
#include "mp3_decode.h"
#include "opus_code.h"
#include "wave_code.h"

struct msi *audio_encode_init(uint32_t coder, uint32_t samplerate, AUENC_INIT *auenc_init)
{
    struct msi *msi = NULL;
    switch(coder) {
        case AAC_ENC:
            msi = aac_encode_init(NULL, samplerate, 0, auenc_init);
            break;
        case ALAW_ENC:
            msi = alaw_encode_init(samplerate, auenc_init);
            break;
        case OPUS_ENC:
            msi = opus_encode_init(samplerate, auenc_init);
            break;
        default:
            break;
    }
    return msi;
}

struct msi *audio_decode_init(uint32_t coder, uint32_t samplerate, AUDEC_INIT *audec_init)
{
    struct msi *msi = NULL;
    switch(coder) {
        case AAC_DEC:
            msi = aac_decode_init(NULL, 0, audec_init);
            break;
        case ALAW_DEC:
            msi = alaw_decode_init(audec_init);
            break;
        case AMRNB_DEC:
        case AMRWB_DEC:
            msi = amr_decode_init(NULL, 0, audec_init);
            break;
        case MP3_DEC:
            msi = mp3_decode_init(NULL, 0, audec_init);
            break;
        case OPUS_DEC:
            msi = opus_decode_init(samplerate, audec_init);
            break;
        default:
            break;
    }
    return msi;
}

int32_t audio_code_set_src_msi(struct msi *msi, struct msi *src_msi)
{
    int32_t ret = RET_ERR;
    ret = msi_do_cmd(msi, MSI_CMD_AUCODER, MSI_AUCODER_SET_SRCMSI, (uint32_t)src_msi);    
    return ret;     
}

int32_t audio_encode_deinit(struct msi *msi)
{
    int32_t ret = RET_ERR;
    ret = msi_do_cmd(msi, MSI_CMD_AUCODER, MSI_AUCODER_DEINIT, 0);    
    return ret; 
}

int32_t audio_decode_deinit(struct msi *msi)
{
    int32_t ret = RET_ERR;
    ret = msi_do_cmd(msi, MSI_CMD_AUCODER, MSI_AUCODER_DEINIT, 0);    
    return ret; 
}

int32_t audio_encode_set_bitrate(struct msi *msi, uint32_t bitrate)
{
    int32_t ret = RET_ERR;
    ret = msi_do_cmd(msi, MSI_CMD_AUCODER, MSI_AUCODER_SET_BITRATE, bitrate);    
    return ret; 
}

int32_t audio_code_continue(struct msi *msi)
{
    int32_t ret = RET_ERR;
    ret = msi_do_cmd(msi, MSI_CMD_AUCODER, MSI_AUCODER_CONTINUE, 0);    
    return ret;     
}

int32_t audio_code_pause(struct msi *msi)
{
    int32_t ret = RET_ERR;
    ret = msi_do_cmd(msi, MSI_CMD_AUCODER, MSI_AUCODER_PAUSE, 0);    
    return ret;   
}

int32_t audio_code_clear(struct msi *msi)
{
    int32_t ret = RET_ERR;
    ret = msi_do_cmd(msi, MSI_CMD_AUCODER, MSI_AUCODER_CLEAR_STREAM, 0);    
    return ret;   
}

int32_t audio_code_add_output(struct msi *msi, const char *msi_name)
{
    int32_t ret = RET_ERR;
    ret = msi_add_output(msi, NULL, msi_name);    
    return ret;   
}

int32_t audio_code_del_output(struct msi *msi, const char *msi_name)
{
    int32_t ret = RET_ERR;
    ret = msi_del_output(msi, NULL, msi_name);    
    return ret;
}

int32_t audio_code_get_status(struct msi *msi)
{
    int32_t ret = RET_ERR;
    msi_do_cmd(msi, MSI_CMD_AUCODER, MSI_AUCODER_GET_STATUS, (uint32_t)(&ret));
    return ret;             
}
