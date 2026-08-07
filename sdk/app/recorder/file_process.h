#ifndef __FILE_PROCESS_H__
#define __FILE_PROCESS_H__

#include "basic_include.h"

struct file_process;

typedef void *(*create_file)(struct file_process *file_process, char *file_name, char *file_path, uint32_t file_size);
typedef void (*loop_free)(void **loop);
typedef void (*lock_file)(char *file_name, char *file_path);

struct file_process
{
    void            *loop;
    char            *rec_path;
    char            *ext_name;
    create_file     create_file;
    loop_free       loop_free;
    lock_file       lock_file;
};

void *rec_create_file(struct file_process *file_process, char *file_name, char *file_path, uint32_t file_size);
void rec_loop_free(void **loop);

#endif