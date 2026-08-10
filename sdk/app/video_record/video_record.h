#ifndef __VIDEO_RECORD_H__
#define __VIDEO_RECORD_H__

#include "file_process.h"

struct mult_record
{
    struct os_mutex mutex;
    uint8_t         init;
    uint8_t         count;
    uint32_t        start_time;
};

extern struct mult_record mult_record;

// MP4 录制模式
enum
{
    MP4_MODE_NORMAL,     // 普通录像模式
    MP4_MODE_TIME_LAPSE, // 缩时录影模式
    MP4_MODE_EVENT,      // 事件录像模式
};

struct video_record_cfg
{
    const char *name;
    uint8_t    srcID;
    uint8_t    filter;
    uint8_t    rec_time;
    uint8_t    audio_en;
    uint8_t    mode;
    uint8_t    video_fps;
    struct file_process *file_process;
};

struct msi *mp4_record_msi_init(struct video_record_cfg *cfg);
struct msi *avi_record_msi_init(struct video_record_cfg *cfg);
struct msi *odml_record_msi_init(struct video_record_cfg *cfg);


#endif  /*  __VIDEO_RECORD_H__  */
