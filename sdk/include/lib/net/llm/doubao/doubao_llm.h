#ifndef _DOUBAO_LLM_H_
#define _DOUBAO_LLM_H_

#ifdef __cplusplus
extern "C" {
#endif

struct Doubao_LLM_Cfg {
    char    *url;                   //必填
    char    *ark_api_key;           //必填
    char    *endpoint_id;           //必填
    
    char    *thinking;              //可选
    char    *stream;                //可选
    char    *max_tokens;            //可选
    char    *max_completion_tokens; //可选
    char    *service_tier;          //可选
    char    *stop;                  //可选
    char    *reasoning_effort;      //可选
    char    *response_format;       //可选

    char    *frequency_penalty;     //可选
    char    *presence_penalty;      //可选
    char    *temperature;           //可选
    char    *top_p;                 //可选
};
extern const struct llm_model_data doubao_llm;

#ifdef __cplusplus
}
#endif

#endif  //_DOUBAO_LLM_H_
