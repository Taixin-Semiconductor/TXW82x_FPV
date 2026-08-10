#ifndef _LLM_CONFIG_H_
#define _LLM_CONFIG_H_

/*
//deepseek
#define CHAT_URL        "https://api.deepseek.com/chat/completions"
#define ARK_API_KEY     "sk-4659ab62e2424306b219f013022a8776"
#define ENDPOINT_ID     "deepseek-chat"

//doubao
#define CHAT_URL            "https://ark.cn-beijing.volces.com/api/v3/chat/completions"
#define CONTEXT_URL         "https://ark.cn-beijing.volces.com/api/v3/context/create"
#define CONTEXT_CHAT_URL    "https://ark.cn-beijing.volces.com/api/v3/context/chat/completions"
#define ARK_API_KEY         "64280263-a5cf-4e74-85e5-6f81c7c18643"
#define ENDPOINT_ID         "ep-20250221172422-7pfn8" //Doubao-1.5-lite-32k｜250115 支持common_prefix
//#define ENDPOINT_ID     "ep-20250305142241-kg9jn" //Doubao-pro-32k｜character-241215 支持session

#define STT_URL         "wss://openspeech.bytedance.com/api/v2/asr"
#define STT_APPID       "1268700412"
#define STT_TOKEN       "FXPi6WqLG8g1vimgXMXKTN18335pz2tW"
#define STT_CLUSTER     "volcengine_input_common"

#define TTS_URL         "wss://openspeech.bytedance.com/api/v1/tts/ws_binary"
#define TTS_APPID       "1268700412"
#define TTS_TOKEN       "FXPi6WqLG8g1vimgXMXKTN18335pz2tW"
#define TTS_CLUSTER     "volcano_tts"

*/
//#define LLM_MESSAGE_QUEUE_TX_CNT            16
//#define LLM_MESSAGE_QUEUE_RX_CNT            16

//#define LLM_RESP_ERROR_MESSAGE_SIZE         256         //存储服务器返回的 ERR 内容的大小


//CHAT
//#define LLM_CHAT_REPLY_FRAGMENT_SIZE        3*50+10     //对话回复内容分片大小(50个中文字符左右)
//#define LLM_CHAT_SESSION_RECV_DELAY         10*50       //500ms
//#define LLM_CHAT_CONTEXT_ID_LEN             24          //上下文缓存 id 大小

//STT
//#define LLM_STT_REPLY_FRAGMENT_SIZE         2048        //STT 回复处理数据大小
//#define LLM_STT_CALLBACK_RECV_MAX_SIZE      1024        //STT 传输层每次接收的数据大小
//#define LLM_STT_SESSION_RECV_DELAY          10*50       //500ms

//TTS
//#define LLM_TTS_RX_BUFF_SIZE                4096        //TTS 回复音频存放 buff 大小
//#define LLM_TTS_CALLBACK_RECV_MAX_SIZE      1024        //TTS 传输层每次接收的数据大小
//#define LLM_TTS_SESSION_RECV_DELAY          10*100      //1000ms

#endif

