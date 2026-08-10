#ifndef _TXSEMI_MEDIATYPES_H_
#define _TXSEMI_MEDIATYPES_H_
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file media_types.h
 * @brief 多媒体基础类型定义 (视频/音频/图片)
 *
 * 包含编解码器 ID、像素格式、色彩空间及元数据结构体定义。
 */

/**
 * @brief 媒体容器/封装格式类型
 *
 * 用于 demuxer 识别输入数据的封装方式。
 */
typedef enum {
    MEDIA_CONTAINER_INVALID = 0,  ///< Unknown
    // 图片容器（单帧文件）
    MEDIA_CONTAINER_JPEG,     ///< JPEG 文件
    MEDIA_CONTAINER_PNG,      ///< PNG 文件
    MEDIA_CONTAINER_GIF,      ///< GIF 文件
    MEDIA_CONTAINER_WEBP,     ///< WebP 文件
    MEDIA_CONTAINER_HEIF,     ///< HEIF/HEIC 文件
    MEDIA_CONTAINER_BMP,      ///< BMP 文件
    MEDIA_CONTAINER_TIFF,     ///< TIFF 文件
    MEDIA_CONTAINER_SVG,      ///< SVG 文件

    // 音频专用容器
    MEDIA_CONTAINER_MP3,      ///< MP3 文件（MPEG-1 Layer 3）
    MEDIA_CONTAINER_WAV,      ///< WAV (RIFF) 文件
    MEDIA_CONTAINER_FLAC,     ///< FLAC 文件（自包含容器）
    MEDIA_CONTAINER_OGG,      ///< Ogg 容器（通常含 Vorbis/Opus/Theora）
    MEDIA_CONTAINER_M4A,      ///< M4A（MP4 音频子集，含 AAC/ALAC）
    MEDIA_CONTAINER_AMR,      ///< AMR-NB/WB 文件

    // 多路复用视频容器
    MEDIA_CONTAINER_MP4,      ///< MP4 (ISO Base Media File Format)
    MEDIA_CONTAINER_MKV,      ///< Matroska Video
    MEDIA_CONTAINER_AVI,      ///< Audio Video Interleave
    MEDIA_CONTAINER_TS,       ///< MPEG-2 Transport Stream（直播/HLS 基础）
    MEDIA_CONTAINER_M2TS,     ///< Blu-ray Transport Stream
    MEDIA_CONTAINER_FLV,      ///< Flash Video
    MEDIA_CONTAINER_MOV,      ///< QuickTime File Format
    MEDIA_CONTAINER_WEBM,     ///< WebM（基于 Matrosika，VP8/VP9/AV1 + Opus/Vorbis）
    MEDIA_CONTAINER_WMV,      ///< Windows Media Video
    MEDIA_CONTAINER_WMA,      ///< Windows Media Audio

    // 裸流（无容器，直接为编码比特流）
    MEDIA_CONTAINER_RAW_H264, ///< Annex B 格式 H.264 流（含 start code）
    MEDIA_CONTAINER_RAW_H265, ///< Annex B 格式 H.265 流
    MEDIA_CONTAINER_RAW_AV1,  ///< AV1 OBUs 流
    MEDIA_CONTAINER_RAW_VP8,  ///< VP8 帧流
    MEDIA_CONTAINER_RAW_VP9,  ///< VP9 帧流
    MEDIA_CONTAINER_RAW_AAC,  ///< ADTS 或 raw AAC 流
    MEDIA_CONTAINER_RAW_PCM,  ///< 原始 PCM 音频流

    MEDIA_CONTAINER_MAX,      ///< 枚举上限，用于校验
} media_container_type_t;

/**
 * @brief 主媒体数据类别
 */
typedef enum {
    MEDIA_DATA_VIDEO   = 0,   ///< 视频流（包含压缩的视频帧，如 H.264/H.265）
    MEDIA_DATA_AUDIO   = 1,   ///< 音频流（包含压缩或未压缩的音频帧）
    MEDIA_DATA_PICTURE = 2,   ///< 静态图片（如专辑封面、缩略图、海报）
    MEDIA_DATA_SUBTITLE = 3,  ///< 字幕数据（文本型如 SRT/WebVTT，或图形型如 PGS/VobSub）

    ///////////////////////////////////////
    //SDK自定义类型，从0x10开始


    ///////////////////////////////////////
    MEDIA_DATA_UNKNOWN = 0xff,
} media_data_category_t;

/**
 * @brief 统一的多媒体像素格式枚举
 *
 */
typedef enum {
    IMAGE_PIX_FMT_NONE = 0,           ///< 未知/无效格式

    // --- 基础 YUV 格式 (最常用) ---
    IMAGE_PIX_FMT_YUV420P     = 1,    ///< Planar YUV 4:2:0, 12bpp (1Y + 0.5U + 0.5V)
    IMAGE_PIX_FMT_YUV422P     = 2,    ///< Planar YUV 4:2:2, 16bpp
    IMAGE_PIX_FMT_YUV444P     = 3,    ///< Planar YUV 4:4:4, 24bpp
    IMAGE_PIX_FMT_NV12        = 4,    ///< Semi-planar YUV 4:2:0 (Y + UV interleaved)
    IMAGE_PIX_FMT_NV21        = 5,    ///< Semi-planar YUV 4:2:0 (Y + VU interleaved)
    IMAGE_PIX_FMT_NV16        = 6,    ///< Semi-planar YUV 4:2:2 (Y + UV interleaved)
    IMAGE_PIX_FMT_YUYV422     = 7,    ///< Packed YUV 4:2:2 (YUYV)
    IMAGE_PIX_FMT_UYVY422     = 8,    ///< Packed YUV 4:2:2 (UYVY)
    IMAGE_PIX_FMT_YUV411P     = 9,    ///< Planar YUV 4:1:1

    // --- 基础 RGB/BGR 格式 ---
    IMAGE_PIX_FMT_RGB24       = 10,   ///< Packed RGB 8:8:8
    IMAGE_PIX_FMT_BGR24       = 11,   ///< Packed BGR 8:8:8
    IMAGE_PIX_FMT_RGBA8888    = 12,   ///< Packed RGBA 8:8:8:8 (Alpha 255=Opaque)
    IMAGE_PIX_FMT_BGRA8888    = 13,   ///< Packed BGRA 8:8:8:8
    IMAGE_PIX_FMT_RGBX8888    = 14,   ///< Packed RGBX 8:8:8:8 (Alpha ignored)
    IMAGE_PIX_FMT_BGRX8888    = 15,   ///< Packed BGRX 8:8:8:8
    IMAGE_PIX_FMT_RGB565      = 16,   ///< Packed RGB 5:6:5
    IMAGE_PIX_FMT_RGB555      = 17,   ///< Packed RGB 5:5:5 (+1 bit padding)
    IMAGE_PIX_FMT_BGR565      = 18,   ///< Packed BGR 5:6:5
    IMAGE_PIX_FMT_BGR555      = 19,   ///< Packed BGR 5:5:5

    // --- 灰度与调色板 ---
    IMAGE_PIX_FMT_GRAY8       = 20,   ///< 8-bit Grayscale
    IMAGE_PIX_FMT_GRAY16LE    = 21,   ///< 16-bit Grayscale Little Endian
    IMAGE_PIX_FMT_GRAY16BE    = 22,   ///< 16-bit Grayscale Big Endian
    IMAGE_PIX_FMT_PAL8        = 23,   ///< 8-bit with Palette

    // --- 高位深 YUV (HDR/10bit/12bit) ---
    IMAGE_PIX_FMT_YUV420P10LE = 50,   ///< 10-bit Planar YUV 4:2:0 Little Endian
    IMAGE_PIX_FMT_YUV420P10BE = 51,   ///< 10-bit Planar YUV 4:2:0 Big Endian
    IMAGE_PIX_FMT_YUV420P12LE = 52,   ///< 12-bit Planar YUV 4:2:0 Little Endian
    IMAGE_PIX_FMT_YUV422P10LE = 53,   ///< 10-bit Planar YUV 4:2:2 Little Endian
    IMAGE_PIX_FMT_YUV444P10LE = 54,   ///< 10-bit Planar YUV 4:4:4 Little Endian
    IMAGE_PIX_FMT_P010LE      = 55,   ///< 10-bit Semi-planar YUV 4:2:0 (Like NV12 but 16bit per component)
    IMAGE_PIX_FMT_P016LE      = 56,   ///< 16-bit Semi-planar YUV 4:2:0

    // --- 高位深 RGB ---
    IMAGE_PIX_FMT_RGB48LE     = 60,   ///< 16-bit per channel RGB Little Endian
    IMAGE_PIX_FMT_RGB48BE     = 61,   ///< 16-bit per channel RGB Big Endian
    IMAGE_PIX_FMT_RGBA64LE    = 62,   ///< 16-bit per channel RGBA Little Endian
    IMAGE_PIX_FMT_RGBA64BE    = 63,   ///< 16-bit per channel RGBA Big Endian

    IMAGE_PIX_FMT_NB                  ///< 格式总数 (用于边界检查)
} image_pixel_format_t;

/**
 * @brief 图像色彩空间标识
 *
 * 用于描述静态图片的颜色原色和传递函数。
 */
typedef enum {
    IMAGE_COLOR_SPACE_UNKNOWN = 0,
    IMAGE_COLOR_SPACE_SRGB,         ///< sRGB (Web/JPEG 标准)
    IMAGE_COLOR_SPACE_BT709,        ///< ITU-R BT.709 (HDTV)
    IMAGE_COLOR_SPACE_BT601,        ///< ITU-R BT.601 (SDTV)
    IMAGE_COLOR_SPACE_BT2020,       ///< ITU-R BT.2020 (UHD/HDR)
    IMAGE_COLOR_SPACE_DCIP3,        ///< DCI-P3 (数字电影/广色域显示器)
    IMAGE_COLOR_SPACE_ADOBE_RGB,    ///< Adobe RGB (摄影打印)
    IMAGE_COLOR_SPACE_GRAY,         ///< Grayscale (无色彩空间)
} image_color_space_t;

/**
 * @brief 判断图像像素格式是否包含 Alpha 通道
 */
static inline int image_pix_fmt_has_alpha(image_pixel_format_t fmt)
{
    return (fmt == IMAGE_PIX_FMT_RGBA8888 ||
            fmt == IMAGE_PIX_FMT_BGRA8888 ||
            fmt == IMAGE_PIX_FMT_RGBA64LE);
}

/**
 * @brief 判断图像像素格式是否为 YUV 类型
 */
static inline int image_pix_fmt_is_yuv(image_pixel_format_t fmt)
{
    return (fmt >= IMAGE_PIX_FMT_YUV420P && fmt <= IMAGE_PIX_FMT_NV21) ||
           (fmt >= IMAGE_PIX_FMT_YUV420P10LE && fmt <= IMAGE_PIX_FMT_P016LE);
}

/**
 * @brief 获取像素格式的近似每像素字节数（仅用于粗略估算）
 *
 * 注意：对于平面格式，返回的是平均每个像素占用的字节数（如 YUV420P 返回 1.5，
 * 此处返回整数乘以 10 的值以避免浮点，例如 YUV420P 返回 15）。
 * 实际内存分配请使用 image_pix_fmt_get_buffer_size()。
 */
int image_pix_fmt_get_bpp_x10(image_pixel_format_t fmt);

/**
 * @brief 计算指定像素格式图像所需的缓冲区大小（字节）
 *
 * @param fmt   像素格式
 * @param width  图像宽度（像素）
 * @param height 图像高度（像素）
 * @param align  行对齐字节数（0 表示不对齐，通常为 1）
 * @return 所需缓冲区大小（字节），若格式未知或参数无效返回 0
 */
size_t image_pix_fmt_get_buffer_size(image_pixel_format_t fmt,
                                               uint32_t width,
                                               uint32_t height,
                                               uint32_t align);

#include "video.h"
#include "audio.h"
#include "subtitle.h"
#include "picture.h"

#ifdef __cplusplus
}
#endif

#endif
