#ifndef _LOOP_RECORD_MOUDLE_H_
#define _LOOP_RECORD_MOUDLE_H_

#include <stdint.h>

void free_list(void *loop_f);
void* get_err_dir_list(void *loop_f);
void err_dir_add_list(void *loop_f, const char *dir_name);
void *get_file_list(const char *rec_path, const char *extension_name);
void *get_file_list2(const char *rec_path, const char *extension_name, void *list);
void free_file_list(void *loop_f);
void *get_file_node(void *loop_f);
void free_file_node(void *node);
char *get_file_name(void *node);
uint32_t get_file_size(void *node);
void *get_file_dir(void *loop_f);
void *get_min_file(void *loop_f);

#endif