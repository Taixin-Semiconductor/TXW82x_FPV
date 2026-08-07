#ifndef __VIDEO_ERR_H
#define __VIDEO_ERR_H
#ifndef BIT
#define BIT(a)             (1UL << (a))
#endif
enum
{
    ISP_VPP_ERR = 0,
    ISP_JPG0_ERR,
    ISP_JPG1_ERR,
    ISP_H264_ERR,
    ISP_SCALE3_ERR,
};
#endif