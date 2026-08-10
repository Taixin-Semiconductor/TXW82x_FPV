#ifndef _TXSDK_TIMEZONE_H
#define _TXSDK_TIMEZONE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 时区模块版本信息
 */
#define TIMEZONE_LIB_VERSION    "2.0.0"

/**
 * @brief 夏令时规则类型
 */
typedef enum {
    DST_RULE_NONE,          // 无夏令时
    DST_RULE_FIXED,         // 固定日期规则 (如: 3月第2个周日 - 11月第1个周日)
    DST_RULE_CUSTOM         // 自定义规则 (通过回调函数判断)
} dst_rule_type_t;

/**
 * @brief 月份枚举 (1-12)
 */
typedef enum {
    MONTH_JAN = 1, MONTH_FEB, MONTH_MAR, MONTH_APR,
    MONTH_MAY, MONTH_JUN, MONTH_JUL, MONTH_AUG,
    MONTH_SEP, MONTH_OCT, MONTH_NOV, MONTH_DEC
} month_t;

/**
 * @brief 星期枚举 (0=周日, 1=周一 ... 6=周六)
 */
typedef enum {
    WEEKDAY_SUN = 0, WEEKDAY_MON, WEEKDAY_TUE, WEEKDAY_WED,
    WEEKDAY_THU, WEEKDAY_FRI, WEEKDAY_SAT
} weekday_t;

/**
 * @brief 固定日期夏令时规则
 * @note 适用于大多数国家的夏令时规则：某月第N个星期几开始/结束
 */
typedef struct {
    uint8_t start_month;        // 开始月份 (1-12)
    uint8_t start_week;         // 开始月份中的第几个星期 (1-5, 5=最后一个)
    uint8_t start_weekday;      // 开始星期几 (0=周日)
    uint8_t end_month;          // 结束月份 (1-12)
    uint8_t end_week;           // 结束月份中的第几个星期 (1-5, 5=最后一个)
    uint8_t end_weekday;        // 结束星期几 (0=周日)
    uint8_t switch_hour;        // 切换小时 (通常为 2 或 3)
} dst_fixed_rule_t;

/**
 * @brief 时区信息结构体
 */
typedef struct {
    int32_t  std_offset_min;    // 标准时间相对于UTC的偏移（分钟），例如东八区 = 8*60 = 480
    int32_t  dst_offset_min;    // 夏令时额外偏移（分钟），通常为 60
    uint8_t  is_dst_active;     // 当前是否处于夏令时 (内部维护，不建议直接修改)

    dst_rule_type_t dst_rule_type;
    union {
        dst_fixed_rule_t fixed_rule;    // 固定日期规则
        // 可扩展: 自定义规则回调等
    } dst_rule;

    char     tz_name[16];       // 时区名称，如 "CST", "EDT"
    char     tz_desc[32];       // 时区描述，如 "Asia/Shanghai"
} timezone_info_t;

/**
 * @brief 预设时区ID (覆盖全球主要时区)
 */
typedef enum {
    TZ_UTC,         // UTC
    TZ_GMT,         // GMT (与UTC相同)
    TZ_CET,         // 中欧时间 (UTC+1)
    TZ_EET,         // 东欧时间 (UTC+2)
    TZ_MSK,         // 莫斯科时间 (UTC+3)
    TZ_GST,         // 海湾标准时间 (UTC+4)
    TZ_PKT,         // 巴基斯坦时间 (UTC+5)
    TZ_BST,         // 孟加拉时间 (UTC+6)
    TZ_ICT,         // 中南半岛时间 (UTC+7)
    TZ_CST,         // 中国标准时间 (UTC+8)
    TZ_JST,         // 日本标准时间 (UTC+9)
    TZ_AEST,        // 澳大利亚东部时间 (UTC+10)
    TZ_NZST,        // 新西兰标准时间 (UTC+12)
    TZ_PST,         // 太平洋标准时间 (UTC-8)
    TZ_MST,         // 山地标准时间 (UTC-7)
    TZ_CST_US,      // 美国中部时间 (UTC-6)
    TZ_EST,         // 美国东部时间 (UTC-5)
    TZ_AST,         // 大西洋标准时间 (UTC-4)
    TZ_BRST,        // 巴西时间 (UTC-3)
    TZ_FNT,         // 费尔南多时间 (UTC-2)
    TZ_COUNT        // 时区数量
} timezone_preset_id_t;

/* ============================================================
 *  基础时区操作接口
 * ============================================================ */

/**
 * @brief 通过预设ID设置时区
 * @param tz_id 预设时区ID
 * @return 0: 成功, -1: 无效ID
 * @note 自动配置对应时区的标准偏移和夏令时规则
 */
int timezone_set_preset(timezone_preset_id_t tz_id);

/**
 * @brief 通过自定义规则设置时区
 * @param info 时区信息结构体指针
 * @return 0: 成功, -1: 参数错误
 */
int timezone_set_custom(const timezone_info_t *info);

/**
 * @brief 获取当前时区信息
 * @param info 输出参数，存储当前时区信息
 * @return 0: 成功, -1: 参数错误
 */
int timezone_get_info(timezone_info_t *info);

/**
 * @brief 获取当前时区总偏移（秒）
 * @return 总偏移秒数 (标准偏移 + 夏令时偏移)
 */
int32_t timezone_get_offset_sec(void);

/**
 * @brief 获取当前时区总偏移（分钟）
 * @return 总偏移分钟数
 */
int32_t timezone_get_offset_min(void);

/**
 * @brief 判断当前是否处于夏令时
 * @return 1: 是, 0: 否
 */
uint8_t timezone_is_dst(void);

/**
 * @brief 手动设置夏令时状态
 * @param enable 1: 启用, 0: 禁用
 * @note 仅在 dst_rule_type == DST_RULE_NONE 时有效，用于手动控制
 */
void timezone_set_dst_manual(uint8_t enable);

/**
 * @brief 更新夏令时状态 (根据当前时间和规则自动计算)
 * @note 应在系统定时器或主循环中定期调用（如每小时一次）
 */
void timezone_update_dst(void);

/* ============================================================
 *  时间转换接口
 * ============================================================ */

/**
 * @brief UTC时间转换为本地时间 (考虑时区偏移)
 * @param utc_sec   UTC时间戳 (秒)
 * @param local_sec 输出参数，本地时间戳
 * @return 0: 成功
 */
int timezone_utc_to_local(time_t utc_sec, time_t *local_sec);

/**
 * @brief 本地时间转换为UTC时间
 * @param local_sec 本地时间戳 (秒)
 * @param utc_sec   输出参数，UTC时间戳
 * @return 0: 成功
 */
int timezone_local_to_utc(time_t local_sec, time_t *utc_sec);

/**
 * @brief 获取当前本地时间（考虑时区偏移和夏令时）
 * @return 本地时间戳 (秒)
 * @note 内部调用 time(NULL) 获取UTC，再转换为本地时间
 */
time_t get_local_time(void);

/**
 * @brief 将UTC时间转换为本地时间的 struct tm 结构
 * @param utc UTC时间戳
 * @param t struct tm 的指针，存储转换后的时间信息
 */
void localtime_tz(time_t utc, struct tm *t);

/* ============================================================
 *  工具函数
 * ============================================================ */

/**
 * @brief 获取指定年月日的星期几
 * @param year  年份 (如 2024)
 * @param month 月份 (1-12)
 * @param day   日期 (1-31)
 * @return 星期几 (0=周日, 1=周一 ... 6=周六)
 */
int timezone_weekday(int year, int month, int day);

/**
 * @brief 获取指定年月第N个星期几的日期
 * @param year     年份
 * @param month    月份 (1-12)
 * @param week     第几个星期 (1-5, 5=最后一个)
 * @param weekday  星期几 (0=周日)
 * @return 日期 (1-31), 失败返回 -1
 */
int timezone_nth_weekday(int year, int month, int week, int weekday);

/**
 * @brief 判断某年是否为闰年
 * @param year 年份
 * @return 1: 是, 0: 否
 */
int timezone_is_leap_year(int year);

#ifdef __cplusplus
}
#endif

#endif // _TXSDK_TIMEZONE_H
