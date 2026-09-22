#ifndef HOGPD_TYPES_H
#define HOGPD_TYPES_H

/* 基础类型复用代码库公共定义(typesdef.h -> uint8/uint16/uint32/int8/...
 * 与 RET_OK/RET_ERR)，不自建类型体系。 */
#include "typesdef.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HOGPD_SAD_CORE_BOX_MAX 3
#define HOGPD_FULLSCAN_TOPK_MAX 5

/* 日志输出回调(如os_printf)，NULL=库内日志静默。 */
typedef void (*hogpd_log_fn_t)(const char *fmt, ...);

/* 内存分配Hook：由应用层注入，在Hook内决定缓冲落在SRAM还是PSRAM。 */
typedef void *(*hogpd_alloc_fn_t)(uint32 bytes);
typedef void (*hogpd_free_fn_t)(void *ptr);

typedef struct {
    int32 x;
    int32 y;
    int32 width;
    int32 height;
    float score;
    double scale;
} hogpd_detection_t;

typedef struct {
    float threshold;
    int32 stride;
    float center_region_start;
    float center_region_end;
} hogpd_detect_config_t;

/* HOGPD运行配置。打包为静态库后功能开关/参数由此结构体在初始化时传入，
 * 字段默认值与历史编译宏一致(见hogpd_config_default())。0=关闭,非0=开启。 */
typedef struct {
    uint32 magic; /* 有效性标记, 由 hogpd_config_default() 填写;
                   * 未初始化/置零的结构体传入 init 时安全回退默认配置 */
    /* —— 加速后端 —— */
    int32 use_dsp_accel;               /* <- HOGPD_HOGSVM_DSP_ENABLE */
    /* —— 全图扫描 ——
     * 注：粗到细策略(PD_FULL_COARSE_TO_FINE)及其块预取模式属于编译期
     * 算法策略，打包.a时固定，不作为运行时参数。 */
    int32 fullscan_bg_filter;          /* <- PD_FULL_BG_FILTER_ENABLE */
    int32 fullscan_drop_scale_1_73;    /* <- PD_FULL_DROP_SCALE_1_73 */
    int32 fullscan_large_level_crop;   /* <- PD_FULL_LARGE_LEVEL_CROP_ENABLE */
    /* —— Tracker功能 —— */
    int32 enable_hog;                  /* <- PD_LIVE_ENABLE_HOG */
    int32 use_fullscan;                /* <- PD_LIVE_USE_FULLSCAN */
    int32 kalman_enable;               /* <- PD_LIVE_KALMAN_ENABLE */
    int32 kalman_track_verify;         /* <- PD_LIVE_KALMAN_TRACK_VERIFY */
    int32 kalman_neighbor_eval;        /* <- PD_LIVE_KALMAN_NEIGHBOR_EVAL_ENABLE */
    int32 flow_track_enable;           /* <- PD_LIVE_FLOW_TRACK_ENABLE */
    int32 flow_hog_neighbor_eval;      /* <- PD_LIVE_FLOW_HOG_NEIGHBOR_EVAL_ENABLE */
    int32 flow_hog_vertical_eval;      /* <- PD_LIVE_FLOW_HOG_VERTICAL_EVAL_ENABLE */
    int32 flow_hog_scale_eval;         /* <- PD_LIVE_FLOW_HOG_SCALE_EVAL_ENABLE */
    int32 flow_hog_unconfirmed_keep;   /* <- PD_LIVE_FLOW_HOG_UNCONFIRMED_KEEP_ENABLE */
    int32 flow_motion_reselect;        /* <- PD_LIVE_FLOW_MOTION_RESELECT_ENABLE */
    int32 flow_direction_gate;         /* <- PD_LIVE_FLOW_KALMAN_DIRECTION_GATE_ENABLE */
    int32 flow_direction_score;        /* <- PD_LIVE_FLOW_KALMAN_DIRECTION_SCORE_ENABLE */
    int32 flow_core_patch;             /* <- PD_LIVE_FLOW_CORE_PATCH_ENABLE */
    int32 flow_sad_scale;              /* <- PD_LIVE_FLOW_SAD_SCALE_ENABLE */
    int32 sad_use_dsp;                 /* SAD差分用DSPV2指令加速, 失败自动回退标量 */
    int32 sad_sample_step;             /* SAD采样步长(像素), 4; 增大省CPU降采样密度 */
    int32 sad_coarse_step;             /* SAD粗扫步长(像素), 12; 增大候选数近减半 */
    int32 sad_search_x;                /* SAD搜索窗基准(正负像素,X), 24 */
    int32 sad_search_y;                /* SAD搜索窗基准(正负像素,Y), 16 */
    float sad_accept_score;            /* SAD位移接受分, 0.50 */
    int32 sad_fb_check;                /* 前向-后向一致性校验, 拒绝不可逆的SAD误匹配 */
    int32 hog_stationary_only;         /* <- PD_LIVE_HOG_STATIONARY_ONLY */
    int32 stationary_force_scale_eval; /* <- PD_LIVE_STATIONARY_HOG_FORCE_SCALE_EVAL */
    int32 stationary_challenger;       /* <- PD_LIVE_STATIONARY_CHALLENGER_ENABLE */
    int32 contrast_norm;               /* <- PD_LIVE_CONTRAST_NORM_ENABLE */
    int32 local_contrast_norm;         /* <- PD_LIVE_LOCAL_CONTRAST_NORM_ENABLE */
    /* —— 内存Hook（NULL=库默认实现）——
     * 应用层注入后由Hook决定缓冲分配到SRAM或PSRAM等介质。 */
    hogpd_alloc_fn_t alloc;
    hogpd_free_fn_t free;
    /* —— 关键调参 —— */
    int32 interval_frames;             /* <- PD_LIVE_INTERVAL_FRAMES, 处理间隔帧数 */
    int32 keep_frames;                 /* <- PD_LIVE_KEEP_FRAMES, 检出保持帧数 */
    int32 track_drop_frames;           /* <- PD_LIVE_TRACK_DROP_FRAMES, 丢帧判定阈值 */
    int32 fullscan_stride;             /* <- PD_LIVE_FULLSCAN_STRIDE, 全扫步长(像素) */
    float track_threshold;           /* <- PD_LIVE_THRESHOLD, HOG接受阈值 */
    float fullscan_threshold;        /* <- PD_LIVE_FULLSCAN_THRESHOLD, 全扫阈值 */
    float bootstrap_threshold;       /* <- PD_LIVE_BOOTSTRAP_THRESHOLD, 自举阈值 */
    uint64 fullscan_boot_budget_us;    /* <- PD_LIVE_FULLSCAN_BOOT_BUDGET_US */
    uint64 fullscan_general_budget_us; /* <- PD_LIVE_FULLSCAN_GENERAL_BUDGET_US */
} hogpd_config_t;

typedef struct {
    int32 x;
    int32 y;
    int32 width;
    int32 height;
} hogpd_box_t;

typedef enum {
    HOGPD_TRACK_REMOVED = 0,
    HOGPD_TRACK_REINIT,
    HOGPD_TRACK_CONFIRMED,
    HOGPD_TRACK_FLOW_ONLY,
    HOGPD_TRACK_PREDICT_ONLY,
    HOGPD_TRACK_LOST
} hogpd_track_state_t;

typedef enum {
    HOGPD_DETECTION_SOURCE_NONE = 0,
    HOGPD_DETECTION_SOURCE_FLOW_BLOCK,
    HOGPD_DETECTION_SOURCE_FLOW_BLOCK_MOTION,
    HOGPD_DETECTION_SOURCE_FLOW_BOUNDARY = 4,
    HOGPD_DETECTION_SOURCE_FLOW_HOLD,
    HOGPD_DETECTION_SOURCE_FLOW_HOG,
    HOGPD_DETECTION_SOURCE_KALMAN_HOG,
    HOGPD_DETECTION_SOURCE_FULL_HOG,
    HOGPD_DETECTION_SOURCE_FULL_STALE
} hogpd_detection_source_t;

typedef enum {
    HOGPD_FLOW_DIAGNOSTIC_NONE = 0,
    HOGPD_FLOW_DIAGNOSTIC_BLOCK_TEXTURE,
    HOGPD_FLOW_DIAGNOSTIC_HOG_REJECT = 5,
    HOGPD_FLOW_DIAGNOSTIC_BACKGROUND_STATIC,
    HOGPD_FLOW_DIAGNOSTIC_BACKGROUND_MODEL,
    HOGPD_FLOW_DIAGNOSTIC_FULL_WALL,
    HOGPD_FLOW_DIAGNOSTIC_FB_MISMATCH
} hogpd_flow_diagnostic_t;

typedef enum {
    HOGPD_LOST_DIAGNOSTIC_NONE = 0,
    HOGPD_LOST_DIAGNOSTIC_UNCONFIRMED,
    HOGPD_LOST_DIAGNOSTIC_HARD,
    HOGPD_LOST_DIAGNOSTIC_SEVERE,
    HOGPD_LOST_DIAGNOSTIC_STATIONARY,
    HOGPD_LOST_DIAGNOSTIC_KALMAN,
    HOGPD_LOST_DIAGNOSTIC_RECOVERY,
    HOGPD_LOST_DIAGNOSTIC_OTHER
} hogpd_lost_diagnostic_t;

typedef enum {
    HOGPD_FULLSCAN_MODE_NONE = 0,
    HOGPD_FULLSCAN_MODE_BOOT_MID,
    HOGPD_FULLSCAN_MODE_BOOT_FAR,
    HOGPD_FULLSCAN_MODE_BOOT_NEAR,
    HOGPD_FULLSCAN_MODE_BOOT_WIDE,
    HOGPD_FULLSCAN_MODE_LOST_PREF,
    HOGPD_FULLSCAN_MODE_LOST_FAST,
    HOGPD_FULLSCAN_MODE_PREF,
    HOGPD_FULLSCAN_MODE_LOW,
    HOGPD_FULLSCAN_MODE_MID,
    HOGPD_FULLSCAN_MODE_FAST,
    HOGPD_FULLSCAN_MODE_CHALLENGER,
    HOGPD_FULLSCAN_MODE_FULL
} hogpd_fullscan_mode_t;

typedef enum {
    HOGPD_CHALLENGER_STATUS_NONE = 0,
    HOGPD_CHALLENGER_STATUS_SCANNING,
    HOGPD_CHALLENGER_STATUS_KEEP,
    HOGPD_CHALLENGER_STATUS_REJECT,
    HOGPD_CHALLENGER_STATUS_SWITCH
} hogpd_challenger_status_t;

typedef struct {
    uint32 frame_number;
    hogpd_track_state_t track_state;
    int32 hog_enabled;
    int32 detection_visible;
    int32 prediction_visible;
    int32 sad_candidate_visible;
    int32 fullscan_topk_visible;
    int32 fullscan_topk_count;
    int32 fullscan_progress_visible;
    int32 fullscan_progress_scale_q100;
    hogpd_fullscan_mode_t fullscan_progress_mode;
    uint32 fullscan_debug_serial;
    int32 fullscan_debug_cycle_index;
    int32 fullscan_debug_pref_miss_count;
    int32 fullscan_debug_bootstrap_count;
    int32 fullscan_debug_recent_track;
    int32 fullscan_debug_lost_fullscan;
    int32 fullscan_debug_attempt_count;
    int32 fullscan_debug_worker_busy;
    int32 fullscan_debug_request_pending;
    int32 sad_core_visible;
    int32 sad_core_count;
    int32 sad_core_fresh;
    int32 hog_debug_visible;
    int32 hog_position_visible;
    int32 hog_scale_visible;
    int32 hog_debug_is_kalman;
    int32 hog_debug_candidate;
    int32 hog_debug_score_q100;
    int32 hog_debug_threshold_q100;
    int32 hog_debug_eval_count;
    int32 detection_is_flow;
    int32 sad_miss_count;
    int32 sad_score_valid;
    int32 sad_raw_score_q100;
    int32 sad_fused_score_q100;
    int32 kalman_velocity_valid;
    int32 kalman_velocity_x;
    int32 kalman_velocity_y;
    int32 hog_stationary_count;
    int32 hog_stationary_ready;
    int32 flow_hog_miss_count;
    int32 lost_diagnostic_visible;
    hogpd_lost_diagnostic_t lost_diagnostic_reason;
    int32 lost_diagnostic_miss_count;
    int32 lost_diagnostic_hard_count;
    int32 lost_diagnostic_stationary_count;
    int32 lost_diagnostic_flow_score_q100;
    int32 lost_diagnostic_hog_score_q100;
    hogpd_challenger_status_t challenger_status;
    int32 challenger_zero_frames;
    int32 challenger_scores_valid;
    int32 challenger_current_score_q100;
    int32 challenger_required_score_q100;
    int32 challenger_candidate_score_q100;
    hogpd_detection_source_t detection_source;
    hogpd_flow_diagnostic_t flow_diagnostic;
    int32 alert_started;
    hogpd_box_t detection_box;
    hogpd_box_t prediction_box;
    hogpd_box_t sad_candidate_box;
    hogpd_box_t fullscan_topk_boxes[HOGPD_FULLSCAN_TOPK_MAX];
    int32 fullscan_topk_scores_q100[HOGPD_FULLSCAN_TOPK_MAX];
    hogpd_box_t fullscan_progress_box;
    hogpd_box_t sad_core_boxes[HOGPD_SAD_CORE_BOX_MAX];
    hogpd_box_t hog_position_box;
    hogpd_box_t hog_scale_box;
    hogpd_box_t search_box;
} hogpd_frame_result_t;

#ifdef __cplusplus
}
#endif

#endif
