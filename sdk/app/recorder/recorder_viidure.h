#ifndef _RECORDER_VIIDURE_H
#define _RECORDER_VIIDURE_H

#include "basic_include.h"
#include "file_process.h"

typedef struct msi *(*create_msi_func)(const char *mp4_msi_name, uint8_t srcID, uint8_t filter_type, uint8_t rec_time, 
                                       uint32_t audio_encode, struct file_process *file_process, uint8_t mode);
typedef uint8_t (*get_video_status)(void);

struct media_s
{
    const char *rtsp_url;           //rtsp的地址的后缀地址
    const char *tran_mode;          //传输方式
};

struct cam_cfg {
    struct msi *msi;
    const char *msi_name;
    struct msi *src_msi;
    const char *src_msi_name;
    struct msi *aac_msi;
    uint8_t srcID;
    uint8_t filter_type;
    uint8_t mode;
    uint8_t mask;
    uint8_t type; // 0:mp4, 1:avi
    uint8_t src_type;
    const char *rec_path;
    const char *img_path;
    uint8_t enable;
    struct media_s media;
    struct file_process file_process;
    create_msi_func create_func;
    get_video_status get_status;
};

void rec_open(void);
void rec_close(void);
int config_Viidure(int port);


#endif
