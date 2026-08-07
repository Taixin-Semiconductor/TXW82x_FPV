#ifndef __VIDEO_APP_H
#define __VIDEO_APP_H 
#include "sys_config.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "typesdef.h"
#include "stream_frame.h"


extern void *get_jpeg_node_buf(void *d);
extern int get_node_count(volatile struct list_head *head);
extern void del_jpeg_node(void *d,uint32_t jpg_num);
extern int get_jpg_node_len();
extern uint32 get_jpg_node_len_new(void *get_f);
extern void jpg_recfg();
extern void jpg_start(uint8 jpgid);
extern void jpg_stop(uint32_t jpg_num);
extern bool csi_recfg();
extern bool csi_open();
extern bool csi_close();
void jpeg_stream_deinit();
stream* jpeg_stream_init();
void stop_jpeg();
void start_jpeg();

#endif