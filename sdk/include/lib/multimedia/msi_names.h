#ifndef _TXSEMI_MSI_NAMES_H_
#define _TXSEMI_MSI_NAMES_H_

/**
 * @file tx_semi_msi_names.h
 * @brief TXSDK 支持的 MSI 组件名称宏定义
 *
 * 本文件定义了所有 MSI 组件的标准名称，代码中必须使用这些宏，禁止直接写死字符串。
 *
 * MSI 组件分为两类：
 * - 动态组件：可存在多个实例，实例对象名称不固定。使用 msi_find2() 自动创建实例，msi_put() 销毁。
 * - 静态组件：全局仅存在一个实例，实例对象名称固定。
 */


#define TXMPLAYER_MSI   "txmplayer"     ///< 播放器模块（动态组件）
#define MP3DEC_MSI      "mp3dec"        ///< MP3 解码器（动态组件）
#define AACDEC_MSI      "aacdec"        ///< AAC 解码器（动态组件）
#define AACENC_MSI      "aacenc"        ///< AAC 编码器（动态组件）
#define ALAWDEC_MSI     "alawdec"       ///< A-law 解码器（动态组件）
#define ALAWENC_MSI     "alawenc"       ///< A-law 编码器（动态组件）
#define AMRNBDEC_MSI    "amrnbdec"      ///< AMR-NB 解码器（动态组件）
#define AMRWBDEC_MSI    "amrwbdec"      ///< AMR-WB 解码器（动态组件）
#define OPUSDEC_MSI     "opusdec"       ///< Opus 解码器（动态组件）
#define OPUSENC_MSI     "opusenc"       ///< Opus 编码器（动态组件）
#define PCMDEC_MSI      "pcmdec"        ///< PCM 解码器（动态组件）
#define ULAWDEC_MSI     "ulawdec"       ///< μ-law 解码器（动态组件）
#define ULAWENC_MSI     "ulawenc"       ///< μ-law 编码器（动态组件）
#define MIXER_MSI       "audio_mixer"   ///< 音频混音器（动态组件）


#define H264DEC_MSI     "h264dec"       ///< H.264 解码器（动态组件）
#define MJPEGDEC_MSI    "mjpegdec"      ///< MJPEG 解码器（动态组件）
#define VDD_MSI         "vdd_msi"       ///< 虚拟显示设备（动态组件）


#define DACMSG_MSI      "dac_msg"       ///< DAC 辅助模块，用于获取 DAC 输出回采数据（静态组件）

#endif /* _TXSEMI_MSI_NAMES_H_ */
