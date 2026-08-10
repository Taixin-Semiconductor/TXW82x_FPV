#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/txmplayer.h"

// 常量定义 - 优化最小检测长度，适配各格式最小需求
#define MIN_DETECTION_LENGTH 8
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

// 辅助函数：忽略空白字符检查签名（大小写不敏感）
static uint8 check_signature_ignore_whitespace(const uint8 *data, uint32 len, const char *sig, uint32 sig_len)
{
    if (len < sig_len) return 0;
    uint32 data_pos = 0;
    
    // 跳过前置空白字符
    while (data_pos < len && (data[data_pos] == ' ' || data[data_pos] == '\t' || data[data_pos] == '\r' || data[data_pos] == '\n')) {
        data_pos++;
    }
    if (len - data_pos < sig_len) return 0;
    
    // 大小写不敏感比较
    for (uint32 i = 0; i < sig_len; i++) {
        uint8 c1 = data[data_pos + i];
        uint8 c2 = (uint8)sig[i];
        if (c1 != c2 && c1 != (c2 ^ 0x20)) {
            return 0;
        }
    }
    return 1;
}

// 辅助函数：查找二进制模式
static const uint8 *find_pattern(const uint8 *haystack, uint32 haystack_len, const uint8 *needle, uint32 needle_len)
{
    if (haystack_len < needle_len || needle_len == 0) return NULL;
    for (uint32 i = 0; i <= haystack_len - needle_len; i++) {
        if (memcmp(haystack + i, needle, needle_len) == 0) {
            return haystack + i;
        }
    }
    return NULL;
}

// 辅助函数：检查TS/M2TS包结构（增强版）
static uint8 check_packet_structure(const uint8 *data, uint32 len, uint32 packet_size, uint32 min_packets)
{
    if (len < packet_size * min_packets) return 0;
    uint32 valid_packets = 0;
    
    for (uint32 i = 0; i < min_packets && i * packet_size < len; i++) {
        const uint8 *packet = data + i * packet_size;
        if (packet[0] != 0x47) {
            continue;
        }
        
        // 增强TS包检测：排除空包（PID=0x1FFF）
        if (packet_size == TS_PACKET_SIZE) {
            uint16_t pid = ((packet[1] & 0x1F) << 8) | packet[2];
            if ((pid & 0x1FFF) == 0x1FFF) {
                continue;
            }
        }
        valid_packets++;
    }
    return valid_packets >= min_packets;
}

// ==================== MP4格式检测函数 ====================
static media_container_type_t detect_mp4(const uint8 *data, uint32 len)
{
    if (len < 8) return MEDIA_CONTAINER_INVALID;
    
    // 检查MP4特征盒子（处理box_size=0的扩展型盒子）
    size_t offset = 0;
    while (offset + 8 <= len && offset < 1024) { // 只检查前1KB
        uint32 box_size = get_unaligned_be32(data + offset);
        uint32 box_type = get_unaligned_be32(data + offset + 4);
        
        // 处理扩展型box（size=0表示到文件末尾）
        if (box_size == 0) {
            box_size = len - offset;
        }
        // 安全检查：防止box_size越界
        if (box_size < 8 || box_size > len - offset) {
            break;
        }
        
        // MP4关键盒子类型
        switch (box_type) {
            case 0x66747970: // "ftyp"
            case 0x6D6F6F76: // "moov"
            case 0x6D6F6F66: // "moof" - fMP4
            case 0x6D646174: // "mdat"
                return MEDIA_CONTAINER_MP4;
        }
        offset += box_size;
    }
    return MEDIA_CONTAINER_INVALID;
}

// ==================== 裸视频编码帧检测函数 ====================
static media_container_type_t detect_h264_raw(const uint8 *data, uint32 len)
{
    if (len < 16) return MEDIA_CONTAINER_INVALID;
    
    uint32 start_code_count = 0;
    uint32 sps_count = 0;
    uint32 pps_count = 0;
    
    // H.264 NAL单元通常以0x000001或0x00000001开头
    for (uint32 i = 0; i <= len - 4; i++) {
        uint8 found_start_code = 0;
        uint32 nal_offset = 0;
        
        // 检查3字节起始码 0x000001
        if (i <= len - 3 && data[i] == 0x00 && data[i + 1] == 0x00 && data[i + 2] == 0x01) {
            nal_offset = i + 3;
            found_start_code = 1;
        }
        // 检查4字节起始码 0x00000001
        else if (i <= len - 4 && data[i] == 0x00 && data[i + 1] == 0x00 && data[i + 2] == 0x00 && data[i + 3] == 0x01) {
            nal_offset = i + 4;
            found_start_code = 1;
        }
        
        if (found_start_code && nal_offset < len) {
            uint8 nal_unit_type = data[nal_offset] & 0x1F;
            // 检查禁止位（bit7）必须为0
            if ((data[nal_offset] & 0x80) != 0) {
                continue;
            }
            
            // 统计关键NAL单元
            switch (nal_unit_type) {
                case H264_NAL_UNIT_TYPE_SPS: sps_count++; break;
                case H264_NAL_UNIT_TYPE_PPS: pps_count++; break;
                case H264_NAL_UNIT_TYPE_IDR:
                case H264_NAL_UNIT_TYPE_NON_IDR: start_code_count++; break;
            }
            
            // 更严格的条件：需要SPS+PPS+帧数据
            if (sps_count >= 1 && pps_count >= 1 && start_code_count >= 2) {
                return MEDIA_CONTAINER_RAW_H264;
            }
        }
    }
    return MEDIA_CONTAINER_INVALID;
}

static media_container_type_t detect_h265_raw(const uint8 *data, uint32 len)
{
    if (len < 16) return MEDIA_CONTAINER_INVALID;
    
    uint32 start_code_count = 0;
    uint32 vps_count = 0;
    uint32 sps_count = 0;
    uint32 pps_count = 0;
    
    // H.265 NAL单元也以0x000001或0x00000001开头
    for (uint32 i = 0; i <= len - 4; i++) {
        uint8 found_start_code = 0;
        uint32 nal_offset = 0;
        
        // 检查3字节起始码 0x000001
        if (i <= len - 3 && data[i] == 0x00 && data[i + 1] == 0x00 && data[i + 2] == 0x01) {
            nal_offset = i + 3;
            found_start_code = 1;
        }
        // 检查4字节起始码 0x00000001
        else if (i <= len - 4 && data[i] == 0x00 && data[i + 1] == 0x00 && data[i + 2] == 0x00 && data[i + 3] == 0x01) {
            nal_offset = i + 4;
            found_start_code = 1;
        }
        
        if (found_start_code && nal_offset < len) {
            uint8 nal_unit_type = (data[nal_offset] >> 1) & 0x3F;
            // 检查禁止位（bit7）必须为0
            if ((data[nal_offset] & 0x80) != 0) {
                continue;
            }
            
            // 统计关键NAL单元
            switch (nal_unit_type) {
                case H265_NAL_UNIT_TYPE_VPS: vps_count++; break;
                case H265_NAL_UNIT_TYPE_SPS: sps_count++; break;
                case H265_NAL_UNIT_TYPE_PPS: pps_count++; break;
                case H265_NAL_UNIT_TYPE_IDR:
                case H265_NAL_UNIT_TYPE_NON_IDR: start_code_count++; break;
            }
            
            // 更严格的条件：需要VPS+SPS+PPS+帧数据
            if (vps_count >= 1 && sps_count >= 1 && pps_count >= 1 && start_code_count >= 2) {
                return MEDIA_CONTAINER_RAW_H265;
            }
        }
    }
    return MEDIA_CONTAINER_INVALID;
}

static media_container_type_t detect_av1_raw(const uint8 *data, uint32 len)
{
    if (len < 8) return MEDIA_CONTAINER_INVALID;
    
    uint32 valid_obu_count = 0;
    // AV1帧通常以OBU（Open Bitstream Unit）开始
    for (uint32 i = 0; i <= len - 4; i++) { // 增加最小长度检查
        uint8 obu_header = data[i];
        // 检查OBU头格式：bit4-7必须为0
        if ((obu_header & 0xF0) != 0x00) {
            continue;
        }
        
        uint8 obu_type = obu_header & 0x0F;
        // 解析OBU长度（简化版，验证基本完整性）
        uint32 obu_size = 0;
        uint8 size_bytes = 1;
        uint8 size_bit = (data[i+1] >> 7) & 0x01;
        
        if (size_bit) {
            size_bytes++;
            if (i + size_bytes >= len) continue;
            obu_size = ((data[i+1] & 0x7F) << 8) | data[i+2];
        } else {
            obu_size = data[i+1] & 0x7F;
        }
        
        // 验证OBU类型和长度合理性
        if ((obu_type == AV1_OBU_TYPE_SEQUENCE_HEADER || 
             obu_type == AV1_OBU_TYPE_FRAME_HEADER || 
             obu_type == AV1_OBU_TYPE_FRAME || 
             obu_type == AV1_OBU_TYPE_TILE_GROUP) && 
            obu_size > 0 && obu_size < len - i) {
            valid_obu_count++;
            if (valid_obu_count >= 3) {
                return MEDIA_CONTAINER_RAW_AV1;
            }
        }
    }
    return MEDIA_CONTAINER_INVALID;
}

static media_container_type_t detect_vp9_raw(const uint8 *data, uint32 len)
{
    if (len < 8) return MEDIA_CONTAINER_INVALID;
    
    uint32 frame_count = 0;
    // VP9帧头检测（增强版）
    for (uint32 i = 0; i <= len - 3; i++) {
        // VP9帧头特征：bit7=保留位必须为0，bit4~6=版本号(0-4)
        if ((data[i] & 0x80) == 0x00) { 
            uint8 version = (data[i] >> 4) & 0x07;
            uint8 frame_marker = (data[i] & 0x01); // 帧开始标记
            
            // 有效的VP9版本+帧开始标记
            if (version <= 4 && frame_marker == 1) { 
                frame_count++;
                if (frame_count >= 3) {
                    return MEDIA_CONTAINER_RAW_VP9;
                }
            }
        }
    }
    return MEDIA_CONTAINER_INVALID;
}

static media_container_type_t detect_vp8_raw(const uint8 *data, uint32 len)
{
    if (len < 8) return MEDIA_CONTAINER_INVALID;
    
    uint32 frame_count = 0;
    // VP8帧头检测（增强版）
    for (uint32 i = 0; i <= len - 6; i++) {
        // VP8帧头特征：bit4~7=0x9，bit0~2=版本(0-3)
        if ((data[i] & 0xF8) == 0x90) { 
            uint8 version = data[i] & 0x07;
            uint8 key_frame = (data[i + 3] & 0x01) == 0; // 关键帧标记
            
            // 有效的VP8版本+关键帧标记
            if (version <= 3 && key_frame) { 
                frame_count++;
                if (frame_count >= 3) {
                    return MEDIA_CONTAINER_RAW_VP8;
                }
            }
        }
    }
    return MEDIA_CONTAINER_INVALID;
}

// ==================== 图片格式检测函数 ====================
static media_container_type_t detect_png(const uint8 *data, uint32 len)
{
    static const uint8 png_sig[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    return CHECK_SIGNATURE(data, len, png_sig, sizeof(png_sig)) ? MEDIA_CONTAINER_PNG : MEDIA_CONTAINER_INVALID;
}

static media_container_type_t detect_jpg(const uint8 *data, uint32 len)
{
    // 增强JPG检测：支持无APP0段的JPG
    if (len >= 2 && data[0] == 0xFF && data[1] == 0xD8) { // SOI标记
        // 检查APP0段或直接检查SOS/DQT段
        if (len >= 4) {
            if (data[2] == 0xFF && (data[3] & 0xF0) == 0xE0) { // APP0
                return MEDIA_CONTAINER_JPEG;
            }
            // 检查SOS(0xFFDA)或DQT(0xFFDB)
            for (uint32 i = 2; i < len - 1 && i < 100; i++) {
                if (data[i] == 0xFF && (data[i+1] == 0xDA || data[i+1] == 0xDB)) {
                    return MEDIA_CONTAINER_JPEG;
                }
            }
        }
    }
    return MEDIA_CONTAINER_INVALID;
}

static media_container_type_t detect_gif(const uint8 *data, uint32 len)
{
    if (len >= 6 && (memcmp(data, "GIF87a", 6) == 0 || memcmp(data, "GIF89a", 6) == 0)) {
        return MEDIA_CONTAINER_GIF;
    }
    return MEDIA_CONTAINER_INVALID;
}

static media_container_type_t detect_bmp(const uint8 *data, uint32 len)
{
    if (len >= BMP_HEADER_MIN_SIZE && data[0] == 0x42 && data[1] == 0x4D) { // BM标记
        uint32 file_size = get_unaligned_le32(data + 2);
        // 修复BMP大小判断：允许文件大小等于len，支持最小54字节的BMP
        if ((file_size == len || file_size > len) && len >= 54) { 
            // 验证位图信息头长度
            uint32 info_header_size = get_unaligned_le32(data + 14);
            if (info_header_size == 40 || info_header_size == 12) { // 标准BITMAPINFOHEADER
                return MEDIA_CONTAINER_BMP;
            }
        }
    }
    return MEDIA_CONTAINER_INVALID;
}

static media_container_type_t detect_webp(const uint8 *data, uint32 len)
{
    // 增强WebP检测：区分有损/无损，但统一返回WEBP类型
    if (len >= 16 && memcmp(data, "RIFF", 4) == 0 && memcmp(data + 8, "WEBP", 4) == 0) {
        uint32 riff_size = get_unaligned_le32(data + 4);
        if (riff_size > 8 && riff_size + 8 <= len) {
            // 检查VP8/VP8L/VP8X块（区分编码类型）
            if (memcmp(data + 12, "VP8 ", 4) == 0 || 
                memcmp(data + 12, "VP8L", 4) == 0 || 
                memcmp(data + 12, "VP8X", 4) == 0) {
                return MEDIA_CONTAINER_WEBP;
            }
        }
    }
    return MEDIA_CONTAINER_INVALID;
}

static media_container_type_t detect_tiff(const uint8 *data, uint32 len)
{
    if (len < TIFF_HEADER_SIZE) return MEDIA_CONTAINER_INVALID;
    
    uint8 is_tiff = 0;
    // 小端TIFF (II)
    if (data[0] == 0x49 && data[1] == 0x49 && data[2] == 0x2A && data[3] == 0x00) {
        uint32 ifd_offset = get_unaligned_le32(data + 4);
        is_tiff = (ifd_offset >= 8 && ifd_offset < len);
    }
    // 大端TIFF (MM)
    else if (data[0] == 0x4D && data[1] == 0x4D && data[2] == 0x00 && data[3] == 0x2A) {
        uint32 ifd_offset = get_unaligned_be32(data + 4);
        is_tiff = (ifd_offset >= 8 && ifd_offset < len);
    }
    return is_tiff ? MEDIA_CONTAINER_TIFF : MEDIA_CONTAINER_INVALID;
}

static media_container_type_t detect_svg(const uint8 *data, uint32 len)
{
    if (len < 5) return MEDIA_CONTAINER_INVALID;
    
    // 增强SVG检测：支持XML命名空间，忽略注释
    if (check_signature_ignore_whitespace(data, len, "<?xml", 5) || 
        check_signature_ignore_whitespace(data, len, "<svg", 4)) {
        // 验证SVG命名空间（可选增强）
        const uint8 *svg_ns = find_pattern(data, len, (uint8*)"xmlns=\"http://www.w3.org/2000/svg\"", 33);
        if (svg_ns != NULL || check_signature_ignore_whitespace(data, len, "<svg", 4)) {
            return MEDIA_CONTAINER_SVG;
        }
    }
    return MEDIA_CONTAINER_INVALID;
}

// ==================== 音频格式检测函数 ====================
static media_container_type_t detect_mp3(const uint8 *data, uint32 len)
{
    if (len < 10) return MEDIA_CONTAINER_INVALID;
    // ID3v2标签
    if (memcmp(data, "ID3", 3) == 0 && data[3] <= 0x04) {
        return MEDIA_CONTAINER_MP3;
    }
    // MPEG音频帧
    if (data[0] == 0xFF && (data[1] & 0xE0) == 0xE0) {
        uint8 version = (data[1] >> 3) & 0x03;
        uint8 layer = (data[1] >> 1) & 0x03;
        uint8 bitrate_index = (data[2] >> 4) & 0x0F;
        uint8 freq_index = (data[2] >> 2) & 0x03;
        if (version != 0x01 && layer != 0x00 && bitrate_index != 0x0F && freq_index != 0x03) {
            return MEDIA_CONTAINER_MP3;
        }
    }
    return MEDIA_CONTAINER_INVALID;
}

static media_container_type_t detect_wav(const uint8 *data, uint32 len)
{
    if (len >= 16 && memcmp(data, "RIFF", 4) == 0 && memcmp(data + 8, "WAVE", 4) == 0) {
        if (memcmp(data + 12, "fmt ", 4) == 0) {
            return MEDIA_CONTAINER_WAV;
        }
    }
    return MEDIA_CONTAINER_INVALID;
}

static media_container_type_t detect_flac(const uint8 *data, uint32 len)
{
    return CHECK_SIGNATURE(data, len, "fLaC", 4) ? MEDIA_CONTAINER_FLAC : MEDIA_CONTAINER_INVALID;
}

static media_container_type_t detect_aac(const uint8 *data, uint32 len)
{
    // 增强AAC检测：支持ADTS格式，验证更多字段
    if (len >= 7) {
        for (uint32 i = 0; i <= len - 7; i++) {
            if (data[i] == 0xFF && (data[i+1] & 0xF0) == 0xF0) { // ADTS同步字
                uint8 profile = (data[i+2] >> 3) & 0x1F;
                uint8 freq_idx = (data[i+2] >> 2) & 0x0F;
                uint8 chan_config = (data[i+3] >> 3) & 0x0F;
                
                // 验证有效参数
                if (profile <= 0x1F && freq_idx <= 0x0C && chan_config <= 0x0F) {
                    return MEDIA_CONTAINER_RAW_AAC;
                }
            }
        }
    }
    return MEDIA_CONTAINER_INVALID;
}

static media_container_type_t detect_ogg(const uint8 *data, uint32 len)
{
    // 增强OGG检测：验证页结构+编码类型
    if (len >= 5 && memcmp(data, "OggS", 4) == 0 && data[4] == 0x00) {
        // 检查Vorbis/Opus编码标记
        const uint8 *vorbis_tag = find_pattern(data, len, (uint8*)"vorbis", 6);
        const uint8 *opus_tag = find_pattern(data, len, (uint8*)"OpusHead", 8);
        if (vorbis_tag != NULL || opus_tag != NULL) {
            return MEDIA_CONTAINER_OGG;
        }
        return MEDIA_CONTAINER_OGG;
    }
    return MEDIA_CONTAINER_INVALID;
}

// 修复：返回值类型从int32改为media_container_type_t
static media_container_type_t detect_amr(const uint8 *data, uint32 len)
{
    if (len >= 6 && memcmp(data, "#!AMR\n", 6) == 0) {
        return MEDIA_CONTAINER_AMR;
    }
    
    if (len >= 9 && memcmp(data, "#!AMR-WB\n", 9) == 0) {
        return MEDIA_CONTAINER_AMR;
    }
    
    return MEDIA_CONTAINER_INVALID;
}

// ==================== 视频格式检测函数 ====================
static media_container_type_t detect_flv(const uint8 *data, uint32 len)
{
    if (len >= FLV_HEADER_SIZE && memcmp(data, "FLV", 3) == 0 && data[3] == 0x01) {
        // 检查音频/视频标记 + 验证第一个Tag（偏移9字节）
        if (((data[4] & 0x01) || (data[4] & 0x04)) && len >= 9) {
            uint32 tag_size = get_unaligned_be24(data + 5); // 修复：FLV Tag大小在偏移5-7字节
            if (tag_size > 0 && tag_size + 13 <= len) { // Tag头+数据+PrevTagSize
                return MEDIA_CONTAINER_FLV;
            }
        }
    }
    return MEDIA_CONTAINER_INVALID;
}

static media_container_type_t detect_avi(const uint8 *data, uint32 len)
{
    if (len >= 12 && memcmp(data, "RIFF", 4) == 0 && memcmp(data + 8, "AVI ", 4) == 0) {
            return MEDIA_CONTAINER_AVI;
        }
    
    return MEDIA_CONTAINER_INVALID;
}

static media_container_type_t detect_ts(const uint8 *data, uint32 len)
{
    return check_packet_structure(data, len, TS_PACKET_SIZE, 3) ? MEDIA_CONTAINER_TS : MEDIA_CONTAINER_INVALID;
}

static media_container_type_t detect_m2ts(const uint8 *data, uint32 len)
{
    return check_packet_structure(data, len, M2TS_PACKET_SIZE, 3) ? MEDIA_CONTAINER_M2TS : MEDIA_CONTAINER_INVALID;
}

static media_container_type_t detect_mkv_webm(const uint8 *data, uint32 len)
{
    static const uint8 ebml_sig[] = {0x1A, 0x45, 0xDF, 0xA3};
    if (!CHECK_SIGNATURE(data, len, ebml_sig, sizeof(ebml_sig))) {
        return MEDIA_CONTAINER_INVALID;
    }
    
    // 增强MKV/WebM检测：解析DocType（简化版）
    const uint8 *pos = data + 4;
    uint32 remaining = len - 4;
    
    // 查找DocType元素（0x4282）
    const uint8 *doctype_elem = find_pattern(pos, remaining, (uint8*)"\x42\x82", 2);
    if (doctype_elem != NULL) {
        uint32 doctype_offset = doctype_elem - data + 2;
        if (doctype_offset + 4 < len) {
            if (memcmp(data + doctype_offset, "matroska", 8) == 0) {
                return MEDIA_CONTAINER_MKV;
            }
            if (memcmp(data + doctype_offset, "webm", 4) == 0) {
                return MEDIA_CONTAINER_WEBM;
            }
        }
    }
    
    // 降级检测：字符串匹配
    if (find_pattern(pos, remaining, (uint8*)"matroska", 8)) {
        return MEDIA_CONTAINER_MKV;
    }
    if (find_pattern(pos, remaining, (uint8*)"webm", 4)) {
        return MEDIA_CONTAINER_WEBM;
    }
    
    // 默认返回MKV
    return MEDIA_CONTAINER_MKV;
}

static media_container_type_t detect_wmv_wma(const uint8 *data, uint32 len)
{
    // ASF 文件的全局文件头 GUID
    static const uint8 asf_header_guid[] = {
        0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11,
        0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C
    };

    // 检查ASF GUID
    if (len < sizeof(asf_header_guid) || 
        memcmp(data, asf_header_guid, sizeof(asf_header_guid)) != 0) {
        return MEDIA_CONTAINER_INVALID;
    }

    // 简单区分WMV（视频）和WMA（音频）：查找视频流标记
    const uint8 *video_stream = find_pattern(data, len, (uint8*)"video", 5);
    if (video_stream != NULL) {
        return MEDIA_CONTAINER_WMV;
    } else {
        return MEDIA_CONTAINER_WMA;
    }
}

// ==================== MOV格式检测函数 ====================
static media_container_type_t detect_mov(const uint8 *data, uint32 len)
{
    if (len < 8) return MEDIA_CONTAINER_INVALID; // 降低最小长度
    
    // 检查ftyp中的qt品牌
    if (memcmp(data + 4, "ftyp", 4) == 0) {
        if (len >= 16 && memcmp(data + 8, "qt ", 4) == 0) {
            return MEDIA_CONTAINER_MOV;
        }
    }
    
    // 检查moov/mdat盒子（MOV文件常见结构）
    size_t offset = 0;
    while (offset + 8 <= len && offset < 1024) {
        uint32 box_size = get_unaligned_be32(data + offset);
        uint32 box_type = get_unaligned_be32(data + offset + 4);
        
        if (box_size == 0) box_size = len - offset;
        if (box_size < 8 || box_size > len - offset) break;
        
        if (box_type == 0x6D6F6F76 || box_type == 0x6D646174) { // moov/mdat
            return MEDIA_CONTAINER_MOV;
        }
        offset += box_size;
    }
    
    return MEDIA_CONTAINER_INVALID;
}

// ==================== FTYP格式检测函数 ====================
static media_container_type_t detect_ftyp_format(const uint8 *data, uint32 len)
{
    if (len < 8) return MEDIA_CONTAINER_INVALID;
    
    uint32 ftyp_size = get_unaligned_be32(data);
    // 处理扩展型ftyp（size=0）
    if (ftyp_size == 0) {
        ftyp_size = len;
    }
    // 严格的FTYP大小验证
    if (ftyp_size < 8 || ftyp_size > len || ftyp_size > 1024 * 1024) {
        return MEDIA_CONTAINER_INVALID;
    }
    
    if (memcmp(data + 4, "ftyp", 4) != 0) {
        return MEDIA_CONTAINER_INVALID;
    }
    
    // 最小ftyp（仅size+type）
    if (ftyp_size == 8) {
        return MEDIA_CONTAINER_MP4;
    }
    
    // 检查品牌标识
    if (len < 12) return MEDIA_CONTAINER_INVALID;
    uint32 brand = get_unaligned_be32(data + 8);
    
    // HEIF/HEIC品牌
    static const uint32 heif_brands[] = {0x68656963, 0x68656978, 0x68657663, 0x68657678, 0x6D696631, 0x6D736631, 0};
    for (int i = 0; heif_brands[i]; i++) {
        if (brand == heif_brands[i]) {
            return MEDIA_CONTAINER_HEIF;
        }
    }
    
    // M4A音频品牌
    static const uint32 m4a_brands[] = {0x4D344120, 0x6D703461, 0x61616320, 0};
    for (int i = 0; m4a_brands[i]; i++) {
        if (brand == m4a_brands[i]) {
            return MEDIA_CONTAINER_M4A;
        }
    }
    
    // MOV品牌
    if (brand == 0x71742020) { // "qt "
        return MEDIA_CONTAINER_MOV;
    }
    
    // MP4视频品牌
    static const uint32 mp4_brands[] = {0x69736F6D, 0x6D703431, 0x6D703432, 0x61766331, 0x68657631, 0};
    for (int i = 0; mp4_brands[i]; i++) {
        if (brand == mp4_brands[i]) {
            return MEDIA_CONTAINER_MP4;
        }
    }
    
    return MEDIA_CONTAINER_INVALID;
}

// ==================== 主检测函数 ====================
media_container_type_t detect_container_type(const uint8 *data, uint32 len)
{
    media_container_type_t result;
    
    // 基础校验：空指针或长度不足
    if (data == NULL || len < MIN_DETECTION_LENGTH) {
        return MEDIA_CONTAINER_INVALID;
    }

    // -------------------------- 有明确签名的容器格式检测 --------------------------
    if ((result = detect_ftyp_format(data, len)) != MEDIA_CONTAINER_INVALID) return result;
    if ((result = detect_mp4(data, len)) != MEDIA_CONTAINER_INVALID) return result;
    if ((result = detect_mov(data, len)) != MEDIA_CONTAINER_INVALID) return result;
    if ((result = detect_flv(data, len)) != MEDIA_CONTAINER_INVALID) return result;
    if ((result = detect_avi(data, len)) != MEDIA_CONTAINER_INVALID) return result;
    if ((result = detect_ts(data, len)) != MEDIA_CONTAINER_INVALID) return result;
    if ((result = detect_m2ts(data, len)) != MEDIA_CONTAINER_INVALID) return result;
    if ((result = detect_mkv_webm(data, len)) != MEDIA_CONTAINER_INVALID) return result;
    if ((result = detect_wmv_wma(data, len)) != MEDIA_CONTAINER_INVALID) return result;

    // -------------------------- 图片格式检测 --------------------------
    if ((result = detect_png(data, len)) != MEDIA_CONTAINER_INVALID) return result;
    if ((result = detect_jpg(data, len)) != MEDIA_CONTAINER_INVALID) return result;
    if ((result = detect_gif(data, len)) != MEDIA_CONTAINER_INVALID) return result;
    if ((result = detect_bmp(data, len)) != MEDIA_CONTAINER_INVALID) return result;
    if ((result = detect_webp(data, len)) != MEDIA_CONTAINER_INVALID) return result;
    if ((result = detect_tiff(data, len)) != MEDIA_CONTAINER_INVALID) return result;
    if ((result = detect_svg(data, len)) != MEDIA_CONTAINER_INVALID) return result;

    // -------------------------- 音频格式检测 --------------------------
    if ((result = detect_wav(data, len)) != MEDIA_CONTAINER_INVALID) return result;  
    if ((result = detect_flac(data, len)) != MEDIA_CONTAINER_INVALID) return result;   
    if ((result = detect_amr(data, len)) != MEDIA_CONTAINER_INVALID) return result;   
    if ((result = detect_ogg(data, len)) != MEDIA_CONTAINER_INVALID) return result;   
    if ((result = detect_mp3(data, len)) != MEDIA_CONTAINER_INVALID) return result;    
    if ((result = detect_aac(data, len)) != MEDIA_CONTAINER_INVALID) return result;   

    // -------------------------- 裸视频编码检测 --------------------------
    if ((result = detect_h264_raw(data, len)) != MEDIA_CONTAINER_INVALID) return result;
    if ((result = detect_h265_raw(data, len)) != MEDIA_CONTAINER_INVALID) return result;
    if ((result = detect_av1_raw(data, len)) != MEDIA_CONTAINER_INVALID) return result;
    if ((result = detect_vp9_raw(data, len)) != MEDIA_CONTAINER_INVALID) return result;
    if ((result = detect_vp8_raw(data, len)) != MEDIA_CONTAINER_INVALID) return result;

    // 未识别格式
    return MEDIA_CONTAINER_INVALID;
}
