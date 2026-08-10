#ifndef _DOUBAO_STT_H_
#define _DOUBAO_STT_H_

#ifdef __cplusplus
extern "C" {
#endif

struct Doubao_STT_Cfg{
    char    *url;                   //必填
    char    *app_appid;             //必填
    char    *app_token;             //必填
    char    *app_cluster;           //必填

    char    *user_uid;              //必填

    char    *audio_format;          //必填
    char    *audio_codec;           //可选
    char    *audio_rate;            //可选
    char    *audio_bits;            //可选
    char    *audio_channel;         //可选

    char    *boosting_table_name;   //可选
    char    *correct_table_name;    //可选
};

struct Doubao_STT_Header{
    uint8   protocol_version: 4, header_size: 4;
    uint8   message_type: 4, specific_flags: 4;
    uint8   serialization_method: 4, compression: 4;
    uint8   reserved;
    size_t  playload_size;
};

extern const struct llm_model_data doubao_stt;

#ifdef __cplusplus
}
#endif

#endif  //_DOUBAO_STT_H_
