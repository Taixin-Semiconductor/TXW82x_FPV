#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "txmplayer.h"

#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "txmplayer.h"

// 常量定义
#define MIN_DETECTION_LENGTH 12
#define BMP_HEADER_MIN_SIZE 6
#define TIFF_HEADER_SIZE 8
#define FLV_HEADER_SIZE 5
#define TS_PACKET_SIZE 188
#define M2TS_PACKET_SIZE 204

// 裸视频检测相关常量
#define H264_NAL_UNIT_TYPE_SPS 7
#define H264_NAL_UNIT_TYPE_PPS 8
#define H264_NAL_UNIT_TYPE_IDR 5
#define H264_NAL_UNIT_TYPE_NON_IDR 1

#define H265_NAL_UNIT_TYPE_VPS 32
#define H265_NAL_UNIT_TYPE_SPS 33
#define H265_NAL_UNIT_TYPE_PPS 34
#define H265_NAL_UNIT_TYPE_IDR 19
#define H265_NAL_UNIT_TYPE_NON_IDR 1

#define AV1_OBU_TYPE_SEQUENCE_HEADER 1
#define AV1_OBU_TYPE_FRAME_HEADER 3
#define AV1_OBU_TYPE_FRAME 6
#define AV1_OBU_TYPE_TILE_GROUP 4

// 辅助宏：检查数据长度是否足够，并比较特征签名
#define CHECK_SIGNATURE(data, len, sig, sig_len) \
    ((len) >= (sig_len) && memcmp(data, sig, sig_len) == 0)

static uint8 check_signature_ignore_whitespace(const uint8_t *data, uint32_t len,
        const char *sig, uint32_t sig_len)
{
    if (len < sig_len) return 0;

    uint32_t data_pos = 0;
    while (data_pos < len && (data[data_pos] == ' ' || data[data_pos] == '\t' ||
                              data[data_pos] == '\r' || data[data_pos] == '\n')) {
        data_pos++;
    }

    if (len - data_pos < sig_len) return 0;

    for (uint32_t i = 0; i < sig_len; i++) {
        if (data[data_pos + i] != (uint8_t)sig[i] &&
            data[data_pos + i] != (uint8_t)(sig[i] ^ 0x20)) {
            return 0;
        }
    }
    return 1;
}

static const uint8_t *find_pattern(const uint8_t *haystack, uint32_t haystack_len,
                                   const uint8_t *needle, uint32_t needle_len)
{
    if (haystack_len < needle_len) return NULL;

    for (uint32_t i = 0; i <= haystack_len - needle_len; i++) {
        if (memcmp(haystack + i, needle, needle_len) == 0) {
            return haystack + i;
        }
    }
    return NULL;
}

static uint8 check_packet_structure(const uint8_t *data, uint32_t len,
                                   uint32_t packet_size, uint32_t min_packets)
{
    if (len < packet_size * min_packets) return 0;

    uint32_t valid_packets = 0;
    for (uint32_t i = 0; i < min_packets && i * packet_size < len; i++) {
        const uint8_t *packet = data + i * packet_size;
        if (packet[0] != 0x47) {
            continue;
        }
        
        // 增强TS包检测：检查PID字段，排除空包
        if (packet_size == TS_PACKET_SIZE) {
            uint16_t pid = ((packet[1] & 0x1F) << 8) | packet[2];
            if ((pid & 0x1FFF) == 0x1FFF) { // PID=0x1FFF表示空包
                continue;
            }
        }
        valid_packets++;
    }
    
    return valid_packets >= min_packets;
}

// ==================== MP4格式检测函数 ====================
static int32_t detect_mp4(const uint8_t *data, uint32_t len)
{
    if (len < 8) return MEDIA_DTYPE_MAX;
    
    // 检查MP4特征盒子
    size_t offset = 0;
    while (offset + 8 <= len && offset < 1024) { // 只检查前1KB
        uint32_t box_size = get_unaligned_be32(data + offset);
        uint32_t box_type = get_unaligned_be32(data + offset + 4);
        
        // 安全的盒子大小检查
        if (box_size < 8 || box_size > len - offset) {
            break;
        }
        
        // MP4关键盒子类型
        switch (box_type) {
            case 0x66747970: // "ftyp"
            case 0x6D6F6F76: // "moov"
            case 0x6D6F6F66: // "moof" - fMP4
            case 0x6D646174: // "mdat"
                return MEDIA_DTYPE_VIDEO_MP4;
        }
        
        offset += box_size;
    }
    
    return MEDIA_DTYPE_MAX;
}

// ==================== 裸视频编码帧检测函数 ====================
static int32_t detect_h264_raw(const uint8_t *data, uint32_t len)
{
    if (len < 16) return MEDIA_DTYPE_MAX;

    uint32_t start_code_count = 0;
    uint32_t sps_count = 0;
    uint32_t pps_count = 0;
    
    // H.264 NAL单元通常以0x000001或0x00000001开头
    for (uint32_t i = 0; i <= len - 4; i++) {
        uint8 found_start_code = 0;
        uint32_t nal_offset = 0;
        
        // 检查3字节起始码 0x000001
        if (i <= len - 3 && data[i] == 0x00 && data[i + 1] == 0x00 && data[i + 2] == 0x01) {
            nal_offset = i + 3;
            found_start_code = 1;
        }
        // 检查4字节起始码 0x00000001
        else if (i <= len - 4 && data[i] == 0x00 && data[i + 1] == 0x00 && 
                 data[i + 2] == 0x00 && data[i + 3] == 0x01) {
            nal_offset = i + 4;
            found_start_code = 1;
        }
        
        if (found_start_code && nal_offset < len) {
            uint8_t nal_unit_type = data[nal_offset] & 0x1F;

            // 检查是否为SPS、PPS或IDR帧等关键NAL单元
            switch (nal_unit_type) {
                case H264_NAL_UNIT_TYPE_SPS:
                    sps_count++;
                    break;
                case H264_NAL_UNIT_TYPE_PPS:
                    pps_count++;
                    break;
                case H264_NAL_UNIT_TYPE_IDR:
                case H264_NAL_UNIT_TYPE_NON_IDR:
                    start_code_count++;
                    break;
            }
            
            // 更严格的条件：需要SPS+PPS+帧数据
            if (sps_count >= 1 && pps_count >= 1 && start_code_count >= 2) {
                return MEDIA_DTYPE_RAW_H264;
            }
        }
    }
    return MEDIA_DTYPE_MAX;
}

static int32_t detect_h265_raw(const uint8_t *data, uint32_t len)
{
    if (len < 16) return MEDIA_DTYPE_MAX;

    uint32_t start_code_count = 0;
    uint32_t vps_count = 0;
    uint32_t sps_count = 0;
    uint32_t pps_count = 0;
    
    // H.265 NAL单元也以0x000001或0x00000001开头
    for (uint32_t i = 0; i <= len - 4; i++) {
        uint8 found_start_code = 0;
        uint32_t nal_offset = 0;
        
        // 检查3字节起始码 0x000001
        if (i <= len - 3 && data[i] == 0x00 && data[i + 1] == 0x00 && data[i + 2] == 0x01) {
            nal_offset = i + 3;
            found_start_code = 1;
        }
        // 检查4字节起始码 0x00000001
        else if (i <= len - 4 && data[i] == 0x00 && data[i + 1] == 0x00 && 
                 data[i + 2] == 0x00 && data[i + 3] == 0x01) {
            nal_offset = i + 4;
            found_start_code = 1;
        }
        
        if (found_start_code && nal_offset < len) {
            uint8_t nal_unit_type = (data[nal_offset] >> 1) & 0x3F;

            // 检查是否为VPS、SPS、PPS等关键NAL单元
            switch (nal_unit_type) {
                case H265_NAL_UNIT_TYPE_VPS:
                    vps_count++;
                    break;
                case H265_NAL_UNIT_TYPE_SPS:
                    sps_count++;
                    break;
                case H265_NAL_UNIT_TYPE_PPS:
                    pps_count++;
                    break;
                case H265_NAL_UNIT_TYPE_IDR:
                case H265_NAL_UNIT_TYPE_NON_IDR:
                    start_code_count++;
                    break;
            }
            
            // 更严格的条件：需要VPS+SPS+PPS+帧数据
            if (vps_count >= 1 && sps_count >= 1 && pps_count >= 1 && start_code_count >= 2) {
                return MEDIA_DTYPE_RAW_H265;
            }
        }
    }
    return MEDIA_DTYPE_MAX;
}

static int32_t detect_av1_raw(const uint8_t *data, uint32_t len)
{
    if (len < 8) return MEDIA_DTYPE_MAX;

    uint32_t valid_obu_count = 0;
    
    // AV1帧通常以OBU（Open Bitstream Unit）开始
    for (uint32_t i = 0; i <= len - 2; i++) {
        uint8_t obu_header = data[i];
        
        // 检查OBU头格式：bit4-7必须为0
        if ((obu_header & 0xF0) != 0x00) {
            continue;
        }
        
        uint8_t obu_type = obu_header & 0x0F; // 实际OBU类型在bit0-3
        //uint8_t has_extension = (obu_header & 0x08) ? 1 : 0; // 实际在bit3
        //uint8_t has_size = (obu_header & 0x04) ? 1 : 0;     // 实际在bit2
        
        // 验证OBU类型
        if (obu_type == AV1_OBU_TYPE_SEQUENCE_HEADER ||
            obu_type == AV1_OBU_TYPE_FRAME_HEADER ||
            obu_type == AV1_OBU_TYPE_FRAME ||
            obu_type == AV1_OBU_TYPE_TILE_GROUP) {
            valid_obu_count++;
            
            // 需要更多证据来确认
            if (valid_obu_count >= 3) {
                return MEDIA_DTYPE_RAW_AV1;
            }
        }
    }
    return MEDIA_DTYPE_MAX;
}

static int32_t detect_vp9_raw(const uint8_t *data, uint32_t len)
{
    if (len < 8) return MEDIA_DTYPE_MAX;

    uint32_t frame_count = 0;
    
    // VP9帧头检测
    for (uint32_t i = 0; i <= len - 3; i++) {
        // VP9帧头特征：第0字节bit0=1表示帧开始，bit1=错误标记，bit2=帧内，bit3=重置上下文
        // bit4~6=版本号，bit7=保留
        if ((data[i] & 0x80) == 0x00) { // 保留位必须为0
            uint8_t version = (data[i] >> 4) & 0x07;
            //uint8 show_frame = (data[i] & 0x08) != 0;
            
            // 有效的VP9版本和合理的帧头
            if (version <= 4) { // VP9版本0-4
                frame_count++;
                // 找到3个有效的帧头才认为是VP9流
                if (frame_count >= 3) {
                    return MEDIA_DTYPE_RAW_VP9;
                }
            }
        }
    }
    return MEDIA_DTYPE_MAX;
}

static int32_t detect_vp8_raw(const uint8_t *data, uint32_t len)
{
    if (len < 8) return MEDIA_DTYPE_MAX;

    uint32_t frame_count = 0;
    
    // VP8帧头检测
    for (uint32_t i = 0; i <= len - 6; i++) {
        // VP8帧头特征：第0字节bit0~2=版本，bit3=显示标记，bit4~7=保留
        if ((data[i] & 0xF0) == 0x90) { // 检查帧头特征
            uint8_t version = data[i] & 0x07;
            
            // 检查关键帧标记（第3字节bit0）
            //uint8 key_frame = (data[i + 3] & 0x01) == 0;
            
            if (version <= 3) { // VP8版本0-3
                frame_count++;
                // 找到3个有效的帧头才认为是VP8流
                if (frame_count >= 3) {
                    return MEDIA_DTYPE_RAW_VP8;
                }
            }
        }
    }
    return MEDIA_DTYPE_MAX;
}

static int32_t detect_mpeg2_raw(const uint8_t *data, uint32_t len)
{
    if (len < 16) return MEDIA_DTYPE_MAX;

    uint32_t start_code_count = 0;
    uint32_t sequence_header_count = 0;
    
    // MPEG-2起始码：0x000001 + 起始码值
    for (uint32_t i = 0; i <= len - 4; i++) {
        if (data[i] == 0x00 && data[i + 1] == 0x00 && data[i + 2] == 0x01) {
            uint8_t start_code = data[i + 3];
            
            // 检查常见的MPEG-2起始码
            if (start_code == 0x00) { // 图片起始码
                start_code_count++;
            } else if (start_code == 0xB3) { // 序列头
                sequence_header_count++;
            } else if (start_code == 0xB8) { // GOP头
                start_code_count++;
            } else if (start_code >= 0x01 && start_code <= 0xAF) { // 切片起始码
                start_code_count++;
            }
            
            // 需要序列头和足够的起始码
            if (sequence_header_count >= 1 && start_code_count >= 3) {
                return MEDIA_DTYPE_RAW_MPEG2;
            }
        }
    }
    return MEDIA_DTYPE_MAX;
}

static int32_t detect_mpeg4_raw(const uint8_t *data, uint32_t len)
{
    if (len < 16) return MEDIA_DTYPE_MAX;

    uint32_t start_code_count = 0;
    uint32_t vol_count = 0;
    
    // MPEG-4 Visual起始码：0x000001Bx
    for (uint32_t i = 0; i <= len - 4; i++) {
        if (data[i] == 0x00 && data[i + 1] == 0x00 && data[i + 2] == 0x01) {
            uint8_t start_code = data[i + 3];
            
            // MPEG-4 Visual对象起始码范围
            if (start_code >= 0x20 && start_code <= 0x2F) {
                if (start_code == 0x20) { // 视频对象序列
                    vol_count++;
                }
                start_code_count++;
                
                // 需要VOL和足够的起始码
                if (vol_count >= 1 && start_code_count >= 3) {
                    return MEDIA_DTYPE_RAW_MPEG4;
                }
            }
        }
    }
    return MEDIA_DTYPE_MAX;
}

// ==================== 图片格式检测函数 ====================
static int32_t detect_png(const uint8_t *data, uint32_t len)
{
    static const uint8_t png_sig[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    return CHECK_SIGNATURE(data, len, png_sig, sizeof(png_sig)) ?
           MEDIA_DTYPE_PIC_PNG : MEDIA_DTYPE_MAX;
}

static int32_t detect_jpg(const uint8_t *data, uint32_t len)
{
    if (len >= 4 && data[0] == 0xFF && data[1] == 0xD8) {
        if (data[2] == 0xFF && (data[3] & 0xF0) == 0xE0) {
            return MEDIA_DTYPE_PIC_JPG;
        }
    }
    return MEDIA_DTYPE_MAX;
}

static int32_t detect_gif(const uint8_t *data, uint32_t len)
{
    if (len >= 6 && (memcmp(data, "GIF87a", 6) == 0 || memcmp(data, "GIF89a", 6) == 0)) {
        return MEDIA_DTYPE_PIC_GIF;
    }
    return MEDIA_DTYPE_MAX;
}

static int32_t detect_bmp(const uint8_t *data, uint32_t len)
{
    if (len >= BMP_HEADER_MIN_SIZE && data[0] == 0x42 && data[1] == 0x4D) {
        uint32_t file_size = get_unaligned_le32(data + 2);
        // 修复BMP文件大小判断逻辑
        if (file_size > 54 && file_size <= len) { // 最小BMP文件至少54字节
            return MEDIA_DTYPE_PIC_BMP;
        }
    }
    return MEDIA_DTYPE_MAX;
}

static int32_t detect_webp(const uint8_t *data, uint32_t len)
{
    // 修复WebP检测逻辑，排除size=0的无效文件
    if (len >= 16 && memcmp(data, "RIFF", 4) == 0 && memcmp(data + 8, "WEBP", 4) == 0) {
        uint32_t riff_size = get_unaligned_le32(data + 4);
        if (riff_size > 8 && riff_size + 8 <= len) {
            return MEDIA_DTYPE_PIC_WEBP;
        }
    }
    return MEDIA_DTYPE_MAX;
}

static int32_t detect_tiff(const uint8_t *data, uint32_t len)
{
    if (len < TIFF_HEADER_SIZE) return MEDIA_DTYPE_MAX;

    uint8 is_tiff = 0;
    if (data[0] == 0x49 && data[1] == 0x49 && data[2] == 0x2A && data[3] == 0x00) {
        uint32_t ifd_offset = get_unaligned_le32(data + 4);
        is_tiff = (ifd_offset >= 8 && ifd_offset < len);
    } else if (data[0] == 0x4D && data[1] == 0x4D && data[2] == 0x00 && data[3] == 0x2A) {
        uint32_t ifd_offset = get_unaligned_be32(data + 4);
        is_tiff = (ifd_offset >= 8 && ifd_offset < len);
    }

    return is_tiff ? MEDIA_DTYPE_PIC_TIFF : MEDIA_DTYPE_MAX;
}

static int32_t detect_svg(const uint8_t *data, uint32_t len)
{
    if (len >= 5) {
        if (check_signature_ignore_whitespace(data, len, "<?xml", 5) ||
            check_signature_ignore_whitespace(data, len, "<svg", 4)) {
            return MEDIA_DTYPE_PIC_SVG;
        }
    }
    return MEDIA_DTYPE_MAX;
}

// ==================== 音频格式检测函数 ====================
static int32_t detect_mp3(const uint8_t *data, uint32_t len)
{
    if (len < 10) return MEDIA_DTYPE_MAX;

    // ID3v2标签
    if (memcmp(data, "ID3", 3) == 0 && data[3] <= 0x04) {
        return MEDIA_DTYPE_AUDIO_MP3;
    }

    // MPEG音频帧
    if (data[0] == 0xFF && (data[1] & 0xE0) == 0xE0) {
        uint8_t version = (data[1] >> 3) & 0x03;
        uint8_t layer = (data[1] >> 1) & 0x03;
        uint8_t bitrate_index = (data[2] >> 4) & 0x0F;
        uint8_t freq_index = (data[2] >> 2) & 0x03;

        if (version != 0x01 && layer != 0x00 && bitrate_index != 0x0F && freq_index != 0x03) {
            return MEDIA_DTYPE_AUDIO_MP3;
        }
    }

    return MEDIA_DTYPE_MAX;
}

static int32_t detect_wav(const uint8_t *data, uint32_t len)
{
    if (len >= 16 && memcmp(data, "RIFF", 4) == 0 && memcmp(data + 8, "WAVE", 4) == 0) {
        if (memcmp(data + 12, "fmt ", 4) == 0) {
            return MEDIA_DTYPE_AUDIO_WAV;
        }
    }
    return MEDIA_DTYPE_MAX;
}

static int32_t detect_flac(const uint8_t *data, uint32_t len)
{
    return CHECK_SIGNATURE(data, len, "fLaC", 4) ?
           MEDIA_DTYPE_AUDIO_FLAC : MEDIA_DTYPE_MAX;
}

static int32_t detect_aac(const uint8_t *data, uint32_t len)
{
    // 修复AAC检测逻辑，仅校验同步字
    if (len >= 7 && data[0] == 0xFF && (data[1] & 0xF0) == 0xF0) {
        uint8_t freq_idx = (data[2] >> 2) & 0x0F;
        if (freq_idx <= 0x0C) { // 有效的频率索引
            return MEDIA_DTYPE_AUDIO_AAC;
        }
    }
    return MEDIA_DTYPE_MAX;
}

static int32_t detect_ogg(const uint8_t *data, uint32_t len)
{
    if (len >= 5 && memcmp(data, "OggS", 4) == 0 && data[4] == 0x00) {
        return MEDIA_DTYPE_AUDIO_OGG;
    }
    return MEDIA_DTYPE_MAX;
}

static int32_t detect_ape(const uint8_t *data, uint32_t len)
{
    if (len >= 8 && memcmp(data, "APE ", 4) == 0) {
        uint16_t version = get_unaligned_le16(data + 4);
        if (version == 0x0F8C || version == 0x0F96) {
            return MEDIA_DTYPE_AUDIO_APE;
        }
    }
    return MEDIA_DTYPE_MAX;
}

// ==================== 视频格式检测函数 ====================
static int32_t detect_flv(const uint8_t *data, uint32_t len)
{
    if (len >= FLV_HEADER_SIZE && memcmp(data, "FLV", 3) == 0 && data[3] == 0x01) {
        if ((data[4] & 0x01) || (data[4] & 0x04)) {
            return MEDIA_DTYPE_VIDEO_FLV;
        }
    }
    return MEDIA_DTYPE_MAX;
}

static int32_t detect_avi(const uint8_t *data, uint32_t len)
{
    if (len >= 20 && memcmp(data, "RIFF", 4) == 0 && memcmp(data + 8, "AVI ", 4) == 0) {
        if (memcmp(data + 12, "hdrl", 4) == 0) {
            return MEDIA_DTYPE_VIDEO_AVI;
        }
    }
    return MEDIA_DTYPE_MAX;
}

static int32_t detect_ts(const uint8_t *data, uint32_t len)
{
    return check_packet_structure(data, len, TS_PACKET_SIZE, 3) ?
           MEDIA_DTYPE_VIDEO_TS : MEDIA_DTYPE_MAX;
}

static int32_t detect_m2ts(const uint8_t *data, uint32_t len)
{
    return check_packet_structure(data, len, M2TS_PACKET_SIZE, 3) ?
           MEDIA_DTYPE_VIDEO_M2TS : MEDIA_DTYPE_MAX;
}

static int32_t detect_mkv_webm(const uint8_t *data, uint32_t len)
{
    static const uint8_t ebml_sig[] = {0x1A, 0x45, 0xDF, 0xA3};

    if (!CHECK_SIGNATURE(data, len, ebml_sig, sizeof(ebml_sig))) {
        return MEDIA_DTYPE_MAX;
    }

    // 修复：简化的MKV/WebM检测，避免复杂的EBML解析
    const uint8_t *pos = data + 4; // 跳过EBML签名
    uint32_t remaining = len - 4;
    
    // 在数据中查找"matroska"或"webm"字符串
    if (find_pattern(pos, remaining, (uint8_t*)"matroska", 8)) {
        return MEDIA_DTYPE_VIDEO_MKV;
    }
    if (find_pattern(pos, remaining, (uint8_t*)"webm", 4)) {
        return MEDIA_DTYPE_VIDEO_WEBM; // 修复常量名称
    }
    
    // 如果找不到明确标记，但EBML签名正确，默认返回MKV
    return MEDIA_DTYPE_VIDEO_MKV;
}

static int32_t detect_wmv_wma(const uint8_t *data, uint32_t len)
{
    static const uint8_t asf_sig[] = {0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11,
                                      0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C
                                     };

    if (!CHECK_SIGNATURE(data, len, asf_sig, sizeof(asf_sig)) || len < 50) {
        return MEDIA_DTYPE_MAX;
    }

    // 在ASF数据对象中查找媒体类型
    static const uint8_t video_marker[] = {0x91, 0x07, 0xDC, 0xB7, 0xB7, 0xA9, 0xCF, 0x11,
                                          0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65}; // Video Media
    
    static const uint8_t audio_marker[] = {0x91, 0x07, 0xDC, 0xB7, 0xB7, 0xA9, 0xCF, 0x11,
                                          0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x66}; // Audio Media

    if (find_pattern(data + 30, len - 30, video_marker, sizeof(video_marker))) {
        return MEDIA_DTYPE_VIDEO_WMV;
    } else if (find_pattern(data + 30, len - 30, audio_marker, sizeof(audio_marker))) {
        return MEDIA_DTYPE_AUDIO_WMA;
    }

    return MEDIA_DTYPE_MAX;
}

// ==================== MOV格式检测函数 ====================
static int32_t detect_mov(const uint8_t *data, uint32_t len)
{
    if (len < 12) return MEDIA_DTYPE_MAX;
    
    // 检查ftyp中的qt品牌
    if (memcmp(data + 4, "ftyp", 4) == 0) {
        if (len >= 16 && memcmp(data + 8, "qt  ", 4) == 0) {
            return MEDIA_DTYPE_VIDEO_MOV;
        }
    }
    
    // 检查moov盒子（MOV文件通常moov在开头）
    if (memcmp(data + 4, "moov", 4) == 0) {
        return MEDIA_DTYPE_VIDEO_MOV;
    }
    
    return MEDIA_DTYPE_MAX;
}

// ==================== FTYP格式检测函数 ====================
static int32_t detect_ftyp_format(const uint8_t *data, uint32_t len)
{
    if (len < 8) return MEDIA_DTYPE_MAX;

    // 修复FTYP大小判断逻辑，允许最小8字节
    uint32_t ftyp_size = get_unaligned_be32(data);
    
    // 更严格的FTYP大小验证
    if (ftyp_size < 8 || ftyp_size > len || ftyp_size > 1024 * 1024) {
        return MEDIA_DTYPE_MAX; // 防止异常大的ftyp盒子
    }
    
    if (memcmp(data + 4, "ftyp", 4) != 0) {
        return MEDIA_DTYPE_MAX;
    }

    // 修复：ftyp大小等于8是有效的（只有大小和类型）
    if (ftyp_size == 8) {
        return MEDIA_DTYPE_VIDEO_MP4; // 最基本的MP4格式
    }

    // HEIF/HEIC品牌
    static const char *heif_brands[] = {"heic", "heix", "hevc", "hevx", "mif1", "msf1", NULL};
    for (int i = 0; heif_brands[i]; i++) {
        if (memcmp(data + 8, heif_brands[i], 4) == 0) {
            return MEDIA_DTYPE_PIC_HEIF;
        }
    }

    // M4A音频品牌
    static const char *m4a_brands[] = {"M4A ", "mp4a", "aac ", NULL};
    for (int i = 0; m4a_brands[i]; i++) {
        if (memcmp(data + 8, m4a_brands[i], 4) == 0) {
            return MEDIA_DTYPE_AUDIO_M4A;
        }
    }

    // MOV品牌
    if (memcmp(data + 8, "qt  ", 4) == 0) {
        return MEDIA_DTYPE_VIDEO_MOV;
    }

    // MP4视频品牌
    static const char *mp4_brands[] = {"isom", "mp41", "mp42", "avc1", "hev1", NULL};
    for (int i = 0; mp4_brands[i]; i++) {
        if (memcmp(data + 8, mp4_brands[i], 4) == 0) {
            return MEDIA_DTYPE_VIDEO_MP4;
        }
    }

    return MEDIA_DTYPE_MAX;
}

int32_t txmplayer_detect_mtype(const uint8_t *data, uint32_t len)
{
    int32_t result;

    if (data == NULL || len < MIN_DETECTION_LENGTH) {
        return MEDIA_DTYPE_MAX;
    }

    // -------------------------- 有明确签名的容器格式检测 --------------------------
    if ((result = detect_ftyp_format(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_mp4(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_mov(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_flv(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_avi(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_mkv_webm(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_ts(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_m2ts(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_wmv_wma(data, len)) != MEDIA_DTYPE_MAX) return result;

    // -------------------------- 常见图片格式检测 --------------------------
    if ((result = detect_png(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_jpg(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_gif(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_bmp(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_webp(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_tiff(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_svg(data, len)) != MEDIA_DTYPE_MAX) return result;

    // -------------------------- 常见音频格式检测 --------------------------
    if ((result = detect_mp3(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_wav(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_flac(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_aac(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_ogg(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_ape(data, len)) != MEDIA_DTYPE_MAX) return result;

    // -------------------------- 裸视频编码帧检测 --------------------------
    // 注意：裸视频检测放在最后，因为它们可能与其他格式有误判
    // 同时要求找到多个特征才确认，减少误判
    if ((result = detect_h264_raw(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_h265_raw(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_av1_raw(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_vp9_raw(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_vp8_raw(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_mpeg2_raw(data, len)) != MEDIA_DTYPE_MAX) return result;
    if ((result = detect_mpeg4_raw(data, len)) != MEDIA_DTYPE_MAX) return result;

    return MEDIA_DTYPE_MAX;
}

