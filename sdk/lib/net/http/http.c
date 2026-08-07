#include <csi_config.h>
// #include <test_kernel_config.h>
#include "csi_kernel.h"
#include "lwip/netif.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "netif/ethernetif.h"
#include "typesdef.h"
#include "osal/mutex.h"
#include "lib/net/eloop/eloop.h"
#include "event.h"
#include "sys_config.h"
#include "lwip/tcpip.h"
#include "osal/sleep.h"
#include "dev/csi/hgdvp.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "dev/jpg/hgjpg.h"
#include "jpgdef.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "osal/msgqueue.h"
#include "basic_include.h"
#include "cjson/cJSON.h"
#include "http.h"
#include "netif/ethernet.h"
#include "lwip/def.h"
#include "lwip/stats.h"
#include "lwip/etharp.h"
#include "lwip/ip.h"
#include "lwip/snmp.h"
#include "syscfg.h"
#include "fs/fatfs/osal_file.h"
#include "app/video_app/file_thumb.h"
#include "video_app_h264_msi.h"
#include "audio_media_ctrl/audio_code_ctrl.h"
#include "audio_msi/audio_adc.h"

#define addWeb(web, url, type,reponse,queue) { url,type,reponse,queue }

// 结构体申请空间函数
#ifdef MORE_SRAM 
#define STREAM_LIBC_MALLOC os_malloc_psram
#define STREAM_LIBC_FREE os_free_psram
#define STREAM_LIBC_ZALLOC os_zalloc_psram
#else
#define STREAM_LIBC_MALLOC os_malloc
#define STREAM_LIBC_FREE os_free
#define STREAM_LIBC_ZALLOC os_zalloc
#endif

extern struct msi *h264_msi_init();
extern struct msi *mp4_encode_msi_init(const char *mp4_msi_name,uint16_t filter_type);
extern struct msi *jpg_thumb_msi_init(const char *msi_name, uint16_t filter);

void free_http_head(struct httpresp *resp);
typedef char *ERR_STR;

#define WEBURLNUM 5

const char *const http404 = "<html><body><p>URL is err</p></body></html>";

const char webstr_ret[][100] = {
	{"<html>\r\n"},
	{"<head>\r\n"},
	{"	  <meta http-equiv=\"refresh\" content=\"0; url=/web\" />\r\n"},
	{"</head>\r\n"},
	{"<body>\r\n"},
	{"</body>\r\n"},
	{"</html>\r\n"},
};

const char webstr[][200] = {
    {"<!DOCTYPE html>\r\n"},
    {"<html lang=\"zh\">\r\n"},
    {"<head>\r\n"},
    {"	  <meta charset=\"UTF-8\">\r\n"},
    {"	  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\r\n"},
    {"	  <title>EAGLE 功能选择</title>\r\n"},
    {"	  <style>\r\n"},
    {"		  body { \r\n"},
    {"	  font-family: Arial, sans-serif;\r\n"},
    {"	  margin: 0;\r\n"},
    {"	  background-color: #e0f7fa; /* 浅蓝色背景 */\r\n"},
    {"}\r\n"},
    {"\r\n"},
    {"/* 包含整个内容的大矩形容器 */\r\n"},
    {".container-wrapper {\r\n"},
    {"	  display: flex;\r\n"},
    {"	  justify-content: center;\r\n"},
    {"	  align-items: center;\r\n"},
    {"	  min-height: 100vh;\r\n"},
    {"	  padding: 20px;\r\n"},
    {"}\r\n"},
    {"\r\n"},
    {"/* 页面主要内容容器 */\r\n"},
    {".container {\r\n"},
    {"	  display: flex;\r\n"},
    {"	  flex-direction: column;\r\n"},
    {"	  align-items: center;\r\n"},
    {"	  padding: 20px;\r\n"},
    {"	  background-color: #ffffff; /* 白色背景 */\r\n"},
    {"	  border-radius: 20px; /* 圆角效果 */\r\n"},
    {"	  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); /* 轻微阴影 */\r\n"},
    {"	  width: 100%;\r\n"},
    {"	  max-width: 800px; /* 最大宽度 */\r\n"},
    {"}\r\n"},
    {"\r\n"},
    {"/* 顶部标题样式 */\r\n"},
    {".header-title {\r\n"},
    {"	  font-size: 28px; /* 大字体大小 */\r\n"},
    {"	  font-weight: bold;\r\n"},
    {"	  color: #000; /* 黑色字体 */\r\n"},
    {"	  margin-bottom: 10px; /* 与内容间距 */\r\n"},
    {"}\r\n"},
    {"\r\n"},
    {"/* 标题区域的样式 */\r\n"},
    {".title-box {\r\n"},
    {"	  display: flex;\r\n"},
    {"	  align-items: center;\r\n"},
    {"	  justify-content: center;\r\n"},
    {"	  background-color: #ffffff; /* 白色背景 */\r\n"},
    {"	  padding: 0px;\r\n"},
    {"	  font-size: 16px; /* 与按钮一致 */\r\n"},
    {"	  color: #0000CD; /* 按钮字体颜色 */\r\n"},
    {"	  margin-bottom: 1px; /* 调整底部间距 */\r\n"},
    {"}\r\n"},
    {"/* 按钮容器的样式 */\r\n"},
    {".button-box {\r\n"},
    {"	  border: 2px solid #f0f0f0; /* 边框颜色为灰白色，边框宽度为 2px */\r\n"},
    {"	  border-radius: 10px; /* 边框圆角半径设置为 10px */\r\n"},
    {"	  padding: 10px; /* 内边距设置为 10px */\r\n"},
    {"	  background-color: #ffffff; /* 白色背景 */\r\n"},
    {"	  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); /* 轻微阴影 */\r\n"},
    {"	  margin-top: 20px; /* 顶部外边距设置为 20px */\r\n"},
    {"	  margin-bottom: 20px; /* 底部外边距设置为 20px */\r\n"},
    {"	  width: 100%; /* 宽度设置为 100% */\r\n"},
    {"	  max-width: 800px; /* 最大宽度设置为 800px */\r\n"},
    {"}\r\n"},
    {"\r\n"},
    {"/* 按钮行的样式 */\r\n"},
    {".button-row {\r\n"},
    {"	  display: flex; /* 使用 Flexbox 布局 */\r\n"},
    {"	  flex-wrap: wrap; /* 允许子元素换行 */\r\n"},
    {"	  justify-content: center; /* 子元素在主轴方向居中对齐 */\r\n"},
    {"	  margin: 0 -5px; /* 调整按钮行的左右间距 */\r\n"},
    {"}\r\n"},
    {"\r\n"},
    {"/* 按钮的基本样式 */\r\n"},
    {".button { \r\n"},
    {"	  padding: 10px 20px; /* 内边距上下设置为 10px，左右设置为 20px */\r\n"},
    {"	  margin: 5px; /* 按钮之间的间距设置为 5px */\r\n"},
    {"	  font-size: 16px; /* 字体大小设置为 16px */\r\n"},
    {"	  cursor: pointer; /* 鼠标悬停时显示为手型 */\r\n"},
    {"	  border: none; /* 去除按钮的边框 */\r\n"},
    {"	  background-color: #f0f0f0; /* 背景颜色设置为灰色 */\r\n"},
    {"	  color: #007bff; /* 按钮字体颜色设置为蓝色 */\r\n"},
    {"	  border-radius: 10px; /* 边框圆角半径设置为 10px */\r\n"},
    {"	  transition: background-color 0.3s, transform 0.2s; /* 设置背景颜色和变换的过渡效果 */\r\n"},
    {"	  flex: 1 1 23%; /* 设置按钮的灵活性，基础宽度占容器的 23% */\r\n"},
    {"	  box-sizing: border-box; /* 确保内边距和边框包含在元素宽度的计算中 */\r\n"},
    {"}\r\n"},
    {"\r\n"},
    {"/* 鼠标悬停时按钮的样式 */\r\n"},
    {".button:hover { \r\n"},
    {"	  background-color: #b0b0b0; /* 悬停时按钮的背景颜色变为浅灰色 */\r\n"},
    {"	  transform: scale(1.05); /* 鼠标悬停时按钮略微放大 */\r\n"},
    {"}\r\n"},
    {"\r\n"},
    {"/* 底部小号字体标题样式 */\r\n"},
    {".footer-title {\r\n"},
    {"	  font-size: 14px; /* 小号字体 */\r\n"},
    {"	  color: #000; /* 黑色字体 */\r\n"},
    {"	  margin-top: 20px; /* 顶部间距 */\r\n"},
    {"	  text-align: center; /* 居中对齐 */\r\n"},
    {"}\r\n"},
    {"\r\n"},
    {"/* 媒体查询：在屏幕宽度小于 600px 时应用以下样式 */\r\n"},
    {"@media (max-width: 600px) {\r\n"},
    {"	  .button {\r\n"},
    {"		  font-size: 14px; /* 减少按钮的字体大小 */\r\n"},
    {"		  padding: 8px 16px; /* 减少按钮的内边距 */\r\n"},
    {"		  flex: 1 1 22%; /* 调整按钮的宽度占比，以适应较小的屏幕 */\r\n"},
    {"	  }\r\n"},
    {"	  .button-row {\r\n"},
    {"		  margin: 0 -3px; /* 调整按钮行的左右间距，以适应较小的屏幕 */\r\n"},
    {"	  }\r\n"},
    {"}\r\n"},
    {"\r\n"},
    {"	  </style>\r\n"},
    {"</head>\r\n"},
    {"<body>\r\n"},
    {"	<div class=\"container-wrapper\">\r\n"},
    {"	  <div class=\"container\">\r\n"},
    {"		<!-- 顶部大标题 -->\r\n"},
    {"		  <div class=\"header-title\">TaiXin V0.0.1</div>\r\n"},
    {"			\r\n"},
    {"			<!-- 功能选择部分 -->\r\n"},
    {"			<header class=\"title-box\">MODULE styels</header>\r\n"},
    {"			<div class=\"button-box\">\r\n"},
    {"			  <div class=\"button-row\">\r\n"},
    {"				<button class=\"button\" onclick=\"location.href='/button?btn=1'\" aria-label=\"选择功能 1 绿色\">w264</button>\r\n"},
    {"				<button class=\"button\" onclick=\"location.href='/button?btn=2'\" aria-label=\"选择功能 2 红色\">reserve1</button>\r\n"},
    {"				<button class=\"button\" onclick=\"location.href='/button?btn=3'\" aria-label=\"选择功能 3 蓝色\">reserve2</button>\r\n"},
    {"				<button class=\"button\" onclick=\"location.href='/button?btn=4'\" aria-label=\"选择功能 4 白色\">reserve3</button>\r\n"},
    {"			  </div>\r\n"},
    {"			  <div class=\"button-row\">\r\n"},
    {"				<button class=\"button\" onclick=\"location.href='/button?btn=5'\" aria-label=\"选择功能 5\">reserve4</button>\r\n"},
    {"				<button class=\"button\" onclick=\"location.href='/button?btn=6'\" aria-label=\"选择功能 6\">reserve5</button>\r\n"},
    {"				<button class=\"button\" onclick=\"location.href='/button?btn=7'\" aria-label=\"选择功能 7\">reserve6</button>\r\n"},
    {"				<button class=\"button\" onclick=\"location.href='/button?btn=8'\" aria-label=\"选择功能 8\">reserve7</button>\r\n"},
    {"			  </div>\r\n"},
    {"			</div>\r\n"},
    {"\r\n"},
    //	{"			  <!-- 车标部分 -->\r\n"},
    //	{"			  <header class=\"title-box\">Logo</header>\r\n"},
    //	{"			  <div class=\"button-box\">\r\n"},
    //	{"				  <div class=\"button-row\">\r\n"},
    //	{"					  <button class=\"button\" onclick=\"location.href='/button_logo?btn=9'\" aria-label=\"选择车标 1\">BMW</button>\r\n"},
    //	{"					  <button class=\"button\" onclick=\"location.href='/button_logo?btn=10'\" aria-label=\"选择车标 2\">Benz</button>\r\n"},
    //	{"				  </div>\r\n"},
    //	{"			  </div>\r\n"},
    //	{"\r\n"},
    //	{"			  <!-- 自定义部分 -->\r\n"},
    //	{"			  <header class=\"title-box\">Customize</header>\r\n"},
    //	{"			  <div class=\"button-box\">\r\n"},
    //	{"				  <div class=\"button-row\">\r\n"},
    //	{"					  <button class=\"button\" onclick=\"location.href='/button?btn=11'\" aria-label=\"选择自定义 1\">GIF</button>\r\n"},
    //	{"					  <button class=\"button\" onclick=\"location.href='/button?btn=12'\" aria-label=\"选择自定义 2\">IMAGE</button>\r\n"},
    //	{"				  </div>\r\n"},
    //	{"			  </div>\r\n"},
    //	{"			  \r\n"},
    {"			  <!-- 底部小号字体标题 -->\r\n"},
    {"			  <div class=\"footer-title\">If it does not display, please refresh or reset</div>\r\n"},
    {"			  <div class=\"footer-title\">Just for test demo</div>\r\n"},
    {"		  </div>\r\n"},
    {"	  </div>\r\n"},
    {"</body>\r\n"},
    {"</html>\r\n"},
    {"\r\n"},

};

//static struct url getweb;
//static struct url postweb;
static struct settingJson postSetting;

#ifndef INVALID_SOCKET
#define INVALID_SOCKET ~0
#endif

// http回应的一些字段
struct httpIndex const httpIndex = {.Type = "Content-Type", .Length = "Content-Length", .Disposition = "Content-Disposition", .Connection = "Connection",.Accept_Range = "Accept-Ranges"};
#define ISSOCKHANDLE(x) (x != INVALID_SOCKET)

int osal_set_tcp_nodelay(SOCK_HDL fd, int o)
{
    uint8_t opt = o;
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&opt, sizeof opt);
}

int osal_set_reuseaddr(SOCK_HDL fd, int o)
{
    uint8_t opt = o;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof opt);
}
void osal_socket_close(SOCK_HDL fd)
{
    if (ISSOCKHANDLE(fd))
    {
        closesocket(fd);
    }
    else
        _os_printf("socket close fail\n");
}

void http_reponse_web(struct httpClient *httpClient);
void http_reponse_ret(struct httpClient *httpClient);


static struct url getweb[] = 
{
    addWeb(web, "", 1, http_send_error, NULL),
    addWeb(web, "web", 1, http_reponse_web, NULL),
};

// 初始化
void initJson(struct settingJson *json)
{
    json->key = NULL;
    json->type = 0;
    json->reponse = NULL;
    json->next = NULL;
}

// 添加json方法
void addJson(struct settingJson *json, char *key, char type, jsonReponse reponse)
{
    struct settingJson *mjson = json;
    struct settingJson *add = STREAM_LIBC_MALLOC(sizeof(struct settingJson));
    while (mjson->next)
    {
        mjson = mjson->next;
    }

    add->next = NULL;
    add->type = type;

    add->key = STREAM_LIBC_MALLOC(strlen(key) + 1);
    memcpy(add->key, key, strlen(key) + 1);

    add->reponse = reponse;

    mjson->next = add;
}

// 配置json拥有的方法
void default_json(struct settingJson *json)
{
    addJson(json, "timer", cJSON_Number, NULL);
}

// 执行json中的函数
void executeJson(struct settingJson *json, cJSON *cJSON)
{
    struct settingJson *Njson = json;
    while (1)
    {
        if (!Njson)
        {
            break;
        }
        if (Njson->type == cJSON->type && !strcmp(Njson->key, cJSON->string))
        {
            _os_printf("found json:%s->type:%d\r\n", Njson->key, Njson->type);
            if (Njson->reponse)
            {
                Njson->reponse(Njson, cJSON);
            }
            else
            {
                _os_printf("json->reponse is NULL!\r\n");
            }
            break;
        }

        Njson = Njson->next;
    }
}
#if 0
static void initWeb(struct url *web)
{
    web->url = NULL;
    web->type = 1;
    web->reponse = http_reponse_web;//http_send_error; // http_reponse_web;//http_send_error;
    web->queue = NULL;
    web->next = NULL;
}

// web方法添加
void addWeb(struct url *web, const char *url, char type, http_reponse reponse, http_queue queue)
{
    _os_printf("url:%s\r\n", url);
    struct url *addweb = web;
    struct url *add = STREAM_LIBC_MALLOC(sizeof(struct url));
    while (addweb->next)
    {
        addweb = addweb->next;
        //_os_printf("test:%d\turl:%s\r\n",addweb->type,addweb->url);
    }

    add->next = NULL;
    add->type = type;
    add->queue = queue;
    add->reponse = reponse;
    add->url = url;
    addweb->next = add;
}
#endif

void http_reponse_button1(struct httpClient *httpClient)
{
    http_reponse_ret(httpClient);
}

// get方法的一些配置
void default_getweb(struct url *web)
{
    //addWeb(web, "button?btn=1", 1, http_reponse_button1, NULL);
}

// post方法的一些配置
void default_postweb(struct url *web)
{
    //addWeb(web, "test", 1, http_post_Test, NULL);
}

// 初始化并申请空间给httpClient
struct httpClient *initHttpClient()
{
    struct httpClient *httpClient = STREAM_LIBC_MALLOC(sizeof(struct httpClient));
    if (!httpClient)
    {
        return NULL;
    }
    httpClient->fdClient = 0;
    httpClient->httpserver = NULL;
    // httpClient->http_request = NULL;
    httpClient->web = NULL;
    httpClient->pattern = NULL;
    return httpClient;
}

// http缩略图，需要的的缩略图有特定格式，因此需要自己拍的格式才会读到正确
int http_thumb(void *fp)
{
    uint32_t thumbLocate;
    int thumbsize;
    uint8_t buf[8];
    int start = osal_ftell(fp);
    osal_fread(buf, 1, 2, fp);
    if (buf[0] != 0xff || buf[1] != 0xd8)
    {
        _os_printf("not photo!!!\r\n");
        osal_fseek(fp, start);
        return -1;
    }
    osal_fseek(fp, start + 20);
    osal_fread(buf, 1, 8, fp);
    //_os_printf("%x\t%x\t%x\t%x\r\n",buf[0],buf[1],buf[2],buf[3]);
    if (buf[0] == 0xff && buf[1] == 0xe1 && buf[2] == 0x00 && buf[3] == 0x06)
    {
        memcpy(&thumbLocate, &buf[4], 4);
        osal_fseek(fp, start + thumbLocate);
        thumbsize = osal_fsize(fp) - thumbLocate;
        _os_printf("thumbLocate:%d\r\n", thumbLocate);
        return thumbsize;
    }
    else
    {
        _os_printf("%x\t%x\t%x\t%x\r\n", buf[0], buf[1], buf[2], buf[3]);
        _os_printf("not our photo!!!\r\n");
        osal_fseek(fp, start);
        return -2;
    }
}

// 寻找AVI缩略图的位置,返回相对本帧图片的缩略图位置,并且偏移到对应位置,最后直接读取缩略图大小即可
int http_avi_thumb_Locate(void *fp)
{
    int thumbLocate;
    uint8_t buf[8];
    // 该帧的起始位置
    int start = osal_ftell(fp);
    osal_fread(buf, 1, 2, fp);
    if (buf[0] != 0xff || buf[1] != 0xd8)
    {
        _os_printf("not photo!!!\r\n");
        // uartPrintfBuf(buf, 2);
        // osal_fseek(fp,start);
        osal_fclose(fp);
        return -1;
    }
    osal_fseek(fp, start + 20);
    osal_fread(buf, 1, 8, fp);
    //_os_printf("%x\t%x\t%x\t%x\r\n",buf[0],buf[1],buf[2],buf[3]);
    if (buf[0] == 0xff && buf[1] == 0xe1 && buf[2] == 0x00 && buf[3] == 0x06)
    {
        memcpy(&thumbLocate, &buf[4], 4);
        osal_fseek(fp, start + thumbLocate);
        _os_printf("thumbLocate:%d\r\n", thumbLocate);
        return thumbLocate;
    }
    else
    {
        _os_printf("%x\t%x\t%x\t%x\r\n", buf[0], buf[1], buf[2], buf[3]);
        _os_printf("not our photo!!!\r\n");
        osal_fseek(fp, start);
        return -2;
    }
}

// 读取一行，以\r\n代表回车，主要用于http请求字段
static uint8_t readline(char *buf, uint32_t *len)
{
    uint32_t lineLen = 0;
    // uartPrintfBuf(buf,4);
    if (*buf == 0 || *buf == '\0')
    {
        return 2; // 结束,代表该字符串没有任何数据
    }

    if (*buf == '\r' && *(buf + 1) == '\n')
    {
        return 3; // http头部分结束,空一行
    }
    while (1)
    {
        if (*buf == 0)
        {
            return 1; // 无检测到换行符
        }
        else
        {
            if (*buf == '\r' && *(buf + 1) == '\n')
            {
                *len = lineLen + 2;
                return 0; // 检测到换行符号
            }
            buf++;
            lineLen++;
        }
    }
}

// 这是对GET请求字段的判断，因为与其他字段不同，所以单独判断
void parseGet(char *recBuf, struct httpEntry *http)
{

    uint8_t len = 0;
    char *src = recBuf;
    while (*recBuf != ' ' && *recBuf)
    {
        len++;
        recBuf++;
    }

    char *method = STREAM_LIBC_MALLOC(len + 1);
    memcpy(method, src, len);
    *(method + len) = '\0';
    recBuf++;
    src = recBuf;
    len = 0;

    while (*recBuf != ' ' && *recBuf)
    {
        len++;
        recBuf++;
    }
    char *url = STREAM_LIBC_MALLOC(len + 1);
    memcpy(url, src, len);
    *(url + len) = '\0';

    http->key = method;
    http->value = url;
    http->next = NULL;
    _os_printf("url:%s\r\n", url);
}

// 解析某一段字符串是否为http请求的字段
// 格式:		key: value
// 这段解析根据chrome浏览器的格式，所以是针对性的，可能不适合其他地方，例如冒号后面有一个空格，程序中直接跳过而不是对空格判断
void parseRequest(char *recBuf, uint8_t buflen, struct httpEntry *http)
{

    uint8_t len = 0;
    char *src = recBuf;
    struct httpEntry *httpNext = STREAM_LIBC_MALLOC(sizeof(struct httpEntry));
    while (http->next)
    {
        http = http->next;
    }
    http->next = httpNext;
    while (*recBuf != ':' && *recBuf)
    {
        if (len >= buflen)
        {
            break;
        }
        len++;
        recBuf++;
    }
    char *index = STREAM_LIBC_MALLOC(len + 1);
    memcpy(index, src, len);
    *(index + len) = '\0';
    recBuf += 2;
    len += 3;
    src = recBuf;

    char *value;

    // 如果非正常情况下，会导致buflen(一行数据的长度)也没有找到":",应该是错误的
    // 未经过验证
    if (buflen >= len)
    {
        value = STREAM_LIBC_MALLOC(buflen - len + 1);
        memcpy(value, src, buflen - len);
        *(value + buflen - len) = '\0';
    }
    else
    {
        value = STREAM_LIBC_MALLOC(1);
        *value = '\0';
    }

    httpNext->key = index;
    httpNext->value = value;
    httpNext->next = NULL;
}

// post内容
void postContent(char *recBuf, uint8_t buflen, struct httpEntry *http)
{
    struct httpEntry *httpNext = STREAM_LIBC_MALLOC(sizeof(struct httpEntry));
    while (http->next)
    {
        http = http->next;
    }
    http->next = httpNext;

    char *index = STREAM_LIBC_MALLOC(8);
    char *value = STREAM_LIBC_MALLOC(buflen + 1);
    memcpy(index, "Content", 7);
    *(index + 7) = '\0';
    memcpy(value, recBuf, buflen);
    *(value + buflen) = '\0';
    httpNext->key = index;
    httpNext->value = value;
    httpNext->next = NULL;
}

// 保存http请求头的信息
void http_request_msg(char *recBuf, struct httpEntry *http_request)
{
    uint8_t times = 0;
    uint8_t result = 0;
    uint32_t len;
    char *startBuf = recBuf;
    while (1)
    {
        result = readline(recBuf, &len);
        if (result != 0)
        {
            //_os_printf("result:%d\r\n",result);
            if (result == 3) // 检测到有空的一行,\r\n	2字节
            {
                recBuf += 2; // 偏移2字节，去掉空行
                uint16_t contentLen = strlen(startBuf) - (recBuf - startBuf);
                _os_printf("contentLen:%d\r\n", contentLen);
                if (contentLen) // 代表空行后面还有内容,很有可能是post过来的内容
                {
                    postContent(recBuf, contentLen, http_request);
                }
            }
            break;
        }
        // 添加一个保存key与value的链表
        times++;
        if (times == 1)
        {
            // 第一次时解析http的get的头部,格式与后面数据不一样
            parseGet(recBuf, http_request);
        }
        else
        {
            parseRequest(recBuf, len, http_request);
        }
        recBuf += len;
    }
}

// 释放http请求头的空间
void free_request(struct httpEntry *http)
{
    struct httpEntry *next = http;
    struct httpEntry *current;
    while (1)
    {
        if (next->next)
        {
            current = next;
            next = next->next;
            STREAM_LIBC_FREE(current->key);
            STREAM_LIBC_FREE(current->value);
            STREAM_LIBC_FREE(current);
        }
        else
        {
            if (next->value)
            {
                STREAM_LIBC_FREE(next->value);
            }

            if (next->key)
            {
                STREAM_LIBC_FREE(next->key);
            }
            STREAM_LIBC_FREE(next);

            break;
        }
    }
}

// 打印接收到的请求字段，现在按使用的是GET字段，其他都没有进行判断
void print_request(struct httpEntry *http)
{
    struct httpEntry *next = http;
    while (1)
    {
        _os_printf("%s:%s\r\n", next->key, next->value);
        if (next->next)
        {
            next = next->next;
        }
        else
        {
            break;
        }
    }
}

// 返回  NULL代表没有匹配成功			      url:返回匹配成功的地址
// 地址的匹配
// 可能要修改web的遍历方式，这样增加的时候可以改动小一点
// 暂时主要识别GET /test HTTP/1.1这部分字段
struct url *parsePath(struct httpEntry *http, struct url *gweb, struct url *pweb)
{
    struct url *parseWeb;
    if (!strcmp(http->key, "GET"))
    {
        _os_printf("\r\nenter GET!\r\n\r\n key:%s  value:%s len:%d  pattern:%s\r\n", http->key, http->value, sizeof(http->value), http->pattern);
        if (http->value[0] != '/')
        {
            _os_printf("\r\nnot GET request!!!\r\n\r\n");
            return NULL;
        }
        // 第一个保留为404页面
        // 循环寻找web的get方法

        parseWeb = gweb;
        while (parseWeb && parseWeb->url)
        {
            // 完全匹配
            if (parseWeb->type == 1)
            {
                if (!strcmp(&http->value[1], parseWeb->url))
                {
                    _os_printf("parseWeb return:%x\r\n", parseWeb);
                    return parseWeb;
                }
            }
            // 前部分匹配
            else if (parseWeb->type == 2)
            {
                uint8_t len;
                len = strlen(parseWeb->url);
                char *pattern = STREAM_LIBC_MALLOC(len + 1);
                memcpy(pattern, &http->value[1], len);
                *(pattern + len) = '\0';
                if (!strcmp(pattern, parseWeb->url))
                {
                    http->pattern = &http->value[1 + len];
                    STREAM_LIBC_FREE(pattern);
                    return parseWeb;
                }
                STREAM_LIBC_FREE(pattern);
            }
            else
            {
                _os_printf("no parsePath!\r\n");
                return NULL;
            }
            parseWeb++;
        }
    }

    else if (!strcmp(http->key, "POST"))
    {
        _os_printf("enter POST!\r\n");
        if (http->value[0] != '/')
        {
            _os_printf("not POST request!!!\r\n");
            return NULL;
        }
        // 第一个保留为404页面
        // 循环寻找web的get方法

        parseWeb = pweb;
        while (parseWeb && parseWeb->url)
        {

            //_os_printf("web%d url:%s\tvalue:%s\r\n",i,web[i].url,&http->value[1]);
            // 完全匹配
            if (parseWeb->type == 1)
            {
                if (!strcmp(&http->value[1], parseWeb->url))
                {
                    _os_printf("parsePath1 correct:%s!!!\r\n", &http->value[1]);
                    return parseWeb;
                }
            }
            // 前部分匹配
            else if (parseWeb->type == 2)
            {
                uint8_t len;
                len = strlen(parseWeb->url);
                char *pattern = STREAM_LIBC_MALLOC(len + 1);
                memcpy(pattern, &http->value[1], len);
                *(pattern + len) = '\0';
                //_os_printf("pattern:%s\r\n",pattern);
                if (!strcmp(pattern, parseWeb->url))
                {
                    http->pattern = &http->value[1 + len];
                    _os_printf("parsePath2 correct:%s!!!\r\n", http->pattern);
                    STREAM_LIBC_FREE(pattern);
                    return parseWeb;
                }
                STREAM_LIBC_FREE(pattern);
            }
            else
            {
                _os_printf("no this parseWeb type:%d!\r\n", parseWeb->type);
                _os_printf("parseWeb:%X\r\n", parseWeb);
                return NULL;
            }
            parseWeb++;
        }
    }

    //_os_printf("not correct path!!!\r\n");

    // 没有匹配的，返回第一个，都是null,没有太多实际作用
    return NULL;
}

const uint8_t reponse2[] = "Hello!!!";

// 释放httpClient的资源，包括socket和结构体其他需要释放的空间
void closeRes(struct httpClient *httpClient)
{
    if(httpClient == NULL)
        return;

    if (httpClient->pattern)
    {
        _os_printf("pattern:%s\r\n", httpClient->pattern);
        STREAM_LIBC_FREE(httpClient->pattern);
    }
    if (httpClient->http_request)
    {
        free_request(httpClient->http_request);
    }

    //检查是否发送完成
    uint32_t start_time = os_jiffies();
    while(!socket_done(httpClient->fdClient))
    {
        if(os_jiffies() - start_time > 1000)
        {
            break;
        }
        os_sleep_ms(10);
    }
    os_printf("done:%d\n",socket_done(httpClient->fdClient));
    osal_socket_close(httpClient->fdClient);
}

// 释放httpClient的资源，包括socket和结构体其他需要释放的空间
void closeKeepRes(struct httpClient *httpClient)
{
    if (httpClient->pattern)
    {
        STREAM_LIBC_FREE(httpClient->pattern);
    }
    if (httpClient->http_request)
    {
        free_request(httpClient->http_request);
    }
}

// 创建http头部和响应码
struct httpresp *http_create_reply(int code, char *reply)
{

    uint32_t len;
    struct httpresp *resp;
    resp = STREAM_LIBC_MALLOC(sizeof(struct httpresp));
    if (!resp)
    {
        return NULL;
    }
    resp->headbuf = STREAM_LIBC_MALLOC(1024);
    resp->msgMaxLen = 1024;
    resp->msgLen = 0;
    resp->stat = 0;
    len = sprintf(resp->headbuf, "HTTP/1.0 %d %s\r\n", code, reply);
    resp->msgLen += len;
    return resp;
}

// 头部添加
void http_add_header(struct httpresp *resp, const char *name, char *value)
{
    if (resp->stat || !resp || !resp->headbuf)
    {
        _os_printf("http_add_header err!\r\n");
        return;
    }
    //uint32_t len;
    uint32_t headLen;
    headLen = strlen(name) + strlen(value) + 4; //  包括4字节的（: \r\n)
    if (headLen + resp->msgLen > resp->msgMaxLen)
    {
        resp->stat = 1;
        return;
    }

    sprintf(resp->headbuf + resp->msgLen, "%s: %s\r\n", name, value);
    resp->msgLen += headLen;
}

// http头部发送,name应该是contentLen字段
// 头部的最后信息应该是内容的长度
int http_header_send(struct httpresp *resp, const char *name, uint32_t len, int fd)
{
    int res;
    char contentLen[10];
    sprintf(contentLen, "%d\r\n", len);
    http_add_header(resp, name, contentLen);
    if (resp->stat || !resp || !resp->headbuf)
    {
        free_http_head(resp);
        _os_printf("http header err!\r\n");
        return -2;
    }
    res = send(fd, resp->headbuf, strlen(resp->headbuf), 0);
    free_http_head(resp);
    return res;
}

// 404返回
void http_send_error(struct httpClient *httpClient)
{
    struct httpresp *head;
    int fd = httpClient->fdClient;
    head = http_create_reply(404, "Not Found");
    http_add_header(head, httpIndex.Connection, "close");
    http_add_header(head, httpIndex.Type, "text/html");
    http_header_send(head, httpIndex.Length, strlen(http404), fd);
    send(fd, http404, strlen(http404), 0);
    closeRes(httpClient);
}

#define HTTPREADLEN 1460 + 0x10

// 文件内容发送，传递的fp已经偏移好位置
int http_file_send(void *fp, int fd)
{
    uint32_t len;
    uint32_t total_len = 0;
    int res = 0;
    char *fileContent = STREAM_LIBC_MALLOC(HTTPREADLEN);
    char *buf = (char*)(((uint32_t)fileContent + 0x10) & (~0x0f));
    os_printf("content:%X\r\n", fileContent);
    _os_printf("buf:%X\r\n", buf);
    while (1)
    {
        len = osal_fread(buf, 1, HTTPREADLEN - 0x10, fp);
        total_len += len;
        //_os_printf("http file send:%d\r\n",len);
        if (len == 0)
        {

            osal_fclose(fp);
            // int iRecvLen = recv( fd, fileContent, 1023, 0);
            STREAM_LIBC_FREE(fileContent);
            os_printf("total_len:%d\r\n", total_len);
            //_os_printf("http_file_send iRecvLen:%d\r\n",iRecvLen);
            return res;
        }
        res = send(fd, buf, len, 0);
        if (res == -1)
        {
            _os_printf("socket err!\r\n");
            osal_fclose(fp);
            STREAM_LIBC_FREE(fileContent);
            return res;
        }
    }
}

int http_send(void *fp, int fd, uint32_t size)
{
    uint32_t readlen = HTTPREADLEN - 0x10;
    int len;
    int res;
    // 为了16byte对齐,所以要增加0x10
    char *fileContent = STREAM_LIBC_MALLOC(HTTPREADLEN);

    if (!fileContent)
    {
        // Todo:申请空间不足
        return 1;
    }
    // 进行16byte对齐
    char *buf =(char*)( ((uint32_t)fileContent + 0x10) & (~0x0f));
    while (1)
    {
        if (size < readlen)
        {
            readlen = size;
        }

        len = osal_fread(buf, 1, readlen, fp);
        if (len == 0)
        {
            res = 3;
            break;
        }
        res = send(fd, buf, len, 0);
        if (res < 0)
        {
            _os_printf("http_send err:%d!\r\n", res);
            res = 2;
            break;
        }
        size -= len;
        if (size == 0)
        {
            res = 0;
            break;
        }
    }
    osal_fclose(fp);
    STREAM_LIBC_FREE(fileContent);
    return res;
}

// 释放http头部回应的空间
void free_http_head(struct httpresp *resp)
{
    if (resp)
    {
        if (resp->headbuf)
        {
            STREAM_LIBC_FREE(resp->headbuf);
        }
        STREAM_LIBC_FREE(resp);
    }
}

// static void http_reponse()

// 测试下载文件
void http_reponse_File(struct httpClient *httpClient)
{
    int fd = httpClient->fdClient;
    void *fp;
    int res;
    _os_printf("Line:%d\tfunc:%s\n", __LINE__, __FUNCTION__); // portMAX_DELAY
    fp = osal_fopen("1:/AVI/AVI1.AVI", "rb");
    if (fp == NULL)
    {
        goto FileFail;
    }

    struct httpresp *head;
    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Type, "application/force-download");
    http_add_header(head, httpIndex.Connection, "close");
    http_add_header(head, httpIndex.Disposition, "attachment; filename=AVI1.AVI");
    res = http_header_send(head, httpIndex.Length, osal_fsize(fp), fd);
    if (res < 0)
    {
        goto HeadSendErr;
    }

    // 文件发送
    res = http_file_send(fp, fd);
    if (res == -1)
    {
        goto SocketErr;
    }

    // 正常结束
    goto ReleaseRes;

HeadSendErr:
    _os_printf("HeadSenderr:%d!\r\n", res);
    osal_fclose(fp);
    goto ReleaseRes;
FileFail:
    _os_printf("open file fail!\r\n");
    goto ReleaseRes;
#if 0
FileNameErr:
    _os_printf("file path err!!!\r\n");
    goto ReleaseRes;
#endif
SocketErr:
    _os_printf("socket err!!!\r\n");
    goto ReleaseRes;

ReleaseRes:
    closeRes(httpClient);
    return;
}

// photo文件夹的原图查看
void new_http_reponse_OJpg(struct httpClient *httpClient)
{
    char path[30];
    void *fp;
    int fd = httpClient->fdClient;
    int res;
    _os_printf("%s\r\n", __FUNCTION__);
    _os_printf("fileName:%s\r\n", httpClient->pattern);
#if 1
    sprintf(path, "%s%s", "1:/", httpClient->pattern);
    _os_printf("path:%s\r\n", path);
    fp = osal_fopen(path, "rb");

    if (fp == NULL)
    {
        goto FileFail;
    }

    _os_printf("Line:%d\tfunc:%s\n", __LINE__, __FUNCTION__); // portMAX_DELAY

    // 头发送
    struct httpresp *head;
    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Type, "image/jpeg");
    http_add_header(head, httpIndex.Connection, "close");
    res = http_header_send(head, httpIndex.Length, osal_fsize(fp), fd);
    if (res < 0)
    {
        goto HeadSendErr;
    }

    // 文件发送
    res = http_file_send(fp, fd);
    if (res == -1)
    {
        goto SocketErr;
    }
#endif
    // 正常结束
    goto ReleaseRes;

HeadSendErr:
    _os_printf("HeadSenderr:%d!\r\n", res);
    osal_fclose(fp);
    goto ReleaseRes;
FileFail:
    _os_printf("open file fail!\r\n");
    goto ReleaseRes;
#if 0
FileNameErr:
    _os_printf("file path err!!!\r\n");
    goto ReleaseRes;
#endif
SocketErr:
    _os_printf("socket err!!!\r\n");
    goto ReleaseRes;

ReleaseRes:
    closeRes(httpClient);
    return;
}

// photo中的缩略图查看
void new_http_reponse_TJpg(struct httpClient *httpClient)
{
    char path[30];
    void *fp;
    int thumbsize;
    int fd = httpClient->fdClient;
    sprintf(path, "%s%s", "1:/", httpClient->pattern);
    _os_printf("path:%s\r\n", path);
    fp = osal_fopen(path, "rb");
    if (fp == NULL)
    {
        goto FileFail;
    }

    thumbsize = http_thumb(fp);
    if (thumbsize < 0)
    {
        goto ThumbFail;
    }

    // http头配置与发送
    struct httpresp *head;
    int res;
    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Type, "image/jpeg");
    http_add_header(head, httpIndex.Connection, "close");
    res = http_header_send(head, httpIndex.Length, thumbsize, fd);
    if (res < 0)
    {
        _os_printf("socket err!\r\n");
        goto HeadSendErr;
    }

    // 文件发送
    res = http_file_send(fp, fd);
    if (res == -1)
    {
        goto SocketErr;
    }

    // 正常结束
    goto ReleaseRes;

HeadSendErr:
    osal_fclose(fp);
    _os_printf("HeadSenderr:%d!\r\n", res);
    goto ReleaseRes;
FileFail:
    _os_printf("open file fail!\r\n");
    goto ReleaseRes;
#if 0
FileNameErr:
    _os_printf("file path err!!!\r\n");
    goto ReleaseRes;
#endif
SocketErr:
    _os_printf("socket err!!!\r\n");
    goto ReleaseRes;

ThumbFail:
    _os_printf("http_thumb err!!!\r\n");
    goto ReleaseRes;

ReleaseRes:
    closeRes(httpClient);
    return;
}

// photo中的缩略图查看
void http_reponse_TJpg(struct httpClient *httpClient)
{
    char path[30];
    void *fp;
    int thumbsize;
    int fd = httpClient->fdClient;
    // 图片文件缩略图的一些判断
    if (strlen(httpClient->pattern) > 12)
    {
        goto FileNameErr;
    }
    _os_printf("fileName:%s\r\n", httpClient->pattern);
    sprintf(path, "%s%s", "1:/DCIM/", httpClient->pattern);
    fp = osal_fopen(path, "rb");
    if (fp == NULL)
    {
        goto FileFail;
    }

    thumbsize = http_thumb(fp);
    if (thumbsize < 0)
    {
        goto ThumbFail;
    }

    // http头配置与发送
    struct httpresp *head;
    int res;
    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Type, "image/jpeg");
    http_add_header(head, httpIndex.Connection, "close");
    res = http_header_send(head, httpIndex.Length, thumbsize, fd);
    if (res < 0)
    {
        _os_printf("socket err!\r\n");
        goto HeadSendErr;
    }

    // 文件发送
    res = http_file_send(fp, fd);
    if (res == -1)
    {
        goto SocketErr;
    }

    // 正常结束
    goto ReleaseRes;

HeadSendErr:
    osal_fclose(fp);
    _os_printf("HeadSenderr:%d!\r\n", res);
    goto ReleaseRes;
FileFail:
    _os_printf("open file fail!\r\n");
    goto ReleaseRes;

FileNameErr:
    _os_printf("file path err!!!\r\n");
    goto ReleaseRes;
SocketErr:
    _os_printf("socket err!!!\r\n");
    goto ReleaseRes;

ThumbFail:
    _os_printf("http_thumb err!!!\r\n");
    goto ReleaseRes;

ReleaseRes:
    closeRes(httpClient);
    return;
}

// 测试http的post方法
void http_post_Test(struct httpClient *httpClient)
{

    struct httpresp *head;
    // char *http_reponse = STREAM_LIBC_MALLOC(1024);
    int fd = httpClient->fdClient;

    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Type, "application/json");
    http_add_header(head, httpIndex.Connection, "close");
    cJSON *json;
    char *out = NULL;
    struct httpEntry *postContent = httpClient->http_request;
    struct httpEntry *postContentLen = NULL;

    // 读取post的内容和长度
    while (1)
    {
        if (!postContent)
        {
            break;
        }
        if (!strcmp(postContent->key, "Content"))
        {
            break;
        }
        if (!strcmp(postContent->key, "Content-Length"))
        {

            postContentLen = postContent;
        }

        postContent = postContent->next;
    }

    if (postContent)
    {
        json = cJSON_Parse(postContent->value); // 获取整个大的句柄
        int i = 0;

        for (i = 0; i < cJSON_GetArraySize(json); i++)
        {
            cJSON *item = cJSON_GetArrayItem(json, i);

            executeJson(&postSetting, item);
            /*
            if(cJSON_String == item->type)
            {
                    _os_printf("%s->%s\r\n",item->string,item->valuestring);
            }
            */
        }

        // out=cJSON_Print(json);  //这个是可以输出的。为获取的整个json的值
        out = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);

        if (postContentLen)
        {
            _os_printf("%s->%s\r\n", postContentLen->key, postContentLen->value);
        }
    }

    // 如果接收不完整，则会在这里重新去尝试接收,前提已经接收到所有的头部,否则可能还是会出错,尚未做处理
    else
    {
        _os_printf("rec continue\r\n");
        if (postContentLen)
        {
            _os_printf("%s->%s\r\n", postContentLen->key, postContentLen->value);
            char *recBuf = STREAM_LIBC_MALLOC(1024);
            int iRecvLen = recv(fd, recBuf, 1024, 0);
            recBuf[iRecvLen] = 0;
            _os_printf("rec again len:%d\r\n", strlen(recBuf));
            json = cJSON_Parse(recBuf); // 获取整个大的句柄
            int i = 0;

            for (i = 0; i < cJSON_GetArraySize(json); i++)
            {
                cJSON *item = cJSON_GetArrayItem(json, i);

                executeJson(&postSetting, item);
            }

            out = cJSON_PrintUnformatted(json);
            cJSON_Delete(json);
            STREAM_LIBC_FREE(recBuf);
            _os_printf("rec again finish!\r\n");
        }
    }

    if (out)
    {
        http_header_send(head, httpIndex.Length, strlen(out), fd);
        send(fd, out, strlen(out), 0);
    }

    STREAM_LIBC_FREE(out);
    // STREAM_LIBC_FREE(http_reponse);
    closeRes(httpClient);
}

// 测试http，显示hello

extern unsigned char webdata[6788];
uint8 read_flash_file_list(char *list);


void http_reponse_ret(struct httpClient *httpClient)
{
    uint32_t sendlen = 0;
    uint32_t itk = 0;
    uint32_t len_str = 0;
    uint32_t len;
    uint32_t str_offset;
    struct httpresp *head;
    uint8_t *list_table = (uint8_t*)os_malloc(3*1024);
    if(!list_table)
    {
        return;
    }
    // char *http_reponse = STREAM_LIBC_MALLOC(1024);
    int fd = httpClient->fdClient;
    _os_printf("%s %d\r\n", __func__, __LINE__);
    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Type, "text/html");
    http_add_header(head, httpIndex.Connection, "close");

    // sprintf(http_reponse, "%s", "Hello!!!");
    memset(list_table, 0, 3 * 1024);
    // read_flash_file_list(list_table);
    //_os_printf("list_table:%s\r\n",list_table);
    // http_header_send(head, httpIndex.Length, strlen(list_table),fd);

    len_str = sizeof(webstr_ret);
    _os_printf("len_str:%d\r\n", len_str);
    len_str = len_str / 100;
    for (itk = 0; itk < len_str; itk++)
    {
        sendlen += strlen(webstr_ret[itk]); // strlen(webstr);
    }

    http_header_send(head, httpIndex.Length, sendlen, fd);

    str_offset = 0;
    while (sendlen)
    {
        if (sendlen > 1024)
        {
            len = 0;
            while (len < 1024)
            {
                memcpy(list_table + len, webstr_ret[str_offset], strlen(webstr_ret[str_offset]));
                len += strlen(webstr_ret[str_offset]);
                str_offset++;
            }

            send(fd, list_table, len, 0);
            sendlen = sendlen - len;
        }
        else
        {
            len = 0;
            for (itk = str_offset; itk < len_str; itk++)
            {
                memcpy(list_table + len, webstr_ret[itk], strlen(webstr_ret[itk]));
                len += strlen(webstr_ret[itk]);
            }

            send(fd, list_table, len, 0);
            sendlen = 0;
        }
    }

    // STREAM_LIBC_FREE(http_reponse);
    closeRes(httpClient);
    // user_action(MSG_ID_TAKE_PHOTO);	//模拟测试按键拍照的效果
    if(list_table)
    {
        os_free(list_table);
    }
}

void http_reponse_web(struct httpClient *httpClient)
{
    uint32_t sendlen = 0;
    //uint32_t offset = 0;
    uint32_t itk = 0;
    uint32_t len_str = 0;
    uint32_t len;
    uint32_t str_offset;
    struct httpresp *head;
    uint8_t *list_table = (uint8_t*)os_malloc(3*1024);
    if(!list_table)
    {
        return;
    }
    // char *http_reponse = STREAM_LIBC_MALLOC(1024);
    int fd = httpClient->fdClient;
    _os_printf("%s %d\r\n", __func__, __LINE__);
    head = http_create_reply(200, "OK");
    http_add_header(head, httpIndex.Type, "text/html");
    http_add_header(head, httpIndex.Connection, "close");

    // sprintf(http_reponse, "%s", "Hello!!!");
    memset(list_table, 0, 3 * 1024);
    // read_flash_file_list(list_table);
    //_os_printf("list_table:%s\r\n",list_table);
    // http_header_send(head, httpIndex.Length, strlen(list_table),fd);

    len_str = sizeof(webstr);
    _os_printf("len_str   :%d\r\n", len_str);
    len_str = len_str / 200;
    for (itk = 0; itk < len_str; itk++)
    {
        sendlen += strlen(webstr[itk]); // strlen(webstr);
    }
    http_header_send(head, httpIndex.Length, sendlen, fd);

    str_offset = 0;
    while (sendlen)
    {
        if (sendlen > 1024)
        {
            len = 0;
            while (len < 1024)
            {
                memcpy(list_table + len, webstr[str_offset], strlen(webstr[str_offset]));
                len += strlen(webstr[str_offset]);
                str_offset++;
            }

            send(fd, list_table, len, 0);
            sendlen = sendlen - len;
        }
        else
        {
            len = 0;
            for (itk = str_offset; itk < len_str; itk++)
            {
                //				_os_printf("web :%d  ",itk);
                memcpy(list_table + len, webstr[itk], strlen(webstr[itk]));
                len += strlen(webstr[itk]);
            }
            //			_os_printf("len_tab:%d\r\n",len);
            send(fd, list_table, len, 0);
            sendlen = 0;
        }
    }

    // STREAM_LIBC_FREE(http_reponse);
    closeRes(httpClient);
    // user_action(MSG_ID_TAKE_PHOTO);	//模拟测试按键拍照的效果
    if(list_table)
    {
        os_free(list_table);
    }
}

static void thread_rec_pool_queue(struct httpserver *httpserver)
{
    int32 ret;
    struct httpClient *httpClient_pool;
    while (1)
    {
        httpClient_pool = (struct httpClient *)os_msgq_get2(&httpserver->thread_pool_queue,-1,&ret);
        if(httpClient_pool && ret == 0)
        {
            _os_printf("%s %d   httpserver->thread_pool_queue:%x\r\n", __FUNCTION__, ret, httpserver->thread_pool_queue);
            http_thread_pool(httpClient_pool, getweb, NULL);
            STREAM_LIBC_FREE(httpClient_pool);
            httpClient_pool = NULL;
        }
    }
}

// 发送有个队列消息
static uint8_t httpqueue_thread_send_pool(struct httpClient *httpClient)
{
    long res;
    res = os_msgq_put(&httpClient->httpserver->thread_pool_queue, (uint32_t)httpClient,-1);
    _os_printf("QueueSend finish:%d!!!\r\n", res);
    if (res != 0)
    {
        STREAM_LIBC_FREE(httpClient);
        _os_printf("QueueSend fail!\r\n");
    }
    return 0;
}

// 监听到socket就进入http_thread进行处理
void http_thread_pool(struct httpClient *httpClient, struct url *gweb, struct url *pweb)
{
    // static uint32_t requestTimes = 0;
    char *recBuf;
    struct url *web;
    int fd = httpClient->fdClient;
    struct httpEntry *http_request = STREAM_LIBC_MALLOC(sizeof(struct httpEntry));
    http_request->next = NULL;
    http_request->key = NULL;
    http_request->value = NULL;
    http_request->pattern = NULL;

    recBuf = STREAM_LIBC_MALLOC(1024);
    //_os_printf("fd:%d  http_request:%08x recBuf:%08x\r\n",fd,http_request,recBuf);
    while (1)
    {
        int iRecvLen = recv(fd, recBuf, 1023, 0);
        _os_printf("iRecvLen:%d\r\n", iRecvLen);
        if (iRecvLen > 0)
        {
            recBuf[iRecvLen] = 0;
            http_request_msg(recBuf, http_request);
            // 这里只是判断了GET方法，其他字段都没有去判断
            web = parsePath(http_request, gweb, pweb);
            httpClient->http_request = http_request;
            if (web && (web->reponse || web->queue))
            {
                // if(web->url)
                //{
                //	_os_printf("web->url:%s\r\n",web->url);
                // }
                // else
                //{
                //	_os_printf("no found web\r\n");
                // }
                // 如果部分匹配，则将后面的内容复制出来
                if (http_request->pattern)
                {
                    _os_printf("request->pattern:%s\r\n", http_request->pattern);
                    httpClient->pattern = STREAM_LIBC_MALLOC(strlen(http_request->pattern) + 1);
                    memcpy(httpClient->pattern, http_request->pattern, strlen(http_request->pattern) + 1);
                }

                httpClient->web = web;
                // httpClient->http_request = http_request;
                if (web->queue)
                {
                    web->queue(httpClient);
                }
                else
                {
                    web->reponse(httpClient);
                }
            }

            // 增加404错误，以及一些提示
            // 不应该出现在这里
            else
            {
                http_send_error(httpClient);
                _os_printf("shouldn't appear here!\r\n");
            }
            break;
        }
        else if (iRecvLen == 0)
        {
            osal_socket_close(fd);
            _os_printf("RecErr:iRecvLen:%d\t%d\r\n", iRecvLen, fd);
            STREAM_LIBC_FREE(http_request);
            break;
        }
        else
        {
            _os_printf("RecErr:iRecvLen:%d\t%d\r\n", iRecvLen, fd);
            osal_socket_close(fd);
            STREAM_LIBC_FREE(http_request);
            break;
        }
    }
    STREAM_LIBC_FREE(recBuf);
    return;
}

// 客户端

// 创建http  的GET请求
struct httpresp *http_create_GET(char *url)
{

    uint32_t len;
    struct httpresp *resp;
    resp = STREAM_LIBC_MALLOC(sizeof(struct httpresp));
    if (!resp)
    {
        return NULL;
    }
    resp->headbuf = STREAM_LIBC_MALLOC(1024);
    resp->msgMaxLen = 1024;
    resp->msgLen = 0;
    resp->stat = 0;
    len = sprintf(resp->headbuf, "GET /%s HTTP/1.1\r\n", url);
    resp->msgLen += len;
    return resp;
}

int http_GET_header_send(struct httpresp *resp, int fd)
{
    int res;
    if (resp->stat || !resp || !resp->headbuf)
    {
        free_http_head(resp);
        _os_printf("http header err!\r\n");
        return -2;
    }
    res = send(fd, resp->headbuf, strlen(resp->headbuf), 0);
    free_http_head(resp);
    return res;
}

/********************* Initial ********************/
static void do_accept(void *ei, void *d)
{
    SOCK_HDL fd;
    unsigned int i;
    struct sockaddr_in addr;
    struct httpClient *httpClient = initHttpClient();
    if (!httpClient)
    {
        return;
    }
    struct httpserver *httpserver = (struct httpserver *)d;
    _os_printf("httpserver fd:%d\r\n", httpserver->fdServer);

    i = sizeof(addr);
    if ((fd = accept(httpserver->fdServer, (struct sockaddr *)&addr, &i)) < 0)
    {
        return;
    }
    _os_printf("accepted connection from %s:%d\r\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

    #if 0
    if (osal_set_tcp_nodelay(fd, 1) < 0)
    {
        //    osal_socket_strerr(strerr1, socket_errno);
        //    p_info("error setting TCP_NODELAY on socket: %s", strerr1);
    }
    #endif
    _os_printf("create new thread fd:%d\r\n", fd);
    httpClient->fdClient = fd;
    httpClient->httpserver = httpserver;
    httpClient->time = os_jiffies();
    httpClient->pattern = NULL;
    httpClient->http_request = NULL;
#if 1
    int nNetTimeout = 1000; // 1秒
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&nNetTimeout, sizeof(int)) < 0)
    {
        _os_printf("111111error setsockopt TCP connection\r\n");
    }
#endif
    httpqueue_thread_send_pool(httpClient);
    // osal_thread_create(http_thread, httpClient, 15, 0, 4096, "http_thread");
}

static int http_listen(int port)
{
    struct sockaddr_in addr;
    SOCK_HDL fd;
    //ERR_STR strerr;
    //k_task_handle_t queue1;
    //k_task_handle_t queue2;
    struct httpserver *httpserver = STREAM_LIBC_MALLOC(sizeof(struct httpserver));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = 0;
    addr.sin_port = htons(port);

    if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        return -1;
    }

    if (osal_set_reuseaddr(fd, 1) < 0) {}
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        //    osal_socket_strerr(strerr, socket_errno);
        //    p_err("unable to bind to tcp socket: %s\r\n", strerr);
        close(fd);
        return -1;
    }
    if (listen(fd, 8) < 0)
    {
        //    osal_socket_strerr(strerr, socket_errno);
        //    p_err("error when attempting to listen on tcp socket: %s\r\n", strerr);
        close(fd);
        return -1;
    }
    httpserver->fdServer = fd;
    os_msgq_init(&httpserver->thread_pool_queue,32);

    os_task_create("queue1", (os_task_func_t)thread_rec_pool_queue, httpserver, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2048);

    //initWeb(&getweb);
    //default_getweb(&getweb);

    eloop_add_fd(fd, EVENT_READ, EVENT_F_ENABLED, do_accept, (void *)httpserver);

    _os_printf("listening on http port %s:%d\r\n", inet_ntoa(addr.sin_addr), port);

    return 0;
}

/********************* GLOBAL CONFIGURATION DIRECTIVES ********************/
int config_http(int port)
{
    if (port <= 0 || port > 65535)
    {
        _os_printf("invalid listen port %d", port);
        return -1;
    }
    return http_listen(port);
}
