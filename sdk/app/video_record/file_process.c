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

#ifndef LOOP_REMAIN_CAP
#define LOOP_REMAIN_CAP (256) // 循环录像剩余空间控制 MB
#endif

struct mult_record mult_record = {0};

static int rec_refresh_file_list(struct file_process *file_process)
{
    void *err_dir_list;
    void *new_loop;

    if (!file_process || !file_process->loop) {
        return RET_ERR;
    }

    err_dir_list = get_err_dir_list(file_process->loop);
    new_loop = get_file_list2(file_process->rec_path, file_process->ext_name, err_dir_list);
    if (!new_loop) {
        os_printf("%s %d, get file list fail!\r\n", __FUNCTION__, __LINE__);
        return RET_ERR;
    }

    free_file_list(file_process->loop);
    file_process->loop = new_loop;
    return 0;
}

static int rec_refresh_file_list_earlier(struct file_process *file_process, const char *file_name)
{
    char *min_file;

    min_file = get_min_file(file_process->loop);
    if (!min_file || os_strcmp(file_name, min_file) >= 0) {
        return 0;
    }

    if (rec_refresh_file_list(file_process)) {
        return RET_ERR;
    }

    os_printf("%s %d, refresh file list\r\n", __FUNCTION__, __LINE__);
    return 0;
}

static int rec_take_oldest_file(struct file_process *file_process, void **node)
{
    char *dir_path;
    int   res;

    if (!file_process || !file_process->loop || !node) {
        return RET_ERR;
    }

    *node = get_file_node(file_process->loop);
    if (!*node) {
        if (rec_refresh_file_list(file_process)) {
            return RET_ERR;
        }
        *node = get_file_node(file_process->loop);
    }

    while (!*node) {
        dir_path = get_file_dir(file_process->loop);
        res = osal_unlink_dir(dir_path, 0);
        if (res != FR_OK) {
            os_printf("unlink dir %s err, res: %d\r\n", dir_path, res);
            if (res == FR_DENIED) {
                os_printf("%s %d, add dir %s to err list\r\n", __FUNCTION__, __LINE__, dir_path);
                err_dir_add_list(file_process->loop, dir_path);
            } else if (res == FR_NO_FILE) {
                os_printf("%s %d, could not find dir %s\r\n", __FUNCTION__, __LINE__, dir_path);
            } else {
                return RET_ERR;
            }
        } else {
            os_printf("unlink dir %s\r\n", dir_path);
        }

        if (rec_refresh_file_list(file_process)) {
            return RET_ERR;
        }
        *node = get_file_node(file_process->loop);
    }

    return 0;
}

static void rec_delete_thumb(const char *file_name)
{
    char thumb_path[64];
    int res;

    gen_thumb_path(file_name, thumb_path, sizeof(thumb_path));
    res = osal_unlink(thumb_path);
    if (res != FR_OK) {
        os_printf("unlink thumb_path %s err, res: %d\r\n", thumb_path, res);
    } else {
        os_printf("unlink thumb_path: %s\r\n", thumb_path);
    }
}

static int rec_delete_file(struct file_process *file_process, void *node)
{
    char  path[64];
    char *dir_path;
    char *file_name;
    int   res;

    file_name = get_file_name(node);
    dir_path = get_file_dir(file_process->loop);
    os_snprintf(path, sizeof(path), "%s/%s", dir_path, file_name);

    res = osal_unlink(path);
    if (res != FR_OK) {
        os_printf("%s %d, unlink file %s fail, res: %d\r\n", __FUNCTION__, __LINE__, path, res);
        return res;
    }

    os_printf("unlink file %s\r\n", path);
    rec_delete_thumb(file_name);
    return FR_OK;
}

static int rec_get_free_size(uint32_t *free_size, uint8_t retry_timeout)
{
    uint8_t retry_count = 0;
    int     res;

    do {
        res = osal_fatfsfree("0:", NULL, free_size);
        if (res != FR_TIMEOUT || !retry_timeout) {
            break;
        }

        if (++retry_count > 3) {
            os_printf("get sd free size timeout, exit\r\n");
            break;
        }

        os_printf("get sd free size timeout, try again\r\n");
        os_sleep_ms(500);
    } while (1);

    return res;
}

static int rec_prepare_reuse_file(struct file_process *file_process, uint32_t file_size, uint32_t *reclaimed_size,
                                  uint8_t *unlink_retried, void **node)
{
    uint32_t old_file_size;
    int      res;

    if (!file_process->loop) {
        file_process->loop = get_file_list2(file_process->rec_path, file_process->ext_name, NULL);
    }
    if (!file_process->loop) {
        os_printf("%s %d, get file list fail\r\n", __FUNCTION__, __LINE__);
        return RET_ERR;
    }

    while (1) {
        if (rec_take_oldest_file(file_process, node)) {
            return RET_ERR;
        }

        old_file_size = get_file_size(*node);
        os_printf("earliest file size: %d\r\n", old_file_size);
        if (*reclaimed_size + old_file_size >= file_size) {
            return 0;
        }

        res = rec_delete_file(file_process, *node);
        free_file_node(*node);
        *node = NULL;
        if (res == FR_OK) {
            *reclaimed_size += old_file_size;
            continue;
        }

        if (res == FR_NO_FILE && !*unlink_retried) {
            *unlink_retried = 1;
            if (rec_refresh_file_list(file_process)) {
                return RET_ERR;
            }
            continue;
        }
        return RET_ERR;
    }
}

static int rec_make_file_path(struct file_process *file_process, char *sub_path, char *file_name, char *file_path)
{
    struct timeval time;

    gettimeofday(&time, NULL);
    if (file_process->frame_time) {
        uint32_t now_tick = os_jiffies();
        uint32_t elapsed_ms = now_tick - file_process->frame_time;

        time.tv_sec -= elapsed_ms / 1000;
        time.tv_usec -= (elapsed_ms % 1000) * 1000;
        if (time.tv_usec < 0) {
            time.tv_sec--;
            time.tv_usec += 1000000;
        }
    }

    if (get_extension_file_name_time(file_process->rec_path, sub_path, file_name, file_process->ext_name, &time)) {
        os_printf("%s %d, get file name fail\r\n", __FUNCTION__, __LINE__);
        return RET_ERR;
    }

    os_snprintf(file_path, 64, "%s/%s", sub_path, file_name);
    os_printf(KERN_INFO "file_path: %s\r\n", file_path);
    return 0;
}

static int rec_ensure_record_dir(struct file_process *file_process, const char *sub_path)
{
    void *dir;
    int   res;

    dir = osal_opendir(sub_path);
    if (dir) {
        osal_closedir(dir);
        return FR_OK;
    }

    res = osal_fmkdir(sub_path);
    if (res == FR_OK) {
        os_printf("mkdir %s\r\n", sub_path);
        return FR_OK;
    }

    if (res != FR_NO_PATH) {
        os_printf("%s %d, mkdir %s fail, res: %d\r\n", __FUNCTION__, __LINE__, sub_path, res);
        return res;
    }

    os_printf("mkdir dir %s\r\n", file_process->rec_path);
    dir = osal_opendir(file_process->rec_path);
    if (dir) {
        osal_closedir(dir);
        os_printf("%s %d, mkdir %s fail, res: %d\r\n", __FUNCTION__, __LINE__, sub_path, res);
        return res;
    }

    res = osal_fmkdir(file_process->rec_path);
    if (res != FR_OK) {
        os_printf("%s %d, mkdir %s fail, res: %d\r\n", __FUNCTION__, __LINE__, file_process->rec_path, res);
        return res;
    }

    res = osal_fmkdir(sub_path);
    if (res != FR_OK) {
        os_printf("%s %d, mkdir %s fail, res: %d\r\n", __FUNCTION__, __LINE__, sub_path, res);
        return res;
    }

    os_printf("mkdir %s\r\n", sub_path);
    return FR_OK;
}

static int rec_rename_reuse_file(struct file_process *file_process, void **node, const char *file_name, const char *file_path,
                                 uint32_t *sd_cap, uint32_t *reclaimed_size, uint8_t *rename_retried, uint8_t *retry_reclaim)
{
    char old_path[64];
    char *old_name;
    char *old_dir;
    uint32_t old_file_size;
    int   res;

    *retry_reclaim = 0;
    old_name = get_file_name(*node);
    old_dir = get_file_dir(file_process->loop);
    os_snprintf(old_path, sizeof(old_path), "%s/%s", old_dir, old_name);

    res = osal_rename(old_path, file_path);
    if (res == FR_OK) {
        os_printf("rename file %s to %s\r\n", old_path, file_path);
        rec_delete_thumb(old_name);
        free_file_node(*node);
        *node = NULL;
        rec_refresh_file_list_earlier(file_process, file_name);
        return FR_OK;
    }

    os_printf("rename file %s err, res: %d\r\n", old_path, res);
    if (res == FR_DENIED && *sd_cap < LOOP_REMAIN_CAP) {
        old_file_size = get_file_size(*node);
        res = rec_delete_file(file_process, *node);
        if (res != FR_OK) {
            return res;
        }
        *reclaimed_size += old_file_size;

        res = rec_get_free_size(sd_cap, 0);
        if (res != FR_OK) {
            return res;
        }
        os_printf("%s %d, sd_cap: %d\r\n", __FUNCTION__, __LINE__, *sd_cap);

        free_file_node(*node);
        *node = NULL;
        *retry_reclaim = 1;
        return FR_OK;
    }

    if (res == FR_NO_FILE && !*rename_retried) {
        *rename_retried = 1;
        free_file_node(*node);
        *node = NULL;
        if (rec_refresh_file_list(file_process)) {
            return RET_ERR;
        }

        *retry_reclaim = 1;
        return FR_OK;
    }

    return res;
}

void *rec_create_file(struct file_process *file_process, char *file_name, char *file_path, uint32_t file_size)
{
    void     *fp = NULL;
    void     *node = NULL;
    uint32_t  sd_cap = 0;
    uint32_t  reclaimed_size = 0;
    uint8_t   unlink_retried = 0;
    uint8_t   rename_retried = 0;
    uint8_t   retry_rename = 0;
    char      sub_path[32];
    int       res;

    res = rec_get_free_size(&sd_cap, 1);
    if (res != FR_OK) {
        os_printf("%s %d, get free err, res: %d\r\n", __FUNCTION__, __LINE__, res);
        goto rec_create_file_end;
    }
    os_printf(KERN_INFO "sd_cap: %d MB, file_size: %d MB\r\n", sd_cap, file_size / (1024 * 1024));

retry_reclaim:
    if (sd_cap < LOOP_REMAIN_CAP) {
        if (rec_prepare_reuse_file(file_process, file_size, &reclaimed_size, &unlink_retried, &node)) {
            goto rec_create_file_end;
        }
    }

    if (rec_make_file_path(file_process, sub_path, file_name, file_path)) {
        goto rec_create_file_end;
    }

    res = rec_ensure_record_dir(file_process, sub_path);
    if (res != FR_OK) {
        if (res == FR_DENIED && node && file_process->loop) {
            uint32_t old_file_size = get_file_size(node);

            res = rec_delete_file(file_process, node);
            if (res != FR_OK) {
                goto rec_create_file_end;
            }
            reclaimed_size += old_file_size;
            free_file_node(node);
            node = NULL;

            res = rec_get_free_size(&sd_cap, 0);
            if (res != FR_OK) {
                os_printf("%s %d, get free err, res: %d\r\n", __FUNCTION__, __LINE__, res);
                goto rec_create_file_end;
            }
            os_printf("%s %d, sd_cap: %d\r\n", __FUNCTION__, __LINE__, sd_cap);
            goto retry_reclaim;
        }
        goto rec_create_file_end;
    }

    if (node) {
        res = rec_rename_reuse_file(file_process, &node, file_name, file_path, &sd_cap, 
                                    &reclaimed_size, &rename_retried, &retry_rename);
        if (res != FR_OK) {
            goto rec_create_file_end;
        }
        if (retry_rename) {
            goto retry_reclaim;
        }
    }

    fp = osal_fopen(file_path, "a+");

rec_create_file_end:
    if (node) {
        free_file_node(node);
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

