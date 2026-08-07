#ifndef _LOOP_RECORD_MOUDLE_H_
#define _LOOP_RECORD_MOUDLE_H_

#include <stdint.h>

void *get_file_list(const char *rec_path, const char *extension_name);
void free_file_list(void *loop_f);
void *get_file_node(void *loop_f);
void free_file_node(void *node);
char *get_file_name(void *node);
uint32_t get_file_size(void *node);
void *get_file_dir(void *loop_f);
void *get_min_file(void *loop_f);

#endif