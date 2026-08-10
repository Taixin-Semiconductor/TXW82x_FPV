#ifndef __TAKEPHOTO_H
#define __TAKEPHOTO_H
#include "basic_include.h"
#include "lib/multimedia/msi.h"
typedef uint8_t (*gen_filename)(char *path, uint32_t path_max_len);
typedef void (*takephoto_done)(void *d);

// 当前版本不支持,先考虑接口如何实现
// free_func是释放函数(如果struct takephoto_s需要释放,则去释放,没有则设置NULL)
struct takephoto_s
{
    uint16_t        thumb_w, thumb_h;
    uint8_t         srcID, fileter;        // 通过srcID和fileter来识别是否为正确的图片来源
    uint8_t         thumb_en : 1, rev : 7; // 是否开启缩略图功能
    void           *priv;                  // 私有结构体
    const char     *prefix;                // 文件名前缀,未实现
    const char     *suffix;                // 文件名后缀(非后缀名,只是在文件名后面增加特定字符串),未实现
    const char     *dir;                   // 目录位置
    const char     *filename;              // 文件名(如果NULL,则内部规则生成)
    gen_filename    gen_filename;          // 文件名字生成函数
    takephoto_done *done;                  // 释放函数,用于释放本结构体,如果是固定,则不需要释放,注意,局部变量一定要等到free才能释放
};
struct msi *new_takephoto_msi(uint8_t srcID, uint8_t filter);
#endif