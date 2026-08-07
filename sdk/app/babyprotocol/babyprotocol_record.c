#include "basic_include.h"
#include "stream_define.h"
#include "lib/multimedia/msi.h"
#include "video_app_h264_msi.h"
#include "audio_media_ctrl/audio_code_ctrl.h"
#include "audio_msi/audio_adc.h"
#include "intercom/intercom.h"
#include "babyprotocol_record.h"
#include "app/video_app/file_thumb.h"
#include "fs/fatfs/osal_file.h"
#include "app/loop_record_moudle/loop_record_moudle.h"
#include "app/video_app/file_common_api.h"

#ifdef SYS_APP_BBM_CAM

#define MP4_LOOP_REMAIN_CAP (256) // 循环录像剩余空间控制
#define MAX_SINGLE_SIZE (100 * 1024 * 1024) // 单个MP4文件大小

extern struct msi *mp4_encode_msi2_init(const char *mp4_msi_name, uint8_t srcID, uint8_t filter_type, 
                                   uint8_t rec_time, uint32_t audio_encode, void *file_create, 
                                   void *loop_free, uint8_t mode);
extern uint8_t get_mp4_file_name(const char *rec_dir, char *sub_path, char *file_name);

static struct msi *rec_msi = NULL;
static struct msi *h264_msi = NULL;
static struct msi *aac_msi = NULL;

static void *bbm_rec_creat_mp4_file(void **loop, char *file_name)
{
    void    *fp   = NULL;
    void    *node = NULL;
    char     file_path[64];
    char     sub_path[32];
    char     dir_path[32];
    char     min_file[32];
    uint32_t sd_cap     = 0;
    int      res        = 0;
    uint8_t  changeflag = 0;
    uint32_t file_size = 0;

    res = osal_fatfsfree("0:", NULL, &sd_cap);
    os_printf(KERN_INFO"sd_cap: %d\n", sd_cap);
    if (res != 0 || sd_cap == 0) {
        goto creat_mp4_file_end;
    }

get_node:
    if (sd_cap < MP4_LOOP_REMAIN_CAP)
    {
        if (!*loop) {
            *loop = get_file_list(REC_PATH, MP4_EXTENSION_NAME);
        }

        if (*loop) {
            node = get_file_node(*loop);
            if (!node) {
                free_file_list(*loop);
                *loop = get_file_list(REC_PATH, MP4_EXTENSION_NAME);
                if (!*loop) {
                    _os_printf("%s %d\r\n", __FUNCTION__, __LINE__);
                    goto creat_mp4_file_end;
                }
                node = get_file_node(*loop);
            }
            while (!node) {
                res = osal_unlink_dir(dir_path, 0);
                if (res != FR_OK) {
                    _os_printf("unlink dir %s err, res: %d\r\n", dir_path, res);
                    res = osal_unlink_dir(dir_path, 1);
                    if (res != FR_OK) {
                        _os_printf("force unlink dir %s err, res: %d\r\n", dir_path, res);
                        _os_printf("%s %d\r\n", __FUNCTION__, __LINE__);
                        goto creat_mp4_file_end;
                    }
                    _os_printf("force unlink dir %s\r\n", dir_path);
                }
                _os_printf("unlink dir %s\r\n", dir_path);
                free_file_list(*loop);
                *loop = get_file_list(REC_PATH, MP4_EXTENSION_NAME);
                if (!*loop) {
                    _os_printf("%s %d\r\n", __FUNCTION__, __LINE__);
                    goto creat_mp4_file_end;
                }
                node = get_file_node(*loop);
                if (node) {
                    break;
                }
            }
            file_size = get_file_size(node);
            if(file_size < MAX_SINGLE_SIZE) {
                char *name = get_file_name(node);
                char path[64];
                os_sprintf(path, "%s/%s", dir_path, name);
                res = osal_unlink(path);
                if(res == FR_OK) {
                    _os_printf("unlink file %s, filesize: %d\r\n", path, file_size);
                } else {
                    _os_printf("%s %d\tunlink file %s fail, res: %d\r\n", __FUNCTION__, __LINE__, path, res);
                }
                free_file_node(node);
                node = NULL;
                goto get_node;
            }
            changeflag = 1;
        } else {
            _os_printf("%s %d\r\n", __FUNCTION__, __LINE__);
            goto creat_mp4_file_end;
        }
    }

    if (get_mp4_file_name(REC_PATH, sub_path, file_name)) {
        _os_printf("%s %d\tget_mp4_file_name fail\r\n", __FUNCTION__, __LINE__);
        goto creat_mp4_file_end;
    }
    os_sprintf(file_path, "%s/%s", sub_path, file_name);
    os_printf(KERN_INFO "mp4 file_path: %s\r\n", file_path);

    void *sub_dir = osal_opendir(sub_path);
    if (!sub_dir) {
        res = osal_fmkdir(sub_path);
        if (res != FR_OK) {
            if(res == FR_DENIED) {
                char *name = get_file_name(node);
                char path[64];
                os_sprintf(path, "%s/%s", dir_path, name);
                res = osal_unlink(path);
                if(res != FR_OK) {
                    _os_printf("%s %d\tunlink file %s fail, res: %d\r\n", __FUNCTION__, __LINE__, path, res);
                    goto creat_mp4_file_end;
                }
                _os_printf("unlink file %s\r\n", path);
                free_file_node(node);
                node = NULL;
                res = osal_fmkdir(sub_path);
                if (res == FR_OK) {
                    goto get_node;
                } else {
                    _os_printf("%s %d\tmkdir %s fail, res: %d\r\n", __FUNCTION__, __LINE__, sub_path, res);
                    goto creat_mp4_file_end;
                }
            }
            _os_printf("mkdir %s err, create rec dir\r\n", sub_path);

            void *rec_dir = osal_opendir(REC_PATH);
            if (!rec_dir) {
                res = osal_fmkdir(REC_PATH);
                if (res != FR_OK) {
                    _os_printf("%s %d\tmkdir %s fail, res: %d\r\n", __FUNCTION__, __LINE__, REC_PATH, res);
                    goto creat_mp4_file_end;
                } else {
                    res = osal_fmkdir(sub_path);
                    if (res != FR_OK) {
                        _os_printf("%s %d\tmkdir %s fail, res: %d\r\n", __FUNCTION__, __LINE__, sub_path, res);
                        goto creat_mp4_file_end;
                    }
                }
            } else {
                osal_closedir(rec_dir);
                _os_printf("%s %d\tmkdir %s fail, res: %d\r\n", __FUNCTION__, __LINE__, sub_path, res);
                goto creat_mp4_file_end;
            }
        }
        _os_printf("mkdir %s\r\n", sub_path);
    } else {
        osal_closedir(sub_dir);
    }

    if (changeflag) {
        char *name = get_file_name(node);
        char old_filepath[64];
        os_sprintf(old_filepath, "%s/%s", dir_path, name);
        res = osal_rename(old_filepath, file_path);
        if (res != FR_OK) {
            _os_printf("rename file %s err, res: %d\r\n", old_filepath, res);
        } else {
            _os_printf("rename file %s to %s\r\n", old_filepath, file_path);
        }
        char thumb_path[64];
        gen_thumb_path(name, thumb_path, sizeof(thumb_path));
        res = osal_unlink(thumb_path);
        if (res != FR_OK) {
            _os_printf("unlink thumb_path %s err, res: %d\r\n", thumb_path, res);
        } else {
            _os_printf("unlink thumb_path: %s\r\n", thumb_path);
        }

        free_file_node(node);
        node = NULL;

        if (os_strcmp(file_name, min_file) < 0) {
            free_file_list(*loop);
            *loop = NULL;
            _os_printf("%s %d\tfree_file_list\r\n", __FUNCTION__, __LINE__);
        }
    }

    fp = osal_fopen(file_path, "wb+");
creat_mp4_file_end:
    if(node) {
        free_file_node(node);
        node = NULL;
    }
    return fp;
}

void bbm_rec_loop_free(void **loop)
{
    if(*loop) {
        free_file_list(*loop);
        *loop = NULL;
    }
}

int32_t client_local_record_init(uint32_t record_time_minutes)
{
    AUENC_INIT auenc_init;
    if(!rec_msi) {
        h264_msi = msi_find(AUTO_H264, 1);
        if(h264_msi) {
            auenc_init.destroy_self = 0;
            auenc_init.src_msi = get_auadc_msi(AUSYS_AUAD);
            aac_msi = audio_encode_init(AAC_ENC, audio_adc_get_samplerate(AUSYS_AUAD), &auenc_init);
            if(!aac_msi) {
                msi_put(h264_msi);
                h264_msi = NULL;  
                return RET_ERR;              
            }
            rec_msi = mp4_encode_msi2_init("bbm_record_mp4",FRAMEBUFF_SOURCE_CAMERA0,FSTYPE_H264_VPP_DATA0, record_time_minutes, 
											AAC_ENC, bbm_rec_creat_mp4_file, bbm_rec_loop_free, 0);
            if(rec_msi) {
                msi_add_output(h264_msi, NULL, "bbm_record_mp4");
                audio_code_add_output(aac_msi, "bbm_record_mp4");
                return RET_OK;
            }
            else {
                msi_put(h264_msi);
                h264_msi = NULL;
                audio_encode_deinit(aac_msi);
                aac_msi = NULL;
                return RET_ERR;
            }
        }
        else {
            return RET_ERR;
        }
    }
    return RET_OK;
}

int32_t client_local_record_deinit(void)
{
    if(rec_msi) {
        if(h264_msi) {
            msi_del_output(h264_msi, NULL, "bbm_record_mp4");
        }
        if(aac_msi) {
            audio_code_del_output(aac_msi, "bbm_record_mp4");
            audio_encode_deinit(aac_msi);
        }
        msi_destroy(rec_msi);
        rec_msi = NULL;
        msi_put(h264_msi);
        h264_msi = NULL;
        aac_msi = NULL;
    }
    return RET_OK;    
}

#endif