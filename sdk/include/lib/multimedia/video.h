#ifndef _TXSEMI_MEDIA_VIDEO_H_
#define _TXSEMI_MEDIA_VIDEO_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 视频编码格式（Codec ID）
 *
 * 表示视频流所使用的压缩编码标准。
 * 这里定义了常见的编码格式，但是不意味着 芯片/SDK 支持所有列出来的格式
 */
typedef enum {
    VIDEO_CODEC_H264   = 1,   ///< H.264 / AVC
    VIDEO_CODEC_H265,   ///< H.265 / HEVC
    VIDEO_CODEC_AV1,   ///< AOMedia Video 1
    VIDEO_CODEC_VP9    ,   ///< Google VP9
    VIDEO_CODEC_VP8    ,   ///< Google VP8
    VIDEO_CODEC_MPEG2  ,   ///< MPEG-2 Video (常用于 DVD/广播)
    VIDEO_CODEC_MPEG4  ,   ///< MPEG-4 Part 2 (如 DivX/Xvid)
    VIDEO_CODEC_MJPEG  ,   ///< Motion JPEG

    ///////////////////////////////////////
    //添加SDK自定义类型，从0x30开始
	VIDEO_CODEC_YUV    = 0x30,

    ///////////////////////////////////////
    VIDEO_CODEC_INVALID = 0xff, ///< Unknown
} video_codec_t;

// --- H.264 Profile IDs ---
#define H264_PROFILE_BASELINE       66
#define H264_PROFILE_MAIN           77
#define H264_PROFILE_HIGH           100
#define H264_PROFILE_HIGH_10        110

// --- H.265 Profile IDs ---
#define H265_PROFILE_MAIN           1
#define H265_PROFILE_MAIN_10        2

// --- AAC Profile IDs (MPEG-4 Audio Object Types) ---
#define AAC_PROFILE_LC              2   ///< Low Complexity (最常用)
#define AAC_PROFILE_MAIN            1
#define AAC_PROFILE_HE              5   ///< HE-AAC (SBR)
#define AAC_PROFILE_HE_V2           29  ///< HE-AAC v2 (SBR + PS)

// --- JPEG Profile (自定义映射) ---
#define JPEG_PROFILE_BASELINE       0
#define JPEG_PROFILE_PROGRESSIVE    1
#define JPEG_PROFILE_LOSSLESS       2

/**
 * @brief 视频编解码参数信息
 *
 * 描述一个视频流的静态元数据，用于初始化解码器。
 * 所有字段在流开始时确定，解码过程中不变。
 */
typedef struct {
    uint16 width;                 ///< 显示宽度（像素），最大 65535（支持 8K）
    uint16 height;                ///< 显示高度（像素）
    uint16 coded_width;           ///< 编码宽度（可能含对齐 padding，如 1280x720 → 1280x736）
    uint16 coded_height;          ///< 编码高度

    uint16 fps_num;               ///< 帧率分子（如 30000 表示 30000/1001 ≈ 29.97 fps）
    uint16 fps_den;               ///< 帧率分母（常见值：1, 1001）
    uint32 time_base_num;         ///< 时间基分子（通常为 1）
    uint32 time_base_den;         ///< 时间基分母（如 90000 for TS, 1001 for NTSC）

    uint8  profile;               ///< 编码 Profile（H.264: Baseline=66, Main=77, High=100）
    uint8  level;                 ///< 编码 Level（H.264: 40=Level 4.0, 51=Level 5.1）
    uint8  has_b_frames: 1,       ///< 是否包含 B 帧（0=无，1=有；影响解码顺序）
           is_vfr: 1,             /// 0=CFR（fps_* 有效），1=VFR（fps_* 为平均值或 0）
           rev: 6;
    uint8  codec_id;              ///< 视频编码格式 ID，取值见 video_codec_t

    uint32 bit_rate;              ///< 标称码率（bps），0 表示可变码率（VBR）

    /**
     * @brief 色彩描述信息（符合 ITU-T H.273 标准）
     *
     * 用于 HDR、色彩空间转换等高级渲染。
     */
    struct {
        uint8 color_primaries;    ///< 色域原色（1=BT.709, 9=BT.2020, 11=DCI-P3）
        uint8 color_transfer;     ///< 电光转换函数（13=sRGB, 14=Gamma 2.2, 16=PQ, 18=HLG）
        uint8 color_space;        ///< 色彩矩阵（1=YUV/Rec.601, 2=Rec.709, 9=Rec.2020 NC）
        uint8 chroma_location;    ///< 色度采样位置（0=左上, 1=中心；影响缩放精度）
    } color;

    uint8 *extradata;             ///< 带外配置数据指针（如 H.264 的 SPS/PPS，AAC 的 ASC）
    uint32 extradata_size;        ///< extradata 数据长度（字节），0 表示无
} txVideoInfo_t;

/**
 * @brief 视频帧解码控制参数信息
 *
 * 描述一个framebuf的解码参数，用于控制解码器如何进行解码，例如指定解码后的 width，height。
 * 可以通过此信息控制每个framebuff的解码行为。
 */
typedef struct {
    uint16 width;                 ///< 解码后的宽度（像素）
    uint16 height;                ///< 解码后的高度（像素）
}txVideoDecInfo_t;

/**
 * @brief H.264 AVCC 配置记录的固定头部部分
 *
 * 注意：该结构体仅包含前 5 个字节，完整的 AVCC 记录还包含 SPS/PPS 个数及数据。
 * 实际解析请参考 ISO/IEC 14496-15，或使用辅助函数。
 */
typedef struct {
    uint8 configurationVersion;     ///< 必须为 1
    uint8 AVCProfileIndication;
    uint8 profile_compatibility;
    uint8 AVCLevelIndication;
    uint8 lengthSizeMinusOne:2;     
    uint8 rev               :6;
} h264_avcc_header_t;

/**
 * @brief H.264 AVCC 解析结果
 */
typedef struct {
    const uint8 *sps_data;     ///< SPS NAL 单元数据指针（指向 extradata 内部）
    uint32       sps_size;     ///< SPS 数据长度（字节）
    const uint8 *pps_data;     ///< PPS NAL 单元数据指针
    uint32       pps_size;     ///< PPS 数据长度（字节）
    uint8        profile;      ///< AVCProfileIndication
    uint8        level;        ///< AVCLevelIndication
    uint8        length_size;  ///< NAL 长度字段字节数（1,2,4）
} h264_avcc_info_t;

/**
 * @brief H.265 HVCC 配置记录的固定头部（前 22 字节）
 *
 * 注意：此结构体仅覆盖配置记录的固定部分，不包含后续的 NAL 数组。
 * 直接通过指针映射可能因对齐、字节序等问题不可移植，建议使用解析函数。
 */
typedef struct {
    uint8 configurationVersion;           ///< 必须为 1
    uint8 general_profile_space;          ///< 高 2 位，低 6 位保留
    uint8 general_tier_flag;              ///< 1 位，其余 7 位保留
    uint8 general_profile_idc;            ///< profile idc
    uint32 general_profile_compatibility_flags;   ///< 4 字节，兼容标志
    uint8 general_constraint_indicator_flags[6];  ///< 6 字节
    uint8 general_level_idc;              ///< level idc
    uint16 min_spatial_segmentation_idc;  ///< 最小空间分割（低位 12 位有效）
    uint8 parallelismType;                ///< 并行类型
    uint8 chromaFormat;                   ///< 色度格式（6 位有效）
    uint8 bitDepthLumaMinus8;             ///< 亮度位深-8（3 位有效）
    uint8 bitDepthChromaMinus8;           ///< 色度位深-8（3 位有效）
    uint16 avgFrameRate;                  ///< 平均帧率（0 表示未知）
    uint8 constantFrameRate  : 2;         ///< 恒定帧率标识
    uint8 numTemporalLayers  : 3;         ///< 时间层数
    uint8 temporalIdNested   : 1;         ///< 是否允许时间 ID 嵌套
    uint8 lengthSizeMinusOne : 2;         ///< NAL 长度字段字节数-1
    uint8 numOfArrays;                    ///< NAL 数组数量
} h265_hvcc_header_t;

/**
 * @brief H.265 HVCC 解析结果
 */
typedef struct {
    const uint8 *vps_data;    ///< VPS NAL 单元数据指针
    uint32       vps_size;
    const uint8 *sps_data;    ///< SPS NAL 单元数据指针
    uint32       sps_size;
    const uint8 *pps_data;    ///< PPS NAL 单元数据指针
    uint32       pps_size;
    uint8        length_size; ///< NAL 长度字段字节数（1,2,4）
    uint8        num_arrays;  ///< 配置记录中的数组个数（通常为3）
} h265_hvcc_info_t;


/**
 * @brief 视频YUV参数信息
 *
 * 描述一个视频YUV的信息
 */
typedef struct {
    uint16 width;                 ///< YUV宽度（像素）
    uint16 height;                ///< YUV高度（像素）
    uint16 x;                     ///< X轴偏移量(像素)(如果是0xffff代表无效,可以修改)
    uint16 y;                     ///< Y轴偏移量(像素)(如果是0xffff代表无效,可以修改)
    uint8 *y_off;                ///< Y通道偏起始地址(暂时没有用)
    uint8 *u_off;                ///< U通道偏起始地址(暂时没有用)
    uint8 *v_off;                ///< V通道偏起始地址(暂时没有用)
    uint8  yuv_type;             ///< YUV的类型,YUV420 YUV422 YUV444(暂时没有用)
} txYuvInfo_t;


/**
 * @brief 解析 H.264 AVCC 格式的 extradata （仅取第一组 SPS/PPS）
 *
 * @param extradata   AVCC 配置数据（ISO/IEC 14496-15 定义的 AVCDecoderConfigurationRecord）
 * @param extradata_size 数据大小（至少 7 字节）
 * @param out         输出结构，成功时填充
 * @return 0 成功，-1 失败（格式错误或数据不足）
 */
int video_parse_h264_avcc(const uint8 *extradata, uint32 extradata_size, h264_avcc_info_t *out);

/**
 * @brief 解析 H.265 HVCC 格式的 extradata （仅取第一组 VPS/SPS/PPS）
 *
 * @param extradata   HVCC 配置数据（ISO/IEC 14496-15 定义的 HEVCDecoderConfigurationRecord）
 * @param extradata_size 数据大小（至少 23 字节）
 * @param out         输出结构，成功时填充
 * @return 0 成功，-1 失败
 */
int video_parse_h265_hvcc(const uint8 *extradata, uint32 extradata_size, h265_hvcc_info_t *out);

#ifdef __cplusplus
}
#endif

#endif
