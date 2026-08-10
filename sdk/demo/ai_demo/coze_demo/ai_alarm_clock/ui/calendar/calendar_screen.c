#include "ui_manager.h"

#include "screen_manager.h"
#include "calendar_screen.h"

#include "../ui_theme.h"
#include "osal/string.h"

typedef struct
{
    screen_t *screen;
    lv_obj_t *detail_page;
    lv_obj_t *calendar;
    lv_obj_t *month_label;
    lv_obj_t *picker_mask;
    lv_obj_t *year_roller;
    lv_obj_t *month_roller;
    const char *day_names[7];
    int today_year;
    int today_month;
    int today_day;
    int showed_year;
    int showed_month;
} ui_calendar_state_t;

static void ui_calendar_create_detail(screen_t *screen, void *params);
static void ui_calendar_destroy_detail(screen_t *screen);
static void ui_calendar_get_lifecycle(screen_lifecycle_t *lifecycle);
static void ui_calendar_push_new_screen(void);
static void ui_calendar_on_entry_click(lv_event_t *event);
static void ui_calendar_on_back_click(lv_event_t *event);
static void ui_calendar_on_month_button_click(lv_event_t *event);
static void ui_calendar_on_picker_mask_click(lv_event_t *event);
static void ui_calendar_on_picker_cancel_click(lv_event_t *event);
static void ui_calendar_on_picker_confirm_click(lv_event_t *event);

static void ui_calendar_update_month_label(ui_calendar_state_t *state)
{
    char text_buffer[24];

    if(state == NULL || state->month_label == NULL) {
        return;
    }

    lv_snprintf(text_buffer, sizeof(text_buffer), "%04d.%02d %s",
                state->showed_year,
                state->showed_month,
                LV_SYMBOL_DOWN);
    lv_label_set_text(state->month_label, text_buffer);
}

static void ui_calendar_apply_showed_date(ui_calendar_state_t *state, int year, int month)
{
    if(state == NULL || state->calendar == NULL) {
        return;
    }

    if(month < 1) {
        month = 1;
    } else if(month > 12) {
        month = 12;
    }

    state->showed_year = year;
    state->showed_month = month;
    lv_calendar_set_showed_date(state->calendar, (uint32_t)year, (uint32_t)month);
    ui_calendar_update_month_label(state);
}

static void ui_calendar_sync_picker_to_showed_date(ui_calendar_state_t *state)
{
    if(state == NULL || state->year_roller == NULL || state->month_roller == NULL) {
        return;
    }

    lv_roller_set_selected(state->year_roller, state->showed_year - 2000, LV_ANIM_OFF);
    lv_roller_set_selected(state->month_roller, state->showed_month - 1, LV_ANIM_OFF);
}

static void ui_calendar_close_picker(ui_calendar_state_t *state)
{
    if(state != NULL && state->picker_mask != NULL) {
        lv_obj_add_flag(state->picker_mask, LV_OBJ_FLAG_HIDDEN);
    }
}

static lv_obj_t *ui_calendar_create_roller(lv_obj_t *parent,
                                           const char *options,
                                           lv_coord_t width,
                                           lv_align_t align,
                                           lv_coord_t x_ofs,
                                           lv_coord_t y_ofs)
{
    lv_obj_t *roller;

    roller = lv_roller_create(parent);
    if(roller == NULL) {
        return NULL;
    }

    lv_roller_set_options(roller, options, LV_ROLLER_MODE_NORMAL);
    lv_obj_set_size(roller, width, 88);
    lv_obj_align(roller, align, x_ofs, y_ofs);
    lv_obj_set_style_bg_color(roller, lv_color_hex(0xe5e7eb), 0);
    lv_obj_set_style_text_color(roller, lv_color_hex(0x48433c), 0);
    lv_obj_set_style_text_font(roller, UI_FONT_BODY, 0);
    lv_obj_set_style_text_align(roller, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_radius(roller, 14, 0);
    lv_obj_set_style_border_width(roller, 0, 0);
    lv_obj_set_style_shadow_width(roller, 0, 0);
    lv_obj_set_style_bg_color(roller, lv_color_hex(UI_COLOR_BG_CARD), LV_PART_SELECTED);
    lv_obj_set_style_text_color(roller, lv_color_hex(0x222222), LV_PART_SELECTED);

    return roller;
}

static void ui_calendar_create_calendar_widget(lv_obj_t *parent, ui_calendar_state_t *state)
{
    lv_obj_t *calendar_btnmatrix;

    if(state == NULL) {
        return;
    }

    state->calendar = lv_calendar_create(parent);
    if(state->calendar == NULL) {
        return;
    }

    lv_obj_set_size(state->calendar, 308, 198);
    lv_obj_align(state->calendar, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_radius(state->calendar, 18, 0);
    lv_obj_set_style_bg_color(state->calendar, lv_color_hex(UI_COLOR_BG_LIGHT), 0);
    lv_obj_set_style_bg_opa(state->calendar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(state->calendar, 0, 0);
    lv_obj_set_style_pad_all(state->calendar, 10, 0);
    lv_calendar_set_day_names(state->calendar, state->day_names);

    calendar_btnmatrix = lv_calendar_get_btnmatrix(state->calendar);
    if(calendar_btnmatrix != NULL) {
        lv_obj_set_style_border_width(calendar_btnmatrix, 0, 0);
        lv_obj_set_style_bg_opa(calendar_btnmatrix, LV_OPA_TRANSP, 0);
        lv_obj_set_style_text_font(calendar_btnmatrix, UI_FONT_BODY, 0);
        lv_obj_set_style_text_color(calendar_btnmatrix, lv_color_hex(0x243042), 0);
        lv_obj_set_style_pad_row(calendar_btnmatrix, 4, 0);
        lv_obj_set_style_pad_column(calendar_btnmatrix, 1, 0);
        lv_obj_set_style_bg_opa(calendar_btnmatrix, LV_OPA_TRANSP, LV_PART_ITEMS);
        lv_obj_set_style_radius(calendar_btnmatrix, 12, LV_PART_ITEMS);
        lv_obj_set_style_bg_color(calendar_btnmatrix, lv_color_hex(0xd9e8ff), LV_PART_ITEMS | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(calendar_btnmatrix, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(calendar_btnmatrix, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_ITEMS | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(calendar_btnmatrix, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(calendar_btnmatrix, lv_color_hex(UI_COLOR_PRIMARY_DARK), LV_PART_ITEMS | LV_STATE_CHECKED);
    }

    lv_calendar_set_today_date(state->calendar,
                               (uint32_t)state->today_year,
                               (uint32_t)state->today_month,
                               (uint32_t)state->today_day);
    ui_calendar_apply_showed_date(state, state->showed_year, state->showed_month);
}

static void ui_calendar_create_picker(lv_obj_t *parent, ui_calendar_state_t *state)
{
    const char *year_options =
        "2000\n2001\n2002\n2003\n2004\n2005\n2006\n2007\n2008\n2009\n"
        "2010\n2011\n2012\n2013\n2014\n2015\n2016\n2017\n2018\n2019\n"
        "2020\n2021\n2022\n2023\n2024\n2025\n2026\n2027\n2028\n2029\n"
        "2030\n2031\n2032\n2033\n2034\n2035\n2036\n2037\n2038\n2039\n"
        "2040\n2041\n2042\n2043\n2044\n2045\n2046\n2047\n2048\n2049\n"
        "2050";
    const char *month_options =
        "01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12";
    lv_obj_t *picker_panel;
    lv_obj_t *panel_title;
    lv_obj_t *cancel_button;
    lv_obj_t *confirm_button;
    lv_obj_t *cancel_label;
    lv_obj_t *confirm_label;

    if(state == NULL) {
        return;
    }

    state->picker_mask = lv_obj_create(parent);
    if(state->picker_mask == NULL) {
        return;
    }

    lv_obj_remove_style_all(state->picker_mask);
    lv_obj_set_size(state->picker_mask, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(state->picker_mask, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(state->picker_mask, LV_OPA_50, 0);
    lv_obj_add_flag(state->picker_mask, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(state->picker_mask, ui_calendar_on_picker_mask_click, LV_EVENT_CLICKED, state);

    picker_panel = lv_obj_create(state->picker_mask);
    if(picker_panel == NULL) {
        return;
    }

    lv_obj_set_size(picker_panel, 264, 188);
    lv_obj_center(picker_panel);
    lv_obj_set_style_radius(picker_panel, 20, 0);
    lv_obj_set_style_bg_color(picker_panel, lv_color_hex(0xf0f1f4), 0);
    lv_obj_set_style_bg_opa(picker_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(picker_panel, 0, 0);
    lv_obj_set_style_pad_all(picker_panel, 14, 0);
    lv_obj_clear_flag(picker_panel, LV_OBJ_FLAG_SCROLLABLE);

    panel_title = lv_label_create(picker_panel);
    if(panel_title != NULL) {
        lv_label_set_text(panel_title, "Select Year / Month");
        lv_obj_set_style_text_font(panel_title, UI_FONT_BODY, 0);
        lv_obj_set_style_text_color(panel_title, lv_color_hex(0x36322b), 0);
        lv_obj_align(panel_title, LV_ALIGN_TOP_MID, 0, 4);
    }

    state->year_roller = ui_calendar_create_roller(picker_panel, year_options, 112,
                                                   LV_ALIGN_TOP_LEFT, 0, 34);
    state->month_roller = ui_calendar_create_roller(picker_panel, month_options, 96,
                                                    LV_ALIGN_TOP_RIGHT, 0, 34);

    cancel_button = lv_btn_create(picker_panel);
    if(cancel_button != NULL) {
        lv_obj_set_size(cancel_button, 108, 36);
        lv_obj_align(cancel_button, LV_ALIGN_BOTTOM_LEFT, 0, 0);
        lv_obj_set_style_radius(cancel_button, 18, 0);
        lv_obj_set_style_bg_color(cancel_button, lv_color_hex(0xe1e3e8), 0);
        lv_obj_set_style_bg_opa(cancel_button, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(cancel_button, 0, 0);
        lv_obj_set_style_shadow_width(cancel_button, 0, 0);
        lv_obj_add_event_cb(cancel_button, ui_calendar_on_picker_cancel_click, LV_EVENT_CLICKED, state);

        cancel_label = lv_label_create(cancel_button);
        if(cancel_label != NULL) {
            lv_label_set_text(cancel_label, "Cancel");
            lv_obj_set_style_text_font(cancel_label, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(cancel_label, lv_color_hex(0x4d4a45), 0);
            lv_obj_center(cancel_label);
        }
    }

    confirm_button = lv_btn_create(picker_panel);
    if(confirm_button != NULL) {
        lv_obj_set_size(confirm_button, 108, 36);
        lv_obj_align(confirm_button, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
        lv_obj_set_style_radius(confirm_button, 18, 0);
        lv_obj_set_style_bg_color(confirm_button, lv_color_hex(UI_COLOR_PRIMARY), 0);
        lv_obj_set_style_bg_opa(confirm_button, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(confirm_button, 0, 0);
        lv_obj_set_style_shadow_width(confirm_button, 0, 0);
        lv_obj_add_event_cb(confirm_button, ui_calendar_on_picker_confirm_click, LV_EVENT_CLICKED, state);

        confirm_label = lv_label_create(confirm_button);
        if(confirm_label != NULL) {
            lv_label_set_text(confirm_label, "Confirm");
            lv_obj_set_style_text_font(confirm_label, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(confirm_label, lv_color_hex(UI_COLOR_PRIMARY_DARK), 0);
            lv_obj_center(confirm_label);
        }
    }

    ui_calendar_sync_picker_to_showed_date(state);
}

static void ui_calendar_on_back_click(lv_event_t *event)
{
    ui_calendar_state_t *state = (ui_calendar_state_t *)lv_event_get_user_data(event);

    if(state == NULL || state->screen == NULL) {
        return;
    }

    screen_finish(state->screen);
}

static void ui_calendar_on_month_button_click(lv_event_t *event)
{
    ui_calendar_state_t *state = (ui_calendar_state_t *)lv_event_get_user_data(event);

    ui_calendar_sync_picker_to_showed_date(state);
    if(state != NULL && state->picker_mask != NULL) {
        lv_obj_clear_flag(state->picker_mask, LV_OBJ_FLAG_HIDDEN);
    }
}

static void ui_calendar_on_picker_mask_click(lv_event_t *event)
{
    ui_calendar_state_t *state = (ui_calendar_state_t *)lv_event_get_user_data(event);

    if(state != NULL && lv_event_get_target(event) == state->picker_mask) {
        ui_calendar_close_picker(state);
    }
}

static void ui_calendar_on_picker_cancel_click(lv_event_t *event)
{
    ui_calendar_state_t *state = (ui_calendar_state_t *)lv_event_get_user_data(event);

    ui_calendar_sync_picker_to_showed_date(state);
    ui_calendar_close_picker(state);
}

static void ui_calendar_on_picker_confirm_click(lv_event_t *event)
{
    ui_calendar_state_t *state = (ui_calendar_state_t *)lv_event_get_user_data(event);

    if(state == NULL || state->year_roller == NULL || state->month_roller == NULL) {
        return;
    }

    ui_calendar_apply_showed_date(state,
                                  2000 + lv_roller_get_selected(state->year_roller),
                                  1 + lv_roller_get_selected(state->month_roller));
    ui_calendar_close_picker(state);
}

static void ui_calendar_create_detail(screen_t *screen, void *params)
{
    ui_calendar_state_t *state;
    lv_obj_t *page;
    lv_obj_t *header_panel;
    lv_obj_t *month_button;
    lv_obj_t *back_btn;
    lv_obj_t *title_label;

    (void)params;

    if(screen == NULL) {
        return;
    }

    state = (ui_calendar_state_t *)os_malloc(sizeof(ui_calendar_state_t));
    if(state == NULL) {
        return;
    }
    lv_memset(state, 0, sizeof(ui_calendar_state_t));
    state->screen = screen;
    state->day_names[0] = "Sun";
    state->day_names[1] = "Mon";
    state->day_names[2] = "Tue";
    state->day_names[3] = "Wed";
    state->day_names[4] = "Thu";
    state->day_names[5] = "Fri";
    state->day_names[6] = "Sat";
    screen_set_user_data(screen, state);

    lv_obj_remove_style_all(screen->root);
    lv_obj_set_size(screen->root, LV_PCT(100), LV_PCT(100));
    lv_obj_center(screen->root);
    lv_obj_clear_flag(screen->root, LV_OBJ_FLAG_SCROLLABLE);

    page = lv_obj_create(screen->root);
    if(page == NULL) {
        return;
    }

    lv_obj_remove_style_all(page);
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
    lv_obj_center(page);
    lv_obj_set_style_bg_color(page, lv_color_hex(UI_COLOR_BLUE), 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    state->detail_page = page;

    state->today_year = 2025;
    state->today_month = 6;
    state->today_day = 15;
    state->showed_year = 2025;
    state->showed_month = 6;

    header_panel = lv_obj_create(page);
    if(header_panel != NULL) {
        lv_obj_remove_style_all(header_panel);
        lv_obj_set_size(header_panel, LV_PCT(96), UI_HEADER_HEIGHT);
        lv_obj_set_style_radius(header_panel, UI_HEADER_RADIUS, 0);
        lv_obj_align(header_panel, LV_ALIGN_TOP_MID, 0, 4);
        lv_obj_set_style_bg_color(header_panel, lv_color_hex(UI_COLOR_PRIMARY), 0);
        lv_obj_set_style_bg_opa(header_panel, LV_OPA_COVER, 0);
        lv_obj_clear_flag(header_panel, LV_OBJ_FLAG_SCROLLABLE);

        back_btn = lv_obj_create(header_panel);
        if(back_btn != NULL) {
            lv_obj_remove_style_all(back_btn);
            lv_obj_set_size(back_btn, 150, 150);
            lv_obj_set_style_radius(back_btn, 15, 0);
            lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(back_btn, 0, 0);
            lv_obj_set_style_shadow_width(back_btn, 0, 0);
            lv_obj_add_flag(back_btn, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 6, 0);
            lv_obj_add_event_cb(back_btn, ui_calendar_on_back_click, LV_EVENT_CLICKED, state);

            lv_obj_t *back_label = lv_label_create(back_btn);
            if(back_label != NULL) {
                lv_label_set_text(back_label, LV_SYMBOL_LEFT);
                lv_obj_set_style_text_font(back_label, UI_FONT_BODY, 0);
                lv_obj_set_style_text_color(back_label, lv_color_hex(UI_COLOR_PRIMARY_DARK), 0);
                lv_obj_center(back_label);
            }
        }

        title_label = lv_label_create(header_panel);
        if(title_label != NULL) {
            lv_label_set_text(title_label, "Calendar");
            lv_obj_set_style_text_font(title_label, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(title_label, lv_color_hex(UI_COLOR_PRIMARY_DARK), 0);
            lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);
        }

        month_button = lv_btn_create(header_panel);
        if(month_button != NULL) {
            lv_obj_set_size(month_button, 96, 28);
            lv_obj_set_style_radius(month_button, 0, 0);
            lv_obj_set_style_bg_opa(month_button, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(month_button, 0, 0);
            lv_obj_set_style_shadow_width(month_button, 0, 0);
            lv_obj_set_style_pad_all(month_button, 0, 0);
            lv_obj_align(month_button, LV_ALIGN_RIGHT_MID, -10, 0);
            lv_obj_add_event_cb(month_button, ui_calendar_on_month_button_click, LV_EVENT_CLICKED, state);

            state->month_label = lv_label_create(month_button);
            if(state->month_label != NULL) {
                lv_obj_set_style_text_font(state->month_label, UI_FONT_BODY, 0);
                lv_obj_set_style_text_color(state->month_label, lv_color_hex(0x403728), 0);
                lv_obj_center(state->month_label);
            }
        }
    }

    ui_calendar_create_calendar_widget(page, state);
    ui_calendar_create_picker(page, state);
}

static void ui_calendar_destroy_detail(screen_t *screen)
{
    ui_calendar_state_t *state;

    if(screen == NULL) {
        return;
    }

    state = (ui_calendar_state_t *)screen_get_user_data(screen);
    if(state == NULL) {
        return;
    }

    state->calendar = NULL;
    state->month_label = NULL;
    state->picker_mask = NULL;
    state->year_roller = NULL;
    state->month_roller = NULL;
    state->detail_page = NULL;
    state->screen = NULL;
    screen_set_user_data(screen, NULL);
    os_free(state);
}

static void ui_calendar_screen_start(screen_t *screen)
{
    (void)screen;
}

static void ui_calendar_screen_resume(screen_t *screen)
{
    (void)screen;
}

static void ui_calendar_screen_pause(screen_t *screen)
{
    (void)screen;
}

static void ui_calendar_screen_stop(screen_t *screen)
{
    (void)screen;
}

static void ui_calendar_screen_restart(screen_t *screen)
{
    (void)screen;
}

static void ui_calendar_screen_finish(screen_t *screen)
{
    (void)screen;
}

static void ui_calendar_get_lifecycle(screen_lifecycle_t *lifecycle)
{
    if(lifecycle == NULL) {
        return;
    }

    *lifecycle = (screen_lifecycle_t) {
        .on_create = ui_calendar_create_detail,
        .on_start = ui_calendar_screen_start,
        .on_resume = ui_calendar_screen_resume,
        .on_pause = ui_calendar_screen_pause,
        .on_stop = ui_calendar_screen_stop,
        .on_destroy = ui_calendar_destroy_detail,
        .on_restart = ui_calendar_screen_restart,
        .on_finish = ui_calendar_screen_finish,
    };
}

static void ui_calendar_push_new_screen(void)
{
    screen_lifecycle_t lifecycle;
    screen_t *screen;

    ui_calendar_get_lifecycle(&lifecycle);
    screen = screen_register(NULL, &lifecycle);
    if(screen != NULL) {
        screen_push(screen, NULL);
    }
}

static void ui_calendar_on_entry_click(lv_event_t *event)
{
    (void)event;

    ui_calendar_push_new_screen();
}

void ui_calendar_screen_create(lv_obj_t *screen, const char *path)
{
    if(screen == NULL) {
        return;
    }

    ui_image_show(screen, path);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, ui_calendar_on_entry_click, LV_EVENT_CLICKED, NULL);
}
