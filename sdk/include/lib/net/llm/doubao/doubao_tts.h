#ifndef _DOUBAO_TTS_H_
#define _DOUBAO_TTS_H_

#ifdef __cplusplus
extern "C" {
#endif

struct Doubao_TTS_Cfg{
    char    *url;                       //必填
    char    *app_appid;                 //必填
    char    *app_token;                 //必填
    char    *app_cluster;               //必填

    char    *user_uid;                  //必填

    char    *audio_voice_type;          //必填
    char    *audio_rate;                //可选
    char    *audio_compression_rate;    //可选
    char    *audio_encoding;            //可选
    char    *audio_speed_ratio;         //可选
    char    *audio_volume_ratio;        //可选
    char    *audio_pitch_ratio;         //可选
    char    *audio_emotion;             //可选
    char    *audio_language;            //可选

    char    *silence_duration;          //可选
};

struct Doubao_TTS_Header{
    uint8   protocol_version: 4, header_size: 4;
    uint8   message_type: 4, specific_flags: 4;
    uint8   serialization_method: 4, compression: 4;
    uint8   reserved;
    int32   sequence_number;
    size_t  playload_size;
};

extern const struct llm_model_data doubao_tts;

#ifdef __cplusplus
}
#endif

#endif  //_DOUBAO_TTS_H_
