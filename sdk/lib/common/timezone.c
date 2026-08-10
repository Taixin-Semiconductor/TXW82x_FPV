#include "basic_include.h"
#include "lib/common/timezone.h"


/* ============================================================
 *  内部数据结构
 * ============================================================ */

static timezone_info_t s_sys_tz = {
    .std_offset_min = 8 * 60,   // 默认东八区
    .dst_offset_min = 60,
    .is_dst_active  = 0,
    .dst_rule_type  = DST_RULE_NONE,
    .tz_name        = "CST",
    .tz_desc        = "Asia/Shanghai"
};

/* ============================================================
 *  预设时区数据库
 * ============================================================ */

typedef struct {
    const char *name;
    const char *desc;
    int32_t     offset_min;     // 标准偏移 (分钟)
    int32_t     dst_offset_min; // 夏令时额外偏移 (分钟)
    dst_rule_type_t dst_type;
    dst_fixed_rule_t dst_rule;  // 仅当 dst_type == DST_RULE_FIXED 时有效
} timezone_preset_t;

/* 夏令时规则宏定义 (用于编译期常量初始化) */
#define DST_RULE_US   { 3,  2, 0, 11, 1, 0, 2 }  /* 北美: 3月第2个周日-11月第1个周日 */
#define DST_RULE_EU   { 3,  5, 0, 10, 5, 0, 1 }  /* 欧洲: 3月最后-10月最后 */
#define DST_RULE_AUS  { 10, 1, 0, 4,  1, 0, 2 }  /* 澳洲: 10月第1个-4月第1个 */
#define DST_RULE_NZ   { 9,  5, 0, 4,  1, 0, 2 }  /* 新西兰: 9月最后-4月第1个 */

static const timezone_preset_t s_preset_db[TZ_COUNT] = {
    /* TZ_UTC */
    { "UTC",  "UTC",                  0,    0, DST_RULE_NONE, {0} },
    /* TZ_GMT */
    { "GMT",  "Greenwich Mean Time",  0,    0, DST_RULE_NONE, {0} },
    /* TZ_CET */
    { "CET",  "Central European Time", 60,   60, DST_RULE_FIXED, DST_RULE_EU },
    /* TZ_EET */
    { "EET",  "Eastern European Time", 120,  60, DST_RULE_FIXED, DST_RULE_EU },
    /* TZ_MSK */
    { "MSK",  "Moscow Time",          180,  0,  DST_RULE_NONE, {0} },
    /* TZ_GST */
    { "GST",  "Gulf Standard Time",   240,  0,  DST_RULE_NONE, {0} },
    /* TZ_PKT */
    { "PKT",  "Pakistan Time",        300,  0,  DST_RULE_NONE, {0} },
    /* TZ_BST */
    { "BST",  "Bangladesh Time",      360,  0,  DST_RULE_NONE, {0} },
    /* TZ_ICT */
    { "ICT",  "Indochina Time",       420,  0,  DST_RULE_NONE, {0} },
    /* TZ_CST */
    { "CST",  "China Standard Time",  480,  0,  DST_RULE_NONE, {0} },
    /* TZ_JST */
    { "JST",  "Japan Standard Time",  540,  0,  DST_RULE_NONE, {0} },
    /* TZ_AEST */
    { "AEST", "Australian Eastern Time",600,  60, DST_RULE_FIXED, DST_RULE_AUS },
    /* TZ_NZST */
    { "NZST", "New Zealand Time",     720,  60, DST_RULE_FIXED, DST_RULE_NZ },
    /* TZ_PST */
    { "PST",  "Pacific Standard Time",-480, 60, DST_RULE_FIXED, DST_RULE_US },
    /* TZ_MST */
    { "MST",  "Mountain Standard Time",-420,60, DST_RULE_FIXED, DST_RULE_US },
    /* TZ_CST_US */
    { "CST",  "US Central Time",      -360, 60, DST_RULE_FIXED, DST_RULE_US },
    /* TZ_EST */
    { "EST",  "US Eastern Time",      -300, 60, DST_RULE_FIXED, DST_RULE_US },
    /* TZ_AST */
    { "AST",  "Atlantic Standard Time",-240,60, DST_RULE_FIXED, DST_RULE_US },
    /* TZ_BRST */
    { "BRST", "Brazil Time",            -180, 0, DST_RULE_NONE, {0} },
    /* TZ_FNT */
    { "FNT",  "Fernando Time",        -120, 0,  DST_RULE_NONE, {0} },
};

/* ============================================================
 *  内部工具函数
 * ============================================================ */

int timezone_is_leap_year(int year)
{
    return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) ? 1 : 0;
}

static int _days_in_month(int year, int month)
{
    static const int days[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month == 2 && timezone_is_leap_year(year)) {
        return 29;
    }
    return days[month];
}

int timezone_weekday(int year, int month, int day)
{
    /* Zeller 公式计算星期几 (0=周日, 1=周一 ... 6=周六) */
    int y = year;
    int m = month;
    int d = day;

    if (m < 3) {
        m += 12;
        y -= 1;
    }
    int c = y / 100;
    y = y % 100;
    int w = (y + y / 4 + c / 4 - 2 * c + 26 * (m + 1) / 10 + d - 1) % 7;
    if (w < 0) w += 7;
    return w;
}

int timezone_nth_weekday(int year, int month, int week, int weekday)
{
    int first_day_weekday = timezone_weekday(year, month, 1);
    int days_in_month = _days_in_month(year, month);
    int first_weekday_date;
    int diff;

    /* 计算该月第1个目标星期几的日期 */
    diff = weekday - first_day_weekday;
    if (diff < 0) diff += 7;
    first_weekday_date = 1 + diff;

    if (week >= 1 && week <= 4) {
        int result = first_weekday_date + (week - 1) * 7;
        if (result > days_in_month) return -1;
        return result;
    } else if (week == 5) {
        /* 最后一个 */
        int result = first_weekday_date;
        while (result + 7 <= days_in_month) {
            result += 7;
        }
        return result;
    }
    return -1;
}

/* ============================================================
 *  夏令时计算
 * ============================================================ */

static void _utc_to_ymdhms(time_t utc_sec, int *year, int *month, int *day,
                           int *hour, int *min, int *sec)
{
    /* 简化的 UTC 时间戳转年月日时分秒 */
    /* 使用1970年1月1日作为基准 */
    int y;
    int64_t days_since_epoch = utc_sec / 86400;
    int64_t sec_of_day = utc_sec % 86400;
    if (sec_of_day < 0) {
        sec_of_day += 86400;
        days_since_epoch--;
    }

    /* 粗略计算年份 (1970 + days/365) */
    y = 1970 + (int)(days_since_epoch / 365);
    /* 精确调整 */
    while (1) {
        int64_t days_to_y = 0;
        int tmp_y;
        for (tmp_y = 1970; tmp_y < y; tmp_y++) {
            days_to_y += timezone_is_leap_year(tmp_y) ? 366 : 365;
        }
        if (days_to_y > days_since_epoch) {
            y--;
        } else {
            break;
        }
    }

    *year = y;
    /* 计算该年已过去的天数 */
    int64_t days_of_year = days_since_epoch;
    int tmp;
    for (tmp = 1970; tmp < y; tmp++) {
        days_of_year -= timezone_is_leap_year(tmp) ? 366 : 365;
    }

    /* 计算月份和日期 */
    static const int month_days[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int m;
    for (m = 1; m <= 12; m++) {
        int md = month_days[m];
        if (m == 2 && timezone_is_leap_year(y)) md = 29;
        if (days_of_year < md) {
            *month = m;
            *day = (int)days_of_year + 1;
            break;
        }
        days_of_year -= md;
    }

    *hour = (int)(sec_of_day / 3600);
    *min  = (int)((sec_of_day % 3600) / 60);
    *sec  = (int)(sec_of_day % 60);
}

static time_t _ymdhms_to_utc(int year, int month, int day, int hour, int min, int sec)
{
    /* 简化的年月日时分秒转 UTC 时间戳 */
    int y;
    int64_t days = 0;
    for (y = 1970; y < year; y++) {
        days += timezone_is_leap_year(y) ? 366 : 365;
    }
    static const int month_days[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int m;
    for (m = 1; m < month; m++) {
        days += month_days[m];
        if (m == 2 && timezone_is_leap_year(year)) days += 1;
    }
    days += day - 1;

    return (time_t)(days * 86400 + hour * 3600 + min * 60 + sec);
}

static int _check_dst_fixed(time_t utc_sec, const dst_fixed_rule_t *rule)
{
    int year, month, day, hour, min, sec;
    int start_day, end_day;
    time_t start_utc, end_utc;

    _utc_to_ymdhms(utc_sec, &year, &month, &day, &hour, &min, &sec);

    /* 计算夏令时开始时间 (UTC) */
    start_day = timezone_nth_weekday(year, rule->start_month,
                                      rule->start_week, rule->start_weekday);
    if (start_day < 0) return 0;
    /* 夏令时开始时间 = 本地时间，需要转换为UTC */
    /* 简化处理：假设标准偏移为 s_sys_tz.std_offset_min 分钟 */
    start_utc = _ymdhms_to_utc(year, rule->start_month, start_day,
                                rule->switch_hour, 0, 0);
    /* 转换为UTC: 减去标准偏移 */
    start_utc -= s_sys_tz.std_offset_min * 60;

    /* 计算夏令时结束时间 (UTC) */
    end_day = timezone_nth_weekday(year, rule->end_month,
                                    rule->end_week, rule->end_weekday);
    if (end_day < 0) return 0;
    end_utc = _ymdhms_to_utc(year, rule->end_month, end_day,
                              rule->switch_hour, 0, 0);
    end_utc -= s_sys_tz.std_offset_min * 60;

    /* 判断当前时间是否在夏令时区间内 */
    /* 注意：这里简化处理，实际应考虑DST生效后的偏移变化 */
    /* 更精确的做法是迭代计算 */
    if (utc_sec >= start_utc && utc_sec < end_utc) {
        return 1;
    }
    return 0;
}

/* ============================================================
 *  公开接口实现
 * ============================================================ */

int timezone_set_preset(timezone_preset_id_t tz_id)
{
    const timezone_preset_t *preset;

    if (tz_id < 0 || tz_id >= TZ_COUNT) {
        return -1;
    }

    preset = &s_preset_db[tz_id];

    s_sys_tz.std_offset_min = preset->offset_min;
    s_sys_tz.dst_offset_min = preset->dst_offset_min;
    s_sys_tz.is_dst_active  = 0;
    s_sys_tz.dst_rule_type  = preset->dst_type;

    if (preset->dst_type == DST_RULE_FIXED) {
        s_sys_tz.dst_rule.fixed_rule = preset->dst_rule;
    }

    strncpy(s_sys_tz.tz_name, preset->name, sizeof(s_sys_tz.tz_name) - 1);
    s_sys_tz.tz_name[sizeof(s_sys_tz.tz_name) - 1] = '\0';
    strncpy(s_sys_tz.tz_desc, preset->desc, sizeof(s_sys_tz.tz_desc) - 1);
    s_sys_tz.tz_desc[sizeof(s_sys_tz.tz_desc) - 1] = '\0';

    /* 初始化后立即计算一次夏令时状态 */
    timezone_update_dst();

    return 0;
}

int timezone_set_custom(const timezone_info_t *info)
{
    if (info == NULL) {
        return -1;
    }

    s_sys_tz = *info;
    timezone_update_dst();
    return 0;
}

int timezone_get_info(timezone_info_t *info)
{
    if (info == NULL) {
        return -1;
    }
    *info = s_sys_tz;
    return 0;
}

int32_t timezone_get_offset_sec(void)
{
    int32_t total = s_sys_tz.std_offset_min * 60;
    if (s_sys_tz.is_dst_active) {
        total += s_sys_tz.dst_offset_min * 60;
    }
    return total;
}

int32_t timezone_get_offset_min(void)
{
    int32_t total = s_sys_tz.std_offset_min;
    if (s_sys_tz.is_dst_active) {
        total += s_sys_tz.dst_offset_min;
    }
    return total;
}

uint8_t timezone_is_dst(void)
{
    return s_sys_tz.is_dst_active;
}

void timezone_set_dst_manual(uint8_t enable)
{
    if (s_sys_tz.dst_rule_type == DST_RULE_NONE) {
        s_sys_tz.is_dst_active = enable;
    }
}

void timezone_update_dst(void)
{
    time_t now;

    if (s_sys_tz.dst_rule_type == DST_RULE_NONE) {
        return; /* 无夏令时规则，不更新 */
    }

    /* 获取当前UTC时间 */
    now = time(NULL);

    if (s_sys_tz.dst_rule_type == DST_RULE_FIXED) {
        s_sys_tz.is_dst_active = _check_dst_fixed(now, &s_sys_tz.dst_rule.fixed_rule);
    }
    /* DST_RULE_CUSTOM 可扩展 */
}

int timezone_utc_to_local(time_t utc_sec, time_t *local_sec)
{
    if (local_sec == NULL) {
        return -1;
    }
    *local_sec = utc_sec + timezone_get_offset_sec();
    return 0;
}

int timezone_local_to_utc(time_t local_sec, time_t *utc_sec)
{
    if (utc_sec == NULL) {
        return -1;
    }
    *utc_sec = local_sec - timezone_get_offset_sec();
    return 0;
}

time_t get_local_time(void)
{
    time_t utc = time(NULL);
    time_t local;
    timezone_utc_to_local(utc, &local);
    return local;
}

void localtime_tz(time_t utc, struct tm *t)
{
    time_t local;
    timezone_utc_to_local(utc, &local);
    gmtime_r(&local, t);
    t->tm_year += 1900;
    t->tm_mon  += 1;
}

