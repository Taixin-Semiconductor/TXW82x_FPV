#ifndef __JPGDEF_H
#define __JPGDEF_H

#include "sys_config.h"

struct stream_jpeg_data_s
{
	struct stream_jpeg_data_s *next;
	void *data;
	int ref;
};


enum
{
	CUSTOM_GET_NODE_LEN,
	CUSTOM_GET_NODE_BUF,
	CUSTOM_GET_FIRST_BUF,
	CUSTOM_FREE_NODE,
	CUSTOM_DEL_NODE,
	CUSTOM_GET_NODE_COUNT,
};

typedef int (*commom_free_node)(void *get_f);
typedef unsigned int (*commom_len)(void *get_f);
typedef unsigned int (*commom_node_len)(void *get_f);
typedef void *(*commom_buf)(void *get_f);
typedef void (*commom_del)(void *get_f);
typedef unsigned int (*commom_get_time)(void *get_f);

typedef struct 
{
	int 			 type;
	commom_free_node free_node;
	commom_node_len	 get_node_len;
	commom_len		 get_len;
	commom_buf       get_buf;
	commom_del       del;
	commom_get_time	 get_time;

}common_ops_s;

typedef struct 
{
	void *priv1,*priv2;//为了兼容jpg.c的frame结构体(struct list_head list),所以去除前面的8字节,后面usb一样
	//注册一个通用的结构体,作为操作帧,通用形(这里用void指针,到.c后赋值一个实体)
	common_ops_s *common_ops;
}common_s;




#if 0
#else
#include "stream_define.h"
#include "basic_include.h"
#include "lib/multimedia/msi.h"
#define DEL_JPG_FRAME(f)					msi_delete_fb(NULL,(struct framebuff *)f)
#endif






#endif
