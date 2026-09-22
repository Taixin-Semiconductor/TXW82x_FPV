#ifndef UI_HOME_HOME_VIEW_H
#define UI_HOME_HOME_VIEW_H

#include "lvgl/lvgl.h"

typedef struct {
    int year;
    int hour;
    int minute;
    int weekday;
    int month;
    int day;
    int temperature;
    const char *weather_text;
    int wifi_connected;
    int wifi_level;
    int battery_percent;
    int battery_charging;
} ui_home_view_model_t;

void ui_home_view_create(lv_obj_t *parent, const char *path);
void ui_home_view_set_time(int hour, int minute);
void ui_home_view_set_date(int weekday, int year, int month, int day);
void ui_home_view_set_weather(int temperature, const char *weather_text);
void ui_home_view_set_wifi(int connected, int level);
void ui_home_view_set_battery(int percent, int charging);
void ui_home_view_set_error(int32_t err_code, const char *err_msg);
void ui_home_view_refresh(const ui_home_view_model_t *view_model);

#endif
