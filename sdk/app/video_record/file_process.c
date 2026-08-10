/******************************************************************************
 *                     这个文件用于录卡文件处理                               *
 *                                                                            *
 ******************************************************************************/

#include "basic_include.h"
#include "file_process.h"
#include "video_record.h"
#include "fs/fatfs/osal_file.h"
#include "video_app/file_thumb.h"
#include "video_app/file_common_api.h"
#include "loop_record_moudle/loop_record_moudle.h"
#include "lib/common/timezone.h"

#define LOOP_REMAIN_CAP (256) // 循环录像剩余空间控制

struct mult_record mult_record = {0};

void *rec_create_file(struct file_process *file_process, char *file_name, char *file_path, uint32_t file_size)
{
    void        *fp             = NULL;
    void        *node           = NULL;
    char        *dir_path       = NULL;
    void        *list           = NULL;
    uint32_t    sd_cap          = 0;
    int         res             = 0;
    uint8_t     changeflag      = 0;
    uint32_t    old_file_size   = 0;
    uint32_t    unlink_size     = 0;
    void        **loop          = &file_process->loop;
    char        *main_path      = file_process->rec_path;
    char        *extension_name = file_process->ext_name;
    uint8_t     try_counts      = 0;
    char        sub_path[32];
    char        thumb_path[64];
    
get_again:
    res = osal_fatfsfree("0:", NULL, &sd_cap);
    if (res == FR_OK) {
        os_printf(KERN_INFO"sd_cap: %d, file_size: %d\r\n", sd_cap, file_size);
    } else if (res == FR_TIMEOUT) {
        try_counts++;
        if(try_counts <= 3) {
            os_printf("get sd free size timeout, try again\r\n");
            os_sleep_ms(500);
            goto get_again;
        } else {
            os_printf("get sd free size timeout, exit\r\n");
            goto rec_create_file_end;
        }
    } else {
        goto rec_create_file_end;
    }

rec_create_file_get_node:
    if (sd_cap < LOOP_REMAIN_CAP)
    {
        if (!*loop) {
            *loop = get_file_list2(main_path, extension_name, list);
        }

        if (*loop) {
            node = get_file_node(*loop);
            if (!node) {
                list = get_err_dir_list(*loop);
                free_file_list(*loop);
                *loop = get_file_list2(main_path, extension_name, list);
                if (!*loop) {
                    os_printf("%s %d\r\n", __FUNCTION__, __LINE__);
                    goto rec_create_file_end;
                }
                node = get_file_node(*loop);
            }
            while (!node) {
                dir_path = get_file_dir(*loop);
                res = osal_unlink_dir(dir_path, 0);
                if (res != FR_OK) {
                    os_printf("unlink dir %s err, res: %d\r\n", dir_path, res);
                    if(res == FR_DENIED) {
                        err_dir_add_list(*loop, dir_path);
                        // res = osal_unlink_dir(dir_path, 1);
                        // if (res != FR_OK) {
                        //     _os_printf("%s %d, force unlink dir %s err, res: %d\r\n", __FUNCTION__, __LINE__, dir_path, res);
                        //     err_dir_add_list(*loop, dir_path);
                        // } else {
                        //     _os_printf("force unlink dir %s\r\n", dir_path);
                        // }
                    } else if(res == FR_NO_FILE) {
                        os_printf("%s %d, could not find dir %s\r\n", __FUNCTION__, __LINE__, dir_path);
                    } else {
                        os_printf("%s %d\r\n", __FUNCTION__, __LINE__);
                        goto rec_create_file_end;
                    }
                } else {
                    os_printf("unlink dir %s\r\n", dir_path);
                }
                
                list = get_err_dir_list(*loop);
                free_file_list(*loop);
                *loop = get_file_list2(main_path, extension_name, list);
                if (!*loop) {
                    os_printf("%s %d\r\n", __FUNCTION__, __LINE__);
                    goto rec_create_file_end;
                }
                node = get_file_node(*loop);
                if (node) {
                    break;
                }
            }
            old_file_size = get_file_size(node);
            os_printf("earliest file size: %d\r\n", old_file_size);
            if(unlink_size + old_file_size < file_size) {
                char path[64];
                char *name = get_file_name(node);
                dir_path = get_file_dir(*loop);
                os_sprintf(path, "%s/%s", dir_path, name);
                res = osal_unlink(path);
                if(res != FR_OK) {
                    os_printf("%s %d\tunlink file %s fail, res: %d\r\n", __FUNCTION__, __LINE__, path, res);
                } else {
                    os_printf("unlink file %s\r\n", path);
                    unlink_size += old_file_size;
                }
                
                if(res == FR_OK) {
                    gen_thumb_path(name, thumb_path, sizeof(thumb_path));
                    res = osal_unlink(thumb_path);
                    if (res != FR_OK) {
                        os_printf("unlink thumb_path %s err, res: %d\r\n", thumb_path, res);
                    } else {
                        os_printf("unlink thumb_path: %s\r\n", thumb_path);
                    }
                }
                free_file_node(node);
                node = NULL;
                goto rec_create_file_get_node;
            }
            changeflag = 1;
        } else {
            os_printf("%s %d\r\n", __FUNCTION__, __LINE__);
            goto rec_create_file_end;
        }
    }

    struct timeval time;
    gettimeofday(&time, NULL);
    if(file_process->frame_time)
    {
        uint32_t now_tick = os_jiffies();
        uint32_t elapsed_ms = now_tick - file_process->frame_time;
        time.tv_sec -= elapsed_ms / 1000;
        time.tv_usec -= (elapsed_ms % 1000) * 1000;
        if (time.tv_usec < 0) {
            time.tv_sec--;
            time.tv_usec += 1000000;
        }
    }
    if (get_extension_file_name_time(main_path, sub_path, file_name, extension_name, &time)) {
        os_printf("%s %d\tget_file_name fail\r\n", __FUNCTION__, __LINE__);
        goto rec_create_file_end;
    }
    os_sprintf(file_path, "%s/%s", sub_path, file_name);
    os_printf(KERN_INFO "file_path: %s\r\n", file_path);

    void *sub_dir = osal_opendir(sub_path);
    if (!sub_dir) {
        res = osal_fmkdir(sub_path);
        if (res != FR_OK) {
            if(res == FR_DENIED) {
                char path[64];
                char *name = get_file_name(node);
                dir_path = get_file_dir(*loop);
                os_sprintf(path, "%s/%s", dir_path, name);
                res = osal_unlink(path);
                if(res != FR_OK) {
                    os_printf("%s %d\tunlink file %s fail, res: %d\r\n", __FUNCTION__, __LINE__, path, res);
                    goto rec_create_file_end;
                }
                os_printf("unlink file %s\r\n", path);
                free_file_node(node);
                node = NULL;
                res = osal_fmkdir(sub_path);
                if (res == FR_OK) {
                    goto rec_create_file_get_node;
                } else {
                    os_printf("%s %d\tmkdir %s fail, res: %d\r\n", __FUNCTION__, __LINE__, sub_path, res);
                    goto rec_create_file_end;
                }
            }
            os_printf("mkdir %s err, create rec dir\r\n", sub_path);

            void *rec_dir = osal_opendir(main_path);
            if (!rec_dir) {
                res = osal_fmkdir(main_path);
                if (res != FR_OK) {
                    os_printf("%s %d\tmkdir %s fail, res: %d\r\n", __FUNCTION__, __LINE__, main_path, res);
                    goto rec_create_file_end;
                } else {
                    res = osal_fmkdir(sub_path);
                    if (res != FR_OK) {
                        os_printf("%s %d\tmkdir %s fail, res: %d\r\n", __FUNCTION__, __LINE__, sub_path, res);
                        goto rec_create_file_end;
                    }
                }
            } else {
                osal_closedir(rec_dir);
                os_printf("%s %d\tmkdir %s fail, res: %d\r\n", __FUNCTION__, __LINE__, sub_path, res);
                goto rec_create_file_end;
            }
        }
        os_printf("mkdir %s\r\n", sub_path);
    } else {
        osal_closedir(sub_dir);
    }

    if (changeflag) {
        char old_filepath[64];
        char *name = get_file_name(node);
        dir_path = get_file_dir(*loop);
        os_sprintf(old_filepath, "%s/%s", dir_path, name);
        res = osal_rename(old_filepath, file_path);
        if (res != FR_OK) {
            os_printf("rename file %s err, res: %d\r\n", old_filepath, res);
            if (sd_cap < LOOP_REMAIN_CAP) {
                if(res == FR_DENIED) {
                    res = osal_unlink(old_filepath);
                    if(res != FR_OK) {
                        os_printf("%s %d\tunlink file %s fail, res: %d\r\n", __FUNCTION__, __LINE__, old_filepath, res);
                        goto rec_create_file_end;
                    } else {
                        os_printf("unlink file %s\r\n", old_filepath);
                        res = osal_fatfsfree("0:", NULL, &sd_cap);
                        if(res != FR_OK) {
                            os_printf("%s %d\tfatfsfree err, res: %d\r\n", __FUNCTION__, __LINE__, res);
                            goto rec_create_file_end;
                        }
                    }
                }
                os_printf("%s %d\tsd_cap: %d\r\n", __FUNCTION__, __LINE__, sd_cap);
                free_file_node(node);
                node = NULL;
                goto rec_create_file_get_node;
            }
        } else {
            os_printf("rename file %s to %s\r\n", old_filepath, file_path);
        }

        if(res == FR_OK) {
            gen_thumb_path(name, thumb_path, sizeof(thumb_path));
            res = osal_unlink(thumb_path);
            if (res != FR_OK) {
                os_printf("unlink thumb_path %s err, res: %d\r\n", thumb_path, res);
            } else {
                os_printf("unlink thumb_path: %s\r\n", thumb_path);
            }
        }
        free_file_node(node);
        node = NULL;

        char *min_file = get_min_file(*loop);
        if (min_file && (os_strcmp(file_name, min_file) < 0)) {
            list = get_err_dir_list(*loop);
            free_file_list(*loop);
            *loop = NULL;
            os_printf("%s %d\tfree_file_list\r\n", __FUNCTION__, __LINE__);
        }
    }

    fp = osal_fopen(file_path, "a+");
rec_create_file_end:
    if(node) {
        free_file_node(node);
        node = NULL;
    }
    return fp;
}

void rec_loop_free(void **loop)
{
    if(*loop) {
        free_list(*loop);
        free_file_list(*loop);
        *loop = NULL;
    }
}

