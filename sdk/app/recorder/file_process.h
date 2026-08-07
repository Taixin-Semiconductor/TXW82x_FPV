#ifndef __FILE_PROCESS_H__
#define __FILE_PROCESS_H__

#include "basic_include.h"

struct file_process;

typedef void *(*create_file)(struct file_process *file_process, char *file_name, char *file_path, uint32_t file_size);
typedef void (*loop_free)(void **loop);
typedef void (*lock_file)(char *file_name, char *file_path);
typedef int32_t (*record_callback)(struct file_process *file_process);

struct file_process
{
    void            *loop;
    char            *rec_path;
    char            *ext_name;
    create_file     create_file;
    loop_free       loop_free;
    lock_file       lock_file;
    record_callback start_encode;   //param代表模式
    record_callback end_encode;     //param代表录制完毕后的返回码
    uint32_t        frame_time;
    uint32_t        param;
};

void *rec_create_file(struct file_process *file_process, char *file_name, char *file_path, uint32_t file_size);
void rec_loop_free(void **loop);

#endif