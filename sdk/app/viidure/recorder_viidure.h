#ifndef _RECORDER_VIIDURE_H
#define _RECORDER_VIIDURE_H

#include "basic_include.h"
#include "app/video_record/file_process.h"
#include "app/video_record/video_record.h"


typedef struct msi *(*create_msi_func)(struct video_record_cfg *cfg);
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
    struct msi *photo_msi;
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
