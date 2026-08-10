#ifndef _HTTP_H
#define _HTTP_H
#include <stdarg.h>
#include "cjson/cJSON.h"
#include "osal/msgqueue.h"
struct httpClient ;
struct settingJson;
typedef void (*http_reponse)(struct httpClient *httpClient);
typedef void (*jsonReponse)(struct settingJson *settingJson,cJSON *cJSON);
typedef uint8_t (*http_queue)(struct httpClient *httpClient);

struct settingJson
{
	char *key;
	char type;
	jsonReponse reponse;
	struct settingJson *next;
};

//type:	0  	默认ip地址，不带参数
//		1	完整匹配(整个地址都要匹配)
//		2	首字符串匹配(匹配前部分,后面动态)
struct url
{
	const char *url;
	char type;
	http_reponse reponse;
	http_queue 	queue;
	//struct url *next;
};


//http头部的信息
struct httpEntry
{
    char *key;
    char *value;
	char *pattern;
    struct httpEntry* next;
};

struct httpserver 
{
	int fdServer;
	os_msgqueue_t thread_pool_queue;
};

struct httpClient 
{
	int fdClient;	//客户端的socket
	char *pattern;	//需要释放
	unsigned int time;
	struct httpserver *httpserver;	//不释放
	struct url *web;	//不能释放
	struct httpEntry *http_request;
};

struct httpClientTest 
{
	int fdClient;	//客户端的socket
};

struct httpIndex
{
	const char *Type;
	const char *Length;
	const char *Disposition;
	const char *Connection;
	const char *Accept_Range;
};

struct httpresp 
{
	char *headbuf;
	uint32_t msgLen;
	uint32_t msgMaxLen;
	uint8_t stat;
};

struct httpKeep
{
	struct event* read_event;		//tcp读取事件
	int fdClient;
};

struct url;
struct httpEntry;
struct httpserver;
struct httpClient;
struct settingJson;

extern struct httpIndex const httpIndex;

int config_http(int port);
int osal_set_reuseaddr(SOCK_HDL fd, int o);
struct httpClient *initHttpClient();
struct httpresp *http_create_reply(int code, char *reply);
void http_add_header(struct httpresp *resp, const char *name, char *value);
int http_header_send(struct httpresp *resp, const char *name, uint32_t len, int fd);
void closeRes(struct httpClient *httpClient);
void addWeb(struct url *web, const char *url, char type, http_reponse reponse, http_queue queue);
void http_reponse_Test(struct httpClient *httpClient);
void http_reponse_File(struct httpClient *httpClient);
void http_reponse_OJpg(struct httpClient *httpClient);
void http_reponse_TJpg(struct httpClient *httpClient);
void http_send_error(struct httpClient *httpClient);
uint8_t httpqueue(struct httpClient *httpClient);
void http_reponse_AVI(struct httpClient *httpClient);
void new_http_reponse_OJpg(struct httpClient *httpClient);
void new_http_reponse_TJpg(struct httpClient *httpClient);
void http_reponse_del(struct httpClient *httpClient);
void http_post_Test(struct httpClient *httpClient);
static void thread_rec_pool_queue(struct httpserver *httpserver);
static uint8_t httpqueue_thread_send_pool(struct httpClient *httpClient);
void http_thread_pool(struct httpClient *httpClient, struct url *gweb, struct url *pweb);


#endif
