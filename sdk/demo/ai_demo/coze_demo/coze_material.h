#ifndef _COZE_MATERIAL_H_
#define _COZE_MATERIAL_H_

#include "sys_config.h"

#if COZE_DEMO
// 声明数组
extern const char beep[];
extern unsigned int beep_size;

extern const char yilianjie[];
extern unsigned int yilianjie_size;

extern const char yiduankai[];
extern unsigned int yiduankai_size;

extern const char tuixia[];
extern unsigned int tuixia_size;

extern const char wozai[];
extern unsigned int wozai_size;

extern const char kaiji[];
extern unsigned int kaiji_size;

extern const char guanji[];
extern unsigned int guanji_size;

extern const char wangluocha[];
extern unsigned int wangluocha_size;

extern const char chaoshi[];
unsigned int chaoshi_size;

extern const char fasong[];
unsigned int fasong_size;

#if LLM_TAIXIN_LOGO
extern const char logo[];
unsigned int logo_size;
#endif

// 如果还有其他数组，也可以在这里声明
// extern const unsigned char another_sound[];
// extern const unsigned int another_sound_size;

#endif // COZE_DEMO
#endif // _COZE_MATERIAL_H_

