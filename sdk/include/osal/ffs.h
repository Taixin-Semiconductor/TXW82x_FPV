#ifndef __OS_FFS_H_
#define __OS_FFS_H_

#if 0
#ifdef __cplusplus
extern "C" {
#endif

static inline int ffs(int x)
{
    int r = 1;

    if (!x) {
        return 0;
    }

    if (!(x & 0xffff)) {
        x >>= 16;
        r += 16;
    }
    if (!(x & 0xff)) {
        x >>= 8;
        r += 8;
    }
    if (!(x & 0xf)) {
        x >>= 4;
        r += 4;
    }
    if (!(x & 3)) {
        x >>= 2;
        r += 2;
    }
    if (!(x & 1)) {
        x >>= 1;
        r += 1;
    }
    return r;
}

static inline int fls(int x)
{
    int r = 32;

    if (!x) {
        return 0;
    }

    if (!(x & 0xffff0000u)) {
        x <<= 16;
        r -= 16;
    }
    if (!(x & 0xff000000u)) {
        x <<= 8;
        r -= 8;
    }
    if (!(x & 0xf0000000u)) {
        x <<= 4;
        r -= 4;
    }
    if (!(x & 0xc0000000u)) {
        x <<= 2;
        r -= 2;
    }
    if (!(x & 0x80000000u)) {
        x <<= 1;
        r -= 1;
    }
    return r;
}

#ifdef __cplusplus
}
#endif
#else
/**
 * @brief 查找第一个为 1 的位
 * @param x 输入整数
 * @return 1-based 索引，若为 0 则返回 0
 */
int ffs(int x);

/**
 * @brief 查找最后一个为 1 的位
 * @param x 输入整数
 * @return 1-based 索引，若为 0 则返回 0
 */
int fls(int x);

#endif
#endif
