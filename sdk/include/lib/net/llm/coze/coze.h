#ifndef _COZE_STS_H_
#define _COZE_STS_H_

#ifdef __cplusplus
extern "C" {
#endif

//详细配置说明文档请查阅https://www.coze.cn/open/docs/developer_guides/streaming_chat_event#91642fa8
struct coze_chat_platform_cfg {
    char    *pat_key;       //必填
    char    *bot_id;        //必填
    char    *host_url;      //必填
    char    *update_url;    //必填

    // 聊天配置参数
    char    *chat_config_auto_save_history; //可选
    char    *chat_config_user_id;           //可选
    char    *chat_config_conversation_id;   //可选
    char    *chat_config_meta_data;         //可选
    char    *chat_config_custom_variables;  //可选
    char    *chat_config_extra_params;      //可选
    char    *chat_config_parameters;        //可选
    
    // 音频输入参数
    char    *input_audio_format;        //可选
    char    *input_audio_codec;         //可选
    char    *input_audio_sample_rate;   //可选
    char    *input_audio_channel;       //可选
    char    *input_audio_bit_depth;     //可选
    
    // 音频输出参数    
    char    *output_audio_codec;                    //可选
    char    *output_audio_speech_rate;              //可选
    char    *output_audio_loudness_rate;            //可选
    char    *output_audio_voice_id;                 //可选
    char    *output_audio_context_texts;            //可选
    char    *output_audio_limit_config_max_frame_num; //可选
    // PCM
    char    *output_audio_pcm_config_sample_rate;   //可选
    char    *output_audio_pcm_config_frame_size_ms; //可选
    // OPUS
    char    *output_audio_opus_config_bitrate;      //可选
    char    *output_audio_opus_config_use_cbr;      //可选
    char    *output_audio_opus_config_sample_rate;  //可选
    char    *output_audio_opus_config_frame_size_ms;//可选
    // MP3
    char    *output_audio_mp3_config_sample_rate;   //可选
    char    *output_audio_mp3_config_bit_rate;      //可选
    

    // 转检测配置
    char    *turn_detection_type;                       //可选

    // server_vad
    char    *turn_detection_prefix_padding_ms;          //可选
    char    *turn_detection_silence_duration_ms;        //可选
    char    *turn_detection_interrupt_config_mode;      //可选
    char   **turn_detection_interrupt_config_keywords;  //可选

    // 其他 asr 参数
    char    *asr_config_user_language;      //可选
    char    *asr_config_enable_ddc;         //可选
    char    *asr_config_enable_itn;         //可选
    char    *asr_config_enable_punc;        //可选
    char    *asr_config_stream_mode;        //可选
    char    *asr_config_enable_nostream;    //可选
    char   **asr_config_hot_words;          //可选
};

extern const struct llm_model_data coze_sts_model; 

enum coze_server_vad_event{
    COZE_SERVER_VAD_START = 1,
    COZE_SERVER_VAD_STOP,
};

#ifdef __cplusplus
}
#endif

#endif




