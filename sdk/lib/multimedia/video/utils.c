#include "basic_include.h"
#include "lib/multimedia/framebuff.h"

/**
 * @brief 获取像素格式的近似每像素字节数（仅用于粗略估算）
 *
 * 注意：对于平面格式，返回的是平均每个像素占用的字节数（如 YUV420P 返回 1.5，
 * 此处返回整数乘以 10 的值以避免浮点，例如 YUV420P 返回 15）。
 * 实际内存分配请使用 image_pix_fmt_get_buffer_size()。
 */
int image_pix_fmt_get_bpp_x10(image_pixel_format_t fmt)
{
    switch (fmt) {
        case IMAGE_PIX_FMT_RGB24:
        case IMAGE_PIX_FMT_BGR24:
            return 30;
        case IMAGE_PIX_FMT_RGBA8888:
        case IMAGE_PIX_FMT_BGRA8888:
            return 40;
        case IMAGE_PIX_FMT_RGB565:
        case IMAGE_PIX_FMT_RGB555:
            return 20;
        case IMAGE_PIX_FMT_GRAY8:
        case IMAGE_PIX_FMT_PAL8:
            return 10;
        case IMAGE_PIX_FMT_YUV420P:
        case IMAGE_PIX_FMT_NV12:
        case IMAGE_PIX_FMT_NV21:
            return 15; // 1.5
        case IMAGE_PIX_FMT_YUV422P:
        case IMAGE_PIX_FMT_NV16:
            return 20; // 2.0
        case IMAGE_PIX_FMT_YUV444P:
            return 30; // 3.0
        default:
            return 0; // 未知或高位深需特殊计算
    }
}

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
                                     uint32_t align)
{
    if (width == 0 || height == 0) {
        return 0;
    }
    if (align == 0) {
        align = 1;
    }

    // 平面格式单独处理，简单起见仅处理常用格式，完整实现可参考实际需求
    switch (fmt) {
        case IMAGE_PIX_FMT_YUV420P: {
            uint32_t y_stride = (width + align - 1) & ~(align - 1);
            uint32_t uv_stride = ((width + 1) / 2 + align - 1) & ~(align - 1);
            return y_stride * height + uv_stride * (height / 2) * 2;
        }
        case IMAGE_PIX_FMT_NV12:
        case IMAGE_PIX_FMT_NV21: {
            uint32_t y_stride = (width + align - 1) & ~(align - 1);
            uint32_t uv_stride = ((width + 1) / 2 * 2 + align - 1) & ~(align - 1);
            return y_stride * height + uv_stride * (height / 2);
        }
        case IMAGE_PIX_FMT_RGB24:
        case IMAGE_PIX_FMT_BGR24: {
            uint32_t stride = (width * 3 + align - 1) & ~(align - 1);
            return stride * height;
        }
        case IMAGE_PIX_FMT_RGBA8888:
        case IMAGE_PIX_FMT_BGRA8888: {
            uint32_t stride = (width * 4 + align - 1) & ~(align - 1);
            return stride * height;
        }
        default:
            return 0;
    }
}

/**
 * @brief 解析 H.264 AVCC 格式的 extradata
 *
 * @param extradata   AVCC 配置数据（ISO/IEC 14496-15 定义的 AVCDecoderConfigurationRecord）
 * @param extradata_size 数据大小（至少 7 字节）
 * @param out         输出结构，成功时填充
 * @return 0 成功，-1 失败（格式错误或数据不足）
 */
int video_parse_h264_avcc(const uint8_t *extradata, size_t extradata_size, h264_avcc_info_t *out)
{
    if (!extradata || extradata_size < 7 || !out) {
        return -1;
    }

    const uint8_t *p = extradata;
    uint8_t version = p[0];
    if (version != 1) { // 目前仅支持 version 1
        return -1;
    }

    out->profile = p[1];
    out->level = p[3];

    // 第5字节：高6位保留，低2位为 lengthSizeMinusOne
    uint8_t len_minus_one = p[4] & 0x03;
    out->length_size = len_minus_one + 1;

    // 第6字节：高5位为 numOfSequenceParameterSets，低3位保留
    uint8_t num_sps = (p[5] >> 3) & 0x1F;
    if (num_sps == 0) { // 至少需要一个 SPS
        return -1;
    }

    // 跳过前6字节，开始读取 SPS 列表
    p += 6;
    size_t remaining = extradata_size - 6;

    // 读取第一个 SPS 的长度
    if (remaining < 2) {
        return -1;
    }

    uint16_t sps_len = (p[0] << 8) | p[1];
    p += 2;
    remaining -= 2;
    if (sps_len > remaining) {
        return -1;
    }

    out->sps_data = p;
    out->sps_size = sps_len;
    p += sps_len;
    remaining -= sps_len;

    // 读取 numOfPictureParameterSets
    if (remaining < 1) {
        return -1;
    }

    uint8_t num_pps = p[0];
    p += 1;
    remaining -= 1;

    if (num_pps == 0) {// 至少需要一个 PPS
        return -1;
    }

    // 读取第一个 PPS 的长度
    if (remaining < 2) {
        return -1;
    }

    uint16_t pps_len = (p[0] << 8) | p[1];
    p += 2;
    remaining -= 2;
    if (pps_len > remaining) {
        return -1;
    }

    out->pps_data = p;
    out->pps_size = pps_len;

    return 0;
}

/**
 * @brief 解析 H.265 HVCC 格式的 extradata
 *
 * @param extradata   HVCC 配置数据（ISO/IEC 14496-15 定义的 HEVCDecoderConfigurationRecord）
 * @param extradata_size 数据大小（至少 23 字节）
 * @param out         输出结构，成功时填充
 * @return 0 成功，-1 失败
 */
int video_parse_h265_hvcc(const uint8_t *extradata, size_t extradata_size, h265_hvcc_info_t *out)
{
    if (!extradata || extradata_size < 23 || !out) {
        return -1;
    }

    const uint8_t *p = extradata;
    uint8_t version = p[0];
    if (version != 1) {
        return -1;
    }

    // 第1字节：版本，第2-4字节：general_profile_space等，这里跳过
    // 第21字节：lengthSizeMinusOne（位于 configurationVersion 之后固定偏移）
    // 实际结构复杂，我们只关心第21字节（偏移20）包含 lengthSizeMinusOne
    // 完整偏移参考 ISO/IEC 14496-15:2017
    if (extradata_size < 22) {
        return -1;
    }

    uint8_t len_minus_one = extradata[21] & 0x03;
    out->length_size = len_minus_one + 1;

    // 跳过固定头部，定位到数组区域
    // HVCC 结构（简化）：
    // 0: version(1)
    // 1-20: 其他字段，最后第21字节是 lengthSizeMinusOne
    // 第22字节：numOfArrays
    if (extradata_size < 23) {
        return -1;
    }

    uint8_t num_arrays = extradata[22];
    out->num_arrays = num_arrays;

    // 指针指向数组开始（偏移23）
    p = extradata + 23;
    size_t remaining = extradata_size - 23;

    // 遍历所有数组，提取 VPS、SPS、PPS 的第一个 NAL
    int found_vps = 0, found_sps = 0, found_pps = 0;
    for (int i = 0; i < num_arrays && remaining > 0; i++) {
        if (remaining < 3) {
            return -1;
        }

        uint8_t array_type = p[0] & 0x3F; // 低6位为 NAL 单元类型
        uint8_t num_nalus = p[1];         // 该数组中的 NAL 数量
        p += 2;
        remaining -= 2;

        for (int j = 0; j < num_nalus && remaining > 0; j++) {
            if (remaining < 2) {
                return -1;
            }

            uint16_t nalu_len = (p[0] << 8) | p[1];
            p += 2;
            remaining -= 2;
            if (nalu_len > remaining) {
                return -1;
            }

            if (!found_vps && array_type == 32) { // VPS
                out->vps_data = p;
                out->vps_size = nalu_len;
                found_vps = 1;
            } else if (!found_sps && array_type == 33) { // SPS
                out->sps_data = p;
                out->sps_size = nalu_len;
                found_sps = 1;
            } else if (!found_pps && array_type == 34) { // PPS
                out->pps_data = p;
                out->pps_size = nalu_len;
                found_pps = 1;
            }

            p += nalu_len;
            remaining -= nalu_len;
        }
    }

    if (!found_vps || !found_sps || !found_pps) {
        return -1;
    }

    return 0;
}