/************************************************************
 * 这文件尽量不要耦合其他文件,仅仅定义一些公共的结构体和枚举
 ************************************************************/
#ifndef __UI_COMMON_H__
#define __UI_COMMON_H__
#define SCREEN_MAGIC 0x6a5b4c3d
enum ui_id
{
    SCREEN_DEL = 0,
    PREVIEW_SCREEN,
    AI_SCREEN,
    MUSIC_SCREEN,
    SCREEN_MAX,
};

enum
{
    SCREEN_TYPE_NONE1,
    SCREEN_MUSIC_TYPE,
    SCREEN_PREVIEW_TYPE,
};

enum screen_ioctl_cmd_t
{
    SCREEN_IOCTL_CMD_NONE = 0,
    SCREEN_IOCTL_CMD_CREATE_UI,
};
enum callback_cmd_t
{
    CALLBACK_CMD_NONE = 0,
    CALLBACK_CMD_CREATE,
    CALLBACK_CMD_START,
    CALLBACK_CMD_RESUME,
    CALLBACK_CMD_PAUSE,
    CALLBACK_CMD_STOP,
    CALLBACK_CMD_DESTROY,
    CALLBACK_CMD_RESTART,
    CALLBACK_CMD_FINISH,
    CALLBACK_CMD_MEDIA_GET_PLAYTIME,
    CALLBACK_CMD_MEDIA_SET_PLAYTIME,
    CALLBACK_CMD_RECORDING,

    // 以上是通用ui以及播放器命令,以下是自定义命令
    CALLBACK_CMD_USER = 0x1000,
    CALLBACK_CMD_MAX,
};
typedef int32_t (*ui_extern_cb)(uint32_t screen_id, enum callback_cmd_t cmd, void *priv,uint32_t param);

/*
uint32_t magic;
uint32_t type; // 类型,类型不匹配,会返回错误
uint32_t ui_id; // ui的id
cb: callback函数
uint32_t priv; // callback的私有结构
*/

#define SCREEN_COMMON_S                                                                                                                                                                                \
    uint32_t     magic;                                                                                                                                                                                \
    uint32_t     type;                                                                                                                                                                                 \
    uint32_t     ui_id;                                                                                                                                                                                \
    uint32_t     screen_id;                                                                                                                                                                            \
    ui_extern_cb cb;                                                                                                                                                                                   \
    void        *cb_priv;

struct screen_common_s
{
    SCREEN_COMMON_S;
    // 以下是私有结构,不同ui,内容不一样
};

// PREVIEW_SCREEN,type = SCREEN_PREVIEW_TYPE
struct screen_preview_msg_s
{
    SCREEN_COMMON_S;
    // 以下是私有结构,不同ui,内容不一样
    uint16_t w;
    uint16_t h;
};

// MUSIC_SCREEN,type = SCREEN_MUSIC_TYPE
struct screen_music_s
{
    SCREEN_COMMON_S;
    // 以下是私有结构,不同ui,内容不一样
    uint32_t stream_id;
};
int32_t screen_ioctl(uint32_t screen_id, uint32_t cmd, uint32_t param1, uint32_t param2);


#endif