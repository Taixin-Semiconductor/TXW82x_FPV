#ifndef HOGPD_H
#define HOGPD_H

#include "hogpd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 以下为ABI契约常量(检测窗口/描述子维度, 与SVM模型绑定, 不可配置)。 */
#define HOGPD_WINDOW_WIDTH 64
#define HOGPD_WINDOW_HEIGHT 128
#define HOGPD_DESCRIPTOR_SIZE 3780

typedef int32 (*hogpd_fullscan_candidate_filter_t)(int32 x, int32 y,
                                                  int32 width, int32 height,
                                                  float score,
                                                  void *context);
// 填充默认运行配置(与历史编译宏默认值一致)。
void hogpd_config_default(hogpd_config_t *config);
// 设置库日志输出回调(NULL=静默)。库内统一经此输出，不直接使用printf/os_printf。
void hogpd_set_log(hogpd_log_fn_t fn);
// 开关调试类日志(关键日志不受此开关影响)。默认0。
void hogpd_set_log_verbose(int enable);
// 初始化HOGPD组件。config为NULL时使用默认配置。
int32 hogpd_init(const hogpd_config_t *config);
// 释放HOGPD组件。
void hogpd_deinit(void);
// 执行多尺度HOG检测。
int32 hogpd_detect(const uint8 *image, int32 width, int32 height,
                 const hogpd_detect_config_t *config,
                 hogpd_detection_t *results, int32 max_results);
// 执行全图扫描并返回首个检测结果。
int32 hogpd_detect_fullscan_first(const uint8 *image, int32 width, int32 height,
                                float threshold, int32 stride,
                                int32 *out_x, int32 *out_y,
                                int32 *out_w, int32 *out_h,
                                float *out_score, float *out_scale,
                                uint64 *out_time_us);
// 获取全图扫描优选尺度。
double hogpd_preferred_scale(void);
// 设置全图扫描尺度范围。
void hogpd_set_scale_range(double min_scale, double max_scale);
// 设置优选尺度扫描顺序。
void hogpd_set_preferred_scale_order_enabled(int32 enabled);
// 设置全图扫描时间预算。
void hogpd_set_budget_us(uint64 budget_us);
// 设置全图扫描区域。
void hogpd_set_fullscan_region(int32 x, int32 y, int32 width, int32 height);
// 设置全图候选过滤器。
void hogpd_set_fullscan_candidate_filter(
    hogpd_fullscan_candidate_filter_t filter, void *context);
// 获取最近一次全图TopK候选。
int32 hogpd_get_last_fullscan_topk(hogpd_detection_t *results,
                                 int32 max_results);
// 获取全图扫描进度。
int32 hogpd_get_fullscan_progress(hogpd_box_t *box, int32 *scale_q100);
// 计算HOG块缓存尺寸。
int32 hogpd_get_block_cache_size(int32 image_width, int32 image_height,
                               int32 *out_blocks_x, int32 *out_blocks_y,
                               int32 *out_cache_count);
// 计算Q8梯度图。
int32 hogpd_compute_gradient_q8(const uint16 *image_q8, int32 width, int32 height,
                              uint16 *grad_weight0_q8, uint16 *grad_weight1_q8,
                              uint8 *angle_bin0, uint8 *angle_bin1);
// 计算Q15块缓存。
int32 hogpd_compute_block_cache_q15(const uint16 *grad_weight0_q8,
                                  const uint16 *grad_weight1_q8,
                                  const uint8 *angle_bin0, const uint8 *angle_bin1,
                                  int32 width, int32 height,
                                  int16 *block_cache, int32 cache_count);
// 计算Q15缓存的SVM分数。
float hogpd_predict_cache_q15(const int16 *block_cache, int32 blocks_y,
                               int32 start_bx, int32 start_by);
// 初始化Tracker组件和状态。config为NULL时使用默认配置。
int32 hogpd_tracker_init(const hogpd_config_t *config);
// 释放Tracker组件。
void hogpd_tracker_deinit(void);
// 处理单通道Y帧。
int32 hogpd_tracker_process_y(const uint8 *y_plane, int32 width, int32 height,
                            hogpd_frame_result_t *result);
// 处理YUV420P帧。
int32 hogpd_tracker_process_yuv420p(const uint8 *yuv420p,
                                  int32 width, int32 height,
                                  hogpd_frame_result_t *result);

#ifdef __cplusplus
}
#endif

#endif
