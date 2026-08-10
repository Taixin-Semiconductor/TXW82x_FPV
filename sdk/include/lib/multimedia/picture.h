#ifndef _TXSEMI_MEDIA_PICTURE_H_
#define _TXSEMI_MEDIA_PICTURE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 图片格式（文件或像素表示格式）
 *
 * 用于静态图像，既可指文件格式（如 JPEG/PNG），也可指解码后的像素格式。
 * 这里定义了常见的图片格式，但是不意味着 芯片/SDK 支持所有列出来的格式
 */
typedef enum {
    PICTURE_FORMAT_JPEG = 0,  ///< Joint Photographic Experts Group
    PICTURE_FORMAT_PNG  = 1,  ///< Portable Network Graphics
    PICTURE_FORMAT_GIF  = 2,  ///< Graphics Interchange Format (支持动画)
    PICTURE_FORMAT_WEBP = 3,  ///< WebP (Google, 支持有损/无损/透明/动画)
    PICTURE_FORMAT_HEIF = 4,  ///< High Efficiency Image File (基于 HEVC)
    PICTURE_FORMAT_BMP  = 5,  ///< Windows Bitmap
    PICTURE_FORMAT_TIFF = 6,  ///< Tagged Image File Format
    PICTURE_FORMAT_SVG  = 7,  ///< Scalable Vector Graphics (矢量图)

    ///////////////////////////////////////
    //添加SDK自定义类型，从0x30开始

    ///////////////////////////////////////
    PICTURE_FORMAT_INVALID = 0xff, ///< Unknown
} picture_format_t;


/**
 * @brief 静态图片编解码参数信息
 *
 * 描述一张静态图像的静态元数据，用于初始化解码器、分配显存/内存或配置后处理流程。
 * 适用于 JPEG, PNG, WebP, BMP, HEIC 等单帧格式。
 */
typedef struct {
    uint8  codec_id;                   ///< 图片编码格式 ID，取值见 picture_format_t
    uint8  profile;                    ///< 编码 Profile
                                       ///< - JPEG: 0=Baseline, 1=Progressive, 2=Lossless
                                       ///< - PNG: 0=Standard, 1=Interlaced
                                       ///< - WebP: 0=Lossy, 1=Lossless

    uint8  orientation;                ///< EXIF 旋转方向 (1=Normal, 6=90° CW, etc.)
                                       ///< 1=Normal, 3=180°, 6=90° CW, 8=90° CCW 等 (0 表示未知/无需旋转)

    uint8  bit_depth;                  ///< 每分量位深度 (8, 10, 12, 16)。
                                       ///< 常见为 8 (SDR), 10/12 (HDR/RAW), 16 (PNG/TIFF)

    uint16 width;                      ///< 图像宽度 (像素)
    uint16 height;                     ///< 图像高度 (像素)

    image_pixel_format_t pixel_format; ///< 解码后目标像素格式枚举，见 image_pixel_format_t
                                       ///< (e.g., PIX_FMT_RGB888, PIX_FMT_RGBA8888, PIX_FMT_YUV420P, PIX_FMT_NV12)

    uint32 file_size;                  ///< 原始压缩数据大小 (字节)，用于估算解码耗时或校验完整性

    image_color_space_t color_space;   ///< 色彩空间标识，见 image_color_space_t
                                       ///< (e.g., COLOR_SPACE_SRGB, COLOR_SPACE_BT709, COLOR_SPACE_BT2020, COLOR_SPACE_GRAY)
    uint32 dpi;                        ///< 分辨率密度 (Dots Per Inch)，通常由 EXIF 解析得到，0 表示未知
                                       ///< 高 16 位可存水平 DPI，低 16 位存垂直 DPI，或统一存一个值

    uint8 *extradata;                  ///< 带外配置数据指针
                                       ///< - JPEG: 可包含量化表 (Quantization Tables) 或特定的 APP 标记数据
                                       ///< - PNG: 包含 IHDR 后的关键 chunk 信息 (如 PLTE, tRNS)
                                       ///< - WebP: VP8/VP9 头部信息
                                       ///< - 通用: 可指向解析后的 EXIF/XMP 元数据块
    uint16 extradata_size;             ///< extradata 数据长度（字节）

    uint8  quality_estimate;           ///< 估计的压缩质量 (1-100)，仅对有损格式 (JPEG/WebP) 有效，0 表示未知
    uint8  _reserved_tail;             ///< 显式保留，确保对齐
} picture_codec_info_t;

#ifdef __cplusplus
}
#endif

#endif
