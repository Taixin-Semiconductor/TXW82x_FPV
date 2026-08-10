#ifndef _NETEAST_STS_H_
#define _NETEAST_STS_H_

#ifdef __cplusplus
extern "C" {
#endif

//详细配置说明文档请查阅https://www.neteast.cn/open/docs/developer_guides/streaming_chat_event#91642fa8
struct neteast_chat_platform_cfg {     
    //鉴权连接相关
    char    *host_url;                              //必填
    char    *license_url;                           //必填
    char    *license_key;                           //必填
    char    *app_key;                               //必填  
    char    *app_secret;                            //必填  
    char    *device_id;                             //必填  
    char    *agent_id;                              //必填  

    // 聊天配置参数
    char    *chat_config_user_id;                   //可选

    // 音频输入参数

    char    *input_audio_format;                    //可选
    char    *input_audio_sample_rate;               //可选
    char    *input_audio_channel;                   //可选
    char    *input_audio_encoding;                  //可选

    // 音频输出参数    :PCM
    char    *output_audio_format;
    char    *output_audio_sample_rate;              //可选
    char    *output_audio_channel;                  //可选
    char    *output_audio_encoding;                 //可选
};

enum neteast_server_vad_event {
    NETEAST_SERVER_VAD_START = 1,
    NETEAST_SERVER_VAD_STOP,
};

extern const struct llm_model_data neteast_sts_model; 

#ifdef __cplusplus
}
#endif

#endif




