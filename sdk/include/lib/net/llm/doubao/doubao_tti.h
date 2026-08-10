#ifndef _DOUBAO_TTI_H_
#define _DOUBAO_TTI_H_

#ifdef __cplusplus
extern "C" {
#endif

struct Doubao_TTI_Cfg {
    char    *url;                   //必填
    char    *ark_api_key;           //必填

    char    *model;                 //必填

    char    *size;                  //可选
    char    *seed;                  //可选
    char    *guidance_scale;        //可选
    char    *watermark;             //可选
};

extern const struct llm_model_data doubao_tti;

#ifdef __cplusplus
}
#endif

#endif  //_DOUBAO_TTI_H_
