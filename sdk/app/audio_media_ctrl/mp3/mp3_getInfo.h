#ifndef _MP3_GETINFO_MSI_H_
#define _MP3_GETINFO_MSI_H_

#include "basic_include.h"

typedef struct {
    struct list_head sub_table_list;
    uint32_t sub_table[100];
}MP3_SEEK_TABLE;

typedef struct {
    uint32_t total_size;
    uint32_t first_frame_offset;
    uint32_t normal_frame_offset; 
    uint32_t total_frame;  
    uint32_t once_move_time;
	uint32_t offset_seek;
    uint32_t samplingrate;
    uint32_t one_second_frame;
    int32_t playtime;      
    int32_t play_frame;

    uint16_t totaltime;     
    uint16_t samples;

    uint8_t get_totaltime;
    uint8_t update_fops;

	uint8_t bitrate_table_index;
    uint8_t padding_mul;

    void *mp3_fp; 
    void *mp3_st_fp;
    char mp3_st_filename[50];  

    struct list_head seek_table_head;
}CUR_MP3_INFO;
CUR_MP3_INFO *cur_mp3_info;

struct os_semaphore seek_sema;

uint32_t get_curmp3_size(uint32_t s);
void curmp3_info_init(uint8_t *mp3_filename);
void find_first_frame(void *fp);

#endif